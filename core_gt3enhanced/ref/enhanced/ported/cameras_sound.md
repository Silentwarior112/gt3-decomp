# Port notes: cameras + sound (wave 1)

Area: `src/game/camera`, `src/game/sound`, their headers
(`include/game/camera`, `include/game/sound`) and
`include/game/enhanced/cameras_sound.h`. Survey: `survey/cameras_sound.md`
(with its Delta review) and `survey/physics.md` "For other areas" (the
engine sound levels).

Every changed file compiles with `tools/mcc.py` (objects in
`build/cameras_sound/`), with the switches at 1 and at 0, and with GCC 14.
Files that include `T121_race_entry.h` (T176, T177, T383, T386..T391, T409
and the race / display files) stop at T121's `EGCS_SIZE(RaceEntryCar,
0x2170)` until the race area updates it (below): for the checks, a copy of
T121.h with 0x2190 was put in front of `include/` (outside the tree).

One struct grew: **`CarSound` 0x818 -> 0x838** (the Pod's voice, appended),
so **`RaceEntryCar` 0x2170 -> 0x2190** (T121.h, race). The size probe lists
nothing else of this area: every other new field sits in GT3's alignment
padding (`VariableCameraBase` stays 0x100, `PasslineCamera` /
`StartingCamera` 0x140, `CameraView` 0x24C0, `CameraManager` 0x9740). The
other diffs of the probe (`RaceBasicWithRaceDisplay` and the race classes)
are the hud's and race's.

## Switches (`include/game/enhanced/cameras_sound.h`)

`1` = Lupo 2 (default), `0` = GT3.

| Switch | Entry | 1 (Lupo 2) | 0 (GT3) |
|---|---|---|---|
| `ENH_CAMERA_TV_AIM` | C1 | TV cameras' pitch aim 0.14 | 0.4 |
| `ENH_CAMERA_LOD_BIAS` | C2 | texture LOD bias + 0.0 (coarser mips) | + (-ln 2) |
| `ENH_CAMERA_DEMO_VIEW_MODE` | C4 | demo replays 75 % traditional / 25 % avantgarde, traditional when the replay BGM is off | 33 / 33 / 33 with broadcast |
| `ENH_CAMERA_ZOOM_BUTTON` | C5 | the zoom button zooms the TV camera up to x4 | no zoom |
| `ENH_CAMERA_START_SEQUENCE` | C6 | replays open on the starting camera; its sweep half as fast, own clock, mirrored for the right side of the grid; demo auto cut 40 s | GT3 |
| `ENH_CAMERA_OCCLUSION_BOX` | C7 | other cars' boxes 1.5 m tall for the in-car view | 1.0 m |
| `ENH_CAMERA_STEREO_EYES` | C8 | i.LINK stereo eye distance 0.063 m | 0.2 m |
| `ENH_SOUND_ENGINE_LOAD` | S2 | engine voices around the engine's real load share, exhaust curve 0.4 + 0.6 (1 - (1 - x)^16) | 0.375, straight line |
| `ENH_SOUND_MIX_TABLE` | S3 | (read by the physics area's T285, requested below) | |
| `ENH_SOUND_TARMAC_SQUEAL` | S4 | Lupo 2's three tarmac rows (squeal volume, pitch, smoke) | GT3's table |
| `ENH_SOUND_SQUEAL_DETUNE` | S5 | right-hand wheels' squeal onset at 0.75 pitch for a frame | no detune |
| `ENH_SOUND_WHINE_REVERSE` | S6 (new, see below) | the gear whine's torque share as \|share\| | the signed share |

`ENH_SOUND_WHINE_REVERSE` is not a survey entry: porting S6 showed one more
difference in the stock part of `car_sound_gear_whine` (L2 0x001DE3E4,
`neg.s` after the division), which the survey had as unchanged. With GT3's
negative reverse ratio (`GearCalc::ratio[0]`), GT3's reverse whine is quieter
and phase-inverted (volume whineLo + (whineHi - whineLo) x a negative share);
Lupo 2's whines like the forward gears.

Not switches (inert with GT3's cars and files): the Pod's voice (S1: a Pod,
`sound/gt3pod.ins`, the se.inf entries, and the race area's frame part) and
the type-3 / Dualnote whines and type-3 engine silence (S6:
`GearCalc::gearboxType` 3, `RaceCarState::hybrid`).

## Entries: cameras

| Entry | Outcome |
|---|---|
| C1 TV aim | **Ported, switch.** `CameraRotDamper::start` / `update` (T379), `ROT_AIM_OFFSET`. |
| C2 LOD bias | **Ported, switch.** `CameraBase::textureLODBias` (static, defined in T397; 0.0 or -ln 2 by the switch), its setter / getter `setTextureLODBias` (T410.h; Lupo 2's setter has no callers either); `VariableCameraBase::setLodBias` adds it. |
| C3 avantgarde presets, no pair shots | **Not ported** (PLAN.md): GT3's random effects and pair shots stay; the presets need Lupo 2 course data. |
| C4 demo view mode | **Ported, switch.** `F_0020E930` (T394): the 0.75 odds, every fourth avantgarde, then traditional when `GameOptions::replayBgm` is 0. |
| C5 zoom button | **Ported, switch; the button needs the race area** (below). `VariableCameraBase::zoomMult` (T397.h, +0xF8 in GT3's padding; 1.0 by the constructor); `CameraZoomSpring` (T395, Lupo 2 L2 0x002154E8..0x002155DC: spring k 40, damping -10, speed within +-900, release x 0.8) in `CameraView::zoomSpring` (+0x2484, padding); `CameraManager::init` resets it, `update` steps it per view every frame; `updateMode0` (T388), with the designer camera: target 20 while `CAMBTN_ZOOM` is held (held buttons), `zoomMult` = clamp(1 + v x 4 / 20, 1, 4); `DesignerCamera::update` (T398): zoom x zoomMult, the dampers' range 30 / (zoom x zoomMult), a record without aim pulls zoomMult to clamp((m - 1) / 3 + 1, 1, 2). `CAMBTN_ZOOM` is **0x40000000** (camera_modes_internal.h), not Lupo 2's 0x00800000: Lupo 2 freed that bit by moving the view-mode bits and cross2..L1 one bit up; GT3's view-mode bits (0x00800000 / 0x01000000, which no GT3 input sets) and replay bits stay, and 0x40000000 is free. |
| C6 start sequence | **Ported, switch** (details below). |
| C7 occlusion box | **Ported, switch.** `OnBoardCamera::isViewBlocked` / `isInsideCar` (T378), `OCCLUSION_BOX_HEIGHT`. |
| C8 stereo eye distance | **Ported, switch.** `CameraManager::init` (T383) `shift`. |
| C9 head tracker | **Not ported** (PLAN.md; needs the device). |
| C10 course demo cameras unused | **Not ported** (removed in Lupo 2: GT3 keeps them). |
| C11 random seeds hook | **Not ported** (survey: no change for one console). |
| C12 MenuCarCamera debug mode | **Not ported** (host-only debug). |
| C13 6-car target selector | **Not ported** (GT3 keeps 7). |
| Delta E1..E4 | car-box helpers: physics' (not this area). |
| Delta E5..E8, E10, E11 | noise. |
| Delta E9 | link-battle only: not ported. |
| Delta E12..E14 | part of C12: not ported. |

C6 in detail (Lupo 2 L2 0x00211D60 `updateMode0`, 0x00217640 / 0x002175A0
`StartingCamera`, 0x002171E8 `PasslineCamera::update`, 0x0020F2B8 `init`,
0x0020FB58 `update`):

- `CameraManager::startClock` (+0x9718, padding): frames since the countdown
  ended, counted by `update` while `org->countDown` is 0, at most 60000;
  `startShown` (+0x971C): the starting camera ran.
- `updateMode0` (replays, view modes 0 / 3): start frame = `countDown` while
  it runs, then 60 - startClock; -60 for `getStartingFormat()` 0 (rolling
  start) or 2 (free run / time trial). While >= 0: the starting camera
  (`F_00209660`), a 1 s cross-fade at 180 / 120 / 60; at 59 (the first frame
  after the countdown) `StartingCamera::setStatic`. The first frame after
  it forces a cut (`autoCut`). "counting" (a target change forces a cut)
  lasts 1 s after the countdown. The demo's automatic cut: 2400 frames (GT3
  300).
- `StartingCamera`: sweep `1 - x / 1200` (GT3 600; this also slows the live
  race start, `updateStart`); in replays (`setup(target, course, ownClock)`
  from `init` with `org->race->replayMode != 0`) x = countDown + 60 during
  the countdown, then 60 - its own frame count; held (`startState` 1) it
  jumps to its end, then stops updating (2); `inverseX` = the target car's
  `RaceEntryCar::order & 1` (Lupo 2's start position, +0x2688 there).
- `PasslineCamera::inverseX` (+0x10C, padding, Lupo 2's own offset):
  mirrors the record's x; `setup` clears it. The starting camera's
  `startState`, `ownClock`, `clockFrame` are in PasslineCamera's padding too
  (+0x110..+0x11B): Lupo 2 grew StartingCamera by 0x40, which here would move
  every later camera of `CameraView` and grow `CameraManager` (4 views).
- Uncertain / not taken: Lupo 2 takes the start record from its course
  package (+0xA4 / +0xA8) or `cameras/replay.cam` for replays; GT3 has
  neither, so start.cam's first record is used for both. Lupo 2 also stopped
  cross-fading on a target change in mode 0; GT3's cross-fade is kept.

## Entries: sound

| Entry | Outcome |
|---|---|
| S1 the Pod's voice | **Ported (data); needs the race area's frame part** (below). Details below. |
| S2 engine load response | **Ported, switch.** `car_sound_engine_levels` (T288). Not in Lupo 2: r = 0.375 when friction + engine torque is 0 (the frames before the first step), and x = 0 when t <= r (Lupo 2 would divide 0 by 0 at r = 1). |
| S3 mix table | **Requested from physics** (T285 is in `src/game/car/physics`; switch `ENH_SOUND_MIX_TABLE` is here). |
| S4 tarmac squeal | **Ported, switch.** `g_tyreTarmacRows` (T333, Lupo 2's rows +0x000 / +0x0A0 / +0x140 of L2 0x002DEE40) used by `tyre_sound_init` and `tyre_sound_update` through `tyre_sound_row` for class 0; GT3's table (T355, physics) is not touched. The smoke row is taken with the other two (the survey: together or not at all; the smoke is evaluated in T333; the rendering survey does not take it): tarmac smoke starts later. If the integrator prefers the data in T355 itself, the three rows can move there under the same switch and `tyre_sound_row` go. |
| S5 squeal detune | **Ported, switch.** `tyre_sound_update` (T333). |
| S6 type-3 / Dualnote whine, type-3 silence | **Ported (data).** `car_sound_gear_whine`: `RaceCarState::hybrid` -> `car_sound_hybrid_whine` (L2 0x001E11C8), `GearCalc::gearboxType == GEARBOX_ELECTRIC` -> `car_sound_electric_whine` (L2 0x001DFED8), then GT3's; `car_sound_engine_levels`: both levels 0 for type 3 below 10.472 rad/s. Plus `ENH_SOUND_WHINE_REVERSE` (above). Assets: none (the whine voice plays `turbine2`, bank 1). |
| S7 BGM fade stop | **Not ported** (PLAN.md; no callers in Lupo 2). |
| S8 demo music stubbed | **Not ported** (removed in Lupo 2: GT3 keeps its demo music). |

S1 in detail:

- `GtSoundChannel` (T468, Lupo 2 L2 0x0023B0A0..0x0023B354): program,
  voice, pending. `set`, `bind` (stop, set), `play` (keyed on once when
  bound; while sounding priority, pitch = note pitch x the factor, volumes;
  both volumes 0: stop), `stop(fade)`, `operator=` (program, no voice).
- `CarSound` (T176) + `podPan` (0x818), `podVoice` (0x82C): built by the
  constructor; `start` resets the pan and sets no program; `stop` stops the
  voice with the fade (pause, race end, cameras without Doppler: with the
  car's other voices); `update(f, pod)` calls `updatePodVoice` after the
  slots (Lupo 2's place); the compiler-generated `operator=` (T121's
  `copyFrom`) copies the channel through its `operator=`.
- `CarSound::updatePodVoice` (Lupo 2's dispatcher L2 0x001A5318): pan from
  the frame's car position; level clamp(32 / d x scale, -1, 1) x volume (1.0)
  x the effect factors; priority from it; emotion 1..10 -> {wink, genki,
  kyoro, okoru, odoroku, iya, sabishii, kowai1, naku, nemui} (L2 table
  0x00342DD0) -> `bind(se_find_program(se, SE_BANK_POD))`; `play(priority,
  l, r, pitch)` every frame, pitch = the Doppler factor.
- `CarSoundPodVoice` (T176.h): {volume, pitch, emotion}, handed to
  `CarSound::update` beside the frame (Lupo 2: frame +0x37C / +0x380 /
  +0x394; its voice block's position is not read by its dispatcher, so it is
  not kept). Without it (`update(&frame)`, as T121 is now) the Pod is silent.
- T469: `SE_POD_*` 0x28..0x32, `SE_TABLE_COUNT` 0x33, `g_seNames` + the 11
  names with index `SE_INDEX_NONE` (-1; GT3's 40 keep 0); `SE_BANK_COUNT` 4
  (gtsound_internal.h); `se_load_pod_bank` (from `se_init`, after rn_01.es:
  Lupo 2 loads it before rn_01; after it GT3's banks keep their SPU places):
  `file_resolve` first (the loader would retry a missing file forever), the
  whole file must fit below SPU 0xAC000 before anything is read (the music
  samples start there), a fixed bank with its own 0x800-byte header buffer
  (GT3's is sized for its banks and unchecked), the header size checked
  before the copy; `se_find_program(se, bank)`: no program unless se.inf
  names the effect on that bank and the bank has the program;
  `se_report_pod_effects` logs once.

## Assets (S1)

| File | Where | Fallback |
|---|---|---|
| `sound/gt3pod.ins` | from the Lupo 2 disc (81,760 bytes); where GT3 reads its sound files (GT3.VOL or the host folder, as the Pod model) | missing, not a bank, too big for SPU or header: the Pod is silent, a log line says why |
| `sound/se.inf` | **GT3's, extended** with Lupo 2's 11 entries `pod_genki`, `pod_iya`, `pod_kowai1`, `pod_kowai2`, `pod_kyoro`, `pod_naku`, `pod_nemui`, `pod_odoroku`, `pod_okoru`, `pod_sabishii`, `pod_wink`: **bank 3, programs 0..10** (Lupo 2's se.inf entries 42..52) | an entry missing or on another bank: that effect is silent (logged) |

Merge, do not replace (Lupo 2's se.inf has no `slide` and adds `bolt`,
`j_up`, `j_down`; its other entries might point at other programs if its
.ins files differ from GT3's):

```
python GT3Hooks-main/tools/gt3/seinf_tool.py dump    <GT3 se.inf>
python GT3Hooks-main/tools/gt3/seinf_tool.py dump    <Lupo 2 se.inf>     # the pod_* entries say bank 3
python GT3Hooks-main/tools/gt3/seinf_tool.py merge   <GT3 se.inf> <Lupo 2 se.inf> se.inf
python GT3Hooks-main/tools/gt3/seinf_tool.py resolve se.inf --banks 4    # "0 problem(s)"
```

The Pod's car also needs its engine sound files (`sound/engine/<code>`,
`sound/<normal0..3|turbo0..3>/<code>`, its ENGINE row's sound code): they
come with the car (other areas).

## Log lines (EE console, at boot, `printf`)

- `snd: sound/gt3pod.ins not found: the Pod's voice is off` (GT3's files: expected)
- `snd: sound/gt3pod.ins -> bank 3, SPU xxxxx..yyyyy (limit AC000), 11 programs`
- `snd: sound/gt3pod.ins (n bytes) does not fit below SPU AC000 (the effects end at xxxxx): the Pod's voice is off`
- `snd: sound/gt3pod.ins is not a sound bank: the Pod's voice is off`
- `snd: sound/gt3pod.ins: its header (n bytes) is larger than 2048: the Pod's voice is off`
- `snd: no Pod bank: its 11 sounds are silent (se.inf names n)`
- `snd: pod_xxx is not in se.inf: silent` / `... is on bank b, not 3: silent` / `... no such program in bank 3: silent`
- `snd: n of 11 Pod sounds ready` (with both files: `11 of 11`)

