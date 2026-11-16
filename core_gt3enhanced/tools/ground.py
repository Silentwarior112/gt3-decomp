"""Groundwork for the main executable's decompilation: functions, translation units,
listings.  Writes, in ref/:

  funcs.json      every function: start, end (next start), size (code bytes without
                  trailing padding), tu, fde, callees, callers, data_xrefs
  tus.json        translation units: C++ units from the FDE-pointer table (exact) and
                  the runs of functions between them that have no FDE (C / SDK code)
  boundary_violations.txt   branches that leave their function (must be empty)
  ctors.json      .ctors / .dtors lists

Function starts come from, in order of trust:
  1. the .eh_frame FDEs (exact start and length, 551 C++ units);
  2. inside the code no FDE covers: jal targets, code pointers in data, the first
     instruction after padding, and prologues right after a return;
  3. ref/extra_starts.txt (hand-confirmed starts), if present.
A heuristic start that makes a branch leave its function is merged back.
"""
import bisect
import collections
import json
import os
import sys

import coreelf as E
import paths

img = E.Image()
OUT = paths.REF
os.makedirs(OUT, exist_ok=True)


def w(va):
    return img.word(va)


# ---------------------------------------------------------------- FDE table
blocks = []
a = E.FDE_TABLE
while True:
    v = w(a)
    if not v or not (E.BASE <= v < E.IMG_END):
        break
    blocks.append(v)
    a += 4
fde_table_end = a

fdes = []            # (start, size, tu)
for tu, blk in enumerate(blocks):
    p = blk
    while True:
        ln = w(p)
        if not ln or ln > 0x10000:
            break
        cie = w(p + 4)
        if cie != 0:
            loc, rng = w(p + 8), w(p + 12)
            if loc and rng and img.in_text(loc):
                fdes.append((loc, rng, tu))
        p += 4 + ln
fdes.sort()
fde_start = {s: (n, tu) for s, n, tu in fdes}
print("FDE blocks:", len(blocks), " FDEs:", len(fdes))

# overlaps / duplicates
for (s0, n0, t0), (s1, n1, t1) in zip(fdes, fdes[1:]):
    if s0 + n0 > s1:
        print("FDE overlap 0x%08X+%X and 0x%08X" % (s0, n0, s1))

# FDE-covered intervals
cov_lo = [s for s, n, t in fdes]
cov_hi = [s + n for s, n, t in fdes]


def fde_covering(va):
    k = bisect.bisect_right(cov_lo, va) - 1
    if k >= 0 and cov_lo[k] <= va < cov_hi[k]:
        return k
    return None


# ---------------------------------------------------------------- code xrefs
N = (E.TEXT_HI - E.TEXT_LO) // 4
jal_targets = collections.Counter()
j_targets = collections.Counter()
for k in range(N):
    va = E.TEXT_LO + 4 * k
    x = w(va)
    op = x >> 26
    if op in (2, 3):
        t = ((x & 0x03FFFFFF) << 2) | (va & 0xF0000000)
        if img.in_text(t):
            (jal_targets if op == 3 else j_targets)[t] += 1

# code pointers in data (vtables, function tables, jump tables, EH landing pads)
# (data_code_ptrs feeds function starts and leaves out the whole frame-block range, as
# the first pass's funcs.json was made; data_refs also scans the ordinary .data that sits
# between the frame blocks and only feeds the data_xrefs annotation)
data_code_ptrs = collections.defaultdict(list)
data_refs = collections.defaultdict(list)
for a in range(E.TEXT_HI, E.IMG_END - 3, 4):
    if img.in_frame_block(a) if E.EH_FRAME_LO <= a < E.EH_FRAME_HI else False:
        continue
    v = w(a)
    if img.in_text(v):
        data_refs[v].append(a)
        if not (E.EH_FRAME_LO <= a < E.EH_FRAME_HI):
            data_code_ptrs[v].append(a)

