import struct, sys

import paths

p = sys.argv[1] if len(sys.argv) > 1 else paths.retail_elf()
d = open(p, "rb").read()
print("size", hex(len(d)))
assert d[:4] == b"\x7fELF"
e_entry, e_phoff, e_shoff = struct.unpack_from("<III", d, 0x18)
e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<5H", d, 0x2A)
print("entry", hex(e_entry), "phnum", e_phnum, "shnum", e_shnum, "shstrndx", e_shstrndx)
for i in range(e_phnum):
    o = e_phoff + i * e_phentsize
    t, off, va, pa, fsz, msz, fl, al = struct.unpack_from("<8I", d, o)
    print("PH%d type=%d off=%s va=%s filesz=%s memsz=%s flags=%d" % (i, t, hex(off), hex(va), hex(fsz), hex(msz), fl))
shs = []
for i in range(e_shnum):
    off = e_shoff + i * e_shentsize
    shs.append(struct.unpack_from("<10I", d, off))
strtab_off = shs[e_shstrndx][4]
def sname(n):
    e = d.index(b"\0", strtab_off + n)
    return d[strtab_off + n:e].decode()
for i, s in enumerate(shs):
    print("%2d %-16s type=%-2d addr=%-10s off=%-8s size=%-8s link=%d entsize=%d" %
          (i, sname(s[0]), s[1], hex(s[3]), hex(s[4]), hex(s[5]), s[6], s[9]))
# dump symbols if present
for i, s in enumerate(shs):
    if s[1] == 2:  # SHT_SYMTAB
        symoff, symsize, entsize, link = s[4], s[5], s[9], s[6]
        stro = shs[link][4]
        def symname(n):
            e = d.index(b"\0", stro + n)
            return d[stro + n:e].decode()
        n = symsize // entsize
        print("SYMTAB: %d symbols" % n)
        for j in range(n):
            name, value, size, info, other, shndx = struct.unpack_from("<IIIBBH", d, symoff + j * entsize)
            typ = info & 0xF
            bind = info >> 4
            nm = symname(name)
            if nm:
                print("  %s val=%s size=%d type=%d bind=%d shndx=%d" % (nm, hex(value), size, typ, bind, shndx))
