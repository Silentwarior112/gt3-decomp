"""Just enough of the GNU v2 (egcs / gcc 2.9x) C++ name mangling to turn a
symbol into its qualified-name parts, for mapping objects to FUNC markers.

    parts("__6BufferUi")              -> ['Buffer', 'Buffer']        (constructor)
    parts("_$_6Buffer")               -> ['Buffer', '~Buffer']       (destructor)
    parts("resize__6BufferUi")        -> ['Buffer', 'resize']
    parts("__as__6BufferRC6Buffer")   -> ['Buffer', 'operator=']
    parts("load__11ImageLoader")      -> ['ImageLoader', 'load']
    parts("what__C9exception")        -> ['exception', 'what']
    parts("__tfQ211ImageLoader6Source") -> ['ImageLoader', 'Source', '__tf']
    parts("sha512_final__FP9SHA512CtxPUx") -> ['sha512_final']
    parts("main")                     -> ['main']
Template arguments are skipped (only the template's name is kept).
"""
import re

OPS = {"as": "operator=", "nw": "operator new", "dl": "operator delete", "vn": "operator new[]",
       "vd": "operator delete[]", "eq": "operator==", "ne": "operator!=", "lt": "operator<",
       "gt": "operator>", "le": "operator<=", "ge": "operator>=", "pl": "operator+", "mi": "operator-",
       "ml": "operator*", "dv": "operator/", "md": "operator%", "apl": "operator+=", "ami": "operator-=",
       "amu": "operator*=", "adv": "operator/=", "amd": "operator%=", "ls": "operator<<", "rs": "operator>>",
       "als": "operator<<=", "ars": "operator>>=", "vc": "operator[]", "cl": "operator()", "rf": "operator->",
       "aa": "operator&&", "oo": "operator||", "nt": "operator!", "co": "operator~", "ad": "operator&",
       "or": "operator|", "er": "operator^", "aad": "operator&=", "aor": "operator|=", "aer": "operator^=",
       "pp": "operator++", "mm": "operator--"}

def _num(s, i):
    j = i
    while j < len(s) and s[j].isdigit():
        j += 1
    return (int(s[i:j]) if j > i else None), j

def _one(s, i):
    """One class component at s[i:] -> (name, next index) or (None, i)."""
    if i < len(s) and s[i] == "t":                      # template: t<len><name><n><args...>
        n, j = _num(s, i + 1)
        if n is None:
            return None, i
        return s[j:j + n], len(s)                        # stop parsing after a template
    n, j = _num(s, i)
    if n is None:
        return None, i
    return s[j:j + n], j + n

def _class(s, i):
    """Class name (possibly Q-qualified) at s[i:] -> ([parts], next index)."""
    if i < len(s) and s[i] == "Q":
        if s[i + 1] == "_":
            cnt, j = _num(s, i + 2)
            j += 1
        else:
            cnt, j = int(s[i + 1]), i + 2
        out = []
        for _ in range(cnt):
            nm, j = _one(s, j)
            if nm is None:
                break
            out.append(nm)
        return out, j
    nm, j = _one(s, i)
    return ([nm] if nm else []), j

def parts(sym):
    if sym.startswith("__tf"):
        c, _ = _class(sym, 4)
        return c + ["__tf"]
    if sym.startswith("_$_"):
        c, _ = _class(sym, 3)
        return c + ["~" + c[-1]] if c else [sym]
    m = re.match(r"^__([a-z]{2,3})__(.*)$", sym)
    if m and m.group(1) in OPS:
        rest = m.group(2)
        if rest.startswith("F"):
            return [OPS[m.group(1)]]
        c, _ = _class(rest, 1 if rest.startswith("C") else 0)
        return c + [OPS[m.group(1)]]
    if sym.startswith("__") and len(sym) > 2 and (sym[2].isdigit() or sym[2] in "Qt"):
        c, _ = _class(sym, 2)
        return c + [c[-1]] if c else [sym]
    k = sym.find("__", 1)
    if k <= 0:
        return [sym]
    name, rest = sym[:k], sym[k + 2:]
    if not rest or rest.startswith("F"):
        return [name]
    c, _ = _class(rest, 1 if rest.startswith("C") else 0)
    return (c + [name]) if c else [name]

def marker_parts(text):
    """Qualified-name parts of a FUNC marker's name text."""
    t = text.strip()
    m = re.match(r"^(__tf[\w$]+(?:\.+[\w$]+)*)", t)
    if m:
        return parts(m.group(1))
    t = re.sub(r"^(typeinfo function for|non-virtual thunk to)\s+", "", t)
    t = t.split("(", 1)[0].strip()
    t = re.sub(r"<[^<>]*>", "", t)                       # drop template args
    t = t.replace(" ", "")
    ps = [p for p in t.split("::") if p]
    return ps
