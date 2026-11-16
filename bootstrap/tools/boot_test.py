"""Boot-test a bootstrap ELF in an isolated PCSX2 against the retail GT3 US disc.

    python tools/boot_test.py [--elf PATH] [--iso PATH] [--seconds N] [--tag NAME] [--card DIR]
    python tools/boot_test.py --baseline          # the retail bootstrap, same run

--card puts a copy of a PCSX2 memory card (a folder card or a .ps2 file) in
slot 2 for the run; the bootstrap prefers a /BASCUS-97102GAMEDATA/core.gt3 on
it to the disc's (core/tools/gt3image.py makes such cards). Without --card,
slot 2 holds the private blank card. --copy NAME runs a second private copy
(build/pcsx2-test-NAME), so two boot tests can run at once.

PCSX2 is started with the ELF as a boot-ELF override and the ISO as the disc
(`pcsx2-qt -batch -nogui -elf ELF -- ISO`), so the ELF runs exactly where
SCUS_971.02 would, reading CORE.GT3, IOPRP224.IMG and the IRX modules from the
real disc. Output under build/boottest/<tag>/: emulog.txt, window captures at
fixed times, and summary.txt with the log markers found.

The PCSX2 used is a private portable copy made on first use from your
install (program files only: no memory cards, save states, cheats, logs or
textures). It points at your BIOS read-only, gets its own blank memory
cards, and has achievements (and their login) stripped from its settings.

This PC's paths come from <repo>/local_paths.json (not in the repository):
    {"pcsx2": "<PCSX2 folder>", "iso": "<GT3 A-Spec US disc image>",
     "bios": "<BIOS folder; default: the one that PCSX2's own settings name>"}
--pcsx2 and --iso override them. Windows only; captures need Pillow.
"""
import argparse, ctypes, ctypes.wintypes as wt, os, re, shutil, subprocess, sys, time

import paths

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
USER_PCSX2 = paths.local("pcsx2")
TEST_PCSX2 = os.path.join(ROOT, "build", "pcsx2-test")
ISO = paths.local("iso")
BIOS = paths.local("bios")
RETAIL_ELF = paths.RETAIL_ELF
DEFAULT_ELF = os.path.join(ROOT, "build", "scus_971.02")
CAPTURE_AT = [6, 12, 20, 30, 45, 60, 90]

PROGRAM_DIRS = ["QtPlugins", "resources", "translations", "D3D12"]

# ---------------------------------------------------------------- setup

def ini_sections(text):
    out, cur = {}, None
    order = []
    for L in text.splitlines():
        m = re.match(r"^\[(.+)\]\s*$", L)
        if m:
            cur = m.group(1); out.setdefault(cur, []); order.append(cur)
        elif cur is not None and L.strip():
            out[cur].append(L)
    return out, order

def set_key(sec, key, value):
    for i, L in enumerate(sec):
        if re.match(r"^%s\s*=" % re.escape(key), L):
            sec[i] = "%s = %s" % (key, value); return
    sec.append("%s = %s" % (key, value))

