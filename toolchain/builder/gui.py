"""The window: a profile list, the selected profile (what to build, with which
of that target's compiler options, where the ELF goes) and the build log.
What a target needs and does not have yet is offered in the setup dialog,
which downloads it.  Builds, detection and downloads run in threads and talk
to the window through a queue."""
import os
import queue
import re
import shutil
import subprocess
import sys
import threading

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from tkinter import font as tkfont

from . import APP_NAME, NO_WINDOW, REPO_DIR, SDK_DIR, TOOLCHAIN_DIR, path_warning
from .config import PROFILES_FILE, Profile, load_profiles, load_settings, save_profiles, save_settings
from .download import InstallContext, install_all, provide_game_file
from .registry import Registry
from .runner import PROGRESS, BuildJob, existing_output, resolve_output

COLORS = {"ok": "#1b7f35", "error": "#c62828", "warn": "#a35a00", "dim": "#6b7280",
          "stage": "#1f4e9a", "note": "#455a64", "check": "#6b7280"}
SOURCE_REF = re.compile(r"(?P<path>(?:[A-Za-z]:)?[\w./\\-]+\.(?:cpp|cxx|cc|c|hpp|h|S|s|ld|inc))(?![\w])"
                        r"(?::(?P<line>\d+))?(?::(?P<col>\d+))?")


def open_path(path, select=False):
    """Show a file or folder in Explorer (or the platform's file manager)."""
    if os.name == "nt":
        if select and os.path.isfile(path):
            subprocess.Popen(["explorer", "/select,", os.path.normpath(path)])
        else:
            os.startfile(path if os.path.isdir(path) else os.path.dirname(path))
    else:
        subprocess.Popen(["xdg-open", path if os.path.isdir(path) else os.path.dirname(path)])


def megabytes(n):
    return "about %d MB" % max(1, round(n / (1 << 20))) if n else ""


# Every file dialog offers these, whatever is being built.  Windows matches
# without case; elsewhere the upper-case forms are listed too.
FILE_TYPES = ([("ELF / PlayStation 2 Executable", "*.elf"), ("ISO disc images", "*.iso"), ("All files", "*")]
              if os.name == "nt" else
              [("ELF / PlayStation 2 Executable", "*.elf *.ELF"), ("ISO disc images", "*.iso *.ISO"),
               ("All files", "*")])


