"""Write the work packet of each unit, ref/units/<unit>/:

  index.txt     one line per function: address, size, name (and where it came from),
                class, callers, unit-external callers; the unit's translation units
  listing.txt   the annotated disassembly of exactly the unit's functions (tools/listing.py)
  m2c.c         machine drafts by m2c (sdk/m2c, target mipsee-gcc-c) from the splat
                assembly (tools/gen_splat.py), headed by a table of the known names of
                every function the drafts mention
  boot.txt      for functions that are the same code as a decompiled bootstrap function:
                that function's source, to reuse

    python tools/packets.py [unit ...] [--jobs N] [--no-m2c]
"""
import argparse
import concurrent.futures
import csv
import json
import os
import re
import subprocess
import sys

import listing
import paths

R = paths.REF
SPLAT = os.path.join(R, "splat")
M2C = paths.sdk("m2c", "m2c.py")
BOOT = os.path.join(paths.REPO, "bootstrap")

ap = argparse.ArgumentParser()
ap.add_argument("units", nargs="*")
ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
ap.add_argument("--no-m2c", action="store_true")
args = ap.parse_args()

units = json.load(open(os.path.join(R, "units.json")))
todo = args.units or list(units)
syms = {int(r["addr"], 16): r for r in csv.DictReader(open(os.path.join(R, "symbols.csv"), encoding="utf-8"))}
lm = json.load(open(os.path.join(R, "libmatch.json")))
ctx = listing.Ctx()
funcs = ctx.funcs
CLASSES = json.load(open(os.path.join(R, "classes.json")))


def unit_of_addr():
    m = {}
    for u, s in units.items():
        lo, hi = (int(x, 16) for x in s["ranges"][0])
        m[u] = (lo, hi)
    return m


URANGE = unit_of_addr()


def unit_at(a):
    for u, (lo, hi) in URANGE.items():
        if lo <= a < hi:
            return u
    return None


def short(a):
    s = syms.get(a)
    return "F_%08X%s" % (a, (" " + s["name"]) if s else "")


# ---- bootstrap sources by function
_boot_cache = {}


def boot_source(addr, rel):
    """The text of the bootstrap function at addr in file rel: from its FUNC marker
    to the next marker (or end of file)."""
    if rel not in _boot_cache:
        try:
            _boot_cache[rel] = open(os.path.join(BOOT, rel), encoding="utf-8", errors="replace").read().splitlines()
        except OSError:
            _boot_cache[rel] = []
    L = _boot_cache[rel]
    pat = re.compile(r"^\s*//\s*FUNC\s+0x%08X\b" % addr, re.I)
    for i, line in enumerate(L):
        if pat.match(line):
            j = i + 1
            while j < len(L) and not re.match(r"^\s*//\s*FUNC\s+0x", L[j]):
                j += 1
            # trim trailing blank lines / comments that belong to the next function
            return "\n".join(L[i:j]).rstrip()
    return None


# ---- rodata blocks for m2c (jump tables, floats, strings)
_rodata = None


def rodata_blocks():
    global _rodata
    if _rodata is None:
        _rodata = {}
        src = open(os.path.join(SPLAT, "asm", "data", "data.rodata.s"), encoding="utf-8").read()
        cur, buf = None, []
        for line in src.splitlines():
            m = re.match(r"^dlabel (\S+)", line)
            if m:
                cur, buf = m.group(1), [line]
                continue
            if cur:
                buf.append(line)
                if line.startswith("enddlabel"):
                    _rodata[cur] = "\n".join(buf)
                    cur = None
    return _rodata


_INS = re.compile(r"^(\s*/\*[^*]*\*/\s+)(\S+)(\s+)(.*)$")
_MEM = re.compile(r"^(\$\w+),\s*(-?(?:0x[0-9A-Fa-f]+|\d+))?\((\$\w+)\)$")
PAIRS = {"ldl": ("ldr", 7, "ld"), "ldr": ("ldl", -7, "ld"), "sdl": ("sdr", 7, "sd"), "sdr": ("sdl", -7, "sd"),
         "lwl": ("lwr", 3, "lw"), "lwr": ("lwl", -3, "lw"), "swl": ("swr", 3, "sw"), "swr": ("swl", -3, "sw")}


