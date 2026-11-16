# GT3 enhanced engine: what Lupo 2 changed, and what is in

`core_gt3enhanced/` is the GT3 US decompilation with the game-logic updates of
Lupo 2 (GT Concept, PAPX-905.09) ported in. This report says what Lupo 2
changed, what went in, what did not and why, and how to test it. The
details behind every line: `survey/<area>.md` (what Lupo 2 does, with
addresses) and `ported/<area>.md` (what the port does, where, how to test).

## Status

- **Surveyed:** every Lupo 2 function against GT3's (9,575 of 11,767
  paired; 2,316 pairs checked by hand in `ref/lupo2/pairs_fix.csv`), in eight
  areas, each twice (a second pass after the pairing tool was improved).
- **Ported:** everything in the game logic except what GT Concept uses to
  *replace* GT3 features and what is dormant in Lupo 2 (lists below).
- **Builds** with the original compiler (builder profile "Core enhanced
  (all c)", or `tools/build.py --compiler 991111b --link full`).
- **Boots** in PCSX2, plays the intro and runs the attract mode's demo
  races (race set-up, physics, AI, HUD, cameras, sound) without errors:
  four demo races in a 12-minute unattended run, each ending at its time
  limit, the menus in between. The attract follows Lupo 2's flow (one demo
  each time the title menu times out, `ENH_MENU_ATTRACT_REPLAYS`; GT3 runs up
  to five demos back to back). With a race loaded, 2,488 KB of the heap are
  free (one block): the larger program costs about 220 KB of it. Not tested
  by hand yet: driving, the menus, the data-driven features.
- **Nothing is lost:** GT3's GT mode, licences, garage, save data and every
  menu are untouched. Save files are compatible both ways.

## How the ports are switched

Every port that changes how GT3's own cars, races or screens behave has a
switch in `include/game/enhanced/<area>.h` (77 in all): `1` = Lupo 2's
behaviour (the default), `0` = GT3's exactly. Rebuild after changing one.

Features that only act on data GT3 does not have (gearbox types, the
Dualnote, the Pod, moving wings, blinking lamps, the new tyre-compound
flags) have no switch: with GT3's own cars they do nothing. They come alive
when you transplant the Lupo 2 data and assets listed below.

## Car physics (detail)

Lupo 2 rewrote every wheel loop to run vehicles with 2, 3 or 4 wheels (bikes
and trikes, which nothing in Lupo 2 ever selects). For a car with four
wheels the rewritten code computes what GT3 computes; the real changes
are these.

### Every car (switches, `physics.h`)

