# SCUS_971.02: shared context for the analysis

(Written for the decompilation's analysis agents; kept as the project's
background notes.)

Target: the boot-loader ELF of Gran Turismo 3 US (SCUS-97102), PlayStation 2,
MIPS R5900 (EE), little-endian. It is loaded by the BIOS/launcher at
0x01000000 and its job is to show the startup/warning screen and then load,
inflate and launch the main image `core.gt3`.

Binary: your retail `SCUS_971.02`, kept at `bootstrap/baserom/SCUS_971.02`
(`tools/paths.py`).

## Workspace files (`bootstrap/ref/`, same directory as this file)

The listings (`full.txt`, `units/`, `chunks/`, `datamap.txt`) are generated
from the retail file and are not in the repository; see the bootstrap README,
"Regenerate ref/".

- `full.txt` — complete annotated disassembly (1.9MB; use Grep / offset reads)
- `units/<unit>.txt` — the listing restricted to one decompilation unit;
  `units/<unit>_symbols.csv` — that unit's symbol rows; `units.json` — all units
- `chunks/NN_<lo>_<hi>.txt` — the same listing split into 15 address chunks
- `funcs.json`, `funcs.csv` — 880 functions (start/end/callees/callers/data xrefs),
  boundaries validated (no branch leaves its function)
- `symbols.csv` — the mapping pass: name, module, kind, confidence, summary,
  C signature for all 880 functions (rows whose summary starts `SPLIT-OFF` were
  added by boundary fixes and have only a note)
- `map_notes.md` — mapping-pass notes per chunk: class layouts, module boundaries,
  boundary fixes, open questions. Grep it for your module/class names.
- `callgraph.txt` — one line per function: size, callees, callers, data xrefs
- `datamap.txt` — annotated dump of .data/.rodata/.rdata/.lit4/.sdata:
  strings, code pointers (`-> F_xxxxxxxx`), data pointers
- `bootstrap_strings.txt` — all strings with addresses (not in the repository)
- `DECOMP_RULES.md` — output conventions (markers, layout, compile check)

## Mapping-pass findings (to be confirmed by decompilation)

- Boot flow: `_start` → `main` (F_01000538) → display init, startup screen,
  IOP reboot + IRX loads + pad init, `select_boot_source` (DISK / MCARD 0 /
  MCARD 1 / HOST candidates, highest u16 version key wins) →
  `ImageLoader::load` (F_010049C8) → `ImageLoader::verify` (F_010044A8):
  inflate the body, SHA-512 over body[hashStart:], RSA
  `sig^66001 mod n` via Montgomery (F_0100B238 → F_01009128 ctor,
  F_0100A5D0 powmod), compare → entry point; stop startup screen, shut down
  IOP services, `ExecPS2(entry, 2, {path, "hot"})` via F_01016F70.
- Three mapping agents independently reported that the RSA **modulus is read
  from the file** (no baked-in modulus compare) and the exponent 66001 is an
  immediate at 0x01004554. This is the project's key question; it is being
  adversarially re-verified — decompile `verify` exactly and flag anything
  that contradicts it.

Annotation notes for the listings:
- `; call F_xxxxxxxx` on jal; `; -> addr` on branches; `; tail-> F_x` on j-tail-calls
- `; 0xADDR .section 'string'` when a lui/addiu(/ori) pair resolves to a data address
- `; gp+/-N = ADDR .section [VALUE 'string']` for $gp-relative accesses ($gp = 0x01049CF0)
- `; exception landing pad next (...)` — the next instruction is a C++ EH landing pad
- `; data xrefs: ...` under a function header — data words pointing at this function
  (vtable slots, function-pointer tables, ctor lists)

## ELF layout

| section | va | end | notes |
|---|---|---|---|
| .text | 0x01000000 | 0x010269E8 | 39.5k instructions |
| .text.sceGsGetGParam | 0x010269E8 | 0x010269F4 | named — sceGsGetGParam |
| .text.sceGsResetGraph | 0x010269F8 | 0x01026B88 | named — sceGsResetGraph |
| .data | 0x01026C00 | 0x0103DDD8 | game vtables to ~0x1027F38, then embedded gzip `startupa.img` (0x1027F38..0x1033C98, inflates to 0x38020-byte `Tex1` texture), then GS display-list-like data, then per-library .data (see anchors) |
| .rodata | 0x0103DE00 | 0x01040BC2 | strings, RTTI names, FP/huffman-ish tables at 0x103EC00..0x103FB00 |
| .rdata | 0x01040C00 | 0x01040F98 | |
| .gcc_except_table | 0x01041000 | 0x01041CE8 | C++ EH tables (landing pads) |
| .lit4 | 0x01041D00 | 0x01041D0C | |
| .sdata | 0x01041D80 | 0x01041E6B | source table DISK/MCARD 0/MCARD 1/HOST @0x1041D98.., `host:` @0x1041E18 |
| .sbss | 0x01041E80 | 0x01041EC8 | |
| .bss | 0x01041F00 | 0x0104DE94 | args buffer @0x1041F00 |

Entry = 0x01000008 (crt0). $gp = 0x01049CF0. InitHeap(_end=0x0104DE94, -1).
Main stack from InitMainThread (syscall 0x3C), RFU/args buffer at 0x1041F00.

## Known anchors

- crt0: F_01000008. After `ei` it does `main(*(int*)0x1041F00, (char**)0x1041F04)`
  where **main = F_01000538**, then `j F_0101B800` (Exit-like) with main's result.
