"""Which compiled C symbol is which original function.

Every function the decomp writes follows a `// FUNC 0x00XXXXXX name` marker.
The compiler tells where each function it emitted starts (file and line): egcs
in the stabs of its -g assembly output (N_FUN stabs carry the line, N_SOL
stabs the file). A function belongs to the marker before it when no other
function starts between the two; helpers without a marker get no address.

    python tools/funcmap.py <build dir>     summarise <build>/funcmap.json

Importable: markers(), stab_functions(asm_text), build(entries) -> (map, notes).
"""
import collections
import json
import os
import re
import sys

import paths

ROOT = paths.ROOT
# the whole name, `*' included (an overload's `setImage(const char *)': the
# u056 agent), as score.py's; a block comment's closing `*/' is not part of it
MARK = re.compile(r"^\s*(?://|/\*)\s*FUNC\s+0x(00[0-9A-Fa-f]{6})\b\s*(.*?)\s*(?:\*/.*)?$")
N_SOL = re.compile(r'^\.stabs\s+"((?:[^"\\]|\\.)*)",132,')
N_FUN = re.compile(r'^\.stabs\s+"((?:[^"\\]|\\.)*?):([Ff])[^"]*",36,0,(\d+),([^\s]+)')


def rel(path):
    """A source path as the compiler names it, relative to core/ with '/'."""
    p = path.replace("\\\\", "/").replace("\\", "/")
    if os.path.isabs(p) or re.match(r"^[A-Za-z]:/", p):
        try:
            p = os.path.relpath(p, ROOT).replace("\\", "/")
        except ValueError:
            pass
    while p.startswith("./"):
        p = p[2:]
    return p


def markers():
    """{file: [(line, address, name)]} of every FUNC marker under src/ and include/."""
    out = {}
    for top in ("src", "include"):
        for dp, _, fns in os.walk(os.path.join(ROOT, top)):
            for fn in fns:
                if not fn.endswith((".c", ".cpp", ".cc", ".h", ".hpp", ".inc", ".S", ".s")):
                    continue
                p = os.path.join(dp, fn)
                found = []
                for n, line in enumerate(open(p, encoding="utf-8", errors="replace"), 1):
                    m = MARK.match(line)
                    if m:
                        found.append((n, int(m.group(1), 16), m.group(2).strip()))
                if found:
                    out[rel(p)] = found
    # in address order (each file's first marker), not the tree's: callers
    # that meet a name in several files (a header's static helper copied into
    # each object) then decide the same way wherever the files live
    return dict(sorted(out.items(), key=lambda kv: (min(a for _, a, _ in kv[1]), kv[0])))


def stab_functions(asm_text):
    """[(symbol, file, line)] of the functions in one egcs -g assembly output."""
    cur, out = None, []
    for line in asm_text.splitlines():
        if not line.startswith(".stabs"):
            continue
        m = N_SOL.match(line)
        if m:
            cur = rel(m.group(1))
            continue
        m = N_FUN.match(line)
        if m and cur:
            out.append((m.group(4), cur, int(m.group(3))))
    return out


BUILTIN = {"i": "int", "c": "char", "s": "short", "l": "long", "f": "float", "d": "double", "b": "bool",
           "v": "void", "x": "long long", "w": "wchar_t", "Ui": "unsigned int", "Uc": "unsigned char",
           "Us": "unsigned short", "Ul": "unsigned long", "Sc": "signed char"}
OPERATORS = {"as": "operator=", "eq": "operator==", "ne": "operator!=", "nw": "operator new",
             "dl": "operator delete", "vn": "operator new []", "vd": "operator delete []",
             "vc": "operator[]", "cl": "operator()", "pl": "operator+", "mi": "operator-",
             "ml": "operator*", "dv": "operator/", "lt": "operator<", "gt": "operator>",
             "le": "operator<=", "ge": "operator>=", "apl": "operator+=", "ami": "operator-=",
             "aml": "operator*=", "adv": "operator/="}


def _name(s, i):
    """(name, next index) of <len><name> at s[i], or None."""
    m = re.match(r"\d+", s[i:])
    if not m:
        return None
    n = int(m.group(0))
    j = i + len(m.group(0))
    return (s[j:j + n], j + n) if len(s) >= j + n else None


def _type(s, i):
    """(type, next index) of a type encoding used as a template argument."""
    for k in sorted(BUILTIN, key=len, reverse=True):
        if s.startswith(k, i):
            return BUILTIN[k], i + len(k)
    return _class(s, i)


def _component(s, i):
    """One scope component: <len><name> or a template t<len><name><n>Z<arg>..."""
    if s.startswith("t", i):
        r = _name(s, i + 1)
        if not r:
            return None
        nm, j = r
        m = re.match(r"\d", s[j:])
        if not m:
            return None
        args, j = [], j + 1
        for _ in range(int(m.group(0))):
            if not s.startswith("Z", j):
                # a value argument: its type, then the value (m: negative)
                v = re.match(r"(?:Ui|Us|Uc|Ul|i|s|c|l|b)(m?)(\d+)", s[j:])
                if not v:
                    return None
                args.append(("-" if v.group(1) else "") + v.group(2))
                j += len(v.group(0))
                continue
            r = _type(s, j + 1)
            if not r:
                return None
            args.append(r[0])
            j = r[1]
        return "%s<%s>" % (nm, ",".join(args)), j
    return _name(s, i)


