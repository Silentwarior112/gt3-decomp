# Port notes: hud (wave 1)

Area: `src/game/race/display` and its headers in `include/game/race/display`
(one new header, `T155_race_pod2p_display.h`), `include/game/enhanced/hud.h`.
Survey: `survey/hud.md` (its Delta review found nothing for the HUD).

All 18 files of the folder compile with `tools/mcc.py` (objects in
`build/hud/`). They were also compiled (egcs, `-Wall`) with every
`ENH_HUD_*` switch at 0: no errors, no warnings from this area's code.

## Switches (`include/game/enhanced/hud.h`)

| Switch | Entry | 1 (default) = Lupo 2 | 0 = GT3 |
|---|---|---|---|
| `ENH_HUD_DISPLAY_RPM` | C2 | tachometer needle on `RaceCarState::displayRpm` (whole rpm) | the engine rpm |
| `ENH_HUD_TELEMETRY_CYCLE` | C3 + C4 | live items = `RaceDisplay::showMask` (reset in `virtual_0`); the accel / brake monitor or steering display in it forces layout variant 1; the remote meter table carries both, its boost meter at y 430; the mirror record only for subType 0 | `RD_SHOW_LIVE`; GT3's tables |
| `ENH_HUD_AB_MONITOR` | C5 | Lupo 2's two framed columns, signs, trailing bars | `accel_gage` / `brake_gage` |
| `ENH_HUD_STEERING_DISPLAY` | C6 | the `replay_sign_gtforce` wheel, blur, ticks, red marker | the `steering` image and line |
| `ENH_HUD_TACHO_DIALS` | C8 | 14,000 rpm dial; full scale = the dial's | GT3's 8 dials, full scale = the car's |
| `ENH_HUD_SPEEDO_SCALE` | C9 | speedometer full scale = the dial's | the car's scale |
| `ENH_HUD_NEEDLE_LOOK` | C10 | needle 0xFF0000FF, copies at 1/6, angle clamped to start..end + 2 | 0xFF0000C0, 1/8, value clamped to 0..scale + 8 |
| `ENH_HUD_MINIMAP_POINTER` | C11 | one `car_pointer` sprite, yellow / green (the car shown) | `1pcar_dot` / `ecar_dot`, white |
| `ENH_HUD_TEXT_LOOK` | C12 (+ C26 labels) | shadowed `sw_20#1*160,160` title and mode label, grey 0x80C0C0C0, course name as stored, the label ("Live" / "Replay" / "Qualify") shown under it, "PP"; "@image" looked up once in `setText`; 0x60 characters; `clear()` hides the message | GT3's |
| `ENH_HUD_CAR_SIGN` | C13 | `replay_sign_car` left of the car name, the name 60 px right / 3 px up | GT3's car name |
| `ENH_HUD_LOGO_LOOK` | C14 | watermark additive at half grey; `gt_aspec_logo_mini` with the fader's `aspec`; logo and car name panel only with `miniLogo`; the car name also with `carNameDisplay` | GT3's |
| `ENH_HUD_DEMO_CAR_NAME` | C15 | a demo shows only `RD_SHOW_CAR_NAME` | the replay display options |
| `ENH_HUD_TIME_LIMIT_LOOK` | C16 | (16, 320) / (16, 336), red below 10 s, "GAME OVER" red and centred; 3 s low only when race.h's `ENH_RACE_TIME_UP_ENDS` is 1 (`RD_TIME_LIMIT_OFFSET`) | (8, 400), GT3's colours |
| `ENH_HUD_FREE_RUN_LIVE_MESSAGES` | C23 | free run start texts only live, `sw_16` | also in replays |
| `ENH_HUD_LICENSE_RESULT_TIMING` | C24 | pass wait 5, fail tail 47 frames | 6 / 48 |

## Entries

