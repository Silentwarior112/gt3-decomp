# Follow-ups found during the first pass

Things the first pass found but did not change, because `ref/funcs.json` and
`ref/units.json` stay fixed while the units are being written.

## Function boundaries

- Handled (2026-10-08): the missed splits are in `ref/extra_starts.txt` (28
  starts: `_root` 0x001000C0, u114 0x0024CED0, u115 0x002501E0 / 0x002502F8,
  u116 0x002519C8, u121 0x0025F568, u123 0x00265880 / 0x002688D8 /
  0x00266A48..0x00266B48, u129 sceCdInitEeCB / scePadGetFrameCount /
  scePadStateIntToStr / scePadReqIntToStr / scePadInfoComb, u130
  mc_getinfo_end_callback / sceScfSetT10kConfig, u140 set_sreg /
  sceSifSetSreg / sceSifSetSysCmdBuffer / _request_call, u141
  sceSifAllocIopHeap / sceSifFreeIopHeap / sceSifLoadIopHeap), the false
  starts from stray data words in `ref/not_starts.txt` (0x00100000,
  0x00100014, 0x00252030, 0x00252520, 0x00262AE0).  Every split function has
  its own FUNC marker; the false starts' code is part of the enclosing
  function (ref/pass2/open/splits/notes.md).
- Left: the 7 shared-tail jumps in `ref/boundary_violations.txt` (pgl code,
  G001), accepted.  The stray `jr $ra` at 0x00288324 stays its own function.
- MMI pixel routines in the MPEG decoder (0x00260D48, 0x00261ED0, 0x00262610)
  are plain C with the same arithmetic; the matching pass needs the MMI form.

## Memory map

- `0x0028F2AC..0x0030729C` is not only `.eh_frame`: ordinary `.data` tables sit
  between the frame blocks (u008: T048's 92-entry page/cursor table, T049's
  button checks). `ground.py` skips that range when it collects code pointers
  from data, so `data_xrefs` miss those tables, and listings label them
  `.eh_frame`. Parse the frame blocks exactly and scan the rest as data.
  (Fixed for the annotations on 2026-10-04: `coreelf.Image.frames`; function
  starts still ignore that range, so `funcs.json` did not change.)
- The linker interleaved each C++ unit's `.data` with its `.eh_frame` block:
  a unit's data objects sit just before its frame block (u003: the movie player
  at 0x002CDC10, movie descriptors at 0x00292960, T010's class-name table at
  0x002944F0). That gives the owner of most `.data` objects, which a buildable
  tree needs.

## Cross-unit layouts to reconcile (integration pass)

