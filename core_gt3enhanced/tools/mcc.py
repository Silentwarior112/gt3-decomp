"""Compile a core source file with the matching compiler (the original's).

    python tools/mcc.py <src> [-o out.o] [-S] [-g] [-- extra compiler flags, e.g. -dG -dl]
    python tools/mcc.py --flags <src>          # the flags that would be used

Compiler: sdk/ee-gcc-2.9-991111b-divs, SCE ee-gcc 2.9-ee-991111b with div.s
made ineligible for delay slots as in the retail compiler (the bootstrap's
tools/patch_divs.py makes it; bootstrap/MATCHING.md), else the pristine
sdk/ee-gcc-2.9-991111b.  Per-file flags: ref/compile_flags.json, the first
rule whose glob matches.  A rule's "asm_pass" (a name or a list) compiles
the object through assembly with those passes applied: "div_nops" (the
retail compiler's two nops before every div.s, see div_nops()), "label_frags"
(no bogus short-loop padding by the assembler, see label_frags()), "gas_swap"
(the Sony SDK libraries' delay-slot swapping, see gas_swap()), "eh_regions"
(the exception table written from the code's regions, less the rule's
"eh_drop" entries: see eh_regions()).  The file is preprocessed into the output folder
first and compiled from there, with paths relative to core/: egcs names
anonymous namespaces after the file it compiles (no temporary file under the
user's profile), and its output depends on the length of the paths in the
line markers (and of the output path: the exception tables, eh_regions()).

Importable: compile(rel, out, asm=False, debug=False, extra=()) -> (ok, messages);
flags_for(rel).
"""
import fnmatch
import json
import os
import subprocess
import sys

import paths

ROOT = paths.ROOT
CCDIR = paths.sdk("ee-gcc-2.9-991111b-divs")
if not os.path.isdir(CCDIR):
    CCDIR = paths.sdk("ee-gcc-2.9-991111b")
CC = os.path.join(CCDIR, "bin", "ee-gcc" + paths.EXE)
FLAGS = os.path.join(paths.REF, "compile_flags.json")
SECONDARY = ()          # other compilers score.py --cc may use (the bootstrap has 991111-01)
use_cc = None
_rules = None


def _rel(p):
    try:
        p = os.path.relpath(p, ROOT)
    except ValueError:
        pass
    return p.replace("\\", "/")


def rule_for(rel):
    """The first ref/compile_flags.json rule whose glob matches rel, or None."""
    global _rules
    if _rules is None:
        _rules = json.load(open(FLAGS))["rules"]
    for r in _rules:
        if fnmatch.fnmatch(rel, r["glob"]):
            return r
    return None


def flags_for(rel):
    r = rule_for(rel)
    return list(r["flags"]) if r else ["-O2", "-G0", "-ffast-math"]


def asm_passes_for(rel):
    r = rule_for(rel)
    p = (r or {}).get("asm_pass", [])
    p = [p] if isinstance(p, str) else list(p)
    # every C++ file's exception table from its code's regions (eh_regions):
    # egcs 991111's remove_handler drops a region's entry depending on the
    # heap layout (header bytes, path lengths), so the same source could give
    # two tables (T113 284 / 296, T157: ref/data/u054/notes.md); the rebuilt
    # table is the original's for every object (a lost retail entry: the
    # rule's "eh_drop").  A rule opts out with "no_eh_regions": true.
    if rel.endswith((".cpp", ".cc")) and "eh_regions" not in p and not (r or {}).get("no_eh_regions"):
        p.insert(0, "eh_regions")
    return p


def eh_drop_for(rel):
    """The rule's "eh_drop" list (eh_regions())."""
    return list((rule_for(rel) or {}).get("eh_drop", []))