class SetupDialog(tk.Toplevel):
    """Gets what an executable needs before it can be built.  A dropdown picks
    the executable; the dialog lists the SDKs any of its compiler options
    lacks (the ticked ones are downloaded) and the files the build needs from
    the user's own copy of the game.  Given sdk_ids instead, it offers just
    those SDKs."""

    def __init__(self, app, target_id=None, sdk_ids=None):
        super().__init__(app.root)
        self.app = app
        self.fixed_sdks = sdk_ids
        self.target = None
        self.title("Set up")
        self.transient(app.root)
        self.resizable(True, False)
        self.events = queue.Queue()
        self.cancel = threading.Event()
        self.running = False
        self.sdk_ids = []
        self.checks = {}
        self.game_rows = {}
        px = app.px
        frame = ttk.Frame(self, padding=px(14))
        frame.pack(fill="both", expand=True)
        frame.columnconfigure(0, weight=1)
        self.c_target = None
        if sdk_ids is None:
            head = ttk.Frame(frame)
            head.grid(row=0, column=0, sticky="ew")
            head.columnconfigure(1, weight=1)
            ttk.Label(head, text="What to build").grid(row=0, column=0, sticky="w", padx=(0, px(10)))
            self.v_target = tk.StringVar()
            self.c_target = ttk.Combobox(head, textvariable=self.v_target, state="readonly",
                                         values=app.target_names())
            self.c_target.grid(row=0, column=1, sticky="ew")
            self.c_target.bind("<<ComboboxSelected>>", lambda e: self.on_target())
        self.intro = ttk.Label(frame, text="", wraplength=px(620), justify="left")
        self.intro.grid(row=1, column=0, sticky="w", pady=(px(10), px(4)))
        self.body = ttk.Frame(frame)
        self.body.grid(row=2, column=0, sticky="ew")
        self.body.columnconfigure(0, weight=1)
        self.progress = ttk.Progressbar(frame, mode="determinate")
        self.progress.grid(row=3, column=0, sticky="ew", pady=(px(14), px(2)))
        self.state_label = ttk.Label(frame, text="", wraplength=px(620))
        self.state_label.grid(row=4, column=0, sticky="w")
        self.log = tk.Text(frame, height=6, wrap="word", font=("Consolas", 9), relief="flat",
                           background="#fcfcfc", padx=px(6), pady=px(4))
        self.log.grid(row=5, column=0, sticky="nsew", pady=(px(6), 0))
        bar = ttk.Frame(frame)
        bar.grid(row=6, column=0, sticky="e", pady=(px(10), 0))
        self.btn_start = ttk.Button(bar, text="Download", style="Build.TButton", command=self.start)
        self.btn_start.pack(side="left", padx=px(4))
        self.btn_close = ttk.Button(bar, text="Close", command=self.close)
        self.btn_close.pack(side="left")
        self.protocol("WM_DELETE_WINDOW", self.close)
        self.bind("<Escape>", lambda e: self.close())
        if sdk_ids is None:
            targets = sorted(app.registry.targets.values(), key=lambda t: t.name)
            t = app.registry.targets.get(target_id) or (targets[0] if targets else None)
            self.v_target.set(t.name if t else "")
            self.target = t
        self.refresh()
        self.after(50, self.poll)

    # ---- what is listed

    def on_target(self):
        t = self.app.target_by_name(self.v_target.get())
        if self.running:                    # one download at a time
            self.v_target.set(self.target.name)
            return
        if t is not self.target:
            self.target = t
            self.state_label.configure(text="", foreground="")
            self.progress.configure(value=0)
            self.refresh()

    def missing_sdks(self):
        """[(SDK id, [option labels that need it])] not installed yet."""
        status, reg = self.app.status, self.app.registry
        if self.fixed_sdks is not None:
            return [(sid, []) for sid in self.fixed_sdks if not (status.get(sid) and status[sid].ready)]
        uses = {}
        for o in self.target.options:
            for sid in reg.needs(self.target, o.id):
                if not (status.get(sid) and status[sid].ready):
                    uses.setdefault(sid, []).append(o.label)
        return list(uses.items())

    def refresh(self):
        """Rebuild the list for the chosen executable (or the given SDKs)."""
        for w in self.body.winfo_children():
            w.destroy()
        self.checks, self.game_rows, self.sdk_ids = {}, {}, []
        app, px = self.app, self.app.px
        if self.fixed_sdks is None and self.target is None:
            self.intro.configure(text="There is nothing to set up: no executable plug-ins were found.")
            self.btn_start.pack_forget()
            return
        if not app.status:
            self.intro.configure(text="Looking for what is installed...")
            self.btn_start.pack_forget()
            return
        missing = self.missing_sdks()
        games = self.target.game_files if self.fixed_sdks is None else []
        warning = "\n\n" + path_warning() if path_warning() else ""
        if not missing and all(g.present() for g in games):
            self.intro.configure(foreground=COLORS["ok"], text="Everything is installed." if self.fixed_sdks
                                 is not None else "%s has everything it needs." % self.target.name)
        elif self.fixed_sdks is not None:
            self.intro.configure(foreground="", text="Download gets the ticked items." + warning)
        else:
            self.intro.configure(foreground="", text=(
                "%s needs what is listed below. Download gets the ticked items; a game file comes from "
                "your own copy of the game." % self.target.name) + warning)
        row = 0
        if missing:
            ttk.Label(self.body, text="Downloads", font=app.font_bold).grid(row=row, column=0, columnspan=2, sticky="w")
            ttk.Label(self.body, text="into " + SDK_DIR, foreground=COLORS["dim"], wraplength=px(480)).grid(
                row=row + 1, column=0, columnspan=2, sticky="w")
            row += 2
            for sid, labels in missing:
                sdk = app.registry.sdks[sid]
                var = tk.BooleanVar(value=sdk.can_install())
                cb = ttk.Checkbutton(self.body, text=sdk.name, variable=var)
                cb.grid(row=row, column=0, sticky="w", pady=(px(4), 0))
                if not sdk.can_install():
                    cb.state(["disabled"])
                ttk.Label(self.body, text=megabytes(sdk.download_size())).grid(row=row, column=1, sticky="e")
                detail = "%s. From %s." % (sdk.summary[:1].upper() + sdk.summary[1:], sdk.source)
                if labels:
                    detail += " Needed for: %s." % ", ".join(labels)
                if not sdk.can_install():
                    detail += " It cannot be downloaded on this system: install it yourself (README.md)."
                ttk.Label(self.body, text=detail, foreground=COLORS["dim"], wraplength=px(560), justify="left").grid(
                    row=row + 1, column=0, columnspan=2, sticky="w", padx=(px(24), 0))
                self.checks[sid] = var
                self.sdk_ids.append(sid)
                row += 2
        if games:
            ttk.Label(self.body, text="From your copy of the game (never downloaded)", font=app.font_bold).grid(
                row=row, column=0, columnspan=2, sticky="w", pady=(px(12) if missing else 0, 0))
            row += 1
            for g in games:
                ttk.Label(self.body, text="%s, %s" % (g.name, g.disc), wraplength=px(440), justify="left").grid(
                    row=row, column=0, sticky="w", pady=(px(4), 0))
                b = ttk.Button(self.body, text="Choose file or disc image...", command=lambda g=g: self.choose(g))
                b.grid(row=row, column=1, sticky="e", pady=(px(4), 0))
                st = ttk.Label(self.body, text="", wraplength=px(560), justify="left")
                st.grid(row=row + 1, column=0, columnspan=2, sticky="w", padx=(px(24), 0))
                self.game_rows[id(g)] = st
                if g.present():
                    st.configure(text="In place: %s" % g.dest, foreground=COLORS["ok"])
                else:
                    st.configure(text="Needed to build: the file itself, or a disc image (ISO) that has it.",
                                 foreground=COLORS["dim"])
                row += 2
        if self.sdk_ids and not self.running:
            self.btn_start.configure(text="Download")
            self.btn_start.state(["!disabled"])
            self.btn_start.pack(side="left", padx=px(4), before=self.btn_close)
        elif not self.running:
            self.btn_start.pack_forget()

    def choose(self, g):
        path = filedialog.askopenfilename(parent=self, title="Choose the game file, or a disc image that has it",
                                          filetypes=FILE_TYPES)
        if not path:
            return
        self.configure(cursor="watch")
        self.update()
        why = provide_game_file(g, path)
        self.configure(cursor="")
        if why:
            self.game_rows[id(g)].configure(text="Not used: " + why, foreground=COLORS["error"])
        else:
            self.refresh()
            self.app.after_game_file()

    # ---- downloading

    def write(self, text):
        self.log.insert("end", text + "\n")
        self.log.see("end")

    def start(self):
        chosen = [sid for sid in self.sdk_ids if self.checks[sid].get()]
        if not chosen:
            self.state_label.configure(text="Nothing is ticked.")
            return
        sdks = [self.app.registry.sdks[sid] for sid in chosen]
        self.running = True
        self.cancel.clear()
        self.btn_start.state(["disabled"])
        self.btn_close.configure(text="Cancel")
        if self.c_target is not None:
            self.c_target.state(["disabled"])
        self.state_label.configure(text="Starting...", foreground="")
        ctx = InstallContext(log=lambda t: self.events.put(("log", t)),
                             progress=lambda t, d, n: self.events.put(("progress", (t, d, n))),
                             cancel=self.cancel)

        def work():
            self.events.put(("done", install_all(sdks, ctx)))
        threading.Thread(target=work, daemon=True).start()

    def poll(self):
        try:
            for _ in range(200):
                ev, payload = self.events.get_nowait()
                if ev == "log":
                    self.write(payload)
                elif ev == "progress":
                    text, done, total = payload
                    if total:
                        self.progress.configure(mode="determinate", maximum=total, value=min(done, total))
                        self.state_label.configure(text="%s: %d of %d MB" % (text, done >> 20, total >> 20)
                                                   if total > (1 << 20) else "%s: %d of %d" % (text, done, total))
                    else:
                        self.state_label.configure(text="%s: %d MB" % (text, done >> 20))
                elif ev == "done":
                    self.finished(payload)
        except queue.Empty:
            pass
        if self.winfo_exists():
            self.after(50, self.poll)

    def finished(self, results):
        self.running = False
        failed = [(s, e) for s, e in results if e]
        if not failed:
            self.progress.configure(value=self.progress["maximum"])
            self.state_label.configure(text="Done: %d installed." % len(results), foreground=COLORS["ok"])
        else:
            self.state_label.configure(text="Not finished: " + "; ".join(
                "%s (%s)" % (s.name, e) for s, e in failed), foreground=COLORS["error"])
        self.btn_close.configure(text="Close")
        if self.c_target is not None:
            self.c_target.state(["!disabled", "readonly"])
        self.app.detect()                   # its result refreshes this list

    def close(self):
        if self.running:
            if messagebox.askyesno(APP_NAME, "Stop the download?", parent=self):
                self.cancel.set()
            return
        self.destroy()


