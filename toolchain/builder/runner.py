"""Runs one profile's build: checks what it needs, runs the target's build
command, streams its output, then puts the finished ELF where the profile
says.  The window and gt3build.py both use BuildJob."""
import hashlib
import os
import re
import shutil
import signal
import subprocess
import threading
import time

from . import BACKUP_DIR, NO_WINDOW, REPO_DIR, python_exe
from .model import BuildContext

MAX_ORIGINAL = 64 << 20          # larger files are never hashed as possible originals

PROGRESS = re.compile(r"^\[(\d+)/(\d+)\]\s")


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def classify(line):
    """The log colour of one line of build output."""
    s = line.strip()
    low = s.lower()
    if s.startswith("== "):
        return "stage"
    if PROGRESS.match(s):
        return "progress"
    if s.startswith("FAIL") or "error:" in low or " failed" in low or low.startswith(("error", "fatal")):
        return "error"
    if "warning:" in low or s.startswith("warn "):
        return "warn"
    return None


def cmdline(argv):
    return subprocess.list2cmdline(argv) if os.name == "nt" else " ".join(argv)


def resolve_output(path, target):
    """The profile's output path as a file path: environment variables and ~
    expanded, relative paths taken from the repository folder, and a folder
    (existing, or written with a trailing slash) given the executable's own
    file name."""
    p = os.path.expandvars(os.path.expanduser(path.strip().strip('"')))
    if not p:
        return ""
    trailing = p.endswith(("/", "\\"))
    p = os.path.normpath(os.path.join(REPO_DIR, p))
    if trailing or os.path.isdir(p):
        p = os.path.join(p, target.output_name)
    return p


def existing_output(path, target):
    """What is at an output path now: (None, "") nothing; ("other", why) not
    one of the target's executables, never replaced; ("original", what);
    ("build", "")."""
    if not os.path.exists(path):
        return None, ""
    if not os.path.isfile(path):
        return "other", "a folder"
    with open(path, "rb") as f:
        head = f.read(len(target.output_magic))
    if target.output_magic and head != target.output_magic:
        return "other", "not an ELF"
    if os.path.getsize(path) <= MAX_ORIGINAL:
        what = target.originals.get(sha256_file(path))
        if what:
            return "original", what
    return "build", ""


def install(artifact, output, target, log):
    """Copy the built ELF to `output`.  An original file found there is first
    copied to backups/<target>/ once; a file that is not one of the target's
    executables is never overwritten.  The copy goes through a temporary file
    in the same folder, so an interrupted copy never leaves a half-written ELF
    under the real name."""
    if os.path.normcase(os.path.abspath(artifact)) == os.path.normcase(os.path.abspath(output)):
        return
    state, what = existing_output(output, target)
    if state == "other":
        raise OSError("%s is %s, so the build will not replace it; pick another output path" % (output, what))
    folder = os.path.dirname(output)
    if not os.path.isdir(folder):
        os.makedirs(folder)
        log("Created the folder %s" % folder, "note")
    if state == "original":
        old = sha256_file(output)
        bdir = os.path.join(BACKUP_DIR, target.id)
        bk = os.path.join(bdir, "%s.original-%s" % (os.path.basename(output), old[:12].lower()))
        if not os.path.exists(bk):
            os.makedirs(bdir, exist_ok=True)
            shutil.copyfile(output, bk)
            log("Backed up the file it replaces (%s) to %s" % (what, bk), "note")
        else:
            log("The file it replaces is %s; a backup is already in %s" % (what, bk), "note")
    tmp = output + ".gt3builder-tmp"
    try:
        shutil.copyfile(artifact, tmp)
        os.replace(tmp, output)
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)
    if sha256_file(output) != sha256_file(artifact):
        raise OSError("the copy at %s does not match the build" % output)


class Result:
    def __init__(self, ok, message, output="", size=0, sha256="", seconds=0.0):
        self.ok = ok
        self.message = message
        self.output = output
        self.size = size
        self.sha256 = sha256
        self.seconds = seconds