Nothing is logged per frame or per play (Lupo 2 does not either).

## What to test in game

- Boot with GT3's files: the line `snd: sound/gt3pod.ins not found ...`, then
  `snd: no Pod bank ...`; menus, countdown, crashes, engines, tyres sound as
  before apart from the switched changes below.
- C1: replay, traditional, TV cameras: the car nearer the vertical centre of
  the shot.
- C2: races and replays: distant textures a little softer, less shimmer.
- C4: attract-mode replays: no broadcast mode; with the replay BGM option
  off, always traditional.
- C5 (after the race area maps the button): replay, traditional, a TV
  camera: hold triangle -> a spring zoom up to x4; release -> back quickly;
  a course camera that looks along its own path zooms at most x2.
- C6: a race replay: the starting camera during the countdown and one second
  after it, cross-fades at 3 / 2 / 1 s, then a cut; the live start's camera
  covers only the second half of start.cam's path (half speed); a car on the
  right of the grid (odd grid position) sees the path mirrored; arcade demo:
  the automatic "car ahead" cut every 40 s. Rolling starts and time trial /
  free run replays: no start sequence.
- C7: in-car view right behind a tall car: the view counts as blocked up to
  1.5 m.
- C8: only with an i.LINK stereo wall.
- S2: in-car: louder off-throttle exhaust (0.4 of full at idle), full almost
  as soon as the engine pulls.
