# Decompilation rules — GT3 US bootstrap

All paths are relative to `bootstrap/`. These are the rules of the first,
behaviour-only pass that wrote the decompilation; the byte-matching pass that
followed has its own rules in `MATCHING.md`.

## Goal

Readable, **semantically faithful** C / C++ for every function in the bootstrap.
Not a byte-matching decomp (the original compiler, GCC 2.9x-ee, is not
available), but every function must do exactly what the machine code does:
same calls in the same order with the same arguments, same constants, same
struct offsets, same loop bounds, same signedness, same return values, same
side effects on globals. When a reader and the disassembly disagree, the
disassembly wins.

## Layout

```
include/            shared headers (types.h, SDK headers, one header per game module)
src/                Polyphony game code (crt0, main, ImageLoader, crypto, GZIP, GS, pdistd)
sdk/                Sony SDK 2.2.x libraries (libkernl, libcdvd, libpad, libmc, libgraph)
runtime/            GCC 2.9x runtime (libgcc, EH, frame.c, libstdc++ new/tinfo) and libc
data/               binary blobs extracted from the ELF by tools/ scripts
ref/                disassembly, symbol tables, unit listings (generated; read-only)
tools/              generators and checkers
```

Only write the files your unit owns (listed in your task). You may create extra
files **inside the same directory with your unit's file prefix** if a module
genuinely needs splitting. Never edit another unit's `.c/.cpp` files. Shared
SDK headers in `include/` (see below) may be **extended** with small Edit
calls (append declarations; never delete or reorder what is there).

## Function markers (mandatory — checked by tools/check_coverage.py)

Immediately above every function definition, one marker line:

```c
// FUNC 0x010044A8 ImageLoader::verify
```

`FUNC`, then the start address exactly as in `ref/funcs.json` (8 hex digits,
upper-case, `0x01...`), then the name. Exactly one marker per function
address across the whole tree. Every function of your unit needs one —
including 2-instruction stubs, dead code, and compiler-generated functions.

For functions that must not (or cannot) be written as C — compiler-generated
RTTI `__tf*` typeinfo functions, vtable thunks, syscall stubs — still write the
marker, then either a faithful C/asm version or a comment block stating
exactly what the code does, e.g.:

```cpp
// FUNC 0x010006B0 __tfQ211ImageLoadert12MCardSource_2Ui1Ui0 (compiler-generated)
//   typeinfo function for ImageLoader::MCardSource_2<1,0>: on first call builds the
//   __si_type_info at 0x0104DD28 via __si_type_info ctor (0x01019C38) with name
//   "Q211ImageLoadert12MCardSource_2Ui1Ui0" and base __tfQ211ImageLoader11MCardSource.
```

Do not put `FUNC 0x...` at the start of any other comment line.

## Per-function comment

Under the marker, a short doc comment: what it does, callers of note, and
anything non-obvious (original vtable slot, EH landing pads, dead code,
quirks). Keep it brief; the code carries the detail.

## Naming

- Use the names in `ref/symbols.csv` (the mapping pass) as the starting point;
  improve them where your deeper reading shows better, and say so in your
  report (`interface_notes`) so dependent units can follow.
- Sony SDK: real Sony 2.2-era names and signatures (sceCdRead, sceSifCallRpc,
  scePadRead, sceMcOpen, ...). Internal statics: `_sceXxx` / descriptive.
- Polyphony C++: real class/method names where evidenced by RTTI strings or the
  Lupo 2 symbol dump (GT3Hooks' `docs/symbols/lupo2_symbols_demangled.txt`);
  otherwise descriptive.
- Unknown functions: descriptive snake_case; only `F_0100XXXX` if truly unknown.
- Globals: descriptive name + address comment, e.g.
  `static int s_boot_source_count; // @0x01041DC0 (.sdata)`.
  Unknown globals: `g_0104DD28`.

## Types and headers

- `#include "types.h"` for u8..u64, s8..s64, u128, REG32/REG64, UNCACHED.
- Shared SDK / libc headers (created before the decompile units start; you may
  extend them): `include/libc.h`, `include/eekernel.h`, `include/sif.h`,
  `include/sifdev.h`, `include/libcdvd.h`, `include/libpad.h`, `include/libmc.h`,
  `include/libgraph.h`, `include/cxxrt.h`.
- Each game unit owns its own header (see its file list) declaring its classes,
  functions and globals for other units.
- If you need something from a module whose header does not exist yet, declare
  it locally in your `.c/.cpp` with a comment `// TODO(integration): from <module>`.
- Struct layouts: define structs with explicit offsets documented, e.g.
  `u32 size; /* +0x08 */`. Use `static_assert`/`_Static_assert` on sizes you
  derived from `operator new` sizes or array strides.

## C++ specifics (GCC 2.9x → modern g++)

- Write classes naturally with virtual methods. Document the original vtable
  address and slot order in a comment above the class (GCC 2.9x vtables are
  `{s16 delta, s16 index?, pfn}` entries; slot 0 is the `__tf` typeinfo
  function, slot 1 the virtual destructor).
- GCC 2.9x destructors take a hidden `int flags` (bit 0 = delete this). Write a
  normal destructor and note the flag in the comment; where the flag logic is
  explicit in callers, mention it.
- `operator new` is 0x01019988, `operator delete` 0x01019910 (see cxxrt unit).
- Exceptions: the code uses sjlj-style EH (landing pads marked in listings). Write
  `try { } catch (...) { }` where there is a real handler; where landing pads only
  run destructors, normal RAII scope is the faithful representation — note it.

## Hardware / asm

- EE registers by address with a comment naming them (e.g.
  `REG32(0x1000F000) /* INTC_STAT */`, `REG64(0x12001000) /* GS CSR */`,
  D1/D2 CHCR etc.).
- Syscalls: the libkernl unit owns the stubs (`sdk/libkernl/syscalls.S`); every
  other unit calls them as normal C functions via `include/eekernel.h`.
- Use inline asm only where C cannot express the operation (ei/di, sync.l,
  cache ops, lq/sq of 128-bit registers with no C equivalent, mfc0). Say so.

## Data

- Tables (SHA-512 K, CRC32, inflate length/distance bases, GS setup tables,
  jump tables for switches) get real values extracted from the ELF. Use a Python
  script under `tools/` (named after your unit) to extract them if large, e.g.
  read the retail file (`baserom/SCUS_971.02`, through `tools/paths.py`),
  file offset = va - 0x01000000 + 0x1000 (single PT_LOAD).
- Large binary blobs (the embedded `startupa.img` gzip) go to `data/` via a
  `tools/` script and are referenced as `extern const u8 name[];`.
- Switch statements: the listing shows jump tables as `.rodata` words pointing
  into your function — reconstruct them as `switch`.

## Compile check (mandatory)

Every `.c/.cpp/.cc/.S` you write must compile with the bundled EE toolchain:

```sh
sh tools/cc.sh src/your_file.cpp         # run from bootstrap/
```

Zero errors required; warnings are acceptable but read them — a warning about
an implicit declaration or a truncated constant is often a real bug.

## Coverage check (mandatory)

```sh
python tools/check_coverage.py --unit <your unit>
```

must report `missing: 0  duplicated: 0` before you finish.

## Honesty

- Mark uncertain reconstructions with `/* ? */` and a one-line reason.
- Never invent behavior to make code look tidier. If a function has a bug or
  quirk (dead store, off-by-one, ignored sign), reproduce it and comment it.