- Library version strings in .data (link order ⇒ module order in .text too):
  `PsIIlibgraph2221` @0x010394F0, `PsIIlibcdvd 2240` @0x0103A3C0,
  `PsIIlibpad  2200` @0x0103BE30, `PsIIlibmc   2240` @0x0103BE78,
  `PsIIlibkernl2240` @0x0103D278.
  Everything before libgraph's data is game/Polyphony code (crt0, main, ImageLoader,
  GZIP inflate, pdistd printf/Allocator, startup-screen display). The tail of .text
  (TTY/sio, `## internel error in libkernl.a!` @0x1040510, `rom0:UDNL` @0x1040720)
  is libkernl; between game code and libkernl sit libgraph, libcdvd, libpad, libmc.
- C++ (GCC 2.9x-ee, egcs era): RTTI type-name strings in .rodata:
  `Q211ImageLoader6Source|10HostSource|10DiskSource|11MCardSource` (@0x103E278..),
  templates `Q211ImageLoadert12MCardSource_2Ui1Ui0` / `Ui0Ui0` (@0x103DE50/78),
  `13FileName_Base` `12HostFileName` `13MCardFileName` `12DiskFileName` (@0x103E218..),
  `Q26pdistd9Allocator` (@0x103E690), `Q26pdistd21PrintFormatTargetBase` (@0x103FC78),
  `Q24GZIP11InflateBase` (@0x103FD08), printf.cxx `PrintTarget` (@0x103FCB8),
  plus libstdc++/libgcc RTTI (`bad_alloc`, `__si_type_info`, ...) @0x10400E0..0x1040510.
  GCC 2.9x mangling: `Q2<len>A<len>B` = A::B; `__ct`/`__dt` ctor/dtor; `_vt$` vtable;
  `__tf` typeinfo function; vtables are arrays of {short delta, short ?, pfn} — in the
  datamap a vtable shows as pairs (word, `-> F_x`).
- Strings: `core.gt3` @0x103DE00, `cdrom0:\IOPRP224.IMG;1` @0x103DEB0,
  IRX list sio2man/mtapman/mcman/mcserv/padman @0x103DEC8.., `GAMEDATA` @0x103E1A8,
  `SCUS-97102` @0x103E350, `Allocator::debug` fmt @0x103E5F8,
  libcdvd debug strings @0x103FD20.., libpad @0x103FEF0.., libmc @0x1040048..,
  TTY/sio @0x1040750.., `(null)` @0x1041E40 (printf's null-string).
- `sceCdCbfunc`, `CD_Init`, `DiskReady` etc. are Sony libcdvd internals; use real
  Sony SDK 2.2-era names when identifiable (sceCdInit, sceCdRead, sceCdSearchFile,
  sceSifInitRpc, sceSifBindRpc, sceSifCallRpc, sceSifLoadModule, scePadInit,
  sceMcInit, sceGsResetGraph, FlushCache, ...). The open-source reference
  ps2sdk (github.com/ps2dev/ps2sdk) has similar RPC client code
  (different codebase, same protocol and often same function shapes).
- Named symbol reference: the GT Concept "Lupo 2" build ships full symbols for the
  same Polyphony library code (`pdistd::`, `GZIP::`, printf, Allocator):
  GT3Hooks' `docs/symbols/lupo2_symbols_demangled.txt`
  (grep it; each entry: addr size T/D name + mangled form on next line).
- Project background: GT3Hooks' `FINDINGS.md` §9
  ("The boot chain, core.gt3, ...", ~lines 780–956) documents the `core.gt3`
  container: u16 loadSourceFlags, u32 rawSize, raw DEFLATE (wbits=-15) body =
  {i16+bytes rsaValue1 (modulus), i16+bytes rsaValue2 (signature),
   [hashStart] i32 nSection, i32 entryPoint, per-section i32 target, i32 size, data}.
  RSA-1024, SHA-512 over body[hashStart:], exponent 66001 (US). OPEN QUESTION the
  decompilation must answer: does the bootstrap verify this signature, and with the
  in-file modulus or a baked-in one?

## R5900 / ABI notes

- 32-bit pointers, 64-bit GPR ops (ld/sd/daddu common), 128-bit lq/sq (quadword
  moves — memcpy/memset and register spills of 128-bit types).
- EE-specific ops in the listing: ei/di, lq/sq, mult3/multu3 (3-operand mult),
  mfsa/mtsa, MMI (madd, mult1/div1/mflo1 — second pipeline, used by 64-bit
  div helpers), plzcw, etc.
- ABI: args in $a0-$a3 then $t0-$t3 ($4..$11); f32 args in $f12..; return $v0
  (int/ptr, 64-bit in $v0), $f0 (float). Stack frames 16-byte aligned; $gp used
  for small data. Delay slots apply to all branches/jumps.
- Syscalls: `addiu $v1, $zero, N; syscall` — N is the EE kernel call number
  (0x3C InitMainThread, 0x3D InitHeap, 0x64 FlushCache, 0x77 sceSifSetDma,
  0x78 sceSifSetDChain, 0x79 sceSifSetReg, 0x7A sceSifGetReg, 0x06 LoadExecPS2,
  0x07 ExecPS2, 0x3E EndOfHeap, 0x29 CreateThread? — use standard EE kernel
  numbering; negative/alias forms exist. RFU/Deci2 calls 0x7C..).
- printf-family here is Polyphony's own (pdistd::PrintFormat*, printf.cxx),
  layered over TTY output via libkernl sio.
