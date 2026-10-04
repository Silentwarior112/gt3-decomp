"""difftest.py - function-level behaviour differential test: retail vs rebuilt.

    python tools/difftest.py --elf matching/build/scus_971.02.elf   # the byte-identical build
    python tools/difftest.py                    # default --elf build/sce/scus_971.02.elf
    python tools/difftest.py --group bignum     # one group's specs (repeatable)
    python tools/difftest.py --fn strlen --fn 0x01011700 --n 50
    python tools/difftest.py --list [--group G] # groups, specs, the functions they cover

Runs each retail function (baserom/SCUS_971.02, tools/paths.py) and its rebuilt
counterpart (found through the rebuilt ELF's symbol table) in the EE interpreter
tools/eeemu.py on identical inputs, and compares everything observable:

  * return values ($v0, 64 bits, or the width the spec declares);
  * every byte written outside the stack: buffers and heap blocks by address (they
    sit at the same addresses on both sides), the image's own .data/.bss by symbol
    (see "Image addresses" below);
  * the calls made to stubbed functions, in order, with their arguments: the
    allocator (malloc/free/calloc/realloc/memalign, operator new/delete[/[]]) is a
    deterministic Python heap on both sides, kernel output (scePrintf) is captured;
  * errors (an EmuError: unknown instruction, misaligned or wild access, runaway
    loop, a callee-saved register not restored, a heap misuse): an error on one
    side only is a failure.  The same error on both sides is the same behaviour:
    the error class must agree, and for a fault the faulting address too (compared
    like a pointer: equal, both on the stack, or the same image object through the
    tokens below - never the pc, which differs in a non-identical build); then the
    events before the error are compared (only as a common prefix for a runaway,
    whose cut-off point depends on the instruction count).  Memory is not compared
    after an error.  A write above the caller's stack pointer is always a failure.

Test definitions (inputs, which functions, how many cases) are in the package
tools/difftest_specs/, one module per group (see its __init__); MATCHING.md
"Function-level behaviour tests" explains how to add one.

Rebuilt symbols.  A retail address is looked up under every name known for it:
matching/symbol_addrs.txt (the matching build's names), build/score/symmap.json
(egcs-mangled names of the compiled C, written by tools/score.py), ref/libmatch.json
(toolchain libgcc.a names of the runtime units) and plain identifiers in the FUNC
markers.  All names found in the rebuilt ELF must agree on one address.

Image addresses.  When the rebuilt image is byte-identical to retail (the matching
build) all addresses are compared as they are, and the harness checks that every
resolved function sits at its retail address.  Otherwise writes into the image and
pointers into it are compared through tokens: a function is its retail address
(through the symbol mapping), a global is (C name, offset) through the source
annotations (`// @0x0104xxxx` on the definition, `.S` data labels) and the rebuilt
symbol table, a type_info node (__ti...) is the __tf function that returns it, a
vtable pointer is the list of its entries' functions, a string pointer is the
string, a pointer one past the end of an object is that object's end.  Linker
symbols (_fbss, _end ...) are read by specs with T.sym.

The GCC 14 build (an n32 ELF, Itanium C++, -fipa-ra) is compared through an
adapter (Harness.adapt, MATCHING.md "The GCC 14 build"): calls and stubs go
through the functions' source prototypes in each side's calling convention,
GNU v2 constructor returns / destructor flags map to the Itanium functions,
polymorphic objects and vtables registered by the specs compare at their GNU v2
places, stubs of same-unit callees keep the caller-saved registers, and a case
that needs a function or static GCC did not emit is skipped (NotEmitted).

Exit status: 0 = every selected spec passed (or skipped).
"""
import argparse, bisect, collections, hashlib, json, os, random, re, struct, sys, time, traceback

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import eeemu as E
import paths
import v2mangle

if __name__ == "__main__":
    # the spec modules do `from difftest import ...`: make that this module, not a
    # second copy (HarnessError raised by a spec must be the one caught here)
    sys.modules.setdefault("difftest", sys.modules["__main__"])

RETAIL_PATH = paths.RETAIL_ELF
RETAIL_GP = 0x01049CF0
# linker-script symbols of the retail link (from the instructions of _start,
# 0x01000008, which build them with lui/addiu); the rebuilt side reads its ELF
# symbol table (T.sym)
RETAIL_LINKER_SYMS = {"_fbss": 0x01041E80, "_end": 0x0104DE94, "_gp": RETAIL_GP, "_stack": 0x01F00000,
                      "_stack_size": 0x00100000, "_heap_size": 0xFFFFFFFF, "_args": 0x01041F00}
DEFAULT_ELF = os.path.join(ROOT, "build", "sce", "scus_971.02.elf")
MATCHING_ELF = os.path.join(ROOT, "matching", "build", "scus_971.02.elf")
SRC_DIRS = ["src", "sdk", "runtime", "include"]

# address map of a test (identical on both sides)
IMAGE_LO, IMAGE_HI = 0x01000000, 0x01100000     # both images must fit here
INPUT_LO, INPUT_HI = 0x01100000, 0x01800000     # spec inputs (T.alloc)
HEAP_LO, HEAP_HI = 0x01800000, 0x01E00000       # stub allocator
HOOK_LO, HOOK_HI = 0x01E00000, 0x01E01000       # fake code addresses for spec callbacks
STACK_LO, STACK_HI = E.CPU.STACK_LO, E.CPU.STACK_HI
SP0 = E.CPU.SP0
STACK_ARGS = 0x100                               # [SP0, SP0+STACK_ARGS): the caller's argument slots

DEFAULT_N = 200

# ------------------------------------------------------------------ source-derived names

TYPE_WORDS = {"void", "int", "char", "short", "long", "unsigned", "signed", "float", "double", "bool", "size_t",
              "u8", "u16", "u32", "u64", "s8", "s16", "s32", "s64", "u128", "s128", "f32", "f64"}
MARK = re.compile(r"^\s*(?://|/\*)\s*FUNC\s+0x(01[0-9A-Fa-f]{6})\b\s*(.*?)\s*(?:\*/)?\s*$")
IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
LIB_UNITS = ("libgcc_eh", "frame", "cxxrt", "libgcc_math")
# toolchain (libgcc.a) names of runtime functions whose ref/libmatch.json entry is
# ambiguous (see Names): the rebuilt image links the toolchain's own member.
TOOLCHAIN_NAMES = {
    0x01019910: "__builtin_delete",           # operator delete (opdel.o)
    0x01019AB0: "__builtin_vec_delete",       # operator delete[] (opvdel.o)
}

_EE_BIN = paths.ee_bin()
CXXFILT = os.path.join(_EE_BIN, paths.EE_PREFIX + "c++filt" + paths.EXE) if _EE_BIN else "c++filt"


def itanium_demangle(syms):
    """{Itanium symbol: demangled text} through the ps2sdk c++filt (or one on PATH);
    {} when there is none."""
    if not syms:
        return {}
    import subprocess
    for exe in (CXXFILT, "c++filt"):
        try:
            r = subprocess.run([exe], input="\n".join(syms) + "\n", capture_output=True, text=True)
        except OSError:
            continue
        out = r.stdout.splitlines()
        if r.returncode == 0 and len(out) == len(syms):
            return {s: d for s, d in zip(syms, out) if d != s}
    return {}


# the decomp's typedefs, spelled as c++filt prints them (parameter keys only)
_PARAM_TYPEDEFS = {"u8": "unsigned char", "s8": "signed char", "u16": "unsigned short", "s16": "short",
                   "u32": "unsigned int", "s32": "int", "u64": "unsigned long long", "s64": "long long",
                   "size_t": "unsigned int", "uint": "unsigned int", "f32": "float", "f64": "double"}


def _param_key(p):
    words = []
    for w in re.findall(r"[A-Za-z_][\w:]*", p):
        words.extend(_PARAM_TYPEDEFS.get(w, w).split())
    return tuple(sorted(words)) + ("*" * p.count("*"), "&" * p.count("&"))


def split_demangled(text):
    """'A::B::f(int, char const*) const' -> (['A', 'B', 'f'], parameter key), the key
    being order-insensitive within each parameter (so 'const void *' and 'void
    const*' agree).  Clones ('[clone .constprop.0]') give (None, None)."""
    if "[clone" in text:
        return None, None
    t = text.replace("(anonymous namespace)", "{anon}").replace("operator()", "operator@@")
    i = t.find("(")
    name, params = (t, None) if i < 0 else (t[:i], t[i + 1:])
    if params is not None:
        depth, j = 1, 0
        while j < len(params) and depth:
            depth += {"(": 1, ")": -1}.get(params[j], 0)
            j += 1
        params = params[:j - 1]
        ps, depth, cur = [], 0, ""
        for ch in params:
            if ch == "," and depth == 0:
                ps.append(cur)
                cur = ""
                continue
            depth += {"<": 1, "(": 1, ">": -1, ")": -1}.get(ch, 0)
            cur += ch
        ps.append(cur)
        ps = [p.strip() for p in ps if p.strip()]
        params = () if ps in ([], ["void"]) else tuple(_param_key(p) for p in ps)
    name = re.sub(r"<[^<>]*>", "", name.replace("operator@@", "operator()")).replace(" ", "")
    return [p for p in name.split("::") if p], params


def norm_parts(ps):
    """qualified-name parts as a lookup key: lower case, no spaces, one spelling of
    the anonymous namespace (v2 '_GLOBAL_$N$...', Itanium '(anonymous namespace)')."""
    if not ps:
        return ()
    out = []
    for p in ps:
        p = p.replace(" ", "").lower()
        if p.startswith("_global_$n$") or p.startswith("_global__n") or p in ("(anonymousnamespace)", "{anon}"):
            p = "{anon}"
        out.append(p)
    return tuple(out)


def _v2_template_args(sym):
    """the integer template arguments of a GNU v2 name as c++filt prints them
    for Itanium: '_$_Q211ImageLoadert12MCardSource_2Ui1Ui0' -> ['1u', '0u']; None
    without a template or with other kinds of arguments."""
    for m in re.finditer(r"t(\d+)", sym):
        n, j = int(m.group(1)), m.end()
        nargs = re.match(r"(\d)", sym[j + n:])
        if not nargs:
            continue
        k, out = j + n + 1, []
        for _ in range(int(nargs.group(1))):
            a = re.match(r"(U?)i(\d+)", sym[k:])
            if not a:
                break
            out.append(a.group(2) + ("u" if a.group(1) else ""))
            k += a.end()
        else:
            return out
    return None


# the same operators under Itanium mangling (the GCC 14 behaviour build)
ITANIUM_NAMES = {
    0x01019988: "_Znwj",                      # operator new(unsigned int)
    0x01019910: "_ZdlPv",                     # operator delete(void *)
    0x01019B28: "_Znaj",                      # operator new[](unsigned int)
    0x01019AB0: "_ZdaPv",                     # operator delete[](void *)
}


def _src_files():
    for d in SRC_DIRS:
        for dp, _, fns in os.walk(os.path.join(ROOT, d)):
            for fn in sorted(fns):
                if fn.endswith((".c", ".cpp", ".cc", ".S", ".h")):
                    yield os.path.join(dp, fn)


class Names:
    """Everything known about retail function and data names."""

    def __init__(self):
        self.funcs = {f["start"]: f for f in json.load(open(os.path.join(ROOT, "ref", "funcs.json")))}
        self.markers = {}                     # addr -> (marker text, rel path)
        self.marker_at = {}                   # addr -> (rel path, 0-based line of the marker)
        self.labels = {}                      # addr -> name of an entry point inside a merged function
        self.data = []                        # (C name, retail addr, rel path)
        decl = re.compile(r"^\s*(?:static\s+|extern\s+\"C\"\s+|const\s+|volatile\s+|unsigned\s+|signed\s+|struct\s+|"
                          r"union\s+|alignas\s*\([^)]*\)\s*)*[\w:<>\s\*]+?\b(\w+)\s*(\[[^\]]*\])*\s*"
                          r"(?:__attribute__\s*\(\(.*?\)\))?\s*(?:=|;)")
        # `Type name("lit");` / `Type name(ident);` / `Type name(123);` (not a prototype)
        dinit = re.compile(r"^\s*(?:static\s+)?(?:const\s+)?[\w:<>]+\s+(\w+)\s*\(\s*(\"(?:[^\"\\]|\\.)*\"|"
                           r"[A-Za-z_]\w*|-?(?:0x)?[0-9A-Fa-f]+)\s*\)\s*;")
        at = re.compile(r"@\s*(0x01[0-9A-Fa-f]{6})")
        for path in _src_files():
            rel = os.path.relpath(path, ROOT).replace("\\", "/")
            lines = open(path, encoding="utf-8", errors="replace").read().split("\n")
            for i, L in enumerate(lines):
                m = MARK.match(L)
                if m:
                    self.markers.setdefault(int(m.group(1), 16), (m.group(2), rel))
                    self.marker_at.setdefault(int(m.group(1), 16), (rel, i))
                    continue
                if rel.endswith(".S"):
                    # an entry point that funcs.json merged into the function before it:
                    # "//   0x0101BC40  -0x46  iPollSema" followed by "ASTUB(iPollSema, -0x46)"
                    m = re.match(r"^\s*//\s+0x(01[0-9A-Fa-f]{6})\s+\S+\s+(\w+)", L)
                    if m and i + 1 < len(lines) and re.match(r"^\s*ASTUB\(%s\b" % m.group(2), lines[i + 1]):
                        self.labels[int(m.group(1), 16)] = m.group(2)
                        continue
                    # a data label with its retail address: "_args:  .space 0x144  /* @0x01041F00 */"
                    m = re.match(r"^(\w+):\s*\.(?:space|skip|zero|fill|word|half|byte|dword|quad)\b.*?"
                                 r"@\s*0x(01[0-9A-Fa-f]{6})", L)
                    if m and 0x01026C00 <= int(m.group(2), 16) < 0x0104DEB0:
                        self.data.append((m.group(1), int(m.group(2), 16), rel))
                    continue
                if rel.endswith(".h"):
                    continue
                m = at.search(L)
                if not m:
                    # also `decl;  /* 0x0104D640 .bss */`: a declaration whose
                    # trailing comment starts with its address
                    m = re.search(r";\s*(?:/\*|//)\s*(0x01[0-9A-Fa-f]{6})\b", L)
                    if not m or not re.match(r"[A-Za-z_]", L):
                        continue                            # file-scope declarations only (column 0)
                    L = L[:m.start()] + ";"
                ra = int(m.group(1), 16)
                if not (0x01026C00 <= ra < 0x0104DEB0):
                    continue
                if re.search(r"\+0x[0-9A-Fa-f]+[^@]*@0x", L):
                    continue                                # a struct member's offset + address note
                if L.lstrip().startswith("*"):
                    # a continuation line of a block comment: only the block's
                    # first annotation describes the declaration after it
                    k = i - 1
                    while k >= 0 and lines[k].lstrip().startswith("*"):
                        k -= 1
                    if k >= 0 and any(at.search(x) for x in lines[k:i]):
                        continue
                # the declaration is on this line, or within the next few code lines
                # after the comment block the annotation is in
                code = 0
                for j in range(i, min(i + 60, len(lines))):
                    cand = lines[j].split("/*")[0].split("//")[0] if j > i else L.split("@")[0]
                    if j == i and re.match(r"^\s*(?:static\s+|const\s+|volatile\s+)*[\w\s\*]+\b\w+\s*(?:\[[^\]]*\]\s*)*$",
                                           L.split("/*")[0].split("//")[0]) and \
                            not re.match(r"^\s*(?:return|else|case|goto)\b", L):
                        # `static u128 s_buf[N]   /* @0x... */` with its attributes and
                        # `;` on the next line(s): the whole declaration
                        cand = L.split("/*")[0].split("//")[0]
                        for k2 in range(i + 1, min(i + 4, len(lines))):
                            cand += " " + lines[k2].split("/*")[0].split("//")[0].strip()
                            if ";" in cand or "=" in cand:
                                break
                    if j > i and at.search(lines[j]) and not lines[j].lstrip().startswith("*"):
                        break                               # a later annotation owns what follows
                    if j > i and lines[j].lstrip().startswith(("/*", "*", "//")):
                        continue
                    code += 1
                    if code > 5:
                        break
                    if re.match(r"^\s*extern\s+(?!\"C\")", cand) and "=" not in cand:
                        continue                            # a declaration, not the definition
                    d = decl.match(cand)
                    plain = re.sub(r"(?:__attribute__|alignas)\s*\((?:[^()]|\((?:[^()]|\([^()]*\))*\))*\)", "",
                                   cand.split("=")[0])
                    if d and "(" not in plain:
                        self.data.append((d.group(1), ra, rel))
                        break
                    d = dinit.match(cand)                   # C++ object with constructor arguments
                    if d and d.group(2) not in TYPE_WORDS:
                        self.data.append((d.group(1), ra, rel))
                        break
                    d = re.match(r"^\s*(?:static\s+|const\s+)*[\w\s\*]+\(\s*\*\s*(\w+)\s*\)\s*\([^;=]*\)\s*[;=]", cand)
                    if d:                                   # a function pointer: `void (*name)(int);`
                        self.data.append((d.group(1), ra, rel))
                        break
                    d = re.match(r"^\s*\}\s*(\w+)\s*(?:\[[^\]]*\]\s*)*(?:__attribute__\s*\(\(.*\)\))?\s*[;=]", cand)
                    if d:                                   # `} name;` closing a struct / union definition
                        self.data.append((d.group(1), ra, rel))
                        break
                    if j > i and cand.strip():
                        break
        # link/*.S: binary blobs (.incbin) described in the file's header comment as
        # " * g_name   ... .data 0x01034730 ..." (the first retail address of the entry)
        ldir = os.path.join(ROOT, "link")
        if os.path.isdir(ldir):
            for fn in sorted(os.listdir(ldir)):
                if not fn.endswith(".S"):
                    continue
                text = open(os.path.join(ldir, fn), encoding="utf-8", errors="replace").read()
                head = text.split("*/", 1)[0]
                for m in re.finditer(r"^\s*\*\s+(g_\w+)\s+(.*?)(?=^\s*\*\s+g_\w+\s|\Z)", head, re.M | re.S):
                    a = re.search(r"0x(01[0-9A-Fa-f]{6})", m.group(2))
                    if a and 0x01026C00 <= int(a.group(1), 16) < 0x0104DEB0:
                        self.data.append((m.group(1), int(a.group(1), 16), "link/" + fn))
        self.symaddrs = {}                    # addr -> name (matching/symbol_addrs.txt)
        p = os.path.join(ROOT, "matching", "symbol_addrs.txt")
        if os.path.exists(p):
            for L in open(p, encoding="utf-8"):
                m = re.match(r"^(\S+)\s*=\s*0x([0-9A-Fa-f]+);", L)
                if m:
                    self.symaddrs[int(m.group(2), 16)] = m.group(1)
        self.symmap = collections.defaultdict(list)     # addr -> [mangled names]
        p = os.path.join(ROOT, "build", "score", "symmap.json")
        if os.path.exists(p):
            for k, v in json.load(open(p)).items():
                self.symmap[v].append(k)
        self.libmatch = {}
        units = json.load(open(os.path.join(ROOT, "ref", "units.json")))
        self.unit_ranges = [(u, int(lo, 16), int(hi, 16)) for u, s in units.items() for lo, hi in s["ranges"]]
        p = os.path.join(ROOT, "ref", "libmatch.json")
        if os.path.exists(p):
            # tools/match_lib.py matches by bytes, so byte-identical retail functions
            # all get the first library member's name (e.g. operator delete[]
            # 0x01019AB0 is listed as opdel.o's __builtin_delete): such a name is
            # ambiguous and is not used
            by_sym = collections.defaultdict(list)
            for k, (lib, member, sym, n) in json.load(open(p)).items():
                a = int(k, 16)
                if self.unit_of(a) in LIB_UNITS:
                    by_sym[sym].append(a)
            for sym, addrs in by_sym.items():
                if len(addrs) == 1:
                    self.libmatch[addrs[0]] = sym
        self.libmatch.update(TOOLCHAIN_NAMES)

    def unit_of(self, a):
        for u, lo, hi in self.unit_ranges:
            if lo <= a < hi:
                return u
        return "?"

    def display(self, a):
        m = self.markers.get(a)
        if m:
            return m[0].split(" (")[0][:60] if m[0] else "F_%08X" % a
        if a in self.labels:
            return self.labels[a]
        return self.symaddrs.get(a, "F_%08X" % a)

    def candidates(self, a):
        """names the rebuilt ELF may use for the function at retail address a."""
        out = []
        for n in [self.symaddrs.get(a)] + sorted(self.symmap.get(a, [])) + [self.libmatch.get(a), self.labels.get(a), ITANIUM_NAMES.get(a)]:
            if n and n not in out:
                out.append(n)
        m = self.markers.get(a)
        if m:
            # plain identifiers of the marker text, but not a part of a qualified
            # name (`ImageLoader::HostSource::open` is not the C function `open`)
            # nor a word inside parentheses (a parameter list or a note)
            text = m[0]
            for mt in re.finditer(r"[A-Za-z_$][\w$.]*", text):
                tok = mt.group(0)
                before = text[:mt.start()]
                if before.rstrip().endswith("::") or text[mt.end():].lstrip().startswith("::"):
                    continue
                if before.count("(") > before.count(")"):
                    continue
                if (IDENT.match(tok) or "__" in tok) and tok not in out and not tok.startswith("F_"):
                    out.append(tok)
        return out

    def lookup(self, key):
        """retail address for a CLI / spec key: 0x... address, marker text or symbol name."""
        if isinstance(key, int):
            return key
        k = key.strip()
        if re.match(r"^(0x)?01[0-9A-Fa-f]{6}$", k):
            return int(k, 16)
        hits = [a for a, (t, _) in self.markers.items() if t.split(" (")[0] == k]
        if not hits:
            hits = [a for a, n in self.symaddrs.items() if n == k]
        if not hits:
            hits = [a for a, ns in self.symmap.items() if k in ns]
        if len(hits) == 1:
            return hits[0]
        if len(hits) > 1:
            raise KeyError("%r is ambiguous: %s" % (key, ", ".join("0x%08X" % h for h in sorted(hits))))
        raise KeyError("no function %r" % key)


