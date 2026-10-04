"""Build SCUS_971.02 (the GT3 US bootstrap) from the decompiled sources.

    python tools/build.py [--out PATH] [--keep-going]
    python tools/build.py --compiler sce        # behaviour build with the retail compiler
    python tools/build.py --compiler 991111-01  # the same with the reference compiler

--compiler gcc14 (default): GCC 14, the EE toolchain's own; output in build/.
--compiler sce: SCE ee-gcc 2.9-ee-991111b (tools/mcc.py, per-file retail flags
  from ref/compile_flags.json), GCC runtime from that toolchain's own libgcc.a
  instead of runtime/'s reimplementations; output in build/sce/. It proves the
  C behaves the same when the retail compiler generates the code
  (python tools/boot_test.py --elf build/sce/scus_971.02).
--compiler 991111-01: as sce, but C and C++ go through Sony's ee-gcc
  2.9-ee-991111-01, the reference compiler (mcc.py --cc 991111-01; needs
  `pip install unicorn`); output in build/991111-01/.

--out PATH also copies the finished ELF there; a retail file found there is
copied to build/backup/ first.  Debug builds (--trace, --opt, --opt-file) are
never copied.
--toolchain DIR picks the EE toolchain: its bin folder, or a folder that has
ee/bin.  Default: tools/paths.py (<repo>/sdk/ps2sdk, ps2sdk-main next to the
repository, $PS2SDK_HOME, $PS2DEV).  The retail-compiler builds use it to link.
--jobs N compiles N files at once (default: one per CPU).  --progress prints a
"[done/total] file" line as each file finishes (the GT3 Decomp Builder reads it).

Stages:
  0. assets    the three blobs link/blobs.S includes, extracted into data/ from
               the retail file (baserom/SCUS_971.02) when they are missing
  1. compile   every .c/.cpp/.cc/.S under src/, sdk/, runtime/ and link/ with the
               EE toolchain (GCC 14.2, n32 ABI)
  2. check     objects: no constructor lists the decomp's crtbegin would not run,
               no hard-coded addresses into the bootstrap's own image
  3. link      link/bootstrap.ld, no toolchain libraries unless listed in
               FALLBACK_LIBS; the map file says where every symbol came from
  4. finish    strip, align segments for the BIOS loader and replay its load
               (tools/align_segments.py), sanity-check the ELF
  5. install   with --out only

Everything intermediate goes to build/. Exit status is non-zero on any failure.
"""
import argparse, concurrent.futures as cf, hashlib, os, re, shutil, struct, subprocess, sys, time

import paths

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
TC_BIN = None                       # the EE toolchain's bin folder (--toolchain, paths.ee_bin)
PREFIX = paths.EE_PREFIX
BUILD = os.path.join(ROOT, "build")
OBJ = os.path.join(BUILD, "obj")
RETAIL_SHA256 = paths.RETAIL_SHA256
ALIGN_TOOL = os.path.join(HERE, "align_segments.py")
# what link/blobs.S includes from data/: not in the repository, extracted from
# the retail file by these two scripts
ASSETS = {"startupa_img.gz": "extract_startup_img.py",
          "gs_vu1_microcode_scrambled.bin": "gs_extract.py",
          "gs_vu0_microcode.bin": "gs_extract.py"}

SRC_DIRS = ["src", "sdk", "runtime", "link"]
BOOT_LO, BOOT_HI = 0x01000000, 0x01050000   # retail image incl. .bss
RETAIL_DATA_LO, RETAIL_DATA_HI = 0x01026C00, 0x0104DEB0   # retail .data .. end of .bss
import json as _json
# 0x01000000 is the unreferenced nop padding before _start, and also 1 << 24.
RETAIL_FUNCS = {f["start"] for f in _json.load(open(os.path.join(ROOT, "ref", "funcs.json")))} - {0x01000000}

COMMON = ["-G0", "-I" + os.path.join(ROOT, "include"),
          "-ffreestanding", "-fno-builtin", "-fno-strict-aliasing", "-fwrapv", "-fno-math-errno",
          "-fno-common", "-fno-asynchronous-unwind-tables", "-fno-unwind-tables",
          "-Wall", "-Wno-unused", "-Wno-unused-parameter", "-Wno-attributes"]
