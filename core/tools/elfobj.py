"""Minimal ELF32 little-endian MIPS object reader for the matching tools:
sections, symbols, REL relocations and stabs (N_SO/N_SOL/N_FUN) line records."""
import struct

R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16, R_MIPS_LITERAL = 2, 4, 5, 6, 7, 8
R_MIPS_PC16, R_MIPS_GPREL32 = 10, 12

# bits of the instruction word a relocation of this type rewrites
RELOC_FIELD = {R_MIPS_26: 0x03FFFFFF, R_MIPS_HI16: 0x0000FFFF, R_MIPS_LO16: 0x0000FFFF,
               R_MIPS_GPREL16: 0x0000FFFF, R_MIPS_LITERAL: 0x0000FFFF, R_MIPS_PC16: 0x0000FFFF,
               R_MIPS_32: 0xFFFFFFFF, R_MIPS_GPREL32: 0xFFFFFFFF}

class Section:
    __slots__ = ("index", "name", "type", "flags", "addr", "offset", "size", "link", "info", "align", "entsize", "data")

class Symbol:
    __slots__ = ("name", "value", "size", "bind", "type", "shndx")

class Reloc:
    __slots__ = ("offset", "type", "sym")

class ElfObject:
    def __init__(self, path_or_bytes):
        d = path_or_bytes if isinstance(path_or_bytes, (bytes, bytearray)) else open(path_or_bytes, "rb").read()
        if d[:4] != b"\x7fELF" or d[4] != 1 or d[5] != 1:
            raise ValueError("not an ELF32 little-endian file")
        self.d = d
        e_shoff, = struct.unpack_from("<I", d, 0x20)
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", d, 0x2E)
        raw = [struct.unpack_from("<10I", d, e_shoff + i * e_shentsize) for i in range(e_shnum)]
        strtab = raw[e_shstrndx][4] if e_shnum else 0
        self.sections = []
        for i, r in enumerate(raw):
            s = Section()
            s.index = i
            s.name = self._cstr(strtab + r[0])
            # r = (name, type, flags, addr, offset, size, link, info, addralign, entsize)
            s.type, s.flags, s.addr, s.offset, s.size, s.link, s.info = r[1:8]
            s.align = r[8]
            s.entsize = r[9]
            s.data = d[s.offset:s.offset + s.size] if s.type != 8 else b""
            self.sections.append(s)
        self.symbols = []
        for s in self.sections:
            if s.type == 2:                                   # SHT_SYMTAB
                strsec = self.sections[s.link]
                for k in range(s.size // 16):
                    n, v, sz, info, other, shndx = struct.unpack_from("<IIIBBH", d, s.offset + 16 * k)
                    y = Symbol()
                    y.name = self._cstr(strsec.offset + n)
                    y.value, y.size, y.bind, y.type, y.shndx = v, sz, info >> 4, info & 0xF, shndx
                    self.symbols.append(y)
        self.relocs = {}                                      # section index -> [Reloc]
        for s in self.sections:
            if s.type == 9:                                   # SHT_REL
                lst = self.relocs.setdefault(s.info, [])
                for k in range(s.size // 8):
                    off, info = struct.unpack_from("<II", d, s.offset + 8 * k)
                    r = Reloc()
                    r.offset, r.type, r.sym = off, info & 0xFF, self.symbols[info >> 8] if (info >> 8) < len(self.symbols) else None
                    lst.append(r)

    def _cstr(self, off):
        e = self.d.index(b"\0", off)
        return self.d[off:e].decode("latin1")

    def section(self, name):
        return next((s for s in self.sections if s.name == name), None)

    def functions(self):
        """(symbol, section) for every function symbol in an executable section,
        with a size filled in from the next symbol when the object left it 0."""
        out = []
        for s in self.sections:
            if s.type != 1 or not (s.flags & 4):
                continue
            fs = sorted([y for y in self.symbols if y.shndx == s.index and y.type == 2], key=lambda y: y.value)
            for i, y in enumerate(fs):
                if y.size == 0:
                    nxt = fs[i + 1].value if i + 1 < len(fs) else s.size
                    y.size = nxt - y.value
                out.append((y, s))
        return out

    def stabs_functions(self):
        """[(name, line, file)] from N_FUN stabs: the defining line of every
        function and the source file (N_SO / N_SOL) it is in."""
        st, ss = self.section(".stab"), self.section(".stabstr")
        if not st or not ss:
            return []
        out, cur, main = [], None, None
        for k in range(st.size // 12):
            strx, typ, other, desc, value = struct.unpack_from("<IBBHI", st.data, 12 * k)
            s = ""
            if strx:
                e = ss.data.index(b"\0", strx)
                s = ss.data[strx:e].decode("latin1")
            if typ == 0x64 and s and not s.endswith("/"):     # N_SO
                cur = main = s
            elif typ == 0x84:                                  # N_SOL
                cur = s
            elif typ == 0x24 and s:                            # N_FUN
                name = s.split(":", 1)[0]
                out.append((name, desc, cur, cur == main))
        return out