- `ShortMenuArgs` (T102's short-menu argument block) is 0x188 bytes with 12
  rank lines (u016: its inlined constructor, next stack object at sp+0x190);
  u015 declared 0x118 bytes with 5.
- `CarEquipments` is polymorphic (passed by value with its vtable, 0x1E8
  bytes, u016); u015 declared it without a vtable.
- `Color4f`'s default is 128/255 (`0x3F008081`); fixed in u008's header.
- T102's `0x0012F820`, called `reset` in u015, is the short-menu rank entry's
  constructor (u017). Units that build `ShortMenuArgs` carry dead duplicate
  copies of the two entry constructors (weak in u013, u017).
- `0x00179C98` (`RaceInformation::virtual_2`) takes no argument and returns the
  course code; u001 declares a parameter. The `mode` argument u001 passes to
  the four T053 race entry points is never read. (u017)

- RaceBase (u043's header): +0xC is the race music object (RaceBGMPS2), not an
  input object; slot 38 returns int (tested by the race loop); CameraManager's
  vptr is at +0, not +0x973C. (u044)

- The object at RaceCarState+0xEC8 (0xBC bytes) is the computer driver
  (`AIDriver`, T362..T367, u086), not "DynSub_EC8" (u077) or a transmission
  (u079); `F_001F8980` switches the driving line rather than requesting a gear.

## Names

- Lupo 2 vtable slot names were carried over by position, but 72 classes have
  more slots in Lupo 2 than in US, so the names after the inserted slots were
  shifted (u025, u026: `MemorycardFileManagerBase` slots 14..30 belong to
  12..28). Fixed in `names.py` on 2026-10-05 for units u029 onward: a slot
  keeps its Lupo 2 name only when it is inherited from a base whose slot count
  is the same in both builds. Units u000..u028 were written with the old
  names; their agents checked the code, but names there may still be off by a
  slot.

- The Lupo 2 name map (`lupo2` / `lupo2m` names) is shifted by one for runs
  of tiny accessors that Lupo 2 emits in the reverse order (u074: RawList
  0x001D5A70..0x001D5AE0, 0x001D5B88..0x001D5BB8, e.g. "getFocusItem" returns
  the item count). Agents corrected them in their units; the map itself is
  unchanged.
- `0x0027D8FC`, named `memcpy_small` by GT3Hooks' engine_abi_addresses.txt, is
  memcmp (u099). `0x0021D2A0`'s bootstrap name (ImageLoader::Source::GetName)
  only comes from identical two-load code; it is `ProductConfig::idleSeconds`.

- `EventResults` (T425) is 0x5F8 bytes (u097, u098); prize_records_internal.h says 0x6A8.
  GameStatus is 0x35210 bytes; 8 unexplained bytes at +0x307C0.
- `F_00218660` is `getGameStatus` (u098); most units call it by address.

- `runway_query` (0x00227D78) writes its query record up to +0x3F; race_championship_internal.h
  asserts that record is 0x38 bytes (u103).

- Bootstrap matching had one false match: `0x00258290` was matched to the
  bootstrap's `gfx_get_packet_ptr`, but it reads ctx+0x1C4 (`videoMode`) where
  the bootstrap reads +0x118 (u118). `libmatch.py` masked the field offset as a
  relocation; it now rejects matches whose differing load/store offsets look
  like struct fields. No other match was affected.

## Accuracy pass: check first

- u099's SAFER SK encrypt/decrypt (0x0021C820, 0x0021CB38) were written from
  the reference algorithm and checked at key points, not instruction by
  instruction (ranking codes depend on them).

## Names (continued)

- RawList: moveLoop is +0x23C, keepSubListIndex +0x240 (u074); u037..u039
  label +0x240 as moveLoop.
- Placement new (second pass, u136): 0x0027C020 is `operator new[](size_t,
  void *)` and 0x0027C028 `operator new(size_t, void *)` (egcs's `<new>`
  order). `ref/symbols.csv` swaps them (lines 3336/3337), and so does the
  bootstrap's `runtime/new.cc` (0x01019870 / 0x01019878; same code, so the
  bytes still match).

## Decoding gaps in the drafts (fixed for u085 onward, 2026-10-05)

- The R5900 `sqrt.s` keeps its source in `ft` (`fs` = 0). rabbitizer printed
  its 69 uses as `c1 0x…` and m2c reported "unknown instruction"; listings and
  drafts now show `sqrt.s $fd, $ft`.
- spimdisasm emits the R5900 `cvt.w.s` (413 uses) as `.word`; m2c then skipped
  it silently and reported "Read from unset register $fN" where the result was
  used. Drafts for u085+ have the instruction back. Units before u085 were
  written from the listing where those errors showed; worth a check in the
  accuracy pass.

## Toolchain (GCC 14 behaviour build)

- The R5900 GCC 14 target rounds float literals toward zero: `999.9f` becomes
  `0x4479F999`, retail has `0x4479F99A`. (u043)
- It passes float arguments by position (`$f13`, `$f14` for the second and
  third argument after an integer) where retail passes them consecutively from
  `$f12`. Code compiled by both must not call across that boundary. (u043)

## Behaviour seen in hybrid boots (tools/hybrid.py, 2026-10-08)

- Fixed 2026-10-08 (u121; ref/matching/u121/notes.md): the intro movie
  played behind the original with u121 from C. The cause was only the MPEG
  decoder's pixel routines (`mpeg_mb_put_block` 0x00260D48, `mpeg_mc_luma`
  0x00261ED0, `mpeg_mc_chroma` 0x00262610), which were plain C where the
  original uses MMI. They are now the original form (C loops around
  one-instruction MMI asm statements on `u128` variables, byte-exact under
  the matching compiler); GCC 14 and `-DNON_MATCHING` assemble the same
  instructions as whole functions. `hybrid.txt` links u121 from C again.
- The second observation (the original player keeping time with the slow C
  routines) was a property of the hybrid link, not of the C player: with the
  player functions kept original, the original thread's calls stay inside
  u121's original assembly (decode_main, decode_slice, the pixel routines),
  so the C routines never ran. The C player, frame queue and PES code were
  compared with the listing and behave the same; 55 of u121's 65 functions
  are now byte-exact, the others differ in register allocation or scheduling
  only. Dense captures (every 0.25 s, by movie time from "FMV started")
  match the retail run over the whole 30 s with no drift, as a control with
  u121 original does.
