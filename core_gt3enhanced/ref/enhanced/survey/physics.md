# Survey: car physics and dynamics

Area: `src/game/car/physics/`, `src/game/race/dynamics/` and the data path
into them (paramDB rows -> `CarEquipments::spec*` / `getSpec` (T482) ->
`SetupCarSpec` (T282) -> `CarDynamics_init` (T284), the engine / gearbox /
tyre set-up, `DynCar_initialize` (T330)). Lupo 2 addresses are `L2 0x...`,
GT3 US addresses `US 0x...`. gp: L2 0x00362B70, US 0x003579F0. Lupo 2's
physics block is at car+0xD8 and its parameter block behind a pointer at
car+0x10; GT3's block is at car+0xF84 and the parameters are embedded at
car+0x10. L2 block offsets at and above about 0x3A0 are GT3's + 4.

## Summary

### Candidates examined

- All 160 differing physics pairs and 202 differing dynamics pairs of the
  map (any difference), the 26 data-path functions of T482, the 124 + 114
  unpaired Lupo 2 functions placed in the two folders, and the 328 GT3
  functions of the area the map left without a twin. The map pairs almost
  none of the per-frame physics (Lupo 2 rewrote every wheel loop, below), so
  about 70 twins were found by hand from the call order of
  `DynamicsConductor::update`, `car_step` and `car_drivetrain_step`
  (`build/scratch/survey_physics/seeds.txt`).
- About 95 functions were read instruction by instruction against our C;
  the rest were decided from call lists, float constants (with both gp
  values) and integer constants. Every pair in the area was also screened
  for float-constant differences (`flall.py`), and the three tuning tables
  the physics reads (DynParams, InputTuning, the surface tables) were
  compared byte by byte.
- Delta review against map v2 (section "Delta review (map v2)" at the end):
  51 new or changed pairs and 110 functions the old function list lacked.
  It found entries 24 - 29 and corrected four noise verdicts and one entry
  (2) of the first pass.

### The structural change behind most "differences"

Lupo 2's physics loops over a **vehicle layout descriptor** instead of four
fixed wheels. A car's layout comes from its CarSpec "special kind" byte.
The table has four layouts: 4 wheels, a 2-wheel bike, and two trikes.

- Every per-wheel and per-axle function of the step changed shape for it,
  so the map paired almost none of them and `changed` shows 0.4 - 0.9 for
  the rest.
- For a 4-wheel car the generalised code computes what GT3 computes. The
  real changes below were separated from it by reading the code.
- No code in Lupo 2 writes the kinds that select a bike or a trike, so
  those paths are dormant (entry 1).

### Changes, in driving terms (each is an entry below)

Set-up and data:

1. **Bikes and trikes in the physics (dormant).** The whole step can run
   2- and 3-wheel vehicles: lean steering, lean camber thrust, a lean model
   in the body motion. Lupo 2 never selects them.
2. **Hard-coded car kinds.** Seven concept-car codes get a "special kind";
   the Lupo GTI Cup gets a complete built-in CarSpec. The kinds drive model
   animation (Pod, moving spoilers) and, for the two Mercedes SLs (kind 7),
   an active suspension (entry 26).
3. **Pitch inertia x1.35 for every car.** The body pitches less readily
   under the suspension forces.
4. **Rear ABS slip limit halved for every car.** The rear brakeK default
   is 0.5 (GT3 1.0), so the rear ABS steps in at half the slip.
5. **Physics tuning table retuned (DynParams):**
   - every car is 50 kg lighter (the 55 kg term is 5 kg);
   - the drive / brake pitch moment is doubled;
   - rolling resistance on surface 0 is x3;
   - worn tyres can lose more grip (the floor scale is 1.10, was 1.25);
   - the stability aid sees oversteer earlier at low speed and acts less
     at high speed.
6. **Rear ride height counts.** The rear axle uses its own ride height. GT3
   used the front's for both axles, a bug.
7. **Tyre reference grip uses the real static load.** GT3 read the sprung
   weights before computing them, a bug. This grip feeds lateral grip use,
   the aid thresholds and the AI corner speed.
8. **Three new tyre-compound flags.** Compound rows +0x53 / +0x54 / +0x55
   switch on slip-velocity tyre models and an ABS slip-limit blend. All are
   0 in the GT Concept data checked.
9. **Automatic gearbox: the 2->1 downshift point is lower** (gear 1
   averaging with the idle rpm).
10. **Starting / by-speed gear: 1st gear is held 1,500 rpm shorter** (the
    shift limit - 2000 rpm, GT3 - 500).
11. **New 4WD sub-type 6.** A fixed-split 4WD with the front axle as the
    primary (drivetrain type 9).
12. **ATTESA-type 4WD (awdType 3) retuned:**
    - twice the front-transfer gain;
    - no coupling torque limit;
    - less front transfer when the tyres are at the limit (0.15, was 0.25);
    - front axle decoupled on the overrun;
    - the front wheels' torque counts in the lock test (a GT3 quirk fixed).

Per frame:

13. **Computer cars get a cornering-grip handicap per drive class.** The
    race rules' `enemySkillC2` is applied each step as +-% on the lateral
    force and the friction circle. The AI no longer reads it as a km/h
    offset (`race_logic.md` item 8).
14. **Body roll rate limited to 4 pi rad/s** (GT3 pi/2). Roll reacts faster
    to kerbs and bumps.
15. **Understeer brake aid works with the throttle open.** It also counts
    as "aid working" for the display.
16. **Computer-car oversteer estimate is 0 below 25 km/h.**
17. **Pad steering:**
    - the loose-surface steering class applies on every course, not only
      dirt ones;
    - slower steering speeds: digital in 20 deg/s (GT3 30), back to centre
      60 (120); analog stick 20 / 60 (120 / 120);
    - a counter-steer helper exists but is switched off by the data.
18. **Steering wheel throttle pedal much more progressive.** A new response
    curve, 0.7 u^8 + 0.3 u (GT3 a quadratic).
19. **New input device 6.** Full lock always, input extrapolation, its own
    force-feedback set.
20. **Wheel force feedback reworked.** Two parameter sets chosen by the
    wheel type, a new slip / load term, centring when the computer drives.
21. **Catch-up boost capped.** A boosted car's time scale is at most x1.3.
22. **Display-only flags:** aid flags for computer cars, a steering display
    taken from the steering angle.
23. **Gearbox types and the Honda Dualnote.** Known; the GT3Hooks ports
    cover them, with the gaps listed next.

Found in the delta review (map v2):

24. **ATTESA-type 4WD with unequal tyre sizes: one radius for both axles.**
    Axle speeds are converted with the driven axle's tyre radius, so the
    front / rear speed difference measures slip, not the tyre-size mismatch.
25. **The car's condition no longer changes engine power.** No run-in loss
    for a new engine, no wear loss, no fresh-oil bonus (GT3: -3% / -2% /
    +5%).
26. **Mercedes SL500 / SL55 AMG (kind 7): active suspension.** A strong
    extra lifting force against compression beyond the static ride height
    (much less roll, dive and squat), and up to 15 mm lower from 125 to
    150 km/h.
27. **Computer cars react to a car close ahead even when not closing on it**
    (AI area). Within 4 m behind a car they take caution 0.25 (lift, look
    for a free line); GT3 reacted only while closing.
28. **Licence-type tests no longer fail on going off the course or on a
    collision** (race-logic area); computer-driven cars in them post no
    results.
29. **Timed races end at the leader** (race-logic area): after the time
    limit a car finishes only once the leader has crossed the line; GT3
    finished every car at its next crossing.

### What the GT3Hooks ports (Gearbox.c, Hybrid.c, Drive.c) miss

The ports cover the four gearbox types and the Dualnote's drive as Lupo 2
does them; `docs/gearbox_dualnote.md` section 6 already lists the HUD gauge,
the CVT menu lock and the menu power bonus as unported. What they also miss:

- **The fuel / driver mass change for every car.** The port keeps retail's
  55 kg and only drops the term for the Dualnote. Lupo 2's table holds 5 kg,
  so all Lupo 2 cars are 50 kg lighter (entry 5). In Lupo 2 a stock car
  weighs its weight + 70 kg and the Dualnote its weight + 65. With the port,
  stock cars keep weight + 120, so the Dualnote ends up 55 kg lighter than
  its rivals instead of 5 kg.
- **The rest of the DynParams retune** (entry 5).
- **The gearbox-adjacent stock changes Lupo 2 made at the same time:**
  - the gear-1 shift-point averaging (entry 9);
  - the 1st-gear threshold (entry 10), which the port deliberately resumes
    as retail;
  - the shift "callbacks" L2 0x001E2A80. These are model-animation event
    bytes (car+0x808), not physics; for the model area.
- **Drivetrain type 9 (4WD sub-type 6)** and the awdType-3 retune (entries
  11, 12). The port routes only sub-type 5.
- **Hybrid-side items that are not in the port:**
  - the per-slot rpm export L2 0x001E4658 / 0x001E4698 (a float per race
    slot at L2 0x004A9520, written every step; for sound or HUD);
  - the car+0x5F0 field the hybrid shares with the Pod's parking counter
    and the concept cars' spoiler position (no physics effect).