- S4 / S5: wheelspin and locking on tarmac squeal by slip speed, x1.5
  louder, other pitch; tarmac smoke starts later; the right-hand wheels'
  squeal onset a little lower for one frame.
- S6 / reverse whine: reverse gear whines like first gear (a car with
  `gearWhineOn`); type-3 / Dualnote cars with the physics area's data.
- S1 (both files, a Pod, the race area's frame part): the voice on each
  emotion change, panned and fading with distance, pitch shifting on a
  fly-by (Doppler), stopping on pause / race end. Compare with
  `docs/pod/POD_SOUND_PLAN.md`'s bring-up steps.

## For other areas

1. **race (`include/game/race/core/T121_race_entry.h`)**:
   `EGCS_SIZE(RaceEntryCar, 0x2170)` -> **0x2190** (`CarSound` 0x838), and the
   note under the class: the members after `sound` (`input` ...) sit 0x44
   later than shown (0x24 RaceCarModel + 0x20 CarSound). Until then every
   file that includes T121.h fails under `mcc.py`.
2. **race (`RaceEntryCar::updateSound`, T121.cpp)**: the Pod voice's part of
   the frame (Lupo 2 L2 0x00176964..0x001769C8). Replace the last
   `snd->update(&frame);` with:

   ```c
       /* The enhanced engine (Lupo 2 L2 0x00176964..0x001769C8,
        * cameras_sound.md S1): the Pod's voice: volume 1.0, the Doppler
        * factor, the emotion the Pod entered this frame (L2 0x001E19D0) */
       CarSoundPodVoice pod;
       pod.volume = 1.0f;
       pod.pitch = doppler;
       pod.emotion = (st->params.specialKind == CAR_KIND_POD) ? st->podSound : 0;
       snd->update(&frame, &pod);
   ```

   (`CarSoundPodVoice` is in T176_car_sound.h, which T121.h includes;
   `CAR_KIND_POD` in T279. If physics ports L2 0x001E19D0 as a function, use
   it for the emotion.) The race pool (0x22000 bytes) holds the entries:
   0x20 more each.
3. **race (`RaceInput::emulatePressure`, T413; race_input_internal.h)**: the
   zoom button: in replay modes 1..3, the pad's triangle (`PADRAW_TRIANGLE`)
   sets **0x40000000** (`CAMBTN_ZOOM`, src/game/camera/camera_modes_internal.h;
   e.g. `RIB_REPLAY_ZOOM = 0x40000000`); Lupo 2 also maps the GT Force
   wheel's button 0x10 (L2 0x0021D2AC). Lupo 2's own bit 0x00800000 is GT3's
   (unused) view-mode bit here: do not use it. The camera reads it held
   (`getButtons`).
4. **physics (`src/game/car/physics/T285_table.cpp`)**: S3, the mix table
   (Lupo 2 L2 0x002DE7E0), under `ENH_SOUND_MIX_TABLE` (game/enhanced.h):

   ```c
   #if ENH_SOUND_MIX_TABLE
       { 2.8f, 2.3f, 0.5f, 0.2f, 0.7f, 0.3f, 1.4f, 0.5f,     /* Lupo 2: in-car engine, exhaust, */
         0.0f, 0.9f, 0.9f, 0.6f, 1.0f, 0.8f, 1.4f, 1.0f },   /* gear whine, road (L2 0x002DE7E0) */
   #else
       { 3.0f, 2.5f, 0.5f, 0.2f, 0.8f, 0.3f, 1.4f, 0.6f,
         0.0f, 0.9f, 0.9f, 0.6f, 1.0f, 0.8f, 1.4f, 1.0f },
   #endif
       ... rows 1..3 unchanged ...
   #if ENH_SOUND_MIX_TABLE
       { 0.5f, 3.0f, 0.0f, 0.0f, 0.2f, 0.5f, 0.2f, 0.0f,     /* Lupo 2: outside, exhaust 3.0 */
         0.8f, 0.9f, 1.0f, 0.5f, 1.0f, 0.3f, 1.0f, 1.0f },
   #else
       { 0.5f, 3.3f, 0.0f, 0.0f, 0.2f, 0.5f, 0.2f, 0.0f,
         0.8f, 0.9f, 1.0f, 0.5f, 1.0f, 0.3f, 1.0f, 1.0f },
   #endif
   ```

   (Columns 0 engine, 1 exhaust, 4 gear whine, 7 road; rows `LISTEN_*`,
   race_entry_internal.h; values from the survey's
   `build/scratch/survey_cameras_sound/tables.txt`.)
5. **physics (data the sound reads)**: `RaceCarState::podSound` must still
   hold the emotion when the sound frame reads it after `dc->update` (Lupo 2
   clears it at the start of each emotion step, L2 0x001E1548, and sets it on
   a change, L2 0x001E1384); `CarDynamics::specialKind` (the Pod 1);
   `GearCalc::gearboxType` and `drivenAxle`; `RaceCarState::hybrid`; the
   Dualnote's front motor load in `DynCarState::clutchTorqueAbs`; for type 3,
   `DynCarState::brake` / `clutch` / the wheels' `brake` as GT3 sets them;
   `engineTorque` / `frictionTorque` every step for every gearbox type (S2
   reads them).
6. **race (T153, optional)**: Lupo 2 also loads `cameras/replay.cam` (its
   replays' start camera, L2 0x0018D944); with a global for it,
   `StartingCamera::setup(target, course, 1)` could take its first record.
   Needs the file from the Lupo 2 disc; not done (start.cam is used).
7. **integrator**: the switch file has one switch more than wave 0 named
   (`ENH_SOUND_WHINE_REVERSE`).