| Change | In driving terms | Switch |
|---|---|---|
| Pitch inertia x1.35 | The body pitches less under braking, power and over crests; the nose dives more slowly. | `ENH_PHYSICS_PITCH_INERTIA` |
| Rear ABS limit halved (rear brakeK 50, GT3 100) | The rear ABS lets the rear wheels slip half as far: more stable braking into corners, slightly longer stops on rear-heavy cars. | `ENH_PHYSICS_REAR_ABS` |
| Tuning table (DynParams) | Every car **50 kg lighter** (the fuel/driver term 55 kg -> 5 kg); drive / brake pitch moment doubled (more squat and dive); rolling resistance x3 on surface 0 (the road); worn tyres can lose more grip (floor 125 % -> 110 %); the stability aid sees oversteer from 7 degrees instead of 20 at low speed and acts with less than half the gain at high speed. | `ENH_PHYSICS_DYNPARAMS` |
| Rear ride height used (GT3 bug) | GT3 put both axles at the front's ride height; the rear now sits at its own. Body attitude at rest and rear geometry follow the car's spec. | `ENH_PHYSICS_REAR_RIDE_HEIGHT` |
| Tyre reference grip at the real load (GT3 bug) | GT3 read the tyres' reference grip before the car's weights were set (a load of 0 or the previous car's). It scales the lateral grip use, the aids' thresholds and the AI's corner speeds, which now fit each car. | `ENH_PHYSICS_TYRE_REF_GRIP` |
| Automatic 2->1 downshift lower | The automatic (and the AI) keeps 2nd until near a standstill. | `ENH_PHYSICS_AUTO_DOWNSHIFT` |
| 1st gear by speed: rev limit - 2000 (GT3 - 500) | Starting / rolling-start gear: 2nd is chosen from a lower speed. | `ENH_PHYSICS_FIRST_GEAR_THRESHOLD` |
| ATTESA-type 4WD retuned (awdType 3) | Twice the front-transfer gain, no coupling torque limit, 15 % (was 25 %) of the transfer at the grip limit, the front axle let go when lifting off, the front torque counted in the lock test. GT-R-type cars pull harder at the front under power and turn in more freely off the throttle. | `ENH_PHYSICS_ATTESA` |
| ATTESA with unequal tyre sizes | Both axles' speeds use the driven axle's radius, so the coupling reacts to slip, not to the size difference (it no longer drags constantly on such cars). | `ENH_PHYSICS_ATTESA_RADII` |
| 4WD coupling direction (found while porting) | The coupling torque now flows from the faster axle to the slower (GT3 used a fixed sign); a locked centre lets go when the strain exceeds the torque asked for. 4WD cars stop fighting themselves when one axle overruns the other. | `ENH_PHYSICS_AWD_COUPLING` |
| Roll rate limit 4 pi rad/s (GT3 pi/2) | The body reacts faster to kerbs, bumps and quick direction changes. | `ENH_PHYSICS_ROLL_RATE` |
| Understeer brake aid with the throttle open | The ASM's inner-wheel braking against understeer also works under power, and lights the aid lamp. | `ENH_PHYSICS_UNDERSTEER_BRAKE` |
| Pad steering | The loose-surface steering class applies wherever the front wheels are (grass or gravel beside a tarmac track), not only on dirt courses; much slower steering: digital 20 deg/s in (GT3 30), 60 back (120); stick 20 in / 60 back (GT3 120 / 120). Calmer, less twitchy on a pad. | `ENH_PHYSICS_PAD_STEERING` |
| Wheel throttle curve 0.7 u^8 + 0.3 u | A steering wheel's throttle pedal is far more progressive: 15 % at half pedal (GT3 37.5 %). | `ENH_PHYSICS_WHEEL_THROTTLE` |
| Force feedback reworked | Two parameter sets (Driving Force Pro / every other wheel; the other wheels' gain 2.4, was 2.0); the wheel is pulled to centre while the computer drives (pit lane, after the finish). | `ENH_PHYSICS_FORCE_FEEDBACK` |
| Catch-up boost capped at x1.3 | A rubber-banded car runs at most 1.3 x normal time. | `ENH_PHYSICS_CATCHUP_CAP` |
| Computer cars: oversteer estimate 0 below 25 km/h | No aid releases at parking speeds for computer-driven cars. | `ENH_PHYSICS_AI_OVERSTEER` |
| Computer cars' grip handicap | The race rules' skill setting per drive class (`enemySkillC2`) becomes a +-% on the computer cars' cornering grip, instead of a km/h offset in the AI's corner speeds. One switch for both sides. | `ENH_AI_SKILL_AS_GRIP` (race.h) |
| Close-behind caution | Computer cars within 4 m behind another car lift and look for a line even when not closing. | `ENH_PHYSICS_PAIR_CONTACT` |
| Timed races end at the leader | After the time limit only the leader is flagged at its crossing, the others at their next one (GT3 flagged lapped cars first). | `ENH_PHYSICS_TIMED_END_AT_LEADER` |
| Display flags | Computer cars light their aid flags; the steering indicator follows the real steering angle. | `ENH_PHYSICS_DISPLAY_FLAGS` |

### Data-driven (no switch; inert with GT3's cars)

- **Gearbox types** (GEAR row +0x30 -> car spec -> transmission; all 33 of
  Lupo 2's type tests):
  - **1, CVT:** R / D only, the ratio slides between 1st and top to hold
    the engine at 60-100 % of its peak-power rpm by the pedal; creeps
    like an automatic; the handbrake holds it.
  - **2, clutchless stepped:** gear changes without a clutch phase, the
    ratio glides to the next step.
  - **3, electric single speed:** one gear, a motor torque law, motor rpm on
    the tachometer, creep, 180 km/h top speed, gear whine.
- **The Honda Dualnote** (DRIVETRAIN layout 2 sub-type 5), in full: engine
  on the rear, two 20 kW front motors with torque vectoring and
  regeneration, a 35 kW crank motor assisting or charging, a 1,125 kJ
  battery, the generator at idle, the pedal mapped to the whole system's
  power, its own mass rule, +102 PS on its spec sheet.
- **4WD sub-type 6:** a fixed-split 4WD with the front axle primary.
- **Mercedes SL500 / SL55 AMG active suspension** (car codes `sl500_02`,
  `sl55_amg_02`): a lifting force against compression (up to 2/3 of the
  wheel's static load at 25 mm), less roll, dive and squat; the car
  settles up to 15 mm lower from 125 to 150 km/h.
- **Tyre-compound flags** (TIRE_COMPOUND +0x53..+0x55): slip-velocity tyre
  models, an ABS limit blend, no friction circle. All 0 in GT3's and GT
  Concept's data.
- **Lupo GTI Cup** (`lupo_gti_cup_03`): Lupo 2's built-in spec.
- **The Pod** (`pod_tmv_01`): the emotion machine (genki, sabishii, iya,
  kyoro, wink, nemui, naku, odoroku, from the race situation and crashes),
  the parking counter and squat (handbrake at a standstill: the Pod sits
  down), the lean / bob animation, the voice.
- **Moving wings** (`r35_gtr_tmv_01`, `crossfire_*`, `cien_dmv_02`,
  `w12_tmv_01`): up / down with speed.
- **Driving Force Pro** (input device 6): full lock at once, input
  prediction, its own force feedback. Needs Lupo 2's `PDIUSB.IRX` (below).

### What the GT3Hooks ports missed

`Gearbox.c`, `Hybrid.c`, `Drive.c` covered the gearbox types and the
Dualnote's drive. They missed: the 50 kg mass change for every car (the
port kept 55 kg and only dropped it for the Dualnote, so the Dualnote ended
up 55 kg lighter than its rivals instead of 5), the rest of the tuning-table
retune, the automatic's downshift and 1st-gear changes, 4WD sub-type 6,
the ATTESA retune, and every other change in the table above. `Pod.c` /
`PodSound.c` missed texture animation for other cars, the menu car, the
wing switch, Lupo 2's per-race-step timing, the voice's Doppler and
priority, and stopping with the car's other sounds.

### Not ported (physics)

Bikes and trikes (dormant in Lupo 2); car condition without power effect
(Lupo 2 drops the run-in / wear / fresh-oil power changes: GT3 keeps its
garage); licence tests no longer failing on going off course or a collision
(GT3's licences keep their rules); the counter-steer helper (switched off
in Lupo 2's own data).

## Race logic and AI (`race.h`, `ported/race.md`)

- **AI, one switch each:** throttle handicap as a share of engine torque;
  much earlier caution (full from 5 km/h closing, GT3 36; half under 5 s to
  contact, GT3 3 s); fewer line changes (180 frames apart, GT3 30); longer
  look-ahead at standing starts (up to 200 m for 5 s); 5 m look-ahead when
  slow out of the pits; backing up when pointing over 60 degrees off the
  line; steering gain eased near peak front slip; the driver spread in every
  race; cool-down pace after the finish.
- **Best-lap ghost keeps the lap's car** (time trial, free run, machine
  test): car, condition and aids stored with the ghost; the ghost file format
  is unchanged.
- **Time limit ends the race in every mode** (+59 frames grace).
- **Replay afterimage** steady between interlace fields.
- **SELECT** cycles the accel / brake monitor and steering display in a live
  race; L1+L2+R1+R2 toggles the replay's mini logo.
- **Wheels:** the wheel's centring spring follows the race state (off while
  racing, on in pause and after); menu steering with the wheel; the Driving
  Force Pro recognised (USB type 4).
- **The Pod race** (`RacePOD`, `RacePOD2P`): one compulsory stop per lap in
  a stop area (608-641 m into the lap): stop with all four wheels in it and
  pull the handbrake until the Pod sits down; two missed stops cost the lap
  and disqualify; computer Pods stop too. Reached from the arcade's Pod
  entry (menus) when its data exists.
- **Not ported:** `RaceTraining` (only reachable from GT Concept's licence
  menus), the i.LINK `Multilink` / linked demo, the replay options and demo
  music Lupo 2 removed.

## Race HUD (`hud.h`, `ported/hud.md`)

- **New:** the Dualnote battery gauge; the Pod race displays, warnings ("stop
  area", "sat down") and texts (English written for the US build).
- **Switched look changes:** tachometer needle on a smoothed rpm; SELECT
  telemetry with the remote layout; redrawn accel / brake monitor and
  steering display; dial scales matching the dial art plus a 14,000 rpm dial;
  needle colour / blur; one tinted mini-map pointer; text shadows and mode
  labels; a car sign by the car name; the logo watermark; the demo HUD with
  the car name only; the time-limit display; free-run messages live only;
  licence result timing.
- **Generalised:** GT3's time-trial disqualification display (Lupo 2's
  `SimpleExam*`), same behaviour in the time trial.
- **Not ported:** Lupo 2's single "ESP" lamp (GT3's separate VSC and TCS
  lamps stay); two things Lupo 2 dropped.
- Every new sprite falls back to GT3's drawing when it is missing.

## Rendering and effects (`rendering.h`, `ported/rendering.md`)

- **Texture animation for every car** (races and the menu car); car models'
  animation tracks (one GT3 car, la0008, has a track that would start
  moving: `ENH_RENDER_CAR_ANIMS` 0 keeps it still).
- **The Pod model:** face, colour, tail lean / bob, state, from the Pod's
  emotions.
- **Active rear wing** on the moving-wing cars; **blinking lamps** (a lamp's
  blink pattern in its model data; no GT3 lamp has one).
- The course animation driver's fifth model; the Dualnote's power on the
  showroom spec; the tyre shoulder strip; flares without fog; the showroom's
  surface tables (correct menu top speeds before the first race).
- **Deferred** (they need Lupo 2's courses and a VU1 microcode change):
  the projected car shadow, falling leaves, the extended course package.
- **Not ported:** bikes and riders.

## Cameras and sound (`cameras_sound.h`, `ported/cameras_sound.md`)

- **Cameras (switches):** TV-camera aim 0.14 (the car nearer the centre of
  the shot); texture LOD bias 0 (softer, less shimmer); demo view-mode
  choice; a TV-camera zoom button (triangle in replays, up to x4); the race
  start camera sequence; a taller occlusion box for in-car sway (tall cars
  such as the Pod); stereo eye distance.
- **Sound (switches):** engine sound following the engine's real load; the
  cockpit / outside mix; tarmac squeal curves; squeal start detune; reverse
  gear whine.
- **The Pod's voice** as Lupo 2 builds it (its own channel, Doppler,
  priority, stops with the car's other sounds); the CVT / electric /
  Dualnote whines.
- **Not ported:** the "avantgarde" replay colour presets (they replace GT3's
  replay effects), a head-tracker view, a debug camera.

## Menus, game flow, save data (`menus.h`, `ported/menus.md`)

- **CVT:** the gear-ratio settings show `----` and the transmission page is
  refused; a CVT is automatic-only in the arcade car select, arcade races
  and free run.
- `gear_max_speed` (top speed of the gearing, in the settings, when the page
  has the item); dirt tyres forced in arcade dirt races; default pedal
  calibration for the GT Force and the C294 wheel; result board:
  disqualified cars at the bottom, gaps on laps-only boards; link-battle
  result waits for all but one car.
- **Assists:** a per-player assist step in the car select and an "assist"
  option (four ASM / TCS presets); free run and the 2P battle pass the
  assists to the race.
- **Screen position adjust**, stored in GT3's reserved option bytes (saves
  stay compatible).
- Short-menu gates, the text width cache, a movie start that waits for the
  first frame (at most 2 s), the Pod race entry.
- **Optional (switches):** replay demos in the attract, a loading mark, the
  arcade cursor remembered, wide screen from the console's setting.
- **Not ported:** GT Concept's settings mode, presents and car-get screens,
  its save layout (GT3's stays), its money transfer to GT3 saves, its boot
  splash, menus and music list, its option defaults.

## Assets to transplant (the user's side)

Without these, everything above that is data-driven stays off and GT3 plays
as before (with the switched changes).

| Feature | What |
|---|---|
| Gearbox types | A widened GEAR table (rows over 0x30 bytes, type at +0x30), or Lupo 2's CVT / clutchless row codes (built-in list); a 2-forward-gear row makes a car a CVT in the menus |
| Dualnote | A DRIVETRAIN row with layout 2, sub-type 5 (its hybrid constants are built in); `meter_sign_dualnote` sprite for its gauge |
| Pod | The car under code `pod_tmv_01` (paramDB row) and its Lupo 2 model; `sound/gt3pod.ins` (Lupo 2 disc, 81,760 bytes) and the 11 `pod_*` entries (bank 3, programs 0-10) merged into GT3's `se.inf` (do not replace GT3's file) |
| Pod race | A course with the stop area (road triangles flagged attr2 bit 2, lap >= 641 m), a race record with licence type 5, `CLASS_PODRACE` cars, the `POD_RACE` course list, race `APO20001`, the `pod_race` / `pod_race_2p` buttons on "ar/top" |
| Moving wings, SL suspension, Lupo GTI Cup | The cars under Lupo 2's codes, with their Lupo 2 models (wing: switch id 6) |
| HUD | `meter_sign_dualnote`, `replay_sign_accel` / `_brake` / `_gtforce` / `_car`, `car_pointer`, `rev_14`, `revb_14`, `gt_aspec_logo_mini` merged into GT3's `race/US/display.imgs` (keep it sorted; Lupo 2's pack lacks GT3 sprites) |
| Menus | `setting.mbl`: `gear_max_speed`, a `your_car_is_CVT` page; `option.mbl`: `multi_assist`, `assist_N_on/off`, screen adjust sliders; `arcade.mbl`: assist items, Pod buttons |
| Driving Force Pro | Lupo 2's `PDIUSB.IRX` (untested with GT3's IOP modules) |

## What to check first

1. Drive any car with every switch at 1: lighter, more squat / dive, slower
   pad steering, smooth tachometer needle. If a change feels wrong, its
   switch at 0 brings back GT3's.
2. A GT-R-type 4WD (ATTESA retune).
3. A race against computer cars (the AI changes) and a timed race.
4. The log lines in the PCSX2 console: `[enhanced] ...`, `hud: ...`,
   `snd: ...`, `menus: ...`. Each race start logs the HUD's sprites and
   `[enhanced] heap: N KB free`: the enhanced program is larger than GT3's,
   so its heap is about 220 KB smaller; this shows the margin a loaded race
   leaves.
5. Then the transplants, one feature at a time.
