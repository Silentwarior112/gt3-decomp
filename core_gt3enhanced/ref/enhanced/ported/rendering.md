# Port notes: rendering (wave 1)

Area: `src/game/car/model`, `src/game/effects`, `src/game/course`,
`src/engine/gfx/T451_model_set.cpp`, `src/engine/pgl/tex1.cpp`, the menu
car's update (`gt2k_menu::Car::virtual_1`, menus_a 33) and their headers.
Survey: `survey/rendering.md` (with its Corrections and Delta review),
`survey/backend.md` B5 and B9, `survey/menus_a.md` 33, `survey/menus_b.md` 18.

Every changed file compiles with `tools/mcc.py` (objects in
`build/rendering/`). No struct grew: `CarModel` stays 0x54, `RaceCarModel`
0x6E4, `CarLamp` 0x80 (two of its unknown words named). The size probe
(`tools/sizeprobe.py --only-diff`) lists only the hud's and race's
growth (RaceDisplay and the race classes), none of this area's.

## Switches (`include/game/enhanced/rendering.h`)

The wave-0 names `ENH_RENDER_*` are kept (INTERFACES.md section 1).

| Switch | Entry | 1 (default) = Lupo 2 | 0 = GT3 |
|---|---|---|---|
| `ENH_RENDER_CAR_ANIMS` | 1 | `CarModel::animate` steps the CLUT records and the model's animation tracks | only the CLUT records step; the tracks stay still (GT3's la0008 keeps its still morph) |
| `ENH_RENDER_TYRE_SHOULDER` | 9 | an ordinary tyre's inside gets its shoulder strip | GT3's tyre |
| `ENH_RENDER_FLARE_NO_FOG` | 10 | fog off in `FlareClass::begin` | GT3 |
| `ENH_RENDER_SHOWROOM_TABLES` | menus_b 18 / "For other areas" | `SetupShowRoomCar` runs `surface_tables_init(0)` first | GT3: the menus' top speed uses the file's defaults until a race has run |

## Entries

