# GT3 main executable: context for the decompilation

Target: the main executable of Gran Turismo 3 A-Spec US (SCUS-97102): the
payload of `core.gt3`, as an ELF, `baserom/BASE_SCUS-97102` (`tools/paths.py`).
PlayStation 2 EE (MIPS R5900), little-endian. One `PT_LOAD` at `0x00100000`,
file offset = va − `0x00100000` + `0x1000`. Entry `0x00100008` (crt0),
`main` = `0x00100140`, `$gp` = `0x003579F0`.

## Memory map

| Range | What |
|---|---|
| `0x00100000..0x0028E760` | `.text`, 408,024 instructions, 11,388 functions |
| `0x0028E780..0x0028F01C` | FDE-pointer table: one pointer per C++ translation unit (551) |
| `0x0028F020..` | `.ctors` (count-headed: 110 entries), then `.dtors` |
| `0x0028F2AC..0x0030729C` | `.eh_frame` blocks (CIE + FDEs, one block per translation unit) |
| `0x0030729C..0x003531E3` | `.data` / `.rodata` / `.sdata` (no section headers survive) |
| `0x00353200..0x004B2C0C` | `.sbss` / `.bss`; `_end` = `0x004B2C0C` |
| `0x004B2C10..0x01F00000` | the engine heap (pdistd allocator, built on the first malloc) |
| `0x01F00000..0x02000000` | main thread stack |

## What the code is

- **Polyphony C++** (most of `.text`): compiled by SCE ee-gcc 2.9-ee-991111
  (egcs era, GCC 2.9x C++ ABI). The menu system is namespace `gt2k_menu`
  ("gt2k" = GT3's working name), over a race core (`RaceBase`,
  `RaceOrganization`, `RaceEntryCar`, `CarEquipments`, `RaceDataBase`,
  `DynamicsConductor*`, cameras `*Camera`, `RaceDisplay*`, sound, tyres,
  models), Polyphony's `pdistd` (allocator, printf), `GZIP`, `pdi*` IOP
  clients (`pdicdvd.cxx`, `pdispu2.cxx`, `pdibgm.cxx`, `pdimpeg.cxx`,
  `pdipipe.cxx`). 477 polymorphic classes are known by name (RTTI).
- **No-FDE regions** (`G000..G007` in ref/tus.json): crt0; a graphics
  library (`__pglShading`, the GS/VIF driver shared with the bootstrap);
  more Polyphony code built without exceptions; Sony libraries libcdvd 2240,
  libpad 2200, libmc 2240, libscf 2200, libgraph 2221, libkernl 2240 (the same
  versions as the bootstrap's); newlib libc (`vfprintf`, `_malloc_r`, string
  functions); the GCC 2.9 runtime (libgcc, EH, RTTI, `new`/`delete`).
- **Shared with the bootstrap.** 681 functions are the same machine code as
  functions already decompiled in `decomp/bootstrap` (`ref/libmatch.json`).
  Their bootstrap source is in each unit's `boot.txt`.

Landmarks: `printf` 0x0026C690, `sprintf` 0x0026C718, `vsprintf` 0x0026C7D8,
`malloc` 0x0026C3A8, `free` 0x0026C380, `memalign` 0x0026C428 (44 callers),
`memset` 0x0027DB38, `memcpy` 0x0027D990, `strcmp` 0x0027EA40, `strcpy`
0x0027EB84, `operator new` 0x0027C138, `operator delete` 0x0027C0C0,
`operator new[]` 0x0027C2D8, `operator delete[]` 0x0027C260, `__main`
0x00276FB0, `__rtti_si` / `__rtti_user` / `__rtti_class` 0x0027C3E8 /
0x0027C408 / 0x0027C3C0, `vol_init` 0x00241A68, `file_resolve` 0x00240AF0,
`file_read_blocking` 0x002417C0. More in ref/symbols.csv.

## Files (ref/)

- `units/<unit>/index.txt` — the unit's functions: address, size, original
  translation unit, known name and its source, class, callers (in unit /
  outside), `BOOT=` when the bootstrap has the same code.
- `units/<unit>/listing.txt` — annotated disassembly of exactly the unit's
  functions. Ground truth.
- `units/<unit>/m2c.c` — machine drafts by m2c (one per function, 99.4%
  coverage), headed by the known names of every function they mention.
- `units/<unit>/classes.txt` — the classes whose typeinfo function or vtable
  methods are in the unit, with their full vtables.
