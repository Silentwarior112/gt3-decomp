# Survey: menus part A and game flow (Lupo 2 against GT3 US)

Area: `src/game/menu/framework/`, `src/game/menu/arcade/`,
`src/game/menu/options/`, `src/game/menu/race_menu/`, `src/game/app/`,
`src/game/common/`.
Format: `ref/ENHANCED.md`. Addresses: "L2" = Lupo 2 (PAPX-905.09), "US" = GT3 US.
Scratch (scripts, dumps): `build/scratch/survey_menus_a/` (`triage.py`,
`classify.py`, `strs.py`, `consts.py`, `ptrs.py`, `who.py`, `fstart.py`,
`callee_diff.py`; dumps `pairs.txt`, `unmatched.txt`, `check.txt`,
`pairs_auto.tsv`).

## Summary

GT Concept turns GT3's arcade menu into the whole game: the top menu is the
arcade, GT mode is never entered, and the arcade gets new modes (the Toyota
Pod race for 1 and 2 players, a licence / training mode), new pages, a
progression of rewards and kiosk features. In the list below, "port" and
"do not port" are the verdicts.

**Game flow (`src/game/app/`)**

1. **Boot splash images** (`DisplayAdvertiseImage`,
   `advertise/startupiw.img`). Four images fade in and out before the menus.
   They replace the memory card warning and the opening movie. **Do not port**
   (GT Concept promotion; GT3 keeps its warning and opening).
2. **Boot load of the save.** A GT3 save on the card only gives the
   language. The autoload screen runs only when no such save is found (?).
   **Do not port** (part B: memory card).
