"""difftest_specs - the test definitions of tools/difftest.py, one module per group.

Groups (module -> ref/units.json units whose functions it tests):

    sha512      sha512                      gs          gs
    gzip        gzip                        pdistd_io   printf, allocator, filename, io
    gameflow    main, startup, imageloader  bignum      bignum, montgomery
    kernl_sys   kernl_sys                   kernl_rest  kernl_rpc_fio, kernl_rest
    libc        libc                        sonyio      libcdvd, libpad, libmc

A spec belongs to the group that owns the functions it calls (by unit, so e.g.
modpow 0x0100B238, in the gzip unit's src/gzip_montgomery_tail.cpp, is a gzip
spec).  `python tools/difftest.py --list` flags a function outside its group's units.

The contract of a group module (tools/difftest_specs/<group>.py) - every module
in this directory except this file, common.py and names starting with "_" is one:

    UNITS = ("unit", ...)       # the units it tests (for --list coverage)
    SPECS = [Spec(...), ...]    # its specs, in run order; may be empty

and nothing else is looked at.  Spec names must be unique over all groups.
Shared helpers (random data, the BigNum memory model, ...) are in common.py;
`from .common import *` gives a module everything it usually needs.

A spec is Spec(name, funcs, gen, n, notes):
  name   short name; `--fn` matches it (or any covered function's name/address)
  funcs  retail addresses of the functions the spec calls directly (they must
         belong to the group's units; --list -v shows the uncovered ones)
  gen    gen(rng, i, n) -> Case(desc, run[, detail]).  ALL randomness is drawn in
         gen (rng is a random.Random seeded from the spec name and --seed); run(t)
         replays the same inputs once per side through the T API of difftest.py
         and must not touch rng (or any other state shared between the sides:
         a server model object with a reset() method is reset per side by Iop)
  n      cases at the default --n 200 (heavy specs use fewer; --n scales them all)
  notes  one line for --list

The T API (tools/difftest.py class T):
  inputs     t.put(bytes, align, offset) / t.cstr(s) / t.alloc(n) - input memory
             (0x01100000.., same address on both sides); t.halloc(n, data) - a
             block of the stub heap (the code may free it); t.write / t.w8..t.w64
  calls      t.call(addr, *args, ret='int'|'void'|'ptr'|'u64'|'u32'|'u16'|'s16'|'u8'|
             's8'|'f0', maxsteps=, floats=(bits, ...), label=)  - integer args in
             order, float args as 32-bit patterns in order (EABI: $a0..$t3 then
             8-byte stack slots, $f12..; against the GCC 14 build placed by the
             prototype per side, see MATCHING.md "The GCC 14 build");
             t.interrupt(addr, *args) - run a handler NOW from inside a stub /
             syscall hook, on a lower stack, all registers restored
  results    t.read / t.r8..t.r64 / t.cstr_at; t.peek / t.peek32 - like read but
             None for stack bytes nothing wrote (safe to compare); t.obs(label,
             value[, 'ptr' | 'ptrs']) - a value to compare ('ptrs': ints inside
             nested tuples compare like pointers); t.written(lo, hi)
  callees    t.stub(addr, pyfn(t, args), nargs, name, log=) - replace a function
             (each call logged with its first `log` args; t.float_args() inside
             gives its float arguments); t.stub_reaches(caller, callee) - False
             when the GCC 14 build inlined callee there (a stub would not see
             it); t.callback(pyfn) - a fake code address for a function pointer
             / vtable slot; t.syscalls(pyfn(t, num, args), nargs) - the kernel
             (use common.Kernel); t.native(addr) - run a base-stubbed function
             for real
  C++ objects (GNU v2 layout in the spec, the Itanium one on the GCC 14 build;
             layouts in common.LAYOUT): t.obj(this, layout) - register an
             object (also one the code made); t.m(this, off) - its member at a
             GNU v2 offset; t.objwrite / t.objread / t.objwords / t.objput /
             t.halloc_obj; t.place / t.alloc_place - members at the same address
             on both sides; t.vtable([slot1, slot2, ...]) - a vtable in this
             side's ABI (slot = fn or (fn, delta)); t.alias(value) - values
             paired by order compare equal; t.unwritten_static(name) - skip the
             case against GCC 14 (a never-written static it folds)
  symbols    t.fn(addr) - this side's address of a function (vtables, handler
             pointers); t.data(name) - of an annotated global (`// @0x...`);
             t.sym(name) - a linker-script symbol (_fbss, _end, _gp, _args ...)
  hardware   t.reg(key, read=, write=) - model an MMIO register (any mirror of its
             address), a COP0 register ('Count', 'Status', 'PCR0' ...) or a VU0
             control register ('vi29'): read = int / list (successive reads, the
             last repeats) / fn(t, n, latched); write = fn(t, value, width);
             t.cop0(name[, value]) / t.vf(i[, xyzw]) / t.vi(i[, value]) - state
             without events; t.cache_tags(fn(t, op, addr)) - the TagLo a CACHE
             index-load-tag returns; t.hw_events() - the hardware events so far
  oracles    t.check(cond, msg) - applied to the retail run only;
             t.unchecked(addr, n) - bytes whose value is indeterminate in the
             original (copied from uninitialised stack): not compared

Everything the code writes outside the stack, every return value declared with
ret=..., every stubbed call and kernel call, every hardware event (MMIO load /
store, DMA start snapshot, COP0 / COP2 / CACHE operation, in program order with
the calls) and every t.obs is compared between the two sides.  The t.check
oracles (Python models: hashlib, zlib, big integers, C-library semantics, quirks
included) run on the retail side only; they validate the harness and the
generators, not the rebuilt code.

Shared models in common.py: random data (rstr, junk, pick_len ...), the BigNum
memory layout (Rep, read_rep), Kernel (a deterministic fake EE kernel: ids,
semaphores, threads, alarms, SIF registers / DMA, Deci2 hooks), Iop + sif_up (a
model IOP behind the real SIF command layer, with per-library server models in
kernl_rest.py / sonyio.py), printf_stub (the pdistd printf with its formatted
text compared) and syscall_stub_spec.  A hardware-facing spec installs a Kernel
(or a gs World, an Iop) in run(t) before its first call, models the registers
the code polls (the defaults are "idle": DMA finished, GS VSINT set, VIF / GIF /
VU stopped) and gives every polling loop an exit.

Inputs are valid unless a case is invalid on purpose (say so in its desc, and
give a runaway a small maxsteps).  Never let a result depend on indeterminate
data - uninitialised stack words, reads past the end of an image table, the
data layout (e.g. how far past _end a loop runs) - since it legitimately differs
between builds; see gzip.py for both rules applied.  On the byte-identical
build every spec must PASS; against the GCC 14 build a case that needs a
function or static GCC did not emit is skipped (difftest.NotEmitted).
"""
import importlib
import os
import pkgutil

GROUP_ORDER = ["libc", "sha512", "bignum", "gzip", "pdistd_io", "gameflow", "gs", "kernl_sys", "kernl_rest",
               "sonyio"]


def _discover():
    here = os.path.dirname(os.path.abspath(__file__))
    found = {}
    for info in pkgutil.iter_modules([here]):
        nm = info.name
        if nm == "common" or nm.startswith("_"):
            continue
        mod = importlib.import_module("%s.%s" % (__name__, nm))
        for attr in ("UNITS", "SPECS"):
            if not hasattr(mod, attr):
                raise ImportError("difftest_specs.%s: no %s (see difftest_specs/__init__.py)" % (nm, attr))
        found[nm] = mod
    order = [g for g in GROUP_ORDER if g in found] + sorted(g for g in found if g not in GROUP_ORDER)
    groups = {g: found[g] for g in order}
    seen = {}
    for g, mod in groups.items():
        for sp in mod.SPECS:
            if sp.name in seen:
                raise ImportError("spec %r is defined in group %s and in group %s" % (sp.name, seen[sp.name], g))
            seen[sp.name] = g
            sp.group = g
    return groups


GROUPS = _discover()                                  # group name -> module
SPECS = [sp for mod in GROUPS.values() for sp in mod.SPECS]
