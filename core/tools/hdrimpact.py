"""A header change's effect on its includers (the u042 agent's objcmp.py):
the egcs objects of sources compiled against the tree's headers and against
an overlay of the old header versions, without touching the shared tree.

    python tools/hdrimpact.py --find H1 [H2 ...] > build/scratch/<u>/incl.txt
        sources whose preprocessed text includes one of the headers
        (game/race/core/T121_race_entry.h)
    (copy the old header versions to build/scratch/<u>/old/game/X.h)
    python tools/hdrimpact.py --overlay build/scratch/<u>/old --list build/scratch/<u>/incl.txt
        compile each source of FILE twice (DIR's headers first / the tree's)
        and print the sections, symbols and relocations that differ.

The overlay DIR holds old header versions under its own game/ (DIR/game/X.h):
it is searched before include/ (quoted includes look in the including file's
directory first, which for include/game/*.h is include/game/game/: not there).
"""
import argparse
import concurrent.futures as cf
import hashlib
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths    # noqa: E402
ROOT = paths.ROOT
os.chdir(ROOT)
import mcc      # noqa: E402
import elfobj   # noqa: E402

SCR = os.path.join(ROOT, "build", "scratch", "hdrimpact", str(os.getpid()))


def sources():
    out = []
    for dp, _, fns in os.walk(os.path.join(ROOT, "src")):
        for fn in fns:
            if fn.endswith((".cpp", ".c")):
                out.append(os.path.relpath(os.path.join(dp, fn), ROOT).replace("\\", "/"))
    return sorted(out)


def _retry(fn, *args):
    """A file operation retried: on Windows another process (an indexer, the
    other compile's driver) may hold the file a moment (WinError 32; the
    u064 agent saw it with -j above 1).  A file already gone is fine."""
    import time
    for i in range(20):
        try:
            return fn(*args)
        except FileNotFoundError:
            return None
        except PermissionError:
            time.sleep(0.1 * (i + 1))
    return fn(*args)


def preprocess(rel, ii, overlay=None):
    cxx = rel.endswith(".cpp")
    inc = mcc._rel(os.path.join(mcc.CCDIR, "include"))
    cmd = [mcc.CC, "-x", "c++" if cxx else "c", "-E"]
    if overlay:
        cmd.append("-I" + os.path.relpath(overlay, ROOT).replace("\\", "/"))
    cmd += ["-Iinclude", "-Isrc", "-I" + os.path.dirname(rel), "-I" + inc, "-o", ii, rel]
    r = subprocess.run(cmd, capture_output=True, text=True, errors="replace", cwd=ROOT,
                       env=paths.egcs_env(mcc.CC))
    return r.returncode == 0, r.stderr


def compile_obj(rel, out, overlay=None):
    """mcc.compile with an optional overlay include directory."""
    cxx = rel.endswith(".cpp")
    out_r = mcc._rel(os.path.abspath(out))
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    pre = out_r + (".ii" if cxx else ".i")
    ok, err = preprocess(rel, pre, overlay)
    if not ok:
        return False, err
    passes = mcc.asm_passes_for(rel)
    s_out = out_r + ".tmp.s"
    cmd = ([mcc.CC, "-x", "c++-cpp-output" if cxx else "cpp-output"] + mcc.flags_for(rel) +
           ["-S", "-o", s_out, pre])
    r2 = subprocess.run(cmd, capture_output=True, text=True, errors="replace", cwd=ROOT,
                       env=paths.egcs_env(mcc.CC))
    _retry(os.remove, os.path.join(ROOT, pre))
    if r2.returncode or not os.path.isfile(os.path.join(ROOT, s_out)):
        return False, r2.stderr
    if "eh_regions" in passes:
        mcc.eh_regions(os.path.join(ROOT, s_out), mcc.eh_drop_for(rel))
    if "div_nops" in passes:
        mcc.div_nops(os.path.join(ROOT, s_out))
    if "label_frags" in passes:
        mcc.label_frags(os.path.join(ROOT, s_out))
    if "gas_swap" in passes:
        mcc.gas_swap(os.path.join(ROOT, s_out))
    r3 = subprocess.run([mcc.CC] + mcc.flags_for(rel) + ["-c", "-o", out_r, s_out],
                        capture_output=True, text=True, errors="replace", cwd=ROOT,
                       env=paths.egcs_env(mcc.CC))
    keep = s_out[:-len(".tmp.s")] + ".s"
    if os.path.exists(os.path.join(ROOT, keep)):
        _retry(os.remove, os.path.join(ROOT, keep))
    _retry(os.replace, os.path.join(ROOT, s_out), os.path.join(ROOT, keep))
    return r3.returncode == 0, r3.stderr


