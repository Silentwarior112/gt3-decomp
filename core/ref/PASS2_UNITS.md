# Second pass, phase 2: units on the canonical headers

Paths are relative to `core/`. Phase 1 wrote one canonical definition of every
shared class, function and global in `include/game/` (ref/PASS2_HEADERS.md).
Phase 2 moves each unit onto them and uses the occasion for more reverse
engineering: names, types and the functions the first pass was unsure about.
Behaviour must not change: every function still does exactly what the machine
code does (`ref/units/<unit>/listing.txt` stays the ground truth).

## What to do

1. **Includes.** Your unit header (`src/<unit>/<unit>.h`) includes the
   canonical headers your code needs (your task lists them) and keeps only
   what is local to the unit: types no other unit uses, static helpers.
   Delete every local definition of a class that now has a canonical
   definition, every local prototype of a function and every `extern` of a
   global that a canonical header declares. Each source file of yours includes
   the canonical header of its own translation unit.
2. **Your own definitions** must match the canonical declarations of your
   translation units: same names, signatures, classes, const-ness. Phase 1
   took them from your first-pass code, so usually little changes; when the
   canonical header renamed a method, a field or a parameter, follow it.
3. **References to other units.** Use the canonical names (your task's rename
   table): `F_00218660()` becomes `getGameStatus()`, `g_00350B14` becomes its
   name, a free call `F_xxx(obj, …)` to what is now a method becomes
   `obj->method(…)`. Field accesses use the canonical field names; replace raw
   offset arithmetic (`*(int *)((char *)p + 0x10)`) with the field.
4. **Names inside your functions.** Give locals, parameters and static
   helpers names that say what they hold (no `temp_v0`, `var_s1`, `sp10`,
   `arg0`). Replace magic numbers with named constants or enums where the
   meaning is known (`kind == 2 /* rally */`).
5. **Unsure functions.** Your task lists the functions with `/* ? */`
   markers. Re-examine each against the listing and the now-known class
   layouts: settle what you can (remove the marker, fix the code if it was
   wrong), and leave a marker with a better reason where you cannot.
6. **Notes from phase 1** in your task are about your unit: apply what they
   ask (renames, removed duplicates, signature fixes).
7. **Global names, not namespace ones.** These types exist only at global
   scope (a namespace typedef of the same type makes egcs call the name
   ambiguous under `using namespace`): GameOptions (not gt2k_game::),
   Color4f, MenuElement, MenuItemArray, McDirEntry, MotionValue (not
   gt2k_menu::). Write them unqualified or with `::`.
8. **Typeinfo functions** (`__tf...`): define each exactly as its header
   declares it: `extern "C"`, inside `#ifndef BOOT_OLD_GCC` (egcs generates
   them itself), returning `void *` or `const type_info &` as the header says.
   A definition with C++ linkage must change.
9. **Functions a canonical header made inline** (a destructor or accessor
   whose body is now in the header: egcs emits the out-of-line copy the
   listing shows): delete your definition and keep its marker, so the address
   stays claimed:

   ```cpp
   // FUNC 0x0019D068 RaceBarMeter::~RaceBarMeter
   /* inline in game/race/display/T160_race_meters.h; egcs emits this copy */
   ```

## SDK and C library code: the local ps2sdk

For Sony library code (libkernl: SIF RPC / commands, fileio, threads, tty,
kprintf; libcdvd, libpad, libmc, libscf, libgraph / libdma; the newlib C
library) take names, structure layouts and constants from the local ps2dev
SDK `ps2sdk-main` next to `decomp/` (`../../ps2sdk-main` from `core/`). Never
fetch from the web.

- `ps2sdk/ee/include`, `ps2sdk/common/include`, `ps2sdk/iop/include`: the
  ps2sdk headers (kernel.h, sifrpc.h, sifcmd.h, fileio.h, libcdvd.h, ...).
- `ee/mips64r5900el-ps2-elf/include`: newlib's headers.
- `ps2sdk/ee/lib/*.a`: compiled ps2sdk libraries; `ee/bin/` has the
  toolchain's `objdump` if comparing code helps.

ps2sdk is an open reimplementation of the same interfaces, not Sony's code:
use it for names and layouts where they agree with the listing; the listing
stays the ground truth, and where they differ, the listing wins (say so in a
comment).

## Canonical headers

- **Your own translation units' headers** (listed as "own" in your task): you
  may fix them when your code or the listing proves them wrong or incomplete:
  add a missing member or declaration, correct a type or an offset. Do not
  rename what is there (other units are switching to those names right now);
  put a rename you want in your notes. After an edit, `python tools/hdrcheck.py
  <header>` must still print OK.
- **Other headers:** do not edit them. If one is wrong or misses something you
  need, write it in your notes and work around it locally (a cast, a local
  accessor) with a `/* TODO(pass2): … */` comment.

## Checks (mandatory)

```sh
sh tools/cc.sh --unit <unit>                       # every file OK
python tools/check_coverage.py --unit <unit>       # missing 0, duplicated 0, unknown 0
python tools/callcheck.py <unit>                   # not fewer calls found than before (your task gives the number)
```

## Notes

Write `ref/pass2/units/<unit>/notes.md`: canonical-header problems you hit
(with the fix you want), renames you propose for names you do not own,
functions still unsure and why. Write nothing outside `src/<unit>/`, your own
canonical headers and that file.

## Scratch files

Other agents work at the same time. Keep any scratch files (test copies of
`include/`, throwaway sources) in a folder of your own named after your unit,
e.g. `build/scratch/<unit>/`, never in a shared folder, and never in `include/` or `src/`.
