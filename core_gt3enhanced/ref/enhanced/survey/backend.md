# Survey: backend folders (engine, SDK, runtime)

Area: `src/engine/`, `src/sdk/`, `src/runtime/` (graphics drivers, IOP /
file / pad / memory card / CD plumbing, the C / C++ runtime), checked for
game logic that hides there or that a game feature needs. Lupo 2 addresses
are `L2 0x...`, GT3 US addresses `US 0x...`. Scratch:
`build/scratch/survey_backend/` (`look.py`, `range.py`, `uscallers.py`,
`irxdis.py`, the tool outputs, `pairs_fix.csv`).

## Summary

Candidates examined: the 250 lines of `build/scratch/delta/other.txt` (75
pairs, 175 new functions; every one identified, list at the end), the 174
`changed --area src/engine --plausible --min 0.3` pairs, the 384 `unmatched
--area src/engine` functions, and the SDK / runtime lists (13 + 60, 28 + 48).
Every input function of GT3 (T472..T480) was compared with its twin. The IOP
modules of both discs were compared (their extracted files), and the VU
microcode blobs were descrambled and
diffed.

Game-relevant, in order of what they are worth to GT3:

1. **Logitech Driving Force Pro support** (USB 046D:C298, the 900-degree
   wheel). This is the physics survey's "input device 6" and its second
   force-feedback set. The wheel port reports it as Control USB type 4.
   `RaceInput::readWheel` reads it as a 14-bit wheel and marks it device 6.
   It needs Lupo 2's `PDIUSB.IRX`: GT3's IOP module does not attach a C298.
   **Port as an optional group**, after a PCSX2 test of the IOP module (B1).
