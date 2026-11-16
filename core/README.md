# GT3 US main executable decompilation (`core.gt3`)

This tree holds the decompilation of the main executable of Gran Turismo 3
A-Spec US (SCUS-97102): the program the bootstrap loads from `core.gt3`. The
input is that payload as an ELF, `BASE_SCUS-97102`: one `PT_LOAD` at
`0x00100000`, entry `0x00100008`, `main` `0x00100140`, `$gp` `0x003579F0`.

**State: second pass done.** All 11,411 functions have C/C++ source (792
files, about 227,000 lines in `src/`). Every file compiles with EE GCC 14 and
every function is claimed exactly once.

- **First pass:** semantically faithful code, written quickly: same calls,
  constants, offsets, conditions and side effects as the machine code, with
  less care for names, layouts and style; each unit declared the classes of
  other units partially.
- **Second pass:** one canonical definition of every shared class, function
  and global, in 511 headers `include/game/<TU>_<name>.h` (one per original
  translation unit, about 51,000 lines): every field at its egcs offset,
  virtuals in slot order, sizes checked with the original compiler. Every
  unit now uses these headers instead of local copies, with names for
  methods, fields, locals and constants, and the unsure functions
  re-examined against the listings. A final sweep moved the constant sets
  several units shared into their headers, made the original's inline
  accessors inline and applied the units' rename proposals. The source call
  check finds every call of the machine code in the code (one exception, the
  CarSound copy constructor).

What is still open after the second pass is listed in
[ref/pass2/after_phase2.md](ref/pass2/after_phase2.md); what the first pass
found for later passes is in [ref/followups.md](ref/followups.md). No data is
defined yet, and matching the original compiler is a later pass.

## How the executable is cut up

| | |
|---|---|
| Functions | 11,411 (`ref/funcs.json`). 10,138 have exact extents from the `.eh_frame` FDEs that survive in the image; the rest, in code without FDEs, come from jal targets, data pointers and padding, corrected by hand in `ref/extra_starts.txt` and `ref/not_starts.txt` |
| Translation units | 551 C++ units from the FDE-pointer table at `0x0028E780`, plus 8 stretches without FDEs (`ref/tus.json`) |
| Work units | 143 address ranges of about 3,500 instructions each (`ref/units.json`), each a run of whole translation units or part of a large one |
| Names | 3,680 functions named (`ref/symbols.csv`): RTTI class index and vtables, the Lupo 2 name map, GT3Hooks' engine addresses, and 681 functions that are the same code as the decompiled bootstrap |
| Classes | 477 polymorphic classes with their vtables (`ref/classes.txt`) |

## Layout

| Path | Content |
|---|---|
| `src/` | The source, grouped by what it is: `game/` (the game), `engine/` (Polyphony's PS2 libraries), `sdk/` (Sony's), `runtime/` (compiler and C runtime); one file per original translation unit; [ref/LAYOUT.md](ref/LAYOUT.md) has the tree and which files each work unit has |
| `include/game/`, `include/engine/`, `include/sdk/`, `include/runtime/` | The canonical headers, one per original translation unit, at the same paths as their sources |
| `include/` | Shared SDK, libc and runtime headers (from the bootstrap decomp) |
| `ref/` | Tables and notes; `CONTEXT.md` and `DECOMP_RULES.md` are the rules of the first pass, `PASS2_HEADERS.md`, `PASS2_INTEGRATE.md`, `PASS2_UNITS.md`, `PASS2_SWEEP.md` and `PASS2_CALLS.md` those of the second |
| `ref/pass2/` | Second-pass plan and notes: per header group, integration partition and unit, the canonical name table `canonical.csv` |
| `tools/` | The generators and checkers below |
| `hybrid.txt` | Which units the period-accurate build links from the C |
| `baserom/` | Your `CORE.GT3` and the `BASE_SCUS-97102` made from it (not in the repository) |

## Building

```sh
python tools/build.py --compiler 991111b    # the original compiler: build/991111b/CORE.GT3, the C linked in
python tools/build.py                       # GCC 14: build/gcc14/CORE.GT3, the original code (C checked)
python tools/build.py --compile-only        # only compile and check the C: build/gcc14/core.o
python tools/relink.py                      # only relink the original code and compare it
python tools/hybrid.py --units u001,u010-u020 --keep-asm 0x00260D48   # link other choices of units
```

