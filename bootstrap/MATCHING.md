# Matching the retail bytes

The aim of this phase is C and C++ that compile, with the compiler the retail
file was built with, to the retail machine code: identical where possible,
and at least about 98% of the bytes and 95% of the size overall. Behaviour
must stay 100% the same everywhere, including functions whose bytes are not
yet identical.

All paths are relative to `bootstrap/`. `sdk/` is the repository's SDK folder,
where the GT3 Decomp Builder downloads the compilers (the repository's README,
"Downloads").

## The compiler: SCE ee-gcc 2.9-ee-991111b

`sdk/ee-gcc-2.9-991111b/` holds SCE's egcs-based EE compiler, `gcc version
2.9-ee-991111b/r4`. It is a native Windows build and needs no WSL. The package
is from decomp.me's compiler archive (`ee-gcc2.9-991111.tar.gz`), and its
SHA-256 is in `toolchain/sdks/ee_gcc_2_9_991111b.py`, which downloads it.

**The matching build uses a patched copy, `sdk/ee-gcc-2.9-991111b-divs/`.**
`python tools/patch_divs.py` creates it from the pristine directory.
`tools/mcc.py` stops with that instruction when it is missing. The patch
changes 4 bytes in each of `cc1.exe` and `cc1plus.exe`: `div.s` (insn
`divsf3`, code 97) becomes ineligible for delay slots, as it was in the
retail compiler (Compiler notes, Scheduling). Everything else in the two
executables is unchanged. Scoring all 881 functions with the patched copy
makes `startup_screen_thread` exact and changes no other function.

| File | SHA-256, pristine | SHA-256, patched |
|---|---|---|
| `cc1.exe` | `124f7c6870b7facbaecbc51f0b87c0aad46db5f6ed491c21638e1da991e45be5` | `66cbff0b094abd768c54b2d94d44cc2035b7d53262555083b559ad365a3a2c1b` |
| `cc1plus.exe` | `65b5134f24377544bb2e5d1d2dd3436807cc50ce2ce77d759e4fca1add18f9a4` | `7abb1aaa7003aedc45b90999d778c75edc818f9e7b1dd9eacbf48ae9ccc6c6c0` |

Evidence that this is the retail compiler:

- **Code.** Compiled at `-O2`, the `Buffer` class of `src/imageloader.cpp`
  reproduces the retail instructions word for word: register saves as
  `sd`/`ld` in 16-byte slots, the same delay-slot scheduling, and functions
  aligned to 8 bytes. SN Systems' ee-gcc 2.95.2
  (decomp.me's `ee-gcc2.95.2-273a`, compared for reference) differs: it saves with
  `sq`/`lq` and orders prologues differently.
- **Libraries.** `tools/match_lib.py` finds 178 of the bootstrap's 186 runtime
  functions byte-identical in this toolchain's own `libgcc.a` / `libstdc++.a`
  (libgcc_math 36/36, frame 18/18, cxxrt 75/78, libgcc_eh 49/54). The
  result is in `ref/libmatch.json`.

Its system headers come from gcc-2.95.2 (`gcc/ginclude`) and sit in
`sdk/ee-gcc-2.9-991111b/include`. The driver predefines `__mips_eabi`,
`__mips_single_float`, `__mips=3`, `__R5900__` and `__EXCEPTIONS`.

### The reference compiler: 2.9-ee-991111-01 (secondary)

```sh
pip install unicorn                                   # the only dependency
python tools/score.py --cc 991111-01 [files]          # -> build/score-991111-01/, plus a list of
                                                      #    functions that differ from build/score/
python tools/score.py --cc 991111-01 --diff <addr>
python tools/mcc.py --cc 991111-01 <file> [-S] [-o out]
python tools/build.py --compiler 991111-01            # the whole ELF -> build/991111-01/ (it boots)
```

It does not need the patched copy: without `ee-gcc-2.9-991111b-divs`, the
pristine 991111b package's driver preprocesses and assembles. The GT3 Decomp
Builder offers it as the bootstrap's period-accurate compiler.

`sdk/ee-gcc-2.9-991111-01/` holds Sony's Linux build of the next revision,
`2.9-ee-991111-01`, from decomp.me's archive
(`toolchain/sdks/ee_gcc_2_9_991111_01.py` lists the archives and hashes). It
is most likely the compiler the retail file was built with:

- **`div.s`.** Its `divsf3` template is `%(nop\n\tnop\n\tdiv.s\t%0,%1,%2%)`:
  the hazard padding is built in, and the three-instruction pattern never
  goes in a delay slot. That is exactly what the patched 991111b emulates
  (`patch_divs.py` plus `fix_fpu_div_hazard`). On `startup_screen.cpp` the
  two compilers give byte-identical objects.
- **Register numbering.** The Windows 991111b has 32 more hard registers (the
  VU0 vector registers `$vf0`-`$vf31`), so its pseudo registers start at 116
  instead of 84. GCSE hashes registers by number, so the order of its PRE
  temporaries, and with it their spill slots, differs between the two. Retail's
  `sha512_transform` is one swapped slot pair away under 991111-01 (99.9%) and
  a permutation of eight slots under 991111b (99.6%).

It is the secondary compiler: the default stays the patched Windows build (fast,
native), and 991111-01 confirms source accuracy. Under it, the preprocessor
and assembler are the default toolchain's. Sony's `cc1`/`cc1plus` are i386 Linux
executables, which `tools/linuxcc.py` runs on Windows: Unicorn executes the
code, and the script loads the ELF and the glibc 2.3.6 its package came with
(glibc 2.27 crashes these 2000-era binaries) and answers their Linux system
calls.

Results on all 881 functions (2026-10-03, 57 s for the whole tree):

- 849 exact, as with the default.
- 41 of 43 objects are byte-identical between the two compilers: code, data
  and relocations.
- Only `sha512_transform` (99.6% → 99.9%) and `inflate_main` (98.5% → 98.0%)
  change, both through PRE spill-slot order.
- So 21 of the 23 open functions get exactly the same code from the
  reference compiler: their residue is in the source.
- `inflate_main`'s declaration order was tuned against 991111b's numbering
  (Register allocation, spill slots).

Every PS2 compiler decomp.me offers was tried the same way: the whole tree,
compared on the 576 functions of `src/` and `sdk/`, where the default gets
553 exact.

| Compiler | Exact | Notes |
|---|---|---|
| Sony 2.9-ee-991111-01, also its `dtls13010` build | 553 | the reference compiler |
| Sony 2.9-ee-991111, 991111a (Linux) | 552 | no `div.s` fix: `startup_screen_thread` misses |
| Sony 2.9-ee-991111b (Windows, unpatched) | 552 | the same |
| Sony 2.9-ee-990721 | 437 | older: 116 exact functions lost |
| Sony 2.96-ee-001003-1 | 53 of 409 | 3 files do not compile |
| SN Systems 2.95.2 v2.74 | 107 of 563 | `sq`/`lq` saves, other prologues |
| SN Systems 2.95.3 v1.36 | 81 of 258 | 19 files do not compile |
| Metrowerks MWCPS2 2.3-3.0.1 | not run | retail's RTTI names are g++ 2.x mangled (`12DiskFileName`, `Q211ImageLoadert12MCardSource_2Ui0Ui0`); the runtime is GCC's `libgcc.a`/`libstdc++.a` |

The archives are at
`https://github.com/decompme/compilers/releases/download/compilers/<name>`.

### Flags ([ref/compile_flags.json](ref/compile_flags.json))

| Files | Flags | Evidence |
|---|---|---|
| `src/*` (Polyphony) | `-O2` (default `-G8`) | `$gp`-relative small data in main, startup, filename, io, gs, allocator, imageloader, printf, gzip |
| `sdk/*` (Sony libraries) | `-O2 -G0` | no `$gp` access anywhere in libkernl, libcdvd, libpad, libmc, libc |
| `src/crtbegin.c` | `-O0` | collect2-generated, `$fp` frames |
| `sdk/*` assembler | `"asm_pass": "gas_swap"` | the SDK libraries were assembled by a GAS that fills delay slots by swapping (see Compiler notes, Assemblers; `tools/sdkas.py`) |
| `src/pdistd/pdistd_misc.cpp`, `src/mcard_io.cpp`, `src/gs_texture.cpp`, `src/gs_driver.cpp`, `src/gs_state.cpp` | `-O2 -fno-exceptions` | Under `-fno-exceptions`, a final call whose value is unused becomes `j callee` with the argument load in the delay slot; with exceptions egcs keeps `jal` and a frame. Retail has that tail-jump shape (the five `mc_*_sync_unlock` stubs; `gfx_dma_lock`, `gfx_set_screen_size` and others in gs). These files also have no entries in retail's `.gcc_except_table`, which references only montgomery, bignum, imageloader, main, gzip, printf and the runtime. Scoring every `src/*.cpp` with and without the flag: only these files improve (gs went from 35 to 49 exact); all others lose |
| `runtime/*` | not compiled for matching | the retail code *is* the toolchain's `libgcc.a` / `libstdc++.a` |

When a function needs a different flag to match, add a narrower rule, above
the broader one, with the evidence.

## One source tree, two compilers

| Build | Compiler | Purpose | Command |
|---|---|---|---|
| matching | SCE ee-gcc 2.9 | byte accuracy | `python tools/score.py`, `python tools/mcc.py <file>` |
| behaviour | GCC 14 (ps2sdk) | the booting `scus_971.02` | `python tools/build.py`, `python tools/boot_test.py` |

Every `.c` / `.cpp` under `src/` and `sdk/` must compile with both. That
means GNU C89 plus the extensions both accept, and the egcs C++ dialect
(C++98 without its later corners). `include/types.h` bridges the gap.
`BOOT_OLD_GCC` is defined under the old compiler.

- **`static_assert` / `_Static_assert`** are macros under the old compiler.
  Use them at file or namespace scope only, never inside a class or function.
- **`noexcept`, `override` and `nullptr`** expand to nothing / nothing / `0`
  under the old compiler. Prefer not to write them at all.
- **`BOOT_USED` and `BOOT_MAY_ALIAS`** replace `__attribute__((used))` and
  `__attribute__((may_alias))`, which egcs does not know.
- **Not allowed in C:** declarations after statements, `for (int i = …)`,
  hex float literals, `_Bool`/`<stdbool.h>`, `__builtin_clz`, and
  `.pushsection`/`.popsection` in inline asm (the old assembler lacks them).
- **Not allowed in C++:** C++11, `<new>` and other standard library headers
  (the old compiler ships none), anonymous structs/classes that declare no
  object, and `enum class`.
- **Allowed in both:** GNU statement expressions, `__attribute__((aligned))`,
  `mode(TI)`, inline asm, GNU range designators `[a ... b] =`, and C99
  designated initializers.
- **Checking.** `sh tools/cc.sh <file>` checks GCC 14, and
  `python tools/mcc.py <file>` checks the old compiler.

**Behaviour is never traded for bytes.** A change made to match must keep the
function's semantics exactly: same calls, arguments, stores, returns and
signedness. `python tools/build.py && python tools/boot_test.py` must still
reach GT3's main loop.

**Assembly is for hand-written retail code only.** A function the compiler
produced is matched in C/C++. A retail function that is demonstrably
hand-written assembly may be kept as assembly: instruction patterns no
compiler emits, such as libc's MMI quadword string routines (`memcpy`,
`memmove`, `memset`, `strcmp`, `strcpy`, `strcat`, `strncpy`, `memcmp`).
`runtime/libc.c` is the model:
- under `BOOT_OLD_GCC && !NON_MATCHING` (`LIBC_RETAIL_ASM`), those functions
  are the retail instructions in top-level `__asm__` blocks;
- the C implementation stays in the `#else` branch, so GCC 14 and
  `-DNON_MATCHING` builds use readable code that behaves the same;
- `tools/difftest.py`'s libc specs test both forms.

## Measuring

```sh
python tools/score.py                      # everything; writes build/score/report.md + score.json
python tools/score.py src/imageloader.cpp  # some files; prints per-function lines
python tools/score.py --diff 0x01001070    # side-by-side diff of one function
python tools/score.py --diff "Buffer::resize"
```

How the scorer works:

1. **Compile.** Each file is compiled twice with `tools/mcc.py`: to an object
   (the bytes that are scored) and with `-g -S` (the `.stabs` line records).
2. **Map.** Each function symbol is mapped to its `// FUNC 0x…` marker:
   - by name first: the marker text contains the mangled symbol, which is how
     compiler-generated `__tf…` functions are marked, or the marker's
     qualified name equals the symbol's GNU v2 demangled parts
     (`tools/v2mangle.py`);
   - otherwise by the nearest marker above the function's defining line.
3. **Align.** Instructions are aligned on their opcode (difflib), so a
   register-allocation difference pairs two instructions up instead of
   reading as an insertion plus a deletion.
4. **Compare.** Bytes under a relocation are masked. For `jal`/`j`, the call
   target is also checked against the retail target through the symbol map.
   A call to a local symbol (a section symbol plus the addend in the `jal`
   field) is looked up by (section index, offset). The section index matters:
   with `-ffunction-sections` every function sits at offset 0 of its own
   section, so a lookup by offset alone would credit a call to the wrong
   function. Trailing alignment nops are ignored on both sides (a `nop`
   that is not in a delay slot, at the end).

| Metric | Meaning |
|---|---|
| bytes % | equal bytes / (4 × max(C, retail) length) — **the headline figure** |
| insns % | whole instructions equal / max length |
| size % | min / max length |
| exact | same length, every instruction equal (code) |
| exact with data | exact, and every checkable data reference points where retail's does |
| … of compiled | over functions that compiled and mapped |
| … of scope | over all 881, with uncompiled functions counting 0 |

Runtime functions found in `ref/libmatch.json` count as exact
(`toolchain:libgcc.a(...)`).

**Data references** (`tools/dataref.py`). A masked relocation compares equal
whatever it points at, so the scorer also checks each `%hi`/`%lo` pair and
each `%gp_rel` or literal-pool reference.

- **Named symbols.** When the symbol's retail address is known (an `@0x…`
  annotation on its definition, or a function address from the symbol map),
  the address the retail instructions build must equal it plus the addend.
