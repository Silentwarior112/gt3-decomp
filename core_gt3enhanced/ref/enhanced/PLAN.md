# Enhanced engine: the integration plan (phase 2)

Phase 1 surveyed Lupo 2 against GT3 area by area (`survey/*.md`). This plan
turns those reports into C in `src/` and `include/`. Read `../ENHANCED.md`
first; the survey of your area is the list of what to port, and its entries'
"Port" fields say how.

## Rules

1. **Port what Lupo 2 changed in the game logic, in GT3's terms.** Read the
   Lupo 2 function (`tools/lupo2.py dis`), write the behaviour into our C
   (GT3's structures, names and style). Never copy a layout or an offset from
   Lupo 2: its objects have other fields.
2. **Never remove a GT3 feature.** Where Lupo 2 drops or replaces something
   GT3 has, GT3's stays (the surveys mark these "removed in Lupo 2" / "do not
   port"). Where Lupo 2 replaces a GT3 behaviour with a better one, the port
   is a switch (rule 3), so GT3's stays one define away.
3. **Switches.** Every port that changes how GT3's own content behaves (a
   retune, a bug fix, an AI change, a new HUD look) is guarded by a switch in
   `include/game/enhanced.h`:

       #define ENH_PHYSICS_PITCH_INERTIA 1   /* Lupo 2: pitch inertia x1.35 (physics.md 3) */

   `1` = Lupo 2's behaviour (the default), `0` = GT3's. In the code:

       #if ENH_PHYSICS_PITCH_INERTIA
           inertia.pitch *= 1.35f;                 /* Lupo 2 (L2 0x001E....) */
       #endif

   One switch per survey entry (not per line), named `ENH_<AREA>_<WHAT>`.
   Features that only act on data GT3 does not have (a gearbox type, the
   Dualnote, the Pod, a lamp blink pattern) need no switch: with GT3's data
   they do nothing. Such a feature must stay inert with GT3's data and
   assets: verify that.