CFLAGS = COMMON + ["-std=gnu99"]
CXXFLAGS = COMMON + ["-std=gnu++14", "-fpermissive", "-fno-exceptions", "-fno-rtti",
                     "-fno-threadsafe-statics", "-fno-use-cxa-atexit",
                     "-fno-sized-deallocation",
                     # keep destructor stores to members (egcs has no lifetime
                     # DSE: retail's ~Allocator clears its fields)
                     "-fno-lifetime-dse"]
ASFLAGS = ["-I" + os.path.join(ROOT, "include")]
FALLBACK_LIBS = []          # e.g. ["-lgcc"]; empty = the decomp supplies everything

def tool(name):
    return os.path.join(TC_BIN, PREFIX + name + paths.EXE)

def run(cmd, **kw):
    env = dict(os.environ)
    env["PATH"] = TC_BIN + os.pathsep + env.get("PATH", "")
    return subprocess.run(cmd, capture_output=True, text=True, env=env, **kw)

def ensure_assets():
    """Extract the data/ blobs link/blobs.S needs from the retail file when
    they are missing.  True when they are all there."""
    missing = [n for n in ASSETS if not os.path.isfile(os.path.join(ROOT, "data", n))]
    if not missing:
        return True
    print("== assets: extracting %s from the retail file" % ", ".join(missing))
    if not os.path.isfile(paths.RETAIL_ELF):
        print("missing %s: the retail SCUS_971.02 of a GT3 A-Spec US disc (SCUS-97102). The "
              "GT3 Decomp Builder copies it there from your file or disc image." % paths.RETAIL_ELF)
        return False
    if hashlib.sha256(open(paths.RETAIL_ELF, "rb").read()).hexdigest().upper() != RETAIL_SHA256:
        print("%s is not the retail US SCUS_971.02 (its SHA-256 differs)" % paths.RETAIL_ELF)
        return False
    for script in sorted({ASSETS[n] for n in missing}):
        r = subprocess.run([sys.executable, os.path.join(HERE, script)], capture_output=True, text=True, cwd=ROOT)
        if r.returncode:
            print((r.stdout + r.stderr).strip()[-3000:]); print("extracting the data blobs failed"); return False
    return all(os.path.isfile(os.path.join(ROOT, "data", n)) for n in ASSETS)

COMPILER = "gcc14"           # or "sce" / "991111-01" (--compiler)
RETAIL_CCS = ("sce", "991111-01")    # compiled by tools/mcc.py with the retail flags
EABI_AS = ["-march=r5900", "-mabi=eabi", "-mgp64", "-EL", "-mno-fix-r5900"]

def sources():
    out = []
    skip = set()
    if COMPILER in RETAIL_CCS:
        skip = set(_json.load(open(os.path.join(ROOT, "ref", "compile_flags.json")))["not_matched"]["files"])
    for d in SRC_DIRS:
        for dp, _, fns in os.walk(os.path.join(ROOT, d)):
            for fn in fns:
                if fn.endswith((".c", ".cpp", ".cc", ".S")):
                    rel = os.path.relpath(os.path.join(dp, fn), ROOT).replace("\\", "/")
                    if rel not in skip:
                        out.append(rel)
    return sorted(out)

MARK = re.compile(r"^\s*(?://|/\*)\s*FUNC\s+0x(01[0-9A-Fa-f]{6})\b")
def first_func_addr(rel):
    lo = None
    for line in open(os.path.join(ROOT, rel), encoding="utf-8", errors="replace"):
        m = MARK.match(line)
        if m:
            a = int(m.group(1), 16)
            lo = a if lo is None else min(lo, a)
    return lo

def obj_path(rel):
    return os.path.join(OBJ, rel.replace("/", "__") + ".o")

OPT = {"default": "-O2"}     # per-file overrides: OPT[rel] = "-O0" etc. (--opt-file)