# runs of >= 3 code pointers whose targets cluster inside one span: jump tables
jumptab = {}                     # target -> table address
a = E.TEXT_HI
while a < E.IMG_END - 3:
    run = []
    b = a
    while b < E.IMG_END - 3 and img.in_text(w(b)):
        run.append(w(b))
        b += 4
    if len(run) >= 2 and max(run) - min(run) < 0x4000:
        # every target inside one FDE function, or (outside FDE code) tightly clustered
        ks = {fde_covering(t) for t in run}
        if (len(ks) == 1 and None not in ks and all(t not in fde_start for t in run)) or \
           (None in ks and len(run) >= 3 and max(run) - min(run) < 0x2000):
            for t in run:
                jumptab.setdefault(t, a)
    a = max(b, a + 4)

# ---------------------------------------------------------------- starts
starts = set(fde_start)
reason = {s: "fde" for s in starts}


def add(s, why):
    if img.in_text(s) and s not in starts and fde_covering(s) is None:
        starts.add(s)
        reason[s] = why


inner_jal = []
for t in jal_targets:
    if fde_covering(t) is not None and t not in fde_start:
        inner_jal.append(t)
    add(t, "jal")
for t in data_code_ptrs:
    if t not in jumptab:
        add(t, "dataptr")
add(E.TEXT_LO, "text")
add(E.ENTRY, "entry")

def ends_here(va):
    """va is the last non-zero word before a zero run: does a function end there?
    (va is the delay slot of jr $ra / j / eret, or is jr $ra / j whose nop slot
    starts the zero run.)"""
    x0, xm = w(va) or 0, (w(va - 4) or 0) if va - 4 >= E.TEXT_LO else 0
    if xm == 0x03E00008 or (xm >> 26) == 2 or xm == 0x42000018:
        return True
    return x0 == 0x03E00008 or (x0 >> 26) == 2 or x0 == 0x42000018


# gaps between FDE functions: the first word after an FDE function, and the first
# word after padding that follows a return
k = E.TEXT_LO
while k < E.TEXT_HI:
    c = fde_covering(k)
    if c is not None:
        k = cov_hi[c]
        continue
    if w(k) == 0:
        k += 4
        continue
    prev = k - 4
    if prev < E.TEXT_LO or fde_covering(prev) is not None:
        add(k, "after-fde")
    elif w(prev) == 0:
        j = prev
        while j > E.TEXT_LO and w(j) == 0 and fde_covering(j) is None:
            j -= 4
        if fde_covering(j) is not None or ends_here(j):
            add(k, "after-pad")
    k += 4

# prologues right after a return (outside FDE code)
for k in range(2, N):
    va = E.TEXT_LO + 4 * k
    if fde_covering(va) is not None:
        continue
    x = w(va)
    if (x >> 26) in (0x09, 0x19) and ((x >> 21) & 31) == 29 and ((x >> 16) & 31) == 29 and (x & 0x8000):
        j = va - 4
        while j > E.TEXT_LO and w(j) == 0:
            j -= 4
        # j is the delay slot (or the jr itself if the slot was a nop we skipped)
        if w(j - 4) == 0x03E00008 or w(j) == 0x03E00008 or (w(j - 4) >> 26) == 2:
            add(va, "prologue")

# j targets outside FDE code that sit right after a function end: tail-called functions
for t in j_targets:
    if fde_covering(t) is None and t not in starts and t - 8 >= E.TEXT_LO:
        p = t - 4
        while p > E.TEXT_LO and w(p) == 0:
            p -= 4
        if ends_here(p) and (p < t - 4 or w(t - 8) == 0x03E00008 or (w(t - 8) >> 26) == 2):
            add(t, "j-target")

extra = os.path.join(OUT, "extra_starts.txt")
if os.path.exists(extra):
    for line in open(extra, encoding="utf-8"):
        tok = line.split("#")[0].strip()
        if tok:
            s = int(tok, 16)
            starts.add(s)
            reason[s] = "extra"

starts = sorted(starts)


def is_branch(x):
    op = x >> 26
    if op in (4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17):
        return True
    if op == 1 and ((x >> 16) & 31) in (0, 1, 2, 3, 0x10, 0x11, 0x12, 0x13):
        return True
    if op == 0x11 and ((x >> 21) & 31) == 8:      # bc1
        return True
    if op == 0x12 and ((x >> 21) & 31) == 8:      # bc2
        return True
    return False


def btarget(va, x):
    return va + 4 + (E.simm16(x) << 2)


# ---------------------------------------------------------------- merge bad heuristic splits
def ranges(st):
    r = []
    for i, s in enumerate(st):
        e = st[i + 1] if i + 1 < len(st) else E.TEXT_HI
        r.append((s, e))
    return r