2. **The wheel's centring spring follows the race state.** `PortBase` gains
   `enableSpring(bool)` and `setDamper(int, int)` (slots 7 and 8, named by
   Lupo 2's linkonce symbols). On a Logitech wheel, `enableSpring` switches
   the wheel's built-in default spring. On connect, a 3-step set-up makes
   that spring weak and turns it on. `RaceInput::setMode` (race_logic C5)
   turns it off while racing and on in pause and after the race. **Port**
   (small; wheel users) (B2).
3. **Menu steering with the wheel.** Left / right now come from the wheel's
   position beyond a dead zone. The auto-repeat speed follows how far the
   wheel is turned (new port slot 6, `Control::getRepeat`). **Optional**
   (B3).
4. **Wheel connect and calibration details.** The 0x6F8 wheel gets a
   centring sequence. Logitech auto-centring waits longer. The pedal
   calibration is applied to the 0xC294 wheel as well as the GT Force.
   **Port the C294 calibration together with menus_b 11** (without it,
   the C294 defaults of 11 do nothing). The rest is optional (B4).
5. **CarModel CLUT animation select (confirmed).**
   `modelset_reset_anims(ModelSet *, int id)` and
   `clut_anim_reset_all(Tex1Image *, int id)` take an animation id, as
   rendering item 1 said. Only matching records are reset, and the reset
   no longer applies the palette. **Port** with rendering 1, but keep GT3's
   immediate apply for id < 0, because GT3's prize models need it (B5).
6. **Screen position adjust**: a new `gfx_set_display_offset(dx, dy)` sets
   the driver's DISPLAY2 offsets. It is the engine half of menus_a 24.
   **Port with menus_a 24** (B6).
7. **Wide screen from the console's aspect at boot**: `RenderContext::init`
   sets `wide = (sceScfGetAspect() == 2)` through a new scf-lock wrapper. It
   is the engine half of menus_b 10. **Optional, with menus_b 10** (B7).
8. **Movie start waits for a frame**: a 3-instruction mpeg accessor, the
   engine half of menus_a 32. **Optional, with menus_a 32** (B8).
9. **Engine support for the projected shadow and the bike rider**: a new
   lighting mode 6 (projection matrix upload), skinned VIF models (matrix
   palette) and a larger VU1 / VU0 microcode. **Port mode 6 and the
   microcode only with rendering 6. Do not port the skinning** (bikes) (B9).

Checked on purpose (other surveys asked):

- **The Pod's 4th sound bank needs nothing from the backend.**
  `file_read_sound_bank` (L2 0x0024F498 = US 0x002418D0) does what GT3's
  does: read the head, read the header, stream `spuSize` bytes to the
  given SPU address. It now goes through a `FileDevice` virtual, which is
  backend. The sound driver (`engine/sound`) differs only by codegen. The
  bank count and SPU addresses are game code (cameras_sound, `se_init`).
- **New asset paths**: none in the backend. The only L2-only paths in these
  folders are `bike/%s.mdl`, `bike/front_tire.mdl` and `bike/rear_tire.mdl`
  (bike loader L2 0x00231C70, rendering 11).
- **Memory**: the heap top is the same (`0x01F00000`, L2 `heap_init_once`
  0x00288748). Lupo 2's image ends at 0x004C4D1C (GT3 0x004B2C0C), so its
  heap is 72 KB smaller. The VRAM frame buffers are the same (0x69000 words,
  `RenderContext::init`). Allocator and unit arena work as in GT3. The game
  pools are game code.

Backend classes of change (not ported):

- A newer SDK (libkernl TLB / alarm / ExecPS2 code, libcdvd, libscf
  without asserts, libmc, libpad), newer IOP modules (IOPRP280, every IRX
  differs), and a newer compiler runtime (EH frames with locks, newlib with
  float printf / dtoa, an exception helper L2 0x002968A0 with 593 callers).
- The file layer sits behind a `FileDevice` / `FileDeviceSingle` interface.
  `vol_mount`, `file_resolve`, `file_locate`, `vol_init` and the sound-bank
  loader use the same paths (`host:data/`, `.gz`).
- i.LINK: `PadManager` lost its eight `IEEE1394Port`s (0x974 bytes down to
  about 0x304). Link input moved to Multilink (race_logic 17), and the
  link-session and pdi1394 internals changed.
- New devices: a USB head tracker and USB serial classes (`pdiHt`,
  `hdx1735`, `laserbird`; IOP `uht.c` / `userial.c`), for camera C9 (do not
  port).
- Bike-rider maths (Geomath templates, `pdiRiderman`) placed in engine/pgl
  and engine/text by link order (bikes, do not port).
- MPEG / IPU decoder rework. Memory-card manager operations
  (`mc_set_file_info` asynchronously, unformat) for the Concept money
  transfer (menus_b 13).
- `expf` / `logf` (pdistd fastmath) use new approximations (Taylor series).
  Their callers are rendering, the camera LOD bias and the logger, not
  physics.
- Graphics-driver codegen, and a CLUT table that is built once
  (`pfx_build_identity_clut32`).

Corrections to the other surveys and to the map:

- **physics 19 / 20**: "device 6" is the Driving Force Pro (B1), not a
  guess any more. The force-feedback set chosen by `controller+0x14 >> 5 ==
  4` is the DFP's set.
- **race_logic C5** (UNCERTAIN there): port slot 7 is
  `PortBase::enableSpring(bool)` and slot 8 is `PortBase::setDamper(int,
  int)` (linkonce names). The damper wrapper `RaceInput` L2 0x0021CEA8 has
  no caller (B2).
- **race_logic** (`RaceInput::read`, "same behaviour for GT3's inputs"):
  that is true for USB types 1..3. `readWheel` (L2 0x0021DCA8) also has a
  new type-4 branch, the DFP (B1).
- **menus_b 11**: GT3's engine applies the wheel calibration only to the
  GT Force. The C294 defaults also need the engine change in B4.
- **rendering 6**: the projected shadow also needs lighting mode 6 and Lupo
  2's VU1 microcode. Its program addresses move, so the EE-side MSCAL
  constants move with it (B9).
- **Map (v2)**: the four display setters are shifted by one (L2 0x0026C628 /
  0x0026C658 / 0x0026C680 / 0x0026C6A8), and so are the selector-table
  setters (0x002681A8..0x002681E0). Wrong pairs: `lcg_rand_range`,
  `vol_init`, `bytes_equal`, and the runtime pairs `atof`, `strtoul`,
  `_lseek_r`, `_mprec_log10`, `fptoui`. v1 had `strtoul` (=
  `__malloc_unlock`) and `fptoui` (= `dptoul`) right. The function list
  merges `flare_copy_visibility` + `flare_reduce` and two display setters.
  All are in `build/scratch/survey_backend/pairs_fix.csv`.
- **Not surveyed by anyone**: `src/game/debug` (the developer logger, 25
  pairs differ, 41 unpaired). `delta.py` has no area for it. Its one line in
  other.txt is codegen.

---

## Entries

### B1. Logitech Driving Force Pro (046D:C298) support

| Field | |
|---|---|
| What | Lupo 2 supports the Driving Force Pro. The IOP driver attaches it and reads its 14-bit wheel. The wheel port reports it as Control USB type 4 (`0x80` in `Control.type`), with the 0xC294's button map, no menu steering from the wheel (B3) and no auto-centring (the wheel centres itself). `RaceInput::readWheel` accepts type 4: device byte 6, steering `0x1000 - x / 2`, clamped to +-0x1000 (centre 0x2000, 14 bits). The pedals work as on the other wheels (dead zones 0x4B / 0x50). The physics treats device 6 apart (physics 19: full lock, extrapolated input) and gives type 4 its own force-feedback set (physics 20). In GT3 the IOP module does not attach the wheel, and even if it did, `translate` would report it as type 7 and `readWheel` would ignore it (`usbType >= 4`). |
| Where | IOP `PDIUSB.IRX`. engine/input: `USBSteeringPort::translate` L2 0x0023FCA0 = US 0x00230D10, `USBSteeringPort::update` L2 0x0023F9C0 = US 0x00230A90, port slot 6 L2 0x0023FFD8 (0 for C298). race/core: `RaceInput::readWheel` L2 0x0021DCA8 = US 0x002115F8 (`T413_race_input.cpp`). physics: `car_input_update` L2 0x001FA690, `steer_force_update` L2 0x001F6C10. |
| Kind | changed logic (engine/input, race/core) + IOP module |
| Evidence | `translate`: `ori 0xC298` → `li 0x80; sw 0x14(control)`, then `usbC294Buttons`; at 0x0023FE14 `beq product, 0xC298` skips the steer buttons. `update` 0x0023FAA8: `0xC298` → `centred = 1`, no `autoCentre`. `readWheel`: `slti type, 5` (GT3 `< 4`); type 4 → `sb 6, 1(in)`, `srl x, 1`, `0x1000 - x`. IOP: GT3's `PDIUSB.IRX` (18,373 bytes) has the device table 046D:C293, 046D:C294, 046D:C295, 06F8:0004. Lupo 2's (29,493 bytes) adds 046D:C298 with its own report layout (table at file 0x5590 → 0x56A8). It also relabels a 046D:C294 whose bcdDevice is 0x1xxx (a DFP in compatibility mode) as C298 (IRX text 0x155C..0x1594). No EE or IOP code sends a native-mode command (no 0xF8 constant): the IOP report layout does the work. 0xC298 = Driving Force Pro is the USB id database's name (INFERRED from it, not from the code). |
| Port | engine/input: type 4 for C298 in `translate`, `centred = 1` for it in `update`, the slot-6 exception (B3). race/core: the type-4 branch of `readWheel`. physics 19 and 20. IOP: Lupo 2's `PDIUSB.IRX` (asset from `lupo2_vol`) or a patched GT3 one. **UNCERTAIN, test first**: whether Lupo 2's IRX runs with GT3's IOP set (it imports `sysmem` and `thfpool`, Lupo 2 boots IOPRP280, GT3 IOPRP224) and talks to GT3's EE `pjoy` client. The client is nearly unchanged in Lupo 2 (pairs 0.0..0.5; `pjoy_send_command` gained a wait argument). PCSX2's USB wheel emulation offers a Driving Force Pro; check which USB id it presents. |
| Verdict | **port, optional group** (a real gain for wheel users: a 900-degree wheel), only after the IOP test. Without the IRX the rest is dead code. |

### B2. Wheel centring spring by race state; Logitech force-feedback set-up

| Field | |
|---|---|
| What | `PortBase` grows from 6 to 9 virtuals. [6] returns the wheel deflection level (B3). [7] is `enableSpring(bool)`. [8] is `setDamper(int, int)`. The defaults are empty, `SteeringPSPort` forwards them to its inner port (+0xC), and `PSPort` keeps the defaults. On a USB wheel, `enableSpring(on)` = `ff_default_spring(port, 0x20, on)`: the wheel's own centring spring. `setDamper(k, …)` downloads and plays a damper in slot 0x80 with K1 = K2 = k (only from L2 0x0021CEA8, which has no caller). When a Logitech wheel connects, a 3-frame set-up runs (new `USBSteeringPort` +0x20, states 0..3): default spring off on all four slots (0xF0); Set Default Spring K1 = 3, K2 = 3, clip 0x1E; default spring on (0x20). Then the race side (race_logic C5): `RaceInput::setMode(mode)` stores the mode (+0x148) and calls `enableSpring(mode)`. `RacePS2Base::virtual_41` passes 0 when the race starts and 1 when it ends, and the pause setter passes `paused`. The result: a weak centring spring in menus and pause, and none while racing, where only the game's force feedback drives the wheel. GT3 never touches the default spring, so the wheel's power-on spring fights the game's forces. |
| Where | Vtables `8PortBase`, `14SteeringPSPort`, `15USBSteeringPort` (L2 slots 6..8: 0x0023DFE8 / 0x002D2A90 / 0x002D2A98; 0x0023EF98 / 0x0023F028 / 0x0023F058; 0x0023FFD8 / 0x0023F680 / 0x0023F7F8). Set-up L2 0x0023F5B8, called from `update` 0x0023FA94. `ff_send` L2 0x00281BD8, `ff_send_download_play` L2 0x00282500 (GT3 0x00266810 / 0x00266830 equivalents). `RaceInput::setMode` L2 0x0021CE60 (race_logic C5). GT3: `T476_port_base.h`, `T478_steering_ps_port.cpp`, `T479_usb_steering_port.cpp`, `pjoy.c`. |
| Kind | new virtuals and logic (engine/input) + race/core caller |
| Evidence | `0023F680`: `sltu a2, 0, a1; li a1, 0x20; jal ff_default_spring`. `0023F5B8`: switch on `+0x20`: `ff_default_spring(port, 0xF0, 0)` → 1; `ff_set_default_spring(port, 0xF0, 3, 3, 0x1E)` → 2; `ff_default_spring(port, 0x20, 1)` → 3. `0021CE60`: `sw a1, 0x148(a0)`, then the call through the port's vtable entry 0x40 (slot 7). Names `enableSpring__8PortBaseb` and `setDamper__8PortBaseii` in `lupo2_symbols_demangled.txt`. |
| Port | Three virtuals at the end of `PortBase` (GT3 builds its vtables from source; `IEEE1394Port` inherits the defaults), the two `SteeringPSPort` forwarders, the USB implementations, a set-up state field in `USBSteeringPort`, the 3-step set-up, and `RaceInput::setMode` plus its three callers (race_logic C5). `pjoy.c` already has every FF builder used. Lupo 2 puts the state at +0x20 (it shifts `st` by 4, drops `prevX`, keeps `actCount` at +0x64) and B3's two fields at +0x74 / +0x78. In GT3's layout, put the new fields in `unk_69` or at the end: the class grows past 0x74 bytes, inside `PadManager`. |
| Verdict | **port** (small; wheel users; no assets) |

### B3. Menu steering with the wheel; repeat speed from the wheel's deflection

| Field | |
|---|---|
| What | (a) A Logitech or 0x6F8 wheel's left / right menu button comes from the wheel's position: left when x < 0x200 - dz, right when x > 0x200 + dz. `onConnect` sets the dead zone `dz` (+0x74): 0x90 for Logitech wheels, 0xC0 for the 0x6F8 wheel, 0 otherwise. GT3 uses fixed limits (0x13F / 0x2C1, 0x11F / 0x2E1) plus a quick-swing detection, with its own function for the 0x6F8 wheel. (b) The new slot 6 returns a level: 0 inside the dead zone, for the DFP or when not connected; else 1 + ((|x - 0x200| - dz) / step & 7), with step = (0x200 - dz) / 7 (+0x78). (c) `Control::getRepeat` counts in eighths of a frame. For left / right it adds the port's level (8 when the level is 0) and sets `repeating` at once, so the first delay is skipped while the wheel is turned. Turned a little, the cursor moves slowly; turned fully, at the pad's speed. Pads and other directions add 8 per frame, which is GT3's timing. |
| Where | `usb46DSteerButtons` L2 0x0023F348 = US 0x00230628 (now `(x, dz)`; the 0x6F8 wheel uses it too), `USBSteeringPort::onConnect` L2 0x0023F490 (= US 0x00230830 plus the dead zone), slot 6 L2 0x0023FFD8, `Control::getRepeat` L2 0x0023D658 = US 0x0022E0A0 (`T474_control.cpp`). |
| Kind | changed logic (engine/input; menu feel) |
| Evidence | `0023F348`: `slt x, 0x200 - dz` → 4, `slt 0x200 + dz, x` → 8. `0023F490`: `li 0x90` / `li 0xC0` → +0x74, `(0x200 - dz) / 7` → +0x78. `getRepeat`: `andi mask, 0xC`, then vtable entry 0x38 (slot 6) of `port`, `addu count, level` or `+8`, `sra count, 3` before the compares. |
| Port | Slot 6 (B2's vtable work), the two fields, the new `usb46DSteerButtons`, and `getRepeat`'s eighths (for pads the counter compares the same). Remove `usb6F8SteerButtons`'s use. |
| Verdict | **optional** (menu feel for wheel users; GT3's behaviour is fine) |

### B4. Wheel connect and calibration details

| Field | |
|---|---|
| What | (a) The vendor-0x6F8 wheel (Guillemot / Thrustmaster (?)) gets a centring sequence of timed constant forces on connect, five states (L2 0x0023F848 with step helper 0x0023F948). GT3 marks it centred at once. (b) Logitech `autoCentre` holds each stop for about 32 frames instead of 12. Its centre window (0x60..0x9E) is unchanged. (c) `update` applies the stored calibration to the 0xC294 wheel (`usbApplyCalibration(..., wheel 3, ...)`) as well as to the GT Force (wheel 2), while the pedal map is on. GT3 calibrates only the GT Force. |
| Where | `USBSteeringPort::update` L2 0x0023F9C0 (0x0023FB10..0x0023FB74), `autoCentre` L2 0x0023F6A8 = US 0x00230938, new L2 0x0023F848 / 0x0023F948. |
| Kind | changed logic (engine/input) |
| Evidence | `update`: `beq product, 0xC293` → wheel 2, `beq product, 0xC294` → wheel 3, both on `GameStatus +0x31268` (L2 options +0x638 = `ControllerConfig`). `autoCentre`: `slti count, 0x1F` (GT3 `count++ >= 0xB`), `sltiu x/4 - 0x60, 0x3F` (the same window as GT3's `abs(pos - 0x7F) < 0x20`). |
| Port | (c) is one more branch in `update`, and it is what menus_b 11's 0xC294 defaults need. (a) and (b) are wheel feel. |
| Verdict | **port (c) with menus_b 11**; (a), (b) optional |

### B5. CLUT animation select in the model set and texture code (rendering 1)

| Field | |
|---|---|
| What | `modelset_reset_anims(m, id)` passes `id` to `clut_anim_reset_all` for every texture: the base table (NULL entries skipped) and each texture set other than the base table. It calls `animset_reset(m->anim)` only when `id < 0`. `clut_anim_reset_all(img, id)` resets a channel (`clut_anim_channel_reset`) and sets `dirty = 1` when `id < 0` or `ch->id == id` (`ClutAnimChannel.id` +0x00). It does **not** apply. GT3 resets every channel and applies it at once (`clut_anim_table_reset`: reset + `clut_anim_channel_apply`). `modelset_step_anims(m, dt, id)` already takes an id in GT3 and is unchanged. |
| Where | `modelset_reset_anims` L2 0x002321D8 = US 0x002248D0 (`src/engine/gfx/T451_model_set.cpp`); `clut_anim_reset_all` L2 0x00264720 = US 0x00249518 (`src/engine/pgl/tex1.cpp`). Callers in L2: `course_reset_anims` L2 0x001AB030 (five model sets, -1) and `spriteset_reset_clut_anims` L2 0x002346B8 (-1); `PrizeBase` (L2 0x002377EC, -1); `CarModel::setAnimSelect` L2 0x0022DC90 (the new id). |
| Kind | changed logic (engine, for a game feature) |
| Evidence | `002321D8`: `a1` kept in `s4`, passed to both `clut_anim_reset_all` calls; `bgezl s4` skips `jal animset_reset`. `00264720`: `bltz s2` (id < 0) → reset; else `lhu 0(ch); bne id` skips; after the reset `sb 1, 0x1F(ch)`; no apply call. |
| Port | Add the `id` parameter to both functions and pass -1 from GT3's callers (`course_reset_anims`, `spriteset_reset_clut_anims`, the three `PrizeBase` calls). **Keep the apply for id < 0**: GT3's prize models never call `modelset_apply_clut_anims` (only the course does, from `RacePS2Base`), so they rely on the reset writing the first palette. Applying after an id reset is also harmless: the car path applies every frame anyway. |
| Verdict | **port** (with rendering 1) |

### B6. Screen position adjust: the display offset (menus_a 24)

| Field | |
|---|---|
| What | A new driver function stores the DISPLAY2 offsets `dispDX` / `dispDY` (`GfxDriverState` +0x8C / +0x8E) and recomputes DISPLAY2 through `gfx_update_display_regs` (DX = dispDX x MAGH + 0x28C, DY = first line + dispDY). GT3 has both fields and uses them in the formula, but nothing sets them. Lupo 2's GameOptions calls it with twice the slider values (+0x64 / +0x68, -16..16): from `reset` (after `setDefaults`), from the two slider setters, and when the options are loaded (from `GameStatus` +0x30C94 / +0x30C98). |
| Where | New L2 0x0026C680 (`src/engine/pgl/gs_display_driver.c`; the map paired it with `gfx_set_display_fb`, which is L2 0x0026C6A8). Callers L2 0x00226A80 (`GameOptions::reset`), 0x00226B58 / 0x00226B88 (setters, from the options page), 0x00226E90 (from `GameOptions::load`). |
| Kind | new function (engine, for a game feature) |
| Evidence | `0026C680`: `sh a0, 0x8C(g); sh a1, 0x8E(g); j gfx_update_display_regs`. Callers: `lw 0x64 / 0x68; sll 1`. |
| Port | `void gfx_set_display_offset(int dx, int dy)` (three lines) plus menus_a 24's game side. GT3's save keeps +0x64 / +0x68 at 0, so old saves load centred. |
| Verdict | **port with menus_a 24** |

### B7. Wide screen from the console's aspect at boot (menus_b 10)

| Field | |
|---|---|
| What | `RenderContext::init` sets `wide` (+0x1C) to `aspect == 2` (16:9 in the console settings) where GT3 writes 0. The aspect comes from a new lock-wrapped helper: `WaitSema(scf lock)`, `sceScfGetAspect()`, then the signal. The same helper feeds `GameOptions::setDefaults`'s wide default (menus_b 10). |
| Where | `RenderContext::init` L2 0x002514E8 = US 0x002431B0 (`src/engine/gfx/T512_render_context.cpp`); helper L2 0x00284CF8 (`src/engine/system/scf_lock.c`, beside `scf_clock_to_gmt`); `sceScfGetAspect` L2 0x00292040 = US 0x00274720 (GT3 has it). |
| Kind | changed logic (engine default) |
| Evidence | `002514E8`: `jal 0x00284CF8; xori v0, 2; sltiu v0, 1; sw v0, 0x1C(s3)`. |
| Port | The wrapper (5 lines, as `scf_clock_to_gmt`) and one line in `RenderContext::init`. GameOptions overrides it once the options load. |
| Verdict | **optional, with menus_b 10** |

### B8. Movie start waits for the first frame (menus_a 32)

| Field | |
|---|---|
| What | `MenuBgMovie::start` loops on `gfx_vsync(1)` until a new mpeg accessor returns non-zero. The accessor returns whether the decoder has posted a frame (a .sbss flag). GT3 can show a blank first frame. |
| Where | L2 0x0027A858 (`src/engine/media`), called from `MenuBgMovie::start` L2 0x001CF508 (= US 0x001CD458). |
| Kind | new function (engine accessor for a game feature) |
| Evidence | `lw v0, gp-0x423C; sltu v0, 0, v0`. |
| Port | Find GT3's equivalent flag in `mpeg_player.c` / `T529_mpegif.cpp` (the frame queue) and add the accessor. |
| Verdict | **optional, with menus_a 32** |

### B9. Lighting mode 6, skinned VIF models, bigger VU microcode (rendering 6 / 11)

| Field | |
|---|---|
| What | (a) `vifmodel_send_lighting` has a mode-6 case. It uploads a 4x4 matrix (VF28..VF31 when the context byte +1 is 2, else VU0 memory 0xC) and the context vector +0xA50 to VU1, and sets the VU1 lighting program (`ctx+0xC`) to 0x2F7 = dword 759: exactly where the 26 new dwords of the VU1 microcode are inserted (c), so that block is mode 6's VU1 program. It is a projected-texture set-up, INFERRED to be the projected shadow's receiver pass (rendering 6; the bbox reader there also works with VF28..VF31). (b) `vifmodel_draw_lit` handles models with `scaleFlags` bit 0x80. After each part it loads up to four bone matrices, picked by index bytes at the end of the part, from the context's matrix palette (+0xB74) into VF8..VF23. That is matrix-palette skinning, for the bike rider (`HumanModel`, rendering 11). (c) The VU microcode grew. VU1 is 0x3FC0 bytes against 0x3DA8 (scrambled with the same LCG, seed 0x32277070). Descrambled, it is GT3's code with 26 dwords inserted at dword 759, branch fixes after the insertion, and 41 dwords appended. VU0 is 0x3D0 against 0x398 bytes: one routine (dwords 72..84) grew to 20 dwords. |
| Where | L2 0x002691B0 (from `vifmodel_send_lighting` L2 0x00269340 = US 0x0024E028); L2 0x00269CE0 → 0x00269A38 / 0x00269D40 → 0x00269BF0 (from `vifmodel_draw_lit` L2 0x002695E0 = US 0x0024E2B8; `src/engine/pgl/vu1_lighting.c`); `gfx_init` L2 0x0026C150 = US 0x00250820 (VU1 blob L2 0x002E2E80 / US 0x002F7440, VU0 L2 0x00352500 / US 0x00349600; `gs_driver_core.c`, `vu1_microcode.inc`, `vu0_microcode.c`). |
| Kind | new functions + new data (microcode) |
| Evidence | `002691B0`: `lbu 1(ctx) == 2` → `vmove vf20..23, vf28..31`, else `vlqi` from VU0 0xC; `sqc2` of vf20..23 and `lqc2 0xA50(ctx)`; `li 0x2F7` → `ctx+0xC`. `002695E0`: `lbu 9(m) & 0x80` → `jal 0x00269CE0`. `00269BF0`: four `lbu` indices × `<< 6` into `lw 0xB74(ctx)`, `lqc2` vf8..vf23. `gfx_init`: `li 0x3FC0` (unscramble, upload VU1), `li 0x3D0` (VU0). The diffs come from `difflib` over the descrambled dwords. |
| Port | (a) and (c) only with rendering 6. Take Lupo 2's VU1 blob, and every EE-side VU1 program address (MSCAL entries) after dword 759 moves by 26. Find and check the VU0 routine that changed. (b) only with bikes. |
| Verdict | **port (a) and (c) with rendering 6 only; do not port (b)** |

---

## other.txt (250 lines), line by line

Classes: **game** (an entry above), **area** (belongs to a game area),
**wrong** (wrong pair / a GT3 function the map missed), **backend** (one
line, what it is). Line numbers are other.txt's.

Pairs (lines 1-75): game 2, wrong 5 (+1 counted in game), area 2, backend
66.

| Line | L2 | Class |
|---|---|---|
| 1 | 00133A58 `T055_static_init` | area: game/debug (logger) static initialiser, codegen (stores through `addiu` bases) |
| 2 | 001B3520 `F_0022D9A8` | area: race/modes Multilink teardown (race_logic 17, do not port) |
| 3 | 0023C1E0 `link_take_commands` | backend: i.LINK session, gp globals (codegen) |
| 4 | 0023CC20 `F_0022D7A0` | backend: i.LINK session getter, field +0x7C (layout) |
| 5 | 0023D890 `PadManager::term` | backend: `PadManager` without IEEE1394 ports (sema +0x300, GT3 +0x970) |
| 6 | 0023EC90 `PSPort::load` | backend: link-frame load, field moved |
| 7 | 0023F508 `USBSteeringPort::onDisconnect` | backend: fields +4 (the new FF state, B2) |
| 8 | 0023F9C0 `USBSteeringPort::update` | **game**: B1, B2, B4 |
| 9 | 002400D0 `USBSteeringPort::load` | backend: fields +4 |
| 10 | 00250CC0 `T509_static_init` | backend: Serialize class-registry initialiser, codegen |
| 11 | 002650A0 `font_get_param` | backend: font struct fields |
| 12 | 00268B78 `fmt_put_decimal` | backend: codegen |
| 13 | 0026B1F0 `pgl_envmap_point_at` | backend: codegen |
| 14 | 0026C680 `gfx_set_display_fb` | **game + wrong**: new `gfx_set_display_offset` (B6); the twin is L2 0x0026C6A8 |
| 15 | 0026C750 `gfx_dma_channel_init` | backend: DMA channel struct layout |
| 16 | 0026EE70 `gs_fill_target_b` | backend: codegen (loop) |
| 17 | 002702E8 `gfx_set_clut_mode` | backend: codegen |
| 18 | 00274F58 `pfx_build_identity_clut32` | backend: CLUT table built once by new L2 0x00274F00, same CLUT |
| 19 | 00276B38 `gs_store_material_vec` | backend: codegen (context offsets) |
| 20-31 | 0027C1C0, 0027C7D8, 0027C848, 0027C8C8, 0027CA10, 0027E898, 0027F7C8, 0027FF00, 0027FFB8, 002802A8, 002805E8, 00280618 | backend: MPEG / IPU decoder rework (frame buffers from new 0x0027C590, semaphores, codegen) |
| 32 | 00281128 `pjoy_bind` | backend: retries with `yield_low_priority` instead of vblank waits |
| 33 | 00281830 `usbd_unregister_driver` | backend: USB manager layout |
| 34 | 00281BF0 `pjoy_send_command` | backend: extra wait argument (`WaitSema`) |
| 35 | 00282550 `ff_set_default_spring` | backend: right pair (via the new `ff_send`); used by B2 |
| 36-37 | 00284D38, 00284D70 `scf_clock_to_gmt` / `_local` | backend: newer libscf time zone handling |
| 38 | 0028A898 `break_stub_1` | backend |
| 39-44 | 0028BA50, 0028BFC0, 0028C588, 0028C790, 0028CB20, 0028D250 | backend: pdi1394 (i.LINK) |
| 45 | 0028E050 `sceCdPOffCallback` | backend: newer libcdvd |
| 46-51 | 00291F10, 002920C0, 00292298, 00292368, 00292520, 002925B8 | backend: newer libscf (asserts and printf removed) |
| 52-53 | 002955C8, 00295688 `__do_global_ctors`, `__main` | backend: newer runtime (atexit) |
| 54-57 | 00297C78, 00297CB8, 00297F08, 002982E0 | backend: newer EH frame code (`frame.c`, sorted FDEs, locks) |
| 58 | 0029AC10 `unpack_f` | backend: fp-bit, codegen |
| 59 | 0029AF58 `fptoui` | **wrong**: = `dptoul` / `__fixunsdfsi` US 0x00279B48 (map v1 had it) |
| 60-64 | 0029B170, 0029B228, 0029C0A8, 0029CBE0, 0029CC98 | backend: newer newlib (`abort`, `fprintf`, `strcasecmp`, `__sbprintf`, `_vfprintf_r` with floats) |
| 65 | 0029F4D8 `atof` | **wrong**: = `atoi` US 0x0027D890 (calls `strtol`) |
| 66 | 002A17D8 `_localeconv_r` | backend |
| 67 | 002A24A0 `strtoul` | **wrong**: = `__malloc_unlock` US 0x00282880 (map v1 had it) |
| 68 | 002A2FD8 `__ulp` | backend |
| 69 | 002A4D48 `_lseek_r` | **wrong**: = `_write_r` US 0x00284000 |
| 70 | 002A4F18 `_mprec_log10` | **wrong**: = `_lseek_r` US 0x00284118 |
| 71-73 | 002A6A00, 002A7DC8, 002AF180 | backend: newer SDK (`_sceCdDelay` with alarms, `sceSifRemoveCmdHandler`, `_InitSys`) |
| 74-75 | 002B1160, 002B16C8 | backend: libgcc (`__udivdi3`, `__fixunsdfdi`) |

New functions (lines 76-250): game 6, area 32 (6 of them in entries
B8 / B9), wrong 7, backend 130.

| Line | L2 | Class |
|---|---|---|
| 76 | 00163B20 | area: menu/race_menu, Concept licence short menu (menus_a R2), a setter `+0x58 = 0` |
| 77-78 | 001B3438, 001B34B0 | area: race/modes Multilink / RaceLinkDemo (race_logic 17) |
| 79-80 | 0022A808, 0022A868 | area: menu/memorycard, GT3-save descriptor 0x00349F50 of the money transfer (menus_b 13); L2 0x0022A818 (the map's `bytes_equal`) and 0x0022A888 belong with them |
| 81 | 00231DF0 | area: bikes, `RaceBikeModel` slot 1 helper (rendering 11) |
| 82 | 002336D0 | area: rendering 6, the opcode-0x0A bbox reader (known) |
| 83-86 | 0023C030, 0023C190, 0023CF40, 0023CF48 | backend: i.LINK session accessors (callers Multilink, the linked attract demo) |
| 87 | 0023DFE8 | **game**: `PortBase` slot 6 default (B2 / B3) |
| 88 | 0023EBE8 | **wrong**: `PSPort::setActuator` US 0x0022FE60 (vtable slot 3, missed) |
| 89 | 0023F490 | **game**: `USBSteeringPort::onConnect` with the dead zone (B3) |
| 90 | 0023FFD8 | **game**: USB wheel slot 6, deflection level (B3) |
| 91 | 0024AA68 | backend: font buffer release |
| 92 | 0024B9E8 | backend: static initialiser / destructor of an io global (0x0035E738) |
| 93-97 | 00250F48, 00250F50, 00250F58, 00250F90, 00251278 | backend: `FileDeviceSingle` virtuals (new file-device class) |
| 98-116 | 0025BEB0, 0025BFA0, 0025CC38, 0025D138, 0025D140, 0025D240, 0025D2E8, 0025DB10, 0025F120, 0025F138, 0025F168, 0025F3B8, 0025F7B8, 0025F878, 0025F930, 0025F978, 0025F9B8, 002608A8, 002608E0 | area: bikes. Geomath / `pdiRiderman` rider maths (angle wraps at pi / 2pi, sin / cos wrappers, sine series, matrix set-up); callers `HumanModel` / `RaceBikerModel` (L2 0x001AE8C8..0x001AFA60), the rider skeleton (L2 0x00254xxx..0x0025A5xx) and the rider simulation L2 0x00209810 (`bike/bikeman_e3.bin`, physics 1) |
| 117 | 00262238 | **wrong**: `flare_copy_visibility` US 0x002470C0 + `flare_reduce` US 0x002471D8 (L2 0x00262358) merged by the function list |
| 118-119 | 002681B8, 002681E0 | **wrong**: `matsel_set_table` US 0x0024CF08, `texsel_set_table` US 0x0024CF30 (the map shifted the four setters) |
| 120 | 002691B0 | area: rendering 6, lighting mode 6 (B9) |
| 121-124 | 00269A38, 00269BF0, 00269CE0, 00269D40 | area: bikes, skinned VIF models (B9) |
| 125 | 0026C128 | **wrong**: `gfx_init_default` US 0x00250808 |
| 126 | 0026C628 | **wrong**: `gfx_set_display_area` US 0x00250CF8 |
| 127 | 00273DA8 | **wrong**: `pgl_set_morph_weight` US 0x00258150 |
| 128 | 002790A8 | backend: IOP module name list (`strcmp` / `strcpy`) |
| 129 | 002794A0 | backend: sector-cache set-up for `vol_init` (L2 0x0024F658 = US `vol_init`) |
| 130 | 0027A858 | area: menus_a 32, mpeg has-a-frame accessor (B8) |
| 131-139 | 0027B3C0, 0027B3D0, 0027B3D8, 0027BED8, 0027C530, 0027C590, 0027C8A8, 0027EF30, 00280218 | backend: pdicdvd clock and MPEG / IPU (frame allocation, semaphores) |
| 140-142 | 002810B0, 002812E0, 00281A48 | backend: new USB class set-up (called from the IOP loader L2 0x00103A80) |
| 143 | 00281BD8 | **game**: `ff_send` (B2) |
| 144 | 00282500 | **game**: `ff_send_download_play` (B2); L2 0x00282528 is `ff_send_refresh` |
| 145-158 | 00282BA8, 00282CC8, 00282E78, 00282F28, 00282F88, 002830A0, 002831D0, 00283508, 00283638, 002836D0, 002836F0, 00283750, 00283858, 00283928 | backend: new USB class drivers (head tracker / serial, IOP `uht.c` / `userial.c`): register, bind, read, write |
| 159-162 | 00283B50, 00283BC8, 00283D88, 00283FD0 | backend: head-tracker devices `pdiHt` / `laserbird` (camera C9, do not port) |
| 163 | 00284CF8 | **game**: scf-lock `sceScfGetAspect` (B7) |
| 164-167 | 00288700, 0028A830, 0028A8A8, 0028C8F0 | backend: pdistd global, semaphore helper, printf flush to DECI2, pdi1394 |
| 168-169 | 0028EE10, 0028F0F0 | backend: newer libcdvd (`DiskReady` over RPC) |
| 170-172 | 00292248, 00292278, 002925B0 | backend: newer libscf BCD helpers |
| 173 | 002928A0 | backend: libm wrapper (caller: prize model, menus_b) |
| 174 | 002954F8 | backend: `__fixsfdi` (float to s64; caller `MemorycardGT3File::makeUniqueName`) |
| 175-180 | 00295810, 002974F0, 00297A68, 00297B98, 00297D48, 002981C0 | backend: newer libgcc EH frame code |
| 181-184 | 00298B88, 00298BE8, 00298C80, 0029AAE8 | backend: C++ runtime `__user_type_info`, double to u64 |
| 185 | 0029BF78 | backend: `srand` (caller: the linked attract demo, menus_a F3) |
| 186-193 | 0029E380, 0029E540, 0029F4C0, 0029F4F8, 0029F728, 002A36C0, 002A3708, 002A4828 | backend: newlib dtoa / float printf / strtod pieces |
| 194-248 | 002A5E10 .. 002B0668 (55 functions) | backend: newer libkernl (initsys syscalls, TLB set-up with its "# TLB" messages, alarms, interrupt handlers, ExecPS2, iopreboot) and a libcdvd callback (0x002A6850) |
| 249 | 002B0BE0 | backend: libgcc 64-bit division (`__udivmoddi4`) |
| 250 | 002B17E0 | backend: libgcc double to u64 helper |

---

## Noise / backend list (beyond other.txt)

`changed` at difference >= 0.3, size within 2x (174 engine, 13 SDK, 28
runtime). All are backend codegen, layout or library-version changes,
except the ones in the entries (USB port: B1-B4, `clut_anim_reset_all`:
B5, `gfx_set_display_fb` = B6 wrong pair, `RenderContext` / scf: B7) and
the wrong pairs above:

- engine/system (12): i.LINK session (`link_*`, `F_0022D7A0..F_0022DA10`,
  static init / dtor) and the scf lock (`scf_clock_*`); `intr_suspend` uses
  the SDK's `DIntr`.
- engine/media (15): MPEG / IPU decoder (frame buffers, queues, DMA
  callbacks); `mpegif::stop/nextFrame/releaseFrame` codegen. Lupo 2's
  linkonce copies name GT3's inline helpers (`fileChangeRequest`,
  `fileChangeRequestWeak`, `stopRequest`, `requestDone`, `deleteOK`,
  `reset_mem_bitstream`): naming hints only.
- engine/pdistd (13): allocator static init / dtor, `PrintTarget`,
  `GZIP::InflateBase::inflate` (name pair), typeinfo functions (name
  pairs), `expf` / `logf` (new approximations, see the summary),
  `bytes_equal` (wrong pair, menus_b 13).
- engine/pdi (30): RPC classes of pdicdvd / pdispu2 / pdibgm / pdimpeg /
  pdipipe (static init, `RPC2::getTime` through new helpers), pdi1394
  (i.LINK), `IOPLoadModuleBase`; Lupo 2's linkonce copies of the RPC
  methods (`volume_set`, `read_start`, `spdif`, `fade`, `tick`, ...)
  have no callers.
- engine/io (24): `vol_mount`, `file_resolve`, `file_locate`,
  `file_read_sound_bank`, `vol_shutdown` through `FileDevice`; `vol_init`
  (wrong pair: L2 0x0024F658); `iso9660_find_in_dir` codegen; memory-card
  manager slot layout (`mc_slot_*`, `mc_do_*`, `mc_set_file_info`) and new
  `mc` operations L2 0x0024DAE8 / 0x0024DC18 (menus_b 13);
  `image_pack_bsearch` template recursion; typeinfo.
- engine/text (4): `FontDict` dtor / static init, `Font::loadKanjiFont`
  (boot font load moved to L2 0x0024A9A8), typeinfo.
- engine/gfx (8): `modelset_set_env_tex` / `F_00225BB8` (right pairs, the
  callee names shifted), `RenderContext::RenderContext` (exception
  handling), `setScreenSize` / `GSBuffer::setDisplay` (display-setter
  shift), `anim_blend_step/reset` (codegen), typeinfo.
- engine/input (33): accessor and typeinfo name pairs, `PadManager::*`
  (no IEEE1394 ports), `PSPort::load`, `padStop`, `Control::latch`
  (codegen), `pjoy_bind` / `pjoy_send_command`; the USB port functions
  are in B1-B4.
- engine/pgl (30): GS driver codegen (`gfx_tex_param`, `gfx_enable`,
  `gfx_set_field_mode`, `gs_set_scanmask_mode`, `gfx_depth_func`,
  `gfx_dma_send`, `texregs_set_size`, `gfx_set_texture_size`,
  `gs_build_image_transfer_packet`, `dma_start_source_chain`, `pkt_ad`,
  `tex1_set_vram_base`, `vif_model_relocate`, `pgl_update_eye_lights`,
  `pgl_rotate`, `vu0_atan2f`, font helpers); `weather_drop_init` (its
  callee is `lcg_rand_range`, map fixed).
- engine/sound (5): `engine_layer_pitch`, `engine_setup`,
  `snd_note_to_pitch`, `seq_init`, `seq_restart`: codegen (the voice
  struct addressed from +0x10; `seq_restart` merged with its neighbour by
  the function list). The car engine sound synthesis is unchanged.
- sdk (13): libkernl, libscf, libcdvd, libmc, libpad (`pad_act_dma_transfer`
  string pair): newer SDK.
- runtime (28): newlib (malloc, printf), C++ runtime (dynamic cast, EH),
  libgcc (frame, fp-bit), libm `scalbn`: newer runtime; wrong pairs listed
  above.

`unmatched` (384 engine, 60 SDK, 48 runtime), grouped:

- Boot plumbing placed in engine/io by link order (L2 0x00101AB0..0x00102150:
  render thread, boot screen clear, `vol_init` wrapper; menus_a "backend").
- Multilink (race_logic 17): L2 0x001B3158..0x001B35B0.
- GT3-save descriptor (menus_b 13): L2 0x0022A808..0x0022AB48.
- Bikes (rendering 11): L2 0x00231C70..0x00231EA8 (bike models),
  0x0025A998..0x00260DF0 (Geomath / rider maths), 0x00269A38..0x00269D40
  (skinning), linkonce `Geomatht` / `pdiRiderman` at 0x002D5098..0x002D5E30.
- Rendering 6: 0x002336D0 (bbox reader), 0x002691B0 (lighting mode 6).
- i.LINK session and pdi1394: 0x0023C030..0x0023CF78, 0x0028C8F0..0x0028D928.
- Input: `PadManager` set-up 0x0023DB60..0x0023DF80, the USB port additions
  (B1-B4), USB class drivers 0x002810B0..0x00283928, head tracker
  0x00283B50..0x00284428 and linkonce `pdiHt` / `hdx1735` / `laserbird`
  (camera C9).
- Text: boot font load 0x0024A958..0x0024B458.
- io: memory card 0x0024DAE8 / 0x0024DC18, vol thread 0x0024ECC8 /
  0x0024ED80, `vol_init` 0x0024F658, FileDevice 0x0024FC08..0x00251278 and
  linkonce `FileDevice` / `FileDeviceSingle`.
- gfx: `RenderContext` variant 0x00251868 (no caller), 0x00251F28.
- pgl: unpaired twins (flare, selector tables, `gfx_init_default`,
  `gfx_set_display_area`, `pgl_set_morph_weight`), display offset (B6).
- pdi / media: RPC helpers 0x00277430..0x00279CD0, MPEG / IPU helpers,
  0x0027A858 (B8).
- system: scf aspect wrapper (B7).
- linkonce inline copies with no callers: `MenuTireModel::setBrakeModel`,
  `GTSOUNDINSTRUMENT::GetHeaderMax/GetAddr`, `_UnitArenaBase::getNumFree/Used`,
  `PortBase::enableSpring/setDamper` (B2), pdi RPC methods, `mpegif`
  request helpers, typeinfo functions.
- SDK / runtime: newer libkernl, libcdvd, libscf, libgcc EH, C++ runtime,
  newlib dtoa / printf.