def _class(s, i):
    """(qualified class name, next index) of a class encoding at s[i]."""
    if s.startswith("Q", i):
        if s.startswith("Q_", i):
            m = re.match(r"Q_(\d+)_", s[i:])
            if not m:
                return None
            n, j = int(m.group(1)), i + len(m.group(0))
        else:
            if i + 1 >= len(s) or not s[i + 1].isdigit():
                return None
            n, j = int(s[i + 1]), i + 2
        parts = []
        for _ in range(n):
            r = _component(s, j)
            if not r:
                return None
            parts.append(r[0])
            j = r[1]
        return "::".join(parts), j
    return _component(s, i)


def _base(cls):
    """The last component of a qualified class name without its template
    arguments ('List' of 'gt2k_menu::List<gt2k_menu::Item>')."""
    depth, last = 0, 0
    for i, ch in enumerate(cls):
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth -= 1
        elif depth == 0 and cls.startswith("::", i):
            last = i + 2
    return cls[last:].split("<")[0]


def demangle(sym):
    """'Class::method' / 'Class::Class' / 'Class::~Class' / 'function' of an
    egcs (GNU v2) symbol, without the parameters; None when not understood."""
    if sym.startswith("_$_"):
        r = _class(sym, 3)
        return "%s::~%s" % (r[0], _base(r[0])) if r else None
    m = re.match(r"^__([a-z]{2,3})__(C?)(.*)$", sym)
    if m and m.group(1) in OPERATORS:
        r = _class(m.group(3), 0)
        return "%s::%s" % (r[0], OPERATORS[m.group(1)]) if r else OPERATORS[m.group(1)]
    if sym.startswith("__") and len(sym) > 2 and (sym[2].isdigit() or sym[2] in "Qt"):
        r = _class(sym, 2)
        return "%s::%s" % (r[0], _base(r[0])) if r else None
    k = sym.find("__", 1)
    while k > 0:
        rest = sym[k + 2:]
        if rest.startswith("F"):
            return sym[:k]
        r = _class(rest[1:] if rest.startswith("C") else rest, 0)
        if r:
            return "%s::%s" % (r[0], sym[:k])
        k = sym.find("__", k + 1)
    return None


def _marker_key(name):
    return re.sub(r"\s+", "", name.split("(")[0])


def by_name(marks=None):
    """{name: address} of the markers whose name is a symbol name as written
    (labels of .S files, typeinfo functions __tf..., C names): no '::', no spaces."""
    marks = markers() if marks is None else marks
    out = {}
    for ms in marks.values():
        for _, addr, name in ms:
            nm = name.split()[0] if name else ""
            if nm and "::" not in nm and re.match(r"^[A-Za-z_$.][\w$.]*$", nm):
                out.setdefault(nm, addr)
    return out