def eh_regions(path, drop=()):
    """The exception table written from the code's regions (rule "asm_pass":
    "eh_regions"): one entry ($LEHB<n>, $LEHE<n>, $L<n>) for every region
    $LEHB<n> of the code whose handler label $L<n> is defined, in code order,
    less the entries named in `drop` ("<function>:<k>", the k-th region of
    that function in code order, 1-based; the rule's "eh_drop").

    egcs 991111 can lose a cleanup region's entry (the code keeps the region
    and its handler): remove_handler (except.c; its only caller is flow.c's
    delete_block, for an unreachable handler block) takes a handler label
    rtx and removes every handler with that pointer from the regions of the
    whole file (function_eh_regions is cleared only after the table is
    written), while each function's RTL is freed and its memory reused; an
    earlier function's handler whose label rtx sat where the deleted label
    now is loses its entry.  Which region, if any, depends on the heap
    layout: every byte of the .ii (header edits, the line markers' path
    lengths) and the .ii's own name (the output path), so the score build
    and the link could differ.  The original's compile lost entries the same
    way (T157: RaceOdometer's first region), with its own layout: "eh_drop"
    names them.  Evidence: ref/data/u054/notes.md (the reference compiler
    traced; 1912 entries of 206 files checked: every entry is $L<region>,
    every region with a handler label has one).  Returns the number of
    entries changed."""
    import re
    lines = open(path, encoding="latin1").read().split("\n")
    ent = re.compile(r"^\s*\.ent\s+(\S+)")
    fn, regions, labels, count = None, [], set(), {}
    for L in lines:
        m = ent.match(L)
        if m:
            fn = m.group(1)
        elif L.startswith("$LEHB") and L.endswith(":") and L[5:-1].isdigit():
            count[fn] = count.get(fn, 0) + 1
            regions.append((int(L[5:-1]), "%s:%d" % (fn, count[fn])))
        elif L.startswith("$L") and L.endswith(":") and L[2:-1].isdigit():
            labels.add(int(L[2:-1]))
    if "__EXCEPTION_TABLE__:" not in lines:
        return 0
    top = lines.index("__EXCEPTION_TABLE__:")
    end = top + 1
    while end < len(lines) and not lines[end].startswith("$LRTH"):
        end += 1
    if end == len(lines):
        return 0
    drop = set(drop)
    old = [L for L in lines[top + 1:end] if L.strip()]
    new = []
    for n, key in regions:
        if n in labels and key not in drop:
            new += ["\t.word\t$LEHB%d" % n, "\t.word\t$LEHE%d" % n, "\t.word\t$L%d" % n, ""]
    if [L for L in new if L] == old:
        return 0
    lines[top + 1:end] = new
    open(path, "w", encoding="latin1", newline="\n").write("\n".join(lines))
    return len(set(old) ^ set(L for L in new if L)) // 3 or 1


def div_nops(path):
    """`nop; nop` in front of every compiled div.s (rule "asm_pass": "div_nops").

    The retail compiler's divsf3 pattern emitted the R5900 FPU hazard padding
    itself (as Sony's 991111-01 does: `nop; nop; div.s`, never in a delay
    slot); our patched cc1 emits a bare div.s.  In the core image 970 of 976
    div.s carry the two nops, after any instruction (also after andi, daddu,
    a branch-likely's delay slot); the 6 without are hand-written assembly
    (u124's fastmath, 0x00257EFC).  So: every div.s that cc1 emitted in
    reorder mode, none inside #APP/#NO_APP (inline or top-level asm).  The
    bootstrap's fix_fpu_div_hazard (pad only after a COP1 instruction) gives
    the same result on its three div.s.  Returns the number of insertions."""
    lines = open(path, encoding="latin1").read().split("\n")
    out, n, app, noreorder = [], 0, False, False
    for L in lines:
        s = L.strip()
        if s == "#APP":
            app = True
        elif s == "#NO_APP":
            app = False
        elif not app and s.startswith(".set"):
            if s.split()[-1] == "noreorder":
                noreorder = True
            elif s.split()[-1] == "reorder":
                noreorder = False
        elif not app and not noreorder and s.split(None, 1)[:1] == ["div.s"]:
            out.append("\tnop")
            out.append("\tnop")
            n += 1
        out.append(L)
    if n:
        open(path, "w", encoding="latin1", newline="\n").write("\n".join(out))
    return n


def label_frags(path):
    """`.p2align 2` in front of every local label `$L<n>:` (rule "asm_pass":
    "label_frags"; no bytes: the code is word-aligned).

    The bootstrap's pass of the same name (bootstrap/MATCHING.md, Compiler
    notes, Assemblers): the bundled GNU as 2.9-ee pads a backward branch that
    closes an R5900 "short loop" with nops, but measures the loop with
    frag-relative addresses, so a long loop whose label sits in an earlier
    frag than its branch can be taken for a short one.  Retail has no such
    padding (u043's RaceOrganization::setupGhost 0x0017AAD8: five bogus nops
    in the 32-byte block-copy loop of a struct copy; u024's mcInitIconSys has
    four).  egcs pads real short loops itself (explicit nops), so nothing
    relies on the assembler's check.  Opt-in per rule.  Returns the number
    of directives added."""
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