- A start-offset caveat for capture comparisons: a bigger core.gt3 starts
  the core later (6.65 s retail, up to 7.7 s with hybrid files) and the movie
  starts 8.63..8.65 s after the core, so compare by movie time ("FMV
  started" in emulog.txt) rather than at fixed capture times.
- The original was built with `-ffast-math` (`max.s` / `min.s` in clamps,
  `sqrt.s` without the `sqrtf` errno call). The GCC 14 flags will need the
  same semantics (`-ffast-math` or `-fno-math-errno` plus `max`/`min`).
- Fixed 2026-10-09: the regression build after 46 matched units crashed at
  the core's start (TLB miss, load from 0x8 in `_GLOBAL_$D$icon_model_size`).
  The function map had given u032's static destructor the address of the
  static constructor 0x0015B9D8 (egcs gives `_GLOBAL_$I$` the line of the
  first object it constructs, `_GLOBAL_$D$` the end of the file: both off
  their markers), so the constructor list ran the destructor. funcmap.py
  now maps a marker that names a symbol of its file, and a file's
  `<TU>_static_init` / `_fini` pair, before the line rule, and notes any
  init / fini mix-up; five other files had their `$D$` on the `__tf`
  address next to it (never run at boot). Boots again, FMV 8.63 s after the
  core.

- Fixed 2026-10-16, three hybrid-link gaps found by a hang at
  PDI1394.IRX after u119's matching pass:
  1. File-static C variables with an original address (u119's RPC objects
     `static RPC g_004A6B00;`, u104's `g_seBanks`, u001/u043/u049/u063's)
     were local symbols, which no link assignment can redirect: the C used
     its own copies, the rest the original's (two SIF RPC client states).
     hybrid.py now makes the `.data` / `.bss` ones global in its copies of the
     C objects, so they take the original copy like every other such
     variable (small-data `$gp` statics keep their own copy).
  2. retarget() took a sizeless symbol (egcs writes no size for a file
     static) as 1 byte, so `&g_seBanks[1]` stayed the C's copy; it now
     reaches to the next symbol or the section end, and an array's
     one-past-the-end bound counts as the array.
  3. retarget() grouped %hi/%lo only as adjacent table entries; the
     linker pairs a HI16 with the next LO16 of the same symbol and lets a
     later LO16 share it. A separated `addiu` stayed section-relative: half
     an original address, half the C's (the crash in
     GTSOUNDINSTRUMENT::transfer). Pairing now follows the linker: 19 more
     references redirected tree-wide.
  Also: a C variable whose name the split's data defines too (u125's
  `heap_object`) is made weak in the C, without a script assignment.

## Behaviour corrections found by the matching pass (step 3)

Matching compares every function with the retail code, so it also finds
places where the first pass's C did something else. Fixed so far (each
unit's notes.md has the details):

- u077 `DynCar_place` 0x001DD210: stores the sector `getContact` found for
  the car body (retail reads it back after the call), not the incoming hint;
  they differ when the hint is wrong and off the road (-1).
- u049 `draw_wave_text`: the alpha clamp in retail's order (a NaN alpha is
  converted as retail does, not clamped to 0).
- u043 RaceOrganization's five flag getters (`getDemonstration` ...) return
  the raw int field as retail, not `!= 0` (no callers).
- u058 T175: the dummy BGM object `g_003521D0` is a real global with a
  compiler-generated constructor, so the GCC 14 build constructs it at
  startup too (nothing registered the hand-written init there before).
- u096 T419: the six `prize_check_*` functions return their PrizeCheck
  through a buffer in `$a0` as retail (the class now has constructors);
  the plain struct came back in `$v0`, which a mixed build's original
  PrizeCarList would not read.
- u104 `music_init` 0x0022B058: installs the VBLANK_E handler through
  `AddIntcHandler` (0x00285940, syscall 0x10's 3-parameter stub) as retail,
  not `AddIntcHandler2` with a NULL argument.
- u104 T469 / T473: the sound-effect banks (`g_roadNoiseBank`, `g_seBanks`)
  and the link's `g_linkRandom` / `g_linkControl` are real globals with
  compiler-generated constructors and destructors (the hand-written
  T469_static_init / T473_static_init are gone), so the GCC 14 build
  constructs them at startup too.
- u103 `runway_query` / `runway_query_contacts`: the plane normalisation is
  retail's `rsqrt.s` (1 / sqrt in one EE instruction), not `sqrt.s` then
  `div.s`, and T457 / T458's clamps are `min.s` / `max.s` (volatile asm)
  as retail, not compare-and-move (they differ for NaN / -0.0); the GCC 14
  build keeps the C forms.
- u111 `loader_thread_main`: reads the command word on every pass as retail
  (the first pass kept a local copy), so a request that arrives right after
  a load skips the idle step in between, as in retail; the inflate window
  and the sound-bank head live on the dynamic stack as retail's.
- u109 T492 / T493 / T496: the two message tables (`g_002F1188`,
  `g_002F12D8`) and the system font `g_002F1968` are real globals with
  compiler-generated constructors and destructors (the hand-written
  T492 / T493 / T496_static_init and _destroy are gone), so the GCC 14 build
  constructs them at startup too.
- u109 `draw_line_quad` 0x0023BEF0: the half width over the length is
  `sqrt.s` then `div.s` as retail (`boot_sqrtf_v`); egcs had folded
  `x / __builtin_sqrtf(y)` into `rsqrt.s` under -ffast-math (other rounding).
- u004 `ArcadeControl::scrollToRow` 0x0010A570 (data pass): the row offset
  is retail's 134.40001f (0x43066667, .lit4 0x0034FA20), not 134.4f
  (0x43066666): one float step, the cross bar's target y.
- u042 `RaceEntryCar::copyFrom`: the sound is assigned as retail
  (CarSound's operator= 0x00179230, EngineSound's operator= copies only the
  header buffer), not copy-constructed in place.  The banks' base words,
  vptr and link stay the target's (bank 0 keeps the link to its own bank 1;
  the old copy took the source car's), in both builds.