- **Anonymous references.** String literals and statics become a section
  symbol plus an offset. Every such reference to the same section of an
  object must imply the same retail base.
- **Reporting.** Differences appear as `data-refs: N differ (…)` in partial
  runs, and as `data_refs_bad` / `exact_with_data` in `score.json`. The code
  can be exact while the data layout still differs.
- **The common cause is the `.data`/`.bss` rule.** Old GCC puts explicitly
  initialised variables, `= 0` included, in `.data`, and only uninitialised
  ones in `.bss`. Declaration order is layout order.

## The byte-identical build (`matching/`)

```sh
python tools/build_matching.py            # assemble + link the split; compare with retail
python tools/build_matching.py --resplit  # regenerate matching/ first
```

- **Split.** [splat](https://github.com/ethteck/splat) (spimdisasm, R5900
  mode) splits the retail file into symbolised assembly and data:
  `matching/asm/text/<unit>.s` for each `.text` unit range,
  `matching/asm/data/data/*.s` for `.data`, `.rodata`, `.rdata`,
  `.gcc_except_table`, `.lit4`, `.sdata`, `.sbss` and `.bss`, and
  `matching/assets/` for the ELF header and trailer.
- **Inputs.** `tools/gen_symbol_addrs.py` writes `matching/symbol_addrs.txt`:
  all 881 functions, named from the FUNC markers, with sizes that exclude
  alignment padding. `tools/gen_splat.py` writes `matching/scus_971.02.yaml`.
- **Build.** `tools/build_matching.py` assembles with ps2sdk's `as`
  (`-march=r5900 -mabi=eabi`), links with splat's script
  (`ld -m elf32lr5900`), flattens the result with `objcopy -O binary` and
  compares it with retail.
- **Fixups.** The build patches three things splat cannot express:
  - `.sdata`'s last 3 bytes, which splat drops;
  - the 128-byte alignment of `.sbss` and `.bss`;
  - the trailer at file offset `0x42E6B` with no ROM rounding.
- **Result.** `matching/build/scus_971.02.bin` is byte-identical to the retail
  file (SHA-1 `9ba003bddd9e03cae0440fd5f587f6201b25bd40`).

This is the base that matched C replaces piece by piece. The next step is
per-function assembly (`INCLUDE_ASM`) inside C files laid out as the
original objects. That needs the object boundaries of `.data`/`.rodata` as
well as `.text`, because string literals and statics move with their C file.

### Translation units and data layout

A source file should be exactly one original translation unit (object), with
its data defined in the original order. Otherwise functions can be byte-exact
while their data references point into another retail section instance than
the rest of the object's ("exact" but not "exact with data"), and the objects
could never be linked into the retail image.

```sh
python tools/layout.py src/gs_driver.cpp [more files]   # summary per file
python tools/layout.py -v src/pdistd/printf.cpp          # + items, gaps, splits, externs
python tools/layout.py                                   # problem files + project summary (--all: every file)
python tools/layout.py --objects                         # the retail EH objects
python tools/layout.py --retail 0x01039400 0x01039600    # what is known at retail data addresses
```

It compiles every C/C++ file into `build/layout/` (about 2 s), because the
neighbours' data is part of the evidence. Per file it reports:

- **Text.** The `.text` runs of the file's functions and the other files'
  functions between them (a run break is an object boundary), and the padding
  at each run end.
  - `text drift`: where the object's own `.text` offsets stop following the
    retail distances. A function before that point is longer or shorter, is
    padded differently, or was emitted in another order (compiler-generated
    and inline functions, merged objects).
  - Local code addresses, such as static callbacks, are then off (the
    scorer's `.text` base differences), and so would a link be.
- **EH objects.** egcs has no `.eh_frame` section here. Every C++ object
  compiled with exceptions ends its own `.data` with a DWARF2 frame block (a
  CIE, one FDE per function, a zero word), and collect2 lists all 36 blocks in
  the table at `0x01026C00`. The FDEs give that object's exact `.text`. The
  file's own object is compared with retail:
  - a frame block or not (`FLAGS`: `-fno-exceptions` is missing, or is wrong);
  - the CIE augmentation `"eh"` (the object has exception regions);
  - the block's size and FDE count.

  When the file is exactly one retail EH object, its block gets the retail
  address. That also checks the `.data` in front of it.
- **Data items.** Every data reference of a mapped function is resolved
  through the aligned retail instruction (`tools/dataref.py`) to an item: a
  symbol, or an anonymous piece such as a jump table or a literal.
  - Items one section implies the same retail base for are laid out as in
    retail; several bases = the scorer's `data-refs differ`.
  - Items no code of the file uses are placed by their `@0x…` annotation or
    by other files' references.
- **Owned landmarks.** Each placed item is owned by the file that defines it,
  not by the files that use it as an extern. Annotated globals that no
  compiled file defines, references from uncompiled code and the retail
  frame blocks complete the map.
- **Splits.** A landmark of another owner between two items of the file puts
  them in different objects. The tool infers the fewest boundaries and the
  range each may lie in ("undetermined" functions can go either way).
- **Neighbours.** For each object and section: the nearest data of other
  owners below and above, i.e. the objects linked right before and after it.
  - `LINK ORDER`: a neighbour whose text is on the wrong side of the object's
    text.
  - `ELSEWHERE`: an item cut off from the file's other data by foreign data,
    or lying between objects linked before / after the file. It belongs to
    another object.
  - `OVERLAP`: two files claim the same bytes.

Facts this rests on (all checked on this binary or with the retail compiler):

- **Link order.** Objects contribute every section in link order, so data
  order mirrors text order.
  - The exception is code in `-ffunction-sections` sections:
    `.text.sceGsGetGParam` / `.text.sceGsResetGraph` follow `.text`, but
    libgraph's `.data` (`0x010394F0`) sits where libgraph.o was linked,
    between two gs objects.
  - Text alignment proves no boundary: functions are 8-byte aligned and the
    old assembler gives an object's `.text` the alignment of its contents (8).
  - A retail function that is NOT 8-byte aligned was not compiled as a
    function. `vu0_load_identity_regs` (`0x0100E404`) directly follows
    `gs_draw_textured_sprite` inside the driver's object, so it was a
    top-level `asm` block at that point of the file. It is now matched that
    way, with the C under `#else` (as `intr_suspend`), and the object's
    `.text` no longer drifts by 4 + 4 bytes after it.
- **cc1plus output order.** An initialised variable is written where it is
  defined. A namespace-scope variable without an initialiser is written only
  at the end of the unit, after all functions. `u8 g_010384D8[0x1000];`
  therefore landed behind `s_gsDmaChan`, while retail has it in front;
  `= { 0 }` keeps the order and changes nothing else.
- **`const`.** A `const` table goes to `.rodata`. Retail `.data` means the
  original was writable (gs's register-number tables).
- **Data-only objects.** A C++ unit with exceptions and no function still gets
  a 0x18-byte frame block (CIE + zero word). Retail has two: `#10` (between
  `imageloader_source` objects) and `#22`, which is PrintFormat's number
  tables.
- **Commons.** C files (the `sdk/` ones) make uninitialised globals common
  symbols, which the linker places after every object's `.sbss`/`.bss` (from
  `0x01041E90` / `0x0104DD28`; egcs's `__ti` type_info nodes are commons too).
  Retail data of such a symbol inside an object's own `.bss` means it was
  `static`, initialised, or built with `-fno-common`.
- **Anonymous namespaces.** egcs names `namespace {` when it first sees it:
  - after the first public symbol already assembled in the unit
    (`first_global_object_name`);
  - else after the input file name plus six random characters (time and
    pid). Retail printf.cxx got `_GLOBAL_$N$.._.._printf.cxxQakP7f` this way.

  A rebuild cannot reproduce random characters, and they would make symbol
  names and FUNC markers change with every compile. `src/pdistd/printf.cpp`
  therefore starts, for the old compiler only, with a zero-size public object
  whose assembler name is `.._.._printf.cxxQakP7f`. egcs then derives exactly
  the retail name, and the RTTI string is byte-identical (51 bytes, so the
  next object's `.rodata` stays at `0x0103FCF0`). The object emits no bytes.
- **Markers for those names.** The `.` in such names stops the name matching
  of `tools/score.py` and the identifier matching of `tools/difftest.py`.
  - Anonymous-namespace members are marked `(anonymous namespace)::Class::fn
    (<mangled>)`; difftest finds them through the demangled name.
  - The `__tf` function's marker sits above the class definition, whose
    closing line is the debug line the scorer maps it by.

The units as they are now laid out (link order; ↑ = data-only object without
code; all three gs files and pdistd_misc are `-fno-exceptions`, the printf
files use the default flags):

| File | `.text` | Data | Evidence |
|---|---|---|---|
| `src/gs_texture.cpp` | `0x0100BE10..0x0100BF98` | `.data` `s_gsTexOverride` `0x01034640` | the VU1 blob (`0x01034730..0x010384D8`) separates it from the main driver's `.data`; no frame block covers it |
| ↑ VU1 microcode (`link/blobs.S`) | – | `.data` `0x01034730..0x010384D8` | |
| `src/gs_driver.cpp` | `0x0100BF98..0x0100EE48` | `.data` `g_010384D8`, `s_gsDmaChan`; `.rodata` `0x0103E360..0x0103E5E0`; `.lit4`, `.sdata`, `.bss` | `.data` order needs `g_010384D8` initialised |
| ↑ libgraph (`sdk/libgraph`) | function sections | `.data` `0x010394F0..0x01039510` | |
| `src/gs_state.cpp` | `0x0100EE48..0x0100F130` | `.data` register tables `0x01039510..`; `.rodata` `0x0103E5E0..` | after libgraph's `.data`; no frame block covers it |
| `src/pdistd/allocator.cpp` (1st) | `0x0100F918..0x01010318` | frame `#21` | |
| ↑ `src/pdistd/printf_tables.cpp` | – | `.rodata` `0x0103E6A8..0x0103FB40`; frame `#22` | between allocator's and PrintFormat's `.rodata`; 0x18-byte block |
| `src/pdistd/printformat.cpp` | `0x01010318..0x01011210` | `.rodata` `0x0103FB40..`; `.sdata` `"(null)"`; frame `#23` | own frame block, its size matches |
| `src/pdistd/allocator.cpp` (2nd) | `0x01011210..0x010114A8` | frame `#24` | |
| `src/pdistd/printf.cpp` | `0x010114A8..0x01011700` | `.rodata` `0x0103FCA0..0x0103FCEB`; frame `#25` (`eh`) | own frame block, its size matches |

Two boundaries are not pinned by any data and were set by reading the code:

- `gfx_wait_vsync_poll` and `gfx_reset_paths` stay with their caller
  `gfx_init` in `gs_driver.cpp`; they could also end `gs_texture.cpp`.
- `gs_emit_masked_regs`, which only `gs_collect_dirty_regs` calls, opens
  `gs_state.cpp`. The data allows the boundary anywhere after
  `dma_start_source_chain`.

Either choice gives the same image.

## The behaviour build with the retail compiler

```sh
python tools/build.py --compiler sce            # -> build/sce/scus_971.02 (never installed)
python tools/boot_test.py --elf build/sce/scus_971.02 --tag sce
```

- **Compiler.** Every `src/` and `sdk/` file is compiled by `tools/mcc.py`
  with its retail flags.
- **Runtime.** It comes from the toolchain's own `libgcc.a`, as in the
  retail link.
- **Link.** `link/bootstrap.ld` is shared with the GCC 14 build.
- **Purpose.** This proves the C behaves the same when the retail compiler
  generates the code. That matters for the functions whose bytes do not match
  yet, because their code is not the retail code.

## Matching a function

### Workflow

The order of attempts that worked best in the round-2 matching reports (the
reports themselves are not part of the repository):

1. **Diff, then sort the differences.** `python tools/score.py --diff <addr>`
   shows C against retail. First rule out causes that are not the
   function's C:
   - `jal` plus a frame where retail ends in `j callee`, or a frame block
     retail lacks: the file's flags (`-fno-exceptions`, see Flags);
   - "exact" with `data-refs differ`, or `.text base off by 8`: data layout or
     a neighbour's length (`python tools/layout.py <file>`, "Translation units
     and data layout");
   - in `sdk/`, only the delay slot of a branch or of the epilogue `j $31`:
     the SDK assembler (Compiler notes, Assemblers);
   - `nop`s in front of a `div.s`: the assembler pass in `tools/mcc.py`;
     a `div.s` in a delay slot: the compiler is not the patched copy
     (`tools/patch_divs.py`).
2. **Control flow.** Match block layout and branch sense first: `if`/`else`
   order, loop form, where the exits and `continue`s are, which tails are
   shared. Allocation and scheduling only settle once the shape is right.
3. **Types.** Field and pointer types decide which loads may pass which
   stores (Aliasing). `int`/`unsigned` picks `slt`/`sltu`, and `char` locals
   add sign extensions.
4. **Statement order.** Try the natural order first, or ps2sdk's for Sony
   code: that matched `sceSifInitRpc` and `_request_bind` outright. Then
   search permutations of the independent statements: score a scratch copy of
   the file with `tools/score.py`'s `compare()` and keep data dependencies in
   order.
   - A hill-climb found `gfx_context_init`, `gfx_begin_packet`,
     `gfx_update_display_regs` and `gfx_set_draw_buffer` quickly.
   - Exhaustive search is cheap up to a few hundred orders (all 720 for six
     statements of `huft_build`).
5. **Statement macros.** Try each site with and without a
   `do { } while (0)` wrapper. It can decide a function either way (Compiler
   notes, Loops).
6. **Registers, last.** Merge or split variables, use one variable per loop,
   compute temporaries before branches, add inline helpers (Register
   allocation).
7. **Dumps.** When edits stop changing anything, use the RTL dumps below to
   find the pass that decides, instead of guessing further.
8. **Stop** when what is left is a spill-slot permutation or a local-alloc
   tie that natural source does not reach. Keep the most plausible source and
   write what was tried into the function's comment. Code nobody would have
   written is normally not kept for one register. **Fakematch policy:** an
   exact form that is not the original is kept only if it provably computes
   the same thing, its comment says so ("an exact form, not the original")
   and it is listed here. The only one is `sceIoctl` `0x0101FD28`:
   `int ret = 1;` and `case 1: return ret - 1;` (retail keeps the constant 1
   in `$s5` from the prologue; the value returned is 0 either way). Round 2
   had rejected it; round 3 kept it on request to match everything that can
   be matched.
9. **Check**: `sh tools/cc.sh <file>`, `python tools/mcc.py <file>`, an SCE
   build in your own `--build-dir`, then `tools/difftest.py --group <g>` on
   it (Function-level behaviour tests). Eventually, the boot test.
10. **Record** what applies generally in "Compiler notes".

### Levers

- statement order;
- temporaries versus reusing an expression;
- types (`int` vs `u32`, `s16` loads, `char` signedness, field and pointer
  types);
- `if` / `else` order;
- loop form (`for` vs `while` vs `do` vs label/`goto`);
- where a variable is declared (this moves its spill slot, not its register);
- pointer increments vs indexing;
- early `return` vs a single exit;
- `inline` helpers vs macros, with or without `do { } while (0)`;
- one variable reused for non-overlapping lifetimes, or several.

egcs specifics:

- it keeps source statement order more than modern GCC does;
- register saves go in 16-byte slots;
- registers are handed out by priority (Register allocation). This file
  used to say "saved registers in order of first use"; PrintFormat's
  matching found that declaration order only breaks exact ties.

### RTL dumps

Extra flags after `--` reach the compiler:

```sh
python tools/mcc.py src/gzip_inflate.cpp -S -o <scratch>/gz.s -- -dG -dl -dg
```

- **Output.** Each `-d<letter>` writes the RTL as it is after one pass, to
  `<source file name>.<pass>`, e.g. `gzip_inflate.cpp.gcse`. `-da` writes
  all of them.
- **Location.** The files land in the compiler's working directory, which
  for `tools/mcc.py` is the project root. Move them out: a stray
  `sha512.cc.greg` was left there once.

The letters, checked on this compiler:

| Flag | File | RTL after | Use |
|---|---|---|---|
| `-dr` | `.rtl` | expansion | order of expansion: reassociation, pre-expanded calls |
| `-dj` | `.jump` | jump 1 (CSE1's input) | |
| `-ds` | `.cse` | CSE 1 | equivalences, extended-block ends |
| `-dF` | `.addressof` | addressof | |
| `-dG` | `.gcse` | GCSE / PRE | expression hashes, PRE insertions, reaching registers |
| `-dL` | `.loop` | loop | "Loop … ignored due to multiple entry points", givs, hoisted invariants |
| `-dt` | `.cse2` | CSE 2 | |
| `-db` | `.bp` | branch probabilities | |
| `-df` | `.flow` | flow | reference counts (loop-weighted), deaths |
| `-dc` | `.combine` | combine | |
| `-dN` | `.regmove` | regmove | |
| `-dS` | `.sched` | sched1 | |
| `-dl` | `.lreg` | local-alloc | quantity priorities, tied registers |
| `-dg` | `.greg` | global-alloc and reload | hard registers, spill slots |
| `-dw` | `.flow2` | flow 2 | |
| `-dR` | `.sched2` | sched2 | the dependences that fix the final order |
| `-dJ` | `.jump2` | jump 2 | cross-jumping |
| `-dM` | `.mach` | machine reorg | |
| `-dd` | `.dbr` | reorg | delay-slot fills |

### Keeping behaviour

The GCC 14 build compiles with `-fno-strict-aliasing -fwrapv -fno-builtin
-ffreestanding`, so the `#else` branches need their own care. The
verification rounds caught these problems in matching edits; all are fixed
unless the table says otherwise.

| Pitfall | Found in | Remedy |
|---|---|---|
| A load moved after a store that retail does before it: the result changes when the two alias | `gs_emit_masked_regs` `0x0100EE48`; `_sceSifLoadModule` `0x01022330` (pre-existing, the edit fixed it) | load in retail's order: it is the original's |
| `memcpy` in source: egcs inlines it, the installed build (`-fno-builtin`) makes real calls, here in an interrupt handler | `_fio_intr` `0x0101EB08` | typed loads in the `#else` branch |
| A macro that names an lvalue twice: egcs CSEs it, GCC 14 loads the table pointer twice around a barrier | `sceSifAddCmdHandler` `0x0101D668` | take the address once |
| Padding nops that egcs inserts itself (short loops) deleted from the source: GCC 14 does not insert them | `sio_putc` `0x0101C990` | `SIO_POLL_PAD()`: empty under egcs, the nops otherwise |
| Inline asm missing clobbers, or guarded by `BOOT_OLD_GCC` alone | `clut_anim_channel_apply` `0x0100B6D0` (`pmulth`/`pmaddh` write HI/LO) | `"hi", "lo"`; `BOOT_OLD_GCC && !NON_MATCHING` |
| `dsllv`/`dsrlv` use the low 6 bits of the count; in C an out-of-range count is undefined | `_dtoi` `0x0101CAB0` (`DTOI_SHIFT`); gzip's `GZ_SH` | mask in the GCC 14 branch only |
| Retail writes that land in frame padding are out of bounds in C | `inflate_main` `0x01012180` | `ll[316 + 30 + 2]` (the SCE frame is unchanged) |
| The R5900 `div` does not trap. egcs checks some divisions (`break 7`), GCC 14 checks all (`teq`) | `gfx_update_display_regs`, `gs_emit_render_target_regs`, `_rpc_get_fpacket` | not fixed: unreachable in this program |

## Compiler notes

Facts about SCE ee-gcc 2.9-ee-991111b, each with the function it was found
on or checked against. Most come from the round-2 matching reports.
**Open** marks a retail form nobody has
reproduced yet. **Reports disagree** marks a point where two reports
contradict each other.

### Basics

- **Prologue.** Saved registers go in 16-byte slots (`sd`/`ld`, s0 at the
  lowest). `$ra` is saved in the highest slot.
- **Alignment.** Functions start 8-byte aligned (`.p2align 3`), so a retail
  function can be followed by one padding `nop`.
- **Delay slots.** `#nop` in `-S` output is only a comment. Load delay slots
  are not filled with real nops on the R5900.
- **Unused parameters.** They still show in callers, which set the argument
  register only if the prototype has the parameter. For example,
  `gfx_set_display_fb` has an unused 4th parameter that retail callers pass
  (`$a3 = 0x1C0`).
- **Short loops.** cc1 pads a loop body shorter than the R5900 short-loop
  erratum limit with `nop`s, visible as `.set noreorder` blocks in `-S`
  output. Examples:
  - an empty source loop gets four: Sony's nopdelay in libcdvd, `sifrpc.c`
    and `fileio.c`;
  - `main`'s spin loop gets five.

  GCC 14 does not pad, so its branch must write the nops out.

### Front end

**Sums are reassociated.** Integer sums of four or more terms are regrouped
(`sha512_transform` `0x01001320`, PrintFormat `0x01010368`):

| Source | Emitted |
|---|---|
| `a+b+c+d`, i.e. `((a+b)+c)+d` | `(a+d)+(c+b)` |
| `a+((b+c)+d)` | `(a+b)+(d+c)` |
| `a+b+c+d+e` | `((a+e)+(c+b))+d` |
| `(a+b)+(c+d)` | `(a+c)+(b+d)` |
| `a+b+c` | as written |

So `h += S1 + Ch + K + W` becomes `((h+S1)+(K+Ch))+W`. Pick the source order
that yields retail's pairing. The regrouping also decides which calls are
expanded first.

**Byte combining.** `(p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]` in `int`
arithmetic is regrouped as `(b0<<24|b3)|(b2<<8|b1<<16)`. With `(u32)` casts
it stays a straight chain. Retail has the regrouped form
(`sha512_load_be64` `0x010012B8`).

**Calls inside expressions (preexpand_calls).** Real calls are expanded
before the rest of the expression. Inline calls are expanded where the tree
walk reaches them, so they come before a real call only if they precede it
in the (reassociated) tree. Operands that are not calls are expanded after
(`sha512_transform`).

**Rotates.** `(x>>n)|(x<<(64-n))` with a constant `n` folds to
LROTATE_EXPR. It expands left shift first, with the IOR operands swapped.
For retail's right-shift-first order, write the rotate as an inline function
that takes the amount as a parameter:

- `rotr64(u64 x, unsigned int y)` matched;
- `(u64, int)` dropped `sha512_transform` to 57%.

**MAX_EXPR / MIN_EXPR.**

- Integers: `x = (a<b) ? b : a` and the equivalent `if`/`else` fold to
  MAX_EXPR, which gives `x=a; movn x,b` and compares on `x`. Retail's
  `x=b; movz x,a,(a<b)` needs `x = b; if (!(a < b)) x = a;` (PrintFormat's
  `%t` width).
- Floats: `a > b ? a : b` is not folded without `-ffast-math` and gives
  `c.lt.s; bc1f; mov.s`. Under `-ffast-math` fold makes `a > b ? a : b`
  MAX_EXPR(b, a) and `a >= b ? a : b` MAX_EXPR(a, b) (operand order shows in
  `max.s`).
- egcs C++ has the GNU operators `a >? b` and `a <? b`. They build
  MAX_EXPR / MIN_EXPR directly, in source operand order, to `max.s` /
  `min.s` (`clut_anim_step` `0x0100B808`; `ee_maxf` / `ee_minf` in
  `include/gs_internal.h`). C has no such operator. For gs, the operators
  and `-ffast-math` ternaries give the same code; the operators need no flag,
  and no other gs function changes under either.

**Other folds.**

| Source | Code | Evidence |
|---|---|---|
| `if (m != 0 && m <= K) do {…} while (m <= K)` | one `(m-1) < K` range test | PrintFormat |
| `if (m != 0) while (m <= K) …` | both branches kept | PrintFormat |
| `(e <= -100 \|\| e >= 100) ? 5 : 4` | `addiu; li 4; sltiu 199; li 5; movz` | PrintFormat |
| `((unsigned)(e+99) < 199) ? 4 : 5` | the inverted slt/movz form | PrintFormat |
| `__builtin_abs(e)` | a branch (`bltzl; negu`) | PrintFormat |
| `if (e < 0) e = -e;` | if-converted (`movz`) | PrintFormat |
| `return (x & 0x10000) != 0;` | `srl 16; andi 1` | kernl_rest |
| `if (x & C) return 1; return 0;` | `and` + `sltu` | `sceSifIsAliveIop` `0x010229E8` |
| early `if (…) return 0;` tests, then `if (bit) return 1; return 0;` | a store-flag | kernl_rest |
| `if (a == 0 \|\| b != c \|\| !(d & 1)) return 0; return 1;` | three branches, two `jr` | `sceSifCheckStatRpc` `0x0101E480` |
| `(unsigned)(p - img) + 11` | `subu; addiu` | kernl_rest |
| `p + 11 - img` | `p - (img - 11)`, with `img - 11` on both paths into the loop exit | `sceSifRebootIop` `0x01022A58` |

**`x & ~C` and `not`.** `x & ~C` folds to a constant mask. egcs never emits
`not` (`nor rd, rs, $zero`) outside MIPS16: its one_cmpl template is
`nor rd, $zero, rs` (checked in cc1plus's strings). The retail `not`s
therefore come from asm (Inline asm):

- `0x0100BED0` in `gfx_set_texture_regs`;
- `0x0100ED80` in `gs_set_bg_color`;
- `0x0100F554` in libc's hand-written `strlen`.

**Constants, widths and signedness.**

- **Signed char stores.** `0x80` stored through a `u8` lvalue loads
  `li 0x80`; through a signed char it loads `li -128` (`sha512_pad`
  `0x010033C8`).
- **`char` locals.** Assigning an `int` to a `char` local adds
  `sll 24; sra 24`. Retail's digit characters were `int`s (PrintFormat).
- **Loop indexes.** An `int` index gives `slti`, an `unsigned` one `sltiu`
  (`sceDevctl` `0x01021948`).
- **HImode values that CSE knows.** These come from a just-stored or
  just-loaded register (`clut_anim_step`):
  - `if (ch->cur != 0)` on the field gives `andi 0xFFFF`; a `u16` local
    gives a plain `bnez`;
  - `ch->hold > 0` on a just-stored `s16` field gives `sll 16; bgtz`; an
    `s16` local gives `sll 16; sra 16`.
- **Case tails.** `ch->cur = <expr>; k = ch->cur;` at the end of each switch
  case gives the cross-jumped tail `sh v0; andi a3,v0,0xFFFF`. A `u16`
  temporary gives `andi; move; sh` instead.

**`va_arg`.** `int v = va_arg(…); uval = (unsigned)v; sval = v;` gives two
loads (combine's added_sets_2), with `lwu` first. Retail's `lw` then `lwu`
needs an explicit second read of the same slot:
`sval = va_arg(args, s32); uval = ((u32 *)args)[-2];` (PrintFormat).

**Bitfields.** Bitfield assignments through a pointer to a `u64` shadow give
retail's `ld; and ~mask; or (v&0x7FF)<<pos; …; sd`, with a single `ld` and a
single `sd`. The order of the assignments sets the order of the masks. A
bitfield read converted to `s32` gives `andi` + `dsll32`/`dsra32`
(`gs_update_scissor_and_viewport` `0x0100DBB8`, GsScissor).

**Block copies.** Three forms give the same `ld`/`sd` sequence:

- a struct copy;
- `memcpy` of 64 aligned bytes;
- eight `u64` assignments.

A `memcpy` initialisation of a local array expands like element
assignments (`sha512_final` `0x010034E0`, `sha512_transform`).

**Inline functions.**

- **Address order.** Arguments are expanded with EXPAND_SUM:
  - `f(&tbl->ch[i])` and a direct `tbl->ch[i].f` give `addu x, idx, base`;
  - a local `ch = &tbl->ch[i]` or an out-of-line call argument gives
    `addu x, base, idx` (`clut_anim_table_relocate` `0x0100BC40`).
- **Fresh pseudos.** Each argument is copied into a fresh pseudo, which
  changes allocation (`gfx_select_list` `0x0100D6E0`: the link pointer got
  `v1` instead of `a3`).
- **No GCSE reuse.** Loads through an inline argument are other GCSE
  expressions than the caller's loads. Take an outer function that tests a
  pointer and passes it to an inline helper, which tests `p->field` and
  loops: the helper reloads the field at loop entry. Retail's tex1 CLUT
  functions have this shape (`clut_anim_apply_all` `0x0100B798`,
  `clut_anim_release` `0x0100BB28`, `clut_anim_reset_all` `0x0100BBA0`).
  Written as one function, PRE carries the tested value into the loop
  instead.
- **Exception: frame addresses.** An argument that is a constant frame
  address (FIXED_BASE_PLUS_P, e.g. `this` of a local object) gets a
  constant equivalence and folds to `sp+offset` (integrate.c). A pseudo copy
  appears only for a register argument that is not a user variable.
  - **Solved in the final pass** for `DiskSource::open` `0x01004F58` and
    `DiskSource::getLoadFlags` `0x010051F8` (both exact). The RAII cleanups
    keep the object's address in an s-register (`addiu s0,sp,0x10`; the
    inlined destructor copies it, `daddu s1,s0,$0`, and stores `fd = -1`
    through it) and test `fd < 0` twice (`bltz`, then a redundant `bltzl`).
    The source shape: `close()` guards with a `bool isOpen() const { return
    fd >= 0; }` helper, and the destructor is `if (fd >= 0) close(); fd =
    -1;`, with `isOpen()` and `close()` defined before the destructor (see
    CSE, "Store-flag tests", and C++, "Definition order"). The memory-card
    pair (`MCardSource::open` `0x01005458`, `getLoadFlags` `0x01005748`) gets
    the same structure but still differs in its cleanup layout (open).
  - `-fthis-is-variable` does not compile: the abstract `Source` can no
    longer be constructed.
  - Round 3: the s-register comes back with `-fno-cse-skip-blocks`
    (`DiskSource::getLoadFlags` 81.9 → 92.0%): the destructor's `this` pseudo
    is a constant-equivalent copy that CSE1 folds to `sp+16` only when
    skip-blocks carries the equivalence around the `if (fd >= 0) sceClose`
    block. The flag is not the answer: it costs this file's three `__tf`
    functions and `HostSource::open`, which retail compiled with
    skip-blocks. A destructor with two tests (`if (fd >= 0) closeFile();
    fd = -1;`, `closeFile()` testing again; or a by-value helper) does not
    help either: CSE1 folds the second `lt` test through the jump
    equivalence of the first, which retail's compiler did not. Pointer
    helpers, `this` copies in locals and reference variables all fold the
    same way. (The reference compiler 991111-01 then showed the same output,
    so it had to be the source: the final pass found the store-flag shape
    above.)

### CSE

- **Store-flag tests are opaque to jump equivalences.** After `if (x >= 0)`
  CSE1 knows `x >= 0` on the fall-through path and deletes a second
  `if (x >= 0)`. It cannot do that through `if (isOpen())` where
  `isOpen()` returns `x >= 0` as a `bool`/`int` value: the test is on the
  flag register, and only combine, after CSE, turns `slt; beqz` back into
  `bltz x`. So a redundant retail test (`bltz` then `bltzl` on the same
  register) points at a helper returning the comparison. A narrower type
  (`short`) keeps the test too, but adds a reload (`DiskFile::close`).
- **Loop notes end CSE1's extended block.** CSE1 ends its extended basic
  block at every NOTE_INSN_LOOP_END. Stores are therefore not forwarded to
  loads across `do { } while (0)` blocks (`sha512_transform`).
- **CSE1 forgets after 1001 insns.** It flushes its table after 1001 insns
  within one extended block. A store is not forwarded to a load about 1000
  insns later; CSE2 may still do it (`sha512_transform`).
- **Canonical register (make_regs_eqv).** On a copy `a = b`, `a` becomes the
  head of the equivalence class only if:
  - it lives beyond the block, and
  - its REGNO_LAST_UID (its last mention, from reg_scan before CSE2) is later
    than the head's.

  Otherwise later uses of `a` are rewritten to `b`. A later mention of a
  variable flips this, even a dead store that flow deletes. In `huft_build`
  `0x01011838`, a dead `tp = NULL;` made `*tp = NULL` reload `tp` from its
  stack slot as retail does. That freed a register and shifted s0..s7/fp
  into the retail assignment.
- **Constant operands are replaced by registers.** A constant operand is
  replaced by any register known to hold that constant earlier in the
  extended block (follow-jumps, skip-blocks), even across calls. That
  register then becomes a long-lived s-register. A dead initialiser is
  deleted before CSE1 and cannot feed this.
  - `sceIoctl` `0x0101FD28` has `s5 = 1`, used only for `case 1`. A live
    `int ret = 1;` with `return ret - 1;` reproduces it (the fakematch in
    Workflow, step 8).
- **Jump equivalences.** In the branch where `x == 1`, CSE deletes `x = 1`.
  Retail's redundant `li v1,1` in `_rpc_get_packet` `0x0101DC18` needs
  `if (++rd->pid == 1) { rd->pid++; rid = 1; } else rid = rd->pid;`.
- **Identical non-volatile asm statements are CSE'd** (`ee_absf` in
  `clut_anim_step`).
- **Constants as asm inputs.** `T v = const;` used as an `"r"`/`"f"` asm
  input is propagated into the asm. The constant is loaded at the asm (by
  force_reg or reload), not at the declaration (gs).

### GCSE and PRE

- **Busy hoisting.** This snapshot's GCSE is edge-based LCM with busy
  hoisting. Anticipatable loads are hoisted into loop preheaders and kept in
  "reaching" registers, which are spilled when registers run out
  (`sha512_transform`).
- **Join hoisting.** Expressions move from joins into predecessors and
  preheaders: PrintFormat's `prefix & 0xFF`, `prec - len` and `len < prec`.
  - `signLen = (sign != 0)` is hoisted;
    `if (sign) signLen = 1; else signLen = 0;` stays, because it is
    if-converted only after GCSE.
  - Moving `str = …` before `zeroPad = …` was worth 3%.
- **Break tests.** A load in a break test gets a PRE copy:
  `if ((f <<= 1) <= *++xp) break; f -= *xp;` gives `lw` + `move`. With
  `u32 cnt = *++xp;` first there is a single load (`huft_build`).
- **Fields vs local copies.** Reading `ch->field` each time lets PRE keep the
  value in a reaching register (`clut_anim_step`'s `lh; bgtz; daddu t1,v0`).
  Index arithmetic on it (`keys[ch->cur]`) is then a separate expression at
  each site, so the shift is recomputed per access, as in retail. With a
  local `k`, `keys[k]` gets the shift PRE'd once.
- **Loop cursors.** Reading `p[0]`/`p[1]` before `p++` keeps GCSE from
  forwarding the new `p` into the loop body. The body then reloads the
  in-memory cursor as retail does (PrintFormat).
- **Split back edges.** PRE on a critical back edge splits the edge. loop.c
  then reports "Loop … ignored due to multiple entry points" (`-dL`), and
  strength reduction of `K[i+j]` does not happen (`sha512_transform`).
- **Inserted insns.** They follow a NOTE_INSN_LOOP_BEG, so the block's own
  insns are not scheduled past them. Their order decides which one reorg puts
  in a delay slot.
  - **Open:** `clut_anim_step` puts `i+1` before the `keys` load.
- **Hashes.** Solved from `-dG` dumps:

  | Expression | Raw hash |
  |---|---|
  | `(plus:SI (reg R) (const_int C))` | 14029 + R + C |
  | `(ashift:SI (reg R) (const_int C))` | 14041 + R + C |
  | `(plus:SI (reg R1) (reg R2))` | 14669 + R1 + R2 |

  - Components (sha512 report): code numbers MEM 62, PLUS 71, SImode 6;
    each REG adds 57·128 + REGNO and each CONST_INT adds 52·128 + value. That
    gives 77 + 7296 + 6656 = 14029, so the gzip and sha512 figures agree.
  - The table size is (insns at GCSE time / 2) | 1; cprop's SET table uses
    insns / 4.
- **Order of the new registers. Reports disagree.**
  - gzip and sha512: reaching registers are created in hash-bucket order
    (raw hash mod table size). Their pseudo numbers, and so their spill
    slots, depend on base REGNOs and on the function's insn count.
    - `inflate_main`'s PRE slots would need T = 431 or 433 instead of 429.
    - `sha512_transform`'s would need REGNO(base) − REGNO(fp) ≡ 88 mod T.
  - gameflow and pdistd_io: PRE inserts on an edge in expression-index
    order (first occurrence in the insn stream), "not hash-bucket order".
  - These may describe different things (pseudo numbering vs the order of
    the inserted insns), but no one has checked both on one function.

### Loops and `do { } while (0)`

egcs keeps NOTE_INSN_LOOP_BEG/END for a `do { } while (0)` (loop.c calls it
a phony loop). This affects code in several ways:

| Effect | Evidence |
|---|---|
| Scheduling barrier: nothing is interleaved across the block, constants are not hoisted, loads do not move over it | `gfx_init` `0x0100C490`: the CSR read stays ahead of the IMR writes; `tex1_relocate` `0x0100B560`: RELOC stores stay ahead of the next load |
| CSE1's extended block ends | `sha512_transform` went from 16% to 99.6% just by wrapping each round |
| shorten_branches aligns the first label after LOOP_BEG (before its END) to 8 bytes, adding a `nop` | retail `0x0100E874` (`intc_vif1_handler`), `0x0100EA64` (`dmac_channel_handler`); PrintFormat's padding nops, which retail lacks, went away when `PUTC_OR_FAIL` became a plain `if` |
| flow weights reference counts by loop depth, which changes local and global allocation priorities | `intc_vblank_handler` `0x0100E630` (s2/s3), `intc_vif1_handler`, `dmac_channel_handler`: exact only with statement macros |
| Removing one can help too | `gs_build_clear_packet` `0x0100DD10`: 89.6 → 94.5% |

Loop forms:

- **Label/goto loops.** They have no loop notes, so loop.c does not hoist
  their invariants: `DIntr` `0x01022B68` reloads `lui 1` on every pass. Put
  `asm(".p2align 3")` before the label for the alignment `nop`. As a
  `do`/`while`, the constant is hoisted.
- **Exit-test duplication.** duplicate_loop_exit_test (jump.c) copies
  everything from the loop start up to the exit jump in front of the loop.
  A load before `if (!c) break` is duplicated (PrintFormat's loop head). It
  refuses an exit test that contains an asm with operands, so that loop keeps
  its `b <test>` entry (`clut_anim_step`, `ee_absf`).
- **Retry loops.** `while (f() == 0) g();` gets its test rotated to the
  bottom. `do { if ((r = f()) == 0) g(); } while (r == 0);` gives retail's
  shape (`sceSifExecRequest` `0x0101E7A8`):
  - `bnez v0, exit` at the top;
  - `beql v0, zero, call` at the bottom, with the argument setup in its
    delay slot.
- **Count-down loops.** `n = (int)*p++; while (n--)` gives
  `addiu -1; bne -1` (`tex1_relocate`). check_dbra_loop reverses
  `for (i = 0; i < CONST; i++) a[i] = c` into a count-down with `bgez`.
  Index loops are never rewritten into pointer compares (no biv elimination
  with `!=`) (gzip).
- **Strength reduction.** `K[i+j]` with `j += 16` becomes one pointer giv,
  unless PRE split the back edge (`sha512_transform`).
- **Invariants.** They are hoisted, e.g. a constant hardware address
  (`lui`/`ori`).
  - **Open:** PrintFormat's renormalisation constant, which retail
    recomputes inside the inner loop.

### Combine

- **Volatile memory is not merged.** A volatile `u16` load converted to
  `u32` gives `lhu` + `andi 0xFFFF`. Retail's plain `lhu` rules out a
  volatile pointer (`gs_readback_begin` `0x0100EB00`).
- **Tying.** For local-alloc tying of an output to a dying input, see
  Register allocation.
- **truncdisi2.** The 64-to-32-bit conversion is one insn printed as
  `dsll32; dsra32`, and reorg never puts it in a delay slot. A retail
  `dsll32; jr; dsra32` is the SDK assembler's swap (Assemblers), not other C.

### Jump and cross-jumping

- **Shared tails.** jump2 (after reload) cross-jumps identical tails. In
  `inflate_main` `0x01012180`, the dynamic and the fixed block each end with
  their own `inflate_codes` sequence, and they are merged as in retail.
  - A `continue` in an `else { … }` right after a then-clause that ends in a
    call leaves a `(use (const_int 0))` fixup after the call, which blocks
    the merge.
  - `else if (cond) { continue; }` does not leave it.
- **Not every duplicate merges.** `sha512_pad`'s
  `if (left >= 16) {A} else {B; A}` keeps A in both arms, as retail does.
  Why it differs from `inflate_main` is not pinned down.
- **Stores in both arms.** A store written in both arms (`qd->end = sd` in
  `_request_call` `0x0101E200`) is cross-jumped into the join after sched1,
  and stays first there. Written once after the `if`, sched1 sinks it.
- **Call tails.** An error path that ends in a call shared with the other
  path is merged only at the `jal`. The argument load stays in both arms, and
  the short arm ends `b; lw a0` (`sceDopen` `0x01020998`).
- **Block order follows the source.** `if (c != 'F') {A} else {B}` puts A on
  the fall-through path; `if`/`else` order also sets the branch sense
  (PrintFormat, `sha512_pad`).

### Register allocation

- **local-alloc** works on pseudos that live in one basic block.
  - Priority: `floor_log2(refs) * refs / (death - birth)` (QTY_CMP_PRI);
    ties go by birth order.
  - The first quantity processed takes `v0`. So v0/v1 choices follow sched1
    order and lifetimes, not the source (`inflate_codes` `0x01011E40`). In
    `vu0_load_identity_regs`, the short-lived constant took `v0`.
- **Tying (combine_regs).** An insn's output is tied to an input that dies
  there only if both are local candidates (one block, exactly one death).
  - `m = mask >> 32; m &= 0x3FF;` keeps the shift in `m`'s register;
  - `(mask >> 32) & 0x3FF` gets a separate temporary
    (`gs_collect_dirty_regs` `0x0100EE80`);
  - a variable set twice with two deaths becomes global, which changes which
    operand is tied.
- **global-alloc** handles the rest.
  - Order: `floor_log2(refs) * refs / live_length`, with refs weighted by
    loop depth.
  - Declaration order and scope matter only for exact ties (PrintFormat).
- **CALLER_SAVE_PROFITABLE.** A pseudo that lives across calls gets a
  caller-saved register only if `4 * calls < refs` (PrintFormat).
- **Spill slots.** Reload (alter_reg) gives spilled pseudos stack slots in
  increasing pseudo number.
  - User variables are numbered in declaration order, so moving a
    declaration moves its slot. In `inflate_main`, declaring
    tlp/mask/nd/nl at function scope before `last` gave retail's
    0x580..0x590. In PrintFormat: prefix 0x284 < prefixLen 0x288 <
    rightPad 0x28C.
  - PRE registers follow GCSE's order (above), which depends on function
    size and on register numbers: GCSE hashes a register by its number, and
    the default 991111b numbers pseudos from 116, the reference 991111-01
    (and so presumably retail) from 84. Judge these slots with
    `score.py --cc 991111-01`. **Open:** `sha512_transform` keeps one
    swapped pair (0xE0/0xE8) under 991111-01 and a permutation of 8 slots
    under 991111b; `inflate_main`, one of 0x598..0x5B0 under both.
- **Copy preference.** A value passed as a call argument prefers that
  argument register. **Open:** `gs_set_bg_color` `0x0100ED58`, where alpha
  gets `$a2` and retail keeps it in `$a0`. Global-alloc tries copy
  preferences lowest register first, after pruning registers that conflict.
  Masking `rgba` in place gives the pseudo the copy preferences {a0, a2},
  but local-alloc has already put the mask and the `lbu` in `$a0`, so `$a0`
  is pruned. Retail's local quantities avoided `$a0` (x in `$a1`, mask in
  `$v1`). A brute force over volatile/non-volatile asms, mask placement and
  five alpha forms did not reach it.
- **No `$v0` pinning.** A local register variable cannot be tied to `$v0`:
  v/h/l/x/a mean hi/lo/accumulator, and z/j/k are rejected.
- **Free argument registers.** global.c gives a pseudo `$a0` when `$a0` is
  free over its range and a dying input ties it there. **Open:**
  `_sif_cmd_intr_handler` `0x0101D878`; retail must have had `$a0` busy.

Steering from source:

| Lever | Effect | Example |
|---|---|---|
| One variable reused for non-overlapping lifetimes, or one split up | merges or splits pseudos and their priorities; the strongest lever in big functions | PrintFormat 70 → 91.4%: `i`, `c`, `len`, `str`, `d` reused as retail's registers show |
| A block-scoped pointer per loop | separate pseudos, not one hard register for the whole function | `huft_build` 70.5 → 97.6% |
| A temporary computed before a branch | lives in the taken path's register or delay slot, as in retail | `inflate_codes`: `xe = e - 16`, the mask before `if (e == 15)` |
| Values or tag constants computed before a run of volatile accesses | changes luid order, so sched1 tie-breaks and lifetimes | `gs_readback_end` `0x0100EB78` 81 → 97.6%; `gs_draw_textured_sprite` `0x0100E2A0` 84 → 91% |
| Statement macros | loop-depth weighting of refs | `intc_vblank_handler`, `dmac_channel_handler` |
| An inline helper | its arguments become fresh pseudos | `gfx_select_list`, the `clut_anim_*` functions |
| A local for a base pointer | a callee-saved register holding it | `sha512_final`: `u64 *st = ctx->state` gives `s0 = ctx+8` |
| An indirect call through a local | another register for the target (`$a2` vs `$v1`) | `_sif_cmd_intr_handler` |
| A pointer assigned after a call | no s-register | `wait_vblank_get_csr` `0x01022C90` |
| A dead store to a local | flips CSE's canonical register | `huft_build`: `tp = NULL;` |

### Scheduling, reorg and delay slots

- **sched1 is on.** `-fno-schedule-insns` breaks exact libkernl functions.
- **Machine model.** Haifa (sched1/sched2) models the R5900 as one memory
  unit plus two ALUs. It pairs a load with an ALU op in one cycle, so a load
  is not moved in front of an independent `addiu` unless the `addiu` is not
  ready. **Open:** `gs_emit_masked_regs` `0x0100EE48`.
- **Tie-breaks (rank_for_schedule)**, in order:
  1. priority;
  2. register pressure: a store that is a value's last use (REG_DEAD) goes
     earlier;
  3. the relation to the last scheduled insn;
  4. the number of dependents;
  5. source order (luid).

  Independent stores therefore come out in source order, adjusted by which
  store is a last use.
- **Barriers.** Loop notes are barriers (Loops). So is a divide-by-zero
  check (`trap_if`, the `break 7` after `div`): a load after a checked
  division in the source stays after it (`gfx_update_display_regs`
  `0x0100C8B0`, `gs_emit_render_target_regs` `0x0100DF40`).
- **Inline syscalls.** Put the number load inside the asm string
  (`addiu $3,$0,-0x2F; syscall`). With a register variable, the scheduler
  hoists the load above the prologue's register saves. Fixing this made
  `iWakeupThread` `0x0101C7E0` and `iSuspendThread` `0x0101C8F8` exact (from
  95.3%).
- **Delay slots of conditional branches.** A computation in the delay slot
  that belongs to the taken path was usually computed before the branch in
  the source: reorg filled the slot from above (`inflate_codes`).
- **The `jal` delay slot.** The source order of stack-struct stores before a
  call decides which one fills it. SemaParam written initCount, maxCount,
  option matched `_fio_fd_sema_init` `0x0101E9B0` and `_fio_lock_sema_init`
  `0x0101EEC0`.
- **Tail jumps.** A final call whose value is unused becomes `j callee`,
  with the argument load in the delay slot:
  - in C always;
  - in C++ only under `-fno-exceptions` (Flags).

  `return f(…)` keeps `jal` and a frame. `exec_ps2_with_sbus_cleanup`
  `0x01016F70` is exact as a `void` function with a plain call statement
  (`include/eekernel.h` declares it `void`; its only caller ignores the
  value).
- **`div.s` stays out of delay slots.** Retail never has a `div.s` in a
  delay slot; the pristine 991111b puts one in a `b` slot
  (`startup_screen_thread` `0x01000C88`). In retail, reorg:
  - filled that `b`'s slot from its target (`lui s0,5`), so the `div.s` was
    not eligible;
  - put the `addu v0,a0,s2` of the join into the earlier `beqz`'s slot, which
    it can only do by scanning the fall-through thread past the `div.s` up to
    the `b` and stealing that `b`'s slot (steal_delay_list_from_fallthrough).
    An asm stops that scan, so an asm `div.s` gets the first point and not
    the second (96.7%).

  So the retail `divsf3` was an ordinary insn that is not delay-slot
  eligible: most likely a pattern that emits the two hazard `nop`s itself
  (length 3), which also explains the padding (Assemblers). The patched cc1
  (`tools/patch_divs.py`) makes code 97 return 0 in the four candidate
  switches of `eligible_for_delay` / `eligible_for_annul_false`.
  `startup_screen_thread` became exact, and no other function changed.

### Aliasing

- **Strict aliasing is on at `-O2`.** Polyphony and Sony both built with it:
  - `-fstrict-aliasing` changes nothing;
  - `-fno-strict-aliasing` changes 11 gs functions and drops
    `sceSifAddCmdHandler` to 19%.

  The GCC 14 build uses `-fno-strict-aliasing`.
- **Alias sets.** Every distinct pointer type, and `void *`, has its own.
  `int` and `unsigned` share one. `char` (signed or not) aliases everything.
- **What may reorder:**
  - same-type accesses through two pointers stay in source order;
  - accesses of different types reorder freely;
  - the same base register with different offsets always reorders.
- **Field types show in retail.** Retail's load/store order therefore shows
  the original field types. A search over more than 1,000 type and order
  combinations found that local "view" structs with the same layout
  reproduce them (sifcmd.c: `_request_end` `0x0101DD50`, `_request_rdata`
  `0x0101DE08`, `_request_bind` `0x0101E010`; addresses as `unsigned int`,
  serve/buff as `void *`).
- **Unions.** A union member access, load or store, gets alias set 0, even
  nested (`p->u.call.fno`). It conflicts with every access on another base,
  so it stays in source order. Retail pattern: a run of strictly serial
  `lw`/`sw` in which one or two typed accesses still move. Used for:
  - gs packet stores (`gs_build_image_transfer_packet` `0x0100E0A0`,
    `gs_emit_render_target_regs`);
  - `gs_readback_begin`'s `rec->h.qwc`;
  - `sceSifRpcCallPkt`'s body in sif.h (`_request_call`, `sceSifCallRpc`
    `0x0101E290`).
- **Bytes.**
  - egcs does not move a byte load over a store through another pointer.
    `gs_emit_masked_regs` reads `*regNums` before both stores, which is also
    what the original did.
  - Byte stores through a separate pointer make egcs re-read other fields:
    `sha512_store_bitlen` `0x01003468` re-reads `total_bits` per byte
    (`lbu`/`ld`/`lwu`/`lhu`).
  - Negative offsets from such a pointer show as a base biased by −0x8000
    (`addiu ctx,-0x7F34` with offsets `0x7FFF…`): a hint that the source
    had `p[-k]`.
- **Volatile views.** A macro that reads or writes a plain variable
  through a `volatile` lvalue (`CD_VI(x)`, `*(volatile int *)&x`) keeps that
  access out of delay slots and in source order, without making the
  variable volatile everywhere. libcdvd uses it where retail shows that
  Sony's objects saw the mutex ids as volatile: the gates
  (`_sceCdCheckNCmd`, `sceCdSearchFile`, …) and, since round 3, the two
  `CreateSema` results stored by `_sceCdSemaInit` `0x01013398`, which made
  it exact. The commands and the exit path use the plain variables.
- **`may_alias`.** egcs ignores `__attribute__((may_alias))` with a
  warning, and `BOOT_MAY_ALIAS` expands to nothing there. Where the old
  compiler must see the aliasing, use a union or a `char` access.

### C++

- **Destructors, constructors, typeinfo.** Destructors take an `int`
  in-charge flag and call `__builtin_delete` when bit 0 is set. They are
  emitted as `_$_<class>`, constructors as `__<class>`, typeinfo functions as
  `__tf<class>`. Name the typeinfo function by its exact mangled name in the
  marker. The `__ti` type_info nodes are common symbols.
- **`-fno-exceptions`** (Flags table). It is needed for tail jumps, and it
  removes the object's frame block. It does not change PrintFormat.
  - Retail has no frame block for `src/tex1.cpp`, `src/hostfs_io.cpp` or
    `src/thread_util.cpp`, and `-fno-exceptions` leaves their `.text`
    byte-identical (layout report).
  - Their rules are not in `ref/compile_flags.json` yet.
- **Vtables and type names.** One unit emits all its vtables first and all
  the names after. Retail's interleaving (`vtH nameH vtD nameD vtM nameM`,
  `0x0103E290..0x0103E34C`) therefore means separate objects. The same holds
  for the printf split.
- **Emission order.** egcs emits the `__tf` functions at the end of the
  unit; that is part of `imageloader_source.cpp`'s text drift.
- **Template instances.** Their vtable, `__tf` function, name and
  synthesised destructor are emitted in reverse order of instantiation. A
  `sizeof()` in a header (here a `static_assert`) sets that order. Instances
  come from the unit that instantiates them implicitly: `main.cpp`'s
  `MCardSource_<1,0>` / `<0,0>`, ordered in `include/imageloader.h`.
- **Anonymous namespaces.** Naming and the printf anchor are covered in
  "Translation units and data layout". **Reports disagree:**
  - the pdistd_io verifier called the anchor "an invented global symbol …
    a fake object" and left the PrintTarget RTTI name as unfixable;
  - the layout round adopted it: zero size, no bytes emitted, the RTTI
    string byte-identical.

  The anchor is what the source has now.
- **`this`.** It folds to a frame address in inline members (Inline
  functions).
- **Definition order.** An inline member calling another inline member is
  integrated only if the callee's body was compiled first: define `close()`
  (and the helpers it uses) above the destructor that calls it. With the
  destructor first, `DiskSource::getLoadFlags` scored 81.3% instead of exact.
- **Unnamed members in C.** egcs C rejects unnamed struct/union members
  ("unnamed struct/union that defines no instances"), so union bodies need a
  name (`u.call`).

### Assemblers

- **`div.s` padding.**
  - The retail assembler put two `nop`s before a `div.s` that directly
    follows a COP1 instruction. All three retail `div.s` are padded
    (`0x01000D40`, `0x01000D88`, `0x0100BAF4`); the two `sqrt.s` are not.
  - cc1 emits a plain `div.s`, and none of the bundled assemblers pads it.
    `tools/mcc.py` therefore compiles to `.s`, inserts the `nop`s under
    exactly that rule (`fix_fpu_div_hazard`), and then assembles.
  - It is behaviour-neutral: with the patched cc1 no `div.s` is ever in a
    delay slot (Scheduling).
  - The retail compiler most likely emitted these `nop`s itself; the pass
    reproduces them for the three `div.s` that exist.
- **GNU as misreads loops after alignment (`label_frags`).** The bundled
  GNU as 2.9-ee pads a backward branch that closes an R5900 short loop (six
  instructions or fewer) with four `nop`s. It measures the loop with
  frag-relative addresses, and egcs puts a `.p2align 3` right after a loop
  label, which ends a frag. A branch back to a label six or more
  instructions into such a frag is therefore taken for a short loop:
  `BigNum::gcd` `0x01005BC0` closes a 317-instruction loop and got four
  `nop`s. Retail has the same layout and no padding.
  - `tools/mcc.py` writes a `.p2align 2` (no bytes: the code is
    word-aligned) before every `$L<n>:` label, which starts a new frag there.
    `BigNum::gcd` became exact; no other `src/` or `sdk/` function changed.
  - egcs pads real short loops itself with explicit `nop`s (Basics), so
    nothing depends on the assembler's check.
  - Skipped for files assembled by SN's assembler (below).
- **Other toolchains tried, per file.** A `ref/compile_flags.json` rule can
  name another compiler (`"cc": "<folder under sdk/>"`, C/C++ to `.s` only)
  or SN Systems' assembler (`"as": "sn"`, ps2eeas 1.9.25 from the bundle's
  `ee/bin/Ps2EeAs.exe`). No rule uses either:
  - SN's ee-gcc 2.95.2 (decomp.me's `ee-gcc2.95.2-273a`) saves registers with
    `sq` and orders prologues differently;
  - ps2eeas breaks many exact functions, so it was not retail's assembler;
  - the other decomp.me EE builds (990721, 991111, 991111a, 991111-01) are
    Linux executables and cannot run here.
- **The Sony SDK assembler swaps delay slots.** When reorg cannot fill a
  branch's delay slot, egcs leaves the branch in `.set reorder` mode.
  - The bundled ee-as 2.9-ee-991111b then always appends a `nop`.
  - The retail SDK code (libkernl, libcdvd) instead shows standard GAS
    moving the previous instruction into the slot.
  - Polyphony's code does not (`gfx_reset_paths` `0x0100BFE8` is
    `sw; jr; nop`). Applied to `src/`, the swap breaks 9 exact functions.

  `tools/sdkas.py` emulates the swap. `tools/mcc.py` runs it on files whose
  `ref/compile_flags.json` rule has `"asm_pass": "gas_swap"`. It is set on the
  `sdk/*` rule, and must never be set on `src/*`. Its rule:
  - the previous instruction, and the one before it, were assembled in
    reorder mode; a label resets that history, and a label on the branch
    forbids the swap;
  - the previous instruction is a single machine instruction: not a branch,
    jump, trap, sync, COP0/COP1 move or compare, and not a load (never seen
    moved);
  - one exception: a store to an absolute address (`sw $2,sym` under `-G0`,
    or a constant beyond 16 bits). That is the macro
    `lui $at,%hi(a); sw $2,%lo(a)($at)`, and GAS moves its final `sw`.
    `wait_vblank_start` `0x01022C50` is retail `lui $at; jr; sw`. Other
    macros (`la`, `li` of 32-bit values) are never moved;
  - it writes no register the branch reads (`$at` included, for the
    exception), and does not touch `$31` when the branch links;
  - branch-likely instructions never take a swapped instruction;
  - volatile stores are moved.

  Evidence: it makes these exact:
  - `sio_putc` `0x0101C990`;
  - `_dtoi` `0x0101CAB0`;
  - `DIntr` `0x01022B68`;
  - `wait_vblank_start`;
  - `_sceCdCallbackThread`.

  It also makes `scePrintf` exact with data. `sdk/` goes from 233 to 238 exact
  of 243, and no `sdk/` function gets worse. This settles an earlier
  disagreement between reports about macros.
