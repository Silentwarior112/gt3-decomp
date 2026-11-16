# Survey: race HUD and displays

Area: `src/game/race/display/` (the race display `RaceDisplay` and its
parts, meters, tachometers, speedometers, boost meter, mini map, race
messages, the per-mode displays, the display events), plus the HUD texts of
`src/game/data/messages/T186_msg_race_display.cpp` that only the HUD reads.
Lupo 2 addresses are `L2 0x...`, GT3 US addresses `US 0x...`. Lupo 2 field
offsets are Lupo 2's (its `RaceDisplay` is 0xA8D4 bytes, GT3's 0xA47C).

## Summary

Candidates examined: all 447 pairs of the folder through `cmp` (and a
gp-aware rerun, see "Tool notes"), the 164 plausible pairs at >= 0.25 and
the 21 size-mismatched ones each by `dis` / `dis --us` against our source,
about 30 low-difference pairs that read tables or constants
(two of them turned out to be real changes: C8, C9), all 133 unpaired
Lupo 2 functions placed here (each identified, or traced to its callers and
handed to its area), the display classes' vtables (`class`), the HUD
message table and the sprite / font names.

Real changes, roughly in order of what they are worth to GT3:

1. **C1 `RaceCapacityMonitor`, the Dualnote battery gauge** (new class,
   `RaceDisplay` +0xA890): a vertical bar = battery level, blue while the
   motor state is >= 0, magenta otherwise, with the `meter_sign_dualnote`
   sign; only for hybrid cars; placed by both meter layouts and by a layout
   record. **Port with the Dualnote physics.**
2. **C2 The tachometer shows a smoothed rpm**: the needle follows a
   per-car spring-damper (k 1000, c 50) the drivetrain updates, not the raw
   rpm. **Port** (physics owns the spring).
3. **C3 SELECT cycles the telemetry HUD in a live race**: the show mask
   becomes a `RaceDisplay` field (+0xA8D0, default GT3's `RD_SHOW_LIVE`);
   SELECT cycles none → accel / brake monitor → monitor + steering → none,
   and either forces layout variant 1. **Port** (race/core owns the button).
4. **C4 Meter layouts**: the gauge in both tables; the accel / brake
   monitor and the steering display join the remote table; the remote boost
   meter moves up 6 px. **Port.**
5. **C5 Accel / brake monitor redrawn**: two framed columns with the
   `replay_sign_brake` / `replay_sign_accel` signs and bars with a fading
   trail from last frame's value. **Port** (sprites).
6. **C6 Steering display redrawn**: a `replay_sign_gtforce` wheel sprite
   rotated by the steering angle (degrees, ±90), tick marks at -90 / 0 / +90,
   a red marker, motion blur from last frame's angle. **Port** (sprite).
7. **C7 VSC + TCS lamps merged into one "ESP" lamp** (green, font
   `sq_9#1*75,75`). **Port (cosmetic)**; it loses which assist is active.
8. **C8 Tachometer dials**: a 14,000 rpm dial is added and the needle's full
   scale is the chosen dial's value instead of the car's rounded rev limit
   (GT3 shows a 13,000 rpm scale on the 18,000 dial). **Port** (dial
   images `rev_14`, `revb_14`).
9. **C9 Speedometer**: full scale = the chosen dial's value (GT3: 160 km/h
   scale on the 180 dial, 100 mph on the 120 dial). **Port** (fix).
10. **C10 Needle look**: brighter red (0xFF instead of 0xC0), blur alpha 1/6
    instead of 1/8, clamped at end angle + 2 instead of value <= scale + 8.
    **Port (cosmetic).**
11. **C11 Mini map**: one `car_pointer` sprite tinted per car (yellow, green
    for the car shown) instead of `1pcar_dot` / `ecar_dot`. **Port** (sprite).
12. **C12 HUD text**: `RaceMessageDisplay` gets a drop shadow (2 px, black,
    faster fade; on for the title and mode message), images resolved once at
    `setText`, a 0x60-character buffer; neutral text colour 0x80D0D0D0 →
    0x80C0C0C0; the course name keeps its case; mode labels "Qualify" /
    "Demonstration" / "Live" / "Replay", "PP". **Port.**
13. **C13 Car name panel**: a `replay_sign_car` icon left of the name, the
    name 60 px right / 3 px up. **Port** (sprite).
14. **C14 Logo watermark**: additive, half bright; an A-spec logo variant
    (`gt_aspec_logo_mini`) selected by a fader flag; a per-display "mini
    logo" flag (default on) gates the logo **and** the car name; a GT Concept
    front-end mode global gates the logo. **Port the look and the flag;
    not the front-end gate.**
15. **C15 Demo HUD**: in a demo only the car name is shown (GT3: the replay
    display options). **Port.**
16. **C16 Time limit**: shown 3 s lower (clamped at 0, GAME OVER at 3 s
    left), moved to (16, 320), red below 10 s, GAME OVER centred and red.
    **Port the look; the 3 s offset only with race/core's reason for it.**
17. **C17 Two new car warnings (Pod)**: kinds 16 "stop and pull the
    handbrake", 17 "return to the stop area". **Port with the Pod mode.**
18. **C18 New information event 7 (Pod)**: "it sat down!" in blue in the
    warning slot for 2 s with the start beep. **Port with the Pod mode.**
19. **C19 The display update takes a time step**: animations (car icons,
    logo, event board, title, mode message) advance only when it is > 0, so
    the HUD can be re-laid-out without animating (C3 uses it). **Port.**
20. **C20 `SimpleExamDisplay` / `SimpleExamMessages` = GT3's
    `RaceTimeTrialDisplay` / `TimeTrialExaminationMessages`, generalised**:
    takes event 0xA or the new 0xB (Pod) for its own player car only; a
    "stopper" holds the disqualification text instead of fading it (and
    skips the jingle); no 30 s cap; the text centred on the buffer passed in.
    The time trial behaves as before. **Port** (base of the Pod displays).
21. **C21 `RacePODDisplay`** (new): the Pod mode's display, the stop-area
    rules on the event board at the start. **Port with the Pod mode.**
22. **C22 `RacePOD2PDisplay` + `RaceSplitDisplayBase`** (new): the Pod's
    split-screen display; the race ends when both players are disqualified.
    `RaceSplitDisplay` now forwards through the new base (same behaviour).
    **Port with the Pod 2P mode.**
23. **C23 Free run**: the start messages only live (not in replays).
    **Port.**
24. **C24 Licence result screens**: the pass wait 6 → 5 frames, the fail
    tail 48 → 47 frames. **Port (trivial).**
25. **C25 Race event renumbering**: Lupo 2 inserts event 0xB; every display
    event type, the BGM / view-mode / error events shift by one. No change of
    the HUD's own; it follows race/core's enum. **Dependency.**
26. **C26 Data**: RACEDISPLAY texts 56..62 (Pod) added, five labels re-cased.
    **Port** (English texts to write for the US build).

Removed in Lupo 2 (not ported, GT3 keeps them): **R1** the second event
board (`events2`) is laid out but never drawn (GT3 never fills it either);
**R2** the 16:9 projection no longer exempts the data logger mode.

Uncertain: who sets the A-spec logo flag (C14) — no writer found; why the
time limit is shown 3 s low (C16); the exact pad input of the mini-logo
toggle in `RaceBasic::virtual_28` (race/core).

## Changes

### C1. `RaceCapacityMonitor`, the Dualnote battery gauge

| Field | |
|---|---|
| What | A vertical battery gauge for hybrid cars. Background box (black, alpha 0x40) and an inner dark box (0x80202020), the `meter_sign_dualnote` sign at the bottom (sign height = (w-4)·5/7), above it a bar filled from the bottom to `capacity` of the free height. The bar colour is lerped by `charge`: 1 → (0, 0.27, 0.80) blue, 0 → (0.55, 0.10, 0.25) magenta. Nothing is drawn unless the hybrid flag is set. Drawn in the centre panel with the meters (`show & RD_SHOW_METERS`). |
| Where | New. L2 class `19RaceCapacityMonitor` (vtable 0x0032F2B8, 10 slots: base slots + draw), ctor 0x001976D0, draw (slot 4) 0x00197720, linkonce `__tf` 0x002C7A48, dtor 0x002C7A98, `setCapacity` 0x002C7B00 (+0x1C), `setCharge` 0x002C7B08 (+0x20), `setHybridCar` 0x002C7B10 (+0x24). Member of `RaceDisplay` at +0xA890 (ctor call 0x0018E340). Fed in `RaceDisplay::virtual_2` L2 0x00190984..0x001909C0: capacity = L2 0x001E02C0 (`car+0x5F0` / max, the battery level), charge = L2 0x001E02E0 (1.0 when the motor state byte `car+0x5F5` >= 0, else 0.0), hybrid = L2 0x001E0288 (`isHybrid`). Placed by `layoutMeters` (C4) and by `setLayout` (record type `RD_ITEM_AB_MONITOR` with subType 1, L2 0x001929E8). GT3 counterparts: `RaceDisplay` (T154), new class in T157. |
| Kind | new class |
| Evidence | String `meter_sign_dualnote` (0x003423A8) at 0x00197774; gate `lbu 0x24($fp); beql ...` at 0x00197754; colours gp-0x7C08..-0x7BFC = 0.8, 0.2745, 0.55, 0.1 plus 0.25; `pgl_color4f` 0x001978F0, `race_fill_rect` / `race_draw_image_rect`. FINDINGS §11 had the class and fields right; this adds the colour meaning and the layout. |
| Port | New part class (base `RaceDisplayObjectBase`, + image, capacity, charge, hybrid flag: 0x28 bytes); a `RaceDisplay` member (grows `RaceDisplay`: every `EGCS_SIZE`/hard-coded size of `RaceDisplay`, `RaceSplitDisplay` +0xA480, `RaceBasicWithRaceDisplay` +0xC8C0 and the derived displays moves); three setter calls in `virtual_2`; the draw call in `virtual_3` after the gear display; layout entries (C4). Physics: the Dualnote battery fields and accessors (`car+0x5F0`, `+0x5F5`, `isHybrid`) — physics area. Asset: `meter_sign_dualnote` in `race/<country>/display.imgs`. |
| Verdict | **port with the Dualnote** |

### C2. The tachometer reads a smoothed "display rpm"

| Field | |
|---|---|
| What | The active tachometer's value is no longer the engine rpm but a per-car display rpm that follows it through a spring-damper: v += ((rpm − d)·1000 − v·50)·dt; d += v·dt (dt from the car's step). The needle lags and settles instead of jumping on shifts and limiter cuts. |
| Where | L2 `RaceDisplay::virtual_2` 0x0019074C `jal 0x001E4728` (returns `(int)d[car]`) then the tacho's `set` (slot 11); GT3 `RaceDisplay::virtual_2` US 0x00192A58 sets `(f32)state->st.rpm`. Physics side: arrays d at 0x004A9520, v at 0x004A9540 (.bss, indexed by the car index byte `car+0xD8+0x3EC`); reset L2 0x001E4658 (d = rpm, v = 0; called from 0x001F4F20), update L2 0x001E4698 (called from the drivetrain step 0x001E6A90). |
| Kind | changed logic (HUD reads a new physics value) |
| Evidence | 0x001E4698: `lui 0x447A` (1000.0), `lui 0x4248` (50.0), `lwc1 $f4,0x14($a0)` (dt), `lh 0x494` (rpm); 0x001E4728: `lbu 0x4C4($a0)`, `lwc1 -0x6AE0(0x4B....)`. |
| Port | `virtual_2` reads the new value; the spring (two floats per car: two 8-entry arrays; reset at race init, update in the drivetrain step) belongs to the physics area. |
| Verdict | **port** (with the physics side) |

### C3. SELECT cycles the telemetry HUD; the live show mask becomes a field

