"""Compile a source file with the matching compiler (the retail build's).

    python tools/mcc.py <src> [-o out.o] [-S] [-g] [-- extra flags]
    python tools/mcc.py --cc 991111-01 <src> ... # the same with the reference compiler
    python tools/mcc.py --flags <src>          # print the flags that would be used

Compiler: sdk/ee-gcc-2.9-991111b-divs, SCE ee-gcc 2.9-ee-991111b (native
Windows build) with div.s made ineligible for delay slots as in the retail
compiler (tools/patch_divs.py creates it from the pristine
sdk/ee-gcc-2.9-991111b; see MATCHING.md).  The compilers live in the
repository's sdk/ folder (tools/paths.py), where the GT3 Decomp Builder
downloads them.  Per-file flags come from ref/compile_flags.json.  Include
path: include/ then the compiler's own gcc-2.95 system headers (include/ of
the compiler: stdarg.h, stddef.h, va-mips.h).

Secondary (reference) compiler, `--cc 991111-01`: Sony's Linux ee-gcc
2.9-ee-991111-01, the revision the retail build most likely used
(sdk/ee-gcc-2.9-991111-01, MATCHING.md).  It confirms source accuracy;
it is not the default.  The default driver preprocesses, Sony's Linux
cc1 / cc1plus compiles under tools/linuxcc.py (needs `pip install unicorn`),
and the assembly goes through the same passes and assembler as the default.
It does not need the patched copy: without it, the pristine package's driver
does the preprocessing and assembling.

Importable: compile(rel, out, asm=False, debug=False, extra=()) -> (ok, stderr);
set mcc.use_cc = "991111-01" first for the reference compiler.
"""
import fnmatch, json, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import paths

CCDIR = paths.sdk("ee-gcc-2.9-991111b-divs")
# The reference compiler (use_cc) takes only the driver, the preprocessor, the
# assemblers and the headers from CCDIR, and the pristine package has the same
# ones, so it works without patch_divs.py.  The default compiler needs the
# patched copy (compile() says so).
DIVS_MISSING = not os.path.isdir(CCDIR)
if DIVS_MISSING:
    CCDIR = paths.sdk("ee-gcc-2.9-991111b")
CC = os.path.join(CCDIR, "bin", "ee-gcc.exe")


def _from_root(path):
    """path relative to ROOT (the compiler runs there) when it can be, with
    forward slashes: egcs's cpp finds a quoted #include's directory by '/'."""
    try:
        path = os.path.relpath(path, ROOT)
    except ValueError:                  # another drive
        pass
    return path.replace("\\", "/")


# Relative, as is the source path given to the compiler: the preprocessor's
# line markers carry these paths into cc1, and 991111b's output depends on
# their length (the C++ exception tables of src/imageloader_source.cpp come
# out different).  Relative paths give every checkout the same objects.
INCLUDES = [_from_root(os.path.join(ROOT, "include")), _from_root(os.path.join(CCDIR, "include"))]

# Secondary compilers: name -> directory with cc1 / cc1plus (Linux ELF, run
# under tools/linuxcc.py).  use_cc selects one for compile(); None = default.
SECONDARY = {
    "991111-01": paths.sdk("ee-gcc-2.9-991111-01", "lib", "gcc-lib", "ee", "2.9-ee-991111-01"),
}
LINUXCC = os.path.join(HERE, "linuxcc.py")
use_cc = None

_rules = None

def rule_for(rel):
    """The first ref/compile_flags.json rule whose glob matches rel (or {})."""
    global _rules
    if _rules is None:
        _rules = json.load(open(os.path.join(ROOT, "ref", "compile_flags.json")))["rules"]
    rel = rel.replace("\\", "/")
    for r in _rules:
        if fnmatch.fnmatch(rel, r["glob"]):
            return r
    return {}

def flags_for(rel):
    return list(rule_for(rel).get("flags", ["-O2"]))

def cc_for(rel):
    """The compiler driver for rel: sdk/<rule "cc">/bin/ee-gcc.exe, or the
    default ee-gcc 2.9-ee-991111b.  Only the C/C++ -> .s step uses it; the
    assembly is always assembled by the default toolchain (plus asm_passes)."""
    name = rule_for(rel).get("cc")
    return paths.sdk(name, "bin", "ee-gcc.exe") if name else CC

SN_AS = os.path.join(CCDIR, "ee", "bin", "Ps2EeAs.exe")

def assembler_for(rel):
    """"sn" when the file's rule says "as": "sn" (SN Systems ps2eeas 1.9.25,
    bundled as ee/bin/Ps2EeAs.exe), else "gnu" (the driver's GNU as 2.9-ee)."""
    return rule_for(rel).get("as", "gnu")

def gvalue(rel):
    """The -G value of rel's flags (8 when not given)."""
    g = 8
    for f in flags_for(rel):
        if f.startswith("-G") and f[2:].isdigit():
            g = int(f[2:])
    return g

