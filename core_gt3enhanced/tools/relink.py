"""Relink the original program from its splat split and check it is byte-identical.

    python tools/relink.py                 assemble ref/splat/, link, compare with baserom/
    python tools/relink.py --pack OUT      ... and pack the result into a signed core.gt3

ref/splat/ (tools/regen.py: gen_splat.py + splat) holds the main executable
split into assembly: one file per work unit (asm/<unit>.s), the data
(asm/data/data.rodata.s, with pointers written as symbols), the .bss, the
exception tables and the ELF's own header and trailer as binary assets, and
the linker script core.ld. This tool assembles all of it with the EE
toolchain, links it at the original addresses ($gp 0x003579F0) and writes
the file image (objcopy -O binary by load address), which must equal
baserom/BASE_SCUS-97102 byte for byte. That proves the split, the
assembler and the linker script reproduce the original exactly: the base
the hybrid build (units swapped from assembly to compiled C) starts from.

Output in build/relink/ (or --build-dir): obj/, core.ld (core.ld with this
build's paths), core.elf, core.bin (the file image), core.map.
--toolchain DIR picks the EE toolchain (default: tools/paths.py).
Exit status 0 only when the image is identical (and --pack wrote OUT).
"""
import argparse
import concurrent.futures as cf
import hashlib
import os
import re
import subprocess
import sys
import time

import gt3image
import paths

SPLAT = os.path.join(paths.REF, "splat")
# the splat assembly: GNU as syntax for the R5900, no reordering (it says so itself)
AS_FLAGS = ["-EL", "-march=r5900", "-mabi=eabi", "-G0", "-no-pad-sections"]
TC_BIN = None


def tool(name):
    return os.path.join(TC_BIN, paths.EE_PREFIX + name + paths.EXE)


def run(cmd, cwd=None):
    return subprocess.run(cmd, capture_output=True, text=True, errors="replace", cwd=cwd)


def inputs(ld_text):
    """The object paths core.ld names, e.g. build/asm/u000.s.o, in order."""
    seen, out = set(), []
    for m in re.finditer(r"(build/[^\s()]+\.o)\(", ld_text):
        if m.group(1) not in seen:
            seen.add(m.group(1))
            out.append(m.group(1))
    return out


def exact_layout(ld_text):
    """core.ld without splat's 16-byte rounding, which the original does not
    have: SUBALIGN(16) would put every unit on a 16-byte boundary (u003 starts
    at 0x001060B8), `. = ALIGN(., 16)` would pad the end of the exception
    tables (the data starts at 0x0030729C) and __romPos = ALIGN(__romPos, 16)
    the ELF trailer (at file offset 0x2541E3).  Each input section keeps the
    alignment its assembly asks for, and the .bss goes to its own address."""
    t = ld_text.replace(" SUBALIGN(16)", "")
    t = re.sub(r"(?m)^\s*\. = ALIGN\(\., 16\);\n", "", t)
    t = re.sub(r"(?m)^\s*__romPos = ALIGN\(__romPos, 16\);\n", "", t)
    # the .bss starts at its own address (core.yaml), not where the data ends
    m = re.search(r"type: bss, vram: (0x[0-9A-Fa-f]+)", open(os.path.join(SPLAT, "core.yaml")).read())
    return t.replace(".main_bss (NOLOAD) :", ".main_bss %s (NOLOAD) :" % m.group(1)) if m else t


def source_of(obj):
    """build/asm/u000.s.o -> asm/u000.s; build/assets/x.bin.o -> assets/x.bin."""
    return obj[len("build/"):-len(".o")]


def assemble(obj, out_dir):
    """(ok, messages): one object of core.ld from the split."""
    src = source_of(obj)
    dst = os.path.join(out_dir, obj[len("build/"):])
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if src.endswith(".bin"):
        stub = dst + ".s"               # a binary asset becomes the .data of an object
        with open(stub, "w", encoding="utf-8") as f:
            f.write('.section .data\n.incbin "%s"\n' % os.path.join(SPLAT, src).replace("\\", "/"))
        r = run([tool("as")] + AS_FLAGS + ["-o", dst, stub])
    else:
        r = run([tool("as")] + AS_FLAGS + ["-I", os.path.join(SPLAT, "include"), "-I", SPLAT,
                                           "-o", dst, os.path.join(SPLAT, src)], cwd=SPLAT)
    return r.returncode == 0, (r.stdout + r.stderr).strip()