3. **The arcade is the top menu.** `main` loops on `main_arcade` instead of
   `gtmode_main`. The arcade menu can hand a main mode back to `main` through
   `GameStatus nextMode`. Two new main modes: 32 plays `stream/demoGT3A.pss`;
   33 is an i.LINK linked attract demo (`RaceLinkDemo`). GT mode as the top
   menu is **removed in Lupo 2** (not ported). Modes 32 and 33: **do not port**
   (optional; 33 needs the race area's `RaceLinkDemo`).
4. **Attract mode.** One demo per idle time-out, with no 5-demo loop and no
   opening movie between demos. A new demo kind (1000) plays a recorded
   replay file as the attract. **Port optional** (the replay attract needs
   demo replay files and a GCF demo kind).
5. **`main_arcade`: new arcade states.** These are the licence / training
   mode, the Pod race (1P) and the Pod race (2P). The i.LINK battle gets a
   time limit from the GCF idle time and a second (replay) run. The run modes
   are renumbered and `MenuArgs` grows from 0x3C to 0x44 bytes. **Port the Pod
   states** (with the race area's `RacePOD` / `RacePOD2P`). **Port licence /
   training only with the race area's `RaceTraining`.**
6. **`run_arcade_race`.** The car comes from the GameStatus running car. A
   CVT forces the automatic gearbox. The race takes a GCF idle time limit.
   The post-race menu no longer goes through `run_short_menu`. After the race
   come a "present" notice and a rewards screen. A GT3-style copy is left as
   dead code. **Port the CVT-forces-AT rule** (with part B's `isCVT`). **Do
   not port the rest** (Concept flow).
7. **Post-race rewards** (`0x001378D8`). Prize cars (car get), pages
   `concept_moviea/movieb/ace/pod/billion` and the long ending movie are
   unlocked by a reward list. **Do not port** (Concept progression; part B
   lists the car-get / present classes).
8. **Free run and split battle.** The ASM / TCS of the options (free run),
   or of each player's menu choice (2P battle), now reach the race entries.
   Free run also forces AT for a CVT. **Port** (small). Time trial: the
   records API changed (part B).
9. **New race runners**: `run_pod_race`, `run_pod_2p_race`,
   `run_concept_license`, `run_training`. **Port the Pod runners** with entry
   5. Concept licence / training: **do not port** (Concept licence content),
   or port only with entry 5.
10. **Prize cars and endings after GT-mode / arcade races**
    (`F_0013F508`) are stubbed to `return 0`. **Removed in Lupo 2** (not
    ported).
11. **Busy mark** of the loading transitions is drawn rotating and scaled.
    **Port optional** (cosmetic).
12. **Menu movies**: `play_movie` is reused for menu requests
    (`MOVIE0` = `stream/opening_GT3UK.pss`, `MOVIE1` =
    `stream/ending_long_full.pss`). **Do not port** (assets; GT3 plays its
    movies itself).

**Arcade menus (`src/game/menu/arcade/`)**

13. **`ArcadeControl` becomes a polymorphic base** (vtable 2 → 9 slots).
    `ConceptControl` (`menu/concept.mbl`) is the running menu; GT3's arcade
    entry is kept but has no caller. New fields (`+0x70` shift at the mode
    field; Pod, difficulty, mpeg images). **Port only the pieces the chosen
    features need.**
14. **Arcade mode list and decode**: license 2, goodies 5,
    `status` = Pod race 6, i.LINK 8. `setPodRace` chooses Pod 1P or 2P. New
    race codes (`ATAF0001`, `A2P0001`, `ALNK0001`, `APO20001`) and GCF classes
    (`CLASS_COMPULSORY` / `CLASS_LICENSE` / `CLASS_PODRACE`). **Port the Pod
    entry.**
15. **Top menu "ar/push_start_wagen"**. The push-start page is now a
    navigable top menu. It remembers the last page and cursor, times out to
    the attract on first entry, and hides "aspec_demo" in one country. Menu
    requests `MOVIE0` / `MOVIE1` / `TRANSFER` / `PODRACE`. **Port optional**
    (the remembered cursor is small and useful; the rest is Concept layout).
16. **New arcade pages**: `ar/status` + `ar/show_room` (prize-car map,
    completion %, available cars n/m, car status texts), `ar/select_license`
    (licence list with gold / silver / bronze medals, ranks),
    `ar/select_category`, difficulty / game / category / licence button lists
    and `ar/connect_ride`. **Do not port** without GT Concept's `.mbl` / `.pss`
    assets and its save data (status page possible later).
17. **Car select.**
    - New: an ESP / assist step per player, which is GameOptions' assist
      level, a tuner logo, and colour chips.
    - Moved: the stat gauges (now bar gauges, on car change).
    - Removed: the car spec panel, the 2P gauges in the list, the colour
      selection and the racing-modification / "dri" toggles.

    **Port the assist (ESP) choice** (gameplay). The rest is UI: **do not
    port** without assets.
18. **Class and course select removed.** `ArcadeControl::selectCourse` and
    `classSelect` are stubbed to `return 0` (delta review, entry 41), so the
    course comes with the car's race. Lupo 2's reskinned class / course
    screens (`mini_classes`, `%s_logo`, `%s_mini1`, `MiniCourseCursor`)
    still exist but nothing calls them. GT3's course logo / name / clear
    mark / prize item factories are gone. **Removed in Lupo 2** (not ported;
    the reskin: do not port).
19. **Arcade classes from the GCF** (class names from the product config's
    category table) and a car index remap for classes 7..10. **Port only with
    16 / 14.**
20. **Arcade background movies** replaced by `*_gtc.pss` and others, plus
    an `alpha_test.pss`. **Do not port** (assets).
21. **Goodies page and arcade "history" labels removed in Lupo 2** (not
    ported).
22. **`ar/select_car_test`** debug tool (camera and movie capture over
    host0). **Do not port.**

**Options (`src/game/menu/options/`)**

23. **New "assist" option.** `multi_assist` and `assist_0/1/2` choose one of
    four {TCS, ASM} presets on GameOptions' existing `assistTcs` / `assistAsm`.
    **Port** (needs an option-page item).
24. **Screen position adjust.** Sliders `screen_adjust_h/v` (-16..16) are
    stored in GameOptions `+0x64` / `+0x68`, which is GT3's reserved,
    zero-default area. Each change is applied at once through a new
    `gfx_set_display_offset(2·h, 2·v)`, and again by `GameOptions::reset`
    (delta review, entry 40). **Port** (save-compatible; GT3 already has the
    display offset fields, only the setter is new).
25. **Options page = `menu/option_1p_limited.mbl`** (a cut-down layout).
    **Do not port.**

**In-race menus (`src/game/menu/race_menu/`)**

26. **Short-menu button rules.** New gates for `load_ghost` and
    `demonstration`. `setting` / `qm_setting` / `save_game` /
    `qm_save_game` are always hidden (**removed in Lupo 2**, not ported).
    `ShortMenuArgs` grows. **Port the two new gates** only if the race modes
    set their flags.
27. **Concept licence short menu** (`ShortmenuConceptLicenseControl` /
    `Render`, `menu/concept_qm_license(_lead).mbl`). **Port only with entry
    9's training.**
28. **Licence time texts** in the short menu are not divided by 1000
    (minor). **Do not port** (follows the race side's units).

**Framework (`src/game/menu/framework/`, `src/game/common/`)**

29. **New widgets**: `BarGauge`, `SelectBox2`, `ColorBox2`,
    `MiniCourseCursor`, `MpegImageAlpha`, `Status`- / `Game`- /
    `License`-buttons and lists, `CategoryButtonList`, `DifficultyButtonList`,
    `SimplePageControl`. **Port with the screens that use them.**
30. **Button**: a new "selectable" flag checked by `isAvailable`, and a new
    empty virtual (vtable 18 → 19 for all buttons and sliders). **Port with
    29** (plumbing).
31. **`Item::setText` caches the text width** for aligned texts. **Port**
    (small; harmless).
32. **Alpha movies**: `MenuBgMovie` copies an alpha mask carried in the
    movie frame into the texture alpha. `start()` takes a file override and
    waits for the first frame. **Port with 29** (start-waits-for-frame is a
    small fix worth taking).
33. **Menu car steps model-set animations** each frame (animated cars in
    the menus, the Pod's face). It also gets new setters (dirtiness,
    reflection, magnify, anti-aliasing). **Port with the Pod** (car-model
    area).
34. **`SmartIndexT`**: `normalize` becomes virtual. The smart indices get a
    wrapping move that skips invalid entries. **Plumbing; port with 16-19.**
35. **Smooth scrolling** in `MapButtonList` / `ButtonList` (new virtual 13).
    **Plumbing; port with 16.**
36. **Cross** marker drawn as an animated strip. **Do not port**
    (cosmetic).
37. **`gt2k_menu::String` class removed** (texts go through `Item`).
    **Removed in Lupo 2** (GT3 keeps it).
38. **`responseEval`: new response curve type 3**, `y = c·u⁸ + b·u`. **Port**
    with whoever uses it (physics / input area: find the curve data with
    mode 3).

**Added by the delta review (map v2)**

39. **Car select locks a CVT car to automatic.** The transmission choice of
    the arcade car select (1P and 2P) has only "AT" when the chosen car is a
    CVT (`isCVT`, menus_b entry 1). **Port** (with the CVT / gearbox work).
40. **Screen adjust is applied through a new display-offset setter**
    (completes entry 24): `GameOptions::setScreenAdjustH/V` call
    `gfx_set_display_offset(2·h, 2·v)`, which sets GT3's existing
    `dispDX` / `dispDY` (never written in GT3). `GameOptions::reset`
    applies the offset too. **Port** with entry 24.
41. **Arcade class select stubbed** (`classSelect` returns 0), and the
    history-class show / hide stubbed to empty (completes entries 18 and
    21). **Removed in Lupo 2** (not ported).
42. **Short menu "root_no_setting_no_save" page variant** registered beside
    "root" (completes entry 26). **Do not port** (it goes with the
    always-hidden setting / save buttons).

**How many I examined.**
- **Pairs.** First pass, with the first version of `tools/lupo2.py`:
  **583 pairs** with difference ≥ 0.25 in the six folders (framework 333,
  arcade 84, options 47, race_menu 50, app 35, common 14). 358 were
  auto-classified as noise: the same calls, floats, strings and
  virtual-call counts after the known compiler differences (spot-checked).
  I read the other **225** by hand.

  Second pass, with the tool updated during the survey (gp floats, resolved
  Lupo 2 virtual slots, tail calls as calls). **445 pairs** are ≥ 0.25
  (framework 246, arcade 82, options 40, race_menu 34, app 33, common 10;
  22 over 2x in size): 263 auto-noise and 182 to check. All 182 were among
  the 225 except 3, which I added; all 228 are in table A. Every resolved
  virtual-slot and gp-float difference was re-checked. Two verdicts
  changed (`Arrow::virtual_1`, `Button::virtual_2`, both minor; entries 30
  and 36). The 257 auto-noise pairs not in table A are in list B.
- **494 unpaired Lupo 2 functions** placed in the area (framework 225,
  arcade 137, app 55, race_menu 38, options 27, common 12), all assigned to a
  cluster (table C).
- About **15 Lupo 2 functions the function list does not have** (no frame
  information: static initialisers, leaves). I found them by hand: the option
  hook tables, the arcade factory tables, the mode decode and the real
  `shortmenu_root_check`.
- The **GT3 functions of the area with no Lupo 2 twin**, checked by file for
  removals (`us_unpaired.txt`).

### Cautions on the map in this area

- **Static initialisers and some leaves are not in Lupo 2's function list**
  (no `.eh_frame` entry). They never show up as "unpaired", and their GT3
  twins stay unpaired. I found these by hand:

  | L2 | What | GT3 twin |
  |---|---|---|
  | `0x00138EF8` | option hook table | `T064_static_init` |
  | `0x0013A9E8` | option multis | `T066_static_init` |
  | `0x00116E28` | arcade item factories | GT3 `T030_static_init` |
  | `0x001148E0` | `menu_*` button-list factories | (none) |
  | `0x00106068` | arcade movie table | `T029_static_init` |
  | `0x001B8B18` | menu movie params | (none) |
  | `0x0016FE18` | short-menu items | `T113_static_init` |
  | `0x0016EA00` | `shortmenu_root_check` | (none) |
  | `0x00106E18` | mode decode | (none) |
  | `0x001119F8` | page focus hooks | (none) |

- **Tail calls and Lupo 2's virtual calls (first pass).** The first tool
  version did not count tail calls (`j`). It also showed Lupo 2's
  computed-slot virtual calls as `vslots indirect`. Table A's verdicts
  "noise: tail call" come from that pass. The updated tool counts both.
- **Resolved slot numbers (second pass).** Many object vtables are shifted
  by one slot in Lupo 2 (`Object`-based calls 3 → 4, `ButtonList` 9 → 10,
  `Car::~Car` 87 → 88, `Slider` 141 → 142). The calls into race objects
  shift by the race vtables' growth (`ShortmenuRender::render` 44 → 49,
  `ShortmenuLicenseRender::render` 10 → 52). Some "slots" are function
  pointers mis-read as virtual calls (`Button::isAvailable` 39,
  `openClassList` 347, `runCarSelect` 1818, `main_arcade` 196). The real
  ones are noted in entries 13 (new `ArcadeControl` virtuals 2 / 4 / 5), 16
  (`runLevelPage`), 30 (`drawFocus`) and 34 (`normalize`, slot 0).
- **`L2 0x002968A0` is Lupo 2's throw / unwind helper** (GT3 `__throw`).
  Lupo 2 also inlines many small functions (`Object::Object`, `normalize`,
  the `ShortMenu*Entry::clear`) and emits inline accessors out of line
  (`.gnu.linkonce`: 144 in this area).
- **Wrong pairs** (each listed in table A):
  - `PageLayout::findHiddenItem` ↔ `CallbackItem::setItem` are swapped (all
    their callers show the swap).
  - `ArcadeControl::setup` → L2 `0x0022BD10` (a ProductConfig getter; the
    real twin is L2 `0x001068E0`).
  - `runMenu` / `runILinkMenu` / `F_00115C80` / `setLinkMode` → the Concept
    text items `available_car_num_*` / `achievement_ratio`.
  - `shortmenu_root_check` → L2 `0x001BC8B8`, which is
    `PageManager::back`.
  - `option_multi_set_wide_mode` / `option_action_set_default` /
    `option_action_stop_music` → the Concept car-get / simple-page loaders.
  - `game_init` → also L2 `0x0013B710` (memory card, part B).
  - `render_thread_release` → L2 `0x0013C5C8` (memory card icons).
  - `RaceIOPLoadModules::load` / `cmdInitClients` → L2 `0x00102CC8` callers.
  - `curveLookup` → L2 `0x001F3B18` (13 vs 90 instructions). **Corrected
    in the delta review:** this pair is right. The old leaf bounds stopped
    at an early `jr`, and the function is 86 instructions.
  - `ButtonList::virtual_1` → a `pList` sort.
  - `RawList::virtual_9` / `virtual_20` → memory card list overrides.
  - `Item::virtual_2` → L2 `0x001BBB18`, a derived draw (the true twin is L2
    `0x001BFE48`, difference 0.00).
- **`ArcadeControl` layout**: `ControlBase` grew by `0x10` (so `+0x288` →
  `+0x298`), and the arcade fields moved by `+0x70` (`mode` `+0x3A2C` →
  `+0x3A9C`) and more after them. **The GameStatus offsets quoted below
  (`+0x315C4` nextMode, `+0x35F50..+0x35FB8` menu requests, `+0x30C30`
  GameOptions) are Lupo 2's layout** (part B: "Save data layout changed").

## Entries

### 1. Boot splash images (DisplayAdvertiseImage)

| Field | |
|---|---|
| What | Before the menus, four images of `advertise/startupiw.img` are shown one after the other, each with a 15-frame fade in, a timed hold and a 15-frame fade out. GT3 shows the memory card warning and then plays `stream/opening.pss`. Lupo 2's `main` calls neither (`memorycard_warning` and the opening `play_movie` have no call). |
| Where | L2 `0x00103800` (the sequence), `0x001034A8` (loads the image file and relocates it), `0x00103520` / `0x00103550` (constructor / destructor), `0x001035B8` (init: image, next object, seconds), `0x001035F8` (update: fade in, hold, fade out, then hands the render object on), `0x001036E8` (draw: textured sprite), `0x001034F0` (free). Class `(anon advertise.cxx)::DisplayAdvertiseImage` (a GameObject), new. Called from L2 `main` `0x001000D0`, which is GT3 `main` (`T000_main.cpp`, US `0x00100140`). |
| Kind | new class; changed logic (`main`) |
| Evidence | String `advertise/startupiw.img` (L2 only). `0x001035B8` stores fade = 15 and hold = seconds × 30 (`0x41F0` = 30.0). Four `0x001035B8` calls with image indices 0..3. The image is handed over through `render_set_object` and a semaphore wait (`0x00102180`). In `main`, `game_boot` is followed directly by `0x00103800`, then `0x00142C30` (entry 2). |
| Port | A GameObject and a 4-image file (asset from GT Concept). |
| Verdict | **do not port** (promotion; replacing the warning would remove a GT3 feature; could be added in front of it if ever wanted) |

### 2. Boot: language from a GT3 save, autoload only without one (?)

| Field | |
|---|---|
| What | At boot Lupo 2 first runs L2 `0x00142C30`: memory card port 0 is checked. If a GT3 game file loads, its language is restored into a scratch GameStatus and returned, and `main` calls `Country::set` with it and goes on to the menus. Only when no such file is found does it run the autoload screen (`F_0014AE00` mode `0x1B`, then `Country::set` of the loaded language through L2 `0x001293D0`). GT3 always runs the autoload and resumes a championship left running. |
| Where | L2 `main` `0x001000D0`; `0x00142C30` (new; memory card); `0x001293D0` (new: `Country::set` plus two unit-setting calls); `0x00129590` (empty stub). GT3 `main` (`T000_main.cpp`). |
| Kind | changed logic |
| Evidence | `0x00142C30`: `MemorycardAutoLoad::checkPort`, `GameStatus::GameStatus` (scratch), `MemorycardGameFile`, `MemorycardGT3File::load`, `serialize_create` / `restore`, returns -1 or the language. `main`: `beq v0, -1` decides between the two paths. |
| Port | Belongs to part B (memory card; Concept reads GT3 saves, see menus_b entry 13). |
| Verdict | **do not port** (GT3 keeps its autoload) |

### 3. The arcade is the top menu; main dispatch changes

| Field | |
|---|---|
| What | (a) `main`'s loop calls `main_arcade` where GT3 calls `gtmode_main`. When it returns 0 (idle) one attract demo runs (entry 4). Otherwise `main` dispatches on `GameStatus nextMode` (34 entries, was 27). (b) `main_arcade` itself returns when the arcade menu leaves a non-zero `nextMode`. The arcade menu can therefore open the options, the memory card screens, replays and the logger, which in GT3 only GT mode's top menu could do. (c) New main modes: **32** `play_movie(stream/demoGT3A.pss)` (an A-spec demo movie, reached from the top menu's `aspec_demo` button (?)), and **33** an **i.LINK linked attract demo**. Mode 33 is repeated until it returns non-zero: L2 `0x00100EF8` with `0x00100C48` exchanges a random demo choice over i.LINK packets, then runs a `RaceLinkDemo`. The modes kept are 2..9, 11..13 and 24..26 with GT3's meaning. They call the unpaired twins of GT3's `main_*` helpers. |
| Where | L2 `0x001000D0` (= GT3 `main`, unpaired). Jump table L2 `0x00335280` (34 entries). L2 `0x00100EF8`, `0x00100C48` (new). L2 `0x001014A8` = US `main_arcade` (`0x00100DB8`). Movie params L2 `0x002DDDA8..0x002DDE28` filled by L2 `0x001B8B18` (unlisted static init). |
| Kind | changed logic; new modes |
| Evidence | `main`: `jal main_arcade; bnez v0` → dispatch, else `jal 0x00100998` (attract). Table: modes 32 → `0x00100358` (`play_movie(0x002DDE28)` = `demoGT3A.pss`) and 33 → `0x00100368`. `main_arcade` `0x001015C0`: `lw 0x315C4(gs)`, then `bnez` sets done. `0x00100EF8` references vtable `12RaceLinkDemo` (Lupo-2-only class). |
| Port | GT3 must keep `gtmode_main` as the top menu. The `nextMode` return from `main_arcade` is only useful together with a Concept-style arcade top menu. Mode 32 needs `demoGT3A.pss`. Mode 33 needs `RaceLinkDemo` (race area) and two linked consoles. |
| Verdict | **removed in Lupo 2** for the GT-mode top menu (not ported); modes 32 / 33: **do not port** (optional) |

### 4. Attract mode: one demo per idle, replay-file demos

| Field | |
|---|---|
| What | GT3 runs up to five attract demo races, then the opening movie, then the menus again. Lupo 2 runs **one** demo each time the top menu times out, chosen in turn from the GCF demo list. A demo entry whose kind is **1000** is a **recorded replay file**: it is read from the disc, restored with `Serialize` and played as a `RaceReplayInformation` race, a `RaceSolitaire` when its control records have slots. Other entries run an arcade demo race as before (no licence-S requirement loop (?)). |
| Where | L2 `0x00100998` (dispatcher, new), `0x00101240` (replay demo, new), `0x00100A28` (demo race, unpaired twin of GT3 `run_attract_demo` US `0x00100A78`). Demo counter in L2 `gp-0x6970`. GCF getters L2 `0x0022C398` (count), `0x0022C428` (kind), `0x0022C3A8` / `0x0022C3C8` / `0x0022C3E8` / `0x0022C408` (file / race / course / car), `0x0022BCE0` (the demo time limit, = GT3 `ProductConfig::F_0021D2B0`). |
| Kind | changed logic; new function |
| Evidence | `0x00100998`: `count = 0x0022C398(); n = (n+1) % count; if (0x0022C428(n) == 1000) j 0x00101240 else j 0x00100A28`. `0x00101240` calls `file_read_blocking`, `serialize_create`, `Serialize::Base::restore`, `RaceReplayInformation::createRace`, `__dynamic_cast` to `RaceSolitaire`. |
| Port | Needs a GCF demo "kind" field (part B: GCF data) and demo replay files (assets). The code is GT3's `play_saved_replay` reading from a file instead of the card. |
| Verdict | **port optional** (nice for attract variety; low priority) |

### 5. main_arcade: licence / training, Pod race (1P and 2P), link-battle changes

| Field | |
|---|---|
| What | The arcade selection now has 8 values, mapped to states as follows. Selection 1 → arcade race. 2 → time trial. 3 → **licence / training**: `run_concept_license` or `run_training`, chosen by `MenuStatus+0x62C`. 4 → free run. 5 → split battle. 6 → i.LINK battle. 7 → **Pod race**. 8 → **Pod race 2P**. The run modes stored in `MenuStatus.runMode` are renumbered (arcade race 7, free run 8, Pod 7, split and Pod 2P 2, link 4, licence 1, time trial 0). The i.LINK battle keeps its race `AEAT0001`, but the race gets `timeLimit = idleSeconds + 3` when the GCF idle time is set. After the race it runs `virtual_41(1, 1)` (the replay) when the i.LINK status says so. `MenuArgs` grows to 0x44 bytes and `selection` moves from `+0x24` to `+0x28`. |
| Where | L2 `0x001014A8` = US `main_arcade` (`T000_main.cpp`); state tables L2 `0x00335320` / `0x00335350`. New runners L2 `0x0012E6F8`, `0x0012F298`, `0x0012E018`, `0x00130CB8` (entry 9). |
| Kind | changed logic |
| Evidence | The state jump table has 10 entries (GT3 7). Selection table: 1..8 → states 2..9. Each state stores `runMode` (`sw v1, 0x315CC(gs)`) and calls its runner. The link battle calls `0x0022BCD0` (= GT3 `ProductConfig::idleSeconds`) and stores `v+3` at `info+0x50` (`RaceInformation::timeLimit`); `pdi1394_get_status` then decides a second `virtual_41`. The `printf("%d:%d:%s\n")` of each node is gone. |
| Port | Add states for the Pod runners and their selection values in GT3's `main_arcade` (keep GT3's numbering for the existing ones). The idle time limit is a kiosk feature (GT3's retail GCF idle time decides whether it does anything). The licence / training state depends on the race area's `RaceTraining` and on Concept licence data. |
| Verdict | **port** (Pod states); licence / training: **port only with the race area's training mode**; link-battle time limit: **do not port** (kiosk) |

### 6. run_arcade_race: car source, CVT forces AT, idle limit, post-race flow

| Field | |
|---|---|
| What | Lupo 2's arcade single race: (a) The car is the GameStatus **running car** (`gs+0x31A40`), set by the menu's `prepareRace` (entry 13). In GT3 it is `args->car1` with the menu's colour and tyres. (b) After the gearbox from `GameOptions.gearAuto`, a **CVT** car (`isCVT` = gearbox row `+0x11` == 2, see menus_b entry 1) gets `setEntryTransmission(0, 0)` (automatic). (c) The race's time limit comes from the GCF idle time when set (kiosk). (d) ASM / TCS / steering assist / vibration come from the options, as in GT3. (e) The post-race loop no longer calls `run_short_menu`: its actions come from the race (a 16-way switch over the race's result, replay `virtual_41(1,1)`, demo `virtual_8(2)` + `virtual_41(2,1)` + `virtual_35`, options, `setting_menu`, save replay `race_memcard(0x19)`) (?). (f) After the result, when L2 `0x00222440` finds new rewards, `NoticePresentControl` (`menu/notice_present.mbl`) is shown and the rewards are marked. At the end, `0x001378D8` (entry 7) runs. (g) GT3's version without these changes is still in the executable, uncalled (L2 `0x00129C18`). |
| Where | L2 `0x0012FCF0` = US `run_arcade_race` `0x0012F840` (`T052_arcade_race_modes.cpp`). Helpers L2 `0x002433B0` (`isCVT`), `0x0022BCD0`, `0x00222440`, `0x0015E3E8`, `0x00220D88`, `0x00220BB0`, `0x001378D8`. Dead copy L2 `0x00129C18`. |
| Kind | changed logic |
| Evidence | `0x00130190 jal 0x002433B0` on `runningCar+0x10`, then `setEntryTransmission(org, 0, 0)`. No `jal run_short_menu` in L2 `0x0012FCF0`; `0x001300E8` `sltiu v1, 0x10` + `jr` (action switch). The callees `NoticePresentControl` / rewards are absent from GT3. |
| Port | (b) is a one-line rule with part B's `isCVT` and the gearbox work (`docs/gearbox`). (a), (c), (e)-(g) follow from the Concept menu and progression. |
| Verdict | **port** (b) (CVT → AT in arcade races); **do not port** (a), (c), (e)-(g) |

### 7. Post-race rewards and unlock pages

| Field | |
|---|---|
| What | After an arcade race, a Concept licence or a training run, L2 `0x00222440` builds a list of new rewards from the prize sets, arcade records and licence records. `0x001378D8` then presents them by type. Type 0 cars → `car_get_prizes` (`CarGetConceptControl`, `menu/concept_car_get.mbl`) and a 30-frame pause. Type 3 → page `concept_moviea`. Type 4 → pages `concept_ace` and `concept_movieb`, then the long ending movie (`MOVIE1`). Type 1 → page `concept_pod` (the Pod unlocked). Type 2 → page `concept_billion`. The last reward becomes a menu request, and the rewards are marked given. |
| Where | L2 `0x001378D8` (new). Pages L2 `0x00166F38`, `0x001670F0`, `0x001672A8`, `0x00167460`, `0x00167618` (`SimplePageControl` wrappers over `0x00166EE0` / `0x00166C68` / `0x00166D78` / `0x00166D20`). Car get L2 `0x001664E0..0x001668B8` (`CarGetConceptControl`). Bookkeeping L2 `0x00222440`, `0x00220B38`, `0x00220C58`, `0x00220C48`, `0x00220D88`, `0x0021DFB8` (GameStatus, part B). |
| Kind | new functions / classes |
| Evidence | Strings `menu/concept_moviea.mbl` … `menu/concept_billion.mbl`, `concept_car_get` (L2 only). Each page is gated by `0x00220C58(list, type) > 0`. `play_movie(0x002DDDE8)` = `stream/ending_long_full.pss`. |
| Port | Concept progression content: GameStatus reward fields (part B), five `.mbl` pages and movies. |
| Verdict | **do not port** (GT3's GT mode has its own prize flow; the Pod unlock could be reproduced separately if the Pod is ported) |

### 8. Free run and split battle: assists reach the race; CVT in free run

| Field | |
|---|---|
| What | `run_free_run` now sets the entry's ASM and TCS from the options (`setEntryAsm` / `setEntryTcs`). It forces the automatic gearbox for a CVT and takes the GCF idle time limit. `run_split_battle` sets ASM / TCS for both players: from the options, or from each player's assist choice in the 2P car select (entry 17) through L2 `0x00161770`. It also calls `CarEquipments::setColorIndex` for both cars and the idle limit. In GT3 these runners leave ASM / TCS at the entry defaults. `run_time_trial` saves its record through a different API (L2 `0x00220870` / `0x00220898` instead of `TimeTrialRecords::enterTime` / `insertName`; part B). |
| Where | L2 `0x0012A310` = US `run_free_run` `0x0012FF48`; L2 `0x00130520` = US `run_split_battle` `0x00133F48` (`T054_split_battle.cpp`); L2 `0x0012AE80` = US `run_time_trial`. |
| Kind | changed logic |
| Evidence | `cmp`: only L2 has `setEntryAsm`, `setEntryTcs`, `0x002433B0`, `0x0022BCD0`. In the split battle: `lw 0xC8C(gs)` / `lw 0xC90(gs)` (GameOptions `assistAsm` / `assistTcs` in Lupo 2's layout) and, per player, two words of the menu's choice (`sp+0x3EA20` / `+0x3EA24`). |
| Port | Small: two calls per entry. The 2P per-player assist needs entry 17's 2P assist step. |
| Verdict | **port** (assists in free run / split battle); CVT rule with entry 6 |

### 9. New race runners: Pod race, Pod 2P, Concept licence, training

| Field | |
|---|---|
| What | **`run_pod_race`** (L2 `0x0012E018`): `RaceArcadeInformation` + `RacePOD` (+ `RacePODDisplay`), the short menu, trophy screen, results, car settings (`setting_menu`), options, replay save. It is the arcade race with the Pod race class. **`run_pod_2p_race`** (L2 `0x00130CB8`): `RaceSplitBattleInformation` + `RacePOD2P`, both players' colours and tyres, the short menu, per-player assists, `RaceDataBase::getCourse` / `unloadCourseData`, and a memory card call. **`run_concept_license`** (L2 `0x0012E6F8`): a licence test like GT3's `run_license_test` (`RaceMachineTestInformation`, `RaceLicense`, ghost, `ShortmenuLicenseControl`, records `RecordsMenuArgLicenseConcept`, name entry, ranking code), with Concept licence records (`0x00162520`), the present notice and rewards. **`run_training`** (L2 `0x0012F298`): `RaceTrainingInformation` + `RaceTraining` (`DynamicsConductorTraining`, `RACE_MODE_LICENSE`) with `ShortmenuConceptLicenseControl` (entry 27). |
| Where | The four L2 addresses above, all new; GT3 `T052` / `T053` / `T054`. |
| Kind | new functions |
| Evidence | `who.py`: `0x0018D498` / `0x0018D548` reference vtables `7RacePOD` / `14RacePODDisplay`; `0x0018D6A0` `9RacePOD2P`; `0x00187560` `12RaceTraining` with string `RACE_MODE_LICENSE`; all Lupo-2-only classes. |
| Port | Each runner is GT3's arcade / split / licence runner pattern around a new race class. The race classes, displays and dynamics are the race area's (Pod, training). The menu side needs entry 14's Pod entry. |
| Verdict | **port** the Pod runners (with the race area's Pod port); licence / training: **do not port** unless the race area takes `RaceTraining` |

### 10. Prize cars / endings after races stubbed

| Field | |
|---|---|
| What | GT3's runners call `F_0013F508(race, -1)` after a race (prize cars, endings). Lupo 2 calls L2 `0x001373E8`, which returns 0 at once. |
| Where | L2 `0x001373E8` (`jr ra; li v0, 0`); its callers are the runners of `run_arcade_race`'s dead copy, `run_license_test`, `run_gtmode_race` and `run_championship` (all twins). GT3 `F_0013F508` (`T061_car_get_control.cpp`). |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | `callee_diff.py`: every GT3 runner's `F_0013F508` ↔ L2 `0x001373E8`. |
| Port | — |
| Verdict | **removed in Lupo 2** (not ported) |

### 11. Busy mark of the loading transitions

| Field | |
|---|---|
| What | The "now loading" mark of `TransitionObject` is drawn as a strip, with its own ortho projection, rotated (`vu0_rotate_z`) and scaled each frame. GT3 draws two flat sprites. |
| Where | L2 `0x001029D0` = US `TransitionObject::drawBusyMark` `0x00102170` (`T005_transition.cpp`); strip helper L2 `0x00102940` (new). |
| Kind | changed logic |
| Evidence | L2 calls `pgl_ortho`, `pgl_push_matrix` / `pop`, `vu0_rotate_z` ×2, `vu0_scale` ×2, `0x00102940` (`pack_rgba_floats_to_u32`, `strip_draw`); GT3 `pgl_color3f`, `gs_draw_flat_sprite`. |
| Port | Self-contained draw code. |
| Verdict | **port optional** (cosmetic) |

### 12. Menu movies and demo file paths

| Field | |
|---|---|
| What | The menu-selectable movies and the demo file paths changed. (a) `play_movie` (unpaired twin L2 `0x001295E8` of GT3 `0x001287A8`) is used for menu requests. `MOVIE0` = `stream/opening_GT3UK.pss` and `MOVIE1` = `stream/ending_long_full.pss` (entry 15), plus main mode 32 and the rewards. (b) The demo file manager and the demo setting loader build their paths differently (`demo/%s%s`, `demo/thumbnail/%s%s`, `%s%d` instead of `demo/%s` / `demo/thumbnail/%s`). |
| Where | L2 `0x001295E8`; params L2 `0x002DDDA8..0x002DDE28` (static init L2 `0x001B8B18`); L2 `0x00145498` = US `MemorycardDemoFileManager::buildList` (`T079_demo_file_manager.cpp`); L2 `0x00162EE0` = US `load_demo_setting`. |
| Kind | changed data / logic |
| Evidence | `consts.py 0x001B8B00..`: the five stream names stored into the five params. String differences in table A. |
| Port | Assets; GT Concept's disc layout. |
| Verdict | **do not port** |

### 13. ArcadeControl becomes a base class; ConceptControl

| Field | |
|---|---|
| What | `ArcadeControl` has 7 new virtual methods, which `ConceptControl` (GT Concept's menu, `menu/concept.mbl`) overrides. Slot 2: car code per class. Slot 3: car count. Slot 4: colour chips (`SelectBox` / `SelectBox2`). Slot 5: course availability. Slot 6: the status page. Slot 7: load (GT3's `load`). Slot 8: `prepareRace`, which fills `MenuArgs`, sets the GameStatus running car (by code, copy or garage car), the race code and `MenuStatus.setEvent`. The menu set-up is GT3's `ArcadeControl::setup` (twin L2 `0x001068E0`, wrongly paired by the map with L2 `0x0022BD10`). It also reads GameOptions' assist level into the menu (entry 17). The finish writes it back (L2 `0x00107AE8`, `setAssist`). GT3's own arcade entries survive without callers: L2 `0x00103D28` / `0x00103DF0` (twins of `run_arcade_menu` / `arcade_menu`), `0x00103F30`, `0x001040C0`. New fields: `podRace +0x3C48`, course / class mpeg images `+0x3ABC` / `+0x3ADC`, ConceptControl `+0x3C50..+0x3C58`. The difficulty is GT3's level field, moved to `+0x3AA0`. |
| Where | Vtables `Q29gt2k_menu13ArcadeControl` (2 → 9 slots) and `Q29gt2k_menu14ConceptControl` (new). L2 `0x001041E8` (ConceptControl constructor), `0x00104230` (load: `menu/concept.mbl`, `stream/course.ipic`, music), `0x0010CA90` / `0x0010CB98` / `0x0010CC70` / `0x0010CCB0` (slots 2 / 3), `0x0010D630` / `0x0010D700` (slot 4), `0x001046C8` / `0x00104750` (slot 5), `0x0010FDF8` / `0x0010FE00` (slot 6), `0x001067E8` (slot 7, = US `load`), `0x00107168` / `0x001074E8` (slot 8). Accessors (linkonce) `setPodRace` `0x002B2640`, `get/setDifficulty` `0x002B2630` / `0x002B2638`, `getCurrentGame(Index)`, `setCourseMpegImage` / `setClassMpegImage`, `getMemoryForCar`. GT3 `T010_arcade_control.cpp`. |
| Kind | changed logic; new class |
| Evidence | `lupo2.py class ArcadeControl` / `ConceptControl`. `0x001041E8`: `ArcadeControl::ArcadeControl`, then the ConceptControl vtable stored at `+0x298` and fields `+0x3C50` = -1, `+0x3C54` = 9, `+0x3C58` = 4. The constructor offsets / sizes in table A (`+0x70` shift). |
| Port | Only the pieces a ported feature needs: `podRace` and the mode decode (entry 14), the assist read / write (entry 17). The polymorphic split itself is not needed in GT3. |
| Verdict | **port partly** (as needed by 14 / 17); the ConceptControl menu: **do not port** |

### 14. Arcade modes, mode decode, Pod race entry, race codes

| Field | |
|---|---|
| What | **Modes.** `setModeByName` maps the button names: `single_race` 0, `compulsory` 1, **`license` 2**, `time_attack` 3, `two_player_battle` 4, `goodies` 5, **`status` 6**, `load_guest_garage` 7, `i_link_battle` 8. GT3 has 0..6 without license / status. **Mode names**: `SINGLE_RACE`, -, -, `TIME_ATTACK`, `TWO_PLAYER`, -, **`POD_RACE`**, -, `I_LINK`. **Decode** (mode → `MenuArgs.selection`): 0→1, 1→2, 2→3, 3→4, 4→5, 8→6. Mode 6 → 7 when `podRace` == 1 and 8 when == 2 (`setPodRace`). **Race codes**: per car and class from the GCF (`ProductConfig::carRace`), with the difficulty added to the last character. Garages as GT3. Compulsory / licence / Pod from GCF categories `CLASS_COMPULSORY` / `CLASS_LICENSE` / `CLASS_PODRACE`. The Pod 2P race is `APO20001`. Other fixed codes: `ATAF0001`, `A2P0001`, `ALNK0001`. GT3's `CLASS_COMPULSORY` string race code is gone. |
| Where | L2 `0x00114A20` = US `setModeByName` (`T028_arcade_menu_hooks.cpp`); names L2 `0x002D8298`; decode L2 `0x00106E18` (unlisted, new); `setPodRace` L2 `0x002B2640`; L2 `0x00113E68` = US `ArcadeControl::raceCode` (`T026_arcade_race_code.cpp`); L2 `0x0010CD68` (class 7..10 car index remap). |
| Kind | changed logic; new data |
| Evidence | String compares and constants in `0x00114A20` (`str_equal` × 9, `li 0..8`). Jump table L2 `0x00335B70`. `0x00106E68`: `lw 0x3C48`, `== 1 → 7`, `== 2 → 8`. gp strings `CLASS_COMPULSORY` / `CLASS_LICENSE` / `CLASS_PODRACE` (L2 `gp-0x48E8..-0x48E0`). |
| Port | Add a Pod mode (name, decode to 2 selections) to GT3's arcade with its own button in GT3's arcade layout (asset change), and a GCF category / race for the Pod. |
| Verdict | **port** (Pod entry only); the other mode renumbering: **do not port** |

### 15. Top menu "ar/push_start_wagen" and menu requests

| Field | |
|---|---|
| What | GT3's push-start page waits for start / decide. Lupo 2's `ar/push_start_wagen` is a **navigable top menu**: the page runs `PageManager::update` and its buttons. It **remembers the last page and cursor** in `MenuStatus` (`+0x35F54` page name, `+0x35F50` cursor), so returning from a race restores the position. On the first entry after boot it times out after 1200 frames to the attract (the `ControlBase` time limit GT3 already has). `TopButtonList::createButtons` drops the `aspec_demo` button when `Country` == 8. The menu can end with a request string in `MenuStatus+0x35FB8`, handled by L2 `0x00103BF0`: `MOVIE0` / `MOVIE1` play a movie (entry 12), `TRANSFER` runs memory card mode `0x1E` (part B: transfer to a GT3 save), `PODRACE` returns to `main_arcade`. |
| Where | L2 `0x00108AB8` = GT3 `ArcadeControl::runPushStartPage` (US `0x001156F8`, `T012_arcade_push_start.cpp`; unpaired); runs L2 `0x00108728`, `0x00108580`; requests L2 `0x00103BF0`; L2 `0x001B8788` = TopButtonList override (map: `ButtonList::createButtons`, wrong pair). |
| Kind | changed logic |
| Evidence | `0x00108AB8`: `strlen(ms+0x35F54)`, `setPage(saved or "ar/push_start_wagen")`, `setCursor(ms+0x35F50)`, `PageManager::update` loop, `strcpy` / `sw` back. `0x00108728`: `sw 0x4B0, 0x284(this)` (time limit). gp strings `MOVIE0`, `MOVIE1`, `TRANSFER`, `PODRACE`. `0x001B8788`: `Country == 8 && str_equal(name, "aspec_demo") → skip`. |
| Port | Remembering the page / cursor across races is small (two MenuStatus fields; GT3's save layout must not change, so keep them in memory only). The page itself is a Concept layout asset. |
| Verdict | **port optional** (remembered cursor); the rest **do not port** |

### 16. New arcade pages: status, show room, licence select, category, game / difficulty lists

| Field | |
|---|---|
| What | **Status page `ar/status` then `ar/show_room`** (L2 `0x0010FE00`, `0x0010FE98`). It shows a map of prize cars (`StatusButtonList`, "menu_map": prize car codes and short names, obtained or not from `PrizeSetFlags`, `StatusButton` drawing), the completion `achievement_ratio` (`%d%%` from `GameStatus::getCompletionRatio`), `available_car_num_S/A/B/C/D` (`%d/%d`), and status texts for the running car (name, price, odometer, power). **Licence select `ar/select_license`** (L2 `0x001102E0`): a car per licence test (`ProductConfig::carCount`, `createCarView`), a licence list (`LicenseButtonList`, gold / silver / bronze medal items `item_*`, `license_*`, `rank_%d`), event names `C%02d%c%d` / `MOVIE%d`. **Category select** (`ar/select_category`, `CategoryButtonList`, `stream/category_gtc.pss`). **Difficulty list** (`DifficultyButtonList` "menu_difficulty", replacing the level page's list; `runLevelPage` builds it from the page's items). **Game list** (`GameButtonList`). **`ar/connect_ride`**, a second i.LINK connect page with the same focus hook. |
| Where | Pages L2 `0x0010FDF8..0x00110540`; factories L2 `0x00114368..0x00114830` registered by L2 `0x001148E0` (unlisted); lists / buttons L2 `0x00117E48..0x0011B748`; item factories L2 `0x00116E28` (unlisted) with L2 `0x00115B28`, `0x00116310..0x00116978`, `0x00116CF8`, `0x00116D68`, `0x0011BC40..0x0011C0C0`; GT3 `T030_arcade_item_factories.cpp`, `T031_arcade_pages.cpp`. |
| Kind | new functions / classes / data |
| Evidence | Strings `ar/status`, `ar/show_room`, `ar/select_license`, `ar/select_category`, `ar/connect_ride`, `game_status`, `license_gold`, `item_gold`, `achievement_ratio`, `available_car_num_*`, `menu_category` / `menu_difficulty` / `menu_game` / `menu_license` / `menu_map` / `menu_top` (all L2 only). |
| Port | Each page needs GT Concept's `.mbl` layouts, movies and GameStatus fields (licence medals, prize sets, completion: part B). The status page is the most self-contained (it reads GT3-like prize flags and records). |
| Verdict | **do not port** (Concept content); status page: possible later with assets |

### 17. Car select: assist (ESP) choice, tuner logo, colour chips, removed panels

| Field | |
|---|---|
| What | **New**: an assist step (`assist_title`, `esp_on_yes/no`, `esp_off_yes/no`) per player, also in the 2P select. It is GameOptions' assist level (entry 23), read at menu set-up and written back at the end. In 2P, each player's choice goes to the split battle (entry 8). Also new: the tuner logo (`tuner_%s`) of the car, the gearbox title items (`gear_title`), colour chips (`SelectBox` 1P, `SelectBox2` with `ball` / `circle` / `frame` for 2P) with the colour name, a drivetrain icon `none`, and power / torque from the car's equipment (`CarEquipments::getMaxPower`, the tuned value). **Moved**: the handling / speed / acceleration gauges are shown on car change, with `BarGauge`. **Removed**: the car spec panel (`Length` / `Width` / `Height` / `WheelBase` texts, GT3 `specPanel*`), the gauges and spec labels of the 2P car list, the colour selection page (`color_selection`), the racing-modification and driver toggles (`racing_yes/no`, `dri_yes/no`), the blur / mini-car animation (now transitions). |
| Where | L2 `0x0010E4D0` = US `buildCarSelect`; `0x0010F968` = US `runCarSelect`; `0x0010C2E8` / `0x0010C6E8` = US `carSelect2PStep` / `Abort`; new L2 `0x0010BC18` (assist panel), `0x0010BEE0` (assist input), `0x0010BFF0` (close), `0x0010DE90` (car change + tuner + gauges), `0x0010ABF8`, `0x0010AF18`, `0x0010D630`, `0x0010D700`; GT3 `T021_car_select_2p.cpp`, `T023_arcade_car_select.cpp`. Assist L2 `0x00226AE8` / `0x00226AC0` from `0x001068E0` / `0x00107AE8`. |
| Kind | changed logic; new functions |
| Evidence | String differences in table A (`buildCarSelect`, `openCarList`, `openClassList`). `strs.py gone`: `Length\n%dmm` and others, `color_selection`, the `*_2p` gauges. `0x001068E0` calls `0x00226AE8` (`getAssist`), `0x00107AE8` calls `0x00226AC0` (`setAssist`). |
| Port | The assist step is gameplay. It needs the preset table and accessors (entry 23), a car-select page item (layout asset), and the per-player 2P path (entry 8). The UI changes are Concept layout. |
| Verdict | **port** (assist / ESP choice); the rest **do not port** (and keep GT3's spec panel, colour selection, racing toggle) |

### 18. Class and course select reskin

| Field | |
|---|---|
| What | Class select shows class pictures from an mpeg ipic (`mini_classes`, `%s_logo`, `cross_bar`) instead of mini cars. Course select uses course pictures (`%s_mini1`, `stream/course.ipic`) with a new `MiniCourseCursor`, separate left / right arrows, and mpeg course / class items. GT3's per-course item factories `course_logo_NN`, `course_name_NN`, `mark_<level>_NN`, `prize_<level>_<x>` (T030) have no Lupo 2 twin. |
| Where | L2 `0x001047C0`, `0x00104868`, `0x00104AA0`, `0x00104DC0` (class); `0x00111E38`, `0x00119190`, `0x001132B0`, `0x00113B90`, `0x00115348`, `0x00115370` (course); L2 `0x001138C0` / `0x00113810` = US `createCourseScreen` / `miniCourseName`; GT3 `T022_class_select.cpp`, `T025_arcade_course_select.cpp`, `T030`. |
| Kind | changed logic; removed data |
| Evidence | L2-only strings `mini_classes`, `%s_logo`, `%s_mini1`; GT3-only T030 static-init strings (`strs.py gone`). |
| Port | Asset-driven. |
| Verdict | **do not port** |

### 19. Arcade classes from the GCF

| Field | |
|---|---|
| What | The player's class name comes from the product config (L2 `0x0022BDB0`: a category table, with `CLASS_C/B/A/S` names built once from gp data). GT3 has a fixed `getClassName` table. Classes 7..10 are remapped through `MenuStatus+0x36000` (L2 `0x0010CD68`). |
| Where | L2 `0x00107C60` = US `getPlayerClassName` (`T010`); L2 `0x00106FF8`, `0x0010CD68`; GT3 `T435_product_config.cpp`. |
| Kind | changed logic |
| Evidence | `getPlayerClassName`: `j 0x0022BDB0(product_config, class)` instead of `getClassName`. |
| Port | Only with the category / licence / Pod classes (entries 14, 16). |
| Verdict | **port only with 14 / 16** |

### 20. Arcade background movies

| Field | |
|---|---|
| What | The menu movie table is now `stream/arcade_menu_gtc.pss`, `class_c_full.pss`, `category_gtc.pss`, `car_gtc.pss`, `midfield_full.pss`, `status_gtc.pss` and `stream_jp_unused/alpha_test.pss` (an alpha-movie test, entry 32). It was `arcade_top`, `arcade_level`, `laguna_full`, `menu_bg`, `menu_bg2`, `arcade_menu_full` and `level_menu_full`. |
| Where | L2 `0x00106068` (static init, unlisted) = GT3 `T029_static_init` (`T029_arcade_movies.cpp`). |
| Kind | changed data |
| Evidence | `consts.py 0x00106068 0x001061D0`. |
| Port | Assets. |
| Verdict | **do not port** |

### 21. Goodies page and history labels removed

| Field | |
|---|---|
| What | `ArcadeControl::runGoodiesPage` (`ar/goodies`) and the history labels of the top page (`history_level_*`, `history_area_*`, `history_class_*`) have no Lupo 2 twin and their strings are gone. The goodies button list itself is still registered (`menu_goodies`). Also gone: `stream/ending_short_full.pss` of `arcade_menu`, and the not-get-car page (`menu/not_get_car.mbl`, part B). |
| Where | GT3 `T031_arcade_pages.cpp` (`runTopPage`, `runGoodiesPage`), `T015_arcade_menu.cpp`. |
| Kind | removed in Lupo 2 |
| Evidence | `strs.py gone`; `us_unpaired.txt`. |
| Port | — |
| Verdict | **removed in Lupo 2** (not ported) |

### 22. ar/select_car_test debug tool

| Field | |
|---|---|
| What | A test car-select page that reads camera data from `host0:/disk/transient/mat/camera.txt`, prints `pushed select` / `pushed start when paused`, and has a `FOR_MOVIE` mode. It is reached through ConceptControl entry L2 `0x001040C0`, which has no caller. |
| Where | L2 `0x001092B0` (new), `0x001089C8`, `0x001040C0`. |
| Kind | new function (debug) |
| Evidence | Strings above (L2 only). |
| Port | — |
| Verdict | **do not port** (debug) |

### 23. Options: "assist" level (multi_assist, assist_0/1/2)

| Field | |
|---|---|
| What | A new option `multi_assist` with `assist_N_on/off` hooks chooses one of four presets of {TCS, ASM}: 0 = {on, on}, 1 = {on, off}, 2 = {off, off}, 3 = {off, on}. GameOptions keeps `assistTcs` / `assistAsm` (`+0x60` / `+0x5C`). The level is computed from them by table search, and setting a level writes both. The arcade car select uses the same level (entry 17). |
| Where | Hooks L2 `0x00138350..0x00138428` (new), registered in L2 `0x00138EF8` (unlisted option hook table = GT3 `T064_static_init` `0x00140C20`). Multi L2 `0x0013A970` / setter `0x0013A950`, registered in L2 `0x0013A9E8` (= GT3 `T066_static_init`). Accessors L2 `0x00226AE8` (get) / `0x00226AC0` (set), table L2 `0x002DF5C8`. GT3 `T064_option_hooks.cpp`, `T066_option_multis.cpp`, `T428_game_options.cpp`. |
| Kind | new function; new data (table) |
| Evidence | `cmpnames.py`: hook names `assist_0_on` … `assist_2_off` and multi `multi_assist` only in L2. Table words `1,1, 1,0, 0,0, 0,1`. |
| Port | Four small functions and an 8-word table over existing fields (no save change). It needs a `multi_assist` item in GT3's option page layout (asset), and GT3's separate ASM / TCS options can stay. |
| Verdict | **port** (with a layout item; or only as the car-select step of entry 17) |

### 24. Options: screen position adjust

| Field | |
|---|---|
| What | Two new sliders `slider_screen_adjust_h` / `_v` and texts `screen_adjust_h` / `_v` (range -16..16) edit GameOptions `+0x64` / `+0x68`. In GT3 these bytes are part of the reserved `unk_064[0x6C]`, which defaults to 0 and is saved. The values move the picture through a new `gfx_set_display_offset(2·h, 2·v)` (the DISPLAY2 DX / DY offsets), applied at once when changed (entry 40). |
| Where | Sliders L2 `0x001386B0` / `0x001386D0` (range), texts L2 `0x00138770` / `0x00138790` (`%2d`), registered in L2 `0x00138EF8`. Defaults zeroed in L2 `0x00226950` (= US `GameOptions::setDefaults`). GT3 `T065_option_sliders.cpp`, `T064_option_hooks.cpp`. |
| Kind | new function; new data (GameOptions fields in reserved space) |
| Evidence | `li -16`, `li 17`, `lw 0xECC(a0)` / `0xED0(a0)` (OptionControl `+0xE68` = GameOptions, so `+0x64` / `+0x68`). `setDefaults` stores 0 there. |
| Port | Option hooks (small) plus the display offset in the GS display code (render area). It is save-compatible: the fields are in GT3's reserved area. Option page layout asset. |
| Verdict | **port** (with the render area's display offset) |

### 25. Options page layout

| Field | |
|---|---|
| What | `OptionControl::load` loads `menu/option_1p_limited.mbl` instead of `menu/option.mbl` (a cut-down 1P layout). The hook names of GT3 are all still registered. The options entry also allocates its 1 MB menu heap with `memalign` instead of on the stack (plumbing). |
| Where | L2 `0x00137C28` = US `OptionControl::load` `0x0013FA88`; L2 `0x00138008` = US `F_0013FE58` (`T062_option_control.cpp`). |
| Kind | changed data; backend |
| Evidence | Strings; `memalign(0x40, 0x100000)` / `free`. |
| Port | — |
| Verdict | **do not port** |

### 26. Short menu: which buttons are available

| Field | |
|---|---|
| What | Lupo 2's `shortmenu_root_check` adds two gates. `load_ghost` is shown only when a new flag `ShortMenuArgs+0x20` is set; `demonstration` only when `+0x28` is set. `replay` / `save_replay` move to `+0x24`. `setting`, `qm_setting`, `save_game` and `qm_save_game` are **always hidden**. `save_setting` / `load_setting` / `exit` / `free_session` keep GT3's flags. |
| Where | L2 `0x0016EA00` (unlisted; the map paired GT3's function with `PageManager::back` L2 `0x001BC8B8`) = US `shortmenu_root_check` `0x0012C430` (`T111_shortmenu_control.cpp`). |
| Kind | changed logic |
| Evidence | The strcmp chain: `replay`, `load_ghost`, `save_replay`, `save_ghost`, `data_logger`, `setting` (returns 0), `qm_setting` (0), `save_setting`, `load_setting`, `exit`, `save_game` (0), `qm_save_game` (0), `free_session`, `demonstration` (`lw 0x28`). |
| Port | Two new flags and two `strcmp` gates. They only matter when a race mode sets the flags (the time-trial ghost load and the demonstration lap are race-side features). Keep GT3's setting / save_game rules. |
| Verdict | **port** the two gates with the race modes that set them; always-hidden buttons: **removed in Lupo 2** (not ported) |

### 27. Concept licence short menu

| Field | |
|---|---|
| What | A second licence short menu for the training / Concept licence. `ShortmenuConceptLicenseControl` with `ShortmenuConceptLicenseRender` draws over a captured copy of the race picture (`GSBuffer::copy`). Its pages are `menu/concept_qm_license.mbl` / `concept_qm_license_lead.mbl`, and `menu/concept_qm_arc_single.mbl` is listed beside them. The ordinary licence short menu is unchanged (twins). |
| Where | L2 `0x00161B98` (runner), `0x00163B28`, `0x001638E8`, `0x00163928`, `0x00163948`, `0x00163968`, `0x00163988`, `0x00163B18..0x00163FD0` (new); table L2 `0x002D9FB4..`. GT3 `T103_shortmenu_license.cpp`. |
| Kind | new class |
| Evidence | Lupo-2-only classes `ShortmenuConceptLicenseControl` / `Render`; strings above. |
| Port | With entry 9's training mode. |
| Verdict | **port only with the training mode**, else **do not port** |

### 28. Licence times in the short menu

| Field | |
|---|---|
| What | `qm_license` no longer divides the times by 1000 (three `/1000` gone); the time unit changed on the race side. |
| Where | L2 `0x0016FA88` = US `qm_license` `0x0012D540` (`T113_shortmenu_items.cpp`). |
| Kind | changed logic (minor) |
| Evidence | `cmp`: imms `1000 ×3` only in GT3. |
| Port | Follows the race area's licence timing. |
| Verdict | **do not port** (unless the race area changes the unit) |

### 29. New menu widgets

| Field | |
|---|---|
| What | These widgets are used by the Concept screens: `BarGauge` (a `CircleGauge` subclass drawn as a bar, on `CarDatabase` data); `SelectBox2` / `ColorBox2` (colour chips drawn as ball / circle / frame); `MiniCourseCursor` (a `MiniCursor` subclass with a 2224-byte draw); `MpegImageAlpha` (entry 32); `StatusButton(List)`, `GameButton(List)`, `LicenseButton(List)`, `CategoryButtonList`, `DifficultyButtonList`; `SimplePageControl` (a one-page control with a `delay` page); `NoticePresentControl` (part B). The `ButtonList::createMultiButton` path builds a sub-list for a `MultiButton` (L2 `0x001D7540`). |
| Where | Table C (W1 rows); vtables in `classes_L2.csv`. GT3 `T032_circle_gauge.cpp`, `T035_select_box.cpp`, `T235_color_box.cpp`, `T024_mini_cursor.cpp`, `T244_button_list.cpp`, `T269_multi_button.cpp`. |
| Kind | new classes |
| Evidence | Lupo-2-only class names (`lupo2.py classes`) and their typeinfo / destructor linkonce functions. |
| Port | Only with the screens that use them. |
| Verdict | **port with 16-18** (otherwise do not port) |

### 30. Button: selectable flag and a new virtual

| Field | |
|---|---|
| What | `Button::isAvailable` first tests a new "selectable" flag (`+0x140`, `setSelectable`), then calls the availability hook as in GT3 (hook and context moved to `+0x144` / `+0x148`). `Button::virtual_2` calls `drawFocus` through the vtable (GT3: a direct call), so subclasses (`StatusButton`, ...) draw their own focus. A new empty virtual is inserted at slot 15, so the vtables of `Button`, `FreeButton`, `MultiButton`, `ConcurrentButton`, `MapButton`, `TopButton`, `RecordsButton` and `Slider` grow from 18 to 19. `Slider` adds a previous-value "pinch" marker (`dispPrevPinch`) and double-range input (L2 `0x001C8818..0x001C8B50`; car settings, part B). |
| Where | L2 `0x001BA120` = US `Button::isAvailable` `0x001B3F18`; L2 `0x001BA8A8` (slot 15); `setSelectable` L2 `0x002CA600`. GT3 `T206_button.cpp`, `T238_slider.cpp`. |
| Kind | changed logic (plumbing) |
| Evidence | `isAvailable`: `lw 0x140; beqz → 0; lw 0x144; jalr` (GT3: `lw 0x140` is the hook). `lupo2.py class 6Button`: slot 15 L2-only (`jr ra`). |
| Port | Only with the screens that use it. A new virtual changes every button vtable, so add it only if needed. |
| Verdict | **port with 29** (otherwise do not port) |

### 31. Item::setText caches the text width

| Field | |
|---|---|
| What | When an item's layout entry is a text with flag 4 and alignment code `0x10` in bits 4..7, `setText` measures the string (`text_string_width` with the menu font) and keeps the width at `+0xE0`. Aligned dynamic texts then place correctly. |
| Where | L2 `0x001BF580` = US `Item::setText` `0x001B9F08` (`T213_menu_item.cpp`). |
| Kind | changed logic |
| Evidence | L2 tail: `lw 0x40(layout) & 4`, `lhu 0x2(...) & 0xF0 == 0x10`, `jal text_string_width`, `swc1 0xE0`. |
| Port | Small; uses an existing field (check that GT3's draw reads `+0xE0`). |
| Verdict | **port** (optional, small) |

### 32. Alpha movies and movie start

| Field | |
|---|---|
| What | `MenuBgMovie` gains an alpha mode. `MovieParam+0x18` → `+0x98`. In the update, each decoded frame's second half (by 64-pixel macroblock columns) is copied into the alpha bytes of the first half, so a movie can carry its own transparency mask. `MpegImage::movieUV` maps the half. `MpegImageAlpha` draws it (new upload path L2 `0x001CEDB8`). `start()` takes an optional file name that overrides the param's, and waits until the stream has a frame (L2 `0x0027A858`) before returning. GT3 can show a blank first frame. |
| Where | L2 `0x001CF118` = US `MenuBgMovie::update` `0x001CD1E0`; L2 `0x001CF468` = US `MenuBgMovie::start` `0x001CD458`; L2 `0x001CDB60` = US `MpegImage::movieUV`; new L2 `0x001CE148`, `0x001CEDB8`, `0x001CF048`, `0x001CF0B8`. GT3 `T253_menu_bg_movie.cpp`, `T249_mpeg_image.cpp`. |
| Kind | changed logic; new class |
| Evidence | `update`: `lw 0x98; beqz`, then the byte loop `sb a1, 0x3(a2)` over `0x7C` (width) / `0x80` (height) in 64-pixel columns. `start`: `sw 0x18(param) → 0x98`, an extra `a2` (file), and a loop `gfx_vsync(1)` until `0x0027A858(stream)`. |
| Port | The wait-for-first-frame is a small fix. The alpha mode only matters with alpha-encoded movies (assets). |
| Verdict | **port** the start wait (optional); alpha mode **with 29** only |

### 33. Menu car: model animations and new render settings

| Field | |
|---|---|
| What | The menu car (`gt2k_menu::Car`) steps its model set's animations every frame (`modelset_step_anims`, `modelset_apply_clut_anims`; reset at set-up), so animated models (the Pod's face / emotions, CLUT animations) move in the menus. New accessors: `setDirtiness`, `setMagnify` (camera projection), `setReflection`, `setAnti`, `setType`, `setEnvTexture`, `setViewport`. The object grew by `0x24`. |
| Where | L2 `0x001BE3B0` = US `Car::virtual_1` `0x001B8EC8` (`T212_menu_car.cpp`); wrappers L2 `0x0022DAE8`, `0x0022DB18`, `0x0022DC90` (car model area); linkonce L2 `0x002CAFB8..0x002CB090`. |
| Kind | changed logic |
| Evidence | L2 calls `0x0022DAE8` → `modelset_step_anims`, `0x0022DB18` → `modelset_apply_clut_anims`, `0x0022DC90` → `modelset_reset_anims`. GT3's `Car::virtual_1` has none. |
| Port | Needed for an animated Pod in the menus. Goes with the car-model area's model-set animation port. |
| Verdict | **port** (with the Pod) |

### 34. SmartIndexT: virtual normalize, wrapping move with skip

| Field | |
|---|---|
| What | `SmartIndexT<int>::set` / `setRange` call `normalize` through the vtable (GT3: a direct call). A new virtual (slot 4) moves with wrap-around (L2 `0x001CE6B8`). `SmartCarIndex` / `SmartCourseIndex` / `SmartSkipIndex` override it to repeat the move while `isValid` fails (L2 `0x00104420`), so locked entries are skipped. |
| Where | L2 `0x001CE578` / `0x001CE5B0` = US `SmartIndexT<int>::setRange` / `set` (`T251_smart_index.cpp`); L2 `0x001CE6B8`, `0x00104420` (new); vtables 4 → 5. GT3 `T017_smart_index.cpp`. |
| Kind | changed logic (plumbing) |
| Evidence | `lw 0x14(a0); jalr` in `setRange`; `lupo2.py class SmartCarIndex` slot 4. |
| Port | Only with the Concept lists. |
| Verdict | **port with 16-19** (otherwise do not port) |

### 35. Smooth scrolling in map / button lists

| Field | |
|---|---|
| What | `MapButtonList` gets a new virtual (slot 13) that eases the scroll position (`MotionValue::approach`). `doControl` moves through it (`moveSelection`) instead of jumping and playing the sound itself. The MagScr begin / end items are built in the constructor. `ButtonList` has a similar eased scroll (L2 `0x001CBA40`). |
| Where | L2 `0x001BB0C8`, `0x001BB158` (new); L2 `0x001BB438` = US `MapButtonList::doControl`; L2 `0x001BACD8` = US `MapButtonList::MapButtonList` (`T207_map_button_list.cpp`); vtable 13 → 14 (also `SettingButtonList`). |
| Kind | changed logic (plumbing) |
| Evidence | `doControl`: `moveSelection ×3` instead of `menu_play_se`; `lupo2.py class 13MapButtonList` slot 13. |
| Port | Used by the status list and Concept's settings list. |
| Verdict | **port with 16** (otherwise do not port) |

### 36. Cross marker drawn as an animated strip

| Field | |
|---|---|
| What | `Cross::virtual_2` (draw) calls a new strip draw that uses `fmod` (a moving pattern). In the same spirit, the list arrows (`Arrow::virtual_1`) are shown from a `SmartIndex` (wrap- and skip-aware, entry 34), not from GT3's button list. They pulse: a phase at `+0x108` goes down by 1/15 per frame and wraps to 1.0. |
| Where | L2 `0x001D05C8` = US `Cross::virtual_2` `0x001CE4A8` (`T255_cross.cpp`); L2 `0x001D0100` (new); L2 `0x001BBA80` = US `Arrow::virtual_1` `0x001B6110` (`T210_arrow.cpp`), helpers L2 `0x001CE600` / `0x001CE658`. |
| Kind | changed logic (cosmetic) |
| Evidence | Callees `gfx_alloc_tail`, `Drawable::getColors`, `fmod_wrap`, `pgl_draw_strip`. |
| Port | — |
| Verdict | **do not port** (cosmetic) |

### 37. gt2k_menu::String removed

| Field | |
|---|---|
| What | The `String` drawable (`T258_menu_string.cpp`, 7 functions) has no Lupo 2 twin, and its class is GT3-only. Lupo 2 sets texts on `Item`s (`Item::setText`), for example in the car spec texts. |
| Where | GT3 `T258_menu_string.cpp`; L2 `0x0010D960` / `0x0010DAB0` (`createPowerText` / `createTorqueText`). |
| Kind | removed in Lupo 2 |
| Evidence | `lupo2.py classes` "GT3 only: Q29gt2k_menu6String". |
| Port | — |
| Verdict | **removed in Lupo 2** (not ported) |

### 38. responseEval: response curve type 3

| Field | |
|---|---|
| What | `responseEval` gains a curve type 3: `y = c·u⁸ + b·u` with slope `8·c·u⁷ + b` (u = \|x\|, sign kept). GT3 knows 1 (quadratic around `a`), 2 (square root) and linear. |
| Where | L2 `0x001FEB00` = US `responseEval` `0x001D92E0` (`T354_response_curve.cpp`). |
| Kind | changed logic |
| Evidence | Path for `mode == 3`: `f0 = u*u; f4 = f0*f0; f1 = c*f4*f4 + b*u`; slope `8.0 * c * f4 * u³ + b`. |
| Port | A small case in `responseEval`. Useful only with the curve data that selects type 3 (input / physics area: find who sets `mode = 3`). |
| Verdict | **port** with the user of the curve |

## For other areas

- **Race modes / dynamics**:
  - New race classes used by this area's runners: `RacePOD` / `RacePODDisplay`
    (L2 `0x0018D498`), `RacePOD2P` (`0x0018D6A0`), `RaceTraining` /
    `RaceTrainingInformation` / `DynamicsConductorTraining` (`0x001872B0`,
    `0x00187560`), `RaceLinkDemo` (`0x001838F8`).
  - The arcade race now decides its post-race actions without
    `run_short_menu` (entry 6e): check `RaceArcade::virtual_8`,
    `RaceBasic::virtual_35` and `RacePS2Base::virtual_41`'s result in Lupo 2.
  - The link battle's input objects (L2 `0x001B2D00..0x001B2F78`, placed here
    by the map) belong to `RaceLinkBattle`.
  - Dynamics code at L2 `0x001FE738` / `0x001FE8A0` was placed in `common`
    by its neighbour.
- **Race display / effects**: `race_begin` allocates a new 0x640-byte buffer
  (L2 `0x001AF6A8`, kept at `gp-0x5238`). It is used by new matrix code at L2
  `0x001AED18..0x001AF6F0`.
- **Car model**: menu-car animation wrappers L2 `0x0022DAE8` / `0x0022DB18`
  / `0x0022DC90` (entry 33).
- **Rendering / GS**: GameOptions `+0x64` / `+0x68` are applied with
  a new `gfx_set_display_offset(dx, dy)` (L2 `0x0026C680`, entry 40).
  It writes GT3's `g_gfx.dispDX` / `dispDY` and calls
  `gfx_update_display_regs`. The map pairs L2 `0x0026C680` with
  `gfx_set_display_fb`, but its twin starts at L2 `0x0026C6A8`
  (screen adjust, entry 24).
- **Physics / input**: `responseEval` type 3 (entry 38). Also, the arcade
  race and free run force AT for a CVT (`isCVT`, menus_b entry 1).
- **Part B (save / memory card / data)**:
  - Boot language from a GT3 save (L2 `0x00142C30`).
  - Memory card mode `0x1E` (`TRANSFER`).
  - L2 `0x0013B710` / `0x0013C620` (memory card control,
    `file_operation_lupo`).
  - Reward bookkeeping L2 `0x00222440` / `0x00220B38` / `0x00220C58` /
    `0x00220D88`.
  - The `run_time_trial` records API.
  - The GCF demo kind (1000) and idle time (`0x0022BCD0`), the category
    table (`0x0022BDB0`), the GameOptions assist preset table and the
    screen-adjust defaults (all also in menus_b entry 10).

## Noise and the full candidate lists

### A. Pairs read by hand (228)

These are the 225 pairs of the first pass that were not auto-noise, plus 3
that only the updated tool flags. The diff column gives the first tool /
the updated tool (`<0.25` = below the threshold now). In the verdict
column, "change N" refers to the entry above; the other verdicts are
"noise" with its reason, "wrong pair" and "backend".

| L2 | GT3 function | diff (old / new) | verdict |
|---|---|---|---|
| 001014A8 | main_arcade | 0.54 / 0.54 | change F3/F5: new states, request exit, link-battle idle time |
| 0013C5C8 | render_thread_release | 0.60 / 0.60 | wrong pair: memory card icon code (icon_slots_alloc) |
| 001021A8 | render_set_object | 0.33 / <0.25 | noise: inlined signal |
| 00102760 | GameStatusHolder::create | 0.34 / 0.28 | noise: GameStatus::init inlined |
| 001029D0 | TransitionObject::drawBusyMark | 0.65 / 0.72 | change F10: rotating busy mark |
| 002B1EB0 | ComboutTransitionObject::~ComboutTransitionObject | 0.91 / 0.91 | noise: destructor chain |
| 002B1D20 | LineoutTransitionObject::~LineoutTransitionObject | 0.91 / 0.91 | noise: destructor chain |
| 002B1B98 | DelayFadeoutTransitionObject::~DelayFadeoutTransitionObject | 0.91 / 0.91 | noise: destructor chain |
| 002B1A68 | FadeoutTransitionObject::~FadeoutTransitionObject | 0.91 / 0.91 | noise: destructor chain |
| 002B19A8 | TransitionObject::~TransitionObject | 0.48 / 0.69 | noise: destructor chain |
| 001030E8 | race_begin | 0.26 / 0.26 | change (other areas): new 0x640-byte race buffer; allocBuffers twin unpaired |
| 001031A0 | game_init | 0.61 / 0.34 | backend: boot plumbing (render thread, scf lock moved) |
| 0013B710 | game_init | 0.90 / 0.90 | wrong pair: memory card control (file_operation_lupo), part B |
| 00102E18 | RaceIOPLoadModules::load | 0.60 / 0.60 | wrong pair / backend: IOP module plumbing |
| 00102E38 | RaceIOPLoadModules::cmdInitClients | 0.68 / 0.68 | wrong pair / backend: IOP module plumbing |
| 0012FCF0 | run_arcade_race | 0.31 / 0.31 | change F6 |
| 0012B708 | machine_test_new_record | 0.27 / 0.27 | noise: MachineTestRecords calls unpaired (part B) |
| 0012B9A8 | run_machine_test | 0.40 / 0.40 | noise: ShortMenu entry clears inlined |
| 00130520 | run_split_battle | 0.46 / 0.46 | change F8: per-player ASM/TCS, idle time-out, RaceSplitBattle base (race area) |
| 00145498 | MemorycardDemoFileManager::buildList | 0.31 / 0.31 | change F12: demo paths (demo/%s%s); do not port |
| 001456B0 | MemorycardDemoFileManager::releaseList | 0.62 / 0.31 | noise: delete[] wrapper |
| 00104360 | SmartSkipIndex::get | 0.77 / 0.73 | noise: base get inlined |
| 001CE578 | SmartIndexT<int>::setRange | 0.77 / 0.74 | change W6: normalize virtual |
| 001CE5B0 | SmartIndexT<int>::set | 0.82 / 0.80 | change W6: normalize virtual |
| 001D6D68 | SmartIndexDoubleRange::set | 0.45 / <0.25 | noise: inlined |
| 001F3B18 | curveLookup | 0.81 / <0.25 | wrong pair (13 vs 90 insns) |
| 001FEB00 | responseEval | 0.31 / <0.25 | change C1: response curve mode 3 |
| 00249470 | str_equal | 0.81 / 0.31 | noise: tail call |
| 00249810 | format_date | 0.39 / <0.25 | noise: tail call |
| 002499F0 | zero_memory | 0.34 / <0.25 | noise: memset loop |
| 00106210 | ArcadeControl::ArcadeControl | 0.47 / 0.47 | change A1: new fields (layout); MenuCarSpec/Ipic constructors inlined |
| 0022BD10 | ArcadeControl::setup | 0.98 / 0.98 | wrong pair: ProductConfig getter; GT3 setup's twin is L2 001068E0 |
| 00107C60 | ArcadeControl::getPlayerClassName | 0.74 / 0.74 | change A7: class names from the GCF |
| 00108120 | ArcadeControl::runTimeAttack | 0.39 / 0.39 | change A2: time attack flow |
| 00116BA8 | ArcadeControl::runMenu | 0.61 / 0.61 | wrong pair: text item available_car_num_C (%d/%d) |
| 00116C18 | ArcadeControl::runILinkMenu | 0.64 / 0.64 | wrong pair: text item available_car_num_B (%d/%d) |
| 002B2408 | ArcadeRender::~ArcadeRender | 0.34 / 0.47 | noise: destructor |
| 00116C88 | ArcadeControl::F_00115C80 | 0.62 / 0.62 | wrong pair: text item available_car_num_A (%d/%d) |
| 00116DD8 | ArcadeControl::setLinkMode | 0.67 / 0.67 | wrong pair: text item achievement_ratio (%d%%) |
| 00110708 | ArcadeControl::compulsoryCreateDoc | 0.38 / 0.38 | noise: layout |
| 00110820 | ArcadeControl::compulsoryCreateSpec | 0.39 / 0.39 | noise: layout |
| 00110B00 | ArcadeControl::compulsoryCreateRecords | 0.36 / 0.35 | noise: layout |
| 00110EC0 | ArcadeControl::compulsoryChangeCar | 0.40 / 0.40 | change A5: transitions instead of arrows/car view motion (UI) |
| 001110E8 | ArcadeControl::compulsoryOpen | 0.54 / 0.54 | change A5: left/right arrows (UI) |
| 00111660 | ArcadeControl::compulsoryClear | 0.78 / 0.78 | noise: inlined |
| 00111820 | ArcadeControl::selectCompulsory | 0.54 / 0.54 | change A5: blur / mini cars replaced by transitions (UI) |
| 0010A0D0 | ArcadeControl::openClassList | 0.53 / 0.53 | change A6: mini_classes images (UI) |
| 0010A3D8 | ArcadeControl::onClassChange | 0.30 / 0.30 | noise: layout |
| 0010A610 | ArcadeControl::openCarView | 0.46 / 0.45 | noise: inlined |
| 0010A848 | ArcadeControl::openCarList | 0.69 / 0.71 | change A5: 2P gauges / spec labels removed from the car list |
| 0010AA88 | ArcadeControl::closeCarList | 0.49 / 0.49 | change A5 (follows openCarList) |
| 0010B060 | ArcadeControl::openTransmission | 0.42 / 0.43 | change A5: gear_title items |
| 0010B538 | ArcadeControl::closeTransmission | 0.30 / 0.30 | noise / A5 |
| 0010B7A8 | ArcadeControl::openRacing | 0.38 / 0.39 | noise: inlined |
| 0010C2E8 | ArcadeControl::carSelect2PStep | 0.46 / 0.46 | change A5: new 2P steps (assist) |
| 0010C6E8 | ArcadeControl::carSelect2PAbort | 0.84 / 0.80 | change A5 (follows carSelect2PStep) |
| 0010C8C8 | ArcadeControl::carSelect2P | 0.49 / 0.49 | noise |
| 00104908 | ArcadeControl::newClassDoc | 0.53 / 0.54 | noise: inlined |
| 0010CE80 | ArcadeControl::carCode | 0.58 / 0.56 | change A1: class cars through virtual 2 |
| 0010CF70 | ArcadeControl::loadCarInfo | 0.55 / 0.49 | change A1: class cars through virtual 2 |
| 0010D148 | ArcadeControl::carName | 0.29 / 0.29 | noise |
| 0010D2F8 | ArcadeControl::garageCar | 0.32 / 0.30 | noise |
| 0010D458 | ArcadeControl::resetColorIndex | 0.56 / 0.37 | noise: inlined |
| 0010D548 | ArcadeControl::createCarView | 0.36 / 0.36 | noise: layout |
| 0010D7D0 | ArcadeControl::createDriveTrainIcon | 0.46 / 0.46 | change A5: drivetrain icon 'none' |
| 0010D960 | ArcadeControl::createPowerText | 0.67 / 0.67 | change A5: power from CarEquipments (tuned) |
| 0010DAB0 | ArcadeControl::createTorqueText | 0.58 / 0.58 | change A5: torque text via Item |
| 0010DBD0 | ArcadeControl::createSpecLabel | 0.83 / 0.83 | noise: refactor (String -> Item) |
| 0010DC48 | ArcadeControl::createColorName | 0.52 / 0.49 | change A5: 2P colour name |
| 0010DD98 | ArcadeControl::createSpecPanel | 0.50 / 0.46 | noise |
| 0010DE38 | ArcadeControl::miniCarImageName | 0.32 / 0.32 | noise |
| 0010E4D0 | ArcadeControl::buildCarSelect | 0.58 / 0.58 | change A5: ESP / gear / tuner, colour chips |
| 0010F280 | ArcadeControl::clearCarSelect | 0.63 / 0.63 | change A5 (follows buildCarSelect) |
| 0010F318 | ArcadeControl::createBlur | 0.56 / 0.52 | noise |
| 0010F968 | ArcadeControl::runCarSelect | 0.64 / 0.64 | change A5: car select flow |
| 00113378 | ArcadeControl::createCourseLogo | 0.41 / 0.41 | noise: inlined |
| 00113440 | ArcadeControl::createCourseDoc | 0.59 / 0.59 | change A6 (course doc arrows) |
| 00113518 | ArcadeControl::createCourseInfo | 0.43 / 0.42 | noise: inlined distance |
| 00113810 | ArcadeControl::miniCourseName | 0.38 / 0.46 | change A6: %s_mini1 images |
| 001138C0 | ArcadeControl::createCourseScreen | 0.50 / 0.51 | change A6: MiniCourseCursor, course image |
| 00113D18 | garage_car_race_code | 0.33 / 0.33 | noise: garage car layout (part B) |
| 00113E68 | ArcadeControl::raceCode | 0.48 / 0.47 | change A2: race codes, difficulty suffix, Pod |
| 002B2B28 | __tfQ29gt2k_menu17GoodiesButtonList | 0.30 / 0.30 | noise: typeinfo |
| 002B28E8 | __tfQ29gt2k_menu15LevelButtonList | 0.30 / 0.30 | noise: typeinfo |
| 00114A20 | ArcadeControl::setModeByName | 0.42 / 0.42 | change A2: mode list |
| 0010FBE8 | ArcadeControl::runLevelPage | 0.59 / 0.63 | change A4: level page via button list (DifficultyButtonList) |
| 0011BAF0 | ConnectStatus::virtual_2 | 0.52 / <0.25 | noise: pdi1394 wrapper |
| 00111FC8 | SelectBox::SelectBox | 0.31 / 0.31 | noise: layout |
| 00112280 | SelectBox::virtual_1 | 0.49 / 0.39 | noise: inlined |
| 001122F0 | SelectBox::virtual_2 | 0.40 / 0.40 | noise: layout |
| 001176D0 | CircleGauge::setKind | 0.45 / 0.36 | change W1: gauge data source (minor) |
| 002B3B80 | MovieControl::~MovieControl | 0.45 / 0.52 | noise |
| 002B3998 | MovieRender::~MovieRender | 0.36 / 0.45 | noise |
| 0015E770 | name_entry | 0.45 / 0.45 | noise |
| 002BF920 | NameEntryControl::~NameEntryControl | 0.47 / 0.57 | noise |
| 001B9D08 | menu_heap_active | 0.68 / 0.68 | backend: menu heap check |
| 001B9DD0 | Button::placeOffPoint | 0.46 / <0.25 | noise |
| 001B9E48 | Button::Button | 0.38 / 0.28 | noise: layout |
| 001BA040 | Button::~Button | 0.34 / <0.25 | noise |
| 001BA7E0 | Button::isClipped | 0.46 / 0.46 | noise: inlined |
| 001BA808 | Button::virtual_2 | 0.53 / 0.48 | change (W30): drawFocus is called through the vtable (subclasses override it) |
| 002CA620 | Cell<Button>::~Cell | 0.44 / 0.65 | noise |
| 002CA550 | __tfQ29gt2k_menu6Button | 0.26 / 0.26 | noise: typeinfo |
| 001BACD8 | MapButtonList::MapButtonList | 0.62 / 0.63 | change W7: MagScr items built in the constructor (plumbing) |
| 001BB438 | MapButtonList::doControl | 0.54 / 0.30 | change W7: smooth selection |
| 002CA920 | __tfQ29gt2k_menu13MapButtonList | 0.30 / 0.30 | noise: typeinfo |
| 002CA858 | __tfQ29gt2k_menu9MapButton | 0.30 / 0.30 | noise: typeinfo |
| 001BBA80 | Arrow::virtual_1 | 0.58 / 0.63 | change (minor, W6/36): the arrow follows a SmartIndex (wrap / skip aware) and pulses (timer -1/15 per frame) |
| 001BD5C0 | PageManager::update | 0.50 / 0.28 | noise: PageManager::back mis-mapped (calls the same function) |
| 002CAA30 | findValidFuncData | 0.41 / 0.41 | noise: recursion / tail call |
| 002CAAE0 | findExecuteFuncData | 0.41 / 0.41 | noise: recursion / tail call |
| 002CAB90 | findFocusFuncData | 0.41 / 0.41 | noise: recursion / tail call |
| 002CAC40 | findSelectableFuncData | 0.47 / 0.47 | noise: recursion / tail call |
| 001BD990 | Car::Car | 0.33 / 0.52 | noise: layout, camera ctor inlined |
| 001BE3B0 | Car::virtual_1 | 0.40 / 0.41 | change W5: model animations stepped |
| 001BE900 | Item::setVertexColors | 0.39 / 0.28 | noise: inlined colour setters |
| 001BED80 | Item::appear | 0.43 / <0.25 | noise: tail call |
| 001BF518 | Item::setResource(const char*, PageLayout*) | 0.32 / 0.32 | noise: setItem / findHiddenItem swap in the map |
| 001BF580 | Item::setText | 0.39 / 0.39 | change W3: text width cached |
| 001BBB18 | Item::virtual_2 | 0.94 / 0.94 | wrong pair: a derived draw calling Item::virtual_2 (the twin is L2 001BFE48) |
| 001C0078 | MenuControl::repeatDir | 0.39 / <0.25 | noise: inlined |
| 001C05C0 | MenuControl::emulatePressure | 0.34 / <0.25 | noise / plumbing: new MenuControl fields |
| 002CB110 | MenuControl::~MenuControl | 0.48 / 0.69 | noise |
| 001C0A20 | Transitive::Transitive | 0.85 / 0.88 | noise: Object ctor inlined |
| 002CB4C0 | List<OperatorHolder>::~List | 0.44 / 0.65 | noise |
| 002CB518 | Cell<OperatorHolder>::~Cell | 0.44 / 0.65 | noise |
| 002CB340 | OperatorHolder::~OperatorHolder | 0.41 / 0.28 | noise |
| 001C2328 | Operator::run | 0.37 / 0.37 | noise: unrolled |
| 001C2438 | Operator::operator new | 0.30 / <0.25 | noise: inlined |
| 001C4F20 | CallbackItem::setItem | 0.89 / 0.89 | wrong pair: swapped with findHiddenItem |
| 001C4BB0 | PageLayout::reloadFile | 0.35 / <0.25 | noise: tail call |
| 001C4D80 | PageLayout::getHiddenLayer | 0.65 / <0.25 | noise: tail call |
| 001C4DA0 | PageLayout::findLayerItem | 0.45 / <0.25 | noise: tail call |
| 001C4E48 | PageLayout::findObject | 0.26 / <0.25 | noise: tail call |
| 001C3858 | PageLayout::findHiddenItem | 0.90 / 0.90 | wrong pair: swapped with CallbackItem::setItem |
| 001C4FC0 | PageLayout::findHook | 0.45 / <0.25 | noise: tail call |
| 001C5000 | PageLayout::findSliderFunc | 0.45 / <0.25 | noise: tail call |
| 001C5040 | PageLayout::findMultiFunc | 0.45 / <0.25 | noise: tail call |
| 001C53F0 | PageLayout::closeTransition | 0.46 / 0.35 | noise: tail calls (same slide-outs) |
| 002CB970 | find_page | 0.41 / 0.41 | noise: recursion |
| 002CBA20 | find_layer | 0.41 / 0.41 | noise: recursion |
| 002CBAD0 | find_layer_item | 0.41 / 0.41 | noise: recursion |
| 002CBB80 | find_item | 0.41 / 0.41 | noise: recursion |
| 002CBC30 | find_hook | 0.47 / 0.47 | noise: recursion |
| 002CBCE8 | find_slider_func | 0.41 / 0.41 | noise: recursion |
| 002CBD98 | find_multi_func | 0.41 / 0.41 | noise: recursion |
| 001C57C0 | ScrollBar::ScrollBar | 0.25 / 0.25 | noise: swap |
| 001C5FF0 | TextBox::virtual_1 | 0.49 / 0.31 | noise: tail calls |
| 001C8490 | Slider::Slider | 0.41 / 0.40 | noise: swap, layout |
| 002CC398 | __tfQ29gt2k_menu6Slider | 0.30 / 0.30 | noise: typeinfo |
| 001C9608 | LinkManager::getCommonPmbPath | 0.70 / <0.25 | noise: tail call |
| 001C9620 | LinkManager::getPagePmbPath | 0.45 / <0.25 | noise: tail call |
| 001C97F8 | LinkManager::findButtonData | 0.45 / <0.25 | noise: tail call |
| 002CC4F8 | bsearch_page | 0.41 / 0.41 | noise: recursion |
| 001B8788 | ButtonList::createButtons | 0.69 / 0.61 | wrong pair: TopButtonList override; change A3 (aspec_demo hidden for country 8) |
| 001CA758 | ButtonList::ButtonList | 0.40 / 0.52 | noise: Object ctor inlined |
| 001CA970 | ButtonList::ButtonList | 0.52 / 0.62 | noise: Object ctor inlined |
| 001CB4A0 | ButtonList::scrollOffset | 0.35 / <0.25 | noise |
| 0024A3B0 | ButtonList::virtual_1 | 0.82 / 0.84 | wrong pair: pList sort |
| 001CBF00 | ButtonList::decideButton | 0.42 / <0.25 | noise: tail call |
| 002CC730 | List<Button>::~List | 0.44 / 0.65 | noise |
| 002CC5F8 | __tfQ29gt2k_menu10ButtonList | 0.26 / 0.26 | noise: typeinfo |
| 002CC788 | RenderBase::~RenderBase | 0.34 / 0.47 | noise |
| 001CD5A8 | ControlBase::~ControlBase | 0.57 / 0.44 | noise |
| 001CDB60 | MpegImage::movieUV | 0.31 / 0.31 | change W4: alpha movie UV |
| 001CF6C0 | MenuBgMovie::MenuBgMovie | 0.86 / 0.86 | change W4 (constructor) |
| 001CE850 | MenuBgMovie::~MenuBgMovie | 0.45 / 0.32 | noise |
| 001CE8C0 | MenuBgMovie::open | 0.42 / 0.42 | noise: layout |
| 001CF080 | MenuBgMovie::upload | 0.55 / 0.34 | noise: inlined |
| 001CF0D8 | MenuBgMovie::release | 0.59 / 0.59 | change W4 |
| 001CF118 | MenuBgMovie::update | 0.47 / 0.47 | change W4: alpha copy |
| 001CF468 | MenuBgMovie::start | 0.39 / 0.39 | change W4: file override, waits for the first frame |
| 001CF718 | T253_static_destroy (_GLOBAL_$D$__11MenuBgMovie) | 0.73 / 0.73 | noise: static dtor |
| 001D05C8 | Cross::virtual_2 | 0.30 / 0.30 | change W11: new strip draw |
| 001D10D8 | Rectangle::drawFrame | 0.38 / <0.25 | noise: tail calls |
| 002CE538 | T261_static_init (_GLOBAL_$I$g_002CF290) | 0.60 / 0.60 | noise: static init |
| 001D31B0 | RawListItem::virtual_21 | 0.42 / <0.25 | noise: tail call |
| 00155F28 | RawList::virtual_9 | 0.97 / 0.97 | wrong pair: memory card list override |
| 001D3478 | RawList::virtual_17 | 0.48 / <0.25 | noise: tail call |
| 00156230 | RawList::virtual_20 | 0.95 / 0.87 | wrong pair: memory card list override |
| 001D3A70 | RawList::scrollToFocus | 0.26 / <0.25 | noise: inlined clamp |
| 001D3F40 | RawList::getItem | 0.79 / 0.54 | noise |
| 002CE6A8 | RawList::getIndex(RawListItem&) (getIndex__Q29gt2k_menu7RawListRQ29gt2k_menu11RawListItem) | 0.75 / <0.25 | noise: inlined |
| 001D49A0 | applyClipRect | 0.44 / 0.44 | noise: float zero |
| 001D49D8 | RawWindow::RawWindow | 0.37 / <0.25 | noise |
| 001D4E88 | RawWindow::virtual_2 | 0.47 / <0.25 | noise: float zero |
| 001D5068 | RawWindow::clipToWindow | 0.39 / 0.39 | noise: float zero |
| 001D6180 | RawWindow::loadChild | 0.44 / <0.25 | noise: frame-time constant folded |
| 001D6488 | RawWindow::setNode | 0.29 / 0.29 | noise: swap |
| 001D66E0 | RawWindow::changeVertexColors | 0.30 / 0.30 | noise: swap |
| 001D6B18 | createWindow | 0.29 / 0.29 | noise: swap |
| 002CECD0 | __tfQ29gt2k_menu16ConcurrentButton | 0.30 / 0.30 | noise: typeinfo |
| 001D7160 | FreeButton::FreeButton | 0.26 / 0.26 | noise: inlined |
| 002CED28 | __tfQ29gt2k_menu10FreeButton | 0.30 / 0.30 | noise: typeinfo |
| 002CED80 | __tfQ29gt2k_menu11MultiButton | 0.30 / 0.30 | noise: typeinfo |
| 001D87B0 | RawScrollBar::create | 0.43 / 0.43 | noise: inlined |
| 001D8FD0 | RawCheckBox::virtual_17 | 0.72 / <0.25 | noise: tail call |
| 00137AB8 | OptionControl::OptionControl | 0.45 / 0.45 | noise: layout |
| 00137B90 | OptionControl::~OptionControl | 0.43 / 0.53 | noise |
| 00138008 | F_0013FE58 | 0.45 / 0.45 | backend: menu heap from memalign |
| 001664E0 | option_multi_set_wide_mode | 0.70 / 0.70 | wrong pair: CarGetConceptControl load (concept_car_get.mbl) |
| 00166C28 | option_action_set_default | 0.71 / 0.71 | wrong pair: SimplePageControl load |
| 00166C48 | option_action_stop_music | 0.71 / 0.71 | wrong pair: SimplePageControl unload |
| 00153160 | PlayMusicBox::virtual_17 | 0.46 / <0.25 | noise: tail call |
| 00153E20 | PlayList::PlayList | 0.41 / <0.25 | noise |
| 001540B0 | PlayList::virtual_17 | 0.44 / <0.25 | noise: tail call |
| 00160808 | KeyConfigItem::resetSelection | 0.35 / 0.35 | noise: layout |
| 001613E8 | KeyConfigItem::handleInput | 0.34 / 0.31 | noise: inlined |
| 00162980 | result_list_by_rank | 0.36 / 0.36 | noise: inlined clear |
| 00162A70 | result_list_by_order | 0.43 / 0.43 | noise: inlined clear |
| 00162BB8 | result_list_from_entries | 0.27 / 0.27 | noise: inlined clear |
| 00162D10 | make_ranking_code | 0.36 / 0.36 | noise: inlined |
| 00162EE0 | load_demo_setting | 0.29 / 0.49 | change F12 (demo setting path); do not port |
| 002C01A0 | ShortmenuLicenseControl::~ShortmenuLicenseControl | 0.47 / 0.57 | noise |
| 002C0370 | ShortmenuLicenseRender::~ShortmenuLicenseRender | 0.34 / 0.47 | noise |
| 0016E8D0 | ShortmenuControl::ShortmenuControl | 0.50 / 0.50 | noise: layout |
| 001BC8B8 | shortmenu_root_check | 0.73 / 0.73 | wrong pair: PageManager::back; the real shortmenu_root_check is L2 0016EA00 (change R1) |
| 002C00B8 | ShortmenuControl::~ShortmenuControl | 0.47 / 0.57 | noise |
| 002C2030 | ShortmenuRender::~ShortmenuRender | 0.34 / 0.47 | noise |
| 0016FA88 | qm_license | 0.34 / <0.25 | change R4 (minor): licence times not divided by 1000 |
| 0016FC88 | qm_session_num | 0.28 / 0.28 | noise |
| 001B2588 | color_candidate_set | 0.30 / 0.30 | noise: sqrt.s inlined |
| 001B5DC8 | race_menu_get_color | 0.44 / 0.44 | noise: swap |
| 001B66D8 | RaceMenuLayer::newItem | 0.43 / <0.25 | noise: tail call |
| 001B6710 | RaceMenuLayer::attach | 0.49 / <0.25 | noise: tail call |
| 0010D4A8 | ArcadeControl::scrollToRow | - / 0.42 | noise: float rounding (134.4) |
| 0012AE80 | run_time_trial | - / 0.39 | change F8 (part B): time-trial records API; F_0013F508 stub (entry 10) |
| 001D2D60 | menu_apply_transform | - / 0.30 | noise: float zero from $zero |

### B. Pairs auto-classified as noise (257, updated tool; not in table A)

These have the same calls, counting tail calls (after the throw helper,
the deleting-destructor calls and the inlined Object constructor are set
aside). They also have the same floats (gp constants included), the same
strings and the same number of virtual calls. Only struct offsets (layout:
`ControlBase` +0x10, `ArcadeControl` +0x70, `Button` +8, ...) and integer
constants (sizes, the vtable-slot immediates of Lupo 2's virtual calls)
differ. Spot-checked; constants that looked meaningful were checked:
- `ArcadeControl::waitFrame` / `isCourseLocked`: 6 → 8 is the i.LINK mode
  renumbering (entry 14).
- `ShortmenuLicenseRender::render`: position / colour constants of the
  licence short menu, layout only.

By file:
- `app/T002_render_thread.cpp` (1): 1 typeinfo functions (002B18B0)
- `app/T003_game_object_list.cpp` (2): 2 typeinfo functions (002B1948 002B18F0)
- `app/T004_game_status_holder.cpp` (1): getGameStatusHolder 001027E8
- `app/T005_transition.cpp` (5): 5 typeinfo functions (002B1F18 002B1D88 002B1C00 002B1AD0 002B1A10)
- `app/T009_race_iop_load_modules.cpp` (2): RaceIOPLoadModules::RaceIOPLoadModules 00103970; 1 typeinfo functions (002B20F0)
- `app/T079_demo_file_manager.cpp` (1): 1 typeinfo functions (002BE518)
- `common/T017_smart_index.cpp` (5): SmartSkipIndex::size 00104548; 4 typeinfo functions (002B22F8 002B2278 002B21F8 002B2378)
- `common/T266_smart_index_double_range.cpp` (1): 1 typeinfo functions (002CEC80)
- `menu/arcade/T010_arcade_control.cpp` (8): ArcadeControl::~ArcadeControl 001066D8, ArcadeControl::waitFrame 00106798, ArcadeControl::load 001067E8, ArcadeControl::isDirtCourse 00106EB8, ArcadeControl::getModeName 00107BF8, ArcadeControl::getClassLabel 00107C48, ArcadeControl::getButtonList 002B25E8; 1 typeinfo functions (002B2588)
- `menu/arcade/T011_arcade_render.cpp` (2): ArcadeRender::render 00108A68; 1 typeinfo functions (002B2480)
- `menu/arcade/T018_select_compulsory.cpp` (1): ArcadeControl::compulsorySelectCar 001116C8
- `menu/arcade/T020_arcade_helpers.cpp` (2): ArcadeControl::transitionIn 00111A68, ArcadeControl::attachToBar 00111C48
- `menu/arcade/T022_class_select.cpp` (1): ArcadeControl::newDisabledMark 00104628
- `menu/arcade/T023_arcade_car_select.cpp` (2): TransitiveEX::~TransitiveEX 002B24D0; 1 typeinfo functions (002B2538)
- `menu/arcade/T025_arcade_course_select.cpp` (1): ArcadeControl::isCourseLocked 00113050
- `menu/arcade/T034_connect_status.cpp` (1): 1 typeinfo functions (002B3350)
- `menu/arcade/T035_select_box.cpp` (2): SelectBox::~SelectBox 002B26A0; 1 typeinfo functions (002B2720)
- `menu/arcade/T098_handicap_item.cpp` (2): HandicapItem::~HandicapItem 002BFA08; 1 typeinfo functions (002BFA80)
- `menu/framework/T024_mini_cursor.cpp` (5): MiniCursor::~MiniCursor 001050E8, MiniCursor::totalWidth 00105500, MiniCursor::widthBefore 00105558, MiniCursor::widthAfter 001055B0; 1 typeinfo functions (002B23B8)
- `menu/framework/T032_circle_gauge.cpp` (2): CircleGauge::~CircleGauge 002B2BA0; 1 typeinfo functions (002B2C08)
- `menu/framework/T033_flash_item.cpp` (2): FlashItem::~FlashItem 002B3298; 1 typeinfo functions (002B3300)
- `menu/framework/T051_movie_player.cpp` (2): 2 typeinfo functions (002B3C30 002B3A28)
- `menu/framework/T097_name_entry.cpp` (1): 1 typeinfo functions (002BF9B8)
- `menu/framework/T204_menu_heap.cpp` (1): menu_heap_current 001B9AA8
- `menu/framework/T206_button.cpp` (6): Button::setCheck 001BA110, Button::isAvailable 001BA120, Button::virtual_9 001BA878, Button::virtual_10 001BA880, Button::virtual_15 001BA8B0; 1 typeinfo functions (002CA500)
- `menu/framework/T207_map_button_list.cpp` (4): MagScrEnd::~MagScrEnd 002CA730, MagScrBegin::~MagScrBegin 002CA678; 2 typeinfo functions (002CA798 002CA6E0)
- `menu/framework/T210_arrow.cpp` (2): Arrow::~Arrow 002CA978; 1 typeinfo functions (002CA9E0)
- `menu/framework/T211_page_manager.cpp` (1): 1 typeinfo functions (002CACF8)
- `menu/framework/T212_menu_car.cpp` (9): menu_car_any_loading 001BD950, Car::~Car 001BDD58, Car::setCar 001BDE70, Car::setWheel 001BDF30, Car::setColor 001BDFF8, Car::modelLoaded 001BE150, Car::wheelLoaded 001BE1D0, Car::loadStep 001BE208; 1 typeinfo functions (002CAF68)
- `menu/framework/T213_menu_item.cpp` (2): Item::Item(const char*, PageLayout*, MenuLayoutItem*) 001BF130; 1 typeinfo functions (002CB0A0)
- `menu/framework/T214_menu_control.cpp` (1): 1 typeinfo functions (002CB178)
- `menu/framework/T216_menu_transitive.cpp` (6): Transitive::~Transitive 001C0A78; 5 typeinfo functions (002CB458 002CB408 002CB3A8 002CB2F0 002CB570)
- `menu/framework/T220_object.cpp` (2): 2 typeinfo functions (002CB640 002CB5B0)
- `menu/framework/T221_callback_item.cpp` (2): CallbackItem::~CallbackItem 001C3760; 1 typeinfo functions (002CB698)
- `menu/framework/T222_decide_effect.cpp` (2): DecideEffect::~DecideEffect 002CB6E8; 1 typeinfo functions (002CB750)
- `menu/framework/T223_pointer.cpp` (2): Pointer::~Pointer 002CB7A0; 1 typeinfo functions (002CB808)
- `menu/framework/T228_slider_item.cpp` (2): SliderItem::~SliderItem 002CB8A8; 1 typeinfo functions (002CB920)
- `menu/framework/T230_scroll_bar.cpp` (2): ScrollBar::~ScrollBar 002CBE48; 1 typeinfo functions (002CBEC0)
- `menu/framework/T231_text_box.cpp` (2): TextBox::insertChar 001C5F58; 1 typeinfo functions (002CBF10)
- `menu/framework/T233_menu_sound.cpp` (1): menu_play_se 001C6A30
- `menu/framework/T234_bool_item.cpp` (2): BoolItem::~BoolItem 002CBF60; 1 typeinfo functions (002CBFC8)
- `menu/framework/T235_color_box.cpp` (2): ColorBox::~ColorBox 002CC018; 1 typeinfo functions (002CC080)
- `menu/framework/T236_free_agent.cpp` (1): 1 typeinfo functions (002CC1B8)
- `menu/framework/T238_slider.cpp` (3): Slider::setValue 001C8708, Slider::setRange 001C8778, Slider::getValue 002CC408
- `menu/framework/T239_callback_text.cpp` (2): CallbackText::~CallbackText 002CC440; 1 typeinfo functions (002CC4A8)
- `menu/framework/T240_page_history.cpp` (3): PageHistory::push 001C8F30, PageHistory::pop 001C8FB8, PageHistory::getTop 001C8FF8
- `menu/framework/T244_button_list.cpp` (5): ButtonList::setDecideFunc 001CB000, ButtonList::hideButtons 001CBB30, ButtonList::movePrev 001CBCD8, ButtonList::moveNext 001CBDC0; 1 typeinfo functions (002CC5A8)
- `menu/framework/T246_render_base.cpp` (2): render_sleep_done 001CD118; 1 typeinfo functions (002CC800)
- `menu/framework/T247_control_base.cpp` (3): ControlBase::ControlBase 001CD518, ControlBase::getPageManager 002CC948; 1 typeinfo functions (002CC8A8)
- `menu/framework/T249_mpeg_image.cpp` (1): 1 typeinfo functions (002CC950)
- `menu/framework/T250_ipic.cpp` (2): 2 typeinfo functions (002CCB28 002CCAD0)
- `menu/framework/T252_timer.cpp` (2): Timer::~Timer 002CCB68; 1 typeinfo functions (002CCBD0)
- `menu/framework/T253_menu_bg_movie.cpp` (3): MenuBgMovie::setSize 001CEA68, MenuBgMovie::stop 001CF538, MenuBgMovie::suspend 001CF570
- `menu/framework/T255_cross.cpp` (2): Cross::~Cross 002CCD68; 1 typeinfo functions (002CCDD0)
- `menu/framework/T256_menu_rectangle.cpp` (2): Rectangle::~Rectangle 002CCE50; 1 typeinfo functions (002CCEB8)
- `menu/framework/T257_menu_image.cpp` (4): Image::fitToTexture 001D12C8, Image::~Image 002CCF50; 2 typeinfo functions (002CD168 002CCFB8)
- `menu/framework/T259_menu_drawable.cpp` (3): Drawable::getScreenY 001D1DB8, Drawable::~Drawable 002CD200; 1 typeinfo functions (002CD268)
- `menu/framework/T261_menu_draw_state.cpp` (1): menu_set_draw_buffer 001D2268
- `menu/framework/T263_raw_list.cpp` (39): RawList::virtual_9 001D33E0, RawList::virtual_26 001D3830, RawList::virtual_27 001D38A8, RawList::virtual_29 001D3A68, RawList::getIndex 001D3EF8, RawList::setVertical 001D3FA0, RawList::addItem 001D3FC8, max_f 002CE508, min_f 002CE520, RawList::setScrollNextOffset 002CE6F8, RawList::setScrollNextMargin 002CE6F0, RawList::setScrollPrevOffset 002CE6E8, RawList::setScrollPrevMargin 002CE6E0, RawList::setScrollCenter 002CE6D8, RawList::setScroll 002CE6D0, RawList::setFocusAnimation 002CE6C8, RawList::getItemCount 002CE6C0, RawList::setKeepSubListIndex 002CE6A0, RawList::setMoveLoop 002CE698, RawList::getFocusItem 002CE690, RawListItem::setHiLightPropertyName 002CE5E8, RawListItem::getNextItem 002CE5E0, RawListItem::getPrevItem 002CE5D8, RawListItem::lockSize 002CE5D0, RawListItem::lockPosition 002CE5C8, RawListItem::isLockSize 002CE5C0, RawListItem::isLockPosition 002CE5B8; 12 typeinfo functions (002CE9D8 002CE988 002CE938 002CE8E8 002CE898 002CE848 002CE7F8 002CE7A8 002CE758 002CE640 002CE5F0 002CE568)
- `menu/framework/T264_raw_window.cpp` (1): 1 typeinfo functions (002CEA28)
- `menu/framework/T270_raw_animation.cpp` (1): 1 typeinfo functions (002CEDD8)
- `menu/framework/T271_raw_anim_scroll.cpp` (1): 1 typeinfo functions (002CEE30)
- `menu/framework/T272_raw_window_classes.cpp` (15): RawCheckBox::create 001D7CC0, RawLabel::create 001D7D88, RawList::create 001D7E50, RawListFixed::create 001D7F18, RawListH::create 001D7FE0, RawListHFixed::create 001D80A8, RawListHPos::create 001D8170, RawListHSize::create 001D8238, RawListItem::create 001D8300, RawListItemFixed::create 001D83C8, RawListV::create 001D8490, RawListVFixed::create 001D8558, RawListVPos::create 001D8620, RawListVSize::create 001D86E8, RawWindow::create 001D8878
- `menu/framework/T273_raw_label.cpp` (1): 1 typeinfo functions (002CEE80)
- `menu/framework/T274_raw_scroll_bar.cpp` (1): 1 typeinfo functions (002CEED0)
- `menu/framework/T275_raw_check_box.cpp` (1): 1 typeinfo functions (002CEF20)
- `menu/framework/T495_menu_list.cpp` (3): pCell::isNot 00249EF0; 2 typeinfo functions (002D4B48 002D4AD0)
- `menu/options/T062_option_control.cpp` (6): OptionControl::attachKeyConfig1 00137E38, OptionControl::detachKeyConfig1 00137E50, OptionControl::attachKeyConfig2 00137E70, OptionControl::detachKeyConfig2 00137E88, OptionControl::mainLoop 00137EA8; 1 typeinfo functions (002B4E20)
- `menu/options/T063_option_calib_checks.cpp` (1): option_check_calib_steer 001380A0
- `menu/options/T064_option_hooks.cpp` (5): option_create_key_config_1 001387B8, option_create_key_config_2 00138888, option_create_key_config_1_display 00138958, option_create_key_config_2_display 00138A28, option_create_steering_assist_type 00138CC8
- `menu/options/T085_play_list.cpp` (18): PlayMusicBox::create 00152ED8, PlayMusicBox::PlayMusicBox 00152F58, PlayMusicBox::setEnabled 00153470, PlayMusicBox::setLabel 00153620, PlayMusicBoxPop::create 00153690, PlayModeBox::create 00153990, PlayModeBox::PlayModeBox 00153A10, PlayModeBox::virtual_29 00153BF0, PlayList::create 00153DA0, PlayList::virtual_30 001543A0, PlayList::save 001544E0, PlayListBox::create 001545E8, PlayListBox::PlayListBox 00154668; 5 typeinfo functions (002BE9E8 002BE980 002BE930 002BE8E0 002BE888)
- `menu/options/T100_calibration_item.cpp` (1): 1 typeinfo functions (002C0010)
- `menu/options/T101_key_config_item.cpp` (1): 1 typeinfo functions (002C0060)
- `menu/race_menu/T102_race_menu_helpers.cpp` (2): run_short_menu 001617C8, race_get_entry_condition 001627D8
- `menu/race_menu/T103_shortmenu_license.cpp` (3): ShortmenuLicenseRender::render 001631C0; 2 typeinfo functions (002C0238 002C03E8)
- `menu/race_menu/T111_shortmenu_control.cpp` (3): ShortmenuControl::setHeap 0016E980, ShortmenuControl::releaseHeap 0016E9D8; 1 typeinfo functions (002C0150)
- `menu/race_menu/T112_shortmenu_render.cpp` (2): ShortmenuRender::render 0016F038; 1 typeinfo functions (002C20A8)
- `menu/race_menu/T113_shortmenu_items.cpp` (2): ShortMenuText::~ShortMenuText 002C2110; 1 typeinfo functions (002C2178)
- `menu/race_menu/T197_race_menu_page.cpp` (4): RaceMenuPage::RaceMenuPage 001B6230, RaceMenuPage::~RaceMenuPage 001B6298, RaceMenuPage::load 001B6308, RaceMenuPage::unload 001B6350
- `menu/race_menu/T198_race_menu_button.cpp` (3): 3 typeinfo functions (002C9C10 002C9BC0 002C9C60)
- `menu/race_menu/T199_race_menu_layer.cpp` (1): 1 typeinfo functions (002C9CA0)

### C. Unpaired Lupo 2 functions placed in the area (494), by cluster

Cluster labels refer to the entries (F = flow 1-12, A = arcade 13-22,
O = options 23-25, R = race menu 26-28, W = widgets 29-37, C = 38) or give
the verdict (noise / backend / other area).
- F1/F3 main (new boot and mode dispatch): 001000D0
- twins of main_championship / main_gtmode_race / main_gtmode_race2 (unpaired twins, noise): 00100380 001003C0 001003F0
- twin of run_menu_demo_race (noise): 00100420
- twins of main_license_test / main_machine_test / main_free_run (noise): 00100598 001005C8 001005F8
- twin of play_saved_replay, split in two (noise): 00100708 00100930
- F4 attract dispatcher (new): 00100998
- F4 twin of run_attract_demo (changed: one demo per call): 00100A28
- F3 mode 33: i.LINK linked attract demo, RaceLinkDemo (new): 00100C48 00100EF8
- F4 replay attract demo from a file (new): 00101240
- render thread helpers (backend): 00102150 00102180
- F10 busy-mark strip draw (new helper): 00102940
- RaceIOPLoadModules internals (backend): 00102CC8
- render thread start / exit (backend): 00102E78 00102F00 00102F98
- boot helper (backend): 00103458
- F1 DisplayAdvertiseImage boot splash (new): 001034A8 00103520 00103550 001035F8 001036E8 00103800
- IOP module loading (backend): 001039A8 00103A10 00103A60 00103A80
- A1/A3 ConceptControl menu entry and request dispatcher MOVIE0/MOVIE1/TRANSFER/PODRACE (new): 00103AC8 00103BF0
- A1 plain ArcadeControl entry, twin of run_arcade_menu / arcade_menu (no callers in Lupo 2): 00103D28 00103DF0
- A1 other ConceptControl entries (no callers: car-select-only run, select_car_test): 00103F30 001040C0
- A1 ConceptControl constructor and load (menu/concept.mbl, stream/course.ipic): 001041E8 00104230
- W6 SmartIndex wrapping move, new virtual 4 (plumbing): 00104420
- twin of SmartCarIndex::isValid (noise): 001044C8
- A4 licence-medal helper (new): 001045C8
- A1 ArcadeControl / ConceptControl virtual 5 (new): 001046C8 00104750
- A6 class select rebuilt: class mpeg / logo / cross bar (changed): 001047C0 00104868 00104AA0 00104DC0
- W1 MiniCursor / MiniCourseCursor draw (new): 001057B0
- A1 menu unload / setup (twin of ArcadeControl::setup, 1336 B; the map paired setup wrongly): 00106878 001068E0
- A2/A7 class names, car index helpers (new): 00106F88 00106FF8 00107138
- A1 prepareRace virtual 8 of ArcadeControl / ConceptControl (new): 00107168 001074E8
- A1 menu finish (twin of ArcadeControl::finish/unload): 00107AE8
- A2 per-mode menu flows (restructured arcade flow): 00107C10 00107CB8 00107DC0 00107EF8 00108040 001080B0 00108248 00108350
- A3 concept menu loops and runs (new): 00108580 001086B8 00108728 001088B0 00108910 00108928 001089C8
- A3 top menu ar/push_start_wagen (changed push-start page): 00108AB8
- i.LINK connect / wait pages (twins of runConnectPage / runWaitOtherPage, changed little): 00108C60 00108EF8 00109128 00109190
- blend helper: 00109280
- A10 ar/select_car_test debug tool (new, debug): 001092B0
- A5 car select: car / colour change, 2P steps, ESP assist panel (new / changed): 0010ABF8 0010AF18 0010B338 0010B3D8 0010B600 0010BC18 0010BEE0 0010BFF0 0010C0E0
- A1 virtuals 2 / 3 (car code / count) and A7 car index remap (new): 0010CA90 0010CB98 0010CC70 0010CCB0 0010CD68 0010CDE8
- A5 CarEquipments helpers (gearbox type, short name): 0010D088
- A5 colour chips SelectBox / SelectBox2 (new): 0010D630 0010D700
- A5 car change with tuner logo and BarGauges (new): 0010DE90 0010E378 0010E3B0
- A5 car select input / transitions (changed): 0010F3F0 0010F428 0010F460 0010F660 0010F800 0010F828 0010F8D0 0010F8F8
- A4 status / show room / licence select pages (new): 0010FE00 0010FE98 00110028 001100A8 001100F8 00110170 00110250 001102E0 00110520
- A5/A6 arrows (inlined createArrows): 00111B00
- A6 MiniCourseCursor creation (new): 00111E38
- W1 SelectBox2 (new): 00112160 00112938 001129C8
- A6 course image (ipic) (new): 001132B0
- A6 course screen (changed): 00113B90
- A4 button list factories menu_top / game / map / difficulty / category / license (new): 00114368 001144A8 00114560 001146A0 00114780 00114830
- A4 GameStatus check hooks and prize car spec (new): 00114BB8 00114BE8 00114C18 00114C40 00114C70 00114CA0 00114CD0 00114D00 00114D30 00114D60 00114D90 00114DC0 00114DE0 00114E78 00114EB0 00114EE8 00114F20 00114F50
- A6 mpeg_course / mpeg_class factories (new): 00115348 00115370 00115398
- A4 licence list items gold / silver / bronze, rank (new): 00115B28
- A4 car items car_render_from / car_name / car_logo / license_car ... (new): 00116310 00116538 00116588 001165E0 00116668 00116708 00116850 001168F0 00116978
- A4 available_car_num_S / _D texts (new): 00116CF8 00116D68
- W1 BarGauge draw (new): 00117B28
- W1/A4 GameButtonList, DifficultyButtonList, LicenseButtonList, CategoryButtonList, MiniCourseCursor, StatusButtonList (new): 00117E48 00117FB8 001180E0 001181A0 00118288 001182F8 00118460 001184C8 00118648 001187A8 001188E0 00118A78 00118BA0 00118C60 00118DA0 00118E40 00118F00 00119068 00119190 00119258 00119360 00119428 00119BA8 00119DF8 00119F48 0011A1F8
- W1/A4 StatusButton (new): 0011A280 0011A340 0011A380 0011A4A0
- A4 status texts: car name / price / odometer / power (new): 0011BC40 0011BCB8 0011BD18 0011BDB8 0011BDF0 0011BEB0 0011BEE0 0011BF28 0011BFB8 0011C058 0011C0C0
- F6 dead copy of the GT3-style run_arcade_race (no callers): 00129C18
- twin of run_license_test (GT mode, changed little; noise): 0012C2E0
- twin of run_gtmode_race (noise): 0012CDD8
- twin of run_championship (noise): 0012D5C0
- F5 run_pod_race (new): 0012E018
- F5 run_concept_license (new): 0012E6F8
- F5 run_training (new): 0012F298
- F5 run_pod_2p_race (new): 00130CB8
- F7 post-race rewards (new): 001378D8
- O1 assist level hooks (new): 00138350 00138370 00138390 001383B8 001383E0 00138408
- twins of option_create_key_config_steer_1/2, option_create_handicap, option_create_calib_bar (noise): 00138AF8 00138BE0 00138D90 00138E28
- memory card control (part B; placed here by its neighbour): 0013B688 0013B6C8 0013C4F0 0013C620
- twin of T085 static init / dtor (noise): 00154818
- F6/F7 NoticePresentControl menu/notice_present.mbl (new): 0015E230 0015E258 0015E278 0015E2D8 0015E318 0015E3A0 0015E3E8
- twin of run_short_menu_license (noise): 001619B0
- R2 concept licence short menu runner (new): 00161B98
- F5 concept licence result recording (new): 00162520
- twins of ShortmenuLicenseControl ctor / heap / mainLoop / run (noise): 00163370 00163428 00163480 00163660 00163768 00163818
- R2 ShortmenuConceptLicenseControl / Render (new): 001638E8 00163928 00163948 00163968 00163988 00163B28
- F7 CarGetConceptControl menu/concept_car_get.mbl (new): 00166508 001665E8 00166620 00166658 001668B8
- F7 SimplePageControl pages concept_moviea / movieb / ace / pod / billion (new): 00166C68 00166D20 00166D78 00166EE0 00166F38 001670F0
- short menu render helper: 0016F0B8
- menu car model set-up (race colour select): 001707F0
- RaceLinkBattle input (race area; placed here by its neighbour): 001B2D00 001B2E50 001B2F20 001B2F78
- TopButton / Item helpers: 001B8C48 001B8CB8 001B8D08 001B8D68
- menu heap check (backend): 001B9BB8 001B9D28
- Button item factory helper: 001BA168
- MapButtonList factories (StatusButtonList base): 001BAAE0 001BAC20
- W7 MapButtonList smooth scroll, new virtual 13: 001BB0C8 001BB158
- Item helper: 001BB9B0
- PageManager internals (twins of PageManager::back / decide helpers; noise): 001BC260 001BC3D0 001BC470 001BC680 001BC738 001BC7E8 001BC860
- small Transitive / Operator accessors (noise): 001C09C0 001C0A08 001C1300 001C2278 001C22D0 001C2468 001C24B0 001C2690 001C29C8 001C2A10
- CallbackItem helpers: 001C3900 001C3958
- MenuCarSpec constructor helper (inlined copy): 001C64F8
- W1 ColorBox2 (new): 001C7510 001C7608 001C76F0 001C7E40
- W1 Slider additions: previous-value pinch, double range (plumbing for car settings): 001C8818 001C8858 001C89D8 001C8AB0 001C8B50
- W7 ButtonList smooth scroll: 001CBA40
- W4 MpegImageAlpha draw (new): 001CE148
- Transitive helpers (inlined copies): 001CE600 001CE658
- W4 MenuBgMovie GSBuffer helper: 001CE7F8
- W4 MenuBgMovie preload / stop wrappers: 001CEA18 001CEA40
- W4 alpha movie upload (new): 001CEDB8 001CF048 001CF0B8
- W4 movie control helper: 001CF598
- W11 Cross strip draw (new): 001D0100
- Object constructor copy (noise): 001D1820
- draw state / clip helpers: 001D2EF0 001D2F58 001D2FE8 001D3008
- twins of FreeButton / ConcurrentButton helpers (noise): 001D7038 001D7080 001D70F8 001D7130 001D7320 001D7368 001D7410 001D7490 001D7520
- MultiButton creation with a sub list (changed): 001D7540
- twins of MultiButton helpers (noise): 001D77E0 001D7828 001D7848 001D78A0 001D7908
- raw window register static init, RawScrollBar constructor (noise): 001D88F8 001D8C30 001D8C50 001D8D78
- dynamics code (race / physics area; placed here by its neighbour): 001FE738 001FE8A0
- FontDict (backend): 0024A4A8
- out-of-line copies of inline functions (.gnu.linkonce, 144): setGameObjectControllerDefault 002B1940, setTopObject 002B1998, getTopObject 002B19A0, setLoading 002B1A60, typeinfo _GLOBAL_$N$.._.._advertise.cxxZ2 002B20A0, dtor ConceptControl 002B2140, typeinfo ConceptControl 002B21A8, getMemoryForCar 002B25D8, setCourseMpegImage 002B2600, setClassMpegImage 002B2608, getCurrentGame 002B2610, getCurrentGameIndex 002B2618, getDifficulty 002B2630, setDifficulty 002B2638, setPodRace 002B2640, typeinfo DifficultyButtonList 002B2648, setVanish 002B2770, dtor SelectBox2 002B2778, typeinfo SelectBox2 002B2828, dtor BarGauge 002B2C58, typeinfo BarGauge 002B2CC0, ctor BarGauge 002B2D10, dtor GameButtonList 002B2D40, typeinfo GameButtonList 002B2DB0, typeinfo GameButton 002B2E08, dtor DifficultyButtonList 002B2E60, dtor LicenseButtonList 002B2ED0, typeinfo LicenseButtonList 002B2F40, typeinfo LicenseButton 002B2F98, dtor CategoryButtonList 002B2FF0, typeinfo CategoryButtonList 002B3060, typeinfo MiniCourseCursor 002B30B8, dtor StatusButtonList 002B3108, typeinfo StatusButtonList 002B3178, dtor StatusButton 002B31D0, typeinfo StatusButton 002B3240, typeinfo CarGetControlBase 002B4DD0, typeinfo SettingControlConcept 002BE838, getPlayData 002BE8D8, isEdit 002BE9D0, dtor NoticePresentControl 002BF838, typeinfo NoticePresentControl 002BF8D0, dtor ShortmenuConceptLicense 002C0288, typeinfo ShortmenuConceptLicens 002C0320, setDisableRender 002C0438, IsDisableRender 002C0440, dtor ShortmenuConceptLicense 002C0450, typeinfo ShortmenuConceptLicens 002C04C8, enableUpdate 002C20F8, disableUpdate 002C2108, isOpened 002C9BA0, setCameraManager 002C9BA8, getLayer 002C9D20, getLayer 002C9D28, getVisible 002C9D38, setSelectable 002CA600, getDispX 002CA610, getDispY 002CA618, setNewControlFuncData 002CAD38, setSliderData 002CAD98, setMultiButtonData 002CADF8, getLinkPage 002CAEB0, getPageRootObject 002CAED0, setType 002CAFB8, setReflection 002CAFC0, setEnvTexture 002CAFC8, setViewport 002CAFD0, setAnti 002CB008, setMagnify 002CB010, setDirtiness 002CB090, getContinueCount 002CB290, isControlOccured 002CB298, dtor ColorBox2 002CC0D0, typeinfo ColorBox2 002CC168, undispPrevPinch 002CC3F0, dispPrevPinch 002CC400, getSmartIndex 002CC410, changeable 002CC418, getAllSize 002CC660, getRangeValue 002CC688, getFocusedButton 002CC6F0, getControl 002CC8E8, getControl2P 002CC8F0, setTimeLimit 002CC8F8, getTimeLimit 002CC910, resetTimeCounter 002CC918, getTimeCounter 002CC928, incTimeCounter 002CC930, getFramgentManager 002CC940, setPartial 002CC9A0, setFixAspect 002CC9B8, dtor MpegImageAlpha 002CC9C0, typeinfo MpegImageAlpha 002CCA28, ctor MpegImageAlphab 002CCA78, size 002CCB10, width 002CCB18, height 002CCB20, setBorderType 002CCF40, set_proper_texCoord 002CD008, getTexW 002CD060, getTexH 002CD098, setTexFile 002CD150, getZValue 002CD2B8, set_proper_vertex 002CD2D0, set_proper_vertex 002CD2F8, set_proper_vertex_fix_aspect 002CD328, set_proper_vertex_fix_aspect 002CD350, packColor 002CD380, getXf 002CD3A8, getYf 002CD3E0, getWf 002CD430, getHf 002CD460, set_vertex 002CD4A8, setXYWH 002CD508, setR 002CD538, setG 002CD548, setB 002CD558, setA 002CD568, setRGBA 002CD578, setRGBA 002CD598, setRGBA 002CD6F8, setColor 002CDC28, setRGBAall 002CDC58, setAnchorType 002CE1E0, setControl 002CE1F8, incR 002CE260, incG 002CE2A0, incB 002CE2E0, incA 002CE320, getR 002CE478, getG 002CE488, getB 002CE498, getA 002CE4A8, setFocusPropertyName 002CEAA0, getRawParent 002CEB48, getRawChild 002CEBA0, getRawSibling 002CEBF8, getEventChild 002CEC50, isUpdateEnable 002CEC70, setUpdateEnable 002CEC78, init 002D4B28, getTop 002D4BA0, getBot 002D4BA8, dtor t14_FontDict_Impl2Ui_40_Ui_15_ 002D4BB0

### D. GT3 functions of the area with no Lupo 2 twin

Most are tiny functions the map cannot pair:
- the 4-instruction option hooks of `T064` / `T066`;
- the `qm_*` items of `T113`;
- the `Operator` op functions of `T217`;
- the `Drawable` accessors of `T259`;
- static initialisers.

Their twins exist (the hook tables in Lupo 2 register the same names).

The real removals are entries 3 (GT mode top menu), 1 (memory card
warning, opening at boot), 10 (`F_0013F508`), 21 (goodies, history, short
ending), 17 (spec panel, colour selection, racing toggle), 18 (course item
factories), 26 (setting / save_game buttons) and 37 (`String`). The list
is in `build/scratch/survey_menus_a/us_unpaired.txt`.

## Corrections (after the pairs check, `build/scratch/survey_menus_a/pairs_fix.csv`)

Two more removals found while checking the pairs (both "removed in Lupo 2", not ported; they
belong with entries 18 and 21):

- `ArcadeControl::selectCourse` (L2 `0x00113D08`) is stubbed to `return 0`: Lupo 2's arcade has
  no course selection (the course comes with the car's race).
- `runWaitSyncPage` (L2 `0x00109270`) is stubbed to `return 0`: the i.LINK selection exchange is gone.

## Delta review (map v2)

These are the 58 candidates of `build/scratch/delta/menus_a.txt`: 22
"pair" lines and 36 "new?" lines. The new pair knowledge is in
`build/scratch/survey_menus_a/pairs_fix.csv` (now 95 right, 174 new, 8
wrong, 2 start rows).

### Pairs (22)

| L2 | map v2 says | verdict |
|---|---|---|
| `0x00102E38` | `RaceIOPLoadModules::load` | backend; plausible (calls L2 `0x00102CC8` where GT3 calls `IOPLoadModuleBase::run`) |
| `0x00103018` | `RaceIOPLoadModules::cmdSetMediaMode` | backend; right (its callee L2 `0x00102E58` = US `F_00102E58`) |
| `0x0010B338` | `ArcadeControl::onTransmissionChange` | right; **change, entry 39** (CVT shows only AT) |
| `0x0010B3D8` | `ArcadeControl::transmissionInput` | right; **change, entry 39** (CVT: index range [0, 1)) |
| `0x00110658` | `T016_static_init` | right; minor change: it also registers an `ar/top` hook and item name formats `tt%02d_mini` / `tt%02d_doc` for the ending select page (Concept layout; do not port) |
| `0x00138140` | `T063_static_init` | right; noise (same names) |
| `0x001386B0` | `option_range_split_race_laps` | **wrong pair**: it is the screen-adjust range (-16..16, GameOptions `+0x64`; entry 24). The right twin of US `0x00140440` is L2 `0x00138670` |
| `0x001386D0` | `option_range_split_handicap` | **wrong pair**: screen-adjust v range (`+0x68`). The right twin of US `0x00140460` is L2 `0x00138690`; US `option_range_race_laps` = L2 `0x00138650` |
| `0x0013A2D0` | `option_multi_set_wide_mode` | **wrong pair**: it is the screen-adjust h slider setter (calls `GameOptions::setScreenAdjustH`, entry 40). The right twin is L2 `0x0013A550` (calls `GameOptions::setWide`) |
| `0x0013ACA8` | `option_action_set_default` | right (registered as `set_default`). It calls L2 `0x00226A80`, which is the real `GameOptions::reset` twin; map v2's `0x0022B838` is wrong (a `CourseBestTimes` helper; part B) |
| `0x0013AD90` | `T067_static_init` | right; noise |
| `0x0015E900` | `T097_static_init` | right; minor: it also registers `handi_1p` / `handi_2p` hooks (handicap items) |
| `0x0016EEE8` | `T111_static_init` | right; **change, entry 42** (adds the page `root_no_setting_no_save`) |
| `0x001B24A0` | `color_similarity` | right; noise (`sqrt.s` inline) |
| `0x001CEA18`, `0x001CEA40`, `0x001CF598` | `MenuBgMovie::preload`, `unload`, `stopAndWait` | right; noise (layout) |
| `0x001D2CD8` | `menu_is_clipped` | right; noise (float zero) |
| `0x001D8D78` | `RawScrollBar::RawScrollBar` | right; noise (landing pad) |
| `0x001F3DE0`, `0x001F3EA8` | `curveLookupLinear`, `curveLookupPair` | right; noise (register copy shifts the offsets) |
| `0x00249B48` | `path_insert_prefix` | **wrong pair**: a new in-place "cut at the last `_`" helper. GT3's `path_insert_prefix` has no callers and no twin |

### New? (36)

| L2 | verdict |
|---|---|
| `0x001000C0` | runtime: `_exit`-style stub before `main` (belongs to `runtime/startup`) |
| `0x00101A30`, `0x00101A58` | right twins of US `PacketBuffers::PacketBuffers` / `T001_static_init` (Lupo 2 buffers `0x0035F080`, 2 × 0xA0000); noise |
| `0x00102E58` | right twin of US `F_00102E58`; noise |
| `0x00102FE8` | backend: render-thread state init (twin not identified) |
| `0x00103498` | backend: IOP module virtual returning 2 |
| `0x001042C0` | new: Concept event code parser (`sscanf "C%02d%c"`), used by a status check hook (entry 16) |
| `0x00106EA0` | new: current game index by mode (table L2 `0x00335B98`; entry 13) |
| `0x00106F78` | new: `isPodRace` (`podRace != 0`; entry 14) |
| `0x001091F0` | right twin of US `link_view_mode` (same 5-case switch on `x - 2`; the body runs to `0x00109270`); noise |
| `0x0010E458`, `0x0010E490` | new / refactor: show / hide the gear and assist items of the car select (entry 17) |
| `0x0010F960` | stub returning 1 in the car-select flow (probably the stubbed racing-modification step; entry 17) (?) |
| `0x0010FDE8` | right twin of US `ArcadeControl::classSelect`, **stubbed** (`return 0`; entry 41) |
| `0x001102D0` | new: licence-select helper (entry 16) |
| `0x00111CB0`, `0x00111CB8` | right twins of US `showHistoryClass` / `hideHistoryClass`, **stubbed** (empty; entry 41) |
| `0x00129590` | empty stub called by `main` before the autoload (a removed language / boot step (?)); entry 2 |
| `0x00129598` | new: static init registering the hook `set_lang` (a language page; with L2 `0x001293D0`, entry 2) |
| `0x00138768`, `0x00138790` | new: screen-adjust h / v option texts (`%2d`; entry 24) |
| `0x0013A488` | right twin of US `T065_static_init`: 5 sliders, GT3's 3 plus `slider_screen_adjust_h/v` (entry 24) |
| `0x00161770` | new: assist level → ASM / TCS for one player (2P battle, Pod 2P; entries 8 / 9) |
| `0x001B2F68`, `0x001B2F70` | race area: `RaceLinkBattle` input setters (placed here by their neighbour) |
| `0x001B8C88` | framework helper (item image index step); plumbing |
| `0x001BB068` | `MapButtonList::refresh` centring helper; plumbing (entry 35) |
| `0x001BD940` | right twin of US `menu_car_clear_loading`; noise |
| `0x001CE2B8` | right twin of US `Ipic::Ipic`; noise |
| `0x001D2EC8` | framework helper (default colour from gp constants); noise |
| `0x001D6C68`, `0x001D6C80` | static-object registration and deleting-destructor helpers; backend / noise |
| `0x001D6DB0` | right twin of US `SmartIndexDoubleRange::move`; noise |
| `0x001F3700` | right twin of US `AutomobileControlRecord::Manager::hasSlots`; noise |
| `0x001F3B18` | right twin of US `curveLookup`. The pair was in map v1; my first report called it wrong (the old bounds were 13 instructions), and that is corrected above; noise |
| `0x001F3D28` | new: inverse curve lookup used by `car_brake_aids` (physics area) |

### 39. Car select: a CVT car can only choose automatic

| Field | |
|---|---|
| What | In the arcade car select, the transmission choice for a car whose gearbox is a CVT (`isCVT`: the GEAR row's forward-gear count `+0x11` == 2, menus_b entry 1) is limited to automatic. The index range becomes [0, 1) and is set to 0. Only the AT item is shown, and the MT item is hidden. This holds for each player in the 2P select and for the 1P select (L2 `0x0010E3B0` from `buildCarSelect` / `compulsoryOpen`). |
| Where | L2 `0x0010B3D8` = US `ArcadeControl::transmissionInput` `0x00107D70`; L2 `0x0010B338` = US `onTransmissionChange` `0x00107CD8` (`T021_car_select_2p.cpp`); new helper L2 `0x0010CDE8` (builds the player's car's `CarEquipments`, returns `isCVT`); L2 `0x0010E3B0` (1P). |
| Kind | changed logic |
| Evidence | `0x0010B424 jal 0x0010CDE8; beqz → setRange(idx, 0, 2)` else `setRange(idx, 0, 1); set(idx, 0)`. `0x0010B374 jal 0x0010CDE8; bnel` → MT item `+0xAC = 0`, AT item `+0xAC = 1`. `0x0010CDE8` calls `ArcadeControl::carCode`, `CarEquipments::setCar`, `0x002433B0` (`isCVT`). |
| Port | It needs part B's `isCVT` and a GT3 car with a CVT gear row. The menu side is small: the CVT test in GT3's `transmissionInput` / `onTransmissionChange` and the 1P select. It matches entries 6 / 8 (the race forces AT anyway). |
| Verdict | **port** (with the CVT / gearbox work) |

### 40. Screen adjust is applied through a new display-offset setter

| Field | |
|---|---|
| What | Moving a screen-adjust slider calls a new GameOptions setter. The setter stores `+0x64` (h) or `+0x68` (v) and calls a new `gfx_set_display_offset(h * 2, v * 2)`, so the picture moves at once. That function stores the offsets in the GS driver state's `dispDX` / `dispDY` (`g_gfx+0x8C` / `+0x8E`; GT3 has the fields, used by `gfx_update_display_regs`, but never writes them) and calls `gfx_update_display_regs`. `GameOptions::reset` (the options' `set_default`) also re-applies the offset after `setDefaults`. |
| Where | L2 `0x00226B58` (h) / `0x00226B88` (v) `GameOptions::setScreenAdjustH/V`, new; L2 `0x0026C680` `gfx_set_display_offset`, new (10 instructions; the map pairs it with `gfx_set_display_fb`, whose twin starts at L2 `0x0026C6A8`; GT3 `src/engine/pgl/gs_display_driver.c`); L2 `0x00226A80` = US `GameOptions::reset` `0x00218460` (`T428_game_options.cpp`; map v2 has it wrong); slider setters L2 `0x0013A2D0` (h), `0x0013A330` (v), descs `0x0013A2F0`, `0x0013A350` (unlisted), table L2 `0x0013A488` = US `T065_static_init`. |
| Kind | new functions; changed logic (`reset`) |
| Evidence | `0x00226B58`: `sw v1, 0x64(a0); lw a1, 0x68(a0); jal 0x0026C680(v1 << 1, a1 << 1)`. `0x0026C680`: `sh a0, 0x8C(g_gfx); sh a1, 0x8E(g_gfx); j gfx_update_display_regs`. `0x00226A80`: `setDefaults`, `0x0026C680`, `apply`. |
| Port | A 3-line display-offset setter in `gs_display_driver.c` over GT3's existing `dispDX` / `dispDY`, two GameOptions setters and a call in `reset`, all over GT3's reserved GameOptions bytes (save-compatible). It also needs the option hooks of entry 24 and the option page items. A boot-time apply (after loading the options) should be added too: I did not find Lupo 2's, and `GameOptions::apply` only sets the wide flag (?). |
| Verdict | **port** (with entry 24) |

### 41. Arcade class select and history class display stubbed

| Field | |
|---|---|
| What | `ArcadeControl::classSelect` returns 0 at once. `showHistoryClass` / `hideHistoryClass` are empty. With `selectCourse` (Corrections), the arcade has no class or course choice: the car comes from the menu and its race from the GCF. Lupo 2's reskinned class and course screens (L2 `0x00104AA0`, `0x00104DC0`, `0x00113B90`, `0x001138C0`) have no callers. |
| Where | L2 `0x0010FDE8` = US `ArcadeControl::classSelect` `0x00109108` (`T022_class_select.cpp`); L2 `0x00111CB0` / `0x00111CB8` = US `showHistoryClass` `0x001054C0` / `hideHistoryClass` `0x001055F8`. |
| Kind | removed in Lupo 2 |
| Evidence | Call positions in the `runILink` and `runCarSelect` twins; bodies `addiu sp; li v0, 0; jr ra` and `jr ra; nop`. No callers for the screens (map v2, tail calls and vtables counted). |
| Port | — |
| Verdict | **removed in Lupo 2** (not ported) |

### 42. Short menu page "root_no_setting_no_save"

| Field | |
|---|---|
| What | The short menu's static init registers a second root page, `root_no_setting_no_save`, beside `root`. This is the layout without the setting and save buttons that entry 26 always hides. |
| Where | L2 `0x0016EEE8` = US `T111_static_init` `0x0012C8A8` (`T111_shortmenu_control.cpp`). |
| Kind | new data |
| Evidence | Strings `root`, `root_no_setting_no_save` (the second only in L2). |
| Port | — (it goes with Lupo 2's removal of those buttons) |
| Verdict | **do not port** |
