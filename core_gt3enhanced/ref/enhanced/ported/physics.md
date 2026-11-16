# Port notes: physics (wave 1)

Area: `src/game/car/physics`, `src/game/race/dynamics`,
`src/game/data/T482_car_equipments.cpp`, their headers in `include/`, and
`include/game/enhanced/physics.h`. Survey: `survey/physics.md` (entries 1-29,
"What the GT3Hooks ports miss", the Delta review), PLAN.md's physics section,
INTERFACES.md sections 1-5.

No new files. Changed sources: T167, T282, T284, T285, T292, T293, T296, T298,
T302, T325, T330, T331, T336, T340, T341, T342, T343, T344, T356, T358, T375,
T376 (`car/physics`), T304, T308, T309, T317, T349, T361 (`race/dynamics`),
T482 (`data`); internal headers `car_box_internal.h`, `car_input_internal.h`,
`car_param_internal.h`, `drivetrain_internal.h`, `suspension_internal.h`,
`control_record_internal.h`; headers T292, T296, T298, T302, T331, T335, T338,
T341, T344, T347 (`include/game/car/physics`), T482 (`include/game/data`),
`include/game/enhanced/physics.h`.

Every changed file compiles with `tools/mcc.py` (objects in `build/physics/`),
and again with every `ENH_PHYSICS_*` switch at 0 (a copy of physics.h with the
1s turned to 0s put first on the include path). A preprocessor check confirmed
that every file with an `#if ENH_PHYSICS_*` really sees the switch (its
preprocessed text differs between the two settings), and that T285 sees
`ENH_SOUND_MIX_TABLE` and T296 `ENH_AI_SKILL_AS_GRIP`.
`python tools/sizeprobe.py --only-diff`: "entries 732, differing 0".
No struct of this area grew (see "Layouts" below).

## Switches (`include/game/enhanced/physics.h`)

`1` = Lupo 2 (the default), `0` = GT3: with every switch at 0 the code
computes what GT3 does. One switch was added (`ENH_PHYSICS_AWD_COUPLING`,
entry 30, found during the port); none was renamed or dropped.

| Switch | Entry | 1 (Lupo 2) | 0 (GT3) |
|---|---|---|---|
| `ENH_PHYSICS_PITCH_INERTIA` | 3 | pitch inertia = yaw inertia x 1.35 | = yaw inertia |
| `ENH_PHYSICS_REAR_ABS` | 4 | rear brakeK default 50 | 100 |
| `ENH_PHYSICS_DYNPARAMS` | 5 | Lupo 2's six DynParams values | GT3's |
| `ENH_PHYSICS_REAR_RIDE_HEIGHT` | 6 | the rear axle at rideHeight[1] | rideHeight[0] for both |
| `ENH_PHYSICS_TYRE_REF_GRIP` | 7 | reference grip at the real static load | at the weights not yet set |
| `ENH_PHYSICS_AUTO_DOWNSHIFT` | 9 | 2->1 point averaged with the idle rpm | GT3's point |
| `ENH_PHYSICS_FIRST_GEAR_THRESHOLD` | 10 | 1st up to revLimit - 2000 | - 500 |
| `ENH_PHYSICS_ATTESA` | 12 | awdType 3: gain x2, no torque limit, 0.15 at the limit, front free on the overrun, front torque in the lock test | GT3's |
| `ENH_PHYSICS_ROLL_RATE` | 14 | roll rate within +-4 pi rad/s | +-pi/2 |
| `ENH_PHYSICS_UNDERSTEER_BRAKE` | 15 | understeer brake with the throttle open, counts as working, shows the aid | throttle closed only |
| `ENH_PHYSICS_AI_OVERSTEER` | 16 | computer cars' oversteer 0 below 25 km/h | always |
| `ENH_PHYSICS_PAD_STEERING` | 17 a, b | pads: the front wheels' lowest surface class on every course; 20 / 90 in, 60 / 120 back (digital), 20 / 90 in, 60 / 150 back (stick), deg/s | dirt courses only, both wheels alike; 30 / 120 in, 120 back; stick 120 both ways |
| `ENH_PHYSICS_WHEEL_THROTTLE` | 18 | the wheel's throttle through 0.7 u^8 + 0.3 u | quadratic, c 0.5 |
| `ENH_PHYSICS_FORCE_FEEDBACK` | 20 | two parameter sets (DFP / the rest), centring while the computer drives, always sent | one set, not sent while the computer drives |
| `ENH_PHYSICS_CATCHUP_CAP` | 21 | the slow-car boost's time scale at most x1.3 | no cap |
| `ENH_PHYSICS_DISPLAY_FLAGS` | 22 a, b | computer cars set their aid flags; the steering display byte from the steering angle (pads' digital / stick, computer-driven cars smoothed) | GT3's |
| `ENH_PHYSICS_ATTESA_RADII` | 24 | awdType 3 with unequal tyre radii: both axles on the driven axle's radius | each its own |
| `ENH_PHYSICS_PAIR_CONTACT` | 27 | `car_pair_contact` without the closing test | with it |
| `ENH_PHYSICS_TIMED_END_AT_LEADER` | 29 | after the time limit only the leader is flagged, the others at their next crossing | every car at its next crossing |
| `ENH_PHYSICS_AWD_COUPLING` | 30 (new) | the coupled 4WD types (`DRIVE_AWD_SPLIT` / `_SPEED` / `_STEER` / `_FIXED`): the coupling torque flows from the faster axle to the slower; a locked centre lets go when the strain passes that torque | a fixed signed share; the lock holds up to awdTorque |

Other areas' switches read here: `ENH_AI_SKILL_AS_GRIP` (race.h; physics 13
with race_logic A8: one switch), `ENH_SOUND_MIX_TABLE` (cameras_sound.h; the
T285 table the sound area asked for).

