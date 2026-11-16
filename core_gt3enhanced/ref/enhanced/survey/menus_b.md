# Survey: menus part B, save data and databases (Lupo 2 against GT3 US)

Area: `src/game/menu/gtmode/`, `src/game/menu/car_settings/`,
`src/game/menu/memorycard/`, `src/game/menu/records/`, `src/game/save/`,
`src/game/data/` and `src/game/data/messages/`.
Format: `ref/ENHANCED.md`. Addresses: "L2" = Lupo 2 (PAPX-905.09), "US" = GT3 US.
Scratch (scripts, dumps): `build/scratch/survey_menus_b/`.

## Summary

Changes found (each detailed below; "port" / "do not port" is the verdict):

1. **CVT lock-out in the car settings** (known start point, now complete):
   `isCVT()` = GEAR row `+0x11` (forward gears) == 2 (L2 `0x002433B0`). The
   nine gear-ratio texts (ratios 0..7 and the final drive) show `----` for a CVT,
   and a new setting check refuses the transmission page with the message
   key `your_car_is_CVT`. The settings screen also has a new "gear_max_speed"
   text that shows `----` for a CVT. 11 of the 14 call sites are in my area.
   **Port.**
2. **New "gear_max_speed" text in the transmission settings**: the top speed
   of the current gearing (from `gear_chart_of_spec`), in the player's unit,
   2 decimals. **Port** (small; needs an item in the settings page layout).
3. **Concept setting mode** (`SettingControlConcept`, new): the settings
   screen opens on a "normal" page or a "custom" page, depending on whether
   the brake controller is fitted. Two actions fit a fixed set of parts
   ("normal": none / stock suspension / stock gearbox / normal tyres;
   "custom": controller / sports suspension / full-custom gearbox). This is
   GT Concept's stand-in for the parts the player owns. **Do not port**
   (GT3 keeps its parts-based settings; arcade use is optional).
4. **Tyre compound slider** (part of 3): three positions for front and rear
   tyres, shown as Soft / Normal / Hard, or Dirt on a dirt race. New
   SETTING messages `TIRE_*`. **Do not port** (as 3; optional for arcade).
5. **Dirt-tyre check extended to arcade races**: in an arcade event
   (event code starting with `A`) on a dirt course (course kind 2), the
   tyre change is refused with `dirt_tire_only`. GT3 does this only for
   GT-mode events. **Port** (small).