def compile_one(rel):
    """Compile one file into obj_path(rel) -> (ok, compiler messages)."""
    src, o = os.path.join(ROOT, rel), obj_path(rel)
    if COMPILER in RETAIL_CCS:
        if rel.startswith("link/") and rel.endswith(".S"):
            # build glue (.incbin blobs): the modern assembler, EABI flags
            r = run([tool("as")] + EABI_AS + ["-I", ROOT, "-o", o, src], cwd=ROOT)
            return r.returncode == 0, r.stderr
        import mcc
        return mcc.compile(rel, o)
    opt = [OPT.get(rel, OPT["default"])]
    if rel.endswith((".cpp", ".cc")):
        cmd = [tool("g++")] + opt + CXXFLAGS + ["-c", src, "-o", o]
    elif rel.endswith(".c"):
        cmd = [tool("gcc")] + opt + CFLAGS + ["-c", src, "-o", o]
    else:
        cmd = [tool("gcc")] + ASFLAGS + ["-c", src, "-o", o]
    r = run(cmd, cwd=ROOT)
    return r.returncode == 0, r.stderr

def compile_all(keep_going, jobs=None, progress=False):
    """Compile every source, `jobs` at a time. Messages come in the order the
    files finish; without keep_going the first failure cancels the files not
    yet started."""
    os.makedirs(OBJ, exist_ok=True)
    rels = sources()
    if COMPILER in RETAIL_CCS:
        import mcc
        mcc.use_cc = "991111-01" if COMPILER == "991111-01" else None
    fails, done = [], 0
    with cf.ThreadPoolExecutor(max_workers=jobs or os.cpu_count() or 1) as ex:
        futs = {ex.submit(compile_one, rel): rel for rel in rels}
        for fut in cf.as_completed(futs):
            if fut.cancelled():
                continue
            rel = futs[fut]
            ok, err = fut.result()
            done += 1
            if progress:
                print("[%d/%d] %s" % (done, len(rels), rel), flush=True)
            if not ok:
                fails.append(rel)
                print("FAIL compile %s\n%s" % (rel, err.strip()[-3000:]), flush=True)
                if not keep_going:
                    for f in futs:
                        f.cancel()
            elif COMPILER == "gcc14" and "warning:" in err:
                print("warn compile %s (%d warnings)" % (rel, err.count("warning:")), flush=True)
    return fails

def link_order(objs_rel):
    # original link order: by the lowest FUNC address in each file; link/ glue last
    def key(rel):
        a = first_func_addr(rel) if not rel.startswith("link/") else None
        return (a is None, a or 0, rel)
    return sorted(objs_rel, key=key)

def check_object(rel):
    problems = []
    o = obj_path(rel)
    r = run([tool("objdump"), "-h", o])
    for sec in (".ctors", ".dtors", ".init_array", ".fini_array", ".preinit_array"):
        if re.search(r"\s%s\s" % re.escape(sec), r.stdout):
            problems.append("%s: has %s (would never run)" % (rel, sec))
    # Hard-coded addresses into the retail image: a relocation-free lui whose
    # full value (with the low half of the next instruction using that
    # register) lands in the retail .data/.bss or on a retail function start.
    d = run([tool("objdump"), "-dr", "--no-show-raw-insn", o]).stdout.splitlines()
    for i, L in enumerate(d):
        m = re.search(r"\blui\s+(\w+),\s*0x([0-9a-f]+)\s*$", L)
        if not m:
            continue
        reg, hi = m.group(1), int(m.group(2), 16) << 16
        if not (BOOT_LO <= hi < BOOT_HI):
            continue
        if i + 1 < len(d) and "R_MIPS" in d[i + 1]:
            continue
        val = hi
        for L2 in d[i + 1:i + 6]:
            m2 = (re.search(r"\b(?:addiu|daddiu)\s+\w+,%s,(-?\d+)\s*$" % reg, L2) or
                  re.search(r"\b(?:l[bhwdq]u?|s[bhwdq]|lwc1|swc1)\s+\w+,(-?\d+)\(%s\)" % reg, L2))
            if m2:
                val = (hi + int(m2.group(1))) & 0xFFFFFFFF
                break
            m3 = re.search(r"\bori\s+\w+,%s,0x([0-9a-f]+)\s*$" % reg, L2)
            if m3:
                val = hi | int(m3.group(1), 16)
                break
        if RETAIL_DATA_LO <= val < RETAIL_DATA_HI or val in RETAIL_FUNCS:
            problems.append("%s: hard-coded retail address 0x%08X: %s" % (rel, val, L.strip()))
    return problems

def check_objects(rels, jobs=None):
    with cf.ThreadPoolExecutor(max_workers=jobs or os.cpu_count() or 1) as ex:
        return [p for probs in ex.map(check_object, rels) for p in probs]