def asm_passes(rel, path):
    """The retail assemblers' behaviour that the bundled GNU as lacks, applied
    to egcs's assembly output: div.s padding (not needed with the SN
    assembler, which pads by itself), no bogus short-loop padding
    (label_frags), and GAS delay-slot swapping where the file's rule asks for
    it ("asm_pass": "gas_swap", the Sony SDK libraries; see tools/sdkas.py)."""
    if assembler_for(rel) != "sn":
        fix_fpu_div_hazard(path)
        label_frags(path)
    if rule_for(rel).get("asm_pass") == "gas_swap":
        import sdkas
        sdkas.apply(path)

def label_frags(path):
    """`.p2align 2` in front of every local label of egcs's output (no bytes:
    the code is word-aligned).

    The bundled GNU as 2.9-ee-991111b pads backward branches that close an
    R5900 "short loop" (six instructions or fewer).  It measures the loop with
    frag-relative addresses, so a branch back to a label that sits six or more
    instructions into a frag ended by an alignment directive (egcs's
    `.p2align 3` loop alignment right after the label) is mistaken for a short
    loop and gets four nops - BigNum::gcd 0x01005BC0 closes a 317-instruction
    loop that way.  Retail has the same label/alignment layout there and no
    padding, so its assembler did not do this.  A `.p2align 2` before every
    label starts a new frag at it and keeps the check from misfiring; across
    every src/ and sdk/ function it changes nothing else.  egcs pads real
    short loops itself (explicit nops), so nothing relies on the assembler's
    check.  Returns the number of directives added."""
    lines = open(path, encoding="latin1").read().split("\n")
    out, n = [], 0
    for L in lines:
        s = L.strip()
        if s.startswith("$L") and s.endswith(":") and s[2:-1].isdigit():
            out.append("\t.p2align 2")
            n += 1
        out.append(L)
    if n:
        open(path, "w", encoding="latin1", newline="\n").write("\n".join(out))
    return n

COP1_MNEMONICS = {"mtc1", "mfc1", "ctc1", "cfc1", "lwc1", "swc1", "l.s", "s.s", "li.s"}

def _is_cop1(mn):
    return mn in COP1_MNEMONICS or mn.endswith((".s", ".w")) or mn.startswith(("c.", "cvt."))

def fix_fpu_div_hazard(path):
    """Two nops before every div.s whose previous instruction is a COP1
    instruction, as the retail file has them.  All three div.s of the retail
    bootstrap follow a COP1 op and all three are padded (0x01000D40,
    0x01000D88, 0x0100BAF4); its two sqrt.s are not.  Our cc1 emits a plain
    div.s and none of the bundled assemblers inserts the padding (an R5900
    FPU hazard workaround).  The retail compiler also never put a div.s in a
    delay slot, so its divsf3 most likely emitted the nops itself (length 3);
    the patched cc1 (tools/patch_divs.py) has the delay-slot half of that
    and this pass the nops.  No div.s is ever in a delay slot, so the
    padding is behaviour-neutral.  Returns the number of insertions."""
    lines = open(path, encoding="latin1").read().split("\n")
    out, prev, n = [], None, 0
    for L in lines:
        s = L.strip()
        tok = s.split(None, 1)[0] if s else ""
        is_insn = tok and not tok.endswith(":") and not tok.startswith((".", "#", "$L"))
        if is_insn and tok == "div.s" and prev is not None and _is_cop1(prev):
            out.append("\tnop")
            out.append("\tnop")
            n += 1
        out.append(L)
        if is_insn:
            prev = tok
    if n:
        open(path, "w", encoding="latin1", newline="\n").write("\n".join(out))
    return n

def secondary_asm(rel, src, out_s, flags):
    """C/C++ -> assembly with the reference compiler use_cc: preprocess with
    the default driver (same flags and include path), then run Sony's Linux
    cc1 / cc1plus under tools/linuxcc.py with the options the driver would pass
    on (-O, -G, -f, -m, -g, -W)."""
    cxx = rel.endswith((".cpp", ".cc", ".cxx"))
    pre = out_s + (".ii" if cxx else ".i")
    cmd = [CC] + flags
    for inc in INCLUDES:
        cmd += ["-I", inc]
    r = subprocess.run(cmd + ["-E", src, "-o", _from_root(os.path.abspath(pre))], capture_output=True, text=True,
                       cwd=ROOT, env=paths.egcs_env(CC))
    err = (r.stderr or "") + (r.stdout or "")
    if r.returncode != 0:
        return False, err
    cc1 = os.path.join(SECONDARY[use_cc], "cc1plus" if cxx else "cc1")
    cc1_flags = [f for f in flags if f.startswith(("-O", "-G", "-f", "-m", "-g", "-W", "-w"))]
    base = os.path.splitext(os.path.basename(rel))[0] + (".cc" if cxx else ".c")
    r = subprocess.run([sys.executable, LINUXCC, cc1, pre, "-quiet", "-dumpbase", base] + cc1_flags
                       + ["-o", out_s], capture_output=True, text=True, cwd=ROOT)
    err += (r.stderr or "") + (r.stdout or "")
    try:
        os.remove(pre)
    except OSError:
        pass
    return r.returncode == 0 and os.path.exists(out_s), err