6. **Car spec sheet from new catalogue fields**: drive type, maximum power,
   length, height, width and weight now come from new database fields,
   with "-" when a field is 0 or its flag is not set (concept cars). The
   new fields are DRIVETRAIN `+0x22`, ENGINE `+0x58`, CHASSIS
   `+0x20/+0x22/+0x24` and RACINGMODIFY `+0x42`. **Do not port** without a
   widened database (GT3's rows lack them or hold other bytes there).
7. **GT-mode screens and data removed** (garage, dealer, wheel shop, tune
   shop, oil change, prize screens, GT-mode trophy flow, memory-card warning,
   GT-mode ranking-code reasons). **Removed in Lupo 2**: not ported.
8. **Concept "present" / car-get / licence flows** (new classes
   `CarGetControlBase`, `CarGetConceptControl`, `PremiumGetControl`,
   `LicenseGetControlBase`, `NoticePresentControl`, `CarsConceptItem`,
   message items; prize checks and `PrizeCarList` reworked;
   `RecordsMenuArgLicenseConcept`). **Do not port** (they replace GT
   mode's prize cars and licences).
9. **Save data layout changed throughout** (Concept save "GTCGameData"): a
   Concept licence block is inserted at `GameStatus+0x23510`, which shifts
   every later record by `0xD48`. There are new "last presents" and a
   `+0x36000` reference buffer, and `MenuStatus` grew. **Do not port: GT3's
   layout must stay** (save compatibility). None of the ports here needs a
   new save field.
10. **Option defaults**: the screen is set to wide from the console's
    aspect setting; race and replay BGM off, manual gears, ASM off and
    steering assist on by default. **Port the wide default (optional); do
    not port the others** (they are Concept's preferences).
11. **Default pedal calibration for the USB wheels** (GT Force and the
    C294): Lupo 2 fills in measured accelerator and brake ranges and marks
    them valid, where GT3 leaves them unset. **Port** (data only, no layout
    change).
12. **Default music play list**: only Daiki Kasho's tracks. **Do not port.**
13. **Memory card**: GTC file kinds, Concept icons and product code, and a
    "transfer money" feature that writes into a GT3 (Japan, `BISCPS-15009`)
    game save, with an "already transferred" dialog. **Do not port**
    (Concept is the sender, GT3 is the receiver).
14. **Race result board**: cars hit by race event `0x0B` (disqualified,
    with the Pod race) go to the bottom of the board. In a laps-only race
    the leader shows laps and cars on the same lap show the time gap.
    **Port with the race-logic port** (the gap display can be ported alone).
15. **Link-battle result**: the end of the result is triggered only once
    all but one car have finished, not at the first goal. **Port**
    (small, low priority).
16. **Database**: tables numbered 30 and up were renumbered (CAR 30→35 and
    others, already in `docs/paramdb_format.md`). The maker table grew from
    46 to 53 names and the brand table from 52 to 60. The GCF format is
    unchanged (only the gcf file differs). **Do not port** (data coupling).
    Remap only if Lupo 2 cars are transplanted.
17. **Message tables**: SETTING (+`UNIT_LAPS`, `TIRE_*`), MEMORYCARDTEXT
    (+7 GTC file kinds), DATABASERACECATEGORY (+Concept licence, single
    race). **Port `TIRE_*` only with 4; do not port the rest.**
18. **Dualnote menu power bonus**: not in my area. It is applied in
    `ShowRoomCar::SetupShowRoomCar` (car/model); the menu power texts read it
    through `ShowRoomCar::power` with no change of their own (see "For other
    areas").

Candidates examined: **883 pairs** with difference ≥ 0.25 in the seven
folders (car_settings 74, gtmode 159, memorycard 267, records 106, save 140,
data + messages 137), all 60 non-plausible pairs (size over 2x), and **742
unpaired Lupo 2 functions** placed in the area (car_settings 230,
gtmode 182, memorycard 64, records 34, save 81, data + messages 151).
Uncertain points are marked "(?)" in the entries. The map v2 delta review
(54 more pairs and 67 newly found functions, at the end) found no new change
to port, and 12 more wrong pairs.

### A caution on the map in this area

The map is unreliable in several places, so pairs in this area need care:
- In `T081_setting_hooks`, the `order` pairs from L2 `0x00146228` to
  `0x00147240` are shifted by two. For example, L2 `0x00146228` is
  `text_spring_rate_f`, not `text_spring_camber_r`, and L2
  `0x00147188`/`0x00147240` are `text_gear_ratio_0`/`_1`, not
  `text_spring_rebound_f`/`_r`. The true twins are in the car-settings
  noise entry below.
- The save records (`T415`..`T434`) are paired loosely, because Lupo 2
  inlines most record accessors.
- Lupo 2 emits some inline accessors twice (a `.gnu.linkonce` copy plus an
  out-of-line copy). An example is `GameStatus::getRunningCar` = L2
  `0x002D1B18` and `0x00227988`. That second copy turns up as "unpaired
  callee" diffs everywhere it is called (74 callers).
- `build/scratch/survey_menus_b/weak.json` holds the weak twins (same mapped
  call set, similar size) that I used to resolve these.

## Entries

### 1. Car settings: the CVT lock-out (UI "CVT" switch)

| Field | |
|---|---|
| What | A car whose gearbox has 2 forward gears is treated as a CVT in the settings: every gear-ratio text (gears 0..7 and final) reads `----`, the new top-speed text reads `----`, and the button that opens the transmission settings is refused with the message `your_car_is_CVT`. |
| Where | Predicate L2 `0x002433B0` `isCVT(CarEquipments*)` = `getGearCount() == 2` (calls L2 `0x00243388` = US `CarEquipments::getGearCount`), new; it belongs with `src/game/data/T482_car_equipments.cpp`. Ratio texts L2 `0x00147188`, `0x00147240`, `0x001472F8`, `0x001473B8`, `0x00147478`, `0x00147538`, `0x001475F8`, `0x001476B8`, `0x00147778` = US `text_gear_ratio_0..7`, `text_gear_ratio_final` (`0x0015E2D8`..`0x0015E798`, `T081_setting_hooks.cpp`). New check L2 `0x0015DB88` (`check_cvt`, wrongly paired with `__tf Cell<GarageCar>`): GT3 `T096_setting_checks.cpp`. New text L2 `0x0014A8F8` (entry 2). |
| Kind | changed logic + new function + new data (message key) |
| Evidence | Each ratio text keeps GT3's test `gears (row+0x11) < N → "----"` and adds `jal 0x002433B0; bnel v0 → "----"` (e.g. L2 `0x00147344`). `check_cvt`: `getGameStatus()->menu.raceCar[0]`, `isCVT(car+0x10)` → returns `0x0033F140` "your_car_is_CVT" else NULL; it is registered in L2's T096 table beside the new Concept buttons `set/adjast_gear` and `set/adjast_tir` (`set/adjast_gear` → `check_cvt` (?) from the order of the stores). The message key is new (L2-only string). The other 3 call sites are outside my area (see "For other areas"). |
| Port | Add `CarEquipments::isCVT()` (GEAR `+0x11` == 2, no new field). Add the `isCVT()` test to the 9 `text_gear_ratio_*` hooks. Add a `check_cvt` T096 entry under the name of GT3's transmission-page button (GT3's settings layout has no `set/adjast_gear`: the entry must use GT3's own button name). Asset: the `your_car_is_CVT` text in the settings page messages (menu `.mbl`). Database: GT3 has no 2-speed gearboxes, so a CVT car needs a GEAR row with `+0x11` = 2 (it pairs with the gearbox type 1 of `docs/gearbox_dualnote.md`). |
| Verdict | **port** |

### 2. Car settings: top speed of the gearing ("gear_max_speed")

| Field | |
|---|---|
| What | A new text in the transmission settings shows the top speed the current gear ratios allow (e.g. `245.37km/h`), or `----` for a CVT. |
| Where | L2 `0x0014A8F8` (text hook, registered as `gear_max_speed`) and L2 `0x0014A8B8` (helper: `getSpec`, `gear_chart_of_spec`, returns `chart.speedHigh[chart.count]`), new; GT3 `T081_setting_hooks.cpp`. |
| Kind | new function |
| Evidence | Calls `isCVT`, `UnitSettings::convSpeed`, `UnitSettings::speedName`, `fmt_sprintf("%.2t%s", speed*100)`; the name `gear_max_speed` is an L2-only string in T081's hook table. |
| Port | Two small functions over GT3's `gear_chart_of_spec` (US `0x001E2538`). Asset: a text item named `gear_max_speed` in the settings page layout (`.mbl`), so a layout change. |
| Verdict | **port** (optional: it needs the layout item) |

### 3. Car settings: Concept setting mode (normal / custom)

| Field | |
|---|---|
| What | GT Concept has no parts shop. Its settings screen opens on a "normal" page or a "custom" page, chosen by whether the car has the brake controller fitted. Actions switch the whole car between a "normal" part set and a "custom" part set, and restore all settings. |
| Where | New class `SettingControlConcept` (vtable L2 `0x00328320`, dtor `0x002BE758`, `mainLoop` `0x00145C48`). `SettingControl::mainLoop` (L2 `0x00145B68` = US `0x0015CC28`) became virtual (vtable 2 → 3 slots). `setting_menu` (L2 `0x00145DA8` = US `0x0015CD60`) always builds the Concept class. Actions L2 `0x00150620` (`retrieve_all`), `0x00150750` (`set_concept_custom`), `0x001507A8` (`set_concept_normal`). Part-set helpers L2 `0x00245D88`, `0x00245DE8` and `0x00240CB8` (reset all settings), and `0x00245E68` (`brake controller == 2`). New class `SettingButtonList` (vtable L2 `0x00329890`; item builder `0x0015D888` matching the `normal`/`custom` buttons; creators `0x001503E0`, `0x00150470`). New strings `menu_map_custom`, `menu_map_normal`, `root_no_setting_no_save`. |
| Kind | new function or class |
| Evidence | `0x00145C48`: `setPage(isBrakeControllerNone ? "root" : "custom")`. `setting_menu` stores vtable `0x00328320` into the object right after the `SettingControl` constructor (L2 `0x00145E60`). Part sets at L2 `0x002E0040` = {2, 0x32, 0x13, 0x38, 0x43} (no brake controller, stock suspension, stock gearbox, normal front and rear tyres) and `0x002E0058` = {3, 0x33, 0x16, 0x38, 0x43} (brake controller, sports suspension, full-custom gearbox, normal tyres; fitted only where `CarInformation::partExists`). |
| Port | It would replace GT3's settings model (parts the player owns, T084 `setting_owns_*`, T096 `check_set_parts`). New page layouts (`.mbl`). It needs no save field (the mode is the fitted brake controller). |
| Verdict | **do not port** (Concept's stand-in for the parts shop; GT3's parts-based settings stay). Optional later: arcade mode only. |

### 4. Car settings: tyre compound slider

| Field | |
|---|---|
| What | Front and rear tyres are each chosen on a three-position slider instead of the parts list. The slider shows ソフト / ノーマル / ハード (Soft / Normal / Hard), or ダート (Dirt) when the race needs dirt tyres. |
| Where | Sliders L2 `0x0014DE30`/`0x0014DE90` (front), `0x0014DF50`/`0x0014DFB0` (rear), registered as `slider_tire_f`/`slider_tire_r`. Texts L2 `0x00148B58`/`0x00148C70` (`parts_tir_f_concept`/`parts_tir_r_concept`). Helpers L2 `0x00245E90` (min 0), `0x00245E98` (max 2), `0x00245EA0`/`0x00245ED0` (part → level), `0x00245F00`/`0x00245F60` (level → `fitPart(0x38+i)` / `fitPart(0x43+i)`). All new, in GT3 `T081`/`T082`/`T482`. `SettingSerialize::applySettings` (L2 `0x002477A8` = US `0x00239A78`) also refits the tyres when it loads a setting. |
| Kind | new function + new data (table L2 `0x0034B520`) |
| Evidence | Level table L2 `0x0034B520`, indexed by tyre kind: kind 0 (normal) → 1, kind 3 (racing hard) → 2, kind 7 (racing soft) → 0, the rest −1. Texts from the SETTING message table (entry 17). |
| Port | As 3 (the parts list goes). |
| Verdict | **do not port** (part of the Concept setting mode; optional for arcade together with 3) |

### 5. Car settings: dirt tyres forced in arcade dirt races

| Field | |
|---|---|
| What | GT3 refuses a tyre change ("dirt_tire_only") only when the current event is a GT-mode dirt event. Lupo 2 also refuses it in an arcade event (event code `A…`) whose course is a dirt course. |
| Where | New L2 `0x00152A80` `isDirtTireOnly()`. Called by `check_change_tire` (L2 `0x0015DB18` = US `0x00127B30`, `T096_setting_checks.cpp`) and by the Concept tyre texts. |
| Kind | changed logic |
| Evidence | `0x00152A80`: `event = menu+0x1504`. If `event[0] == 0` → 0. If `event[0] == 'A'` → `RaceDataBase::getCourseKind(nameToCode(menu+0x1544 course)) == 2`. Else → `race_dirt_tire_only(event)`. GT3: `strcmp(event, "")`, then `race_dirt_tire_only(event)` only. |
| Port | Small. GT3's `MenuStatus` has no course name next to the event, but it has the arcade course (`arcadeCourse`). The arcade branch should read GT3's arcade course and test its course kind (RaceDataBase course table, GT3 index 34). No save change. |
| Verdict | **port** |

### 6. Car spec sheet: catalogue fields with "-" for unknown values

| Field | |
|---|---|
| What | The spec-sheet texts (garage, dealer and arcade car spec) show a car's catalogue figures from new database fields, and "-" / "- mm" / "- kg" when a figure is missing (concept and prototype cars). |
| Where | L2 hooks in GT3 `T036_garage.cpp`, all wrongly paired or unpaired by the map: `0x00121C00` = `text_spec_drive_type`, `0x00121D00` = `text_spec_height`, `0x00121D70` = `text_spec_length`, `0x00121DE0` = `text_spec_max_power`, `0x00121ED0` = `text_spec_max_power_real` (mapped to `text_spec_max_power`), `0x00122398` = `text_spec_weight`, `0x001224A0` = `text_spec_width`. New accessors (GT3 `T482`): L2 `0x002445F8` (DRIVETRAIN `+0x22` == 1), `0x00244620` (ENGINE `+0x58` == 1), `0x00244578` (CHASSIS `+0x20` u16), `0x00244598` (CHASSIS `+0x22` u16), `0x002445B8` (CHASSIS `+0x24` u16), `0x002445D8` (RACINGMODIFY `+0x42` u16). |
| Kind | changed logic + new data (database fields) |
| Evidence | Drive type: `"-"` unless DRIVETRAIN `+0x22` == 1 (this is the "badge" byte of `docs/paramdb_format.md` §5). Max power (catalogue and real): `"-"` unless ENGINE `+0x58` == 1. Length, height and width read the new u16 fields instead of GT3's `getLength`/`getHeight`/`getWidth` (CHASSIS `+0x14/+0x16`, RACINGMODIFY `+0x36`), with `"- mm"` when 0. Weight reads CHASSIS `+0x24` instead of the computed `menu_car_spec()->car.spec.weight`, with `"- kg"` when 0. GT3's `EngineRecord` ends at `+0x57`. Its `ChassisRecord` is known only up to `+0x1E` (nothing in GT3 reads `+0x20..+0x25`). GT3's DRIVETRAIN `+0x22` and RACINGMODIFY `+0x42` lie in bytes our headers mark unknown, and their GT3 values are not checked. |
| Port | Database: the new ENGINE and CHASSIS fields need wider rows; DRIVETRAIN `+0x22` would have to be 1 in every GT3 row, or every drive type reads "-". Code: 7 hooks and 6 accessors. |
| Verdict | **do not port** (data-driven; GT3's cars all have catalogue data and its rows lack the fields). Revisit if Lupo 2 concept cars are transplanted (then guard each field with a fallback to GT3's computed value). |

### 7. GT-mode screens, data and texts removed in Lupo 2

| Field | |
|---|---|
| What | GT mode's screens and data are gone or stubbed: the garage, dealer, used cars, wheel shop, tune shop and oil-change flows; the GT-mode top menu; the not-car-get screen; the memory-card space warning; and the ranking-code reasons for GT mode and licences. |
| Where | GT3-only classes (`classes`): `GtmodeControl`, `GtmodeRender`, `GarageButton(List)`, `WheelButton(List)`, `WheelShopButton(List)`, `DealerButtonList`, `CarGoodsButtonList`, `NotCarGetControl`, `MemorycardWarningControl`, `Cell/List<GarageWheel>`, `List<GarageCar>`. GT3-only strings: `menu/gt_mode.mbl`, `hm/*`, `ts/*`, `00cmn/*`, `lc/no_license_*` and the ranking reasons `SOLO_RACE_CLEAR` … `ARCADE_COMPULSOLY_COMPLETE` (`T431_ranking_code`). 304 GT3 functions of `menu/gtmode` have no Lupo 2 twin (`build/scratch/survey_menus_b/usonly_gtmode.txt`). Some code stays in Lupo 2, unused or reused: most of T036's hooks (`text_price_*`, `make_regulation_*`) are still there. |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | Class and string diffs. `MemorycardControl::begin` (L2 `0x0013AEF8`, mapped to `car_get_event_name` by mistake; true twin US `0x00149A00`) keeps only two ranking reasons. The L2 range of the GT3 warning control (`0x001452E8`..) holds demo-file functions (`demo/%s%s`, `_uk`, `L0S0001`). |
| Port | — |
| Verdict | **do not port** (GT3's versions stay) |

### 8. Concept presents, car-get and licence-get flows

| Field | |
|---|---|
| What | GT Concept gives cars as "presents" for achievements, has its own "concept licences", and shows premium, congratulation and next-message items and a "notice present" screen. GT mode's prize-car logic is reworked to serve this. |
| Where | New classes `CarGetControlBase` (dtor L2 `0x002B4D28`), `CarGetConceptControl` (`0x002C0A88`..`0x002C0E10`), `PremiumGetControl` (`0x002B49E0`..), `LicenseGetControlBase` (`0x002B46A0`..), `NoticePresentControl` (`menu/notice_present.mbl`), `SimplePageControl`, `CarsConceptItem` (`0x002C24D0`..), `NextMessageItem`, `PremiumMessageItem`, `CongratulationMessageItem`, `BlinkItem`. Vtable growth: `CarGetControl` 2 → 5, `LicenseGetControl` 2 → 5 slots. Reworked: `car_get_prizes` (L2 `0x001373F0` = US `0x0013EF40`, 314 vs 370 insns), `car_get_load_car` (no random colour), `CarsItem::virtual_1/2` (L2 `0x00171408`, `0x00171710`; new draw helpers `0x00170940`..`0x001737C0`), `prize_check_*` (L2 `0x00221478`..`0x00221EB0`), `PrizeCarList` (L2 `0x00221F88`), `PrizeSetFlags` (L2 `0x00223F70`, `0x00224080`), `ProductConfig::prizeCarCode` (L2 `0x0022C830`, 110 vs 30 insns, `%s#%03d` car variants), new `ProductConfig` helpers L2 `0x0022BD68`, `0x0022BE98`, `0x0022CAC0`. Records: `RecordsMenuArgLicenseConcept` (L2 `0x002BF150`..), `RecordsButtonList` built from 7 plain items (L2 `0x0015C918`) and its new helpers L2 `0x0015B960`..`0x0015D1C8`. |
| Kind | new function or class (Concept-only) |
| Evidence | L2-only strings `concept_car_get`, `premium_get`, `prem_mess_billion`, `congratulation`, `go_present`, `notice_present`, `license_gold/silver/bronze`, `TEXT_CATEGORY_CONCEPT_LICENSE`; new save blocks (entry 9). |
| Port | — |
| Verdict | **do not port** (they replace GT mode's prize cars and licences, which GT3 keeps) |

### 9. Save data: the Concept GameStatus layout

| Field | |
|---|---|
| What | Lupo 2's game save ("GTCGameData") is laid out differently from GT3's. A GT3 save cannot be read by Lupo 2's code, and the reverse is also true. |
| Where | `GameStatus` (GT3 `T429_game_status.cpp`) and its records, `MenuStatus` (`T427`). Lupo 2's named accessors (linkonce, L2 `0x002D1BC8`..`0x002D1C98`) give its layout: licences `+0x1F550` (as GT3), **Concept licences `+0x23510` (new)**, race status `+0x24258` (GT3 events `+0x23510`), course status `+0x24850` (GT3 `+0x23B08`), arcade `+0x24900` (GT3 `+0x23BB8`), compulsory (time trial) `+0x25440` (GT3 `+0x246F8`), free run `+0x26EA8` (GT3 `+0x26160`), machine test `+0x2F2A8` (GT3 `+0x2E560`), presents `+0x2FBA8` (GT3 prize sets `+0x2EE60`), **last presents `+0x2FBF4` (new)**, championship `+0x2FC20` (GT3 `+0x2EF28`), options `+0x30C30` (GT3 `+0x2FF38`), work = MenuStatus `+0x314C0` (GT3 `+0x307C8`), **Concept reference buffer `+0x36000` (new)**. Header (0x130 bytes) and `cars[200]` / `carList` (`+0x1B6B8`) are unchanged; `GarageCar` save/load are unchanged (inline copy of the same size). `GameStatus::save` (L2 `0x00228D78`) writes the header, the cars and 5 record blocks; it does not write GT3's wheels, licences, best times, free run, prize sets or championship blocks. `MenuStatus::reset` (L2 `0x00226528`): new fields (course name `+0x84`/`+0xC4`, `+0x4A94`, `+0x4B38`, an embedded object at `+0x4AD4` set up by L2 `0x0021DF28`), and `runMode` defaults to 9 (GT3 8). |
| Kind | changed data layout (Concept-only) |
| Evidence | Offsets above; `GameStatus::save/load/dataSize/init` (L2 `0x00228D78`, `0x00228A68`, `0x00228928`, `0x002276A0`) call a different set of record functions; MemorycardControl reads GT3's Japanese save through its own descriptor (entry 13). |
| Port | — Every port in this report works without a new save field: entry 3's mode is a fitted part, and entries 1, 2, 5, 11, 14 and 15 keep no state. |
| Verdict | **do not port: GT3's save layout must stay** (save compatibility). |

### 10. Option defaults (a new save / "reset options")

| Field | |
|---|---|
| What | New default options: the wide screen follows the console's 16:9 setting; race BGM off; replay BGM off; manual gearbox; ASM (stability control) off; steering assist on for both players. |
| Where | `GameOptions::setDefaults` L2 `0x00226950` = US `0x00218318` (`T428_game_options.cpp`); the aspect query L2 `0x00284CF8` (semaphore-wrapped call to L2 `0x00292040`, the system configuration's aspect (?)). |
| Kind | changed logic (default values) |
| Evidence | L2: `wide = (aspect() == 2)`, `raceBgm(+0xC) = 0`, `gearAuto(+0x24) = 0`, `replayBgm(+0x54) = 0`, `assistAsm(+0x5C) = 0`, `steeringAssist1p(+0x3C) = 1`, `steeringAssist2p(+0x44) = 1`; GT3: `wide = 0`, those `1, 1, 1, 1, 0, 0`. Other defaults unchanged. |
| Port | Wide default: one call to the SDK's system-configuration aspect (libscf; GT3 links libscf). The rest: values only. |
| Verdict | **port the wide default** (optional); **do not port** the others (Concept's choices; GT3's defaults stay) |

### 11. Default calibration of the USB wheels' pedals

| Field | |
|---|---|
| What | When the controller options are reset, Lupo 2 fills in measured accelerator and brake ranges for the two USB wheels and marks the calibration valid. The wheels then work without the calibration screen. GT3 leaves the ranges at 0..0x100 / 0..0x100 and invalid. |
| Where | `ControllerConfig::resetCalibration` L2 `0x00225DC0` = US `0x00217880` (`T426_controller_config.cpp`); valid flag set by L2 `0x002261A0`. |
| Kind | changed logic (default data) |
| Evidence | Wheel 2 (GT Force): `+0x10/+0x14` = 0x29/0xD5 and `+0x18/+0x1C` = 0x2B/0xDA, then `setValid(port, 2, 1)`. Wheel 3 (C294): 0x25/0xB4 and 0x48/0xDF, then `setValid(port, 3, 1)`. Steering range (`+0x8` = 0x400, `+0xC` = 0x200) and the layout `calib[wheel-2][port]` (0x40 / 0x20 strides) are as in GT3. |
| Port | Values only; GT3's `WheelCalibration` already has the fields (check what `setValid` writes against GT3's `valid` member before porting). |
| Verdict | **port** |

### 12. Default music play list

| Field | |
|---|---|
| What | The race play list built for a new save holds only the tracks whose artist is "daiki kasho". |
| Where | `MusicPlayList::init` L2 `0x00229100` = US `0x0021ABB0` (`T430_music_play_list.cpp`). |
| Kind | changed logic |
| Evidence | `strcmp(track+0xC, "daiki kasho")` before each add (L2-only string `0x00349F18`). |
| Port | — |
| Verdict | **do not port** (Concept's music choice) |

### 13. Memory card: Concept files and the money transfer to GT3

| Field | |
|---|---|
| What | The memory card screens handle GT Concept's own file kinds (GTC game, replay, licence replay, best lap, setting; Lupo Cup V1/V2 replays) with new icons and product code. A new game-menu item "transfer_money" reads a GT3 (Japan) game save and puts money into it, once. A dialog says when this was already done. |
| Where | `MemorycardControl::gameMenu` L2 `0x0013BC28` = US `0x0014A3C0` (adds `transfer_money`, 6 items instead of 5); `MemorycardAlreadyTransferedDialog` (new class, L2 `0x002BDE08`; text callback `0x00143470` "already"); GT3-save descriptor at L2 `0x00349F50`..`0x00349F9C` (offsets 0x10, 0x30, 0x34, 0x130, 0x204, … and `BISCPS-15009GAMEDATA`); `mcGetNamePrefix` (`PAPX-90509`); icons `menu/icon/ICON_GTC_*.ico`; `MemorycardPS2File::writeIconSys/writeIcons`, `MemorycardGT3File::saveFiles` (uses `mc_set_file_info` where GT3 renames), the `*File::save` title builders and `IconView::virtual_2` (Concept look). Vtable growth: `MemorycardGT3File` family 21 → 22 slots (one pure virtual added), `MemorycardFileManager(Base)` 31 → 33 slots (two added at the end). |
| Kind | new function or class (Concept-only) + backend |
| Evidence | L2-only strings `transfer_money`, `already_window`, `TRANSFER`, `BISCPS-15009GAMEDATA`, `GTC*Data`, `file_operation_lupo`. |
| Port | — |
| Verdict | **do not port** (GT Concept sends the money to GT3; GT3, the receiver, needs no code) |

### 14. Race result board: disqualified cars; gaps in laps-only races

| Field | |
|---|---|
| What | (a) Cars named by race event `0x0B` are placed at the bottom of the result board in turn, marked as not running. Their own rows get no time. (b) In a laps-only result (GT3 shows only each car's laps), the leader shows laps and every car on the leader's lap shows its time gap. |
| Where | `ResultArcade::update` L2 `0x00173FC8` = US `0x00174E58` (83 vs 40 insns) and `ResultArcade::setRowResult` L2 `0x001744B8` = US `0x00175298` (`T117_result_arcade.cpp`). New `ResultBoard` helpers L2 `0x001B83A0` (row → `ResultRankBar::setRunning`) and `0x001B8218` (leader laps, `setTimeGap`, `setLaps`) (`T200_result_board.cpp`). `ResultChampionship::update` (L2 `0x00174588`) also highlights rows. |
| Kind | changed logic + new function |
| Evidence | `update`: loop `RaceEventQueue::findNth(queue, 0x0B, &car, n)`; first time per car: `dq[car]=1`, `dqCount++`, row = `entries->count - dqCount`, `setRunning(row)`. `setRowResult`: returns at once for a disqualified car; laps-only → `0x001B8218` (stores the leader's total and laps at board `+0x7EC/+0x7F0`, flag `+0x820`). GT3's event `0x0A` is the time-trial disqualification; in Lupo 2 the event numbering around it may have moved (?): `0x0B` is most likely Lupo 2's per-car disqualification (Pod race "Disqualification" message, RACEMENUTEXT `Disqualified`). |
| Port | New fields: `ResultArcade` +disqualified flags and count; `ResultBoard` +leader time, laps and flag. (a) needs the race-logic event (other area); (b) stands alone. |
| Verdict | **port** (a) with the race-logic disqualification event; (b) optional on its own |

### 15. Link-battle result: end only when one car is left

| Field | |
|---|---|
| What | In an i.LINK battle, GT3 starts the result's end as soon as the first car reaches the goal. Lupo 2 waits until all but one car have finished. |
| Where | `ResultLinkBattle::goal` L2 `0x001B2390` = US `0x001765C8` (`T191_result_link_battle.cpp`). |
| Kind | changed logic |
| Evidence | GT3: `flag(+0x950) = 1; ResultArcade::goal()`. L2: counts the finished cars (`+0x83C[]`) and sets the flag (`+0x880`) only if `finished >= count - 2`, then `ResultArcade::goal()`. |
| Port | A few lines, no new field. |
| Verdict | **port** (small, low priority) |

### 16. Car database: table numbering and maker tables

| Field | |
|---|---|
| What | (a) The paramDB tables numbered 30 and up were renumbered (CAR 30 → 35, COURSE 34 → 33, the spec table 33 → 32, the `+0x10` table 31 → 30). (b) The maker table gained 7 makers (asl, cadillac, dome, hyundai, mini, pontiac, spyker: 46 → 53). The brand table gained those 7 and pescarolo (52 → 60). Both are alphabetical, so most indices move. (c) The GCF file is `gameconf/concept-wagen.gcf`; its format and section types are unchanged. |
| Where | (a) `CarEquipments::setPreset/setCar/getDefaultPartCode`, `check_regulation`, `race_name_split`, `db_course_index`, `RaceDataBase::~RaceDataBase` (constants 35 / 31 where GT3 has 30 / 32), per `docs/paramdb_format.md` §4.2. (b) Tables L2 `0x002E0070` (53) and `0x002E0148` (60) = GT3 `g_002EFF20`/`g_002EFFD8` (`T486_maker_names.cpp`). (c) `product_config_init` L2 `0x0022CC70`; `ProductConfig::ProductConfig` L2 `0x0022B920`: same 9 section types (0 general, 4 events, 6, 8 courses, 0xA services, 0xC demos, 0xD card demos, 0xE cars, 0xF prizes). |
| Kind | new data or table |
| Evidence | Constants as listed; the string tables dumped. |
| Port | Only for transplanted Lupo 2 cars: their CAR rows' maker and brand indices must be remapped to GT3's tables, or the 7/8 new names appended at the end of GT3's tables (with their logos). Never renumber GT3's tables. |
| Verdict | **do not port** as such (data coupling); remap on transplant |

### 17. Message tables

| Field | |
|---|---|
| What | New entries in the real-time text tables. |
| Where | GT3 `src/game/data/messages/`: SETTING (`T093`, 5 → 11): `UNIT_LAPS` "%d lap(s)", `TIRE_NORMAL/SOFT/MEDIUM/HARD/DIRT` (ノーマル, ソフト, ミディアム, ハード, ダート). MEMORYCARDTEXT (`T095`, 15 → 22): `GTCGameData` "ＧＴＣ０１東京 ゲームデータ", `GTCReplayData`, `GTCLicenseReplayData`, `GTCBestLapReplayData`, `GTCSettingData`, `GTCVWBestLapReplayData` / `GTCVW2BestLapReplayData` (Lupo Cup V1/V2). DATABASERACECATEGORY (`T493`, 14 → 16): `TEXT_CATEGORY_CONCEPT_LICENSE` "License ", `TEXT_CATEGORY_SINGLE` "Single Race". DATABASEPARTS, SHORTMENU: unchanged. (RACEDISPLAY and RACEMENUTEXT: see "For other areas".) |
| Kind | new data |
| Evidence | `build/scratch/survey_menus_b/rtt.txt` (every `GT2KRealTimeText` table of both builds, by key). |
| Port | `TIRE_*` only with entry 4. |
| Verdict | **do not port** (all Concept-only), except `TIRE_*` with 4 |

## Unpaired Lupo 2 functions placed in the area

Every unpaired function, by group (L2 addresses):

**car_settings (230)**
- `0x00145B68` = GT3 `SettingControl::mainLoop` (twin US `0x0015CC28`, same size; now virtual): noise.
- `0x00145C48` `SettingControlConcept::mainLoop`: new, entry 3.
- `0x001465E8`, `0x00146698`, `0x00146888`, `0x00146928`, `0x00146A68`, `0x00146DC8`, `0x00146E68`, `0x00146FA8`, `0x00147048`, `0x001470E8`: these are the twins of `text_spring_toe_f/r`, `text_spring_brake_f/r`, `text_lsd_int_r`, `text_spring_damper_f/r`, `text_spring_bound_r`, `text_spring_rebound_f/r`. They are unpaired because the map shifted T081 by two. Noise: only the settings-object offsets moved (+0x10).
- `0x001472F8`..`0x00147778` (7): `text_gear_ratio_2..7`, `_final`: entry 1.
- `0x00147AC8` = `text_elec_ascc`, `0x00147CE8` = `gear_chart_car`: noise.
- `0x00147D28`..`0x001487D8` (20): `text_parts_*` (computer … weight), twins of US `0x0015ECE8`..`0x0015F668` (35 vs 32 insns): noise.
- `0x00148B58`, `0x00148C70`: `parts_tir_f/r_concept`, new, entry 4.
- `0x00148E50`..`0x0014A518` (74): the `attach_*` / `setting_attach_*` hooks, twins of US `0x0015FA40`..`0x00161108` (19 vs 20 insns, same counts per part): noise.
- `0x0014A8B8`, `0x0014A8F8`: entry 2.
- `0x0014ACE0` = `setting_text_user_carname`, `0x0014AD40` = `setting_text_car_current_power`: noise.
- `0x0014DE30`, `0x0014DE90`, `0x0014DF50`, `0x0014DFB0`: tyre sliders, new, entry 4.
- `0x0014E070` = `setting_slider_set_toe_f`: noise.
- `0x001503E0`, `0x00150470` (`SettingButtonList` creators) and `0x00150620`, `0x00150750`, `0x001507A8` (Concept actions): new, entry 3.
- `0x00150578` = `setting_commit_slider`, `0x001505D0` = `setting_retrieve_slider`: noise.
- `0x00150690`: twin of `setting_set_part` (US `0x00167038`; same calls, 48 vs 48 insns): noise.
- `0x00150F80`..`0x00152138` (64): `setting_owns_*`, twins of US `0x00167840`..`0x001689F8` (17 vs 18 insns): noise.
- `0x001528E8`..`0x00152A68` (17): `setting_adjust_*`, twins of US `0x00169208`..`0x00169408`: noise.
- `0x00152A80`: `isDirtTireOnly`, new, entry 5.
- `0x002BE6D8`..`0x002BE748` (13): `SettingControl` inline accessors emitted out of line: noise.
- `0x002BE758`: `~SettingControlConcept`, new, entry 3.

**gtmode (182)**
- T036 garage and spec hooks `0x0011BF28`..`0x0011DD70`: twins of `text_car_buy/sell_price`, `text_car_kilometrage`, `text_previous_power`, `text_car_drive_type`, `text_car_max_power/torque`, `make_time_*`, `make_car_render_*` and `text_focused_partsname`, all noise. The new ones are `0x0011C0C0` (current power text), `0x0011C188` (car list power/price) and `0x0011C588` ("Cr. %s" price from a car name). Those three are Concept car-select spec texts: **do not port** (entries 6 and 8).
- `0x00120958`..`0x00120CD8` (5): event-status objects and an event `%s_doc` item: Concept event pages, do not port.
- `0x00121B68`..`0x001224A0` (10): spec hooks, entry 6. `0x00121B68` (`aspiration`), `0x00121BB0` (`displacement`), `0x00121C90` (`engine_type`) and `0x00122450` (`wheel_base`) are noise.
- `0x001257F0`..`0x00126640` (35): T037 actions. These are twins of `action_buy_car`, `action_buy_guest_car`, `action_ride_car`, `action_sell_car`, the 11 `action_set_*` option toggles (11 insns each), `action_set_focus_parts`, the 6 `action_sort_*` and the oil change (`0x00126640`), all noise. `0x001257F0` and `0x00125888` (set the menu car from a garage or status index) are Concept, do not port.
- `0x00126DA8`, `0x00126E00`, `0x00126E20`: static init/destroy of the GTMODE message object: noise.
- `0x00129340`, `0x001293D0`, `0x00129418`: country and language set-up from a code (`Country::set`, nine `str_equal` tests). `0x001295E8` (396 insns): a page control with an MPEG image and background movie. They are placed in gtmode by link order only; both belong to app flow / menus A.
- `0x00135E18`..`0x00136FF8`, `0x0013A2D0`..`0x0013AED8`, `0x00163BE0`..`0x00166340`: licence-get, premium-get, car-get and notice-present controls (Concept): entry 8.
- `0x00170940`..`0x001737C0`: CarsItem drawing (concept car rows) and CarGetMessageItem: entry 8.
- `0x001B8A60`, `0x001C65F0`, `0x001C6620`, `0x001C6638`: TopButtonList and MenuCarSpec accessors: noise.
- `0x00237D10` (976 bytes, 1 caller): prize-model renderer variant near `PrizeBase`: entry 8 (?).
- `0x002B4478`..`0x002C2A00`, `0x002CA398`, `0x002D2520`, `0x002D2540`: linkonce accessors, constructors, destructors and typeinfo of the new classes (entry 8). `TrophyItem::setMixRatio` and `getMixRatio` are noise.

**memorycard (64)**
- `0x0013BB40`, `0x0013C170`, `0x0013C1E8`: MemorycardControl helpers (`begin`'s file-manager set-up, called 6 and 5 times): noise.
- `0x0013D190`, `0x0013D220`: the selected file's name texts (`getSelectedFile`, `strncpy`), weak twins of `memcard_text_title1/2`: noise.
- `0x0013D2D0`, `0x0013D318`, `0x0013D360`, `0x0013D5F8`, `0x0013D790`, `0x0013D918`: new `----` placeholder texts of the game, replay, best-lap and setting thumbnails: Concept card screens (entry 13).
- `0x0013E9B8`..`0x0013EA68`, `0x001400B8`, `0x00140100`: `MemorycardSlot` and layer helpers (vtable-indirection rewrites of GT3's `getManager` calls): noise.
- `0x00142C30` (193 insns): reads a game save from a port into a temporary `GameStatus` (`MemorycardAutoLoad::checkPort`, `MemorycardGameFile`, `mc_slot_read_size`, serialize). Probably the read side of the transfer (?): entry 13.
- `0x00143470`: the "already" text callback (twin shape of US `0x00159020`): entry 13.
- `0x001442B0`, `0x00144350`: icon-heap helpers (twins of US `0x0015A9A8`/`0x0015AA48`): noise.
- `0x00145198`: demo-file loader (in the L2 range of GT3's warning control): Concept demos (entry 7/13).
- `0x001672A8`, `0x00167460`, `0x00167618`: three menu runners (menu heap, PageManager, one control built by L2 `0x00166EE0`): Concept memory-card flows (entry 13) (?).
- `0x00169200`, `0x0016AAC0`..`0x0016B428`: title and `makeSearchPattern` / `getIconFiles` / `getInfoName` rewrites of the file kinds: entry 13.
- `0x0016BEE0`, `0x0016C590`..`0x0016CF68`: `MemorycardFileManager` constructor and slots (`updateFileIdForDelete`, `clearChangeCard`, `setScreenTop`, `setFilter`, `isThreadActive`, `pause`, `getXIconNum`, two new end slots); twins of US `0x00147870`..`0x00149758`: noise (backend).
- `0x002B4E70`..`0x002C1E70`: linkonce accessors, IconView / TitleView / SlotTitle factories, `CommentMenu`, `SaveWorkingMenu` progress and `MemorycardAlreadyTransferedDialog` dtor and typeinfo. The dialog is entry 13; the rest is noise.

**records (34)**
- `0x0015B960`..`0x0015BB00`, `0x0015C4E8`..`0x0015C7B0`, `0x0015D150`, `0x0015D1C8`: `RecordsMenuArgLicenseConcept` getters and the RecordsButtonList rework: entry 8.
- `0x001B7868` (twin of US `0x001B1900`): noise.
- `0x001B81C0`, `0x001B82C8`: ResultBoard setter variants: noise.
- `0x001B8218`, `0x001B83A0`: entry 14.
- `0x002BEFD0`..`0x002C9F18`: linkonce accessors (the logger graph, `Multilink`, `RacePODDisplay`, `CarIconItem`, `ResultBar`). They sit here by link order only. `Multilink` and `RacePODDisplay` belong to race logic and HUD (other areas); the rest is noise.

**save (81)**
- `0x0021E0F0`, `0x0021E1B0`, `0x0021E2D0`, `0x00220D88`, `0x002222A8`, `0x00222440`, `0x00223F70`, `0x00224080`, `0x00225040`..`0x002251B8`: present, car-count and arcade completion logic of the Concept save: entries 8 and 9.
- `0x0021EE80`, `0x0021F988`, `0x00220B20`, `0x00223208`, `0x002239F8`, `0x00225440`, `0x00225A78`, `0x0022B678`: empty constructors and `clear` stubs: noise.
- `0x002203D8`, `0x002204B8`, `0x00220A70`, `0x00223A10`..`0x00223CF0`, `0x00224E40`..`0x00224EE0`, `0x00226B58`, `0x00226B88`, `0x00226E90`: record load, save and accessor splits of GT3's `TimeTrialCourse`, `LicenseRecords`, `PrizeSetFlags`, `ArcadeRecords` and `GameOptions`: noise, or entry 9 where the layout differs.
- `0x00224688`, `0x002247D0`: ChampionshipSave car serialize and deserialize (wraps `CarEquipments::(de)serialize`): entry 9.
- `0x00226388`: `MenuStatus` constructor (new members): entry 9.
- `0x002267F0`: `MenuStatus::setCar` (weak twin US `0x00218078`): noise.
- `0x00226D08`: `GameOptions::save` with the controller block: noise.
- `0x00227988`: second copy of `getRunningCar`: noise.
- `0x00227C18`: `GameStatus::init` presents: entry 9.
- `0x002289C8`: `minDataSize`: noise.
- `0x00228FB8`: `setRaceCar2` helper: noise.
- `0x00229578`: RankingDigest: noise.
- `0x0022B710`: CourseBestTimes init: noise.
- `0x00249000`: GarageWheel accessor: noise.
- `0x002D1700`..`0x002D1800`: `RaceInput` linkonce accessors (race logic, other area).
- `0x002D19D8`..`0x002D1C98`: `GameStatus` linkonce accessors; they are the source of the layout in entry 9.

**data and messages (151)**
- `0x0015D558`, `0x0015D7F0`, `0x0015D848`, `0x0015D868`, `0x001B1B38`, `0x001B1C10`, `0x001B1D58`, `0x001B1DB0`, `0x001B1DD0`, `0x001B8528`, `0x001B8580`, `0x001B85A0`, `0x00249288`, `0x002493D8`, `0x00254518`, `0x00254570`, `0x00254590`: per-object static init/destroy wrappers (Lupo 2's compiler emits an extra one): noise.
- `0x0015D888`: SettingButtonList item set-up: entry 3.
- `0x001738B0` (gfx state set-up), `0x001B1320`/`0x001B1908`/`0x001B1B00` (biker model pose from the car view), `0x001B85C0` (a ButtonList factory): placed here by link order only. They belong to rendering / car model and to menus part A.
- `0x0022BD68`, `0x0022BE98`, `0x0022CAC0`: ProductConfig present-car helpers: entry 8.
- `0x00240790`: CarDataBase static destroy: noise.
- `0x00240CB8`: CarEquipments "reset all settings" (GT3 inlines the 9 `reset*` calls): noise.
- `0x002432D8`, `0x00243308`: accessor copies (weak twins US `0x002344A0`/`0x002344D0`): noise.
- `0x002433B0`: isCVT, entry 1.
- `0x00244548` (`getDisplacementText` copy): noise.
- `0x00244578`..`0x00244620`: entry 6.
- `0x002450D0`, `0x00245100`: `getMakerName`/`getBrandName` copies: noise (tables: entry 16).
- `0x00245D28`..`0x00245F60`: entries 3 and 4.
- `0x00247BE8` (`db_course_index` copy for the new numbering): entry 16.
- `0x00247EF0` (CAR-table accessor copy): noise.
- `0x002545B0`..`0x0025A998` (≈45 functions): the `pdiRiderman` bike-rider skeleton and Geomath helpers. They sit here by link order only (rendering / car model, other area).
- `0x002BF770`, `0x002BF7E0` (`SettingButtonList`): entry 3.
- `0x002C2A68`..`0x002C2C10`: message-item classes, entry 8.
- `0x002C90B8`..`0x002C9408`: Geomath, `HumanModel` and `RaceBikerModel` linkonce (other area).
- `0x002CA400` (`BlinkItem` typeinfo): entry 8.
- `0x002CEA78`, `0x002CEA80` (RawWindow): menus A.
- `0x002D2EC0`..`0x002D4608`: CarDataBase, CarEquipments, CarInformation and RaceDataBase inline accessors emitted out of line (`cannotBuy`, `setNatune`, `getSettingGearShiftMax`, `getOil`, `getGloss`, `IsChampionship`, …): noise.

## For other areas

- **Menus A / app flow (CVT)**: `isCVT` is also called by
  `ArcadeControl::isCarCVT(name)` (L2 `0x0010CDE8`, 4 arcade callers), by
  `run_free_run` (L2 `0x0012A310` call `0x0012AA24`) and by
  `run_arcade_race` (L2 `0x0012FCF0` call `0x00130190`). After the race-menu
  item set-up these lock the transmission (AT/MT) item for a CVT car
  (`docs/gearbox/gear-data.md` item 17c).
- **Car model / physics (Dualnote power bonus)**: `+75 kW / 0.7355`
  (L2 `0x001E1238`) is added in `ShowRoomCar::SetupShowRoomCar`
  (L2 `0x001D9908` call `0x001D99A8`, gate: spec `+0xB2` = 2 and
  `+0xB3` = 5) and in `gear_chart_with_sheet` (L2 `0x001E3AE8`). Once those
  are ported, every menu power text shows the bonus: `text_car_max_power`,
  `text_spec_max_power_real` and `setting_text_car_current_power` through
  `ShowRoomCar::power` / `GarageCar::getPower`. No menu code needs to change.
- **HUD / race messages**: the RACEDISPLAY table (`T186`) has +7 Pod-race
  messages (`PodSitDown`, `PodSitDownOK`, `PodOverrun`, `Disqualification`,
  `PodRaceStart1..3`). Its mode labels changed (`QUALIFY`/`DEMONSTRATION`/
  `LIVE`/`REPLAY`/`P.P.` → `Qualify`/`Demonstration`/`Live`/`Replay`/`PP`).
  RACEMENUTEXT (`T201`) has +`Disqualified`.
- **Race logic**: race event `0x0B` as a per-car disqualification
  (consumed by entry 14).
- **Arcade (menus A)**: `ArcadeRecords::difficultyOf` (L2 `0x00224C48`)
  parses Lupo 2's arcade race codes (`C%02d%c%d`) instead of GT3's
  difficulty letters (`E/N/H/P`). The framework API changed too:
  `CallbackItem::setItem` replaces `PageLayout::findHiddenItem` +
  `Item::setRect` in the `make_*` hooks, and `Button` grew 18 → 19 slots.
- **Rendering / car model**: the `pdiRiderman` / `HumanModel` /
  `RaceBikerModel` code at L2 `0x002545B0`..`0x0025A998`,
  `0x001B1320`..`0x001B1B38` and `0x002C90B8`..`0x002C9408`, and the gfx
  state helper L2 `0x001738B0`. They are placed in my folders by link order
  only.
- **App flow / menus A**: other code placed in my folders by link order
  only: country and language set-up L2 `0x00129340`, `0x001293D0`,
  `0x00129418`; a movie page control L2 `0x001295E8`; a race-start helper L2
  `0x00129C18` (`race_begin`, `select_race`); a ButtonList factory L2
  `0x001B85C0`; and `game_init` (L2 `0x0013B710`, uses
  `file_operation_lupo`).
- **Database (shared with physics)**: GEAR `+0x30` (gearbox type) is already
  covered by `docs/gearbox_dualnote.md`. The new spec fields of entry 6 are
  read only by menus.

## Noise and wrong pairs (looked at, no change of their own)

Reasons, in order of frequency:
- Lupo 2 builds every destructor as a deleting destructor (`operator
  delete` added, 25 vs 8–10 insns).
- Getters inlined or emitted out of line (records getters, `setting_set_*`
  20 vs 26 insns).
- The settings, menu and status objects moved by +0x10 (`SettingControl`
  `+0xE50` → `+0xE60`) and GameStatus offsets moved (`+0x307C8` → `+0x31A40`
  for the menu cars).
- A virtual call goes through another slot, because the vtables grew.
- Tail calls (`j fmt_sprintf`, `j strcpy`) replace `jal` + return.
- Regional strings (`SCUS-97102` → `PAPX-90509`, `BHP`/`mph` units,
  `%02d/%02d/%04d` → `%04d/%02d/%02d`) and file names (`paramdb_us.db` →
  `paramdb_bs.db`, `product.gcf` → `concept-wagen.gcf`).
- Leaf functions without frame info, where the tool's bounds stop at the
  first `jr $ra`: `ControllerConfig::resetKeys` L2 `0x00225C40` (18 vs 92;
  the full L2 range is 96 insns: same shape (?)) and `showroom_flag` /
  `showroom_value` / `showroom_index` (several L2 leaves read as one).

Wrong pairs (size over 2x, or a wrong `order` / `calls` pairing) and their
right twins:
- L2 `0x0013AEF8` → `car_get_event_name`: the right twin is
  `MemorycardControl::begin` US `0x00149A00`.
- L2 `0x0022BD10` (13 insns) → `ArcadeControl::setup` (284): a ProductConfig
  accessor; its twin is among US `0x0021D2C0`..`0x0021D320`.
- L2 `0x0015DB88` → `__tf Cell<GarageCar>`: it is the new `check_cvt`.
- L2 `0x00121ED0` → `text_spec_max_power`: the right twin is
  `text_spec_max_power_real`.
- The T081 shift by two (see the caution at the top).
- `CarGetControl::run` L2 `0x00136FB0` and `NotCarGetControl::run` L2
  `0x00164400`: their callees are paired across classes
  (TrophyGetControl / MiniPrizeControlBase). The functions are equivalent.
- `MachineTestRecords::getTime/getSpeed/getName`,
  `TimeTrialRecords::getTime/getName/insertCode` and `LicenseRecords::*`
  (save): pairs one slot off, because L2 inlines the table accessors.
- `GameOptions::load` L2 `0x00226BD8` (75 vs 20): `memcpy` and
  `ControllerConfig::load` are inlined.
- `T094`/`T114_static_destroy` (22 vs 10): they include the constructor of
  the next object.
- `Country::code`, `UnitSettings::powerName`, `UnitSettings::convSpeed`:
  regional variants (Japan only).
- `PrizeBase::setUpdateFlag/setDispFlag`: linkonce copies.

Below, one line per GT3 file: the pairs with difference ≥ 0.25 in this area
that are not a change entry above. "*" marks a pair the feature diff flagged
(calls, strings or floats differ). Each "*" pair was put to one of the
reasons above from its feature diff (unpaired callee resolved by weak twin,
deleting destructor, inlining, vtable slot); only those that looked
different were read instruction by instruction. "!" marks a size mismatch
over 2x. Generated from `build/scratch/survey_menus_b/cls_*.txt`.

Some pairs below carry a change that an entry above already covers, and
have nothing more of their own:
- The message constructors (`T093`, `T095`, `T186`, `T201`, `T493`) differ
  only in table size: entry 17.
- `db_course_index`, `check_regulation`, `race_name_split`,
  `CarEquipments::setCar/setPreset/getDefaultPartCode` carry the table
  renumbering: entry 16.
- The save pairs (`T415`..`T434`, `GameStatus::*`, `MenuStatus::reset`,
  `ChampionshipSave`, `PrizeSetFlags`) carry the Concept save layout:
  entry 9.
- `car_get_prizes`, `CarsItem`, `prize_check_*` and `PrizeCarList` carry
  the Concept presents: entry 8.
- The memory-card file and menu pairs carry the Concept file kinds, icons,
  vtable growth and transfer: entry 13.


#### car_settings

- `T080_setting_control.cpp` (4): 001458B8 SettingControl; 001459B8 begin; 002BE5A8* ~SettingControl(void); 002BE688 __tfQ29gt2k_menu14SettingCon
- `T081_setting_hooks.cpp` (22): 00146228* text_spring_camber_r; 001462C8* text_spring_stabilizer_f; 00146368* text_spring_stabilizer_r; 00146408* text_spring_brake_f; 001464A8* text_spring_brake_r; 00146548* text_lsd_int_f; 00146748* text_lsd_acl_f; 001467E8* text_lsd_acl_r; 001469C8* text_lsd_dec_f; 00146B48* text_lsd_dec_r; 00146BE8* text_spring_damper_f; 00146C88* text_spring_damper_r; 00146D28* text_spring_bound_f; 00146F08 text_spring_bound_r; 00147830 text_gear_ratio_auto; 001478E8 text_aero_r; 00147988* text_elec_aycc; 00147A28* text_elec_ascc; 00147B70 text_elec_tcsc; 00147C48 text_elec_vcdc; 00148D88* text_parts_oth_ayc_s; 0014ABF0 setting_create_turbine_zero_
- `T082_setting_sliders.cpp` (32): 0014D3B0 setting_slider_set_brake_f; 0014D4B8 setting_slider_set_brake_r; 0014D5C0 setting_slider_set_camber_f; 0014D6D0 setting_slider_set_camber_r; 0014D7E0 setting_slider_set_height_f; 0014D8F0 setting_slider_set_height_r; 0014DA00 setting_slider_set_rate_f; 0014DB10 setting_slider_set_rate_r; 0014DC20 setting_slider_set_stabilize; 0014DD28 setting_slider_set_stabilize; 0014E180 setting_slider_set_toe_r; 0014E290 setting_slider_set_lsd_init_; 0014E3A0 setting_slider_set_lsd_accl_; 0014E4B0 setting_slider_set_lsd_accl_; 0014E5C0 setting_slider_set_lsd_decl_; 0014E6D0 setting_slider_set_lsd_decl_; 0014E7E0 setting_slider_set_lsd_init_; 0014EA58 setting_slider_set_gear_rati; 0014EBB0 setting_slider_set_gear_rati; 0014ED08 setting_slider_set_gear_rati; 0014EE60 setting_slider_set_gear_rati; 0014EFB8 setting_slider_set_gear_rati; 0014F110 setting_slider_set_gear_rati; 0014F268 setting_slider_set_gear_rati; 0014F3C0 setting_slider_set_gear_rati; 0014F518 setting_slider_set_gear_rati; 0014F878 setting_slider_set_aero_f; 0014F988 setting_slider_set_aero_r; 0014FA98 setting_slider_set_elec_aycc; 0014FBA8 setting_slider_set_elec_ascc; 0014FCB0 setting_slider_set_elec_tcsc; 0014FDF8 setting_slider_set_elec_vcdc
- `T084_setting_checks.cpp` (5): 00152290 can_adjust_damper; 001522F8 can_adjust_bound_rebound; 001527B0 can_adjust_ayc; 001527F8 can_adjust_asm; 00152840 can_adjust_tcs
- `T096_setting_checks.cpp` (1): 0015D9A8* check_set_parts
- `T237_gear_chart.cpp` (3): 001C7FA0* GearChart; 002CC248 ~GearChart; 002CC2B0 __tfQ29gt2k_menu9GearChart

#### gtmode

- `T036_garage.cpp` (28): 0011CAB8* make_license_mark; 0011CC00* make_license_mark_rally; 0011CCE8* make_user_license; 0011CE30* make_need_license; 0011CFE0* make_license_cup; 0011D330 make_time_bronze; 0011D350* make_license_car_name; 0011D628* make_car_render; 0011DAD0 make_car_logo; 0011DBE8 car_color_box_select; 00120598* make_race_item; 00120690 make_event_hide; 00120748 make_event_flyer; 00120850 make_event_text; 00120E08* make_regulation_license; 00121080 make_regulation_power; 00121548* make_regulation_aspiration; 001216F0 make_regulation_tire; 001218A8* make_regulation_car; 00121A28 make_violated_license; 001228C8 make_oil_change_cylinder; 00122A10* make_tuner_logo; 00122AC8 is_ride_car; 002B3420* __tf9GarageCar; 002B34F8 ~DecayItem; 002B3560 __tfQ29gt2k_menu9DecayItem; 002B35D0 __tfQ29gt2k_menut4Cell1Z9Gar; 002D48D0* ~Cell
- `T037_gtmode_actions.cpp` (10): 001255E0* action_set_car_code; 00125630 action_set_race_code; 001256F8 action_set_race_code_spot; 00125BF8 action_fit_wheel; 00125D10 action_go_preview; 001261D0 action_fit_parts; 00126538* action_wash_car; 002B3620 ~TumbleCar; 002B3688 __tfQ29gt2k_menu9TumbleCar; 002B36D8 virtual_1
- `T039_gtmode_messages.cpp` (2): 00126D40* ~Message; 002B37A0 __tfQ213GT2KMSGGTMODE7Messag
- `T040_oil_change_cylinder.cpp` (4): 00126E40* OilChangeCylinder; 00127068* virtual_1; 001271D8 virtual_2; 002B37F0 __tfQ29gt2k_menu17OilChangeC
- `T041_event_status.cpp` (6): 00127720 virtual_1; 001277D8 virtual_2; 00127E48 virtual_1; 00127EB0 virtual_2; 002B3840 __tfQ29gt2k_menu17EventStatu; 002B3890 __tfQ29gt2k_menu23EventStatu
- `T042_event_hider.cpp` (3): 00128C38 virtual_2; 002B38E0 ~EventHider (_$_Q29gt2k_men; 002B3948 __tfQ29gt2k_menu10EventHider
- `T056_gtmode_trophy_control.cpp` (16): 00133BC0 create_get_cr_mini; 00133C80 create_get_cr_prize; 00133D40 load; 00133DC8 begin; 00133ED0 mainLoop; 00134008* trophy_prize_screen; 002B3DE0* ~ControlBas; 002B3E48 __tfQ29gt2k_menu26ControlBas; 002B3F10* ~MiniPrizeControl; 002B3FC0 __tfQ29gt2k_menu20MiniPrizeC; 002B4010* ~ArcadeTrophyContr; 002B40C0 __tfQ29gt2k_menu19ArcadeTrop; 002B4110* ~GTmodeTrophyContr; 002B41D0 __tfQ29gt2k_menu19GTmodeTrop; 002B4220* ~ConsolationControl; 002B42E0 __tfQ29gt2k_menu18Consolatio
- `T057_consolation_control.cpp` (3): 00134B58 load; 00134BE0 begin; 00134CE0 mainLoop
- `T058_trophy_item.cpp` (3): 001352D8* virtual_1; 002B4330 __tfQ29gt2k_menu10TrophyItem; 002B4580 __tfQ29gt2k_menu22ChampionSh
- `T059_license_get_control.cpp` (6): 00135E90* load; 00135FD8 mainLoop; 002B45D0* ~PrizeControlBase; 002B4650 __tfQ29gt2k_menu16PrizeContr; 002B47B0* ~LicenseGetControl; 002B4870* __tfQ29gt2k_menu17LicenseGet
- `T060_trophy_get.cpp` (4): 00136AC0 load; 00136D98 mainLoop; 002B4C10* ~TrophyGetControl; 002B4CD8 __tfQ29gt2k_menu16TrophyGetC
- `T061_car_get_control.cpp` (8): 00136C80* begin; 00136FB0* run; 00165E68* car_get_load_car; 001661F0 mainLoop; 002C0780* ~CarGetControl; 002C0828* __tfQ29gt2k_menu13CarGetCont; 002C0EB0 ~EventNameItem; 002C0F18 __tfQ29gt2k_menu13EventNameI
- `T099_prize_counter.cpp` (21): 0015EC90 advance; 0015ED10 PrizeCounter; 0015EFC0 setScale; 0015F028* create_count_T056; 0015F118* create_count_T057; 0015F208* create_count_T060; 0015F2F8 create_count_shadow_T056; 0015F3C0 create_count_shadow_T057; 0015F488 create_count_shadow_T060; 0015F550 create_get_money_disp_T060; 0015F630 create_get_money_disp_T056; 0015F710 create_get_money_disp_T057; 002B3E98 __tfQ29gt2k_menu12PrizeCount; 002B3ED8 ~PrizeCounter; 002BFAD0 ~PrizeCounterSh; 002BFB38 __tfQ29gt2k_menu22PrizeCount; 002BFBF0 ~PrizeCounterC; 002BFC58 __tfQ29gt2k_menu23PrizeCount; 002BFE08 ~PrizeCounterRe; 002BFE70 __tfQ29gt2k_menu22PrizeCount; 002BFEC0* virtual_1
- `T104_mini_prize_control.cpp` (3): 001640D8 set; 00164190 loadPages; 00164318 mainLoop
- `T105_not_car_get.cpp` (2): 00164218 begin; 00164400* run
- `T107_cars_item.cpp` (9): 001713E0 othersGone; 00171408* virtual_1; 00171710* virtual_2; 002C21C8 __tfQ29gt2k_menu6TheCar; 002C2208 ~TheCar; 002C22C8 __tfQ29gt2k_menu8CarsItem; 002C2320 ~CarsItem; 002C2708 ~CarGetMessageItem; 002C2770 __tfQ29gt2k_menu17CarGetMess
- `T047_top_button_list.cpp` (5): 001B8908 scrollTarget; 002CA208 ~TopButton (_$_Q29gt2k_menu9; 002CA278* __tfQ29gt2k_menu9TopButton; 002CA2D0 ~TopButtonList (_$_Q29gt; 002CA340* __tfQ29gt2k_menu13TopButtonL
- `T215_showroom_data.cpp` (5): 001C0798! showroom_index; 001C0820* showroom_read_files; 001C08E8*! showroom_flag; 001C0910*! showroom_value; 001C0938* showroom_value_ptr
- `T232_menu_car_spec.cpp` (7): 001C6530 setCar; 001C6598 setCar; 001C6608 getCode; 001C6650 getColorCount; 001C67A8 hasDirtTires; 001C67D0 getEquipments; 001C67D8* menu_car_spec_create
- `T459_prize_model.cpp` (2): 00236ED8 prize_matrix_copy; 00237578 prize_model_shadow
- `T460_prize_base.cpp` (12): 00237660* PrizeBase; 00237740 ~PrizeBase; 00237818 load; 00237940 update; 002379F8 show; 00237A38 hide; 00237A58 draw; 002D2468 __tf20PrizeModelSetContext; 002D24A8 ~PrizeModelSetContext; 002D24E0 __tf9PrizeBase; 002D2570 setDispFlag; 002D2590 setUpdateFlag
- `T045_gtmode_page_checks.cpp` (2): 002B3480 __tf16RegurationResult; 002B34C0 ~RegurationResult
- `T106_car_get_camera.cpp` (3): 002C0530* ~CarGetMenuCamera; 002C0598 __tfQ29gt2k_menu16CarGetMenu; 002C05E8* CarGetMenuCamera

#### memorycard

- `T068_memorycard_control.cpp` (13): 0013BE10* autoLoad; 0013BF50 getSelectedFile; 0013BF70 getCurrentSlot; 0013BFE0 getCurrentPort; 0013C000* getOtherPort; 0013C070 getOtherManager; 0013C0C8 isSaveMode; 0013C128 isLoadMode; 0013C260 pauseThreads; 0013C3D8 restartThreads; 0013C740* F_0014AE00; 002B4EA8 __tfQ29gt2k_menu17Memorycard; 002B4EF8* ~MemorycardControl
- `T069_memorycard_items.cpp` (21): 0013CE08 memcard_text_size; 0013CF60 memcard_text_comment; 0013D018 memcard_text_mode; 0013D118 memcard_text_copy_protect; 002B5078 memcard_game_thumbnail; 002B5118 memcard_replay_thumbnail; 002B51B8 memcard_bestlap_thumbnail; 002B5258 memcard_setting_thumbnail; 002B5AA8* memcard_create_progress_bar; 002B5E88 ~IconView; 002B5F48 __tfQ29gt2k_menu8IconView; 002B5F98* virtual_1; 002B61C8* virtual_2; 002B6628 __tfQ29gt2k_menu14SingleIcon; 002B6890 virtual_2; 002B6970 ~ProgressBar; 002B69D8 __tfQ29gt2k_menu11ProgressBa; 002B6AD8 ~TitleView; 002B6B50 __tfQ29gt2k_menu9TitleView; 002B6BC0 ~SlotTitle; 002B6C28 __tfQ29gt2k_menu9SlotTitle
- `T071_memorycard_slots.cpp` (15): 0013E448* setTopRow; 0013E500 setCursor; 0013E5C0* isReady; 0013E688* getCount; 0013E700* isOutside; 0013E798* checkCard; 0013E8A8 getFile; 0013E910 renderFile; 0013EB60* getCurrentPort; 0013EBC8 getPageManager; 0013EBD8 getPages; 0013ECE8* update; 0013F708 updateInfo; 0013F7F8 openMenu; 0013FB80 begin
- `T072_memorycard_icon_sprites.cpp` (1): 0013FF60 draw
- `T073_memorycard_menu.cpp` (20): 00140208 nextSelectable; 00140288 prevSelectable; 00140318 select; 00140910* MemorycardLayerMen; 00140C08 setPosition; 00140DA0 ~MemorycardLay; 00140E40 select; 00140F08 checkDecide; 00140F98 update; 00141038 render; 002B6D28 ~MemorycardMenuItem; 002B6D90 __tfQ29gt2k_menu18Memorycard; 002B6DF0 __tfQ29gt2k_menut4List1ZQ29g; 002B6E40 __tfQ29gt2k_menu14Memorycard; 002B6EA8 ~MemorycardMenu; 002B6F10 __tfQ29gt2k_menu22Memorycard; 002B6F78 __tfQ29gt2k_menu19Memorycard; 002B6FE0 __tfQ29gt2k_menu23Memorycard; 002B7058* ~L; 002B70B0 __tfQ29gt2k_menut4Cell1ZQ29g
- `T074_memorycard_operation_menu.cpp` (44): 00141098* MemorycardOper; 002B7100 ~MemorycardOpe; 002B7170* __tfQ29gt2k_menu23Memorycard; 002B71D8 ~CopyMenu; 002B7240 __tfQ29gt2k_menu8CopyMenu; 002B7340 select; 002B73C0* makeSubMenu; 002B7C68 ~DeleteMenu; 002B7CD0 __tfQ29gt2k_menu10DeleteMenu; 002B7D40* makeSubMenu; 002B8190 __tfQ29gt2k_menu11CommentMen; 002B81E8 select; 002B8398 ~CommentChangeMenu; 002B8400 __tfQ29gt2k_menu17CommentCha; 002B8450 preCommentEdit; 002B8468* makeSubMenu; 002B8808 ~SaveMenu; 002B8870 __tfQ29gt2k_menu8SaveMenu; 002B88C0 preCommentEdit; 002B88D0* makeSubMenu; 002B8CC8 ~OverwriteMenu; 002B8D30 __tfQ29gt2k_menu13OverwriteM; 002B8D80 preCommentEdit; 002B8DF0* makeSubMenu; 002B90F8 ~LoadMenu; 002B9160 __tfQ29gt2k_menu8LoadMenu; 002B91B0* makeSubMenu; 002B9548 ~ConfirmMenu; 002B95B8* __tfQ29gt2k_menu11ConfirmMen; 002B9610 ~CopyWorkingMenu; 002B9678 __tfQ29gt2k_menu15CopyWorkin; 002B96C8* makeSubMenu; 002B9D90 ~DeleteWorkingMenu; 002B9DF8 __tfQ29gt2k_menu17DeleteWork; 002B9E48* makeSubMenu; 002BA418 ~CommentChang; 002BA480 __tfQ29gt2k_menu24CommentCha; 002BA4D0* makeSubMenu; 002BAAC8 ~SaveWorkingMenu; 002BAB30 __tfQ29gt2k_menu15SaveWorkin; 002BAB80 makeSubMenu; 002BB1B8 ~LoadWorkingMenu; 002BB220 __tfQ29gt2k_menu15LoadWorkin; 002BB270* makeSubMenu
- `T075_memorycard_game_slots.cpp` (16): 00141BF0 memorycard_game_file_exists; 00141CA8 ~MemorycardSlot; 00141D20* open; 00142078 update; 001422E8 ~MemorycardAutoLoad; 001423C8 load; 00142AA0 check; 00142AB8 start; 00142B60 update; 002BBB30 ~MemorycardSlotMenu; 002BBB98 __tfQ29gt2k_menu18Memorycard; 002BBC40 select; 002BBCC0* makeSubMenu; 002BC8C0* makeSubMenu; 002BD830 ~WorkingMenu; 002BD898 __tfQ29gt2k_menu11WorkingMen
- `T076_memorycard_dialogs.cpp` (18): 001432D0* MemorycardNoSp; 001434C0* MemorycardForma; 002BD928 ~MemorycardCancel; 002BD990 __tfQ29gt2k_menu20Memorycard; 002BDA20 ~MemorycardError; 002BDA90* __tfQ29gt2k_menu21Memorycard; 002BDAE8 ~MemorycardR; 002BDB58 __tfQ29gt2k_menu25Memorycard; 002BDBB0 ~Memorycard; 002BDC20 __tfQ29gt2k_menu26Memorycard; 002BDC78 ~MemorycardNoS; 002BDCE8 __tfQ29gt2k_menu23Memorycard; 002BDD40 ~Memoryca; 002BDDB0 __tfQ29gt2k_menu28Memorycard; 002BDED0 ~MemorycardForm; 002BDF40* __tfQ29gt2k_menu22Memorycard; 002BDF98 ~FormatWorkingMenu; 002BE000 __tfQ29gt2k_menu17FormatWork
- `T077_memorycard_icon.cpp` (8): 001437D8 icon_model_size; 001440C0 icon_slot_create; 001441B8 icon_slot_get; 00144240* icon_slots_alloc; 001443E0 icon_light_init; 00144490 icon_light_vertex; 001449A0 icon_model_draw; 00144F08 icon_anim_key_value
- `T078_memorycard_warning.cpp` (5): 001452E8* load; 00145308* unload; 00145328* game_save_size; 00145388* run; 001453D8* __tfQ29gt2k_menu24Memorycard
- `T108_memorycard_file.cpp` (78): 00167A28 MemorycardFile; 00167AE0* MemorycardFile; 00167B68* ~MemorycardFile; 00167C40 getInfoName; 00167C50 checkSelectable; 00167DC0 updateSize; 00167DD8 calcSize; 00168080* copyTo; 00168698 readInfo; 00168738 render; 00168B78 packTitle; 00168CF0 readInfo; 00168DE0* writeIconSys; 00168F50* writeIcons; 00169310* MemorycardOther; 00169370* ~MemorycardOthe; 001693F8 readIcon; 001697E8 getTitle; 001698A0 getTitle; 00169B68*! ~MemorycardGT3File; 00169BD0 mcInitIconSys; 00169D00* mcGetNamePrefix; 00169F78 makeUniqueName; 0016A078* mcGetNameCourse; 0016A0B0* mcGetNameCar; 0016A0E8* mcGetNameCategory; 0016A120* mcGetNameSubCategory; 0016A2B8 calcSize; 0016A388* saveFiles; 0016A7D8* changeComment; 0016A8E0* MemorycardGameFile; 0016A9F8 getInfoName; 0016AA08 getIconFiles; 0016AA18* save; 0016AB10 checkSelectable; 0016ACF0* save; 0016ADA8* getNameCode; 0016AF08* render; 0016B168* getNameCod; 0016B350* getNameCode; 0016B4D8* save; 0016B698* ~MemorycardFileList; 0016B760 releaseAll; 0016BA00 release; 0016BB10 findMaxScoreWithIco; 002C1010* mcDecodeNameCode; 002C1060 __tfQ29gt2k_menu14Memorycard; 002C11B8* ~MemorycardPS1File; 002C1220 __tfQ29gt2k_menu17Memorycard; 002C1270* clone; 002C1380* ~MemorycardPS2File; 002C13E8 __tfQ29gt2k_menu17Memorycard; 002C1440 __tfQ29gt2k_menu22Memorycard; 002C1490* clone; 002C14F0* ~MemorycardDamage; 002C1558 __tfQ29gt2k_menu20Memorycard; 002C15A8* clone; 002C1610* ~MemorycardDummyFi; 002C1678 __tfQ29gt2k_menu19Memorycard; 002C16C8* clone; 002C1728* ~MemorycardDelet; 002C1790 __tfQ29gt2k_menu21Memorycard; 002C17E0 __tfQ29gt2k_menu17Memorycard; 002C18A0* ~MemorycardGameFile; 002C1908 __tfQ29gt2k_menu18Memorycard; 002C1958* clone; 002C1A00* ~MemorycardReplay; 002C1A68 __tfQ29gt2k_menu20Memorycard; 002C1AB8* clone; 002C1B60* ~Memorycar; 002C1BC8 __tfQ29gt2k_menu27Memorycard; 002C1C18* clone; 002C1CC0* ~MemorycardSetti; 002C1D28 __tfQ29gt2k_menu21Memorycard; 002C1D78* clone; 002C1E20 __tfQ29gt2k_menu18Memorycard; 002C1E78 __tfQ29gt2k_menut4Cell1ZQ29g; 002C1EC8 __tfQ29gt2k_menut4List1ZQ29g
- `T109_memorycard_file_manager.cpp` (36): 0016C158 ~MemorycardFileM; 0016C248 clear; 0016C318 getFile; 0016C3A0 updateFileId; 0016C7B0 createThread; 0016C848 deleteThread; 0016C8B8 isConnect; 0016C998 stop; 0016C9E8 threadMain; 0016CBD8* process; 0016CE78* getFileDistance; 0016CEA8 getIndexDistance; 0016D100 buildIndex; 0016D1D8* buildSortedIndex; 0016D9E0 readScreenInfo; 0016DB20 readOtherInfo; 0016DBE0 readInfo; 0016DF28 readAllSizes; 0016DFD0 readSize; 0016E0C8 readScreenIcons; 0016E1B8 readAllIcons; 0016E260 readIcon; 002BE3B0 __tfQ29gt2k_menu25Memorycard; 002BE3F0 ~MemorycardF; 002C1F18 __tfQ29gt2k_menu21Memorycard; 002C1F68 getPort; 002C1F78 clearError; 002C1F98 clearChangeCard; 002C1FA8 setSelect; 002C1FB8 setScreenTop; 002C1FC8 setScreenSize; 002C1FD8 isError; 002C1FE8 isReading; 002C1FF8 isFormat; 002C2008 isThreadActive; 002C2018 getFileCount
- `T110_memorycard_dir_cache.cpp` (3): 0016E4C8* ~MemorycardDirCache; 0016E580 clear; 0016E610 findPage

#### records

- `T089_records_menu_args.cpp` (33): 0015B378* getName; 0015B3C0* getDate; 0015B418* getCode; 0015B4A8* deleteEnable; 0015B4F0 RecordsMenuArgFre; 0015B5D8 getName; 0015B620 getDate; 0015B6C8 getCode; 0015B760* isValid; 0015B7C0* getTime; 0015B820* getName; 0015B868* getDate; 0015B8C0* getCode; 0015BBF0 getName; 0015BC28* getDate; 0015BC60* getCarName; 0015BCB0* getCode; 0015BDC8 getName; 0015BE00* getDate; 0015BE38* getCarName; 0015BE88* getCode; 0015BFA8 getName; 0015BFE0* getDate; 0015C018* getCarName; 0015C068* getCode; 002BF018 __tfQ29gt2k_menu24RecordsMen; 002BF088 __tfQ29gt2k_menu20RecordsMen; 002BF0E0 __tfQ29gt2k_menu21RecordsMen; 002BF1C0 __tfQ29gt2k_menu25RecordsMen; 002BF228 __tfQ29gt2k_menu20RecordsMen; 002BF288 __tfQ29gt2k_menu21RecordsMen; 002BF2E8 __tfQ29gt2k_menu22RecordsMen; 002BF348 __tfQ29gt2k_menu14RecordsMen
- `T090_records_control.cpp` (5): 0015C128 begin; 0015C248 mainLoop; 0015C330 run_records_menu; 002BF388* ~RecordsControl; 002BF420 __tfQ29gt2k_menu14RecordsCon
- `T092_records_button_list.cpp` (7): 0015C918* RecordsButtonList; 0015CDD0* createButton; 0015CE88 openPanel; 0015D010* decide; 002BF470 ~RecordsButton; 002BF4E0* __tfQ29gt2k_menu13RecordsBut; 002BF618* __tfQ29gt2k_menu17RecordsBut
- `T116_race_result_base.cpp` (3): 00173D50 update; 002C2CF0 __tf14RaceResultBase; 002C2D48 ~RaceResultBase
- `T117_result_arcade.cpp` (9): 00173EA8 start; 00173F80 cleanup; 00173FC0 setDisplays; 00174118 waitEnd; 001742B8 render; 001742E0 goal; 00174478 finish; 002C2DB0 ~ResultArcade; 002C2E28 __tf12ResultArcade
- `T118_result_championship.cpp` (6): 00174550 start; 00174588* update; 00174868 countDown; 002C2E78 ~ResultChampionship; 002C2EF0 __tf18ResultChampionship; 002C2F40* ResultChampionship
- `T119_result_implement_records.cpp` (6): 00174A88 finish; 00174AE8 update; 00174CA0 render; 00174E38 drawRecord; 002C2FA8 ~ResultImplementRecords; 002C2FE0 __tf22ResultImplementRecords
- `T120_result_machine_test.cpp` (4): 00174F68* checkRecords; 00175010 getRecordTitle; 002C3038 ~ResultMachineTest; 002C3070 __tf17ResultMachineTest
- `T189_result_time_trial.cpp` (3): 001B20E0* checkRecords; 002C94E0 ~ResultTimeTrial; 002C9518 __tf15ResultTimeTrial
- `T190_result_free_run.cpp` (5): 001B2178 setCourse; 001B21A8 checkRecords; 001B2208 getRecord; 002C9568 ~ResultFreeRun; 002C95A0 __tf13ResultFreeRun
- `T191_result_link_battle.cpp` (6): 001B2240 start; 001B2278 update; 001B22F8 waitEnd; 001B2390 goal; 002C95F0 ~ResultLinkBattle; 002C9668 __tf16ResultLinkBattle
- `T192_result_2p_battle.cpp` (3): 001B2430 waitEnd; 002C96B8 ~Result2PBattle; 002C9730 __tf14Result2PBattle
- `T200_result_board.cpp` (25): 001B7028 update; 001B7068 draw; 001B7938* ResultBoard; 001B7C78 ~ResultBoard; 001B7DA8 setup; 001B7EE8 start; 001B7EF8 update; 001B7FA0 stop; 001B7FA8 draw; 001B8050 close; 001B8128 setTime; 001B8300 setPoints; 001B8350 setCar; 001B83D0 highlight; 001B8450 isDone; 002C9D40 ~CarIconItem; 002C9DA8 __tf11CarIconItem; 002C9E00 __tf9ResultBar; 002C9EA8* ~ResultBar; 002C9F20* ~ResultTitleBar; 002C9F88 __tf14ResultTitleBar; 002C9FD8* ~ResultTermBar; 002CA040 __tf13ResultTermBar; 002CA090* ~ResultRankBar; 002CA0F8 __tf13ResultRankBar

#### save

- `T415_machine_test_records.cpp` (13): 0021E3A8* clearZero; 0021E448* insertTime; 0021EA60 setCode; 0021EB30 insertDate; 0021EBB0* remove; 0021EE98* init; 0021EED0 table; 0021EF48*! getTime; 0021EFD8*! getSpeed; 0021F008* setCar; 0021F048*! getName; 0021FCB8* load; 0021FD00* save
- `T416_free_run_records.cpp` (11): 0021F550 setCar; 0021F5A8 setCode; 0021F678 insertDate; 0021F6F8 remove; 0021FA40* findTime; 0021FA70* insertTime; 0021FAA0* getTime; 0021FB10* getName; 0021FB80* getCar; 0021FBF0* getCode; 0021FC20* getDate
- `T417_time_trial_records.cpp` (11): 0021FF70 insertTime; 00220218 insertDate; 002202B8 remove; 002203B8* getSize; 00220598 TimeTrialRecords; 002207F0* getName; 00220818*! getTime; 00220840* insertCode; 002209A8 allFirstPlaces; 00220A18* load; 002253A0* save
- `T419_prize_cars.cpp` (7): 00221478* prize_check_race; 002218C8 prize_check_series; 00221BC8 prize_check_gtmode_complete; 00221D00 prize_check_achieve_half; 00221DD8 prize_check_achieve_3_quarte; 00221EB0 prize_check_achieve_complete; 00221F88* PrizeCarList
- `T420_license_records.cpp` (9): 00222DB8 insertDate; 002232F0 setMedal; 00223340* isPassed; 002233D0* isAllGold; 00223500* countMedal; 00223638* passAll; 002237B0 remove; 00223AC0* load; 00223BF0* save
- `T421_prize_set_flags.cpp` (4): 00223CB8* carCode; 00223D20* save; 00224048* set; 002241C0* isSet
- `T424_arcade_records.cpp` (16): 00223D58* addTime; 00223D88* getTime; 00223EF0* save; 00224990 ArcadeRecord; 002249E8 updatePlace; 00224AD8* getSize; 00224AF8* load; 00224B48* save; 00224B98 ArcadeRecords; 00224C48* difficultyOf; 00224EB0* getPlace; 00224FC0 allWonEasy; 00224FE0 allWonNormal; 00225000 allWonHard; 00225020 allWonProfessional; 002251F0* score
- `T434_best_times.cpp` (4): 00223DB8* BestTime; 00223DF0* CourseBestTimes; 00223E70* get; 00223E98* load
- `T422_championship_save.cpp` (1): 00224420* ChampionshipSave
- `T425_event_results.cpp` (12): 00225408* save; 00225428* load; 00225458* EventResults; 00225568 count; 00225620* leagueWon; 00225830 beginnerLeagueWon; 00225848 amateurLeagueWon; 00225860 professionalLeagueWon; 00225940 dirtEventsWon; 00225958 enduranceEventsWon; 002259B0* getSize; 00225A30* save
- `T426_controller_config.cpp` (3): 00225C40! resetKeys; 00225F60 setKey; 00225F90 getKey
- `T427_menu_status.cpp` (5): 00226528* reset; 00226790 setEvent; 002267C0 setStr84; 00226828* __tfQ29gt2k_menut4Cell1Z9Gar; 00226880 clearFocusCar
- `T428_game_options.cpp` (4): 00226890* GameOptions; 00226A80* reset; 00226BD8*! load; 00226E28 apply
- `T429_game_status.cpp` (31): 00226EE0 getClassInfo; 00226EF0 createGameStatus; 00226F60* GameStatus; 00227430* ~GameStatus; 002276A0* init; 00227950 setRunningCarIndex; 002279E0 findFreeCar; 00227A40* addCar; 00227FC8 getWinRate; 00228500* getCompletionRatio; 002285B8* allArcadeRacesWon; 002286A8* updateArcadeProgress; 00228928* dataSize; 00228988 getDataSize; 002289A8 maxDataSize; 00228A10* restoreRandomSeed; 00228A58 getVersion; 00228A68* load; 00228D78* save; 00228F50* describe; 00229018* setRaceCar2; 00229038* setRaceCar2ByCode; 0022B838* minDataSize; 002D1808* ~GarageCarList; 002D1870 __tf13GarageCarList; 002D18C0* ~GarageWheelList; 002D1928 __tf15GarageWheelList; 002D1978 __tfQ29gt2k_game10GameStatus; 002D1B18* getRunningCar; 002D1CA8 __tfQ29gt2k_menut4List1Z9Gar; 002D1CF8 __tfQ29gt2k_menut4List1Z11Ga
- `T430_music_play_list.cpp` (2): 00229278* reorder; 00229308* orderShuffle
- `T431_ranking_code.cpp` (2): 002295E8* RankingHash; 00229990* RankingCodeBlock
- `T488_setting_serialize.cpp` (7): 00247660 getClassInfo; 00247680 getSubVersion; 002477A8* applySettings; 00247AB8*! T488_static_fini (_GLOBAL_$D; 00247B18* T488_static_init (_GLOBAL_$I; 002D4770 __tf16SettingSerialize; 002D4820* ~SettingSerialize
- `T490_garage_car.cpp` (5): 00248CF8 getName; 00248D98 getModelName; 00248DB8* getWheelModelName; 00248EE0* load; 00248F28* save
- `T491_garage_wheel.cpp` (3): 002D4928* ~GarageWheel (_$_11GarageWhe; 002D4990 __tf11GarageWheel; 002D49E0 __tfQ29gt2k_menut4Cell1Z11Ga

#### data

- `T093_msg_setting.cpp` (4): 0015D4F0* ~Message; 0015D5B0* T093_static_destroy (_GLOBAL; 0015D5D0* T093_static_init (_GLOBAL_$I; 002BF680 __tfQ214GT2KMSGSETTING7Messa
- `T094_msg_prize_rt.cpp` (4): 0015D638* ~Message; 0015D6A0*! T094_static_destroy (_GLOBAL; 0015D6F8* T094_static_init (_GLOBAL_$I; 002BF6D0 __tfQ219GT2KMSGPRIZE_RT_MSG7
- `T114_msg_shortmenu.cpp` (4): 0015D718* T114_static_init (_GLOBAL_$I; 00173BE8* ~Message; 00173C50*! T114_static_destroy (_GLOBAL; 002C2CA0 __tfQ216GT2KMSGSHORTMENU7Mes
- `T095_msg_memorycard.cpp` (5): 0015D738* Message; 0015D788* ~Message; 00173CA8* T095_static_destroy (_GLOBAL; 00173CC8* T095_static_init (_GLOBAL_$I; 002BF720 __tfQ221GT2KMSGMEMORYCARDTEX
- `T186_msg_race_display.cpp` (5): 001B1B58* Message; 001B1BA8* ~Message; 001B1C68* T186_static_destroy (_GLOBAL; 001B1C88* T186_static_init (_GLOBAL_$I; 002C9440 __tfQ218GT2KMSGRACEDISPLAY7M
- `T187_msg_race_msg.cpp` (2): 001B1CF0* ~Message; 002C9490 __tfQ214GT2KMSGRACEMSG7Messa
- `T201_msg_racemenutext.cpp` (3): 001B8470* Message; 001B84C0* ~Message; 002CA1B8 __tfQ219GT2KMSGRACEMENUTEXT7
- `T435_product_config.cpp` (8): 0022C448 findEvent; 0022C528 find1C; 0022C830*! prizeCarCode; 0022CC70* product_config_init; 002D1D48* gcf_find_event; 002D1E00* gcf_find_1C; 002D1EB8* gcf_find_prize_set; 002D1F70* gcf_find_service
- `T481_car_data_base.cpp` (19): 00240200 ~DataBlock; 00240248 ~StringsDataBase; 00240280 CarDataBase; 002402A0 ~CarDataBase; 00240330* loadStrings; 00240378* loadUniStrings; 00240448* loadDataBase; 00240608 getIndex; 00240668 isJapan; 002406A8* isEurope; 002407E8* T481_static_fini (_GLOBAL_$D; 00240808* T481_static_init (_GLOBAL_$I; 002D2D10 __tf9DataBlock; 002D2D98 __tf15StringsDataBase; 002D2E28 __tf11CarDataBase; 002D2F40 getString (getString__11CarD; 002D2F58 getStringSize (getStringSize; 002D2F70 getMultiByteString (getMulti; 002D2F88 getMultiByteStringSize (getM
- `T482_car_equipments.cpp` (65): 002409C8* setPreset; 00240AE8* setCar; 00243158 getCarRecord; 00243468* getPartCategory; 00243550 getPartKind; 00243630* getDefaultPartCode; 00243740 getEngineRecord; 00243780 getSuspensionRecord; 002437C0 getBrakecontrollerRecord; 00243800 getGearRecord; 00243840 getDrivetrainRecord; 00243880 getRacingmodifyRecord; 002438C0 getChassisRecord; 002438E0 getLSDRecord; 00243920 getASCCRecord; 00243960 getTCSCRecord; 002439A0 getBrakeRecord; 002439E0 getLightweightRecord; 00243A20 getPortpolishRecord; 00243A60 getEnginebalanceRecord; 00243AA0 getDisplacementRecord; 00243AE0 getComputerRecord; 00243B20 getNATuneRecord; 00243B60 getTurbineRecord; 00243BA0 getFlywheelRecord; 00243BE0 getClutchRecord; 00243C20 getPropellershaftRecord; 00243C60 getIntercoolerRecord; 00243CA0 getMufflerRecord; 00243CE0 getWheelRecord; 00243D20 getFronttireRecord; 00243D60 getReartireRecord; 002443B8* getDriveTrainText; 002444A8 getMaxPowerRpmText; 00244518 getMaxTorqueRpmText; 00244858 getPartFullName; 00244880 getPartName; 002448A8 getBrakeName; 002448C8 getBrakecontrollerName; 002448E8 getClutchName; 00244908 getComputerName; 00244928 getDisplacementName; 00244948 getEnginebalanceName; 00244968 getFlywheelName; 00244988 getGearName; 00244A00 getLightweightName; 00244A20 getLSDName; 00244A40 getMufflerName; 00244A60 getNATuneName; 00244A80 getPortpolishName; 00244AF8 getRacingmodifyName; 00244B18 getSuspensionName; 00244B38 getTCSCName; 00244B58 getFronttireName; 00244B78 getReartireName; 00244B98* getTurbineName; 00244C10 getASCCName; 00244C30 getDrivetrainName; 00244C88* F_00235C28; 00244D60* fitPart; 00245B30 F_00236A38; 002D2FE0 __tf13CarEquipments; 002D3D08* F_002376C0 (F_002376C0__13Ca; 002D3D50* F_002376E8 (F_002376E8__13Ca; 002D3D80 __tf17CarSaveEquipments
- `T483_car_information.cpp` (6): 00245B50* CarInformation; 00245C38* ~CarInformation; 002461B8 getPartPrice; 00246230* getPartPrice(int, u64); 002D3DC0* __tf14CarInformation; 002D4008 __tf18CarSaveInformation
- `T484_race_database.cpp` (3): 00246718* ~RaceDataBase; 00246DE8 getChampionshipPoint; 002D4048 __tf12RaceDataBase
- `T487_race_mode_database.cpp` (3): 00247240 ~RaceModeDatabase; 002473A8 selectMode(const char *); 002D4620 __tf16RaceModeDatabase
- `T489_car_database.cpp` (13): 00247B38* db_course_index; 00247B98 db_race_index; 00247BC0 db_car_index; 00247C60 db_course_code; 00247CA0 db_car_code; 00247CE0 db_car_name; 00247D60 db_car_name2; 00247F20 check_regulation_current_car; 00248110* check_regulation; 002485E0* race_name_split; 00248870 race_kind_text; 00248988 db_car_is_locked; 00248A28 region_lock_check
- `T492_msg_database_parts.cpp` (4): 00249220* ~Message; 002492E0* T492_static_destroy (_GLOBAL; 00249300* T492_static_init (_GLOBAL_$I; 002D4A30 __tfQ220GT2KMSGDATABASEPARTS
- `T493_msg_database_race_category.cpp` (5): 00249320* Message; 00249370* ~Message; 00249430* T493_static_destroy (_GLOBAL; 00249450* T493_static_init (_GLOBAL_$I; 002D4A80 __tfQ227GT2KMSGDATABASERACEC
- `T516_country.cpp` (1): 00253AE8! code
- `T518_unit_settings.cpp` (8): 00253F80* ~UnitSettings; 002541B8* speedName; 002541F0*! powerName; 00254280* torqueName; 002542E0* distanceName; 00254318* shortDistanceName; 00254350! convSpeed; 00254398 convPower
- `T203_part_names.cpp` (1): 002CA450* find_part_name

## Corrections (after the pairs check, `build/scratch/survey_menus_b/pairs_fix.csv`)

Checking every pair by its callees, constants and strings changed these points of the report above;
the verdicts of entries 1 to 17 do not change.

1. L2 `0x0011C0C0`, `0x0011C188`, `0x0011C588` are GT3 twins (`text_current_power`,
   `text_ps_after`, `make_goods_car_price`), not Concept-new (gtmode list).
2. L2 `0x00120958`..`0x00120CD8` are twins of `make_event_status`, `make_event_status_button`,
   `make_arrow`, `make_event_course_doc`, `make_event_course_logo`, not Concept event pages.
3. L2 `0x001257F0` / `0x00125888` are `action_set_garage_car` / `action_set_garage_car_trade`;
   the 11 "option toggles" are the `action_go_*` page jumps; `go_save_in_home`'s mode changed
   from 30 to 31 (follows Lupo 2's run-mode renumbering: not ported).
4. The memory-card file-manager vtables do not grow at the end: Lupo 2 inserts
   `removeFileCache` and `addDeletedEntry` at slots 12-13 (from L2 slot 14, slot j = GT3 slot j-2).
5. L2 `0x001738B0` is `CongratulationMessageItem`'s draw (Concept, entry 8), not a rendering helper.

## Delta review (map v2)

Source: `build/scratch/delta/menus_b.txt`. It lists 54 pairs that are new or
changed in map v2 and 67 Lupo 2 functions ("new?") that the new function
list adds, are unpaired, and are placed in this area. Every line was checked
by its callees, constants, strings and callers (`build/scratch/survey_menus_b/delta_pairs.txt`,
`delta_new.txt`). The pair knowledge is added to `build/scratch/survey_menus_b/pairs_fix.csv`
(written by `make_fix.py`).

**Result: no new change to port.** Entries 1 to 17 and their verdicts stand.
- No layout change behind entry 11's port: `ControllerConfig::load/save`
  still copy 0x250 bytes.
- Entry 9, sharpened: Lupo 2's game save writes the free-run block
  (`+0x26EA8`) but no machine-test block. The block at `+0x23510` is a second
  licence-records table of 340-byte tests (the Concept licences).
- Twelve map v2 pairs are wrong. The right twins are given below where they
  could be found.

### Pairs (54)

| L2 | map v2 pair | Verdict |
|---|---|---|
| `0x001148E0` | `T044_static_init` | **Wrong pair.** The Concept top menu's factory table (`menu_category`, `menu_difficulty`, `menu_game`, `menu_goodies`, `menu_level`, `menu_license`, `menu_map`, `menu_top`): menus A. GT3's T044 table (garage, dealer, wheel shop …) has no twin: removed in Lupo 2. |
| `0x0011BEE0` | `text_focused_carname` | Right; noise. |
| `0x00134A38`, `0x00134E18`, `0x001372F8`, `0x0015C800` | `T056/T057/T060/T091_static_init` | Right; noise (the same hook names). |
| `0x00135C00` | `T058_static_init` | Right; adds 8 trophy-item hooks `prem_00..07` (Concept premium cars): entry 8, do not port. |
| `0x001361B0` | `license_get_screen` | Right. Concept licence screen: `LicenseGetControl::run` is inlined, load/begin/mainLoop are virtual, and `mpeg_course` / `available_car_num_S` are new. Entry 8, do not port. |
| `0x00136A58` | `LicenseGetControl::run` | **Wrong pair.** It is `T059_static_init` (US `0x0013A6E0`: `render_model` → `create_trophy_license`). GT3's `run` has no twin (inlined). Map v2's `T059_static_init` = L2 `0x00164448` is also wrong: that table registers `rank_disp` and `render_model` → `create_trophy_arcade`. |
| `0x00136FF8` | `trophy_get` | Right; noise (exception paths). |
| `0x0013B710` | `MemorycardControl::fileOperation` | Right (my survey's "game_init" was the old map). It opens Concept's page `file_operation_lupo` and loads the kanji font (`font/system.hf/tf/kf`) first. Concept UI: entry 13, do not port. |
| `0x0013C170`, `0x0013C1E8` | `MemorycardControl::setSort/setEffectiveFilter` | Right; noise. The virtual slots are 29/31 in L2 against GT3's 27/29 (two slots inserted at 12/13). |
| `0x0015D150` | `RecordsButton::RecordsButton` | Right; noise (object 8 bytes larger: `Button` grew). |
| `0x00165FD0` | `CarGetControl::run` | **Wrong pair.** It is `CarGetControl::end` (US `0x0013ED68`, the same five calls). GT3's `run` has no twin. |
| `0x00166018` | `car_get_event_name` | Right; noise (23 reasons; the names come from a table instead of literals). |
| `0x001690A8` | `MemorycardPS2File::writeIcons` | **Wrong pair.** `writeIcons` = L2 `0x00168F50` (the file I/O). `0x001690A8` is a new helper that builds the `icon%d.ico` names, split out of it; behaviour unchanged. |
| `0x00169860`, `0x0016AC20`, `0x0016AF90`, `0x0016B330`, `0x0016C2E8`, `0x0016C540`, `0x0016CF18` | `MemorycardDummyFile` ctor, 3 × `getInfoName`, `setPort`, `findIndexByOldId`, `findIndexById` | Right; noise. |
| `0x001712B8` | `CarsItem::setupLayout` | Right; noise. |
| `0x001B8B18` | `GtmodeControl::runRootPage` | **Wrong pair.** The static initialiser of a movie table (`stream/opening.pss`, `opening_GT3UK`, `ending_long_full`, `title_full`, `demoGT3A`, 640 × 448): app flow / menus A. `GtmodeControl` is removed in Lupo 2. |
| `0x001B8EA0` | `T203_static_init` | Right; noise. |
| `0x0021E728`, `0x0021E7B8`, `0x0021EA08`, `0x0021F3D8`, `0x0021FD48`, `0x00220148`, `0x00222BE8`, `0x00222E38`, `0x002232A8`, `0x00223638`, `0x00224A08` | record-table functions (`MachineTestTable`, `FreeRunTable`, `TimeTrialCourse`, `LicenseTest`, `LicenseRecords`, `ArcadeRecord`) | Right. Entry sizes changed (e.g. `MachineTestTable` 240 → 328 bytes, the licence stride): the Concept save layout, entry 9. Do not port. |
| `0x00223A58` | `LicenseRecords::getSize` | Uncertain (?). It sits in Lupo 2's second licence-records block (see the next row); possibly that block's `getSize`. |
| `0x00223C88`, `0x00223CB8`, `0x00223CF0`, `0x00223D20`, `0x00223D58`, `0x00223D88` | `PrizeSetFlags::load/save`, `ArcadeRecords::getPlace/addTime/getTime/getSize` | **Wrong pairs.** Each calls one `LicenseTest` method: they belong to Lupo 2's second licence-records block (Concept licences, `GameStatus+0x23510`, entry 9). The block's load and save are also wrongly paired (map v1/v2): L2 `0x00223E98` = "`CourseBestTimes::load`" and L2 `0x00223EF0` = "`ArcadeRecords::save`" load and save it through the 340-byte loops `0x00223028`/`0x00223118`. The right twins: `ArcadeRecords::getSize` = L2 `0x00225330`; `ArcadeRecords::save` = L2 `0x002253A0` (it saves the arcade block `+0x24900`). `PrizeSetFlags::load/save`: not found. |
| `0x00225330` | `FreeRunRecords::size` | **Wrong pair.** It is `ArcadeRecords::getSize` (calls `ArcadeRecord::getSize`). The free-run record wrappers are `size` = L2 `0x0021FC80`, `load` = `0x0021FCB8` and `save` = `0x0021FD00`; maps v1/v2 have the last three as `MachineTestRecords::size/load/save`, which is wrong. `MachineTestRecords::size/load/save` have no Lupo 2 twin. |
| `0x0022B838` | `GameOptions::reset` | **Wrong pair** (8 insns; GT3's `reset` = `setDefaults` + `apply`); twin not found. |
| `0x0022C3E8` | `ProductConfig::F_0021D6F8` | **Wrong pair by one.** It reads demo record `+8` = `F_0021D718`. `F_0021D6F8` = L2 `0x0022C3C8`; `demoCount` = L2 `0x0022C398`. |
| `0x00243308`, `0x002450D0`, `0x00245100` | `getMakerDisplayName`, `getMakerName`, `getBrandName` | Right; code noise (the tables changed: entry 16). |
| `0x00247B18`, `0x00247B38` | `T488_static_fini/init` | Right; noise (Lupo 2's `$D`/`$I` stubs around `0x00247AB8`). |
| `0x00253F80` | `UnitSettings::UnitSettings` | Right; noise. |

### Functions new to the list (67)

- **GT3 twins, no change of their own:**
  - `0x00140CC8` = `MemorycardLayerMenu::setPriority`
  - `0x00141BC8` = `memorycard_set_confirm_center` (Lupo 2 rounds the centre to whole pixels: UI noise)
  - `0x001422E0` = `MemorycardAutoLoad::MemorycardAutoLoad`
  - `0x0015EFF8` = `PrizeCounter::start`
  - `0x0016AC30`, `0x0016B340` = `getIconFiles` of the replay and setting files
  - `0x0016AFA0` = `MemorycardBestlapReplayFile::getIconFiles` (picks one of two icon sets through the flag `0x0016AF00`: the GTC / Lupo Cup files of entry 13)
  - `0x00170D30` = `TheCar::updateAlpha`
  - `0x0021ED00`/`0x0021EDC0` = `MachineTestTable::load/save` and `0x0021F808`/`0x0021F8C8` = `FreeRunTable::load/save` (the `memcpy` of 768 bytes inlined)
  - `0x00220B08` = `BestPlace::save`
  - `0x002236D0` = `LicenseRecords::insertCode`
  - `0x002261C8`/`0x002262A8` = `ControllerConfig::load/save` (0x250 bytes, as GT3)
  - `0x0022C398` = `ProductConfig::demoCount`
  - `0x0022C3C8` = `ProductConfig::F_0021D6F8`
- **Concept-only, do not port:**
  - entry 3: `0x00150500` (registers `menu_map_custom`/`menu_map_normal`)
  - entry 8: `0x00135120` (the premium car's model spec); `0x00165018`, `0x00165160`, `0x00165AC0`, `0x00165C08`, `0x00166468`, `0x001665D0` (concept car-get and message-item helpers); `0x002211C0`, `0x002223D0`, `0x00223FC0` (present requirements, present set by name `F`..`I`)
  - entry 8 (?): `0x00220B38`, `0x00220BB0`, `0x00220C48`, `0x00220C58`, `0x00220CA8`, `0x00220D68`, `0x00220D78` (present-list helpers, called by `car_get_prizes`, `run_arcade_race` and the concept car-get)
  - entry 9: `0x00224220`, `0x00224228`, `0x00224308`, `0x002243E8` (the presents block: size, load, save, init); `0x00223028`, `0x00223118` (the second licence block's 340-byte load and save loops); `0x00223E68`, `0x00225328` (the constant 20, used by `getCompletionRatio`; GT3's counts are 10 courses and 48 tests)
  - entry 13: `0x0013E3D8` (registers `input_text` in a memory-card hook table; GT3 registers it only in T097)
  - entry 16: `0x00240698` (region test inside `CarDataBase::isEurope`)
- **Noise:**
  - `0x0013E430` (a 4-field setter used by a `MemorycardSlot` helper)
  - `0x00141BB0` (6 insns, called by the operation-menu constructor; twin not found)
  - `0x0016D7F8` (a step outlined from `buildSortedIndex`)
  - `0x00170938`, `0x00172848` (empty stubs)
  - `0x00224918` (set-up called by the `MenuStatus` constructor)
  - `0x0022B6D8`, `0x0022B6F8` (stubs with no callers)
  - `0x00246DF8`, `0x00247188`, `0x002475C8`, `0x00253AB0`, `0x00253C38`, `0x00253FE0` (static init/destroy wrappers of the global `CarColorDataBase`, `RaceModeDatabase`, `Country` and `UnitSettings`)
- **Belongs elsewhere:**
  - `0x001B1DF0`: HUD, a helper of `ExamMessageLine::draw`.
  - `0x0021E400`: a helper called from arcade and app code, offsets 0/160 (?): menus A.
  - `0x0022BDB0`: car lookup in the GCF car section, used by `main_arcade` and `ArcadeControl::getPlayerClassName`: menus A.
  - `0x00254078`: sets the units from the country (`UnitSettings::setPreset`), called by the country set-up `0x001293D0`: app flow.
  - `0x00255E48`, `0x00255EC0`: the bike rider (`pdiRiderman`): rendering / car model.
