"""The delay-slot filling of the assembler Sony built the SDK libraries with.

egcs leaves a branch or jump in `.set reorder` mode when its reorg pass could
not fill the delay slot.  The assembler bundled with ee-gcc 2.9-ee-991111b
then always appends a nop.  Standard GAS of that era (tc-mips.c append_insn)
instead moves the preceding instruction into the slot when that is safe, and
the retail SDK code (libkernl, libcdvd, ...) shows exactly that.  Polyphony's
own code does not: e.g. gfx_reset_paths is retail `sw; jr; nop`.  So the game
and the SDK libraries were assembled by different assemblers, and this pass
applies to the files whose ref/compile_flags.json rule says
"asm_pass": "gas_swap" (sdk/*).

The rule, reverse-engineered from every sdk/ function and matching GAS 2.9:
  - the previous instruction must have been assembled in reorder mode, and the
    one before it too (GAS's prev_prev_insn_unreordered); a label forgets that
    history, and a label on the branch itself forbids the swap;
  - the previous instruction must be a single machine instruction, not a
    branch, jump, trap, sync, COP0/COP1 move or compare - except a store to an
    absolute address (`sw $2,sym` under -G0, or a constant beyond 16 bits: the
    macro `lui $at,%hi(a); sw $2,%lo(a)($at)`), whose final `sw` GAS moves like
    any other instruction (wait_vblank_start: retail `lui $at; jr $31; sw`);
  - it must not write a register the branch reads, nor read or write $31 when
    the branch links (jal / jalr / bltzal / bgezal);
  - loads are never moved (never seen in the SDK code: GAS's load-delay
    bookkeeping keeps them out);
  - branch-likely instructions never take a swapped instruction (their slot
    runs only when the branch is taken, so GAS never fills it this way).
Every swap keeps the program's semantics: the moved instruction executes in
the delay slot, i.e. before the branch target, and nothing between them reads
what it writes.

Evidence: with this pass, sio_putc, _dtoi, DIntr, wait_vblank_start and
_sceCdCallbackThread become exact and scePrintf exact with data, and no
function of sdk/ gets worse (sdk/: 233 -> 238 exact of 243); applied to src/
it would break 9 exact functions.

    python tools/sdkas.py file.s     # rewrite an egcs assembly file in place
"""
import re
import sys

REG = re.compile(r"\$(\d+|sp|fp|ra|gp|at)\b")
ALIAS = {"sp": "29", "fp": "30", "ra": "31", "gp": "28", "at": "1"}

BRANCH = re.compile(r"^(b|beq|bne|beqz|bnez|blez|bgtz|bltz|bgez|beql|bnel|blezl|bgtzl|bltzl|bgezl|"
                    r"bltzal|bgezal|j|jr|jal|jalr|bc1t|bc1f|bc1tl|bc1fl|bc0t|bc0f|bc2t|bc2f)$")
LIKELY = {"beql", "bnel", "blezl", "bgtzl", "bltzl", "bgezl", "bc1tl", "bc1fl"}
LINKS = {"jal", "jalr", "bltzal", "bgezal"}
NOSWAP_PREV = {"la", "dla", "div", "divu", "ddiv", "ddivu", "rem", "remu", "drem", "dremu", "mul", "dmul",
               "sync", "sync.l", "sync.p", "nop", "syscall", "break", "di", "ei", "eret",
               "teq", "tne", "tge", "tlt", "mfc0", "mtc0", "ctc1", "cfc1", "ulw", "usw", "uld", "usd",
               "cache", "mtc1", "mfc1", "li.s", "li.d"}
LOADS = {"lw", "ld", "lb", "lbu", "lh", "lhu", "lwu", "lq", "lwl", "lwr", "ldl", "ldr", "lwc1", "l.s"}
STORES = {"sw", "sd", "sb", "sh", "sq", "swl", "swr", "sdl", "sdr", "swc1", "s.s"}
HILO_WRITERS = {"mult", "multu", "dmult", "dmultu", "mult1", "multu1", "div1", "divu1",
                "madd", "maddu", "madd1", "maddu1", "mthi", "mtlo", "mthi1", "mtlo1"}


def _regs(ops):
    return [ALIAS.get(r, r) for r in REG.findall(ops)]


def _num(x):
    try:
        return int(x, 0)
    except ValueError:
        return None


def _single_insn(tok, ops):
    """True when the line assembles to exactly one instruction."""
    if tok in ("li", "dli"):
        v = _num(ops.split(",", 1)[1].strip()) if "," in ops else None
        if v is None:
            return False
        v &= 0xFFFFFFFFFFFFFFFF
        return v < 0x10000 or v >= 0xFFFFFFFFFFFF8000 or \
            (tok == "li" and (v & 0xFFFF) == 0 and v < 0x100000000)
    if tok in LOADS or tok in STORES:
        parts = [x.strip() for x in ops.split(",", 1)]
        return len(parts) == 2 and "(" in parts[1]
    return True


