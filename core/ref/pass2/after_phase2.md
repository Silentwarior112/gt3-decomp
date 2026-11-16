# After phase 2: cross-unit cleanups

Proposals from the units' notes that touched several units at once, so no
single unit could make them.  The sweep (ref/PASS2_SWEEP.md, 2026-10-07) applied
them partition by partition; what each partition changed, renamed and skipped is
in ref/pass2/sweep/s1..s6/notes.md.  Whole tree after it: every unit compiles,
11388/11388 functions claimed, callcheck 21140/21784 (97.0%, no unit below its
count), hdrcheck --all 511/511 (7350 offsets / sizes checked), hdrall 0 errors
with GCC 14 and egcs, vtcheck 0.

Open-items pass (2026-10-08, notes in ref/pass2/open/ and ref/pass2/calls/):
whole tree: 649 files compile, 11411/11411 functions claimed, callcheck
21786/21787 in code alone (the one is the CarSound copy below), hdrcheck,
hdrall (both compilers) and vtcheck clean, egcs compiles 631 of 646 files.

## Still open

- For the matching pass (ref/pass2/open/matching/notes.md): T068 getManager's
  place at its end (after `__tf`: no spelling found that keeps getOtherPort's
  `jal`; four placements tried under egcs); Color8's setter copies in T146
  (T146 calls T188's copies with `jal` yet has its own after
  `__tf12LicensePrize`; inline-at-end, inline-before-use and -fno-inline
  tried, none gives that place); the CarSound copy constructor (0x00179230
  copies only the engine banks' buffer: no C++ constructor spells it; u042's
  TODO(pass2)); T548's own order (inline ctors / implicit dtors of the
  type_info nodes under egcs) and T216's OperatorHolder copies; the
  bootstrap's own pdistd/allocator.h still has reset before repValid; T542
  (`__throw` / `__rethrow`) needs `-fexceptions` and the original statement
  order, so the matching build needs per-file flags
  (ref/pass2/open/egcs/notes.md).

Done:
- `struct stat` moved into G007_stdio.h; u138's and u139's copies deleted (both compile).
- IoStat moved into G002_mpeg_frameq.h; u111's and u124's copies deleted (all G002 users compile).
- PjoyState / PjoyInfo / FfCmd moved into G002_mpeg_frameq.h; u105's and u123's copies deleted (all G002 users compile).
- enum RaceKind moved into T489_car_database.h; u096's / u109's enums and u097's two macros deleted (all compile).
- CARDB_* / PART_* / TIRE_KIND_* / LAYOUT_* moved into T482; u106 / u107 / u108 copies deleted, u109's PART_NONE dropped (all T482 users compile).
- RawWindow events RAW_EVENT_* moved into T264; u074's / u075's copies deleted (u075's RAWWINDOW_EVENT_* renamed).
- ShowRoomCar: T278_show_room_car.h written by u076; u068, u070, u108, u109 switched to it (local copies deleted; calls unchanged).
- T180 drawMenuTires(ShowRoomCar *): declaration, u060's definition and u061's call changed together (casts gone).
- RaceSmoke / RaceAmbientSound: T173 / T177 written (u058 / u059); T130 holds them by value; u044's hand-made construction and the partial copies in u044 / u045 / u050 deleted. Under egcs RacePS2Base's constructor / destructor now match the listing exactly (member order, 10 landing pads).
- enum ArcadeClass moved into T010 (global scope); u003 / u004 / u005 / u007 copies deleted (u001..u007 compile).
- The sweep: shared constant sets moved into their headers (MENU_SE, MC_RESULT, RACE_EVENT, RACE_ALIGN, pause results, replay modes, pad / controller, sound-effect ids, machine-test / ranking kinds, regulation rules, tyre kinds, the dynamics sets, COURSE_KIND in T484, ...), nearly 300 accessors made inline, header signatures fixed, several hundred renames applied (the per-partition lists are in ref/pass2/sweep/s*/notes.md); TODO(pass2) 94 -> 6 (-> 1 after ref/pass2/open/decls).
- ref/funcs.json splits: 28 starts added (ref/extra_starts.txt) and 5 false starts dropped (ref/not_starts.txt); every split function has its own FUNC marker, the false starts' code is merged into its function (u000, u114..u116, u121..u123, u129, u130, u140, u141; 11411/11411 claimed; ref/pass2/open/splits/notes.md).
- Camera slots 23 / 24: setup (Passline / Starting / Automatic / Automount / Matrix / Zoomup), applyMountView (OnBoard / Designer), setupOnRecord (Designer / Heli slot 24); DynamicsConductor slot 38 -> isPitEnabled; CameraManager F_002080E0 .. F_002081A0 -> isCenterTopLeft / TopRight / BottomLeft / BottomRight / isCenterScreen, F_00209FA0 .. FD0 -> getTileFromTop / Bottom / Left / Right (ref/pass2/open/camera/notes.md).
- include/libc.h: `s64 lseek(int, s64, int)` (newlib's 64-bit off_t; __sseek truncates the result with dsll32 / dsra32); u139's definition and _lseek_r changed, its alias deleted (bootstrap keeps its own libc.h; ref/pass2/open/decls/notes.md).
- T428 GameOptions +0x2C: `SplitBattleOption splitBattle` (T089); u014 / u022 / u047 / u097 use its members, u040's cast deleted.
- LINK_STATE_* moved into T473_link_session.h; u047's / u104's copies deleted.
- T482 Curve6 = { Curve6Row x, y } (6-byte records, the listing's lwl / lwr + 2 x lb copies); u076's local type and casts deleted, u076 / u077 index `.v[]`.
- G001 GsMaterial +0x48: union GsMaterialX48 x48 {powTableScale, offset}; T451 `typedef struct GsMaterial ModelMaterial`, T454 holds GsMaterial *; u102's casts deleted, u117 reads x48.powTableScale.
- gt2k_menu::McDateTime and ::McDirEntry moved from T108 into T501 (mc_get_dir fills them); T108 includes T501, u110 no longer includes T108.
- Matching-pass items (ref/pass2/open/matching/notes.md; egcs order checked against the listings): T060 sees PrizeCounter after PrizeControlBase (T059 includes only T056's ControlBaseWithPageManager part); T543 / T548 / T549 / T550 compile under both compilers with egcs emitting their `__tf` (T543's EH declarations skipped after egcs's `<new>`, the `__tf` declarations GCC 14 only; also T305's), u111's T548 workaround gone; T537 heap_clear / setOutputData / inflate inline (#pragma interface / implementation; callers call them); core's pdistd/allocator.h defines reset before repValid; uncalled accessors inline with #pragma interface / implementation in T397, T383, T305, T264, T263, T206, T238 (~Slider implicit), T250, T216, T249, T255, T270, T275, T277.
- callcheck counts code only (`--comments` gives the old count); calls the compiler emits without the source spelling them are named in `/* implicit: ... */` comments (ref/PASS2_CALLS.md). Two rounds over all units: 2 real code fixes (u027 IconView::virtual_1 inlines scrollTarget, u049 draw_wave_text arithmetic), every other callee implicit; the class name no longer stands for a member callee.
- Every file compiles with the original compiler too (SCE ee-gcc 2.9-ee-991111b, assembler included; ref/pass2/open/egcs/notes.md): hex float constants as decimals, egcs spellings under BOOT_OLD_GCC where none fits both, VU0 special registers through include/vu0asm.h. `python tools/build.py --compiler 991111b` builds core.o.