## Outcome of every entry

| Entry | Outcome | Where (file: function) | Switch |
|---|---|---|---|
| 1 bikes and trikes | **not ported**: dormant in Lupo 2 (nothing selects layouts 2..4); PLAN | - | - |
| 2 special kinds, Lupo GTI Cup spec | **ported (data)** | T282 `SetupCarSpec` (`g_carKinds`, `g_lupoGtiCupSpec`), T284 `CarDynamics_init` (copy) | - |
| 3 pitch inertia x1.35 | **ported** | T282 `SetupCarSpec` | `ENH_PHYSICS_PITCH_INERTIA` |
| 4 rear ABS brakeK 50 | **ported** | T282 `SetupCarSpec` | `ENH_PHYSICS_REAR_ABS` |
| 5 DynParams retune | **ported** (six values) | T325 `g_002D28B0` (`DYNP`) | `ENH_PHYSICS_DYNPARAMS` |
| 6 rear ride height | **ported** | T284 `CarDynamics_init` | `ENH_PHYSICS_REAR_RIDE_HEIGHT` |
| 7 tyre reference grip | **ported** | T284 `CarDynamics_init` (moved after the mass) | `ENH_PHYSICS_TYRE_REF_GRIP` |
| 8 compound flags A / B / C | **ported (data)** | T482 `specTire`, T284, T296 `tyre_slip_ratio` (A), `car_tyre_lat_forces` (B, C, latUse, frontSlip), `car_long_accel` (C), T356 `car_brake_aids` (A's ABS blend) | - |
| 9 automatic 2->1 point | **ported** | T292 `gear_calc` | `ENH_PHYSICS_AUTO_DOWNSHIFT` |
| 10 1st-gear threshold | **ported** | T343 `car_gear_for_speed` | `ENH_PHYSICS_FIRST_GEAR_THRESHOLD` |
| 11 4WD sub-type 6 | **ported (data)**: `DRIVE_AWD_FIXED_FRONT` (8) | T293 `drivetrain_layout`, T292 `gear_calc`, T296, T344 | - |
| 12 ATTESA retune | **ported** | T293 `drivetrain_layout` (gain x2, torque 0), T344 `car_centre_coupling` | `ENH_PHYSICS_ATTESA` |
| 13 grip handicap from enemySkillC2 | **ported** | T296 `car_drivetrain_step` | `ENH_AI_SKILL_AS_GRIP` (race.h) |
| 14 roll rate 4 pi | **ported** | T298 (the body's roll step) | `ENH_PHYSICS_ROLL_RATE` |
| 15 understeer brake aid | **ported** | T356 `tcs_brakes`, T340 `tcs_brake_shown` / `car_assists` | `ENH_PHYSICS_UNDERSTEER_BRAKE` |
| 16 computer oversteer < 25 km/h | **ported** | T340 `car_slip_estimate` | `ENH_PHYSICS_AI_OVERSTEER` |
| 17 a / b pad steering | **ported** | T342 `input_tuning_init`, `car_process_input` | `ENH_PHYSICS_PAD_STEERING` |
| 17 c counter-steer helper | **not ported**: its enable byte (Lupo 2 InputTuning +0x24) is 0, so Lupo 2 never runs it; porting it would change `car_steer_toward`'s signature for no behaviour | - | - |
| 18 wheel throttle curve | **ported** (the throttle); modes 1 / 2 of T354: **requested** (menus / common, below) | T342 `input_tuning_init` | `ENH_PHYSICS_WHEEL_THROTTLE` |
| 19 input device 6 (DFP) | **ported (data)**: inert until `RaceInput::readWheel` reports device 6 (race area did it; needs Lupo 2's USB IRX) | T342 `car_steer_predict`, `car_process_input`, `car_input_update` | - |
| 20 force feedback | **ported** | T336 `steer_force_init`, `steer_force_update`, `pad_vibration_send`; T342 (steerIn) | `ENH_PHYSICS_FORCE_FEEDBACK` |
| 21 catch-up cap | **ported** | T304 `DynamicsConductor::virtual_3` | `ENH_PHYSICS_CATCHUP_CAP` |
| 22 a aid flags of computer cars | **ported** | T340 `car_assists` | `ENH_PHYSICS_DISPLAY_FLAGS` |
| 22 b steering display byte | **ported** | T342 `car_steer_display`, `car_process_input`, `car_input_update` | `ENH_PHYSICS_DISPLAY_FLAGS` |
| 22 c per-slot rpm export | **ported (data)** as the per-car `displayRpm` spring (hud C2 reads it) | T296 `car_display_rpm_*` | - |
| 23 gearbox types 1 / 2 / 3, CVT | **ported (data)**, all 33 of Lupo 2's type tests | T482, T284, T292, T296, T330, T342, T343, T344, T358, T317 | - |
| 23 Honda Dualnote | **ported (data)** in full | T293, T284, T292, T296, T317, T330, T344, T361 | - |
| 24 ATTESA tyre radii | **ported** | T292 `gear_calc`, T330 (rolling start), T344 `car_drive_torque` / `car_axle_differentials` | `ENH_PHYSICS_ATTESA_RADII` |
| 25 condition without power effect | **not ported**: PLAN (GT3 keeps its garage, run-in and oil) | - | - |
| 26 SL active suspension | **ported (data)**: kind 7 only | T298 `sl_active_suspension_*`, `susp_wheel_force`; T317 `virtual_20`; T284 | - |
| 27 car_pair_contact | **ported** | T349 `car_pair_contact` | `ENH_PHYSICS_PAIR_CONTACT` |
| 28 licence tests | **not ported**: PLAN (GT3's licences keep their rules) | - | - |
| 29 timed end at the leader | **ported** | T308 `DynamicsConductor::virtual_17` | `ENH_PHYSICS_TIMED_END_AT_LEADER` |
| 30 AWD coupling direction / lock hold (new) | **ported** | T344 `car_drive_torque` | `ENH_PHYSICS_AWD_COUPLING` |
| Pod emotion machine | **ported (data)**: kind 1 only | T302 `pod_emotion_*`, `pod_crash_reaction`, `pod_sound`; T317 `update`; T167 / T375 / T376 (crash hooks); T330 | - |
| Pod parking counter | **ported (data)** | T302 `pod_parking_*`, `pod_is_seated`; T317; T330 | - |
| Pod squat | **ported (data)** | T298 `pod_squat_spring_rate` | - |
| Pod race lap check hook (race's request) | **ported** (inert outside a Pod race) | T308 / T309 `virtual_17` | - |
| moving part (wing) updater | **ported (data)**: kinds 2 / 8 / 9 / 10 | T302 `car_wing_*`; T317; T330 | - |
| model animation channels and events | **ported** (every car; rendering reads them) | T302 `car_anim_*`, T296 (shift events) | - |
| display rpm spring (hud C2) | **ported** (every car) | T296 `car_display_rpm_*`; T330 | - |
| g_002D32F0 to 18 entries | **done** | T331 | - |
| T285 mix table (sound's request) | **done** | T285 | `ENH_SOUND_MIX_TABLE` |
| `car_dirt` returning 0 | **not ported**: a visual change of GT3's own cars (no dirt); not in PLAN; rendering agrees | - | - |
| dormant Lupo 2 items | **not ported**: the kind-3 (trike) animation channel 1 from `car_input_update`; awdType >= 8 -> type 3; the transMode 3 / 6 axle-speed generalisation in `car_engine_speed` (a Lupo 2 slip, see "Uncertain") | - | - |

## The changes in driving terms

### Set-up (every car, switches)

- **3 pitch inertia.** `SetupCarSpec` gives the spec's pitch byte 135 % of the
  yaw byte (u8, truncated, as Lupo 2). The body pitches less under braking
  and acceleration and over crests; the nose dives more slowly.
- **4 rear ABS.** A car whose spec leaves the rear brakeK at 0 gets 50 (GT3
  100): the rear ABS lets the rear wheels slip half as far before releasing.
  More stable braking into corners, a little longer stops on cars that rely
  on the rear.
- **5 DynParams.** Six bytes: every car 50 kg lighter (fuel term 55 -> 5;
  cars weigh their weight + 65 + 5 kg); the drive / brake pitch moment
  doubled (more squat and dive); rolling resistance x3 on surface 0; the
  wear grip floor 125 -> 110 % (worn tyres lose more grip); the stability
  aid's oversteer angle at low speed 20 -> 7 degrees and its gain at high
  speed 14 -> 6 (it steps in earlier when slow, gentler when fast).
- **6 rear ride height.** The rear axle sits at its own ride height
  (`axleHeight[1] = rideHeight[1]`): body pitch at rest and the rear's
  geometry follow the spec. GT3 put both axles at the front's height.
- **7 reference grip.** The tyres' reference grip (`TyreData.grip`) is read
  off the load curve at the real static load per tyre, after the mass and
  the suspension are set. GT3 read it before, at a load of 0 (a fresh
  block) or the previous car's. It normalises the lateral grip use, the
  aids' thresholds and the AI's corner speeds, so these now fit each car.
- **9 automatic 2->1.** The automatic (and the AI) drops to 1st at the
  average of the old point and the idle rpm: 1st is kept for crawling only.
- **10 1st gear by speed.** The "gear for this speed" (starting gear,
  rolling starts, the by-speed automatic) holds 1st only up to the rev limit
  - 2000 rpm (GT3 - 500): 2nd is chosen earlier.
- **12 ATTESA (awdType 3).** Twice the front-transfer gain, no coupling
  torque limit, 15 % (GT3 25 %) of the transfer left with the tyres at the
  limit, the front axle decoupled on the overrun (lifting off makes the car
  rear-driven), and the front wheels' torque counted in the lock test.
  Skyline GT-R-type cars push the front harder under power and turn in more
  freely off the throttle.
- **24 ATTESA tyre radii.** With different front / rear tyre radii, the
  axle speeds, the undriven axle's inertia and the coupling torque use the
  driven axle's radius, so the coupling sees wheel slip, not the tyre-size
  mismatch (it no longer drags constantly on such cars).
- **30 AWD coupling (new).** In `car_drive_torque`, the coupling torque of
  the 4WD types now flows from the faster axle to the slower one (GT3 used a
  fixed sign), and a locked centre holds only while the strain is within
  the torque asked for (GT3: up to awdTorque). 4WD cars stop fighting
  themselves when one axle overruns the other (e.g. braking, lift-off).
- **14 roll rate.** The roll rate is clamped to 4 pi rad/s (GT3 pi/2): the
  body reacts faster to kerbs, bumps and quick direction changes.

### Per frame (switches)

- **13 grip handicap.** A computer-driven car's tyres (lateral factor and
  friction limit) are scaled by the race rules' `enemySkillC2` of its drive
  class, as a percentage. With `ENH_AI_SKILL_AS_GRIP` 1 the AI no longer
  adds it as km/h to its corner speeds (race area).
- **15 understeer brake.** The ASM's inner-wheel braking against understeer
  also works with the throttle open, and counts as "working" (the aid lamp
  lit, brakeAssist 2). A block with only that part shows the aid as armed.
- **16 computer oversteer.** Computer cars (and the computer driving the
  pad car) see no oversteer below 25 km/h: no ASM / ABS releases at
  parking speeds.
- **17 pad steering.** Pads (digital, stick, pressure buttons) use the
  front wheels' lowest surface class wherever they are: on gravel or grass
  of a tarmac course the dirt lock curve and speeds apply. Steering is
  slower: digital in 20 deg/s (GT3 30), back to centre 60 (120); on loose
  ground 90 / 120; the stick 20 in / 60 back (GT3 120 / 120), loose 90 / 150.
  Steering on a pad feels calmer and less twitchy.
- **18 wheel throttle.** A steering wheel's throttle pedal goes through
  0.7 u^8 + 0.3 u: 15 % at half pedal (GT3 37.5 %), very progressive.
- **20 force feedback.** The wheel's force comes from one of two sets: the
  Driving Force Pro's (gain 0.8, a square-root curve, no filter, no rate
  term, a quarter of the ground-normal term) or every other wheel's (GT3's
  tuning, gain 2.4 instead of 2.0). While the computer drives the car (pit
  lane, after the finish, the other cars in a 2-player race after one
  finished) the wheel is pulled back to the centre by its position, and the
  vibration block is always sent (GT3 sent nothing then).
- **21 catch-up cap.** A rubber-banded car runs at most 1.3 x normal time.
- **22 a / b display.** Computer cars light their aid flags from their own
  aid settings (the HUD shows them when viewing such a car). The steering
  indicator is the steering angle (100 = 15 degrees) for pad digital / stick
  steering (GT3: the raw button, +-100 at once) and, smoothed (old + 2 new) /
  3, for every car the computer drove this frame. The same byte turns a
  stopped car that is hit (`car_box_steer_at_rest`, T374), so that reaction
  follows the real steering too.
- **27 pair contact.** Computer cars also get the "car close ahead" call
  when not closing; with race A2's thresholds they lift and look for a free
  line within 4 m of a bumper.
- **29 timed races.** After the time limit only the leader gets the flag at
  its crossing; the others finish at their next crossing (because the leader
  has). GT3 flagged lapped cars as they crossed.

### Data-driven features (no switch; GT3's data leaves them inert)

- **2 special kinds.** `SetupCarSpec` gives a kind by car code: `sl500_02`,
  `sl55_amg_02` -> 7 (active suspension); `pod_tmv_01` -> 1 (Pod);
  `r35_gtr_tmv_01` -> 2, `crossfire_dmv_02` / `crossfire_04` -> 8,
  `cien_dmv_02` -> 9, `w12_tmv_01` -> 10 (moving wings). `lupo_gti_cup_03`
  gets Lupo 2's built-in CarSpec (0x3E2 bytes; FF, 6 gears, 1010 kg, all
  three compound flags on): the car drives on those values whatever parts it
  has. No GT3 car code is in the list (checked against paramdb_us / _eu).
- **8 compound flags** (TIRE_COMPOUND +0x53 / +0x54 / +0x55 -> CarSpecTyre
  +0x6C..0x6E -> TyreData +0x10C..0x10E, the unused 12th slip point): A the
  longitudinal slip as a slip velocity over 50 km/h, no low-speed fade, and
  the ABS limit blended toward the slip where the friction limit falls to
  the lateral use (40 % front, 70 % rear, only lower); B the lateral curve
  over the lateral velocity x 0.1131, no low-speed factor; C no friction
  circle (lateral alone, clamped, no drag), the sign by the travel when
  reversing, and `car_long_accel`'s resistance along the steering. Every car
  also gets each wheel's lateral use x 128 (`DynWheel.latUse`) and the front
  wheels' mean folded slip (`RaceCarState.frontSlip`, rad; race A7 reads it).
  All 17 GT3 compounds have the three bytes 0.
- **11 4WD sub-type 6.** DRIVETRAIN layout 2 sub-type 6: a fixed-split 4WD
  with the front axle as the primary (Lupo 2's drivetrain type 9; here
  `DRIVE_AWD_FIXED_FRONT` = 8 so GT3's 7 keeps its meaning). No GT3 row has
  it.
- **23 gearbox types** (`CarSpec.gearboxType` from the GEAR row: +0x30 of a
  widened table, rows over 0x30 bytes; for GT3's 0x30-byte rows a list of
  Lupo 2's 16 typed row codes, `g_gearboxTypeRows`; then
  `GearCalc.gearboxType`). 8 gears or more become 7; type 3 has 1 gear; a
  CVT with fewer than 2 gears is stock.
  - **Type 1 CVT.** R / D only (no neutral, no gear steps). The ratio slides
    between 1st and top: the engine is held at a target of 60 % .. 100 % of
    min(peak power rpm, rev limit - 500) by the pedal, gliding at 15 % .. 40 %
    of the ratio per second. Creeps like a torque converter: the throttle is
    at least what holds idle + 500 rpm, unless the handbrake (over 0.8) holds
    it below 1 km/h, which also opens the clutch. Top speed (the AI's) at the
    target rpm. Rolling starts hold the target rpm.
  - **Type 2 clutchless.** Stepped gears 2 and up as steps relative to 1st;
    a shift has no clutch phase, the ratio glides to the new step at 150 %
    per second (the automatic waits meanwhile); the idle is the engine's;
    the same creep and handbrake hold as the CVT.
  - **Type 3 electric single speed.** One gear, R / D selector; the motor
    law (294 Nm x the throttle less a loss with the motor speed, plus the
    engine curve as a speed servo); the tachometer is the motor's rpm; its
    own clutch (open braking at a standstill) and creep to 12 km/h; the AI
    drives it without start revving; top speed 180 km/h; gear whine on.
- **23 Honda Dualnote** (DRIVETRAIN layout 2 sub-type 5): rear-driven by the
  engine, two 20 kW front motors with torque vectoring and regeneration, a
  35 kW crank motor (IMA) assisting or charging, a 1125 kJ battery, the
  clutch-open idle-up and generator, the pedal mapped to the whole system's
  power. No fuel term in the mass (+65 kg, not +70). The rear-drive class
  for the aids and the rules (`drive_class`). Its spec sheet / gear chart
  power + 102 PS (`gear_chart_with_sheet`, rendering's request). The HUD
  gauge reads `batteryCharge / batteryCapacity`, `batteryFlow`; the sound
  reads `hybrid` and the front motors' load in `clutchTorqueAbs`.
- **26 SL active suspension** (kind 7): a lifting force against compression
  past the static ride height (up to 2/3 of the wheel's static load at
  25 mm; 75 % per axle), less roll, dive and squat; the car settles up to
  15 mm lower from 125 to 150 km/h. Table built at the race's set-up
  (`DynamicsConductor::virtual_20`).
- **The Pod** (kind 1): the emotion machine each race step (finished:
  genki / sabishii; WRONG WAY: iya; pit lane: kyoro; side by side: wink;
  full slipstream: genki; all wheels off: nemui; else neutral), crash
  reactions (wall: naku; a car by the part hit; a loose object: odoroku),
  the lean / bob animation, the voice byte (`pod_sound`). The parking
  counter: handbrake full below 1 km/h counts up to 34 (seated), each
  wheel's spring lets go in turn (the squat), and back up when released.
  Computer Pods too (the AI pulls the handbrake in the stop area). In a Pod
  race, a lap with two stops owed does not count and the Pod turns sad for
  good (`virtual_17`, the race area's rule).
- **Moving wings**: kind 2 up above 80 km/h, down below 60; kind 8 70 / 65
  mph; kind 10 120 / 80 km/h; all at 0.5 of the travel per second; kind 9
  slides with the speed from 35 to 50 mph at 2 per second. Up already at a
  rolling start's speed. `RaceCarState.wingPosition` for the model.
- **Model animation** (every car): channels 0..3 (2 the smoothed throttle,
  3 the smoothed brake), 4 / 5 the axle load share, 7 the longitudinal g, 8
  a spring after the steering; event 0 the gear shift (+1 / -1) - read by
  rendering.
- **Tachometer spring** (every car): `displayRpm` follows the rpm through a
  spring (k 1000, damping 50); the HUD shows it (hud C2).
- **19 Driving Force Pro** (device 6): the car's full lock at once (no speed
  curve, no lock following the slide, no throttle / brake curve), the raw
  input predicted ahead: input + 2 x its last step.

## What to test in game

With GT3's own data (switches at 1), compared with GT3:

- Any car: lighter (-50 kg) and a little more squat / dive under power and
  brakes; less nose pitch; rear ABS earlier; the needle of the tachometer
  smooth. Automatic: drops to 1st only near a standstill; starts in 2nd
  sooner by speed.
- A GT-R-type 4WD (awdType 3): more front pull under power, freer turn-in
  when lifting.
- With a pad: slower, smoother steering; on grass / gravel of a tarmac track
  the dirt lock and speeds. The steering indicator moves with the wheels
  (no +-100 jump on a digital tap).
- With a wheel (GT Force): softer throttle at part pedal; the force
  feedback slightly stronger; the wheel centres itself while the computer
  drives (pit lane, after the finish).
- ASM car: the understeer brake also works with throttle on; the aid lamp
  lights then.
- Races with a strong catch-up setting: no car faster than 1.3 x.
- Computer cars sitting behind another close: they lift / change lines.
- A timed race (arcade demo / race area's time limit): lapped cars are not
  flagged before the leader.
- Each switch at 0 restores GT3's behaviour of that item.

With test data (the user's modified `paramdb_us.db`: one DRIVETRAIN (2, 5)
row and a widened GEAR table with types 1 / 2 on two rows):

- Log: `[enhanced] gearbox: GEAR table N rows x 0x38 - types from row +0x30`
  (or `x 0x30 - types from the list of row codes` with GT3's table).
- A car on the type-1 row: `[enhanced] gearbox: car 0 has gearbox type 1,
  CVT`; R / D only, the revs stay near the target while accelerating, the
  car creeps forward off the brake, holds still on the handbrake.
- A car on the type-2 row: gear changes without the clutch drop, the revs
  glide between gears.
- The (2, 5) car: `[enhanced] dualnote: car 0 is a Dualnote hybrid, battery
  1125 kJ`; extra front drive, regeneration under braking, the HUD's battery
  gauge (hud C1), the whine (sound S6), +102 PS on its sheet.
- With a transplanted Lupo 2 car under one of the codes above:
  `[enhanced] kind: car N has special kind K (Pod / active suspension /
  moving wing)`; the Pod's faces / lean / squat when parked on the
  handbrake, the wing rising with speed, the SL staying flat in corners.
  A DRIVETRAIN (2, 6) row: `[enhanced] drivetrain: car N is a
  front-primary fixed-split 4WD`.

## Data / assets

- Nothing loads a file. Without Lupo 2 data every data-driven item is inert.
- Special kinds need the car code (the user's paramdb row for the
  transplanted car must carry Lupo 2's code, e.g. `pod_tmv_01`); the
  models are rendering's (SWITCH 6..10).
- Gearbox types: a widened GEAR table (rows > 0x30 bytes, the type at
  +0x30), or a row whose code is in `g_gearboxTypeRows` (Lupo 2's CVT and
  clutchless rows; add a line for any other). Type 3 needs a widened table.
- Dualnote: a DRIVETRAIN row with layout 2 and sub-type 5; the hybrid's 27
  constant bytes are built in (Lupo 2's).
- Compound flags: TIRE_COMPOUND rows +0x53..+0x55 (inside GT3's 0x58-byte
  rows; 0 in GT3).
- Lupo GTI Cup: car code `lupo_gti_cup_03` (the whole spec is built in).
- DFP: Lupo 2's `PDIUSB.IRX` (backend B1, the user's to test).

## Log lines (PCSX2 console)

- `[enhanced] gearbox: GEAR table %d rows x 0x%X - types from row +0x30 /
  the list of row codes` - once, the first gearbox read (T482).
- `[enhanced] gearbox: car %d has gearbox type %d, CVT / clutchless stepped
  / electric single speed` - each car with a type, at its set-up (T330).
- `[enhanced] dualnote: car %d is a Dualnote hybrid, battery %d kJ`.
- `[enhanced] kind: car %d has special kind %d (Pod / active suspension /
  moving wing)`.
- `[enhanced] drivetrain: car %d is a front-primary fixed-split 4WD`.
- None for GT3's cars.

## Layouts

- No struct of this area changed size. `CarSpec` uses its spare bytes
  (`specialKind` +0x011, `gearboxType` +0x0E1, the compound flags +0x6C..),
  `TyreData` the never-used 12th slip point (+0x10C..0x10F), `GearCalc`
  `gearboxType` +0x12 and `sharedRadius` +0x01, `DynWheel.latUse` +0x27,
  channel 8's spring two floats of `DynCarState.unk_3A8` (+0x3AC..+0x3B3,
  unused by GT3). The `RaceCarState` fields after GT3's 0x16DC (wave 0's,
  INTERFACES section 2) are not in the snapshot (T351 copies 0x16DC), so
  saves and the ghost / replay snapshot keep GT3's size.
- `g_002D32F0` has 18 entries (0x10 bytes more of T331's .data).

## Coordinator requests

- **rendering**: the Dualnote power bonus in `gear_chart_with_sheet` -
  done (T292: `if (drivetrain_is_hybrid(spec)) hybrid_power_bonus(sheet)`,
  the spec's layout 2 / sub-type 5 test, + (2 x 20 + 35) / 0.7355 PS).
- **cameras + sound**: the T285 mix table under `ENH_SOUND_MIX_TABLE` -
  done (rows 0 and 4 as requested). `podSound` holds the emotion until the
  next emotion step (cleared at its start, as L2 0x001E1548), so the sound
  frame after `dc->update` sees it; `pod_sound(car)` (T302, L2 0x001E19D0)
  returns it, 0 for a non-Pod. `specialKind`, `gearboxType`, `drivenAxle`,
  `hybrid` are set at the set-up; the Dualnote's front motors write
  `clutchTorqueAbs`; `engineTorque` / `frictionTorque` are set every step
  for every gearbox type (`car_pre_step`); type 3 sets `clutch` and the
  brakes as GT3's paths do. The T121.h size failure was CarSound's growth
  (RaceEntryCar 0x2190, race's); sizeprobe is clean now.
- **race**: (1) the Pod lap check at the start of both `virtual_17`s - done
  (T308, T309; the sad emotion 7 held, as Lupo 2 sets it inside its check);
  (2) `frontSlip` is the front wheels' mean folded slip angle, rad, as L2
  0x001E5B78 averages it; (3) `podParking` reaches 0x22 for any Pod with the
  handbrake at 1.0 below 1 km/h, computer cars included
  (`pod_parking_step` runs for every car after `car_input_update`); (4) the
  message table has 18 entries; (5) the steering display byte is written by
  the physics with `ENH_PHYSICS_DISPLAY_FLAGS`; (6) `car_mode_stop_roll` is
  declared in `T341_gear_mode.h` now (race_pod.cpp's own declaration is
  compatible and can go); (7) the DFP: device 6 (`INPUT_DEV_DFP`,
  car_input_internal.h) and USB type 4 (`USB_WHEEL_DFP`) select the force
  set.

## For other areas

1. **menus / common (`src/game/common/T354_response_curve.cpp`), physics
   18's other half** (INTERFACES section 4 names it physics's, but the file
   is not in this area): Lupo 2's `responseInit` (L2 0x001FE968) also
   changes mode 1 (n clamped to 0..100, n = 0 allowed, c = n x 0.01 at
   least 0.001) and mode 2 (n 0..100, c = (100 - n) x 0.01 + 1, a = -((c^2 -
   1)^2) / (4 c^2), b = -c sqrt(-a)). GT3's own curves use neither (modes 1
   with n 50, 0). Suggested under `ENH_PHYSICS_WHEEL_THROTTLE`. The force
   feedback does not depend on it: T336 builds the DFP set's mode-2 curve
   itself (`steer_force_curve`).
2. **race (`src/game/race/modes/race_pod.cpp`)**: lines 83, 166 and 186 have
   a raw newline inside a printf string (egcs accepts it; a newer compiler
   would not): `\n` is meant.
3. **sound**: `pod_sound(RaceCarState *)` (T302_car_state.h) is L2
   0x001E19D0, for the Pod voice.
4. **integrator / INTERFACES corrections**:
   - animation channel 3 is the smoothed **brake** (L2 0x001EAAC8 reads the
     block's brake), not the handbrake;
   - the Pod's squat zeroes each wheel's spring rate in turn (L2 0x001E1D00),
     it does not lower the static stroke;
   - entry 24 also scales the undriven axle's wheel torque in
     `car_drive_torque` and its inertia in `gear_calc`;
   - survey 22 b: the pads' digital / stick display byte also becomes the
     steering angle (L2 0x001F9C58), not only the computer's;
   - survey 20: the new per-wheel force term is off in both of Lupo 2's sets
     (their bytes +0x0A..+0x0D are 0), and set 1's curve is mode 2 n 100
     (y = sqrt(u) with Lupo 2's mode 2);
   - survey 17: Lupo 2's pads take the class from the surfaces only - the
     course kind no longer matters for them (classes 0 and 2 have the same
     speeds there).
5. **hud**: `displayRpm` is reset to the rpm at the set-up, so it is 0 only
   before the first car set-up; `car_display_rpm(car)` (T296) is Lupo 2's
   getter.

## Uncertain

- The car codes of the special kinds are `nameToCode` of Lupo 2's names; a
  transplanted car must use exactly these codes (rendering's mapping of the
  wing models to kinds is its own guess).
- The DFP (19 / 20) could not be tested: GT3's IOP module does not report it
  (backend B1).
- `ENH_PHYSICS_FORCE_FEEDBACK` sends the vibration block every frame, also
  for computer-driven cars and while the computer has the player's car (as
  Lupo 2); `RaceInput::sendVibration` ignores inputs without a port. A pad
  then also keeps its rumble after the finish.
- Lupo 2's `car_engine_speed` generalises the transMode 3 / 6 axle-speed
  case in a way that looks like a slip (it reads the other axle); not ported
  (GT3's kept).
- Lupo 2's rolling-start throttle preset (its car+0x9F, probably
  AIDriver.state) was not identified with certainty; not ported.
- The RaceCarState fields after 0x16DC (battery, Pod, wing, display rpm) are
  not restored by `car_restore` (the free run's one-lap ghost restart): the
  battery keeps its charge across such a restart.