TRACE_DIR = os.path.join(HERE, "trace")

# One shim per traced symbol (n32 ABI): save a0-a7, f12-f19 and ra, copy the
# caller's first 8 stack-argument slots so 9+-argument calls still work, log
# entry, call the real function with everything restored, log the result.
WRAP_TEMPLATE = """
	.section .rodata
.Lname_{n}: .asciz "{demangled}"
	.text
	.align 3
	.globl __wrap_{sym}
	.type __wrap_{sym}, @function
__wrap_{sym}:
	addiu $29, $29, -0xC0
	sd $31, 0xA0($29)
	sd $4, 0x40($29)
	sd $5, 0x48($29)
	sd $6, 0x50($29)
	sd $7, 0x58($29)
	sd $8, 0x60($29)
	sd $9, 0x68($29)
	sd $10, 0x70($29)
	sd $11, 0x78($29)
	swc1 $f12, 0x80($29)
	swc1 $f13, 0x84($29)
	swc1 $f14, 0x88($29)
	swc1 $f15, 0x8C($29)
	swc1 $f16, 0x90($29)
	swc1 $f17, 0x94($29)
	swc1 $f18, 0x98($29)
	swc1 $f19, 0x9C($29)
{copy}
	lui $4, %hi(.Lname_{n})
	addiu $4, $4, %lo(.Lname_{n})
	ld $5, 0x40($29)
	ld $6, 0x48($29)
	jal boot_trace_enter
	ld $7, 0x50($29)
	ld $4, 0x40($29)
	ld $5, 0x48($29)
	ld $6, 0x50($29)
	ld $7, 0x58($29)
	ld $8, 0x60($29)
	ld $9, 0x68($29)
	ld $10, 0x70($29)
	ld $11, 0x78($29)
	lwc1 $f12, 0x80($29)
	lwc1 $f13, 0x84($29)
	lwc1 $f14, 0x88($29)
	lwc1 $f15, 0x8C($29)
	lwc1 $f16, 0x90($29)
	lwc1 $f17, 0x94($29)
	lwc1 $f18, 0x98($29)
	jal __real_{sym}
	lwc1 $f19, 0x9C($29)
	sd $2, 0xA8($29)
	sd $3, 0xB0($29)
	swc1 $f0, 0xB8($29)
	lui $4, %hi(.Lname_{n})
	addiu $4, $4, %lo(.Lname_{n})
	jal boot_trace_exit
	daddu $5, $2, $0
	ld $2, 0xA8($29)
	ld $3, 0xB0($29)
	lwc1 $f0, 0xB8($29)
	ld $31, 0xA0($29)
	jr $31
	addiu $29, $29, 0xC0
	.size __wrap_{sym}, . - __wrap_{sym}
"""

def trace_objects():
    syms = []
    for L in open(os.path.join(TRACE_DIR, "trace_syms.txt"), encoding="utf-8"):
        L = L.split("#", 1)[0].strip()
        if L:
            syms.append(L)
    tdir = os.path.join(BUILD, "trace")
    os.makedirs(tdir, exist_ok=True)
    dem = run([tool("c++filt")] + syms).stdout.split("\n")
    copy = "\n".join("\tld $2, 0x%X($29)\n\tsd $2, 0x%X($29)" % (0xC0 + 8 * k, 8 * k) for k in range(8))
    parts = ["\t.set noreorder", "\t.set noat"]
    for n, s in enumerate(syms):
        d = (dem[n] if n < len(dem) and dem[n] else s).replace("\\", "\\\\").replace('"', '\\"')
        parts.append(WRAP_TEMPLATE.format(n=n, sym=s, demangled=d, copy=copy))
    wraps = os.path.join(tdir, "wraps.S")
    open(wraps, "w", encoding="utf-8", newline="\n").write("\n".join(parts) + "\n")
    objs = []
    for src, cmd in ((wraps, [tool("gcc")] + ASFLAGS),
                     (os.path.join(TRACE_DIR, "trace_rt.c"), [tool("gcc"), "-O2"] + CFLAGS)):
        o = os.path.join(tdir, os.path.basename(src) + ".o")
        r = run(cmd + ["-c", src, "-o", o], cwd=ROOT)
        if r.returncode:
            raise SystemExit("trace build failed:\n" + r.stderr)
        objs.append(o)
    return objs, ["-Wl,--wrap=" + s for s in syms]

