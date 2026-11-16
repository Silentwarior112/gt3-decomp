# Enhanced engine: the shared interfaces (wave 0)

What wave 0 put in for the wave-1 areas (`PLAN.md`): the switch files, the
fields several areas share, the appended event numbers, three signatures.
Nothing reads the new fields yet and every new parameter is passed GT3's
value, so the game behaves as before. The layouts did change (section 5).

The interfaces are fixed for wave 1: an area that needs one changed asks
(its port notes, "For other areas"); it does not change it.

Lupo 2 offsets below: "car+0x..." is Lupo 2's car object (GT3's
`RaceCarState`); "block+0x..." its physics block at car+0xD8 (GT3's
`RaceCarState::st`, `DynCarState`, at +0xF84; at and above about 0x3A0 Lupo
2's block offsets are GT3's + 4); "params+0x..." its `CarDynamics`.

## 1. Switch files

`include/game/enhanced.h` includes one file per area. `1` = Lupo 2 (the
default), `0` = GT3. Each switch's comment says what Lupo 2 does and its
survey entry. Each file belongs to its area's agent.

| File | Owner | Switches |
|---|---|---|
| `include/game/enhanced/physics.h` | physics | `ENH_PHYSICS_PITCH_INERTIA` (3), `_REAR_ABS` (4), `_DYNPARAMS` (5), `_REAR_RIDE_HEIGHT` (6), `_TYRE_REF_GRIP` (7), `_AUTO_DOWNSHIFT` (9), `_FIRST_GEAR_THRESHOLD` (10), `_ATTESA` (12), `_ROLL_RATE` (14), `_UNDERSTEER_BRAKE` (15), `_AI_OVERSTEER` (16), `_PAD_STEERING` (17), `_WHEEL_THROTTLE` (18), `_FORCE_FEEDBACK` (20), `_CATCHUP_CAP` (21), `_DISPLAY_FLAGS` (22), `_ATTESA_RADII` (24), `_PAIR_CONTACT` (27), `_TIMED_END_AT_LEADER` (29) |
| `include/game/enhanced/rendering.h` | rendering | `ENH_RENDER_TYRE_SHOULDER` (9), `ENH_RENDER_FLARE_NO_FOG` (10) |
| `include/game/enhanced/hud.h` | hud | `ENH_HUD_DISPLAY_RPM` (C2), `_TELEMETRY_CYCLE` (C3 + C4), `_AB_MONITOR` (C5), `_STEERING_DISPLAY` (C6), `_TACHO_DIALS` (C8), `_SPEEDO_SCALE` (C9), `_NEEDLE_LOOK` (C10), `_MINIMAP_POINTER` (C11), `_TEXT_LOOK` (C12), `_CAR_SIGN` (C13), `_LOGO_LOOK` (C14), `_DEMO_CAR_NAME` (C15), `_TIME_LIMIT_LOOK` (C16), `_FREE_RUN_LIVE_MESSAGES` (C23), `_LICENSE_RESULT_TIMING` (C24) |
| `include/game/enhanced/race.h` | race | `ENH_AI_TORQUE_HANDICAP` (A1), `ENH_AI_CAUTION` (A2), `ENH_AI_TRAFFIC` (A3), `ENH_AI_START_LOOKAHEAD` (A4), `ENH_AI_PIT_EXIT` (A5), `ENH_AI_RECOVERY` (A6), `ENH_AI_SLIP_STEER` (A7), `ENH_AI_DRIVER_SPREAD` (A8 a), `ENH_AI_SKILL_AS_GRIP` (A8 b + physics 13), `ENH_AI_COOL_DOWN` (A9), `ENH_RACE_GHOST_CAR` (M1), `ENH_RACE_TIME_UP_ENDS` (C1), `ENH_RACE_TIME_LIMIT_GRACE` (C2), `ENH_RACE_AFTERIMAGE` (D1), `ENH_INPUT_WHEEL_MENU_STEER` (backend B3) |
| `include/game/enhanced/menus.h` | menus | `ENH_MENU_ATTRACT_REPLAYS` (menus_a 4), `ENH_MENU_BUSY_MARK` (menus_a 11), `ENH_MENU_REMEMBER_CURSOR` (menus_a 15), `ENH_MENU_WIDE_FROM_CONSOLE` (menus_b 10 + backend B7) |
| `include/game/enhanced/cameras_sound.h` | cameras + sound | `ENH_CAMERA_TV_AIM` (C1), `_LOD_BIAS` (C2), `_DEMO_VIEW_MODE` (C4), `_ZOOM_BUTTON` (C5), `_START_SEQUENCE` (C6), `_OCCLUSION_BOX` (C7), `_STEREO_EYES` (C8); `ENH_SOUND_ENGINE_LOAD` (S2), `_MIX_TABLE` (S3), `_TARMAC_SQUEAL` (S4), `_SQUEAL_DETUNE` (S5) |