def setup():
    exe = os.path.join(TEST_PCSX2, "pcsx2-qt.exe")
    if os.path.exists(exe):
        return
    if not USER_PCSX2 or not os.path.isfile(os.path.join(USER_PCSX2, "pcsx2-qt.exe")):
        sys.exit('no PCSX2 install: set "pcsx2" in %s or pass --pcsx2' % os.path.join(paths.REPO, "local_paths.json"))
    print("setting up private PCSX2 in %s" % TEST_PCSX2)
    os.makedirs(TEST_PCSX2, exist_ok=True)
    for fn in os.listdir(USER_PCSX2):
        src = os.path.join(USER_PCSX2, fn)
        if os.path.isfile(src) and (fn.lower().endswith((".exe", ".dll")) or fn in ("qt.conf", "portable.ini")):
            if fn.lower() == "updater.exe":
                continue
            shutil.copy2(src, os.path.join(TEST_PCSX2, fn))
    for d in PROGRAM_DIRS:
        s = os.path.join(USER_PCSX2, d)
        if os.path.isdir(s):
            shutil.copytree(s, os.path.join(TEST_PCSX2, d), dirs_exist_ok=True)

    secs, order = ini_sections(open(os.path.join(USER_PCSX2, "inis", "PCSX2.ini"), encoding="utf-8", errors="replace").read())
    secs.pop("Achievements", None)
    order = [s for s in order if s != "Achievements"]
    ui = secs.setdefault("UI", [])
    for k, v in [("ConfirmShutdown", "false"), ("StartPaused", "false"), ("PauseOnFocusLoss", "false"),
                 ("StartFullscreen", "false"), ("SetupWizardIncomplete", "false")]:
        set_key(ui, k, v)
    ui[:] = [L for L in ui if not L.startswith(("MainWindowGeometry", "MainWindowState", "DisplayWindowGeometry"))]
    f = secs.setdefault("Folders", [])
    bios_dir = BIOS
    if not bios_dir:            # the BIOS folder the install itself is set to
        named = [L.split("=", 1)[1].strip() for L in f if re.match(r"^Bios\s*=", L)]
        bios_dir = os.path.join(USER_PCSX2, named[0]) if named else os.path.join(USER_PCSX2, "bios")
    for k, v in [("Bios", bios_dir), ("Snapshots", "snaps"), ("Savestates", "sstates"), ("MemoryCards", "memcards"),
                 ("Logs", "logs"), ("Cheats", "cheats"), ("CheatsWS", "cheats_ws"), ("CheatsNI", "cheats_ni"),
                 ("Cache", "cache"), ("Textures", "textures"), ("InputProfiles", "inputprofiles"), ("Videos", "videos"),
                 ("Covers", "covers")]:
        set_key(f, k, v)
    mc = secs.setdefault("MemoryCards", [])
    for k, v in [("Slot1_Enable", "true"), ("Slot1_Filename", "Mcd001.ps2"),
                 ("Slot2_Enable", "true"), ("Slot2_Filename", "Mcd002.ps2")]:
        set_key(mc, k, v)
    ec = secs.setdefault("EmuCore", [])
    for k, v in [("EnableCheats", "false"), ("EnableFastBoot", "true"), ("HostFs", "true"),
                 ("EnableDiscordPresence", "false")]:
        set_key(ec, k, v)
    lg = secs.setdefault("Logging", [])
    for k, v in [("EnableFileLogging", "true"), ("EnableEEConsole", "true"), ("EnableIOPConsole", "true"),
                 ("EnableTimestamps", "true"), ("EnableLogWindow", "false"), ("EnableSystemConsole", "false")]:
        set_key(lg, k, v)
    au = secs.setdefault("AutoUpdater", [])
    set_key(au, "CheckAtStartup", "false")
    for s in ("AutoUpdater",):
        if s not in order:
            order.append(s)
    os.makedirs(os.path.join(TEST_PCSX2, "inis"), exist_ok=True)
    with open(os.path.join(TEST_PCSX2, "inis", "PCSX2.ini"), "w", encoding="utf-8", newline="\r\n") as fh:
        for s in order:
            fh.write("[%s]\n" % s)
            for L in secs.get(s, []):
                fh.write(L + "\n")
            fh.write("\n")
    for d in ("memcards", "logs", "snaps", "sstates", "cheats", "cache", "textures", "videos", "covers", "bios"):
        os.makedirs(os.path.join(TEST_PCSX2, d), exist_ok=True)

# ---------------------------------------------------------------- window capture

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32

def windows_of(pid):
    found = []
    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(hwnd, _):
        p = wt.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(p))
        if p.value == pid and user32.IsWindowVisible(hwnd):
            n = user32.GetWindowTextLengthW(hwnd)
            buf = ctypes.create_unicode_buffer(n + 1)
            user32.GetWindowTextW(hwnd, buf, n + 1)
            r = wt.RECT(); user32.GetClientRect(hwnd, ctypes.byref(r))
            found.append((r.right * r.bottom, hwnd, buf.value))
        return True
    user32.EnumWindows(cb, 0)
    return sorted(found, reverse=True)

