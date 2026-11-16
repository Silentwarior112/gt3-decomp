#!/usr/bin/env python3
"""Data report for the size / all-C pass (ref/DATA.md): how each source
file's data sections compare with the original's, and what of the original's
data in the file's range the C does not define yet.

    python tools/datacheck.py --unit u001          every source file of a unit
    python tools/datacheck.py src/game/app/T004*.cpp   chosen files
    python tools/datacheck.py --unit u001 -v       every differing range / string / name
    python tools/datacheck.py --unit u001 --brief  one line per section and the score

The original's range of a file, per area (.data, .rodata, .lit4, .sdata):
from the original code's references (each referenced address given to one
of the files referencing it, in link order: datalayout.owners), from the
file's first referenced address to the next file's.  .data of a C++ file
also from collect2's frame table: the file's own variables end where its
frame info (`__FRAME_BEGIN__') begins.

Per section: the base (declared addresses of the section's own symbols, else
where most of its bytes are the original's near the range), EXACT when every
byte is the original's (relocated words are compared by target in the link,
so not here), else the differing ranges.  tools/fulllink.py links a section
at its original place only when EXACT; any other section is in the image
twice (the C's copy and the original's bytes).  Strings: the original's
string literals and type names in the range against the section's
(missing / extra / out of order: an extra type name means the file sees a
class the original did not; see ref/DATA.md).  Names: the original's named
variables in the range, and whether this file defines them.

The data score: bytes of the file's sections that are EXACT at their base,
of the original's bytes in the file's ranges (or the section's size when
larger).
"""
import argparse
import bisect
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths                                            # noqa: E402
import score                                            # noqa: E402
import datalayout                                       # noqa: E402
import hybrid                                           # noqa: E402
import funcmap                                          # noqa: E402

_MARK_NAMES = None                                      # marker address -> its name key (fde_report)

ROOT = paths.ROOT
AREA_OF_SEC = datalayout.AREA_OF_SEC
AREA = {k: (lo, hi) for k, lo, hi in datalayout.AREAS}


def names_by_addr():
    out = collections.defaultdict(set)
    for src in (hybrid.declared(), hybrid.table_names(), hybrid.vtables()):
        for n, a in src.items():
            if a is not None and 0x0028E760 <= a < 0x004B2C0C:
                out[a].add(n)
    return out


def printable(b):
    return "".join(chr(c) if 0x20 <= c < 0x7F else "." for c in b)


def ranges(idx):
    out = []
    for i in idx:
        if out and i == out[-1][1]:
            out[-1][1] = i + 1
        else:
            out.append([i, i + 1])
    return out


STR = re.compile(rb"[\x20-\x7e\t\n]{2,}\x00")


def strings(buf, base):
    """[(address, text)] of the NUL-terminated strings in buf that start at a
    word-aligned address after a NUL (or at the start), or 8-aligned (egcs
    aligns every literal, type name and char array to 8: a type name that
    follows a base-class table starts after its 0x40 flag byte).  Unaligned
    runs are pointer bytes (`@P*K' of 0x40000000 0x004B2A50, `35' of
    0x00353300), not strings."""
    out = []
    i, n = 0, len(buf)
    while i < n:
        a = base + i
        after_nul = i == 0 or buf[i - 1] == 0
        if a % 4 == 0 and (after_nul or a % 8 == 0):
            m = STR.match(buf, i)
            # (8-aligned after other bytes: at least 4 characters, else a
            # pointer into the .bss, `P*K' of 0x004B2A50)
            if m and (after_nul or m.end() - i > 4):
                out.append((a, m.group(0)[:-1].decode("latin-1")))
                i = m.end()
                continue
        i += 1
    return out


def lcs_len(a, b):
    """Length of the longest common subsequence (strings in the same order)."""
    if not a or not b:
        return 0
    prev = [0] * (len(b) + 1)
    for x in a:
        cur = [0]
        for j, y in enumerate(b):
            cur.append(prev[j] + 1 if x == y else max(prev[j + 1], cur[j]))
        prev = cur
    return prev[-1]


file_ranges = datalayout.file_ranges