def link(order, extra_objs=(), extra_flags=()):
    out = os.path.join(BUILD, "scus_971.02.elf")
    mp = os.path.join(BUILD, "scus_971.02.map")
    if COMPILER in RETAIL_CCS:
        # retail-compiler objects are EABI; the GCC runtime comes from the
        # toolchain's own libgcc.a, as in the retail link
        cmd = ([tool("ld"), "-m", "elf32lr5900", "--no-warn-mismatch", "-static",
                "-T", os.path.join(ROOT, "link", "bootstrap.ld"),
                "-Map=" + mp, "--no-undefined", "--cref"] +
               [f.replace("-Wl,", "") for f in extra_flags] +
               ["-o", out] + [obj_path(r) for r in order] + list(extra_objs) +
               [os.path.join(sys.modules["mcc"].CCDIR, "lib", "gcc-lib", "ee", "2.9-ee-991111", "libgcc.a")])
        return run(cmd, cwd=ROOT), out, mp
    cmd = ([tool("gcc"), "-nostdlib", "-nostartfiles", "-static",
            "-T", os.path.join(ROOT, "link", "bootstrap.ld"),
            "-Wl,-Map=" + mp, "-Wl,--no-undefined", "-Wl,--cref"] + list(extra_flags) +
           ["-o", out] + [obj_path(r) for r in order] + list(extra_objs) + FALLBACK_LIBS)
    r = run(cmd, cwd=ROOT)
    return r, out, mp