- **Every non-gearbox physics change above** (entries 1 - 22 and 24 - 29):
  the ports never set out to cover them. Entry 24 touches the same cars as
  entry 12; entry 26 needs the kind byte of entry 2.

### Tool pairs that are wrong (fix before using the map here)

| tool pair | what the Lupo 2 function really is | right twin |
|---|---|---|
| `CarDynamics_getTyre` = L2 0x001DB848 (calls) | the vehicle-layout descriptor getter (new) | L2 0x001DC168 |
| `car_tyre_wear` = L2 0x001E43E8 (calls) | `car_brake_heat` | wear: L2 0x001E64C0; brake heat US 0x001E6928 = L2 0x001E43E8 |
| `DynamicsConductor::isDirtCourse` = L2 0x001F7BF0 (calls) | the Pod stop-area per-frame rule, from Battle2P / SinglePlayer (race logic) | isDirtCourse is inlined as L2 0x001F0C28 (`getCourseKind() == 2`) |
| `DynamicsConductorLicense::virtual_26` = L2 0x001EE8A8 (order) | `DynamicsConductorLicense` constructor (vptr 0x00333270) | US 0x001F0AD8 |
| `DynamicsConductorLicense::virtual_37` = L2 0x001EEFD8 (order) | `DynamicsConductorFreeRun` constructor (vptr 0x003333E0) | US 0x001F1190 |
| `DynamicsConductorTimeTrial::getEachEntrantInfo_` = L2 0x001EF900 (order) | `DynamicsConductorTimeTrial` constructor (vptr 0x00333550) | US 0x001F1AD8 |
| `DynamicsConductorFreePractice::getEachEntrantInfo_` = L2 0x001EFD88 (order) | `DynamicsConductorFreePractice` constructor (vptr 0x003336C0) | US 0x001F2010 |
| `DynamicsConductor` vtable pairs past the inserted slots | Lupo 2's vtable has 44 slots (GT3 42); two slots are inserted at 14 and 15 (FreeRun's L2 0x001EF128 / 0x001EF140; delta review), so from slot 16 on Lupo 2 slot n = GT3 slot n - 2 | e.g. `chargePenaltyBySurface` US 0x001DA020 = L2 0x001DA910, `issueCrashEvent` US 0x001DA128 = L2 0x001DAA28, `F_001D9FE0` = L2 0x001DA8C8 (all unchanged) |
| map v2: `car_display_level` = L2 0x0017D4F8 (calls) | a render-side caller of it (twin unknown) | L2 0x001DE818 |
| map v2: `getGapDistance` / `getGapLaps` / `getRunState` = L2 0x001FD188 / 0x001FD1A8 / 0x001FD1C8 (order) | the slipstream table setter / getter / clear | `dc_set_slip` US 0x001EBE98 / `dc_get_slip` US 0x001EBEB8 / `dc_clear_slip` US 0x001EBED8 |
| map v2: `DynamicsConductorLicense::virtual_36` = L2 0x001EEFB0 | slot 36 = GT3 slot 34 (the inserted slots are FreeRun's / the base's 14 and 15) | `DynamicsConductorLicense::virtual_34` US 0x001F10B8 |

Further twins the map lacks (all checked by hand, used below):

- Per-step functions:
  - `car_pre_step` L2 0x001E9768
  - `car_input_update` L2 0x001FA690
  - `car_wheel_steer_angles` L2 0x001E6DC8
  - `car_tyre_slip_ratios` / `tyre_slip_ratio` L2 0x001E4F00 / 0x001E4BA8
  - `car_rolling_resistance` L2 0x001E6820
  - `car_tyre_force_limits` / `car_tyre_long_forces` / `car_tyre_lat_forces`
    L2 0x001E4FE8 / 0x001E5098 / 0x001E5B78
  - `car_long_accel` L2 0x001E5738
  - `car_wheel_torques` / `car_active_yaw_control` L2 0x001FB3B8 / 0x001FB490
  - `car_axle_differentials` L2 0x001FC558
  - `car_yaw_lateral` / `car_integrate_velocity` L2 0x001E5F78 / 0x001E6308
- Assists:
  - `car_assists` / `_own` / `_ai` L2 0x001F8AB0 / 0x001F89F8 / 0x001F8A60
  - `car_traction_control` L2 0x001FEE30
  - `car_brake_aids` L2 0x001F82C8
  - `tcs_brakes` L2 0x001F7FA8
  - `car_slip_estimate` L2 0x001F8658
- Gearbox:
  - `car_select_gear` L2 0x001FB2A8
  - `car_shift_auto` / `_semi_auto` / `_manual` / `_sequential` L2 0x001FAB50 /
    0x001FB050 / 0x001FB140 / 0x001FB210
  - `car_update_gear_offset` L2 0x001FAA88
- Input:
  - `input_tuning_init` L2 0x001F8E38
  - `car_steer_toward` / `approach` / `car_get_steer_assist` L2 0x001F9578 /
    0x001F9530 / 0x001F96B8
- Set-up:
  - `DynCar_initialize` / `_place` / `_resetState` / `_initTyreWear` L2
    0x001F4F20 / 0x001F4590 / 0x001F49A8 / 0x001F4430
  - `car_update_pit` L2 0x001FD8B0
  - `DynamicsConductor::countLap` L2 0x001ECBA0
  - `steer_force_init` L2 0x001F6990

The pairs the map does have for this area check out (`gear_calc`,
`drivetrain_layout`, `car_drive_torque`, `car_engine_speed`, the car boxes,
`Gadget`).

`conductor_load_car_boxes` (L2 0x001FCD08) is still called, by a tail jump
from `collideAllCars` / `collideWalls`. An older tool build made it look
uncalled; it is not a change.

### For other areas