| Entry | Outcome |
|---|---|
| 1 CLUT animation for every car | **Ported, switch for the tracks.** `CarModel::animate` / `applyAnims` / `setAnimSelect` (T446, new; L2 0x0022DAE8 / 0x0022DB18 / 0x0022DC90). `RaceCarModel::updateBodyState` (T180) animates every car each frame; `RaceCarModel::applyAnims` (T180, new, Lupo 2's slot 3 L2 0x001A8B00) writes the palettes, **to be called by race/core** (below). `ENH_RENDER_CAR_ANIMS` (see the data check). |
| backend B5 | **Ported.** `modelset_reset_anims(m, id)` (T451) and `clut_anim_reset_all(img, id)` (tex1) take the id; id < 0 is GT3's reset (every record, the tracks, and the immediate apply the prize models need); id >= 0 resets only that id's records, marks them dirty, no apply, no tracks (Lupo 2). C++ callers that pass no id get -1 (default argument), so T454's `spriteset_reset_clut_anims` and T460's `PrizeBase` (other areas' files) are unchanged; the course passes -1 explicitly. New `modelset_step_clut_anims` (T451, not Lupo 2's: the switch's 0 path). |
| 2 `CarModel` words, SWITCH 6..10 | **Ported.** `CarModel::Callback::selectSwitch` (T446) answers 6 (wing morph), 7 (podLean morph arm 0 when != 0.5, else podBob arm 1), 8 (podColorBlend morph, arm podColor), 9 (podState), 10 (animSelect), reaching the fields through `(CarModel *)this` (the callback is at +0). The constructor sets Lupo 2's defaults (podLean / podBob 0.5, animSelect -1). Setters `setWing`, `setPodTail`, `setPodColor`, `setPodState` (L2 0x0022DC60..0x0022DC88). |
| 3 active rear wing | **Ported (data).** `updateBodyState` feeds `CarModel::wing` from `RaceCarState::wingPosition` for the special kinds 2 / 8 / 9 / 10 (L2 0x001E2110 / 0x001E1E00), 0 for every other car. Needs the physics' moving-part updater and special kind. |
| 4 blinking lamps | **Ported (data).** `CarLamp::blink` (+0x6C) / `blinkOffset` (+0x70) named (T159_flare.h); `LAMP_BLINK_*` (T180 header); the pattern table, the pulse curve (L2 0x001A9048 / 0x001A8FB0) and the two phases (`RaceCarModel::blinkPhase`, stepped 1/60 in `updateBodyState`); `addLamps` and `drawLampReflections` light a blinking list A lamp at its pattern's brightness. Lupo 2's lights-on argument is always 1, so its off branches are not ported. Not in Lupo 2: a pattern above 2 is treated as steady (Lupo 2 would read past its two records). |
| 5 Pod model drive | **Ported (data).** In `updateBodyState`, for special kind 1: colour category and cross-fade (L2 0x001E17D8, table 0x002DE940 = {0,4,4,4,1,1,1,2,2,2,3}), tail look / bob (L2 0x001E18A0, triangle L2 0x001E1470), state and face = the current emotion (L2 0x001E1950 / 0x001E1990), `setAnimSelect(emotion)`. Reads the physics' `podEmotion`, `podEmotionHold`, `podAnimFrame`, `podLean`. |
| 6 projected shadow, backend B9 | **Deferred** (Lupo 2 courses, VU1 mode 6 and microcode). |
| 7 falling leaves | **Deferred** (Lupo 2 courses, `race/particle.img`). |
| 8 a course animation: fifth model | **Ported.** `course_reset_anims` / `course_step_anims` / `course_apply_clut_anims` / `course_release_clut_anims` (T179) also tick `model34`. No switch: no GT3 course's model34 is animated (data check). |
| 8 b, c course package / CourseEnv extension | **Deferred** (Lupo 2 courses, needs a format check). |
| 9 tyre shoulder | **Ported, switch** `ENH_RENDER_TYRE_SHOULDER` (T462 `TirePacket::virtual_4`, `buildShoulder(sb, c, 0, 1, 1, 0)`). |
| 10 flares without fog | **Ported, switch** `ENH_RENDER_FLARE_NO_FOG` (T159 `FlareClass::begin`, `gfx_disable(PGL_CAP_FOG)` after the two VRAM allocations, as L2 0x00198A04). |
| 11 bikes and riders | **Not ported** (no bike dynamics or assets). |
| 12 / D2 lights left on | **Not ported** (removed in Lupo 2; GT3 keeps `course_env_lights_off`). |
| D1, D3..D13 | Noise or twins (survey); D11 / D12 are menus' gauge values (always 0). Nothing to port. |
| menus_a 33 menu car | **Ported** (T212 `gt2k_menu::Car::virtual_1`): while shown, `model.animate()` + `model.applyAnims()` each frame; while not shown, `setAnimSelect(1)` then `setAnimSelect(0)` (L2 0x001BE448 / 0x001BE450 / 0x001BE52C / 0x001BE538). Not in Lupo 2: the id resets run only once the model is loaded (`ready`); while a new car loads, the CarModel still points into the buffer the loader is filling, and the rewind would walk and write a half-read model. Then only `animSelect = 0` is set (the load's relocation rewinds every record anyway: the same end state). The other menus_a 33 accessors (setMagnify, setReflection, setAnti, setType, setEnvTexture, setViewport, +0x24 bytes) are Concept menu settings with no GT3 caller: not ported. |
| menus_b 18 Dualnote power bonus | **Ported in `ShowRoomCar::SetupShowRoomCar`** (T278): `sheet.maxPower += (2 x 20 + 35 kW) / 0.7355` (+102 PS) when the spec is the 4WD layout with sub-type 5 (L2 0x001D9990..0x001D99A8, 0x001E1238). No GT3 DRIVETRAIN row has sub-type 5. Lupo 2's `surface_tables_init(0)` in the same function: switch `ENH_RENDER_SHOWROOM_TABLES`. **`gear_chart_with_sheet` is physics's (T292): request below** (it has no caller in either build). |
| "For other areas" (survey) | `CarRender::draw`'s explicit environment texture (L2 0x001CCAE8): no GT3 caller, not ported. `ShowRoomCar::power` / `torque` dropping GT3's catalogue figures: not ported (GT3 feature; note that with catalogue figures in the Dualnote's row, a catalogue power text shows them, not the bonus). Head tracker, prize, sound, engine/io items: other areas / not needed. |
| GT3Hooks Pod.c gaps 1-4, 6 | Covered: every car animates (1); the menu car (2); SWITCH 6 and the defaults for non-Pods (3); wing and blink in the same seam (4); recognition by the special kind (6). Gap 5 (emotion per race step) is physics's. Gap 7: race / hud. |

## The car+0x828 object, `computeDriverPosture`, `renderDriver`

Nothing for cars. In Lupo 2, `RaceCarModel`'s slots 10 / 15 / 20
(`delayLoadDriverModel`, `computeDriverPosture`, `renderDriver`) are
`VehicleModel`'s empty defaults (L2 0x002C8AD8 / 0x002C8AE0 / 0x002C8AE8,
`jr $ra; nop`); only `RaceBikeModel` overrides them (slots 15 / 20: L2
0x001A9AC8 / 0x001A9AF0). The car+0x828 object (init L2 0x00209558 and step
L2 0x0020A668 -> 0x00209810, run for every car by the conductor's wrappers
L2 0x001F3710 / 0x001F3730) reads the car's speed, brake, model channels 2 / 3
and event bytes and writes only itself; its readers are the rider's posture
code (L2 0x001B1908, called from `RaceBikeModel` slot 15, which calls L2
0x001B00D8, and L2 0x001B09DC / 0x001B10F4 / 0x001B11B8 through the
parameter accessors L2 0x00208E20 / 0x00208E40). physics.md calls L2
0x001B00D8 "HUD-area code": it is the rider posture (Geomath vectors). Not
ported, and the physics area need not port the object for cars.

## Data checks (PLAN.md)

Scripts and full output: `build/rendering/tools/` (`scan_models.py`,
`dump_anim.py`, `scan_courses.py`, `scan_gt3.txt`, `scan_lupo2.txt`), run
over the extracted files of the GT3 US and Lupo 2 discs.

- **Lamp blink fields**: every lamp record of every GT3 car model (cars/day,
  night, eve, menu/cars; lists A, B and C, about 2,600 records per folder)
  has +0x6C = +0x70 = 0. GT3's lamps stay steady.
- **CLUT animation records**: none in any GT3 car model (the four folders).
- **Model animation tracks**: 7 GT3 day models carry an AnimSet (la0008,
  la0011, ma0095, ma0099, ml0003, pd0010, ty0032; 6 in night / eve, no
  ma0099). All tracks have zero targets except **la0008's track 1** (day,
  night and eve): kind 7, rate 1, written into a morph command (0x34) of
  part 0 that weights packet 6 (command 0x35, n = 1). Lupo 2's
  `CarModel::animate` would start that morph cycling once a second (Lupo 2's
  own la0008 has the same track, so Lupo 2 shows it). Hence
  `ENH_RENDER_CAR_ANIMS`: 0 keeps la0008 still and still runs the CLUT
  records.
- **SWITCH ids** asked by GT3's car models: 0..5 only, so ids 6..10 and their
  Lupo 2 defaults never reach GT3's models.
- **Course model34**: 41 of GT3's 321 course files have one, none with CLUT
  records or tracks with targets.
- GT3's `cars/day|night|eve/ho0100` is an empty file (skipped).

## What to test in game

GT3's own content must look as before (switches at 1):

- Any GT3 car, any race: lamps steady, nothing animates; no
  `[enhanced] car model:` line.
- la0008 (a Lancia): with `ENH_RENDER_CAR_ANIMS 1` one morphing part of the
  body cycles once a second (Lupo 2's behaviour); with 0 it is still.
- Menus: the menu cars look as before. The car spec screens' top speed
  before the first race now equals the one after a race
  (`ENH_RENDER_SHOWROOM_TABLES`).
- Tyres: close-ups (replay, showroom) of an ordinary tyre's inner side show
  its shoulder bevel.
- Flares: night races (headlights, course lamps) with fog: no fog tint on
  the flare set-up.

With Lupo 2 assets (the user's transplants):

- **The Pod** (Lupo 2 `cars/day|night|eve/ty0150`; physics must recognise
  it as special kind 1 and run the emotion machine): in a race the face
  changes with the emotion (`[enhanced] pod: face A -> B` per change), the
  colour cross-fades, the tail leans / bobs. Needs race/core's apply loop
  (below) for the face's palettes to move.
- **Wing cars** (models asking SWITCH 6 in Lupo 2: cd0001, cs0001, cs0005,
  ni0167, vw0008 in cars/day|night|eve, presumably the Cien, the Crossfires,
  the GT-R concept and the W12 (uncertain mapping); physics must give them
  kinds 2 / 8 / 9 / 10 and run the wing updater): the rear wing rises above
  about 80 km/h and drops below 60 (GT-R concept), linearly 56..80 km/h
  (Cien).
- **Blinking lamps** (Lupo 2 `cars/*/ni0177`: 14 list A lamps, 2 with
  pattern 1, 8 with pattern 2, three of them offset 0.45 s; it also has a
  morph track): the lamps pulse / strobe, their road reflections too.
- **Menu animation** (Lupo 2 `menu/cars/sz0024`: one CLUT record, id 0):
  its texture animates on the menu car. Lupo 2's `menu/cars/ty0150` has no
  CLUT records and no SWITCH 6..10, so the Pod is static in the menus (as in
  Lupo 2); a race model copied there would show face 0.
- Missing assets: none of these ports loads a file. Without the models,
  nothing changes. With a Lupo 2 model on a car the physics does not give
  its special kind (for example the Pod model swapped into a GT3 car's
  slot, under that car's code), the model shows Lupo 2's defaults: wing
  down, and for ty0150 no face at all (SWITCH 10 answers -1, past its 11
  arms) while its 27 face records all step. That is Lupo 2's behaviour for
  a non-Pod car; the Pod needs its own car code (kind 1, physics).

## Log lines

- `[enhanced] car model: kind K, animated textures T, tracks yes|no, blinking lamps L`
  once per car at the race's set-up (`RaceCarModel::setupTires`) when the
  model has a special kind, animated textures or blinking lamps. GT3's cars
  print nothing.
- `[enhanced] pod: face A -> B` when a Pod's emotion (its face id) changes
  (A is -1 the first time).

## For other areas

- **race (race/core, T130)**: `RacePS2Base::virtual_3` must apply every
  entry's car model animations once a frame, after
  `course_apply_clut_anims` (Lupo 2 L2 0x0017BA98..0x0017BADC, every entry,
  not only enabled ones):

      course_apply_clut_anims(org->course->data);
      RaceEntryBase *entries = org->entries;
      for (int i = 0; i < entries->count; i++)
          entries->cars[i]->model.applyAnims();   /* Lupo 2 (rendering.md 1) */

  Without it the CLUT animations never reach the palettes in a race (the
  Pod's face keeps its first palette); nothing else breaks.
- **physics (T292)**: `gear_chart_with_sheet` adds the Dualnote bonus after
  `engine_spec_sheet` (Lupo 2 L2 0x001E3B74..0x001E3B84). Lupo 2 tests
  `GearCalc.driveType == 7`, which in GT3 is the fixed-split 4WD: test the
  spec as `SetupShowRoomCar` does (`spec->layout == LAYOUT_4WD &&
  spec->awdType == 5`), then `sheet->maxPower += (2 * 20 + 35) / 0.7355f`.
  No caller in either build: low priority. If physics names the hybrid
  parameters or sub-type 5, T278's local defines can use them.
- **physics (fields)**: rendering reads `CarDynamics::specialKind`,
  `wingPosition` (kinds 2 / 8 / 9 / 10 only), `podEmotion`,
  `podEmotionHold` (the colour cross-fade: the count from 36 down, see
  L2 0x001E17D8), `podAnimFrame` (0..47, the bob) and `podLean` (-30..30).
  Lupo 2's `car_dirt` returning 0 (physics.md "For other areas") removes
  GT3's dirty cars: not ported here (the function is physics's; GT3 keeps
  it).
- **integrator (INTERFACES.md / T302 comments)**: `podAnimFrame` feeds
  SWITCH 7's bob (CarModel::podBob, via L2 0x001E18A0 / 0x001E1470), not
  SWITCH 9; SWITCH 9 (`podState`, L2 0x001E1950) and the face (SWITCH 10,
  L2 0x001E1990) are both the current emotion. `podEmotionHold` has a
  rendering reader (SWITCH 8). `CarModel::animSelect` now defaults to -1
  (no longer 0) and podLean / podBob to 0.5 (section 6's note). `CarLamp`
  +0x6C / +0x70 are named `blink` / `blinkOffset`.
- **menus**: `include/game/menu/framework/T212_menu_car.h`'s comment of
  `Car::virtual_1` could mention the model animation (the .cpp has it).
  T460 `PrizeBase` keeps calling `modelset_reset_anims(m)` (= -1, GT3's
  reset with the immediate apply); nothing to change.

## Uncertain

- The menu car's `ready` gate is a guard Lupo 2 does not have (its own code
  walks the stale model while a new car loads); the end state is the same.
- Applying the palettes from `RacePS2Base::virtual_3` (Lupo 2's place)
  depends on race's port; the CarModel side is ready.
- The wing car model files' mapping to the car names (cd0001 = Cien, etc.)
  is inferred from the maker prefixes, not read from the paramDB.
- The Pod is recognised by its special kind only (as Lupo 2; GT3Hooks'
  Pod.c recognised the model by its SWITCH ids 1 and 10). If the user keeps
  transplanting the model under a GT3 car code, the physics' kind
  assignment has to cover that code, or the face does not show.