- **`dli`.** The bundled GAS expands `dli 0x3800000000000000` as
  `ori 0xE000; dsll32 14`. Retail's `addiu 0x380; dsll32 20` came from an RTL
  shift, not from the macro. **Open:** PrintFormat's `%F` conversion.
- **Absolute addresses.** GCSE cprop can turn a constant address into
  `lw $2,0x1000F000`. The assembler expands that to
  `lui $2,0x1001; lw $2,-0x1000($2)`; stores go through `$at`.
  - A local `volatile u32 *p = &REG;` used in one block gives the register
    form for the first access and the macro form for accesses in later
    blocks (`wait_vblank_start`, `wait_vblank_get_csr`).
  - `REG32(addr)` everywhere gives `lui`/`ori` into a register, which loop.c
    hoists out of loops.
- **`not`.** egcs never emits it (Front end).
- **GP0.** The old assembler writes GP0 = `0x4000` into the `.reginfo` of
  every object that has small data (0 otherwise). A `%gp_rel` reference to a
  *section* symbol then carries that bias in its in-place addend: objdump
  shows `lw s3,-16384(gp)  R_MIPS_GPREL16 .sdata+0x4000` for offset 0.
  `tools/dataref.py` corrects for it. References to named symbols are not
  biased.
- **Sections.** The old assembler has no `.pushsection`/`.popsection` but
  accepts `.previous`.