class SdksWindow(tk.Toplevel):
    """Every SDK: what detection found, where; download the missing ones; or
    choose a folder for one that has a folder setting."""

    def __init__(self, app):
        super().__init__(app.root)
        self.app = app
        self.title("SDKs")
        self.transient(app.root)
        self.frame = ttk.Frame(self, padding=app.px(12))
        self.frame.pack(fill="both", expand=True)
        self.bind("<Escape>", lambda e: self.destroy())
        self.fill()

    def fill(self):
        for w in self.frame.winfo_children():
            w.destroy()
        app, px = self.app, self.app.px
        row = 0
        for s in sorted(app.registry.sdks.values(), key=lambda s: s.name.lower()):
            st = app.status.get(s.id)
            head = ttk.Frame(self.frame)
            head.grid(row=row, column=0, sticky="ew", pady=(0 if row == 0 else px(12), px(2)))
            ttk.Label(head, text=s.label(st), font=app.font_bold).pack(side="left")
            if st is not None and not st.ready and s.can_install():
                ttk.Button(head, text="Download...", command=lambda s=s: app.setup_sdk(s.id)).pack(side="right")
            row += 1
            ttk.Label(self.frame, text="%s. From %s." % (s.summary[:1].upper() + s.summary[1:], s.source),
                      foreground=COLORS["dim"], wraplength=px(600)).grid(row=row, column=0, sticky="w", padx=(px(16), 0))
            row += 1
            if st is None:
                text, color = "Checking...", COLORS["check"]
            elif st.ready:
                text, color = "Ready: %s" % st.location, COLORS["ok"]
            else:
                text, color = "Not ready: %s" % "; ".join(st.problems), COLORS["error"]
            ttk.Label(self.frame, text=text, foreground=color, wraplength=px(600)).grid(
                row=row, column=0, sticky="w", padx=(px(16), 0))
            row += 1
            if s.setting:
                chosen = app.settings.get("folders", {}).get(s.setting)
                line = ttk.Frame(self.frame)
                line.grid(row=row, column=0, sticky="w", padx=(px(16), 0), pady=(px(4), 0))
                ttk.Label(line, text="Folder: " + (chosen if chosen else "found automatically")).pack(side="left")
                ttk.Button(line, text="Choose folder...", command=lambda s=s: self.choose(s)).pack(side="left", padx=(px(10), px(4)))
                b = ttk.Button(line, text="Automatic", command=lambda s=s: self.auto(s))
                b.pack(side="left")
                if not chosen:
                    b.state(["disabled"])
                row += 1
        bar = ttk.Frame(self.frame)
        bar.grid(row=row, column=0, sticky="e", pady=(px(16), 0))
        ttk.Button(bar, text="Re-detect", command=app.detect).pack(side="left", padx=px(4))
        ttk.Button(bar, text="Close", command=self.destroy).pack(side="left")

    def choose(self, s):
        start = self.app.settings.get("folders", {}).get(s.setting) or REPO_DIR
        d = filedialog.askdirectory(parent=self, title="Choose " + s.setting_label, initialdir=start)
        if d:
            self.app.settings.setdefault("folders", {})[s.setting] = os.path.normpath(d)
            self.app.save_settings()
            self.app.detect()

    def auto(self, s):
        self.app.settings.get("folders", {}).pop(s.setting, None)
        self.app.save_settings()
        self.app.detect()


