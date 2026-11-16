# Survey: rendering and effects (game side)

Area: `src/game/car/model/`, `src/game/effects/`, `src/game/course/` (car and
tyre models, smoke, flares, particles, blur, environment maps, course data
and environment), plus the engine pieces a game feature here needs.
Lupo 2 addresses are `L2 0x...`, GT3 US addresses `US 0x...`.

## Summary

Candidates examined: all 340 paired functions of the three folders (189
car/model, 90 effects, 61 course: every pair through `cmp`, ~60 of them side
by side with `dis` / `dis --us`) and all 147 unpaired Lupo 2 functions placed
here (96 + 8 + 43), each identified or traced to its callers.

Real changes, in order of what they are worth to GT3:

1. **Car-model texture animation for every car** (the base of the Pod's face).
   Lupo 2 advances every car model's CLUT animation records each frame
   (`RaceCarModel::update` L2 0x001A8938 → `CarModel::animate` L2 0x0022DAE8),
   applies them once per frame for all entries (`RacePS2Base::virtual_3`
   L2 0x0017BA68 → vehicle slot 3 → `CarModel::reset` L2 0x0022DB18) and does
   the same for the menu car (`gt2k_menu::Car::virtual_1` L2 0x001BE3B0). The
   select index (`CarModel` +0x3C, init -1 = all records) and the engine
   rebind that resets only the selected id are part of it. **Port.**
2. **`CarModel` grew 0x1C bytes of render inputs and the SWITCH query answers
   ids 6..10** (L2 0x0022D9E8): 6 = active-wing deploy, 7 = Pod tail motion,
   8 = Pod colour category, 9 = Pod state, 10 = Pod face select. **Port.**
3. **Active rear wing** on the concept cars tagged special kind 2 / 8 / 9 /
   10 (GT-R concept, Crossfire, Cien, W12): a speed-driven deploy fraction
   (physics side, `car+0x5F0`) fed every frame into `CarModel` +0x24 and
   shown through SWITCH id 6. Visual only. **Port with those cars** (needs
   the special-kind byte and the physics updater; assets: the Lupo 2 models).
4. **Blinking lamps**: lamp records get a blink pattern (+0x6C) and phase
   (+0x70); two patterns (0.4 s single pulse; 0.9 s burst of five 0.05 s
   flashes) driven by two phase accumulators per car. Changes `addLamps` /
   `drawLampReflections` and `updateBodyState`. **Port** (data-driven, inert
   for models whose lamp records carry 0).
5. **Pod emotion drive of the model** (ids 7..10, select from the emotion
   machine, gated on `isPod`) — what GT3Hooks `Pod.c` does; the gaps of
   `Pod.c` against Lupo 2 are listed in their own section below. **Port**
   (with the physics-side emotion machine).
6. **Projected car shadow** (new): the car (body silhouette + tyres) is
   rendered from the light into a 256x256 buffer and projected either onto a
   course "shadow receiver" model (package +0xAC) or onto a ground quad.
   Used when the course environment sets flag 0x800000, and always for bikes.
   Needs new per-vehicle bounding boxes (the opcode-0x0A model reader
   L2 0x002336D0 — so that reader *is* needed, not for the Pod but for this).
   **Port later, with Lupo 2 courses** (inert on GT3 courses unless the flag
   is set).
7. **Falling leaves** (new `LeafParticle`, new polymorphic particle pool):
   when a wheel rolls on a road polygon with attribute bit 0x20 the tyre
   kicks up a tumbling textured leaf (8 leaf types, textures 1..8 of
   `race/particle.img`). Replaces GT3's dead `emitDebris`. **Port with Lupo 2
   courses and `race/particle.img`.**
8. **Course data package and environment extended**: new pointers (+0x44
   model with visions +0x48..+0x50 and a third reflection pass, +0xA4 / +0xA8
   course camera sets, +0xAC shadow receiver, a model per gadget object);
   `CourseEnv` grew 0xF50 → 0x1794 bytes; the animate driver ticks a fifth
   model (+0x34). **Port the fifth animated slot now; port the loader
   extension only with a format check, for Lupo 2 courses.**
9. **Tyre mesh fix**: the inside face of an ordinary (non-F1) procedural
   tyre also gets its shoulder strip (`TirePacket::virtual_4`). **Port** (one
   call).
10. **Flares: fog off while the flare buffers are set up**
    (`FlareClass::begin`, one `gfx_disable(PGL_CAP_FOG)`). **Port** (trivial).
11. **Bikes and riders** (`VehicleModel` interface with 36 virtuals,
    `RaceBikeModel`, `RaceBikerModel`, `HumanModel`, `pdiRiderman` bones,
    `bike/%s.mdl`, `bike/front_tire.mdl`, `bike/rear_tire.mdl`), with the
    generalisations they forced (variable wheel count in the tyre smoke /
    showroom accessors, sparks from the vehicle's ground frame). **Do not
    port** unless the physics area ports bike dynamics.
12. **Race end no longer switches the course lights off** (found in the
    delta review): `course_env_lights_off` is an empty function in Lupo 2.
    **Removed in Lupo 2: do not port** (GT3 keeps its clean-up).

Not changes (beyond compiler noise): the `RaceCarModel` virtualisation (36
slots; almost every slot is a GT3 function made virtual), the
`CarRenderCtx` shadow getters, the env-map size parameters, and a long list
of wrong pairs and codegen differences (end of the file).

Corrections to earlier notes: `POD_PORT_SPEC.md` §8 #9 calls L2 0x00234B90 /
0x00234CF0 "Lupo-2-only relocators" — they are GT3's `carmodelinfo_relocate`
/ `carmodelfile_relocate`, instruction for instruction (codegen only). §4.3 /
FINDINGS §12 rate the opcode-0x0A bbox reader "not needed": true for the Pod,
but it is the projected shadow's input (item 6).

---

## Changes

### 1. CLUT animation of every car model, in race and in menus

| Field | |
|---|---|
| What | Every car model's animated textures run: Lupo 2 advances the model's animation records each frame for every car (select -1 = every record unless something selects an id), applies the dirty records once per frame before drawing, and does both for the car shown in menus. GT3 never animates a car body model (only the course driver calls `Model::animate`). |
| Where | L2 `RaceCarModel::update` 0x001A8938 (tail: `CarModel::animate` 0x0022DAE8 at 0x001A8A7C, reached for every car — the `isPod` branch at 0x001A8A00 jumps to it); L2 `RacePS2Base::virtual_3` 0x0017BA68 (per entry: vehicle slot 3 0x001A8B00 → `CarModel::reset` 0x0022DB18 = `modelset_apply_clut_anims`); L2 `gt2k_menu::Car::virtual_1` 0x001BE3B0 (animate + reset every frame; at a camera reset `setAnimSelect(1)` then `setAnimSelect(0)`). GT3: `RaceCarModel::updateBodyState` US 0x001AB0F8 (T180), `RacePS2Base::virtual_3` US 0x0017DBE8 (T130, race/core), `gt2k_menu::Car::virtual_1` US 0x001B8EC8 (T212, menu/framework); new `CarModel::animate` / `reset` / `setAnimSelect` in T446. |
| Kind | changed logic + new functions |
| Evidence | `CarModel::animate`: `lw model,0x40(cm)`; `lwc1 $f12,-0x6C88($gp)` (= 1/60) ; `jal 0x00232320` (US `modelset_step_anims` 0x002249F8) with `$a1 = cm+0x3C`. `setAnimSelect` 0x0022DC90: on change `jal 0x002321D8` (US `modelset_reset_anims` 0x002248D0) with the new id, store +0x3C. `Callback` ctor 0x0022D900 stores -1 at +0x3C. `RacePS2Base::virtual_3`: GT3 only calls `course_apply_clut_anims`; Lupo 2 also loops the entries calling vtable entry 0x20 of `RaceEntryCar+0x1E3C`. |
| Engine change it needs | `modelset_reset_anims` (src/engine/gfx/T451_model_set.cpp, US 0x002248D0 / L2 0x002321D8) takes a select id and skips `animset_reset` when it is >= 0; `clut_anim_reset_all` (src/engine/pgl/tex1.cpp, US 0x00249518 / L2 0x00264720) takes it too, resets only the records with that id (all when < 0), marks them dirty and **does not apply** (GT3 calls `clut_anim_channel_apply` per record). The course callers pass -1. |
| Port | `CarModel` select field (init -1) and the three wrappers; one `animate` call in `updateBodyState`; the apply loop in `RacePS2Base::virtual_3` (race/core); animate/apply in the menu car (menu/framework); the two engine functions. No data. Assets: none (Lupo 2 models bring the records). Risk to check: any GT3 car model that carries CLUT animation records would start animating — scan the GT3 car models' Tex1 anim blocks first. |
| Verdict | **port** |

### 2. `CarModel` render inputs (+0x24..+0x3C) and SWITCH ids 6..10

| Field | |
|---|---|
| What | The model's SWITCH query answers five more ids: 6 = morph weight `+0x24` (active wing), arm 0; 7 = if `+0x28 != 0.5` morph `+0x28`, arm 0, else morph `+0x2C`, arm 1 (Pod tail lean / bob); 8 = morph `+0x34`, arm `+0x30` (Pod colour category cross-fade); 9 = arm `+0x38` (Pod state); 10 = arm `+0x3C` (face select). GT3 answers 0..5 and 0 for the rest. |
| Where | L2 `CarModel::Callback::selectSwitch` 0x0022D9E8 (11-entry jump table 0x0034A200) vs US 0x0021EC90 (T446); setters L2 0x0022DC60 (+0x24), 0x0022DC68 (+0x28/+0x2C), 0x0022DC78 (+0x30/+0x34), 0x0022DC88 (+0x38); ctor defaults L2 0x0022D900: +0x24 0, +0x28 0.5, +0x2C 0.5, +0x30 0, +0x34 0, +0x38 0, +0x3C -1. |
| Kind | changed logic, new fields |
| Evidence | `sltiu $v0,$a1,0xB` vs GT3 `slti 5`; every field above the insert moved by +0x1C (model +0x24→+0x40, dirt +0x30→+0x4C, lamps +0x34→+0x50: `setModel`, `clearModel`, `setDirt`, `beginFade`, `draw`, `F_0021F010/2E0/328` differ only by that shift). |
| Port | Seven words in `CarModel` (GT3's `CarModel` has no spare word: +0x20 steer, +0x28/+0x2C used). Growing it moves `RaceCarModel` (CarModel embedded at +0x04) and everything after it: check every hard-coded `RaceCarModel` offset in race/core (`RaceEntryCar` +0x11F8 ...). Non-Pod cars keep the defaults, so ids 7..10 on a non-Pod answer arm 1 / arm 0 / 0 / -1 instead of GT3's 0 — only models with those SWITCH ids (Lupo 2 models) see it. |
| Verdict | **port** |

### 3. Active rear wing (special kinds 2 / 8 / 9 / 10)

| Field | |
|---|---|
| What | Four concept cars deploy a rear spoiler with speed; the model shows it through SWITCH id 6. Mode 1 (kinds 2, 8, 10): hysteresis — deploy at or above the up speed, retract below the down speed; kind 2 (GT-R concept) 22.22 / 16.67 m/s (80 / 60 km/h), kind 8 (Crossfire) 31.29 / 29.06 m/s, kind 10 (W12) 33.33 / 22.22 m/s; the fraction moves at 0.5 /s. Mode 2 (kind 9, Cien): fraction linear in speed from 15.65 m/s over 6.705 m/s (≈ 56 → 80 km/h), moving at 2.0 /s. At a reset the wing starts deployed if already above the threshold. |
| Where | Render side (mine): L2 `RaceCarModel::update` 0x001A89E4 `jal 0x001E2110` (fraction, 0 for other cars) → `0x0022DC60` (`CarModel`+0x24), every car, before the Pod gate. Physics side (other area): predicate L2 0x001E1E00 (kind → mode 0/1/2), thresholds 0x001E1E58, ramp 0x001E1EB0, rate 0x001E1EF8, init 0x001E1F30, updater 0x001E1FD0 (called from `DynamicsConductor::update` L2 0x001F1B5C; init from L2 0x001F5530); state `car+0xD8+0x518` (= `car+0x5F0`, the same word the Dualnote uses for its charge — a union) and the deployed byte `car+0x5F4`. |
| Kind | new logic (render input + physics-side updater) |
| Evidence | `lbu 0x32(m10)` = special kind; constants gp-0x74C4..-0x74A4 (16.6667, 22.2222, 29.0568, 31.2919, 22.2222, 33.3333, 15.646, 22.3514, 6.70542); the only reader of `+0x518`/`+0x5F0` besides the hybrid accessors is 0x001E2110 → no aero effect. |
| Port | The special-kind byte (paramDB car-code hash chain, the same one the Pod needs: physics/data area), one float per car, the updater in the race step, the setter call in `updateBodyState`, SWITCH id 6. Assets: the Lupo 2 models of those cars (their SWITCH 6 morph); without them nothing shows. |
| Verdict | **port with those cars** (cheap once item 2 and the special-kind byte exist) |

### 4. Blinking lamps

| Field | |
|---|---|
| What | A lamp in the model's first lamp group (CarModelInfo +0x40 count / +0x44 records of 0x80 bytes) can blink: record +0x6C = pattern (1 or 2; 0 = steady; bit 0x80 = always lit), +0x70 = phase offset. Brightness of a pulse = 1 − (2t/on − 1)². Pattern 1: period 0.4 s, 1 pulse, on 0.25 s. Pattern 2: period 0.9 s, 5 pulses 0.05 s apart, on 0.05 s (a strobe burst). The flare and its reflection follow. |
| Where | L2 `setLampFlare` 0x001A8B20 (= US `RaceCarModel::addLamps` 0x001AB1B8) and `setReflectionLampFlare` 0x001A8DA0 (= US `drawLampReflections` 0x001AB370); blink curve L2 0x001A8FB0; table L2 0x001A9090 / init 0x001A9048 (static at 0x004A8F28, 2 x 16 bytes); phases advanced in `update` L2 0x001A8A84..0x001A8ADC at `RaceCarModel`+0x6DC / +0x6E0 (dt gp-0x79D0 = 1/60, wrapped at the period). |
| Kind | changed logic + new data table |
| Evidence | GT3 adds every first-group lamp at 1.0; Lupo 2 reads `lw 0x6C(rec)`, `lwc1 0x70(rec)`, `jal 0x001A8FB0`, `lwc1 0x6CC(this+0xC+mode*4)`. Pattern constants gp-0x79C4 0.4, gp-0x79C0 0.9, gp-0x79C8 0.05, literals 0.5 / 0.25. The new bool argument (lights-on) is always 1 (L2 `addCarLamps` 0x0017EB20 `addiu $a2,$zero,1`), so non-blinking lamps behave as GT3. |
| Port | Two floats per `RaceCarModel`, the table, the curve, the per-lamp branch in the two lamp functions. Data: lamp records' +0x6C/+0x70 (verify they are 0 in every GT3 car model, else GT3 lamps would start blinking). Assets: Lupo 2 models with blinking lamps. |
| Verdict | **port** |

### 5. Pod model drive (ids 7..10 and the face select)

| Field | |
|---|---|
| What | For a Pod (`isPod` L2 0x001E1288, special kind 1) the update writes the colour category / blend (id 8, from L2 0x001E17D8), the tail look / bob weights (id 7, from 0x001E18A0), the state (id 9, from 0x001E1950) and the face select (`setAnimSelect` from 0x001E1990). |
| Where | L2 `RaceCarModel::update` 0x001A89F8..0x001A8A6C; GT3 seam `RaceCarModel::updateBodyState` US 0x001AB0F8. |
| Kind | new logic |
| Evidence | FINDINGS §12-13, `docs/pod/POD_PORT_SPEC.md`; re-read here. |
| Port | Items 1-2 plus the emotion machine (physics area: per race step from `DynamicsConductor::update` L2 0x001F18DC → 0x001E1520) and the special-kind byte. Assets: the Pod model (`ty0150` / `pod_tmv_01_day`), `sound/gt3pod.ins` + `se.inf` (sound area). |
| Verdict | **port** (see "GT3Hooks Pod.c against Lupo 2" below for what the plugin left out) |

### 6. Projected car shadow (render-to-texture)

| Field | |
|---|---|
| What | Instead of the flattened-model shadow passes, a car can cast a texture shadow: its body silhouette (model program 1 without the ground pass) and its tyres are rendered from the light direction into a 256x256 buffer cleared to the shadow colour, framed by the body's and tyres' model-view bounding boxes; the texture is then either projected onto the course's shadow-receiver model (package +0xAC, drawn with a depth shift) or drawn as a quad on the ground under the car. Used when `CourseEnv.flags & 0x800000`, and always for a bike (`dynamic_cast` to `RaceBikeModel`, L2 0x00175B50). |
| Where | New. Driver L2 0x0017DA70 (called from Lupo 2's `drawCars` L2 0x0017D5B0 at 0x0017D890; GT3 `RacePS2Base::drawCars` US 0x0017F730, race/core); light-view setup + bounds L2 0x001A3598; render L2 0x001A3828; texture matrix L2 0x001A3A90; ground quad L2 0x001A3AE0; receiver draw L2 0x001AAF68 (course, package +0xAC); course flag query = race vtable entry 0x1F8 → L2 0x0017DBD0 (`env->flags & 0x800000`); shadow direction / colour getters L2 0x001ABE90 / 0x001ABF18 (`CarRenderCtx`, T181) and 0x001A6778 (`CourseEnv` direction); vehicle virtuals: slot 24 body silhouette 0x001A8130 → `CarModel` 0x0022E130 (program 1, no ground pass), slot 25 tyres 0x001A8198 → `RaceTireModel` 0x0022FCD0 → 0x00231590 → `TirePacket` 0x00238458, slot 32 ground plane 0x001A7670, slot 33 body bbox 0x001A7FA8 → opcode-0x0A reader 0x002336D0, slot 34 tyre bbox 0x001A8010 → `TireModelBase` 0x0022E808. |
| Kind | new function / feature |
| Evidence | `GSBuffer(int,int,int)` with 0x100 x 0x100 at 0x0017DAB8; `lui $a2,0x80; and` at 0x0017DBE4; virtual offsets 0xC8/0xD0/0x110/0x118 at 0x001A3948/0x001A3964/0x001A36F8/0x001A3718. |
| Port | ~0x600 bytes of new code in effects + course + car/model, a VRAM budget for the 256x256 target, the 0x0A bbox reader (src/engine/gfx; a VU0 transform input `VF28..VF31` the caller sets), the course flag and the receiver model (Lupo 2 course data). On GT3 courses it stays off unless their env sets bit 23 (check GT3's course env flags first). |
| Verdict | **port later, with Lupo 2 courses** (or bikes) |

### 7. Falling leaves (`LeafParticle`) and the polymorphic particle pool

| Field | |
|---|---|
| What | A wheel rolling on a road polygon whose attribute byte has bit 0x20 kicks up a leaf: a tumbling rigid square (mass 1 g, 0.10..0.14 m; inertia from its size) drawn as a textured quad with one of 8 leaf textures (textures 1..8 of `race/particle.img`), lit side chosen by facing. `ParticleManager` became a pool of the largest of several particle classes (`SparkParticle` = GT3's particle, `LeafParticle`) with virtual update / draw / resetTrail and 8 leaf-type records. GT3's `emitDebris` (coloured debris, never called: `RaceCourse::emitDebris` has no callers in GT3) is replaced by the leaf emitter, now called. |
| Where | L2 `ParticleManager` ctor 0x0019BD38 (vs US 0x0019EEF8, T165): pool size from the size table 0x002DD560 (0x0019B500), 8 leaf types 0x0019C970 at +0xC + i*0x74; emitter 0x0019BFD8 (= US `emitDebris` 0x0019F0D8) → alloc 0x0019C540, leaf init 0x0019CC90; leaf update 0x0019CD68, draw 0x0019D248, maths 0x0019CAD0/0x0019CBE0/0x0019CC38; spark draw 0x0019BAE8 (= US `Particle::draw` 0x0019ECA8), resetTrails 0x0019C758 (= US 0x0019F820). Trigger: L2 per-wheel smoke emitter 0x001F6348 (twin of US `tyre_smoke_emit` 0x001DF150, T334_tyre_smoke.cpp) at 0x001F6448: `lbu 0x33(contact); andi 0x20` → `RaceCourse::emitDebris` 0x0018DFE0. |
| Kind | new class + changed logic |
| Evidence | Lupo-2-only RTTI `12LeafParticle`, `13SparkParticle`; leaf constants gp-0x7B58 0.001, gp-0x7B54 0.1, gp-0x7B50/-0x7B4C/-0x7B48 0.12/0.13/0.14; `gfx_set_texture_regs(tex->+0x18 + idx*0x28)` in 0x0019D248. At the polygon byte (+0x33) GT3 tests bits 0x01 and 0x02 (`car_ground_contact`, `ground_surface_type`) and copies it to the wheel contact, where 0x10 is the pit-lane bit; no reader of 0x20 was found. |
| Port | Leaf class and pool change in T165; one call in `tyre_smoke_emit`. Data: road polygons with bit 0x20 (Lupo 2 courses; check that GT3 course runways never set it). Assets: Lupo 2's `race/particle.img` (GT3's has the spark texture only — verify its texture count). |
| Verdict | **port with Lupo 2 courses and particle image** |

### 8. Course data package and environment

| Field | |
|---|---|
| What | (a) The course animate / reset / apply / release drivers tick a fifth model, package +0x34 (the second reflection model). (b) The package has new pointers: +0x44 a model and +0x48/+0x4C/+0x50 vision lists (a third vision-culled draw, L2 0x001AAE28, called from the race at L2 0x0017E870), +0xA4 / +0xA8 two more course-camera sets, +0xAC the shadow-receiver model (item 6), and every gadget object carries a model at +0xAC. (c) `CourseEnv` grew from 0xF50 to 0x1794 bytes; every field GT3 knows keeps its offset (lights, smoke, flares 0xC60.., car shadow 0xA60..0xA78 — checked through `course_env_set_flare_types`, `set_shadow_dir`, `applyDefault`, `applyShadowPass`); new: flag 0x800000 (projected shadow); the projected shadow's alpha uses +0xA7C. |
| Where | L2 `course_step_anims` 0x001AB098 / `course_reset_anims` 0x001AB030 / `course_apply_clut_anims` 0x001AB128 / `course_release_clut_anims` 0x001AB178 vs US 0x001A9768 / 0x001A9720 / 0x001A97E8 / 0x001A9830 (T179); relocator L2 0x001AA3C8 vs US `course_data_relocate` 0x001A8D18; `course_env_host_dump` L2 0x001A6980 (writes 0x1B94 bytes) vs US 0x001A8BD8 (0x1350). |
| Kind | changed logic (a); new data layout (b, c) |
| Evidence | (a) one more `modelset_*_anims` call on `lw 0x34(cd)` in each driver. (b) the L2 relocator relocates +0x44 (`modelset_relocate`), +0x48/+0x4C/+0x50 (`vision_relocate`), +0xA4/+0xA8 (`CourseCamera::relocate`), +0xAC (`modelset_relocate`), obj+0xAC in the gadget loop; GT3's struct (`include/game/course/T179_course_data.h`) has `unk_3C[0x18]` there and ends at 0xA4. (c) the dump's copy length 0x1780+0x14 vs 0xF40+0x10. |
| Port | (a) four calls; harmless for GT3 courses (their model34 records were never ticked). (b, c) a loader that knows both layouts — needs a way to tell a Lupo 2 package from a GT3 one (none seen in the relocator: it just reads the fields), so the user's course transplants decide it. |
| Verdict | **port (a) now; (b, c) only with Lupo 2 courses, behind a format check** |

### 9. Tyre mesh: inner shoulder on ordinary tyres

| Field | |
|---|---|
| What | The procedural tyre's inside (face + reversed tread) of a non-F1 tyre also gets its shoulder strip. |
| Where | L2 `TirePacket::virtual_4` 0x00238870 vs US 0x00220020 (T462_tire_packet.cpp, the `else` arm). |
| Kind | changed logic |
| Evidence | L2 0x00238994..0x002389AC: after `buildTread(c,0,1,1)` a `buildShoulder(c,0,1,1,0)` GT3 does not have. (The other call-count differences in `virtual_3` / `virtual_4` are tail calls the tool does not count.) |
| Port | One call. No data. |
| Verdict | **port** |

### 10. Flares: fog disabled while the flare buffers are set up

| Field | |
|---|---|
| What | `FlareClass::begin` disables fog before preparing the flare buffers. |
| Where | L2 0x001989A0 vs US `FlareClass::begin` 0x0019B358 (T159). |
| Kind | changed logic (one state call) |
| Evidence | L2 0x00198A04 `jal gfx_disable` with `$a0 = 8` (`PGL_CAP_FOG`); the rest matches (`flare_init` twin L2 0x00262AA0). |
| Port | One call. |
| Verdict | **port** |

### 11. Bikes and riders (`VehicleModel` family)

| Field | |
|---|---|
| What | A vehicle interface `VehicleModel` (36 virtual slots, abstract) with `RaceCarModel` and `RaceBikeModel` under it; `RaceEntryCar` keeps a `VehicleModel*` at +0x1E3C and calls through it (L2 wrappers 0x00175EC0..0x00176230, race/core). The bike draws `bike/%s.mdl`, `bike/front_tire.mdl`, `bike/rear_tire.mdl` (loader L2 0x00231C70, engine/gfx), an articulated fork / swing arm (L2 0x001A9718, many bike-state accessors 0x001AA108..0x001AA3B0 → physics 0x001E23D0), its rider (`RaceBikerModel` 0x001AF7B8.., `HumanModel` 0x001AE860.., models loaded at 0x001AE940 / host 0x001AEA78, posture 0x001AF1B8 / 0x001AF838, skeleton `pdiRiderman` built at L2 0x0025A560), always the projected shadow (item 6). The generalisations it forced: the tyre smoke emitter loops the vehicle's wheel count and pairs a bike's wheels (L2 0x001F6480 / 0x001F6348, kind 3 path), the showroom per-wheel accessors go through the wheel layout (L2 0x001D9AB8.., 0x001D9DF0..0x001DA120, `ShowRoomCar::wheel*`), the sparks use the vehicle's ground frame (slot 32) for their normal and height (L2 0x001A85B0 vs US `emitSparks` 0x001AADD0), the tyre geometry accessors take a wheel index (L2 0x001EB1A8/0x001EB208/0x001EB268). |
| Where | new (RTTI `12VehicleModel`, `13RaceBikeModel`, `14RaceBikerModel`, `10HumanModel`, `Q211pdiRiderman*Bone`). |
| Kind | new classes |
| Evidence | `class RaceCarModel`: GT3 1 slot → Lupo 2 36; `VehicleModel` all `__pure_virtual` (0x002956C0) except three no-ops (`delayLoadDriverModel`, `computeDriverPosture`, `renderDriver`). |
| Port | Bike physics (special kinds 3/4 and the wheel-layout table are physics), the interface, the rider skeleton (engine), assets `bike/*.mdl` and the rider model. |
| Verdict | **do not port** (no bike dynamics or assets in GT3; revisit only if the physics area takes bikes) |

---

## GT3Hooks Pod.c against Lupo 2

`GT3Hooks-main/source/games/gt3/Pod.c` reproduces the Pod's own visual
chain faithfully (switch ids 6..10 for a Pod, the tail blend / colour, the
face select with reset-only-selected-id, apply every frame, dt 1/60, the
squat, the triggers it can reach). Against Lupo 2 it leaves out:

1. **Every other car's texture animation.** Lupo 2 runs `CarModel::animate`
   for every car (not only Pods) and applies every entry's records once per
   frame (`RacePS2Base::virtual_3`); `Pod.c` animates and applies Pods only.
   Lupo 2 models other than the Pod that carry CLUT animation records stay
   frozen under `Pod.c`.
2. **The menu / showroom car.** Lupo 2 animates and applies the menu car
   every frame (`gt2k_menu::Car::virtual_1` L2 0x001BE3B0) and at a camera
   reset stamps id 1 then selects id 0; `Pod.c` hooks only the race update,
   so the Pod's face is never applied in menus.
3. **SWITCH id 6 for non-Pods (active wing).** Lupo 2 answers id 6 for every
   car with the wing fraction and answers 7..10 with the defaults for
   non-Pods; `Pod.c` swaps the query only for Pods (others keep retail's 0).
4. **The blink phases and wing feed** that live in the same update
   (items 3-4) — not Pod features, but the same seam.
5. **Emotion timing.** Lupo 2 steps the emotion machine per race step
   (`DynamicsConductor::update` L2 0x001F18DC → 0x001E1520); `Pod.c` ticks it
   per rendered frame in the model update. Same at one step per frame; it
   may differ while paused or in a replay (uncertain: not run here).
6. **Recognition** by model content (ids 1 and 10) instead of the paramDB
   special kind 1 — known and deliberate; the special-kind byte is also what
   the wing (kinds 2/8/9/10) needs, so the engine port should carry it.
7. The POD race mode, rule engine and HUD — known, not in this area.

---

## For other areas

- **race/core**: `RaceEntryCar` holds a `VehicleModel*` (+0x1E3C) and wraps
  its slots (L2 0x00175EC0..0x00176230); Lupo 2's `drawCars` (L2 0x0017D5B0)
  picks projected or classic shadows per car (item 6) and
  `RacePS2Base::virtual_3` applies every car's CLUT animations (item 1); a race
  method L2 0x0017E870 draws the third course pass (item 8); race vtable entry
  0x1F8 (L2 0x0017DBD0) is the course projected-shadow flag. L2 0x00175598 is
  a `RaceEntryCar` model-copy helper (calls `RaceCarModel::copyFrom` L2
  0x001A6FB8 or the bike's 0x001A91C0 after `__dynamic_cast`), not
  `RaceCarModel::copyFrom`; L2 0x00175660 (placed in car/model) is
  `RaceEntryCar::copyFrom` (twin of US 0x00177488).
- **car/physics**: the active-wing updater (item 3: L2 0x001E1E00..0x001E2140,
  called from `DynamicsConductor::update` and L2 0x001F5530); the Pod
  emotion step (L2 0x001E1520); the special kinds (1 Pod, 2/8/9/10 wing,
  3/4 read at L2 0x001E24F0 / 0x001E2808 — presumably the two-wheelers,
  uncertain); the per-wheel geometry accessors through the wheel layout
  (L2 0x001EAFD8.., 0x001EB1A8..); the tyre smoke emitter moved to L2
  0x001F6480 (it is GT3's `tyre_smoke_emit`, T334, effects folder) with the
  leaf trigger (item 7) and bike pairing.
- **menus**: `gt2k_menu::Car::virtual_1` animates the menu car (item 1);
  `ShowRoomCar::SetupShowRoomCar` (L2 0x001D9908, car/model folder but a
  spec-sheet change) adds the Dualnote motor's power when the drivetrain
  word is 4WD sub-type 5 (L2 0x001E1238 → hybrid initialiser 0x001DFFF0) and
  calls `surface_tables_init(0)`; `ShowRoomCar::power` / `torque` (L2
  0x001D9D28 / 0x001D9D60) dropped GT3's catalogue-value override (GT3
  behaviour stays unless menus decide otherwise); `CarRender::draw` (L2
  0x001CCAE8) can take an explicit environment texture (new `CarRender`
  +0x0C) instead of loading the texture set — only an L2 menu would set it.
- **camera / input**: L2 0x001AE008..0x001AE390 (placed in car/model) are a
  head-tracker look-around for `OnBoardCamera::applyView` (polls L2
  0x00283CA0; classes `5pdiHt`, `7hdx1735`).
- **menu/gtmode (prize)**: L2 0x002380E0 and 0x00237D10 belong to the prize
  model code, not the tyre packet.
- **sound**: `setMuffler__11EngineSound` (linkonce, placed in course).
- **engine/gfx**: `modelset_reset_anims` / `clut_anim_reset_all` select
  argument (item 1 — needed); opcode-0x0A bbox reader L2 0x002336D0 (item 6 —
  needed only for the projected shadow); model-header +0x40 relocation (L2
  0x002321A4 — not needed, nothing reads it); bike model loader L2 0x00231C70.
- **engine/io**: `car_list_load` / `course_table_init` call a `FileDevice`
  virtual instead of `vol_toc_lookup` (backend).

---

## Wrong pairs (tool)

| Lupo 2 | tool says | is |
|---|---|---|
| 0x00175598 | `RaceCarModel::copyFrom` | a `RaceEntryCar` helper (race/core); the real twin is L2 0x001A6FB8 |
| 0x001A6AD0 | `onTireLoaded` | `RaceCarModel::setArena` (US 0x001AB630) |
| 0x001A6B50 | `onWheelLoaded` | `RaceCarModel::operator delete` (US 0x001A9C80: `Allocator::free`) |
| 0x001A6B78 | `initArena` | right twin, calls `setArena` (0x001A6AD0) |
| 0x001A6BF0 | `operator new` | `RaceCarModel::freeBlock` (US 0x001AB668); `operator new` is the unpaired 0x001A6B28 (`allocate`, alignment 16) |
| 0x001A6C98 | `operator delete` | the arena's static-initialiser wrapper (calls 0x001A6C28 with 1, 0xFFFF) |
| 0x00238E78 / 0x00239280 | `buildSidewall` / `buildShoulder` | right twins (the call-list difference was tail calls) |
| 0x002C8BB8 | `F_00179460` | `RaceBikeModel` slot 4 inline (8 bytes) |
| 0x002C8AD8 | `course_set_region` | `VehicleModel::delayLoadDriverModel` (`jr $ra; nop`); `course_set_region` stores the region (`sw $a0,-0x57B8($gp)`), a wrong pair |
| 0x002C8AE0 / 0x002C8AE8 | `RaceCarModel::F_001AA568` / `F_001AAD70` | `computeDriverPosture` / `renderDriver` (`jr $ra; nop`); GT3's two empty stubs in T180 sit at the same places, so plausibly the same empty driver hooks |

Unpaired Lupo 2 functions that are GT3 functions (the tool missed the twin
because they became virtual or moved): L2 0x001A6CD8 / 0x001A6CF8 /
0x001A6D18 = `onModelLoaded` / `onTireLoaded` / `onWheelLoaded`; 0x001A6D38 =
ctor (US 0x001A9CA8); 0x001A6FB8 = `copyFrom`; 0x001A70D8 = `load`;
0x001A7150 = `setData`; 0x001A71B0 = `loadTire` (now `overrideTire`);
0x001A7240 = `setTireData`; 0x001A7278 = `loadWheel` (`overrideWheel`);
0x001A72E0 = `setWheelData`; 0x001A7318 = `setupTires` (`initTire`, per-wheel
sizes through the wheel layout); 0x001A7478..0x001A7498 = the small accessors;
0x001A74A8 = `loadBodyMatrix`; 0x001A7568 = `setReflectionPlane`; 0x001A7788
= `loadGroundMatrix`; 0x001A7848 = `drawShadow`; 0x001A78A8 = `F_001AA680`;
0x001A78F8 = `F_001AA6D0`; 0x001A7948 = `drawBody`; 0x001A79F0 = `drawTires`;
0x001A7CD8 = `drawTireCylinders`; 0x001A7E30 = `drawMenuTires`; 0x001A82B8 =
`F_001AAD78`; 0x001A85B0 = `emitSparks` (item 11); 0x001A8938 =
`updateBodyState` (items 1-5); 0x001A8B20 / 0x001A8DA0 = `addLamps` /
`drawLampReflections` (item 4); 0x001A6B08 / 0x001A6BD0 = `releaseArena` /
`allocBlock`; 0x001A6B28 = `operator new`; 0x001A6C28 / 0x001A6CB8 = arena
static init-fini body / fini wrapper; 0x0022DFA8
= `CarModel::drawShadow`; 0x0022E0B0 = `CarModel::F_0021F258`; 0x0022DBF0 /
0x0022DC30 / 0x0022DC50 = `pushLamps` / `setWheelShake` / `setSteer`;
0x0022E928 = a `TStripBuilder` ctor variant; 0x002382F0 =
`tire_packet_pass_callback`; 0x00231688 = T449 static init / fini body;
0x001AB4E0 = `CarRenderCtx::setup` (US 0x001AB7D0; addressing style only, same
loops: 9 candidates, 4 lights, 6 glows, 4 wheels); 0x001AA3C8 =
`course_data_relocate` (item 8); 0x001A8DA0 (placed in course) is car/model.

---

## Noise (looked at, no change)

Recurring causes, so a later reader can skip them: virtual calls coded as
`addiu vptr,off; lh 0; lw 4` in Lupo 2 vs `lh off; lw off+4` in GT3 (shows as
`vslots indirect` + offsets 0/4); tail calls (`j`) the tool does not count as
calls; leaf functions without frame info cut short at their first `jr $ra`
(L2 sizes too small, e.g. `runway_find_interval`, `runway_marker_value`);
GT3 `__throw` vs Lupo 2's unpaired throw helper L2 0x002968A0; deleting
destructors without a direct `operator delete`; `__tf*` typeinfo functions;
pointer-increment vs displacement addressing; float literals one ulp apart;
`-FLT_MAX` (GT3) vs `0xFFFFFFFF` (PS2 FPU max) sentinels; the +0x1C `CarModel`
shift and the car / race-entry layouts.

- car/model: `ShowRoomCar::wheelAxleX`, `wheelHeight`, `wheelLateralPos`,
  `wheelSteerAngle`, `wheelCamber` (wheel-layout generalisation, item 11);
  `ShowRoomCar::SetupShowRoomCarByName`, `driveName`, `setupDynamics`;
  `__tf12RaceCarModel` (now `__rtti_si`: derived class); `RaceCarModel::release`,
  `~RaceCarModel`, `getTire`; `CarModel::CarModel`, `~CarModel`,
  `Callback::Callback` (new field defaults: item 2), `Callback::~Callback`,
  `selectLOD`, `setModel`, `clearModel`, `setDirt`, `setLamps`, `beginFade`,
  `endFade`, `draw` (engine twins L2 0x00263BB8/0x00263D50 unpaired),
  `F_0021EFC8`, `F_0021F010`, `F_0021F2E0`, `F_0021F328`, `beginGroundPass`,
  `endGroundPass`, `car_shadow_set_dir`, `car_shadow_set_color`,
  `CarModelLamps::fill`, `fraction`, `car_model_load`, `car_model_path`,
  `car_list_find`, `car_list_name`, `car_list_load` (FileDevice, backend);
  `carmodelinfo_relocate`, `carmodelfile_relocate`, `carmodelfile_work_area`;
  `TireModelBase` ctor / dtor / `setSize` / `setSizeProfile` / `clipTest` /
  `drawWheel` / `setVisible` / `virtual_1` / `isF1Tire` and accessors;
  `tire_file_*`, `wheel_file_*`; `TStripBuilder` (all: ctor, dtor, `begin`,
  `beginMeasure`, `endMeasure`, `setShader`, `setPrim`, `addStrip`,
  `addVertex`, `endStrip`, `flush`, `finish`); `RaceTireModel` ctor / dtor /
  `build` / `selectLOD` / `screenSize` / `draw` (tail calls, same shadow and
  cylinder logic) / `drawCylinder` (new depth-test argument, every caller
  passes 1 = GT3) / `setCaliperTexture` and the inline accessors
  (`needTireMotionBlur`, `getCaliperData`, `setDiskTexture`,
  `setHeatTexture`, `getClutID`: out-of-line copies of logic GT3 inlines);
  `BrakeFile::*`, `brake_file_*`, `brake_textures_link/unlink`;
  `RaceTirePacket` (all, incl. `drawTreadBlur`, `drawWheelTexture`,
  `setDisc/CaliperVertices`, `build*`, `Callback`); `MenuTireModel` ctor /
  dtor / `build` / `draw` / `virtual_1`; `TirePacket` ctor / dtor / `setup` /
  `draw` / `drawShadow` / `bindPassCallback` / `virtual_1` / `virtual_2` /
  `virtual_3` / `virtual_5` / `build*` / `makeCircle` / `bevelRatio`;
  `CarRender` ctor / `setColor` / `drawModel` / `hiddenWheel` /
  `beginFrame` / `endFrame` / `setLights` (CarRender +4 bytes: the env-tex
  field noted under menus); `CarRenderCtx::apply` / `applyShadowPass` /
  `applyDefault` / `drawGlows`, `car_glow_draw`; `CarIconMaker` (all: the
  icon now draws through the vehicle virtuals), `CarIcon::CarIcon`,
  `car_icons_make`; T449 static ctor / dtor.
- effects: `RaceSmoke` (all 25 pairs: emit, update, reset, draw, render,
  edgeCross, toStrip, sortByDepth, the quads, color, findWeakest, ...);
  `FlareClass` (all but `begin`: ctor, dtor, project, addSource, addLight,
  setFlareType, setLightParams, spotFactor, directionalFactor, setSun,
  drawSunDisc, query, update, flush, draw, reflections; `FlareGroup::submit`,
  `FlareSourceData::submit`); `ReplayBlur` (all), `race_draw_vignette`,
  `Blur2D` (all); `envmapInit`, `envmapDrawScene`, `envmapRender` /
  `envmapUpdate` (work-buffer size now passed as 0x80 x 0x100 by both
  callers = GT3's constants), `F_001D8280`; `DynamicEnvmapCamera` (all);
  `Particle::init`, `particle_draw_line`, `particle_draw_streak`,
  `ParticleManager::add`, `emitSparks`, `release`, `~ParticleManager`
  (behaviour unchanged; the pool / class change is item 7); `ParticleManager`
  `alloc`, `clear`, `update`, `draw`, `Particle::update` (restructured for the
  polymorphic pool, item 7).
- course: `course_env_*` (apply, envmap, fog, lights, car lights, body light,
  sun, sun disc, shadow dir (split into a getter), lamps, flare types,
  reflection, host load), `angles_to_dir`, `dir_to_angles`, `F_001A8208`,
  `F_001A8360`, `F_001A85A8`, `F_001A8618`, `F_001A8678`; `course_draw`,
  `course_draw_parts`, `course_draw_model`, `course_draw_model54`,
  `F_001A9240`, `course_draw_reflection_a/_b`, `course_submit_flares`,
  `F_001A96B8`, `F_001A9178`, `F_001A9180`, `course_table_init` (FileDevice),
  `course_find`, `course_file_name`, `course_path`, `course_read`; all
  `runway_*` (22) and `vision_nearest`.

## Corrections (after the pairs check, `build/scratch/survey_rendering/pairs_fix.csv`)

- L2 `0x002C8AE0` / `0x002C8AE8` (`computeDriverPosture` / `renderDriver`) are new (rider code),
  not GT3's empty stubs `F_001AA568` / `F_001AAD70`; those stubs' twins are L2 `0x001A7780` /
  `0x001A82B0` (`RaceCarModel` vtable slots 14 and 27, T180).
- L2 `0x002C8BB8` is `setBodyColor`, a bike-only no-op (new).
- L2 `0x00175598` is a helper GT3 inlines into `RaceEntryCar::copyFrom`; the `copyFrom` twin is
  L2 `0x001A6FB8`.
- The `ShowRoomCar` per-wheel functions are not new: they are GT3's `strokeMin`, `strokeMax` and
  the camber-gain, swing-arm, track-offset and bump-travel getters / setters, rewritten per wheel
  (matched by offsets and constants). GT3's `steerLock`, `maxSpeed` and the caster getter / setter
  have no twin in that range.

## Delta review (map v2)

The 13 lines of `build/scratch/delta/rendering.txt`, one by one. One real
change (D2, full entry below); the rest are twins the old map lacked,
compiler-generated helpers, or bike code.

| # | Candidate | Verdict |
|---|---|---|
| D1 | pair L2 `0x00234C50` = US `0x00227220` `carmodelinfo_tire` | **noise**: the same 11-way jump table on `(kind - 0x38)`, the same `+0x70` / `+0x3C` reads and the `movn` pick of entry +8; addressing only |
| D2 | L2 `0x001A5F30` (2 insns, `jr $ra; nop`) | **real change, removed in Lupo 2**: twin of US `0x001A80F0` `course_env_lights_off`, which GT3 fills with four `gfx_disable(PGL_CAP_LIGHT0 + i)`; Lupo 2 calls the same slot from the same place (`RacePS2Base::virtual_35` L2 `0x0017B620` = US `0x0017D808`) and it does nothing. Entry D2 below |
| D3 | L2 `0x001A6C18` (3 insns) | **noise (new helper)**: returns `0xA8000`, the race-car model block size, for `RaceCarModel::copyFrom` (L2 `0x001A7030`); GT3 uses the constant inline |
| D4 | L2 `0x001A7478` (2 insns) | **noise (new accessor)**: `RaceCarModel` vtable slot 7 `setLODBase(f32)`, stores `CarModel` +0x0C (the LOD distance, default 220.0); GT3 writes that field inline (T131) |
| D5 | L2 `0x001A7480` (2 insns) | **twin**: slot 4 `setBodyColor` = US `0x001AB720` `RaceCarModel::setColorIndex` (+0x6D0 / +0x6B0) |
| D6 | L2 `0x001A7488` (2 insns) | **twin**: slot 5 `getBodyColor` = US `0x001AB718` `getColorIndex` |
| D7 | L2 `0x001A7490` (2 insns) | **twin**: slot 6 `setLOD` = US `0x001AB6E8` `setModelLod` (`CarModel` +0x08; GT3 goes through `getCarModel`) |
| D8 | L2 `0x001A7498` (3 insns) | **twin**: slot 8 = US `0x001AB6B8` `getInfo` (`[[this+8]+4]`, GT3 `[[this+0]+4]`) |
| D9 | L2 `0x001AA0F8` (3 insns) | **new code (bike)**: returns 0.0; called from L2 `0x001B15C8` in the rider posture code (with `0x001B1908`, `RaceBikeModel` slot 15's callee) |
| D10 | L2 `0x001D9DA8` (7 insns) | **twin**: US `0x001D9778` `ShowRoomCar::maxSpeed` (speed × 3.6; car field +0x11F4 / +0x11DC). Corrects the note above that `maxSpeed` has no twin |
| D11 | L2 `0x001D9DC8` (2 insns) | **belongs elsewhere (menus), new**: returns 0; with D12 called by `gt2k_menu::CircleGauge::setKind` (L2 `0x001176D0` = US `0x00116048`), which in GT3 asks only `maxSpeed`; a gauge range value that is always 0 (not the caster getter) |
| D12 | L2 `0x001D9DD0` (2 insns) | **belongs elsewhere (menus), new**: the same as D11 |
| D13 | L2 `0x002315F8` (5 insns) | **twin**: US `0x00223DB0` `RaceTirePacket::Callback::Callback` (vptr, +4 = 0); called by `RaceTirePacket::RaceTirePacket` |

Also established while checking D2 (rows added to the pairs file): L2
`0x001A5F20` = US `0x001A80E0` `F_001A80E0` (the env block's empty relocation,
called from the course relocator), L2 `0x001A5F28` = US `0x001A80E8`
`F_001A80E8` (empty, `RaceBase::virtual_8`).

### D2. Race end no longer switches the course lights off

| Field | |
|---|---|
| What | When a race ends, GT3 disables the four GL lights the course environment had enabled (`course_env_lights_off`). In Lupo 2 the function is empty, so the lights stay enabled after the race until something else sets them (the menus set their own lighting). |
| Where | L2 `0x001A5F30` = US `0x001A80F0` `course_env_lights_off` (`src/game/course/T178_course_env.cpp`); caller `RacePS2Base::virtual_35` L2 `0x0017B5F8` = US `0x0017D7E0` (T130). |
| Kind | removed in Lupo 2 (not ported) |
| Evidence | L2 `0x001A5F30`: `jr $ra; nop`. The call stays: L2 `0x0017B620 jal 0x001A5F30` with `$a0 = env` (`lw 0x80` of the course data), as US `0x0017D808 jal course_env_lights_off`. The rest of `virtual_35` is the same (the play-list copy back into the options moved with the layouts). |
| Port | — |
| Verdict | **do not port** (GT3 keeps its clean-up; no visible feature) |