class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG), ("biPlanes", wt.WORD),
                ("biBitCount", wt.WORD), ("biCompression", wt.DWORD), ("biSizeImage", wt.DWORD),
                ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG), ("biClrUsed", wt.DWORD),
                ("biClrImportant", wt.DWORD)]

def capture(hwnd, path):
    from PIL import Image
    r = wt.RECT(); user32.GetClientRect(hwnd, ctypes.byref(r))
    w, h = r.right, r.bottom
    if w <= 0 or h <= 0:
        return None
    hdc = user32.GetDC(hwnd)
    mdc = gdi32.CreateCompatibleDC(hdc)
    bmp = gdi32.CreateCompatibleBitmap(hdc, w, h)
    gdi32.SelectObject(mdc, bmp)
    user32.PrintWindow(hwnd, mdc, 3)          # PW_CLIENTONLY | PW_RENDERFULLCONTENT
    bi = BITMAPINFOHEADER(ctypes.sizeof(BITMAPINFOHEADER), w, -h, 1, 32, 0, 0, 0, 0, 0, 0)
    buf = ctypes.create_string_buffer(w * h * 4)
    gdi32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bi), 0)
    gdi32.DeleteObject(bmp); gdi32.DeleteDC(mdc); user32.ReleaseDC(hwnd, hdc)
    img = Image.frombuffer("RGB", (w, h), buf, "raw", "BGRX", 0, 1)
    img.save(path)
    px = img.convert("L").resize((64, 48))
    vals = list(px.getdata())
    return sum(vals) / len(vals)

# ---------------------------------------------------------------- run

# Stage markers, in boot order (taken from a run of the retail bootstrap):
#   the BIOS loads the ELF; the bootstrap reboots the IOP with IOPRP224.IMG and
#   loads its five IRX modules; ExecPS2 restarts the kernel a second time
#   ("Restart Without Memory Clear" #2); core.gt3 then reboots the IOP again and
#   loads its own module set, starting with MEMCLEAR.IRX; its main loop prints
#   Timezone= every frame.
MARKERS = [
    ("1 elf executing",   r"ELF .* with entry point at 0x[0-9A-Fa-f]+ is executing", 1),
    ("2 iop rebooted",    r"PlayStation 2 ======== Update reboot complete", 1),
    ("3 bootstrap irx",   r"loadmodule: fname cdrom0:PADMAN\.IRX", 1),
    ("4 execps2 restart", r"# Restart Without Memory Clear\.", 2),
    ("5 core.gt3 irx",    r"loadmodule: fname cdrom0:MEMCLEAR\.IRX", 1),
    ("6 gt3 main loop",   r"Timezone=", 1),
    ("! crash signs",     r"(?i)(TLB Miss|Unhandled|undefined \(|R5900 exception|Address Error)", 0),
]

CARD_NAME = "boottest-card.ps2"

def use_card(card):
    """Slot 2 of the private PCSX2: a fresh copy of `card`, or the blank Mcd002.ps2."""
    dst = os.path.join(TEST_PCSX2, "memcards", CARD_NAME)
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    elif os.path.exists(dst):
        os.remove(dst)
    if card:
        (shutil.copytree if os.path.isdir(card) else shutil.copy2)(card, dst)
    ini = os.path.join(TEST_PCSX2, "inis", "PCSX2.ini")
    with open(ini, encoding="utf-8", newline="") as fh:
        text = fh.read()
    text = re.sub(r"(?m)^Slot2_Filename[ \t]*=[^\r\n]*", "Slot2_Filename = " + (CARD_NAME if card else "Mcd002.ps2"), text)
    if card:
        # A boot-ELF override leaves PCSX2 without a game serial, and folder-card
        # auto-management would then hide every save folder (BASCUS-97102GAMEDATA too).
        text = re.sub(r"(?m)^McdFolderAutoManage[ \t]*=[^\r\n]*", "McdFolderAutoManage = false", text)
    with open(ini, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)

