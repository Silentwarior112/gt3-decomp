"""GT3 Decomp Builder from the command line (the window is GT3Builder.pyw).

    python gt3build.py                  the profiles, and whether each can build
    python gt3build.py <profile>        build one: its name, its short form
                                        ("bootstrap-fast"), or a unique start of either
    python gt3build.py --sdks           what was found for every SDK
    python gt3build.py --gui            open the window

A profile that lacks something is set up first: the builder asks before it
downloads (--yes answers for you) and asks for a game file it needs
(--game-file PATH gives your file, or the disc image it is on).
Exit status: 0 when the build (or listing) worked, 1 otherwise.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "toolchain"))

from builder import APP_NAME, SDK_DIR, path_warning                     # noqa: E402
from builder.config import PROFILES_FILE, load_profiles, load_settings, save_profiles, slug  # noqa: E402
from builder.download import InstallContext, install_all, provide_game_file  # noqa: E402
from builder.registry import Registry                                   # noqa: E402
from builder.runner import BuildJob, resolve_output                     # noqa: E402


def ask(question, assume_yes):
    if assume_yes:
        return True
    if not sys.stdin.isatty():
        return False
    return input(question + " [Y/n] ").strip().lower() in ("", "y", "yes")


def show_sdks(reg, status):
    for s in sorted(reg.sdks.values(), key=lambda s: s.name.lower()):
        st = status[s.id]
        print("%-44s %s" % (s.label(st), ("ready: " + st.location) if st.ready else "NOT READY: " + "; ".join(st.problems)))
    return all(st.ready for st in status.values())


def list_profiles(reg, profiles, status):
    print("%s - profiles (%s)" % (APP_NAME, PROFILES_FILE))
    print()
    for p in profiles.items:
        t = reg.targets.get(p.target)
        o = t.option(p.option) if t else None
        if o is None:
            print("  %-34s  ? unknown target or option (%s, %s)" % (p.name, p.target, p.option))
            continue
        ready, problems = reg.readiness(t, o.id, status)
        sdk = reg.sdks.get(o.sdk)
        print("  %-34s  %s, %s: %s" % (p.name, t.name, o.label.lower(), sdk.label(status.get(o.sdk)) if sdk else o.sdk))
        print("  %-34s  -> %s" % ("(%s)" % slug(p.name), resolve_output(p.output, t) or "(no output path)"))
        if not ready:
            print("  %-34s  not set up: %s" % ("", "; ".join(problems)))
    if not profiles.items:
        print("  (none: open the window to make one)")
    print()
    print("Build one:  python gt3build.py <profile>")


def progress_printer():
    state = {"text": None, "pct": -1}
    tty = sys.stdout.isatty()

    def progress(text, done, total):
        pct = int(100 * done / total) if total else -1
        if text != state["text"] or (pct // (2 if tty else 20)) != (state["pct"] // (2 if tty else 20)):
            line = "  %s: %s" % (text, "%d%%" % pct if pct >= 0 else "%d MB" % (done >> 20))
            if tty:
                sys.stdout.write("\r" + line.ljust(70))
                sys.stdout.flush()
            else:
                print(line, flush=True)
            state.update(text=text, pct=pct)
    return progress


def set_up(reg, target, option, settings, args):
    """Provide what the profile lacks. True when it can build now."""
    status = reg.detect_all(settings)
    sdk_ids, files = reg.missing(target, option.id, status)
    for g in files:
        path = args.game_file
        if not path and sys.stdin.isatty():
            path = input("%s needs your %s (%s).\nPath to that file, or to the disc image it is on: "
                         % (target.name, g.name, g.disc)).strip().strip('"')
        if not path:
            print("%s needs your %s (%s): pass --game-file PATH" % (target.name, g.name, g.disc))
            return False
        why = provide_game_file(g, path)
        if why:
            print("Not used: " + why)
            return False
        print("Your %s is in place: %s" % (g.name, g.dest))
    if sdk_ids:
        sdks = [reg.sdks[sid] for sid in sdk_ids]
        print("%s, %s, needs these downloads (into %s):" % (target.name, option.label.lower(), SDK_DIR))
        for s in sdks:
            size = s.download_size()
            print("  %-44s %s  from %s" % (s.name, ("%d MB" % max(1, size >> 20)) if size else "", s.source))
        cant = [s.name for s in sdks if not s.can_install()]
        if cant:
            print("These cannot be downloaded on this system; install them yourself: " + ", ".join(cant))
            return False
        if not ask("Download them now?", args.yes):
            print("Not downloaded.")
            return False
        progress = progress_printer()

        def log(text):
            if sys.stdout.isatty():
                sys.stdout.write("\r" + " " * 70 + "\r")
            print(text, flush=True)
        results = install_all(sdks, InstallContext(log, progress))
        if any(err for _, err in results):
            return False
    return True


def main():
    for stream in (sys.stdout, sys.stderr):
        try:                    # compiler messages may hold characters the console cannot show
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    ap = argparse.ArgumentParser(description="Build the decompiled GT3 executables from profiles.")
    ap.add_argument("profile", nargs="?", help="the profile to build")
    ap.add_argument("--sdks", action="store_true", help="show what was found for every SDK")
    ap.add_argument("--gui", action="store_true", help="open the window")
    ap.add_argument("--yes", "-y", action="store_true", help="download what is missing without asking")
    ap.add_argument("--game-file", metavar="PATH", help="your game file, or the disc image it is on")
    ap.add_argument("--jobs", "-j", type=int, default=0, help="files to compile at once (default: one per CPU)")
    a = ap.parse_args()
    if a.gui:
        from builder import gui
        return gui.main()

    reg = Registry()
    for e in reg.errors:
        print("plug-in problem: " + e)
    if path_warning():
        print("Warning: " + path_warning())
    settings, note = load_settings()
    folders = settings.get("folders", {})
    if note:
        print(note)
    profiles, note = load_profiles(reg)
    if note:
        print(note)
    if not os.path.exists(PROFILES_FILE):
        save_profiles(profiles)

    if a.sdks:
        return 0 if show_sdks(reg, reg.detect_all(folders)) else 1
    if not a.profile:
        list_profiles(reg, profiles, reg.detect_all(folders))
        return 0

    found = profiles.find(a.profile)
    if len(found) != 1:
        print("%s profile matches %r%s" % ("No" if not found else "More than one", a.profile,
              "" if not found else ": " + ", ".join(p.name for p in found)))
        print("Profiles: " + ", ".join(profiles.names()))
        return 1
    p = found[0]
    t = reg.targets.get(p.target)
    o = t.option(p.option) if t else None
    if o is None:
        print("The profile %r builds %s / %s, which no plug-in provides." % (p.name, p.target, p.option))
        return 1
    if not set_up(reg, t, o, folders, a):
        return 1

    def log(text, tag=None):
        print(text, flush=True)

    job = BuildJob(p, reg, folders, log, jobs=a.jobs or None)
    try:
        result = job.run()
    except KeyboardInterrupt:
        job.stop()
        print("stopped")
        return 1
    return 0 if result.ok else 1


if __name__ == "__main__":
    sys.exit(main())