def build(entries, marks=None, defined=None):
    """entries: [(symbol, file, line)] from all objects; defined: the symbols
    the objects define.  Returns ({symbol: address}, notes): the symbol of the
    first function after each marker, and markers named after a defined symbol."""
    marks = markers() if marks is None else marks
    starts = collections.defaultdict(set)              # file -> lines where functions start
    at = collections.defaultdict(set)                  # (file, line) -> symbols
    for sym, f, line in entries:
        starts[f].add(line)
        at[(f, line)].add(sym)
    result, notes = {}, []
    # first by name, whatever the lines: a marker that names one of its file's
    # symbols ("T077_static_init (_GLOBAL_$I$icon_model_size__FPCUi)"), and the
    # file's static constructor / destructor for its <TU>_static_init / _fini
    # marker.  egcs gives _GLOBAL_$I$ the line of the first object it
    # constructs and _GLOBAL_$D$ the end of the file: both can sit on the
    # wrong side of their markers (T077's destructor took the init address).
    pinned = set()
    # a mangled name in a marker is unique: it may name an inline copy whose
    # stab line is in a header (u072's setColor__Q29gt2k_menu8DrawablePC7Color4fi)
    every = {s for ss in at.values() for s in ss} | set(defined or ())
    for f, ms in marks.items():
        syms = {s for (ff, _), ss in at.items() if ff == f for s in ss}
        for kind, prefix in (("_static_init", "_GLOBAL_$I$"), ("_static_fini", "_GLOBAL_$D$")):
            km = [addr for _, addr, name in ms if _marker_key(name).endswith(kind)]
            ks = [s for s in syms if s.startswith(prefix)]
            if len(km) == 1 and len(ks) == 1:
                result[ks[0]] = km[0]
                pinned.add(km[0])
        for _, addr, name in ms:
            # any token before the first '(', only mangled-looking ones inside
            # it (a comment's plain words are not the marker's symbol)
            head, _, rest = (name or "").partition("(")
            toks = [(t, syms) for t in re.findall(r"[A-Za-z_$.][\w$.]*", head)] + \
                [(t, every) for t in re.findall(r"[A-Za-z_$.][\w$.]*", rest) if "$" in t or "__" in t or t.startswith("_")]
            for tok, pool in toks:
                if tok in pool and tok not in result:
                    result[tok] = addr
                    pinned.add(addr)
    for f, ms in marks.items():
        lines = sorted(starts.get(f, ()))
        for i, (mline, addr, name) in enumerate(ms):
            if addr in pinned:
                continue
            nxt = ms[i + 1][0] if i + 1 < len(ms) else 1 << 30
            between = [L for L in lines if mline < L < nxt]
            if not between:
                continue                                # inline, written elsewhere, or not compiled
            # the function named like the marker; else the first one (egcs puts an
            # implicit destructor it emits at whatever line it is on, too)
            key = _marker_key(name)
            named = [s for L in between for s in at[(f, L)] if (demangle(s) or s).replace(" ", "") == key]
            first = [s for s in at[(f, between[0])] if s not in result or result[s] not in pinned]
            for sym in (named or first):
                if sym in result and result[sym] in pinned:
                    continue
                if sym in result and result[sym] != addr:
                    notes.append("%s: two addresses 0x%08X and 0x%08X" % (sym, result[sym], addr))
                result[sym] = addr
    if defined:
        have = set(result.values())
        for nm, addr in by_name(marks).items():
            if nm in defined and nm not in result and addr not in have:
                result[nm] = addr
        # functions compiled where no marker precedes them (inline copies emitted
        # from a header, implicit constructors and destructors): by their name,
        # when exactly one marker of that name is still without a symbol
        have = set(result.values())
        free = collections.defaultdict(set)
        for ms in marks.values():
            for _, addr, name in ms:
                if addr not in have and name:
                    free[_marker_key(name)].add(addr)
        params = {}                                     # address -> encoded parameters of its marker
        for ms in marks.values():
            for _, addr, name in ms:
                m = re.search(r"\(([^)]*)\)", name or "")
                if m:
                    params[addr] = encode_params(m.group(1))
        for sym in sorted(defined):
            if sym in result:
                continue
            q = demangle(sym)
            cands = free.get(re.sub(r"\s+", "", q)) if q else None
            if cands and len(cands) > 1:                # overloads: by the marker's parameter list
                code = param_code(sym)
                cands_p = {a for a in cands if params.get(a) is not None and params[a] == code}
                cands = cands_p if len(cands_p) == 1 else None
                if cands:
                    free[re.sub(r"\s+", "", q)].discard(next(iter(cands)))
            if cands and len(cands) == 1:
                addr = next(iter(cands))
                result[sym] = addr
                cands.discard(addr)
    # a static constructor run as a destructor (or the other way) breaks the boot
    names = {addr: name for ms in marks.values() for _, addr, name in ms}
    for sym, addr in result.items():
        for mine, other in (("_GLOBAL_$I$", r"_GLOBAL_\$D\$|_static_(fini|dtor|destroy)\b"),
                            ("_GLOBAL_$D$", r"_GLOBAL_\$I\$|_static_(init|ctor)\b")):
            if sym.startswith(mine) and re.search(other, names.get(addr, "")):
                notes.append("%s mapped to 0x%08X, marker '%s'" % (sym, addr, names[addr]))
    return result, notes


PARAM_CODES = {"void": "", "": "", "int": "i", "s32": "i", "unsigned int": "Ui", "u32": "Ui", "float": "f",
               "f32": "f", "double": "d", "char": "c", "short": "s", "s16": "s", "u16": "Us", "u8": "Uc",
               "unsigned char": "Uc", "bool": "b", "long": "l"}


def encode_params(text):
    """egcs's encoding of a parameter list of builtin types ('int, float' -> 'if');
    None when a parameter is not a plain builtin."""
    out = []
    for p in [x.strip() for x in text.split(",")] if text.strip() else []:
        p = re.sub(r"\s+\w+$", "", p) if p not in PARAM_CODES else p     # drop a parameter name
        if p not in PARAM_CODES:
            return None
        out.append(PARAM_CODES[p])
    return "".join(out)


def param_code(sym):
    """The parameter part of an egcs method / constructor symbol (after the class)."""
    if sym.startswith("_$_"):
        return ""
    k = sym.find("__", 1) if not sym.startswith("__") else 0
    rest = sym[k + 2:] if k >= 0 else ""
    if rest.startswith("F"):
        return rest[1:]
    if rest.startswith("C"):
        rest = rest[1:]
    r = _class(rest, 0)
    return rest[r[1]:] if r else None


def main():
    build_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "991111b")
    data = json.load(open(os.path.join(build_dir, "funcmap.json")))
    by_addr = collections.defaultdict(list)
    for s, a in data["map"].items():
        by_addr[a].append(s)
    print("%d symbols for %d addresses; %d notes" % (len(data["map"]), len(by_addr), len(data["notes"])))
    for n in data["notes"][:20]:
        print("  " + n)


if __name__ == "__main__":
    main()
