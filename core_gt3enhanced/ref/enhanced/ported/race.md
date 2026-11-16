# Port notes: race (wave 1)

Area: `src/game/race/core`, `modes`, `ai`, `pause`, `src/engine/input`, their
headers, `include/game/enhanced/race.h`. Surveys: `survey/race_logic.md` (with
its Delta review), `survey/backend.md` B1..B4, the race items of
`survey/physics.md` (27..29) and `survey/hud.md` (C3, C14, C16).

New files: `src/game/race/modes/race_pod.cpp` (the Pod race's rules and its
two race classes), `src/game/race/modes/race_pod_runners.cpp` (their runners),
`include/game/race/modes/race_pod.h`, `include/game/race/modes/race_pod_rules.h`.
`race_pod.cpp` has a static initialiser (`_GLOBAL_$I$g_podStopStart`, the
registration of "RacePOD" / "RacePOD2P") and frame info: `tools/fulllink.py`
registers both.

Every changed file (and every other file of the five folders) compiles with
`tools/mcc.py` (objects in `build/race/`). The changed files also compile with
every `ENH_*` switch at 0. `python tools/sizeprobe.py --only-diff`
lists nothing (730 entries) after the size updates below.

## Switches (`include/game/enhanced/race.h`)

`1` = Lupo 2 (default), `0` = GT3. No switch was added, renamed or dropped.

| Switch | Entry | 1 (Lupo 2) | 0 (GT3) |
|---|---|---|---|
| `ENH_AI_TORQUE_HANDICAP` | A1 | a throttle scale s < 1 = the pedal giving s of the net engine torque | the pedal x s |
| `ENH_AI_CAUTION` | A2 | caution 1 from 5 km/h closing, 0.5 under 5 s (from 0.1 km/h), 0.25 under 4 m | 36 km/h, 3 s (1 km/h), 2 m |
| `ENH_AI_TRAFFIC` | A3 | 180 frames between caution line changes, none while blending, the timer only mutes the caution, crawling pit approach holds the timer | 30 frames, the timer stops everything |
| `ENH_AI_START_LOOKAHEAD` | A4 | computer cars, standing / slow rolling start (< 60 km/h), not dirt, not Pod: + up to 200 m over 5 s | + up to 40 m over 2 s for all |
| `ENH_AI_PIT_EXIT` | A5 | leaving the pits under 30 km/h: 5 m look-ahead | 20 m |
| `ENH_AI_RECOVERY` | A6 | turning round, a front hit also backs up when > 60 deg off the line's direction | heading error / box only |
| `ENH_AI_SLIP_STEER` | A7 | yaw-rate gain x clamp((peakSlip - frontSlip) / peakSlip, -1, 1) | the gain |
| `ENH_AI_DRIVER_SPREAD` | A8 a | the per-car spread of computer drivers in every race | only in conductor mode 2 |
| `ENH_AI_SKILL_AS_GRIP` | A8 b + physics 13 | the AI ignores `enemySkillC2` (physics applies it as grip) | the AI adds it as km/h |
| `ENH_AI_COOL_DOWN` | A9 | after the finish: throttle x0.5 (>= 0.4), corners x0.5, braking scale x0.8 | throttle 0.5, corners x0.8 |
| `ENH_RACE_GHOST_CAR` | M1 | the saved best-lap ghost keeps its lap's car | only its transmission |
| `ENH_RACE_TIME_UP_ENDS` | C1 | the time limit ends every race (code 5) | only the arcade demo |
| `ENH_RACE_TIME_LIMIT_GRACE` | C2 | a set limit + 59 frames | x 60 |
| `ENH_RACE_AFTERIMAGE` | D1 | the replay trail half a line shifted on the other field; none behind menus | GT3's |
| `ENH_INPUT_WHEEL_MENU_STEER` | B3 | menu left / right from the wheel's position, repeat by its deflection | fixed limits, swing test |

`ENH_AI_DRIVER_SPREAD` stays separate from `ENH_AI_SKILL_AS_GRIP` (INTERFACES
offered a merge): the two halves of A8 are independent (one is who gets
spread, the other what C2 means), and A8 b must follow physics 13.