def missing():
    """Why compile() cannot run with the current use_cc, or None."""
    if not os.path.isfile(CC):
        return "missing %s: the GT3 Decomp Builder downloads it (README.md, Downloads)" % CC
    if use_cc is None:
        if DIVS_MISSING:
            return "missing sdk/ee-gcc-2.9-991111b-divs: run python tools/patch_divs.py"
        return None
    # <package>/lib/gcc-lib/ee/<version> -> <package>/rootfs/lib (tools/linuxcc.py)
    rootfs = os.path.join(SECONDARY[use_cc], "..", "..", "..", "..", "rootfs", "lib")
    for f in [os.path.join(SECONDARY[use_cc], n) for n in ("cc1", "cc1plus")] + \
             [os.path.normpath(os.path.join(rootfs, n)) for n in ("ld-linux.so.2", "libc.so.6")]:
        if not os.path.isfile(f):
            return "missing %s: the GT3 Decomp Builder downloads it (README.md, Downloads)" % f
    import importlib.util
    if importlib.util.find_spec("unicorn") is None:
        return "the reference compiler runs under Unicorn: pip install unicorn"
    return None

def compile(rel, out, asm=False, debug=False, extra=()):
    why = missing()
    if why:
        return False, why + "\n"
    rel = rel.replace("\\", "/")
    src = rel if os.path.isabs(rel) else _from_root(os.path.join(ROOT, rel))     # see INCLUDES
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    flags = flags_for(rel) + (["-g"] if debug else []) + list(extra)
    cmd = [cc_for(rel)] + flags
    for inc in INCLUDES:
        cmd += ["-I", inc]
    secondary = use_cc is not None and not rel.endswith((".S", ".s"))
    if secondary and asm:
        ok, err = secondary_asm(rel, src, out, flags)
        if ok:
            asm_passes(rel, out)
        return ok, err
    if rel.endswith((".S", ".s")):
        cmd[0] = CC                                 # assembly: the default toolchain
    if rel.endswith((".S", ".s")) or asm:
        cmd += (["-S"] if asm and not rel.endswith((".S", ".s")) else ["-c"]) + [src, "-o", _from_root(os.path.abspath(out))]
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT, env=paths.egcs_env(cmd[0]))
        ok = r.returncode == 0 and os.path.exists(out)
        if ok and asm and not rel.endswith((".S", ".s")):
            asm_passes(rel, out)
        return ok, (r.stderr or "") + (r.stdout or "")
    # C / C++ to an object: compile to assembly, apply the retail assembler's
    # div.s padding, then assemble (the driver runs only `as` for a .s input).
    tmp = out + ".tmp.s"
    if secondary:
        ok, err = secondary_asm(rel, src, tmp, flags)
        if not ok:
            return False, err
    else:
        r = subprocess.run(cmd + ["-S", src, "-o", _from_root(os.path.abspath(tmp))], capture_output=True, text=True,
                           cwd=ROOT, env=paths.egcs_env(cmd[0]))
        err = (r.stderr or "") + (r.stdout or "")
        if r.returncode != 0 or not os.path.exists(tmp):
            return False, err
    asm_passes(rel, tmp)
    if assembler_for(rel) == "sn":
        r2 = subprocess.run([SN_AS, "-G%d" % gvalue(rel), "-o", out, tmp],
                            capture_output=True, text=True, cwd=ROOT)
    else:
        r2 = subprocess.run([CC] + flags_for(rel) + list(extra) + ["-c", _from_root(os.path.abspath(tmp)),
                             "-o", _from_root(os.path.abspath(out))],
                            capture_output=True, text=True, cwd=ROOT, env=paths.egcs_env(CC))
    err += (r2.stderr or "") + (r2.stdout or "")
    ok = r2.returncode == 0 and os.path.exists(out)
    try:
        os.remove(tmp)
    except OSError:
        pass
    return ok, err

def main():
    global use_cc
    args = sys.argv[1:]
    if args and args[0] == "--flags":
        print(" ".join(flags_for(args[1])))
        return 0
    if args and args[0] == "--cc":
        use_cc = None if args[1] == "default" else args[1]
        if use_cc is not None and use_cc not in SECONDARY:
            sys.exit("unknown compiler %s (known: default, %s)" % (args[1], ", ".join(SECONDARY)))
        args = args[2:]
    extra = []
    if "--" in args:
        i = args.index("--")
        args, extra = args[:i], args[i + 1:]
    asm = "-S" in args
    debug = "-g" in args
    args = [a for a in args if a not in ("-S", "-g")]
    out = None
    if "-o" in args:
        i = args.index("-o")
        out = args[i + 1]
        del args[i:i + 2]
    src = args[0]
    if out is None:
        base = os.path.join(ROOT, "build", "match", src.replace("/", "__").replace("\\", "__"))
        out = base + (".s" if asm else ".o")
    ok, err = compile(src, out, asm=asm, debug=debug, extra=extra)
    if err.strip():
        sys.stderr.write(err)
    print(("OK   " if ok else "FAIL ") + out)
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())