changed = True
rounds = 0
while changed and rounds < 20:
    changed = False
    rounds += 1
    sset = set(starts)
    drop = set()
    for s, e in ranges(starts):
        if reason.get(s) == "fde":
            continue
        for va in range(s, e, 4):
            x = w(va)
            if is_branch(x):
                t = btarget(va, x)
                if not (s <= t < e) and img.in_text(t):
                    # a branch out of a heuristic function: merge the starts between
                    lo, hi = (t, va) if t < s else (e, t + 4)
                    for z in starts:
                        if lo <= z <= hi and z != s and reason.get(z) not in ("fde", "jal", "dataptr", "entry", "text", "extra", "after-fde", "j-target"):
                            drop.add(z)
                        if t < s and z == s and reason.get(z) not in ("fde", "jal", "dataptr", "entry", "text", "extra", "after-fde", "j-target"):
                            drop.add(z)
    if drop:
        starts = [z for z in starts if z not in drop]
        changed = True

# hand-confirmed non-starts (stray data words, second halves of functions)
not_starts = os.path.join(OUT, "not_starts.txt")
if os.path.exists(not_starts):
    gone = set()
    for line in open(not_starts, encoding="utf-8"):
        tok = line.split("#")[0].strip()
        if tok:
            gone.add(int(tok, 16))
    starts = [z for z in starts if z not in gone]

funcs = []
for s, e in ranges(starts):
    if reason.get(s) == "fde":
        size = fde_start[s][0]
    else:
        # strip trailing zero padding, keeping the nop delay slot of a final jr / j
        # or b (beq $0,$0: an endless thread loop; u140, ref/matching/followups.md)
        q = e
        while q - 4 >= s and w(q - 4) == 0:
            q -= 4
        if q > s and q + 4 <= e and (w(q - 4) == 0x03E00008 or (w(q - 4) >> 26) == 2
                                     or (w(q - 4) >> 16) == 0x1000):
            q += 4
        size = max(q - s, 4)
    funcs.append({"start": s, "end": e, "size": size, "how": reason.get(s, "?"),
                  "tu": fde_start[s][1] if s in fde_start else None})

# ---------------------------------------------------------------- translation units
# C++ units: the FDE blocks.  Code no FDE covers forms "gap" units: maximal runs of
# non-FDE functions between two FDE units.
tus = []
for t, blk in enumerate(blocks):
    fs = [f for f in funcs if f["tu"] == t]
    if not fs:
        tus.append({"id": "T%03d" % t, "kind": "fde", "block": "0x%08X" % blk, "lo": None, "hi": None, "nfuncs": 0})
        continue
    tus.append({"id": "T%03d" % t, "kind": "fde", "block": "0x%08X" % blk,
                "lo": min(f["start"] for f in fs), "hi": max(f["end"] for f in fs), "nfuncs": len(fs)})

# assign non-FDE functions: inside an FDE unit's span -> that unit (no-FDE functions of a
# C++ unit, e.g. leaf stubs); otherwise a gap unit
spans = sorted([(u["lo"], u["hi"], i) for i, u in enumerate(tus) if u["lo"] is not None])
span_lo = [x[0] for x in spans]
gap_id = None
gapn = 0
for f in funcs:
    if f["tu"] is not None:
        gap_id = None
        continue
    k = bisect.bisect_right(span_lo, f["start"]) - 1
    if k >= 0 and spans[k][0] <= f["start"] < spans[k][1]:
        f["tu"] = spans[k][2]
        gap_id = None
        continue
    if gap_id is None:
        gap_id = len(tus)
        tus.append({"id": "G%03d" % gapn, "kind": "gap", "block": None, "lo": f["start"], "hi": f["end"], "nfuncs": 0})
        gapn += 1
    f["tu"] = gap_id
    tus[gap_id]["hi"] = f["end"]
for u in tus:
    u["nfuncs"] = 0
for f in funcs:
    tus[f["tu"]]["nfuncs"] += 1
for f in funcs:
    f["tu"] = tus[f["tu"]]["id"]

# interleaving check: units must be contiguous
order = sorted(funcs, key=lambda f: f["start"])
runs = []
for f in order:
    if runs and runs[-1][0] == f["tu"]:
        runs[-1][2] = f["end"]
    else:
        runs.append([f["tu"], f["start"], f["end"]])