def preprocess(text):
    """Rewrite what m2c cannot read: unaligned load/store pairs (ldl+ldr, sdl+sdr,
    lwl+lwr, swl+swr) become one aligned ld/sd/lw/sw at the second instruction;
    VU0 operands like ($vi10++) lose their syntax; the R5900 forms spimdisasm leaves
    as data (cvt.w.s as `.word`, sqrt.s with its source in ft as `c1`) become the
    instructions.  For drafts only."""
    lines = text.split("\n")
    for k, L in enumerate(lines):
        m = re.match(r"^(\s*/\*\s*\S+\s+\S+\s+([0-9A-F]{8})\s*\*/\s+)(\.word\s+0x[0-9A-F]+\s+#\s+(cvt\.w\.s\s+\$f\d+,\s*\$f\d+).*|c1\s+0x[0-9A-Fa-f]+.*)$", L)
        if not m:
            continue
        if m.group(4):
            lines[k] = m.group(1) + m.group(4)
        else:
            b = bytes.fromhex(m.group(2))
            w = int.from_bytes(b, "little")
            if (w >> 26) == 0x11 and ((w >> 21) & 31) == 0x10 and (w & 0x3F) == 0x04:
                lines[k] = m.group(1) + "sqrt.s $f%d, $f%d" % ((w >> 6) & 31, (w >> 16) & 31)
    parsed = []
    for L in lines:
        m = _INS.match(L)
        parsed.append(m)
    for i, m in enumerate(parsed):
        if not m or m.group(2) not in PAIRS:
            continue
        other, adj, merged = PAIRS[m.group(2)]
        a = _MEM.match(m.group(4).strip())
        if not a:
            continue
        rt, off, base = a.group(1), int(a.group(2) or "0", 0), a.group(3)
        for j in range(i + 1, min(i + 8, len(parsed))):
            n = parsed[j]
            if not n:
                continue
            if n.group(2) == other:
                b = _MEM.match(n.group(4).strip())
                if b and b.group(1) == rt and b.group(3) == base and int(b.group(2) or "0", 0) == off - adj:
                    lo = min(off, off - adj)
                    lines[i] = m.group(1) + "nop"
                    lines[j] = n.group(1) + merged + n.group(3) + "%s, 0x%X(%s)" % (rt, lo, base) if lo >= 0 else \
                        n.group(1) + merged + n.group(3) + "%s, -0x%X(%s)" % (rt, -lo, base)
                    parsed[j] = None
                break
    out = "\n".join(lines)
    # spimdisasm writes o32 names ($t0-$t3 = $8-$11); m2c's EE target uses the EE/n32
    # names ($a4-$a7 = $8-$11, arguments 5-8; $t0-$t3 = $12-$15)
    o32_to_n32 = {"$t0": "$a4", "$t1": "$a5", "$t2": "$a6", "$t3": "$a7",
                  "$t4": "$t0", "$t5": "$t1", "$t6": "$t2", "$t7": "$t3"}
    out = re.sub(r"\$t[0-7]\b", lambda m: o32_to_n32[m.group(0)], out)
    out = re.sub(r"\((?:--)?(\$vi\d+)(?:\+\+|--)?\)", r"\1", out)
    # COP0 registers by number read as GPRs to m2c: name them
    out = re.sub(r"(\b(?:mfc0|mtc0)\s+\$\w+,\s*)\$(\d+)\b", r"\1$c0_\2", out)
    return out