# ------------------------------------------------------------------ prototypes (n32 calls)

# The class of a parameter or return value, for an n32 caller (the GCC 14 build):
#   'i'  32-bit integer, enum or pointer: the caller sign-extends it to 64 bits
#   'u8' 's8' 'u16' 's16'  promoted by the caller (zero- / sign-extended)
#   'l'  64-bit integer, 'd' double (soft-float on the EE: a 64-bit GPR)
#   'f'  float: $f12 + its argument position
#   'q'  a 16-byte aligned 16-byte value (u128, GsVec4): an even slot pair
#   'v'  void (return only)
_TYPE_WORDS_SKIP = {"const", "volatile", "struct", "union", "class", "enum", "register", "static", "inline",
                    "extern", "typename", "restrict", "__restrict", "__inline", "__inline__"}
_FIXED_CLASSES = {"float": "f", "f32": "f", "SFtype": "f", "double": "d", "f64": "d", "DFtype": "d",
                  "u128": "q", "s128": "q", "u128_a": "q", "__int128": "q", "GsVec4": "q", "GsVec4Arg": "q",
                  "bool": "u8", "_Bool": "u8", "void": "v"}


class Proto:
    """A function as an n32 caller sees it.  params: the class of every
    argument in order, hidden ones included (a returned class object's
    address, then `this`); ret: the return class."""

    def __init__(self, name, ret, params, variadic, kind, linkage, where):
        self.name, self.ret, self.params, self.variadic = name, ret, params, variadic
        self.kind = kind                      # 'ctor' / 'dtor' / None
        self.linkage = linkage                # 'static' / 'inline' / 'extern'
        self.where = where                    # rel path:line of the declaration used

    def __repr__(self):
        return "%s %s(%s%s) [%s]" % (self.ret, self.name, "?" if self.params is None else ", ".join(self.params),
                                      ", ..." if self.variadic else "", self.linkage)


