# Survey: race logic, AI and organisation

Area: `src/game/race/core/`, `src/game/race/modes/`, `src/game/race/ai/`,
`src/game/race/pause/` (race set-up, organisation, entries, events, replays,
the race modes, the computer drivers and their lines, the pause and replay
menus). Lupo 2 addresses are `L2 0x...`, GT3 US addresses `US 0x...`. Lupo 2's
gp is 0x00362B70, GT3's 0x003579F0 (`gp-0x...` constants below are read
with them).

## Summary

Candidates examined: all 340 paired functions at difference >= 0.25 with
sizes within 2x (170 core, 127 modes, 11 AI, 32 pause), the 24 pairs over
2x, and all 290 unpaired Lupo 2 functions placed in the four folders (175 +
107 + 7 + 1), plus the GT3 functions of the area the map left without a twin
(337, most of them leaf functions Lupo 2 has no frame record for). About 110
pairs were read side by side (`dis` / `dis --us` with both gp values, against
our C); the rest were decided on their call lists, constants and strings
(`build/scratch/survey_race_logic/calls.txt`). Every Lupo 2 race class's
vtable was aligned against GT3's. 25 tool pairs are wrong (list below).

GT Concept keeps every GT3 race class (the registry has all 14 GT3 classes)
and adds four: `RaceTraining`, `RaceLinkDemo`, `RacePOD`, `RacePOD2P`.

**Game-logic changes worth porting (computer drivers and ghosts):**

1. **AI throttle handicap as a share of the engine's net torque**
   (`ai_brake_factor`): a throttle scale under 1 (race-rule handicap, the
   cool-down lap) becomes (s x torque + friction) / (torque + friction), so
   engine braking no longer makes a handicapped car slower than intended.
2. **AI caution far more sensitive** (`ai_note_closing`): full caution from
   5 km/h closing speed (GT3 36 km/h), half under 5 s to contact (GT3 3 s,
   from 0.1 km/h instead of 1 km/h), quarter under 4 m (GT3 2 m).
3. **AI traffic line changes calmer** (`ai_avoid_traffic`): 180 frames
   between caution-driven changes (GT3 30), no change while the last one is
   still blending in, the line timer no longer blocks the return to the
   racing line, a slow car (< 30 km/h) on the pit approach is not throttled.
4. **AI start look-ahead**: computer cars in a standing or slow rolling start
   (launch speed < 60 km/h, not dirt, not the Pod race) look up to 200 m
   further ahead during the first 5 s (GT3: 40 m over 2 s for everyone).
5. **AI pit exit**: look-ahead 5 m below 30 km/h (GT3 always 20 m).
6. **AI recovery**: a turning car that hits something in front backs up also
   when it points more than 60 deg off the line's own direction (new helper
   L2 0x00201668, the line's heading at a distance).
7. **AI steering correction eased near the tyres' peak slip**: the yaw-rate
   gain is scaled by (1 - frontSlip / peakSlip), clamped to +-1 (needs a new
   run-state value from the physics side).
8. **AI driver spread and rules**: the per-car spread of computer cars
   (corner speed, braking, corner km/h, steering gain by car index) applies
   in every race (GT3: only in conductor mode 2); the race rules'
   `enemySkillC2` corner-speed offset is no longer read (removed in Lupo 2).
9. **AI cool-down after the finish** (`ai_reduce_pace`): throttle scale
   halved but not below 0.4, corner speeds x0.5 (GT3 x0.8), braking x0.8
   (GT3: throttle 0.5 flat, corners x0.8).
10. **Best-lap ghost keeps the car of the best lap** (`RaceSolitaire` /
    `RaceFreeRun`, so time trial, free run and machine test): when a new best
    lap becomes the ghost, its car specification, condition and the aids
    (transmission, steering assist, ASM, TCS) are saved and written into the
    saved ghost block; GT3 keeps only the transmission.

**Race organisation (small, port optional):**

11. **Time-up ends every race** (end code 5): moved from
    `RaceArcadeDemo::virtual_27` into `RaceBase::virtual_27`.
12. **Time limit + 59 frames** (`RaceOrganization::initRaceState`).
13. **Replay / pause HUD controls** (with the HUD area): in replays the
    shoulder buttons toggle a new `RaceDisplay` flag; SELECT while paused
    cycles two HUD panel bits (3 states).
14. **Pause / race start / race end told to the controller port**
    (`RaceBase` paused setter, `RaceInput::setMode`, new wheel-port virtuals)
    — input / force-feedback area.

**New modes (described, verdict per entry):**

15. **`RacePOD` / `RacePOD2P`** — the Toyota Pod race: a stop area (608..641 m
    on the course) to stop in with all wheels and make the Pod sit down by
    pulling the side brake, every lap; overrun and missed stops cost a lap;
    computer cars do it too. New mode class, display, rule engine in the
    conductor, AI, two car messages, one race event. **Do not port now**
    (needs the Lupo 2 course with the jump ramp, race data and a menu entry).
16. **`RaceTraining`** — a licence-style race with a second, computer-driven
    car (race DB entry 2), retry pause, licence display and BGM, a conductor
    with car-to-car collisions. **Do not port by default** (new mode; needs
    race codes with an entry-2 car and a menu entry).
17. **`RaceLinkDemo` / `Multilink`** — the i.LINK battle rewritten around a
    `Multilink` link layer, plus a demo mode for linked consoles. **Do not
    port** (i.LINK; no game logic GT3 can use).
18. `SimpleExamDisplay` / `SimpleExamMessages` / `MultilinkInput` /
    `MultilinkLocalInput` / `RaceSplitBattleBase` are **renames or
    refactors** of GT3 classes (time-trial disqualification display, link
    inputs, split battle), nothing to port on their own.

**Removed in Lupo 2 (GT3 keeps its version):** the race BGM's demo-music
path; the replay option rows *target* (manual / top / auto) and the *tyre*
item, and the replay display modes *broadcast* / *crs*; the rules'
`enemySkillC2`; the unused AI speed-step debug adjustment.

**Delta review (map v2, at the end):** one more change, for the rendering
area: the replay afterimage (`captureAfterimage` / `drawAfterimage`, race/core
T131) is field-mode aware and drawn after the 2D overlays (D1); this corrects
the first report, which had the afterimage dropped. The other 17 delta lines
are noise, wrong pairs, Pod / replay-option code already covered, or another
area's code (a menu request record and save code placed here by neighbour).

No change was found in the race organisation's entries, ranking, lap and
section timing, event queue logic, replay data format (other than the ghost
slot contents), the championship / arcade / free practice / licence / machine
test flows, or the enemy-line geometry.

---

## AI changes

### A1. Throttle handicap as a share of net engine torque

| Field | |
|---|---|
| What | `ai_brake_factor` multiplies its result by the driver's throttle scale (1, the rules' `enemySkillA` %, 0.5 after the finish). Lupo 2 converts a scale s < 1 into the pedal position that gives s of the engine's full-throttle net torque: s' = (s x engineTorque + frictionTorque) / (engineTorque + frictionTorque) (unchanged when the sum is 0). With engine braking the old linear scale gave less drive than intended. |
| Where | L2 `ai_brake_factor` 0x00200C20 (end, 0x0020134C..0x00201398); GT3 `ai_brake_factor` US 0x001F7088 (`src/game/race/ai/T366_ai_drive.cpp`). |
| Kind | changed logic |
| Evidence | `lwc1 $f1,0x6C(params)`; `c.lt.s $f1,1.0`; `lwc1 $f4,0x4B8($s5)`, `lwc1 $f2,0x4B4($s5)`, `add.s`, `c.eq.s` 0, `mul.s; add.s; div.s`. L2 run state +0x4B4 / +0x4B8 are `engineTorque` / `frictionTorque` (GT3 +0x4B0 / +0x4B4): written by L2 0x001E9768 (= GT3 `car_pre_step` US 0x001EA428) from `engine_torque_at` and the `engine_scale` twin. |
| Port | Three lines at the end of `ai_brake_factor` (GT3 fields `st->engineTorque`, `st->frictionTorque`). No data. |
| Verdict | **port** |

### A2. Caution thresholds

| Field | |
|---|---|
| What | The caution a closing car causes: 1.0 when closing faster than 5 km/h (GT3 10 m/s = 36 km/h); 0.5 when closing faster than 0.1 km/h (GT3 1 km/h) and under 5 s to contact (GT3 3 s); 0.25 under 4 m (GT3 2 m). Computer cars lift and change line much earlier. |
| Where | L2 0x002007A8 (`ai_note_closing`, leaf); GT3 `ai_note_closing` US 0x001F6CC0 (`T365_ai_traffic.cpp`). |
| Kind | changed logic (constants) |
| Evidence | L2 gp-0x7070 = 1.38889, gp-0x706C = 0.0277778, `lui 0x40A0` (5.0), `lui 0x4080` (4.0); GT3 `lui 0x4120` (10.0), gp-0x7418 = 0.2777778, `lui 0x4040` (3.0), `lui 0x4000` (2.0). |
| Port | Four constants. |
| Verdict | **port** (with A3: the two were tuned together) |