- `units/<unit>/boot.txt` — bootstrap source for the unit's shared functions.
- `classes.txt` / `classes.json` — every polymorphic class and vtable.
- `symbols.csv` — function names (sources: abi, boot, lupo2, tf, vslot,
  lupo2m, dtor, vslotgen); `data_symbols.csv` — vtables, typeinfo names.
- `funcs.json` — all functions; `tus.json` — translation units; `full.txt` —
  the whole listing (446k lines; grep it, never read it whole).

## Listing notation

- `; call F_x name` (jal), `; tail-> F_x` (j to a function), branch targets as
  addresses.
- `; &ADDR name 'string'` for a lui/addiu or lui/ori pair; `; [ADDR] ...` for a
  load/store through a lui base; `; gp+-N = ADDR name (=value)` for
  `$gp`-relative accesses (with the word's initial value; `lwc1` also shows it
  as a float).
- `; switch table @ADDR (n cases)` with `case i -> target` lines at `jr $reg`.
- `; EH ref from 0xDATA`: an exception table points here (a landing pad or a
  region bound).
- Registers use the EE ABI names: `$a0-$a7` = `$4-$11` (8 integer
  arguments), `$t0-$t3` = `$12-$15`, `$f12-$f19` float arguments, `$v0`/`$f0`
  return. m2c.c uses the same names.

## R5900 / ABI notes

- 32-bit pointers, 64-bit GPRs (`ld`/`sd`/`daddu` are common: `daddu $a0, $s0,
  $zero` is a move), 128-bit `lq`/`sq` (quadword copies, `u128`).
- Arguments: integers/pointers in `$a0..$a7`, floats in `$f12..$f19`, mixed
  freely; more on the stack. Return in `$v0` (64-bit in `$v0`), `$f0`.
- EE-only instructions: `ei`/`di`, `sync.l`/`sync.p`, `mult`/`multu` with 3
  operands, `mult1`/`div1` (pipeline 1), MMI (`paddh`, `pcpyld`, `pextlb`,
  `psrlh`, `por`, `pand`, ...), `max.s`/`min.s`, `madd.s`/`msub.s` and the
  accumulator ops, `rsqrt.s`, VU0 macro mode (`lqc2`/`sqc2`, `qmtc2`/`qmfc2`,
  `vadd`, `vmul`, `vmadd*`, `vopmula`, `vdiv`, `vsqrt`, `vwaitq`,
  `vcallms`).
- Branch-likely (`beql`, `bnel`, ...): the delay slot runs only when the
  branch is taken.
- Syscalls: `addiu $v1, $zero, N; syscall` — the libkernl stub table is at
  0x00285840..0x002860C0.

## egcs C++ ABI (GCC 2.9x)

- **vtables** are arrays of 8-byte entries `{s16 delta; s16 index; void
  *pfn}`. The vptr points at entry 0 (`{delta, __tf}`, the typeinfo
  function). Method slot k is entry k+1, so a virtual call is `lw vt,
  VPTR(obj); lh d, 8k+8(vt); lw fn, 8k+12(vt); jalr fn` with `this` = obj +
  d. ref/classes.txt lists each slot's call offset (`+0xC` = slot 0).
- **The vptr is a data member.** egcs places it after the data members of the
  first class in the hierarchy that has virtuals (often at +0, but not
  always).
- **Destructors** take a hidden `int` flags argument in `$a1`: bit 0 = call
  `operator delete(this)` afterwards. The deleting form tests `andi …, 1` and
  calls 0x0027C0C0.
- **Constructors** store the class's vtable address into the vptr field after
  constructing the bases; `operator new(size)` followed by a constructor call
  gives the object size.
- **`__tf<mangled>`** typeinfo functions build the `type_info` node on first
  call (`__rtti_user`/`__rtti_si`/`__rtti_class`) and return it.
- **Mangling**: `Q29gt2k_menu6Button` = `gt2k_menu::Button`, `__ct`/`__dt` =
  ctor/dtor, `_vt$X` = vtable, `t11RaceBattleT1i8` = `RaceBattleT<8>`.
- **Static constructors** run from `__main` through the `.ctors` list
  (`_GLOBAL_$I$...` functions).
- **Exceptions** are table-based (`.eh_frame` + exception tables); landing
  pads show as `; EH ref`. Most landing pads only run destructors.