cnt = collections.Counter(r[0] for r in runs)
split_units = {u: c for u, c in cnt.items() if c > 1}

# ---------------------------------------------------------------- xrefs + validation
startset = set(starts)
fidx = {f["start"]: f for f in funcs}
for f in funcs:
    callees = set()
    for va in range(f["start"], f["end"], 4):
        x = w(va)
        if (x >> 26) == 3:
            t = ((x & 0x03FFFFFF) << 2)
            if img.in_text(t):
                callees.add(t)
    f["callees"] = sorted(callees)
    f["data_xrefs"] = data_refs.get(f["start"], [])
callers = collections.defaultdict(set)
for f in funcs:
    for c in f["callees"]:
        callers[c].add(f["start"])
for f in funcs:
    f["callers"] = sorted(callers.get(f["start"], ()))

viol = []
for f in funcs:
    for va in range(f["start"], f["end"], 4):
        x = w(va)
        if is_branch(x):
            t = btarget(va, x)
            if not (f["start"] <= t < f["end"]):
                viol.append((va, t, f["start"]))
        elif (x >> 26) == 2:
            t = ((x & 0x03FFFFFF) << 2)
            if t not in startset and not (f["start"] <= t < f["end"]):
                viol.append((va, t, f["start"]))

with open(os.path.join(OUT, "boundary_violations.txt"), "w") as fh:
    for va, t, fs in viol:
        fh.write("branch at 0x%08X in F_%08X targets 0x%08X\n" % (va, fs, t))
    for t in sorted(inner_jal):
        fh.write("jal target 0x%08X lies inside FDE function F_%08X\n" % (t, cov_lo[fde_covering(t)]))
    for u, c in sorted(split_units.items()):
        fh.write("unit %s is split into %d address runs\n" % (u, c))

# ---------------------------------------------------------------- ctors / dtors
def count_list(a):
    n = w(a)
    return n, [w(a + 4 + 4 * i) for i in range(n)]


nct, ctors = count_list(E.CTORS)
dt_at = E.CTORS + 4 + 4 * nct
ndt, dtors = count_list(dt_at)
json.dump({"ctors_at": "0x%08X" % E.CTORS, "ctors": ["0x%08X" % x for x in ctors],
           "dtors_at": "0x%08X" % dt_at, "dtors": ["0x%08X" % x for x in dtors],
           "fde_table": "0x%08X" % E.FDE_TABLE, "fde_table_end": "0x%08X" % fde_table_end},
          open(os.path.join(OUT, "ctors.json"), "w"), indent=1)

for f in funcs:
    f["start"] = "0x%08X" % f["start"]
    f["end"] = "0x%08X" % f["end"]
    f["callees"] = ["0x%08X" % x for x in f["callees"]]
    f["callers"] = ["0x%08X" % x for x in f["callers"]]
    f["data_xrefs"] = ["0x%08X" % x for x in f["data_xrefs"]]
for u in tus:
    if u["lo"] is not None:
        u["lo"] = "0x%08X" % u["lo"]
        u["hi"] = "0x%08X" % u["hi"]
json.dump(funcs, open(os.path.join(OUT, "funcs.json"), "w"), indent=0)
json.dump(tus, open(os.path.join(OUT, "tus.json"), "w"), indent=1)
json.dump({"0x%08X" % t: "0x%08X" % a for t, a in sorted(jumptab.items())},
          open(os.path.join(OUT, "jumptab_targets.json"), "w"), indent=0)

hows = collections.Counter(f["how"] for f in funcs)
print("functions:", len(funcs), dict(hows))
print("units: %d fde (%d non-empty), %d gap" % (len(blocks), sum(1 for u in tus if u["kind"] == "fde" and u["nfuncs"]),
                                               sum(1 for u in tus if u["kind"] == "gap")))
print("ctors %d dtors %d" % (nct, ndt))
print("boundary violations:", len(viol), " jal-inside-FDE:", len(inner_jal), " split units:", len(split_units))
print("instructions: %d, FDE-covered: %.1f%%" % (N, 100.0 * sum(n for s, n, t in fdes) / (E.TEXT_HI - E.TEXT_LO)))
