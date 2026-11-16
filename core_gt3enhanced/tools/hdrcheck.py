"""Check canonical headers (include/game/*.h):

  1. GCC 14: a file that includes only the header compiles (as C++, and as C for C
     headers: the ones without classes);
  2. egcs layout: every data member whose comment documents an offset (/* 0x10C */,
     // +0x10C) and every documented size (static_assert(sizeof(X) == N)) is computed
     by SCE ee-gcc 2.9-ee-991111b (the original compiler, sdk/ee-gcc-2.9-991111b) and
     compared.  The original layout is what the comments describe: egcs puts the
     vptr after the first polymorphic base's data, makes bool 4 bytes, ...

    python tools/hdrcheck.py include/game/menu/framework/T206_button.h [...]   or   --all
    options: --no-egcs, --verbose

Headers must be accepted by egcs: C++98 as egcs knows it (namespaces, templates,
bool are fine; no override, = default, = delete, in-class member initialisers,
enum class, constexpr; static_assert only at namespace scope, through types.h).
"""
import os
import re
import subprocess
import sys

import paths
import layout

ROOT = paths.ROOT
INC = os.path.join(ROOT, "include")
BUILD = os.path.join(ROOT, "build", "hdrcheck")
EGCS = paths.sdk("ee-gcc-2.9-991111b")
EGCS_CC = os.path.join(EGCS, "bin", "ee-gcc.exe")

sys.path.insert(0, paths.TOOLS)
import inventory as inv  # noqa: E402  (mask_comments, DEF_RE, match_brace, parse_body)


def gcc14(hdr, as_c):
    b = paths.ee_bin()
    if not b:
        return False, "no EE toolchain"
    os.makedirs(BUILD, exist_ok=True)
    name = os.path.basename(hdr)
    src = os.path.join(BUILD, name + (".c" if as_c else ".cpp"))
    open(src, "w").write('#include "%s"\n' % layout.header_rel(name))
    cc = os.path.join(b, paths.EE_PREFIX + ("gcc" if as_c else "g++") + paths.EXE)
    flags = ["-O2", "-G0", "-I" + INC, "-Wall", "-Wno-unused", "-Wno-attributes", "-fsyntax-only"]
    flags += ["-std=gnu99"] if as_c else ["-std=gnu++14", "-fpermissive", "-Wno-pmf-conversions", "-Wno-invalid-offsetof"]
    env = dict(os.environ)
    env["PATH"] = b + os.pathsep + env.get("PATH", "")        # the toolchain's DLLs
    r = subprocess.run([cc] + flags + [src], capture_output=True, text=True, env=env)
    return r.returncode == 0, (r.stdout + r.stderr).strip()


def documented(hdr):
    """[(qualified class, member, offset)] and [(class, size)] documented in hdr."""
    raw = open(hdr, encoding="utf-8", errors="replace").read()
    masked = inv.mask_comments(raw)
    ns_spans = []
    for m in inv.NS_RE.finditer(masked):
        b = m.end() - 1
        ns_spans.append((b, inv.match_brace(masked, b), m.group(1)))
    offs, sizes = [], []
    for m in inv.DEF_RE.finditer(masked):
        name = m.group(2)
        b = m.end() - 1
        e = inv.match_brace(masked, b)
        # a template definition cannot be measured without its arguments
        pre = masked[max(0, m.start() - 200):m.start()]
        if re.search(r"template\s*<[^>]*>\s*$", pre):
            continue
        # skip classes nested in other class bodies
        outer = [o for o in inv.DEF_RE.finditer(masked) if o.start() < m.start() and inv.match_brace(masked, o.end() - 1) > e]
        if outer:
            continue
        ns = [n for (lo, hi, n) in ns_spans if lo < m.start() < hi]
        q = "::".join(ns + [name])
        fields, _ = inv.parse_body(raw, masked, b + 1, e - 1)
        for f in fields:
            # (a bit-field has no address: offsetof cannot measure it; its
            # neighbours' offsets and the class size still pin it)
            if f["off"] is not None and not f["name"].startswith("__") and not f.get("bits"):
                offs.append((q, f["name"], f["off"]))
    for m in re.finditer(r"static_assert\s*\(\s*sizeof\s*\(\s*([\w:<>, ]+?)\s*\)\s*==\s*(0x[0-9A-Fa-f]+|\d+)", masked):
        sizes.append((m.group(1), int(m.group(2), 0)))
    for m in re.finditer(r"\bEGCS_SIZE\s*\(\s*([\w:<>, ]+?)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*\)", masked):
        if m.group(1) != "T":
            sizes.append((m.group(1), int(m.group(2), 0)))
    return offs, sizes