| Field | |
|---|---|
| What | GT3 draws a live race with the constant `RD_SHOW_LIVE` (everything but the steering display 0x10 and the accel/brake monitor 0x40). Lupo 2 keeps that mask in `RaceDisplay` (+0xA8D0, set to -0x51 = `RD_SHOW_LIVE` in `virtual_0`) and lets SELECT cycle it in a live race: none → 0x40 (accel / brake) → 0x40 + 0x10 (+ steering) → none (table 0x002DD390 = {1, 3, 0, 0}). While either bit is set, `virtual_2` lays out variant 1 (the remote meter set, which now carries both elements, C4) whatever the camera; after a toggle the display is updated with a time step of 0 (C19) so the new layout shows at once. |
| Where | L2 `RaceDisplay::virtual_3` 0x00191148 (`show = flags` when live); `virtual_0` 0x0018F314 (init); `virtual_2` 0x00190554..0x00190590 (`flag40 || flag10` → `setLayout(1)`); flag helpers L2 0x00193668 (set/clear by mask), 0x001936B0 (0x40), 0x001936D0 (0x10), 0x001936F0 / 0x00193708 (tests); linkonce `get_race_display_flag` 0x002C63A8. The toggle itself: L2 0x0017F9A0 (race/core, new; call sites 0x0017F758 in `RaceBasic::virtual_30`, 0x0018AC60 in `RaceMachineTest::virtual_30`, 0x00187080 in L2 0x00186F78): live only (`race+0x9684 == 0`), input slot 4 == 0x20 and slot 5 & 0x20 (`CONTROL_SELECT`), `dynamic_cast<RaceDisplay*>` of the race's display. GT3: T154 `virtual_0` / `virtual_2` / `virtual_3`. |
| Kind | changed logic + new field |
| Evidence | 0x0018F2E0 `addiu $v0,$zero,-0x51` stored at +0xA8D0; 0x0017FA84..0x0017FAD0 `jal 0x001936F0`, `jal 0x00193708`, table 0x002DD390, `jal 0x001936B0` / `0x001936D0`, then display slot 2 with `$f12 = 0`. |
| Port | An `int` show mask in `RaceDisplay` (init in `virtual_0`), its use in `virtual_3`, the layout override in `virtual_2`, the helpers; the button handler in race/core (check SELECT has no other job in GT3's race input first). Depends on C4 (positions for the two elements in the remote layout) and C19. |
| Verdict | **port** |

### C4. Meter layout tables

| Field | |
|---|---|
| What | `layoutMeters` builds its two tables on the stack (GT3: static tables built by T154's static initializer) and they gain entries. Onboard (kind 0): + capacity monitor (615, 435, 30×70, align wide-right / bottom). Remote (kind 1): + capacity monitor (620, 370, 28×64, wide-right / bottom), + accel / brake monitor (68, 435, 30×100, wide-left / bottom), + steering display (320, 420, 75×68, centre / centre); the boost meter's y 436 → 430. All other entries, the digital speedometer unit offsets, gear lamp offsets and back panels are unchanged. `setLayout` additionally places the capacity monitor from an `RD_ITEM_AB_MONITOR` record with subType 1 (subType 0 = the accel / brake monitor), places the mirror only for subType 0, and builds the car-name pair of C13. |
| Where | L2 `layoutMeters` 0x00193018 (GT3 `RaceDisplay::layoutMeters` US 0x00194FF8 and the tables `g_002BC080` / `g_002BC120`, T154); L2 `setLayout` 0x00192570 (US 0x00194888; jump table 0x00342000, 27 types as GT3). |
| Kind | changed data (layout tables) + changed logic |
| Evidence | stores at sp+0x90..0x9D (`0xA890`, 0x267, 0x1B3, 0x1E, 0x46, 5, 1), sp+0x130..0x15D (`0xA890` 0x26C/0x172/0x1C/0x40; `0xA644` 0x44/0x1B3/0x1E/0x64, 4, 1; `0xA674` 0x140/0x1A4/0x4B/0x44, 2, 2), sp+0xD6 = 0x1AE (GT3 0x1B4); 0x001929A8 subType switch, 0x00192728 mirror subType test. |
| Port | Three table entries + one changed y in `g_002BC080` / `g_002BC120` (GT3 keeps its static tables), the subType switch in `setLayout`. |
| Verdict | **port** (with C1 / C3) |

### C5. Accel / brake monitor redrawn

| Field | |
|---|---|
| What | GT3 draws two gauge images (`accel_gage`, `brake_gage`) over green / red bars. Lupo 2 draws a black box (alpha 0x40) per column, the `replay_sign_brake` (left) and `replay_sign_accel` (right) signs at the bottom of each column (height (w−4)·16/22), dark (0x80202020) bar wells, and bars drawn as a 6-vertex strip: solid from the bottom to min(now, last frame), then fading to transparent up to max(now, last frame) — a trail showing the change since the last frame. Red 0x800000F0 brake, green 0x802BCC49 accel. A new `update` (slot 3) stores the values as "last frame"; `disableAll` resets them. |
| Where | L2 `RaceABMonitor::draw` 0x00196588 (US 0x00198940, T157), bar strip L2 0x00196828 (new), `update` L2 0x00196570 (new override; GT3 uses the base's empty one), reset L2 0x00196560 (new), ctor 0x00196518 (+0x28 / +0x2C last values). Called after the draw in `virtual_3` (L2 0x00191858). |
| Kind | changed logic + new fields |
| Evidence | strings 0x00342280 / 0x00342298; `gfx_alloc_tail(5)` / `(2)`, `pgl_draw_strip` 6 vertices in 0x00196828; `max.s`/`min.s` of the two values. |
| Port | Two floats, the new draw, update, reset. Assets: `replay_sign_accel`, `replay_sign_brake` in `display.imgs`. Only visible in replays and (with C3) live. |
| Verdict | **port** |

### C6. Steering display redrawn

| Field | |
|---|---|
| What | GT3 draws a line rotated over the `steering` image. Lupo 2: `setAngle` takes the steering fraction ×(−90) (degrees; `virtual_2` passes `steer/100`); the draw (additive, colour 0x10FFFFFF) draws the `replay_sign_gtforce` wheel rotated by the angle eight times with sub-pixel jitter offsets (table 0x003421C8) and an angular spread of (angle − last)/8 (motion blur), then grey (0x80C0C0C0) tick marks at −90°, 0°, +90° (a 4-vertex strip, 0x003422D0), then the red marker rotated with the same blur at alpha 1/6. A new `update` stores the angle as "last"; reset sets "last" to NaN (= no blur). |
| Where | L2 `RaceSteeringDisplay::draw` 0x001969C8 (US 0x00198B90, T157), marker helper 0x00196C90, `update` 0x001969B8 (new override), reset 0x001969A8, ctor 0x00196960, `setAngle` 0x002C76A8; `virtual_2` L2 0x0019095C..0x00190980. |
| Kind | changed logic + new fields |
| Evidence | string `replay_sign_gtforce` 0x003422B0; `lui 0x3E00` (0.125), `lui 0xC2B4` (−90), `vu0_rotate_z` ×10, `gfx_offset_xy`, `pgl_draw_strip`; GT3's `pgl_draw_line_3d` and `steering` gone. |
| Port | Two fields (sign image, last angle), the new draw / update / reset, the ×(−90) in `setAngle`. Asset: `replay_sign_gtforce`. |
| Verdict | **port** |

### C7. One "ESP" lamp instead of the VSC and TCS lamps

| Field | |
|---|---|
| What | GT3's indicator row is four glyphs of the `odo#1` font: `!` VSC (green), `"` TCS (yellow), `#` handbrake, `$` oil. Lupo 2 draws the text "ESP" in `sq_9#1*75,75`, lit green (0x802BCC49) when either assist is on or blinking (blink phase > 0), else unlit (0x40000000), then `#` and `$` as before; the auto width is measured on "! # $". The setters are unchanged (VSC +0x18, TCS +0x19). |
| Where | L2 `RaceIndicators::draw` 0x00196018 (US 0x001983D8, T157). |
| Kind | changed logic |
| Evidence | strings `ESP` 0x0035D790, `sq_9#1*75,75` 0x00342260, `! # $` 0x0035D788 (GT3 `! " # $`); no TCS yellow. |
| Port | The draw only. The font style `sq_9#1` exists in GT3 (string at US 0x00352720). |
| Verdict | **port (cosmetic)** — note it hides which assist acted. |

### C8. Tachometer dials: a 14,000 rpm dial, full scale = the dial

| Field | |
|---|---|
| What | `setScale(rpm)` picks the first dial whose limit is >= the car's scale (`(redline + 1500) / 1000 · 1000`) and sets the dial image `<prefix><nn>`. GT3: dials 6..12 (500 rpm marks) and 18 (1000), full scale = the car's value, unchanged when no dial fits. Lupo 2: adds a 14,000 dial (1000 rpm marks), full scale = the dial's limit (0 when none fits; the table ends in a NaN sentinel). So a 13,000 rpm scale shows on a 14,000 dial with a 14,000 full scale; GT3 put it on the 18,000 dial with a 13,000 full scale (needle and numbers disagree). |
| Where | L2 `RaceRoundTachometerBase::setScale` 0x0019A7F0, table 0x00342690 (10 entries); GT3 US 0x0019D350, table `g_0032F5B8` (8 entries), T161. |
| Kind | changed logic + changed table |
| Evidence | L2 loop `slti $v1,0xA`, `mov.s $f20,$f1` (the entry's limit) then `swc1 $f20,0x1C`; table: 6000..12000 / 500, 14000 / 1000 "14", 18000 / 1000, 0x7FFFFFFF / 1000. Pair diff only 0.10: the tool sees nothing. |
| Port | The table entry and the full-scale rule. Assets: `rev_14` (onboard, prefix `rev_`) and `revb_14` (remote, `revb_`) in `display.imgs` — check GT3's pack. |
| Verdict | **port** |

### C9. Speedometer: full scale = the dial

| Field | |
|---|---|
| What | Same rule for the onboard speedometer: GT3 sets the full scale to the car's speed scale (km/h: 160 / 240 / 320; mph: 100 / 150 / 200, from `SelectMeterScale`) while the dial image is the first ≥ it (180 / 240 / 320 km/h, 120 / 160 / 200 mph) — a 160 km/h car's needle reaches the "180" mark at 160. Lupo 2 sets the full scale to the dial's value. Tables unchanged except the FLT_MAX sentinel → NaN. |
| Where | L2 `RaceOnboardSpeedmeter::setupScale` 0x0019AE18 (US 0x0019DF08, T162), tables L2 0x003427C0 / 0x00342810 (US `g_0032F998` / `g_0032F9E8`). |
| Kind | changed logic (a fix) |
| Evidence | `mov.s $f20,$f0` (entry) in the `setImage` delay slot, `swc1 $f20,0x1C`; GT3 stores the argument. |
| Port | One line. |
| Verdict | **port** |

### C10. Needle look

| Field | |
|---|---|
| What | Round-meter needle: colour 0xFF0000C0 → 0xFF0000FF (full red), blur blend alpha 1/8 → 1/6; GT3 clamps the value to [0, scale + 8] before mapping it to an angle, Lupo 2 maps first and clamps the angle to [start, end + 2.0]. The 8-copy motion blur is the same; the "no previous angle" sentinel FLT_MAX → NaN. |
| Where | L2 `RaceRoundMeterBase::draw_needle` 0x00199DE8 (US 0x0019C738, T160). |
| Kind | changed logic (cosmetic) |
| Evidence | `ori $a0,0xFF` vs `0xC0`; gp-0x7BD0 = 0.16667 vs `lui 0x3E00`; `max.s/min.s` on the angle with `+2.0`. |
| Port | Three constants and the clamp. |
| Verdict | **port (cosmetic)** |

### C11. Mini map: one tinted car pointer

| Field | |
|---|---|
| What | GT3 loads two dot images (`1pcar_dot` for the car shown, `ecar_dot` for the others) and draws them white (half alpha for map-only / ghost cars). Lupo 2 loads one `car_pointer` image and tints it per call: other cars 0x8000F8FF (yellow), the car shown 0x8000FF90 (green); half alpha for map-only / ghost cars as before. |
| Where | L2 `RaceMiniMap::init` 0x0019B1E8, `drawMap` 0x0019B288, `drawCar` 0x0019B3D8 (colour argument); US 0x0019E380 / 0x0019E420 / 0x0019E560 (T164). `RaceMiniMap` loses one image field (0x30 → 0x2C). |
| Kind | changed logic |
| Evidence | string `car_pointer` 0x00342860; `ori $a3,0xF8FF` / `0xFF90` at 0x0019B388 / 0x0019B3B0; `srl 25` alpha halving in `drawCar`; GT3's `pgl_color` and 8.0 gone. |
| Port | The draw; asset `car_pointer`. |
| Verdict | **port** |

### C12. HUD text: shadow, cached images, longer texts, colours, wording

| Field | |
|---|---|
| What | `RaceMessageDisplay` (0x88 → 0xB0 bytes): `setFont(name, shadow)` stores a shadow flag; with it the text is drawn first in black, offset 2·scaleX right / down, alpha = text alpha × clamp((level − 0.3)/0.7, 0, 1) (the shadow fades out earlier). `setText("@name")` resolves the image once (`race_display_find_image`) and keeps it (+0x3C, `setTexset`); the draw uses it. Text buffer 0x40 → 0x60 characters (the Pod texts are long). Default colour 0x80D0D0D0 → 0x80C0C0C0, also for the title, car name, "@start" / "@finish" / "@count". In `virtual_0`: title and mode message use `sw_20#1*160,160` with shadow (GT3 `sw_20*160,160`); the course name is shown as stored (GT3 upper-cases it); mode labels from the re-cased texts (C26); the record label "PP". |
| Where | L2 `RaceMessageDisplay` ctor 0x001955B0, `draw` 0x001956F8, `setText` 0x00195A58, `clear` 0x00195AF0, `setFont` 0x00195AD0, `setTexset` 0x002C7238 (US T157 0x00197AA0 / 0x00197BF0 / 0x00197E90 / 0x00199C90 / 0x00197EE8); L2 `RaceDisplay::virtual_0` 0x0018F1B0 (US 0x001918D8). `RaceEventDisplay` follows (8 messages at stride 0xB0). |
| Kind | changed logic + new fields + changed data |
| Evidence | draw: gp-0x7C28 0.3, gp-0x7C24 1.42857, `and $v1,$v0,0xFF000000` (black), `add.s $f20,$f20,$f20` (2·scaleX); setText: `lb 0($a0)` = '@' → `jal race_display_find_image`; `addiu $a2,$zero,0x60`; `lui 0x80C0; ori 0xC0C0`; virtual_0 has no upper-case loop. |
| Port | Three fields (shadow, image, longer text) — 0x28 bytes more per message: `RaceDisplay` holds 23 (16 in the two event boards, + the new car sign), so it grows by about 0x398 here; the shadow pass; the image cache; colour constant; `virtual_0` font / case. |
| Verdict | **port** |

### C13. Car name panel: car sign icon

| Field | |
|---|---|
| What | A new `RaceMessageDisplay` (`RaceDisplay` +0xFE4) shows the image `replay_sign_car` at the car-name record's aligned position (left / bottom), and the car name moves 60 px right and 3 px up next to it. Both are drawn in the bottom-right panel (C14 gates). The icon is kept visible (`show(0, 0)` every frame in `virtual_2`). |
| Where | L2 `setLayout` 0x001928A8..0x00192940 (US `setLayout` `RD_ITEM_CAR_NAME`), `virtual_2` 0x00190A6C, `virtual_3` 0x001918E4, `virtual_0` 0x0018F4F4 (colour), ctor 0x0018E200. |
| Kind | new element |
| Evidence | string `@replay_sign_car` 0x00341FE0; `addiu $s2,$s2,0x3C`, `addiu $s1,$s1,-3`. |
| Port | One message member, the layout code, draw call. Asset: `replay_sign_car`. |
| Verdict | **port** |

### C14. Logo watermark and the "mini logo" flag

| Field | |
|---|---|
| What | The `gt_logo_mini` watermark (shown in demos and when no item is shown) is drawn additively (blend 0x48) at half grey (0.5, 0.5, 0.5, alpha) instead of white with normal blending; a new fader field (+0x10) selects `gt_aspec_logo_mini` instead. A new `RaceDisplay` byte (+0xA8CD, `setMinilogo` / `getMinilogo`, default 1) must be set for the logo **and** for the bottom-right car name panel; a second new byte (+0xA8CE, `setCarNameDisplay`, default 0) shows the car name even when the item mask lacks `RD_SHOW_CAR_NAME`. The logo also needs a global (L2 0x0035EE04, gp-0x3D6C) to be 0: a front-end mode word written by L2 0x00109128 / 0x001090C0 (GT Concept menus). The mini-logo byte is toggled from `RaceBasic::virtual_28` (L2 0x0017F5F4..0x0017F62C) on a pad input in replays. |
| Where | L2 `RaceDisplayFader::draw` 0x001944F8 (US 0x00196A38, T157), fader ctor 0x001943D8; `RaceDisplay::virtual_3` 0x001911C0..0x001911FC and 0x001918A0..0x0019192C; linkonce 0x002C65F8 / 0x002C6630 / 0x002C6640 / 0x002C6690 (`getMiniLogo` returns the fader); ctor 0x0018E39C (defaults). |
| Kind | changed logic + new fields |
| Evidence | string `gt_aspec_logo_mini` 0x00342238 chosen by `lw 0x10($s0)`; `gfx_blend_func(0x48, 1.0)`, `lui 0x3F00` ×3. No writer of fader+0x10 found (searched for stores at display+0x5C / via `getMiniLogo`). |
| Port | The additive look; the two bytes and their gates (defaults keep GT3's behaviour); the A-spec selector field (asset `gt_aspec_logo_mini` only if used). Not the front-end global (GT3 has no such mode: treat as 0). The toggle input is race/core's. |
| Verdict | **port** the look and the flags; **do not port** the GT Concept front-end gate |

### C15. Demo HUD: car name only

| Field | |
|---|---|
| What | In a demo (replay mode 2) the item mask is `RD_SHOW_CAR_NAME` only; GT3 uses the replay display options (`GameOptions::displayFlags`) for demos as for replays. Race mode 0xF still shows everything. |
| Where | L2 `RaceDisplay::virtual_3` 0x00191130..0x00191174 (US 0x001934B8, T154). |
| Kind | changed logic |
| Evidence | `beq $v1(replay),2` with `addiu $s3,$zero,2` in the delay slot. |
| Port | One case. |
| Verdict | **port** |

### C16. Time limit display

| Field | |
|---|---|
| What | `drawTimeLimit(sec)` shows `max(sec − 3, 0)` seconds and "GAME OVER" once that is 0 (3 s before the real end); label and value at (16, 320) / (16, 336) (GT3 (8, 400) / (8, 416)), label colour (0.8, 0.8, 0.7, 0.9), the value red (0x800000F0) below 10 s (GT3 (0.8, 0.2, 0.1, 0.9)); "GAME OVER" in `sw_20`, red, centred on (320, 224) (GT3 left-aligned at (234, 224) in the default colour). |
| Where | L2 `RaceDisplay::drawTimeLimit` 0x001900B8 (US 0x00192678, T154); the caller passes `timeRemain / 60` as GT3. |
| Kind | changed logic |
| Evidence | `addiu $s0,$a1,-3; slti; movn $s0,$zero`; `setPos(0x10,0x140)`, `(0x10,0x150)`; `race_align_posf(…, 2)` on 320, y 224. |
| Port | The layout and colours. The −3 s: only together with whatever race/core changed in the time limit (Lupo 2's arcade timer may run 3 s long); ask race/core. |
| Verdict | **port** the look; the offset **only with race/core's counterpart** |

### C17. Two new car warnings (Pod stop area)

| Field | |
|---|---|
| What | `showWarning(kind)` handles kinds 1..17 (GT3 1..12): 16 → "一時停止してサイドブレーキを引いてください" (stop and pull the handbrake, amber 0x801880D0), 17 → "一時停止エリアに戻ってください" (return to the stop area, red), each for 0.2 s renewed every frame; 13..15 show nothing (new kinds without a message). |
| Where | L2 `RaceDisplay::showWarning` 0x00190228 (US 0x001927E0, T154), jump table 0x00341F60 (17 entries), texts RACEDISPLAY[56] / [58]. The kind is the car's `msgKind` (L2 `state+0x5F8`), written by the Pod rule engine (L2 0x001F7708..0x001F7D98, physics / race rules). |
| Kind | changed logic + new data |
| Evidence | table targets 0x001902B0 (`lw -0x2370` = [56]) and 0x001902E8 (`-0x2368` = [58]). |
| Port | Two cases and two texts. Useless without the Pod rules. |
| Verdict | **port with the Pod mode** |

### C18. New information event 7: the Pod sat down

| Field | |
|---|---|
| What | `pollInformationEvents` handles information 7: for the player's car, the warning message shows "おすわりしました！" (it sat down!) in blue (0x80FF8080) for 2 s and plays SE 0x15 (the start beep); the walk then ends. |
| Where | L2 `RaceDisplay::pollInformationEvents` 0x00191E00 (US 0x001941C0, T154), jump table 0x00341FC0 (7 entries, GT3 6), text RACEDISPLAY[57]. Posted by the Pod rule engine (race rules). |
| Kind | changed logic + new data |
| Evidence | 0x00191FC0..0x00192004: `addiu $a0,$s1,0xF34`, `ori 0x8080`, `lw -0x236C` ([57]), `jal race_se_play(0x15)`. |
| Port | One case, one text. |
| Verdict | **port with the Pod mode** |

### C19. The display update takes a time step

| Field | |
|---|---|
| What | `RaceDisplayBase` slot 2 (`virtual_2`) takes a float. The car icons, logo fader, event board, title and mode message are updated only when it is > 0 (and after the other elements, not before the warning). The races pass 1/60 (`RaceSinglePlayer::virtual_28` L2 0x0018041C); the HUD toggle passes 0 to re-lay-out at once without advancing anything. |
| Where | L2 `RaceDisplay::virtual_2` 0x00190510 (`mov.s $f22,$f12`), 0x00190AA4..0x00190AEC; `SimpleExamDisplay::virtual_2` and the split / Pod displays pass it on. US 0x00192A58 (no argument). |
| Kind | changed interface + logic |
| Evidence | `c.lt.s $f20(0),$f22; bc1f` around the six updates. |
| Port | Signature change through `RaceDisplayBase` and every override / caller (race modes: other area). |
| Verdict | **port** (needed by C3) |

### C20. `SimpleExamDisplay` / `SimpleExamMessages` (GT3's time-trial display, generalised)

| Field | |
|---|---|
| What | Same classes as GT3's `RaceTimeTrialDisplay` / `TimeTrialExaminationMessages` (same vtable shapes: 24 and 8 slots; same sizes + 4), renamed and generalised: (a) phase 0 waits for event 0xA **or** the new 0xB (the Pod's disqualification) **whose argument is this display's player car** (GT3: event 0xA, any car); (b) a "stopper" (`setStopper`, messages +0x1C): when set, the message holds at full strength instead of fading out (sets "stopped" +0x1D) and the disqualification jingle (BGM 0x20) is not posted; (c) phase 2 no longer gives up after 1800 frames; (d) the failure text is centred on the buffer passed in (w = buf+0x20 or width, h = buf+0x24 or display height) instead of (320, 224) — `F_0018ABC8` takes w, h; (e) the text is RACEDISPLAY[59] (the same "失格です" as [8]); (f) `reset` sets `passed = 1`; the frame timer stops at −1. The time trial (stopper 0, car 0) behaves as in GT3. |
| Where | L2 vtables 0x0032D4E8 / 0x0032D5B8; ctor 0x00185FC0, dtor 0x00186068, reset 0x001860E0, setPhase 0x00186178, `updateDisqualify` 0x00186190, `endDisqualify` 0x001863E0, `virtual_2` 0x00186438, `virtual_3` 0x00186658, `virtual_0` 0x00186710; messages ctor 0x00185AB0, reset 0x00185B38, start 0x00185B60, update 0x00185BB8, tick 0x00185D10, render 0x00185D30, inline 0x002C5018..0x002C5088 (`setStopper` 0x002C5088); failure text L2 0x00188870. GT3: T141 (US 0x00188478..0x00188FE0), `F_0018ABC8` US 0x0018ABC8 (T146). Users: `RaceTimeTrial` ctor L2 0x0018577C, `RacePODDisplay` (C21). |
| Kind | changed logic + new fields (class generalised) |
| Evidence | 0x001864C0..0x00186560: loop over types 0xA / 0xB (`movn`), `findNth`, compare with `lb -0x5741` (+0xA8BF, `playerCar`); 0x001865AC `lbu -0x5710` (+0xA8F0 = stopper) gates `post(0x15, 0x20)`; 0x00185C5C stopper test in the hold state; 0x001862A8 only `finished` tested; 0x00185F10..0x00185F70 width / height passed to 0x00188870 (`sra 1` of both). |
| Port | Generalise GT3's T141 in place (two bytes, the event filter, the stopper, the centring); the licence display's caller passes (640, 448). Needs event 0xB only with the Pod (C25). |
| Verdict | **port** (base of C21 / C22; harmless for the time trial) |

### C21. `RacePODDisplay`

| Field | |
|---|---|
| What | The Pod mode's display: a `SimpleExamDisplay` whose `put_start_display` (live only) puts three texts on the event board (green, `sw_16`, 3 s each, the first after 2 s): "ジャンプ台の手前に一時停止エリアがあります" (there is a stop area before the jump ramp), "そこでは完全に停止し、サイドブレーキを引いてください" (stop completely there and pull the handbrake), "サイドブレーキを引くと pod がおすわりします" (pull the handbrake and the Pod sits down). Disqualification (event 0xB) through C20. |
| Where | New. L2 vtable 0x00330660 (24 slots), ctor 0x001B40E8, `put_start_display` 0x001B4118, dtor 0x002C97E8, `__tf` 0x002C9850; built by `RacePOD`'s ctor (L2 0x0018D4C8). Texts RACEDISPLAY[60..62]. |
| Kind | new class |
| Evidence | `lw 0xF0/0xF4/0xF8` off the text table 0x002DDBB0; `lui 0x4000` (delay 2.0) on the first. |
| Port | Small class on C20; the texts. Only with the Pod race mode (race/modes; FINDINGS §12 recommended deferring the mode). |
| Verdict | **port with the Pod mode** |

### C22. `RacePOD2PDisplay` and `RaceSplitDisplayBase`

| Field | |
|---|---|
| What | `RaceSplitDisplay` is split into an abstract `RaceSplitDisplayBase` (vptr, target display +4, pure `getDisplay(i)` slots 18 / 19, `getCurrentDisplay`) and `RaceSplitDisplay` (two `RaceDisplay` at +8, stride 0xA8D4, display ids 1 / 0); every forwarding method loops through `getDisplay(i)`. Behaviour of the ordinary split screen unchanged. `RacePOD2PDisplay` derives from the base with two `RacePODDisplay` (stride 0xA908): `virtual_0` turns both messages' stoppers on; `virtual_2` posts event 1 (end of the race) once both players' messages are held (both disqualified). |
| Where | New. L2 `RaceSplitDisplayBase` vtable 0x0032F1A8, ctor 0x00193720, dtor 0x00193738, linkonce 0x002C67C8 / 0x002C6820 / 0x002C6850; `RaceSplitDisplay` vtable 0x0032F0F8 (20 slots, GT3 18), ctor 0x00193F58 (US 0x00195B88, T155), `getDisplay` 0x002C68D0 / 0x002C68E8; `RacePOD2PDisplay` vtable 0x003305B0, ctor 0x001B41D0, `virtual_0` 0x001B4498, `virtual_2` 0x001B43F8, dtor 0x001B4318, `getDisplay` 0x002C98F0 / 0x002C9908; built by `RacePOD2P`'s ctor (L2 0x0018D6D0). |
| Kind | new classes + refactor |
| Evidence | 0x001B44C0..0x001B44D0 `ori 1` into each display's +0xA8D4+0x1C; 0x001B4428..0x001B4480 test of +0xA8F9 (+0x1D) of both, `post(1)`, flag +0x15218. |
| Port | The base-class refactor of T155 and the Pod 2P class, only with the Pod 2P race mode. |
| Verdict | **port with the Pod 2P mode** |

### C23. Free run: start messages live only

| Field | |
|---|---|
| What | `RaceFreeRunDisplay::put_start_display` posts its two texts ("timing starts at the next control line", "START ends the session") only in a live session (GT3 also in its replays), and sets the board font `sw_16` explicitly. |
| Where | L2 0x00187218 (US 0x001899D0, T143). |
| Kind | changed logic |
| Evidence | `lw $v0,0xC($s0); bnez` around the two `addEvent`. |
| Port | One test. |
| Verdict | **port** |

### C24. Licence result screens: one frame shorter

| Field | |
|---|---|
| What | `updatePass` waits 5 frames before the pass screen (GT3 6); `updateFail` waits 47 frames after the fail screen (GT3 48). Everything else of the licence display (texts, prize, colours, timings, events) is the same apart from the BGM event's number (C25). |
| Where | L2 `RaceLicenseDisplay::updatePass` 0x00189CA0 (US 0x0018BF98), `updateFail` 0x00189EF8 (US 0x0018C1E0), T147. |
| Kind | changed constants |
| Evidence | `addiu $v1,$zero,0x5` vs `0x6`; `0x2F` vs `0x30`. |
| Port | Two constants. |
| Verdict | **port (trivial)** |

### C25. Race event renumbering (dependency)

| Field | |
|---|---|
| What | Lupo 2 inserts race event 0xB (posted at L2 0x001F7BC4 by the Pod rule engine: the Pod disqualification). Every event from GT3's 0xB up moves by one: lap time 0xB→0xC, check point 0xC→0xD, general time 0xE→0xF, time diff 0xF→0x10, lap diff 0x10→0x11, speed 0x11→0x12, information 0x12→0x13, BGM 0x14→0x15, view mode 0x15→0x16, record full 0x16→0x17, error 0x17→0x18. The HUD's uses (`getEventType` copies, `race_display_poll_events` / `drop_events`, `pollViewModeEvent`, `ErrorEventView::collect` / `drain`, every BGM post) only follow. |
| Where | T194, T154, T185, T141, T146, T147 (all the "imms +1" pairs in the noise list). |
| Kind | none of its own (follows race/core's `RACE_EVENT_*`) |
| Evidence | getEventType 11→12, 12→13, 14→15, 15→16, 16→17, 17→18, 18→19; drop list {0xC, 0xD, 0xF, 0x16, 0x10, 0x11, 0x13, 5}. |
| Port | If race/core adds the Pod event, append it after GT3's last event instead of inserting it (no renumbering, no risk to replays / i.LINK); the HUD then only needs C20's 0xA-or-new test. |
| Verdict | **do not renumber**; take the new event as an appended number |

### C26. HUD texts (RACEDISPLAY table)

| Field | |
|---|---|
| What | The table grows 56 → 63 entries: 56 "stop and pull the handbrake" (C17), 57 "it sat down!" (C18), 58 "return to the stop area" (C17), 59 "失格です" (disqualified, C20), 60..62 the Pod rules (C21). Re-cased: 19 "QUALIFY"→"Qualify", 20 "DEMONSTRATION"→"Demonstration", 21 "LIVE"→"Live", 22 "REPLAY"→"Replay"; 24 "P.P."→"PP". |
| Where | L2 table 0x002DDBB0 (US `g_002C43E8`, `src/game/data/messages/T186_msg_race_display.cpp`). |
| Kind | new / changed data |
| Evidence | EUC-JP strings dumped from both tables (indices 0..55 otherwise identical). |
| Port | Seven entries (English texts to write for the US build), five re-cased labels. |
| Verdict | **port** (56..62 with the Pod mode) |

### R1. Removed: the second event board is no longer drawn

| Field | |
|---|---|
| What | `events2` (GT3 `RaceDisplay` +0x838, L2 +0x978) is still constructed, reset and laid out (`RD_ITEM_EVENTS2`), but Lupo 2's `virtual_3` no longer draws it in the bottom-right panel. GT3 never adds an event to it either, so nothing visible is lost. |
| Where | L2 `virtual_3` 0x00190FE0 (no reference to +0x978); GT3 US 0x00193D1C..0x00193D34. |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | +0x978 only in the ctor, dtor, `setLayout` 0x0019295C and `disableAll` 0x00192D64. |
| Port | — |
| Verdict | **do not port** (keep GT3's draw) |

### R2. Removed: the data-logger exception of the 16:9 projection

| Field | |
|---|---|
| What | GT3's `virtual_3` keeps the 4:3 projection in 16:9 mode while the data logger runs (`org->loggerMode != 0`); Lupo 2 drops that test. The logger is a GT3 feature. |
| Where | L2 0x001910B4 (no `0x70(org)` test); US 0x00193594..0x0019359C. |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | GT3 `lw $v1,0x70($v0); bnez`. |
| Port | — |
| Verdict | **do not port** (keep GT3's test) |

## Names (Lupo 2's linkonce symbols for GT3 functions; no change)

Useful when naming our sources; the code is the same:
`RaceGearDisplay`: `setShiftWarning` = `setTimingLamp`, `setLampOffset` =
`setTimingLampOffset`, `setBright` = `setClutch` (the clutch is engaged),
`setSuggestBlink` = `setBrakingSign` (the AI's suggested gear blinks as a
braking sign). `RaceLapTimesDisplay`: `setLines` = `setDisplayLaps`,
`setLineSpacing` = `setLineHeightScale`. `RaceTireWearDisplay`: `setColor` =
`setTireWearColor`, `setWarning` = `setWheelWarning`. `RaceOdometer::setValue`
= `setDistance`. `RaceBoostmeter::setVisible` = `setBoost`.
`RaceRoundTachometerBase`: `setTransmission` = `setTransmissionType`,
`updateScale` = `build`, `setImage(const char*)` = `selectTexture`,
`setImage(Tex1Image*)` = `setTexset`; `RaceBattleTachometer` likewise.
`RaceCarIconDisplay::setIcon` = `setCarIcon`. `RaceDisplay`: `getImages` =
`getImagePack`, `getPanels` = `getPanelFlags`, `isLive` / `setLive` =
`isLiveMode` / `setLiveMode`, `setFinishDisplay` = `enableFinishDisplay`,
`ackReplayRecorded` = `inhibitRecordEndDisplay`, `setDisplayNumber` =
`setDisplayID` (+ `getDisplayID`), `setCarIcons` = `setCarIconHolder`
(`CarIconHolder*`), new plain getters `getPlayerCarNumber`,
`getTargetCarNumber`. `RaceDisplayObjectBase`: `drawIfEnabled()` /
`drawIfEnabled(GSBuffer&)` = `render`, `setEnabled` = `enable`,
`race_display_align(f)` = `align`, `race_align_shift_x` = `wideOffset`,
`setImagePack` = `setTexset`. `RaceDisplayInformationEvent::setInformation` /
`getInformation` = `setType` / `getType`. `LicenseExaminationMessages`
`setState` / `nextState` / `getState` = `specifyPhase` / `nextPhase` /
`getPhase`; `RaceLicenseDisplay::setSubPhase` = `specifySubPhase`.

## For other areas

- **race/core**: the race event 0xB (Pod disqualification) and the
  renumbering (C25 — recommend appending); the SELECT HUD toggle L2
  0x0017F9A0 called from `RaceBasic::virtual_30` and `RaceMachineTest::
  virtual_30` (C3); the mini-logo toggle in `RaceBasic::virtual_28` L2
  0x0017F548..0x0017F62C (C14); `RaceDisplayBase::virtual_2(float)` callers
  pass 1/60 (C19); the time-limit 3 s offset (C16) may pair with a timer
  change there; `UnitArena<64>` linkonce L2 0x002C5F90 / 0x002C5FE8.
- **race/modes**: `RaceTraining` / `RaceTrainingInformation` (L2
  0x001872B0..0x00187560: information ctor / dtor / car set-up, operator new
  / delete on 0x002DD368, name "RaceTraining", create 0x32380 bytes, ctor =
  `RaceSinglePlayer` + licence BGM + `RaceLicenseDisplay` +
  `DynamicsConductorLicense`), placed in this folder by the tool;
  `RacePOD` / `RacePOD2P` ctors build C21 / C22 (L2 0x0018D498 / 0x0018D6A0);
  `MiniPrize` linkonce L2 0x002C5628 / 0x002C5670 / 0x002C56C8;
  `prize_rank_name` is L2 0x00187E80 (unpaired, same code).
- **physics / race rules**: the display-rpm spring (C2: L2 0x001E4658 reset,
  0x001E4698 update in the drivetrain step 0x001E6A90, 0x001E4728 read;
  arrays 0x004A9520 / 0x004A9540); the Dualnote accessors the gauge reads
  (C1: 0x001E0288, 0x001E02C0, 0x001E02E0); new `msgKind` values 13..17 (16 /
  17 from the Pod stop-area rule, C17) and information event 7 (C18) from the
  Pod rule engine 0x001F7708..0x001F7D98.
- **car/model**: L2 0x001AE778..0x001B00D8 (Lupo-2-only vehicle / bike-rider
  code: `pdiRiderman` bones 0x001AE818, model-set loading 0x001AE940 (disc) /
  0x001AEA78 (host), rider draws 0x001AED18 / 0x001AEED0 / 0x001AF1B8,
  0x001B00D8 calling into 0x00208D28.. (near race/dynamics), a 0x640-byte
  buffer allocated at `race_begin` 0x001AF6A8 / freed 0x001AF6D0) and the
  `Geomath` templates 0x002C8D58..0x002C90A0, placed in this folder by the
  tool.
- **course**: 0x0019B098 (`RaceMiniMapData` relocation out of line, GT3
  inline `RaceMiniMapData::relocate` US 0x0019E230) and 0x001B4B88, both
  called by the course relocator L2 0x001AA3C8.
- **camera**: 0x001AE390, a helper of `OnBoardCamera::applyView`
  (rotations / translate from +0x44..+0x60).
- **effects**: `FlareClass::setGlareId` linkonce L2 0x002C7B70.
- **data/messages**: C26 (T186).
- **assets** (the user's transplant list): `race/<country>/display.imgs`
  images `meter_sign_dualnote`, `replay_sign_accel`, `replay_sign_brake`,
  `replay_sign_gtforce`, `replay_sign_car`, `car_pointer`, `rev_14`,
  `revb_14`, optionally `gt_aspec_logo_mini`. Fonts `sq_9#1` and `sw_20#1`
  already exist in GT3. Lupo 2's per-mode layout data (`racedetail` records
  of `RaceModeDatabase`) may carry the capacity-monitor record (C4); GT3's
  does not, and C4's tables place it anyway.

## Tool notes

- `lupo2.py`'s `_find_gp` finds no `$gp` in either image (both set it with
  `daddu $gp,$a0,$zero` from `$a0`: L2 0x00362B70, US 0x003579F0), so
  gp-relative float constants are never compared. With the two values set by
  hand (scratch rerun) the HUD gained only rounding differences
  (106.66666 / 106.66667, 999.89996 / 999.90002, 200.31999 / 200.32001) and
  the real ones above (0.2 / 0.1 in `drawTimeLimit`, 0.3 / 1.42857 in
  `RaceMessageDisplay::draw`, 1/6 in the needle and steering).
- Lupo 2 calls virtuals as `addiu $v,$v,d; lh 0($v); lw 4($v)`; GT3 as
  `lh d($v); lw d+4($v)`. `features()` counts only the second as a slot, so
  "vslots only L2: indirect / only US: <n>" is noise in every pair.
- FDE-less leaf functions in Lupo 2 get bounds up to the first `jr $ra`
  (`race_format_time` L2 0x00197FE8 shows 16 instructions but runs to
  0x00198148), which makes size-mismatch pairs that are not.
- Two real changes sat at diff 0.10 / 0.18 (C8, C9): table-driven code
  whose immediates match; the tables do not.

## Wrong pairs

| L2 | paired with (how) | is | right twin |
|---|---|---|---|
| 0x00186710 | US 0x00188EE0 `__tf28TimeTrialExaminationMessages` (order) | `SimpleExamDisplay::virtual_0` | US 0x00188DE0 |
| 0x001863E0 | US 0x00188E70 `__tf20RaceTimeTrialDisplay` (order) | `SimpleExamDisplay::endDisqualify` | US 0x00188BB8 |
| 0x00195A58 | US 0x0019AA38 `race_copy_string` (calls) | `RaceMessageDisplay::setText` | US 0x00197E90 (`race_copy_string` = L2 0x001982E0) |
| 0x00195AF0 | US 0x00197E90 `RaceMessageDisplay::setText` (calls) | `RaceMessageDisplay::clear` | US 0x00199C90 (inline) |
| 0x002C82A8 | US 0x0019DBC8 `RaceRoundTachometerBase::setImage(const char*)` (order) | `RaceBattleTachometer::setTexset` = `setImage(Tex1Image*)` | US 0x0019D8D0; the round one is L2 0x002C7F30 |
| 0x002C82C8 | US 0x0019DBF8 `RaceRoundTachometerBase::updateScale` (order) | `RaceBattleTachometer::selectTexture` | US 0x0019D8B0; `updateScale` is L2 0x002C7F00 |
| 0x002C7B08 | US 0x00199D38 `RaceLapTimesDisplay::setLines` (order) | `RaceCapacityMonitor::setCharge` | new; `setLines` is L2 0x002C7130 |
| 0x00196570 | US 0x00196570 `RaceDisplayObjectBase::update` (vtable) | `RaceABMonitor::update` (new override) | none; base update is L2 0x00194270 |
| 0x001969B8 | US 0x00196570 `RaceDisplayObjectBase::update` (vtable) | `RaceSteeringDisplay::update` (new override) | none |
| 0x002C5650 | US 0x0018BD18 `ExaminationDisplayer::~ExaminationDisplayer` (order) | another inline copy | L2 0x002C4FA0 |
| 0x00197FE8 | US 0x0019A7F0 `race_format_time` (calls) | right pair, wrong L2 bounds | — |

Missed pairs found by hand (unpaired on one side): RaceTimeTrialDisplay
`updateDisqualify` / `virtual_2` / `virtual_3` = L2 0x00186190 / 0x00186438 /
0x00186658; `RaceLicenseDisplay::updatePass` / `updateFail` /
`endExamination` = L2 0x00189CA0 / 0x00189EF8 / 0x0018A120;
`RaceDisplay::showWarning` / `setLayout` / `disableAll` / `layoutMeters` /
`race_display_apply_layout` = L2 0x00190228 / 0x00192570 / 0x00192A98 /
0x00193018 / 0x00193588; `RaceSplitDisplay` ctor = L2 0x00193F58;
`RaceDisplayFader::update` = L2 0x00194400; `RaceLapTimesDisplay::
setCurrentLapTime` = L2 0x00195590; `race_copy_string` = L2 0x001982E0;
`ExamMessageLine::init` / `update` = L2 0x00188980 / 0x001889D0;
`SelectMeterScale(RaceCarState*)` = L2 0x001DA338; `level_to_rgb` (T286) =
L2 0x001DD858 (identical); `prize_rank_name` = L2 0x00187E80.

## Noise and classification: every pair at >= 0.25

Columns: diff, L2 (instructions), US (instructions), how, name, file,
verdict. "real: Cn" points at the entry above.

| diff | L2 | US | how | name | file | verdict |
|---|---|---|---|---|---|---|
| 0.87 | 00197FE8 (16) | 0019A7F0 (94) | calls | race_format_time | T158_race_display_draw | bounds artefact: an FDE-less leaf, the tool stops at the first jr; the function runs to 00198148 and matches race_format_time (noise) |
| 0.87 | 002C63D0 (12) | 00195958 (4) | name | RaceDisplay::innerPanel (innerPanel__11RaceDis | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.80 | 00196570 (5) | 00196570 (2) | vtable | RaceDisplayObjectBase::update | T156_race_display_object | wrong pair (vtable): L2 is RaceABMonitor::update, a new override (C5); the base update is L2 00194270 |
| 0.80 | 00195A58 (30) | 0019AA38 (24) | calls | race_copy_string | T158_race_display_draw | wrong pair (calls): L2 is RaceMessageDisplay::setText (twin of 0x00197E90); race_copy_string is L2 001982E0 |
| 0.79 | 00194090 (56) | 00195CC0 (44) | vtable | RaceSplitDisplay::~RaceSplitDisplay | T155_race_split_display | noise: destructor (deleting form / EH codegen) |
| 0.78 | 00186710 (11) | 00188EE0 (20) | order | __tf28TimeTrialExaminationMessages | T141_race_time_trial_display | wrong pair (order): L2 is SimpleExamDisplay::virtual_0, twin of RaceTimeTrialDisplay::virtual_0 0x00188DE0 |
| 0.78 | 00193918 (33) | 00195EB8 (22) | vtable | RaceSplitDisplay::virtual_2 | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.75 | 001938A0 (29) | 00195E60 (22) | vtable | RaceSplitDisplay::virtual_1 | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.74 | 00193ED0 (33) | 00196330 (26) | vtable | RaceSplitDisplay::setCenterPanel | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.74 | 00193E48 (33) | 001962C8 (26) | vtable | RaceSplitDisplay::setBottomRightPanel | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.74 | 00193DC0 (33) | 00196260 (26) | vtable | RaceSplitDisplay::setBottomLeftPanel | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.74 | 00193D38 (33) | 001961F8 (26) | vtable | RaceSplitDisplay::setTopRightPanel | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.74 | 00193CB0 (33) | 00196190 (26) | vtable | RaceSplitDisplay::setTopLeftPanel | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.74 | 00193A80 (33) | 00195FC0 (26) | vtable | RaceSplitDisplay::innerPanel | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.74 | 001939A0 (33) | 00195F10 (26) | vtable | RaceSplitDisplay::virtual_3 | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.74 | 00193770 (33) | 00195D70 (26) | vtable | RaceSplitDisplay::setOrganization | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.73 | 001937F8 (41) | 00195DD8 (34) | vtable | RaceSplitDisplay::virtual_0 | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.72 | 00193648 (7) | 00195208 (10) | calls | race_display_find_image | T154_race_display | noise: same tail call to image_pack_find (j instead of jal) |
| 0.70 | 00194280 (12) | 00196580 (10) | vtable | RaceDisplayObjectBase::drawMeter | T156_race_display_object | noise: vtable call addressing (addiu+lh 0/lw 4 vs lh d/lw d+4) |
| 0.70 | 002C5808 (8) | 0018C758 (6) | name | RaceLicenseDisplay::nextSubPhase | T147_race_license_display | noise: inline accessor (bit-field store, offset) |
| 0.70 | 001863E0 (22) | 00188E70 (20) | order | __tf20RaceTimeTrialDisplay | T141_race_time_trial_display | wrong pair (order): L2 is SimpleExamDisplay::endDisqualify, twin of RaceTimeTrialDisplay::endDisqualify 0x00188BB8 |
| 0.69 | 002C4FD8 (9) | 00188F38 (4) | vtable | ExaminationDisplayer::activate | T141_race_time_trial_display | noise: bool bit-field store (active/finished bytes of one word) |
| 0.68 | 002C69F0 (28) | 001967A8 (26) | name | RaceDisplayObjectBase::setRegion (setRegion__2 | T156_race_display_object | noise: inline accessor (bit-field store, offset) |
| 0.67 | 002C57E0 (5) | 0018C780 (6) | name | RaceLicenseDisplay::getPhase | T147_race_license_display | noise: inline accessor (bit-field store, offset) |
| 0.67 | 001969B8 (3) | 00196570 (2) | vtable | RaceDisplayObjectBase::update | T156_race_display_object | wrong pair (vtable): L2 is RaceSteeringDisplay::update, a new override (C6) |
| 0.65 | 00195DB0 (22) | 00198188 (24) | vtable | RaceEventDisplay::draw | T157_race_display_parts | noise: 8 messages, stride 0xB0 (was 0x88: RaceMessageDisplay grew, C12) |
| 0.64 | 002C5000 (5) | 00188F30 (2) | vtable | ExaminationDisplayer::deactivate | T141_race_time_trial_display | noise: bool bit-field store |
| 0.64 | 001969C8 (178) | 00198B90 (76) | vtable | RaceSteeringDisplay::draw | T157_race_display_parts | real: C6 steering display redrawn |
| 0.63 | 002C8230 (28) | 0019D8F8 (26) | name | RaceBattleTachometer::setRegion (setRegion__20 | T161_race_tachometer | noise: inline accessor (bit-field store, offset) |
| 0.63 | 002C7E90 (28) | 0019DC18 (26) | name | RaceRoundTachometerBase::setRegion (setRegion_ | T161_race_tachometer | noise: inline accessor (bit-field store, offset) |
| 0.63 | 002C64F0 (13) | 00195880 (10) | name | RaceDisplay::setCenterPanel (setCenterPanel__1 | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.63 | 002C64B8 (13) | 001958A8 (10) | name | RaceDisplay::setBottomRightPanel (setBottomRig | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.63 | 002C6480 (13) | 001958D0 (10) | name | RaceDisplay::setBottomLeftPanel (setBottomLeft | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.63 | 002C6448 (13) | 001958F8 (10) | name | RaceDisplay::setTopRightPanel (setTopRightPane | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.61 | 00190FE0 (615) | 001934B8 (572) | vtable | RaceDisplay::virtual_3 | T154_race_display | real: C3/C14/C15 show mask, demo HUD, logo / car name gates, gauge draw |
| 0.61 | 0018A178 (119) | 0018C458 (90) | vtable | RaceLicenseDisplay::virtual_2 | T147_race_license_display | real: C24 virtual_2 (event / BGM renumbering only) |
| 0.60 | 00188478 (63) | 0018A798 (90) | calls | draw_shadow_text | T146_license_examination_messages | noise: Color8 setters inlined |
| 0.60 | 002C6558 (4) | 00195850 (4) | name | RaceDisplay::setTargetCarNumber (setTargetCarN | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.60 | 002C6538 (4) | 00195860 (4) | name | RaceDisplay::setPlayerCarNumber (setPlayerCarN | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.60 | 002C6400 (4) | 00195948 (4) | name | RaceDisplay::innerPanel (innerPanel__C11RaceDi | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.60 | 002C5838 (4) | 0018C738 (4) | name | RaceLicenseDisplay::setInput | T147_race_license_display | noise: inline accessor (bit-field store, offset) |
| 0.60 | 002C5828 (4) | 0018C748 (4) | name | RaceLicenseDisplay::getSubPhase | T147_race_license_display | noise: inline accessor (bit-field store, offset) |
| 0.59 | 002C6410 (13) | 00195920 (10) | name | RaceDisplay::setTopLeftPanel (setTopLeftPanel_ | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.58 | 00193C08 (22) | 00196100 (18) | vtable | RaceSplitDisplay::virtual_9 | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.58 | 00193BB0 (22) | 001960B8 (18) | vtable | RaceSplitDisplay::setPlayerCarNumber | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.58 | 00193B58 (22) | 00196070 (18) | vtable | RaceSplitDisplay::setTargetCarNumber | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.58 | 00193A28 (22) | 00195F78 (18) | vtable | RaceSplitDisplay::drawTarget | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.58 | 001900B8 (91) | 00192678 (82) | string | RaceDisplay::drawTimeLimit | T154_race_display | real: C16 time limit 3 s offset, new layout |
| 0.57 | 002C6880 (20) | 00196400 (20) | name | __tf16RaceSplitDisplay | T155_race_split_display | noise: typeinfo builder |
| 0.55 | 002C5650 (7) | 0018BD18 (12) | order | ExaminationDisplayer::~ExaminationDisplayer | T146_license_examination_messages | wrong pair (order): ExaminationDisplayer::~ExaminationDisplayer is L2 002C4FA0 |
| 0.55 | 00193C60 (19) | 00196148 (18) | vtable | RaceSplitDisplay::getMirrorObject | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.55 | 00193B08 (19) | 00196028 (18) | vtable | RaceSplitDisplay::innerPanel | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.54 | 002C82C8 (7) | 0019DBF8 (8) | order | RaceRoundTachometerBase::updateScale (updateSc | T161_race_tachometer | wrong pair (order): L2 is RaceBattleTachometer::selectTexture (GT3 0x0019D8B0); updateScale is L2 002C7F00 build |
| 0.54 | 002C82A8 (7) | 0019DBC8 (8) | order | RaceRoundTachometerBase::setImage (setImage__2 | T161_race_tachometer | wrong pair (order): L2 is RaceBattleTachometer::setTexset = setImage(Tex1Image*) (GT3 0x0019D8D0); setImage(const char*) is L2 002C7F30 selectTexture |
| 0.54 | 00190500 (401) | 00192A58 (378) | vtable | RaceDisplay::virtual_2 | T154_race_display | real: C2/C3/C17/C19/C1 rpm spring, HUD toggle, dt, hybrid gauge |
| 0.54 | 001894C8 (18) | 0018B758 (16) | vtable | LicenseExaminationMessages::updateAlways | T146_license_examination_messages | noise: ExamMessageLine::update inlined, FrameTimer tick clamps at -1 |
| 0.54 | 00192DD8 (144) | 00194DF0 (130) | order | RaceDisplay::layoutTimeDisplay | T154_race_display | noise: same 14 cases (tail calls to setLabel) |
| 0.54 | 00195D20 (35) | 001980F8 (36) | vtable | RaceEventDisplay::update | T157_race_display_parts | noise: stride 0xB0, same fades |
| 0.53 | 00195E08 (103) | 001981E8 (94) | calls | RaceEventDisplay::addEvent | T157_race_display_parts | noise: stride 0xB0, setText through the new setText; same timing logic |
| 0.52 | 00196588 (167) | 00198940 (132) | vtable | RaceABMonitor::draw | T157_race_display_parts | real: C5 accel / brake monitor redrawn |
| 0.52 | 00185FC0 (41) | 00188828 (44) | order-gap | RaceTimeTrialDisplay::RaceTimeTrialDisplay | T141_race_time_trial_display | real: C20 SimpleExamDisplay ctor (+ stopper fields) |
| 0.51 | 00192520 (19) | 00194838 (20) | string | RaceDisplay::put_finish_display | T154_race_display | noise: colour 0x80C0C0C0 (C12), text through setText |
| 0.51 | 001924D0 (19) | 001947E8 (20) | string | RaceDisplay::put_start_display | T154_race_display | noise: colour 0x80C0C0C0 (C12), text through setText |
| 0.50 | 002C7260 (47) | 00199BB0 (32) | name | RaceEventDisplay::~RaceEventDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.50 | 002C6678 (3) | 001957F0 (4) | name | RaceDisplay::getMirrorObject (getMirrorObject_ | T154_race_display | noise: inline accessor (bit-field store, offset) |
| 0.50 | 00191B88 (46) | 00193F58 (46) | calls | RaceDisplay::pollLapDiffEvents | T154_race_display | noise: event queue at org+0x84+0x800, type through the vtable |
| 0.50 | 00189C48 (16) | 0018BF50 (12) | calls | RaceLicenseDisplay::reset | T147_race_license_display | real: C24 (reset itself: noise) |
| 0.48 | 00191C40 (67) | 00194010 (64) | calls | RaceDisplay::pollTimeDiffEvents | T154_race_display | noise: same (leader for qualify, else rank 1/2) |
| 0.48 | 002C7988 (25) | 00199710 (8) | name | RaceReplayModeDisplay::~RaceReplayModeDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C7880 (25) | 001997D0 (8) | name | RaceCarIconDisplay::~RaceCarIconDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C77C0 (25) | 00199848 (8) | name | RaceSpeedDisplay::~RaceSpeedDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C7398 (25) | 00199B18 (8) | name | RaceIndicators::~RaceIndicators | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C7140 (25) | 00199D10 (8) | name | RaceMessageDisplay::~RaceMessageDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C7078 (25) | 00199D90 (8) | name | RaceLapTimesDisplay::~RaceLapTimesDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C6FB0 (25) | 00199E10 (8) | name | RaceTimeDisplay::~RaceTimeDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C6E68 (25) | 00199F28 (8) | name | RaceGearDisplay::~RaceGearDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C6DA8 (25) | 00199FA0 (8) | name | RaceOdometer::~RaceOdometer | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C6CD8 (25) | 0019A028 (8) | name | RaceDigitalSpeedmeter::~RaceDigitalSpeedmeter | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C6C10 (25) | 0019A0A8 (8) | name | RaceRankDisplay::~RaceRankDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C6B48 (25) | 0019A128 (8) | name | RaceLapDisplay::~RaceLapDisplay | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 002C6060 (25) | 00199E80 (8) | name | RaceValueDisplayBase::~RaceValueDisplayBase | T157_race_display_parts | noise: destructor (deleting form / EH codegen) |
| 0.48 | 0018A358 (45) | 0018C5C0 (40) | vtable | RaceLicenseDisplay::virtual_3 | T147_race_license_display | noise: same draw (passed ? renderWhenPassing : renderWhenDisqualifying) |
| 0.47 | 001AE418 (10) | 001AE4E0 (16) | order | ErrorEventView::ErrorEventView | T185_error_event_view | noise: setPosition(100, 100) as a tail call |
| 0.46 | 00191D50 (44) | 00194110 (44) | calls | RaceDisplay::pollSpeedEvents | T154_race_display | noise: same |
| 0.45 | 002C4FA0 (13) | 00188F48 (12) | name | ExaminationDisplayer::~ExaminationDisplayer | T141_race_time_trial_display | noise: destructor (deleting form / EH codegen) |
| 0.45 | 001AE440 (13) | 001AE520 (12) | vtable | ErrorEventView::~ErrorEventView | T185_error_event_view | noise: destructor (deleting form / EH codegen) |
| 0.45 | 00185B00 (13) | 001884A8 (12) | calls | TimeTrialExaminationMessages::~TimeTrialExamin | T141_race_time_trial_display | noise: destructor (deleting form / EH codegen) |
| 0.44 | 002C68E8 (5) | 001963D8 (6) | name | RaceSplitDisplay::getDisplay | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.44 | 002C68D0 (5) | 001963D8 (6) | name | RaceSplitDisplay::getDisplay | T155_race_split_display | refactor: forwards through RaceSplitDisplayBase::getDisplay(i) (C22), same behaviour |
| 0.43 | 001944F8 (43) | 00196A38 (30) | string | RaceDisplayFader::draw | T157_race_display_parts | real: C14 additive logo, A-spec logo |
| 0.43 | 00189AF8 (53) | 0018BE40 (46) | calls | RaceLicenseDisplay::RaceLicenseDisplay | T147_race_license_display | noise: layout (messages at +0xA8D4) |
| 0.43 | 001B4590 (31) | 00190230 (32) | order-gap | RaceDisplayEventBase::take | T194_race_display_event | noise: event queue offsets |
| 0.43 | 001981F0 (13) | 0019AA08 (12) | calls | race_rank_name | T158_race_display_draw | noise: same 12 rank names (RACEDISPLAY 37..48) |
| 0.42 | 0018FB20 (4) | 00192128 (6) | vtable | RaceDisplay::virtual_9 | T154_race_display | noise: layoutVariant = (mount != 0) |
| 0.42 | 001B4510 (31) | 001901B8 (30) | calls | RaceDisplayEventBase::post | T194_race_display_event | noise: event queue offsets |
| 0.40 | 0018FB88 (98) | 00192190 (90) | calls | RaceDisplay::setupCarMeters | T154_race_display | noise: same scales; SelectMeterScale(RaceCarState*) wrapper L2 001DA338 not inlined |
| 0.39 | 001B4610 (41) | 001902B0 (40) | calls | RaceDisplayEventBase::peek | T194_race_display_event | noise: event queue offsets |
| 0.39 | 00190C48 (80) | 00193118 (82) | order-gap | RaceDisplay::pollLapTimeEvents | T154_race_display | noise: same |
| 0.38 | 00195AD0 (8) | 00197EE8 (8) | calls | RaceMessageDisplay::setFont | T157_race_display_parts | noise: setFont(name, shadow) stores the new shadow flag (C12) |
| 0.38 | 00194F20 (12) | 001973F8 (12) | calls | RaceValueDisplayBase::setLabel | T157_race_display_parts | noise: race_copy_string is L2 001982E0 (unpaired) |
| 0.37 | 00191E00 (168) | 001941C0 (138) | calls | RaceDisplay::pollInformationEvents | T154_race_display | real: C18 information 7 (Pod sat down) |
| 0.36 | 001956F8 (215) | 00197BF0 (168) | vtable | RaceMessageDisplay::draw | T157_race_display_parts | real: C12 drop shadow, cached image |
| 0.35 | 00194170 (36) | 00196490 (32) | order | RaceDisplayObjectBase::RaceDisplayObjectBase | T156_race_display_object | noise: same base fields |
| 0.35 | 0018E0B8 (690) | 001909E0 (584) | order | RaceDisplay::RaceDisplay | T154_race_display | real: C1/C13/C12 new members (capacity monitor, car sign, larger messages) |
| 0.35 | 00188FD8 (61) | 0018B2C0 (48) | vtable | LicenseExaminationMessages::setup | T146_license_examination_messages | noise: same set-up (bit-field stores) |
| 0.34 | 0019B1E8 (39) | 0019E380 (40) | calls | RaceMiniMap::init | T164_race_minimap | real: C11 car_pointer |
| 0.34 | 00188F10 (49) | 0018B1F8 (50) | vtable | LicenseExaminationMessages::~LicenseExaminatio | T146_license_examination_messages | noise: destructor (deleting form / EH codegen) |
| 0.33 | 002C9B48 (2) | 001907B0 (2) | name | RaceDisplayInformationEvent::getEventType | T194_race_display_event | event renumbering (+1 for events >= 0xB, C25) |
| 0.33 | 002C9AE0 (2) | 00190818 (2) | name | RaceDisplaySpeedEvent::getEventType | T194_race_display_event | event renumbering (+1 for events >= 0xB, C25) |
| 0.33 | 002C9A28 (2) | 001908D0 (2) | name | RaceDisplayCheckPointEvent::getEventType | T194_race_display_event | event renumbering (+1 for events >= 0xB, C25) |
| 0.33 | 002C99C0 (2) | 00190938 (2) | name | RaceDisplayLapTimeEvent::getEventType | T194_race_display_event | event renumbering (+1 for events >= 0xB, C25) |
| 0.33 | 002C67C0 (2) | 001956D8 (2) | name | RaceDisplayGeneralTimeEvent::getEventType | T154_race_display | event renumbering (+1 for events >= 0xB, C25) |
| 0.33 | 002C6768 (2) | 00195730 (2) | name | RaceDisplayLapDiffEvent::getEventType | T154_race_display | event renumbering (+1 for events >= 0xB, C25) |
| 0.33 | 002C6710 (2) | 00195788 (2) | name | RaceDisplayTimeDiffEvent::getEventType | T154_race_display | event renumbering (+1 for events >= 0xB, C25) |
| 0.32 | 00188CA0 (156) | 0018AF98 (152) | order-gap | LicenseExaminationMessages::LicenseExamination | T146_license_examination_messages | noise: prize memory through an out-of-line arena helper L2 001A6BD0 (same arena alloc) |
| 0.32 | 00199BE0 (130) | 0019C540 (126) | vtable | RaceRoundMeterBase::render_2nd | T160_race_meters | noise: same (draw_scale / draw_needle through the vtable) |
| 0.32 | 0019A708 (57) | 0019D278 (54) | calls | RaceRoundTachometerBase::RaceRoundTachometerBa | T161_race_tachometer | noise: EH / bit-field init |
| 0.32 | 00190018 (39) | 001925E8 (36) | string | RaceDisplay::drawCountdown | T154_race_display | noise: same "@count%d" logic |
| 0.30 | 001DA270 (50) | 001D9978 (38) | calls | SelectMeterScale | T279_meter_scale | noise: same formula (x80 / x50 as shifts; redline + 1500) |
| 0.30 | 002C9920 (15) | 001909A0 (16) | name | __tf20RaceDisplayEventBase | T194_race_display_event | noise: typeinfo builder |
| 0.30 | 002C8D18 (15) | 001AE840 (16) | name | __tf14ErrorEventView | T185_error_event_view | noise: typeinfo builder |
| 0.30 | 002C6900 (15) | 001968D0 (16) | name | __tf21RaceDisplayObjectBase | T156_race_display_object | noise: typeinfo builder |
| 0.30 | 002C4F38 (15) | 00188FA0 (16) | name | __tf20ExaminationDisplayer | T141_race_time_trial_display | noise: typeinfo builder |
| 0.30 | 002C3F78 (15) | 00195B48 (16) | name | __tf15RaceDisplayBase | T154_race_display | noise: typeinfo builder |
| 0.30 | 0018F1B0 (518) | 001918D8 (462) | string | RaceDisplay::virtual_0 | T154_race_display | real: C3/C12 show mask field, title / mode message look |
| 0.30 | 002C9AE8 (19) | 001907C8 (20) | name | __tf27RaceDisplayInformationEvent | T194_race_display_event | noise: typeinfo builder |
| 0.30 | 002C9A80 (19) | 00190830 (20) | name | __tf21RaceDisplaySpeedEvent | T194_race_display_event | noise: typeinfo builder |
| 0.30 | 002C9A30 (19) | 00190880 (20) | name | __tf20RaceDisplayDiffEvent | T194_race_display_event | noise: typeinfo builder |
| 0.30 | 002C9960 (19) | 00190950 (20) | name | __tf23RaceDisplayLapTimeEvent | T194_race_display_event | noise: typeinfo builder |
| 0.30 | 002C79F0 (19) | 001996C0 (20) | name | __tf21RaceReplayModeDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C78E8 (19) | 00199780 (20) | name | __tf18RaceCarIconDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C76C0 (19) | 001998D8 (20) | name | __tf19RaceTireWearDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C76A8 (5) | 00199928 (2) | name | RaceSteeringDisplay::setAngle | T157_race_display_parts | noise: inline accessor (bit-field store, offset) |
| 0.30 | 002C75F0 (19) | 00199958 (20) | name | __tf19RaceSteeringDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C7528 (19) | 001999E0 (20) | name | __tf13RaceABMonitor | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C7470 (19) | 00199A58 (20) | name | __tf13RaceBackPanel | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C7400 (19) | 00199AC8 (20) | name | __tf14RaceIndicators | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C7320 (19) | 00199B60 (20) | name | __tf16RaceEventDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C71A8 (19) | 00199CC0 (20) | name | __tf18RaceMessageDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C70E0 (19) | 00199D40 (20) | name | __tf19RaceLapTimesDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C6ED0 (19) | 00199ED8 (20) | name | __tf15RaceGearDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C6E10 (19) | 00199F50 (20) | name | __tf12RaceOdometer | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C6D40 (19) | 00199FD8 (20) | name | __tf21RaceDigitalSpeedmeter | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C6C78 (19) | 0019A058 (20) | name | __tf15RaceRankDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C6BB0 (19) | 0019A0D8 (20) | name | __tf14RaceLapDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C6358 (19) | 00195978 (20) | name | __tf11RaceDisplay | T154_race_display | noise: typeinfo builder |
| 0.30 | 002C62A0 (19) | 001959F0 (20) | name | __tf11RaceMiniMap | T154_race_display | noise: typeinfo builder |
| 0.30 | 002C6118 (19) | 00195AF8 (20) | name | __tf13RaceMeterBase | T154_race_display | noise: typeinfo builder |
| 0.30 | 002C60C8 (19) | 00199E30 (20) | name | __tf20RaceValueDisplayBase | T157_race_display_parts | noise: typeinfo builder |
| 0.30 | 002C5720 (19) | 0018BCB0 (20) | name | __tf26LicenseExaminationMessages | T146_license_examination_messages | noise: typeinfo builder |
| 0.30 | 00196270 (34) | 00198658 (32) | vtable | RaceBackPanel::draw | T157_race_display_parts | noise: same dispatch (<0x40 image, 0x40 frame, 0x41 shape) as tail calls |
| 0.29 | 002C99C8 (20) | 001908E8 (20) | name | __tf26RaceDisplayCheckPointEvent | T194_race_display_event | noise: typeinfo builder |
| 0.29 | 002C82E8 (20) | 0019E1D0 (20) | name | __tf14RaceBoostmeter | T163_race_boostmeter | noise: typeinfo builder |
| 0.29 | 002C80F0 (20) | 0019DA18 (20) | name | __tf20RaceBattleTachometer | T161_race_tachometer | noise: typeinfo builder |
| 0.29 | 002C8020 (20) | 0019DAC8 (20) | name | __tf20RaceRemoteTachometer | T161_race_tachometer | noise: typeinfo builder |
| 0.29 | 002C7F50 (20) | 0019DB78 (20) | name | __tf21RaceOnboardTachometer | T161_race_tachometer | noise: typeinfo builder |
| 0.29 | 002C7D38 (20) | 0019DD50 (20) | name | __tf23RaceRoundTachometerBase | T161_race_tachometer | noise: typeinfo builder |
| 0.29 | 002C7C80 (20) | 0019D090 (20) | name | __tf12RaceBarMeter | T160_race_meters | noise: typeinfo builder |
| 0.29 | 002C7BF8 (20) | 0019D118 (20) | name | __tf14RaceRoundMeter | T160_race_meters | noise: typeinfo builder |
| 0.29 | 002C7B78 (20) | 0019D198 (20) | name | __tf18RaceRoundMeterBase | T160_race_meters | noise: typeinfo builder |
| 0.29 | 002C7828 (20) | 001997F8 (20) | name | __tf16RaceSpeedDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.29 | 002C7018 (20) | 00199DC0 (20) | name | __tf15RaceTimeDisplay | T157_race_display_parts | noise: typeinfo builder |
| 0.29 | 002C6770 (20) | 001956E0 (20) | name | __tf27RaceDisplayGeneralTimeEvent | T154_race_display | noise: typeinfo builder |
| 0.29 | 002C6718 (20) | 00195738 (20) | name | __tf23RaceDisplayLapDiffEvent | T154_race_display | noise: typeinfo builder |
| 0.29 | 002C66C0 (20) | 00195790 (20) | name | __tf24RaceDisplayTimeDiffEvent | T154_race_display | noise: typeinfo builder |
| 0.29 | 002C61E8 (20) | 00195A68 (20) | name | __tf21RaceOnboardSpeedmeter | T154_race_display | noise: typeinfo builder |
| 0.29 | 002C5790 (20) | 0018C798 (20) | name | __tf18RaceLicenseDisplay | T147_race_license_display | noise: typeinfo builder |
| 0.29 | 002C56D0 (20) | 0018BDB0 (20) | name | __tf12LicensePrize | T146_license_examination_messages | noise: typeinfo builder |
| 0.29 | 002C5238 (20) | 00189A90 (20) | name | __tf18RaceFreeRunDisplay | T143_race_free_run_display | noise: typeinfo builder |
| 0.28 | 002C7950 (5) | 00199758 (4) | name | RaceCarIconDisplay::setBlink | T157_race_display_parts | noise: inline accessor (bit-field store, offset) |
| 0.28 | 002C7220 (5) | 00199C58 (4) | name | RaceMessageDisplay::setBlink | T157_race_display_parts | noise: inline accessor (bit-field store, offset) |
| 0.27 | 00187218 (38) | 001899D0 (36) | vtable | RaceFreeRunDisplay::put_start_display | T143_race_free_run_display | real: C23 start messages live only |
| 0.27 | 00188A48 (77) | 0018AD58 (86) | calls | ExamMessageLine::draw | T146_license_examination_messages | noise: Color8::setClamped out of line (L2 001B1DF0) |
| 0.27 | 0019ABE0 (32) | 0019D750 (30) | order-gap | RaceBattleTachometer::RaceBattleTachometer | T161_race_tachometer | noise: bit-field init |
| 0.27 | 00191980 (130) | 00193DA8 (108) | string | RaceDisplay::virtual_16 | T154_race_display | noise: BGM event renumbered (0x14 -> 0x15) |
| 0.27 | 001B46F0 (32) | 00190388 (26) | vtable | RaceDisplayLapTimeEvent::pack | T194_race_display_event | noise: same clamps 0..15 / 0..999 |
| 0.27 | 00189510 (202) | 0018B798 (172) | vtable | LicenseExaminationMessages::renderWhenPassing | T146_license_examination_messages | noise: same colours / positions; prize_rank_name is L2 00187E80 (unpaired) |
| 0.26 | 001890D0 (15) | 0018B380 (12) | vtable | LicenseExaminationMessages::deactivate | T146_license_examination_messages | noise: bit-field store |
| 0.25 | 0019AF18 (50) | 0019E088 (50) | order | RaceBoostmeter::RaceBoostmeter | T163_race_boostmeter | noise: bit-field init, float rounding (200.32) |
| 0.25 | 0019B3D8 (74) | 0019E560 (70) | order | RaceMiniMap::drawCar | T164_race_minimap | real: C11 tinted pointer |
| 0.25 | 00185D30 (163) | 00188630 (126) | order | TimeTrialExaminationMessages::renderWhenDisqua | T141_race_time_trial_display | real: C20 SimpleExamMessages::renderWhenDisqualifying (text centred on the buffer) |
| 0.25 | 00194200 (23) | 00196510 (20) | vtable | RaceDisplayObjectBase::~RaceDisplayObjectBase | T156_race_display_object | noise: destructor (deleting form / EH codegen) |
| 0.25 | 00197F30 (15) | 0019A730 (24) | calls | race_align_posf | T158_race_display_draw | noise: same alignment maths |
| 0.25 | 0018FB38 (20) | 00192140 (20) | order | RaceDisplay::setDisplayCar | T154_race_display | noise: same |
| 0.25 | 001955B0 (43) | 00197AA0 (44) | order | RaceMessageDisplay::RaceMessageDisplay | T157_race_display_parts | noise: default colour 0x80C0C0C0 (C12), shadow off, image NULL |

Below 0.25 (not listed one by one): the remaining 262 pairs were read
through `cmp`; besides C8 / C9 (found by reading the tables) they differ by
register allocation, bit-field stores, vtable-call addressing, EH
bookkeeping (`__throw` → L2 0x002968A0), the +1 event renumbering, and float
rounding.

## Unpaired Lupo 2 functions placed in this folder (133)

| L2 | bytes | name | verdict |
|---|---|---|---|
| 00186190 | 592 | - | C20 SimpleExamDisplay::updateDisqualify (twin of RaceTimeTrialDisplay 0x00188978) |
| 00186438 | 544 | - | C20 SimpleExamDisplay::virtual_2 (twin of 0x00188C10) |
| 00186658 | 180 | - | C20 SimpleExamDisplay::virtual_3 (twin of 0x00188D40; same draw) |
| 001872B0 | 48 | - | other area (race/modes): RaceTraining / RaceTrainingInformation (new mode) |
| 001872E0 | 100 | - | other area (race/modes): RaceTraining / RaceTrainingInformation (new mode) |
| 00187348 | 300 | - | other area (race/modes): RaceTraining / RaceTrainingInformation (new mode) |
| 00187478 | 28 | - | other area (race/modes): RaceTraining / RaceTrainingInformation (new mode) |
| 00187498 | 36 | - | other area (race/modes): RaceTraining / RaceTrainingInformation (new mode) |
| 001874C0 | 36 | - | other area (race/modes): RaceTraining / RaceTrainingInformation (new mode) |
| 001874F8 | 56 | - | other area (race/modes): RaceTraining / RaceTrainingInformation (new mode) |
| 00187530 | 48 | - | other area (race/modes): RaceTraining / RaceTrainingInformation (new mode) |
| 00187560 | 448 | - | other area (race/modes): RaceTraining / RaceTrainingInformation (new mode) |
| 00189CA0 | 596 | - | C24 RaceLicenseDisplay::updatePass (twin of 0x0018BF98; wait 6 -> 5 frames) |
| 00189EF8 | 548 | - | C24 RaceLicenseDisplay::updateFail (twin of 0x0018C1E0; 48 -> 47 frames) |
| 0018A120 | 88 | - | noise: RaceLicenseDisplay::endExamination (twin of 0x0018C400) |
| 00190228 | 244 | - | C17 RaceDisplay::showWarning (twin of 0x001927E0; kinds 16 / 17 added) |
| 00192570 | 1316 | - | C4/C13 RaceDisplay::setLayout (twin of 0x00194888) |
| 00192A98 | 828 | - | noise: RaceDisplay::disableAll (twin of 0x00194BF8; also the new objects, resets the AB / steering trails) |
| 00193018 | 1392 | - | C4 RaceDisplay::layoutMeters (twin of 0x00194FF8; tables built on the stack) |
| 00193588 | 188 | - | noise: race_display_apply_layout (twin of 0x00195168) |
| 00193F58 | 308 | - | C22 RaceSplitDisplay::RaceSplitDisplay (twin of 0x00195B88, on RaceSplitDisplayBase) |
| 00196828 | 312 | - | C5 RaceABMonitor bar strip (new helper) |
| 00196C90 | 48 | - | C6 RaceSteeringDisplay tick / marker strip (new helper) |
| 001976D0 | 76 | - | C1 RaceCapacityMonitor::RaceCapacityMonitor |
| 00197720 | 548 | - | C1 RaceCapacityMonitor::draw |
| 0019B098 | 44 | - | noise / course: RaceMiniMapData relocation out of line (GT3 0x0019E230), caller the course relocator 001AA3C8 |
| 001AE390 | 136 | - | other area (camera): helper of OnBoardCamera::applyView |
| 001AE778 | 36 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AE7A0 | 36 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AE7C8 | 36 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AE7F0 | 36 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AE818 | 72 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AE860 | 100 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AE8C8 | 116 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AE940 | 312 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AEA78 | 272 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AEB88 | 396 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AED18 | 436 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AEED0 | 632 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AF148 | 56 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AF180 | 56 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AF1B8 | 1240 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AF6A8 | 36 | - | other area (app / car model): allocates a 0x640-byte buffer at race_begin (L2 0x001030E8); free at 0x001AF6D0 |
| 001AF700 | 180 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AF7B8 | 100 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AF838 | 552 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AFA60 | 164 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AFB08 | 200 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AFBD0 | 580 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AFE18 | 144 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001AFEA8 | 184 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001B0020 | 184 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001B00D8 | 4680 | - | other area (car/model): vehicle / bike-rider code (see For other areas) |
| 001B4318 | 224 | - | C22 RacePOD2PDisplay::~RacePOD2PDisplay |
| 001B43F8 | 160 | - | C22 RacePOD2PDisplay::virtual_2 (ends the race when both players are held) |
| 001B4498 | 96 | - | C22 RacePOD2PDisplay::virtual_0 (stoppers on) |
| 001B4B88 | 88 | - | other area (course): called by the course relocator 001AA3C8 |
| 002C5018 | 76 | __tf18SimpleExamMessages | C20 SimpleExamMessages inline copy |
| 002C5068 | 8 | specifyPhase__18SimpleExamMessagesi | C20 SimpleExamMessages inline copy |
| 002C5070 | 16 | nextPhase__18SimpleExamMessages | C20 SimpleExamMessages inline copy |
| 002C5080 | 8 | getPhase__C18SimpleExamMessages | C20 SimpleExamMessages inline copy |
| 002C5088 | 24 | setStopper__18SimpleExamMessages | C20 SimpleExamMessages::setStopper (new) |
| 002C5628 | 40 | updateContextToLoopFrame__9MiniPrizeR20P | other area (race/modes): MiniPrize inline copy |
| 002C5670 | 88 | setModel__9MiniPrizeRC15MiniPrizeStatusb | other area (race/modes): MiniPrize inline copy |
| 002C56C8 | 8 | getStatus__C9MiniPrize | other area (race/modes): MiniPrize inline copy |
| 002C5770 | 8 | specifyPhase__26LicenseExaminationMessag | noise: inline accessor (GT3 setState / nextState / getState / setSubPhase) |
| 002C5778 | 16 | nextPhase__26LicenseExaminationMessages | noise: inline accessor (GT3 setState / nextState / getState / setSubPhase) |
| 002C5788 | 8 | getPhase__C26LicenseExaminationMessages | noise: inline accessor (GT3 setState / nextState / getState / setSubPhase) |
| 002C57F8 | 16 | specifySubPhase__18RaceLicenseDisplayUc | noise: inline accessor (GT3 setState / nextState / getState / setSubPhase) |
| 002C5F90 | 84 | _$_t9UnitArena1Ui_64_ | other area (race/core): UnitArena<64> template copy |
| 002C5FE8 | 116 | reset__t9UnitArena1Ui_64_ | other area (race/core): UnitArena<64> template copy |
| 002C63A8 | 24 | get_race_display_flag__C11RaceDisplayi | C3 RaceDisplay show-mask accessor (new field +0xA8D0) |
| 002C6528 | 16 | getPanelFlags__C11RaceDisplay | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C6548 | 16 | getPlayerCarNumber__C11RaceDisplay | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C6568 | 16 | getTargetCarNumber__C11RaceDisplay | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C6578 | 48 | setLiveMode__11RaceDisplayb | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C65A8 | 16 | isLiveMode__C11RaceDisplay | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C65B8 | 12 | inhibitRecordEndDisplay__11RaceDisplay | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C65C8 | 44 | enableFinishDisplay__11RaceDisplayb | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C65F8 | 52 | setMinilogo__11RaceDisplayb | C14 RaceDisplay mini-logo flag / fader accessor (new) |
| 002C6630 | 16 | getMinilogo__C11RaceDisplay | C14 RaceDisplay mini-logo flag / fader accessor (new) |
| 002C6640 | 52 | setCarNameDisplay__11RaceDisplayb | C14 RaceDisplay car-name flag (new) |
| 002C6688 | 8 | setCarIconHolder__11RaceDisplayP13CarIco | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C6690 | 8 | getMiniLogo__11RaceDisplay | C14 RaceDisplay mini-logo flag / fader accessor (new) |
| 002C6698 | 16 | setDisplayID__11RaceDisplayi | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C66A8 | 16 | getDisplayID__C11RaceDisplay | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C66B8 | 8 | getImagePack__11RaceDisplay | name only: GT3 has the accessor (getPanels, isLive/setLive, setFinishDisplay, ackReplayRecorded, setCarIcons, setDisplayNumber, getImages) or a new plain getter |
| 002C67C8 | 76 | __tf20RaceSplitDisplayBase | C22 RaceSplitDisplayBase (new base class) |
| 002C6820 | 48 | getCurrentDisplay__20RaceSplitDisplayBas | C22 RaceSplitDisplayBase (new base class) |
| 002C6850 | 48 | getCurrentDisplay__C20RaceSplitDisplayBa | C22 RaceSplitDisplayBase (new base class) |
| 002C6940 | 60 | render__21RaceDisplayObjectBase | name only: GT3 inline twins drawIfEnabled / setEnabled / race_display_align(f) / race_align_shift_x |
| 002C6980 | 60 | render__21RaceDisplayObjectBaseR8GSBuffe | name only: GT3 inline twins drawIfEnabled / setEnabled / race_display_align(f) / race_align_shift_x |
| 002C6A60 | 36 | enable__21RaceDisplayObjectBaseb | name only: GT3 inline twins drawIfEnabled / setEnabled / race_display_align(f) / race_align_shift_x |
| 002C6AD0 | 8 | setTexset__21RaceDisplayObjectBaseP10PGL | name only: GT3 inline twins drawIfEnabled / setEnabled / race_display_align(f) / race_align_shift_x |
| 002C6AD8 | 20 | align__21RaceDisplayObjectBaseffi | name only: GT3 inline twins drawIfEnabled / setEnabled / race_display_align(f) / race_align_shift_x |
| 002C6AF0 | 20 | align__21RaceDisplayObjectBaseiii | name only: GT3 inline twins drawIfEnabled / setEnabled / race_display_align(f) / race_align_shift_x |
| 002C6B08 | 60 | wideOffset__21RaceDisplayObjectBasei | name only: GT3 inline twins drawIfEnabled / setEnabled / race_display_align(f) / race_align_shift_x |
| 002C6E60 | 8 | setDistance__12RaceOdometerUi | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C6F30 | 28 | setTimingLamp__15RaceGearDisplayb | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C6F50 | 12 | setTimingLampOffset__15RaceGearDisplayii | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C6F60 | 36 | setClutch__15RaceGearDisplayb | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C6F88 | 36 | setBrakingSign__15RaceGearDisplayb | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C7130 | 8 | setDisplayLaps__19RaceLapTimesDisplayi | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C7138 | 8 | setLineHeightScale__19RaceLapTimesDispla | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C7238 | 8 | setTexset__18RaceMessageDisplayP10PGLUte | C12 RaceMessageDisplay image pointer (new field +0x3C) |
| 002C7778 | 40 | setTireWearColor__19RaceTireWearDisplayi | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C77A0 | 32 | setWheelWarning__19RaceTireWearDisplayib | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C7938 | 8 | setCarIcon__18RaceCarIconDisplayP7CarIco | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C7A48 | 76 | __tf19RaceCapacityMonitor | C1 RaceCapacityMonitor |
| 002C7A98 | 100 | _$_19RaceCapacityMonitor | C1 RaceCapacityMonitor |
| 002C7B00 | 8 | setCapacity__19RaceCapacityMonitorf | C1 RaceCapacityMonitor |
| 002C7B10 | 28 | setHybridCar__19RaceCapacityMonitorb | C1 RaceCapacityMonitor |
| 002C7B70 | 8 | setGlareId__10FlareClassi | other area (effects): FlareClass::setGlareId |
| 002C7E40 | 28 | setTransmissionType__23RaceRoundTachomet | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C7F00 | 28 | build__23RaceRoundTachometerBase | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C7F28 | 8 | setTexset__23RaceRoundTachometerBaseP10P | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C7F30 | 28 | selectTexture__23RaceRoundTachometerBase | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C81E0 | 28 | setTransmissionType__20RaceBattleTachome | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C8338 | 28 | setBoost__14RaceBoostmeterb | name only: an inline accessor GT3 has under another name (see "Names") |
| 002C8D58 | 156 | identity__Q27Geomatht13SquareMatrix_2Zfi | other area (car/model): Geomath template copies |
| 002C8DF8 | 104 | __as__Q27Geomatht13SquareMatrix_2Zfi4RCQ | other area (car/model): Geomath template copies |
| 002C8E60 | 52 | copy__Q27Geomatht7Vector_2Zfi3RCQ27Geoma | other area (car/model): Geomath template copies |
| 002C8E98 | 104 | setComponents__Q27Geomatht13SquareMatrix | other area (car/model): Geomath template copies |
| 002C8F00 | 136 | transpose__CQ27Geomatht13SquareMatrix_2Z | other area (car/model): Geomath template copies |
| 002C8F88 | 208 | __ad__CQ27Geomatht13SquareMatrix_2Zfi4RC | other area (car/model): Geomath template copies |
| 002C9058 | 72 | copy__Q27Geomatht7Vector_2Zfi4RCQ27Geoma | other area (car/model): Geomath template copies |
| 002C90A0 | 20 | __Q27Geomatht13SquareMatrix_2Zfi4fffffff | other area (car/model): Geomath template copies |
| 002C9850 | 80 | __tf14RacePODDisplay | C21/C22 RacePODDisplay / RacePOD2PDisplay |
| 002C98A0 | 80 | __tf16RacePOD2PDisplay | C21/C22 RacePODDisplay / RacePOD2PDisplay |
| 002C98F0 | 20 | getDisplay__16RacePOD2PDisplayi | C21/C22 RacePODDisplay / RacePOD2PDisplay |
| 002C9908 | 20 | getDisplay__C16RacePOD2PDisplayi | C21/C22 RacePODDisplay / RacePOD2PDisplay |
| 002C9B38 | 8 | setType__27RaceDisplayInformationEventi | name only: GT3 setInformation / getInformation (0x001907C0 / 0x001907B8) |
| 002C9B40 | 8 | getType__C27RaceDisplayInformationEvent | name only: GT3 setInformation / getInformation (0x001907C0 / 0x001907B8) |

---

## Delta review (map v2)

Three lines (`build/scratch/delta/hud.txt`), reviewed by the race-logic
survey. None is HUD code: all three sit in Lupo 2's bike-rider block
(`HumanModel` / `RaceBikerModel` posture code, L2 0x001AF1B8..0x001B00D8;
see `rendering.md`), placed next to race/display by neighbour.

| Candidate | Verdict |
|---|---|
| new? L2 0x001AF6D0..0x001AF6FC (11 insns) | belongs elsewhere (car model, bike rider): frees the buffer held in gp-0x5238 (0x0035D938, set by L2 0x001AF6A8 and used by the rider model loaders L2 0x001AED18 / 0x001AEED0) and clears the pointer; called by `race_end` (L2 0x00103150 = US 0x00102BB8). New code, no GT3 twin |
| new? L2 0x001AFF60..0x001AFFBC (23 insns) | belongs elsewhere (bike rider): a clamped linear curve, x x 0.375 + 0.125 held between 0.2 and 0.5, used by the rider posture function L2 0x001B00D8. New code |
| new? L2 0x001AFFC0..0x001B001C (23 insns) | belongs elsewhere (bike rider): the same curve shape, x x 0.0625 + 0.1875 held between 0.2 and 0.25, three calls from L2 0x001B00D8. New code |

No real HUD change; nothing to add to the summary.