The GT3 Decomp Builder runs the same script (the **Core** profiles, "GT3 US
main executable"). It needs your `CORE.GT3`, from the disc or a disc image,
which it keeps in `baserom/`. [tools/build.py](tools/build.py):

1. **Base.** Makes `baserom/BASE_SCUS-97102`, the program as an ELF, from
   `CORE.GT3` when it is missing, and the splat split in `ref/splat/`
   (`tools/gen_splat.py`, then splat) when it is missing or out of date.
2. **Compile.** Every file under `src/`: GCC 14 with the flags of
   `tools/cc.sh`, or SCE ee-gcc 2.9-ee-991111b at `-O2 -G0 -ffast-math` (the
   original has the `max.s` / `min.s` and the `sqrt.s` without `sqrtf` calls
   only that flag gives). The original compiler also writes where each
   function starts (stabs of a `-g` pass), which
   [tools/funcmap.py](tools/funcmap.py) ties to the `// FUNC` markers.
3. **Combine.** `ld -r` of all objects into one relocatable `core.o`, and
   `undefined.txt`: what it uses but does not define.
4. **Link.** [tools/relink.py](tools/relink.py) assembles the split and links
   it at the original addresses; the image must equal `BASE_SCUS-97102` byte
   for byte.
5. **Hybrid** (original compiler only). [tools/hybrid.py](tools/hybrid.py)
   links the compiled C of the units [hybrid.txt](hybrid.txt) names over that.
   A unit whose C provides all its functions replaces its original code (its
   C goes into the unit's old slot when it fits); in the others the original
   code stays with the functions the C defines made local, so every call,
   vtable, function table and the constructor list reaches the C, and what
   the C does not emit (inline copies, typeinfo functions) stays original.
   The data never moves. What the C uses and no object defines gets
   its original address (from the function map, the declarations' address
   comments, `g_XXXXXXXX` names). The compiled C goes after the original
   `.bss`, and the heap starts after it (about 2 MB less heap).
   `build/<compiler>/hybrid/report.txt` lists what was redirected and resolved.
6. **Pack.** A signed `CORE.GT3` ([tools/gt3image.py](tools/gt3image.py)),
   for the disc or a memory card (`/BASCUS-97102GAMEDATA/core.gt3`).

With every unit from C except u121, the MPEG decoder (its C plays the intro
movie behind the original: `hybrid.txt` and `ref/followups.md` say why), the
program boots and plays the intro like the original; every unit alone, and
random sets of them, link too. GCC 14's C++
(exceptions, RTTI, vtables) does not mix with the original's, so the GCC 14
build links the original code for now. VU0 inline assembly writes the
special registers with `include/vu0asm.h`'s macros (`VU0_ACC`, `VU0_Q`, ...),
because the two assemblers spell them differently.

## Tools

Run from `core/`. Python 3 with `rabbitizer`, `spimdisasm`/`splat64` and
`pycparser`; m2c is a clone of [matt-kempster/m2c](https://github.com/matt-kempster/m2c)
in the repository's `sdk/m2c`.

| Tool | Purpose |
|---|---|
| `tools/regen.py` | Rebuild `ref/` from the game file: `ground.py` (functions, translation units), `libmatch.py` (code shared with the bootstrap), `names.py`, `classes.py`, `listing.py` (`ref/full.txt`), `gen_splat.py` + splat, `packets.py` |
| `tools/packets.py` | Per unit, `ref/units/<unit>/`: `index.txt` (functions), `listing.txt` (annotated disassembly), `m2c.c` (machine drafts), `classes.txt`, `boot.txt` (bootstrap source to reuse) |
| `tools/agent_prompt.py <unit>` | The task given to the agent that decompiles a unit |
| `tools/build.py [--compiler gcc14\|991111b]` | The build: `CORE.GT3` (see Building) |
| `tools/relink.py [--pack OUT]` | Relink the original program from its splat split and check it is byte-identical |
| `tools/hybrid.py [--units ...] [--keep-asm ...]` | Link the compiled C of chosen units over the original program (see Building) |
| `tools/funcmap.py` | Which compiled symbol is which original function: compiler line information against the `// FUNC` markers |
| `tools/cc.sh --unit <unit>` | Compile check with EE GCC 14 |
| `tools/check_coverage.py [--unit <unit>]` | Every function claimed by exactly one `// FUNC 0x00XXXXXX` marker |
| `tools/callcheck.py <unit>\|--all` | Does each function's code name every function the machine code calls (comments do not count, except `/* implicit: ... */` notes for calls the compiler emits; ref/PASS2_CALLS.md) |
| `tools/status.py [--compile]` | Progress per unit |
| `tools/hdrcheck.py <header>\|--all` | Compile a canonical header alone with GCC 14 (and as C when it is a C header) and check every documented offset and size with egcs |
| `tools/hdrall.py` | Compile all canonical headers in one file with both compilers; probe names that are ambiguous under `using namespace` |
| `tools/vtcheck.py` | Overrides use the name and parameter types of their base's slot |
| `tools/pass2_names.py` | The canonical name of every address (`ref/pass2/canonical.csv`) and the class-to-header map |
| `tools/gt3image.py` | `core.gt3` images: pack an ELF into one signed with a fresh key (the bootstrap takes the modulus from the file), check one, write a PCSX2 folder memory card holding it |
| `tools/packtest.py [--run]` | Three test cards (re-signed retail payload, edited payload with a stale signature, edited and re-signed) and, with `--run`, their boots through the retail bootstrap in the bootstrap's boot test |
| `tools/pass2_plan.py`, `pass2_tasks.py`, `pass2_integrate.py`, `pass2_unit_tasks.py`, `pass2_schedule.py`, `pass2_sweep.py`, `pass2_report.py` | Second-pass planning, agent tasks, launch order and the call-check comparison |

`units.json` must not be regenerated once work has started: the source tree is
organised by unit.