class App:
    def __init__(self, root):
        self.root = root
        self.registry = Registry()
        self.settings, settings_note = load_settings()
        self.profiles, profiles_note = load_profiles(self.registry)
        self.status = {}              # SDK id -> Status, from the last detection
        self.detecting = False
        self.job = None               # the running BuildJob
        self.job_target = None
        self.events = queue.Queue()   # (event, payload) from the worker threads
        self.current = None           # the Profile shown in the form
        self.loading = False          # True while the form is being filled from a profile
        self.save_pending = None
        self.sdks_window = None
        self.setup_window = None
        self.prompted = set()         # (target, option) offered the setup dialog this session
        self.first_error = None       # log index of the build's first compiler error
        self.output_blocked = False   # the output path holds something a build must not replace
        self.option_rows = {}         # option id -> (radio, name, state, button)
        self.shown_target = None      # the target the option rows were built for

        self.scale = root.winfo_fpixels("1i") / 96.0
        self.setup_style()
        self.build_ui()
        self.restore_size()

        for e in self.registry.errors:
            self.log("Plug-in problem: " + e, "error")
        if path_warning():
            self.log(path_warning(), "warn")
        for note in (settings_note, profiles_note):
            if note:
                self.log(note, "warn")
        if self.profiles.added:
            self.log("New executable: added the profiles %s" % ", ".join(self.profiles.added), "note")
        if self.profiles.added or not os.path.exists(PROFILES_FILE):
            self.save_profiles()
        self.refresh_list()
        names = self.profiles.names()
        self.select(self.profiles.selected if self.profiles.selected in names else (names[0] if names else None))
        self.detect()
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        self.root.after(50, self.poll)

    # ------------------------------------------------------------------ layout

    def px(self, n):
        return int(n * self.scale)

    def setup_style(self):
        style = ttk.Style(self.root)
        if "vista" in style.theme_names():
            style.theme_use("vista")
        base = tkfont.nametofont("TkDefaultFont")
        self.font_bold = base.copy()
        self.font_bold.configure(weight="bold")
        style.configure("Build.TButton", font=self.font_bold, padding=(self.px(14), self.px(4)))

    def build_ui(self):
        r = self.root
        r.title(APP_NAME)
        self.build_menu()
        outer = ttk.Frame(r, padding=self.px(10))
        outer.pack(fill="both", expand=True)

        top = ttk.Frame(outer)
        top.pack(fill="x")

        left = ttk.LabelFrame(top, text="Profiles", padding=self.px(6))
        left.pack(side="left", fill="y")
        self.listbox = tk.Listbox(left, width=30, height=12, exportselection=False, activestyle="none",
                                  relief="flat", highlightthickness=1, highlightcolor="#9ab",
                                  selectbackground="#2f6fcf", selectforeground="white")
        self.listbox.pack(fill="both", expand=True)
        self.listbox.bind("<<ListboxSelect>>", self.on_list_select)
        self.listbox.bind("<Button-3>", self.on_list_menu)
        btns = ttk.Frame(left)
        btns.pack(fill="x", pady=(self.px(6), 0))
        ttk.Button(btns, text="New", width=6, command=self.new_profile).pack(side="left")
        ttk.Button(btns, text="Copy", width=6, command=self.copy_profile).pack(side="left", padx=self.px(3))
        self.btn_delete = ttk.Button(btns, text="Delete", width=6, command=self.delete_profile)
        self.btn_delete.pack(side="left")
        ttk.Button(btns, text="↓", width=2, command=lambda: self.move_profile(1)).pack(side="right")
        ttk.Button(btns, text="↑", width=2, command=lambda: self.move_profile(-1)).pack(
            side="right", padx=(self.px(3), self.px(2)))
        self.list_menu = tk.Menu(self.root, tearoff=False)
        self.list_menu.add_command(label="Build", command=self.build)
        self.list_menu.add_command(label="Go to source", command=self.open_project)
        self.list_menu.add_command(label="Go to output", command=self.open_output)
        self.list_menu.add_separator()
        self.list_menu.add_command(label="Copy", command=self.copy_profile)
        self.list_menu.add_command(label="Delete", command=self.delete_profile)
        self.list_menu.add_separator()
        self.list_menu.add_command(label="Move up", command=lambda: self.move_profile(-1))
        self.list_menu.add_command(label="Move down", command=lambda: self.move_profile(1))

        form = ttk.LabelFrame(top, text="Profile", padding=(self.px(12), self.px(6)))
        form.pack(side="left", fill="both", expand=True, padx=(self.px(10), 0))
        form.columnconfigure(1, weight=1)
        self.form = form
        pad = {"pady": self.px(3)}

        ttk.Label(form, text="Name").grid(row=0, column=0, sticky="w", **pad)
        self.v_name = tk.StringVar()
        self.e_name = ttk.Entry(form, textvariable=self.v_name)
        self.e_name.grid(row=0, column=1, columnspan=2, sticky="ew", **pad)
        self.l_name_err = ttk.Label(form, text="", foreground=COLORS["error"])
        self.l_name_err.grid(row=1, column=1, columnspan=2, sticky="w")
        self.l_name_err.grid_remove()

        ttk.Label(form, text="What to build").grid(row=2, column=0, sticky="w", **pad)
        self.v_target = tk.StringVar()
        self.c_target = ttk.Combobox(form, textvariable=self.v_target, state="readonly")
        self.c_target.grid(row=2, column=1, columnspan=2, sticky="ew", **pad)
        self.c_target.bind("<<ComboboxSelected>>", lambda e: self.on_target_change())
        self.l_target_desc = ttk.Label(form, text="", foreground=COLORS["dim"])
        self.l_target_desc.grid(row=3, column=1, columnspan=2, sticky="w")

        ttk.Label(form, text="Compiler").grid(row=4, column=0, sticky="nw", pady=(self.px(8), 0))
        self.opt_frame = ttk.Frame(form)
        self.opt_frame.grid(row=4, column=1, columnspan=2, sticky="ew", pady=(self.px(8), 0))
        self.opt_frame.columnconfigure(2, weight=1)
        self.v_option = tk.StringVar()

        self.l_game = ttk.Label(form, text="Game files")
        self.l_game.grid(row=5, column=0, sticky="nw", pady=(self.px(8), 0))
        self.game_frame = ttk.Frame(form)
        self.game_frame.grid(row=5, column=1, columnspan=2, sticky="ew", pady=(self.px(8), 0))
        self.game_frame.columnconfigure(0, weight=1)

        ttk.Label(form, text="Output ELF").grid(row=6, column=0, sticky="w", pady=(self.px(8), self.px(3)))
        self.v_output = tk.StringVar()
        self.e_output = ttk.Entry(form, textvariable=self.v_output)
        self.e_output.grid(row=6, column=1, sticky="ew", pady=(self.px(8), self.px(3)))
        ttk.Button(form, text="Browse...", command=self.browse_output).grid(
            row=6, column=2, sticky="e", padx=(self.px(6), 0), pady=(self.px(8), self.px(3)))
        self.l_output_state = ttk.Label(form, text="", foreground=COLORS["dim"], wraplength=self.px(640))
        self.l_output_state.grid(row=7, column=1, columnspan=2, sticky="w")

        actions = ttk.Frame(form)
        actions.grid(row=8, column=0, columnspan=3, sticky="ew", pady=(self.px(12), self.px(2)))
        self.btn_build = ttk.Button(actions, text="Build", style="Build.TButton", command=self.build)
        self.btn_build.grid(row=0, column=0)
        self.btn_stop = ttk.Button(actions, text="Stop", command=self.stop)
        self.btn_stop.grid(row=0, column=1, padx=self.px(6))
        self.btn_stop.state(["disabled"])
        ttk.Button(actions, text="Go to source", command=self.open_project).grid(row=0, column=2)
        ttk.Button(actions, text="Go to output", command=self.open_output).grid(row=0, column=3, padx=self.px(6))
        actions.columnconfigure(4, weight=1)
        self.progress = ttk.Progressbar(actions, mode="determinate", length=self.px(150))
        self.progress.grid(row=0, column=4, sticky="e", padx=(self.px(12), self.px(8)))
        self.l_build_state = ttk.Label(actions, text="", width=20)
        self.l_build_state.grid(row=0, column=5, sticky="e")

        logf = ttk.LabelFrame(outer, text="Build log", padding=self.px(4))
        logf.pack(fill="both", expand=True, pady=(self.px(10), 0))
        self.text = tk.Text(logf, wrap="word", height=12, font=("Consolas", 9), relief="flat",
                            padx=self.px(6), pady=self.px(4), background="#fcfcfc", undo=False)
        ys = ttk.Scrollbar(logf, orient="vertical", command=self.text.yview)
        self.text.configure(yscrollcommand=ys.set)
        self.text.grid(row=0, column=0, sticky="nsew")
        ys.grid(row=0, column=1, sticky="ns")
        logf.rowconfigure(0, weight=1)
        logf.columnconfigure(0, weight=1)
        for tag in ("ok", "error", "warn", "note", "stage"):
            self.text.tag_configure(tag, foreground=COLORS[tag])
        self.text.tag_configure("stage", font=("Consolas", 9, "bold"))
        self.text.tag_configure("ok", font=("Consolas", 9, "bold"))
        self.text.tag_configure("cmd", foreground=COLORS["dim"])
        self.text.tag_configure("progress", foreground=COLORS["dim"])
        self.text.tag_configure("first_error", background="#fde8e8")
        # read-only, but selectable and copyable
        self.text.bind("<Key>", self.on_log_key)
        self.text.bind("<Double-Button-1>", self.on_log_double)
        self.log_menu = tk.Menu(self.root, tearoff=False)
        self.log_menu.add_command(label="Copy", command=lambda: self.text.event_generate("<<Copy>>"))
        self.log_menu.add_command(label="Select all", command=lambda: self.text.tag_add("sel", "1.0", "end"))
        self.log_menu.add_separator()
        self.log_menu.add_command(label="Clear", command=self.clear_log)
        self.text.bind("<Button-3>", lambda e: self.log_menu.tk_popup(e.x_root, e.y_root))

        self.statusbar = ttk.Label(r, text="", padding=(self.px(10), self.px(2)), foreground=COLORS["dim"])
        self.statusbar.pack(fill="x", side="bottom")

        r.bind("<F5>", lambda e: self.build())
        r.bind("<Control-n>", lambda e: self.new_profile())
        self.v_output.trace_add("write", lambda *a: self.on_output_change())
        self.v_name.trace_add("write", lambda *a: self.on_name_change())

    def build_menu(self):
        m = tk.Menu(self.root)
        f = tk.Menu(m, tearoff=False)
        f.add_command(label="New profile", accelerator="Ctrl+N", command=self.new_profile)
        f.add_command(label="Copy profile", command=self.copy_profile)
        f.add_command(label="Delete profile", command=self.delete_profile)
        f.add_separator()
        f.add_command(label="Exit", command=self.on_close)
        m.add_cascade(label="File", menu=f)
        b = tk.Menu(m, tearoff=False)
        b.add_command(label="Build", accelerator="F5", command=self.build)
        b.add_command(label="Stop", command=self.stop)
        b.add_separator()
        b.add_command(label="Go to source", command=self.open_project)
        b.add_command(label="Go to output", command=self.open_output)
        b.add_separator()
        b.add_command(label="Clear log", command=self.clear_log)
        m.add_cascade(label="Build", menu=b)
        t = tk.Menu(m, tearoff=False)
        t.add_command(label="Set up...", command=self.setup_target)
        t.add_command(label="SDKs...", command=self.show_sdks)
        t.add_command(label="Re-detect", command=self.detect)
        t.add_separator()
        t.add_command(label="Open the repository folder", command=lambda: open_path(REPO_DIR))
        m.add_cascade(label="Tools", menu=t)
        h = tk.Menu(m, tearoff=False)
        h.add_command(label="Read me", command=self.open_readme)
        h.add_command(label="About", command=self.about)
        m.add_cascade(label="Help", menu=h)
        self.root.configure(menu=m)

    def restore_size(self):
        try:
            w, h = (int(v) for v in self.settings.get("window_size", (0, 0)))
        except (TypeError, ValueError):
            w, h = 0, 0
        sw, sh = self.root.winfo_screenwidth(), self.root.winfo_screenheight()
        w = w if 600 <= w <= sw else min(self.px(1060), sw - 40)
        h = h if 400 <= h <= sh else min(self.px(780), sh - 80)
        self.root.geometry("%dx%d" % (w, h))
        self.root.minsize(self.px(820), self.px(580))

    # ------------------------------------------------------------------ events from threads

    def poll(self):
        try:
            for _ in range(500):
                ev, payload = self.events.get_nowait()
                if ev == "log":
                    self.on_build_line(*payload)
                elif ev == "detected":
                    self.on_detected(payload)
                elif ev == "done":
                    self.on_build_done(payload)
        except queue.Empty:
            pass
        self.root.after(50, self.poll)

    # ------------------------------------------------------------------ log

    def log(self, text, tag=None):
        at_end = self.text.yview()[1] >= 0.999
        self.text.insert("end", text + "\n", (tag,) if tag else ())
        if tag == "error" and self.first_error is None and "error:" in text.lower():
            self.first_error = self.text.index("end-2l linestart")
        if at_end:
            self.text.see("end")

    def clear_log(self):
        self.text.delete("1.0", "end")

    def on_log_key(self, event):
        if event.state & 0x4 and event.keysym.lower() in ("c", "a", "insert"):
            if event.keysym.lower() == "a":
                self.text.tag_add("sel", "1.0", "end")
                return "break"
            return None
        if event.keysym in ("Up", "Down", "Left", "Right", "Prior", "Next", "Home", "End"):
            return None
        return "break"

    def source_at(self, line):
        """(path, line, column) of the first existing source file a log line
        names, e.g. a compiler message "src/main.cpp:12:5: error: ...", or None."""
        t = self.job_target or (self.registry.targets.get(self.current.target) if self.current else None)
        base = t.project if t else REPO_DIR
        for m in SOURCE_REF.finditer(line):
            path = os.path.normpath(os.path.join(base, m.group("path")))
            if os.path.isfile(path):
                return path, m.group("line") or "1", m.group("col") or "1"
        return None

    def on_log_double(self, event):
        line = self.text.get("@%d,%d linestart" % (event.x, event.y), "@%d,%d lineend" % (event.x, event.y))
        ref = self.source_at(line)
        if ref:
            self.open_source(*ref)
            return "break"
        return None

    def open_source(self, path, line, col):
        code = shutil.which("code")
        if code:
            subprocess.Popen(["cmd", "/c", code, "-g", "%s:%s:%s" % (path, line, col)] if os.name == "nt"
                             else [code, "-g", "%s:%s:%s" % (path, line, col)], creationflags=NO_WINDOW)
        elif os.name == "nt":
            os.startfile(path)
        else:
            subprocess.Popen(["xdg-open", path])

    # ------------------------------------------------------------------ profiles

    def target_names(self):
        return [t.name for t in sorted(self.registry.targets.values(), key=lambda t: t.name)]

    def target_by_name(self, name):
        for t in self.registry.targets.values():
            if t.name == name:
                return t
        return None

    def cur_target(self):
        return self.registry.targets.get(self.current.target) if self.current else None

    def refresh_list(self):
        self.listbox.delete(0, "end")
        for name in self.profiles.names():
            self.listbox.insert("end", name)

    def select(self, name):
        names = self.profiles.names()
        if name not in names:
            self.current = None
            self.show_profile()
            return
        i = names.index(name)
        self.listbox.selection_clear(0, "end")
        self.listbox.selection_set(i)
        self.listbox.see(i)
        self.current = self.profiles.get(name)
        self.profiles.selected = name
        self.show_profile()
        self.schedule_save()
        self.maybe_prompt()

    def on_list_select(self, event=None):
        sel = self.listbox.curselection()
        if not sel:
            return
        name = self.listbox.get(sel[0])
        if self.current is None or name != self.current.name:
            self.select(name)

    def on_list_menu(self, event):
        i = self.listbox.nearest(event.y)
        if 0 <= i < self.listbox.size():
            self.select(self.listbox.get(i))
            self.list_menu.tk_popup(event.x_root, event.y_root)

    def move_profile(self, delta):
        if self.current is None:
            return
        items = self.profiles.items
        i = items.index(self.current)
        j = i + delta
        if 0 <= j < len(items):
            items[i], items[j] = items[j], items[i]
            self.refresh_list()
            self.select(self.current.name)

    def show_profile(self):
        """Fill the form from self.current."""
        self.loading = True
        try:
            p = self.current
            widgets = [self.e_name, self.c_target, self.e_output]
            if p is None:
                for w in widgets:
                    w.state(["disabled"])
                for v in (self.v_name, self.v_target, self.v_output):
                    v.set("")
                self.l_target_desc.configure(text="No profile: click New to make one.")
            else:
                for w in widgets:
                    w.state(["!disabled"])
                self.c_target.state(["readonly"])
                self.v_name.set(p.name)
                self.name_error("")
                self.c_target.configure(values=self.target_names())
                t = self.cur_target()
                self.v_target.set(t.name if t else "(no target plug-in %r)" % p.target)
                self.l_target_desc.configure(text=t.description if t else "")
                self.v_option.set(p.option)
                self.v_output.set(p.output)
        finally:
            self.loading = False
        self.rebuild_options()
        self.update_options()
        self.update_games()
        self.update_output_state()

    def schedule_save(self):
        if self.save_pending is not None:
            self.root.after_cancel(self.save_pending)
        self.save_pending = self.root.after(400, self.save_profiles)

    def save_profiles(self):
        self.save_pending = None
        try:
            save_profiles(self.profiles)
        except OSError as e:
            self.log("Could not save %s: %s" % (PROFILES_FILE, e), "error")

    def save_settings(self):
        try:
            save_settings(self.settings)
        except OSError as e:
            self.log("Could not save the settings: %s" % e, "error")

    def name_error(self, text):
        self.l_name_err.configure(text=text)
        if text:
            self.l_name_err.grid()
        else:
            self.l_name_err.grid_remove()

    def on_name_change(self):
        if self.loading or self.current is None:
            return
        new = self.v_name.get().strip()
        if not new:
            self.name_error("A profile needs a name.")
            return
        if any(p is not self.current and p.name.lower() == new.lower() for p in self.profiles.items):
            self.name_error("Another profile has this name.")
            return
        self.name_error("")
        if new != self.current.name:
            i = self.profiles.items.index(self.current)
            self.current.name = new
            self.profiles.selected = new
            self.listbox.delete(i)
            self.listbox.insert(i, new)
            self.listbox.selection_set(i)
            self.schedule_save()

    def on_target_change(self):
        if self.loading or self.current is None:
            return
        t = self.target_by_name(self.v_target.get())
        if t is None or t.id == self.current.target:
            return
        old, p = self.cur_target(), self.current
        # an output that was the old target's default follows the new target
        if not p.output.strip() or (old and p.output == old.default_output(p.option)):
            p.output = ""
        p.target = t.id
        if t.option(p.option) is None:
            p.option = t.options[0].id
        if not p.output:
            p.output = t.default_output(p.option)
        self.show_profile()
        self.schedule_save()
        self.maybe_prompt()

    def on_option_change(self):
        if self.loading or self.current is None:
            return
        p, t, new = self.current, self.cur_target(), self.v_option.get()
        if not t or new == p.option:
            return
        if p.output == t.default_output(p.option):
            p.output = t.default_output(new)
            self.loading = True
            self.v_output.set(p.output)
            self.loading = False
        p.option = new
        self.update_options()
        self.update_output_state()
        self.schedule_save()
        self.maybe_prompt()

    def on_output_change(self):
        if self.loading or self.current is None:
            return
        self.current.output = self.v_output.get()
        self.update_output_state()
        self.schedule_save()

    def browse_output(self):
        t = self.cur_target()
        if self.current is None or t is None:
            return
        cur = resolve_output(self.current.output, t) if self.current.output.strip() else ""
        start_dir = os.path.dirname(cur) if cur and os.path.isdir(os.path.dirname(cur)) else REPO_DIR
        path = filedialog.asksaveasfilename(parent=self.root, title="Where should the built ELF go?",
                                            initialdir=start_dir, initialfile=os.path.basename(cur) or t.output_name,
                                            confirmoverwrite=False, filetypes=FILE_TYPES)
        if path:
            self.v_output.set(os.path.normpath(path))

    def new_profile(self):
        targets = sorted(self.registry.targets.values(), key=lambda t: t.name)
        if not targets:
            messagebox.showerror(APP_NAME, "There are no target plug-ins in %s." %
                                 os.path.join(TOOLCHAIN_DIR, "targets"), parent=self.root)
            return
        t = self.cur_target() or targets[0]
        p = Profile(self.profiles.unique_name("New profile"), t.id, t.options[0].id,
                    t.default_output(t.options[0].id))
        self.insert_profile(p)
        self.e_name.focus_set()
        self.e_name.select_range(0, "end")

    def copy_profile(self):
        if self.current is None:
            return
        p = self.current.copy()
        p.name = self.profiles.unique_name(self.current.name + " copy")
        self.insert_profile(p)

    def insert_profile(self, p):
        i = self.profiles.items.index(self.current) + 1 if self.current in self.profiles.items else len(self.profiles.items)
        self.profiles.items.insert(i, p)
        self.refresh_list()
        self.select(p.name)

    def delete_profile(self):
        if self.current is None:
            return
        if not messagebox.askyesno(APP_NAME, "Delete the profile \"%s\"?\n\nOnly the profile goes; built "
                                   "files stay where they are." % self.current.name, parent=self.root):
            return
        i = self.profiles.items.index(self.current)
        self.profiles.items.pop(i)
        self.refresh_list()
        names = self.profiles.names()
        self.select(names[min(i, len(names) - 1)] if names else None)
        self.save_profiles()

    # ------------------------------------------------------------------ compiler options and game files

    def rebuild_options(self):
        """One row per option of the current target: radio, SDK, state, button."""
        t = self.cur_target()
        if t is self.shown_target and self.option_rows:
            return
        for w in self.opt_frame.winfo_children():
            w.destroy()
        self.option_rows = {}
        self.shown_target = t
        if t is None:
            return
        for i, o in enumerate(t.options):
            r0, top = 2 * i, (self.px(2) if i == 0 else self.px(6))
            rb = ttk.Radiobutton(self.opt_frame, text=o.label, value=o.id, variable=self.v_option,
                                 command=self.on_option_change)
            rb.grid(row=r0, column=0, sticky="w", pady=(top, 0))
            name = ttk.Label(self.opt_frame, text="", font=self.font_bold)
            name.grid(row=r0, column=1, sticky="w", padx=(self.px(14), 0), pady=(top, 0))
            state = ttk.Label(self.opt_frame, text="")
            state.grid(row=r0, column=2, sticky="w", padx=(self.px(14), 0), pady=(top, 0))
            btn = ttk.Button(self.opt_frame, text="Set up...", width=10, command=self.setup_target)
            btn.grid(row=r0, column=3, rowspan=2, sticky="e")
            ttk.Label(self.opt_frame, text=o.summary, foreground=COLORS["dim"]).grid(
                row=r0 + 1, column=1, columnspan=2, sticky="w", padx=(self.px(14), 0))
            self.option_rows[o.id] = (rb, name, state, btn)

    def update_options(self):
        """The option rows' SDK names and states, and the Build button."""
        t = self.cur_target()
        for oid, (rb, name, state, btn) in self.option_rows.items():
            o = t.option(oid)
            sdk = self.registry.sdks.get(o.sdk)
            name.configure(text=sdk.label(self.status.get(o.sdk)) if sdk else o.sdk)
            if not self.status:
                state.configure(text="checking...", foreground=COLORS["check"])
                btn.grid_remove()
                continue
            sdks, files = self.registry.missing(t, oid, self.status)
            if not sdks:
                state.configure(text="Ready", foreground=COLORS["ok"])
                btn.grid_remove()
            else:
                state.configure(text="Not set up", foreground=COLORS["error"])
                btn.grid()
        self.update_build_button()

    def update_games(self):
        """One line per game file the target needs."""
        for w in self.game_frame.winfo_children():
            w.destroy()
        t = self.cur_target()
        files = t.game_files if t else []
        if not files:
            self.l_game.grid_remove()
            self.game_frame.grid_remove()
            return
        self.l_game.grid()
        self.game_frame.grid()
        for i, g in enumerate(files):
            ok = g.present()
            ttk.Label(self.game_frame, text="%s: %s" % (g.name, "in place" if ok else "needed, from your own disc"),
                      foreground=COLORS["ok"] if ok else COLORS["error"]).grid(row=i, column=0, sticky="w")
            if not ok:
                ttk.Button(self.game_frame, text="Provide...", width=10,
                           command=self.setup_target).grid(row=i, column=1, sticky="e")

    def after_game_file(self):
        self.update_games()
        self.update_options()

    def detect(self):
        if self.detecting:
            return
        self.detecting = True
        self.status = {}
        self.update_options()
        self.statusbar.configure(text="Looking for SDKs...")
        settings = dict(self.settings.get("folders", {}))

        def work():
            self.events.put(("detected", self.registry.detect_all(settings)))
        threading.Thread(target=work, daemon=True).start()
        if self.sdks_window and self.sdks_window.winfo_exists():
            self.sdks_window.fill()

    def on_detected(self, status):
        self.detecting = False
        self.status = status
        self.update_options()
        self.update_games()
        ready = sum(1 for s in status.values() if s.ready)
        self.statusbar.configure(text="SDKs: %d of %d installed (Tools > SDKs)" % (ready, len(status)))
        if self.sdks_window and self.sdks_window.winfo_exists():
            self.sdks_window.fill()
        if self.setup_window and self.setup_window.winfo_exists() and not self.setup_window.running:
            self.setup_window.refresh()
        self.maybe_prompt()

    def ready(self):
        """(ready, problems) of the current profile."""
        t = self.cur_target()
        if self.current is None or t is None or not self.status:
            return False, []
        return self.registry.readiness(t, self.current.option, self.status)

    def maybe_prompt(self):
        """Offer the setup dialog once per target and option per session when
        the current profile cannot build for want of something."""
        t = self.cur_target()
        if (t is None or not self.status or self.job is not None or
                (self.setup_window and self.setup_window.winfo_exists())):
            return
        key = (t.id, self.current.option)
        if key in self.prompted:
            return
        sdks, files = self.registry.missing(t, self.current.option, self.status)
        if sdks or files:
            self.prompted.add(key)
            self.root.after(150, self.setup_target)

    def setup_target(self, *_):
        """The setup dialog, on the current profile's executable; its dropdown
        picks any other."""
        if self.setup_window and self.setup_window.winfo_exists():
            self.setup_window.lift()
            return
        t = self.cur_target()
        self.setup_window = SetupDialog(self, target_id=t.id if t else None)

    def setup_sdk(self, sdk_id):
        """The setup dialog for one SDK and what it requires."""
        if self.setup_window and self.setup_window.winfo_exists():
            self.setup_window.lift()
            return
        order, todo = [], [sdk_id]
        while todo:
            sid = todo.pop(0)
            if sid not in order:
                order.append(sid)
                todo += self.registry.sdks[sid].requires
        self.setup_window = SetupDialog(self, sdk_ids=order)

    def show_sdks(self):
        if self.sdks_window and self.sdks_window.winfo_exists():
            self.sdks_window.lift()
            return
        self.sdks_window = SdksWindow(self)

    # ------------------------------------------------------------------ output file

    def update_build_button(self):
        p = self.current
        ok = (p is not None and self.job is None and bool(self.status) and p.output.strip() != ""
              and not self.output_blocked and self.ready()[0])
        self.btn_build.state(["!disabled"] if ok else ["disabled"])
        self.btn_delete.state(["disabled"] if self.job else ["!disabled"])

    def update_output_state(self):
        p, t = self.current, self.cur_target()
        self.output_blocked = False
        if not p or not t:
            self.l_output_state.configure(text="")
            self.update_build_button()
            return
        out = resolve_output(p.output, t)
        if not out:
            self.l_output_state.configure(text="Where the finished ELF is copied after each build.",
                                          foreground=COLORS["dim"])
            self.update_build_button()
            return
        shown = "" if os.path.normcase(out) == os.path.normcase(p.output.strip()) else out + ": "
        try:
            state, what = existing_output(out, t)
        except OSError as e:
            state, what = "other", "unreadable (%s)" % e.strerror
        if state is None:
            text = ("Not there yet; the build creates it." if os.path.isdir(os.path.dirname(out))
                    else "Not there yet; the build creates the folder too.")
        elif state == "other":
            text = "This is %s, so a build will not replace it. Pick another path." % what
            self.output_blocked = True
        elif state == "original":
            text = "Holds %s now; the first build backs it up, then replaces it." % what
        else:
            text = "Exists (%s bytes); each build replaces it." % format(os.path.getsize(out), ",")
        self.l_output_state.configure(text=shown + text,
                                      foreground=COLORS["error"] if state == "other" else COLORS["dim"])
        self.update_build_button()

    def open_output(self):
        p, t = self.current, self.cur_target()
        if not t:
            return
        out = resolve_output(p.output, t)
        if out and os.path.exists(out):
            open_path(out, select=True)
        elif out and os.path.isdir(os.path.dirname(out)):
            open_path(os.path.dirname(out))
        else:
            messagebox.showinfo(APP_NAME, "The output folder does not exist yet; the first build creates it.",
                                parent=self.root)

    def open_project(self):
        t = self.cur_target()
        open_path(t.project if t and os.path.isdir(t.project) else REPO_DIR)

    # ------------------------------------------------------------------ building

    def build(self):
        p = self.current
        if p is None or self.job is not None:
            return
        if not self.ready()[0]:
            self.setup_target()
            return
        if "disabled" in self.btn_build.state():
            return
        if self.save_pending is not None:
            self.root.after_cancel(self.save_pending)
            self.save_profiles()
        self.clear_log()
        self.first_error = None
        job = BuildJob(p, self.registry, self.settings.get("folders", {}),
                       log=lambda text, tag=None: self.events.put(("log", (text, tag))))
        self.job, self.job_target = job, self.cur_target()
        self.btn_stop.state(["!disabled"])
        self.update_build_button()
        self.progress.configure(mode="indeterminate", value=0)
        self.progress.start(12)
        self.l_build_state.configure(text="Starting...", foreground="")
        self.statusbar.configure(text="Building %s..." % p.name)

        def work():
            try:
                result = job.run()
            except Exception as e:                      # a plug-in bug: report it, keep the window
                import traceback
                self.events.put(("log", ("Build crashed: %s\n%s" % (e, traceback.format_exc()), "error")))
                result = None
            self.events.put(("done", result))
        threading.Thread(target=work, daemon=True).start()

    def on_build_line(self, text, tag):
        self.log(text, tag)
        m = PROGRESS.match(text.strip())
        if m:
            done, total = int(m.group(1)), int(m.group(2))
            if str(self.progress["mode"]) != "determinate":
                self.progress.stop()
                self.progress.configure(mode="determinate")
            self.progress.configure(maximum=total + 3, value=done)
            self.l_build_state.configure(text="Compiling %d/%d" % (done, total))
        elif text.startswith("== "):
            stage = text[3:].split()[0].rstrip(":") if len(text) > 3 else ""
            names = {"check": "Checking objects", "link": "Linking", "finish": "Finishing", "assets": "Extracting data"}
            if stage in names:
                if str(self.progress["mode"]) == "determinate" and stage != "assets":
                    total = float(self.progress["maximum"])
                    self.progress.configure(value=total - 3 + {"check": 1, "link": 2, "finish": 3}[stage])
                self.l_build_state.configure(text=names[stage] + "...")

    def on_build_done(self, result):
        self.progress.stop()
        self.progress.configure(mode="determinate", maximum=1, value=1 if result and result.ok else 0)
        self.job = None
        self.btn_stop.state(["disabled"])
        if result and result.ok:
            self.l_build_state.configure(text="Built in %.1f s" % result.seconds, foreground=COLORS["ok"])
            self.statusbar.configure(text=result.message)
        else:
            stopped = result is not None and result.message == "Stopped."
            self.l_build_state.configure(text="Stopped" if stopped else "Failed", foreground=COLORS["error"])
            self.statusbar.configure(text=result.message if result else "The build crashed; see the log.")
            if self.first_error:
                # the first compiler error, not the end of the log (double-click opens the file)
                self.text.tag_add("first_error", self.first_error, self.first_error + " lineend")
                self.text.see(self.first_error)
                self.statusbar.configure(text=self.statusbar.cget("text") +
                                         "  Double-click an error in the log to open the file there.")
            self.root.bell()
        self.update_build_button()
        self.update_output_state()

    def stop(self):
        if self.job is not None:
            self.l_build_state.configure(text="Stopping...")
            threading.Thread(target=self.job.stop, daemon=True).start()

    # ------------------------------------------------------------------ misc

    def open_readme(self):
        path = os.path.join(REPO_DIR, "README.md")
        if os.name == "nt":
            os.startfile(path)
        else:
            subprocess.Popen(["xdg-open", path])

    def about(self):
        messagebox.showinfo(APP_NAME, "%s\n\nBuilds the decompiled GT3 executables from profiles: what to "
                            "build, with which compiler, and where the ELF goes.\n\n%s\n\nPython %s" % (
                                APP_NAME, REPO_DIR, sys.version.split()[0]), parent=self.root)

    def on_close(self):
        if self.job is not None:
            if not messagebox.askyesno(APP_NAME, "A build is running. Stop it and quit?", parent=self.root):
                return
            self.job.stop()
        if self.setup_window and self.setup_window.winfo_exists() and self.setup_window.running:
            if not messagebox.askyesno(APP_NAME, "A download is running. Stop it and quit?", parent=self.root):
                return
            self.setup_window.cancel.set()
        if self.save_pending is not None:
            self.root.after_cancel(self.save_pending)
            self.save_profiles()
        if self.root.state() == "normal":
            self.settings["window_size"] = [self.root.winfo_width(), self.root.winfo_height()]
            self.save_settings()
        self.root.destroy()


def make_icon(root):
    """A small window icon drawn in code: a blue tile with three white bars."""
    img = tk.PhotoImage(width=32, height=32)
    img.put("#1f4e9a", to=(0, 0, 32, 32))
    img.put("#2f6fcf", to=(2, 2, 30, 30))
    for y in (8, 14, 20):
        img.put("#ffffff", to=(7, 1 + y, 25, 4 + y))
    img.put("#ffd54a", to=(7, 21, 13, 24))
    root.iconphoto(True, img)
    return img


def main():
    if os.name == "nt":
        try:                                    # sharp text on scaled displays
            import ctypes
            ctypes.windll.shcore.SetProcessDpiAwareness(1)
        except Exception:
            pass
    root = tk.Tk()
    root._icon = make_icon(root)
    App(root)
    root.mainloop()
    return 0
