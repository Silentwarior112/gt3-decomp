"""Where the bootstrap's tools find what is not in the source tree.

All of it is relative to this checkout, and nothing is specific to one PC:

    <repo>/sdk/                 toolchains and compilers (the GT3 Decomp Builder
                                downloads them; README.md, "Downloads")
    bootstrap/baserom/SCUS_971.02   your retail SCUS_971.02 (the builder copies it
                                there from your file or your disc image)
    <repo>/local_paths.json     this PC's paths for the PCSX2 boot test, e.g.
                                {"pcsx2": "C:/PCSX2", "iso": "D:/GT3 (USA).iso"}

Environment overrides: GT3_SDK_DIR, GT3_BOOTSTRAP_ELF; for the EE toolchain,
see ee_bin().  Neither the SDKs, the retail file nor local_paths.json is part
of the repository.
"""
import json
import os
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(TOOLS, ".."))          # bootstrap/
REPO = os.path.normpath(os.path.join(ROOT, ".."))           # the repository
SDK = os.path.abspath(os.environ.get("GT3_SDK_DIR") or os.path.join(REPO, "sdk"))
RETAIL_ELF = os.path.abspath(os.environ.get("GT3_BOOTSTRAP_ELF")
                             or os.path.join(ROOT, "baserom", "SCUS_971.02"))
RETAIL_SHA256 = "E60A8821D01051CECE0EDEDCD600BCA0A79FB460BAA2A5BABEDD72B3D18E5518"
EE_PREFIX = "mips64r5900el-ps2-elf-"
EXE = ".exe" if os.name == "nt" else ""


def sdk(*parts):
    """A path inside the SDK folder, e.g. sdk("ee-gcc-2.9-991111b")."""
    return os.path.join(SDK, *parts)


def retail_elf():
    """The retail SCUS_971.02, or exit saying how to provide it."""
    if not os.path.isfile(RETAIL_ELF):
        sys.exit("missing %s\n(the retail SCUS_971.02 of a GT3 A-Spec US disc, SCUS-97102). The GT3 "
                 "Decomp Builder puts it there from your file or disc image; GT3_BOOTSTRAP_ELF "
                 "overrides the path." % RETAIL_ELF)
    return RETAIL_ELF


def ee_bin(arg=None):
    """The EE toolchain's bin folder, or None.  arg (a bin folder, or a folder
    that has ee/bin) when given; else <repo>/sdk/ps2sdk, then ps2sdk-main next
    to the repository, then $PS2SDK_HOME and $PS2DEV."""
    if arg:
        cands = [arg]
    else:
        cands = [sdk("ps2sdk"), os.path.join(REPO, "..", "ps2sdk-main"),
                 os.environ.get("PS2SDK_HOME"), os.environ.get("PS2DEV")]
    for c in cands:
        if not c:
            continue
        for b in (c, os.path.join(c, "ee", "bin")):
            if os.path.isfile(os.path.join(b, EE_PREFIX + "gcc" + EXE)):
                return os.path.normpath(os.path.abspath(b))
    return None


def ee_tool(name, bin_dir=None):
    """The path of an EE toolchain program, e.g. ee_tool("objdump")."""
    b = bin_dir or ee_bin()
    if b is None:
        sys.exit("no EE toolchain (%sgcc): the GT3 Decomp Builder downloads it to %s, or set "
                 "PS2SDK_HOME" % (EE_PREFIX, sdk("ps2sdk")))
    return os.path.join(b, EE_PREFIX + name + EXE)


EGCS_HEADERS = ("new", "new.h", "typeinfo", "exception")   # gcc-2.95.2's gcc/cp/inc, in <egcs>/include
_EGCS_ENV = {}


def egcs_env(cc):
    """The environment for SCE ee-gcc 2.9's driver `cc` (<egcs>/bin/ee-gcc.exe),
    run with cwd=ROOT.  The driver starts cpp / cc1 / cc1plus / as by its own
    absolute path and names its temporary files after TMP, both unquoted, so
    a space in either ("gt3-decomp (1)", a user name with a space) splits its
    command lines.  GCC_EXEC_PREFIX relative to ROOT and a temporary folder
    under build/ keep them free of both; the output does not change.  A
    patched copy (-divs) made from a compiler without the C++ headers gets
    them from the pristine one, at its own path (the output depends on it)."""
    egcs = os.path.dirname(os.path.dirname(os.path.abspath(cc)))
    if egcs in _EGCS_ENV:
        return _EGCS_ENV[egcs]
    for h in EGCS_HEADERS if os.path.basename(egcs) == "ee-gcc-2.9-991111b-divs" else ():
        mine, pristine = os.path.join(egcs, "include", h), sdk("ee-gcc-2.9-991111b", "include", h)
        if not os.path.isfile(mine) and os.path.isfile(pristine):
            import shutil
            shutil.copyfile(pristine, mine)
    lib = os.path.join(egcs, "lib", "gcc-lib")
    try:
        lib = os.path.relpath(lib, ROOT)
    except ValueError:                  # the SDK on another drive: its short (8.3) name
        lib = _short_path(lib)
    tmp = os.path.join(ROOT, "build", "egcs-tmp")
    os.makedirs(tmp, exist_ok=True)
    env = dict(os.environ)
    env["GCC_EXEC_PREFIX"] = lib.replace("\\", "/") + "/"
    env["TMP"] = env["TEMP"] = env["TMPDIR"] = os.path.relpath(tmp, ROOT)
    _EGCS_ENV[egcs] = env
    return env


def _short_path(p):
    if os.name != "nt":
        return p
    import ctypes
    buf = ctypes.create_unicode_buffer(32768)
    n = ctypes.windll.kernel32.GetShortPathNameW(p, buf, len(buf))
    return buf.value if 0 < n < len(buf) else p


def local(key, default=None):
    """This PC's setting `key` from <repo>/local_paths.json, or default."""
    try:
        with open(os.path.join(REPO, "local_paths.json"), encoding="utf-8") as f:
            return json.load(f).get(key, default)
    except (OSError, ValueError):
        return default