| Entry | Outcome |
|---|---|
| C1 `RaceCapacityMonitor` | **Ported (data, no switch).** New class (T157, end of the header; L2 0x001976D0 / 0x00197720 / setters 0x002C7B00..10), member `RaceDisplay::capacityMonitor` (appended). Fed in `virtual_2` from the car shown: level = `batteryCharge / batteryCapacity` (only when `hybrid` and capacity > 0), charge = `batteryFlow >= 0`, `hybrid`. Drawn with the meters after the gear (`virtual_3`), placed by both meter tables (onboard 615, 435, 30 x 70; remote 620, 370, 28 x 64; align 5 / 1) and by an `RD_ITEM_AB_MONITOR` record with subType 1 (GT3's only such record, 2649A8FF1B33D07E, is in no mode). Nothing is drawn for a car that is not a hybrid. Without the sign sprite the bar takes the sign's place. |
| C2 display rpm | **Ported, switch.** `virtual_2`: `(f32)(int)state->displayRpm`. Not in Lupo 2: while the field is exactly 0 with the engine turning (no physics spring ran), GT3's rpm is shown. |
| C3 show mask | **Ported, switch.** `virtual_0` resets `showMask`; `virtual_3` uses it live; `virtual_2` lays out variant 1 when `RD_SHOW_AB_MONITOR` or `RD_SHOW_STEERING` is in it. Inline helpers in T154: `setShowFlag(mask, on)`, `isShown(mask)`, `setMiniLogo`, `getMiniLogo`, `setCarNameDisplay`, `getLogoFader` (Lupo 2's linkonce accessors). The SELECT handler is race/core's (below). |
| C4 meter tables | **Ported, switch** (with C3). `g_002BC080[11]` / `g_002BC120[12]` (`RD_METERS_*_ENTRIES`, T154 header): the gauge in both (always), the accel / brake monitor (68, 435, 30 x 100, 4 / 1) and the steering display (320, 420, 75 x 68, 2 / 2) in the remote one, its boost meter y 0x1AE. `setLayout`: AB-monitor subType switch, mirror subType 0 (switch; GT3's racedetail.db has no mirror record), the car-name pair (C13). |
| C5 accel / brake monitor | **Ported, switch.** `RaceABMonitor` (+0x10: signs, last values): `update()` override (L2 0x00196570), `resetTrail()` (disableAll), `drawBar` (L2 0x00196828), Lupo 2's draw (L2 0x00196588). GT3's look when a sign is missing **or the layout gives no height** (GT3's racedetail.db has h = 0 in records 2807E20B58465DDE, A152B2EF53346885, EDD8B84BB3F7D6E5; Lupo 2's own records have 50 / 100 there). `virtual_3` calls `update()` after the draw. |
| C6 steering display | **Ported, switch.** `RaceSteeringDisplay` (+8: sign, last angle in degrees; sentinel FLT_MAX as GT3's needle, Lupo 2's 0x7FFFFFFF): `drawEnhanced` (L2 0x001969C8 / 0x00196C90, jitter table L2 0x003421C8, tick strip L2 0x003422D0), `drawGT3` (GT3's draw, moved), `update()`, `resetTrail()`. GT3's look when the sprite is missing or the layout gives no size (GT3's records 0A7BDA683932EB85, 6257BB50F221EA16, C3C5654220D0ECB5: w = h = 0). `setAngle` keeps GT3's fraction; the draw multiplies by -90 (Lupo 2 stores degrees). |
| C7 one ESP lamp | **Not ported** (PLAN: GT3's VSC and TCS lamps stay). |
| C8 tachometer dials | **Ported, switch.** T161 `setScale`: Lupo 2's 9 dials (`s_tachoScalesL2`), full scale = the dial's limit. Not in Lupo 2: the 14,000 dial is skipped when `<prefix>14` is not in the pack (then the 18,000 dial, full scale 18,000); past 18,000 the 18 dial with the car's scale (Lupo 2's 0x7FFFFFFF sentinel would freeze the needle). |
| C9 speedometer | **Ported, switch.** T162 `setupScale`: full scale = the dial's (the FLT_MAX entries keep the car's scale). |
| C10 needle | **Ported, switch.** T160 `draw_needle` (L2 0x00199DE8). |
| C11 mini map | **Ported, switch.** `RaceMiniMap::pointer` (+4), `init` looks it up, `drawMap` tints it (0x8000F8FF others, 0x8000FF90 the car shown), `drawCar(car, dot, color)` halves the alpha of map-only / ghost cars (white gives GT3's 0x40FFFFFF). Missing sprite: GT3's dots. |
| C12 HUD text | **Ported, switch.** `RaceMessageDisplay` 0x88 -> 0xB0 (text 0x60, `shadow`, `textImage` at the end); `drawEnhanced` (L2 0x001956F8: shadow 2 x scaleX right / down, alpha clamp((a - 0.3) / 0.7)); `setText` caches "@image" (L2 0x00195A58); `clear()` = Lupo 2's (L2 0x00195AF0); `setShadow` (Lupo 2's setFont 2nd argument: 1 only for title and mode label in `virtual_0`, 0 for the split battles' "PLAYER n WIN!"); default colour 0x80C0C0C0; `virtual_0`'s title / label block (L2 0x0018F1B0). **Finding:** GT3's `virtual_0` clears the mode label right after setting it (US 0x00191DB4, `setText("")`), so GT3 never shows "LIVE" / "REPLAY" / "QUALIFY" at the start; Lupo 2 shows them blinking under the course name for 4 s (2 s rolling). With the switch a demo no longer shows "DEMONSTRATION" (GT3: forever, if its layout places the label; C15 shows the car name only). |
| C13 car sign | **Ported, switch.** Member `RaceDisplay::carSign` (appended), `layoutCarSign()` (L2 0x001928A8) when the pack has `replay_sign_car` (else GT3's placement), shown every frame (`virtual_2`), coloured in `virtual_0`, drawn before the car name, cleared in `disableAll`. **Data note:** Lupo 2's car-name records are 20 high, aligned bottom; GT3's (6618CD1ACB5BFB9D: ax 4, ay 2, y 290, h 100; 8618CD1ACB5BFB9E: ax 5, ay 2, y 433, h 100) are centred and 100 high, so with GT3's racedetail.db the panel sits higher than in Lupo 2 (30 px for 8618CD1ACB5BFB9E, the replay's bottom right; 80 px for 6618CD1ACB5BFB9D). Lupo 2's values for those two records: ay 1, h 20 (and 6618CD1ACB5BFB9D y 340, mode 127). |
| C14 logo / mini-logo | **Ported, switch** (look and flags; the GT Concept front-end gate is not ported). `RaceDisplayFader::aspec` (+4, cleared by the constructor, kept by `reset`; no writer known in Lupo 2 either). |
| C15 demo HUD | **Ported, switch.** |
| C16 time limit | **Ported, switch** (layout, colours); the 3 s offset with `ENH_RACE_TIME_UP_ENDS` (race_logic C1: the race ends at the limit). The limit is drawn live only; in GT3 only the arcade demo has a limit (a demo, so nothing shows until a race mode sets one). |
| C17 Pod warnings | **Ported (data).** `showWarning`: `CARMSG_POD_STOP` (0x10, amber, "Stop and pull the handbrake"), `CARMSG_POD_RETURN` (0x11, red, "Return to the stop area"). Lupo 2's kinds 13..15 show nothing, as GT3's default. |
| C18 Pod sat down | **Ported (data).** `pollInformationEvents`: `RD_INFO_POD_SAT_DOWN` for the player's car: "The Pod sat down!" in blue, 2 s, `SE_COUNT_S`; the walk ends (Lupo 2). |
| C19 time step | **Ported.** `virtual_2(dt)`: fader, event board, title, mode label and car icons update only when dt > 0 (GT3's order kept; Lupo 2 moved them after the other elements: same result). |
| C20 SimpleExam | **Ported (no switch; the time trial behaves as GT3's).** In place in T141: `TimeTrialExaminationMessages` 0x1C -> 0x20 (`stopper`, `stopped`, `setStopper`), the hold (L2 0x00185C48), `start` clears `stopped`, the text centred on the buffer (`exam_draw_failure_text`, T146, Lupo 2 L2 0x00188870; F_0018ABC8 unchanged for the licences); `RaceTimeTrialDisplay::takeDisqualification` (event 0xA or `RACE_EVENT_CAR_DISQUALIFIED`, the player car only; 0xA is taken from the queue as GT3 takes it, 0x18 left for the result board); no jingle with a stopper; the 1800-frame cap off only with a stopper. Not ported (no visible effect): Lupo 2's `reset` setting `passed = 1`, its timers stopping at -1. |
| C21 `RacePODDisplay` | **Ported** (T141): constructor, `put_start_display` (L2 0x001B4118). For the race area to build. |
| C22 `RacePOD2PDisplay` | **Ported** (T155, new header `T155_race_pod2p_display.h`): `RaceSplitDisplayBase` (data-free: every slot through virtual `getDisplay(i)` / `getTargetDisplay()`), `RacePOD2PDisplay` (0x15244: two RacePODDisplays, target, ended): stoppers on in `virtual_0`, event 1 once both messages hold. GT3's `RaceSplitDisplay` keeps its own class and forwarding (Lupo 2 rebased it on the base: same behaviour; not done, so its vtable, typeinfo and members stay GT3's). |
| C23 free run | **Ported, switch.** |
| C24 licence timing | **Ported, switch.** |
| C25 renumbering | **Dependency, done in wave 0** (event 0x18 appended; this area uses `RACE_EVENT_CAR_DISQUALIFIED`). |
| C26 texts | **Ported** as `g_raceDisplayEnhTexts` (T154, English): 56 "Stop and pull the handbrake", 57 "The Pod sat down!", 58 "Return to the stop area", 60 "There is a stop area before the jump ramp", 61 "Stop completely there and pull the handbrake", 62 "Pull the handbrake and the Pod sits down"; labels "Qualify", "Demonstration", "Live", "Replay", "PP" (with `ENH_HUD_TEXT_LOOK`). 59 is GT3's `RACEDISPLAY_TIME_TRIAL_FAIL` (Lupo 2's 59 is its 8). T186's table is the menus area's and is replaced at run time by `text/realtime.dat`; these are not. |
| R1, R2 | **Not ported** (GT3's events2 draw and the logger's 4:3 exception stay). |

## Layout changes (sizes from `tools/sizeprobe.py`)

| Class | Before (wave 0) | Now |
|---|---|---|
| `RaceDisplayFader` | 0x10 | 0x14 |
| `RaceMessageDisplay` | 0x88 | 0xB0 |
| `RaceEventDisplay` | 0x47C | 0x5BC |
| `RaceABMonitor` | 0x28 | 0x38 |
| `RaceSteeringDisplay` | 0x20 | 0x28 |
| `RaceMiniMap` | 0x30 | 0x34 |
| `RaceCapacityMonitor` | new | 0x28 |
| `RaceDisplay` | 0xA480 | 0xA8E8 (members from `fader` on later than the comments; Lupo 2's is 0xA8D4) |
| `TimeTrialExaminationMessages` | 0x1C | 0x20 |
| `RaceTimeTrialDisplay` / `RacePODDisplay` | 0xA4B0 / new | 0xA91C |
| `RaceFreeRunDisplay` | 0xA480 | 0xA8E8 |
| `RaceLicenseDisplay` | 0xA6A0 | 0xAB08 |
| `RaceSplitDisplay` | 0x14908 | 0x151D8 |
| `RaceSplitDisplayBase` / `RacePOD2PDisplay` | new | 4 / 0x15244 |

No code reaches these classes by raw offset or a size literal (grep of
`src/`); `T179` reads `T164_race_minimap.h` for the type name only. The
meter tables are .bss built by T154's static initializer: 11 and 12 entries
(10 with the switch at 0).

## What to test in game

- **Logs first:** at each race start (the first RaceDisplay loads
  `race/US/display.imgs`) nine lines `hud: display.imgs <name>: found` or
  `... missing, <what is drawn instead>`.
- C2: any race; onboard tachometer (cockpit / bumper view) or remote (chase
  view): the needle should glide through shifts and limiter cuts. Needs the
  physics' display-rpm spring; without it GT3's needle.
- C3 / C4: a live single race (arcade): SELECT (race/core's port) cycles none
  -> accel / brake -> + steering -> none; with either on, the chase view's
  meter set shows whatever the camera, the boost meter 6 px higher.
- C5 / C6: a replay with the accel / brake and steering items on in the
  replay display options (chase camera), or live via SELECT; needs the
  sprites. Watch the trailing bar tops and the blurred wheel.
- C8: a car whose rev limit gives a 13,000 or 14,000 scale (rev limit
  11,500..12,499): the 14 dial (`rev_14` onboard, `revb_14` chase) with the
  needle reaching 14,000 at its end.
- C9: cockpit view, a car with a 160 km/h (100 mph) scale: the needle is at
  the dial's 180 (120) mark at 180 (120).
- C10: any needle: brighter red.
- C11: any race with opponents: yellow pointers, green for the car shown.
- C12: race start (live and replay): the course name in its own case with a
  shadow, "Live" / "Replay" / "Qualify" blinking under it.
- C13: a replay with the car name item on: the sign left of the name (bottom
  right; see the data note above).
- C14: replay with every item off or a demo: the logo additive / dimmer; the
  race area's replay toggle hides logo and car name.
- C15: the arcade attract demo: only the car name.
- C16: needs a live timed race (race area).
- C17 / C18 / C20 / C21 / C22: the Pod races only (race area's RacePOD /
  RacePOD2P, the Pod rule's events). Log lines `hud: car N: the Pod sat
  down`, `hud: car N disqualified (the Pod rule)`, `hud: the Pod's
  stop-area rules on the event board`, `hud: both Pods disqualified: the race
  ends`.
- C20 regression: a time trial disqualification (course out) must look and
  time as before (message centred, jingle, 4 s, race end).
- C23: a free run replay: no start texts. C24: licence pass / fail (one frame).
- C1: a hybrid car (the physics' Dualnote data; GT3 has none): the gauge right
  of the meters; log `hud: car N is a hybrid: battery gauge on`.

## Assets

`race/<country>/display.imgs` (US: `race/US/display.imgs`), merged into
GT3's pack (an IMGS pack; its entries must stay sorted by name, signed-char
order, as `image_pack_find` binary-searches them). Do not replace GT3's pack
with Lupo 2's: Lupo 2's lacks `accel_gage`, `brake_gage`, `steering`,
`1pcar_dot`, `ecar_dot`, `win`, `lose`.

| Sprite | Used by | Without it |
|---|---|---|
| `meter_sign_dualnote` | C1 gauge sign (drawn tinted blue) | the gauge without its sign |
| `replay_sign_accel`, `replay_sign_brake` | C5 | GT3's monitor (both needed) |
| `replay_sign_gtforce` | C6 | GT3's steering display |
| `replay_sign_car` | C13 | GT3's car name |
| `car_pointer` | C11 | GT3's dots |
| `rev_14` (onboard), `revb_14` (chase) | C8 | the 18,000 dial |
| `gt_aspec_logo_mini` | C14 (only when `aspec` is set: nothing sets it) | `gt_logo_mini` |

Fonts `sw_20#1` (C12) and `sw_12` / `sw_20` are GT3's. Optional data: the
racedetail.db records named above (car name, accel / brake monitor,
steering) to get Lupo 2's placements with GT3's modes.

## For other areas

- **Integrator / race / race modes (sizes):** the probe's differing
  entries are all in others' headers (values with every area's changes at
  18:20; the race's own growth may still change them): `RaceBasicWithRaceDisplay`
  (T132) 0x171C0, `RaceSinglePlayer` 0x17340, `RaceArcade` /
  `RaceArcadeSingle` 0x34C00, `RaceArcadeDemo` 0x35400, `RaceFreePractice`
  0x342C0, `RaceSplitBattle` 0x3F740, `RaceLinkBattle` 0x35680,
  `RaceSolitaire` 0x22300, `RaceTimeTrial` 0x49BC0, `RaceFreeRun` 0x3BA00,
  `RaceLicense` 0x3C580, `RaceMachineTest` 0x49BC0, `RaceChampionship`
  0x34C80.  The HUD's share of the growth: 0x468 per RaceDisplay (0x8D0 for
  a split display); race objects are stack objects (INTERFACES section 5).
- **race (core), C3:** SELECT (live only, `CONTROL_SELECT`, Lupo 2 L2
  0x0017F9A0 from `RaceBasic::virtual_30` / `RaceMachineTest::virtual_30`):
  cycle through `showMask` (`setShowFlag(RD_SHOW_AB_MONITOR / RD_SHOW_STEERING,
  on)`, table {1, 3, 0, 0}), then `virtual_2(0.0f)` on the display (the
  re-layout without animation). Only on a `RaceDisplay` (not a split one).
- **race (core), C14:** the replay mini-logo toggle (L2 0x0017F548..
  0x0017F62C) writes `miniLogo` (`setMiniLogo`); nothing needs to write
  `carNameDisplay` or the fader's `aspec`.
- **race (modes), C21 / C22:** `RacePOD` builds a `RacePODDisplay` (T141
  header) as `RaceTimeTrial` builds its display (input via `setInput`);
  `RacePOD2P` a `RacePOD2PDisplay` (`T155_race_pod2p_display.h`), setting
  its target display (`setTargetDisplay`) and player cars as `RaceSplitBattle`
  does. The Pod rule must post `RACE_EVENT_CAR_DISQUALIFIED` with the car as
  arg, `RD_INFO_POD_SAT_DOWN` information events with the car, and set
  `msgKind` `CARMSG_POD_STOP` / `_RETURN` (physics: the duration table must
  have 18 entries first, INTERFACES section 3). Each Pod display's
  `playerCar` must be its player's car (`setPlayerCarNumber`).
- **race, C16:** with a live timed race the limit shows 3 s low only while
  `ENH_RACE_TIME_UP_ENDS` is 1.
- **physics, C1 / C2:** `displayRpm` (the spring) and the battery fields
  (`hybrid`, `batteryCharge`, `batteryCapacity`, `batteryFlow`) as
  INTERFACES section 2 describes; the HUD only reads them.
- **menus (T186), optional:** append Lupo 2's RACEDISPLAY 56..62 to GT3's
  table (and realtime keys) if the texts should follow the language file;
  until then the HUD uses `g_raceDisplayEnhTexts`.
- **Integrator:** new code lives in existing objects (T141, T146, T154,
  T155, T157, T160..T164); one new header. `RaceMessageDisplay::clear()`
  (inline, the "" anchors of many objects' .sdata) changed with
  `ENH_HUD_TEXT_LOOK`; its "" stays.
