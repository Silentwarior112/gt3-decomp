"""Compile every canonical header (include/game/*.h) together in one translation
unit, with GCC 14 and with egcs, and list the errors.  hdrcheck checks a header
alone; this finds what only shows when headers meet: a class defined in two
headers, a typedef that clashes with a class or a forward declaration, two
declarations of one function with different signatures.

    python tools/hdrall.py [--no-egcs] [--skip-stubs] [--max N]
"""
import os
import re
import subprocess
import sys

import hdrcheck
import inventory as inv
import paths
import layout

OUT = os.path.join(paths.ROOT, "build", "hdrall")


def headers(skip_stubs):
    out = []
    for fn in layout.header_names():
        if not fn.endswith(".h"):
            continue
        if skip_stubs and "STUB: written by tools/pass2_plan.py" in open(layout.header_path(fn), encoding="utf-8",
                                                                         errors="replace").read():
            continue
        out.append(fn)
    return out


def errors(text):
    """(header, line, message) of every error, deduplicated."""
    seen = []
    for L in text.splitlines():
        # GCC 14: "x.h:12:5: error: msg"; egcs: "x.h:12: msg" (warnings say "warning:")
        m = re.search(r"([\w./\\-]+\.(?:h|cpp)):(\d+):(?:\d+:)?\s*(?:error:\s*)?(.*)$", L)
        if not m or not m.group(3).strip() or "In file included" in L or re.match(r"(warning|note)\b|previous definition|previous declaration|.*In instantiation",
                             m.group(3)):
            continue
        e = (os.path.basename(m.group(1)), int(m.group(2)), m.group(3).strip())
        if e not in seen:
            seen.append(e)
    return seen


TYPEDEF_RE = re.compile(r"\btypedef\b[^;{}]*?(?:\(\s*\*\s*(\w+)\s*\)[^;{}]*|(\w+)\s*(?:\[[^\]]*\]\s*)*);")
TYPEDEF_BODY_RE = re.compile(r"\}\s*(\w+)\s*;")


def type_names(hs, templates=False):
    """{name: {namespace ('' = global): header}} of the classes and typedefs at
    namespace scope (not nested in classes; templates only if asked).  When a
    name is both defined and typedef'd (a re-export), the defining header wins."""
    best = {}
    for fn in hs:
        raw = open(layout.header_path(fn), encoding="utf-8", errors="replace").read()
        if "STUB: written by tools/pass2_plan.py" in raw:
            continue
        masked = inv.mask_comments(raw)
        nss = [(m.end() - 1, inv.match_brace(masked, m.end() - 1), m.group(1)) for m in inv.NS_RE.finditer(masked)]
        cls = [(m.end() - 1, inv.match_brace(masked, m.end() - 1)) for m in inv.DEF_RE.finditer(masked)]

        def where(pos):
            if any(a < pos < b for a, b in cls):
                return None                      # nested in a class
            return "::".join(n for (a, b, n) in nss if a < pos < b)

        found = []                               # (name, position, 0 = definition / 1 = alias)
        for m in inv.DEF_RE.finditer(masked):
            if not templates and re.search(r"template\s*<[^>]*>\s*$", masked[max(0, m.start() - 200):m.start()]):
                continue
            found.append((m.group(2), m.start(), 0))
        for m in TYPEDEF_RE.finditer(masked):
            found.append((m.group(1) or m.group(2), m.start(), 1))
        for m in re.finditer(r"\btypedef\s+(?:struct|union|enum)\b[^;]*?\{", masked):
            e = inv.match_brace(masked, m.end() - 1)
            mm = TYPEDEF_BODY_RE.match(masked, e - 1)
            if mm:
                found.append((mm.group(1), m.start(), 0))
        for name, pos, prio in found:
            ns = where(pos)
            if ns is not None and name:
                cur = best.setdefault(name, {}).get(ns)
                if cur is None or prio < cur[0]:
                    best[name][ns] = (prio, fn)
    return {name: {ns: fn for ns, (prio, fn) in w.items()} for name, w in best.items()}


def ambiguity_probe(hs):
    """Lines that name, under `using namespace N`, every type declared both in N
    and at global scope: egcs calls such a name ambiguous even when one is a
    typedef of the other."""
    names = type_names(hs)
    by_ns = {}
    for name, where in sorted(names.items()):
        if "" in where:
            for ns in where:
                if ns and "::" not in ns:
                    by_ns.setdefault(ns, []).append((name, where[""], where[ns]))
    L = []
    for ns, items in sorted(by_ns.items()):
        L.append("namespace hdrall_probe_%s { using namespace %s;" % (ns, ns))
        for i, (name, g, n) in enumerate(items):
            L.append("  typedef %s t%d;   /* ::%s in %s, %s::%s in %s */" % (name, i, name, g, ns, name, n))
        L.append("}")
    return L, sum(len(v) for v in by_ns.values())


def main():
    skip = "--skip-stubs" in sys.argv
    mx = int(sys.argv[sys.argv.index("--max") + 1]) if "--max" in sys.argv else 80
    os.makedirs(OUT, exist_ok=True)
    src = os.path.join(OUT, "all.cpp")
    hs = headers(skip)
    probe, nprobe = ambiguity_probe(hs)
    with open(src, "w", newline="\n") as fh:
        for h in hs:
            fh.write('#include "%s"\n' % layout.header_rel(h))
        fh.write("int hdrall_main;\n")
        fh.write("\n".join(probe) + "\n")
    print("names declared both globally and in a namespace (probed under using-directives): %d" % nprobe)
    inc = hdrcheck.INC
    b = paths.ee_bin()
    env = dict(os.environ)
    env["PATH"] = b + os.pathsep + env.get("PATH", "")        # the toolchain's DLLs
    runs = [("GCC 14", [os.path.join(b, paths.EE_PREFIX + "g++" + paths.EXE), "-O2", "-G0", "-I" + inc, "-Wall",
                        "-Wno-unused", "-Wno-attributes", "-fsyntax-only", "-std=gnu++14", "-fpermissive",
                        "-Wno-pmf-conversions", "-Wno-invalid-offsetof", src], env)]
    if "--no-egcs" not in sys.argv:
        rel = lambda p: os.path.relpath(p, paths.ROOT).replace("\\", "/")     # space-safe (paths.egcs_env)
        runs.append(("egcs", [hdrcheck.EGCS_CC, "-x", "c++", "-S", "-O0", "-I" + rel(inc),
                              "-I" + rel(os.path.join(hdrcheck.EGCS, "include")), "-o",
                              rel(os.path.join(OUT, "all.s")), rel(src)],
                     paths.egcs_env(hdrcheck.EGCS_CC)))
    bad = 0
    for name, cmd, env in runs:
        r = subprocess.run(cmd, capture_output=True, text=True, env=env, cwd=paths.ROOT)
        es = errors(r.stdout + r.stderr)
        print("%s: %d headers, %s, %d errors" % (name, len(hs), "OK" if r.returncode == 0 else "FAIL", len(es)))
        for h, ln, msg in es[:mx]:
            print("  %s:%d: %s" % (h, ln, msg[:200]))
        if len(es) > mx:
            print("  ... %d more" % (len(es) - mx))
        if r.returncode != 0 and not es:
            print("  " + (r.stdout + r.stderr).strip()[:1500].replace("\n", "\n  "))
        bad += r.returncode != 0
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