### A3. Traffic line changes

| Field | |
|---|---|
| What | `ai_avoid_traffic`: (a) after a caution-driven line change the next one waits 180 frames (GT3 30); (b) no line change while `lineBlend` > 0 (the previous change still blending); (c) while `lineTimer` runs the caution is ignored but the function goes on (GT3 returns), so the return-to-racing-line and move-away-from-a-car-beside checks still run (gated by (b)); after a caution change it also goes on instead of returning, without the caution lift; (d) on the pit approach (line 3) with cars beside and ahead on one side: below 30 km/h the line timer is held (`lineTimer++`) instead of halving the throttle. |
| Where | L2 0x00200848; GT3 `ai_avoid_traffic` US 0x001F6D70 (`T365_ai_traffic.cpp`). |
| Kind | changed logic |
| Evidence | `addiu $v1,$zero,0xB4; sh $v1,0x9A($s0)` (GT3 `li 30`); `lwc1 $f0,0x94($s0)` / `c.lt.s 0,$f0` / `bc1tl … daddu $s5,$zero` (mayChange = 0); `blez … lwc1 $f20,0xB0` then `mtc1 $zero,$f20` (caution 0 while the timer runs, no early return); gp-0x7068 = 8.33333 with `lhu 0x9A; addiu 1; sh` in the pit-approach branch. |
| Port | Rewrite of `ai_avoid_traffic`'s control flow (same fields). |
| Verdict | **port** |

### A4. Start look-ahead