def fde_report(obj, blk, sym_addr, lim, rel=None):
    """Where the object's frame info (its function order and each function's
    size) differs from the original's: a .data EXACT but for its frame info
    has functions out of the original's order (egcs emits a class's inline
    members in reverse definition order) or not yet exact in size."""
    ours = datalayout.object_fdes(obj)
    orig = datalayout.orig_fdes(blk)
    rel_map = {}
    for f, m in sym_addr.items():
        if f != rel:
            rel_map.update(m)
    # the file's own functions win: copies of one inline function in several
    # files of a run (u017's T053 / T054 clear()) have one symbol name
    rel_map.update(sym_addr.get(rel, {}))
    # a weak copy (an inline function emitted in many objects) is scored at
    # the first linked copy's address (T490's GarageCar copies for T080's):
    # the same function when its name is the marker's at the original's pc
    global _MARK_NAMES
    if _MARK_NAMES is None:
        _MARK_NAMES = {a: funcmap._marker_key(n or "") for ms in funcmap.markers().values() for _, a, n in ms}
    out = []
    for i in range(max(len(ours), len(orig))):
        u = ours[i] if i < len(ours) else None
        o = orig[i] if i < len(orig) else None
        ua = rel_map.get(u[0]) if u else None
        if u and o and ua != o[0] and _MARK_NAMES.get(o[0]) and \
                (funcmap.demangle(u[0]) or u[0]).replace(" ", "") == _MARK_NAMES[o[0]]:
            ua = o[0]
        if u and o and ua == o[0] and u[1] == o[1]:
            continue
        if u and o and ua == o[0]:
            out.append("#%d %s: size 0x%X, the original's 0x%X" % (i, u[0], u[1], o[1]))
        else:
            out.append("#%d ours %s (0x%s) | the original's 0x%08X" % (
                i, u[0] if u else "-", "%08X" % ua if ua else "?", o[0] if o else 0))
    if out:
        print("      frame info: %d FDEs, the original %d; differing: %s" % (
            len(ours), len(orig), "; ".join(out[:lim]) + (" ... (%d)" % len(out) if lim and len(out) > lim else "")))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("--unit")
    ap.add_argument("--all", action="store_true", help="every file (with --brief: the baseline)")
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--brief", action="store_true")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    a = ap.parse_args()
    pats = list(a.files)
    if a.unit:
        import layout
        pats += layout.files_of_unit(a.unit)
    if a.all:
        pats = ["src/*"]
    if not pats:
        ap.error("name files, --unit or --all")
    hybrid.TC_BIN = paths.ee_bin(None)
    rels, records, failed, orphans = score.run(pats, a.jobs)
    for rel, err in failed.items():
        print("FAIL %s\n%s" % (rel, err[-1500:]))
    fresh = score.LAST_LAYOUT
    sym_addr = {}                                   # our function symbol -> its original address
    for r in records:
        sym_addr.setdefault(r["file"], {})[r["symbol"]] = r["addr"]
    fr = file_ranges()
    named = names_by_addr()
    named_sorted = sorted(named)
    decl = hybrid.declared()
    lim = None if a.verbose else 6

    tot_exact = tot_all = 0
    for rel in sorted(fresh):
        obj, bases = fresh[rel]
        rr = fr.get(rel, {})
        print("== %s" % rel)
        f_exact = f_all = 0
        for sec in obj.sections:
            area = AREA_OF_SEC.get(sec.name)
            if area is None or sec.size == 0:
                continue
            nobits = sec.type == 8
            syms = sorted((y for y in obj.symbols if y.shndx == sec.index and y.name and y.type in (0, 1)
                           and not y.name.startswith("$")), key=lambda y: y.value)
            rng = rr.get(area)
            if area == ".gcc_except_table":
                print("  %-18s %6d bytes  (placed from the frame info's pointer in the link)" % (sec.name, sec.size))
                continue
            if area == ".bss":
                print("  %-18s %6d bytes  %s" % (sec.name, sec.size, ", ".join(
                    "%s%s" % (y.name, "@0x%08X" % decl[y.name] if y.name in decl else "") for y in syms[:lim])))
                continue
            base, how = datalayout.choose_base(obj, sec, rng, decl, bases.get(sec.index),
                                               rr.get(".frame") if sec.name == ".data" else None)
            if base is not None and not (AREA[area][0] - 0x100 <= base < AREA[area][1]):
                # (the code's references put T158's .data in the .bss: its
                # variables are zero-initialised in the original)
                print("  %-18s %6d bytes  base 0x%08X (%s) is outside the original's %s: these variables "
                      "belong elsewhere (zero-initialised ones in the .bss?)" % (sec.name, sec.size, base, how, area))
                f_all += sec.size
                continue
            size_orig = (rng[1] - rng[0]) if rng else 0
            f_all += max(sec.size, size_orig)
            rtxt = "orig 0x%08X..0x%08X (%d)" % (rng[0], rng[1], size_orig) if rng else "orig range ?"
            if base is None:
                print("  %-18s %6d bytes  NO BASE  %s" % (sec.name, sec.size, rtxt))
                for y in syms[:lim]:
                    print("      +0x%04X %s" % (y.value, y.name))
                continue
            data, mask = datalayout.masked_bytes(obj, sec)
            off = base - datalayout.VRAM + datalayout.ROM
            ret = datalayout.image()[off:off + sec.size]
            diff = [] if nobits else [i for i in range(len(data)) if i not in mask and data[i] != ret[i]]
            ok = not diff
            # a .data whose own variables are exact and only its frame info
            # differs (code not exact yet): the link places it, so it counts
            fbo = next((y.value for y in syms if y.name == "__FRAME_BEGIN__"), None)
            frame_only = bool(diff) and fbo is not None and min(diff) >= fbo
            if ok or frame_only:
                f_exact += sec.size
                # the range's end up to the next section's alignment: zero
                # padding, not data of the file (as far as the range's end
                # is aligned: T539's 64-aligned table, 56 bytes, u125)
                pad = rng[1] - (base + sec.size) if rng else 0
                if 0 < pad < max(16, min(rng[1] & -rng[1], 128)) and rng[0] <= base and not any(
                        datalayout.image()[off + sec.size:off + sec.size + pad]):
                    f_exact += pad
                # and the alignment padding in front of it (an aligned(64)
                # SIF buffer starts up to 63 zero bytes into the range)
                lead = base - rng[0] if rng else 0
                if 0 < lead < max(sec.align, 16) and not any(
                        datalayout.image()[off - lead:off]):
                    f_exact += lead
            print("  %-18s %6d bytes  base 0x%08X (%s)  %s  %s" % (
                sec.name, sec.size, base, how, rtxt,
                "EXACT" if ok else "EXACT but its frame info (%d bytes: code)" % len(diff) if frame_only else
                "%d bytes differ in %d ranges" % (len(diff), len(ranges(diff)))))
            if a.brief:
                continue
            for y in syms:
                d = decl.get(y.name)
                if d is not None and d != base + y.value:
                    print("      %s at +0x%X = 0x%08X, declared 0x%08X" % (y.name, y.value, base + y.value, d))
            rs = ranges(diff)
            if diff and sec.name == ".data" and ".frame" in rr:
                fde_report(obj, rr[".frame"][0], sym_addr, lim, rel)
            for lo_, hi_ in rs[:lim]:
                at = base + lo_
                ours = [y.name for y in syms if y.value <= lo_][-1:] or ["?"]
                j = bisect.bisect_right(named_sorted, at) - 1
                n = min(hi_ - lo_, 20)
                print("      +0x%04X..+0x%04X 0x%08X ours %s %s | orig %s %s  [ours: %s; orig: %s]" % (
                    lo_, hi_, at, data[lo_:lo_ + n].hex(), printable(data[lo_:lo_ + n]),
                    ret[lo_:lo_ + n].hex(), printable(ret[lo_:lo_ + n]), ours[0],
                    "%s@0x%08X" % (sorted(named[named_sorted[j]])[0], named_sorted[j]) if j >= 0 else "?"))
            if lim and len(rs) > lim:
                print("      ... %d more ranges (-v)" % (len(rs) - lim))
            # strings: the original's in the range against ours
            if rng and area in (".rodata", ".sdata", ".data"):
                o_lo, o_hi = rng
                if area == ".data" and ".frame" in rr:
                    o_hi = rr[".frame"][0]
                img = datalayout.image()
                # (pointers are no strings: a jump table's 0x00222568 is `h%"',
                # u102's notes, a table's 0x00352B28 `(+5', u104's: words that
                # look like addresses are blanked on both sides, so a short
                # string that does ("4WD" 0x00445734) counts on neither; and
                # our relocated words)
                def no_code_ptrs(buf, at):
                    buf = bytearray(buf)
                    for k in range((-at) % 4, len(buf) - 3, 4):
                        if 0x00100000 <= int.from_bytes(buf[k:k + 4], "little") < datalayout.AREAS[-1][2]:
                            buf[k:k + 4] = bytes(4)
                    return bytes(buf)
                ob = img[o_lo - datalayout.VRAM + datalayout.ROM:o_hi - datalayout.VRAM + datalayout.ROM]
                orig_s = [s for _, s in strings(no_code_ptrs(ob, o_lo), o_lo)]
                fb = [y.value for y in syms if y.name == "__FRAME_BEGIN__"]
                ub = bytearray(data[:fb[0]] if fb else data)
                for k in datalayout.masked_bytes(obj, sec)[1]:
                    if k < len(ub):
                        ub[k] = 0
                ours_s = [s for _, s in strings(no_code_ptrs(ub, base), base)]
                miss = [s for s in orig_s if s not in ours_s]
                extra = [s for s in ours_s if s not in orig_s]
                common_o = [s for s in orig_s if s in ours_s]
                common_u = [s for s in ours_s if s in orig_s]
                l = lcs_len(common_o[:400], common_u[:400])
                if miss or extra or l < len(common_o[:400]):
                    print("      strings: orig %d, ours %d; missing %d, extra %d, %d of %d common in the original's order"
                          % (len(orig_s), len(ours_s), len(miss), len(extra), l, len(common_o)))
                    if miss:
                        print("        missing: " + " | ".join(repr(s)[1:-1][:40] for s in miss[:lim]))
                    if extra:
                        print("        extra:   " + " | ".join(repr(s)[1:-1][:40] for s in extra[:lim]))
            # the original's named variables in the range: defined here or not
            if rng:
                ourn = {y.name for y in obj.symbols if y.name and y.shndx != 0}     # defined here
                lo_i = bisect.bisect_left(named_sorted, rng[0])
                hi_i = bisect.bisect_left(named_sorted, rng[1])
                undef = []
                ours_at = dict(strings(data, base)) if not nobits else {}
                img = datalayout.image()
                for x in named_sorted[lo_i:hi_i]:
                    ns = named[x]
                    if all(n_.startswith("__tn") for n_ in ns):
                        continue                        # a type name: a string (the strings above)
                    if x in ours_at and img[x - datalayout.VRAM + datalayout.ROM:].startswith(
                            ours_at[x].encode("latin-1") + b"\x00"):
                        continue                        # a name for a literal this file has there
                    # (a copy's name carries its address: `_vt$...Manager_003199A8'
                    # is the vtable this file defines there under its own name)
                    if not (ns & ourn) and not ({re.sub(r"_[0-9A-F]{8}$", "", n_) for n_ in ns} & ourn):
                        undef.append("%s@0x%08X" % (sorted(ns)[0], x))
                if undef:
                    print("      not defined here: " + ", ".join(undef[:lim if lim else None]) +
                          (" ... (%d)" % len(undef) if lim and len(undef) > lim else ""))
        # an area of the original's this file has a range in but our object no
        # section of (T212's 24 bytes of .sdata: the C did not define them,
        # so no section showed them; the u064 agent)
        have = {AREA_OF_SEC.get(s.name) for s in obj.sections if s.size and AREA_OF_SEC.get(s.name)}
        for area in (".data", ".rodata", ".sdata", ".lit4"):
            rng = rr.get(area)
            if not rng or area in have:
                continue
            o_lo, o_hi = rng
            if area == ".data" and ".frame" in rr:
                o_hi = rr[".frame"][0]
            if o_hi <= o_lo:
                continue
            img = datalayout.image()
            body = img[o_lo - datalayout.VRAM + datalayout.ROM:o_hi - datalayout.VRAM + datalayout.ROM]
            lo_i = bisect.bisect_left(named_sorted, o_lo)
            hi_i = bisect.bisect_left(named_sorted, o_hi)
            names_here = ["%s@0x%08X" % (sorted(named[x])[0], x) for x in named_sorted[lo_i:hi_i]]
            if not any(body) and not names_here:
                continue                                # zero padding, nothing named
            f_all += o_hi - o_lo
            print("  %-18s      no section here: orig 0x%08X..0x%08X (%d bytes) is not in the C%s"
                  % (area, o_lo, o_hi, o_hi - o_lo,
                     (": " + ", ".join(names_here[:lim if lim else None])) if names_here else ""))
        if f_all:
            print("  file data: %d of %d bytes exact at the original place" % (f_exact, f_all))
        tot_exact += f_exact
        tot_all += f_all
    if tot_all:
        print("data score: %d of %d bytes exact at their original place (%.1f%%)"
              % (tot_exact, tot_all, 100.0 * tot_exact / tot_all))
    return 0


if __name__ == "__main__":
    sys.exit(main())
