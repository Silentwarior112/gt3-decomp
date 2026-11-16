"""Where the core decomp's tools find what is not in the source tree.

All of it is relative to this checkout:

    <repo>/sdk/                     toolchains, compilers and m2c (README.md, "Downloads")
    core/baserom/BASE_SCUS-97102    the main executable of GT3 A-Spec US as an ELF:
                                    core.gt3's sections in one PT_LOAD at 0x00100000
    <repo>/local_paths.json         this PC's paths (PCSX2, disc image)

Environment overrides: GT3_SDK_DIR, GT3_CORE_ELF; for the EE toolchain, see
ee_bin().  Neither the SDKs nor the game file is part of the repository.
"""
import json
import os
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(TOOLS, ".."))          # core/
REPO = os.path.normpath(os.path.join(ROOT, ".."))           # the repository
REF = os.path.join(ROOT, "ref")
SDK = os.path.abspath(os.environ.get("GT3_SDK_DIR") or os.path.join(REPO, "sdk"))
RETAIL_ELF = os.path.abspath(os.environ.get("GT3_CORE_ELF")
                             or os.path.join(ROOT, "baserom", "BASE_SCUS-97102"))
RETAIL_SHA256 = "D18C002E106E52A65D5362F6196A5BC2EFB5ECD11078CFC5AC601AD1A6CB7235"
EE_PREFIX = "mips64r5900el-ps2-elf-"
EXE = ".exe" if os.name == "nt" else ""


def sdk(*parts):
    """A path inside the SDK folder, e.g. sdk("m2c")."""
    return os.path.join(SDK, *parts)


def retail_elf():
    """The main executable's ELF, or exit saying how to provide it."""
    if not os.path.isfile(RETAIL_ELF):
        sys.exit("missing %s\n(the GT3 A-Spec US main executable as an ELF: core.gt3's payload "
                 "at 0x00100000). GT3_CORE_ELF overrides the path." % RETAIL_ELF)
    return RETAIL_ELF


def ee_bin(arg=None):
    """The EE toolchain's bin folder, or None: arg when given; else <repo>/sdk/ps2sdk,
    then ps2sdk-main next to the repository, then $PS2SDK_HOME and $PS2DEV."""
    cands = [arg] if arg else [sdk("ps2sdk"), os.path.join(REPO, "..", "ps2sdk-main"),
                               os.environ.get("PS2SDK_HOME"), os.environ.get("PS2DEV")]
    for c in cands:
        if not c:
            continue
        for b in (c, os.path.join(c, "ee", "bin")):
            if os.path.isfile(os.path.join(b, EE_PREFIX + "gcc" + EXE)):
                return os.path.normpath(os.path.abspath(b))
    return None


def ld_path(p):
    """p as written inside a linker script or an @response file: relative to
    ROOT (the linker runs there) with forward slashes.  ld splits both at
    spaces, so an absolute path in a folder like "gt3-decomp (1)" breaks them;
    the build folders are inside ROOT."""
    try:
        p = os.path.relpath(p, ROOT)
    except ValueError:                  # another drive
        pass
    return p.replace("\\", "/")


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