def find_one(rel, heads):
    ii = os.path.join(SCR, "find", rel.replace("/", "__") + ".ii")
    os.makedirs(os.path.dirname(ii), exist_ok=True)
    ok, err = preprocess(rel, os.path.relpath(ii, ROOT).replace("\\", "/"))
    if not ok:
        return rel, None
    txt = open(ii, encoding="latin-1").read()
    _retry(os.remove, ii)
    return rel, [h for h in heads if ('/' + h + '"') in txt or ('"' + h + '"') in txt]


def describe(path):
    o = elfobj.ElfObject(path)
    secs = {}
    for s in o.sections:
        if s.type in (1, 8) and s.name and not s.name.startswith(".stab") and s.name not in (".comment", ".mdebug", ".reginfo", ".pdr"):
            secs[s.name] = (s.size, hashlib.md5(s.data).hexdigest() if s.type == 1 else "nobits")
    syms = sorted((y.name, y.bind, o.sections[y.shndx].name if 0 < y.shndx < len(o.sections) else y.shndx, y.value, y.size)
                  for y in o.symbols if y.name and y.type != 4)
    rel = {}
    for idx, lst in o.relocs.items():
        nm = o.sections[idx].name
        if nm.startswith(".stab") or nm in (".mdebug", ".pdr"):
            continue
        rel[nm] = sorted((r.offset, r.type, r.sym.name if r.sym else None) for r in lst)
    return secs, syms, rel


def cmp_one(rel, overlay, outdir):
    base = os.path.join(outdir, rel.replace("/", "__"))
    a, b = base + ".old.o", base + ".new.o"
    ok1, e1 = compile_obj(rel, a, overlay)
    ok2, e2 = compile_obj(rel, b, None)
    if not (ok1 and ok2):
        return rel, ["COMPILE old=%s new=%s %s" % (ok1, ok2, (e1 + e2).strip().splitlines()[:3])]
    da, db = describe(a), describe(b)
    diffs = []
    for nm in sorted(set(da[0]) | set(db[0])):
        if da[0].get(nm) != db[0].get(nm):
            diffs.append("section %s: %s -> %s" % (nm, da[0].get(nm), db[0].get(nm)))
    if da[1] != db[1]:
        sa, sb = set(da[1]), set(db[1])
        diffs.append("symbols: -%s +%s" % (sorted(sa - sb)[:6], sorted(sb - sa)[:6]))
    for nm in sorted(set(da[2]) | set(db[2])):
        if da[2].get(nm) != db[2].get(nm):
            diffs.append("relocs %s differ" % nm)
    return rel, diffs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--find", nargs="+")
    ap.add_argument("--overlay")
    ap.add_argument("--list")
    ap.add_argument("--out", default=os.path.join(SCR, "obj"))
    ap.add_argument("-j", type=int, default=6)
    a = ap.parse_args()
    if a.find:
        with cf.ThreadPoolExecutor(a.j) as ex:
            for rel, hit in ex.map(lambda r: find_one(r, a.find), sources()):
                if hit is None:
                    print("#FAIL", rel)
                elif hit:
                    print(rel, " ".join(hit))
        return
    rels = [L.split()[0] for L in open(a.list, encoding="utf-8-sig") if L.strip() and not L.startswith("#")]
    os.makedirs(a.out, exist_ok=True)
    nd = 0
    with cf.ThreadPoolExecutor(a.j) as ex:
        for rel, diffs in ex.map(lambda r: cmp_one(r, a.overlay, a.out), rels):
            if diffs:
                nd += 1
                print("==", rel)
                for d in diffs:
                    print("   ", d)
    print("%d sources, %d differ" % (len(rels), nd))


if __name__ == "__main__":
    main()
