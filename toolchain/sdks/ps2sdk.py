"""EE GCC 14.2: the ps2dev toolchain for the PS2's Emotion Engine, in the
standalone Windows build of github.com/ThirteenAG/ps2sdk (pinned commit).
Only its ee/ folder is installed."""
import os
import shutil
import subprocess

from builder import NO_WINDOW, REPO_DIR, exe_name
from builder.download import unpack_zip
from builder.model import Download, Sdk, Status

PREFIX = "mips64r5900el-ps2-elf-"
TOOLS = ("gcc", "g++", "as", "ld", "objdump", "strip")
COMMIT = "057829f258b2fd8762ce5b05b3af3a585f3ffa42"
GCC_SHA256 = "A6400EAB9D71BF47B474E5E09AAC606C2529B3080C7542D9A2D282D7CF164E57"


def bin_dir(folder):
    """The folder with the compiler: `folder` itself, its ee/bin, or the ee/bin
    of a folder inside it (Explorer's "Extract all" nests a ZIP's folder)."""
    cands = [folder, os.path.join(folder, "ee", "bin")]
    if os.path.isdir(folder):
        cands += [os.path.join(folder, d, "ee", "bin") for d in sorted(os.listdir(folder))]
    for b in cands:
        if os.path.isfile(os.path.join(b, exe_name(PREFIX + "gcc"))):
            return os.path.abspath(b)
    return None


def gcc_version(b):
    """(version, None) or (None, why it does not run)."""
    env = dict(os.environ)
    env["PATH"] = b + os.pathsep + env.get("PATH", "")
    try:
        r = subprocess.run([os.path.join(b, exe_name(PREFIX + "gcc")), "-dumpfullversion"],
                           capture_output=True, text=True, timeout=30, env=env, creationflags=NO_WINDOW)
    except (OSError, subprocess.TimeoutExpired) as e:
        return None, "the compiler in %s does not run: %s" % (b, e)
    if r.returncode == 0 and r.stdout.strip():
        return r.stdout.strip(), None
    code = r.returncode & 0xFFFFFFFF
    if code in (0xC0000135, 0xC0000139):
        return None, "the compiler in %s lacks a DLL it needs (0x%08X)" % (b, code)
    return None, "the compiler in %s does not run (exit code 0x%08X)" % (b, code)


class Ps2sdk(Sdk):
    id = "ps2sdk"
    name = "EE GCC 14.2 (ps2dev toolchain)"
    summary = "the modern compiler, assembler and linker for the PS2's Emotion Engine"
    source = "github.com/ThirteenAG/ps2sdk"
    home = "ps2sdk"
    downloads = [Download("https://github.com/ThirteenAG/ps2sdk/archive/%s.zip" % COMMIT, None, 310 << 20)]
    setting = "ps2sdk"
    setting_label = "the ps2sdk / ps2dev folder (the one that has ee\\bin)"

    def can_install(self):
        return os.name == "nt"          # the archive holds Windows programs

    def candidates(self, settings):
        """Folders to look in, in order.  A folder chosen in the window is the
        only one."""
        chosen = settings.get(self.setting)
        if chosen:
            return [chosen]
        out = [self.path(), os.path.join(REPO_DIR, "..", "ps2sdk-main"),
               os.environ.get("PS2SDK_HOME"), os.environ.get("PS2DEV")]
        on_path = shutil.which(PREFIX + "gcc")
        if on_path:
            out.append(os.path.dirname(on_path))
        return [c for c in out if c]

    def detect(self, settings):
        for folder in self.candidates(settings):
            b = bin_dir(folder)
            if b is None:
                continue
            missing = [PREFIX + t for t in TOOLS if not os.path.isfile(os.path.join(b, exe_name(PREFIX + t)))]
            if missing:
                return Status(False, location=b, problems=["%s has no %s" % (b, ", ".join(missing))])
            version, why = gcc_version(b)
            if version is None:
                return Status(False, location=b, problems=[why])
            return Status(True, version, b)
        if settings.get(self.setting):
            return Status(False, problems=["no EE GCC in the chosen folder %s" % settings[self.setting]])
        return Status(False, problems=["not installed"])

    def label(self, status=None):
        if status and status.version:
            return "EE GCC %s (ps2dev)" % status.version
        return self.name

    def unpack(self, files, staging, ctx):
        unpack_zip(files[0], os.path.join(staging, "ee"), ctx, "ps2sdk-%s/ee/" % COMMIT)
        ctx.verify(os.path.join(staging, "ee", "bin", PREFIX + "gcc.exe"), GCC_SHA256)


SDK = Ps2sdk()