class BuildJob:
    """One build.  run() blocks; stop() may be called from another thread.

    log(text, tag) gets every line.  tag: None, "cmd", "stage", "progress",
    "warn", "error", "ok" or "note".
    """

    def __init__(self, profile, registry, settings, log, jobs=None):
        self.profile = profile.copy()
        self.registry = registry
        self.settings = dict(settings)
        self.log = log
        self.jobs = jobs or os.cpu_count() or 1
        self._proc = None
        self._stopped = False
        self._lock = threading.Lock()

    def _fail(self, message, t0):
        self.log(message, "error")
        return Result(False, message, seconds=time.time() - t0)

    def run(self):
        t0 = time.time()
        p = self.profile
        target = self.registry.targets.get(p.target)
        if target is None:
            return self._fail("The profile builds %r, which no target plug-in provides." % p.target, t0)
        option = target.option(p.option)
        if option is None:
            return self._fail("%s has no option %r." % (target.name, p.option), t0)
        status = {sid: self.registry.detect(sid, self.settings) for sid in self.registry.needs(target, option.id)}
        ready, problems = self.registry.readiness(target, option.id, status)
        if not ready:
            for pr in problems:
                self.log("  " + pr, "error")
            return self._fail("%s cannot be built %s yet: set up what is missing first." % (
                target.name, option.label.lower()), t0)
        output = resolve_output(p.output, target)
        if not output:
            return self._fail("The profile has no output path.", t0)
        try:
            state, what = existing_output(output, target)
        except OSError as e:
            state, what = "other", "unreadable (%s)" % e.strerror
        if state == "other":
            return self._fail("%s is %s, so a build will not replace it; pick another output path." %
                              (output, what), t0)
        sdk = self.registry.sdks[option.sdk]
        self.log("Building %s with %s" % (target.name, sdk.label(status[sdk.id])), "stage")
        argv, cwd = target.command(option.id, BuildContext(python_exe(), self.jobs, status))
        self.log("> " + cmdline(argv), "cmd")
        env = dict(os.environ, PYTHONUNBUFFERED="1", PYTHONUTF8="1")
        with self._lock:
            if self._stopped:
                return self._fail("Stopped.", t0)
            try:
                self._proc = subprocess.Popen(
                    argv, cwd=cwd, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace",
                    bufsize=1, creationflags=NO_WINDOW, start_new_session=(os.name != "nt"))
            except OSError as e:
                return self._fail("Could not start the build: %s" % e, t0)
        for line in self._proc.stdout:
            if self._stopped:
                continue            # the killed compilers' failures are not news
            line = line.rstrip("\r\n")
            self.log(line, classify(line))
        rc = self._proc.wait()
        if self._stopped:
            return self._fail("Stopped.", t0)
        if rc != 0:
            return self._fail("The build failed (exit code %d)." % rc, t0)
        art = target.artifact(option.id)
        if not os.path.isfile(art):
            return self._fail("The build finished but did not leave %s." % art, t0)
        try:
            install(art, output, target, self.log)
        except OSError as e:
            return self._fail("Could not write %s: %s" % (output, e), t0)
        size, sha, secs = os.path.getsize(output), sha256_file(output), time.time() - t0
        msg = "Built %s with %s in %.1f s: %s (%s bytes)" % (
            target.output_name, sdk.label(status[sdk.id]), secs, output, format(size, ","))
        self.log(msg, "ok")
        return Result(True, msg, output, size, sha, secs)

    def stop(self):
        with self._lock:
            self._stopped = True
            proc = self._proc
        if proc is None or proc.poll() is not None:
            return
        if os.name == "nt":
            # the whole tree: the build script and the compilers it started
            subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)],
                           capture_output=True, creationflags=NO_WINDOW)
        else:
            os.killpg(proc.pid, signal.SIGTERM)
