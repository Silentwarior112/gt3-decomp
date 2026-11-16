# GT3 enhanced engine

`core_gt3enhanced/` is a copy of the finished GT3 US decompilation (`core/`)
that takes in the game-logic updates of **Lupo 2**, a later revision of the
same code base (GT Concept, PAPX-905.09). `core/` stays the faithful GT3 US;
this tree is where the engine grows.

## What goes in, what does not

- **In:** game logic. Menus, game flow and save data; car physics and
  dynamics; rendering and effects; the race HUD and displays; race logic, AI
  and organisation; cameras; sound logic. Known examples: the gearbox types,
  the Honda Dualnote (hybrid), the Toyota Pod (animation, emotions, sounds).
- **Out:** backend: the SDK, the runtime, the IOP / file / pad / GS
  plumbing, unless a game feature cannot work without a change there.
- **Never remove a GT3 feature.** GT Concept has only the arcade mode (or its
  equivalent under another name): where Lupo 2 drops or stubs something GT3
  has (GT mode, its menus, save data, races), GT3's version stays. A change
  Lupo 2 made *together with* a removal is taken only where it is a change
  of its own.
- **Lupo 2 is the source of truth.** The GT3Hooks plugins and notes
  (`../../docs/gearbox/`, `../../docs/pod/`, `../../docs/gearbox_dualnote.md`,
  `../../FINDINGS.md` sections 11-14, `../../GT3Hooks-main/source/games/gt3/`)
  already ported parts of three features as hooks; they are a head start,
  not the full list, and some of them stop short of what Lupo 2 does.

## Tools

`tools/lupo2.py` pairs Lupo 2's functions with GT3's (the Lupo 2 executable's
path is <repo>/local_paths.json `lupo2_elf`):

```sh
python tools/lupo2.py summary                 # per folder: paired, differing, unpaired Lupo 2 functions
python tools/lupo2.py changed --area src/game/car/physics --plausible --min 0.3
python tools/lupo2.py unmatched --area src/game/car/physics   # Lupo 2 code with no GT3 twin, placed by neighbour
python tools/lupo2.py cmp 0x001E3060          # a pair's features: calls, virtual slots, floats, offsets, constants
python tools/lupo2.py dis 0x001E3060          # Lupo 2 disassembly, calls named through the map (--us: GT3)
python tools/lupo2.py who 0x001E3060          # a Lupo 2 address in GT3 terms (--us: the reverse)
python tools/lupo2.py classes                 # shared / Lupo-2-only / GT3-only classes, vtable growth
python tools/lupo2.py class RaceCarModel      # a class's vtable slot by slot with our source functions
```

The two builds are different compiles (another compiler set-up, another SDK),
so bytes never match; features do. The map (`ref/lupo2/map.json`) is built from
names, vtables, unique strings, call lists and link order; `how` says which.
Pairs established by hand go into `ref/lupo2/pairs_fix.csv` (`l2,us,verdict,end,note`;
verdict `right`, `new` = Lupo 2 only, `wrong`, `start` = a function the list
lacks): `python tools/lupo2.py fixes FILE...` merges more of them in, and the
next `map` keeps them fixed (`how` = `fix`) and builds the rest around them
(`L2_MAP=<file>` writes the map elsewhere). Lupo 2's function list is its frame
info plus every `jal` target and vtable entry outside it (leaves have no frame
info).
`order-gap` and `calls` pairs are the least certain, and a pair whose sizes
differ by more than 2x is more likely a wrong pair than a changed function:
check before you trust it (a changed function keeps its callers, strings and
most of its constants). Class and object layouts differ between the builds
(fields added), so a Lupo 2 function is read for what it does, then written
in GT3's terms; nothing is copied verbatim.

Our GT3 sources: `ref/LAYOUT.md` (folders), FUNC markers give each
function's GT3 US address.

## Survey reports

`ref/enhanced/survey/<area>.md`, one per area. For every change found:

| Field | |
|---|---|
| What | the behaviour in game terms |
| Where | Lupo 2 address(es); the GT3 function / file it changes, or "new" |
| Kind | changed logic / new function or class / new data or table / removed in Lupo 2 (not ported) / compiler noise (no change) |
| Evidence | what in the code shows it (calls, constants, fields, strings) |
| Port | what porting takes: new fields (object layout), data (paramDB, tables), assets (models, sounds, textures), other areas it touches |
| Verdict | port / do not port (why) |

The noise (pairs that differ only by register allocation or instruction
order) is listed briefly at the end, so a later reader knows it was looked at.