- **Car model / Pod**:
  - the Pod's emotion state machine L2 0x001E1520 and its parking counter
    L2 0x001E1C88 / 0x001E1BC8, run from the physics loop;
  - the Pod's parking squat (L2 0x001E1D00 lowers each wheel's static
    stroke);
  - the concept cars' speed-driven moving part (kinds 2, 8, 9, 10: deploys
    above 80 km/h, retracts below 60 km/h; L2 0x001E1FD0 -> car+0x5F0);
  - the model animation channels (car+0x80C floats, car+0x808 event bytes,
    L2 0x001E2988 / 0x001E2AA0 / 0x001E2A80, fed from `car_view_update` and
    `car_input_update`);
  - the per-car animation object at car+0x828 (L2 0x00209558 init,
    0x0020A668 -> 0x00209810 step, from `DynamicsConductor::initialize` /
    `update`). It reads the car's speed, brake, view position and model
    event bytes and writes only itself; a 0x68-byte float parameter block
    (accessors L2 0x00208D28..0x00209074, defaults L2 0x002090A8, global
    block L2 0x002DF178) is also read by HUD-area code L2 0x001B00D8;
  - `car_dirt` (L2 0x001EAFC8) returns 0: no dirt on the car models (GT3
    shows the condition's dirt except on course kind 1);
  - Pod helpers L2 0x001E12F8..0x001E1CF0 (emotions, crash reactions,
    model offsets, sound byte, "sat down" test): known from `docs/pod`.
- **Sound**: `car_sound_engine_levels` L2 0x001DE458 has new engine-level
  formulas:
  - the on-throttle level 0.4 + 0.6 (1 - (1 - x)^16);
  - the off-throttle level centred on the engine's real load share instead
    of 0.375.

  The per-slot rpm export L2 0x004A9520 (written by L2 0x001E4658 /
  0x001E4698) is listed under the ports above.
- **Effects**: new `LeafParticle` (L2 0x0019CD68 / 0x0019D248, falling
  leaves with gravity 9.81 and flutter) and `SparkParticle` classes, linked
  next to the rigid-body file.
- **Race logic / modes**:
  - the new `DynamicsConductorTraining` (44 slots, a licence-like conductor);
  - L2 0x001F7BF0, the Pod stop-area per-frame rule (`race_logic.md`;
    from Battle2P and SinglePlayer);
  - `licenseFinished` now posts a lap difference;
  - `attach` resets `RaceResult`;
  - `AutomobileControlRecord` load / save slots copy inline (noise);
  - the per-frame AI call L2 0x001FE8A0: when the race rules have +0x64
    set and +0x68 == 0xEF, every unfinished car is counted (+0x46B), a
    pad-driven one is handed to the computer (driver 2), and
    `ai_find_corner` runs for it;
  - licence-type rules (entry 28): `DynamicsConductorLicense` slots 34 / 35
    / 37 return 0 (GT3 3 / 2 / !isDirtCourse), `licenseCheckStop` L2
    0x001FE738 clears a collision fault instead of failing (reason 4) and
    calls `car_sector_end` only when `isDataLoggerSupported` (slot 4) says
    so, `licenseFinished` /
    `licenseFailed` L2 0x001FE518 / 0x001FE468 skip computer-driven cars;
  - timed races end at the leader (entry 29, L2 0x001EDB18 =
    `DynamicsConductor::virtual_17`);
  - `DynamicsConductorFreeRun` new slots 14 / 15 (L2 0x001EF128 /
    0x001EF140, get / set dc+0xF100[i]); the Pod stop-area mode set-up L2
    0x001F75C8, now called from `DynamicsConductor::virtual_20` (L2
    0x001F0C78), which also builds the kind-7 table of entry 26.
- **AI**: `car_pair_contact` (L2 0x001FD0A0) lost its closing test, so
  `ai_note_closing` also gets cars that are not closing (entry 27); with
  `race_logic.md` item 2's thresholds that gives caution 0.25 within 4 m.
- **HUD**: the per-slot rpm export getter L2 0x001E4728 (from
  `RaceDisplay::virtual_2`).
- **AI**: the AI corner-speed estimate uses `TyreData.grip`, so it changes
  with entry 7. The AI's yaw-rate easing (`race_logic.md` item 7) reads the
  new mean front slip (L2 block+0x424) that `car_tyre_lat_forces` now
  writes.
- **Input**: input device type 6 (entry 19).

---

## Entries

### 1. Vehicle layouts: bikes and trikes in the physics (dormant)

| Field | |
|---|---|
| What | The step runs vehicles with 2 or 3 wheels. A layout row gives per wheel: axle, side, mirror wheel, lateral / longitudinal sign, steered flag; per axle: wheel count. Layout 1 (a bike, 1 wheel per axle) has its own physics: lean steering from the stick (`car_process_input` branch for kind 4, with lean rate, 50 km/h, 6/pi gains), lean camber thrust (`car_tyre_lat_forces`, 30 deg / 0.2), lean in `car_body_motion` (gravity / centrifugal moment, 80, 0.8, 10), a bike camber law in `car_suspension_forces` / `car_wheel_grip`, special wheelbase / caster / tyre sizes for kinds 3 / 4 (`CarDynamics_init`, `tyre_dimensions`). For a 4-wheel car every generalised function computes what GT3's does. |
| Where | descriptor table L2 0x002DE690 (rows of 0x54: 0 = 4 wheels / 2 axles, 1 = 2 wheels, 2 = 1 front + 2 rear, 3 = 2 front + 1 rear); getters L2 0x001DB848 (params) / 0x001DB870 (car) / 0x001DB820 (by index); kind -> layout L2 0x001DB7D0 (kind 4 -> 1, 5 -> 2, 6 -> 3, else 0), kind 3 / 4 test L2 0x001E24D8; params+0x30 = layout, params+0x32 = kind (L2 0x001DC7A4..0x001DC7B0). Touches every per-wheel function of T296..T302, T340, T342, T344, T356, T357. |
| Kind | new function or class / changed logic |
| Evidence | The descriptor's +1 (wheel count) bounds every L2 wheel loop. The kind comes only from `SetupCarSpec`'s car-code list (entry 2), which never writes 3..6, and a scan of the whole L2 image finds no other store of 3..6 to +0x11. So no Lupo 2 vehicle reaches the bike / trike paths. |
| Port | A large rewrite of every wheel loop. It needs the layout table, a layout byte in CarDynamics, and the bike model (a rider, models) to be useful. |
| Verdict | do not port now (dormant in Lupo 2; nothing selects it). Keep GT3's fixed 4-wheel loops; port only the behaviour changes below. |

### 2. Hard-coded car kinds and the Lupo GTI Cup spec

| Field | |
|---|---|
| What | `SetupCarSpec` writes CarSpec+0x11 ("special kind") from the car code: `sl500_02` / `sl55_amg_02` 7, `pod_tmv_01` 1, `r35_gtr_tmv_01` 2, `crossfire_dmv_02` / `crossfire_04` 8, `cien_dmv_02` 9, `w12_tmv_01` 10. For `lupo_gti_cup_03` it overwrites the whole CarSpec (0x3E2 bytes) with a built-in one: FF, 6 gears, 1010 kg. The kinds select model animation (Pod 1; moving spoiler 2/8/9/10) and the bike/trike layouts (3..6, unused). Kind 7 has physics: with flag L2 0x0035DFD0 = 1 (as shipped) an active suspension (entry 26; the first pass missed it); with the flag 0 it would instead zero the drive/brake pitch moment. |
| Where | L2 0x001DAC18 (US 0x001DA308 `SetupCarSpec`): name list L2 0x001DACDC..0x001DADB8, strings L2 0x00348D40..0x00348DD8, spec copy L2 0x001DB0E4..0x001DB1B0 from L2 0x002DE150. |
| Kind | new data or table / changed logic |
| Evidence | `nameToCode` x9 and the stores to spec+0x11 in the disassembly; the static block reads layout 1 at +0xB2, 6 gears at +0xC8, weight 1010 at +0x2A. |
| Port | The kind byte is needed by the model / Pod side and by entry 26. The Lupo Cup block is data for one car that GT3 does not have. |
| Verdict | port the kind byte only together with the Pod / concept-car / SL models; the Lupo Cup spec only if that car is transplanted. |

### 3. Pitch inertia x1.35

| Field | |
|---|---|
| What | Every car's pitch inertia is 1.35 x its yaw inertia (GT3: equal). The body pitches more slowly under the suspension and drive forces: `pitchFront` / `pitchRear` = arm x 2 pi / inertia, 26% smaller. It also lowers the drive / brake pitch-moment gain `unk_130` (entry 5 doubles that gain). |
| Where | `SetupCarSpec` L2 0x001DAE0C..0x001DAE44 (spec+0x2F = spec+0x2F x 135 / 100); read by `CarDynamics_init` L2 0x001DD3BC. GT3: `getSpec` sets pitch = yaw, `SetupCarSpec` leaves it. |
| Kind | changed logic |
| Evidence | `sll 4; addu; sll 3; subu` (x135), `div 100`, `sb 0x2F`. |
| Port | One line in `SetupCarSpec` (u8, truncating; wraps only above 188, i.e. 18,800 kg m^2). |
| Verdict | port (with entry 5's pitch gain, which assumes it). |

### 4. Rear ABS slip limit halved (rear brakeK default 50)

| Field | |
|---|---|
| What | No part writes CarSpec `brakeK[]`, so it always takes `SetupCarSpec`'s default. Lupo 2's default is front 100 / rear 50, GT3's 100 / 100. brakeK x 0.01 scales each axle's ABS slip limit, so the rear wheels' ABS now steps in at half the slip: more stable, earlier-releasing rear braking with ABS on. |
| Where | L2 0x001DAF74..0x001DAF84 (`li 0x32; sb 0x113`); US 0x001DA308 (both 100). Used by `tcs_brakes` (L2 0x001F7FA8 / US 0x001E2D08). |
| Kind | changed logic (a default) |
| Evidence | The two defaults side by side in the disassembly. No `sb` to spec+0x112 / +0x113 in any L2 part function. |
| Port | One constant. |
| Verdict | port. |

### 5. DynParams (physics tuning bytes) retuned

| Field | |
|---|---|
| What | Six values of the table differ. (a) +0x04 (GT3 "fuelMass") 55 -> 5: every car's mass is weight + 65 + 5, 50 kg lighter: quicker everywhere, less load on the tyres. (b) +0x07 50 -> 100: the gain of the pitch moment the drive and brake forces put on the body (`unk_130`, `car_drive_pitch_heave`) doubles: more squat and dive. (c) +0x2C `rollingResistance[0]` 10 -> 30 (x0.01): rolling resistance on surface 0 is x3. INFERRED: surface 0 is the ordinary road, so this means more drag at speed, slower coasting and a lower `CarDynamics_calcTopSpeed` estimate. (d) +0x8F..0x91 `compoundLoad` 125 -> 110: the wear grip floor is scale / grip, so a worn tyre can fall to about 76% of new grip for a 145% compound (GT3 about 86%). (e) +0xAD `oversteerAngleLo` 20 -> 7 deg: the pad driver's oversteer is detected from 7 deg rear slip at standstill (GT3 20). (f) +0xB0 `oversteerGainHi` 140 -> 60: at high speed the oversteer value rises less than half as fast. (e) and (f) drive the stability brake aid and the ABS release. |
| Where | L2 0x002DEB60 vs US 0x002D28B0 (`g_002D28B0`, T325). Readers: `CarDynamics_init` (mass, `unk_130`, compoundLoad), `surface_tables_init` -> `car_rolling_resistance` / `CarDynamics_calcTopSpeed`, `car_slip_estimate`. |
| Kind | new data or table (values) |
| Evidence | Byte compare of the 0xBC bytes: only +0x04, +0x07, +0x2C, +0x8F..0x91, +0xAD, +0xB0 differ. The L2 readers use the same offsets (e.g. L2 0x001DD548 `lbu 3(..)`, 0x001DD568 `lbu 4(..)`, 0x001DD69C `lbu 7(..)`). |
| Port | Change the initialiser in T325. The GT3Hooks Dualnote mass hook (`Hyb_MassThunk`) then has to agree: with +0x04 = 5 for every car, the Dualnote, which leaves the term out, is weight + 65. |
| Verdict | port. It is the bulk of Lupo 2's handling retune; values only. |

### 6. Rear axle ride height used (GT3 bug fixed)

| Field | |
|---|---|
| What | The axle height (static stroke base, centre of gravity, stroke limits) of the rear axle comes from `rideHeight[1]`. GT3 uses `rideHeight[0]` for both axles, so a car's rear ride-height setting (setting menu, part data) had no effect on the rear suspension geometry or the CG. |
| Where | `CarDynamics_init` L2 0x001DD4C4 (`lbu 0x5D(spec)`) vs US 0x001DBFD8 (`axleHeight[1] = rideHeight[0]`, T284). |
| Kind | changed logic (fix) |
| Evidence | L2 reads spec+0x5C into params+0x40 and spec+0x5D into +0x44; our matched C reads +0x5C twice. |
| Port | One field in `CarDynamics_init`. Note: this changes every car whose two ride heights differ. |
| Verdict | port. |

### 7. Tyre reference grip computed at the real static load (GT3 bug fixed)

| Field | |
|---|---|
| What | `TyreData.grip` (the grip at rest x the load curve at the static load per tyre width) is computed after the mass and the suspension. GT3 computes it inside the tyre loop, before `SuspParam_init` and the mass section, so it reads `sprungWeight[]` / `unsprungWeight` before they are set (0 on a fresh block, else the previous car's). `TyreData.grip` normalises the lateral grip use (`car_yaw_lateral`: which feeds the 4WD coupling and the TCS fade), the understeer / oversteer estimate of the aids (`car_slip_estimate`), and the AI corner-speed estimate (`ai_corner_speed`). |
| Where | L2 0x001DD718..0x001DD7AC (loop over the 6 sets, `tyre_load_per_width` L2 0x001E9440, `curveLookupLinear`) vs US 0x001DBFD8 (inside the set loop, T284 lines 415-423). |
| Kind | changed logic (fix) |
| Evidence | Call order in the two disassemblies: GT3 calls `tyre_load_per_width` right after `TyreData_initLoadCurve`; Lupo 2 calls it after `SuspParam_init` and the mass. |
| Port | Move the block in `CarDynamics_init`. |
| Verdict | port. |

### 8. New tyre-compound flags (paramDB TIRE_COMPOUND +0x53 / +0x54 / +0x55)

| Field | |
|---|---|
| What | Three new compound bytes travel row +0x53/+0x54/+0x55 -> CarSpecTyre +0x6C/+0x6D/+0x6E -> TyreData +0x1B4/+0x1B5/+0x1B6 (TyreData grows 0x1B4 -> 0x1B8). **A (+0x1B4 == 1)**: the longitudinal slip is (ground speed - tread speed) / 13.89 m/s, a slip velocity, clamped to +-1 with no low-speed fade (`tyre_slip_ratio`). The ABS slip limit is blended toward the second slip curve read at the wheel's lateral-force use: 40% front, 70% rear, only when lower (`car_brake_aids`). **B (+0x1B5 != 0)**: the lateral force curve is read at the lateral velocity x 0.1131 instead of the slip angle, with no low-speed factor (`car_tyre_lat_forces`). **C (+0x1B6 == 1)**: the lateral force has no friction circle with the drag (drag 0, lateral clamped to forceMax) and flips sign when reversing (`car_tyre_lat_forces`); `car_long_accel` uses the steering angle instead of the travel direction for its resistance. With all three 0, every function computes GT3's result. New outputs every car gets: a per-wheel byte +0x27, the lateral force use x 128; the block's mean front folded slip (L2 block+0x424). |
| Where | `specTire` L2 0x00242108 (US 0x002333A0) 0x00242380..0x00242394; `CarDynamics_init` L2 0x001DCACC..0x001DCAE8; `tyre_slip_ratio` L2 0x001E4BDC; `car_brake_aids` L2 0x001F83B0..0x001F8430; `car_tyre_lat_forces` L2 0x001E5C7C..0x001E5E78; `car_long_accel` L2 0x001E58EC. |
| Kind | new data or table / changed logic |
| Evidence | The `lbu 0x1B4/0x1B5/0x1B6` tests listed; the 0x1B8 stride in `CarDynamics_getTyre` L2 0x001DC168. The related GT Concept database `GTDataSQLiteConverter/gt3paramDB/gtc/paramdb_bs.db` (39 compounds, rows 0x58) has all three bytes 0; Lupo 2's own `paramdb_bs.db` (inside its ISO) was not opened. |
| Port | TyreData +4 bytes (or a side table), CarSpecTyre bytes (unused +0x6C..), `specTire`, and the branches in five functions. GT3's compound rows are 0x58 bytes: +0x53..0x55 exist and are 0 in GT3's 17 compounds. Adding them changes nothing until a compound sets them. |
| Verdict | port the plumbing (a no-op for all GT3 data). The per-wheel lateral-use byte and the mean front slip are wanted by `race_logic.md`'s AI yaw easing anyway. |

### 9. Automatic gearbox: lower 2->1 downshift point

| Field | |
|---|---|
| What | The shift table entry for gear 1->2 (`shiftRpm[0]`, the rpm in 2nd below which the automatic goes back to 1st; also where the gear chart's 2nd-gear line starts) is the average of the idle rpm and GT3's value, minus the 10 km/h margin, at least idle + 500. The automatic (player AT, computer cars) drops into 1st much later. Higher gears unchanged. |
| Where | `gear_calc` L2 0x001E3060 at 0x001E3898..0x001E38B4 (`bne a2,1` -> `(T2C + rpm) / 2`) vs US 0x001E1C98 (T292 shift-point loop). |
| Kind | changed logic |
| Evidence | Disassembly; `docs/gearbox_dualnote.md` sec. 8 lists it as a known Lupo 2 revision outside the gearbox types. |
| Port | Three lines in `gear_calc`. |
| Verdict | port. |

### 10. 1st gear's speed threshold: shift limit - 2000 rpm

| Field | |
|---|---|
| What | `car_gear_for_speed` (the gear a speed calls for: AT "by speed" mode, semi-auto from reverse, the rolling start's starting gear, the AI) treats 1st as too short above the speed at (shift limit - 2000) rpm. GT3 uses - 500 for every gear. So 2nd is chosen from a lower speed. Types 1 / 3 (gearbox) give 1. |
| Where | L2 0x001FA960 (0x001FAA00 `li 0x7D0`, 0x001FAA08 `li 0x1F4`) vs US 0x001E4E80. |
| Kind | changed logic |
| Evidence | Disassembly; `gear-behaviour.md` VERIFIER note on finding 15. |
| Port | One condition. The GT3Hooks P6 thunk resumes retail's search on purpose. |
| Verdict | port (in the C, not the hook). |

### 11. New 4WD sub-type 6: front-primary fixed split (drivetrain type 9)

| Field | |
|---|---|
| What | DRIVETRAIN +0x14 = 2 with +0x15 = 6 decodes to type 9. It is driven like retail's fixed split (Lupo 2 type 8), but with the front axle as the driven axle, so awdSplit x the drive goes to the rear. Lupo 2 renumbers GT3's type 7 to 8 and gives 7 to the hybrid. All bounded switches in L2 take 10 types (traction control, shift-up wheel check, drive bits, split table L2 0x00349200 / 0x00349230: 9 shares 8's case). |
| Where | `drivetrain_layout` L2 0x001E3CE0 (table L2 0x00348E80 slot 6 -> 0x001E3EF0: type 9, axle 0) vs US 0x001E2700 (slot 6 a no-op). |
| Kind | new function or class / changed logic |
| Evidence | Jump tables dumped; `docs/gearbox/dualnote-drivetrain.md` finding 4 records it and it is not ported. |
| Port | A new drive type in GT3's numbering (8, since GT3 keeps 7 = fixed split) through every switch on `driveType` (T292, T296, T340, T343, T344, T357, T330 `centreLocked`). The data needs DRIVETRAIN rows with sub-type 6. |
| Verdict | port if a car that uses it is transplanted; otherwise optional. |

### 12. ATTESA-type 4WD (awdType 3, DRIVE_AWD_SPEED) retuned

| Field | |
|---|---|
| What | (a) `split` = awdSplit x 0.36 x 2.0: twice GT3's front-transfer gain per unit of axle speed difference. (b) `awdTorque` = 0: GT3 used `awd_param(15) x 98`, now no coupling torque limit, which also skips GT3's "locked" cases. (c) The coupling factor falls from 1 at 25% lateral grip use to 0.15 at 100%: less front drive at the limit (GT3 0.25). (d) On the overrun (negative drive, sign flipped in reverse) the centre coupling returns "free" (-98000): the front axle is decoupled and engine braking goes through the rear only. (e) The lock test adds the front wheels' torque; GT3 ignored it, see our "Quirk kept" comment. Drives like a more rear-biased, more aggressive GT-R. |
| Where | `drivetrain_layout` L2 0x001E3DB8..0x001E3E54 (bytes L2 gp-0x4B90 = 200, -0x4B8F = 0, -0x4B88 = 25, -0x4B80 = 15, written to the floats gp-0x4B8C / -0x4B84); `car_centre_coupling` L2 0x001FB6E8 vs US 0x001E5998; flag byte L2 0x0035DFE2 = 1; `car_drive_torque` L2 0x001FBE88..0x001FBF98 (handling of -98000). |
| Kind | changed logic / new data |
| Evidence | Constants 0.25 / 0.15 / -98000 only in L2. The .sdata bytes at L2 0x0035DFE0.. are read and scaled by the decoder. |
| Port | `drivetrain_layout`, `car_centre_coupling` and the -98000 case in `car_drive_torque`. The constants can be plain (no runtime bytes needed). |
| Verdict | port. It changes every GT3 car with awdType 3 (Skyline GT-R family): decide deliberately, it is a handling retune of popular cars. |

### 13. Computer cars: cornering-grip handicap per drive class

| Field | |
|---|---|
| What | Each step, for a car driven by the computer (driver AUTO / COMPUTER, or the computer has the pad car), every wheel's lateral force factor (the low-speed factor that scales lateral and drag force) and friction-circle limit are multiplied by 1 + rules.enemySkillC2[driveClass] / 100. enemySkillC2 is the race record's signed byte +0x1B0; GT3 read it as an AI corner-speed offset in km/h, which Lupo 2's AI no longer reads (`race_logic.md` item 8). Positive values make computer cars corner harder, negative softer. |
| Where | `car_drivetrain_step` L2 0x001E68E8, block 0x001E6918..0x001E69D0 (dc+0xEC80 = rules, +0x18 + params driveClass, x gp-0x7364 = 0.01) vs US 0x001E8598 (absent). |
| Kind | changed logic |
| Evidence | `lb -0x1368(at)` from dc + 0xEC98 + params[0x31]; dc+0xEC80 is the rules block (`dc_setup_slow_car_boost` L2 0x001EBD78 reads +0x24..+0x2A from it, matching GT3's RaceRuleOptions). |
| Port | A loop in `car_drivetrain_step` between the force limits and the long forces. It needs the AI area to drop GT3's km/h use of the same byte, as Lupo 2 did. |
| Verdict | port together with race_logic item 8. |

### 14. Body roll rate limit 4 pi rad/s

| Field | |
|---|---|
| What | The body roll rate is clamped to +-4 pi rad/s (GT3 +-pi/2); roll itself still +-pi/4. Hard roll inputs (kerbs, landing, sharp turn-in) are no longer rate-limited, so the body rolls faster. Pitch clamps are unchanged. |
| Where | `car_body_motion` L2 0x001E8188 at 0x001E8630 (gp-0x733C = 12.566) vs US 0x001E96F0. |
| Kind | changed logic (a constant) |
| Evidence | Disassembly; the rest of the 4-wheel path matches GT3 once generalisation is set aside. |
| Port | One constant. |
| Verdict | port. |

### 15. Understeer brake aid with the throttle; aid flags

| Field | |
|---|---|
| What | The ASM's second set (brakeOn2: brake the inner wheels against understeer) works with the throttle open; GT3 only with the throttle closed. It now counts as "working" (brakeAssist 2). The aid flag the HUD shows (brakeAssist 1) is also set when only the understeer brake is fitted. The oversteer brake and the two ABS releases are unchanged; they now go through a modifier block applied after the computation (L2 0x001F7DC0 / 0x001F7E30), same result. |
| Where | `tcs_brakes` L2 0x001F7FA8 (flag tests 0x001F8034 / 0x001F8098, gp-0x4AAF = L2 0x0035E0C1 = 1, gp-0x4AB0 = 1, gp-0x4AAE = 1) vs US 0x001E2D08; `car_brake_aids` L2 0x001F82C8 vs US 0x001E3068; L2 0x001F7F60 (flag test). |
| Kind | changed logic |
| Evidence | Disassembly; flags read from L2 0x0035E0C0 = 01 01 01. |
| Port | Drop the throttle test in `tcs_brakes`; set `working`; extend `tcs_brake_or_abs`. |
| Verdict | port. |

### 16. Computer-car oversteer estimate off below 25 km/h

| Field | |
|---|---|
| What | For computer drivers (and the computer driving the pad car) the oversteer value (excess of speed x yaw rate over the lateral acceleration) is 0 below 25 km/h. It feeds the computer's own ASM / ABS release. The pad driver's estimate is unchanged in code (its constants change, entry 5). |
| Where | `car_slip_estimate` L2 0x001F8658 at 0x001F8958 (6.944 m/s) vs US 0x001E3708. |
| Kind | changed logic |
| Evidence | Constant only in L2; the rest matches. |
| Port | One test. |
| Verdict | port. |

### 17. Pad steering: surface class everywhere, slower steering speeds, counter-steer (off)

| Field | |
|---|---|
| What | (a) For pads the steering class is always the front wheels' surface class (the lowest class among the steered wheels): loose ground gives the dirt lock curve and speeds even on tarmac courses. GT3 used it only on dirt courses and needed both front wheels on the same class. (b) Steering speeds (deg/s, class 0 tarmac / 1 loose / 2 course kind 1): digital steer-in 20 / 90 / 20 (GT3 30 / 120 / 30), back to centre 60 / 120 / 60 (GT3 120 / 120 / 120); analog stick steer-in 20 / 90 / 20, back 60 / 150 / 60 (GT3 120 everywhere). The bytes are now plain deg/s, no x10. (c) `car_steer_toward` takes the front wheels' travel angle and a third speed: with gain InputTuning+0x25 enabled by +0x24 it would spring the steering toward the slide (counter-steer help); +0x24 is 0, so it is off and the third speed is never used. (d) The steering-wheel and stick lock logic is unchanged. GT3's `st->unk_420 = 1` store is no longer in this function. |
| Where | `car_process_input` L2 0x001F9CC0 -> new car-steering function L2 0x001F9710 (class choice 0x001F98D8; device 6 at 0x001F98B8 / 0x001F9A7C), `car_steer_toward` L2 0x001F9578, `input_tuning_init` L2 0x001F8E38 (tables L2 0x004A9C48..0x004A9C98, InputTuning L2 0x002DED48 +0x13..+0x1A, +0x26..+0x29) vs US 0x001E4478 / 0x001E4368 / 0x001E3EA0, InputTuning US 0x002D4628. |
| Kind | changed logic / new data |
| Evidence | Disassembly. InputTuning was reorganised (new layout), so the values above are computed from each build's own init code. |
| Port | The new InputTuning layout or just the new values in GT3's; the class choice; `car_steer_toward`'s signature if the counter-steer is wanted. |
| Verdict | port (a) and (b): they define Lupo 2's pad feel. (c) only as dormant code. |

### 18. Steering-wheel throttle response curve

| Field | |
|---|---|
| What | The USB wheel's throttle pedal goes through a new response mode 3, y = c u^8 + (1 - c) u with c = 0.7. That is 15% torque at half pedal (GT3: mode 1 quadratic, c = 0.5, 37.5%). The brake curve is unchanged (mode 1, 50). Mode 2 (sqrt) is rescaled, c = 1 + (100 - n) / 100, and mode 1 accepts n = 0. Neither is used by the default data. |
| Where | `responseInit` L2 0x001FE968 / `responseEval` L2 0x001FEB00 vs US 0x001D91D8 / 0x001D92E0; InputTuning L2 +0x3C / +0x3D = 3 / 70 (GT3 +0x2C / +0x2D = 1 / 50). |
| Kind | changed logic / new data |
| Evidence | Disassembly of both modes. |
| Port | Mode 3 in T354, the tuning bytes. |
| Verdict | port. |

### 19. Input device type 6

| Field | |
|---|---|
| What | A new controller type 6 (raw input `device`). Its steering lock is always the car's full lock, with no lock-follows-slide and no speed curve. Its raw steering is extrapolated: input + 2 x (input - last input), InputTuning +0x7A / +0x7B, mode 1, gain 2. It has its own force-feedback parameter set (entry 20). The ordinary wheel (type 4) has the same extrapolation code, but its bytes +0x5A / +0x5B are 0. |
| Where | `car_input_update` L2 0x001FA690 (0x001FA71C / 0x001FA73C), car steering L2 0x001F9710 vs US 0x001E4D38 / 0x001E4478. |
| Kind | changed logic |
| Evidence | Disassembly. INFERRED: device 6 is a newer wheel (a port class in engine/input). |
| Port | Needs the input area's device. |
| Verdict | port only with the input area's new device. |

### 20. Steering-wheel force feedback reworked

| Field | |
|---|---|
| What | Two parameter sets of 0x20 bytes (L2 0x002DED94) are expanded into 0x40-byte runtime blocks (L2 0x004A9BA8). The set is chosen by the wheel's type: controller+0x14 >> 5 == 4 selects set 1. Each set has its own gains, response curve and a new term per steered wheel: the lateral force x clamp(k0 + k1 (1 - k2 |slip angle|) + k3 load share, +-2). While the computer drives, the wheel is centred from the input position instead (car+0x7FC). GT3: one set, lateral force through a curve plus rate, angle and slip terms and four filters. INFERRED for the exact meaning of each new term. |
| Where | `steer_force_init` L2 0x001F6990 / `steer_force_update` L2 0x001F6C10 (called directly by `DynamicsConductor::virtual_20` and `input_tuning_init`) vs US 0x001DFF40 / 0x001E0068. |
| Kind | changed logic / new data |
| Evidence | Disassembly (loop of 2 in the init, set selection at the head of the update). |
| Port | A self-contained rewrite of T336 plus the bytes. |
| Verdict | port (wheel users); low priority. |

### 21. Catch-up boost capped at x1.3

| Field | |
|---|---|
| What | The slow-car boost (a car behind the reference runs its dynamics faster) is capped at 1.3 x before the rules' master volume. GT3 has no cap, so a large `slowEnemyBoostMax` made rubber-band cars visibly fast. |
| Where | `DynamicsConductor::virtual_3` L2 0x001EC080 at 0x001EC14C (gp-0x72E8 = 1.3) vs US 0x001ECEB8 (T304). |
| Kind | changed logic |
| Evidence | Constant only in L2; the rest matches. |
| Port | One clamp. |
| Verdict | port. |

### 22. Display-only changes in the physics files

| Field | |
|---|---|
| What | (a) `car_assists`: computer cars also set `tractionAssist` / `brakeAssist` (GT3 only for the computer on a pad car). (b) The steering display byte (block+0x527): the human player's comes from the raw input as in GT3; for a computer-driven car it is the steering angle (100 = 15 deg, L2 0x001F96C0), smoothed (prev + 2 new) / 3. (c) Per-step rpm export per race slot (L2 0x004A9520). |
| Where | L2 0x001F8AB0, 0x001F96C0, 0x001FA690 tail, 0x001E4698 / 0x001E4658. |
| Kind | changed logic (display) |
| Evidence | Disassembly. |
| Port | Small; for the HUD area. |
| Verdict | port with the HUD work if it shows these values; no physics effect. |

### 23. Gearbox types and the Honda Dualnote (known)

| Field | |
|---|---|
| What | As documented in `FINDINGS.md` sections 11 / 14, `docs/gearbox/*.md` and `docs/gearbox_dualnote.md`. Checked again here: the 33 type tests, the type-3 motor law, the hybrid battery / motors, `drive_class` (sub-type 5 -> class 0, L2 0x001FF928), the mass term and the rear-only routing (table L2 0x00348E10). |
| Where | as in those documents |
| Kind | changed logic / new function or class |
| Evidence | — |
| Port | GT3Hooks ports exist; in the C tree they become ordinary code. What they miss is listed in the summary. |
| Verdict | port (as C, from the ports' specs), plus the missing items. |

### Changed but without effect

- **`getSpec` no longer writes CarSpec+0x0F = 9.** L2 0x00241310 vs US
  0x002325B8. Nothing reads +0x0F in either build. Do not port.
- **`tyre_dimensions`: a 0 front tyre width counts as 10 mm**, as the rear
  always did. L2 0x001E3BC0. Only for broken data; port if convenient.
- **`car_rolling_resistance` / `car_long_accel` / `car_suspension_forces`
  drop GT3's always-zero `unk_4D0` term.** No effect.

---

## Noise (looked at, no behaviour change)

**Generalised to the layout descriptor; same result for a 4-wheel car**
(constants identical):

- Steering, tyres, drivetrain step: `car_pre_step`, `car_wheel_grip`,
  `car_wheel_low_speed_factors`, `car_wheel_steer_angles`,
  `car_tyre_slip_ratios`, `car_brake_torques` (plus the known type-3 state),
  `car_rolling_resistance`, `car_tyre_force_limits`, `car_tyre_long_forces`,
  `car_wheel_torques`, `car_active_yaw_control`, `car_axle_differentials`
  (bar entry 24), `car_wheel_spin` (bar entry 24),
  `car_yaw_lateral`, `car_integrate_velocity`, `car_engine_speed_from_wheels`,
  `car_driven_axle_speed`, `car_update_rpm`.
- Suspension, body and ground: `car_ground_contact`, `car_susp_prepare`,
  `susp_wheel_force` (bar kind 7, entry 26), `susp_antiroll_force`, `car_wheel_gaps`,
  `car_wheel_ground_bits`, `car_wheel_heights`, `car_wheel_planar_offsets`,
  `car_ground_penetration`, `car_ground_attitude`, `car_drive_pitch_heave`,
  `car_wheel_speed_factors`.
- Wear, heat and view: `car_tyre_wear`, `car_brake_heat`, `car_tyre_check`,
  `car_wheel_shake`, `car_view_update` (bar the animation channels),
  `car_save_prev`.
- Set-up: `SuspParam_initStroke`, `DynCar_place`, `DynCar_resetState`,
  `DynCar_initTyreWear`, `DynCarState_resetWheels`,
  `CarDynamics_initTopSpeed` / `CarDynamics_calcTopSpeed` (bar the gearbox
  types), `DynCar_setInitialSpeed` (bar type 1 and entry 24).
- Gearbox and race: `car_select_gear`, `car_shift_auto` /
  `_semi_auto` / `_manual` / `_sequential` (10 drive types, same rules),
  `car_update_gear_offset`, `car_stopped_in_box`, `car_update_pit`
  (`car_race_done` inlined).

**Register allocation, scheduling, strength reduction, 1-ulp constants:**

- Data path and set-up: all `CarEquipments::spec*` except `specGear` /
  `specTire` (`specRacingModify` converts floats unsigned; `specEngine` x10
  by shifts); `SuspParam_init` (979.99988); `engine_setup` (716.19995);
  `engine_update` (587.99994); `turbo_setup`, `turbo_boost_stage` /
  `_sum` / `_factor`, `engine_torque_at` (its condition factor is entry
  25), `engine_power_limit`,
  `engine_spec_sheet`; `TyreData_initSlipCurves` / `_initAngleCurves` /
  `_initLoadCurve`, `damper_unpack`, `damper_fit`, `curve_setup_kmh_deg`;
  `gear_auto_apply`, `gear_auto_ratios`, `gear_ranges`, `gear_auto_ranges`,
  `gear_chart_fill` / `_with_sheet`, `diff_code_to_index`;
  `surface_tables_init`, `slipstream_init`, `car_slipstream_update`,
  `car_time_scale`, `DynCar_tickMessage`, `DynCar_setMessage`,
  `CarCondition::*`, `ConditionModel::*` bar `powerFactor` (entry 25),
  `WearCurve::*`.
- Per frame: `clutch_slip_factor` (52.35987 / 837.75793); `slip_angle_fold`,
  `slip_grip_factor`; `car_drive_torque` (bar the gearbox / hybrid / entries
  11-12; 97999.99 = 98000 1-ulp; L2 0x001FB8F0 is an "any wheel braked"
  helper); `car_update_gear_offset`; `car_wear_grip`.
- Rigid body (`Gadget::*`, `RigidBodyManager::*`, `rb_*`): `Gadget::step`'s
  0.6756 / -0.1756 are GT3's integrator steps folded at compile time.
- Car boxes (`car_box_*`, `boxes_*`), `conductor_store_car_boxes` /
  `conductor_load_car_boxes` (still called, by tail jumps), `wrap_angle`,
  `clamp_yaw_rate`.
- `drive_class` (bar sub-type 5), `getDriveParam`, `dc_setup_slow_car_boost`,
  `boost_factor` / `slow_factor`, `RaceRuleOptions::setFromRace`,
  `chargePenaltyBySurface` (new vtable slots only), `F_001D9FE0`,
  `CarPairStates::*`, `dc_car_pairs`, `car_slipstream`, `car_near_code`
  (`car_pair_contact` is entry 27; the first pass had it here).
- Dynamics-conductor accessors: Lupo 2 emits linkonce copies (`Get*`,
  `CurrentTotalTime`, ...); GT3 inlines them.
- The DC subclass virtual stubs (bar `DynamicsConductorLicense` slots 34 /
  35 / 37, entry 28), `getSpecialStart`, `CurrentLapTime`,
  `getEachEntrantInfo_` (vtable shift, see wrong pairs),
  `AutomobileControlRecord::*` (`loadSlot` / `saveSlot` copy inline instead
  of `memcpy`), lap / gate / sector timing (`countLap`, `recordGate`,
  `reportGateTimes`, `checkLapRecord`, `restoreSector`): structure and
  constants as GT3. For lap-logic detail see `race_logic.md`.
- `car_input_pad`, `car_input_ai_apply`, `car_steering_display`,
  `approach`, `car_get_steer_assist`, `car_mode_*`, `pad_vibration_*`,
  `car_attitude_lag_*`.

Scratch (scripts, extended map, seeds, notes):
`build/scratch/survey_physics/`.

---

## Delta review (map v2)

Source: `build/scratch/delta/physics.txt` (51 "pair" lines, 110 "new?"
lines). Each was read against its GT3 twin (`ddiff3.py`, `newq.py` in the
scratch folder). Verdicts: **change** (a full entry below), **noise**,
**wrong pair**, **new code**, **elsewhere** (a change for another area).
New pair knowledge is in `build/scratch/survey_physics/pairs_fix.csv`.

### Pairs

| L2 | map v2 pair | verdict |
|---|---|---|
| 0x0017D4F8 | `car_display_level` | wrong pair: a render-side caller of it (calls L2 0x001DE818, a virtual, L2 0x0017D450; twin unknown). `car_display_level` is L2 0x001DE818 (same tests; block offsets +4, and the field behind the race rules' +0x3C pointer at +0x226E0 vs +0x217A4). |
| 0x001DBFE0 | `SuspParam_initStroke` | noise: the axle's sprung weight is divided by the layout's wheels per axle (2 for a car; entry 1) instead of x 0.5; CarDynamics offsets +0x18. |
| 0x001E4518 | `car_brake_heat_sq` | noise (offsets). |
| 0x001E6F00 | `ground_is_off_course` | noise (table address). |
| 0x001E7880 | `car_hold_lift` | noise (the five zeroed fields are GT3's + 4). |
| 0x001E9E18 | `car_wheel_spin` | change: entry 24 (common tyre radius); otherwise the layout loops of entry 1. |
| 0x001EA040, 0x001EA090 | `car_speed_view`, `car_set_condition` | noise (offsets). |
| 0x001EA4A8 | `car_set_brake` | noise (stores the brake, then tail-calls `car_set_wheel_brakes`; GT3 the other order; independent fields). |
| 0x001EAE40..0x001EAF30 (6) | `car_ground_view_x/y/z`, `car_ground_pitch/roll`, `car_yaw2` | noise (interpolation getters, offsets). |
| 0x001EB2C8, 0x001EB310, 0x001EB358, 0x001EB5B0, 0x001EB608 | `car_wheel_steer/camber/rotation`, `car_wheel_shake_at`, `car_steer_view` | noise (offsets). |
| 0x001EB3D0, 0x001EB448, 0x001EB4C8 | `car_contact_u/v`, `car_wheel_pos` | noise for cars: the wheel index first goes through the layout's display map (new L2 0x001EB3A0, row +0x4C; identity 0, 1, 2, 3 for 4 wheels; entry 1). |
| 0x001EC1C0 | `DynamicsConductor::getOrder` | noise (offsets). |
| 0x001ED3E0 | `DynamicsConductor::updateCar` | noise (conductor offsets, the lap-distance virtual at L2 slot 29 = GT3 27, `car_update_pit` by tail jump). |
| 0x001ED858, 0x001ED888 | `lapTime`, `lapTimeIfStarted` | noise (tail call; both reached by `j`, added as "start" rows). |
| 0x001EDA90 | `DynamicsConductor::virtual_15` | noise (tail call to `dc_post_section_time`). |
| 0x001EDB18 | `DynamicsConductor::virtual_17` | elsewhere: entry 29 (timed races end at the leader) and the Pod lap check L2 0x001F7B20 before `dc_post_lap_time` (`race_logic.md`, Pod mode). |
| 0x001EE468, 0x001EF988 | `MachineTest::` / `TimeTrial::CurrentLapTime` | noise (tail calls). |
| 0x001EE8E0 | `DynamicsConductorLicense::initialize` | noise. |
| 0x001EF398 | `FreeRun::recordOneLapGhostControl` | noise (the ghost object's slot is one later). |
| 0x001EF9B8 | `TimeTrial::disqualify` | noise (tail call to `car_finish`). |
| 0x001EFEC8 | `FreePractice::virtual_1x` | noise (offsets +4). |
| 0x001F03B0 | `SinglePlayer` constructor | noise (the copied 0x40-byte default block is byte-identical). |
| 0x001F0920, 0x001F0B28 | `hasFinishLine`, `hasPitLane` | noise (conductor slots +2, offsets). |
| 0x001F4988 | `DynCar_setSpeed` | noise. |
| 0x001F55B8 | `DynCar_setMessage` | noise (table address; 18 messages vs 16, see `race_logic.md`). |
| 0x001F55F0 | `CourseEnv_initSurfaceSmoke` | noise (the 11-row smoke table is byte-identical). |
| 0x001F8D88..0x001F8E28 (4) | `car_mode_stop_roll/stop_brake/stop/still` | noise (the last two reached by a jump table, added as "start" rows). |
| 0x001F96B0 | `car_set_steer_assist` | noise. |
| 0x001FD188, 0x001FD1A8, 0x001FD1C8 | `getGapDistance`, `getGapLaps`, `getRunState` | wrong pair: `dc_set_slip` / `dc_get_slip` / `dc_clear_slip` (US 0x001EBE98 / 0x001EBEB8 / 0x001EBED8; the old map had the last two right). No change. |
| 0x001FE518 | `DynamicsConductor::licenseFinished` | elsewhere: entry 28 (skips computer-driven cars); the lap-difference post is known. |
| 0x001FE738 | `DynamicsConductor::licenseCheckStop` | elsewhere: entry 28 (collision fault cleared, sector end behind `isDataLoggerSupported`). Its callees L2 0x001FE468 = `licenseFailed` (entry 28) and L2 0x001FF248 = `car_stopped_in_box` (noise: all the layout's wheels in the box, 4 for a car). |

### Functions the old list lacked

| L2 | verdict |
|---|---|
| 0x001DB3C8 | change: `ConditionModel::powerFactor`, now a stub returning 1.0 (entry 25). |
| 0x001DE818 | noise: `car_display_level` (see the pairs). |
| 0x001E02A0..0x001E07E4 (11) | new code, known: the Dualnote hybrid's battery accessors, flow meter, regen / assist, throttle map, socFactor, motorDemand (`docs/gearbox/dualnote-hybrid.md`; the ports). |
| 0x001E12F8..0x001E1CF0 (13) | new code, known: Pod emotions, crash reactions (from `car_report_crash`), model offsets (from `RaceCarModel::updateBodyState`), sound byte (from `RaceEntryCar::updateSound`), "sat down" test (`docs/pod`, `race_logic.md`). |
| 0x001E2150, 0x001E2280 | change: the kind-7 active suspension's table builder (from `DynamicsConductor::virtual_20`) and force (from `susp_wheel_force`) (entry 26). |
| 0x001E23A0 (and 0x001E2378), 0x001E24F0 | new code: kind 3 / 4 geometry rows and test (entry 1, dormant). |
| 0x001E28C0 | new code: model animation event byte get (car+0x808), read by the car+0x828 object (model area). |
| 0x001E2EA0 | noise: `diff_code_to_index`; with it `diff_codes_to_index` L2 0x001E2EF0 (the list's start at 0x001E2F0C is a loop label inside it) and `diff_codes_to_letters` L2 0x001E2F38. |
| 0x001E3000 | new code, known: driven axles per drive type for `gear_calc` (table L2 0x00348E10, entry 23). |
| 0x001E4338 | noise: `brake_heat_setup` (DynParams +0x7C..0x7F = 07 14 64 05 in both). |
| 0x001E4728 | new code: the per-slot rpm export's getter for the HUD (`RaceDisplay::virtual_2`). |
| 0x001EAFC8 | elsewhere (model): `car_dirt` returns 0, so no dirt is drawn on the cars. |
| 0x001EE0B8, 0x001EE360 | noise: `Battle2P` / `BattleMP::virtual_29` (L2 slot 31). |
| 0x001EE998 | new code: a licence helper (byte != 0). |
| 0x001EEFB8, 0x001EEFC0, 0x001EEFC8 | elsewhere: `DynamicsConductorLicense` slots 35 / 36 / 37 (L2 37..39) return 0 / 1 / 0, GT3 2 / 1 / !isDirtCourse; with slot 34 (L2 0x001EEFB0, 0, GT3 3) entry 28. |
| 0x001EF128, 0x001EF140 | new code: the two inserted `DynamicsConductor` slots 14 / 15 (FreeRun: get / set dc+0xF100[i]); race logic. |
| 0x001F0900, 0x001F1070 | noise: `DynamicsConductor::virtual_30`, `getSpecialStart`. |
| 0x001F6BF0 | new code: the per-slot vibration / force-feedback record (dc+0xC0A8 + slot x 8) used by `pad_vibration_*`, `steer_force_update`, `DynCar_resetState`; the storage behind entries 19 / 20, no change by itself. |
| 0x001FA950 | noise: a revLimit accessor (GT3 reads it inline). |
| 0x001FCE60, 0x001FD1F0, 0x001FD260 | noise: `dc_set_near`, `car_slipstream`, `car_near_code` (with `dc_car_pairs` = L2 0x001FD378, same call sequence). |
| 0x001FD0A0 | change (AI area): `car_pair_contact` without its closing test (entry 27). |
| 0x00208D28..0x00209074 (53) | new code (model / HUD area): the float accessors of the car+0x828 object's 0x68-byte parameter block, used by its init L2 0x00209558, its step L2 0x00209810 / 0x0020A818 and HUD-area code L2 0x001B00D8. |
| 0x002090A0..0x002091C8 (5) | new code: the same object's constant 0.4, parameter defaults (0.25, 0.8, 0.1, 0.75, 0.5, 0.6, 0.5, 0), parameter load, global block L2 0x002DF178 and static constructor. |

Also from the review: `DynamicsConductor::virtual_20` is L2 0x001F0C78 (GT3
US 0x001F2FE0); besides GT3's calls it builds the kind-7 table (entry 26)
and sets up the Pod stop-area mode (L2 0x001F75C8, `race_logic.md`).

### 24. ATTESA-type 4WD with unequal tyre radii: one radius for both axles

| Field | |
|---|---|
| What | `gear_calc` sets GearCalc +0x01 (unused in GT3) to 1 when the drive type is 5 (`DRIVE_AWD_SPEED`, awdType 3) and the front and rear tyre radii differ. With it set, every conversion between an axle's angular speed and its wheels' tread speed uses the driven axle's (GearCalc.drivenAxle) tyre radius instead of the axle's own: the axle speed from the wheels (`car_wheel_spin`), the wheel spin from the axle speed (`car_axle_differentials`, both branches) and the initial axle speeds (`DynCar_setInitialSpeed`). Both axle speeds are then on one scale, so the front / rear speed difference that drives the speed-sensing front transfer (entry 12) and the centre coupling measures wheel slip only. In GT3 a car with different front and rear tyre sizes shows a permanent axle speed difference at a steady speed, which the coupling reads as slip. |
| Where | `gear_calc` L2 0x001E3574..0x001E3598 (US 0x001E1C98 never writes +0x01); `car_wheel_spin` L2 0x001E9F80..0x001E9FF0 vs US 0x001EA720; `car_axle_differentials` L2 0x001FC8B8, 0x001FC98C vs US 0x001E6488; `DynCar_setInitialSpeed` L2 0x001F4EC0..0x001F4EDC vs US 0x001DD8A8. |
| Kind | changed logic (bug fix) |
| Evidence | Disassembly: `lbu 1(GearCalc)`, then `lbu 3(GearCalc)` selects `tyreRadius` / `tyreRadiusInv`; GT3 indexes by the axle. Only drive type 5 with unequal radii sets the flag. |
| Port | The flag in `gear_calc` and the index choice in the three functions (a few lines each). |
| Verdict | port with entry 12 (same cars). Only awdType-3 cars with unequal front / rear tyre radii change; which GT3 cars those are was not checked. |

### 25. The car's condition no longer changes engine power

| Field | |
|---|---|
| What | `ConditionModel::powerFactor` returns 1.0, so `engine_torque_at` no longer scales the torque curve by the car's engine and oil mileage. In GT3 (DynParams cond[] = 30, 30, 50, 10, 20, 20, 50, 10, 30; mileage in metres): a new engine lacks 3%, recovering over its first 300 km; a worn engine loses up to 2% (from 500 to 600 km); fresh oil adds 5% for 200 km, gone after 300 km. `engine_torque_at` feeds the per-step torque (`car_pre_step`), the top speed and the spec sheet, so Lupo 2 drives and shows every car at its nominal power. Mileage is still counted. |
| Where | L2 0x001DB3C8 (`lui 0x3F80; jr`) in the place of US 0x001DAD28 (between `ConditionModel::distance` L2 0x001DB3B8 and the unit's static constructor L2 0x001DB3D8); caller `engine_torque_at` L2 0x001DED10 vs US 0x001E0BC0. |
| Kind | changed logic (feature removed) |
| Evidence | Disassembly; `engine_torque_at` is GT3's only caller of `powerFactor` (T291). |
| Port | One line. |
| Verdict | do not port by default: GT3's run-in and oil change belong to its garage, which GT Concept lacks. Offer it as an option if the enhanced engine drops the garage. |

### 26. Mercedes SL500 / SL55 AMG (kind 7): active suspension

| Field | |
|---|---|
| What | For CarSpec kind 7 (`sl500_02`, `sl55_amg_02`, entry 2) while flag byte L2 0x0035DFD0 is 1 (as shipped), `susp_wheel_force` adds a force from L2 0x001E2280 to each wheel's damper force. Let x = stroke - static stroke + b, b = 0 below 125 km/h, rising to 15 mm at 150 km/h. (a) When x < 0 (compressed beyond the static position), a lifting force (0.5 / 0.75) x min(1, -x / 25 mm) x the wheel's share of its axle's sprung weight: up to 2/3 of the wheel's static load at 25 mm. (b) Always - spring x b: the car settles up to 15 mm lower at speed. Like Active Body Control: far less roll, dive and squat, lower at speed. GT3 has neither the kind nor the force. |
| Where | `susp_wheel_force` L2 0x001E7530..0x001E7564 vs US 0x001E8E58; force L2 0x001E2280; table L2 0x002DE950 (0x1C bytes) built by L2 0x001E2150 (from `DynamicsConductor::virtual_20` L2 0x001F0C78) from L2 0x0035DFD0 = 01 19 32 4B 4B 0F 7D 96 (on, 25 mm, 0.50, front 0.75, rear 0.75, 15 mm, 125 km/h, 150 km/h). |
| Kind | new function or class / new data |
| Evidence | Disassembly; the only readers of the table are its builder and the force. The flag byte is 1 in the image, so the branch runs (entry 2's first-pass "no effect" covered only the pitch-moment branch, which runs when the flag is 0). |
| Port | The kind byte (entry 2), the table (constants) and the force in `susp_wheel_force`. |
| Verdict | port with the SL500 / SL55 AMG transplants only; no GT3 car has kind 7. |

### 27. Computer cars react to a car close ahead even when not closing (AI area)

| Field | |
|---|---|
| What | `car_pair_contact` (from `dc_car_pairs`, for pairs within 20 m) still returns 0 for a pad-driven car under player control, a car not ahead with a gap, or one outside the mean width, but no longer returns 0 when the relative speed shows the cars separating. So `ai_note_closing` is also called with closing <= 0. With Lupo 2's thresholds (`race_logic.md` item 2) that sets caution 0.25 whenever the gap is under 4 m: a computer car sitting on another car's bumper lifts and looks for a free line even when it is not catching it. GT3 gave no caution unless closing. |
| Where | L2 0x001FD0A0 vs US 0x001EBDA0 (GT3's `0 <= relSpeed -> return 0` at US 0x001EBE74..0x001EBE80 has no Lupo 2 counterpart); caller L2 0x001FD378 (`dc_car_pairs`, same call sequence as US 0x001EC098); `ai_note_closing` L2 0x002007A8. |
| Kind | changed logic |
| Evidence | Disassembly: after the relative-speed loop Lupo 2 stores gap / -relSpeed and returns 1 unconditionally. |
| Port | One condition (remove the closing test), together with `race_logic.md` item 2. |
| Verdict | port with the AI changes (it only matters with Lupo 2's caution thresholds). |

### 28. Licence-type tests: no fail on going off the course or on a collision (race-logic area)

| Field | |
|---|---|
| What | For `DynamicsConductorLicense` (licence tests; also the conductor of the new `RaceTraining`): (a) all four wheels off the course no longer count as a course-out fault (slot 37 returns 0; GT3 1 unless on dirt); the counted-ground rule inside the pit lane (slot 36 = 1) stays. (b) A collision fault (a wall hit above 10 m/s of speed change, or a course object) is cleared instead of failing the test with reason 4. (c) Computer-driven cars (driver 2) are skipped: `licenseFailed` does nothing for them, `licenseFinished` finishes them but posts no result. (d) `car_sector_end` runs only when `isDataLoggerSupported`. Slots 34 / 35 return 0 (GT3 3 / 2): slot 34 only feeds `car_tyre_check`'s "== 4" test and slot 35 has no caller, so no effect. |
| Where | L2 0x001EEFB0..0x001EEFC8 (slots 36..39) vs US 0x001F10B8..0x001F10D0 (slots 34..37); `licenseCheckStop` L2 0x001FE738 (0x001FE780..0x001FE790) vs US 0x001EF780; `licenseFailed` L2 0x001FE468 vs US 0x001EF520; `licenseFinished` L2 0x001FE518 vs US 0x001EF5C0. |
| Kind | changed logic |
| Evidence | Disassembly; vtable slots compared with the 2-slot shift. |
| Port | Constants in four slot functions and two conditions. |
| Verdict | for the race-logic area to decide: it makes GT3's licence tests far more lenient, so keep GT3's rules unless the Lupo 2 test set (or `RaceTraining`) is brought over. |

### 29. Timed races end at the leader (race-logic area)

| Field | |
|---|---|
| What | When a car crosses the line, `DynamicsConductor::virtual_17` finishes it if the leader has finished, or it has run the race's laps, or the time limit (minutes x 60000) has run out. Lupo 2 adds to the last case: and this car is the leader (`getOrder(0)`). So after the time runs out the leader takes the flag first and the others finish at their next crossing; in GT3 every car finished at its first crossing after the limit, lapped cars included. GT3 sets a time limit only for the arcade demo as far as known (`race_logic.md` C1). |
| Where | L2 0x001EDBE4..0x001EDC1C vs US 0x001EFC28..0x001EFC48. |
| Kind | changed logic |
| Evidence | Disassembly: the second `getOrder(0)` call and `xor / movz` on the car index exist only in Lupo 2. |
| Port | One condition. |
| Verdict | port with the race-logic changes if timed races are added. |