def _symbolic_store(tok, ops):
    """A store to an absolute address with no base register - a symbol under
    -G0, or a constant outside the signed 16-bit range: the two-instruction
    macro `lui $at,%hi(a); sw $r,%lo(a)($at)`."""
    if tok not in STORES or tok in ("swc1", "s.s"):
        return False
    parts = [x.strip() for x in ops.split(",", 1)]
    if len(parts) != 2 or "(" in parts[1]:
        return False
    v = _num(parts[1])
    return v is None or not -0x8000 <= v < 0x8000


def _hi_lo(addr):
    """%hi / %lo operands for an absolute address (symbolic or numeric)."""
    v = _num(addr)
    if v is None:
        return "%%hi(%s)" % addr, "%%lo(%s)" % addr
    v &= 0xFFFFFFFF
    hi = ((v + 0x8000) >> 16) & 0xFFFF
    lo = v - (hi << 16)
    lo = ((lo + 0x8000) & 0xFFFF) - 0x8000
    return "0x%X" % hi, "%d" % lo


def _dests(tok, ops):
    r = _regs(ops)
    if not r or tok in STORES:
        return set()
    if tok in HILO_WRITERS:
        return {"HILO"} | ({r[0]} if len(r) == 3 else set())
    return {r[0]}


def _srcs(tok, ops):
    r = _regs(ops)
    if tok in STORES:
        return set(r)
    if tok in ("mfhi", "mflo", "mfhi1", "mflo1"):
        return {"HILO"}
    return set(r[1:])


def gas_swap(text):
    """Apply the swap to egcs assembly text; returns (new text, swaps made)."""
    out = []
    noreorder = False
    stack = []
    prev = None                     # (out index, tok, ops) of a swappable previous insn
    cur_unreordered = False         # the most recent insn was assembled in noreorder mode
    prev_prev_unreordered = False
    label_pending = False
    n = 0
    for line in text.split("\n"):
        s = line.split("#")[0].strip()
        if not s or s.startswith((".loc", ".stabn", ".stabs", ".stabd")):
            out.append(line)
            continue
        if s.startswith(".set"):
            w = s.split()[1:2]
            if w == ["noreorder"]:
                noreorder = True
            elif w == ["reorder"]:
                noreorder = False
            elif w == ["push"]:
                stack.append(noreorder)
            elif w == ["pop"] and stack:
                noreorder = stack.pop()
            out.append(line)
            continue
        if s.endswith(":") or re.match(r"^[\w$.]+:", s):
            label_pending = True
            cur_unreordered = False
            out.append(line)
            continue
        if s.startswith("."):
            prev = None
            out.append(line)
            continue
        tok = s.split()[0]
        ops = s[len(tok):].strip()
        if noreorder:
            prev = None
            cur_unreordered = True
            label_pending = False
            out.append(line)
            continue
        if BRANCH.match(tok):
            ok = prev is not None and not label_pending and not prev_prev_unreordered and tok not in LIKELY
            if ok:
                pi, ptok, pops = prev
                pd, ps, bs = _dests(ptok, pops), _srcs(ptok, pops), set(_regs(ops))
                if _symbolic_store(ptok, pops):
                    ps = ps | {"1"}                  # the expansion's %lo(sym)($at)
                if tok in LINKS and ("31" in pd or "31" in ps):
                    ok = False
                if pd & bs or ptok in LOADS or "1" in bs:
                    ok = False
            if ok:
                pline = out[pi]
                if _symbolic_store(ptok, pops):
                    # GAS moves the macro's final store; its lui stays in place
                    reg, addr = [x.strip() for x in pops.split(",", 1)]
                    hi, lo = _hi_lo(addr)
                    out[pi] = "\t.set\tnoat\n\tlui\t$1,%s" % hi
                    pline = "\t%s\t%s,%s($1)\n\t.set\tat" % (ptok, reg, lo)
                else:
                    out[pi] = None
                out += ["\t.set\tnoreorder", line, pline, "\t.set\treorder"]
                n += 1
            else:
                out.append(line)
            prev = None
            cur_unreordered = False
            label_pending = False
            continue
        prev_prev_unreordered = cur_unreordered
        label_pending = False
        cur_unreordered = False
        out.append(line)
        if tok in NOSWAP_PREV or tok.startswith(("c.", "b", "j")) or \
                not (_single_insn(tok, ops) or _symbolic_store(tok, ops)):
            prev = None
        else:
            prev = (len(out) - 1, tok, ops)
    return "\n".join(x for x in out if x is not None), n


def apply(path):
    text = open(path, encoding="latin1").read()
    new, n = gas_swap(text)
    if n:
        open(path, "w", encoding="latin1", newline="\n").write(new)
    return n


if __name__ == "__main__":
    for p in sys.argv[1:]:
        print("%s: %d swap(s)" % (p, apply(p)))