def _gcc14_lines(lines):
    """the lines GCC 14 compiles: those inside an `#if(def) BOOT_OLD_GCC` branch
    (or LIBC_RETAIL_ASM) become blank."""
    out, stack = [], []
    for L in lines:
        m = re.match(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$", L)
        if m:
            d, cond = m.group(1), m.group(2)
            if d in ("if", "ifdef", "ifndef"):
                old = re.search(r"\b(BOOT_OLD_GCC|LIBC_RETAIL_ASM)\b", cond)
                neg = d == "ifndef" or re.search(r"!\s*defined\s*\(?\s*(BOOT_OLD_GCC|LIBC_RETAIL_ASM)", cond)
                stack.append(None if not old else ("new" if neg else "old"))
            elif d in ("else", "elif") and stack and stack[-1] is not None:
                stack[-1] = "new" if stack[-1] == "old" else "old"
            elif d == "endif" and stack:
                stack.pop()
            out.append("")
            continue
        out.append("" if "old" in stack else L)
    return out


def _strip_comments(lines):
    """the lines without // and /* */ comments (line numbers kept)."""
    out, inblock = [], False
    for L in lines:
        s, i, res = L, 0, ""
        while i < len(s):
            if inblock:
                j = s.find("*/", i)
                if j < 0:
                    i = len(s)
                    break
                inblock, i = False, j + 2
                continue
            if s.startswith("/*", i):
                inblock, i = True, i + 2
                continue
            if s.startswith("//", i):
                break
            if s[i] == '"':                                  # a string literal: copy it whole
                j = i + 1
                while j < len(s) and s[j] != '"':
                    j += 2 if s[j] == "\\" else 1
                res += s[i:j + 1]
                i = j + 1
                continue
            res += s[i]
            i += 1
        out.append(res)
    return out


class Protos:
    """Function prototypes from the decomp's sources, the GCC 14 view (the n32
    calling convention needs every argument's type: float slots, promotions,
    16-byte values).  For the function at a retail address: its definition after
    the FUNC marker, else its declaration in the sources by name."""

    def __init__(self, names):
        self.names = names
        self._files = {}
        self._cache = {}
        self.typedefs, self.classes, self.static_members, self.decls = {}, set(), set(), collections.defaultdict(list)
        for path in _src_files():
            rel = os.path.relpath(path, ROOT).replace("\\", "/")
            if rel.endswith(".S"):
                continue
            lines = self._lines(rel)
            for i, L in enumerate(lines):
                m = re.match(r"^\s*typedef\s+([^;{}()]+?)\s+(\w+)\s*(?:BOOT_MAY_ALIAS\s*)?;", L)
                if m:
                    self.typedefs.setdefault(m.group(2), m.group(1))
                m = re.match(r"^\s*(?:typedef\s+)?(?:class|struct|union)\s+(\w+)\s*(?:[:{]|$)", L)
                if m:
                    self.classes.add(m.group(1))
                m = re.match(r"^\s+static\s+[^=;(]*?\b(\w+)\s*\(", L)
                if m:
                    self.static_members.add(m.group(1))
                # declarations and definitions by plain name: "ret name(" at a line start
                m = re.match(r"^\s*(?:EXTERN_C\s+|extern\s+(?:\"C\"\s+)?|static\s+|inline\s+|virtual\s+)*"
                             r"[A-Za-z_][\w\s\*&:<>,]*?[\s\*&]((?:\w+::)*~?\w+)\s*\(", L)
                if m and not re.match(r"^\s*(?:return|else|if|while|for|switch|case|do|goto|sizeof)\b", L):
                    self.decls[m.group(1).split("::")[-1]].append((rel, i))
                    continue
                # constructors / destructors: declared in their class body, or
                # defined as X::X(...) / X::~X(...)
                m = re.match(r"^\s*(?:explicit\s+|virtual\s+|inline\s+)*((?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*)\s*\(", L)
                if m:
                    qn = m.group(1).split("::")
                    if len(qn) >= 2:
                        if qn[-1].lstrip("~") == qn[-2]:
                            self.decls[qn[-1]].append((rel, i))
                    else:
                        sc = self._scope(rel, i)
                        if sc and sc[-1][0] == "class" and qn[-1].lstrip("~") == sc[-1][1]:
                            self.decls[qn[-1]].append((rel, i))
            # typedef struct [tag] { ... } Name;  (one nesting level)
            for m in re.finditer(r"\btypedef\s+(?:struct|union)\s*\w*\s*\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\}\s*"
                                 r"(?:__attribute__\s*\(\((?:[^()]|\([^()]*\))*\)\)\s*)?(\w+)\s*;", "\n".join(lines)):
                self.classes.add(m.group(1))

    def _lines(self, rel):
        if rel not in self._files:
            raw = open(os.path.join(ROOT, rel), encoding="utf-8", errors="replace").read().split("\n")
            self._files[rel] = _strip_comments(_gcc14_lines(raw))
        return self._files[rel]

    def _scope(self, rel, j):
        """the enclosing braces at the start of line j: [(kind, name)], kind
        'class' (class / struct / union body), 'ns', 'extern' or 'block'."""
        key = ("scopes", rel)
        if key not in self._files:
            lines = self._lines(rel)
            per_line, stack, since = [], [], ""
            for L in lines:
                per_line.append(list(stack))
                for c in L:
                    if c == "{":
                        m = re.search(r"\b(class|struct|union)\s+(\w+)[^;()]*$", since)
                        if m:
                            stack.append(("class", m.group(2)))
                        elif re.search(r"\bnamespace\b[^;]*$", since):
                            stack.append(("ns", ""))
                        elif re.search(r"\bextern\s*\"C\"\s*$", since):
                            stack.append(("extern", ""))
                        else:
                            stack.append(("block", ""))
                        since = ""
                    elif c == "}":
                        if stack:
                            stack.pop()
                        since = ""
                    elif c == ";":
                        since = ""
                    else:
                        since += c
                since += " "
            self._files[key] = per_line
        per_line = self._files[key]
        return per_line[j] if j < len(per_line) else []

    # -------------------------------------------------------------- types
    def type_class(self, text, depth=0):
        t = re.sub(r"__attribute__\s*\(\((?:[^()]|\([^()]*\))*\)\)", "", text)
        if "*" in t or "&" in t or "[" in t or "(" in t:
            return "i"
        words = [w for w in re.findall(r"[A-Za-z_]\w*", t) if w not in _TYPE_WORDS_SKIP]
        if not words:
            return "i"
        for w in words:
            if w in _FIXED_CLASSES:
                return _FIXED_CLASSES[w]
        ws = set(words)
        if words.count("long") >= 2 or ws & {"u64", "s64", "uint64_t", "int64_t", "u_long64", "UDItype", "DItype",
                                               "u64_a", "UDWtype", "DWtype"}:
            return "l"
        if "char" in ws:
            return "u8" if "unsigned" in ws else "s8"
        if "short" in ws:
            return "u16" if "unsigned" in ws else "s16"
        if ws & {"u8", "uint8_t", "u_char", "UQItype"}:
            return "u8"
        if ws & {"s8", "int8_t"}:
            return "s8"
        if ws & {"u16", "uint16_t", "u_short", "UHItype"}:
            return "u16"
        if ws & {"s16", "int16_t"}:
            return "s16"
        if depth < 4:
            for w in words:
                if w in self.typedefs and self.typedefs[w] != w:
                    return self.type_class(self.typedefs[w], depth + 1)
        for w in words:
            if w in self.classes:
                return "agg"                                 # a struct / class by value
        return "i"

    _BASIC = {"int", "char", "short", "long", "unsigned", "signed", "float", "double", "void", "bool"}

    def _param_class(self, p):
        p = p.split("=")[0].strip()                          # default argument
        if not re.search(r"[*&(\[]", p):
            words = [w for w in re.findall(r"[A-Za-z_]\w*", re.sub(r"__attribute__\s*\(\(.*?\)\)", "", p))
                     if w not in _TYPE_WORDS_SKIP]
            if len(words) >= 2 and words[-1] not in self._BASIC:
                p = p[:p.rfind(words[-1])]                   # drop the parameter's name
        return self.type_class(p)

    # -------------------------------------------------------------- definitions
    def _header_at(self, rel, i, want):
        """(text before '(', parameter text, text after ')') of a declaration of
        one of the names `want` starting at line i, or None."""
        lines = self._lines(rel)
        L = lines[i]
        for m in re.finditer(r"((?:\(anonymous namespace\)::)?(?:[A-Za-z_]\w*(?:<[^<>()]*>)?::)*~?[A-Za-z_]\w*)\s*\(", L):
            nm = m.group(1)
            if nm.split("::")[-1] not in want:
                continue
            pre = L[:m.start()]
            if re.search(r"[=;(){}]|\breturn\b|\bnew\b|\.|->", pre):
                continue                                     # an expression, not a declaration
            if not re.sub(r"\b(?:static|inline|virtual|extern|EXTERN_C|explicit)\b|\"C\"", "", pre).strip() and i:
                # the return type on the line before:  "DWtype\n__divdi3(DWtype u, DWtype v)"
                prev = lines[i - 1].strip()
                if prev and re.match(r"^[\w\s\*&:<>,]+$", prev) and not re.match(r"^(?:public|private|protected)\s*:",
                                                                                prev):
                    pre = prev + " " + pre
            text, depth, j, k = "", 1, i, m.end()
            while j < len(lines) and j < i + 12:
                s = lines[j] if j > i else L
                while k < len(s):
                    c = s[k]
                    if c == "(":
                        depth += 1
                    elif c == ")":
                        depth -= 1
                        if depth == 0:
                            rest = s[k + 1:]
                            nxt = " ".join(x.strip() for x in lines[j + 1:j + 3])
                            return pre, nm, text, (rest + " " + nxt).strip()
                    text += c
                    k += 1
                text += " "
                j, k = j + 1, 0
            return None
        return None

    def proto(self, a):
        if a in self._cache:
            return self._cache[a]
        p = None
        try:
            p = self._proto(a)
        except Exception:
            p = None
        self._cache[a] = p
        return p

    def _qualifier(self, a):
        """the class part of the marker's qualified name (None for none)."""
        m = self.names.markers.get(a)
        if not m:
            return None
        t = m[0].strip()
        first = t.split()[0] if t else ""
        v2sym = "::" not in first and "(" not in first and (first.startswith(("__", "_$_")) or "__" in first[1:])
        if v2sym and " " in t and "::" in t.split(" ", 1)[1]:
            ps = v2mangle.marker_parts(t.split(" ", 1)[1])      # "mangled Class::name ..."
        elif v2sym:
            ps = v2mangle.parts(first)
        else:
            ps = v2mangle.marker_parts(t.replace("(anonymous namespace)", "{anon}"))
        return ps[-2] if len(ps) >= 2 and ps[-2] != "{anon}" else None

    def _want(self, a):
        want = set()
        m = self.names.markers.get(a)
        if m:
            t = m[0].strip()
            if t.startswith("_$_") or ("::" not in t and "(" not in t and "__" in t[1:]):
                ps = v2mangle.parts(t.split()[0])
            else:
                ps = v2mangle.marker_parts(t.replace("(anonymous namespace)", "{anon}"))
            if ps:
                want.add(ps[-1])
            if not (ps and ps[-1].startswith("~")):          # ("~X": the candidates would add X)
                for tok in self.names.candidates(a):
                    if IDENT.match(tok):
                        want.add(tok)
        for n in self.names.symmap.get(a, []):
            ps = v2mangle.parts(n)
            if ps:
                want.add(ps[-1])
        if a in self.names.symaddrs:
            want.add(self.names.symaddrs[a])
        return {w for w in want if w and not w.startswith("F_") and not w.startswith("func_")}

    def _class_ok(self, q, rel, j, nm):
        """is the declaration of nm at rel:j a member of class q (the marker's
        qualifier; None: no constraint)?"""
        if q is None:
            return True
        qual = nm.split("::")
        if len(qual) >= 2:
            return qual[-2] == q
        sc = self._scope(rel, j)
        cls = [s[1] for s in sc if s[0] == "class"]
        return not cls or cls[-1] == q

    def _proto(self, a):
        want = self._want(a)
        if not want:
            return None
        q = self._qualifier(a)
        hit = None
        at = self.names.marker_at.get(a)
        if at and at[0].endswith((".c", ".cpp", ".cc")):
            rel, i = at
            lines = self._lines(rel)
            for j in range(i + 1, min(i + 120, len(lines))):
                if MARK.match(open_line(rel, j)):
                    break
                h = self._header_at(rel, j, want)
                if h and not h[3].startswith(";") and not h[3].startswith(",") and self._class_ok(q, rel, j, h[1]):
                    hit = (rel, j) + h
                    break
        if hit is None:
            # a declaration elsewhere (a header's prototype, an inline member)
            found = []
            for w in want:
                for rel, j in self.decls.get(w, []):
                    h = self._header_at(rel, j, want)
                    if h and self._class_ok(q, rel, j, h[1]):
                        found.append((rel, j) + h)
            if not found:
                return None
            static = any(re.search(r"\bstatic\b", f[2]) for f in found)
            inline = any(re.search(r"\binline\b", f[2]) or
                         re.sub(r"^(?:\s*(?:const|volatile|noexcept|override)\b)*", "", f[5]).lstrip().startswith(
                             ("{", ":")) and f[0].endswith(".h") for f in found)
            protos = [self._build(a, f, static, inline) for f in found]
            if len({(p.ret, tuple(p.params), p.variadic) for p in protos}) != 1:
                # overloads the marker does not tell apart: the linkage is known
                # (all inline / static or not), the arguments are not
                p = protos[0]
                return Proto(p.name, None, None, False, p.kind, p.linkage, p.where)
            return protos[0]
        static = bool(re.search(r"\bstatic\b", hit[2]))
        inline = bool(re.search(r"\binline\b", hit[2]))
        return self._build(a, hit, static, inline)

    def _build(self, a, hit, static, inline):
        rel, j, pre, nm, ptext, post = hit
        qual = nm.split("::")
        last = qual[-1]
        kind = None
        member = False
        scope = self._scope(rel, j)
        if len(qual) >= 2 and qual[-2] in self.classes:
            member = last not in self.static_members
            cls = qual[-2]
        elif scope and scope[-1][0] == "class":
            member = not re.search(r"\bstatic\b", pre)       # declared in the class body
            cls = scope[-1][1]
        if member or (len(qual) >= 2 and qual[-2] in self.classes) or (scope and scope[-1][0] == "class"):
            if last == cls:
                kind = "ctor"
            elif last == "~" + cls:
                kind = "dtor"
        ret_text = re.sub(r"\b(?:static|inline|virtual|extern|EXTERN_C|explicit|__inline__|__inline)\b|\"C\"", "",
                          pre).strip()
        params, variadic = [], False
        parts_ = []
        depth, cur = 0, ""
        for c in ptext:
            if c == "," and depth == 0:
                parts_.append(cur)
                cur = ""
                continue
            depth += {"(": 1, "<": 1, ")": -1, ">": -1}.get(c, 0)
            cur += c
        parts_.append(cur)
        parts_ = [x.strip() for x in parts_ if x.strip()]
        if parts_ in ([], ["void"]):
            parts_ = []
        for x in parts_:
            if x == "...":
                variadic = True
                continue
            params.append(self._param_class(x))
        if kind in ("ctor", "dtor") or not ret_text:
            ret = "v"
        else:
            ret = self.type_class(ret_text)
        hidden = []
        if ret == "agg":
            hidden.append("i")                               # the returned object's address
            ret = "i"
        if member:
            hidden.append("i")                               # this
        anon = "(anonymous namespace)" in (self.names.markers.get(a, ("", ""))[0])
        linkage = "static" if static or anon else ("inline" if inline else "extern")
        return Proto(nm, ret, hidden + params, variadic, kind, linkage, "%s:%d" % (rel, j + 1))


def promote(c, v):
    """an argument / return value of class c as the register holds it after a C
    caller (n32: the caller extends; MIPS III keeps 32-bit values sign-extended
    in 64-bit registers)."""
    if c in ("i", "p"):
        return E.sx32(v & 0xFFFFFFFF) & E.M64
    if c == "u8":
        return v & 0xFF
    if c == "s8":
        return E.sx8(v & 0xFF) & E.M64
    if c == "u16":
        return v & 0xFFFF
    if c == "s16":
        return E.sx16(v & 0xFFFF) & E.M64
    if c in ("l", "d"):
        return v & E.M64
    return v                                                 # 'q' (up to 128 bits), 'agg', unknown


def open_line(rel, j):
    """raw line j (0-based) of a source file (for marker detection)."""
    lines = _RAW_LINES.get(rel)
    if lines is None:
        lines = _RAW_LINES[rel] = open(os.path.join(ROOT, rel), encoding="utf-8", errors="replace").read().split("\n")
    return lines[j] if j < len(lines) else ""


_RAW_LINES = {}


# ------------------------------------------------------------------ one address space

class HarnessError(RuntimeError):
    pass


class NotEmitted(HarnessError):
    """a case uses a function the rebuilt image does not have as such, and for
    a reason that is not a defect (Harness.not_emitted): the case is skipped."""
    pass


def elf_abi(path):
    """(n32, compiler banner) of an ELF: n32 = EF_MIPS_ABI2 (0x20) in e_flags (the
    GCC 14 build; the retail and SCE images are EABI), the banner = the first
    string of its .comment section ('GCC: (GNU) 14.2.0'; '' when there is none)."""
    d = open(path, "rb").read()
    e_flags = struct.unpack_from("<I", d, 0x24)[0]
    banner = ""
    e_shoff = struct.unpack_from("<I", d, 0x20)[0]
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", d, 0x2E)
    if e_shoff and e_shnum and e_shstrndx < e_shnum:
        secs = [struct.unpack_from("<IIIIIIIIII", d, e_shoff + i * e_shentsize) for i in range(e_shnum)]
        shstr = secs[e_shstrndx][4]
        for s in secs:
            nm = d[shstr + s[0]:d.index(b"\0", shstr + s[0])]
            if nm == b".comment" and s[5]:
                banner = d[s[4]:s[4] + s[5]].split(b"\0")[0].decode("latin1", "replace").strip()
                if banner:
                    break
    return bool(e_flags & 0x20), banner


def map_text_units(map_path):
    """[(lo, hi, object file)] of the .text input sections in a GNU ld map."""
    out = []
    if not map_path or not os.path.exists(map_path):
        return out
    pend = False
    for L in open(map_path, encoding="utf-8", errors="replace"):
        m = re.match(r"^ \.text\S*\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S.*\.o)\)?\s*$", L)
        if m is None and pend:
            m = re.match(r"^\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S.*\.o)\)?\s*$", L)
        pend = bool(re.match(r"^ \.text\S*\s*$", L))
        if m and int(m.group(2), 16):
            lo = int(m.group(1), 16)
            out.append((lo, lo + int(m.group(2), 16), m.group(3).strip()))
    out.sort()
    return out


def _pattern_block(seed, n=0x10000):
    """64 KB of address-dependent filler: a word never equals the same word at
    another offset of the block, so a read from the wrong place shows."""
    ws = [((i * 0x9E3779B1) ^ seed) & 0xFFFFFFFF for i in range(n // 4)]
    return struct.pack("<%dI" % len(ws), *ws)


class Side:
    """One side of the comparison: an ELF loaded into eeemu, its symbols, hooks,
    heap, and what the current case did."""

    def __init__(self, label, path, names, gp=None, retail=False):
        self.label = label
        self.path = path
        self.names = names
        self.retail = retail
        self.mem = E.Mem(path)
        lo, hi = self.mem.image_range()
        if lo < IMAGE_LO or hi > IMAGE_HI:
            raise HarnessError("%s: image %08X..%08X is outside %08X..%08X" % (path, lo, hi, IMAGE_LO, IMAGE_HI))
        self.image_lo, self.image_hi = lo, hi
        self.syms = [] if retail else E.elf_symbols(path)
        self.by_name = collections.defaultdict(list)
        for s in self.syms:
            if s[5] != 0 and s[3] != 4:                  # defined, not STT_FILE
                self.by_name[s[0]].append(s)
        if gp is None:
            g = [s for s in self.by_name.get("_gp", [])]
            if not g:
                raise HarnessError("%s: no _gp symbol" % path)
            gp = g[0][1]
        self.cpu = E.CPU(self.mem, gp)
        self._fmap = {}                                  # retail addr -> side addr (cache)
        self.identical = False                           # set by Harness: byte-identical to retail
        self.absent_reason = None                        # set by Harness: fn(retail addr) -> why it is not emitted
        # the calling convention of this image (see MATCHING.md "Function-level
        # behaviour tests", "The GCC 14 build")
        if retail:
            self.n32, self.banner = False, ""
        else:
            self.n32, self.banner = elf_abi(path)
        self.itanium = any(s[0].startswith("_Z") and s[3] == 2 for s in self.syms)
        m = re.search(r"GCC: \([^)]*\) (\d+)\.", self.banner)
        self.gcc_major = int(m.group(1)) if m else None
        # GCC >= 5 at -O1 and up allocates registers across calls to functions of
        # the same unit whose clobbers it knows (-fipa-ra).  The GCC 14 link drops
        # .comment, so an n32 image (only GCC 14 makes one here) counts as such.
        self.ipa_ra = self.gcc_major >= 5 if self.gcc_major is not None else self.n32
        self.units = map_text_units(os.path.splitext(path)[0] + ".map") if self.ipa_ra else []
        self._unit_lo = [u[0] for u in self.units]
        # pristine state: pattern-filled inputs, heap, stack and scratchpad
        blk = _pattern_block(0xA5A55A5A)
        for lo_, hi_ in ((INPUT_LO, INPUT_HI), (HEAP_LO, HEAP_HI), (HOOK_LO, HOOK_HI), (STACK_LO, STACK_HI)):
            n = hi_ - lo_
            self.mem.m[lo_:hi_] = (blk * (n // len(blk) + 1))[:n]
        self.mem.m[E.SPR_OFF:E.SPR_OFF + E.SPR_SIZE] = _pattern_block(0x5C5C3A3A, E.SPR_SIZE)
        self.mem.commit()
        self.io = self.mem.io                            # hardware model (eeemu.IoModel)
        self.mem.set_watch(STACK_LO, SP0 + STACK_ARGS)   # note loads of never-written stack bytes
        self.base_hooks = {}
        self.unhooked = []                               # base stubs whose function was not found
        self.heap = Heap()

    def unit_of(self, addr):
        """the object file whose .text holds addr (from the link map), or None."""
        i = bisect.bisect_right(self._unit_lo, addr) - 1
        if i >= 0 and addr < self.units[i][1]:
            return self.units[i][2]
        return None

    def keeps_caller_saved(self, cpu, callee):
        """may the caller of `callee` (the code $ra returns to) hold values in
        caller-saved registers across this call?  Only on an IPA-RA image, for a
        callee of the caller's own unit (or, without a link map, any callee): a
        stub must then not clobber them (see _clobber)."""
        if not self.ipa_ra:
            return False
        if not self.units:
            return True
        ra = cpu.g[31] & 0xFFFFFFFF
        u = self.unit_of(ra)
        return u is not None and u == self.unit_of(callee)

    # -------------------------------------------------------------- symbols
    def func(self, a):
        """this side's address of the function at retail address a."""
        if self.retail:
            return a
        r = self._fmap.get(a)
        if r is None:
            try:
                r = self._resolve(a)
            except HarnessError as e:
                if not self.identical:
                    why = self.absent_reason(a) if self.absent_reason else None
                    if why:
                        raise NotEmitted("%s (0x%08X): %s" % (self.names.display(a), a, why))
                    raise
                r = a                                    # byte-identical image: same address
            self._fmap[a] = r
        return r

    def _demangled(self):
        """{qualified-name parts (lower case): [(function symbol, parameter key)]}
        of this ELF.  GNU v2 names (the SCE build) go through v2mangle (parameter
        key None); Itanium names (the GCC 14 build) through the toolchain's
        c++filt, keeping the parameter list to tell overloads apart."""
        if getattr(self, "_dm", None) is None:
            self._dm = collections.defaultdict(list)
            funcs = [s for s in self.syms if s[3] == 2 and s[5] != 0]
            itanium = itanium_demangle([s[0] for s in funcs if s[0].startswith("_Z")])
            self._dmtext = itanium
            for s in funcs:
                full = itanium.get(s[0])
                if full:
                    ps, params = split_demangled(full)
                    if ps is None:
                        continue                         # a GCC clone (.constprop / .isra)
                else:
                    ps, params = v2mangle.parts(s[0]), None
                self._dm[norm_parts(ps)].append((s, params))
        return self._dm

    def xtor(self, addr):
        """for an Itanium image: ('ctor', None) when the function at addr (this
        side's address) is a constructor, ('dtor', deleting-destructor address or
        None) for a destructor (the complete-object one, D1, is what the name
        lookup resolves); None otherwise."""
        if not self.itanium:
            return None
        if not hasattr(self, "_xtors"):
            self._xtors = {}
            for s in self.syms:
                if s[3] != 2 or not s[0].startswith("_Z"):
                    continue
                m = re.match(r"^(_ZN.*?)([CD])([0-2])E(.*)$", s[0])
                if not m:
                    continue
                if m.group(2) == "C":
                    self._xtors[s[1]] = ("ctor", None)
                elif m.group(3) in "12":
                    d0 = [x[1] for x in self.by_name.get(m.group(1) + "D0E" + m.group(4), []) if x[3] == 2]
                    self._xtors[s[1]] = ("dtor", d0[0] if d0 else None)
        return self._xtors.get(addr)

    def _resolve_by_name(self, a):
        """the function at retail address a by its marker's qualified name, or None."""
        m = self.names.markers.get(a)
        if not m:
            return None
        t = m[0].strip()
        if t.startswith("_$_") or ("::" not in t and "(" not in t and "__" in t[1:]):
            want, params = v2mangle.parts(t.split()[0]), None     # marker text is a v2 symbol
        else:
            want = v2mangle.marker_parts(t.replace("(anonymous namespace)", "{anon}"))
            params = split_demangled(t)[1] if "(" in t else None
        if want and want[-1].startswith("~"):
            params = None                    # "~X(int)": the GNU v2 in-charge flag; Itanium dtors take none
        # the C definition tools/score.py mapped to this marker (egcs-mangled in
        # build/score/symmap.json) names the function better than the marker
        # text: 0x0100B2C8 is marked "BigNum::swap (Montgomery TU-local copy)"
        # but is the static BigNum_swap_localcopy, which GCC 14 drops as unused;
        # the marker name would resolve it to BigNum::swap 0x01007128's copy
        defs = {norm_parts(v2mangle.parts(n)) for n in self.names.symmap.get(a, [])} - {()}
        if defs and norm_parts(want) not in defs:
            hits = [h for d in sorted(defs) for h in self._demangled().get(d, [])]
            addrs = {s[1] for s, _ in hits}
            return addrs.pop() if len(addrs) == 1 else None
        hits = self._demangled().get(norm_parts(want), [])
        if len({s[1] for s, _ in hits}) > 1 and params is not None:
            # overloads: keep those whose parameter list matches the marker's
            hits = [h for h in hits if h[1] == params]
        targs = _v2_template_args(t.split()[0]) if t else None
        if len({s[1] for s, _ in hits}) > 1 and targs is not None:
            # instances of a class template: the GNU v2 name's integer template
            # arguments ("t12MCardSource_2Ui1Ui0") against the demangled ones
            want_t = "<%s>" % ", ".join(targs)
            hits = [h for h in hits if want_t in self._dmtext.get(h[0][0], "")]
        if len({s[1] for s, _ in hits}) > 1:
            # Itanium complete / base object constructors and destructors at
            # different addresses: the complete-object one (C1 / D1) is the call target
            c1 = [h for h in hits if re.search(r"[CD]1E", h[0][0])]
            if len({s[1] for s, _ in c1}) == 1:
                hits = c1
        addrs = {s[1] for s, _ in hits}
        return addrs.pop() if len(addrs) == 1 else None

    def _resolve(self, a):
        found = {}
        for n in self.names.candidates(a):
            for s in self.by_name.get(n, []):
                # NOTYPE / FUNC; OBJECT too, since the EABI assembler of the SCE
                # build types a plain global code label (syscalls.S ASTUB) OBJECT
                if s[3] in (0, 1, 2):
                    found.setdefault(s[1], []).append(n)
        if not found:
            # the marker's qualified name against the demangled symbols (as
            # tools/score.py maps them); only an unambiguous hit counts
            r = self._resolve_by_name(a)
            if r is not None:
                return r
            raise HarnessError("%s: no symbol for %s (0x%08X); tried %s" % (
                self.label, self.names.display(a), a, ", ".join(self.names.candidates(a)) or "no names"))
        if len(found) > 1:
            raise HarnessError("%s: names of 0x%08X disagree: %s" % (
                self.label, a, "; ".join("%08X=%s" % (k, ",".join(v)) for k, v in sorted(found.items()))))
        return next(iter(found))

    # -------------------------------------------------------------- case state
    def begin(self):
        self.mem.reset()
        self.cpu.hooks = dict(self.base_hooks)
        self.cpu.syscall_hook = None
        self.heap.reset()
        self.io.reset()                                  # registers, COP0, VU0: reset state
        self.events = []
        self.io.events = self.events                     # hardware events go in the same ordered list
        self.err = None                                  # the EmuError of this case, if any
        self.err_msg = None
        self.unchecked = set()                           # byte addresses excluded from the memory diff
        self.next_input = INPUT_LO
        self._ncb = 0
        self._cbinfo = {}                                # callback address -> (pyfn, nargs, name)
        self._nthunk = 0                                 # fake code addresses from HOOK_HI down (T.vtable)
        self.objs = []                                   # T.obj: (this, (vptr, shift, size), placed) in order
        self.snaps = {}                                  # this -> (lo, bytes, written words): before its destructor
        self.aliases = []                                # T.vtable / T.alias: values paired by order
        self.steps0 = self.cpu.steps

    def hook(self, addr, fn):
        self.cpu.hooks[addr] = fn

    def snapshot(self, this):
        """keep the bytes of the registered object at `this` (and which of its
        words were written) as they are now, before a destructor call."""
        for t_, lay, placed in self.objs:
            if t_ == this and this not in self.snaps:
                lo = this
                n = lay[2]
                self.snaps[this] = (lo, bytes(self.mem.m[lo:lo + n]),
                                    {k for k in range(lo, lo + n, 4) if k in self.mem.orig})
                return


class Heap:
    """Deterministic bump allocator for the allocator stubs (both sides)."""

    def __init__(self):
        self.reset()

    def reset(self):
        self.next = HEAP_LO
        self.live = {}
        self.dead = set()

    def alloc(self, n, align=16):
        p = (self.next + align - 1) & -align
        size = max(int(n), 1)
        end = p + ((size + 15) & -16) + 16                # 16-byte gap between blocks
        if end > HEAP_HI:
            raise E.EmuError("stub heap exhausted (allocation of %d bytes)" % n, "heap")
        self.next = end
        self.live[p] = size
        return p

    def free(self, p):
        if p == 0:
            return 0
        if p not in self.live:
            raise E.EmuError("free of %08X: %s" % (p, "double free" if p in self.dead else "not a heap block"),
                             "heap", p)
        n = self.live.pop(p)
        self.dead.add(p)
        return n


# ------------------------------------------------------------------ the spec-facing API

CLOBBER = 0xC10BBE2D00000000


class T:
    """What a spec's case function uses.  The same case function runs once per
    side; every value it passes and every address it allocates is identical on
    both, so anything that differs afterwards is the code's doing."""

    def __init__(self, side, ctx):
        self.side = side
        self.ctx = ctx
        self.mem = side.mem
        self.retail = side.retail

    # ---- inputs
    def alloc(self, size, align=16, offset=0):
        """reserve size bytes of input memory at an address = offset (mod align)."""
        s = self.side
        p = ((s.next_input + align - 1) & -align) + offset
        s.next_input = ((p + size + 63) & -16)
        if s.next_input > INPUT_HI:
            raise HarnessError("input arena exhausted")
        return p

    def put(self, data, align=16, offset=0, pad=0):
        p = self.alloc(len(data) + pad, align, offset)
        self.mem.wr(p, bytes(data))
        return p

    def cstr(self, s, align=16, offset=0):
        if isinstance(s, str):
            s = s.encode("latin1")
        return self.put(bytes(s) + b"\0", align, offset)

    def halloc(self, n, data=None):
        """a block from the stub heap without logging a call (as if the code had
        allocated it earlier); free() by the code under test works on it."""
        p = self.side.heap.alloc(n)
        if data is not None:
            self.mem.wr(p, bytes(data))
        return p

    def write(self, a, bs):
        self.mem.wr(a, bytes(bs))

    def w8(self, a, v): self.mem.w8(a, v)
    def w16(self, a, v): self.mem.w16(a, v)
    def w32(self, a, v): self.mem.w32(a, v)
    def w64(self, a, v): self.mem.w64(a, v)
    def peek(self, a, n):
        """the n bytes at a as a tuple, with None for every stack byte nothing
        wrote in this case (an uninitialised local passed by address, e.g. a
        SemaParam field the code never set): safe to t.obs between builds."""
        k = self.mem._chk(a, n)
        data = self.mem.m[k:k + n]
        w = self.mem.watch
        mask = self.mem._wmask
        out = []
        for i in range(n):
            j = k + i
            if w is not None and w[0] <= j < w[1] and not mask[j - w[0]]:
                out.append(None)
            else:
                out.append(data[i])
        return tuple(out)

    def peek32(self, a, n):
        """n words at a (little-endian), None for a word with a never-written stack byte."""
        b = self.peek(a, 4 * n)
        return tuple(None if None in b[4 * i:4 * i + 4] else int.from_bytes(bytes(b[4 * i:4 * i + 4]), "little")
                     for i in range(n))

    def read(self, a, n): return self.mem.rd(a, n)
    def r8(self, a): return self.mem.u8(a)
    def r16(self, a): return self.mem.u16(a)
    def r32(self, a): return self.mem.u32(a)
    def r64(self, a): return self.mem.u64(a)
    def cstr_at(self, a): return self.mem.cstr(a)

    # ---- symbols
    def fn(self, retail_addr):
        """this side's address of a function (e.g. for a vtable built by the test)."""
        return self.side.func(retail_addr)

    def data(self, name):
        """this side's address of an annotated global (C name)."""
        return self.ctx.data_addr(self.side, name)

    def stub_reaches(self, caller, callee):
        """would a stub of `callee` intercept the calls `caller` makes (retail
        addresses)?  False when the rebuilt (GCC 14) image's caller has no
        direct call (jal / j) to it, i.e. GCC inlined it there.  The same answer
        on both sides; always True unless adapted."""
        return self.ctx.calls_directly(caller, callee)

    def unwritten_static(self, name):
        """the case is about to give the static `name` a value other than its
        initial one, and the program never writes that variable: GCC 14 treats
        such a static as constant and folds its initial value into some or all
        of its readers, so against the GCC 14 build (adapted runs) the case is
        skipped (NotEmitted).  Call it only when the value differs."""
        if self.ctx.adapt:
            raise NotEmitted("static %s: never written by the program, GCC folds its initial value into its "
                             "readers (the case sets another value)" % name)

    def sym(self, name):
        """this side's value of a linker-script symbol (_fbss, _end, _gp, _stack,
        _stack_size, _heap_size, _args): retail from RETAIL_LINKER_SYMS, rebuilt
        from its ELF symbol table (the byte-identical image: retail's)."""
        s = self.side
        if s.retail or s.identical:
            if name not in RETAIL_LINKER_SYMS:
                raise HarnessError("no retail value for linker symbol %s (RETAIL_LINKER_SYMS)" % name)
            return RETAIL_LINKER_SYMS[name]
        hits = sorted({x[1] for x in s.by_name.get(name, [])})
        if len(hits) != 1:
            raise HarnessError("rebuilt ELF: %s symbol %s" % ("no" if not hits else "ambiguous", name))
        return hits[0]

    # ---- calls
    def call(self, func, *args, ret="int", maxsteps=20000000, label=None, floats=()):
        """call the function at retail address `func` with integer args (EABI
        $a0..$t3, then 8-byte stack slots) and float args given as 32-bit
        patterns in `floats` ($f12..$f19).  ret: 'void', 'int'/'u64'/'ptr' (all 64
        bits of $v0), 'u8', 's8', 'u16', 's16', 'u32' (that many bits), 'f0' (the
        bits of $f0).  The result is recorded and returned.

        Against an n32 image (the GCC 14 build) the arguments are passed as a C
        caller of the function's prototype would (Harness.adapt, on both sides):
        promoted to their declared types, and on the n32 side in their argument
        positions (an int and a float argument share the slot numbering, a
        16-byte value takes an even slot pair).  A constructor's return value
        is not compared there (Itanium constructors return void); a destructor
        call (this, in-charge flags) becomes the complete-object destructor, or
        the deleting one (flags & 1)."""
        side = self.side
        a = side.func(func)
        name = label or self.ctx.names.display(func)
        side.events.append(("call", name))
        xt = side.xtor(a) if self.ctx.adapt else None
        if xt is None and self.ctx.adapt and self.ctx.rebuilt.itanium:
            # the retail side of an Itanium comparison: the kind from the rebuilt image
            try:
                xt = self.ctx.rebuilt.xtor(self.ctx.rebuilt.func(func))
            except HarnessError:
                xt = None
        if xt and xt[0] == "dtor" and args:
            side.snapshot(args[0] & 0xFFFFFFFF)          # the object's lifetime ends (see _mem_diff)
        if xt and xt[0] == "dtor" and side.itanium:
            return self._itanium_dtor(func, a, xt[1], args, maxsteps)
        regs, fregs = self._marshal(func, args, floats)
        v0, v1, f0 = side.cpu.call(a, regs, fregs, maxsteps=maxsteps)
        if xt and xt[0] == "ctor" and ret != "void":
            # GNU v2 constructors return `this`, Itanium ones nothing: compare
            # whether the retail one returned `this`, and hand `this` on
            this = (args[0] & 0xFFFFFFFF) if args else None
            got = this if side.itanium or (v0 & 0xFFFFFFFF) == this else v0
            side.events.append(("ret", name, "ctor", "this" if got == this else got))
            return got
        if ret == "void":
            val = None
        elif ret in ("int", "u64", "s64", "ptr"):
            val = v0
        elif ret == "u32":
            val = v0 & 0xFFFFFFFF
        elif ret == "u16":
            val = v0 & 0xFFFF
        elif ret == "s16":
            val = E.sx16(v0)
        elif ret == "u8":
            val = v0 & 0xFF
        elif ret == "s8":
            val = E.sx8(v0)
        elif ret == "f0":
            val = f0
        else:
            raise HarnessError("bad ret kind %r" % ret)
        if val is not None:
            self.side.events.append(("ret", label or self.ctx.names.display(func), ret, val))
        return val if val is not None else v0

    def _marshal(self, func, args, floats):
        """(register ints, register floats) for a call of func; stack arguments
        are written to the caller's argument slots ($sp + 8k)."""
        side = self.side
        proto = self.ctx.proto(func) if self.ctx.adapt else None
        if proto is None:
            if side.n32 and floats and args:
                raise HarnessError("%s: no prototype found, so the n32 positions of its float arguments are "
                                   "unknown" % self.ctx.names.display(func))
            regs = [x if x > E.M64 else x & E.M64 for x in args[:8]]     # > 64 bits: a 128-bit register arg
            for k, x in enumerate(args[8:]):              # EABI: 9th.. argument in 8-byte slots at $sp+0
                if 8 * k >= STACK_ARGS:
                    raise HarnessError("too many stack arguments")
                self.mem.w64(SP0 + 8 * k, x & E.M64)
            return regs, floats
        # the arguments as a C caller of the prototype passes them
        ints, fls, vals = list(args), list(floats), []
        for c in proto.params:
            if c == "f":
                if not fls:
                    break
                vals.append(("f", fls.pop(0)))
            else:
                if not ints:
                    break
                vals.append((c, promote(c, ints.pop(0))))
        vals += [("x", x) for x in ints] + [("f", x) for x in fls]   # variadic / unused extra arguments
        if not side.n32:
            # EABI: integers in order ($4.., a 128-bit value in one register),
            # then 8-byte stack slots; floats in order ($f12..)
            iv = [(v & E.M64, (v >> 64) & E.M64) if c == "q" else v if v > E.M64 else v & E.M64
                  for c, v in vals if c != "f"]       # a 16-byte value: one whole 128-bit register
            fv = [v & 0xFFFFFFFF for c, v in vals if c == "f"]
            for k, x in enumerate(iv[8:]):
                if 8 * k >= STACK_ARGS:
                    raise HarnessError("too many stack arguments")
                self.mem.w64(SP0 + 8 * k, (x[0] if isinstance(x, tuple) else x) & E.M64)
            if len(fv) > 8:
                raise HarnessError("more than 8 float arguments")
            return iv[:8], fv
        # n32: argument k in $4+k or $f12+k; from k = 8 on in the 8-byte slot
        # $sp + 8(k-8); a 16-byte aligned value starts at an even k
        regs, fregs, pos = [None] * 8, [None] * 8, 0
        for c, v in vals:
            if c == "q":
                pos += pos & 1
                words = [v & E.M64, (v >> 64) & E.M64]
            elif c == "f":
                words = [v & 0xFFFFFFFF]
            else:
                words = [v & E.M64]
            for w in words:
                if pos < 8:
                    if c == "f":
                        fregs[pos] = w
                    else:
                        regs[pos] = w
                else:
                    if 8 * (pos - 8) >= STACK_ARGS:
                        raise HarnessError("too many stack arguments")
                    self.mem.w64(SP0 + 8 * (pos - 8), w)
                pos += 1
        while regs and regs[-1] is None:
            regs.pop()
        while fregs and fregs[-1] is None:
            fregs.pop()
        return regs, fregs

    def _itanium_dtor(self, func, a, d0, args, maxsteps):
        """a GNU v2 destructor call (this, in-charge flags) on an Itanium image:
        the complete-object destructor (D1); with flags & 1 (delete) the deleting
        one (D0), or D1 and then operator delete(this) when the class has no D0
        (a non-virtual destructor: `delete p` is that pair in the caller)."""
        side = self.side
        this = args[0] & 0xFFFFFFFF if args else 0
        flags = args[1] if len(args) > 1 else 2
        regs = [promote("i", this)]
        if flags & 1 and d0 is not None:
            side.cpu.call(d0, regs, (), maxsteps=maxsteps)
        else:
            side.cpu.call(a, regs, (), maxsteps=maxsteps)
            if flags & 1:
                side.cpu.call(side.func(0x01019910), regs, (), maxsteps=maxsteps)   # operator delete
        return 0

    def float_args(self):
        """inside a stub: the float arguments of the stubbed call, as bit
        patterns in order (EABI: $f12, $f13 ...; n32: $f12 + their argument
        position, from the stubbed function's prototype)."""
        st = getattr(self, "_stub_floats", None)
        if st is not None:
            return list(st)
        return list(self.side.cpu.f[12:20])

    def check(self, cond, msg):
        """an oracle check of the retail result (retail is known-good, so a
        failure means the harness or the emulator is wrong)."""
        if self.retail and not cond:
            raise HarnessError("oracle check failed on retail: %s" % msg)

    def obs(self, label, value, kind="raw"):
        """record anything the spec computed from memory (compared in order)."""
        self.side.events.append(("obs", label, kind, value))

    def unchecked(self, addr, n):
        """bytes [addr, addr+n) hold an INDETERMINATE value in the original - e.g.
        copied from an uninitialised stack slot, whose content depends on the
        frame layout - so the memory comparison skips them (the union over both
        sides).  Use it only for that, and say why in the spec."""
        self.side.unchecked.update(range(addr, addr + n))

    def written(self, lo, hi):
        """word addresses in [lo, hi) written so far in this case (by anyone)."""
        return sorted(E.Mem.index_addr(k) for k in self.mem.orig if lo <= E.Mem.index_addr(k) < hi)

    # ---- hardware (eeemu.IoModel: MMIO registers, COP0, VU0)
    def reg(self, key, read=None, write=None):
        """model a hardware register for this case.  key: its address (any mirror:
        0x1000xxxx, 0xB000xxxx, 0x12001000 ...), a COP0 register name ('Count',
        'Status', 'PCR0', 'TagLo' ...) or a VU0 control register ('vi29').
        read: an int (every read), a list (successive reads, the last repeats) or
        fn(t, n, latched) -> int (n = reads so far, latched = last value stored);
        None keeps the default (latched / idle).  write: fn(t, value, width), called
        after each store.  Reads and stores are events either way."""
        key = E.Mem.phys(key) if isinstance(key, int) else key
        t = self
        rd = (lambda n, latched: read(t, n, latched)) if callable(read) else read
        wr = (lambda v, w: write(t, v, w)) if write is not None else None
        self.side.io.model(key, rd, wr)

    def cop0(self, name, value=None):
        """COP0 register `name` of this side (set it when value is given; no event)."""
        c = self.side.io.cop0
        if value is not None:
            c[name] = value & 0xFFFFFFFF
        return c.get(name, 0)

    def vf(self, i, value=None):
        """VU0 vf register i as [x, y, z, w] float bit patterns (set when given)."""
        io = self.side.io
        if value is not None and i:
            io.vf[i] = [x & 0xFFFFFFFF for x in value]
        return list(io.vf[i])

    def vi(self, i, value=None):
        """VU0 vi register i (integer or control; set when given, no event)."""
        io = self.side.io
        if value is not None and i:
            io.vi[i] = value
        return io.vi[i]

    def cache_tags(self, fn):
        """fn(t, op, addr) -> the TagLo a CACHE index-load-tag (op 0x00 / 0x10) loads."""
        t = self
        self.side.io.cache_tag = lambda op, addr: fn(t, op, addr)

    def hw_events(self):
        """the hardware events of this case so far: [(key, value, count)]."""
        return [e[1:] for e in self.side.events if e[0] == "hw"]

    def stub(self, func, pyfn, nargs=4, name=None, log=None):
        """replace the function at retail address `func` (resolved per side) by
        pyfn(t, args) -> return value; each call is logged and compared with its
        first `log` arguments (default nargs: pass a smaller `log` for a variadic
        callee, whose unused registers hold leftovers).  args are the low 32
        bits of the integer arguments in order; t.float_args() gives the float
        ones (both decoded per side's ABI from the function's prototype)."""
        proto = self.ctx.proto(func) if self.ctx.adapt else None
        a = self.side.func(func)
        name = name or self.ctx.names.display(func)
        xt = self.side.xtor(a) if self.ctx.adapt else None
        if xt and xt[0] == "dtor":
            # a GNU v2 destructor (this, in-charge flags) on an Itanium image:
            # the complete-object destructor stands for flags 2, the deleting one
            # (`delete p`) for flags 3 - the stub sees the flags a GNU v2 caller passes
            self._hook(a, pyfn, nargs, name, log, None, fix=lambda args: [args[0], 2] + list(args[2:]))
            if xt[1] is not None:
                self._hook(xt[1], pyfn, nargs, name, log, None, fix=lambda args: [args[0], 3] + list(args[2:]))
            return
        self._hook(a, pyfn, nargs, name, log, proto)

    def syscalls(self, pyfn, nargs=4):
        """emulate SYSCALL: pyfn(t, num, args) -> $v0, num = the kernel call number
        ($v1, signed), args = the low 32 bits of $a0..$t3.  Each call is logged and
        compared like a stub call ("syscall N") with its first `nargs` arguments;
        nargs may be fn(t, num, pc) -> count (pc = the SYSCALL's address on this
        side), so that registers a call does not take are not compared."""
        t = self

        def h(cpu, code):
            num = E.sx32(cpu.g[3])
            allargs = tuple(cpu.g[4 + i] & 0xFFFFFFFF for i in range(8))
            n = nargs(t, num, getattr(cpu, "syscall_pc", 0)) if callable(nargs) else nargs
            args = allargs[:n]
            r = pyfn(t, num, allargs)
            t.side.events.append(("stub", "syscall %d" % num, args, r))
            # the EE kernel restores the caller's context: only $v0 changes (inline
            # `syscall`s rely on it, e.g. $a0 across _iGetThreadId in iWakeupThread)
            cpu.g[2] = (r or 0) & E.M64
        self.side.cpu.syscall_hook = h

    def interrupt(self, func, *args, label=None, maxsteps=2000000, raw=False):
        """run the function at retail address `func` NOW, from inside a stub,
        callback or syscall hook, as an interrupt handler would run: on a stack
        below the interrupted code's, with every register of the interrupted
        code saved and restored.  raw=True: `func` is already this side's
        address (a handler pointer the code under test registered).  Logged as
        a call; returns its $v0."""
        cpu = self.side.cpu
        g, gh, f = list(cpu.g), list(cpu.gh), list(cpu.f)
        rest = (cpu.hi, cpu.lo, cpu.hi1, cpu.lo1, cpu.sa, cpu.acc, cpu.cond)
        sp = ((cpu.g[29] & 0xFFFFFFFF) - 0x400) & ~0xF
        a = func if raw else self.side.func(func)
        if label is None:
            label = "interrupt: %s" % ("handler" if raw else self.ctx.names.display(func))
        self.side.events.append(("call", label))
        try:
            v0, _, _ = cpu.call(a, [x & E.M64 for x in args], maxsteps=maxsteps, sp=sp)
        finally:
            cpu.g[:] = g                                 # in place: the decoded closures hold these lists
            cpu.gh[:] = gh
            cpu.f[:] = f
            cpu.hi, cpu.lo, cpu.hi1, cpu.lo1, cpu.sa, cpu.acc, cpu.cond = rest
        return v0

    def native(self, func):
        """run the function at retail address `func` as code in this case even
        though the harness stubs it by default (scePrintf, the allocator)."""
        a = self.side.func(func)
        self.side.cpu.hooks.pop(a, None)
        reach = getattr(self.ctx, "reach", None)
        if self.side.retail and reach is not None and a in reach:
            self.side.cpu.hooks[a] = lambda cpu: self.ctx.reached.add(a) and False

    def callback(self, pyfn, nargs=4, name="callback"):
        """a fake code address (same on both sides) that runs pyfn(t, args)."""
        s = self.side
        k = getattr(s, "_ncb", 0)
        s._ncb = k + 1
        a = HOOK_LO + 16 * k
        self._hook(a, pyfn, nargs, name)
        s._cbinfo[a] = (pyfn, nargs, name)
        return a

    # ---- C++ objects: the GNU v2 layout (retail, SCE) and the Itanium one (GCC 14)
    #
    # A polymorphic class whose vtable is introduced by a class with data
    # members has, in GNU v2, those members first and the vptr after them
    # (FileName_Base: name +0, path +4, vptr +8); Itanium puts the vptr at +0
    # and the members after it, at `shift` (4, or 8 for 8-byte aligned
    # members).  Members of derived classes keep their offsets.  A layout is
    # (vptr, shift, size): vptr = the GNU v2 offset of the vptr.  Specs write
    # and read objects at GNU v2 offsets through T.m / T.objwrite / T.objread,
    # register every object the comparison sees (T.obj, T.place), and build
    # vtables with T.vtable; on a GNU v2 image all of this is the identity.

    def _rot(self, off, layout):
        vptr, shift, size = layout
        if not self.side.itanium or not shift or off >= vptr + shift:
            return off
        return off + shift if off < vptr else off - vptr

    def obj(self, this, layout, placed=False):
        """register the object at `this` (this side's pointer) with its layout:
        the memory comparison and pointer values (`this`, member addresses)
        then pair it with the other side's object registered in the same order."""
        self.side.objs.append((this, tuple(layout), placed))
        return this

    def place(self, addr, layout):
        """register an object whose members before the vptr must sit at addr on
        both sides (their addresses are compared as plain pointers, e.g. an
        arena inside it): returns this side's `this` (addr, or addr - shift on
        an Itanium image: the `shift` bytes before addr must be free)."""
        return self.obj(addr - (layout[1] if self.side.itanium else 0), layout, True)

    def _layout_of(self, this):
        for t_, lay, _ in self.side.objs:
            if t_ == this:
                return lay
        raise HarnessError("no object registered at %08X (T.obj)" % this)

    def m(self, this, off, layout=None):
        """this side's address of the member at GNU v2 offset `off` of the object
        at `this` (registered with T.obj, or give its layout)."""
        return this + self._rot(off, layout or self._layout_of(this))

    def objwrite(self, this, data, layout=None, off=0):
        """write bytes given in the GNU v2 layout (from v2 offset off) into the
        object, word by word at this side's offsets."""
        lay = layout or self._layout_of(this)
        data = bytes(data)
        if not self.side.itanium:
            self.mem.wr(this + off, data)
            return
        for k in range(0, len(data), 4):
            self.mem.wr(this + self._rot(off + k, lay), data[k:k + 4])

    def objread(self, this, n, layout=None, off=0):
        """n bytes of the object from GNU v2 offset off, in the GNU v2 order."""
        lay = layout or self._layout_of(this)
        if not self.side.itanium:
            return self.mem.rd(this + off, n)
        return b"".join(self.mem.rd(this + self._rot(off + k, lay), min(4, n - k)) for k in range(0, n, 4))

    def objwords(self, this, n, layout=None):
        """the first n bytes of the object as u32 words in the GNU v2 order, None
        for a word with a never-written stack byte (like Kernel.words)."""
        lay = layout or self._layout_of(this)
        return tuple(self.peek32(this + self._rot(k, lay), 1)[0] for k in range(0, n, 4))

    def halloc_obj(self, n, layout, data=None, place=False):
        """a stub-heap block for an object of n bytes (GNU v2 size) that the code
        may delete: returns this side's `this`, registered.  place=True: the
        members before the vptr sit at the same addresses on both sides (as
        T.place), the block then starts at `this` on each side."""
        s = self.side
        if not place or not s.itanium:
            p = s.heap.alloc(n + (16 if place and self.ctx.adapt else 0))
            if place and self.ctx.adapt:
                # same block size on both sides; the object 16 bytes in
                s.heap.live.pop(p)
                p += 16
                s.heap.live[p] = n
            this = p
        else:
            q = s.heap.alloc(n + 16)
            s.heap.live.pop(q)
            this = q + 16 - layout[1]
            s.heap.live[this] = n + layout[1]
        self.obj(this, layout)
        if data is not None:
            self.objwrite(this, data, layout)
        return this

    def objput(self, data, layout, align=16):
        """an object in input memory holding `data` (GNU v2 layout): returns
        this side's `this`, registered."""
        this = self.alloc(max(len(data), layout[2]), align)
        self.obj(this, layout)
        self.objwrite(this, data, layout)
        return this

    def alloc_place(self, size, layout, align=16):
        """input memory for an object placed as by T.place (its members before
        the vptr at the same address on both sides): returns this side's
        `this`; the members' address is t.m(this, 0).  Against an Itanium image
        16 bytes in front are reserved on both sides for the moved vptr."""
        pre = 16 if self.ctx.adapt else 0
        return self.place(self.alloc(size + pre, align) + pre, layout)

    def alias(self, value):
        """declare that this value and the value the other side declares in the
        same order are the same thing (compared as equal pointers)."""
        self.side.aliases.append(value)
        return value

    def _fake_code(self):
        s = self.side
        s._nthunk += 1
        a = HOOK_HI - 16 * s._nthunk
        if a < HOOK_LO + 16 * s._ncb:
            raise HarnessError("fake code area exhausted")
        return a

    def vtable(self, slots):
        """a vtable built by the test, in this side's ABI; returns the vptr value.
        slots: the GNU v2 slots from 1 on (slot 0 is the typeinfo function, left
        0), each a function address (t.fn / t.callback / 0) or (address, delta):
        the GNU v2 caller adds delta to `this`.  Slot 1 is the destructor
        (every vtable here declares it first).  Itanium: [offset-to-top 0,
        typeinfo 0] then the complete- and deleting-destructor entries (D1, D0;
        for a callback: the callback with in-charge flags 2 / 3, as GNU v2
        callers pass them), then the other slots; a delta becomes a thunk.  Both
        sides reserve the same input memory, whose bytes are not compared."""
        s = self.side
        ents = [x if isinstance(x, tuple) else (x, 0) for x in slots]
        nbytes = 8 * (len(ents) + 2) + 8
        p = self.alloc(nbytes, 16)
        s.unchecked.update(range(p, p + nbytes))
        if not s.itanium:
            data = struct.pack("<hhI", 0, 0, 0) + b"".join(struct.pack("<hhI", d, 0, f) for f, d in ents)
            data += struct.pack("<hhI", 0, 0, 0)                 # a zero slot ends the table
            self.mem.wr(p, data)
            return self.alias(p)
        words = []
        for k, (f, d) in enumerate(ents):
            if k == 0:
                words += self._itanium_dtor_entries(f)
            else:
                words.append(self._thunk(f, d) if d and f else f)
        self.mem.wr(p, struct.pack("<%dI" % (len(words) + 3), 0, 0, *(words + [0])))
        return self.alias(p + 8)

    def _thunk(self, target, delta):
        """fake code that adds delta to $a0 and continues at target."""
        a = self._fake_code()

        def h(cpu):
            cpu.g[4] = E.sx32((cpu.g[4] + delta) & 0xFFFFFFFF) & E.M64
            return target
        self.side.hook(a, h)
        return a

    def _itanium_dtor_entries(self, f):
        """[D1, D0] vtable entries for a GNU v2 destructor slot holding f."""
        s = self.side
        if not f:
            return [0, 0]
        info = s._cbinfo.get(f)
        if info is not None:
            pyfn, nargs, name = info
            out = []
            for flags in (2, 3):
                a = self._fake_code()
                self._hook(a, pyfn, nargs, name, fix=lambda args, flags=flags: [args[0], flags] + list(args[2:]))
                out.append(a)
            return out
        xt = s.xtor(f)
        if xt and xt[0] == "dtor":
            return [f, xt[1] or 0]
        return [f, f]

    def _hook(self, addr, pyfn, nargs, name, log=None, proto=None, fix=None):
        t = self
        nlog = nargs if log is None else log
        side = self.side

        def h(cpu):
            args, fl = decode_args(side, cpu, nargs, proto)
            if fix is not None:
                args = fix(args)
            t._stub_floats = fl
            try:
                r = pyfn(t, args)
            finally:
                t._stub_floats = None
            t.side.events.append(("stub", name, tuple(args[:nlog]), r))
            stub_return(side, cpu, addr, r, proto)
            return True
        self.side.hook(addr, h)


def decode_args(side, cpu, nargs, proto):
    """(the low 32 bits of the first nargs integer arguments, the float
    arguments' bit patterns) of the call a stub intercepts: EABI in order
    ($4.., $f12..); n32 by argument position (needs the prototype; without one
    every argument is taken as an integer, as on EABI)."""
    g = cpu.g
    if not side.n32 or proto is None:
        return [g[4 + i] & 0xFFFFFFFF for i in range(nargs)], list(cpu.f[12:20])
    ints, fl, pos = [], [], 0

    def slot(k):
        return g[4 + k] if k < 8 else side.mem.u64(((g[29] & 0xFFFFFFFF) + 8 * (k - 8)))
    for c in proto.params:
        if c == "q":
            pos += pos & 1
            ints.append(slot(pos) & 0xFFFFFFFF)
            pos += 2
        elif c == "f":
            fl.append(cpu.f[12 + pos] if pos < 8 else slot(pos) & 0xFFFFFFFF)
            pos += 1
        else:
            ints.append(slot(pos) & 0xFFFFFFFF)
            pos += 1
    while len(ints) < nargs:                                 # variadic: the next slots
        ints.append(slot(pos) & 0xFFFFFFFF)
        pos += 1
    return ints[:nargs], fl


def stub_return(side, cpu, callee, r, proto=None):
    """leave a stubbed call: clobber the caller-saved registers as a real callee
    may, then $v0 = r.  On an IPA-RA image a caller of the same unit may keep
    values in the registers its callee is known not to touch (GCC -fipa-ra), so
    nothing is clobbered there, and $v0 is left alone for a void stub (r None).
    In an adapted (n32) comparison r is promoted to the declared return type."""
    keep = side.keeps_caller_saved(cpu, callee)
    if not keep:
        _clobber(cpu)
    if r is None and keep:
        return
    v = r or 0
    if proto is not None and proto.ret not in ("v", "f", "q", "agg"):
        v = promote(proto.ret, v)
    cpu.g[2] = v & E.M64


def _clobber(cpu):
    """what a real callee may do to the caller-saved registers."""
    g = cpu.g
    for r in (1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25):
        g[r] = CLOBBER | r
    cpu.hi, cpu.lo = CLOBBER | 0x40, CLOBBER | 0x41


# ------------------------------------------------------------------ allocator and kernel stubs

HEAP_FUNCS = [  # retail address, name, kind
    (0x01011390, "malloc", "alloc"), (0x01011368, "free", "free"), (0x01011320, "calloc", "calloc"),
    (0x010113C8, "realloc", "realloc"), (0x01011410, "memalign", "memalign"),
    (0x01019988, "operator new", "alloc"), (0x01019910, "operator delete", "free"),
    (0x01019B28, "operator new[]", "alloc"), (0x01019AB0, "operator delete[]", "free"),
]
SCEPRINTF = 0x0101D2A8
FREED = 0xDD
FRESH = 0xCD


def install_base_hooks(side):
    """allocator and kernel-TTY stubs, identical on both sides."""
    mem, heap = side.mem, side.heap

    def mk(name, kind, addr):
        def h(cpu):
            g = cpu.g
            a0, a1 = g[4] & 0xFFFFFFFF, g[5] & 0xFFFFFFFF
            if kind == "alloc":
                p = heap.alloc(a0)
                mem.wr(p, bytes([FRESH]) * max(a0, 1))
                side.events.append(("stub", name, (a0,), p))
            elif kind == "calloc":
                n = a0 * a1
                p = heap.alloc(n)
                mem.wr(p, bytes(max(n, 1)))
                side.events.append(("stub", name, (a0, a1), p))
            elif kind == "memalign":
                p = heap.alloc(a1, max(16, a0))
                mem.wr(p, bytes([FRESH]) * max(a1, 1))
                side.events.append(("stub", name, (a0, a1), p))
            elif kind == "realloc":
                old = heap.live.get(a0, 0) if a0 else 0
                if a0 and a0 not in heap.live:
                    raise E.EmuError("realloc of %08X: not a heap block" % a0, "heap", a0)
                p = heap.alloc(a1)
                mem.wr(p, bytes([FRESH]) * max(a1, 1))
                if a0:
                    mem.wr(p, mem.rd(a0, min(old, a1)))
                    n = heap.free(a0)
                    mem.wr(a0, bytes([FREED]) * n)
                side.events.append(("stub", name, (a0, a1), p))
            else:
                n = heap.free(a0)
                if n:
                    mem.wr(a0, bytes([FREED]) * n)
                side.events.append(("stub", name, (a0,), None))
                p = None                                  # void
            stub_return(side, cpu, addr, p)
            return True
        return h

    for ra, name, kind in HEAP_FUNCS:
        try:
            a = side.func(ra)
            side.base_hooks[a] = mk(name, kind, a)
        except HarnessError as e:
            side.unhooked.append(str(e))              # reported by main()

    def sce_printf(cpu):
        g = cpu.g
        fmt = mem.cstr(g[4] & 0xFFFFFFFF)
        args = []
        if fmt == b"%s":
            args.append(mem.cstr(g[5] & 0xFFFFFFFF))
        side.events.append(("stub", "scePrintf", (fmt,) + tuple(args), None))
        stub_return(side, cpu, printf_addr[0], 0)
        return True
    printf_addr = [None]
    try:
        printf_addr[0] = side.func(SCEPRINTF)
        side.base_hooks[printf_addr[0]] = sce_printf
    except HarnessError as e:
        side.unhooked.append(str(e))


# ------------------------------------------------------------------ image address tokens

class ImageMap:
    """Maps image addresses of either side to comparable tokens (used only when
    the two images are laid out differently)."""

    def __init__(self, ctx, retail, rebuilt):
        self.ctx = ctx
        self.retail, self.rebuilt = retail, rebuilt
        self.identity = bytes(retail.mem.m[retail.image_lo:retail.image_hi]) == \
            bytes(rebuilt.mem.m[retail.image_lo:retail.image_hi]) and \
            (rebuilt.image_lo, rebuilt.image_hi) == (retail.image_lo, retail.image_hi)
        self._built = False

    def build(self):
        if self._built or self.identity:
            return
        self._built = True
        names = self.ctx.names
        rb = self.rebuilt
        # functions: rebuilt addr -> retail addr
        self.func_r2 = {}
        for a in names.funcs:
            try:
                self.func_r2[rb.func(a)] = a
            except HarnessError:
                pass
        if rb.itanium:
            # an Itanium deleting destructor (D0) stands for the same GNU v2
            # destructor as the complete-object one (D1) the names resolve to
            for b, a in list(self.func_r2.items()):
                xt = rb.xtor(b)
                if xt and xt[0] == "dtor" and xt[1] is not None:
                    self.func_r2.setdefault(xt[1], a)
        self.func_retail = set(names.funcs)
        # data objects: [(lo, hi, token_name)] per side
        self.data_rt, self.data_rb = [], []
        marks = sorted({ra for _, ra, _, _, _ in self.ctx.datamap()} | set(names.funcs) | {0x0104DEB0})
        for nm, ra, rel, sa, size in self.ctx.datamap():
            if not size and (nm in self.ctx._data_absent or nm in self.ctx._data_renamed):
                # a static GCC 14 dropped (its stores were dead), or one the GCC 14
                # branch names differently: retail's object up to the next known
                # retail item, so that its writes have a token (compared for a
                # renamed one: "on one side only")
                k = bisect.bisect_right(marks, ra)
                size = marks[k] - ra if k < len(marks) else 0
                if size:
                    self.data_rt.append((ra, ra + size, nm))
                continue
            if size:
                self.data_rt.append((ra, ra + size, nm))
                if sa is not None:
                    self.data_rb.append((sa, sa + size, nm))
        self.data_rt.sort()
        self.data_rb.sort()
        self.rt_lo = [x[0] for x in self.data_rt]
        self.rb_lo = [x[0] for x in self.data_rb]

    def token(self, side, a):
        self.build()
        if side.retail:
            if a in self.func_retail:
                return ("func", a)
            tab, los = self.data_rt, self.rt_lo
        else:
            if a in self.func_r2:
                return ("func", self.func_r2[a])
            tab, los = self.data_rb, self.rb_lo
        i = bisect.bisect_right(los, a) - 1
        if i >= 0 and tab[i][0] <= a < tab[i][1]:
            return ("data", tab[i][2], a - tab[i][0])
        return ("raw", side.label, a)

    def in_image(self, side, v):
        return side.image_lo <= v < side.image_hi

    def ptr_equiv(self, va, vb):
        """are retail pointer va and rebuilt pointer vb the same thing?  The same
        segment bits on both (kseg0 0x8..., uncached 0x2..., a DMA address's
        scratchpad bit) are looked through: 0x81034000 ~ 0x81035000 when 0x01034000
        and 0x01035000 are the same object."""
        if va == vb:
            return True
        if self.identity:
            return False
        if not (isinstance(va, int) and isinstance(vb, int)):
            return False
        seg = va >> 28
        if seg and seg == vb >> 28 and seg != 7:
            va &= 0x0FFFFFFF
            vb &= 0x0FFFFFFF
        A, B = self.retail, self.rebuilt
        if va == A.cpu.gp and vb == B.cpu.gp:
            return True                                  # the program's _gp (e.g. ThreadParam.gpReg)
        if not (self.in_image(A, va) and self.in_image(B, vb)):
            return False
        ta, tb = self.token(A, va), self.token(B, vb)
        if ta == tb and ta[0] != "raw":
            return True
        # one past the end of the same object (a buffer's end pointer): the byte
        # before it is the same byte of the same object
        ea, eb = self.token(A, va - 1), self.token(B, vb - 1)
        if ea == eb and ea[0] == "data":
            return True
        # a vtable pointer: the same functions in the same slots
        if B.itanium:
            sa, sb = self._vtable_norm(A, va), self._vtable_norm(B, vb)
        else:
            sa, sb = self._vtable(A, va), self._vtable(B, vb)
        if sa and sa == sb:
            return True
        # a string: the same bytes
        try:
            xa, xb = A.mem.cstr(va, 512), B.mem.cstr(vb, 512)
            if xa == xb and len(xa) >= 1 and all(32 <= c < 127 or c in (9, 10, 13) for c in xa):
                return True
        except E.EmuError:
            pass
        return False

    def _vtable(self, side, v):
        """the {delta, function} slots of a GCC 2.9x vtable at v, or None.  The
        walk stops at the first slot that is not a function and at a typeinfo
        function (__tf...) after the first slot: that is the next vtable's slot
        0, and what follows a vtable depends on the data layout."""
        out = []
        for k in range(8):
            try:
                pfn = side.mem.u32(v + 8 * k + 4)
                delta = side.mem.u16(v + 8 * k)
            except E.EmuError:
                break
            t = self.token(side, pfn)
            if t[0] != "func":
                break
            if k and self.ctx.names.display(t[1]).startswith("__tf"):
                break
            out.append((delta, t))
        return out if len(out) >= 2 else None

    def _vtable_norm(self, side, v):
        """a vtable at v as ((delta, function token), ...) from the first virtual
        function on, the same for a GNU v2 vtable (slot 0, the typeinfo
        function, dropped) and an Itanium one (4-byte entries from the address
        point; a destructor's complete-object entry followed by its own
        deleting one, D1 D0, counted once).  A deleting destructor not right
        after its D1 (a vptr one entry past the address point) stays a slot of
        its own ('D0'), so it never equals the GNU v2 destructor slot.  None if
        v is none."""
        if not side.itanium:
            s = self._vtable(side, v)
            return tuple(s[1:]) if s else None
        ents = []
        for k in range(16):
            try:
                pfn = side.mem.u32(v + 4 * k)
            except E.EmuError:
                break
            t = self.token(side, pfn)
            if t[0] != "func":
                break
            ents.append((pfn, t))
        side.xtor(0)                                     # (builds the table)
        d0s = {x[1] for x in getattr(side, "_xtors", {}).values() if x[0] == "dtor" and x[1] is not None}
        out, k = [], 0
        while k < len(ents):
            pfn, t = ents[k]
            xt = side.xtor(pfn)
            if xt and xt[0] == "dtor" and xt[1] is not None and k + 1 < len(ents) and ents[k + 1][0] == xt[1]:
                out.append((0, t))                       # D1, D0: the GNU v2 destructor slot
                k += 2
                continue
            out.append(("D0" if pfn in d0s else 0, t))
            k += 1
        return tuple(out) if out else None


# ------------------------------------------------------------------ specs, cases

class Spec:
    def __init__(self, name, funcs, gen, n=DEFAULT_N, notes=""):
        self.name = name
        self.funcs = list(funcs)          # retail addresses covered
        self.gen = gen                    # gen(rng, i, n) -> Case
        self.n = n
        self.notes = notes
        self.group = None                 # set by difftest_specs (the module it is in)


class Case:
    def __init__(self, desc, run, detail=None):
        self.desc = desc
        self.run = run                    # run(t)
        self.detail = detail              # optional callable -> str (full inputs)


# ------------------------------------------------------------------ the harness

class Harness:
    def __init__(self, elf, verbose=False):
        self.names = Names()
        self.verbose = verbose
        self.retail = Side("retail", RETAIL_PATH, self.names, gp=RETAIL_GP, retail=True)
        self.rebuilt = Side("rebuilt", elf, self.names)
        self.map = ImageMap(self, self.retail, self.rebuilt)
        self.rebuilt.identical = self.map.identity
        # an n32 image (the GCC 14 build): calls go through the prototypes
        # (T.call / T.stub), with each side's calling convention
        self.adapt = self.rebuilt.n32
        self.protos = Protos(self.names) if self.adapt else None
        if self.adapt:
            self.rebuilt.absent_reason = self.not_emitted
        self._datamap = None
        self._data_absent = set()                        # globals GCC 14 dropped or folded (adapted runs)
        self._data_renamed = set()                       # globals only the BOOT_OLD_GCC branch defines (adapted runs)
        self._typeinfo = None
        self.typeinfo_nodes()                          # runs code: before any case
        for s in (self.retail, self.rebuilt):
            install_base_hooks(s)

    def proto(self, func):
        """the Proto of the function at retail address func (adapted runs), or
        None (also when its arguments are not known)."""
        p = self.protos.proto(func) if self.protos is not None else None
        return p if p is not None and p.params is not None else None

    def calls_directly(self, caller, callee):
        """does the rebuilt image's function at retail address `caller` contain
        a jal / j to the one at `callee` (T.stub_reaches)?  True when not adapted."""
        if not self.adapt:
            return True
        rb = self.rebuilt
        try:
            a, b = rb.func(caller), rb.func(callee)
        except HarnessError:
            return False
        size = max([s[2] for s in rb.syms if s[1] == a and s[3] == 2] or [0])
        for k in range(a, a + size, 4):
            w = rb.mem.u32(k)
            if w >> 26 in (2, 3) and ((k + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2) == b:
                return True
        return False

    def not_emitted(self, a):
        """why the rebuilt (GCC 14) image legitimately has no function for retail
        address a, or None (then a missing symbol is a real MISSING):
          * a GNU v2 typeinfo function (__tf...): an Itanium image built with
            -fno-rtti has none;
          * a function with internal linkage (static, anonymous namespace) or an
            inline one: GCC 14 inlines it into its callers or drops it unused,
            or emits only a clone (name.isra.0 / .constprop.0 / .part.0) whose
            calling convention it changed - its code is tested through its
            callers."""
        rb = self.rebuilt
        disp = self.names.display(a)
        if rb.itanium and (disp.startswith("__tf") or self.names.symaddrs.get(a, "").startswith("__tf")):
            return "a GNU v2 typeinfo function (an Itanium -fno-rtti image has none)"
        p = self.protos.proto(a) if self.protos is not None else None
        linkage = p.linkage if p is not None else None
        mtext = self.names.markers.get(a, ("", ""))[0]
        if linkage != "static" and re.search(r"\binline\b|compiler-generated|\bimplicit\b", mtext):
            # the marker says so: an inline member's or an implicit one's copy -
            # unless the image has a function of that name (then the lookup failed)
            first = mtext.split()[0] if mtext.split() else ""
            v2sym = "::" not in first and "(" not in first and (first.startswith(("__", "_$_")) or "__" in first[1:])
            want = v2mangle.parts(first) if v2sym else v2mangle.marker_parts(mtext)
            if want and norm_parts(want) in rb._demangled():
                return None
            linkage = "inline"
        if linkage not in ("static", "inline"):
            return None
        base = p.name.split("::")[-1] if p is not None else disp
        clones = sorted(s[0] for s in rb.syms if s[3] == 2 and re.match(r"^%s\.(isra|constprop|part|cold)\.\d+$" %
                                                                        re.escape(base), s[0]))
        if clones:
            return "%s function emitted only as GCC clone(s) %s (changed calling convention)" % (
                linkage, ", ".join(clones))
        return "%s function not emitted by GCC (inlined into its callers or unused)" % linkage

    def typeinfo_nodes(self):
        """[(name, retail addr, rebuilt addr, size)] of the GCC 2.9x type_info
        nodes (__ti<type>, .bss / .sdata objects with no source annotation, which
        the typeinfo functions __tf<type> fill in and return).  Retail's address
        is what its __tf function returns, run once in the pristine image; the
        rebuilt one is its __ti symbol (or what its __tf returns)."""
        if self._typeinfo is not None:
            return self._typeinfo
        out = []
        if not self.map.identity:
            for a, (text, rel) in sorted(self.names.markers.items()):
                if not text.startswith("__tf"):
                    continue
                ti = "__ti" + text.split()[0][4:]
                syms = [x for x in self.rebuilt.by_name.get(ti, []) if x[1]]
                ra = self._run_pristine(self.retail, a)
                sa = syms[0][1] if len(syms) == 1 else None
                if sa is None:
                    try:
                        sa = self._run_pristine(self.rebuilt, self.rebuilt.func(a))
                    except HarnessError:
                        sa = None
                size = syms[0][2] if len(syms) == 1 and syms[0][2] else 8
                if ra is not None and sa is not None and IMAGE_LO <= ra < IMAGE_HI:
                    out.append((ti, ra, sa, size))
        self._typeinfo = out
        return out

    @staticmethod
    def _run_pristine(side, addr):
        """$v0 of a call of addr (no arguments) on the pristine image, which is
        restored afterwards; None if it fails."""
        hooks, sc = side.cpu.hooks, side.cpu.syscall_hook
        side.mem.reset()
        side.cpu.hooks, side.cpu.syscall_hook = {}, None
        try:
            v0, _, _ = side.cpu.call(addr, [], maxsteps=100000)
            v0 &= 0xFFFFFFFF
        except E.EmuError:
            v0 = None
        finally:
            side.mem.reset()
            side.io.reset()
            side.cpu.hooks, side.cpu.syscall_hook = hooks, sc
        return v0

    # -------------------------------------------------------------- data symbols
    def datamap(self):
        """[(C name, retail addr, rel, rebuilt addr or None, size)]"""
        if self._datamap is not None:
            return self._datamap
        rb = self.rebuilt
        by_base = collections.defaultdict(list)
        # Itanium-mangled data (GCC 14: `_ZL5g_gfx`, `_ZN4GZIPL6cpdistE`): by the
        # last part of the demangled name
        dm = itanium_demangle(sorted({s[0] for s in rb.syms if s[0].startswith("_Z") and s[5] != 0 and
                                      s[3] not in (2, 4)}))
        for s in rb.syms:
            if s[5] == 0 or s[3] in (2, 4):
                continue
            n = s[0]
            if n.startswith("_GLOBAL_$F$"):
                continue                     # egcs's .eh_frame label of a file, named after its first global
            if n in dm:
                d = dm[n]
                if not re.match(r"^(vtable|typeinfo|VTT|guard variable|construction vtable) ", d):
                    by_base[re.sub(r"<[^<>]*>", "", d).split("::")[-1].strip()].append(s)
                continue
            base = n.rsplit("$", 1)[-1] if "$" in n else n
            base = re.sub(r"\.\d+$", "", base)
            by_base[base].append(s)
            if "__" in n and not n.startswith("_"):
                by_base[n.split("__", 1)[0]].append(s)
        # a symbol without a size (egcs's local .bss objects are NOTYPE, size 0)
        # extends to the next rebuilt symbol of its section, but not past the
        # next annotated retail object or function
        nexts = collections.defaultdict(list)
        for s in rb.syms:
            if s[5] not in (0, 0xFFF1) and s[3] != 4 and s[1]:
                nexts[s[5]].append(s[1])
        for k in nexts:
            nexts[k] = sorted(set(nexts[k]))
        rt_marks = sorted(set([ra for _, ra, _ in self.names.data] + list(self.names.funcs) + [0x0104DEB0]))

        def inferred_size(s, ra):
            lst = nexts.get(s[5], [])
            j = bisect.bisect_right(lst, s[1])
            if j >= len(lst):
                return 0
            k = bisect.bisect_right(rt_marks, ra)
            gap_rt = rt_marks[k] - ra if k < len(rt_marks) else 0
            gap_rb = lst[j] - s[1]
            return min(gap_rb, gap_rt) if gap_rt > 0 else gap_rb
        # a name defined several times (function-local statics such as
        # `initialized` in eh.c and libgcc2.c): when the rebuilt ELF has as many
        # objects of that name, pair them in address order (link order)
        retail_addrs = collections.defaultdict(set)
        for nm, ra, rel in self.names.data:
            retail_addrs[nm].add(ra)
        paired = {}
        for nm, ras in retail_addrs.items():
            cands = by_base.get(nm, [])
            if len(ras) > 1 and len(cands) == len(ras):
                for ra, s in zip(sorted(ras), sorted(cands, key=lambda x: x[1])):
                    paired[(nm, ra)] = s
        out = []
        seen = set()
        for nm, ra, rel in self.names.data:
            if (nm, ra) in seen:
                continue
            seen.add((nm, ra))
            cands = by_base.get(nm, [])
            if (nm, ra) in paired:
                cands = [paired[(nm, ra)]]
            elif len(cands) > 1 and len(retail_addrs[nm]) == 1 and len([s for s in cands if s[0] == nm]) == 1:
                cands = [s for s in cands if s[0] == nm]     # the exact name over decorated ones
            if self.map.identity:
                sa, size = ra, (cands[0][2] if len(cands) == 1 else 0)
            elif len(cands) == 1:
                sa, size = cands[0][1], cands[0][2]
                if not size:
                    size = inferred_size(cands[0], ra)
            else:
                sa, size = None, 0
                if not cands and self.adapt:
                    # dropped / folded by GCC only if the GCC 14 side of the
                    # source still defines it under this name; a name that only
                    # the BOOT_OLD_GCC branch has (s_allocator, whose GCC 14
                    # storage is s_allocatorStorage) is not dead: its writes
                    # must not vanish from the comparison
                    if self._gcc14_names(nm, rel):
                        self._data_absent.add(nm)
                    else:
                        self._data_renamed.add(nm)
            out.append((nm, ra, rel, sa, size))
        for nm, ra, sa, size in self.typeinfo_nodes():
            out.append((nm, ra, "(type_info node)", sa, size))
        self._datamap = out
        return out

    def data_addr(self, side, name):
        for nm, ra, rel, sa, size in self.datamap():
            if nm == name:
                if side.retail or self.map.identity:
                    return ra
                if sa is None:
                    if name in self._data_absent:
                        # GCC 14 keeps no object for a static the program never
                        # writes: its value is folded into the readers
                        raise NotEmitted("global %s: no object in the rebuilt image (a static GCC folded into "
                                         "its readers or dropped)" % name)
                    if name in self._data_renamed:
                        raise HarnessError("rebuilt ELF: no symbol for global %s, which only the BOOT_OLD_GCC "
                                           "branch of %s defines (annotate the GCC 14 object with its retail "
                                           "address)" % (name, rel))
                    raise HarnessError("rebuilt ELF: no unique symbol for global %s" % name)
                return sa
        raise HarnessError("no annotated global %s" % name)

    def _gcc14_names(self, name, rel):
        """does the GCC 14 side of `rel` (BOOT_OLD_GCC branches and comments
        removed) mention the identifier `name`?"""
        if self.protos is None or rel.startswith("("):
            return True
        try:
            lines = self.protos._lines(rel)
        except OSError:
            return True
        pat = re.compile(r"(?<![\w$])%s(?![\w$])" % re.escape(name))
        return any(pat.search(L) for L in lines)

    # -------------------------------------------------------------- checks
    def check_identity_mapping(self, funcs):
        """on a byte-identical image every resolved function must sit at its
        retail address (validates the symbol lookup itself)."""
        bad = []
        for a in funcs:
            try:
                r = self.rebuilt.func(a)
            except HarnessError as e:
                bad.append(str(e))
                continue
            if r != a:
                bad.append("%s resolved to %08X, retail %08X" % (self.names.display(a), r, a))
        return bad

    # -------------------------------------------------------------- one case
    def run_case(self, case):
        out = []
        for side in (self.retail, self.rebuilt):
            side.begin()
            if side.retail and getattr(self, "reach", None) is not None:
                for f in self.reach:
                    if f not in side.cpu.hooks:
                        side.cpu.hooks[f] = (lambda cpu, f=f: self.reached.add(f) and False)
            t = T(side, self)
            try:
                case.run(t)
            except NotEmitted:
                raise                                    # the case is skipped (run_spec)
            except E.EmuError as e:
                where = getattr(e, "pc", None)
                side.err = e
                side.err_msg = "%s%s" % (e, "" if where is None or "%08X" % where in str(e) else " (pc %08X)" % where)
            except HarnessError:
                raise
            out.append(side.cpu.steps - side.steps0)
        return out

    def compare(self):
        d = self._compare()
        if d:
            # a difference after a read of uninitialised stack may be the
            # indeterminate value itself rather than the code (see MATCHING.md)
            for side in (self.retail, self.rebuilt):
                u = side.mem.unwritten_reads
                if u:
                    d.append("note: %s read %s never-written stack byte(s) (first %08X): if the difference "
                             "comes from that value it is indeterminate data, not behaviour" % (
                                 side.label, "16+" if len(u) >= 16 else len(u), u[0]))
        return d

    def _compare(self):
        A, B = self.retail, self.rebuilt
        above = self._above_sp()
        if above:
            return above
        if A.err is not None or B.err is not None:
            if A.err is None or B.err is None:
                return ["error on one side only: retail: %s | rebuilt: %s" % (A.err_msg or "none", B.err_msg or "none")]
            why = self._err_differs(A.err, B.err)
            if why:
                return ["errors differ (%s): retail: %s | rebuilt: %s" % (why, A.err_msg, B.err_msg)]
            # the same failure on both sides: what happened before it must agree too
            return self._event_diff(prefix=A.err.kind == "runaway")
        return self._event_diff() or self._mem_diff()

    def _err_differs(self, ea, eb):
        """None when two errors are the same failure, else why not."""
        if ea.kind != eb.kind:
            if self.adapt and {ea.kind, eb.kind} & {"break 7"} and {ea.kind, eb.kind} & {"div0", "trap"}:
                return "class %s vs %s; known GCC 14 difference: its divide checks trap with `teq` after the " \
                       "`div`, retail's with `break 7` (MATCHING.md \"Keeping behaviour\")" % (ea.kind, eb.kind)
            return "class %s vs %s" % (ea.kind, eb.kind)
        if ea.addr is None and eb.addr is None:
            return None
        if ea.addr is None or eb.addr is None or not self._val_equal(ea.addr, eb.addr):
            return "faulting address"
        return None

    def _event_diff(self, prefix=False):
        """events in order; prefix: only over the length of the shorter list (and
        the repeat count of the last merged read there is not compared: the run
        was cut off in the middle of a polling loop)."""
        ea, eb = self.retail.events, self.rebuilt.events
        if self.adapt:
            ea, eb = _cache_runs(ea), _cache_runs(eb)
        n = min(len(ea), len(eb)) if prefix else max(len(ea), len(eb))
        for i in range(n):
            x = ea[i] if i < len(ea) else None
            y = eb[i] if i < len(eb) else None
            if prefix and i == n - 1 and x is not None and y is not None and x[0] == y[0] == "hw":
                x, y = x[:3], y[:3]
            if not self._event_equal(x, y):
                ctx = ea[max(0, i - 3):i]
                return ["event %d differs:\n      retail:  %s\n      rebuilt: %s%s" % (
                    i, _fmt_event(x), _fmt_event(y),
                    ("\n      (after %s)" % " / ".join(_fmt_event(c) for c in ctx)) if ctx else "")]
        return []

    def _event_equal(self, x, y):
        if x is None or y is None:
            return x is y
        if x[0] != y[0] or x[1] != y[1]:
            return False
        if x[0] == "hw" and x[1][0] == "cache-run":
            return self._cache_runs_equal(x[2], y[2])
        if x[0] == "ret" and x[2] == "ptr":
            return self.map.ptr_equiv(x[3], y[3])
        if x[0] == "obs" and x[2] == "ptr":
            return self.map.ptr_equiv(x[3], y[3])
        if x[0] == "obs" and x[2] == "ptrs" and x != y:
            return x[2] == y[2] and self._deep_equal(x[3], y[3])
        if x[0] == "stub" and x != y:
            if len(x[2]) != len(y[2]) or not self._val_equal(x[3], y[3]):
                return False
            return all(self._val_equal(p, q) for p, q in zip(x[2], y[2]))
        if x[0] == "hw" and x != y:
            return x[3:] == y[3:] and self._hw_val_equal(x[1], x[2], y[2])
        return x == y

    def _deep_equal(self, p, q):
        """t.obs(..., 'ptrs') values: nested tuples / lists whose ints compare
        like pointers (_val_equal) and everything else exactly."""
        if isinstance(p, (tuple, list)) and isinstance(q, (tuple, list)):
            return len(p) == len(q) and all(self._deep_equal(a, b) for a, b in zip(p, q))
        if isinstance(p, int) and isinstance(q, int) and not isinstance(p, bool):
            return self._val_equal(p, q)
        return p == q

    def _hw_val_equal(self, key, p, q):
        """the values of two hardware events with the same key.  Values that are
        memory addresses (DMA MADR/TADR/ASR stores and loads, CACHE addresses, the
        tag ADDR words of a DMA snapshot) compare like pointers; everything else
        exactly.  A DMA snapshot's payload compares by content."""
        if p == q:
            return True
        k = key[0]
        if k in ("r", "w"):
            return key[1] in E.DMA_ADDR_REGS and self._val_equal(p, q)
        if k == "cache":
            return self._val_equal(p, q)
        if k == "dma":
            if key[2] == "to-memory":
                return p[1] == q[1] and self._val_equal(p[0], q[0])
            if not (isinstance(p, tuple) and isinstance(q, tuple)) or len(p) != len(q):
                return False
            for a, b in zip(p, q):
                if a == b:
                    continue
                if a[0] != b[0] or a[0] == "end":
                    return False
                if a[0] == "tag":
                    if a[1] != b[1] or a[3:] != b[3:] or not self._val_equal(a[2], b[2]):
                        return False
                elif a[0] == "data":
                    if len(a[1]) != len(b[1]):
                        return False
                    wa = struct.unpack("<%dI" % (len(a[1]) // 4), a[1])
                    wb = struct.unpack("<%dI" % (len(b[1]) // 4), b[1])
                    if not all(u == v or self.map.ptr_equiv(u, v) for u, v in zip(wa, wb)):
                        return False
                else:
                    return False
            return True
        return False

    def _val_equal(self, p, q):
        """stub arguments / results: equal, the same image object, or both on the
        stack (also through the same segment bits: kseg0, uncached, the DMA
        scratchpad bit)."""
        if p == q:
            return True
        if not (isinstance(p, int) and isinstance(q, int)):
            return False
        seg = p >> 28
        if seg and seg == q >> 28 and seg != 7:
            p &= 0x0FFFFFFF
            q &= 0x0FFFFFFF
        if STACK_LO <= p < STACK_HI and STACK_LO <= q < STACK_HI:
            return True
        if self.adapt and self._paired_equal(p, q):
            return True
        return self.map.ptr_equiv(p, q)

    # -------------------------------------------------------------- objects (T.obj, T.alias)
    def _obj_pairs(self):
        """[(retail this, rebuilt this, layout)] of the objects both sides
        registered, paired in order (adapted runs only)."""
        if not self.adapt:
            return []
        return [(ta, tb, la, pa) for (ta, la, pa), (tb, lb, pb) in zip(self.retail.objs, self.rebuilt.objs)]

    def _rot(self, o, lay):
        """the rebuilt (Itanium) offset of GNU v2 offset o."""
        vptr, shift, size = lay
        if not self.rebuilt.itanium or not shift or o >= vptr + shift:
            return o
        return o + shift if o < vptr else o - vptr

    def _unrot(self, b, lay):
        """the GNU v2 offset of rebuilt (Itanium) offset b."""
        vptr, shift, size = lay
        if not self.rebuilt.itanium or not shift or b >= vptr + shift:
            return b
        return vptr + b if b < shift else b - shift

    def _paired_equal(self, p, q):
        """p (retail) and q (rebuilt) are the same object or member of objects
        registered with T.obj, or values declared the same with T.alias."""
        A, B = self.retail, self.rebuilt
        for x, y in zip(A.aliases, B.aliases):
            if x == p and y == q:
                return True
        for ta, tb, lay, placed in self._obj_pairs():
            size = lay[2]
            if placed:
                if p == ta and q == tb:
                    return True                      # `this` (the members sit at the same addresses)
                continue
            if ta <= p <= ta + size:
                o = p - ta
                if q == tb + self._rot(o, lay) or (o == 0 and q == tb) or (o == size and q == tb + size):
                    return True
        return False

    def _cache_runs_equal(self, ra, rb):
        """two runs of CACHE operations (adapted runs, _cache_runs; their SYNC
        kinds are part of the event key): per operation, the lines outside
        the images are the same, stack lines only by whether there are any;
        image lines (whose number and addresses follow the data layout, e.g. a
        write-back of a buffer that starts mid-line in one image) are on both
        sides or neither, and each side covers every image byte the other
        side's lines certainly cover for the code's own range (_image_core)."""
        A, B = self.retail, self.rebuilt
        ops = {op for op, _ in ra} | {op for op, _ in rb}
        for op in ops:
            la = {a & ~63 for o, a in ra if o == op}
            lb = {a & ~63 for o, a in rb if o == op}
            ia = {a for a in la if self.map.in_image(A, a & 0x1FFFFFFF)}
            ib = {a for a in lb if self.map.in_image(B, a & 0x1FFFFFFF)}
            sa = {a for a in la if STACK_LO <= (a & 0x1FFFFFFF) < STACK_HI}
            sb = {a for a in lb if STACK_LO <= (a & 0x1FFFFFFF) < STACK_HI}
            if bool(sa) != bool(sb):                     # frames differ: only "some stack lines"
                return False
            if la - ia - sa != lb - ib - sb:
                return False
            if bool(ia) != bool(ib):
                return False
            if ia:
                ca, cb = self._image_cover(A, ia), self._image_cover(B, ib)
                if not (self._image_core(A, ia) <= cb and self._image_core(B, ib) <= ca):
                    return False
        return True

    def _image_cover(self, side, lines):
        """the image object bytes ((name, offset) tokens) that these 64-byte
        lines of one side cover."""
        out = set()
        for a in lines:
            base = a & 0x1FFFFFFF
            for i in range(64):
                t = self.map.token(side, base + i)
                if t[0] == "data":
                    out.add(t[1:])
        return out

    def _image_core(self, side, lines):
        """the image object bytes these lines certainly cover for the range the
        code asked for: a contiguous block of lines [lo, hi) comes from a range
        that starts at most 63 bytes after lo and ends at most 63 bytes before
        hi (alignment slack), so [lo + 63, hi - 63) is inside the range and is
        covered on the other side too, whatever its alignment."""
        out = set()
        ls = sorted({a & 0x1FFFFFFF for a in lines})
        k = 0
        while k < len(ls):
            lo = hi = ls[k]
            while k < len(ls) and ls[k] == hi:
                hi += 64
                k += 1
            for x in range(lo + 63, hi - 63):
                t = self.map.token(side, x)
                if t[0] == "data":
                    out.add(t[1:])
        return out

    def _classify(self, a):
        if STACK_LO <= a < STACK_HI:
            return "stack"
        if IMAGE_LO <= a < IMAGE_HI:
            return "image"
        return "data"

    def _above_sp(self):
        """writes into the caller's frame (above its argument slots): always a failure."""
        diffs = []
        for side in (self.retail, self.rebuilt):
            above = [k for k in side.mem.orig if SP0 + STACK_ARGS <= k < STACK_HI]
            if above:
                diffs.append("%s wrote above the caller's stack pointer: %08X" % (side.label, min(above)))
        return diffs

    def _mem_diff(self):
        A, B = self.retail, self.rebuilt
        wa, wb = set(A.mem.orig), set(B.mem.orig)
        diffs = []
        ma, mb = A.mem.m, B.mem.m
        bad = []
        image_a, image_b = [], []
        unchk = A.unchecked | B.unchecked
        allpairs = self._obj_pairs()
        # an object a destructor call ended (T.call) is compared as it was right
        # before that call (Side.snapshot): a GNU v2 destructor's stores into
        # the dying object (its vptr, cleared members) are dead stores GCC 14
        # removes (-flifetime-dse); what the destructor does elsewhere (frees,
        # calls) is compared as usual
        ov_a, ov_b = {}, {}
        for ta, tb, lay, placed in allpairs:
            if ta in A.snaps and tb in B.snaps:
                for side, this, ov, w in ((A, ta, ov_a, wa), (B, tb, ov_b, wb)):
                    lo, data, written = side.snaps[this]
                    for k in range(lo, lo + len(data), 4):
                        ov[k] = data[k - lo:k - lo + 4]
                        w.discard(k)
                    w.update(written)

        def word_a(k):
            return ov_a[k] if k in ov_a else bytes(ma[k:k + 4])

        def word_b(k):
            return ov_b[k] if k in ov_b else bytes(mb[k:k + 4])
        # objects registered with T.obj: the rebuilt (Itanium) words are compared
        # at their GNU v2 places ("k" below is a retail word / the GNU v2 view)
        pairs = [(ta, tb, lay) for ta, tb, lay, placed in allpairs if not placed and self._rot(0, lay)]
        # placed objects (T.place): the members compare at their (common)
        # addresses; the two vptrs are compared as a pair below, the vptr words
        # themselves (and the Itanium padding after it) not as memory
        skip_a, skip_b, vptrs = set(), set(), []
        for ta, tb, lay, placed in allpairs:
            vptr, shift, size = lay
            if placed and B.itanium and shift:
                skip_a.add(ta + vptr)
                skip_b.update(range(tb, tb + shift, 4))
                vptrs.append((ta + vptr, tb))
        if pairs:
            def to_b(k):
                for ta, tb, lay in pairs:
                    if ta <= k < ta + lay[2]:
                        return tb + self._rot(k - ta, lay)
                return k

            def from_b(b):
                for ta, tb, lay in pairs:
                    if tb <= b < tb + lay[2]:
                        return ta + self._unrot(b - tb, lay)
                return b
            keys = wa | {from_b(b) for b in wb if b not in skip_b}
        else:
            to_b = None
            keys = wa | {b for b in wb if b not in skip_b}
        for k in sorted(keys - skip_a):
            c = self._classify(k)
            if c == "stack":
                continue
            if c == "image" and not self.map.identity:
                if k in wa:
                    image_a.append(k)
                if k in wb:
                    image_b.append(k)
                continue
            kb = to_b(k) if to_b else k
            if kb in skip_b:
                continue
            xa, xb = word_a(k), word_b(kb)
            if xa != xb:
                if unchk and any(k + i in unchk for i in range(4)):
                    if any(xa[i] != xb[i] for i in range(4) if k + i not in unchk):
                        bad.append(k)
                    continue
                va, vb = struct.unpack("<I", xa)[0], struct.unpack("<I", xb)[0]
                if self._val_equal(va, vb):       # the same object, or two stack addresses
                    continue
                bad.append(k)
        if bad:
            k = bad[0]
            kb = to_b(k) if to_b else k
            diffs.append("memory differs at %d word(s), first %08X%s:\n%s" % (
                len(bad), E.Mem.index_addr(k), self._where(k) + (
                    " (rebuilt %08X: the object's Itanium layout)" % kb if kb != k else "") + (
                    " (an object as it was before its destructor)" if k in ov_a else ""),
                _hexdiff(ma, mb, k, kb)))
        for ka, kb in vptrs:
            if ka not in wa and kb not in wb:
                continue
            va, vb = struct.unpack("<I", word_a(ka))[0], struct.unpack("<I", word_b(kb))[0]
            if self._val_equal(va, vb) or va == struct.unpack("<I", word_b(ka))[0]:
                continue                             # (or both overwritten at that address, e.g. an overrun)
            diffs.append("vptr differs: retail %08X = 0x%08X, rebuilt %08X = 0x%08X" % (ka, va, kb, vb))
        if image_a or image_b:
            diffs += self._image_diff(image_a, image_b)
        return diffs

    def _where(self, k):
        if k >= E.SPR_OFF:
            return " (scratchpad +0x%X)" % (k - E.SPR_OFF)
        h = self.retail.heap
        for blk, n in list(h.live.items()):
            if blk <= k < blk + n:
                return " (heap block %08X+0x%X)" % (blk, k - blk)
        if HEAP_LO <= k < HEAP_HI:
            return " (stub heap)"
        if INPUT_LO <= k < INPUT_HI:
            return " (test input area)"
        return ""

    def _image_diff(self, wa, wb):
        """compare writes into the images by token."""
        A, B = self.retail, self.rebuilt
        da, db = {}, {}
        skip = set()                              # tokens of bytes a side declared unchecked (T.unchecked)
        for side, ws, d in ((A, wa, da), (B, wb, db)):
            for k in ws:
                old = side.mem.orig.get(k)
                for i in range(4):
                    t = self.map.token(side, k + i)
                    if t[0] == "raw" and old is not None and side.mem.m[k + i] == old[i]:
                        continue                  # padding next to a small object: the journal is per word
                    if k + i in side.unchecked:
                        skip.add(t)
                        continue
                    d[t] = side.mem.m[k + i]
        # a byte written on one side only must still hold the same value on the other
        out = []
        vals = {}
        for t in sorted(set(da) | set(db), key=repr):
            x, y = da.get(t), db.get(t)
            if t[0] == "data" and t[1] in self._data_absent:
                continue                          # no such object in the rebuilt image: dead stores
            if t in skip:
                continue
            if x is None or y is None:
                if t[0] == "raw":
                    out.append("%s side wrote an image byte with no symbol: %s" % (
                        "retail" if x is not None else "rebuilt", _fmt_tok(t)))
                    continue
                other = B if x is None else A
                addr = self._tok_addr(other, t)
                if addr is None:
                    out.append("image write %s on one side only (no counterpart)" % _fmt_tok(t))
                    continue
                if x is None:
                    x = other.mem.m[addr]
                else:
                    y = other.mem.m[addr]
            vals[t] = (x, y)
        # differing bytes: a whole word of an object that holds the same pointer on
        # both sides (an image object, a stack address) is not a difference
        for t, (x, y) in sorted(vals.items(), key=lambda kv: repr(kv[0])):
            if x == y:
                continue
            if t[0] == "data":
                w = [vals.get((t[0], t[1], (t[2] & ~3) + i)) for i in range(4)]
                if None not in w:
                    pa = sum(v[0] << (8 * i) for i, v in enumerate(w))
                    pb = sum(v[1] << (8 * i) for i, v in enumerate(w))
                    if self._val_equal(pa, pb):
                        continue
            out.append("image byte %s: retail %02X rebuilt %02X" % (_fmt_tok(t), x, y))
        return out[:1] + (["... %d more image differences" % (len(out) - 1)] if len(out) > 1 else [])

    def _tok_addr(self, side, t):
        if t[0] == "data":
            try:
                return self.data_addr(side, t[1]) + t[2]
            except HarnessError:
                return None
        if t[0] == "func":
            return side.func(t[1])
        return None

    # -------------------------------------------------------------- one spec
    def run_spec(self, spec, n, seed, keep_going=False, only=None):
        """only: a set of 0-based case numbers to run (the others are still
        generated, so every case gets the same inputs as in a full run)."""
        rng = random.Random(int.from_bytes(hashlib.sha256(("%s/%d" % (spec.name, seed)).encode()).digest()[:8],
                                           "little"))
        t0 = time.time()
        steps = [0, 0]
        fails = []
        self.skipped = []                                # (case number, why): uses a function not emitted
        for i in range(n):
            case = spec.gen(rng, i, n)
            if only is not None and i not in only:
                continue
            try:
                s = self.run_case(case)
            except NotEmitted as e:
                self.skipped.append((i, str(e)))
                continue
            steps[0] += s[0]
            steps[1] += s[1]
            if getattr(self, "show", False):
                print("  case %d: %s" % (i + 1, case.desc))
                for e in self.retail.events:
                    print("      " + _fmt_event(e))
                if self.retail.err is not None:
                    print("      error: %s" % self.retail.err_msg)
            d = self.compare()
            if d:
                fails.append((i, case, d))
                if not keep_going:
                    break
        return fails, time.time() - t0, steps


def _cache_runs(events):
    """events with every maximal run of CACHE / SYNC hardware events that
    holds a CACHE operation merged into one ('hw', ('cache-run', SYNC kinds
    in the run), ((op, address), ...), 1) event (adapted runs:
    Harness._cache_runs_equal)."""
    out, run = [], []

    def flush():
        if any(e[1][0] == "cache" for e in run):
            syncs = tuple(sorted({e[1][0] for e in run if e[1][0] != "cache"}))
            out.append(("hw", ("cache-run", syncs), tuple((e[1][1], e[2]) for e in run if e[1][0] == "cache"), 1))
        else:
            out.extend(run)
        del run[:]
    for e in events:
        if e[0] == "hw" and e[1][0] in ("cache", "sync.l", "sync.p"):
            run.append(e)
            continue
        flush()
        out.append(e)
    flush()
    return out


def _fmt_val(v):
    if isinstance(v, int):
        return "0x%X" % v
    if isinstance(v, (bytes, bytearray)):
        b = bytes(v)
        return repr(b[:120]) + ("...(%d bytes)" % len(b) if len(b) > 120 else "")
    if isinstance(v, tuple):
        return "(" + ", ".join(_fmt_val(x) for x in v) + ")"
    if isinstance(v, list):
        return "[" + ", ".join(_fmt_val(x) for x in v[:16]) + (", ..." if len(v) > 16 else "") + "]"
    return repr(v)


def _fmt_event(e):
    if e is None:
        return "(nothing)"
    if e[0] == "call":
        return "call %s" % e[1]
    if e[0] == "ret":
        return "%s returned %s" % (e[1], _fmt_val(e[3]))
    if e[0] == "obs":
        return "%s = %s" % (e[1], _fmt_val(e[3]))
    if e[0] == "stub":
        return "%s(%s) -> %s" % (e[1], ", ".join(_fmt_val(x) for x in e[2]), _fmt_val(e[3]))
    if e[0] == "hw":
        return _fmt_hw(e)
    return repr(e)


def _fmt_hw(e):
    """a hardware event: 'w64 GS_CSR (12001000) = 0x8', 'r32 D1_CHCR (10009000) -> 0x0 x3'."""
    key, v, cnt = e[1], e[2], e[3]
    rep = " x%d" % cnt if cnt != 1 else ""
    k = key[0]
    if k == "cache-run":
        return "cache run (%d ops%s): %s%s" % (len(v), "".join(", " + s for s in key[1]),
                                               ", ".join("0x%02X %X" % (o, a) for o, a in v[:4]),
                                               " ..." if len(v) > 4 else "")
    if k in ("r", "w"):
        return "%s%d %s (%08X) %s %s%s" % (k, 8 * key[2], E.io_name(key[1]), key[1], "->" if k == "r" else "=",
                                           _fmt_val(v), rep)
    if k == "dma":
        if key[2] == "to-memory":
            return "DMA %s start (%s): MADR 0x%X QWC 0x%X" % (E.DMA_NAMES[key[1]], key[2], v[0], v[1])
        parts = []
        for it in v:
            if it[0] == "tag":
                parts.append("tag %08X %08X %08X %08X" % it[1:])
            elif it[0] == "data":
                parts.append("%d qw %s" % (len(it[1]) // 16, it[1][:32].hex()))
            else:
                parts.append("end: %s" % it[1])
        more = "" if len(parts) <= 12 else " ... (%d items)" % len(parts)
        return "DMA %s start (%s): %s%s" % (E.DMA_NAMES[key[1]], key[2], " | ".join(parts[:12]), more)
    if k in ("mfc0", "cfc2"):
        return "%s %s -> %s%s" % (k, key[1], _fmt_val(v), rep)
    if k in ("mtc0", "ctc2"):
        return "%s %s = %s" % (k, key[1], _fmt_val(v))
    if k == "cache":
        return "cache 0x%02X, 0x%X" % (key[1], v)
    if k == "vcallms":
        return "vcallms %s%s" % (_fmt_val(key[1]), "" if v is None else " (CMSAR0 0x%X)" % v)
    return k + rep


def _fmt_tok(t):
    if t[0] == "data":
        return "%s+0x%X" % (t[1], t[2])
    if t[0] == "func":
        return "function 0x%08X" % t[1]
    return "%s:%08X" % (t[1], t[2])


def _hexdiff(ma, mb, k, kb=None):
    """three lines of the two memories around index k (addresses as the code
    sees them); kb: the rebuilt index of the same word, when it differs."""
    kb = k if kb is None else kb
    lo = max(k - 16, 0) & ~15
    lob = lo + (kb - k)
    lines = []
    for a, b in zip(range(lo, lo + 48, 16), range(lob, lob + 48, 16)):
        x, y = bytes(ma[a:a + 16]), bytes(mb[b:b + 16])
        mark = "  " if x == y else "!!"
        va, vb = E.Mem.index_addr(a), E.Mem.index_addr(b)
        lines.append("      %s %08X retail  %s\n         %08X rebuilt %s" % (mark, va, x.hex(" "), vb, y.hex(" ")))
    return "\n".join(lines)


# ------------------------------------------------------------------ main

def select(specs, keys, names):
    if not keys:
        return specs
    out = []
    for k in keys:
        kl = k.lower()
        addr = None
        try:
            addr = names.lookup(k)
        except KeyError:
            pass
        hit = [s for s in specs if s.name.lower() == kl or (addr is not None and addr in s.funcs)]
        if not hit:
            hit = [s for s in specs if kl in s.name.lower() or
                   any(kl in names.display(a).lower() for a in s.funcs)]
        if not hit:
            raise SystemExit("--fn %s: no spec covers it (see --list)" % k)
        for s in hit:
            if s not in out:
                out.append(s)
    return out


def _load_specs():
    import difftest_specs
    return difftest_specs


def _list(specmod, groups, names, verbose=False):
    """--list: per group, its units, specs and the functions they call directly
    (-v: also the group's functions no spec calls)."""
    unit_funcs = collections.defaultdict(set)
    for a in names.funcs:
        unit_funcs[names.unit_of(a)].add(a)
    tot_specs, tot_cov, tot_funcs = 0, set(), 0
    for g in groups:
        mod = specmod.GROUPS[g]
        specs = mod.SPECS
        units = list(mod.UNITS)
        in_units = set().union(*[unit_funcs[u] for u in units]) if units else set()
        cov = {f for sp in specs for f in sp.funcs}
        print("group %-11s units %s: %d spec(s), %d of %d functions" % (
            g, "/".join(units), len(specs), len(cov & in_units), len(in_units)))
        for sp in specs:
            print("  %-26s %4d cases  %s" % (sp.name, sp.n, sp.notes))
            for f in sp.funcs:
                u = names.unit_of(f)
                print("      0x%08X %-40s %s" % (f, names.display(f), u if u in units else
                                                   "%s  <- not a unit of this group" % u))
        if verbose:
            for f in sorted(in_units - cov):
                print("    not called by a spec: 0x%08X %s (%s)" % (f, names.display(f), names.unit_of(f)))
        tot_specs += len(specs)
        tot_cov |= cov
        tot_funcs += len(in_units)
    print("%d group(s), %d spec(s), %d functions covered of %d in the groups' units" % (
        len(groups), tot_specs, len(tot_cov), tot_funcs))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--elf", default=None, help="rebuilt ELF with a symbol table (default build/sce/scus_971.02.elf)")
    ap.add_argument("--group", action="append", default=[], metavar="NAME",
                    help="only this group's specs (repeatable; see --list)")
    ap.add_argument("--fn", action="append", default=[], metavar="NAME|ADDR",
                    help="only the specs covering this function / spec name (repeatable)")
    ap.add_argument("--n", type=int, default=DEFAULT_N,
                    help="cases for a standard spec (default %d); heavier specs scale in proportion" % DEFAULT_N)
    ap.add_argument("--seed", type=int, default=1, help="input seed (default 1)")
    ap.add_argument("--case", type=int, action="append", default=[], metavar="K",
                    help="run only case K (1-based, as FAIL prints it) of the selected specs (repeatable)")
    ap.add_argument("--list", action="store_true", help="list the groups, specs and the functions they cover")
    ap.add_argument("--keep-going", action="store_true", help="collect every failing case of a spec")
    ap.add_argument("--no-selftest", action="store_true", help="skip the quick eeemu self-test")
    ap.add_argument("--reach", action="store_true",
                    help="list the functions a spec names that no case executed (retail side)")
    ap.add_argument("--show", action="store_true",
                    help="print every event of each case run on the retail side (use with --case): what a "
                         "spec exercises - calls, stubs, kernel calls, hardware accesses, DMA snapshots")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="spec notes; with --keep-going every failing case; with --list the uncovered functions")
    a = ap.parse_args(argv)
    try:
        sys.stdout.reconfigure(line_buffering=True)
    except AttributeError:
        pass

    specmod = _load_specs()
    for g in a.group:
        if g not in specmod.GROUPS:
            print("difftest: no group %r; groups: %s" % (g, ", ".join(specmod.GROUPS)))
            return 2
    groups = a.group or list(specmod.GROUPS)
    specs = [sp for g in groups for sp in specmod.GROUPS[g].SPECS]
    if a.list:
        _list(specmod, groups, Names(), a.verbose)
        return 0
    if not specs:
        print("difftest: group(s) %s have no specs yet" % ", ".join(groups))
        return 0

    elf = a.elf or DEFAULT_ELF
    if not os.path.exists(elf):
        print("difftest: %s not found.  Build it (python tools/build.py --compiler sce) or pass --elf, e.g.\n"
              "          --elf %s" % (elf, os.path.relpath(MATCHING_ELF, os.getcwd())))
        return 2
    if not a.no_selftest:
        import eeemu_selftest
        lines = []
        if eeemu_selftest.main(["--quick"], log=lines.append):
            print("\n".join(lines))
            print("difftest: the eeemu self-test failed; results would not mean anything")
            return 1
        print("eeemu self-test: PASS (tools/eeemu_selftest.py --quick)")

    h = Harness(elf, a.verbose)
    h.show = a.show
    sel = select(specs, a.fn, h.names)
    print("retail  %s  (gp %08X)" % (os.path.relpath(RETAIL_PATH, os.getcwd()), RETAIL_GP))
    print("rebuilt %s  (gp %08X)%s" % (os.path.relpath(elf, os.getcwd()), h.rebuilt.cpu.gp,
                                        "  - byte-identical image" if h.map.identity else ""))
    rb = h.rebuilt
    if h.adapt:
        print("note: n32 image (%s%s%s): calls, stubs and destructors are adapted per side's ABI\n"
              "      (MATCHING.md \"The GCC 14 build\"); functions GCC did not emit are skipped, not MISSING" % (
                  rb.banner or "no .comment", ", Itanium C++" if rb.itanium else "",
                  ", IPA-RA: same-unit stubs keep caller-saved registers%s" % (
                      "" if rb.units else " (no link map: every stub)") if rb.ipa_ra else ""))
    covered = sorted({f for s in sel for f in s.funcs})
    if h.rebuilt.unhooked:
        # an allocator / TTY function the rebuilt image has under no known name
        # would run unstubbed there and show up as a call difference everywhere
        print("HARNESS: base stubs not installed in the rebuilt image:")
        for x in h.rebuilt.unhooked:
            print("  " + x)
        return 1
    if h.map.identity:
        bad = h.check_identity_mapping(covered + [x[0] for x in HEAP_FUNCS] + [SCEPRINTF])
        if bad:
            print("HARNESS: symbol mapping is wrong on a byte-identical image:")
            for b in bad[:20]:
                print("  " + b)
            return 1
    else:
        # two retail functions must not resolve to one rebuilt function (a wrong
        # name would silently stub or label the wrong code)
        seen = collections.defaultdict(list)
        for f in sorted(set(covered + [x[0] for x in HEAP_FUNCS] + [SCEPRINTF])):
            try:
                seen[h.rebuilt.func(f)].append(f)
            except HarnessError:
                pass
        clash = {k: v for k, v in seen.items() if len(v) > 1}
        if clash:
            print("HARNESS: retail functions resolve to the same rebuilt function:")
            for k, v in sorted(clash.items()):
                print("  %08X <- %s" % (k, ", ".join("%s 0x%08X" % (h.names.display(a), a) for a in v)))
            return 1
    only = {k - 1 for k in a.case} if a.case else None
    total_fail = 0
    total_skip = [0]
    t_all = time.time()
    for spec in sel:
        n = max(1, int(round(spec.n * a.n / float(DEFAULT_N))))
        missing, absent = [], []
        for f in spec.funcs:
            try:
                h.rebuilt.func(f)
            except NotEmitted as e:
                absent.append(str(e))                # its cases are skipped, the others run
            except HarnessError as e:
                missing.append(str(e))
        if missing:
            total_fail += 1
            print("MISSING %-26s %s" % (spec.name, missing[0]))
            continue
        if a.reach:
            h.reach, h.reached = set(spec.funcs), set()
        try:
            fails, dt, steps = h.run_spec(spec, n, a.seed, a.keep_going, only)
        except HarnessError as e:
            total_fail += 1
            print("ERROR   %-26s harness: %s" % (spec.name, e))
            continue
        except Exception:
            total_fail += 1
            print("ERROR   %-26s harness exception:" % spec.name)
            traceback.print_exc()
            continue
        nrun = n if only is None else len([k for k in only if k < n])
        skipped = h.skipped
        nrun -= len(skipped)
        total_skip[0] += bool(skipped)
        if skipped and not nrun and not fails:
            print("SKIP    %-26s all %d cases use a function the rebuilt image does not have: %s" % (
                spec.name, len(skipped), skipped[0][1]))
        elif fails:
            total_fail += 1
            i, case, d = fails[0]
            print("FAIL    %-26s case %d/%d: %s" % (spec.name, i + 1, n, case.desc))
            for x in d:
                print("    " + x)
            if case.detail:
                print("    inputs: %s" % case.detail())
            if len(fails) > 1:
                print("    (%d failing cases of %d)" % (len(fails), nrun))
                if a.verbose:
                    for i, case, d in fails[1:]:
                        print("      case %d: %s: %s" % (i + 1, case.desc[:100], d[0].split("\n")[0][:120]))
        else:
            print("PASS    %-26s %4d cases  %6.1fs  %s insns/side%s" % (
                spec.name, nrun, dt, _si(max(steps)), ("  " + spec.notes) if a.verbose and spec.notes else ""))
        if skipped and nrun:
            whys = sorted({w for _, w in skipped})
            print("        %d case(s) skipped: %s%s" % (len(skipped), whys[0],
                                                       " (+%d more functions)" % (len(whys) - 1) if len(whys) > 1
                                                       else ""))
            if a.verbose:
                for w in whys[1:]:
                    print("          also: %s" % w)
        if a.reach:
            missed = [f for f in spec.funcs if f not in h.reached]
            print("        reached %d of %d named functions%s" % (
                len(spec.funcs) - len(missed), len(spec.funcs),
                "; never executed: " + ", ".join("%s 0x%08X" % (h.names.display(f), f) for f in missed)
                if missed else ""))
    print("%d spec(s), %d failed, %d functions covered%s, %.0fs" % (
        len(sel), total_fail, len(covered),
        ", %d with cases skipped (functions not emitted)" % total_skip[0] if total_skip[0] else "",
        time.time() - t_all))
    return 1 if total_fail else 0


def _si(n):
    for u, d in (("G", 1e9), ("M", 1e6), ("k", 1e3)):
        if n >= d:
            return "%.1f%s" % (n / d, u)
    return str(n)


if __name__ == "__main__":
    sys.exit(main())
