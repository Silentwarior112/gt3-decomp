"""Matching build: reassemble the splat split (matching/) into scus_971.02 and
check it is byte-identical to the retail file.

    python tools/build_matching.py [--resplit]

  --resplit   regenerate matching/ first (tools/gen_symbol_addrs.py,
              tools/gen_splat.py, splat split)

Steps: assemble every matching/asm/**/*.s (GNU as from the ps2sdk toolchain,
-march=r5900, no automatic R5900 short-loop nops: the split is .set noreorder
and already contains the retail nops), wrap the two ELF container blobs in
matching/assets as objects, link with splat's matching/scus_971.02.ld, write
the result as a flat file image (objcopy -O binary) and compare it with the
retail file (baserom/SCUS_971.02). Output: matching/build/scus_971.02.bin (+ .elf, .map).
"""
import argparse, hashlib, os, subprocess, sys

import paths

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
MDIR = os.path.join(ROOT, "matching")
PFX = paths.EE_PREFIX
RETAIL = paths.retail_elf()
# EABI: the PS2's ABI, and the register naming spimdisasm writes ($t0-$t7 =
# $8-$15); under the toolchain's default n32, $t4-$t7 would not assemble.
AS_FLAGS = ["-march=r5900", "-mabi=eabi", "-mgp64", "-EL", "-G0", "-mno-fix-r5900",
            "-I", os.path.join(MDIR, "include")]

def tool(n):
    return paths.ee_tool(n)

def run(cmd, cwd=MDIR):
    env = dict(os.environ)
    env["PATH"] = os.path.dirname(tool("as")) + os.pathsep + env.get("PATH", "")
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, env=env)

def resplit():
    for c in ([sys.executable, os.path.join(HERE, "gen_symbol_addrs.py")],
              [sys.executable, os.path.join(HERE, "gen_splat.py")],
              [sys.executable, "-m", "splat", "split", "scus_971.02.yaml"]):
        r = run(c)
        if r.returncode:
            print(r.stdout[-3000:], r.stderr[-3000:]); raise SystemExit("resplit failed: %s" % c)

def fixups():
    """Corrections to splat's output that the YAML cannot express (idempotent):
      * .sdata is 0xEB bytes; spimdisasm drops a trailing partial word, so
        the last 3 bytes ("%s\\0" at 0x01041E68) are written here;
      * retail .sbss and .bss start 128-aligned (0x01041E80, 0x01041F00);
      * the ELF trailer follows the loaded image at file offset 0x42E6B
        exactly, with no 16-byte ROM rounding in between."""
    tail = os.path.join(MDIR, "asm", "data", "data", "sdata_tail.sdata.s")
    if os.path.exists(tail) and ".byte" not in open(tail).read():
        with open(tail, "a") as fh:
            fh.write("    /* 42E68 01041E68 */ .byte 0x25, 0x73, 0x00   /* \"%s\" */\n")
    ld = os.path.join(MDIR, "scus_971.02.ld")
    t = open(ld).read()
    t2 = t.replace("        main_SBSS_START = .;", "        . = ALIGN(0x80);\n        main_SBSS_START = .;", 1) \
        if "ALIGN(0x80);\n        main_SBSS_START" not in t else t
    t2 = t2.replace("        main_BSS_START = .;", "        . = ALIGN(0x80);\n        main_BSS_START = .;", 1) \
        if "ALIGN(0x80);\n        main_BSS_START" not in t2 else t2
    t2 = t2.replace("    __romPos += SIZEOF(.main);\n    __romPos = ALIGN(__romPos, 16);\n",
                    "    __romPos += SIZEOF(.main);\n")
    if t2 != t:
        open(ld, "w", newline="\n").write(t2)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--resplit", action="store_true")
    a = ap.parse_args()
    if a.resplit:
        resplit()
    fixups()
    build = os.path.join(MDIR, "build")
    objs, fails = [], 0
    for dp, _, fns in os.walk(os.path.join(MDIR, "asm")):
        for fn in sorted(fns):
            if not fn.endswith(".s"):
                continue
            src = os.path.join(dp, fn)
            rel = os.path.relpath(src, MDIR).replace("\\", "/")
            out = os.path.join(build, rel + ".o")
            os.makedirs(os.path.dirname(out), exist_ok=True)
            r = run([tool("as")] + AS_FLAGS + ["-o", out, src])
            if r.returncode:
                fails += 1
                print("FAIL as %s\n%s" % (rel, r.stderr[-2500:]))
            objs.append(out)
    for blob in ("elf_header", "elf_trailer"):
        # objcopy's binary input gives a byte-aligned .data section; an assembled
        # .incbin would get gas's default 16-byte .data alignment and move the
        # trailer off the retail file offset 0x42E6B.
        out = os.path.join(build, "assets", blob + ".bin.o")
        os.makedirs(os.path.dirname(out), exist_ok=True)
        r = run([tool("objcopy"), "-I", "binary", "-O", "elf32-littlemips", "-B", "mips",
                 os.path.join("assets", blob + ".bin"), out])
        if r.returncode:
            fails += 1
            print("FAIL objcopy %s\n%s" % (blob, r.stderr[-1500:]))
    if fails:
        print("%d file(s) failed to assemble" % fails); return 1
    elf = os.path.join(build, "scus_971.02.elf")
    r = run([tool("ld"), "-m", "elf32lr5900",          # the non-n32 (EABI/o32) R5900 emulation
             "--no-warn-mismatch",                     # the two container blobs are flagged 32-bit data
             "-T", "scus_971.02.ld", "-T", "undefined_syms_auto.txt", "-T", "undefined_funcs_auto.txt",
             "-Map", os.path.join(build, "scus_971.02.map"), "--no-check-sections", "-o", elf])
    if r.returncode:
        print(r.stderr[-6000:]); print("link failed"); return 1
    binp = os.path.join(build, "scus_971.02.bin")
    r = run([tool("objcopy"), "-O", "binary", "--gap-fill", "0x00", elf, binp])
    if r.returncode:
        print(r.stderr); return 1
    got, want = open(binp, "rb").read(), open(RETAIL, "rb").read()
    if got == want:
        print("MATCH: matching/build/scus_971.02.bin is byte-identical to the retail file (sha1 %s)"
              % hashlib.sha1(got).hexdigest())
        return 0
    diff = [i for i in range(min(len(got), len(want))) if got[i] != want[i]]
    print("DIFFERENT: %d vs %d bytes, %d differing bytes in the common part%s" % (
        len(got), len(want), len(diff), (", first at file offset 0x%X" % diff[0]) if diff else ""))
    return 1

if __name__ == "__main__":
    sys.exit(main())
