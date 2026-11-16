# Decompilation rules: GT3 main executable, first pass

Paths are relative to `core/`. Read `ref/CONTEXT.md` first.

## Goal of the first pass

C / C++ source for every function of your unit that **does what the machine
code does**: the same calls in the same order with the same arguments, the
same constants, offsets and access widths, the same conditions (signed vs
unsigned), the same loops, return values and stores. Speed matters more than
polish in this pass: later passes do naming, layout and byte-matching. When
the source and `listing.txt` disagree, the listing wins.

## How to work

1. Read `ref/units/<unit>/index.txt` and `classes.txt`; skim `m2c.c`.
2. For each function, start from its m2c draft and turn it into clean source:
   real types, struct fields instead of `->unk10`, names, `switch` instead of
   jump-table code, loops instead of gotos where obvious.
3. **Check against `listing.txt`** wherever the draft is suspect:
   - `M2C_ERROR`, `unknown instruction`, `Read from unset register`,
     `unable to parse jump table`, missing functions (m2c skips a few);
   - calls with more than 4 arguments (`$a4-$a7`) or float arguments;
   - branch-likely instructions, 64-bit arithmetic (`dsll32`, `dsra32`, `ld`),
     signed vs unsigned compares (`slt`/`sltu`, `bltz`/`bgez`), `lb` vs `lbu`,
     `lh` vs `lhu`, float compares (`c.lt.s` + `bc1f`/`bc1t`);
   - virtual calls (`lw fn, 8k+12(vt)`; slot k in `classes.txt`).
4. Functions listed `BOOT=` in index.txt: copy their source from `boot.txt`
   (it is the same machine code), change the FUNC address to the core one and
   adapt the names to your files.
5. Compile and check coverage (below) until both pass.

## Files you own

Everything under `src/<unit>/`, nothing else. Never edit another unit's
directory, `include/`, `ref/` or `tools/`.

- **One source file per original translation unit** listed in index.txt
  (`T###` = a C++ unit, `G###` = a stretch of no-FDE code). Name it
  `<TU>_<what_it_is>.cpp`, e.g. `T083_race_course.cpp`. If index.txt says the
  translation unit is split across units, add your unit:
  `T036_garage_u011.cpp`. In a `G###` stretch, split the code into the modules
  you recognise (`G004_libpad.c`, `G007_vfprintf.c`).
- **Language.** `.cpp` for C++ (`T###`, and `G###` code that uses classes or
  `this`), `.c` for C, `.S` only for hand-written assembly (crt0, syscall
  stubs).
- **One header**, `src/<unit>/<unit>.h`, included by your files: the
  structs and classes they need, and declarations of everything they use from
  outside the unit.

Shared read-only headers in `include/`: `types.h` (u8..u64, s8..s64, f32,
u128, REG32/REG64, UNCACHED, EXTERN_C), `libc.h`, `eekernel.h`, `sif.h`,
`sifdev.h`, `libcdvd.h`, `libpad.h`, `libmc.h`, `libgraph.h`, `cxxrt.h`,
`pdistd/allocator.h`, `pdistd/printf.h`. If you need something they lack,
declare it in your own header.

## FUNC markers (checked by tools/check_coverage.py)

Directly above every function definition, one line:

```c
// FUNC 0x00101A40 __tf10GameObject
```

`FUNC`, the start address exactly as in index.txt (`0x00` + 6 upper-case hex
digits), then the name you give it. Exactly one marker per function of your
unit, including 2-instruction stubs, dead code, `__tf*` typeinfo functions,
thunks and compiler-generated functions. A function that cannot be C (crt0,
a syscall stub, a function that jumps into another's tail) still gets its
marker, with inline assembly or a faithful C equivalent. Do not start any other
comment line with `FUNC 0x`.

## Names

- Use the names in index.txt and in the m2c.c name table. `Class::virtual_N`
  names may be replaced by a descriptive method name (keep the class).
- Unnamed functions **of your unit**: give a descriptive name when the
  purpose is clear (`RaceCourse::loadSectors`, `menu_draw_frame`), else keep
  `F_00XXXXXX`.
- Functions **outside your unit**: use their known name, or `F_00XXXXXX`.
  Never invent a name for a function you do not own. Put the address in a
  comment on its declaration: `void F_00218660(void); // 0x00218660`.
- Globals: `extern` declarations named `g_00XXXXXX` (or a descriptive name)
  with the address in a comment, e.g.
  `extern int g_003531C8; // gp-0x4828, heap guard`. The first pass defines no
  data: string literals and float constants are written inline, jump tables
  become `switch`.

## Types, classes, structs

- Polymorphic classes your unit owns (`classes.txt`, slot methods marked `*`):
  declare them as C++ classes with their bases and with the virtual methods in
  slot order; comment the vtable address and each slot's offset. Methods are
  real member functions (`this` = `$a0`).
- Classes owned elsewhere that you use: declare them in your header with only
  what you need, marked `// partial: owned by T###`. Virtual calls on them: a
  virtual method declaration with the slot in a comment.
- Data layouts: fields with their offsets in comments, e.g.
  `int count; /* 0x10 */`; unknown space as `u8 pad_14[0xC];`. Size from
  `operator new(size)` when you see it: `static_assert(sizeof(X) == 0x40)`.
- Destructors: write a normal destructor; the egcs `int flags` / `operator
  delete` logic is implied (mention it if the code does more).
- C code: plain structs, functions; `void *` + casts are acceptable where a
  layout is unclear.

## EE specifics

- Inline asm only where C cannot express it: `ei`/`di`, `sync`, `syscall`,
  `mfc0`/`mtc0`, MMI (`pextlb`, `pcpyld`, `paddh`, ...), VU0 macro code
  (`vadd`, `vmul`, `lqc2`, ...), `lq`/`sq` of 128-bit values. Keep the asm small
  and faithful (`__asm__ volatile`).
- `max.s` / `min.s` can be written as `a > b ? a : b`. `madd.s` / accumulator
  sequences as the float arithmetic they compute.
- Hardware registers by address with a comment: `REG32(0x1000F000) /* INTC_STAT */`.

## Checks (mandatory)

```sh
sh tools/cc.sh --unit <unit>                     # every file: OK, 0 errors
python tools/check_coverage.py --unit <unit>     # missing 0, duplicated 0, unknown 0
```

Warnings are allowed, but read them: an implicit declaration or a truncated
constant is often a real mistake.

## Honesty

- `/* ? */` marks a guess (a name, a type, a layout) and a one-line reason.
- Never invent behaviour to make code look tidy. Reproduce quirks, dead
  stores and bugs, with a comment.
- If you could not finish a function properly, still give it its marker and
  a body, and list it in your report.