Other areas' switches read here (read only): `ENH_HUD_TELEMETRY_CYCLE` (the
SELECT cycle), `ENH_HUD_LOGO_LOOK` (the mini-logo toggle),
`ENH_PHYSICS_DISPLAY_FLAGS` (the AI's steering display byte, below),
`ENH_MENU_WHEEL_CALIBRATION` (B4 c), `ENH_CAMERA_ZOOM_BUTTON` (the replay zoom
button).

## The AI changes in driving terms

- **A1 throttle handicap.** A race's "computer throttle %" (and the cool-down)
  now means a share of the engine's real pull. GT3 scaled the pedal, so with
  engine braking a 90 % car was much slower than 90 %; now it is close to the
  intended pace, especially on low-power cars and at low revs.
- **A2 + A3 traffic.** Computer cars react to a car ahead much earlier (lift
  from 5 km/h closing speed, plan a pass 5 s before contact, and - with
  physics 27 - lift whenever they sit within 4 m of a bumper even without
  closing). In exchange they change line far less often: once per 3 s after a
  traffic move, never while the previous change is still blending, and they
  can return to the racing line or step away from a car alongside while that
  wait runs. Expect less weaving and fewer bumps, more queues behind slow cars.
- **A4 start.** From a standing start the computer cars look up to 200 m down
  the road for the first 5 s instead of 40 m for 2 s: they hold a straight line
  off the grid and through the first corner instead of darting.
- **A5 pit exit.** Pulling out of the pit box slowly they aim 5 m ahead (was
  20 m): tighter, cleaner exits from the box.
- **A6 recovery.** A car turning round after a spin that hits something now
  also backs up when it points more than 60 deg away from the line's own
  direction (not only the aim point's): fewer cars stuck nose-in against a
  wall.
- **A7 slip steering.** The steering correction fades as the front tyres
  reach their peak slip and reverses beyond twice it: less sawing at the wheel
  at the grip limit. Needs physics' `RaceCarState::frontSlip` (0 until the
  physics writes it: then GT3's behaviour).
- **A8 spread.** Every race's computer cars get the small per-car spread
  (odd cars 4 % slower in corners, braking -0/7/14 %, -0..8 km/h, steering
  +-3 %), not only the finish-timer races: the field strings out more
  naturally. With `ENH_AI_SKILL_AS_GRIP` the race rules' C2 handicap becomes a
  grip handicap (physics) instead of a corner-speed offset.
- **A9 cool-down.** After the flag the computer cars slow much more for
  corners (half speed) and brake earlier, with at least 40 % throttle: a calm
  lap back instead of a fast one.
- **A10 Pod stop** (Pod race only): a computer Pod owing a stop brakes for a
  point in the stop area (leaders at its far end), crawls the last 20 m at
  10 km/h, stops within 0.5 m, pulls the side brake so the Pod sits, waits
  until the stop counts, then drives on.

## Entries: race_logic.md

| Entry | Outcome |
|---|---|
| A1 | **Ported, switch.** `ai_brake_factor` (T366), Lupo 2 L2 0x0020134C. |
| A2 | **Ported, switch.** `ai_note_closing` (T365), L2 0x002007A8. |
| A3 | **Ported, switch.** `ai_avoid_traffic` (T365), L2 0x00200848. |
| A4 | **Ported, switch.** `ai_drive` (T366), L2 0x00201D38. |
| A5 | **Ported, switch.** `ai_drive` (T366), L2 0x00201CC4. |
| A6 | **Ported, switch.** `ai_drive` + new `ai_line_heading` (T366, declared in T366_ai_drive.h; L2 0x00201668) and the static `ai_line_off` (the difference is not wrapped to +-pi, as in Lupo 2). |
| A7 | **Ported, switch.** `ai_drive` (T366), L2 0x0020234C; reads `RaceCarState::frontSlip` (wave 0). |
| A8 | **Ported, two switches** (a: `ai_apply_rules` drops the mode test; b: C2 not read as km/h). T362, L2 0x001FFDE0. |
| A9 | **Ported, switch.** `ai_reduce_pace` (T362), L2 0x001FFF98. |
| A10 | **Ported (Pod race only, no switch).** `ai_brake_factor` (the stop point, L2 0x00200DC8..0x00200F9C / 0x00201218 / 0x0020133C), `ai_drive` (the approach / stop / wait, L2 0x00202564..0x002026A8), `ai_apply_outputs` (**the side brake while stopped, L2 0x00201B24: missed by the survey**). State in `AIDriver::unk_AE` (Lupo 2's +0xAE; T361 is physics', the name stays). Lupo 2 compares the stop point's own distance with -half a lap where it means the difference (never true): kept, commented. |
| A11 AIParams in the conductor | **Not ported** (refactor, same values). |
| A12 speed-step debug | **Not ported** (removed in Lupo 2; GT3 keeps it). |
| A13 line queries' outputs | **Not ported** (no caller uses them). |
| C1 time-up ends every race | **Ported, switch.** `RaceBase::virtual_27` (T129, L2 0x0017A354); `RaceArcadeDemo::virtual_27` (T135) only RacePS2Base's with the switch (L2 0x00181078). |
| C2 59 frames | **Ported, switch.** `RaceOrganization::initRaceState` (T125), L2 0x001784EC. |
| C3 the new race event | **Used** (wave 0's `RACE_EVENT_CAR_DISQUALIFIED`, 0x18, arg = the car): posted by `pod_rules_lap_check`. |
| C4 a mini-logo toggle | **Ported** under hud's `ENH_HUD_LOGO_LOOK`: `RaceBasic::virtual_28` (T132), L2 0x0017F548..0x0017F62C: in a replay, the four shoulder buttons together (the pad's raw bits of RacePS2Base's own input, slot 0x15, == L1+L2+R1+R2 with one of them newly pressed: the survey's "one of the shoulder buttons" is the chord) toggle `RaceDisplay::miniLogo` (dynamic_cast, so nothing for the split display). |
| C4 b SELECT cycle | **Ported** under hud's `ENH_HUD_TELEMETRY_CYCLE`: new `RaceBasic::cycleTelemetry` (T132, L2 0x0017F9A0), called at the end of `RaceBasic::virtual_30`, `RaceFreeRun::virtual_30` (T142), `RaceMachineTest::virtual_30` (T148) (L2 0x0017F758 / 0x00187080 / 0x0018AC60). Live only, SELECT held alone and newly pressed; `showMask`'s `RD_SHOW_AB_MONITOR` / `RD_SHOW_STEERING` by the table {1, 3, 0, 0}; then `virtual_2(0.0f)`. SELECT has no job in GT3's pause menus; the time trial's live SELECT (show / hide the ghost, `RaceSolitaire::virtual_28`) runs only unpaused. |
| C4 c START ends a saved replay | **Not ported** (the app's replay player, GT Concept attract). |
| C5 pause / race state to the controller | **Ported** with backend B2 (below): new `RaceBase::setPausedState` (T129, L2 0x0017A650; from `RaceBase::virtual_8` and `RaceBasic::setPause`), `RaceInput::setMode` (T413, L2 0x0021CE60), `RacePS2Base::virtual_41` mode 0 at a live start, 1 after the race (T130, L2 0x0017B168 / 0x0017B240). Lupo 2 also stores the mode at RaceInput +0x148: nothing reads it (its getter has no caller), so RaceInput keeps its size (RaceSplitBattle has an array). The damper helper (L2 0x0021CEA8) has no caller: not ported. |
| C6 new RaceBase virtuals | **Not ported here**: slot 10 (the course's model kind / lighting) is rendering's to decide; 35..38 unused. |
| C7 RaceInput::read split | **Not ported** (refactor). |
| C8 demo music stub | **Not ported** (GT3 keeps its demo music). |
| C9 replay.cam | **Not ported** (asset name; see the cameras request 4 below). |
| M1 ghost car | **Ported, switch.** `RaceSolitaire::keepGhostCar` (new, T139; from both `virtual_23`s, L2 0x00184630 / 0x00186D5C) keeps the first car's `CarSaveEquipments` (0x1D0 bytes of its `CarEquipments`), condition, transmission, steering assist, ASM, TCS; `RaceSolitaire::virtual_21` (L2 0x00184FF8) writes them into `RaceInfoBlock::slots[0]` (info, condition, the four option bytes; the tyre byte stays `virtual_7`'s) once a ghost was kept, else GT3's transmission-only write. **The ghost file is unchanged:** `RaceInfoBlock` is still 0x191C bytes with GT3's `RaceInfoSlot` (0x280) layout - the fields written are the ones `RaceBase::virtual_7` already fills and `RaceBase::virtual_19` reads back for the ghost car; only what slot 0 holds differs. `RaceSolitaire` grew by 0x200 (the block; `ghostKept` guards a save without a ghost). |
| M2 RacePOD / RacePOD2P | **Ported** (below). |
| M3 RaceTraining | **Not ported** (PLAN.md: GT Concept's licence menus only). |
| M4 RaceLinkDemo / Multilink | **Not ported** (PLAN.md). |
| M5 renamed / refactored classes | **Not ported as such**; what the Pod race needs: hud's `RacePODDisplay` (T141) / `RacePOD2PDisplay` (T155_race_pod2p_display.h), and `RaceSplitBattle::podDisplay` standing in for Lupo 2's `RaceSplitBattleBase::getDisplay`. |
| P1 replay options | **Not ported** (removed in Lupo 2; GT3 keeps them). |
| D1 afterimage | **Ported, switch (a).** `captureAfterimage` keeps `gs_get_field_mode()` in the new `RacePS2Base::afterimageField` (GT3's alignment byte +0xBCCD: no size change); `drawAfterimage` reads the planes half a line up (current field `PGL_FIELD_HALF_ODD`) or down when the field differs, and draws nothing while the race runs in background mode (behind a short menu; part of the same Lupo 2 function, L2 0x0017EDDC). **(b) not ported**: Lupo 2 draws the trail after the 2D overlays (`virtual_46`); GT3's leaves the HUD sharp. |
| Wrong pairs / noise / Delta lines | Informational (no port). |

### M2: the Pod race

- **Rules** (`race_pod.cpp`, `race_pod_rules.h`; Lupo 2's conductor members
  L2 0x001F75C8..0x001F7DBC): `pod_rules_init` (L2 0x001F75C8), `_on`
  (0x001F7708), `_area` (0x001F7720), `_set/get_stop_point` (0x001F7788 /
  0x001F7840), `_pending` (0x001F79D0), `_stop_done` (0x001F7A38), `_lap_check`
  (0x001F7B20), `_step` (0x001F7BF0). Lupo 2's set / get stops (0x001F7908 /
  0x001F7980) have no callers: not ported. The state is `g_podRules`, bound to
  the conductor it was set up for (Lupo 2: dc +0xF088..; GT3's conductor is
  physics' and full). Not in Lupo 2: a lap length of 0 leaves the rules off
  (Lupo 2's wrap loop would not end).
- **Where they run:** `pod_rules_init` after the conductor's `initialize` in
  `RaceBase::virtual_9` (Lupo 2: its `virtual_20`, inside `initialize`; the
  rules read only the race rules and the lap length, so the moment is
  equivalent); `pod_rules_step` after `dc->update(0)` in
  `RaceBase::virtual_29` (Lupo 2: the per-frame slot of SinglePlayer /
  Battle2P inside `update`, before the gap table and ranks, none of which the
  step reads). Difference: Lupo 2 runs the step only for those two conductors;
  here every race's (it does nothing unless the rules are on, which needs
  licenseType 5). `pod_rules_lap_check` must be called by the conductors' lap
  end: **physics' hook** (below). Without it the stops work (messages, "sat
  down", the AI's stops) but a missed stop is never punished.
- **Messages / events:** `CARMSG_POD_STOP` / `CARMSG_POD_RETURN` through
  `DynCar_setMessage`; `RD_INFO_POD_SAT_DOWN` (information event, the car) for
  a car its driver drives, not with 2 cars (Lupo 2); `RACE_EVENT_CAR_
  DISQUALIFIED` (arg the car, delay 0) and `car_mode_stop_roll` at a lap with
  two stops owed (the lap taken back).
- **Classes** (`race_pod.cpp`): `RacePOD` = `RaceArcade` + `RacePODDisplay`
  (hud, T141) as the race display with the player's pad (L2 0x0018D498);
  `RacePOD2P` = `RaceSplitBattle` + `RacePOD2PDisplay` (hud, T155) as the race
  display and as `RaceSplitBattle::podDisplay` (new member in GT3's alignment
  +0x3EC58; `updateDisplay`, `render` (new `setDisplayTarget`) and `virtual_45`
  drive it when set). Names "RacePOD" / "RacePOD2P"; `operator new / delete`
  from the race pool; registry records `g_racePODInfo` / `g_racePOD2PInfo`
  with create / construct, registered by the file's static initialiser.
  Sizes (with this wave's displays, size checks in race_pod.h, probed):
  `RacePOD` 0x3F540 (`RaceArcade` 0x34C00 + `RacePODDisplay` 0xA91C, rounded
  to 64), `RacePOD2P` 0x549C0 (`RaceSplitBattle` 0x3F740 + `RacePOD2PDisplay`
  0x15244): the largest registered race class (see "integrator" below).
- **Runners** (`race_pod_runners.cpp`): `int run_pod_race(MenuArgs *args)`
  (GT3's `run_arcade_race` with `RacePOD`, no arcade record update and no
  prize-car check, as Lupo 2's) and `int run_pod_2p_race(MenuArgs *args)`
  (GT3's `run_split_battle` with `RacePOD2P` on `args->race` instead of
  "yotsuya001"). As the menus' enhanced arcade runners: an arcade car takes
  the player's own assist level (`args->assist1` / `assist2`,
  `MENU_ASSIST_OPTIONS` = the options' ASM / TCS; `pod_entry_assist`, the
  same as T054's `set_entry_assist`), a CVT runs automatic
  (`isCVT() != 0` -> `setEntryTransmission(0, entry)`), and the short menus
  hide "load_ghost" and "demonstration" (`setNoLoadGhost(1)` /
  `setNoDemo(1)`: no ghost and no demo for the Pod race). Lupo 2's own
  runners (L2 0x0012E018 / 0x00130CB8) follow GT Concept's flow (running
  car, race behind its short menu, setting menu, rewards, kiosk idle limit):
  not taken.

**What starts the Pod race (none of it is in GT3's data):**
1. a race record with `licenseType` (RaceEvent +0x19A, `RaceRuleOptions` +2)
   = 5 on a course whose lap is at least 641 m;
2. that course's stop-area road triangles with attr2 bit 2 (`RunwayContact.
   attr2 & 4`) at 608..641 m (Lupo 2's Pod course with its jump ramp);
3. the Pod car (`pod_tmv_01`, specialKind 1) - only it sits down
   (`podParking` == 0x22, physics), so no other car can complete a stop;
4. the arcade menu's Pod entry calling the runners (menus), with Lupo 2's
   Pod race codes (the GCF category `CLASS_PODRACE`, and "APO20001" for 2P);
5. for the texts: hud's English RACEDISPLAY 56..62 (in the build).
Without 1 nothing changes for any race; without the menu entry the classes
are registered but never created.

## Entries: backend.md

| Entry | Outcome |
|---|---|
| B1 Driving Force Pro | **Game side ported (no switch: only Lupo 2's PDIUSB.IRX reports the device).** `USBSteeringPort::update` (C298 centres itself), `translate` (USB type 4, the C294's buttons with Lupo 2's four moves, no menu steering), `getSteerLevel` 0; `RaceInput::readWheel` type 4 (device `STEER_WHEEL_DFP` = 6, `0x1000 - x / 2`), `emulatePressure` type 4 = the C294's button config (L2 0x0021D248). The IRX itself is the user's to test (backend.md B1). |
| B2 wheel spring | **Ported (no switch).** `PortBase` slots 6..8 (`getSteerLevel`, `enableSpring`, `setDamper`; defaults empty), `SteeringPSPort` forwards them, `USBSteeringPort`: `setupSpring` (L2 0x0023F5B8, three steps before the Logitech centring; `springSetup` in GT3's spare byte +0x69), `enableSpring` (`ff_default_spring(port, 0x20, on)`), `setDamper` (no caller); `resetCentring` restarts the set-up. Race side: C5 above. |
| B3 menu steering | **Ported, switch.** `usbWheelSteerButtons` (L2 0x0023F348, Logitech and 0x6F8 wheels), `onConnect` sets the dead zone 0x90 / 0xC0 / 0 and the level step (`steerDeadZone` / `steerStep` in GT3's spare bytes +0x6C / +0x70; `init` sets Logitech's for an i.LINK node's port, which never connects locally); `getSteerLevel` (L2 0x0023FFD8); `Control::getRepeat` (T474, L2 0x0023D658) counts in eighths: left / right with a turned wheel add its level 1..8 and skip the first delay. Not in Lupo 2: a Control without a port counts 8 (Lupo 2 does not test for NULL). |
| B4 c C294 calibration | **Ported** under the menus' `ENH_MENU_WHEEL_CALIBRATION` (`USBSteeringPort::update`, L2 0x0023FB20): menus_b 11's C294 defaults take effect. B4 a / b (0x6F8 centring sequence, Logitech's longer centring holds): not ported (optional feel). |

## Entries: physics.md and hud.md (race items)

| Entry | Outcome |
|---|---|
| physics 27 (`car_pair_contact`) | **Physics'** (T349, race/dynamics): with A2 it gives the 4 m caution. Nothing in race/core. |
| physics 28 (licence tests) | **Not ported** (PLAN.md). |
| physics 29 (timed races end at the leader) | **Physics'**: its code is `DynamicsConductor::virtual_17` (T308, race/dynamics); nothing of it is in race/core. |
| hud C3 | race side ported (C4 b above). |
| hud C14 | race side ported (C4 a above). |
| hud C16 | The hud ties its 3 s offset to `ENH_RACE_TIME_UP_ENDS` (kept, not renamed). For the record: in Lupo 2 the -3 s pairs with its runners' `timeLimit = idleSeconds + 3` (menus_a.md 5, the kiosk idle limit, not ported); C2's +59 frames is separate (the full value shown for the first second). |

## Requests done for other areas

- **rendering:** `RacePS2Base::virtual_3` (T130) applies every car's model
  animations after the course's (`RaceCarModel::applyAnims`, Lupo 2 L2
  0x0017BA98): the Pod's face palettes in a race.
- **cameras + sound 1:** `RaceEntryCar` 0x2170 -> 0x2190 (`CarSound` + 0x20),
  T121.h.
- **cameras + sound 2:** `RaceEntryCar::updateSound` (T121) passes
  `CarSoundPodVoice` {1.0, the Doppler factor, `podSound` for a Pod else 0}
  (L2 0x001769A4..0x001769C8).
- **cameras + sound 3:** `RaceInput::emulatePressure` (T413): in replay modes
  1..3 the pad's triangle and the GT Force's button 0x10 set
  `RIB_REPLAY_ZOOM` = 0x40000000 (race_input_internal.h), under
  `ENH_CAMERA_ZOOM_BUTTON` (L2 0x0021D1CC / 0x0021D2AC).
- **cameras + sound 4 (optional, replay.cam):** not done (needs the camera
  side and the Lupo 2 file).
- **hud 1:** the size checks of the 14 race classes updated (T132, T133,
  T134 x2, T135, T136, T137, T138, T139, T140, T142, T144, T148, T151) to the
  probe's values; the comments say the member offsets stay GT3's.
- **hud 2..4:** done as above (fields `showMask` / `miniLogo` written
  directly; `RacePODDisplay`, `RacePOD2PDisplay` built, target display and
  player cars set every frame by `updateDisplay`; the event, the information
  and the two car messages). **hud 5:** the name stays.
- **menus 1:** `run_pod_race` / `run_pod_2p_race` defined (race_pod.h,
  race_pod_runners.cpp) with `MenuArgs::assist1` / `assist2` (-1 = the
  options), CVT -> automatic, and the short menus' `setNoLoadGhost` /
  `setNoDemo` (see M2 Runners). T000 calls them.
- **menus 2:** B4 c (C294 calibration) in `USBSteeringPort::update` under
  `ENH_MENU_WHEEL_CALIBRATION` (see backend B4 c).
- **menus 3:** `ResultArcade` 0x94C -> 0x96C: the probe shows no size change
  of `RaceArcade` / `RaceArcadeSingle` / `RaceArcadeDemo` /
  `RaceSplitBattle` / `RaceLinkBattle` / `RaceChampionship` (the +0x20 falls
  in their 64-byte alignment padding: 0x34C00, 0x34C00, 0x35400, 0x3F740,
  0x35680, 0x34C80 stay); `sizeprobe.py --only-diff` reports 0 differences
  of 732 entries, with the new `RacePOD` / `RacePOD2P` checks.

## Struct growth (rule 7)

- `RaceSolitaire` + 0x200 (the ghost car block), so `RaceTimeTrial`,
  `RaceFreeRun`, `RaceLicense`, `RaceMachineTest` + 0x200. No raw offsets or
  size literals of them in code (only comments); `RaceClassInfo` uses sizeof.
- `RaceSplitBattle::podDisplay`, `RacePS2Base::afterimageField`,
  `USBSteeringPort` springSetup / steerDeadZone / steerStep: in existing
  padding, no size change.
- `PortBase` and the three port classes: three virtual slots more (vtables
  only).
- `RaceEntryCar` size check 0x2190 (sound's growth).
- New `RacePOD` 0x3F540 / `RacePOD2P` 0x549C0 (race_pod.h).

## What to test in game

- **AI (any race against computer cars):** a standing start (computer cars
  keep their line through the first corner); traffic (earlier lifts, fewer
  line changes, queues); a spin into a wall (backs out); after the finish the
  cool-down lap is slow; a race with an "enemy skill" handicap in its rules
  (pace closer to the %). Compare with the AI switches at 0.
- **Time limit:** the arcade demo still ends on time (C1 now in RaceBase);
  with the hud's look, the limit shows its full value for a second (C2).
- **Replays:** camera cuts with the trail (afterimage) in field mode: no
  vertical jitter; no trail on the race behind a short menu. L1+L2+R1+R2
  together toggle the logo / car name panel (with the hud's C14). Triangle
  zooms the TV camera (cameras C5).
- **Pause (live race):** SELECT cycles none -> accel/brake monitor -> + the
  steering display -> none; the HUD changes at once.
- **Wheel (GT Force / C294, PCSX2's wheel emulation):** in the menus a weak
  centring spring; racing, no spring (only the game's force feedback); pause
  -> spring on; after the race -> on. Menus: left / right only beyond a dead
  zone, faster the further the wheel is turned. C294: the default calibration
  (menus) applies.
- **Ghost (time trial / free run):** set a best lap, change the car's
  settings (or the car), save the ghost, load it later: the ghost drives with
  the settings of the lap that set it.
- **Pod race:** only with the data above; see the log lines.

## Log lines

- `[enhanced] pod race: on, stop area 608..641 m of a N m lap` - the race start of a Pod race.
- `[enhanced] pod race: car N sat down, stop K done` - a stop counted.
- `[enhanced] pod race: car N disqualified (lap L, K stops owed)` - only with physics' lap-check hook.
- (hud) `hud: the Pod's stop-area rules on the event board`, `hud: car N disqualified (the Pod rule)`.
- `[enhanced] wheel N: default spring set up (on)` - a Logitech wheel connected (B2).
- `[enhanced] wheel N: Driving Force Pro (USB type 4)` - only with Lupo 2's PDIUSB.IRX.

## For other areas

1. **physics (race/dynamics, T308 / T309): the Pod race's lap check.** At the
   very start of `DynamicsConductor::virtual_17` and
   `DynamicsConductorBattle2P::virtual_17` (Lupo 2 L2 0x001EDB68 /
   0x001EE1B8):

   ```c
       /* The enhanced engine: the Pod race's lap check (Lupo 2 L2 0x001F7B20):
        * a lap with two stops owed does not count (race_pod_rules.h) */
       if (pod_rules_lap_check(this, car) == 0) {
           pod_set_emotion(getCar(car), 7, 0xFF);   /* Lupo 2: sad, held (L2 0x001E1308) */
           return;
       }
   ```

   (`#include "game/race/modes/race_pod_rules.h"`; `pod_set_emotion` = the
   physics port's setter of L2 0x001E1308, whatever its name.) Lupo 2 skips
   everything of the lap end then (lap time, finish, faults, lapStartTime).
2. **physics:** `RaceCarState::frontSlip` (A7) in the units of
   `TyreData::peakSlipAngle` (rad, the front wheels' mean slip angle as L2
   0x001E5B78 averages it); `podParking` must reach 0x22 for a stopped Pod
   with the side brake at 1.0 (L2 0x001E1C18 / 0x001E1C88) for computer cars
   too - the AI pulls `st.handbrake = 1.0` in `ai_apply_outputs` while it
   waits; the message duration table to 18 entries before kinds 0x10 / 0x11.
3. **physics 22 b:** with `ENH_PHYSICS_DISPLAY_FLAGS` the AI's
   `ai_apply_outputs` no longer writes `st->steering` (Lupo 2 L2 0x00201A88
   has no such store): the physics port must write the computer car's
   steering display byte (L2 0x001F96C0 from `car_input_update`), or the
   switch's 1 leaves it stale.
4. **physics (optional):** `car_mode_stop_roll` is declared only in
   `car_input_internal.h`; race_pod.cpp declares it itself. A declaration in
   `T341_gear_mode.h` would be cleaner.
5. **physics 19 / 20:** `RaceInput::readWheel` gives a Driving Force Pro
   `CarInput::device` 6 (`STEER_WHEEL_DFP`, race_input_internal.h) and the
   USB type 4 (`USB_WHEEL_DFP`, T474.h) for the force-feedback set.
6. **menus (the arcade Pod entry, menus_a.md 5 / 9 / 14; done: T000 calls
   them):** call
   `int run_pod_race(MenuArgs *args)` (Pod race 1P: `args->race` the Pod
   race code from the GCF `CLASS_PODRACE` category, `args->course`,
   `color1` / `tyre1` / `transmission1`) and `int run_pod_2p_race(MenuArgs
   *args)` (2P: `args->race` "APO20001" in Lupo 2, `args->course`, both
   players' colour / tyre / transmission), `#include
   "game/race/modes/race_pod.h"`. Lupo 2's decode: mode 6 (`POD_RACE`) ->
   selection 7 (`podRace` 1) / 8 (`podRace` 2). Offer it only when the Pod
   car, course and race records are present. Also: `KeyConfigItem` (T101)
   and other menu code that tests USB types 1..3 now may see type 4 (only
   with Lupo 2's IRX).
7. **integrator:** two new objects: `race_pod.cpp` (static initialiser
   `_GLOBAL_$I$g_podStopStart`, frame info `_GLOBAL_$F$g_podStopStart`) and
   `race_pod_runners.cpp` (frame info `_GLOBAL_$F$run_pod_race__FP8MenuArgs`
   only). `RacePOD2P` (0x549C0) is larger than every GT3 race class: its
   registration raises `g_00351DF0`, so T000's race work area
   (`(g_00351DF0 + 0x3F) & ~0x3F`) grows by about 0xAE00 over
   `RaceTimeTrial`'s 0x49BC0 - check the memory map has room. The runner
   keeps it on the main stack (1 MB, `_stack_size`) as GT3's
   `run_split_battle` keeps `RaceSplitBattle`.