def run_m2c(u, outdir):
    asm0 = os.path.join(SPLAT, "asm", u + ".s")
    text = preprocess(open(asm0, encoding="utf-8").read())
    asm = os.path.join(outdir, "_" + u + ".s")
    open(asm, "w", encoding="utf-8", newline="\n").write(text)
    refs = set(re.findall(r"%(?:hi|lo|gp_rel)\((\w+)\)", text))
    blocks = rodata_blocks()
    ro = os.path.join(outdir, "_rodata.s")
    with open(ro, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(".section .rodata\n\n")
        for r in sorted(refs):
            if r in blocks:
                fh.write(blocks[r] + "\n\n")
    cmd = [sys.executable, M2C, "--target", "mipsee-gcc-c", "--globals", "used", "--comment-style", "oneline",
           "--heuristic-strings", "--hex-case", asm, ro]
    p = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    os.remove(ro)
    os.remove(asm)
    return p.stdout, p.stderr, p.returncode


def write_packet(u):
    s = units[u]
    lo, hi = (int(x, 16) for x in s["ranges"][0])
    d = os.path.join(R, "units", u)
    os.makedirs(d, exist_ok=True)
    fs = [f for f in funcs if lo <= int(f["start"], 16) < hi]
    # index
    idx = ["# unit %s  0x%08X..0x%08X  %d functions, %d instructions, language %s" % (u, lo, hi, len(fs), s["instrs"], s["lang"]),
           "# translation units: %s%s" % (", ".join(s["tus"]), ("  (split: %s)" % ", ".join(s["split_tus"])) if s["split_tus"] else ""),
           "# columns: address size tu name [source] {class} callers(in unit/outside) boot-match",
           ""]
    boot_txt = []
    for f in fs:
        a = int(f["start"], 16)
        sy = syms.get(a)
        nm = "%s [%s]" % (sy["name"], sy["source"]) if sy else "-"
        cl = (" {%s}" % sy["class"]) if sy and sy["class"] else ""
        cin = sum(1 for c in f["callers"] if lo <= int(c, 16) < hi)
        cout = len(f["callers"]) - cin
        bm = lm.get(f["start"])
        bms = "  BOOT=%s %s" % (bm["boot"], bm["file"]) if bm else ""
        dx = "  vtbl/dataref" if f["data_xrefs"] else ""
        idx.append("%s 0x%04X %s %s%s %d/%d%s%s" % (f["start"], f["size"], f["tu"], nm, cl, cin, cout, dx, bms))
        if bm and bm["file"]:
            src = boot_source(int(bm["boot"], 16), bm["file"])
            if src:
                boot_txt.append("// ===== core %s  <=  bootstrap %s (%s)\n%s\n" % (f["start"], bm["boot"], bm["file"], src))
    open(os.path.join(d, "index.txt"), "w", encoding="utf-8", newline="\n").write("\n".join(idx) + "\n")
    if boot_txt:
        open(os.path.join(d, "boot.txt"), "w", encoding="utf-8", newline="\n").write(
            "// Bootstrap sources of the functions in this unit that are the same machine code\n"
            "// (relocations masked). Reuse them: change the FUNC address to the core one.\n\n" + "\n".join(boot_txt))
    elif os.path.exists(os.path.join(d, "boot.txt")):
        os.remove(os.path.join(d, "boot.txt"))

    # listing
    def hdr(va):
        f = ctx.fidx[va]
        out = []
        ext = [int(c, 16) for c in f["callers"]]
        if ext:
            out.append("; callers: " + ", ".join(short(c) for c in ext[:8]) + (" ..." if len(ext) > 8 else ""))
        sy = syms.get(va)
        if sy and sy["note"]:
            out.append("; note: " + sy["note"][:200])
        return out

    lines = []
    listing.annotate(ctx, lo, hi, lines, header_extra=hdr)
    open(os.path.join(d, "listing.txt"), "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")

    # classes this unit owns: typeinfo function, or any vtable slot method, inside it
    def inside(a):
        return a and lo <= int(a, 16) < hi
    own = [e for e in CLASSES.values()
           if inside(e["typeinfo_fn"]) or any(inside(s["fn"]) for v in e["vtables"] for s in v["slots"])]
    cl = ["# Polymorphic classes with their typeinfo function or a vtable method in %s." % u,
          "# Per slot: index, call offset from the vptr, method, its translation unit (* = in this unit), name.",
          "# The whole index: ref/classes.txt.", ""]
    for e in sorted(own, key=lambda e: e["typeinfo_fn"] or ""):
        cl.append("class %s   [%s]  typeinfo %s%s (%s)%s" % (e["name"], e["mangled"], e["typeinfo_fn"],
                  "*" if inside(e["typeinfo_fn"]) else "", e["rtti"], ("  bases: " + ", ".join(e["bases"])) if e["bases"] else ""))
        for v in e["vtables"]:
            cl.append("  vtable %s %s this_delta %s, %s slots" % (v["addr"], v["kind"], v["this_delta"], v["len"]))
            for sl in v["slots"]:
                cl.append("    [%2d] %-6s %s %s%s  %s" % (sl["slot"], sl["off"], sl["fn"], sl["tu"], "*" if inside(sl["fn"]) else " ",
                                                       sl["name"] or ""))
        cl.append("")
    open(os.path.join(d, "classes.txt"), "w", encoding="utf-8", newline="\n").write("\n".join(cl) + "\n")
    return d


def m2c_job(u):
    d = os.path.join(R, "units", u)
    out, err, rc = run_m2c(u, d)
    s = units[u]
    lo, hi = (int(x, 16) for x in s["ranges"][0])
    mentioned = sorted({int(x, 16) for x in re.findall(r"\bF_([0-9A-F]{8})\b", out)})
    head = ["// m2c drafts for unit %s (0x%08X..0x%08X). Machine output: types, signedness and" % (u, lo, hi),
            "// control flow need checking against listing.txt. '?' = unknown type.",
            "// Known names of the functions mentioned here:"]
    for a in mentioned:
        sy = syms.get(a)
        if sy:
            head.append("//   F_%08X = %s" % (a, sy["name"]))
    head.append("")
    # mark each function definition with its known name
    body = []
    for line in out.splitlines():
        m = re.match(r"^[^\s/].*\bF_([0-9A-F]{8})\(.*\)\s*\{\s*$", line)
        if m:
            a = int(m.group(1), 16)
            sy = syms.get(a)
            f = ctx.fidx.get(a)
            body.append("")
            body.append("// ---- F_%08X%s%s" % (a, (" = " + sy["name"]) if sy else "", ("  (size 0x%X)" % f["size"]) if f else ""))
        body.append(line)
    if err.strip():
        body.append("\n/* m2c stderr:\n%s\n*/" % err.strip()[:4000])
    open(os.path.join(d, "m2c.c"), "w", encoding="utf-8", newline="\n").write("\n".join(head + body) + "\n")
    return u, rc, len(out.splitlines())


if __name__ == "__main__":
    for u in todo:
        write_packet(u)
    print("packets written:", len(todo))
    if not args.no_m2c:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as ex:
            for u, rc, n in ex.map(m2c_job, todo):
                if rc:
                    print("m2c exit %d on %s" % (rc, u))
        print("m2c drafts written")