def first_difference(a, b):
    n = min(len(a), len(b))
    for i in range(0, n, 4096):
        if a[i:i + 4096] != b[i:i + 4096]:
            for j in range(i, min(i + 4096, n)):
                if a[j] != b[j]:
                    return j
    return n if len(a) != len(b) else None


def relink(out, jobs):
    """Assemble the split and link it in `out`.  Prints its stages; returns the
    file image (bytes) when it equals the original, else None."""
    ld_path = os.path.join(SPLAT, "core.ld")
    if not os.path.isfile(ld_path):
        print("error: no split in %s: run python tools/regen.py (gen_splat.py and splat)" % SPLAT)
        return None
    retail = open(paths.retail_elf(), "rb").read()
    obj_dir = os.path.join(out, "obj")
    os.makedirs(obj_dir, exist_ok=True)

    ld_text = open(ld_path, encoding="utf-8").read()
    objs = inputs(ld_text)
    print("== assemble (%d files of the split)" % len(objs), flush=True)
    fails = []
    with cf.ThreadPoolExecutor(max_workers=max(1, jobs)) as ex:
        for obj, (ok, msg) in zip(objs, ex.map(lambda o: assemble(o, obj_dir), objs)):
            if not ok:
                fails.append(obj)
                print("FAIL assemble %s\n%s" % (source_of(obj), msg[-2000:]), flush=True)
    if fails:
        print("error: %d file(s) failed to assemble" % len(fails))
        return None

    print("== link", flush=True)
    script = os.path.join(out, "core.ld")
    with open(script, "w", encoding="utf-8") as f:
        f.write(exact_layout(ld_text).replace("build/", paths.ld_path(obj_dir) + "/"))
    elf, image, mapf = (os.path.join(out, n) for n in ("core.elf", "core.bin", "core.map"))
    for p in (elf, image):
        if os.path.exists(p):
            os.remove(p)
    r = run([tool("ld"), "-EL", "--no-check-sections", "--no-warn-mismatch", "-nostdlib", "-T", script,
             "-T", os.path.join(SPLAT, "undefined_syms_auto.txt"),
             "-T", os.path.join(SPLAT, "undefined_funcs_auto.txt"),
             "-Map", mapf, "-o", elf], cwd=paths.ROOT)          # the script's objects are ROOT-relative
    if r.returncode:
        print((r.stdout + r.stderr).strip()[-4000:])
        print("error: the link failed")
        return None
    r = run([tool("objcopy"), "-O", "binary", elf, image])
    if r.returncode:
        print((r.stdout + r.stderr).strip()[-2000:])
        print("error: objcopy failed")
        return None

    print("== compare with %s" % paths.RETAIL_ELF, flush=True)
    got = open(image, "rb").read()
    d = first_difference(got, retail)
    if d is not None:
        print("error: the image differs from the original: %d bytes vs %d, first difference at file "
              "offset 0x%X (address 0x%08X if in the program)" % (len(got), len(retail), d, d - 0x1000 + 0x00100000))
        return None
    print("identical: %d bytes, SHA-256 %s" % (len(got), hashlib.sha256(got).hexdigest().upper()[:16] + "..."))
    return got


def pack(image, path):
    """A signed core.gt3 of the ELF file image `image` at `path` (loadFlags 0x0101,
    the key from gt3image's default seed: the same image gives the same file)."""
    print("== pack %s" % path, flush=True)
    img = gt3image.Image.from_elf(image)
    img.sign()
    raw = img.pack()
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "wb") as f:
        f.write(raw)
    for line in gt3image.describe(raw):
        print("  " + line)


def main():
    global TC_BIN
    ap = argparse.ArgumentParser(description="Relink the original program from its splat split.")
    ap.add_argument("--toolchain", help="the EE toolchain (default: tools/paths.py)")
    ap.add_argument("--build-dir", default=os.path.join(paths.ROOT, "build", "relink"))
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    ap.add_argument("--pack", metavar="OUT", help="also write a signed core.gt3 of the result there")
    a = ap.parse_args()
    t0 = time.time()
    TC_BIN = paths.ee_bin(a.toolchain)
    if not TC_BIN:
        print("error: no EE toolchain (GCC 14): pass --toolchain, or see the README (Downloads)")
        return 1
    image = relink(os.path.abspath(a.build_dir), a.jobs)
    if image is None:
        return 1
    if a.pack:
        pack(image, a.pack)
    print("done in %.1f s" % (time.time() - t0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
