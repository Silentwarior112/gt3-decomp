# GT3 enhanced engine (`core_gt3enhanced`)

The main executable of Gran Turismo 3 A-Spec US (`CORE.GT3`) from the
decompiled C of [`../core`](../core/README.md), with the game-logic updates of
**Lupo 2** ported in: a later revision of the same code base, GT Concept
(PAPX-905.09). `core/` stays the faithful GT3 US; this tree is where the
engine grows.

**State: the Lupo 2 updates are in.** Every Lupo 2 function was compared
with GT3's, area by area, and every game-logic change was ported to C in
GT3's terms: car physics (gearbox types, the Honda Dualnote hybrid, a
retuned chassis model, two GT3 physics bugs fixed), the AI, the race HUD,
cameras, sound, rendering (the Toyota Pod's animation, texture animation for
every car), menus and the Pod race. Nothing of GT3's is removed: where GT
Concept replaces a GT3 feature (GT mode, licences, the save data), GT3's
stays. It builds, boots and runs the attract mode's races; driving it by
hand is still to do.

- [ref/enhanced/REPORT.md](ref/enhanced/REPORT.md): what Lupo 2 changed and
  what went in, the physics in detail, the assets each new feature needs,
  what to test.
- [ref/ENHANCED.md](ref/ENHANCED.md): the rules (game logic only, never
  remove a GT3 feature, Lupo 2 as the source of truth) and the Lupo 2 tool.
- [ref/enhanced/PLAN.md](ref/enhanced/PLAN.md),
  [ref/enhanced/INTERFACES.md](ref/enhanced/INTERFACES.md): how the ports are
  organised, and the fields, events and signatures the areas share.
- `ref/enhanced/survey/<area>.md`: what Lupo 2 does differently, with
  addresses; `ref/enhanced/ported/<area>.md`: what each port does, where, and
  how to test it.

## Switches

Every port that changes how GT3's own cars, races or screens behave is a
switch in `include/game/enhanced/<area>.h` (physics, race, hud, rendering,
menus, cameras_sound): `1` = Lupo 2's behaviour (the default), `0` = GT3's
code exactly. Change one and rebuild. Features that only act on data GT3 does
not have (gearbox types, the Dualnote, the Pod, moving wings) have no switch:
with GT3's cars and files they do nothing.

## Building

```sh
python tools/build.py --compiler 991111b --link full    # build/991111b/CORE.GT3
```

The GT3 Decomp Builder runs the same (the **Core enhanced** profile). Only
this build applies: the program is linked from the C alone
([tools/fulllink.py](tools/fulllink.py)), with every object's data at its
original address where its bytes are still GT3's and the rest after it. The
period-accurate and GCC 14 builds of `core/` link GT3's original code, which
the enhanced objects no longer fit (`hybrid.txt` and `tools/hybrid.py` are
kept only because the tools share code with `core/`). Like `core/`, it needs
your `CORE.GT3` in `baserom/`: the split the link is checked against and the
alignment padding come from it.

The enhanced program is larger than GT3's, so the heap (from the program's
end to `0x01F00000`) is about 220 KB smaller; each race start prints
`[enhanced] heap: N KB free` in the PCSX2 console. New source files need no
registration: the link adds their static constructors and frame information
to collect2's lists.

## Layout and tools

The source is organised as `core/`'s ([ref/LAYOUT.md](ref/LAYOUT.md), and
[`../core/README.md`](../core/README.md) for the decompilation, its tables and
its tools). In addition:

| Path | Content |
|---|---|
| `include/game/enhanced.h`, `include/game/enhanced/` | The switches, one file per area |
| `ref/enhanced/` | The plan, the shared interfaces, the surveys, the port notes, the report |
| `ref/lupo2/` | Lupo 2's classes, vtables, RTTI and functions; `map.json`, which Lupo 2 function is which GT3 function; `pairs_fix.csv`, the pairs established by hand |
| `tools/lupo2.py` | Lupo 2 against GT3: `summary`, `changed`, `unmatched`, `cmp`, `dis`, `who`, `class`, `classes`, `map`, `fixes` (ref/ENHANCED.md). It reads the Lupo 2 executable from the repository's `local_paths.json`, key `lupo2_elf` (not in the repository) |
| `tools/sizeprobe.py [--only-diff]` | Every documented class size (`EGCS_SIZE`) against what egcs computes: run it after growing a struct |