def elf_info(path):
    d = open(path, "rb").read()
    e_entry, e_phoff, e_shoff = struct.unpack_from("<III", d, 0x18)
    e_phentsize, e_phnum = struct.unpack_from("<HH", d, 0x2A)
    segs = []
    for i in range(e_phnum):
        t, off, va, pa, fsz, msz, fl, al = struct.unpack_from("<8I", d, e_phoff + i * e_phentsize)
        segs.append(dict(type=t, off=off, va=va, filesz=fsz, memsz=msz, flags=fl, align=al))
    return e_entry, segs, len(d)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", metavar="PATH", help="also copy the finished ELF here")
    ap.add_argument("--no-install", action="store_true", help="ignore --out")
    ap.add_argument("--keep-going", action="store_true")
    ap.add_argument("--compiler", choices=("gcc14",) + RETAIL_CCS, default="gcc14",
                    help="gcc14 (default, modern GCC), sce (the retail compiler) or "
                         "991111-01 (the reference compiler)")
    ap.add_argument("--toolchain", metavar="DIR",
                    help="the EE toolchain: its bin folder, or a ps2sdk/ps2dev folder with ee/bin "
                         "(default: tools/paths.py)")
    ap.add_argument("--jobs", "-j", type=int, default=0, metavar="N",
                    help="files to compile at once (default: one per CPU)")
    ap.add_argument("--progress", action="store_true",
                    help="print a [done/total] line as each file finishes compiling")
    ap.add_argument("--build-dir", metavar="DIR",
                    help="output directory instead of build/ (gcc14) or build/<compiler>/; "
                         "lets several builds run side by side")
    ap.add_argument("--trace", action="store_true",
                    help="debug build: log entry/exit of tools/trace/trace_syms.txt (never installed)")
    ap.add_argument("--opt", default="-O2", help="optimisation level for every file (default -O2)")
    ap.add_argument("--opt-file", action="append", default=[], metavar="REL=LEVEL",
                    help="per-file override, e.g. sdk/libkernl/sifrpc.c=-O0 (repeatable)")
    a = ap.parse_args()
    OPT["default"] = a.opt
    for spec in a.opt_file:
        rel, _, lvl = spec.partition("=")
        OPT[rel.replace("\\", "/")] = lvl
    if a.trace or a.opt != "-O2" or a.opt_file:
        if a.out and not a.no_install:
            print("debug builds (--trace, --opt, --opt-file) are not copied to --out")
        a.no_install = True
    global COMPILER, BUILD, OBJ, TC_BIN
    COMPILER = a.compiler
    TC_BIN = paths.ee_bin(a.toolchain)
    if TC_BIN is None:
        print("no EE toolchain (%sgcc) %s" % (PREFIX, "in " + a.toolchain if a.toolchain else
              "found: the GT3 Decomp Builder downloads it, or pass its folder with --toolchain"))
        return 1
    if COMPILER in RETAIL_CCS:
        if a.trace or a.opt != "-O2" or a.opt_file:
            print("--trace/--opt/--opt-file apply to the gcc14 build only (%s flags: ref/compile_flags.json)" % COMPILER)
            return 1
        BUILD = os.path.join(ROOT, "build", COMPILER)
        OBJ = os.path.join(BUILD, "obj")
    if a.build_dir:
        BUILD = os.path.abspath(a.build_dir)
        OBJ = os.path.join(BUILD, "obj")
    # a failed build must not leave an older image behind under the final name
    for stale in ("scus_971.02", "scus_971.02.elf"):
        if os.path.exists(os.path.join(BUILD, stale)):
            os.remove(os.path.join(BUILD, stale))
    t0 = time.time()

    if not ensure_assets():
        return 1
    jobs = a.jobs or os.cpu_count() or 1
    print("== compile (%s, %d at a time)" % (COMPILER, jobs))
    if COMPILER in RETAIL_CCS:
        import mcc
        mcc.use_cc = "991111-01" if COMPILER == "991111-01" else None
        why = mcc.missing()
        if why:
            print("FAIL compile: " + why); return 1
    fails = compile_all(a.keep_going, jobs, a.progress)
    if fails:
        print("compile failed: %d file(s)" % len(fails)); return 1
    rels = sources()

    print("== check")
    probs = check_objects(rels, jobs)
    for p in probs:
        print("  " + p)
    if probs:
        print("object checks failed: %d problem(s)" % len(probs)); return 1

    print("== link")
    order = link_order(rels)
    extra_objs, extra_flags = trace_objects() if a.trace else ([], [])
    r, elf, mp = link(order, extra_objs, extra_flags)
    if r.returncode:
        print(r.stderr.strip()[-20000:]); print("link failed"); return 1

    print("== finish")
    final = os.path.join(BUILD, "scus_971.02")
    shutil.copyfile(elf, final)
    s = run([tool("strip"), "--strip-all", "-R", ".MIPS.abiflags", final])
    if s.returncode:
        print(s.stderr); return 1
    al = subprocess.run([sys.executable, ALIGN_TOOL, final], capture_output=True, text=True)
    print(al.stdout.strip())
    if al.returncode:
        print(al.stderr.strip()); print("segment alignment / load replay failed"); return 1
    entry, segs, size = elf_info(final)
    loads = [s for s in segs if s["type"] == 1]
    print("entry 0x%08X, %d bytes, %d segment(s)" % (entry, size, len(segs)))
    for sg in segs:
        print("  type %d  off 0x%06X  va 0x%08X  filesz 0x%06X  memsz 0x%06X" % (
            sg["type"], sg["off"], sg["va"], sg["filesz"], sg["memsz"]))
    bad = [sg for sg in loads if sg["va"] < 0x00800000 or sg["va"] + sg["memsz"] > 0x01F00000]
    if len(loads) != len(segs) or bad or not (loads and loads[0]["va"] <= entry < loads[0]["va"] + loads[0]["filesz"]):
        print("ELF sanity check failed (non-LOAD segment, overlap with core.gt3/stack, or entry outside)"); return 1
    sha = hashlib.sha256(open(final, "rb").read()).hexdigest().upper()
    print("sha256 %s" % sha)
    print("built %s in %.1f s" % (final, time.time() - t0))

    if a.no_install or not a.out:
        return 0
    print("== install")
    out = os.path.abspath(a.out)
    if os.path.exists(out):
        cur = hashlib.sha256(open(out, "rb").read()).hexdigest().upper()
        if cur == RETAIL_SHA256:
            backup_dir = os.path.join(ROOT, "build", "backup")
            os.makedirs(backup_dir, exist_ok=True)
            bk = os.path.join(backup_dir, os.path.basename(out) + ".retail")
            if not os.path.exists(bk):
                shutil.copyfile(out, bk)
                print("backed up retail file to %s" % bk)
    shutil.copyfile(final, out)
    print("wrote %s" % out)
    return 0

if __name__ == "__main__":
    sys.exit(main())