def egcs_layout(hdr, offs, sizes):
    if not os.path.isfile(EGCS_CC):
        return None, "egcs not installed (sdk/ee-gcc-2.9-991111b)"
    os.makedirs(BUILD, exist_ok=True)
    name = os.path.basename(hdr)
    src = os.path.join(BUILD, name + ".egcs.cpp")
    L = ["#define private public", "#define protected public", "#include <stddef.h>",
         '#include "%s"' % layout.header_rel(name), "const unsigned int layout_vals[] = {"]
    for q, f, o in offs:
        L.append("  (unsigned int) offsetof(%s, %s)," % (q, f))
    for q, s in sizes:
        L.append("  (unsigned int) sizeof(%s)," % q)
    L.append("  0x12345678 };")
    open(src, "w").write("\n".join(L) + "\n")
    out = os.path.join(BUILD, name + ".egcs.s")
    rel = lambda p: os.path.relpath(p, paths.ROOT).replace("\\", "/")     # space-safe (paths.egcs_env)
    r = subprocess.run([EGCS_CC, "-x", "c++", "-S", "-O0", "-I" + rel(INC), "-I" + rel(os.path.join(EGCS, "include")),
                        "-o", rel(out), rel(src)], capture_output=True, text=True, cwd=paths.ROOT,
                       env=paths.egcs_env(EGCS_CC))
    if r.returncode != 0 or not os.path.exists(out):
        return None, (r.stdout + r.stderr).strip()[:3000]
    asm = open(out).read()
    i = asm.find("layout_vals")
    vals = [int(v, 0) for v in re.findall(r"\.word\s+(0x[0-9a-fA-F]+|-?\d+)", asm[i:])]
    vals = [v & 0xFFFFFFFF for v in vals]
    return vals, ""


CPP_RE = re.compile(r"^\s*(class|namespace)\b|\bvirtual\b|\btemplate\s*<", re.M)


def is_cpp(path, seen=None):
    """A header is C++ when it, or a header it includes (quoted includes under
    include/), uses C++: a C header must not include it."""
    seen = set() if seen is None else seen
    if path in seen or not os.path.isfile(path):
        return False
    seen.add(path)
    raw = open(path, encoding="utf-8", errors="replace").read()
    masked = inv.mask_comments(raw)
    # C++ outside `#ifdef __cplusplus` blocks would break a C compile; inside them it is fine
    plain = re.sub(r"(?ms)^\s*#\s*ifdef\s+__cplusplus\b.*?^\s*#\s*endif\b", "", masked)
    if CPP_RE.search(plain):
        return True
    # (mask_comments blanks string contents, so the include names come from raw)
    return any(is_cpp(os.path.join(INC, m), seen) for m in re.findall(r'^\s*#\s*include\s+"([^"]+)"', raw, re.M))


def check(hdr, use_egcs=True, verbose=False):
    has_class = is_cpp(hdr)
    ok_cpp, msg = gcc14(hdr, False)
    ok_c, msg_c = (True, "")
    if not has_class:
        ok_c, msg_c = gcc14(hdr, True)
    problems = []
    if not ok_cpp:
        problems.append("GCC 14 (C++) failed:\n" + msg[:3000])
    if not ok_c:
        problems.append("GCC 14 (C) failed:\n" + msg_c[:3000])
    n_off = n_bad = 0
    if use_egcs:
        offs, sizes = documented(hdr)
        n_off = len(offs) + len(sizes)
        if n_off:
            vals, err = egcs_layout(hdr, offs, sizes)
            if vals is None:
                problems.append("egcs failed:\n" + err)
            else:
                for (q, f, o), v in zip(offs, vals):
                    if v != o:
                        n_bad += 1
                        problems.append("layout: %s::%s documented 0x%X, egcs puts it at 0x%X" % (q, f, o, v))
                    elif verbose:
                        print("  ok %s::%s 0x%X" % (q, f, o))
                for (q, s), v in zip(sizes, vals[len(offs):]):
                    if v != s:
                        n_bad += 1
                        problems.append("size: sizeof(%s) documented 0x%X, egcs says 0x%X" % (q, s, v))
    status = "OK  " if not problems else "FAIL"
    print("%s %s  (%d documented offsets/sizes, %d wrong)" % (status, os.path.relpath(hdr, ROOT).replace("\\", "/"), n_off, n_bad))
    for p in problems[:60]:
        print("   " + p.replace("\n", "\n   "))
    return not problems


def main():
    args = sys.argv[1:]
    use_egcs = "--no-egcs" not in args
    verbose = "--verbose" in args
    files = [a for a in args if not a.startswith("--")]
    if "--all" in args:
        files = [layout.header_path(f) for f in layout.header_names()]
    ok = True
    for f in files:
        p = f if os.path.isabs(f) else os.path.join(ROOT, f)
        ok &= check(p, use_egcs, verbose)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