Choices made here:

- `ENH_AI_SKILL_AS_GRIP` is in `race.h`; the physics area reads it (physics
  13: the grip handicap under `#if`, GT3's km/h use of `enemySkillC2` in the
  AI under `#if !`). Never both.
- Race_logic A8 has two halves. (b) is `ENH_AI_SKILL_AS_GRIP`. (a), the driver
  spread in every race, got its own `ENH_AI_DRIVER_SPREAD`. The race agent may
  merge them.
- Physics 27 and 29 are coded in `race/dynamics`, so their switches are in
  `physics.h`. Physics 27 was tuned together with race_logic A2.
- hud C3 + C4 share one switch (PLAN.md lists them together). C8 and C9 have
  one switch each.

## 2. Fields

### The car data path: special kind and gearbox type

GT3 has unused bytes at exactly the places Lupo 2 uses, so these fields are
named there and nothing grows. No GT3 code reads or writes these bytes:
`SetupCarSpec` clears the spec, and a whole-image scan found the
Transmission byte to be padding (`docs/gearbox_dualnote.md` section 4). None
of them is saved data: `CarSaveEquipments` does not change.

| Name | Type | Where | Writer | Readers | Lupo 2 |
|---|---|---|---|---|---|
| `CarSpec::specialKind` | u8 | `include/game/data/T482_car_equipments.h`, +0x011 (was `unk_011`) | physics: `SetupCarSpec` from the car code (`nameToCode` chain) | physics (`CarDynamics_init` copies it) | CarSpec+0x11, L2 0x001DACDC..0x001DADB8 |
| `CarDynamics::specialKind` | u8 | `include/game/race/display/T279_meter_scale.h`, +0x032 (was `unk_032`) | physics: `CarDynamics_init` | physics (kind 7 suspension, wing, Pod), rendering (wing, Pod model), sound (Pod voice), race (Pod rules); reach it as `car->params.specialKind` | params+0x32, copy L2 0x001DC7A4; `isPod` L2 0x001E1288 |
| `CAR_KIND_*` | enum | T279 | | | 1 Pod, 2 / 8 / 9 / 10 wing cars, 7 SL active suspension (physics.md 2) |
| `TransmissionRecord::gearboxType` | u8 | T482, +0x30 | paramdb data (widened GEAR table, rows of 0x38) | physics: `getSpec` / `specGear`. Read it only when the table's element size is over 0x30. GT3's rows are 0x30 bytes, so there +0x30 is the next row's code. | GEAR row +0x30 |
| `CarSpec::gearboxType` | u8 | T482, +0x0E1 (was `unk_0E1`) | physics: `getSpec`, from the row or from a list of row ids (as GT3Hooks `g_gbxRows`) | physics (`gear_calc`) | CarSpec+0xE1 (GT3Hooks `GB_ApplyGear`) |
| `GearCalc::gearboxType` | u8 | `include/game/car/physics/T292_gear_calc.h`, +0x12 (was `unk_12[0]`; `unk_13` stays) | physics: `gear_calc` / the Transmission set-up | physics (the 33 type tests), sound (S6: type-3 whine and engine silence), reach it as `car->params.gear.gearboxType` | Transmission+0x12 (car+0x208+0x12) |
| `GEARBOX_*` | enum | T292 | | | 0 stock, 1 CVT, 2 clutchless, 3 electric |

### RaceCarState: Lupo 2's per-car state

These are appended after GT3's 0x16DC bytes
(`include/game/car/physics/T302_car_state.h`), and the size is now 0x1718.

In Lupo 2, the Dualnote, the moving wing and the Pod share the word
car+0x5F0 and the bytes +0x5F4 / +0x5F5 (one feature per car). Here each
feature has its own fields.