| Field | |
|---|---|
| What | `ai_drive`'s look-ahead on the line gets a start bonus: GT3 adds (2000 - t_ms) x 40 / 2000 m for every car. Lupo 2, for computer cars (`driver` 2) on a non-dirt course outside the Pod race with the rules' `launchSpeed` < 60 km/h (standing and slow rolling starts), adds (5000 - t_ms) x 200 / 5000 m instead; other cases keep GT3's. Computer cars hold their line through the first corners of a standing start instead of weaving. |
| Where | L2 0x00201B50 (`ai_drive` twin, 966 insns) at 0x00201D38..0x00201E24; GT3 `ai_drive` US 0x001F7AE0 (`T366_ai_drive.cpp`). |
| Kind | changed logic |
| Evidence | `lb 0x3ED(st)` == 2 (L2 `driver`, GT3 +0x3E9); `jal 0x001F0C28` (L2 `isDirtCourse`; the tool's twin is wrong, see the wrong pairs); `jal 0x001F7708` (Pod mode); `lbu -0x1380(dc+0x10000)` = dc +0xEC80 = `RaceRuleOptions.launchSpeed` (the rules start at dc +0xEC80 in L2: `DynamicsConductor::attach` L2 0x001F12A8 calls `setFromRace(dc+0xEC80)`), `sltiu 0x3C`; `li 0x1388`, `lui 0x4348` (200.0), gp-0x7040 = 5000.0. |
| Port | A second branch in `ai_drive`'s look-distance code (fields exist in GT3: `st->driver`, `dc->rules.launchSpeed`, `dc->clock`, `isDirtCourse`). |
| Verdict | **port** |

### A5. Pit-exit look-ahead

| Field | |
|---|---|
| What | Leaving the pit box (`ctrlMode` 4, `inPitLane`), the look-ahead is 5 m below 30 km/h, 20 m above (GT3 20 m always). |
| Where | L2 0x00201B50 at 0x00201CC4..0x00201CF8; GT3 `ai_drive` US 0x001F7AE0. |
| Kind | changed logic |
| Evidence | `lwc1 $f1,0x48C(st)` (speedX), gp-0x7044 = 8.33333, `lui 0x41A0` (20) / `lui 0x40A0` (5). |
| Port | One comparison. |
| Verdict | **port** |

### A6. Recovery: back up when off the line's direction

| Field | |
|---|---|
| What | In the turning-round recovery state a front hit switches to backing up when the box reports the zero query, or the heading error to the look-ahead point is over 60 deg (GT3), **or** the car's yaw is more than 60 deg off the direction of the line itself at the car's lap distance (new). The new helper finds a line point at the distance and one 0.5 m further (pushed on until they are 0.2 m apart, at most 10 tries) and returns `atan2f(-dx, dy)`. Note: the difference is not wrapped to +-pi. |
| Where | L2 0x00201B50 at 0x00202038..0x002020D0; new helper L2 0x00201668 (109 insns, between `ai_look_ahead` and `ai_steer_toward`); GT3 `ai_drive` US 0x001F7AE0. |
| Kind | changed logic + new function |
| Evidence | `beqz $s3` (hitFront) → `jal 0x00201668` with `lwc1 $f12,0x3E8(st)` (lapDistance) → `sub.s` yaw (0x474), `neg.s` if negative, gp-0x7010 = 1.0472 compare → `sb 1,0x80($s1)`. The helper: two `line_point_at` / `line_point_at_linear` calls, `lui 0x3F00` step, gp-0x7050 = 0.2, `slti 0xA`, `atan2f`. |
| Port | The helper (uses `line_point_at` / `line_point_at_linear`) and the extra test. |
| Verdict | **port** |

### A7. Steering gain eased by the front slip

| Field | |
|---|---|
| What | The yaw-rate correction `(wantRate - yawRate) x steerGain` uses steerGain x clamp((peakSlip - x) / peakSlip, -1, 1), x a new run-state value at L2 st+0x424 (one word after the steering angle). It is written by Lupo 2's tyre-force loop (L2 0x001E5B78: an average over the wheels it accumulates in $f24) — read as the front wheels' slip angle, INFERRED. Also: the peak slip angle and the front wheels' velocity angle are averaged over the wheels the car's wheel layout marks (bike generalisation; same values for a car). |
| Where | L2 0x00201B50 at 0x0020234C..0x002023A8; GT3 `ai_drive` US 0x001F7AE0. |
| Kind | changed logic (depends on a physics change) |
| Evidence | `lwc1 $f0,0x424(st)`, `sub.s $f0,$f0,$f21` (maxSteer), `neg.s`, `div.s`, clamps `lui 0x3F80` / `lui 0xBF80`, `lwc1 $f3,0xC(params)` (steerGain), `mul.s $f3,$f3,$f0`. Writer: L2 0x001E5F0C / 0x001E5F24 `swc1 $f24,0x424(car+0xD8)`. |
| Port | The scale in `ai_drive`, and the physics area must supply the value (what L2 0x001E5B78 averages). |
| Verdict | **port with the physics area** (only if the slip value is ported; uncertain meaning) |

### A8. Driver spread in every race; `enemySkillC2` dropped

| Field | |
|---|---|
| What | `ai_apply_rules` scales the computer drivers by the rules' per-drive-class handicaps. (a) The spread of computer cars by index (odd cars 4% slower corners, braking -0/7/14%, corner speed -0..8 km/h, steering gain +-3%) applied in GT3 only when the conductor's mode is 2; Lupo 2 applies it to every `driver` 2 car. (b) Lupo 2 no longer reads `enemySkillC2` (the corner-speed km/h offset): the offset is only the spread's. |
| Where | L2 0x001FFDE0 (unpaired twin); GT3 `ai_apply_rules` US 0x001F6380 (`T362_ai_params.cpp`). |
| Kind | changed logic (a); removed in Lupo 2 (b) |
| Evidence | GT3 `lbu 0x37AC(dc+0x8000)` == 2 before the `driver` test; L2 tests only `lb 0x3ED(st)` == 2. GT3 loads `lb 0x18(rules+idx)` (C2); L2 loads only +0xC / +0x10 / +0x14 of the rules (dc +0xEC8C.. +0xEC94) and starts the km/h at 0 (`daddu $v0,$zero,$zero`). |
| Port | (a) drop the mode test. (b) keep GT3's C2 (never remove a GT3 feature). |
| Verdict | **port (a)**; (b) not ported |

### A9. Cool-down pace after the finish

| Field | |
|---|---|
| What | `ai_reduce_pace` (car_finish, race end): GT3 sets throttleScale 0.5 and cornerScale x0.8. Lupo 2: throttleScale = max(throttleScale x 0.5, 0.4), cornerScale x0.5, brakeScale x0.8. |
| Where | L2 0x001FFF98 (unpaired twin); GT3 `ai_reduce_pace` US 0x001F6518 (`T362_ai_params.cpp`). |
| Kind | changed logic |
| Evidence | `lwc1 0x6C(p)`, `lui 0x3F00` (0.5) `mul`, gp-0x7080 = 0.4 floor; `lwc1 0x4(p)` x 0.5; `lwc1 0x0(p)` x gp-0x707C = 0.8. |
| Port | Three lines. |
| Verdict | **port** |

### A10. Pod stop area driving (computer cars)

| Field | |
|---|---|
| What | In the Pod race (A10 belongs to M2) a computer car with a stop pending brakes for a point in the stop area (start + length x {0.75, 0.5, 0.25, 0} by (rank-1)/2, table L2 0x002DF128), stores that point in the conductor (L2 0x001F7788), crawls at 10 km/h for the last 20 m, stops within 0.5 m (under 1 km/h), waits while the stop is pending, then drives on (`AIDriver` +0xAE: 0 / 1 approaching / 2 stopped). |
| Where | L2 0x00200C20 at 0x00200DC8..0x00200F9C and 0x00201200..0x00201244; L2 0x00201B50 at 0x00202564..0x002026A8. |
| Kind | new logic (Pod mode only) |
| Evidence | `jal 0x001F7708` (Pod mode = dc +0xF088 == 1), `jal 0x001F79D0` (stops pending), `jal 0x001F7720` (area), `jal 0x001F7840` (stored point), constants gp-0x6FD0 = 0.2778, gp-0x6FCC = 2.7778, `lui 0x41A0` (20 m), `lui 0x3F00` (0.5 m). |
| Port | Only with M2. |
| Verdict | do not port (with M2) |

### A11. AI parameters moved into the conductor (refactor)

| Field | |
|---|---|
| What | The 0x74-byte `AIParams` (GT3 `AIDriver` +0x0C) live in an array in the `DynamicsConductor`, one per car (L2 dc +0xECE8 + carIndex x 0x74, getter L2 0x00202CA8; +0x24 getter L2 0x00202CD8). `AIDriver` keeps its other fields at GT3's offsets. Every AI function reads the parameters through the getter (`ai_load_params`, `ai_corner_speed`, `ai_steer_toward`, `ai_brake_factor`, `ai_drive`, `ai_apply_rules`, `ai_reduce_pace`, `ai_set_downforce`). Same values. |
| Where | L2 0x00202CA8 / 0x00202CD8 (in GT3's `ai_step_speed` / `ai_step_unk99` slot); GT3 `AIDriver` (`T361_drive_params.h`). |
| Kind | changed layout, no behaviour change |
| Evidence | `lw a1,4(a0); lbu 0x4C4(a1) x 0x74 + 0xECE8 + conductor`; `ai_load_params` L2 0x001FFA20 stores the same fields with the same formulas into it. |
| Port | Nothing. |
| Verdict | do not port |

### A12. Speed-step debug adjustment removed

| Field | |
|---|---|
| What | GT3's `ai_step_speed` / `ai_step_unk99` (no callers: a debug +-10 km/h step on every corner speed) and the `speedStep x 10 km/h` term of `ai_corner_speed` are gone. No effect in GT3 (speedStep is always 0). |
| Where | GT3 US 0x001F8888 / 0x001F88F8, `ai_corner_speed` US 0x001F6678; L2 `ai_corner_speed` 0x00200138. |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | L2 `ai_corner_speed` sums `cornerScale x v + cornerOffset + speedAdj x 0.2778` with no read of `AIDriver` +0x98. |
| Port | — |
| Verdict | do not port (GT3 keeps it) |

### A13. Line queries: two optional outputs

| Field | |
|---|---|
| What | `line_point_at` / `line_point_at_linear` take two more optional pointers that receive the s16 at node +0x10 of the segment's two nodes. Every AI caller passes NULL. |
| Where | L2 0x002043E0 / 0x002047C0; GT3 US 0x001FA018 / 0x001FA368 (`T371_line_segment.cpp`). |
| Kind | new parameters, no behaviour change |
| Evidence | `beqz $s2 / sw $zero,0($s2)`, `lh 0x10(node); sw … 0($s2)`; callers `daddu $t2,$zero; daddu $t3,$zero`. |
| Port | Nothing (unless a later feature needs the node value). |
| Verdict | do not port |

---

## Race core and organisation changes

### C1. Time-up ends every race

| Field | |
|---|---|
| What | `RACE_EVENT_TIME_UP` (posted by `updateStartAndTimer` when the time limit runs out) ends the race with end code 5 in every mode: the test moved from `RaceArcadeDemo::virtual_27` into `RaceBase::virtual_27`. In GT3 only the arcade demo reacted; as far as the race classes show, only the demo sets a time limit (`RaceArcadeDemoInformation::getTimeLimit`), so the visible effect needs a timed race elsewhere (UNCERTAIN whether a GT3 menu sets one). |
| Where | L2 `RaceBase::virtual_27` 0x0017A328 (0x0017A354..0x0017A384), L2 `RaceArcadeDemo::virtual_27` 0x00181078 (now a tail call); GT3 `RaceBase::virtual_27` US 0x0017C630 (`T129_race_base.cpp`), `RaceArcadeDemo::virtual_27` US 0x00182C68 (`T135_race_arcade_demo.cpp`). |
| Kind | changed logic |
| Evidence | `li $a1,7; jal RaceEventQueue::find; … li $a1,5; jalr` (vtable entry 0x140 = Lupo 2 slot 39 = GT3 `virtual_34`). |
| Port | Move the three lines into `RaceBase::virtual_27`. |
| Verdict | port (optional; harmless with GT3's races) |

### C2. Time limit grace of 59 frames

| Field | |
|---|---|
| What | `initRaceState`: `timeRemain = timeLimit x 60 + 59` when a limit is set (GT3 x 60), so the shown limit starts at the full value for a second. |
| Where | L2 0x001784A8 (unpaired twin; the tool pairs it with `DynamicsConductor::attach`); GT3 `RaceOrganization::initRaceState` US 0x0017A578 (`T125_race_organization.cpp`). |
| Kind | changed logic |
| Evidence | `addiu $t0,$v1,0x3B; beqz timeLimit; sw $t0,0x68`. |
| Port | One line. |
| Verdict | port (optional, with C1) |

### C3. New race event 0x0B (Pod), later codes + 1

| Field | |
|---|---|
| What | Lupo 2 inserts a race event at 0x0B, posted by the Pod rule when a lap does not count (M2); every later code moves up by one (`LAP_TIME` 0x0C, …, `BGM` 0x15, `VIEW_MODE` 0x16, `RECORD_FULL` 0x17). The event queue grew one word (+0x818 → +0x81C). |
| Where | Posted by L2 0x001F7B20; codes in L2 `getEventType` linkonces (0x002C99C0 = 0x0C …), `RaceSinglePlayer::virtual_28` L2 0x00180320 (`li 0x17`), `updateStartAndTimer` L2 0x00178530 (`li 0x15`); GT3 enum `T122_race_event_queue.h`. |
| Kind | new data (enum) |
| Evidence | as listed. |
| Port | Only with M2 (append the event at the end instead of renumbering). |
| Verdict | do not port (with M2) |

### C4. Replay / pause HUD controls (hooks in RaceBasic)

| Field | |
|---|---|
| What | (a) In a replay (`replayMode` != 0) a new press of one of the four shoulder buttons toggles a new `RaceDisplay` byte (L2 +0xA8CD; GT3's flags end at `finishDisplay` +0xA478 = L2 +0xA8CC) that gates the fader and some panels in `RaceDisplay`'s draw. (b) While paused in a live race, SELECT (pressed alone) cycles two `RaceDisplay` panel bits (word L2 +0xA8D0, bits 0x40 and 0x10) through none → 0x40 → 0x40+0x10 → none (table L2 0x002DD390 {1, 3, 0, 0}) and updates the display with dt 0; called from the paused update of `RaceBasic`, `RaceMachineTest` and `RaceFreeRun`. (c) A flag at `RaceBase` L2 +0x9694 (set only by the app's saved-replay player L2 0x00101240) makes START end the race (code 2) instead of pausing. |
| Where | L2 `RaceBasic::virtual_28` 0x0017F3F8 (0x0017F4BC..0x0017F4E8 and 0x0017F548..0x0017F62C); new L2 0x0017F9A0 (called by `RaceBasic::virtual_30` L2 0x0017F648, `RaceMachineTest::virtual_30` L2 0x0018AB58, L2 0x00186F78); RaceDisplay accessors L2 0x001936B0 / 0x001936D0 / 0x001936F0 / 0x00193708; GT3 `RaceBasic::virtual_28` US 0x001811B8 / `virtual_30` US 0x001812D8 (`T132_race_basic.cpp`). |
| Kind | new logic (HUD control) |
| Evidence | `__dynamic_cast` to `RaceDisplay` (typeinfo fns L2 0x002C6358 / 0x002C3F78); `lbu 0xA8CD; xori 1; sll 8; or; sw 0xA8CC`; SELECT: control slots 4 / 5 with mask 0x20, `lui 0x2E; addiu -0x2C70` table. |
| Port | Belongs with the HUD area's `RaceDisplay` changes (what the bits hide). The race side is two small hooks. (c) not (app attract mode). |
| Verdict | port with the HUD area (a, b); do not port (c) |

### C5. Pause and race state told to the controller

| Field | |
|---|---|
| What | A new `RaceBase` setter (L2 0x0017A650) stores `paused` (+0x9690) and, in a live race, passes it to the player's `RaceInput` (`setMode`, L2 0x0021CE60), which calls a new virtual of the controller port (`PortBase` grew 6 → 9 slots; only the PS and USB wheel ports implement 6..8). `RaceBasic::setPause` goes through it; `RacePS2Base::virtual_41` (the race loop) sets mode 0 before the race and 1 after it. A second `RaceInput` helper (L2 0x0021CEA8) calls port slot 8. Presumably the wheel's force feedback / centring per race state (UNCERTAIN; input area). |
| Where | L2 0x0017A650, `RaceBasic::setPause` L2 0x0017F870, `RacePS2Base::virtual_41` L2 0x0017B110, `RaceInput` L2 0x0021CE60 / 0x0021CEA8; GT3 `RaceBasic::setPause` US 0x001814D8, `RacePS2Base::virtual_41` US 0x0017D3D8. |
| Kind | new logic (input) |
| Evidence | `sw $a1,-0x6970(+0x10000)` after `jal 0x0021CE60`; `RaceInput` +0xC port, vtable entry 0x40 / 0x48 (slots 7 / 8). |
| Port | Only with the input area's wheel changes. |
| Verdict | for the input area |

### C6. New RaceBase virtuals

| Field | |
|---|---|
| What | `RaceBase` 41 → 46 slots, `RacePS2Base` 56 → 63. New: slot 10 (L2 0x00179C98): 3 when the course record (car database table 0x1F, byte +0x1F7, L2 0x00247EF0) has a flag, else 0 — used by `RaceBase::virtual_8` (the cars' model kind) and `RacePS2Base::drawView` (rendering); slots 35..38 (L2 0x0017A738..0x0017A750): empty / return 0, never overridden, reached only from four new `RaceOrganization` wrappers (L2 0x00178FD8, 0x00179020, 0x00179068, 0x001790B0) that nothing calls; `RacePS2Base` slots 58 (L2 0x0017E870, a third course draw pass) and 62 (L2 0x0017DBD0, the course's projected-shadow flag) — see `rendering.md`. Every other slot keeps its function: Lupo 2 slot = GT3 slot for 0..9, + 1 for GT3 10..33, + 5 for 34..52, + 6 for 53..55, + 7 from 56 (subclass slots). One subclass override is new too: `RaceLinkBattle` overrides slot 39 (L2 0x00183650, link session; GT3 inherits `RaceBase::virtual_34`). The slot-by-slot pairs are in `build/scratch/survey_race_logic/pairs_fix.csv`. |
| Where | vtables L2 `_vt$8RaceBase` / `_vt$11RacePS2Base` against GT3's. |
| Kind | new functions |
| Evidence | slot-by-slot alignment (`build/scratch/survey_race_logic/vtalign.py`): every slot except 10, 35..38, 58 and 62 has a GT3 twin of the same shape. |
| Port | Slot 10 with the rendering area (model kind / lighting); 35..38 nothing. |
| Verdict | slot 10 for rendering; 35..38 do not port |

### C7. RaceInput read split into virtuals (refactor)

| Field | |
|---|---|
| What | GT3's `RaceInput::read` (pad + wheel + record, or play back) becomes `read` (L2 0x0021CC70) calling two new virtuals: slot 11 play the record unless held (L2 0x0021CD00), slot 12 read pad and wheel and record (L2 0x0021CD28 → `readPad` L2 0x0021DA90 / `readWheel` L2 0x0021DCA8). `MultilinkInput` overrides slot 11 (network input). Same behaviour for GT3's inputs. |
| Where | GT3 `RaceInput::read` US 0x00210740, `readPad` US 0x00211430, `readWheel` US 0x002115F8 (`T413_race_input.cpp`). |
| Kind | refactor, no behaviour change |
| Evidence | call lists match GT3's read / readPad / readWheel. |
| Port | Nothing. |
| Verdict | do not port |

### C8. Race BGM: demo music removed

| Field | |
|---|---|
| What | `RaceBGMPS2::isDemoWithoutPreview` is a stub returning 0, so the demo race never switches to the demo music group 0x22 and an ended song always starts the next one. `getVolume` is split in two (option test L2 0x0017BF88, volume L2 0x0017BFF0), same options (+0xC3C race, +0xC84 replay). |
| Where | L2 0x0017BC70 (`jr $ra; daddu $v0,$zero`), `RaceBGMPS2::update` L2 0x0017BCB0, `play` L2 0x0017C020; GT3 `isDemoWithoutPreview` US 0x0017DD80, `getVolume` US 0x0017E0D8 (`T130_race_ps2_base.cpp`). |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | as listed; the song volumes 0.5 / 0.7 are unchanged. |
| Port | — |
| Verdict | do not port (GT3 keeps its demo music) |

### C9. Camera file renamed

| Field | |
|---|---|
| What | The global course-camera file "cameras/attach.cam" is "cameras/replay.cam" in Lupo 2 (same loader, same use). |
| Where | L2 `RaceCourse::loadCameras` 0x0018D900; GT3 US 0x0018F740 (`T153_race_course.cpp`). |
| Kind | new data (asset name) |
| Evidence | strings. |
| Port | — |
| Verdict | do not port |

---

## Race mode changes

### M1. Best-lap ghost keeps the car of the best lap

| Field | |
|---|---|
| What | When the pad's best lap becomes the ghost (`RaceSolitaire::virtual_23` and its `RaceFreeRun` override), Lupo 2 also copies the first car's `CarEquipments` (0x1E0 bytes), its accumulated status / condition block (0x20 bytes, `RaceEntryCar` L2 +0x26AC) and its transmission, steering-assist type, ASM and TCS into the race object (L2 +0x22420..+0x2261C); the best-lap save extension (GT3 `RaceSolitaire::virtual_21`) writes them all into the info block's slot 0 (GT3 writes only the transmission byte). Loading a saved ghost (`RaceBase::virtual_19`, unchanged) then rebuilds the ghost with the car that set the lap even if the player changed car or settings later. Applies to `RaceTimeTrial`, `RaceFreeRun`, `RaceMachineTest` (all `RaceSolitaire`s). `RaceSolitaire` grew by two car blocks (constructors at L2 +0x22220 / +0x22420, `CarCondition` at +0x223F0 / +0x225F0); the first one is the data logger's (`logger_run`, L2 0x00184768 / 0x001848B0, debug). |
| Where | L2 `RaceSolitaire::virtual_23` 0x001845D8, `RaceFreeRun::virtual_23` 0x00186CD0, slot 22 L2 0x00184FF8 (= GT3 `RaceSolitaire::virtual_21`), constructor L2 0x00183F70; GT3 US 0x00187430 / 0x00189538 / 0x00187968 / 0x00186E10 (`T139_race_solitaire.cpp`, `T142_race_free_run.cpp`). |
| Kind | changed logic + new fields |
| Evidence | copy loops `+0x20..+0x1E0` of `entries->cars[0]`, `ldl/ldr 0x26AC..0x26CB`, `lw 0x268C/0x2694/0x2698/0x269C` → `sw 0x2610..0x261C`; the save: `sw 1,0x1918(blk)`, copy to `blk+0x518`, `sdl/sdr 0x6E8..0x707`, `sb 0x710/0x712/0x713/0x714`, `Manager::save`. |
| Port | Fields in `RaceSolitaire` (a `CarEquipments`-sized block, the status block, four ints), the copy in both `virtual_23`s, the slot-0 write in `virtual_21` (through `RaceInfoSlot`'s fields). No asset. Check the replay / ghost save size is unchanged (slot 0 already exists). |
| Verdict | **port** |

### M2. RacePOD and RacePOD2P (the Toyota Pod race)

| Field | |
|---|---|
| What | A race on a course with a stop area before a jump ramp. While a car has a stop pending (one per lap: laps run minus stops done, L2 0x001F79D0), it has to stop with all four wheels on the stop-area surface (wheel flag 4) and make the Pod sit down by pulling the side brake (the Pod's squat state, `car` L2 +0x5F0 == 0x22, L2 0x001E1CF0). Messages: car message 0x10 "一時停止してサイドブレーキを引いてください" (stop and pull the side brake) while in the area, 0x11 "一時停止エリアに戻ってください" (go back to the stop area) when past its end; display event 7 "おすわりしました！" (it sat down) when done (not for car-only 2-player counts). Crossing the line with two or more stops pending: the lap does not count (lap - 1), the Pod looks sad (emotion 7) and race event 0x0B is posted (L2 0x001F7B20). Computer cars do the stop (A10). The start display explains the rules ("there is a stop area before the jump ramp", "stop completely there and pull the side brake", "pulling the side brake makes the Pod sit down"; L2 0x00343578..0x00343648). Stop area: lap distance 608 m, 33 m long (sdata L2 0x0035E010 / 0x0035E012); the mode is on when the race rules' `licenseType` (dc rules +2) is 5 (L2 0x001F75C8, from `DynamicsConductor::virtual_20`). `RacePOD` is `RaceArcade` with a `RacePODDisplay` (a `SimpleExamDisplay` with its own start messages, L2 0x001B4118); `RacePOD2P` is `RaceSplitBattleBase` with a `RacePOD2PDisplay` (a `RaceSplitDisplayBase`). Launched by app code L2 0x0012E018 / 0x00130CB8. |
| Where | Classes: registry entries L2 0x002DD520 {"RacePOD", 0x3E200, create 0x0018D430, construct 0x0018D468 → 0x0018D498} and 0x002DD530 {"RacePOD2P", 0x3E480, 0x0018D638, 0x0018D670 → 0x0018D6A0}, dtors 0x0018D548 / 0x0018D740, names 0x0018D420 / 0x0018D628, `__tf9RacePOD2P` 0x002C5C80, `getDisplay` 0x002C5CD0 / 0x002C5CE0; displays L2 0x001B40C8..0x001B4314; rules (conductor) L2 0x001F75C8 init, 0x001F7708 isPod, 0x001F7720 area, 0x001F7788 / 0x001F7840 per-car stop point, 0x001F7908 / 0x001F7980 stop state (no callers), 0x001F79D0 pending, 0x001F7A38 stop done, 0x001F7B20 lap check (from L2 0x001EDB18 / 0x001EE170), 0x001F7BF0 per-frame rule (from `DynamicsConductorSinglePlayer::virtual_34` L2 0x001F05E8 and `…Battle2P::virtual_34` L2 0x001EE148); car message table L2 0x002DEC38 (18 entries, GT3 16, `DynCar_setMessage` twin L2 0x001F55B8). No GT3 equivalent. |
| Kind | new class / new logic / new data |
| Evidence | as listed; vtable `7RacePOD` differs from `10RaceArcade` only in slots 0 and 25 (and the typeinfo). |
| Port | New: the two classes and displays (HUD area), the conductor's Pod fields (dc L2 +0xF088 mode, +0xF08A per-car stop counts, +0xF094 / +0xF098 area, per-car stop points +0xF09C/+0xF0BC/+0xF0DC) and the six rule functions (physics/dynamics area), car messages 16 / 17 and their HUD text, display event 7 text, race event 0x0B (append), the AI stop logic (A10), a race record with `licenseType` 5 on a course whose stop area is at 608 m (a Lupo 2 course: assets), a menu entry. Depends on the Pod car (already ported as hooks: the squat state and emotions). |
| Verdict | **do not port now** (no GT3 course / race for it; FINDINGS 12 agrees); describe for later |

### M3. RaceTraining

| Field | |
|---|---|
| What | A licence-style race (mode `RACE_MODE_LICENSE`) with two entries: the player (race DB entry 0, or 1) and a second car from the race DB's entry 2 with its colour (`RaceTrainingInformation::createEntries` L2 0x00187348, L2 0x00246938 colour), driven by the computer. It has the licence pause with retry, the replay pause, `RaceLicenseBGM`, `RaceLicenseDisplay`; its start (slot 33) posts only `RACE_EVENT_GO` (no race-music event). Its conductor `DynamicsConductorTraining` (vtable L2 0x0032DC58) is the licence conductor with: data logger supported, no slow-car boost, collisions between all cars (`collideAllCars`), the race version of the entrant info and slow-car-boost type. Launched from the arcade main loop by app code L2 0x0012F298 (race code by name, entry count 2). |
| Where | Registry L2 0x002DD450 {"RaceTraining", 0x32380, create 0x001874F8, construct 0x00187530 → ctor 0x00187560}; dtor 0x00187720, start 0x001877F0, slot 33 0x00187880, name 0x001874E8, static init 0x001878B8 / 0x001878F0; info ctor 0x001872B0, dtor 0x001872E0, slots 10 / 11 0x00187348 / 0x00187478; conductor linkonces 0x002C53B0..0x002C5440. No GT3 equivalent (nearest: `RaceLicense`). |
| Kind | new class |
| Evidence | vtable `12RaceTraining` = `16RaceSinglePlayer` + slots 0, 8, 25, 33; constructor calls `RacePauseWithRetry`, `ReplayPause`, `RaceLicenseBGM`, `RaceLicenseDisplay`, the licence conductor's constructor and stores vtable 0x0032DC58. |
| Port | Two classes, a conductor subclass, a race record with an entry-2 car, a menu entry (menus area). |
| Verdict | do not port by default (a new mode; worth it only with races designed for it) |

### M4. RaceLinkDemo and the Multilink link layer

| Field | |
|---|---|
| What | The i.LINK battle's link handling moved into a `Multilink` layer (L2 0x001B2D00..0x001B4018: node inputs, session events, `updateLink` and `drawConnectionError` twins); `RaceLinkBattle`'s constructor, destructor, start and update shrink accordingly (GT3's `setupView`, `drawWaitingForResponse`, `virtual_26..28` have their logic there); the shared random seed is no longer set in `RaceLinkBattleInformation::setupOrganization`. `RaceLinkBattleNodeInput` / `RaceLinkBattleInput` are renamed `MultilinkInput` / `MultilinkLocalInput`. New `RaceLinkDemo` = `RaceLinkBattle` with its own start, update (polls link events) and slot 51 — a demo race across linked consoles. |
| Where | Registry L2 0x002DD410 {"RaceLinkDemo", 0x34380, 0x00183890, 0x001838C8}; L2 0x00183810..0x00183AD0; GT3 `T138_race_link_battle.cpp`. |
| Kind | new class + refactor |
| Evidence | vtable `12RaceLinkDemo` = `14RaceLinkBattle` + slots 0, 8, 25, 29, 51; `RaceInput` subclasses' vtables match the GT3 ones slot for slot. |
| Port | — |
| Verdict | do not port (i.LINK, no use on GT3 today) |

### M5. Renamed / refactored classes

| Field | |
|---|---|
| What | `SimpleExamDisplay` = GT3 `RaceTimeTrialDisplay` (24 slots, same functions; new inline accessors for a phase, sub-phase, input and "stopper" used by `RacePODDisplay`); `SimpleExamMessages` = `TimeTrialExaminationMessages` (8 slots; `canEnd` / `mustStop`). `RaceSplitBattle` is split into `RaceSplitBattleBase` (the battle, the display reached through new pure virtual `getDisplay` slots 63 / 64) and `RaceSplitBattle` (owns a `RaceSplitDisplay`, now a `RaceSplitDisplayBase`; the factory function constructs the display, L2 0x00182940), for `RacePOD2P`. Same behaviour. |
| Where | vtables L2 0x0032D4E8 / 0x0032D5B8 / 0x0032C290 / 0x0032C078. |
| Kind | refactor, no behaviour change |
| Evidence | slot-by-slot vtables. |
| Port | — |
| Verdict | do not port (HUD area may want the accessors with M2) |

---

## Pause and replay menu changes

### P1. Replay options reduced

| Field | |
|---|---|
| What | The replay option window lost the *target* row (manual / top / auto) and the *tyre* item; the display row offers *traditional* and *avantgarde* only (GT3 also *broadcast*, *crs*). `refresh` / `update` are smaller accordingly (row helpers L2 0x001B4D80 / 0x001B4D98). |
| Where | L2 `ReplayOption` 0x001B4DE8..0x001B5A08; GT3 `T196_replay_option.cpp` (US 0x001AEA98..). |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | "item_tire", "target_manual / top / auto" absent from the Lupo 2 image; the page-name table at L2 0x003438B0 has two modes. |
| Port | — |
| Verdict | do not port (GT3 keeps its options) |

The rest of the pause menus (`PauseMenu`, `PauseMenuWithRetry`, `RacePause`,
`ReplayPause`) are unchanged.

---

## Wrong pairs (tool)

| Lupo 2 | tool says | is |
|---|---|---|
| 0x0017FC20 | `race_class_register` | `RaceBasic`'s registration (registers L2 0x002DD380); `race_class_register` is L2 0x001795C8 |
| 0x0018DB00 | `RaceEntryCar::loadModel` | `RaceCourse::load` (US 0x0018F930); `RaceEntryCar::loadModel` is L2 0x00175BA8 (with the bike / rider selection) |
| 0x002C5C30 | `RaceCourse::load` | `__tf7RacePOD` |
| 0x002C30E0 | `RaceEntryCar::recordSectionTime` | `__tf12VehicleModel`; the twin is L2 0x00176DF8 |
| 0x00175D38 | `RaceEntryCar::loadWheel` | `loadTires` (US 0x001776F8); `loadWheel` is L2 0x00175DA8 |
| 0x00175E10 | `file_req_run` | `RaceEntryCar::loadSound` (US 0x001777B0); `file_req_run` is L2 0x0024F980 |
| 0x0017A650 | `RaceOrganization::initRaceState` | the new `RaceBase` paused setter (C5); `initRaceState` is L2 0x001784A8 |
| 0x001784A8 | `DynamicsConductor::attach` | `RaceOrganization::initRaceState`; `attach` is L2 0x001F12A8 |
| 0x00179C98 / 0x00179EC0 / 0x00179EF8 | `RaceBase::virtual_10 / 11 / 12` | new slot 10 / GT3 `virtual_10` / GT3 `virtual_11`; GT3 `virtual_12` is L2 0x00179F58, `virtual_16` L2 0x0017A010 |
| 0x0017A950 | `RaceBase::virtual_38` | `RaceBase::signal` (US 0x0017CB80); `virtual_38` is L2 0x0017A9D0, `takeSignal` L2 0x0017A980 |
| 0x0017ABF0 | `race_mem_pool_static_dtor` | calls `~RaceInput`: probably `race_input_static_dtor` (`__tcf_0`, US 0x0017C4E0) |
| 0x00179480 | `LapRecord::updateTotal` | `LapRecord::totalDiff` (called where GT3 calls it) |
| 0x002C3A58 | `RaceOrganization::updateLapRecordTotal` | an event accessor linkonce (`RaceEventQueue::find`) |
| 0x00182910 | `race_split_battle_static_ctor` | `RaceSplitBattle`'s construct; the static constructor is L2 0x00182AA0 |
| 0x00182AA0 | `RaceSplitBattleInformation::setEquipments` | the static constructor; `setEquipments` is linkonce L2 0x002C4640 (`setPlayerCar`) |
| 0x001826C0 | `RaceSplitBattle::virtual_45` | slot 51 = GT3 `virtual_46` (US 0x00184338); `virtual_45` is L2 0x00182668 |
| 0x001828A0 | `RaceSplitBattle::virtual_46` | an `operator delete` (`Allocator::free`) |
| 0x00183858 | `race_link_battle_static_init` | an `operator delete`; the static init is L2 0x00183810 |
| 0x00183890 / 0x001838C8 / 0x00183928 | `RaceLinkBattleNodeInput` typeinfo / dtor, `__tf19RaceLinkBattleInput` | `RaceLinkDemo` create / construct / start |
| 0x00186710 / 0x001863E0 | `__tf28TimeTrialExaminationMessages` / `__tf20RaceTimeTrialDisplay` | `SimpleExamDisplay::virtual_0` / slot 23 (`endDisqualify`) |
| 0x002C4EA8 / 0x002C5118 | `RaceTimeTrial::virtual_24` / `RaceFreeRun::virtual_24` | linkonce `setFrameDeltaTime__13RaceSolitairef` (twin unknown) / `specifySubPhase__17SimpleExamDisplayUc` = `RaceTimeTrialDisplay::setSubPhase` US 0x00188E48 (map v2 has it); the name getters are L2 0x00185648 / 0x00186830 |
| 0x001F7BF0 (dynamics) | `DynamicsConductor::isDirtCourse` | the Pod per-frame rule; `isDirtCourse` is L2 0x001F0C28 |
| 0x00203FB0, 0x002042D0 | `lines_select`, `line_find_segment` | right twins (the first tool version cut these leaves at an early `jr $ra`; the improved one gives 37 / 68 insns, difference 0.13 / 0.08) |

---

## For other areas

- **physics / dynamics**: the run state sits at `car` +0xD8 in Lupo 2 (GT3
  +0xF84) with +4 shifts around +0x3E4..+0x4B8; the wheel-layout object
  (L2 0x001DB848 returns it: wheel count +1, front / rear wheel index +0xC /
  +0xD, axle per wheel +0x14.., flags +0x3C..; per-wheel tyre L2 0x001DC168)
  that the AI's grip and steering reads go through; the new value at st+0x424
  (A7, writer L2 0x001E5B78); `AIParams` in the conductor (A11); the Pod rule
  engine and conductor fields (M2); `DynamicsConductorTraining` (M3);
  `RaceRuleOptions` at dc +0xEC80 with `enemySkillC2` unused (A8); the car
  message table grew to 18 entries (L2 0x002DEC38).
- **HUD**: the `RaceDisplay` flag +0xA8CD and panel bits +0xA8D0 (C4);
  `RaceDisplay::virtual_2` takes dt (1/60 passed by
  `RaceSinglePlayer::virtual_28` L2 0x00180320); display event codes + 1
  (C3); `SimpleExamDisplay` accessors, `RacePODDisplay` /
  `RacePOD2PDisplay`, `RaceSplitDisplayBase` (M2, M5); the Pod messages
  (M2).
- **rendering** (race/core files, already in `rendering.md`): Lupo 2's
  `drawCars` L2 0x0017D5B0 with the projected shadow driver L2 0x0017DA70
  (helpers L2 0x001A3A90 / 0x001A3AE0); `RacePS2Base::virtual_3` L2
  0x0017BA68 applies every car's model animation; slot 58 L2 0x0017E870,
  slot 62 L2 0x0017DBD0; RaceBase slot 10 (C6). Not in `rendering.md`:
  the afterimage is kept but changed (corrected in the delta review below:
  `captureAfterimage` / `drawAfterimage` are L2 0x0017EC80 / 0x0017EDB0,
  both now field-mode aware, and the draw moved from `drawView` into
  `RacePS2Base::virtual_46` L2 0x0017CD48, after the 2D overlays); `drawCourse` L2
  0x0017D3A8 also calls `course_draw_parts`. The twins of GT3's
  `drawCarReflections`, `drawLampReflections`, `addCarLamps`,
  `addCourseLights` are L2 0x0017E070, 0x0017E2C8, 0x0017EA60, 0x0017EB68.
- **car model**: `RaceEntryCar` holds a `VehicleModel*` (+0x1E3C; L2
  0x00175AA0 / 0x00175AF8 wrappers), its `loadModel` (L2 0x00175BA8) picks a
  bike and rider (`bike/man3.mdl`, codes `cbr600f4i_02`, `yzr500_antena3_99`,
  `bike_test0`, …), `loadTires` / `loadWheel` take front / rear parts
  separately; `RaceEntryCar::copyFrom` is L2 0x00175660.
- **sound**: GT3's `RaceEntryCar::updateSound` (US 0x00177A50, 792 insns)
  has no twin among `RaceEntryCar`'s functions in Lupo 2 (moved out); the
  race BGM change (C8).
- **input**: C5, C7; the per-frame call in `RacePS2Base::virtual_27` L2
  0x0017B700 to the head-tracker singleton L2 0x001AE008 / 0x001AE068
  (`rendering.md`: camera / input).
- **menus / app**: the runners of `RaceTraining` (L2 0x0012F298, from
  `main_arcade`), `RacePOD` (L2 0x0012E018), `RacePOD2P` (L2 0x00130CB8),
  the saved-replay player L2 0x00101240 (sets the START-ends flag, C4 c).
- **save**: L2 0x0021E150..0x0021E2D0 (placed in race/core by neighbour;
  called by `GameStatus::init` / `load`): builds `"%s#%03d"` car names from
  `product_config` and checks `PrizeSetFlags` — a Lupo 2 unlock pass.
- **effects**: linkonce `RigidBodyManager::SetUseShadow` (placed in
  race/pause).

---

## Noise (looked at, no game-logic change)

Re-checked with the improved `tools/lupo2.py` (gp constants, Lupo 2 virtual
calls resolved to slots, tail calls, leaf bounds): 51 pairs of the area still
differ in virtual slots, and every one is explained by the renumbering
(`RaceBase`-derived slots + 1 for GT3 10..33, + 5 for 34..52, + 6 for 53..55, + 7 from 56;
`DynamicsConductor` + 2; `Serialize` classes + 1), by Lupo 2 calls through a
getter the tool cannot follow (`getDisplay`, the conductor at a far offset),
or by the changes above. The float constants of the area differ only where an
entry says so (the 999.9 and 20 km/h literals differ by one ulp).

Pairs (Lupo 2 address first; "layout" = only field offsets differ, "vcall" =
Lupo 2's virtual-call sequence `addiu; lh; lw 4; jalr` that the tool reads as
`indirect`, "EH" = the exception landing pads differ):

- **AI**: `ai_load_params` 0x001FFA20 (A11 only), `ai_select_gear` 0x00200060
  (`car_gear_for_speed` twin L2 0x001FA960), `ai_update_gear` 0x00200110 (tail
  call), `ai_corner_speed` 0x00200138 (A11, wheel layout, A12),
  `ai_next_corner` 0x00200440, `ai_find_corner` 0x00200510 (tail call),
  `ai_look_ahead` 0x00201500, `ai_steer_toward` 0x00201820 (A11), `ai_stop`
  0x002018A8 (tail call to `car_mode_still`), `ai_idle` 0x002019A0,
  `ai_reset_outputs` 0x00201A38, `ai_apply_outputs` L2 0x00201A88 (car_set_brake
  twin L2 0x001EA4A8, layout), `ai_look_distance` L2 0x002019E8..0x00201A34
  (split by the leaf scan), `ai_speed_throttle` / `ai_hold_speed` L2
  0x002013E8 / 0x00201450, `ai_brake_margin` / `ai_clear_traffic` /
  `ai_note_car` L2 0x00200640..0x002007A4, `ai_set_downforce` L2 0x00200008
  (layout), `ai_nearest_line`, `ai_reset` (drops the empty display call),
  `ai_corner_count`, `ai_corners`, `ai_set_line` (tail call),
  `ai_change_line`, `ai_rev_engine` L2 0x00202DA0.., `ai_init` / `ai_construct`
  L2 0x00202A68 / 0x00202A80, `ai_on_line4` L2 0x00202C58, all `lines_*`,
  `pit_*`, `line_*` (x40 multiplies become shifts; A13), `pit_lane_contains`.
- **race/core entries**: `RaceEntryInformation` ctor / dtor; `RaceEntryCar`
  ctor / dtor / `unloadModel` / `updateColorIndex` / `applyPendingModel`
  (VehicleModel), `updateGhostLapTime`, `recordLapTime` (totalDiff twin),
  `resetLapTime`, `setDriverName`, `setDate` (same BCD code), operator new /
  delete (allocator inlined); `RaceEntryBase` (all: vcall, EH, layout);
  `getCarCode` (inline field), `IsValidAutomobile`, accessors.
- **events, set-up, information, organisation**: `RaceEventQueue` (all;
  queue +0x818 → +0x81C), `race_pool_*`, `race_organization_create` (vcall,
  EH), `RaceInformation` (all), `RaceOrganization` ctor / dtor /
  `setLoggerMode` / `releaseRaceDB` / `releaseEntries` / `releaseCourse` /
  `updateRanksAndEvents` (event codes + 1) / `resetEntryOrder` /
  `moveEntryOrder` / `attachInputs` / `F_0017AA48` / `setupGhost` /
  `F_0017AD28` / `recordSectionTime` / `recordLapTime` / `setEntry*` /
  accessors; `updateStartAndTimer` L2 0x00178530 (event codes + 1);
  `RaceLapTime` (all), `TimeRecord`, `SpeedRecord::update` (999.9 literal
  1 ulp apart), `LapRecord` (all).
- **RaceBase / RacePS2Base / RaceBasic**: `race_create_by_name`,
  `race_destroy`, operator new / delete, ctor / dtor, `setBackgroundMode`,
  `virtual_4..9`, `virtual_7` (layout), `virtual_13..26`, `update`
  (`paused` +0x9690), `virtual_28`, `start`, `virtual_33..35`, `endRace`,
  `startSounds` / `stopSounds`, `takeSignal`, `virtual_39`,
  `getNumberOfLaps`, `race_mem_pool_*`; `RacePS2Base` ctor / dtor,
  `virtual_8`, `virtual_27` (head tracker only), `virtual_28`, `virtual_29`
  (L2 0x0017B8B0), `virtual_42`, `virtual_43`, `getFlare` / `getSmoke`;
  `RaceBGMPS2` (all but C8); `RaceEntryPS2`; `RaceBasic` ctor / dtor /
  `virtual_8` / `virtual_35` / `virtual_45` / `setPauseMenu` /
  `setSubDisplay` / `setRaceDisplay`; `setRaceMode` (mode-id lookup moved
  into L2 0x0017F968); `RaceBasicWithRaceDisplay` ctor (EH).
- **replays, serialise, course, pool, input, random**:
  `RaceReplayInformation` (all), `Serialize::*` (`load` inlines the
  0x41940-byte copy; four static destructors registered with `atexit`),
  `RaceGTmodeInformation` (all), `RaceCourse` (all but C9; accessors with Lupo
  2 names: `getAutoDrive(i)` = data +0x94 + 4i, `getGadgetShapeList` +0x90,
  `getEnvPtr` +0x80, checkpoints through +0x70), `RaceMemPool`, `RaceInput`
  (ctor / dtor / lap functions / `sendVibration` / button and analog
  readers / `emulatePressure` / `attachBuffer`), `RaceRandom` (all).
- **modes**: every `operator new`, `create`, static constructor (Lupo 2 adds
  a `__static_initialization` helper per file: L2 0x001804D8, 0x00180BA0,
  0x00181150, 0x00181510, 0x00182A68, 0x001837D8, 0x00183A78, 0x001855A0,
  0x00185A58, 0x00187190, 0x001878B8, 0x00187E00, 0x0018ADD8, 0x0018D138,
  0x0018D7C8) and typeinfo function; `RaceSinglePlayer` (ctor, `virtual_8`,
  `virtual_13..15`, `virtual_26`, `virtual_28` (dt to the display, event
  0x17)); `RaceSinglePlayerInformation` (all); `RaceArcade`,
  `RaceArcadeSingle`, `RaceArcadeDemo` (`virtual_27` → C1),
  `RaceArcadeDemoInformation`, `RaceFreePractice`; `RaceSplitBattle` (all,
  M5; its update L2 0x00181D88, `updateDisplay` L2 0x00181EF8, control-record
  save / load L2 0x001826E0 / 0x00182768 / 0x001827F8); `RaceBattleT` /
  `RaceBattleInformationT` (the "eedmeter" / "TI," strings are data bytes);
  `RaceSolitaire` (all but M1: `virtual_9`, `virtual_11`, `virtual_13`,
  `virtual_26`, `virtual_35`, `virtual_56`, `virtual_57` (arena alloc twin
  L2 0x001A6BD0), `virtual_59` / `virtual_28` / `virtual_45` L2 0x00185178 /
  0x00185208 / 0x00185558, dtor); `RaceSolitaireEntry`,
  `RaceSolitaireInformation`; `RaceTimeTrial`; `RaceFreeRun` (all but
  `virtual_23`; `virtual_59` / `virtual_30` L2 0x00186EB0 / 0x00186F78);
  `RaceLicense`, `RaceLicenseBGM`, `LicensePause`; `MiniPrize`,
  `prize_model_path` (`PrizeRankEntry::get` twin L2 0x00187E58);
  `RaceMachineTest` (all; `onExit` identical, `shortenDelay` tail call);
  `RaceChampionship`, `RaceChampionshipInformation` (all: loops and layout).
- **pause**: `PauseBase`, `PauseMenu` (`toggle` inlines `showMain`),
  `RacePause`, `RaceMenuWindow`, `ReplayPause` (`virtual_3` tail call),
  `ReplayPauseMenu`, `PauseMenuWithRetry`, `RacePauseWithRetry`;
  `ReplayOption` functions other than the rows of P1.

Unpaired Lupo 2 functions placed in the area that are not new logic:

- out-of-line copies of inline members (GT3 has the same members inline):
  `RaceEntryCar` (`setStartPosition` = GT3 `order` +0x20E0,
  `setDrift` / `getDrift` = GT3's field +0x20E8 (named `tyre` in our source),
  `setSteerAssistType` / `Asm` / `Tcs`, `getVibrartion`, `setPoint` /
  `getPoint`, `setMaxSpeed` = `topSpeed`, `getComment` = `driverName`,
  `getDateTime` = `date`, `setAccumlatedStatus` (two overloads), `setCarCode`,
  `getModelCode`, `getName`, `getGrade`, the `VehicleModel` wrappers
  `getRaceCarModel` / `getCarModelInfo` / `get/setRaceCarModelEnable` /
  `soundStart` / `soundStop` / `update` / `DMAsafe` / `setLOD` /
  `set/getBodyColor`), `RaceEntryBase::getCar`, `RaceEventQueue::count`,
  `RaceOrganization` (`entry`, `inquiry`, `newEntryForm`, `getCarControl`,
  fastest / record times, `getSessionSpeedRecord`, `eventCheck` / `eventGet`
  / `eventPut` / `eventCount`, `clearLoggerSector`), `RaceBase`
  (`set/getReplay`, `getVisual`, `set/getDrawable`, `getPause`,
  `set/getPauseExit`, `setCameraRandom`, `getTerminate`), `RaceBGMPS2`
  (`setMusic`, `getMusic`, `initMusic`, `setBgmTimingFunction`),
  `RacePS2Base` (`set_back_mirror_region`, `setCrossFadeTime`,
  `getParticle`, `getRigid`, `isDying`), `RaceBasic::displayInnerPanel`,
  `RaceSinglePlayerInformation` / `RaceSolitaireInformation`
  `setPlayerTransmission` / `Drift` / `Vibration`, `RaceSolitaire`
  `set/isGhostMapped`, `getInputAlways`, `RaceSplitBattle::getDisplay`,
  `RaceChampionshipInformation::IsFinal`, `MiniPrize::updateContext`,
  `RaceCourse` accessors, `UnitArena<64>` typeinfo, `RaceInput` accessors
  and recorder wrappers, `RigidBodyManager::SetUseShadow`;
- twins of GT3 functions the map missed: `RaceEventQueue::RaceEventQueue`
  L2 0x00177448 (+ clear helper 0x001774A0), `LapRecord::LapRecord` L2
  0x00179368, `RaceEntryCar::recordSectionTime` L2 0x00176DF8,
  `race_basic_create` L2 0x0017F128, the `create` functions of every mode,
  `RaceBase::virtual_12` / `virtual_16` / `virtual_29` / `takeSignal` /
  `virtual_38` (L2 0x00179F58, 0x0017A010, 0x0017A4E0, 0x0017A980,
  0x0017A9D0), `RacePS2Base::virtual_29` L2 0x0017B8B0, `updateStartAndTimer`
  L2 0x00178530, the `RaceInput` functions of C7, `RaceRandom` static init
  L2 0x0021E0F0, the mode functions listed above, `RaceLinkBattle` slots
  26..29 L2 0x001836C8..0x00183740 (Multilink), the rendering twins listed
  under "For other areas";
- compiler helpers: static-initialisation wrappers (above, plus L2
  0x00177A70, 0x0017AB98, 0x0017AC38, 0x0021DE60), L2 0x0017C348 (a tail
  call into the runtime), L2 0x0018E008 (`UnitArena` helper), L2 0x0018DB60
  (`RaceCourse::load(name)` → `nameToCode`);
- new but covered above: AI (A6 helper L2 0x00201668; the twins of
  `ai_brake_factor` L2 0x00200C20, `ai_drive` L2 0x00201B50,
  `ai_apply_rules` L2 0x001FFDE0, `ai_reduce_pace` L2 0x001FFF98, getters L2
  0x00202CA8 / 0x00202CD8), C4 helper L2 0x0017F9A0, C5 L2 0x0017A650 /
  0x0021CE60 / 0x0021CEA8, C6 slots, M2..M5 classes, the Multilink region,
  the rendering functions.

---

## Delta review (map v2)

The map rebuilt with gp, Lupo 2 virtual calls, tail calls, ~3,200 leaf
functions and the surveys' fixed pairs (`build/scratch/delta/race_logic.txt`,
19 lines; the 3 HUD lines are in `hud.md`). The summary of the area now
reads 515 / 100 / 180 (core), 338 / 70 / 75 (modes), 63 / 7 / 3 (AI), 66 / 7 / 3
(pause) paired / differing / unpaired. One line each:

| Candidate | Verdict |
|---|---|
| pair L2 0x0017EC80 = US 0x00180AE0 `captureAfterimage` (was unpaired) | right pair, **real change** (D1): keeps the field mode of the capture |
| pair L2 0x0017EDB0 = US 0x00180BF8 `drawAfterimage` (was unpaired) | right pair, **real change** (D1): half-line shift by field parity; called from `virtual_46` now. Corrects the first report, which read these two as new passes |
| pair L2 0x00204180 = US 0x001F9DB8 `pit_node_dist_wrapped` (was unpaired) | right pair, noise (12 / 14 insns, the x40 multiply as shifts) |
| pair L2 0x0021DED8 = US 0x00211920 `RaceInput::reset` (was US 0x00211940) | **wrong pair**: L2 0x0021DED8 calls the T413 static helper with (0, 0xFFFF): it is `T413_static_destroy` (US 0x00211750). `RaceInput::reset` is the linkonce `resetRecorder__9RaceInput` L2 0x002D1680 |
| pair L2 0x0021DFB8 = US 0x00211940 `RaceInput::attachBuffer` (was unpaired) | **wrong pair**: L2 0x0021DFB8 sets a request record to kind 6 with a name (see 0x0021DEF8 below), called from menu code. `attachBuffer` is the linkonce `attachRecorder__9RaceInputi` L2 0x002D1660 (likewise `play` / `skip` / `record` = `readRecorder` 0x002D16A0 / `seekRecorder` 0x002D16C0 / `writeRecorder` 0x002D16E0) |
| new? L2 0x00177F18..0x00177F40 | compiler helper: T124's static initialiser (`Random` seed 0x87654321 into gp-0x5648, the `RaceInformation` random); its `.ctors` wrapper L2 0x00177F40 = US 0x00179FC8 `race_info_static_ctor`. Noise |
| new? L2 0x0018D420..0x0018D42C | new code: `RacePOD`'s name getter (slot 25, "RacePOD") — M2 |
| new? L2 0x0018D628..0x0018D634 | new code: `RacePOD2P`'s name getter (slot 25) — M2 |
| new? L2 0x001B4D80..0x001B4D94 | twin of `ReplayOption::rowItems1` US 0x001AEAC0 with the row numbers one lower (1P 1 / 2P 2): the *target* row is gone — P1 |
| new? L2 0x001B4D98..0x001B4DAC | twin of `ReplayOption::rowItems2` US 0x001AEAD8, one lower (2 / 3) — P1 |
| new? L2 0x0021CBC8..0x0021CBE0 | twin of `RaceInput::isLapFinished` US 0x00210690 (same code). Noise |
| new? L2 0x0021DEF8..0x0021DF28 | belongs elsewhere (menus / save): constructor of a small request record {state +0x0, kind +0x4 = -1, words +0x8..+0x20, name +0x24}, called at L2 0x00226460 (game status) |
| new? L2 0x0021DF28..0x0021DF58 | belongs elsewhere: the same record's reset (identical code), called by `gt2k_game::MenuStatus::reset` (L2 0x00226528 = US 0x00217DF8) and L2 0x0010FE00 |
| new? L2 0x0021DF58..0x0021DF64 | belongs elsewhere: the record's clear (state 0), called by `ArcadeControl::runAttract` (L2 0x00108728 = US 0x001153A0) and L2 0x00108580 |
| new? L2 0x0021DF68..0x0021DF7C | belongs elsewhere: set (state 1, kind), called by L2 0x00108580 |
| new? L2 0x0021DF80..0x0021DFB4 | belongs elsewhere: set with a name (state 1, kind, flag, `strcpy` name), used by L2 0x0021DFB8 (kind 6), which menu code calls at L2 0x00114DE0..0x00114EE8 and 0x001378D8 |
| new? L2 0x0021DFD8..0x0021DFE0 | belongs elsewhere: the record's state getter (`runAttract`) |
| new? L2 0x0021E148..0x0021E150 | belongs elsewhere (save): a 2-insn initialiser called by `gt2k_game::GameStatus::GameStatus` (L2 0x00226F60 = US 0x00218668); part of the Lupo 2 unlock block L2 0x0021E148..0x0021E2D0 |
| new? L2 0x0021E150..0x0021E1AC | belongs elsewhere (save): helper of the unlock pass L2 0x0021E2D0 (`GameStatus::init` / `load`) |

The request record (L2 0x0021DEF8..0x0021DFD8) sits between T413 and T414
but nothing in the race uses it; GT3's `MenuStatus` / arcade attract code is
the menus area's to compare. Counts: 5 pairs (2 real changes, 1 noise, 2
wrong pairs), 14 new? (2 new Pod code, 2 twins of P1, 2 noise twins / helpers,
8 belong elsewhere).

### D1. Afterimage: field-mode aware, drawn after the overlays

| Field | |
|---|---|
| What | The replay afterimage (motion trail: the last frame split into three 8-bit planes, drawn back over the next frames with a fading weight) is kept, with two changes. (a) The capture stores the GS field mode (`gs_get_field_mode`) it was taken in, and the draw shifts the planes by half a line (-0.5) when the current field differs, so the trail does not jitter between odd and even fields. (b) The draw moved out of `drawView` into `RacePS2Base::virtual_46` (render order: `drawView`, `virtual_45` the 2D overlays, `virtual_46`), so the trail is now blended over the HUD as well, before the error view. |
| Where | L2 `captureAfterimage` 0x0017EC80, `drawAfterimage` 0x0017EDB0, `virtual_46` 0x0017CD48, `drawView` 0x0017C400; GT3 US 0x00180AE0 / 0x00180BF8 / 0x0017EFD0 / 0x0017E740 (`src/game/race/core/T131_race_ps2_render.cpp`). |
| Kind | changed logic (rendering) |
| Evidence | `jal gs_get_field_mode` in both; capture stores it at race +0xBF68 (L2 layout), draw compares, `lui 0xBF00` (-0.5) offset; `virtual_46`: `jal 0x0017EDB0` before `jal ErrorEventView::draw`; `drawView` calls only the capture. |
| Port | (a) small, in the two functions (one field in `RacePS2Base`); (b) a move of one call. Visual only. |
| Verdict | port (a) with the rendering area (field / frame mode work, FINDINGS 17); (b) rendering area to decide (GT3's trail leaves the HUD sharp) |
