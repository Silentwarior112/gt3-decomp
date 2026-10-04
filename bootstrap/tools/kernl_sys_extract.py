"""kernl_sys data extractor (unit kernl_sys: sdk/libkernl/{syscalls.S,kernel.c,sifcmd.c}).

Reads the bootstrap ELF and prints / checks the .data/.rodata objects that
libkernl part 1 embeds:

  0x0103D278  "PsIIlibkernl2240" version string (16 bytes, no NUL)
  0x0103D288  alarm kernel patch, 0x740 bytes of EE kernel-mode code copied by
              InitAlarm to 0x80076000 (Copy syscall 0x5A)
  0x0103D9C8  alarm callback trampoline, 0x20 bytes copied to 0x00082000
  0x0103D9E8  InitAlarm syscall install table, 8 x {int num; void *handler}
  0x0103DA28  thread-server tid / kputs count / putc hook / sifcmd+sifrpc init flags
  0x01040510  "## internel error in libkernl.a!\\n"
  0x01040540  "0.%d" "e+%d" "e%d" and the doubles 0.1, 0.1, 1e6 used by _print_double
  0x01040570  0x49-entry jump table of the kernel printf conversion switch

Usage:
  python tools/kernl_sys_extract.py            print everything (C initialisers)
  python tools/kernl_sys_extract.py --check    verify the arrays embedded in
                                               sdk/libkernl/kernel.c against the ELF
"""
import os, re, struct, sys

import paths

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
ELF = paths.retail_elf()

data = open(ELF, "rb").read()


def off(va):
    return va - 0x01000000 + 0x1000


def words(va, n):
    return list(struct.unpack_from("<%dI" % n, data, off(va)))


def c_words(ws, per_line=6, indent="    "):
    out = []
    for i in range(0, len(ws), per_line):
        chunk = ws[i:i + per_line]
        out.append(indent + ", ".join("0x%08X" % w for w in chunk) + ",")
    return "\n".join(out)


def alarm_code():
    return words(0x0103D288, 0x740 // 4)


def alarm_stub():
    return words(0x0103D9C8, 0x20 // 4)


def main():
    if "--check" in sys.argv:
        src = open(os.path.join(ROOT, "sdk", "libkernl", "kernel.c"), encoding="utf-8").read()
        ok = True
        for name, ws in (("_alarm_patch_code", alarm_code()), ("_alarm_patch_stub", alarm_stub())):
            m = re.search(name + r"\[[^\]]*\][^=]*=\s*\{(.*?)\};", src, re.S)
            if not m:
                print("MISSING", name)
                ok = False
                continue
            got = [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]{8}", m.group(1))]
            if got != ws:
                print("MISMATCH", name, len(got), len(ws))
                ok = False
            else:
                print("OK", name, "%d words" % len(ws))
        sys.exit(0 if ok else 1)

    print("/* version @0x0103D278 */ %r" % data[off(0x0103D278):off(0x0103D288)])
    print("/* alarm patch code @0x0103D288, 0x740 bytes */")
    print(c_words(alarm_code()))
    print("/* alarm trampoline @0x0103D9C8, 0x20 bytes */")
    print(c_words(alarm_stub()))
    tbl = words(0x0103D9E8, 16)
    print("/* InitAlarm table @0x0103D9E8 */")
    for i in range(0, 16, 2):
        print("    { 0x%X, 0x%08X }," % (tbl[i], tbl[i + 1]))
    print("/* .data 0x0103DA28.. */", ["0x%X" % w for w in words(0x0103DA28, 5)])
    print("/* blob service table (blob+0x710 = 0x80076710) */")
    t = struct.unpack_from("<12I", data, off(0x0103D288) + 0x710)
    for i in range(0, 12, 2):
        print("    service 0x%X -> 0x%08X" % (t[i], t[i + 1]))
    s = data[off(0x01040510):off(0x01040540)]
    print("/* 0x01040510 */", s.split(b"\0")[0])
    for va in (0x01040540, 0x01040548, 0x01040550):
        print("/* 0x%08X */" % va, data[off(va):off(va) + 8].split(b"\0")[0])
    for va in (0x01040558, 0x01040560, 0x01040568):
        print("/* 0x%08X */ double %r" % (va, struct.unpack_from("<d", data, off(va))[0]))
    jt = words(0x01040570, 0x49)
    print("/* printf switch table @0x01040570 (index = c - '0') */")
    default = 0x0101D238
    for i, w in enumerate(jt):
        if w != default:
            print("    '%c' -> 0x%08X" % (0x30 + i, w))
    print("    (all other entries -> 0x%08X default)" % default)


if __name__ == "__main__":
    main()