### Data layout

The tools and the evidence are in "Translation units and data layout".
Compiler facts:

- **Initialisers.** Explicitly initialised variables, `= 0` included, go to
  `.data`/`.sdata`; only uninitialised ones go to `.bss`.
  - cc1plus writes an initialised namespace-scope variable where it is
    defined, and an uninitialised one at the end of the unit.
  - `u8 g_010384D8[0x1000] = { 0 };` keeps retail's order in front of
    `s_gsDmaChan`.
- **C file scope.** Uninitialised statics are emitted as
  `.section .bss; label: .align; .space`, in definition order. Globals
  become `.comm` and are placed after every object.
  - `__attribute__((section(".bss")))` is ignored on arrays and on
    variables redeclared from an extern. Where it does apply, it emits
    `.section .bss,"aw",@progbits`. It is not a placement lever.
  - Retail `fileio.o`'s shared buffers were statics of one unit (merging
    `filedev.c` into `fileio.c` is open).
- **`const`.** A `const` table goes to `.rodata`. Retail `.data` means the
  original was writable (gs's register tables).
- **Jump tables align `.rodata`.** An object whose `.rodata` holds a jump
  table gets `.align 4`, so the link 16-aligns that object's `.rodata`.
  - In a file that merges several objects, restore the gap at the boundary
    with `__asm__("\t.section\t.rodata\n\t.align\t4\n\t.previous\n")`.
  - kernel.c does this before `sio_putc`, which fixed `_print_double`
    `0x0101CB40`'s 7 data references.
  - GCC 14 keeps its own section state across it.
- **Frame blocks.** Every C++ object compiled with exceptions ends its
  `.data` with its own frame block, including data-only objects.

### Inline asm and MMI

Compiled functions are matched in C. Asm is used only in two cases:

- whole retail functions that are demonstrably hand-written ("One source
  tree, two compilers"; e.g. libc's MMI routines, `intr_suspend`,
  `vu0_load_identity_regs`);
- single instructions that egcs cannot emit, or cannot emit in retail's
  context, wrapped in a helper with a C fallback.

Rules for both:

- guard with `#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)`, with the
  C under `#else`;
- declare every register the asm writes;
- make single-instruction helpers non-volatile, so identical ones are CSE'd
  as retail's are.

Accepted helpers:

| Helper | Where | Emits | Evidence |
|---|---|---|---|
| `ee_not64`, `ee_not32` | `src/gs_texture.cpp`, `src/gs_driver.cpp` | `not` | egcs's one_cmpl is `nor rd, $zero, rs`; retail `not` at `0x0100BED0` (`gfx_set_texture_regs`) and `0x0100ED80` (`gs_set_bg_color`). Kept non-volatile: a volatile `ee_not32` scored 77.3% instead of 76.1% in `gs_set_bg_color`, but would be inconsistent with `ee_not64` |
| `ee_maxf`, `ee_minf` (not asm since round 3) | `include/gs_internal.h` | `max.s`, `min.s` | the C++ operators `>?` / `<?` (Front end); used by `gs_set_material_param` and by `pack_rgba_floats_to_u32`'s C model |
| `ee_absf` | `src/tex1.cpp` | `abs.s` | `__builtin_fabsf` also gives `abs.s`, but only an asm stops duplicate_loop_exit_test from copying the loop test, as retail shows; the second `abs.s` is CSE'd away as in retail (`clut_anim_step`) |
| `EE_MAXF` (not asm) | `src/tex1.cpp` | `max.s` | the C++ `>?` operator (`clut_anim_step`) |
| `GS_SPR_CTX` | `include/gs_internal.h` | `li 0x70003000` | retail never folds the scratchpad base into field offsets: every function loads it whole at its start, and a wrapper that only passes it on does not tail-call |
| `CLUT_PEXTLW`, `CLUT_BLEND` | `src/tex1.cpp` | `pextlw`; `pextlb`/`pextlh`/`pmulth`/`pmaddh`/`psrlw`/`ppach`/`ppacb` | cc1plus knows only `por`/`lq`/`sq` for TImode and has no templates for the rest. `CLUT_BLEND` clobbers `"hi", "lo"`. `clut_anim_channel_apply` `0x0100B6D0` is still 75.5% (open) |
| `_iGetThreadId` | `sdk/libkernl/kernel.c` | `addiu $3,$0,-0x2F; syscall` | the number load must sit inside the asm (Scheduling) |

- **max/min (settled in round 3).** `ee_maxf` / `ee_minf` are the `>?` /
  `<?` operators now, not asm. `gs_set_material_param` `0x0100EFF0` went
  from 92.7% to 96.8% (with the conversion in its own variable,
  `f32 raw = (f32)value;`); no other function changed.
  - **Open:** retail loads 128.0 before 0.0 and gives 128.0 `$f0`. sched1
    ranks the 0.0 load higher (it feeds max, then min). An asm max (the
    scheduler charges no latency into an asm) with an RTL min gets the
    load order but not the registers. Brute-forced: 63 combinations of
    operator / ternary / template / reference / asm helpers and call
    shapes.

  The GCC 14 fallback `a > b ? a : b` differs from `max.s` only for −0.0
  (or a negative denormal) against 0. Which result is right is undetermined: eeemu's `max.s` returns `fs` on a
  tie, and PCSX2 compares sign-magnitude integers.
- **Open policy question.** `pack_rgba_floats_to_u32` `0x0100EDB0` keeps its
  float part as one asm block, on the claim that it is hand-written. The
  asm hard-codes `$f12..$f16` and changes inputs it does not declare; it
  works only because the arguments stay in place, as the SCE output shows.
  The gs verifier flagged it as a policy decision.
- **`gs_set_bg_color` MMI asm.** Its `ppacb` output is tied to its input.
  Neither a fully hand-written block nor 48 shape/mask combinations reached
  retail (76.1%, open).

## Open functions

After the final pass: 851 of 881 exact. Of the 30 others, 9 are runtime
functions that are not byte-identical in the toolchain's own `libgcc.a` /
`libstdc++.a` (`ref/libmatch.json`) and are not compiled for matching. The
21 below compile and behave the same (difftest, boot). "Registers" counts
instructions that differ only in register numbers; "moved" counts
instructions present on one side only (order or count).

All of them were checked with the reference compiler (`score.py --cc
991111-01`). For all but two it produces byte-identical objects, so what is
left is in the source. The two exceptions are hash-ordered spill slots
(marked "ref" below). The bootstrap is considered done at this point; the
table is what a further round would start from.

| Function | Address | Bytes | Registers / moved | What decides it (see Compiler notes) |
|---|---|---|---|---|
| `pdistd::PrintFormat` | `0x01010368` | 91.4% | many / few | global-alloc rotation of `fixedPoint`/`base`/`sval` (a1/a2/a3); CSE keeps `prec` canonical where retail compares against it. Round 3: `len = (i < prec) ? prec : i;` made the size equal (922) |
| `ImageLoader::MCardSource::open` | `0x01005458` | 86.6% | | was 82.6%: the RAII shape (Inline functions) and the dir entry pointer taken before `GetPath()`. Left: retail reuses `dir`'s stack slot for `f` (sp+0x10, ours sp+0x20) and computes `&dir` after `memalign` with a cleanup copy |
| `ImageLoader::MCardSource::getLoadFlags` | `0x01005748` | 87.7% | | was 81.2%: the RAII shape. Left: retail's success-path cleanup recomputes `&f` and is cross-jumped with the landing pad's copy; five return/flag forms tried |
| `sceCdInit` | `0x01013E70` | 90.2% | 35 / 14 | register allocation and order |
| `GZIP::InflateBase::inflate_codes` | `0x01011E40` | 91.5% | 18 / 22 | local-alloc follows sched1 order (Register allocation) |
| `scePadPortOpen` | `0x010147A0` | 89.5% | 10 / 15 | five pointer copies; alias order of the table stores |
| `GZIP::InflateBase::inflate_main` | `0x01012180` | 98.5% | 36 / 4 | PRE spill-slot permutation `0x598..0x5B0`; ref 98.0%. All 24 orders of `j, l, n, nb`, `td_` first, and swaps of `type`/`i`/`last` change neither compiler: the order comes from GCSE's hash table (size = instructions / 2, keys include the `this`-relative addresses), so it needs the original statement shape |
| `clut_anim_channel_apply` | `0x0100B6D0` | 75.5% | 19 / 7 | the MMI asm helpers (`CLUT_BLEND`) and loop shape |
| `GZIP::InflateBase::huft_build` | `0x01011838` | 97.6% | 6 / 14 | statement order (720 orders of six statements searched) |
| `sha512_transform` | `0x01001320` | 99.6% | 26 / 0 | PRE spill-slot permutation of 8 slots; ref 99.9%: one swapped pair, `0xE0`/`0xE8` (`state[0]`, `W[1]`) |
| `gs_draw_textured_sprite` | `0x0100E2A0` | 91.0% | 20 / 2 | luid order around the volatile packet stores |
| `gs_set_bg_color` | `0x0100ED58` | 76.1% | | alpha's register (copy preference pruned, Register allocation) |
| `gs_build_clear_packet` | `0x0100DD10` | 94.5% | 6 / 4 | allocation and order of the packet stores |
| `_sif_cmd_intr_handler` | `0x0101D878` | 93.6% | | global-alloc order (retail: `hdr` in `$a3`, `size` in `$a1`) |
| `gs_emit_render_target_regs` | `0x0100DF40` | 96.0% | 5 / 4 | the checked division is a scheduling barrier |
| `clut_anim_step` | `0x0100B808` | 98.5% | | sched2 puts `add.s` straight after its `lwc1` in retail, which changes the `b` slot |
| `gs_readback_begin` | `0x0100EB00` | 91.7% | 10 / 0 | registers only |
| `gs_set_material_param` | `0x0100EFF0` | 96.8% | | the order of the 128.0 / 0.0 loads (Inline asm, max/min) |
| `gs_emit_masked_regs` | `0x0100EE48` | 86.5% | 3 / 2 | sched1 pairs the load with an ALU op (Scheduling) |
| `gs_readback_end` | `0x0100EB78` | 97.6% | 0 / 2 | one store order |
| `pack_rgba_floats_to_u32` | `0x0100EDB0` | 98.7% | 2 / 0 | the `or` ties after the asm block; the C form uses `$at` constants and is far off (44%) |

The final pass made `DiskSource::open` and `DiskSource::getLoadFlags` exact
(the RAII shape). Round 3 also made these exact: `exec_ps2_with_sbus_cleanup` (`void`),
`mc_init_once` and the new `mc_init_once_unlock` (a missed function start,
now in `ref/extra_starts.txt` and `ref/map_notes.md`), `nop_padding` (the
scorer now trims trailing alignment nops on the C side too),
`_sceCdSemaInit` (volatile views), `BigNum::gcd` (the assembler label pass),
`sceIoctl` (the documented fakematch) and `startup_screen_thread` (the
patched cc1).

## Function-level behaviour tests

`tools/difftest.py` runs a retail function and its rebuilt counterpart in the
EE interpreter `tools/eeemu.py` on the same inputs and compares what they do.
It tests behaviour, not bytes, so it also covers functions that do not match
yet.

```sh
python tools/difftest.py --elf matching/build/scus_971.02.elf   # byte-identical build: must be all PASS
python tools/build.py --compiler sce --build-dir build/sce-mine  # your own SCE image (never build/ itself)
python tools/difftest.py --elf build/sce-mine/scus_971.02.elf    # every spec
python tools/build.py --build-dir build/gcc14-mine --no-install  # your own GCC 14 image: the code that boots
python tools/difftest.py --elf build/gcc14-mine/scus_971.02.elf  # adapted to its ABI ("The GCC 14 build")
python tools/difftest.py --elf ... --group bignum                # one group (repeatable)
python tools/difftest.py --elf ... --fn modInverse --fn 0x01011700   # specs calling these (or spec names)
python tools/difftest.py --elf ... --fn vprintf --case 50        # one case, numbered as FAIL prints it
python tools/difftest.py --elf ... --fn gs_init --case 3 --show  # every event of that case (retail side)
python tools/difftest.py --list [-v] [--group G]                 # groups, specs, functions (-v: uncovered ones)
```

Other options: `--n N` scales every spec's case count (200 = as written),
`--seed S` draws other inputs, `--keep-going -v` lists every failing case,
`--reach` lists the functions a spec names that no case executed, and
`--no-selftest` skips the quick `tools/eeemu_selftest.py` run. The sources
change while you test, so rebuild first: a result describes the image it ran
on.

### What is compared

For each case, on both sides:

- **Return values**, at the width the spec declares.
- **Memory written outside the stack.** Test buffers, the stub heap and the
  scratchpad are compared by address, since they sit at the same place on both
  sides. When the images differ, writes into the image's `.data`/`.bss` are
  compared by symbol (see "Image addresses").
- **Calls to stubbed functions**, in order, with their arguments. The
  allocator (`malloc`/`free`/`calloc`/`realloc`/`memalign`,
  `operator new`/`delete` and their `[]` forms) is a deterministic Python heap
  on both sides, and `scePrintf` is captured. Specs can stub any other callee
  (`t.stub`) and the kernel (`t.syscalls`, normally through `common.Kernel`).
- **Hardware events**, in program order with the calls: every MMIO load and
  store (address, width, value), a snapshot of every DMA transfer started,
  COP0 accesses, `di`/`ei`/`eret`/`sync`, every `CACHE` op with its address,
  VU0 control-register accesses and `vcallms` (see "The EE model").
- **Values a spec records** with `t.obs`.
- **Errors.** An `EmuError` (bad instruction, misaligned or wild access,
  runaway loop, a callee-saved register not restored, heap misuse, a fake-kernel
  deadlock) on one side only is a failure. The same error on both sides is the
  same behaviour if:
  - the error class matches;
  - for a fault, the faulting address matches like a pointer (never the pc);
  - the events before it match (only as a common prefix for a runaway, whose
    cut-off depends on the instruction count).

  Memory is not compared after an error. A write above the caller's stack
  pointer always fails.

The `t.check` oracles (hashlib, zlib, Python big integers, an exact model of
`BigNumRep`, C-library semantics, the Tex1 rebase and CLUT blend) run on the
retail side only. They model the retail code, quirks included, and check the
generators and the emulator, not the rebuilt code.

### Results

| Status | Meaning |
|---|---|
| PASS | every case agreed |
| FAIL | a case differs; the first difference is printed (events, a memory hexdump or the two errors) |
| ERROR | a harness problem: a retail oracle failed, or a spec raised an exception. Fix the spec, not the C |
| MISSING | a function the spec calls has no symbol in the rebuilt ELF (or its names disagree) |
| SKIP | GCC 14 build only: every case needs a function or static GCC did not emit (see "The GCC 14 build"); with some cases skipped, PASS / FAIL is followed by `N case(s) skipped: …` (`-v`: every reason) |
| `HARNESS:` | before any spec runs: the symbol mapping is wrong on the identical build, two retail functions resolve to one rebuilt function, or an allocator stub could not be installed |

On `matching/build/scus_971.02.elf` every spec must PASS; anything else is a
harness or spec bug. On an SCE build, a FAIL is a behaviour difference of the
C, with one exception. A FAIL followed by `note: … read never-written stack
byte(s)` may come from an uninitialised value that the original also reads, as
in the `%#.0f` case below. Such a value depends on the frame layout, not on
the code. Check whether the differing output comes from that read. On the
GCC 14 build the same holds, plus the GCC 14 differences listed at the end of
"The GCC 14 build" (a divide-by-zero FAIL names its own).

### Rebuilt symbols

Rebuilt functions are found under every name known for the retail address:

- `matching/symbol_addrs.txt`;
- `build/score/symmap.json` (written by `tools/score.py`; it can be stale);
- unambiguous `ref/libmatch.json` names;
- plain identifiers of the FUNC marker - not a part of a qualified name
  (`ImageLoader::HostSource::open` is not the C function `open`) and not a
  word in parentheses;
- the GNU v2 demangled qualified name of the marker;
- entry points inside a merged function, written in a `.S` file as
  `//   0x0101BC40  -0x46  iPollSema` followed by `ASTUB(iPollSema, …)`.

All names found must agree on one address.

### Image addresses

When the rebuilt image is byte-identical to retail, addresses are compared as
they are. Otherwise an image address is compared through a token:

- **a function**: its retail address, through the symbol mapping;
- **a global**: (C name, offset), from the `// @0x…` annotation on its
  definition (also a declaration whose `;` is on a following line, and a
  `.S` data label `name: .space N /* @0x… */`), the `link/*.S` blob table, and
  the rebuilt symbol table (the exact name wins over decorated ones such as
  egcs's `_GLOBAL_$F$name` frame label);
- **a type_info node** (`__ti<type>`, unannotated): retail's address is what
  its `__tf<type>` function returns, run once on the pristine image when the
  harness starts; the rebuilt one is its `__ti` symbol;
- **a vtable**: the list of its slots' functions, up to the next typeinfo
  function (the next vtable's slot 0);
- **a string**: its bytes;
- **one past the end** of an object: the same object's last byte plus one;
- the program's `_gp`; segment bits (kseg0 `0x8…`, uncached `0x2…`/`0x3…`, a
  DMA address's scratchpad bit) are looked through.

Linker-script symbols that are not objects (`_fbss`, `_end`, `_stack`,
`_heap_size` …) have no token: a spec reads its side's value with `t.sym` and
compares what the code does with it (see `gf_start`).

### The GCC 14 build

The installed image is the GCC 14 one, so its behaviour is the one that
boots. Its ELF is n32 (`EF_MIPS_ABI2` in `e_flags`) with Itanium C++ names,
and GCC 14 optimises across functions (`-fipa-ra`, inlining, clones). The
harness then prints `note: n32 image (…)` and adapts every place where the
calling convention or the C++ ABI would make equal behaviour look different
(`Harness.adapt`). Nothing of this is active for the byte-identical and SCE
images, which are EABI and GNU v2.

- **Arguments.** EABI numbers integer and float arguments separately (`$4..`,
  `$f12..`); n32 gives each argument a position (`$4+k` or `$f12+k`), a
  16-byte aligned value (u128, `GsVec4`) an even pair of slots, and positions
  8 and up 8-byte stack slots (as EABI). An n32 caller extends its arguments:
  32-bit values sign-extended to 64 bits, `u8`/`s8`/`u16`/`s16` from their
  width. `t.call(f, ints…, floats=(bits…))` takes the function's prototype
  from the source (the definition after its FUNC marker, else a declaration;
  the GCC 14 side of `#if BOOT_OLD_GCC`, typedefs followed, `this` and a
  returned object's address added) and passes the arguments as a C caller of
  that prototype, on both sides, each in its own convention. Without a
  prototype the integers go as before, and floats next to integers are a
  harness error. A `va_list` needs nothing: both pass a pointer to 8-byte
  slots.
- **Stubs.** `t.stub` decodes the intercepted call the same way (integer
  arguments as `args`, floats through `t.float_args()`) and extends its
  return value to the declared type. A function GCC 14 inlined into its
  caller never reaches a stub: `t.stub_reaches(caller, callee)` checks the
  caller for a `jal`/`j` (`busy_wait_delay` in `yield_low_priority`).
- **Caller-saved registers.** A stub normally clobbers them all afterwards.
  With `-fipa-ra`, GCC keeps values in the caller-saved registers that a
  same-unit callee is known not to touch (`sha512_load_be64`). So when the
  stubbed function and the code `$ra` returns to are in the same object file
  (the link map next to the ELF; without a map every stub counts), nothing is
  clobbered and a void stub leaves `$v0` alone. Calls across units keep the
  clobbering: there GCC assumes the full ABI.
- **Constructors and destructors.** GNU v2 constructors return `this` and
  Itanium ones nothing: the return is recorded as `this` (or the retail value
  when it is not `this`), and the spec gets `this`. A GNU v2 destructor call
  (this, in-charge flags) becomes the complete-object destructor (D1); with
  `flags & 1` the deleting one (D0), or D1 then `operator delete` for a class
  without a virtual destructor. A stubbed destructor sees flags 2 for D1 and 3
  for D0. Before a destructor call the object is snapshot and compared as it
  was then: GCC 14 drops stores into a dying object (`-flifetime-dse`:
  `~Allocator` clearing its members, every destructor's vptr store).
- **Polymorphic objects.** GCC 2.9x places the vptr after the data members of
  the class that introduces virtual functions; Itanium puts it at +0 and those
  members after it (`common.LAYOUT`: FileName +8, Source +0xC, Allocator +0x14,
  InflateBase +0x3030 with 8-aligned members at +8). A spec registers every
  object the comparison sees with `t.obj(this, layout)`, also one the code
  made, and addresses it by GNU v2 offsets (`t.m`, `t.objwrite`,
  `t.objread`, `t.objwords`, `t.objput`, `t.halloc_obj`); its memory and
  pointers into it then compare at the GNU v2 places. `t.place` /
  `t.alloc_place` keep the members at one address on both sides instead (the
  gzip arena, whose table pointers are compared as they are; `this` is then 8
  lower in the GCC 14 image). A vtable built by a spec is `t.vtable([slot 1,
  slot 2, …])`: GNU v2 `{delta, index, pfn}` slots, or an Itanium table (D1
  and D0 for the destructor slot, a callback there seeing flags 2 / 3; a delta
  becomes a thunk). Two vtable pointers compare equal when their tables list
  the same functions from the address point on; a D1 followed by its own D0
  counts as the one GNU v2 destructor slot, a D0 elsewhere (a vptr one entry
  off) as a slot of its own.
- **Not emitted.** GCC 14 has no symbol for a static or inline function it
  inlined everywhere or dropped, for one it only emits as a clone with another
  signature (`_kernel_vprintf.isra.0`), or for a GNU v2 typeinfo function
  (`__tf…`, `-fno-rtti`). A case that calls or stubs such a function is
  skipped and counted, and so is a case that needs a static GCC dropped or
  folded into its readers (`s_gsTexOverride`, `s_heapStart`).
  `t.unwritten_static(name)` skips a case that gives a static the program
  never writes another value (GCC folds `s_startupImageSeq[0]`). Writes into
  a static GCC dropped because nothing reads it are not compared. That needs
  the GCC 14 side of the source to define the static under the same name: a
  global that only the `BOOT_OLD_GCC` branch defines (`s_allocator`,
  `s_mc_base_dir_obj` and `s_host_dir_obj`, which the GCC 14 branch keeps as
  raw storage under other names) is not dropped, and a write to it fails as
  "on one side only" (no spec writes them on the GCC 14 build yet).
- **Cache maintenance.** How many D-cache lines a buffer write-back touches,
  and where, follows the buffer's alignment. A run of `CACHE`/`SYNC` events is
  compared as one: the `SYNC` kinds in it, other lines exactly, stack lines
  only by whether there are any, image lines by the object bytes they cover.
  A block of lines `[lo, hi)` comes from a range that starts less than 64
  bytes after `lo` and ends less than 64 bytes before `hi`, so the bytes of
  `[lo + 63, hi - 63)` were asked for: the other side's lines must cover
  them too (as image object bytes). A write-back cut short by 64 bytes or
  more fails.
- **Names.** Itanium functions and data are demangled with the toolchain's
  `c++filt`; a destructor marker's GNU v2 `(int)` is ignored, class template
  instances match by their integer arguments, and the egcs name in
  `build/score/symmap.json` wins over the marker text.

What remains are differences of GCC 14's code, and one spec bug; none is a
harness problem:

| Difference | Where | Status |
|---|---|---|
| Functions and statics GCC 14 does not emit (see "Not emitted") | SKIP: `gz_typeinfo`, `printtarget`, `filename`, `mc_sync`, `heap_frontend`, `pdistd_typeinfo`, `ks_doubles`, `ks_dtoi_edges`; skipped cases in 41 more specs (in `gs` about a third, the `s_gsTexOverride` cases) | tested through their callers; the C heap front-ends (`s_heapStart` folded) have no GCC 14 coverage |
| A decimal float constant is converted by truncation (GCC's R5900 float format): `0.2f` = `0x3E4CCCCC`, retail `0x3E4CCCCD` | `gfx_init_lights`, `gfx_init_material` (0.2, 0.8); `gs_set_material_param` (`1.0f / 255.0f`) | fixed: exact hex constants in the GCC 14 branch, egcs output unchanged |
| Divide-by-zero trap `teq` instead of `break 7` | `gs_display` cases with a zero divisor | documented in "Keeping behaviour"; the FAIL says so |
| Stores into an object whose destructor runs are dropped | every destructor | compared before the destructor |
| Merged stores also write padding | `SourceRecord` +2 (after the `u16`) | not compared |
| An unpassed variadic argument: `sceOpen`'s `mode` is the caller's `$a2` | the boot sources call `sceOpen(path, flags)` | not compared (`Kernel.open_mode_unknown`) |
| A short read leaves stack garbage: `HostSource::getLoadFlags` returns it | `gf_sources` | compared only after a full read |
| Globals adjacent in retail are placed independently (`s_poff_arg` before `s_poff_func`, then `s_cd_user_cb`) | `sonyio.py` `poff_notify` sets both with one 8-byte write, which hits `s_cd_user_cb` | spec bug for its owner: two `t.w32` |

### The EE model (`tools/eeemu.py`)

- **CPU.** The integer ISA with 128-bit GPRs, every MMI instruction and the
  SA register, COP1 single precision with the EE's arithmetic (round toward
  zero, no denormals, no Inf/NaN, overflow clamped). Division by zero, traps
  and unmodelled instructions (TLB, BC0x/BC2x, VCLIP, the VU random ops,
  `rsqrt.s`, `break`, a `syscall` without a hook) are `EmuError`s.
  `tools/eeemu_selftest.py` checks the MMI and COP2 decoders against
  rabbitizer, the MMI semantics against an independent model, the memory map,
  the DMA engine, COP0/COP2, and runs retail routines against oracles.
- **Memory map** (virtual address -> what it is):

  | Address | Model |
  |---|---|
  | `0x00000000..0x01FFFFFF` | 32 MB RAM (kuseg) |
  | `0x20000000..`, `0x30000000..` | the uncached / uncached-accelerated RAM mirrors |
  | `0x70000000..0x70003FFF` | the 16 KB scratchpad (its own memory) |
  | `0x80000000..0xBFFFFFFF` | kseg0/kseg1: physical = address & `0x1FFFFFFF` |
  | `0x10000000..0x1000FFFF` | EE registers (timers, IPU, GIF, VIF0/1, FIFOs, DMAC, INTC, SIO, SIF): MMIO |
  | `0x11000000..0x1100FFFF` | VU0/VU1 micro and data memory: MMIO |
  | `0x12000000..0x12001FFF` | GS privileged registers: MMIO |
  | `0x1FC00000..0x1FFFFFFF` | BIOS ROM: MMIO |

  Anything else is a `range` error. The test layout: images at
  `0x01000000`, spec inputs from `0x01100000`, the stub heap from
  `0x01800000`, fake code addresses for callbacks at `0x01E00000`, the stack
  below `0x01FFE000`. Inputs, heap, stack and scratchpad start every case as
  an address-dependent pattern.
- **MMIO.** An MMIO load or store is not memory: it is an event
  `('hw', ('r'|'w', physical address, width), value, count)` and the value a
  load returns comes from the register model. Identical consecutive reads
  merge into one event with a count, so a polling loop stays one event while
  the number of polls is still compared. Byte-range copies of MMIO are refused.
- **Register model.** By default a register reads what was last stored there
  (latched per byte, never-written bytes read their reset value), with "idle"
  rules: `Dn_CHCR` reads with STR clear (every DMA has finished), `GS_CSR`
  reads `0x551B4008` (VSINT set, FIFO empty) and `INTC_STAT` reads 0, so a
  VIF/GIF STAT reads 0 (idle) and VPU-STAT reads 0 (VUs stopped). A spec
  overrides a register with `t.reg(addr, read=…, write=…)`: a value, a list of
  successive values (the last repeats), or a function of the read count and
  the latched value; `write` sees every store.
- **DMA.** A store that sets `Dn_CHCR.STR` snapshots the transfer from memory
  at that moment: normal mode from MADR/QWC, or the source chain from TADR
  (`cnt next ref refs refe call ret end`, ASR stack, an IRQ tag with TIE stops
  it, at most 4096 tags / 1 MB) as tag and data items ending with why it
  ended; a peripheral-to-memory transfer is just (MADR, QWC). DMA addresses
  are physical, bit 31 = scratchpad. Tag addresses compare like pointers,
  payload words compare by value or as the same pointer. Nothing is
  delivered anywhere: the snapshot is what the hardware would read.
- **COP0.** MFC0/MTC0 (including the performance counters MFPS/MTPS/MFPC/MTPC
  and the debug registers), DI/EI (clear/set Status.EIE), ERET (to EPC or
  ErrorEPC; both start at the harness return address), SYNC.L/SYNC.P and
  CACHE are events. Count advances by `0x1000` per read, never per
  instruction. A CACHE index-load-tag sets TagLo from `t.cache_tags`
  (default 0: no valid line). A case starts as a user thread of the kernel
  (Status `0x70030C13`).
- **COP2 / VU0 macro mode.** QMFC2/QMTC2/CFC2/CTC2, LQC2/SQC2 and the vector
  instructions run on a VU0 register file (vf, vi, ACC, Q, I) with the EE
  float model. The MAC/status/clipping flags are not computed. VU0 data
  memory accesses (`vlqi`/`vsqi`/`vilwr`/`viswr`…) are MMIO events at
  `0x11004000`; the hardware control registers (vi26-31: TPC, CMSAR0, FBRST,
  VPU-STAT, CMSAR1) are events answered by the register model (`t.reg('vi29',
  …)`); `vcallms`/`vcallmsr` are events and no micro program runs.
- **No** threads, interrupts or timing of its own: a spec runs interrupt
  handlers explicitly (`t.interrupt`), usually from the fake kernel.

### The fake kernel and the model IOP (`tools/difftest_specs/common.py`)

- **`Kernel`** (installed with `t.syscalls`): every kernel call is logged with
  the arguments it takes (`SYS_ARGC`) and its result. It returns increasing
  ids for semaphores, threads and handlers; counts the semaphores it created
  like the EE kernel (WaitSema on zero calls `on_block`, then fires pending
  alarms, else raises a `blocked` error on both sides); records the parameter
  blocks of CreateSema/CreateThread and the transfer lists of `sceSifSetDma`
  (with the words each element sends) as observations; keeps SIF registers
  (SMFLAG acknowledged by writing 1s); runs `sleep(t, k)` at the k-th
  SleepThread (what interrupt handlers would do meanwhile) and alarm handlers
  when the thread waits. `ret={number: value or fn}` overrides any call.
- **`Iop`** (`sif_up(t, iop)` brings SIF up with the real `sceSifInitRpc`):
  the IOP side of the SIF command layer. Every command packet the EE sends
  with `sceSifSetDma` is decoded; answers are written into the EE's SIF0
  receive buffer and the EE's own `_sif_cmd_intr_handler` runs, right after
  the DMA or (`defer=True`) only when the EE waits. It answers INIT (the RPC
  ready flag), RESET (the IOP reboots: SMFLAG comes back two polls later),
  RPC_BIND (every server sid, others with `bind_unknown`) and RPC_CALL (the
  server's reply written to the receive buffer; END packet or the in-use bit
  cleared). Server models, `fn(t, fno, send_words, rsize, iop) -> reply`:
  - FILEIO `0x80000001` (`kernl_rest.FileioServer`): setup, every call with
    its completion block DMAd back and command `0x80000011`;
  - LOADFILE `0x80000006` (`kernl_rest.LoadfileServer`); the DECI2 manager is
    a Deci2Call model (`kernl_rest.Deci2`);
  - libcdvd `0x80000592..` (`sonyio.CdServers`, plus the power-off command
    `0x80000012`), padman `0x80000100/101` (`sonyio.PadServer`, with the pad
    DMA areas), mcserv `0x80000400` (`sonyio.McServer`: versions, end
    parameters, deferred calls).
- **`printf_stub`** compares the formatted text of the pdistd `printf`
  instead of its argument registers.
- **`gs.World`** (`tools/difftest_specs/gs.py`) is a whole GS driver state: a
  GsContext in scratchpad, `g_gfx`, the packet buffer and a Kernel.

### Adding a spec

The specs are in `tools/difftest_specs/`, one module per group. Every module
except `common.py` is discovered automatically. The module contract and the
whole `T` API are in `tools/difftest_specs/__init__.py`.

| Group | Units | Owner |
|---|---|---|
| `libc` | libc | verifier |
| `sha512` | sha512 | verifier |
| `bignum` | bignum, montgomery | harness |
| `gzip` | gzip (including modpow 0x0100B238, in `src/gzip_montgomery_tail.cpp`) | verifier |
| `pdistd_io` | printf, allocator, filename, io | verifier |
| `gameflow` | main, startup, imageloader | harness |
| `gs` | gs | harness |
| `kernl_sys` | kernl_sys | harness |
| `kernl_rest` | kernl_rpc_fio, kernl_rest | harness |
| `sonyio` | libcdvd, libpad, libmc | harness |

1. Write `gen(rng, i, n)` returning `Case(desc, run)`. Draw every random value
   in `gen`. `run(t)` is replayed once per side and must not use `rng`.
2. In `run`, set the world up (inputs, a `Kernel`, an `Iop`, register models,
   stubs), call, and record what the caller can see.
3. Append `Spec(name, [retail addresses called directly], gen, n, notes)` to
   `SPECS`. Spec names are unique across groups, and the functions belong to
   the group's units (`--list` flags any that do not).
4. Run it on the byte-identical build until it passes, then on an SCE build
   and on the GCC 14 build. C++ objects with virtual functions go through
   `t.obj` / `t.m` / `t.vtable` (see "The GCC 14 build"), and no two globals
   may be written as one block: GCC 14 places them independently.

A **hardware-facing spec**:

- installs the kernel before the first call: `Kernel(...).install(t)`, or
  `sif_up(t, Iop(servers=…))` for code that talks to the IOP, or a
  `gs.World(rng).put(t)` for the GS driver;
- models every register the code polls with an exit (`t.reg(GS_CSR,
  read=[0, 0, 8])`; a list ends the loop after so many polls), and keeps a
  never-ending poll for an on-purpose runaway with a small `maxsteps`;
- uses `Kernel(sleep=…)` / `on_block` / `t.interrupt` for what an interrupt
  would do while the code waits (a DMA end, a vblank, an IOP reply);
- stubs a callee that has its own spec when the callee is heavy or needs a
  different world (`t.stub(addr, fn, nargs)`; read float arguments with
  `t.float_args()`), and ends code that never returns (Exit, a thread's
  ExitDeleteThread is fine) with a stub that sets `$ra` to the harness
  return address (see `gf_start`);
- compares layout-dependent values (linker symbols, how far past `_end` a
  loop runs) as relations (`a[0] == t.sym("_end")`), never raw.

Rules for inputs:

- **Valid inputs by default.** An oracle must model the retail code exactly,
  quirks included (see `BnModel` in `bignum.py`), or be dropped. A stub
  returns what the real callee can return (a `u16` result below `0x10000`).
- **Invalid inputs on purpose** are welcome when the original handles them the
  same way every time. Examples are stored blocks with a wrong NLEN or a
  runaway `%*d` width. Say "on purpose" in the case's description, and give a
  runaway a small `maxsteps`.
- **No results from indeterminate data.** This covers uninitialised stack
  words and reads past the end of an image table. Their values differ between
  builds without any change in behaviour. Examples left out for this reason:
  - huft_build sets with zero lengths that are incomplete (stale `v[]`);
  - corrupted DEFLATE streams;
  - gzip headers at `in % 4 == 3`;
  - `%t` with precision 0 or 20 and up;
  - `%#.0f` and `%#g` (`printf.cpp` prints a buffer byte it never stored).

  If the original copies such bytes into memory that the test compares,
  exclude exactly those bytes with `t.unchecked(addr, n)` and say why. The
  example is gzip's Huffman tables, whose non-link entries carry the
  uninitialised `r.t`. A kernel parameter block is compared through
  `t.peek32` (never-written stack words as None).

### Runtime

The whole suite at the default `--n` takes about 4 minutes on the
byte-identical build (240 s for 133 specs; one Python process, retail and
rebuilt in turn, about 1.6 M emulated instructions per second): libc 1 s,
sha512 11 s, bignum 53 s, gzip 90 s, pdistd_io 5 s, gameflow 45 s, gs 9 s,
kernl_sys 10 s, kernl_rest 9 s, sonyio 7 s.  The heavy specs are the
multi-precision ones (`modpow_1024` 45 s, `gf_imageloader` 34 s: a 513-bit
modpow per accepted image, `bn_modinverse`, `mont_*`, `modpow`) and the
inflater (`huft_build`, `gzip_raw`); they run few cases at the default and
more with a larger `--n`.  The self-test adds 2 s.  The GCC 14 build takes
about as long (its skipped cases do not run).  The groups are independent:
one process per `--group` runs them side by side.

### Limits

- **Never executed, so not tested:** `nop_padding` 0x01000000 (the two nops in
  front of the entry point) and `F_01014984` (libpad's alignment nop).
- **Runtime units.** libgcc_eh, frame, cxxrt and libgcc_math are the
  toolchain's own `libgcc.a` and are not tested.
- **Not modelled:** a real thread switch or interrupt (a spec drives them),
  the VU micro programs (`vcallms` is only an event), the IOP itself (its
  servers are models of the replies the EE code relies on), the GS drawing a
  packet (the DMA snapshot is compared, not a picture), the MAC/status flags
  of VU0, the caches' contents (CACHE ops are events, tags come from the
  spec).
- **Image tokens.** These depend on the `// @0x…` annotations. A global that
  the code writes and that has no annotation shows as `wrote an image byte
  with no symbol`.
- **Coverage.** `--list` counts the functions a spec calls directly; callees
  that are not stubbed run as well. `--reach` shows which named functions
  the cases actually executed.
