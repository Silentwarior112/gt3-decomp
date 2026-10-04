"""Worst-case stack depth of the bootstrap's threads, from GCC's call-graph info.

    python tools/stack_depth.py [--opt -O2] [root ...]

Compiles every source with -fcallgraph-info=su (into build/stackinfo/), builds
the static call graph with per-function frame sizes, and prints the deepest
path from each root.  Default roots are the entry points of the threads that
run on fixed-size stacks:
  _Z21startup_screen_threadPv   warning-screen render thread, stack 0x4000
  _start / main                 main thread, 1 MB
Indirect calls (function pointers, virtuals) are not followed and are listed.
"""
import argparse, os, re, subprocess, sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import build

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--opt", default="-O2")
    ap.add_argument("roots", nargs="*")
    a = ap.parse_args()
    out = os.path.join(ROOT, "build", "stackinfo" + a.opt.replace("-", "_"))
    os.makedirs(out, exist_ok=True)
    frame, edges, dynamic = {}, defaultdict(set), set()
    for rel in build.sources():
        if not rel.endswith((".c", ".cpp", ".cc")):
            continue
        cxx = rel.endswith((".cpp", ".cc"))
        base = os.path.join(out, rel.replace("/", "__"))
        cmd = [build.tool("g++" if cxx else "gcc"), a.opt] + (build.CXXFLAGS if cxx else build.CFLAGS) + \
              ["-fcallgraph-info=su", "-c", os.path.join(ROOT, rel), "-o", base + ".o"]
        r = build.run(cmd, cwd=out)
        if r.returncode:
            print("compile failed:", rel, r.stderr[-500:]); continue
    for fn in os.listdir(out):
        if not fn.endswith(".ci"):
            continue
        txt = open(os.path.join(out, fn), encoding="utf-8", errors="replace").read()
        for m in re.finditer(r'node:\s*\{\s*title:\s*"([^"]+)"\s*label:\s*"([^"]*)"', txt):
            title, label = m.group(1), m.group(2)
            s = re.search(r"\\n(\d+) bytes \((static|dynamic[^)]*)\)", label)
            if s:
                frame[title] = max(frame.get(title, 0), int(s.group(1)))
                if "dynamic" in s.group(2):
                    dynamic.add(title)
            else:
                frame.setdefault(title, 0)
        for m in re.finditer(r'edge:\s*\{\s*sourcename:\s*"([^"]+)"\s*targetname:\s*"([^"]+)"', txt):
            edges[m.group(1)].add(m.group(2))
    # titles look like "file.c:name" or bare "name" for externals: key by bare name
    def bare(t):
        return t.split(":")[-1]
    F, E = {}, defaultdict(set)
    for t, v in frame.items():
        F[bare(t)] = max(F.get(bare(t), 0), v)
    for s, ts in edges.items():
        for t in ts:
            E[bare(s)].add(bare(t))
    memo, onstack = {}, set()
    def depth(f):
        if f in memo:
            return memo[f]
        if f in onstack:
            return (0, [f + " (recursion)"])
        onstack.add(f)
        best = (0, [])
        for g in E.get(f, ()):
            d = depth(g)
            if d[0] > best[0]:
                best = d
        onstack.discard(f)
        res = (F.get(f, 0) + best[0], [f] + best[1])
        memo[f] = res
        return res
    roots = a.roots or ["_Z21startup_screen_threadPv", "main"]
    for r in roots:
        if r not in F and r not in E:
            cands = [k for k in F if r in k]
            print("root %s not found; similar: %s" % (r, cands[:5])); continue
        d, path = depth(r)
        print("%s  %s: worst-case %d bytes (0x%X)" % (a.opt, r, d, d))
        for p in path:
            print("    %6d  %s%s" % (F.get(p, 0), p, "  [dynamic]" if any(bare(t) == p for t in dynamic) else ""))

if __name__ == "__main__":
    main()