`car_init` clears all of them: its memset is now `sizeof(RaceCarState)`.
A snapshot does not keep them (section 5).

| Name | Type | Offset | Writer | Readers | Lupo 2 |
|---|---|---|---|---|---|
| `batteryCharge` | f32 | 0x16DC | physics: full at the car's set-up, regeneration and assist | physics; hud C1 | car+0x5F0 (kJ) |
| `batteryCapacity` | f32 | 0x16E0 | physics at the set-up (the constant CAP, 1125 kJ); 0 when not a hybrid | hud C1 shows `batteryCharge / batteryCapacity` | constant block; L2 0x001E02C0 divides by it |
| `wingPosition` | f32 | 0x16E4 | physics: the moving-part updater (L2 0x001E1E00..0x001E2140, from `DynamicsConductor::update` and the set-up) | rendering: `CarModel::wing` (SWITCH 6) | car+0x5F0 (kinds 2 / 8 / 9 / 10) |
| `displayRpm` | f32 | 0x16E8 | physics: reset at the set-up (L2 0x001E4658), spring k 1000 / c 50 in the drivetrain step (L2 0x001E4698) | hud C2 (tachometer), physics 22 c | .bss 0x004A9520[carIndex]; getter L2 0x001E4728 |
| `displayRpmSpeed` | f32 | 0x16EC | physics (the spring's rate) | physics | 0x004A9540[carIndex] |
| `frontSlip` | f32 | 0x16F0 | physics: `car_tyre_lat_forces` (physics.md 8) | race: AI steering gain (A7) | block+0x424 |
| `cvtRatio` | f32 | 0x16F4 | physics (GEARBOX_CVT) | physics | block+0x3A4 |
| `animChannel[4]` | f32 | 0x16F8 | physics: `car_view_update` (channels 2 / 3: smoothed throttle / handbrake), `car_input_update` (1, kind 3) | rendering (and the per-car animation object, if ported) | car+0x80C; set L2 0x001E2AA0, get L2 0x001E2988 (which also derives channels 4..8) |
| `animEvent[4]` | s8 | 0x1708 | physics: the shift callbacks (L2 0x001E2A80 from 0x001E5524 / 0x001E55E8 / 0x001E5608) | rendering | car+0x808; get L2 0x001E28C0 |
| `hybrid` | u8 | 0x170C | physics at the set-up: DRIVETRAIN layout 2 and awdType 5 (`spec->layout == 2 && spec->awdType == 5`) | physics; hud C1 (the gauge only for a hybrid); sound S6 | `GearCalc.driveType == 7` (L2 0x001E0288). GT3 keeps 7 for its fixed-split 4WD, so the flag is separate |
| `batteryFlow` | s8 | 0x170D | physics: the step's energy flow, clamped -100..100, cleared each race step (L2 0x001E0348 / 0x001E0378) | hud C1: blue when >= 0, magenta when < 0 | car+0x5F5 |
| `wingUp` | u8 | 0x170E | physics: the wing's hysteresis | physics | car+0x5F4 |
| `podParking` | u8 | 0x170F | physics: the parking counter, 0..0x22, 0x22 = it sits (L2 0x001E1C88 / 0x001E1BC8) | physics (squat); race: the stop-area rule ("sat down", L2 0x001E1CF0) | car+0x5F0 (byte) |
| `podEmotion` | u8 | 0x1710 | physics: the emotion machine (L2 0x001E1308 set, 0x001E1520 step, reset 0x001E12A0). Low nibble = current, high = previous | rendering: face select (`CarModel::animSelect`), colour; sound (with `podSound`) | car+0x5F1 |
| `podEmotionTimer` | u8 | 0x1711 | physics (frames left; 0xFF = held; 60 after a reset) | physics | car+0x5F2 |
| `podEmotionHold` | s8 | 0x1712 | physics | physics; rendering (Pod model drive) | car+0x5F3 |
| `podAnimFrame` | u8 | 0x1713 | physics: the animation frame 0..0x2F | rendering: SWITCH 7's bob (`CarModel::podBob`); SWITCH 9 and the face are the current emotion (rendering's port, wave 1) | car+0x5F4 |
| `podLean` | s8 | 0x1714 | physics: the tail / look lean -30..30 | rendering: SWITCH 7 (`CarModel::podLean` / `podBob`, L2 0x001E18A0) | car+0x5F5 |
| `podSound` | u8 | 0x1715 | physics (`setAnimState`'s target) | sound S1: the emotion the voice plays (L2 0x001E19D0) | car+0x5F6 |
| `RACE_CAR_STATE_GT3_SIZE` | #define 0x16DC | T302 | | the snapshot (section 5) | |

Values that already have a GT3 field (nothing was added for them):

- The Dualnote's front motor load. It drives the whine volume (load / 4).
  Lupo 2 keeps it in block+0x4F8, which is GT3's
  `DynCarState::clutchTorqueAbs` (+0x4F4, T288): GT3's gear whine reads it.
  The Dualnote writes its load there, as Lupo 2 and the GT3Hooks port do.
- The car messages: `DynCarState::msgKind` (+0x51C, Lupo 2 block+0x520).
- The steering display byte: `DynCarState::steering` (+0x523).
- Lupo 2's channel 7 is a steering spring at block+0x3B0 / +0x3B4. That is
  probably GT3's `DynCarState::unk_3A8[0xC]`, 12 bytes no GT3 code touches
  (the CVT ratio Lupo 2 inserted at block+0x3A4 moves its fields up by 4).
  If physics or rendering ports channel 7, check this and name those bytes;
  nothing needs to grow.

### CarModel: Lupo 2's render inputs (rendering.md 2)

These are appended after GT3's 0x38 bytes
(`include/game/car/model/T446_car_model.h`), and the size is now 0x54.
Lupo 2 put them in its `CarModel::Callback` at +0x24..+0x3C. Its
`selectSwitch` answers SWITCH ids 6..10 from them; the callback is
CarModel's first member, so `(CarModel *)this` reaches them. The
constructor clears them.

When the rendering port gives them meaning, it sets Lupo 2's defaults
(Callback constructor L2 0x0022D900): podLean 0.5, podBob 0.5, animSelect
-1 (-1 = every animation record). Writer and reader: rendering
(`RaceCarModel::updateBodyState`, the menu car), fed from the
`RaceCarState` fields above.

| Name | Type | Offset | Use | Lupo 2 |
|---|---|---|---|---|
| `wing` | f32 | 0x38 | SWITCH 6 morph, the moving part 0..1 | +0x24, setter L2 0x0022DC60 |
| `podLean` | f32 | 0x3C | SWITCH 7 morph, branch 0, when != 0.5 | +0x28, L2 0x0022DC68 |
| `podBob` | f32 | 0x40 | SWITCH 7 morph, branch 1, when podLean == 0.5 | +0x2C |
| `podColor` | int | 0x44 | SWITCH 8 branch (colour category) | +0x30, L2 0x0022DC78 |
| `podColorBlend` | f32 | 0x48 | SWITCH 8 morph | +0x34 |
| `podState` | int | 0x4C | SWITCH 9 branch | +0x38, L2 0x0022DC88 |
| `animSelect` | int | 0x50 | SWITCH 10 branch; the CLUT animation id (`setAnimSelect` L2 0x0022DC90) | +0x3C |

`RaceCarModel::blinkPhase[2]` (f32, +0x6D8,
`include/game/car/model/T180_race_car_model.h`): the lamps' two blink phase
accumulators (rendering.md 4; Lupo 2 `RaceCarModel` +0x6DC / +0x6E0). They
are used by rendering only. They are added now so that `RaceEntryCar`
(race/core) grows only once. The constructor clears them.

### RaceDisplay: the display flags (hud C3, C14)

`include/game/race/display/T154_race_display.h`; the size is now 0xA480.
Lupo 2's flags follow `finishDisplay` exactly as here: L2 +0xA8CC
finishDisplay, +0xA8CD, +0xA8CE, +0xA8D0. The constructor sets GT3's
behaviour.

| Name | Type | Offset | Default | Writer | Readers | Lupo 2 |
|---|---|---|---|---|---|---|
| `miniLogo` | u8 | 0xA479 (GT3's tail padding) | 1 | race: the replay toggle (`RaceBasic::virtual_28`, L2 0x0017F548..0x0017F62C) | hud C14 (logo and car name panel) | +0xA8CD |
| `carNameDisplay` | u8 | 0xA47A (tail padding) | 0 | none found | hud C14 | +0xA8CE |
| `showMask` | int | 0xA47C | `RD_SHOW_LIVE` | race: the SELECT cycle (L2 0x0017F9A0; table {1, 3, 0, 0} over `RD_SHOW_AB_MONITOR` / `RD_SHOW_STEERING`); hud C3 (Lupo 2 also resets it in `virtual_0`) | hud C3 (`virtual_3` live, `virtual_2`'s layout variant) | +0xA8D0 |

`RD_SHOW_*` moved from `src/game/race/display/race_display_internal.h` to
the public T154 header, so race/core can use the names.

## 3. Event and message numbers (appended, never inserted)

| Name | Value | Where | Lupo 2's number | Posted by | Taken by |
|---|---|---|---|---|---|
| `RACE_EVENT_CAR_DISQUALIFIED` | 0x18 | `include/game/race/core/T122_race_event_queue.h` | 0x0B (inserted; Lupo 2 moved GT3's 0x0B..0x17 up by one) | the Pod race's lap check (L2 0x001F7B20), arg = the car | hud C20 (`SimpleExamDisplay`: 0xA or this, own player car); menus_b 14 (result board) |
| `RACE_EVENT_TYPES` | 0x19 | T122 | | | sizes `RaceEventQueue::counts` |
| `RD_INFO_POD_SAT_DOWN` | 7 | `include/game/race/display/T194_race_display_event.h` | 7 (also appended in Lupo 2) | the Pod stop-area rule (L2 0x001F7708..0x001F7D98), as an information event (type 0x12) | hud C18 (`pollInformationEvents`, RACEDISPLAY 57) |
| `CARMSG_POD_STOP` | 0x10 | `include/game/car/physics/T331_dyn_car_message.h` | 16 (the same) | the Pod rule (L2 0x001F7D08 / 0x001F7D70) | hud C17 (`showWarning`, RACEDISPLAY 56, amber) |
| `CARMSG_POD_RETURN` | 0x11 | T331 | 17 (the same) | the Pod rule (L2 0x001F7D28) | hud C17 (RACEDISPLAY 58, red) |

Lupo 2's renumbering, for reading its code (GT3 -> Lupo 2):

- lap time 0x0B -> 0x0C, check point 0x0C -> 0x0D, logger sector
  0x0D -> 0x0E;
- general time 0x0E -> 0x0F, time diff 0x0F -> 0x10, lap diff 0x10 -> 0x11;
- speed 0x11 -> 0x12, information 0x12 -> 0x13, lap record 0x13 -> 0x14;
- BGM 0x14 -> 0x15, view mode 0x15 -> 0x16, record full 0x16 -> 0x17,
  error 0x17 -> 0x18.

Lupo 2's `getEventType` 0x0C and its posts `li 0x15` / `li 0x17` are GT3's
0x0B, 0x14 and 0x16 here.

The RD_INFO_* codes 1..6 moved from `race_display_internal.h` into T194 with
the new one. The conductors keep their own names for 2 and 3 in
`dc_modes_internal.h`.

Notes:

- The new event has the highest type, so it is the least important: a full
  queue (256 events) drops it first.
- `DynCar_setMessage` reads the duration table `g_002D32F0[16]` (T331)
  indexed by the kind. It must grow to 18 entries (Lupo 2 L2 0x002DEC38)
  before anything sets 0x10 / 0x11; otherwise the kind reads the .eh_frame
  CIE that follows the table. This is the physics area's job, with the Pod
  rule.

## 4. Signatures

- `RaceDisplayBase::virtual_2(f32 dt)`: the display update's time step
  (hud C19).
  - Declared in `include/game/race/display/T154_race_display.h`.
  - Overrides: `RaceDisplay` (T154: `dt` not read yet),
    `RaceTimeTrialDisplay` (T141) and `RaceLicenseDisplay` (T147) pass it to
    `RaceDisplay::virtual_2`; `RaceSplitDisplay` (T155) passes it to both
    displays.
  - Callers pass `RACE_DISPLAY_STEP` (`1.0f / 60.0f`, defined in the T154
    header): `RaceSinglePlayer::virtual_28` (T133), `RaceSplitBattle`'s
    update (T137), `RaceLinkBattle` (T138), `RaceSolitaire::virtual_28`
    (T139, both displays).
  - GT3's display advances one frame per call. Lupo 2's races pass a
    constant 1/60 (`lwc1 $f12, 1/60` at L2 0x0018041C), and its HUD toggle
    passes 0 so the display is re-laid-out without animating.
  - The hud port makes the animations step only when `dt > 0`, as Lupo 2
    does. The race port (C3 toggle) calls `virtual_2(0.0f)`.
- `int CarEquipments::isCVT()`: `getGearCount() == 2` (menus_b 1; Lupo 2
  L2 0x002433B0).
  - Declared in `include/game/data/T482_car_equipments.h`, defined in
    `src/game/data/T482_car_equipments.cpp`.
  - It tests the GEAR row's gear count, not the gearbox type. No GT3
    gearbox has 2 gears.
  - No caller yet. The menus area will call it from the ratio texts,
    `check_cvt`, the car select, `run_arcade_race` and `run_free_run`.
- Response curve mode 3 (`RESPONSE_POWER8`, `src/game/common/T354_response_curve.cpp`):
  y = c u^8 + b u, slope 8 c u^7 + b, in Lupo 2's order of products (L2
  0x001FEBC0).
  - `responseInit` also knows mode 3 (L2 0x001FEAA8): n is clamped to
    0..100, c = n / 100, b = 1 - c, a = 0. Without it, mode 3 would fall into
    the default case (a = b = c = 0) and give 0.
  - GT3's curves use modes 1, 1 and 0 (`g_002D4628`), so nothing uses mode 3.
    Physics 18 switches the wheel throttle to it (InputTuning bytes 3 / 70).
  - The other changes of physics 18 are the physics area's, under its
    switch: mode 2 rescaled, mode 1 accepting n = 0.

## 5. Layout changes

Every size below was computed by egcs: `tools/sizeprobe.py` compiles
every `include/game` header together and compares each `EGCS_SIZE` with
`sizeof`. After a struct grows, run it from the tree's root:
`python tools/sizeprobe.py --only-diff`.

Member offsets in the headers' comments stay GT3's. Where members moved,
a note at the class says by how much.

| Class | GT3 | Now | Why |
|---|---|---|---|
| `CarModel` | 0x38 | 0x54 | render inputs |
| `RaceCarModel` | 0x6C0 | 0x6E4 | CarModel, blink phases (members from `tires` on +0x1C) |
| `RaceEntryCar` | 0x2148 | 0x2170 | RaceCarModel (members from `pendingModel` on +0x24) |
| `gt2k_menu::Car` | 0x720 | 0x73C | CarModel |
| `gt2k_menu::TheCar` / `CarsItem` | 0x36C / 0x1048 | 0x388 / 0x10B8 | CarModel |
| `RaceCarState` | 0x16DC | 0x1718 | per-car state |
| `DynamicsConductor` and its 8 subclasses | 0xE3AC | 0xE58C | 8 cars; members from `course` on +0x1E0 |
| `RaceEventQueue` | 0x820 | 0x824 | counts[0x19]; `count` 0x81C, vptr 0x820 |
| `RaceOrganization` | 0x8C8 | 0x8C8 | members after `eventQueue` +4 (the tail alignment absorbs it) |
| `RaceDisplay` | 0xA47C | 0xA480 | flags |
| `RaceTimeTrialDisplay` / `RaceFreeRunDisplay` / `RaceLicenseDisplay` | 0xA4AC / 0xA47C / 0xA69C | 0xA4B0 / 0xA480 / 0xA6A0 | RaceDisplay |
| `RaceSplitDisplay` | 0x14900 | 0x14908 | 2 RaceDisplay |
| `RaceSolitaire` | 0x21800 | 0x21800 | members after `splitDisplay` +8 |
| `RaceArcade`, `RaceArcadeSingle` | 0x34580 | 0x34780 | conductor |
| `RaceArcadeDemo` | 0x34D80 | 0x34F80 | RaceArcade |
| `RaceFreePractice` | 0x33C40 | 0x33E40 | conductor |
| `RaceSplitBattle` | 0x3EC80 | 0x3EE40 | conductor, split display |
| `RaceLinkBattle` | 0x35000 | 0x35200 | conductor, display |
| `RaceTimeTrial` | 0x48A80 | 0x48C80 | display, conductor |
| `RaceFreeRun` | 0x3A880 | 0x3AA80 | display, conductor |
| `RaceLicense` | 0x3B440 | 0x3B600 | display, conductor |
| `RaceMachineTest` | 0x48A80 | 0x48C40 | display, conductor |
| `RaceChampionship` | 0x34600 | 0x347C0 | conductor |

What was checked:

- No raw offsets. Code reaches these classes only by member names: no
  pointer arithmetic, `offsetof`, literal stride or `0x16DC` / `0x2148` /
  `0x720` literals in code. The `RaceClassInfo` sizes and every `new` /
  `alloca` use `sizeof`.
- No save data changed: `GameStatus`, `GameOptions`, `CarSaveEquipments`,
  the replay and ghost records are untouched.
- The snapshot keeps its layout. `OneLapGhostInfo` (T313) keeps 0x1B00
  bytes, because the literal `0x1B00` slot head is used in T318, T413 and
  the logger. Its `car[]` is `RACE_CAR_STATE_GT3_SIZE` (0x16DC), and
  `car_copy` / `car_restore` (T351) copy those bytes only. A restore (ghost
  restart, logger sector) therefore leaves the appended fields as they are.
  If a port needs, say, the battery charge restored with the car, keep it in
  the snapshot's `unk_14[0x10]` (check those bytes are free first) and do not
  grow the snapshot.
- Memory: race objects are stack objects of the race runners (`alloca` or
  locals; the main stack is 1 MB) and grow by at most 0x200. The race pool
  (0x22000 bytes) holds the entries: 0x28 more each. The menu heap holds
  `gt2k_menu::Car`: 0x1C more.

## 6. For wave 1: layout changes these interfaces do not cover

Some ports grow structs that other areas' headers embed. Each growth breaks
an `EGCS_SIZE` in a header the porting agent does not own, so the build
fails until the sizes are updated. The integrator runs the size probe after
the wave and fixes them; each agent says in its notes what it grew.

- **hud:** `RaceDisplay` grows with C1 (a `RaceCapacityMonitor` member),
  C5 / C6 (two fields each), C12 (0x28 per `RaceMessageDisplay`, about
  0x398 in all) and C13 (a message). The race classes and
  `RaceBasicWithRaceDisplay` / `RaceSplitDisplay` sizes follow. They are in
  race- and hud-owned headers.
- **sound (S1):** `CarSound` grows by a pan and the Pod voice channel
  (0x20). Append them at its end, not before `field_7CC` as Lupo 2 did.
  `RaceEntryCar`'s size follows (T121, race). The sound frame filled by
  `RaceEntryCar::updateSound` (T121) gains the voice block.
- **physics (8):** `TyreData` +4 would move `CarDynamics` and with it
  `RaceCarState`, `ShowRoomCar` and two stacks. Prefer GT3's unused
  `CarSpecTyre::unk_6C[4]` for the spec side, as physics.md 8 suggests, and
  a side table or spare bytes for the run side.
- **physics / race (the Pod race, M2):** the conductor's Pod fields (Lupo 2
  dc +0xF088..) and FreeRun's slots 14 / 15 (dc +0xF100) grow
  `DynamicsConductor`. All the race classes' sizes follow.
- **menus (b 14):** the `ResultArcade` / `ResultBoard` fields are in the
  menus' own folders.
- **race (M1):** the `RaceSolitaire` ghost blocks change the race classes'
  sizes, which are in the race's own headers.

Other things to know:

- The new fields are zero (or GT3's value) until a port writes them. A
  reader must not assume Lupo 2's defaults:
  - `CarModel::animSelect` defaults to -1 and `podLean` / `podBob` to 0.5 (set in the constructor by rendering, wave 1);
  - `batteryCapacity` is 0 for every car until physics sets it, so divide
    only when `hybrid` is set.
- `CarDynamics::specialKind` is cleared with the car (`car_init`). In a
  `ShowRoomCar` or a stack `CarDynamics` (`ValuateAutomobilePerformance`) it
  is whatever `CarDynamics_init` writes, so the physics port must always
  write it there.
- `RaceCarState::hybrid` is the Dualnote test. Do not test `driveType == 7`
  as Lupo 2 does: in GT3, 7 is the fixed-split 4WD (DRIVE_AWD_FIXED).
