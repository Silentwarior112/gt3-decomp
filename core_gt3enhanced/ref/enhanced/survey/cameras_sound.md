# Survey: cameras and sound logic (Lupo 2 against GT3 US)

Area: `src/game/camera/` and `src/game/sound/`, plus the game-side sound code
that lives in other folders: `RaceEntryCar::updateSound` (T121), the sound
gain table (T285), the tyre sound leaves (T287), the tyre sound table
(T355's `g_002D35B8`), `RaceBGMPS2` (T130), `RaceLicenseBGM` (T144) and
`MusicPlayList` (T430).
Format: `ref/ENHANCED.md`. Addresses: "L2" = Lupo 2 (PAPX-905.09), "US" = GT3 US.
Scratch (scripts, dumps): `build/scratch/survey_cameras_sound/` (`skel.py`
skeleton diffs, `rawdis.py`, `lx.py`; `cam_skel.txt`, `sound_batch.txt`).

## Summary

### Cameras

The user expected few changes here. There are several. Most of them are in
the replay / TV cameras.

1. **Replay TV cameras aim closer to the target's centre.** The pitch damper
   of the designer (TV) cameras has a smaller aiming offset: 0.4 becomes 0.14
   in `CameraRotDamper::start` and `update`. **Port** (two constants).
2. **Texture LOD bias of the race cameras**: `setLodBias` adds 0.0 where GT3
   adds −ln 2, so the mip levels are about 0.7 level coarser. The value is
   a settable static (`CameraBase::setTextureLODBias`), but nothing in L2
   sets it. **Port, optional** (it changes how sharp the textures look).
3. **The avantgarde replay mode (view kind 1) has new cut effects.** GT3
   picks a random level override, colour tint and motion blur at each cut.
   L2 drops all three and picks colour-correction presets stored in the
   course data instead (32 presets after the 0xF50-byte CourseEnv). This mode
   also no longer frames a pair of cars. **Do not port**: the presets need
   Lupo 2 course data, and the random effects and two-car shots are GT3
   features. Optional: use the presets when a transplanted course has them.
4. **Demo replays pick their view mode differently.** The odds are 75 %
   traditional and 25 % avantgarde; broadcast is never drawn. Avantgarde is
   only chosen when replay BGM is on. **Port the BGM condition**; the new
   odds are optional.
5. **New: zoom button for the TV camera** (view mode 0). Holding a camera
   button (bit 0x00800000; L2 moved the view-mode buttons up a bit to free
   it) springs a zoom multiplier up to 4×. The DesignerCamera applies it to
   its zoom and its damper range. A new
   field `VariableCameraBase::zoomMult` holds it. **Port, optional** (it needs a pad
   mapping: race input area).
6. **Race start sequence in view mode 0.** At the start of a race or replay,
   mode 0 shows the starting camera, cutting with cross-fades at 1, 2 and
   3 s. The starting camera's sweep is half as fast (1200 frames, GT3 600).
   It has a new set-up that runs on its own frame counter after the
   countdown, takes start-camera records from the course, and is mirrored to
   the car's side. In demos the automatic "car ahead" cut comes every 2400
   frames, GT3 300 (40 s against 5 s). **Port (?)**, medium effort; some details are uncertain.
7. **The in-car (sway) camera's occlusion box is taller**: 1.5 m instead of
   1.0 m, in `isInsideCar` / `isViewBlocked`. **Port** (constants; it
   helps tall cars such as the Pod).
8. **Default stereo eye distance of the i.LINK screen wall**: 0.063 m (GT3
   0.2). L2's names show that GT3's "wall shift" is a two-console stereo
   mode. **Port** (one constant; it only matters for that i.LINK set-up).
9. **New: head-tracker view** (`pdiHt` device) that turns and moves the in-car
   view. **Do not port** (it needs the hardware).
10. **Course demo cameras are no longer called** (`startDemoCameras` /
    `updateDemoCameras` are dead in L2). **Removed in Lupo 2**: GT3 keeps them.
11. **Camera random seeds come through a race-installed function**
    (`CameraBase::random`). Its default is the clock, as in GT3. This is
    plumbing for linked consoles. **Do not port.**
12. **MenuCarCamera debug mode 12**: it loads `host0:/disk/transient/mat/camera.txt`
    for `ar/select_car_test`. **Do not port** (developer tool; menus area).
13. **The target selector and target info hold 6 cars, not 7.** **Do not port.**

### Sound

1. **Toyota Pod voice**: port exists (`PodSound.c`). It matches L2's
   emotion → effect table, levels, pan and release (the emotion triggers are
   `Pod.c`'s, not re-checked here). It is missing these: the voice's **Doppler
   pitch** (at key-on and every frame), the per-frame **priority** update,
   and the **stop / reset together with the car's sounds** (pause, race end,
   cameras without Doppler). In the enhanced engine the Pod voice should
   be built as L2 builds it: a new channel class, a pan and a channel in
   `CarSound`, and a voice block plus the emotion one-shot in the sound frame. **Port.**
   Assets: `sound/gt3pod.ins` and `se.inf` entries `pod_genki` …
   `pod_wink` on bank 3.
2. **Engine sound load response changed** (`car_sound_engine_levels`). The
   throttle's break point is the engine's zero-torque throttle,
   friction / (friction + torque), where GT3 uses a fixed 0.375. The exhaust
   voice follows a new curve, 0.4 + 0.6·(1 − (1 − x)^16) over the
   positive load x, where GT3 has a straight line. **Port** (no new field,
   no asset).
3. **Sound mix table** (T285, gain per listening mode): in the own-car
   in-car view, engine 3.0→2.8, exhaust 2.5→2.3, gear whine 0.8→0.7 and
   road 0.6→0.5; outside views: exhaust 3.3→3.0. **Port** (data).
4. **Tyre squeal on tarmac** (tyre sound table, class 0). The squeal volume now follows
   the longitudinal slip speed, not the slip ratio, with new points and
   1.5× scale. The squeal pitch curves are reshaped. The smoke curve on
   tarmac also changed (effects area). **Port** (data, three rows).
5. **Squeal onset detune**: the right-hand wheels' squeal starts at 0.75 pitch
   for its first frame, so the left and right samples do not phase. **Port**
   (small).
6. **Gearbox type 3 and Dualnote**: the drivetrain-whine branches and the
   type-3 engine silence below 100 rpm. These are already ported
   (`Gearbox.c`, `Hybrid.c`); checked against L2, identical. No asset.
7. **BGM fade-out stop**: a new function with no callers in L2. **Do not
   port.**
8. **The race BGM's demo-music case is stubbed out** in L2
   (`isDemoWithoutPreview` returns 0). **Removed in Lupo 2**: GT3 keeps its demo music.

Unchanged, checked: course ambience (T177, T458), BGM / music / ADS (T463..T467),
SE engine (T468, T469 apart from the Pod bank), `CarSound` engine voices,
slots and one-shots, wind / road / rolling tyre / crash sounds, the per-car
Doppler, `RaceBGMPS2` volumes and play list, menu music selection, and the
engine-sound codes of the cars.

Candidates examined: cameras: **474 pairs** (104 plausible at ≥ 0.25, 14 non-plausible,
the rest skeleton-diffed: `build/scratch/survey_cameras_sound/cam_skel.txt`) and
**72 unpaired** L2 functions. Sound folder: **125 pairs** (30 plausible at ≥ 0.25,
1 non-plausible) and **25 unpaired**. Sound code outside the folder: **47 functions**
(updateSound, T285, T287 ×7, T333/T288 leaves the map missed ×7, RaceBGMPS2 ×17,
RaceLicenseBGM ×3, MusicPlayList ×7, start/stopSounds ×2, the race-entry copy
L2 `0x00175660`, the engine-sound getters ×2, MenuSound ×2). Hand-made pairs for the
map: `build/scratch/survey_cameras_sound/pairs_fix.csv` (102 right, 45 new) and the data tables (gain table,
tyre sound table, SE name table, both `se.inf`). Uncertain points are marked "(?)".

### Wrong or missing pairs in this area

- `GtSoundHandle` (T468). The map paired GT3's ctor, `stop` and `operator=` with the
  **new channel class**: L2 `0x0023B0A0`, `0x0023B2B8` and `0x0023B310`. The true twins are L2 `0x0023AE50`
  (ctor), `0x0023B000` (`stop`) and `0x0023B058` (`operator=`). `bgm_stop_unlocked`'s
  true twin is L2 `0x0023A288`. The mapped L2 `0x0023A2E8` is a new fade-out variant
  (entry S8).
- L2 `0x001DB848`, mapped to `CarDynamics_getTyre` by calls, is a new
  vehicle-layout accessor (see "For other areas").
- Unpaired L2 functions that are twins: `car_sound_wind` L2 `0x001DDF28`,
  `car_sound_road` `0x001DE018`, `car_sound_gear_whine` `0x001DE2C0`,
  `car_sound_engine_levels` `0x001DE458`, `car_crash_sound_clear` `0x001DE610`,
  `car_crash_sound_get` `0x001DE6E8`, `tyre_sound_update` `0x001F5CA0`,
  `RaceEntryCar::updateSound` `0x00176270`, `isOwnCar` `0x00210170`,
  `F_0020ED50` (StepPair step) `0x00215608`, T469's static init / fini `0x0023BD20` / `0x0023BD40` (body `0x0023BAF0`),
  `MusicPlayList::orderStraight` `0x002292B0`, `StartingCamera::update` `0x00217640`,
  `StartingCamera::setup` `0x002175A0` (with a new argument),
  `OnBoardCamera::getMountCount` `0x0020C688`, `CameraManager::CameraManager`
  `0x0020E580` (with `CameraView::CameraView` inlined), `startDemoCameras` /
  `updateDemoCameras` `0x002112E8` / `0x002113D0`, `F_0020EB38` `0x002152D0`,
  `CameraManager::getColourCorrection` `0x0020FAD0` (now virtual, slot 15),
  `CourseCamera::loopLength` … `findZone` `0x001A1560`..`0x001A188C`.
- Lupo 2's linkonce names give real names for GT3's anonymous accessors:
  `F_00209F88` = `getWideLinkStereoLevel`, `F_00209F68/78` =
  `get/setStereoLRdistance`, `getTileFrom*` = `getWideLink*Level`, `F_0020A038` =
  `isForciblyGamma`, `F_0020A030/28` = `isEnableBlur` / `setBlurLevel`,
  `F_0020A018/20` = `getCrossFadeTime` / `isStartCrossFade`, `F_0020A068` =
  `setLoggerSectorChanged`, `F_0020A060` = `setLoggerNewTarget`,
  `getViewKind/Variant` = `getReplayFollowMode` / `getFocusFollowMode`,
  `getViewCount/getCurrentView` = `getMonitorMax/Number`, `setInput` =
  `setInputToMonitor`, `F_00209BE0` = `getCameraInformation`, `F_00209DA0` =
  `getCar`, `F_00209E30` = `getDoppler`, `F_00209DC0` = `getGamma`,
  `OnBoardCamera::setFov/getCar/isMountChangeable/setMountChangeable` =
  `setBaseFovy/getMountCar/isEnableCtrl/setCtrl`, `CameraRotDamper::getShake/getAngle`
  = `getNoiseRatio/getPosition`.

## Entries: cameras

### C1. Replay TV cameras: smaller vertical aiming offset

| Field | |
|---|---|
| What | The designer (TV) cameras of replays aim with their pitch damper in mode 1. The aim is offset by a·k·range·cos(pitch), where range is the field-of-view rate (30 / zoom). k is 0.4 in GT3 and 0.14 in L2. In L2 the car sits nearer the vertical centre of the shot. |
| Where | `CameraRotDamper::start` L2 `0x0020DA88` = US `0x00206710`; `CameraRotDamper::update` L2 `0x0020DB80` = US `0x00206808` (`T379_camera_rot_damper.cpp`). Used by `DesignerCamera::update` (`pitchDamper.start(..., pitchMode = 1, ...)`, T398). |
| Kind | changed logic (constant) |
| Evidence | L2 `0x0020DADC` `lwc1 gp-0x6E64` = 0.14 (US `0x00206764` = 0.4); L2 `0x0020DBF4` = 0.14 (US `0x0020687C` = 0.4). Everything else in both functions is the same. |
| Port | Two constants. No field, no data. |
| Verdict | **port** |

### C2. Texture LOD bias of the race cameras

| Field | |
|---|---|
| What | Every race / replay camera's `setProjection` sets the GS texture LOD bias from its zoom: `(-log2(zoom) + c) × 256`. GT3's c is the constant −ln 2. In L2, c is a static variable, 0.0 by default, with a setter `CameraBase::setTextureLODBias`. Nothing in L2 calls the setter. So L2 selects mip levels about 0.69 level coarser: softer textures with less shimmer. |
| Where | `VariableCameraBase::setLodBias` L2 `0x00215A90` = US `0x001FEAA8` (`T397_variable_camera_base.cpp`); the variable is L2 `0x0035E164` (gp-0x4A0C); the setter is L2 `0x002D1038` (linkonce, no callers). |
| Kind | changed logic (constant → variable) |
| Evidence | L2 `0x00215AA8` `lwc1 gp-0x4A0C` (0.0) where US `0x001FEAC0` loads −0.693147. The only references to gp-0x4A0C are this load and the setter. |
| Port | A static `f32` in T397 (default 0.0) and the setter. Optional: make GT3's −ln 2 the default and leave the switch to the user. |
| Verdict | **port, optional** (a rendering taste; compare in game) |

### C3. Avantgarde replay mode (view kind 1): course colour presets; no two-car shots

| Field | |
|---|---|
| What | At each cut of view kind 1 (the BGM-synchronised "avantgarde" replay), GT3 rolls three random screen effects: a level override (lo 0..0.175, hi 0.2..1), the motion blur "frame effect" (10 %, 0.5..2.0) and a colour tint (5 fixed colours, saturation). L2 removes all three. Instead it reads colour-correction presets from the course's environment block (32 presets of 0x40 bytes after the 0xF50-byte CourseEnv). With 10 % it applies one of presets 16..31; otherwise, with 70 %, one of presets 0..15. With 20 % it clears the override. An active "special" preset ends with 50 % at the next cut. A preset sets the colour, saturation, level lo / hi and both override flags; a new state field marks it. The cross-fade (40 %) and the camera picks (`pickAutoCamera` tables unchanged; designer record, 25 % other camera; random bank, zoom and yaw offsets) are as in GT3. **Also: mode 1 no longer frames a pair of cars.** The second car of the selector slot is read but ignored, and the target is always one car. |
| Where | `CameraManager::updateMode1` L2 `0x00212D10` = US `0x0020C580` (`T390_camera_mode1.cpp`); preset collector L2 `0x00212CC0` (new: indices whose flag word +0x18 bit 0 is set, from a start index 0 or 16). The course environment is reached as `course +0x38 → [0] → +0x80` (L2), presets at `+0xF50 + i × 0x40`: r, g, b, levelLo, levelHi, saturation, flags. New CameraManager field: preset state (L2 `+0x268`, 2 = a preset is active), cleared by `resetState` (L2 `0x0020EAD8`). |
| Kind | changed logic + new data (course) + removed in Lupo 2 (random effects, pair shots) |
| Evidence | L2 calls `setPair` (L2 `0x0020B550`) only from modes 0 and 2 (`0x00212118`, `0x002121EC`, `0x00212804`, `0x00212994`); GT3 also from mode 1 (`0x0020C8BC`, `0x0020CA04`, `0x0020CAD8`). In L2 mode 1 the call to `CameraTargetSlot::getTarget` (L2 `0x00213F88`) is followed directly by `setCar(car, 1/120)` (`0x00213160..0x00213170`, `0x0021304C..0x0021305C`). GT3's constants 0.25, 0.175, 0.8, 0.0125, 0.2, 0.1, 150, 0.85 (effects) are absent. The new constants are 0.4 (fade), 0.5, 0.1, 0.7 and 0.9. The preset copies are at L2 `0x002136E4..0x00213740` / `0x002137FC..0x0021385C`. No store sets the frame effect (+0x26C) to 1 any more; only its fade remains (`0x00212FBC`). GT3's tint table `g_002DED30` is still in L2's data (`0x002DF1E8`) but unused. |
| Port | Presets: they need Lupo 2 course files (GT3's CourseEnv ends at 0xF50), so only with transplanted courses, falling back to GT3's random effects when a course has none. That needs a field for the state and the presets' place in the course data (course area). Pair shots: keep GT3's. |
| Verdict | **do not port** (the random effects and pair shots are GT3 features; presets only with Lupo 2 course data) |

### C4. Demo replays: choice of view mode

| Field | |
|---|---|
| What | A demo replay (attract mode) picks its view mode at random. GT3: draw < 0.33 traditional, < 0.66 broadcast (target by draw), else avantgarde; every 4th demo is avantgarde. L2: draw < 0.75 traditional, else avantgarde, so broadcast is never drawn. Every 4th is still avantgarde. **New: when the replay BGM option is off, the mode is forced to traditional.** Avantgarde cuts to the music's markers and is useless without it. |
| Where | `F_0020E930` L2 `0x002150B8` = US `0x0020E930` (`T394_replay_view_mode.cpp`); caller `CameraManager::init`. |
| Kind | changed logic |
| Evidence | L2 `0x002150E0` 0.75 (`lui 0x3F40`); `0x00215128..0x00215140`: `getGameStatus()`, options `+0x30C84` (`replayBgm`, the save survey's `+0x54`) == 0 → `*viewMode = 0`. The broadcast branch's code is still there but cannot be reached. The view-mode info table (L2 `0x002DF2B8`) is byte-identical to GT3's `g_002DF228`. |
| Port | A few lines; GT3's `GameOptions::replayBgm`. No field. |
| Verdict | **port** the replay-BGM condition; the 75 / 25 odds are optional (taste) |

### C5. New: TV-camera zoom button

| Field | |
|---|---|
| What | In view mode 0 with the designer (TV) camera, holding camera button bit `0x00800000` zooms in. A per-view value chases a target of 20 (0 when released) with a spring, and falls back quickly (×0.8 a frame) on release. The camera's `zoomMult` = clamp(1 + value × 4 / 20, 1, 4). `DesignerCamera::update` multiplies its zoom by `zoomMult` and divides the dampers' range by it. On a new course record it pulls `zoomMult` back to at most 2: clamp((m − 1) / 3 + 1, 1, 2). L2 moved the view-mode buttons from bits `0x00800000` / `0x01000000` (GT3) to `0x01000000` / `0x02000000` to make room. |
| Where | `CameraManager::updateMode0` L2 `0x00211D60` = US `0x0020B8E8` (`0x00212328..0x002123C4`); per-view spring object (3 floats: value, target, velocity; L2 view `+0x24C4`), reset L2 `0x002154E8` (called by `init`), update L2 `0x00215510` (called each frame per view by `CameraManager::update` L2 `0x0020FB58`), set-target L2 `0x002155D0`, get L2 `0x002155D8`. `VariableCameraBase::zoomMult` L2 `+0xE8` (ctor L2 `0x00215750` sets 1.0; linkonce `get/setZoomMult` L2 `0x002D08F8` / `0x002D0900`). `DesignerCamera::update` L2 `0x00215F50` = US `0x00201830` (`0x00216370..0x002163B0`, `0x0021674C..0x002167B4`). View-mode buttons: `CameraManager::update` L2 `0x0020FE78` / `0x0020FE98` (`lui 0x100` / `0x200`) against US `0x00208F2C`. |
| Kind | new function + new fields |
| Evidence | Spring constants 40, −10, 60, ±900 (L2 `0x00215510`), 0.8 (`0x002155B0`); zoom 20.0, 4.0, 1.0 (`0x00212364..0x002123BC`); `swc1 0x7E8(view)` = designer +0xE8; `& 0x00800000` on the held buttons (`0x00212350`). All VariableCameraBase fields after +0xE4 moved by 4 (`setGiveup` writes +0xEC/+0xF0, GT3 +0xE8/+0xEC). |
| Port | New field `zoomMult` in `VariableCameraBase` (GT3 layout: a free word or a new member); the spring per `CameraView`; the mode-0 and designer changes. Race-input area: a pad button must produce the bit, and GT3's view-mode bits must move or another bit must be used. |
| Verdict | **port, optional** (a new replay control; low risk) |

### C6. View mode 0: race start sequence and the starting camera (?)

| Field | |
|---|---|
| What | (a) In view mode 0, during the start, L2 uses the starting (pass-line) camera through `F_00209660`. It cuts with a 1 s cross-fade when its frame count (60 − the manager's counter, ?) reaches 60, 120 and 180. A counter of the camera manager (`+0x9504`) and the dynamics conductor's state (virtual slot `+0x48`: states 0 / 2 skip it) decide this. (b) `StartingCamera::update`: GT3 sweeps the pass-line parameter as 1 − (countDown − 60) / 600. L2 divides by 1200, half as fast. A flag set by a new set-up overload makes it run on its own frame counter once the countdown is over (60 − n). A state 2 stops it. The X axis is mirrored when bit 0 of the target car's `+0x2688` is set (? the car's side of the grid). The pass-line camera can take its records from the course (RaceCourse virtual slots `+0xA4` / `+0xA8`) instead of the one global `start.cam`. (c) In demos, the automatic "car ahead" cut fires after 2400 frames without a cut (GT3 300). |
| Where | `CameraManager::updateMode0` L2 `0x00211D60` = US `0x0020B8E8` (`T388_camera_mode0.cpp`; L2 499 against 348 insns); `StartingCamera::update` L2 `0x00217640` = US `0x00203028`; new `StartingCamera::setup(target, course, flag)` L2 `0x002175A0` (vtable slot 24, new); `StartingCamera::setStatic` (linkonce L2 `0x002D0CA0`: state 0 → 1); `PasslineCamera::setup` L2 `0x00217198` = US `0x00202B58`, `PasslineCamera::update` L2 `0x002171E8` = US `0x00202B90` (`setInverseX` field L2 `+0x10C`, read at `0x002172D0`) (`T399`, `T400`). |
| Kind | changed logic + new function + new fields |
| Evidence | L2 mode 0: `li 60`, `li 59`, `slti 60` (`0x00211E0C..0x00211E18`), `slti 0x960` (`0x00211F38`, GT3 `autoTimer < 300`), `jal F_00209660` (`0x002123D0`), `li 0x3C/0x78/0xB4` with `crossFade = 1`, `crossFadeTime = 1.0` (`0x002123D8..0x00212418`). L2 StartingCamera: `lui 0x4496` (1200.0) at `0x002176EC` where GT3 has 600; `0x00217748..0x00217750` `car +0x2688 & 1 → +0x10C`. |
| Port | Medium. StartingCamera grows by 0x40 bytes in L2 (counter, flag, state); add the new set-up and the mode-0 start phase. GT3's global `start.cam` can stand in for the course-supplied records. The details of (a) are not fully decoded (?). (c) is a constant. |
| Verdict | **port (?)**: (c) for sure; (a) and (b) after a closer reading of `updateMode0` |

### C7. In-car (sway) camera: taller occlusion box

| Field | |
|---|---|
| What | `isInsideCar` and `isViewBlocked` test whether the sway camera's position is inside another car's box, built from the car's collision box. The face points' height (y) is 1.5 m in L2 and 1.0 m in GT3, so tall vehicles count fully. |
| Where | `OnBoardCamera::isInsideCar` L2 `0x0020D010` = US `0x00200E08`; `OnBoardCamera::isViewBlocked` L2 `0x0020CBC8` = US `0x002009F0` (`T378_onboard_camera.cpp`). |
| Kind | changed logic (constant) |
| Evidence | L2 `0x0020D180` `lui 0x3FC0` (1.5) stored as `facePoint[k][1]` (`0x0020D1F0`..`0x0020D22C`); GT3 stores 1.0 there. The margins 0.04 / 0.05 and the 25 m² range are the same. The extra virtual calls (model info through the model object) are noise (C14). |
| Port | Constants. |
| Verdict | **port** (small; useful for the Pod, 1.68 m tall) |

### C8. i.LINK screen wall: stereo eye distance

| Field | |
|---|---|
| What | `CameraManager::init` sets the screen shift (GT3 `shift`, L2 `StereoLRdistance`) to 0.063 m; GT3 uses 0.2. With `layoutFlags & 3` (L2 `getWideLinkStereoLevel`) the view moves by ±shift / 2: one console renders each eye. 63 mm is a human eye spacing. |
| Where | `CameraManager::init` L2 `0x0020F2B8` = US `0x00208320` (`T383_camera_manager.cpp`). |
| Kind | changed logic (constant) |
| Evidence | L2 `0x0020F360` `lwc1 gp-0x6E14` = 0.063 → `+0x961C` (L2 `0x0020F398`); US `0x002083D8` 0.2 → `shift`. |
| Port | One constant. |
| Verdict | **port** (low priority: only an i.LINK stereo set-up uses it) |

### C9. New: head-tracker view for the in-car camera

| Field | |
|---|---|
| What | A new unit polls an input device each frame. In state 2 its readings (scaled by 2.79e-5) become yaw, pitch, roll and position offsets. `OnBoardCamera::applyView` applies them when enabled. The device is most likely the L2-only `pdiHt` class (? a head tracker). |
| Where | New L2 `0x001AE008` (singleton getter, init fov 60), `0x001AE068`, `0x001AE0A8` (poll, device from `0x00283CA0`), `0x001AE128`, `0x001AE188`, `0x001AE390` (apply). Called by `RacePS2Base` L2 `0x0017B754` and by `OnBoardCamera::applyView` L2 `0x0020C5F8` (= US `0x002004B0`, 16 against 8 insns). |
| Kind | new function (backend + camera) |
| Evidence | `0x0020C604..0x0020C618`: singleton `+0x64` enable → `0x001AE390` (rotations −[+0x4C], −[+0x44], −[+0x48]); L2-only string `pdiHt` (`0x0035E9B1`) and class `5pdiHt`. |
| Port | — (needs the device and its IOP driver) |
| Verdict | **do not port** |

### C10. Course demo cameras no longer run

| Field | |
|---|---|
| What | GT3's `CameraManager::update` runs the course demo cameras alone while `demoMode` is on. L2 never calls them; the functions are still there but have no callers. |
| Where | `startDemoCameras` / `updateDemoCameras` L2 `0x002112E8` / `0x002113D0` = US `0x0020A9C0` / `0x0020AA90` (`T385_camera_demo.cpp`); `CameraManager::update` L2 `0x0020FB58` = US `0x00208BE8`. |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | Neither function has a `jal` or a data reference in L2. US `0x00208C90` `jal updateDemoCameras` has no counterpart in L2's update. |
| Port | — |
| Verdict | **do not port** (GT3 keeps them) |

### C11. Camera random seeds through a race-installed function

| Field | |
|---|---|
| What | GT3 seeds every camera generator from the clock (`gfx_perf_counter0`). L2 seeds them from `CameraBase::random()`, which calls a function pointer the race installs (`RaceBase +0x10`, set by L2 `0x00179B78`). The default (L2 `0x0026BBE0`) reads the same clock, so a single console behaves as GT3. The hook lets linked consoles share seeds (L2-only `Multilink`, `RaceLinkDemo`). |
| Where | L2 `0x0021B410` (`CameraBase::random`), `setRandomFunction` (linkonce L2 `0x002D1030`); callers `CameraSway::init`, `CameraManager::init`, `CameraGrooveMeter::reset`, `F_0020E020`, `AutomaticCamera::setup`, `AutomountCamera::setup`, `MatrixCamera::setup`, `ZoomupCamera::setup`. |
| Kind | new function (infrastructure) |
| Evidence | `0x0021B410`: `lw gp-0x4A10; beqz → 0; jalr`. RaceBase ctor L2 `0x00179684` stores `0x0026BBE0`. |
| Port | — |
| Verdict | **do not port** (no change for one console) |

### C12. MenuCarCamera: debug camera mode 12

| Field | |
|---|---|
| What | `MenuCarCamera` gains mode 12 with its own view and update paths (new vtable slot 9). A loader parses `host0:/disk/transient/mat/camera.txt`; L2's arcade car-select test page `ar/select_car_test` calls it. |
| Where | L2 `0x0021BFD0`, `0x0021C000` (= US `applyView` `0x0020FB30`), `0x0021C040`, `0x0021C0E0`, `0x0021C100` (slot 9), `0x0021C388`, `0x0021C3B8` (loader); caller L2 `0x001092B0` (menus). Same slot added to `CarGetMenuCamera`. |
| Kind | new function (developer) |
| Evidence | Strings `host0:/disk/transient/mat/camera.txt` (`0x00335D38`), `ar/select_car_test` (`0x00335D60`). |
| Port | — |
| Verdict | **do not port** (host-only debug; menus area) |

### C13. Target selector and target info sized for 6 cars

| Field | |
|---|---|
| What | `CameraTargetSelector` (slots, hold timers, relation and rank tables) and `CameraTargetInfo::index[]` have 6 entries in L2; GT3 has 7. Each loop is shortened by one; the logic is the same. |
| Where | `CameraTargetSelector::reset` L2 `0x00213FA0` = US `0x0020D840`, `findTarget` L2 `0x00214068`, `update` L2 `0x00214220`; `CameraTargetInfo::save` L2 `0x0020B3C0` = US `0x001FF380`; `F_0020E1F0` L2 `0x00214950`; `CarProgress::init` (T392, T377, T393, T408). |
| Kind | changed data size (no behaviour change for ≤ 6 cars) |
| Evidence | Loop counters 5 (L2) against 6 (GT3), `bestRank` 6 against 7. |
| Port | — |
| Verdict | **do not port** (GT3 keeps 7) |

## Entries: sound

### S1. Toyota Pod voice (the port and what it misses)

| Field | |
|---|---|
| What | Each car's sound has a voice of its own for the Pod. When the Pod's emotion changes, `CarSound::update` releases the old voice and keys on the emotion's `pod_*` effect: emotion → SE slot {—, 50, 40, 44, 48, 47, 41, 49, 42, 45, 46}; slot 43 `pod_kowai2` is unused. Every frame the voice's level, pan, **pitch** and **priority** are set again from the car's position. It stops when both levels reach 0. Level = clamp(32 / distance × frame scale, −1, 1) × 1.0 × the two race SE factors, constant-power pan, the same in every listening mode. **Pitch = the program's note pitch × the car's Doppler factor** (the factor the engine voices get; 1.0 under a camera without Doppler). |
| Where | Dispatcher L2 `0x001A5318` (jump table `0x00342DD0`), called from `CarSound::update` L2 `0x001A4878` = US `0x001A6C38` (`0x001A48F4`). New channel class (T468): ctor L2 `0x0023B0A0`, set `0x0023B0B0`, bind `0x0023B0C8`, position `0x0023B110`, stop `0x0023B2B8`, `operator=` `0x0023B310`. `CarSound` grows by 0x20: a `CarSoundPan` (L2 `+0x7CC`) and the channel (`+0x7E0`). Built in the ctor (L2 `0x001A4370`), reset in `start` (L2 `0x001A47C0`: pan reset, `set(0)`), stopped with fade in `stop` (L2 `0x001A4CA8`), copied by the channel's `operator=` inside the race-entry copy L2 `0x00175660` (which inlines `CarSound::operator=`, US `0x00179230`). The sound frame grows. Its new voice block at L2 frame `+0x36C` holds the position, volume `+0x37C` = 1.0 and pitch `+0x380` = Doppler. The one-shots move to `+0x384`. The emotion one-shot is at `+0x394`, filled by `RaceEntryCar::updateSound` L2 `0x00176270` (= US `0x00177A50`; `0x00176964..0x001769C8`, value from `0x001E19D0`). |
| Kind | new function or class + new fields + new data (SE names, bank) |
| Evidence | The channel `position` (L2 `0x0023B1D4..0x0023B228`): `pitch = note2pitch(prog) × (f14 × 4096) >> 12`, `snd_voice_set_priority`, `snd_voice_set_pitch`, `snd_voice_set_volume` each frame. The dispatcher passes `f14 = frame +0x380`. Compare `PodSound.c`. The emotion → slot table and the triggers match L2, and so do the gains (32 / d clamped, race SE level × fade), the pan, the Q14 levels and the stop at level 0. **It lacks:** (1) the Doppler pitch, both at key-on (`req.pitch` is the bare note pitch) and every frame (only volume is re-set); (2) the per-frame priority; (3) the stop on `CarSound::stop` (pause, race end, replay / camera changes; `Pod_VoiceStop` runs only when an entry is cleared or its model changes) and the reset on `CarSound::start`. These follow from the plugin's hook point, which runs after `updateSound` has thrown the Doppler away (`POD_SOUND_PLAN.md` §4.5). |
| Port | Build it as L2 does. Add the channel class to T468. Add a pan and a channel to `CarSound` (+0x20 bytes before `field_7CC`, as L2; `RaceEntryCar` grows with it). Fill the frame's voice block and emotion one-shot in `updateSound` (the frame grows past GT3's one-shots). Call the dispatcher from `CarSound::update`. It needs the Pod's emotion source (`car+0x5F6`, car-model / physics area: `Pod.c`'s machine). The boot part of `PodSound.c` (51-entry SE table, `slti 0x33`, 4 bank objects, loading `gt3pod.ins` before `rn_01.es`) becomes plain source in T469 / T470: L2 `se_init` `0x0023B788` (bank 3 at `+0x60`, `0x0023B820..0x0023B830`), static init `0x0023BAF0` (4 banks). **Assets:** `sound/gt3pod.ins` (from the Lupo 2 disc, 81,760 bytes), and the 11 entries `pod_genki`, `pod_iya`, `pod_kowai1`, `pod_kowai2`, `pod_kyoro`, `pod_naku`, `pod_nemui`, `pod_odoroku`, `pod_okoru`, `pod_sabishii`, `pod_wink` on **bank 3, programs 0..10** (Lupo 2's `se.inf` entries 42..52). Merge them into GT3's `se.inf` (`GT3Hooks-main/tools/gt3/seinf_tool.py merge`). Do not replace it: Lupo 2's `se.inf` has no `slide` and adds `bolt`, `j_up` and `j_down`, which L2's code never names. The SE banks must end below SPU `0xAC000`. The Pod car's own engine sound files (`sound/engine/<code>`, `sound/<normal0..3/turbo0..3>/<code>`, from its ENGINE row's sound code) come with the car. |
| Verdict | **port** (the plugin's behaviour plus the three gaps) |

### S2. Engine sound levels follow the engine's real load

| Field | |
|---|---|
| What | Each frame, `car_sound_engine_levels` sets the two engine-voice levels from the turbo throttle t (≤ 1). GT3: engine voice = (\|t − 0.375\| + 0.375) × (1 locked / 0.5 slipping) × 127 × engine64; exhaust voice = (3t + 1) × 32 × engine68, a straight line from 32 to 127. L2 replaces 0.375 with r = frictionTorque / (frictionTorque + engineTorque), the throttle at which the engine gives no net torque at this rpm (`engine_output`: load × (torque + friction) − friction). The exhaust voice becomes (0.4 + 0.6 × (1 − (1 − x)^16)) × 127 × engine68 with x = max(0, t − r) / (1 − r). It idles louder (0.4 against 0.25) and rises to full almost as soon as the engine pulls. The turbo level and the blow-off decay are unchanged; each level is still averaged with the last frame. |
| Where | `car_sound_engine_levels` L2 `0x001DE458` (unpaired) = US `0x001DFC60` (`T288_car_sound.cpp`); reads `DynCarState.engineTorque` / `frictionTorque` (US +0x4B0 / +0x4B4, set by `car_pre_step` US `0x001EA428` = L2 `0x001E9768`). |
| Kind | changed logic |
| Evidence | L2 `0x001DE4BC..0x001DE4F8`: `f4 = [0x4B8] / ([0x4B8] + [0x4B4])` (L2's DynCarState is shifted by 4 after +0x436: throttle `0x444`, engineSnd `0x4E8`). `0x001DE554..0x001DE5A4`: `max(0, t − r) / (1 − r)`, four squarings, `× gp-0x75CC (0.6) + gp-0x75D0 (0.4)`. GT3 `0x001DFC88..0x001DFCF8`: 3.0, 32.0, 0.375. The type-3 gate at the top (`0x001DE478`) is the one `Gearbox.c` already ports. |
| Port | A few lines in T288, no new field (`engineTorque` / `frictionTorque` exist), no asset. Guard r against torque + friction = 0 (L2 does not). |
| Verdict | **port** |

### S3. Sound mix: gains per listening mode

| Field | |
|---|---|
| What | `updateSound` scales every car voice by a gain from a 5 × 16 table: listening mode × voice. L2 changes row 0 (own car, in-car view): engine 3.0→2.8, exhaust 2.5→2.3, gear whine 0.8→0.7, road 0.6→0.5. It also changes row 4 (outside cameras): exhaust 3.3→3.0. Rows 1..3 are unchanged. |
| Where | `g_002D33A8` (`T285_table.cpp`, US `0x002D33A8`) = L2 `0x002DE7E0`; lookup `F_001DE050` = L2 `0x001DD838`. |
| Kind | new data or table (values) |
| Evidence | Dump of both tables (`build/scratch/survey_cameras_sound/tables.txt`): L2 row 0 `[2.8, 2.3, 0.5, 0.2, 0.7, 0.3, 1.4, 0.5, 0.0, 0.9, 0.9, 0.6, 1.0, 0.8, 1.4, 1.0]`, row 4 `[0.5, 3.0, …]`. Columns: 0 engine (`sound/engine/<code>`), 1 exhaust (`sound/<kind>/<code>`), 4 gear whine, 7 road (`race_entry_internal.h` `SND_GAIN_*`); rows: `LISTEN_*`. |
| Port | Five floats. |
| Verdict | **port** |

### S4. Tyre squeal on tarmac: new curves

| Field | |
|---|---|
| What | The tyre sound table (3 sets × 4 surface classes × 0x28 bytes, then the car sound block) changes the three rows of class 0 (tarmac). **Volume:** curve 2 (wheelspin / locking) now reads the longitudinal slip speed (×1/18 m/s, `input2` 0→1), not the slip ratio. Its points change: x− {9, 80, 148, 255} → {33, 84, 140, 255}, y− {0, 100, 203, 203} → {0, 144, 255, 255}, x+ {9, 80, 148, 255} → {17, 92, 140, 255}, y+ {0, 100, 203, 203} → {0, 160, 255, 255}. The scales of curve 1 and curve 2+ go 20→30 (×1.5). **Pitch:** curve 1 x {4, 30, 54, 83} → {4, 62, 110, 207}, y {0, 56, 116, 179} → {0, 72, 132, 255}; curve 2 x− {4, 64, 168, 255} → {4, 64, 156, 255}, y− {0, 36, 92, 147} → {0, 80, 124, 151}, x+ {0, 52, 184, 255} → {0, 64, 156, 255}, y+ {0, 28, 104, 147} → {0, 80, 124, 155}. **Smoke (effects):** curve 2 starts later: x− {136, 144, 160, 180} → {168, 176, 192, 212}, x+ {144, 152, 168, 188} → {196, 204, 220, 240}. The car sound block (wind, road, rolling, whine, engine and crash parameters) and the other classes are unchanged. |
| Where | `g_002D35B8` (built by `tyre_sound_init` US `0x001DE830` = L2 `0x001F5938`, `T333_tyre_squeal.cpp`; the table belongs to `T355_setting_check`) = L2 `0x002DEE40`. |
| Kind | new data or table (values) |
| Evidence | Byte diff of the two tables (`build/scratch/survey_cameras_sound/tables.txt`): rows `+0x000`, `+0x0A0`, `+0x140` differ; `+0x1E0` (car sound block) is identical. |
| Port | Three 0x28-byte rows. The smoke row is the effects area's call; take it together with the others or not at all. |
| Verdict | **port** (the two sound rows; the smoke row with the effects survey) |

### S5. Squeal onset detuned on the right-hand wheels

| Field | |
|---|---|
| What | When a wheel's squeal starts (last frame's volume equals the curve base, 0 on tarmac, and this frame's does not), the odd wheels (front right, rear right) get their squeal pitch × 0.75 for that one frame. The two sides' identical samples then start out of phase. |
| Where | `tyre_sound_update` L2 `0x001F5CA0` (unpaired) = US `0x001DEBB0` (`T333_tyre_squeal.cpp`). |
| Kind | changed logic |
| Evidence | L2 `0x001F5F5C..0x001F5F7C` sets a flag; `0x001F6050..0x001F6074` `(wheel & 1) && flag → pitch × 0.75` (`lui 0x3F40`). The rest is the same: surface classes (jump table L2 `0x00349100` = US `0x0033C420` targets), kerb formula, smoke, dirt × 8 and 0.001. The loop runs over the vehicle layout's wheel count (4 for a car; see "For other areas"). |
| Port | A flag and two lines. |
| Verdict | **port** |

### S6. Drivetrain whine for gearbox type 3 and the Dualnote; engine silence for type 3

| Field | |
|---|---|
| What | `car_sound_gear_whine` goes to the Dualnote whine (pitch 0.012 × front wheels' mean \|speed\| + 0.4, volume front load × 0.25) or the type-3 whine (silent unless braking with the clutch closed; pitch 0.012 × driven wheels' mean \|speed\| + 0.3, volume (brake torques) × 0.2) before the stock code. `car_sound_engine_levels` zeroes both engine levels for a type-3 car below 10.472 rad/s (100 rpm). |
| Where | L2 `0x001DE2C0` (= US `car_sound_gear_whine` `0x001DFB58`), `0x001E11C8` (Dualnote), `0x001DFED8` (type 3), `0x001DE458` (= US `0x001DFC60`). |
| Kind | changed logic (already ported) |
| Evidence | Checked operation by operation against `Hybrid.c` `Hyb_Whine`, `Gearbox.c` `GB_Whine` / `GB_EngSndWrap`: same constants (gp-0x74D4 0.012, gp-0x74D0 0.4, 0.25; gp-0x7524 0.012, gp-0x7520 0.3, gp-0x751C 0.2; gp-0x75D4 10.472) and the same inputs. The stock whine part is unchanged. |
| Port | Already in the plugins; in C it goes in T288. **Assets: none.** The whine plays through the gear-whine voice, effect `turbine2` (`se.inf` → bank 1 = `gt3se01.ins`). If the motor whine should sound exactly as in Lupo 2, compare Lupo 2's `gt3se01.ins` (270,576 bytes, on the Lupo 2 disc) with GT3's: both name the program by the same `se.inf` name, and the samples may differ (not checked: GT3's file is not in the workspace). |
| Verdict | **port** (as already done) |

### S7. BGM fade-out stop (unused)

| Field | |
|---|---|
| What | A new `bgm_fade_stop(f32)` takes the BGM lock and stops the music with a fade (ADS: pdibgm RPC 3 with the negated time, clamped −1..1 × 32768). Nothing calls it. |
| Where | L2 `0x0023A6C0`, `0x0023A2E8` (the fade variant of `bgm_stop_unlocked`, mis-paired by the map with US `0x0022B678`), `0x0023AC60` (`ads` fade stop), `0x00278258` (pdibgm). |
| Kind | new function (dead) |
| Evidence | The only reference to `0x0023A6C0` is its FDE (`0x0031CC18`). |
| Port | — |
| Verdict | **do not port** |

### S8. Race BGM: the demo-music case stubbed

| Field | |
|---|---|
| What | In GT3, `RaceBGMPS2::isDemoWithoutPreview` makes a demo race play the demo music (group 0x22) and not chain songs. In L2 it returns 0, so demos play the play list. L2 also splits `getVolume` into an "is the option on" test and the volume (same logic). It reads the BGM status through a function pointer (`+8`, default `pdibgm_get_status`) for link play. |
| Where | L2 `0x0017BC70` (= US `0x0017DD80`), `0x0017BF88` / `0x0017BFF0` (= US `0x0017E0D8`), `RaceBGMPS2::update` L2 `0x0017BCB0` = US `0x0017DDF0`, ctor L2 `0x0017BB80` (`+8 = 0x0017C348`) (`T130_race_ps2_base.cpp`). |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | `0x0017BC70`: `jr ra; v0 = 0`. Volumes 0.5 / 0.7, group 0x14, demo group 0x22, the marker logic and `MusicPlayList` (L2 `0x00229100..0x002294B8`) are unchanged. |
| Port | — |
| Verdict | **do not port** (GT3 keeps its demo music) |

## For other areas

- **Race events renumbered**: GT3's event numbers from 0x14 on are one higher in L2 (BGM 0x14→0x15, view mode 0x15→0x16, error 0x17→0x18), as seen in `RaceBase::startSounds/stopSounds`, `CameraManager::update`, `updateMode1/2`, `updateDesigner` and `DesignerCamera::update`. This is a race-logic matter; nothing to port in the cameras.
- **Vehicle layouts (2-, 3- and 4-wheeled vehicles)**: table L2 `0x002DE690` (4 entries of 0x54 bytes: wheel count, wheel → axle, …), accessor L2 `0x001DB848` / `0x001DB870` (by car params `+0x30`). `car_sound_tyre_roll` (L2 `0x001DE0D8` = US `0x001DF9D8`) and `tyre_sound_update` use it; for a car it gives GT3's result. This belongs with the bike classes (`RaceBikeModel`, `HumanModel`, `VehicleModel`): physics / car model area.
- **Camera button bits**: the zoom (C5) uses `0x00800000`, and the view-mode buttons moved to `0x01000000` / `0x02000000`. `RaceInput` (vtable 11 → 13) must map them: race input area.
- **Head tracker** (`pdiHt`, C9): input / backend.
- **CourseEnv grew** by 32 colour presets (C3) and the course can supply start-camera records (C6, RaceCourse vtable `+0xA4` / `+0xA8`): course data area.
- **Tyre smoke curve on tarmac** (S4, row `+0x140`): effects.
- **Placed in the camera area, but car-box / collision code**: L2 `0x0020A668`, `0x0020A688`, `0x0020A7A8`, `0x0020A818` and the leaves up to `0x0020AC9C` (calls into L2 `0x00208E20..0x00209810`; callers in the dynamics conductor). These are for the physics survey.
- **Placed in the sound area, but tyre smoke / particle code**: L2 `0x001F6348` (smoke emit, calls `RaceCourse::emitDebris`), `0x001F6480`, `0x001F66B8` (called from car step `0x001EAB5C`), `0x001F6990`, `0x001F6C10`. These are for the effects survey.
- **`MenuCarCamera` debug mode** (C12): menus.

## Noise (looked at, no change)

Sound folder (the map's pairs and the unpaired):
- `T175`: `RaceBGMBase` new / delete / ctor / dtor / getters, `__tf11RaceBGMBase`. The static init / fini go through a compiler helper (L2 `0x001A4028`), and `get/setOrganization` are linkonce copies.
- `T176`: `EngineSound` ctor / dtor (L2 inlines the deleting dtor) / `onTransfer` / `copyHeader`, `car_sound_reset_spu`, `CarSoundPan::reset`, `car_sound_update_se_switch`, `reserveSpu`, `load` (only the offsets move: `CarSound` +0x20), `updateEngine` / `updateSlot` / `playOneShot` (the clamp is written as `< -0x4000`), `race_se_play`, `__tf11EngineSound`. The `CarSound` ctor / start / stop / update differ only by the Pod channel (S1).
- `T177`: `RaceAmbientSound` (all 6; the handle calls are the mis-paired `GtSoundHandle` twins).
- `T288`: `car_sound_params_init` (the 10000-rpm constant is 1047.19739 against 1047.19763, a rounding difference), `wind`, `road` (u16 pitch conversion), `car_crash_sound_add/clear/get`, `car_sound_tyre_roll` (vehicle layout; same for cars). `car_report_crash` differs only by the Pod crash triggers, which `Pod.c` ports (POD_TRIGGERS).
- `T333`: `tyre_curve1/2_build`, `tyre_curve_extra` (27.77777 against 27.77778), `tyre_sound_init`, the kerb constant 2449.9998 against 2450.
- `T458`: all 10 (loop forms).
- `T463..T467`: `music_*` (`music_get_tracks` is cut at an early `jr` by the tool), `bgm_kind` (the same bound written as `< 0x23`), `bgm_*`, `clock_*` (backend calls), `ads_*`, `bgm_*_relocate`.
- `T468`: `GtSoundHandle` / `GTSOUNDINSTRUMENT` / `GtSoundVoice` (with the twin corrections above), `se_play_program`, `sound_set_*`, the load callbacks, linkonce `setSpuLimit`, `getMUheader`, `getESheader`.
- `T469` / `T470`: `se_code`, `se_program`, `se_play`, `se_set_volume`, `se_term`, `se_inf_relocate`, `sound_init/term`. `se_init` and the static init differ only by the Pod bank (S1).
- Outside the folder: `RaceEntryCar::updateSound`. It is the same apart from the Pod block. The gains, Doppler, 4 wheels × 7 voices, crash one-shots and listening modes are unchanged. The `isOwnCar` and `getCameraKind` twins are L2 `0x00210170` and `0x00210420`. Also unchanged: T287's 7 tyre leaves (stride / base only), `RaceLicenseBGM` (3), `MusicPlayList` (`orderShuffle` inlines `fptoui`), `RaceBase::startSounds/stopSounds` (event number), `CarEquipments::getEngineSoundCode/getExhaustSoundKind`, and `MenuSound::selectMusic` (the same page → music pairs, reordered).
- `isMade__10TirePacket` (linkonce, placed here by neighbour).

Camera folder:
- Linkonce accessors and typeinfo functions (all `__tf*`, `RaceCameraBase` dtor and vtable, now emitted; `SquareMatrix` operator L2 `0x002CFD40`) and deleting destructors with `operator delete` inlined (the 14 non-plausible "size over 2x" pairs: 12 camera dtors, `MenuCarCamera` / `SimpleCameraBase` dtors).
- The camera classes' layout moved (view stride 0x24C0 → 0x2880, StartingCamera +0x40, VariableCameraBase +4 after +0xE4, CameraRequest flags in one word: `F_0020EE28/40/50/60` 4 → 9 insns). So did the addressing of CameraManager fields above 0x8000 (`lui $at, 1`): this explains `updateMarkers` (332 against 258), `resetState` (111 against 72), `updateWindow`, `setScreenLayout` (tail calls), `updateMarkerTime`, `updateStart`, `updateLogger`, `updateMode2` (block order), `updateDesigner`, `canCut` (one shared 1.3 load), `getCameraKind`, `isCenterScreen`, `virtual_12`, `F_00209370`, `F_002096C0`, `F_00209738`, `F_00209808`, `CameraGrooveMeter` (all), `CarProgress` (all), `F_0020EBA0`, `F_0020E020`, `F_0020E790`, `CameraManager::~CameraManager`, `init` apart from C8 and the zoom reset.
- `CameraManager` vtable 15 → 18: slot 15 = `getColourCorrection` (now virtual), 16 = `getCar`, 17 = `F_00209370`. `RaceCameraBase` gets the same interface: noise.
- `OnBoardCamera` (`update` reordered, `applyMountView`, `virtual_8`, `stepMount`, `findMountDown`, `findMountsToward/ByDirection`, `getMountDirection`): the model info and mount flags are reached through the car model's virtual calls (L2's vehicle model classes), with the same logic. The only exceptions are C7 and C9.
- `DesignerCamera`: `update` (single precision where GT3 used soft-float double; the rest is C5), `isTargetOutOfFrame` (the same), ctor / dtor, `setupOnRecord`, `setForceOnboard`, `getChangeLevel`, `designer_change_level`, `getFadeOutTime`, `isEnableChanging`, `isNeedReset`.
- `CameraRotDamper` (except C1), `CameraSway::pickTarget` (L2 drops GT3's dead computation), `CameraSpring`, `CameraShake`, `CameraCurve::init` (no soft-float double).
- `CameraTargetInfo` (`update`, `init`, `setCar`, `setPair`, `startBlend`, `finishBlend`, `refresh`, `getSingleWeight`, `getSecondCar`, `getSavedPosition`, `getPairPoint`, `getCarPoint`), `CameraTargetSelector` (C13), `TargetChooser` functions.
- `PasslineCamera`, `AutomaticCamera` (`placeAhead`, `placeBeside`, `setup`, `update`), `AutomountCamera`, `LoggerCamera`, `HeliCamera`, `MatrixCamera` (`matrix_ease` 0.128572 against 0.128571), `ZoomupCamera`: the same apart from the seed hook (C11).
- `CourseCamera` / `CourseCameraRecord` (T172: the same 13 pointers relocated; the six unpaired lookups have identical skeletons), `MenuCarCamera` (apart from C12), `SimpleCameraBase`, `CameraBase`.
- `VariableCameraBase::updateFrame` (96 against 118: the same, tighter code), ctor (`zoomMult`, C5).
- Unpaired L2 functions placed in the camera area that are only GT3 accessors under their L2 names (see "Wrong or missing pairs"): all of `0x002CFE08`..`0x002D1038` except `setZoomMult` / `getZoomMult` (C5), `setInverseX` / `setStatic` (C6) and `setRandomFunction` / `setTextureLODBias` (C11, C2).

## Delta review (map v2)

The 14 lines of `build/scratch/delta/cameras_sound.txt`, reviewed by the
rendering survey's agent (this survey's agent was not running). No new real
change: four are twins the old map missed, three are compiler-level helpers,
three belong to C12, four are car-box helpers for the physics survey, one is
uncertain link-battle code.

| # | Candidate | Verdict |
|---|---|---|
| E1 | L2 `0x0020AC20` (7 insns) | **belongs elsewhere (physics), new**: `lerp(t, a, b)` on floats; used only by E4 |
| E2 | L2 `0x0020AC40` (11 insns) | **belongs elsewhere (physics), new**: fills a 32-float array with one value; called by `CarBox::CarBox` (L2 `0x00209338` = US `0x001F42E0`), L2 `0x002093B0` and E4 |
| E3 | L2 `0x0020AC70` (12 insns) | **belongs elsewhere (physics), new**: copies a 32-float array; called from the car-box code L2 `0x00209370`..`0x00209880` and E4 |
| E4 | L2 `0x0020ACA0` (41 insns) | **belongs elsewhere (physics), new**: element-wise lerp of two 32-float arrays (E2, E1 × 32, E3); called by L2 `0x00209418`. E1..E4 are leaves of the car-box code this report's "For other areas" already hands to the physics survey |
| E5 | L2 `0x0020EFF0` (14 insns) | **twin, noise**: US `0x002080E0` `CameraManager::isCenterTopLeft` (fields +0x9608 / +0x9614 = GT3 +0x9700 / +0x970C: the manager's layout moved by −0xF8) |
| E6 | L2 `0x0020F028` (14 insns) | **twin, noise**: US `0x00208110` `isCenterTopRight` |
| E7 | L2 `0x0020F060` (14 insns) | **twin, noise**: US `0x00208140` `isCenterBottomLeft` |
| E8 | L2 `0x0020F098` (14 insns) | **twin, noise**: US `0x00208170` `isCenterBottomRight` (the same code as `isCenterScreen` L2 `0x0020F0D0`, as in GT3). The map missed E5..E8 because their caller moved: GT3 calls them inline in `RaceLinkBattle::virtual_8` (US `0x001853A0`); Lupo 2 calls them from a new link-battle set-up function L2 `0x001B31F0` (race modes area), together with `setScreenLayout` L2 `0x0020F108` = US `0x002081D0` |
| E9 | L2 `0x0020F290` (9 insns) | **new code (?), belongs elsewhere (race modes / i.LINK)**: sets `CameraManager` +0x20 = 0.0, +0x24 = 224.0, +0x28 = 1/2.0; called once from L2 `0x001B31F0` in one layout case (`+0xC14 == 9`); no GT3 counterpart found. Link-battle only: do not port |
| E10 | L2 `0x002156B8` (2 insns) | **noise**: clears a camera request's flag word (`sw $zero, 0($a0)`), called by `CameraManager::update` (L2 `0x0020FD10`); the request flags are one word in L2 (noise list); GT3 clears them inline |
| E11 | L2 `0x0021B2C8` (4 insns) | **noise (out-of-line base constructor)**: `CameraBase::CameraBase` (vptr `0x00334420`), called by the camera constructors (`DynamicEnvmapCamera`, `CameraManager`, `VariableCameraBase`, `MenuCarCamera`, `SimpleCameraBase`, `CarGetMenuCamera`, `gt2k_menu::Car` and two L2-only menu objects); GT3 inlines it |
| E12 | L2 `0x0021BA28` (4 insns) | **new, part of C12**: `MenuCarCamera` set debug mode: mode (+0x04) = 12, parameter (+0x14C); called by the `select_car_test` page L2 `0x001092B0` |
| E13 | L2 `0x0021C638` (25 insns) | **new, part of C12**: called twice by the same page L2 `0x001092B0` |
| E14 | L2 `0x0021C6A0` (39 insns) | **new, part of C12**: called by L2 `0x0021C040` (C12's view path) |