4. **Missing assets never crash.** A port that needs a file GT3 lacks (a
   sound bank, a sprite, a model) checks for it and falls back to GT3's
   behaviour without it (the user moves assets between builds: name every
   file and log line in the area's port notes).
5. **Comments.** Each ported change says where it comes from, in the
   style of the surrounding code: `/* Lupo 2: ... (L2 0x001E3060) */`. A new
   function gets a header comment like the decompiled ones (what it does,
   Lupo 2 address, callers).
6. **Save data stays GT3's.** No port changes the layout of GameStatus,
   GameOptions or any memory card file (GT3's reserved bytes may be used
   where a survey says they are zero by default: say so).
7. **Object layouts.** New fields go at the end of a struct or class (decomp
   code may still address fields by raw offset); grep for raw offsets and
   `sizeof` literals of a struct you grow (`0x5F0`, `new (0x...)`,
   allocation sizes) before you grow it.

## Process

- **Compile only your files**: `python tools/mcc.py <src> -o build/<area>/<name>.o`
  (the matching compiler, the file's flags). Never run `tools/build.py`,
  `tools/score.py` or `tools/fulllink.py`: several agents work at once and
  those write shared output. The integrator links and boot-tests after each
  wave.
- **Ownership.** Each agent edits only its area's folders (below) and the
  headers of those folders. A change another area must make goes into
  your notes under "For other areas" (the integrator hands it on). The
  shared interfaces (wave 0) are fixed before the waves start; ask for a
  change, do not make it.
- **Port notes**: `ref/enhanced/ported/<area>.md`, one line per survey
  entry: ported (file, function, switch) / not ported (why) / deferred
  (what it waits for); then what to test in game, the assets each port
  needs and the log lines that show it working.

## Waves

**Wave 0, shared interfaces (one agent, then link + boot):**
`include/game/enhanced.h`, which includes one switch file per area
(`include/game/enhanced/<area>.h`, each owned by its area's agent; wave 0
creates them with the switches this plan names); the fields several areas
read, added (unused) at the ends of their structs: the car's special kind,
gearbox type, hybrid state (battery, motor), Pod state (emotion, parking,
animation select), moving-part position, model animation channels and
events, display rpm; `CarModel`'s new words; the new race event, the Pod
information event and car warnings (appended to GT3's numbering, never
inserted: `hud.md` C25); the race display's new flags and its update's time
step (the signature, callers passing GT3's step); `CarEquipments::isCVT`;
the response curve type 3 (menus_a 38). `ref/enhanced/INTERFACES.md` names
each with its writer and readers. Behaviour does not change in wave 0: the
build boots as before.

**Wave 1, in parallel (owners):**

| Agent | Folders | Survey |
|---|---|---|
| physics | `src/game/car/physics`, `src/game/race/dynamics`, `src/game/data/T482_car_equipments.cpp` | physics.md |
| rendering | `src/game/car/model`, `src/game/effects`, `src/game/course`, `src/engine/gfx/T451_model_set.cpp`, the texture animation file (`tex1`) | rendering.md, backend.md 5 |
| hud | `src/game/race/display` | hud.md |
| race | `src/game/race/core`, `modes`, `ai`, `pause`, `src/engine/input` | race_logic.md, backend.md 2-3 |
| menus | `src/game/menu/*`, `src/game/app`, `src/game/common`, `src/game/save`, `src/game/data` (except T482), the display-offset function's file in `src/engine/gfx` | menus_a.md, menus_b.md, backend.md 4, 6-8 |
| cameras + sound | `src/game/camera`, `src/game/sound` | cameras_sound.md |

**Integration** after the wave: all-C build, boot test, the ports' notes
merged into the report for the user.

## What each area ports

Numbers are the survey entries (`survey/<area>.md`, including their "Delta
review" and "Corrections" sections). "Switch" = rule 3; "data" = inert with
GT3's data, no switch; "not ported" names the reason. An entry not listed
here follows its survey's verdict.

### physics (`physics.md`; owner also of `src/game/data/T482_car_equipments.cpp`)

- Switch: 3 pitch inertia; 4 rear ABS; 5 DynParams retune (one switch for
  the table, its parts described in the comment); 6 rear ride height; 7 tyre
  reference grip; 9 automatic 2->1 downshift; 10 1st-gear threshold; 12
  ATTESA retune; 14 roll rate limit; 15 understeer brake aid; 16 computer
  oversteer estimate; 17 pad steering; 18 wheel throttle curve (curve type 3,
  wave 0); 20 force feedback; 21 catch-up cap; 22 display flags; 24 ATTESA
  tyre radii; 27 (`car_pair_contact`, AI caution) and 29 (timed race ends at
  the leader) where their code is in this area.
- 13 (computer cars' grip handicap from `enemySkillC2`) together with
  race_logic 8 under ONE switch `ENH_AI_SKILL_AS_GRIP`: 1 = Lupo 2 (grip
  handicap, the AI no longer adds it as km/h), 0 = GT3 (km/h offset). Never
  both.
- Data: 2 special kinds (the car-code chain, the Lupo GTI Cup spec); 8 tyre
  compound flags; 11 4WD sub-type 6; 19 input device 6 (the game side; the
  wheel itself needs Lupo 2's USB IOP module: backend.md 1); 23 gearbox
  types, CVT, the Honda Dualnote (all of it, Lupo 2 as truth, the GT3Hooks
  ports as a reference: physics.md lists their gaps); 26 SL active
  suspension; the Pod's emotion machine, parking counter and squat; the
  moving part (wing) updater; the model animation channels and events; the
  display rpm spring (hud.md C2); the per-slot rpm export if a port reads it.
- Not ported: 1 bikes and trikes (dormant in Lupo 2: nothing selects them);
  25 condition without power effect (GT3 keeps its garage, run-in and oil);
  28 licence tests no longer failing (GT3's licences keep their rules).

### rendering (`rendering.md`, `backend.md` 5 and 9)

- Port: 1 texture animation for every car (`modelset_reset_anims`,
  `clut_anim_reset_all` take an id; id -1 keeps GT3's immediate apply,
  backend.md 5); 2 `CarModel` words and the switch query ids 6-10; 5 the Pod
  model drive; 8 the course animation driver's fifth model; the Dualnote's
  power bonus in `ShowRoomCar::SetupShowRoomCar` and `gear_chart_with_sheet`
  (menus_b 18).
- Switch: 9 tyre mesh shoulder; 10 flares without fog.
- Data: 3 active rear wing; 4 blinking lamps (verify every GT3 lamp record's
  blink fields are 0, and that GT3 car models carry no texture-animation
  records that 1 would start playing; if some do, 1 gets a switch).
- Deferred (Lupo 2 courses / microcode): 6 projected shadow (with backend.md
  9's VU1 mode 6), 7 falling leaves, 8's course package extensions. Not
  ported: 11 bikes; the car+0x828 rider / driver object unless it draws
  something for cars (find out, and say); D2 (lights left on).

### hud (`hud.md`)

- Switch: C2 (needle on the display rpm), C3 + C4 telemetry cycling and the
  layout tables, C5 / C6 redrawn monitors, C8 / C9 dial scales (+ the 14,000
  rpm dial), C10 needle look, C11 mini-map pointer, C12 text look, C13 car
  sign, C14 logo and mini-logo flag (not the GT Concept front-end gate), C15
  demo HUD, C16 time limit look (the 3 s offset only with race_logic's time
  limit), C23 free-run messages, C24 licence result timing.
- Port: C1 the Dualnote gauge; C17 / C18 / C21 / C22 the Pod displays and
  warnings; C19 the time step (signature: wave 0); C20 the SimpleExam
  generalisation; C26 the Pod texts (write English ones for the US build).
- Every new sprite (`meter_sign_dualnote`, `replay_sign_*`, `car_pointer`,
  `rev_14`, `revb_14`, `gt_aspec_logo_mini`) falls back to GT3's drawing
  when the sprite is missing.
- Not ported: C7 the single ESP lamp (GT3's VSC and TCS lamps stay: the
  merged lamp hides which aid acted); R1, R2.

### race (`race_logic.md`, `backend.md` 2-3; owner also of `src/engine/input`)

- Switch: AI 1-7, 9 (`ENH_AI_*`, one per entry; 7 reads the physics' front
  slip); 8 with physics 13 (`ENH_AI_SKILL_AS_GRIP`); 10 the best-lap ghost
  keeps the lap's car (GT3's ghost file format must not change: verify);
  the time limit ending the race and its grace; physics.md 29 where its
  code is in race/core; D1 the replay afterimage (field-steady trail).
- Port: the HUD buttons in replay and pause (with hud C3 / C14); the wheel
  spring by race state (backend.md 2); the new race event (wave 0); the Pod
  race `RacePOD` / `RacePOD2P` with its rules, AI stop, runners and the
  arcade mode decode (menus_a 9 / 14: reachable from the arcade decode;
  without the Pod course and car it is never offered: say what data starts
  it); backend.md 3 menu steering with the wheel (switch).
- Not ported: `RaceTraining` (reached only from GT Concept's licence menus),
  `Multilink` / `RaceLinkDemo`, the removed replay options and demo music.

### menus (`menus_a.md`, `menus_b.md`, `backend.md` 4, 6-8)

- Port: menus_b 1 the CVT lock-out (`isCVT`: wave 0), 2 `gear_max_speed`
  (the text works when the page has the item), 5 dirt tyres in arcade dirt
  races, 11 + backend.md 4 the wheels' default calibration, 14 the result
  board (a with the new event, b the gaps), 15 the link-battle result;
  menus_a 6 + 39 CVT forces automatic, 8 free run / 2P pass ASM / TCS, 17 the
  per-player assist step, 23 the assist option, 24 + 40 + backend.md 6
  screen adjust (GT3's reserved option bytes), 26 the two short-menu gates,
  31 the text width cache, 32 + backend.md 8 the movie wait, 33 the menu
  car's animation (with rendering 1), 9 / 14 the Pod runners and entry
  (with race).
- Switch (optional items, default 1): menus_a 4 replay attract demo, 11
  loading mark, 15 remembered cursor; menus_b 10 + backend.md 7 wide screen
  from the console.
- Not ported: every Concept replacement (menus_b 3, 4, 6, 7, 8, 9, 12, 13,
  16, 17; menus_a 1, 2, 3, 7, 10, 12, 13, 16, 18-22, 25, 27, 28, 36, 37, 41,
  42).

### cameras + sound (`cameras_sound.md`)

- Switch: cameras 1 TV aim, 2 LOD bias, 4 demo view-mode choice, 5 zoom
  button, 6 race start in view mode 0, 7 occlusion box, 8 stereo eye
  distance; sound 2 engine load response, 3 mix table, 4 tarmac squeal, 5
  squeal detune.
- Port: sound 1 the Pod voice as Lupo 2 builds it (a `CarSound` channel,
  doppler, priority, stop / reset with the car's sounds; `sound/gt3pod.ins`
  as the 4th bank and the 11 `pod_*` entries merged into GT3's `se.inf`:
  without the bank the Pod is silent, nothing else changes); sound 6 the
  gearbox type 3 / Dualnote whine.
- Not ported: cameras 3 avantgarde presets, 9; sound 7.

### backend (`backend.md`)

- 1 Driving Force Pro: the game side is ported with physics 19 / 20 (inert
  until the wheel reports); the USB IOP module is the user's to test
  (note in the report). Every other item is listed above with its area.
