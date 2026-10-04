"""GT3 Decomp Builder: builds the decompiled GT3 executables from saved profiles.

The repository's README.md is the manual.  Plug-ins: toolchain/targets/ (what
can be built) and toolchain/sdks/ (what it is built with), one file each.
"""
import os
import subprocess
import sys

TOOLCHAIN_DIR = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO_DIR = os.path.normpath(os.path.join(TOOLCHAIN_DIR, ".."))
SDK_DIR = os.path.join(REPO_DIR, "sdk")              # downloaded SDKs (not in the repository)
OUTPUT_DIR = os.path.join(REPO_DIR, "output")        # where new profiles put their ELF
BACKUP_DIR = os.path.join(REPO_DIR, "backups")       # originals a build replaced

APP_NAME = "GT3 Decomp Builder"

# Console programs started from the window (pythonw) would each open a console
# window without this.  Their own children share the hidden console.
NO_WINDOW = subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0


def python_exe():
    """The console Python next to the running one: builds print to a pipe, and
    pythonw.exe (which runs the window) has no console of its own."""
    exe = sys.executable
    if os.name == "nt" and os.path.basename(exe).lower() == "pythonw.exe":
        cand = os.path.join(os.path.dirname(exe), "python.exe")
        if os.path.isfile(cand):
            return cand
    return exe


def exe_name(name):
    return name + (".exe" if os.name == "nt" else "")


MAX_REPO_PATH = 100


def path_warning():
    """A warning when this folder's path is too long for the toolchain: GCC
    cannot open a file whose path passes Windows' 260 characters, and its own
    header paths add about 160 to this folder's."""
    if os.name == "nt" and len(REPO_DIR) > MAX_REPO_PATH:
        return ("This folder's path is %d characters long. GCC cannot open files whose path passes "
                "260 characters, so builds may fail: move the folder somewhere shorter (under %d "
                "characters), e.g. C:\\gt3-decomp." % (len(REPO_DIR), MAX_REPO_PATH))
    return None
