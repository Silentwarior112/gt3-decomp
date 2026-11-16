# Port notes: menus (wave 1)

Area: `src/game/menu/*`, `src/game/app`, `src/game/common`, `src/game/save`,
`src/game/data` (not T482), the display-offset function's file in
`src/engine/gfx` (`T512_render_context.cpp`), their headers and
`include/game/enhanced/menus.h`. Surveys: `survey/menus_a.md`,
`survey/menus_b.md` (with their Corrections and Delta reviews),
`survey/backend.md` B4, B6, B7, B8. Lupo 2 was read with `tools/lupo2.py
dis`; every port is written in GT3's structures.

Every changed file compiles with `tools/mcc.py` (output `build/menus/`). The
files that include the race-mode headers (T000, T052, T053, T054, and the
other areas' files that include my changed headers) cannot pass their
`EGCS_SIZE` checks until the integrator updates the race classes' sizes
(other areas grew RaceDisplay, and ResultArcade grew, below); they were
checked with the same compiler and flags with the size checks off
(`python build/menus/chk_nosize.py <src>...` with `build/menus/nosize.h`:
288 files, all clean; `build/menus/sw0.h` also compiled every switch's
GT3 side). `tools/sizeprobe.py`
shows none of my structs differing.

## Switches (`include/game/enhanced/menus.h`)

`1` = Lupo 2 (default), `0` = GT3.

| Switch | Entry | What it guards |
|---|---|---|
| `ENH_MENU_ATTRACT_REPLAYS` | menus_a 4 | `main`: one attract demo per menu time-out (no five-demo loop, no opening movie in between); a GCF demo entry with requirement word 1000 plays a replay file (T000 `run_attract`, `run_replay_demo`) |
| `ENH_MENU_BUSY_MARK` | menus_a 11 | `TransitionObject::drawBusyMark`: nine blue hexagon dots turning on an arc at the bottom right (T005). Fixed 2026-11-15: the hexagon table must be 16-byte aligned (`strip_draw` sends it to VU1 by DMA address); at 8 mod 16 nothing was drawn |
| `ENH_MENU_REMEMBER_CURSOR` | menus_a 15 | "ar/top" opens on the mode chosen last (memory only; T031) |
| `ENH_MENU_WIDE_FROM_CONSOLE` | menus_b 10, B7 | wide screen from the console's 16:9 setting: `RenderContext::init` (T512) and `GameOptions::setDefaults` (T428) |
| `ENH_MENU_ARCADE_DIRT_TYRES` | menus_b 5 | the settings refuse a tyre change in an arcade event ("A...") on a dirt course (T096, T080) |
| `ENH_MENU_WHEEL_CALIBRATION` | menus_b 11 | reset controller options: GT Force / C294 pedal ranges, marked valid (T426) |
| `ENH_MENU_RESULT_LAP_GAPS` | menus_b 14 b | laps-only result boards: leader's laps, the gaps of the cars on its lap (T117 / T200) |
| `ENH_MENU_LINK_RESULT_END` | menus_b 15 | i.LINK result: the 20 s wait starts when all cars but one are in (T191) |
| `ENH_MENU_FREE_RUN_ASSISTS` | menus_a 8 | free run: the options' ASM / TCS for every car, garage cars too (T052) |
| `ENH_MENU_TEXT_WIDTH` | menus_a 31 | `Item::setText` re-measures a scrolling text (T213) |
| `ENH_MENU_MOVIE_WAIT` | menus_a 32, B8 | `MenuBgMovie::start` waits (at most 2 s) for the first decoded frame (T253) |

The four optional items are the PLAN's; the other seven are ports that change
what GT3's own content does (PLAN rule 3). Ports that act only on data GT3
lacks have no switch (CVT, Pod, disqualification, the layout items).

## menus_b

| # | Outcome |
|---|---|
| 1 | **Ported.** T081 `text_gear_ratio_0..7`, `_final`: "----" also when `settingCar->isCVT()` (L2 0x00147344). T096 new `check_cvt` registered under GT3's transmission page link **`set/adjast_drv_gear_ratio`** (Lupo 2: its Concept `set/adjast_gear`): a CVT opens `your_car_is_CVT`; GT3's `menu/setting.mbl` has no such page, so it falls back to GT3's own `not_to_change` with the refused sound (a missing page would leave the PageManager without a page). Table 75 -> 76. No switch (no GT3 gearbox has 2 gears). |
| 2 | **Ported.** T081 new hook `gear_max_speed` (`text_gear_max_speed`, L2 0x0014A8F8) and helper (L2 0x0014A8B8: `getSpec`, `gear_chart_of_spec`, top gear's `speedHigh`), "%.2t%s" in the unit (`UnitSettings`), "----" for a CVT. Table 161 -> 162 (sorted). Shown only by a layout item named `gear_max_speed`; GT3's layout has none (hook unused). |
| 3, 4 | Not ported (Concept setting mode / tyre slider; PLAN). |
| 5 | **Ported, switch.** T096 `setting_dirt_tire_only`: for an "A..." event also the course kind (2 = dirt) of the course `setting_menu` runs for (T080 keeps it: `setting_menu_course()`; Lupo 2 reads MenuStatus +0x84, which GT3's arcade does not fill). GT3's race-record test still applies (no GT3 refusal lost). |
| 6, 7, 8, 9, 12, 13, 16 | Not ported (Concept / data; PLAN). Save layout unchanged. |
| 10 | **Ported, switch** `ENH_MENU_WIDE_FROM_CONSOLE` (wide default only; the other Concept defaults are not). `scf_get_aspect()` (T512, under the libscf lock). |
| 11 | **Ported, switch.** T426 `resetCalibration`: GT Force 0x29..0xD5 / 0x2B..0xDA, C294 0x25..0xB4 / 0x48..0xDF, valid (L2 0x00225DC0). New / reset options only; a loaded save keeps its own. The C294 part needs backend B4 c (below, race area). |
| 14 a | **Ported** (no switch: only the Pod race posts the event). T117 `ResultArcade::update` reads `RACE_EVENT_CAR_DISQUALIFIED` (non-destructive `findNth`, as the HUD): each car once, the board's last free row reads "Disqualified" (new `ResultBoard::setDisqualified`, `ResultRankBar::setDisqualified`; RACEMENUTEXT +"Disqualified", T201, `RACEMENUTEXT_DISQUALIFIED` 5); `setRowResult` leaves that row. |
| 14 b | **Ported, switch.** New `ResultBoard::setLapsGap` (L2 0x001B8218) for laps-only (time-limit) results. No new board field (Lupo 2's +0x7EC / +0x7F0 are GT3's `bestTime` / `bestLaps`). |
| 15 | **Ported, switch.** T191 `ResultLinkBattle::goal`. |
| 17 | Not ported, except RACEMENUTEXT "Disqualified" (with 14; listed under "For other areas" there). |
| 18 | Rendering's (done there). |

## menus_a

| # | Outcome |
|---|---|
| 1, 2, 3, 7, 10, 12, 13, 16, 18-22, 25, 27, 28, 36, 37, 41, 42 | Not ported (Concept replacements; PLAN). |
| 4 | **Ported, switch** `ENH_MENU_ATTRACT_REPLAYS` (T000). The replay demo is GT3's disc demo-replay read (file_read_blocking + "ReplayData" restore, as T071) played as `play_saved_replay`; a missing / bad file plays nothing (Lupo 2 does not check). Lupo 2 drops the licence requirement of the demos; GT3's stays (`run_attract_demo` still picks the demo). |
| 5 | **Ported** (the Pod states): `main_arcade` selection 6 -> `run_pod_race`, 7 -> `run_pod_2p_race` (race area, `race_pod.h`). Licence / training, link-battle time limit: not ported. |
| 6 | **Ported** (b): `run_arcade_race` forces automatic for a CVT (`gcar->isCVT()`). (a), (c), (e)-(g): not ported. |
| 8 | **Ported.** `run_free_run`: CVT -> automatic; options' ASM / TCS for every car (switch). `run_split_battle`: each player's own assist level (MenuArgs `assist1` / `assist2`, `set_entry_assist`, presets `GameOptions::getAssistPreset`, L2 0x00161770) for an arcade car when the car select offered the step, else the options as GT3. Time-trial records API: not ported (Concept save). |
| 9, 14 | **Ported** (the Pod entry): mode `ARCADE_POD_RACE` (8), buttons `pod_race` / `pod_race_2p` (exec table 13 -> 15; `setModeByName`, `podPlayers`), course list `POD_RACE`, class `ARCADE_CLASS_POD` (9, cars of `CLASS_PODRACE`), `raceCode` (1P the car's race, else the first car's; 2P `APO20001`), `getSelection` 6 / 7, `finish` (running car, player 2's car), `runPodRace` (course, then `runCarSelect` / `carSelect2P` without the class step). `podRaceAvailable` checks the GCF cars and courses and the race record; without them the button is refused (sound, back to "ar/top"). The runners are the race area's. Other mode renumbering: not ported. |
| 11 | **Ported, switch** (T005). |
| 15 | **Ported, switch** (the remembered cursor only, T031). |
| 17 | **Ported** (the assist step): T021 `hasAssistStep`, `openAssist`, `onAssistChange`, `assistInput`, `closeAssist`; 2P states 11 / 12 between racing (or transmission) and the wait; T023 `selectAssist` in `runCarSelect`'s state 4; T010 `assist1` / `assist2` start at the options' level (0..2), `finish` writes player 1's to the options (L2 0x00107B94) and passes both. Lupo 2's "yes" items blink (an Item kind of its own); plain Items here. The UI changes (tuner logo, chips, gauges, removed panels) are not ported. |
| 23 | **Ported** (T064 `assist_0..2_on/off`, T066 `multi_assist`, T428 `getAssist` / `setAssist`, preset table L2 0x002DF5C8). GT3's separate ASM / TCS options stay. |
| 24, 40 | **Ported.** GameOptions +0x064 `screenAdjustH`, +0x068 `screenAdjustV`: the first 8 bytes of GT3's reserved, zero-default, saved `unk_064[0x6C]` (now `unk_06C[0x64]`; no GT3 code reads it; layout and size unchanged; old saves load centred). T065 sliders `slider_screen_adjust_h/v` [-16, 17) (`setScreenAdjustH/V` apply at once), T064 ranges and texts `screen_adjust_h/v` ("%2d"), `reset` and `load` apply (`applyScreenAdjust`). |
| 26 | **Ported** with the flag's sense turned: `ShortMenuArgs::noLoadGhost` / `noDemo` (0x188 / 0x18C) hide `load_ghost` / `demonstration`. Lupo 2 shows them only when set, and its arcade sets neither; GT3's modes set neither here, so GT3 keeps both buttons. |
| 29, 30, 34, 35 | Not ported (only with the Concept screens). |
| 31 | **Ported, switch** (scroll-mode texts, TEXT_SCROLL; the survey's "aligned" is the scroll mode 0x10). |
| 32 | **Ported, switch** (the start wait; the alpha movies need assets: not ported). |
| 33 | Rendering's (T212, done there; my T212 header comment updated). |
| 38 | Wave 0 / physics. |
| 39 | **Ported.** `ArcadeControl::isCarCVT` (garage car: itself; data base car: a CarEquipments of its code; L2 0x0010CDE8); 1P `updateTransmissionButtons` / `selectTransmission` (also the compulsory page), 2P `onTransmissionChange` / `transmissionInput`: a CVT shows and keeps automatic only. |

## backend

| # | Outcome |
|---|---|
| B4 | (c) **request** (below, race area: `src/engine/input`). (a), (b) not mine. |
| B6 | **Ported:** `gfx_set_display_offset(dx, dy)` in `src/engine/gfx/T512_render_context.cpp` (the file this area owns; Lupo 2 has it in the display driver, L2 0x0026C680). It sets `g_gfx.dispDX / dispDY` and recomputes DISPLAY2 through `gfx_set_screen_size` with the driver's own size (the update function is G001-internal). |
| B7 | **Ported, switch:** `scf_get_aspect()` (T512; Lupo 2's is in scf_lock.c) and `RenderContext::init`. |
| B8 | **Ported, switch:** no new engine accessor: GT3's `mpegif::getSize` (no callers in GT3) already answers from `g_frameq_filled_once`, the flag Lupo 2's accessor reads. |

## Layout changes

| Struct / table | GT3 | Now | Where |
|---|---|---|---|
| `ResultArcade` | 0x94C | 0x96C | + `disqualified[7]`, `disqualifiedCount` (T117) |
| `ResultChampionship` / `ResultLinkBattle` / `Result2PBattle` | 0x9A8 / 0x954 / 0x94C | 0x9C8 / 0x974 / 0x96C | follow (members +0x20) |
| `ArcadeControl` (stack object) | 0x3BE0 | 0x3C50 | + `assist1/2`, the assist items, `assistChosen`, `podPlayers` |
| `MenuArgs` | 0x3C | 0x44 | + `assist1`, `assist2` (default `MENU_ASSIST_OPTIONS` -1) |
| `ShortMenuArgs` | 0x188 | 0x190 | + `noLoadGhost`, `noDemo` |
| `GameOptions` | 0x888 | 0x888 | names in reserved bytes only |
| tables | | | T081 hooks 161 -> 162, T096 checks 75 -> 76, T064 hooks 105 -> 115, T065 sliders 3 -> 5, T066 multis 21 -> 22, T028 execs 13 -> 15, RACEMENUTEXT 5 -> 6, T010 mode names 8 -> 9, class names / labels 9 -> 10 |

No raw offsets / sizeof literals of these were found in code.

## What to test in game

- **Settings** (GT mode garage car or arcade free run -> setting): ratios
  and the transmission page as before for every GT3 car. With a transplanted
  CVT row (2 gears): ratios "----", the gear-ratio button opens
  `not_to_change` (or `your_car_is_CVT` if the layout has it).
- **Arcade free run on a dirt course with a garage car** -> setting -> change
  tyres: refused "dirt_tire_only" (GT3 allowed it).
- **Options -> reset to defaults**, then a GT Force / Driving Force (C294) on
  USB: pedals work without calibrating (GT Force; C294 needs B4 c).
- **Arcade single race with a time limit** (laps-only board): leader shows
  laps, the others on its lap "+gap".
- **i.LINK battle** with 3+ consoles: the board waits for all but one.
- **Free run**: with ASM / TCS off in the options a garage car runs without
  them.
- **Boot on a 16:9 console** (PCSX2: BIOS aspect 16:9): the screen is wide
  until a save loads; a new game's options say wide.
- **Loading screens** (transitions after 3 s): the turning blue dots.
- **Arcade**: after a race "ar/top" opens on the mode chosen last.
- **Menus with movies**: no blank first frame.
- **Attract** (leave the GT-mode top menu idle): one demo, then the menu
  again; no opening movie after five.
- **2P battle**: assists as before (the options) with GT3's layout.

## Assets each port needs (and the fallback)

| Port | Asset | Without it |
|---|---|---|
| gear_max_speed | `menu/setting.mbl`: a text item `gear_max_speed` | hook unused |
| check_cvt | `menu/setting.mbl`: a page `your_car_is_CVT` (popup with the message) | GT3's `not_to_change` page |
| CVT anything | a GEAR row with 2 forward gears (paramdb; physics area's gearbox type 1) | GT3 cars: nothing changes |
| assist option | `menu/option.mbl`: multi button `multi_assist` (4 choices), BoolItems `assist_0..2_on/off` | GT3's ASM / TCS options only |
| screen adjust | `menu/option.mbl`: sliders `slider_screen_adjust_h/v`, texts `screen_adjust_h/v` | centred picture (0) |
| assist step | `menu/arcade.mbl` pages `ar/select_car` / `ar/select_car_2p`, hidden layer: `assist_title(_2p)`, `assist_0..2_yes(_2p)`, `assist_0..2_no(_2p)` | step skipped, the options decide |
| Pod entry | `menu/arcade.mbl` "ar/top": buttons with exec `pod_race` / `pod_race_2p` (decide action); GCF: car list `CLASS_PODRACE` (each car's race code = a Pod race record), course list `POD_RACE`, the race `APO20001` (2P); the race area's Pod course, Pod car, rules | buttons absent: nothing; buttons without data: refused |
| result board DQ | (RACEMENUTEXT "Disqualified" is built in; a language file can replace it) | |
| attract replay | GCF demo entry: race word = the replay file path on the volume (e.g. `demo/xxx`), requirement word 1000 | entry never chosen by GT3's code; a missing file plays nothing |
| short-menu gates | none | |

## Log lines (printf, "menus: ...")

- `menus: console aspect: wide N` (boot, switch on)
- `menus: screen adjust H, V` (a loaded save with an offset)
- `menus: CVT: the transmission page refused (...)`
- `menus: assist step, player N`, `menus: assist level N (player N)`
- `menus: pod race, N player(s)` / `menus: pod race refused: no CLASS_PODRACE cars, POD_RACE courses or race record`
- `menus: result board: car N disqualified`
- `menus: attract replay FILE` / `...: not on the volume` / `...: not a replay`
- `menus: movie FILE: no frame after 120 vsyncs`

## For other areas

- **race (`src/engine/input`)**: backend B4 c: `USBSteeringPort::update`
  (T479) should apply the stored calibration to the C294 (product 0xC294,
  `usbApplyCalibration(..., wheel 3, ...)`) as it does for the GT Force
  (L2 0x0023FB10..0x0023FB74). Without it menus_b 11's C294 defaults are
  stored but unused.
- **race (Pod runners)**: the arcade calls `run_pod_race(MenuArgs *)` and
  `run_pod_2p_race(MenuArgs *)` (`race_pod.h`); they are declared but not
  defined yet (`race_pod_runners.cpp` is announced). The menu passes
  `args->race` (1P: the chosen CLASS_PODRACE car's race, else the first
  car's; 2P: `APO20001`), `args->course` (the POD_RACE list's), `car1/2`,
  colours, tyres, transmission, and `assist1/2` (-1 = the options; a level
  0..2 otherwise: `GameOptions::getAssistPreset`, as T054's
  `set_entry_assist`). A CVT forcing automatic, as T052, is up to the
  runner. If the Pod's short menus show `load_ghost` / `demonstration`,
  `ShortMenuArgs::setNoLoadGhost(1)` / `setNoDemo(1)` hide them.
- **integrator (sizes)**: ResultArcade +0x20 makes RaceArcade,
  RaceArcadeSingle, RaceArcadeDemo, RaceChampionship (ResultChampionship),
  RaceSplitBattle (Result2PBattle), RaceLinkBattle (ResultLinkBattle) and
  the race area's RacePOD / RacePOD2P 0x20 larger: their `EGCS_SIZE` (race
  headers) need the size probe's values.
- **integrator (placement)**: `gfx_set_display_offset` and `scf_get_aspect`
  are in `src/engine/gfx/T512_render_context.cpp` (this area's engine file)
  rather than Lupo 2's places (`gs_display_driver.c`, `scf_lock.c`); move
  them if those files get an owner.

## Uncertain

- Lupo 2's attract replay demo sets a race word (its RaceBase +0x9694 = 1)
  before playing and returns whether the replay ended by "exit"; the
  meaning of that word was not found (GT3's `play_saved_replay` sets
  nothing), so it is not set here.
- The assist step's "yes" items are plain Items (Lupo 2: a blinking Item
  kind, L2 0x001B8C48); options' level 3 (TCS off, ASM on) shows as level 2
  in the car select (Lupo 2's range [0, 3)); deciding it writes level 2.
- `check_cvt`'s page name assumes the gear-ratio page is reached through a
  button linking to `set/adjast_drv_gear_ratio` (GT3's `setting.mbl` strings);
  not tested in game.
- Lupo 2's free run also takes the transmission from the options
  (gearAuto) for every car (its Concept flow): not ported, GT3's arcade
  choice stays.