def run(elf, iso, seconds, tag, card=None):
    setup()
    use_card(card)
    out = os.path.join(ROOT, "build", "boottest", tag)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    log = os.path.join(out, "emulog.txt")
    exe = os.path.join(TEST_PCSX2, "pcsx2-qt.exe")
    cmd = [exe, "-batch", "-nogui", "-nofullscreen", "-logfile", log, "-elf", os.path.abspath(elf), "--", iso]
    print("running: %s" % " ".join('"%s"' % c if " " in c else c for c in cmd))
    p = subprocess.Popen(cmd, cwd=TEST_PCSX2)
    t0 = time.time()
    shots = []
    try:
        for t in [c for c in CAPTURE_AT if c <= seconds] + [seconds]:
            while time.time() - t0 < t:
                if p.poll() is not None:
                    break
                time.sleep(0.25)
            if p.poll() is not None:
                print("PCSX2 exited early (code %s) at %.1fs" % (p.returncode, time.time() - t0))
                break
            ws = windows_of(p.pid)
            if ws:
                path = os.path.join(out, "t%03d.png" % t)
                lum = capture(ws[0][1], path)
                shots.append((t, path, lum, ws[0][2]))
                print("  t=%3ds  window '%s'  mean luma %s" % (t, ws[0][2], "%.1f" % lum if lum is not None else "-"))
    finally:
        subprocess.run(["taskkill", "/PID", str(p.pid), "/T", "/F"], capture_output=True)
        time.sleep(1.0)

    text = open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""
    lines = text.splitlines()
    summ = ["elf %s" % elf, "iso %s" % iso] + (["card %s (slot 2)" % card] if card else []) + \
           ["seconds %d" % seconds, "log lines %d" % len(lines), ""]
    reached = True
    for name, pat, need in MARKERS:
        hits = [L for L in lines if re.search(pat, L)]
        ok = (len(hits) >= need) if need else (len(hits) == 0)
        if need and not ok:
            reached = False
        first = hits[need - 1 if need else 0].strip()[:130] if hits and len(hits) >= max(need, 1) else ""
        summ.append("%-4s %-18s %5d   %s" % ("ok" if ok else "FAIL", name, len(hits), first))
    summ.append("")
    summ.append("RESULT: %s" % ("core.gt3 reached its main loop" if reached else "did not reach core.gt3's main loop"))
    summ.append("")
    for t, path, lum, title in shots:
        summ.append("capture t=%3ds luma %s  %s" % (t, "%.1f" % lum if lum is not None else "-", os.path.basename(path)))
    open(os.path.join(out, "summary.txt"), "w", encoding="utf-8").write("\n".join(summ) + "\n")
    print("\n".join(summ))
    return reached

def main():
    global USER_PCSX2, TEST_PCSX2
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", default=DEFAULT_ELF)
    ap.add_argument("--iso", default=ISO, help="the GT3 A-Spec US disc image (default: local_paths.json)")
    ap.add_argument("--pcsx2", default=USER_PCSX2, help="your PCSX2 folder (default: local_paths.json)")
    ap.add_argument("--seconds", type=int, default=60)
    ap.add_argument("--tag", default=None)
    ap.add_argument("--baseline", action="store_true", help="run the retail bootstrap instead")
    ap.add_argument("--card", help="a PCSX2 memory card (folder or file) to put in slot 2 for the run")
    ap.add_argument("--copy", metavar="NAME", help="use its own private PCSX2 copy, build/pcsx2-test-NAME "
                    "(for boot tests that run at the same time)")
    ap.add_argument("--setup-only", action="store_true")
    a = ap.parse_args()
    USER_PCSX2 = a.pcsx2
    if a.copy:
        TEST_PCSX2 = os.path.join(ROOT, "build", "pcsx2-test-" + a.copy)
    if a.setup_only:
        setup(); return 0
    if not a.iso or not os.path.isfile(a.iso):
        sys.exit('no disc image: set "iso" in %s or pass --iso' % os.path.join(paths.REPO, "local_paths.json"))
    elf = paths.retail_elf() if a.baseline else a.elf
    tag = a.tag or ("retail" if a.baseline else "rebuilt")
    if a.card and not os.path.exists(a.card):
        sys.exit("no memory card at %s" % a.card)
    return 0 if run(elf, a.iso, a.seconds, tag, os.path.abspath(a.card) if a.card else None) else 1

if __name__ == "__main__":
    sys.exit(main())