def gas_swap(path):
    """The Sony SDK libraries' assembler (rule "asm_pass": "gas_swap"; run
    after label_frags): where egcs left a branch's delay slot unfilled (the
    branch in `.set reorder` mode), standard GAS moves the previous
    instruction into the slot when that is safe; the bundled assembler always
    appends a nop.  The pass is the bootstrap's tools/sdkas.py, imported from
    there (bootstrap/MATCHING.md, Compiler notes, Assemblers: the rule and its
    evidence).  Sony's libraries in the core are the bootstrap's (libcdvd 2240,
    libpad 2200: u129's `_sceCdCallbackThread` is retail `jal ExitDeleteThread;
    sw` / `jalr; lw` / `b; sw` exactly as there); Polyphony's code must never
    get it.  Every swap keeps the semantics.  Returns the number of swaps."""
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "sdkas", os.path.join(paths.REPO, "bootstrap", "tools", "sdkas.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.apply(path)


def compile(rel, out, asm=False, debug=False, extra=(), flags_of=None):
    """Compile src-relative `rel` to `out` (an object, or assembly with asm),
    with the flags of `flags_of` when given (a file generated at link time
    compiled as the source it stands for).  Returns (ok, compiler messages).

    One compile per output at a time (`out`.lock: score.py and datacheck.py
    of several agents compile into the same build/score/obj/, and two
    compiles of one object took each other's .ii, u106's notes); the
    output is replaced whole, so a reader never sees half of it."""
    import time
    lock = os.path.abspath(out) + ".lock"
    os.makedirs(os.path.dirname(lock), exist_ok=True)
    while True:
        try:
            os.close(os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY))
            break
        except FileExistsError:
            try:
                if time.time() - os.path.getmtime(lock) > 600:     # left by a killed compile
                    os.remove(lock)
                    continue
            except OSError:
                continue
            time.sleep(0.2)
    try:
        return _compile(rel, out, asm, debug, extra, flags_of)
    finally:
        try:
            os.remove(lock)
        except OSError:
            pass


def _compile(rel, out, asm, debug, extra, flags_of):
    rel = _rel(os.path.join(ROOT, rel))
    frel = rel if flags_of is None else _rel(os.path.join(ROOT, flags_of))
    cxx = rel.endswith((".cpp", ".cc"))
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    out_r = _rel(os.path.abspath(out))
    pre = out_r + (".ii" if cxx else ".i")
    inc = _rel(os.path.join(CCDIR, "include"))
    r = subprocess.run([CC, "-x", "c++" if cxx else "c", "-E", "-Iinclude", "-Isrc", "-I" + os.path.dirname(rel), "-I" + inc,
                        "-o", pre, rel], capture_output=True, text=True, errors="replace", cwd=ROOT,
                       env=paths.egcs_env(CC))
    if r.returncode:
        return False, r.stdout + r.stderr
    passes = asm_passes_for(frel)
    via_s = bool(passes) and not asm             # object through assembly + the passes
    part = out_r + ".part"                       # (renamed to out_r when done)
    s_out = out_r + ".tmp.s" if via_s else part
    cmd = ([CC, "-x", "c++-cpp-output" if cxx else "cpp-output"] + flags_for(frel) + list(extra) +
           (["-g"] if debug else []) + (["-S"] if asm or via_s else ["-c"]) + ["-o", s_out, pre])
    r2 = subprocess.run(cmd, capture_output=True, text=True, errors="replace", cwd=ROOT, env=paths.egcs_env(CC))
    try:
        os.remove(os.path.join(ROOT, pre))
    except OSError:
        pass
    msgs = r.stderr + r2.stdout + r2.stderr
    ok = r2.returncode == 0 and os.path.isfile(os.path.join(ROOT, s_out))
    if ok and "eh_regions" in passes:
        eh_regions(os.path.join(ROOT, s_out), eh_drop_for(frel))
    if ok and "div_nops" in passes:
        div_nops(os.path.join(ROOT, s_out))
    if ok and "label_frags" in passes:
        label_frags(os.path.join(ROOT, s_out))
    if ok and "gas_swap" in passes:
        gas_swap(os.path.join(ROOT, s_out))
    if ok and via_s:
        r3 = subprocess.run([CC] + flags_for(frel) + ["-c", "-o", part, s_out],
                            capture_output=True, text=True, errors="replace", cwd=ROOT, env=paths.egcs_env(CC))
        msgs += r3.stdout + r3.stderr
        ok = r3.returncode == 0 and os.path.isfile(os.path.join(ROOT, part))
        try:
            os.remove(os.path.join(ROOT, s_out))
        except OSError:
            pass
    if ok:
        os.replace(os.path.join(ROOT, part), os.path.join(ROOT, out_r))
    else:
        try:
            os.remove(os.path.join(ROOT, part))
        except OSError:
            pass
    return ok, msgs


def main():
    a = sys.argv[1:]
    extra = []
    if "--" in a:
        extra = a[a.index("--") + 1:]
        a = a[:a.index("--")]
    if a and a[0] == "--flags":
        print(" ".join(flags_for(_rel(os.path.abspath(a[1])))))
        return 0
    src = next(x for i, x in enumerate(a) if not x.startswith("-") and (i == 0 or a[i - 1] != "-o"))
    out = a[a.index("-o") + 1] if "-o" in a else os.path.splitext(src)[0] + (".s" if "-S" in a else ".o")
    ok, msg = compile(_rel(os.path.abspath(src)), out, asm="-S" in a, debug="-g" in a, extra=extra)
    if msg.strip():
        print(msg.strip())
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
