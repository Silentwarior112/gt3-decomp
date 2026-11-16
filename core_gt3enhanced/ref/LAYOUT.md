# Source layout

The tree is grouped by what the code is. `include/` mirrors `src/`:
a file's canonical header is at the same path under `include/`
(`src/game/race/core/T121_race_entry.cpp`,
`include/game/race/core/T121_race_entry.h`).

| Folder | What | Sources |
|---|---|---|
| `src/game/` | Gran Turismo 3 itself (Polyphony's game code, C++) | 483 |
| `src/game/app/` | main, boot and the top-level game modes, game objects, transitions | 14 |
| `src/game/menu/framework/` | the menu system: pages, buttons, lists, sliders, windows, menu drawing | 71 |
| `src/game/menu/arcade/` | Arcade mode screens | 22 |
| `src/game/menu/gtmode/` | GT mode screens: garage, shops, trophies, licences, prize cars | 32 |
| `src/game/menu/car_settings/` | the car settings (tuning) screens | 7 |
| `src/game/menu/options/` | the options screens | 9 |
| `src/game/menu/memorycard/` | the memory card (save / load) screens | 14 |
| `src/game/menu/records/` | the records and results screens | 14 |
| `src/game/menu/race_menu/` | menus during a race: the short menu, race options | 10 |
| `src/game/race/core/` | race set-up, organisation, entries, events, replays | 19 |
| `src/game/race/modes/` | the race modes (arcade, single race, time trial, licence, championship, battles) | 13 |
| `src/game/race/display/` | the race HUD: meters, messages, displays | 18 |
| `src/game/race/pause/` | pause and replay menus | 5 |
| `src/game/race/dynamics/` | the dynamics conductors: per-mode physics drivers, rules, lap counting | 27 |
| `src/game/race/ai/` | AI drivers and racing lines | 11 |
| `src/game/car/physics/` | car physics: engine, drivetrain, tyres, suspension, input, assists, collisions | 49 |
| `src/game/car/model/` | car and tyre models and their rendering | 13 |
| `src/game/camera/` | race, replay and menu cameras | 37 |
| `src/game/course/` | course data and environment | 4 |
| `src/game/effects/` | smoke, flares, particles, blur, environment maps | 11 |
| `src/game/sound/` | game sound: cars, ambience, BGM, sound effects | 14 |
| `src/game/data/` | databases and tables: cars, races, GCF configuration; data/messages/: text tables | 32 |
| `src/game/save/` | game status and save data: records, prizes, options, ranking codes | 21 |
| `src/game/common/` | helpers the game shares: smart indices, curves, angle maths, text | 11 |
| `src/game/debug/` | the developer logger | 5 |
| `src/engine/` | Polyphony's PS2 system libraries (C and C++) | 99 |
| `src/engine/pgl/` | the graphics library: GS / VIF / VU0 / VU1 drivers, fonts, printf | 24 |
| `src/engine/gfx/` | VRAM, GS buffers, textures, models, render context | 7 |
| `src/engine/text/` | fonts and text drawing | 4 |
| `src/engine/media/` | MPEG / IPU movie and image decoding | 5 |
| `src/engine/sound/` | the SPU2 sound driver and sequencer | 2 |
| `src/engine/io/` | files: volumes, loaders, host files, ISO 9660, memory card I/O, serialisation | 16 |
| `src/engine/pdi/` | the pdi device interface: CD/DVD, SPU2, IOP RPC, i.LINK | 11 |
| `src/engine/input/` | controllers: pads, ports, USB steering wheels | 10 |
| `src/engine/system/` | threads, locks, the i.LINK session | 3 |
| `src/engine/pdistd/` | pdistd: allocator, heap, printf, random, inflate, hash, bignum / cipher | 17 |
| `src/sdk/` | Sony's PS2 SDK libraries: libkernl, libcdvd, libpad, libmc, libscf, libgraph | 22 |
| `src/runtime/` | compiler and C runtime: crt0 / collect2 (startup), libgcc, libm, newlib, the C++ runtime (cxx) | 39 |

## Names

- `Tnnn_name.cpp`: one original translation unit (an object of the
  original link); nnn is its place in the link order (`ref/tus.json`).
  `Tnnna_name.cpp`: a data-only object linked after Tnnn.
- Library files are named after the library member they rebuild
  (`libpad_part1.c`: an original object split into two files).
- `name_internal.h` next to the sources: declarations a group of
  files shares and nothing else uses (one per work unit, below; files
  in other folders include it by its path from `src/`, the build
  passes `-Isrc`).

## Work units

The first passes cut the program into 143 work units of about 3,500
instructions (`ref/units.json`: address ranges). They still name what
the period-accurate build links from the C (`hybrid.txt`), the notes
of the passes (`ref/matching/<unit>/`, `ref/data/<unit>/`) and the
`--unit` option of the tools. `ref/file_units.json` maps each file to
its unit (`tools/layout.py --files u042`, `--unit <file>`). Until
2026-10-17 the sources lived in one folder per unit, `src/uNNN/`;
notes written before then name those paths (the table gives the new
ones).

| Unit | Range | Files |
|---|---|---|
| u000 | 0x00100000..0x00100140 | `runtime/startup/startup_internal.h`<br>`runtime/startup/collect2_ctors.c`<br>`runtime/startup/crt0.S` |
| u001 | 0x00100140..0x001036D0 | `game/app/app_internal.h`<br>`game/app/T000_main.cpp`<br>`game/app/T001_packet_buffers.cpp`<br>`game/app/T002_render_thread.cpp`<br>`game/app/T003_game_object_list.cpp`<br>`game/app/T004_game_status_holder.cpp`<br>`game/app/T005_transition.cpp`<br>`game/app/T006_file_resolve_retry.cpp`<br>`game/app/T007_game_init.cpp`<br>`game/app/T008_build_switches.cpp`<br>`game/app/T009_race_iop_load_modules.cpp`<br>`game/common/T017_smart_index.cpp`<br>`game/menu/framework/T016_ending_menu.cpp`<br>`game/menu/framework/T033_flash_item.cpp` |
| u002 | 0x001036D0..0x001060B8 | `game/menu/arcade/arcade_select_internal.h`<br>`game/menu/arcade/T018_select_compulsory.cpp`<br>`game/menu/arcade/T019_page_hooks.cpp`<br>`game/menu/arcade/T020_arcade_helpers.cpp`<br>`game/menu/arcade/T034_connect_status.cpp` |
| u003 | 0x001060B8..0x001095A8 | `game/menu/arcade/arcade_car_select_2p_internal.h`<br>`game/menu/arcade/T021_car_select_2p.cpp`<br>`game/menu/arcade/T022_class_select.cpp` |
| u004 | 0x001095A8..0x0010C9E0 | `game/menu/arcade/arcade_car_select_internal.h`<br>`game/menu/arcade/T023_arcade_car_select.cpp`<br>`game/menu/arcade/T035_select_box.cpp` |
| u005 | 0x0010C9E0..0x0010FF90 | `game/menu/arcade/arcade_course_select_internal.h`<br>`game/menu/arcade/T025_arcade_course_select.cpp`<br>`game/menu/arcade/T026_arcade_race_code.cpp`<br>`game/menu/arcade/T027_level_button_list.cpp`<br>`game/menu/arcade/T028_arcade_menu_hooks.cpp`<br>`game/menu/arcade/T029_arcade_movies.cpp`<br>`game/menu/framework/T024_mini_cursor.cpp` |
| u006 | 0x0010FF90..0x00113540 | `game/menu/arcade/arcade_item_factories_internal.h`<br>`game/menu/arcade/T030_arcade_item_factories.cpp` |
| u007 | 0x00113540..0x001168B8 | `game/menu/arcade/arcade_control_internal.h`<br>`game/menu/arcade/T010_arcade_control.cpp`<br>`game/menu/arcade/T011_arcade_render.cpp`<br>`game/menu/arcade/T012_arcade_push_start.cpp`<br>`game/menu/arcade/T013_arcade_ilink.cpp`<br>`game/menu/arcade/T014_arcade_ilink_sync.cpp`<br>`game/menu/arcade/T015_arcade_menu.cpp`<br>`game/menu/arcade/T031_arcade_pages.cpp`<br>`game/menu/framework/T032_circle_gauge.cpp` |
| u008 | 0x001168B8..0x00119778 | `game/menu/gtmode/gtmode_buttons_internal.h`<br>`game/menu/gtmode/T040_oil_change_cylinder.cpp`<br>`game/menu/gtmode/T041_event_status.cpp`<br>`game/menu/gtmode/T042_event_hider.cpp`<br>`game/menu/gtmode/T047_top_button_list.cpp`<br>`game/menu/gtmode/T048_page_cursor_hooks.cpp`<br>`game/menu/gtmode/T049_page_button_checks.cpp` |
| u009 | 0x00119778..0x0011C9C8 | `game/menu/gtmode/gtmode_control_internal.h`<br>`game/menu/gtmode/T043_gtmode_control.cpp`<br>`game/menu/gtmode/T044_gtmode_menu_factories.cpp`<br>`game/menu/gtmode/T045_gtmode_page_checks.cpp` |
| u010 | 0x0011C9C8..0x0011F0E0 | `game/menu/gtmode/garage_internal.h`<br>`game/menu/gtmode/T036_garage.cpp` |
| u011 | 0x0011F0E0..0x001218F0 | (merged into the unit before) |
| u012 | 0x001218F0..0x001258E8 | (merged into the unit before) |
| u013 | 0x001258E8..0x00128D98 | `game/menu/gtmode/gtmode_main_internal.h`<br>`game/data/T050_country.cpp`<br>`game/menu/car_settings/T096_setting_checks.cpp`<br>`game/menu/framework/T051_movie_player.cpp`<br>`game/menu/framework/T097_name_entry.cpp`<br>`game/menu/gtmode/T037_gtmode_actions.cpp`<br>`game/menu/gtmode/T038_gtmode_main.cpp`<br>`game/menu/gtmode/T039_gtmode_messages.cpp`<br>`game/menu/gtmode/T046_gtmode_render.cpp` |
| u014 | 0x00128D98..0x0012C300 | `game/menu/options/option_items_internal.h`<br>`game/menu/arcade/T098_handicap_item.cpp`<br>`game/menu/gtmode/T099_prize_counter.cpp`<br>`game/menu/options/T100_calibration_item.cpp`<br>`game/menu/options/T101_key_config_item.cpp` |
| u015 | 0x0012C300..0x0012F840 | `game/menu/race_menu/shortmenu_internal.h`<br>`game/menu/race_menu/T102_race_menu_helpers.cpp`<br>`game/menu/race_menu/T111_shortmenu_control.cpp`<br>`game/menu/race_menu/T112_shortmenu_render.cpp`<br>`game/menu/race_menu/T113_shortmenu_items.cpp` |
| u016 | 0x0012F840..0x00131410 | `game/app/arcade_race_modes_internal.h`<br>`game/app/T052_arcade_race_modes.cpp` |
| u017 | 0x00131410..0x00134630 | `game/app/gtmode_races_internal.h`<br>`game/app/T053_gtmode_races.cpp`<br>`game/app/T054_split_battle.cpp` |
| u018 | 0x00134630..0x00137840 | `game/debug/logger_internal.h`<br>`game/debug/T055_logger.cpp`<br>`game/menu/gtmode/T104_mini_prize_control.cpp`<br>`game/menu/race_menu/T103_shortmenu_license.cpp` |
| u019 | 0x00137840..0x0013A8D0 | `game/menu/gtmode/trophy_internal.h`<br>`game/menu/gtmode/T056_gtmode_trophy_control.cpp`<br>`game/menu/gtmode/T057_consolation_control.cpp`<br>`game/menu/gtmode/T058_trophy_item.cpp`<br>`game/menu/gtmode/T059_license_get_control.cpp` |
| u020 | 0x0013A8D0..0x0013C8F0 | `game/menu/gtmode/trophy_get_internal.h`<br>`game/menu/gtmode/T060_trophy_get.cpp`<br>`game/menu/gtmode/T105_not_car_get.cpp`<br>`game/menu/gtmode/T106_car_get_camera.cpp` |
| u021 | 0x0013C8F0..0x0013FF98 | `game/menu/gtmode/car_get_internal.h`<br>`game/menu/gtmode/T061_car_get_control.cpp`<br>`game/menu/gtmode/T107_cars_item.cpp`<br>`game/menu/options/T062_option_control.cpp` |
| u022 | 0x0013FF98..0x00142790 | `game/menu/options/options_internal.h`<br>`game/menu/options/T063_option_calib_checks.cpp`<br>`game/menu/options/T064_option_hooks.cpp`<br>`game/menu/options/T065_option_sliders.cpp`<br>`game/menu/options/T066_option_multis.cpp`<br>`game/menu/options/T067_option_actions.cpp` |
| u023 | 0x00142790..0x001447D8 | `game/menu/memorycard/memorycard_file_internal.h`<br>`game/menu/memorycard/T108_memorycard_file.cpp` |
| u024 | 0x001447D8..0x00147870 | (merged into the unit before) |
| u025 | 0x00147870..0x001499B8 | `game/menu/memorycard/memorycard_file_manager_internal.h`<br>`game/menu/memorycard/T109_memorycard_file_manager.cpp` |
| u026 | 0x001499B8..0x0014B610 | `game/menu/memorycard/memorycard_control_internal.h`<br>`game/menu/memorycard/T068_memorycard_control.cpp` |
| u027 | 0x0014B610..0x0014E880 | `game/menu/memorycard/memorycard_items_internal.h`<br>`game/menu/memorycard/T069_memorycard_items.cpp` |
| u028 | 0x0014E880..0x00151AD0 | `game/menu/memorycard/memorycard_menu_internal.h`<br>`game/menu/memorycard/T070_memorycard_actions.cpp`<br>`game/menu/memorycard/T071_memorycard_slots.cpp`<br>`game/menu/memorycard/T072_memorycard_icon_sprites.cpp`<br>`game/menu/memorycard/T073_memorycard_menu.cpp`<br>`game/menu/memorycard/T110_memorycard_dir_cache.cpp` |
| u029 | 0x00151AD0..0x00153B70 | `game/menu/memorycard/memorycard_operation_menu_internal.h`<br>`game/menu/memorycard/T074_memorycard_operation_menu.cpp` |
| u030 | 0x00153B70..0x00156A80 | (merged into the unit before) |
| u031 | 0x00156A80..0x00159EE8 | `game/menu/memorycard/memorycard_game_slots_internal.h`<br>`game/menu/memorycard/T075_memorycard_game_slots.cpp`<br>`game/menu/memorycard/T076_memorycard_dialogs.cpp` |
| u032 | 0x00159EE8..0x0015D448 | `game/menu/memorycard/memorycard_icon_internal.h`<br>`game/app/T079_demo_file_manager.cpp`<br>`game/menu/car_settings/T080_setting_control.cpp`<br>`game/menu/memorycard/T077_memorycard_icon.cpp`<br>`game/menu/memorycard/T078_memorycard_warning.cpp` |
| u033 | 0x0015D448..0x0015FC70 | `game/menu/car_settings/setting_hooks_internal.h`<br>`game/menu/car_settings/T081_setting_hooks.cpp` |
| u034 | 0x0015FC70..0x001638F8 | (merged into the unit before) |
| u035 | 0x001638F8..0x00166F98 | `game/menu/car_settings/setting_sliders_internal.h`<br>`game/menu/car_settings/T082_setting_sliders.cpp` |
| u036 | 0x00166F98..0x00169780 | `game/menu/car_settings/setting_actions_internal.h`<br>`game/menu/car_settings/T083_setting_actions.cpp`<br>`game/menu/car_settings/T084_setting_checks.cpp` |
| u037 | 0x00169780..0x0016C260 | `game/menu/options/play_list_internal.h`<br>`game/debug/T086_logger_top.cpp`<br>`game/menu/options/T085_play_list.cpp` |
| u038 | 0x0016C260..0x0016E640 | `game/debug/logger_windows_internal.h`<br>`game/debug/T087_logger_windows.cpp` |
| u039 | 0x0016E640..0x00171CA8 | `game/debug/logger_course_map_internal.h`<br>`game/debug/T088_logger_course_map.cpp` |
| u040 | 0x00171CA8..0x001753C0 | `game/menu/records/records_internal.h`<br>`game/data/messages/T093_msg_setting.cpp`<br>`game/data/messages/T094_msg_prize_rt.cpp`<br>`game/data/messages/T095_msg_memorycard.cpp`<br>`game/data/messages/T114_msg_shortmenu.cpp`<br>`game/menu/framework/T188_color.cpp`<br>`game/menu/race_menu/T115_race_option.cpp`<br>`game/menu/records/T089_records_menu_args.cpp`<br>`game/menu/records/T090_records_control.cpp`<br>`game/menu/records/T091_records_items.cpp`<br>`game/menu/records/T092_records_button_list.cpp`<br>`game/menu/records/T116_race_result_base.cpp`<br>`game/menu/records/T117_result_arcade.cpp` |
| u041 | 0x001753C0..0x00177108 | `game/menu/records/results_internal.h`<br>`game/menu/race_menu/T193_race_color_select.cpp`<br>`game/menu/records/T118_result_championship.cpp`<br>`game/menu/records/T119_result_implement_records.cpp`<br>`game/menu/records/T120_result_machine_test.cpp`<br>`game/menu/records/T189_result_time_trial.cpp`<br>`game/menu/records/T190_result_free_run.cpp`<br>`game/menu/records/T191_result_link_battle.cpp`<br>`game/menu/records/T192_result_2p_battle.cpp` |
| u042 | 0x00177108..0x0017A038 | `game/race/core/race_entry_internal.h`<br>`game/race/core/T121_race_entry.cpp`<br>`game/race/core/T122_race_event_queue.cpp`<br>`game/race/core/T123_race_setup.cpp`<br>`game/race/core/T124_race_information.cpp` |
| u043 | 0x0017A038..0x0017CF30 | `game/race/core/race_organization_internal.h`<br>`game/race/core/T125_race_organization.cpp`<br>`game/race/core/T126_race_lap_time.cpp`<br>`game/race/core/T127_race_records.cpp`<br>`game/race/core/T128_race_lap_record.cpp`<br>`game/race/core/T129_race_base.cpp` |
| u044 | 0x0017CF30..0x0017E6C0 | `game/race/core/race_ps2_base_internal.h`<br>`game/race/core/T130_race_ps2_base.cpp` |
| u045 | 0x0017E6C0..0x001817F8 | `game/race/core/race_ps2_render_internal.h`<br>`game/race/core/T131_race_ps2_render.cpp`<br>`game/race/core/T132_race_basic.cpp` |
| u046 | 0x001817F8..0x001848A8 | `game/race/modes/race_modes_internal.h`<br>`game/race/modes/T133_race_single_player.cpp`<br>`game/race/modes/T134_race_arcade.cpp`<br>`game/race/modes/T135_race_arcade_demo.cpp`<br>`game/race/modes/T136_race_free_practice.cpp`<br>`game/race/modes/T137_race_split_battle.cpp` |
| u047 | 0x001848A8..0x00187F98 | `game/race/modes/race_battles_internal.h`<br>`game/race/modes/T138_race_link_battle.cpp`<br>`game/race/modes/T139_race_solitaire.cpp` |
| u048 | 0x00187F98..0x0018A798 | `game/race/modes/race_time_trial_internal.h`<br>`game/race/display/T141_race_time_trial_display.cpp`<br>`game/race/display/T143_race_free_run_display.cpp`<br>`game/race/modes/T140_race_time_trial.cpp`<br>`game/race/modes/T142_race_free_run.cpp`<br>`game/race/modes/T144_race_license.cpp`<br>`game/race/modes/T145_mini_prize.cpp` |
| u049 | 0x0018A798..0x0018DD80 | `game/race/display/race_license_internal.h`<br>`game/race/core/T149_race_replay_information.cpp`<br>`game/race/core/T150_serialize_replay_data.cpp`<br>`game/race/display/T146_license_examination_messages.cpp`<br>`game/race/display/T147_race_license_display.cpp`<br>`game/race/modes/T148_race_machine_test.cpp` |
| u050 | 0x0018DD80..0x001909E0 | `game/race/core/race_championship_internal.h`<br>`game/race/core/T152_race_gtmode_information.cpp`<br>`game/race/core/T153_race_course.cpp`<br>`game/race/display/T194_race_display_event.cpp`<br>`game/race/modes/T151_race_championship.cpp` |
| u051 | 0x001909E0..0x00193040 | `game/race/display/race_display_internal.h`<br>`game/race/display/T154_race_display.cpp` |
| u052 | 0x00193040..0x00196490 | `game/race/display/race_split_display_internal.h`<br>`game/race/display/T155_race_split_display.cpp` |
| u053 | 0x00196490..0x00196910 | `game/race/display/race_display_object_internal.h`<br>`game/race/display/T156_race_display_object.cpp` |
| u054 | 0x00196910..0x0019A148 | `game/race/display/race_display_parts_internal.h`<br>`game/race/display/T157_race_display_parts.cpp` |
| u055 | 0x0019A148..0x0019D278 | `game/race/display/race_display_draw_internal.h`<br>`game/effects/T159_flare.cpp`<br>`game/effects/T195_flare_sources.cpp`<br>`game/race/display/T158_race_display_draw.cpp`<br>`game/race/display/T160_race_meters.cpp` |
| u056 | 0x0019D278..0x0019F9F8 | `game/race/display/race_meters_internal.h`<br>`game/effects/T165_particle_manager.cpp`<br>`game/effects/T166_particle_params.cpp`<br>`game/race/display/T161_race_tachometer.cpp`<br>`game/race/display/T162_race_onboard_speedmeter.cpp`<br>`game/race/display/T163_race_boostmeter.cpp`<br>`game/race/display/T164_race_minimap.cpp` |
| u057 | 0x0019F9F8..0x001A2EC8 | `game/race/pause/race_pause_internal.h`<br>`game/car/physics/T167_rigid_body.cpp`<br>`game/race/pause/T168_pause_base.cpp`<br>`game/race/pause/T169_race_pause.cpp` |
| u058 | 0x001A2EC8..0x001A6488 | `game/race/pause/replay_pause_internal.h`<br>`game/camera/T172_course_camera.cpp`<br>`game/effects/T173_race_smoke.cpp`<br>`game/race/core/T174_race_mem_pool.cpp`<br>`game/race/pause/T170_replay_pause.cpp`<br>`game/race/pause/T171_race_pause_with_retry.cpp`<br>`game/sound/T175_race_bgm_base.cpp` |
| u059 | 0x001A6488..0x001A9B90 | `game/sound/car_sound_internal.h`<br>`game/course/T178_course_env.cpp`<br>`game/course/T179_course_data.cpp`<br>`game/sound/T176_car_sound.cpp`<br>`game/sound/T177_race_ambient_sound.cpp` |
| u060 | 0x001A9B90..0x001AC870 | `game/car/model/race_car_model_internal.h`<br>`game/car/model/T180_race_car_model.cpp`<br>`game/car/model/T181_car_lighting.cpp` |
| u061 | 0x001AC870..0x001AFA60 | `game/effects/replay_effects_internal.h`<br>`game/car/model/T184_car_icon_maker.cpp`<br>`game/data/messages/T186_msg_race_display.cpp`<br>`game/data/messages/T187_msg_race_msg.cpp`<br>`game/effects/T182_replay_blur.cpp`<br>`game/effects/T183_race_vignette.cpp`<br>`game/race/display/T185_error_event_view.cpp`<br>`game/race/pause/T196_replay_option.cpp` |
| u062 | 0x001AFA60..0x001B2C20 | `game/menu/race_menu/race_menu_internal.h`<br>`game/common/T266_smart_index_double_range.cpp`<br>`game/data/messages/T201_msg_racemenutext.cpp`<br>`game/menu/framework/T202_menu_sound_timer.cpp`<br>`game/menu/race_menu/T197_race_menu_page.cpp`<br>`game/menu/race_menu/T198_race_menu_button.cpp`<br>`game/menu/race_menu/T199_race_menu_layer.cpp`<br>`game/menu/records/T200_result_board.cpp` |
| u063 | 0x001B2C20..0x001B6250 | `game/menu/framework/menu_buttons_internal.h`<br>`game/data/T203_part_names.cpp`<br>`game/menu/framework/T204_menu_heap.cpp`<br>`game/menu/framework/T205_blink.cpp`<br>`game/menu/framework/T206_button.cpp`<br>`game/menu/framework/T207_map_button_list.cpp`<br>`game/menu/framework/T209_name_button_list.cpp`<br>`game/menu/framework/T210_arrow.cpp`<br>`game/menu/gtmode/T208_wheel_shop_button_list.cpp` |
| u064 | 0x001B6250..0x001B9318 | `game/menu/framework/page_manager_internal.h`<br>`game/menu/framework/T211_page_manager.cpp`<br>`game/menu/framework/T212_menu_car.cpp` |
| u065 | 0x001B9318..0x001BBC58 | `game/menu/framework/menu_item_internal.h`<br>`game/menu/framework/T213_menu_item.cpp`<br>`game/menu/framework/T214_menu_control.cpp`<br>`game/menu/framework/T216_menu_transitive.cpp`<br>`game/menu/gtmode/T215_showroom_data.cpp` |
| u066 | 0x001BBC58..0x001BF390 | `game/menu/framework/operator_internal.h`<br>`game/menu/framework/T217_operator.cpp`<br>`game/menu/framework/T218_color.cpp`<br>`game/menu/gtmode/T219_garage_button_list.cpp` |
| u067 | 0x001BF390..0x001C2318 | `game/menu/framework/menu_widgets_internal.h`<br>`game/menu/framework/T220_object.cpp`<br>`game/menu/framework/T221_callback_item.cpp`<br>`game/menu/framework/T222_decide_effect.cpp`<br>`game/menu/framework/T223_pointer.cpp`<br>`game/menu/framework/T225_menu_item_array.cpp`<br>`game/menu/framework/T226_menu_image_holder.cpp`<br>`game/menu/framework/T227_text_input.cpp`<br>`game/menu/framework/T228_slider_item.cpp`<br>`game/menu/framework/T267_concurrent_button.cpp`<br>`game/menu/gtmode/T224_wheel_button_list.cpp` |
| u068 | 0x001C2318..0x001C5228 | `game/menu/framework/page_layout_internal.h`<br>`game/menu/framework/T229_page_layout.cpp`<br>`game/menu/framework/T230_scroll_bar.cpp`<br>`game/menu/framework/T231_text_box.cpp`<br>`game/menu/framework/T233_menu_sound.cpp`<br>`game/menu/framework/T234_bool_item.cpp`<br>`game/menu/framework/T268_free_button.cpp`<br>`game/menu/framework/T269_multi_button.cpp`<br>`game/menu/gtmode/T232_menu_car_spec.cpp` |
| u069 | 0x001C5228..0x001C7FE0 | `game/menu/framework/menu_links_internal.h`<br>`game/menu/car_settings/T237_gear_chart.cpp`<br>`game/menu/framework/T235_color_box.cpp`<br>`game/menu/framework/T236_free_agent.cpp`<br>`game/menu/framework/T238_slider.cpp`<br>`game/menu/framework/T239_callback_text.cpp`<br>`game/menu/framework/T240_page_history.cpp`<br>`game/menu/framework/T241_link_manager.cpp`<br>`game/menu/framework/T242_link_relocate.cpp`<br>`game/menu/framework/T243_pmb_relocate.cpp` |
| u070 | 0x001C7FE0..0x001CB3F0 | `game/menu/framework/button_list_internal.h`<br>`game/car/model/T245_car_render.cpp`<br>`game/menu/framework/T244_button_list.cpp` |
| u071 | 0x001CB3F0..0x001CE5D8 | `game/menu/framework/render_base_internal.h`<br>`game/common/T251_smart_index.cpp`<br>`game/effects/T254_blur2d.cpp`<br>`game/menu/framework/T246_render_base.cpp`<br>`game/menu/framework/T247_control_base.cpp`<br>`game/menu/framework/T248_menu_signal.cpp`<br>`game/menu/framework/T249_mpeg_image.cpp`<br>`game/menu/framework/T250_ipic.cpp`<br>`game/menu/framework/T252_timer.cpp`<br>`game/menu/framework/T253_menu_bg_movie.cpp`<br>`game/menu/framework/T255_cross.cpp` |
| u072 | 0x001CE5D8..0x001D1AE0 | `game/menu/framework/menu_drawables_internal.h`<br>`game/menu/framework/T256_menu_rectangle.cpp`<br>`game/menu/framework/T257_menu_image.cpp`<br>`game/menu/framework/T258_menu_string.cpp`<br>`game/menu/framework/T259_menu_drawable.cpp`<br>`game/menu/framework/T260_motion_value.cpp` |
| u073 | 0x001D1AE0..0x001D3F70 | `game/menu/framework/menu_draw_state_internal.h`<br>`game/menu/framework/T261_menu_draw_state.cpp`<br>`game/menu/framework/T262_menu_bgm.cpp`<br>`game/menu/framework/T270_raw_animation.cpp`<br>`game/menu/framework/T271_raw_anim_scroll.cpp`<br>`game/menu/framework/T272_raw_window_classes.cpp`<br>`game/menu/framework/T273_raw_label.cpp`<br>`game/menu/framework/T275_raw_check_box.cpp` |
| u074 | 0x001D3F70..0x001D5D60 | `game/menu/framework/raw_list_internal.h`<br>`game/menu/framework/T263_raw_list.cpp`<br>`game/menu/framework/T274_raw_scroll_bar.cpp` |
| u075 | 0x001D5D60..0x001D93C0 | `game/common/curves_internal.h`<br>`game/common/T319_angle_math.cpp`<br>`game/common/T320_record_set.cpp`<br>`game/common/T321_curve_lookup.cpp`<br>`game/common/T322_curve_linear.cpp`<br>`game/common/T323_curve_pair.cpp`<br>`game/common/T354_response_curve.cpp`<br>`game/effects/T276_envmap_render.cpp`<br>`game/effects/T277_dynamic_envmap_camera.cpp`<br>`game/menu/framework/T264_raw_window.cpp`<br>`game/menu/framework/T265_raw_window_register.cpp` |
| u076 | 0x001D93C0..0x001DB220 | `game/car/physics/car_spec_internal.h`<br>`game/car/model/T278_showroom_car.cpp`<br>`game/car/physics/T280_car_condition.cpp`<br>`game/car/physics/T282_car_spec.cpp`<br>`game/car/physics/T283_condition_model.cpp`<br>`game/car/physics/T325_dyn_params.cpp`<br>`game/car/physics/T327_wear_curve.cpp`<br>`game/car/physics/T328_surface_tables.cpp`<br>`game/car/physics/T329_slipstream.cpp`<br>`game/race/display/T279_meter_scale.cpp`<br>`game/race/dynamics/T281_battle_rule_presets.cpp`<br>`game/race/dynamics/T324_dynamics_conductor_rules.cpp`<br>`game/race/dynamics/T326_race_rule_options.cpp` |
| u077 | 0x001DB220..0x001DE5E0 | `game/car/physics/car_param_internal.h`<br>`game/car/physics/T284_car_param.cpp`<br>`game/car/physics/T285_table.cpp`<br>`game/car/physics/T287_surface_grip.cpp`<br>`game/car/physics/T330_dyn_car_init.cpp`<br>`game/car/physics/T331_dyn_car_message.cpp`<br>`game/car/physics/T332_surface_params.cpp`<br>`game/car/physics/T355_setting_check.cpp`<br>`game/race/display/T286_level_colour.cpp` |
| u078 | 0x001DE5E0..0x001E1B28 | `game/car/physics/car_engine_internal.h`<br>`game/car/physics/T289_car_mode.cpp`<br>`game/car/physics/T291_engine.cpp`<br>`game/car/physics/T335_pad_vibration.cpp`<br>`game/car/physics/T336_steer_force.cpp`<br>`game/car/physics/T337_attitude_lag.cpp`<br>`game/car/physics/T338_engine_output.cpp`<br>`game/debug/T290_logger_sectors.cpp`<br>`game/effects/T334_tyre_smoke.cpp`<br>`game/sound/T288_car_sound.cpp`<br>`game/sound/T333_tyre_squeal.cpp` |
| u079 | 0x001E1B28..0x001E4E78 | `game/car/physics/car_input_internal.h`<br>`game/car/physics/T292_gear_calc.cpp`<br>`game/car/physics/T293_drivetrain_spec.cpp`<br>`game/car/physics/T294_gear_auto.cpp`<br>`game/car/physics/T339_input_tuning.cpp`<br>`game/car/physics/T340_assist_control.cpp`<br>`game/car/physics/T341_gear_mode.cpp`<br>`game/car/physics/T342_car_input.cpp`<br>`game/car/physics/T356_brake_assist.cpp`<br>`game/car/physics/T357_traction_control.cpp`<br>`game/car/physics/T358_car_input_auto.cpp` |
| u080 | 0x001E4E78..0x001E6C28 | `game/car/physics/drivetrain_internal.h`<br>`game/car/physics/T295_tyre_heat.cpp`<br>`game/car/physics/T343_gear_select.cpp`<br>`game/car/physics/T344_drivetrain.cpp`<br>`game/car/physics/T345_wheel_shake.cpp` |
| u081 | 0x001E6C28..0x001E9E80 | `game/car/physics/suspension_internal.h`<br>`game/car/physics/T296_car_drivetrain.cpp`<br>`game/car/physics/T297_wheel_steer.cpp`<br>`game/car/physics/T298_suspension_body.cpp`<br>`game/car/physics/T299_wheel_offsets.cpp`<br>`game/car/physics/T346_conductor_car_boxes.cpp`<br>`game/car/physics/T347_ground_surface.cpp`<br>`game/car/physics/T348_wheel_speed_factor.cpp` |
| u082 | 0x001E9E80..0x001ED000 | `game/car/physics/car_motion_internal.h`<br>`game/car/physics/T300_car_tyre_grip.cpp`<br>`game/car/physics/T301_car_motion.cpp`<br>`game/car/physics/T302_car_state.cpp`<br>`game/race/dynamics/T303_race_time_events.cpp`<br>`game/race/dynamics/T304_slow_car_boost.cpp`<br>`game/race/dynamics/T349_car_pairs.cpp` |
| u083 | 0x001ED000..0x001F05F8 | `game/race/dynamics/dynamics_conductor_internal.h`<br>`game/race/dynamics/T305_dynamics_conductor.cpp`<br>`game/race/dynamics/T306_sector_record.cpp`<br>`game/race/dynamics/T307_lap_counter.cpp`<br>`game/race/dynamics/T308_race_progress.cpp`<br>`game/race/dynamics/T309_dynamics_conductor_battle2p.cpp`<br>`game/race/dynamics/T310_dynamics_conductor_battlemp.cpp`<br>`game/race/dynamics/T350_pit_lane.cpp`<br>`game/race/dynamics/T351_car_snapshot.cpp`<br>`game/race/dynamics/T352_license_end.cpp`<br>`game/race/dynamics/T359_stop_in_box.cpp` |
| u084 | 0x001F05F8..0x001F2A10 | `game/race/dynamics/dc_modes_internal.h`<br>`game/race/dynamics/T311_dc_machine_test.cpp`<br>`game/race/dynamics/T312_dc_license.cpp`<br>`game/race/dynamics/T313_dc_free_run.cpp`<br>`game/race/dynamics/T314_dc_time_trial.cpp`<br>`game/race/dynamics/T315_dc_free_practice.cpp`<br>`game/race/dynamics/T316_dc_single_player.cpp` |
| u085 | 0x001F2A10..0x001F5FD8 | `game/race/dynamics/control_record_internal.h`<br>`game/race/dynamics/T317_dynamics_conductor.cpp`<br>`game/race/dynamics/T318_control_record.cpp`<br>`game/race/dynamics/T353_control_frame.cpp`<br>`game/race/dynamics/T360_car_pair_states.cpp`<br>`game/race/dynamics/T361_drive_params.cpp` |
| u086 | 0x001F5FD8..0x001F8BA0 | `game/race/ai/ai_internal.h`<br>`game/race/ai/T362_ai_params.cpp`<br>`game/race/ai/T363_ai_gear.cpp`<br>`game/race/ai/T364_ai_corner_speed.cpp`<br>`game/race/ai/T365_ai_traffic.cpp`<br>`game/race/ai/T366_ai_drive.cpp`<br>`game/race/ai/T367_ai_line.cpp` |
| u087 | 0x001F8BA0..0x001FBCE8 | `game/race/ai/enemy_lines_internal.h`<br>`game/car/physics/T373_car_box_list.cpp`<br>`game/car/physics/T374_car_box.cpp`<br>`game/race/ai/T368_enemy_lines.cpp`<br>`game/race/ai/T369_line_select.cpp`<br>`game/race/ai/T370_pit_line.cpp`<br>`game/race/ai/T371_line_segment.cpp`<br>`game/race/ai/T372_pit_lane.cpp` |
| u088 | 0x001FBCE8..0x001FED38 | `game/car/physics/car_box_internal.h`<br>`game/camera/T397_variable_camera_base.cpp`<br>`game/car/physics/T375_car_box_cars.cpp`<br>`game/car/physics/T376_car_box_walls.cpp` |
| u089 | 0x001FED38..0x00201418 | `game/camera/onboard_camera_internal.h`<br>`game/camera/T377_camera_target_info.cpp`<br>`game/camera/T378_onboard_camera.cpp` |
| u090 | 0x00201418..0x00204B10 | `game/camera/cameras_internal.h`<br>`game/camera/T398_designer_camera.cpp`<br>`game/camera/T399_passline_camera.cpp`<br>`game/camera/T400_starting_camera.cpp`<br>`game/camera/T401_automatic_camera.cpp`<br>`game/camera/T402_automount_camera.cpp`<br>`game/camera/T403_logger_camera.cpp` |
| u091 | 0x00204B10..0x00207768 | `game/camera/camera_motion_internal.h`<br>`game/camera/T379_camera_rot_damper.cpp`<br>`game/camera/T380_camera_spring.cpp`<br>`game/camera/T381_camera_shake.cpp`<br>`game/camera/T382_camera_select.cpp`<br>`game/camera/T404_heli_camera.cpp`<br>`game/camera/T405_matrix_camera.cpp`<br>`game/camera/T406_zoomup_camera.cpp`<br>`game/camera/T407_camera_follow.cpp`<br>`game/camera/T408_car_progress.cpp` |
| u092 | 0x00207768..0x0020AB80 | `game/camera/camera_manager_internal.h`<br>`game/camera/T383_camera_manager.cpp`<br>`game/camera/T384_camera_groove_meter.cpp`<br>`game/camera/T385_camera_demo.cpp` |
| u093 | 0x0020AB80..0x0020E020 | `game/camera/camera_modes_internal.h`<br>`game/camera/T386_camera_bgm_markers.cpp`<br>`game/camera/T387_camera_start.cpp`<br>`game/camera/T388_camera_mode0.cpp`<br>`game/camera/T389_camera_mode2.cpp`<br>`game/camera/T390_camera_mode1.cpp`<br>`game/camera/T391_camera_logger.cpp`<br>`game/camera/T392_camera_target_selector.cpp`<br>`game/camera/T409_camera_change.cpp` |
| u094 | 0x0020E020..0x002104E8 | `game/camera/camera_base_internal.h`<br>`game/camera/T393_target_chooser.cpp`<br>`game/camera/T394_replay_view_mode.cpp`<br>`game/camera/T395_step_pair.cpp`<br>`game/camera/T396_camera_request.cpp`<br>`game/camera/T410_camera_base.cpp`<br>`game/camera/T411_menu_car_camera.cpp`<br>`game/camera/T412_simple_camera_base.cpp` |
| u095 | 0x002104E8..0x00213210 | `game/race/core/race_input_internal.h`<br>`game/race/core/T413_race_input.cpp`<br>`game/race/core/T414_race_random.cpp`<br>`game/save/T415_machine_test_records.cpp`<br>`game/save/T416_free_run_records.cpp` |
| u096 | 0x00213210..0x00216308 | `game/save/prize_records_internal.h`<br>`game/save/T417_time_trial_records.cpp`<br>`game/save/T418_best_place.cpp`<br>`game/save/T419_prize_cars.cpp`<br>`game/save/T420_license_records.cpp`<br>`game/save/T421_prize_set_flags.cpp`<br>`game/save/T422_championship_save.cpp`<br>`game/save/T423_cursor_memory.cpp` |
| u097 | 0x00216308..0x002185D8 | `game/save/game_options_internal.h`<br>`game/save/T424_arcade_records.cpp`<br>`game/save/T425_event_results.cpp`<br>`game/save/T426_controller_config.cpp`<br>`game/save/T427_menu_status.cpp`<br>`game/save/T428_game_options.cpp`<br>`game/save/T434_best_times.cpp` |
| u098 | 0x002185D8..0x0021B790 | `game/save/game_status_internal.h`<br>`game/save/T429_game_status.cpp`<br>`game/save/T430_music_play_list.cpp`<br>`game/save/T431_ranking_code.cpp` |
| u099 | 0x0021B790..0x0021E7D8 | `game/data/gcf_internal.h`<br>`engine/pdistd/T432_bignum.cpp`<br>`engine/pdistd/T433_safer.cpp`<br>`game/data/T435_product_config.cpp`<br>`game/data/T436_gcf_header.cpp`<br>`game/data/T437_gcf_cars.cpp`<br>`game/data/T438_gcf_general.cpp`<br>`game/data/T439_gcf_courses.cpp`<br>`game/data/T440_gcf_table_06.cpp`<br>`game/data/T441_gcf_demos.cpp`<br>`game/data/T442_gcf_prizes.cpp`<br>`game/data/T443_gcf_services.cpp`<br>`game/data/T444_gcf_card_demos.cpp`<br>`game/data/T445_gcf_events.cpp` |
| u100 | 0x0021E7D8..0x00221E08 | `game/car/model/car_model_internal.h`<br>`game/car/model/T446_car_model.cpp`<br>`game/car/model/T447_tire_model_base.cpp`<br>`game/car/model/T448_strip_builder.cpp`<br>`game/car/model/T462_tire_packet.cpp` |
| u101 | 0x00221E08..0x00224600 | `game/car/model/tire_model_internal.h`<br>`game/car/model/T449_race_tire_model.cpp`<br>`game/car/model/T450_menu_tire_model.cpp` |
| u102 | 0x00224600..0x00227378 | `engine/gfx/model_set_internal.h`<br>`engine/gfx/T451_model_set.cpp`<br>`engine/gfx/T452_model_anim.cpp`<br>`engine/gfx/T454_sprite_model.cpp`<br>`game/car/model/T455_car_model_info.cpp`<br>`game/car/model/T456_car_model_file.cpp`<br>`game/course/T453_vision_list.cpp` |
| u103 | 0x00227378..0x0022AA78 | `game/menu/gtmode/prize_model_internal.h`<br>`game/common/T461_matrix_interpolate.cpp`<br>`game/course/T457_runway.cpp`<br>`game/menu/gtmode/T459_prize_model.cpp`<br>`game/menu/gtmode/T460_prize_base.cpp`<br>`game/sound/T458_course_sound.cpp` |
| u104 | 0x0022AA78..0x0022DA98 | `game/sound/gtsound_internal.h`<br>`engine/input/T472_analog_curve.cpp`<br>`engine/pdistd/T471_unit_arena.cpp`<br>`engine/system/T473_link_session.cpp`<br>`game/sound/T463_music_seq.cpp`<br>`game/sound/T464_bgm.cpp`<br>`game/sound/T465_clock.cpp`<br>`game/sound/T466_bgm_ads.cpp`<br>`game/sound/T467_bgm_inf_reloc.cpp`<br>`game/sound/T468_gtsound.cpp`<br>`game/sound/T469_sound_effects.cpp`<br>`game/sound/T470_sound_init.cpp` |
| u105 | 0x0022DA98..0x00231190 | `engine/input/control_internal.h`<br>`engine/input/T474_control.cpp`<br>`engine/input/T475_pad_manager.cpp`<br>`engine/input/T476_port_base.cpp`<br>`engine/input/T477_ps_port.cpp`<br>`engine/input/T478_steering_ps_port.cpp`<br>`engine/input/T479_usb_steering_port.cpp`<br>`engine/input/T480_ieee1394_port.cpp` |
| u106 | 0x00231190..0x00234020 | `game/data/car_equipments_internal.h`<br>`game/data/T481_car_data_base.cpp`<br>`game/data/T482_car_equipments.cpp` |
| u107 | 0x00234020..0x00236600 | (merged into the unit before) |
| u108 | 0x00236600..0x00239ED0 | `game/data/race_database_internal.h`<br>`game/data/T483_car_information.cpp`<br>`game/data/T484_race_database.cpp`<br>`game/data/T485_car_color.cpp`<br>`game/data/T486_maker_names.cpp`<br>`game/data/T487_race_mode_database.cpp`<br>`game/save/T488_setting_serialize.cpp` |
| u109 | 0x00239ED0..0x0023D690 | `game/data/car_database_internal.h`<br>`engine/pdistd/T497_hash.cpp`<br>`engine/text/T496_font.cpp`<br>`engine/text/T498_font_print.cpp`<br>`game/common/T494_text_utils.cpp`<br>`game/data/T489_car_database.cpp`<br>`game/data/messages/T492_msg_database_parts.cpp`<br>`game/data/messages/T493_msg_database_race_category.cpp`<br>`game/menu/framework/T495_menu_list.cpp`<br>`game/save/T490_garage_car.cpp`<br>`game/save/T491_garage_wheel.cpp` |
| u110 | 0x0023D690..0x00240AF0 | `engine/io/vol_internal.h`<br>`engine/io/T500_vol_toc.cpp`<br>`engine/io/T501_memorycard_manager.cpp`<br>`engine/io/T510_vol_device.cpp`<br>`engine/io/T511_inflate_loader.cpp`<br>`engine/text/T499_font_metrics.cpp` |
| u111 | 0x00240AF0..0x00243850 | `engine/io/file_loader_internal.h`<br>`engine/gfx/T512_render_context.cpp`<br>`engine/io/T502_file_resolve.cpp`<br>`engine/io/T503_vol_thread.cpp`<br>`engine/io/T504_file_queue.cpp`<br>`engine/io/T505_host_files.cpp`<br>`engine/io/T506_image_pack.cpp`<br>`engine/io/T507_async_loader.cpp`<br>`engine/io/T508_loader_thread.cpp`<br>`engine/io/T509_serialize.cpp` |
| u112 | 0x00243850..0x00245C10 | `engine/gfx/vram_internal.h`<br>`engine/gfx/T513_vram_allocator.cpp`<br>`engine/gfx/T514_gs_buffer.cpp`<br>`engine/gfx/T515_tex_set.cpp`<br>`engine/text/T517_realtime_text.cpp`<br>`game/data/T516_country.cpp`<br>`game/data/T518_unit_settings.cpp` |
| u113 | 0x00245C10..0x00249B28 | `engine/pgl/pgl_effects_internal.h`<br>`engine/pgl/flare.c`<br>`engine/pgl/particles.c`<br>`engine/pgl/tex1.cpp`<br>`engine/pgl/vu0_math.c` |
| u114 | 0x00249B28..0x0024D8C0 | `engine/pgl/pgl_font_internal.h`<br>`engine/pgl/font.c`<br>`engine/pgl/fontfmt.c`<br>`engine/pgl/fontprint.c`<br>`engine/pgl/format_part1.c`<br>`engine/pgl/render.c`<br>`engine/pgl/texregs.c`<br>`engine/pgl/vecmath.c` |
| u115 | 0x0024D8C0..0x00250A58 | `engine/pgl/pgl_vu1_internal.h`<br>`engine/pgl/format_part2.c`<br>`engine/pgl/gs_driver_core.c`<br>`engine/pgl/vu1_lighting.c`<br>`engine/pgl/vu1_prim.c` |
| u116 | 0x00250A58..0x00254828 | `engine/pgl/gs_display_driver_internal.h`<br>`engine/pgl/gs_display_driver.c` |
| u117 | 0x00254828..0x00257EF4 | `engine/pgl/pgl_internal.h`<br>`engine/pgl/pgl.c`<br>`engine/pgl/vu0_matrix_asm.c` |
| u118 | 0x00257EF4..0x0025B0C8 | `engine/pgl/gs_driver_internal.h`<br>`engine/pgl/color_filter.c`<br>`engine/pgl/gs_context.c`<br>`engine/pgl/gs_driver.c`<br>`engine/pgl/gs_lights.c`<br>`engine/pgl/vu0_current_matrix.c` |
| u119 | 0x0025B0C8..0x0025E548 | `engine/pdi/pdi_internal.h`<br>`engine/io/T525_sector_cache.cpp`<br>`engine/io/T526_iso9660.cpp`<br>`engine/pdi/T519_pdicdvd.cpp`<br>`engine/pdi/T520_pdispu2.cpp`<br>`engine/pdi/T521_pdibgm.cpp`<br>`engine/pdi/T522_pdimpeg.cpp`<br>`engine/pdi/T523_pdipipe.cpp`<br>`engine/pdi/T524_iop_load_module_base.cpp`<br>`engine/pdi/T527_pdi_client_rpc.cpp`<br>`engine/pdi/T528_sif_call_rpc.cpp` |
| u120 | 0x0025E548..0x0025F390 | `engine/media/mpegif_internal.h`<br>`engine/media/T529_mpegif.cpp` |
| u121 | 0x0025F390..0x00262AE0 | `engine/media/mpeg_player_internal.h`<br>`engine/media/mpeg_player.c` |
| u122 | 0x00262AE0..0x00265468 | `engine/media/ipu_internal.h`<br>`engine/media/dma_util.c`<br>`engine/media/ipu.c`<br>`engine/media/ipu_image.c` |
| u123 | 0x00265468..0x00268ED8 | `engine/system/system_drivers_internal.h`<br>`engine/input/pjoy.c`<br>`engine/input/usbd.c`<br>`engine/io/mcard_io.c`<br>`engine/sound/sound_driver.c`<br>`engine/system/scf_lock.c` |
| u124 | 0x00268ED8..0x0026A7C8 | `engine/system/system_util_internal.h`<br>`engine/io/hostfs_io.c`<br>`engine/pdistd/fastmath.c`<br>`engine/pdistd/strlen.S`<br>`engine/sound/sound_seq.c`<br>`engine/system/thread_util.c` |
| u125 | 0x0026A7C8..0x0026CA90 | `engine/pdistd/pdistd_internal.h`<br>`engine/pdistd/T530_allocator.cpp`<br>`engine/pdistd/T530a_printf_tables.cpp`<br>`engine/pdistd/T531_random.cpp`<br>`engine/pdistd/T532_heap.cpp`<br>`engine/pdistd/T533_printf.cpp`<br>`engine/pdistd/T534_sprintf.cpp`<br>`engine/pdistd/T535_vsprintf.cpp`<br>`engine/pdistd/T536_bit_stream_buffer.cpp`<br>`engine/pdistd/T539_print_format.cpp` |
| u126 | 0x0026CA90..0x0026E310 | `engine/pdistd/gzip_inflate_internal.h`<br>`engine/pdistd/T537_gzip_inflate.cpp`<br>`engine/pdistd/T538_fmod_wrap.cpp` |
| u127 | 0x0026E310..0x0026F538 | `engine/pdi/pdi1394_rpc_internal.h`<br>`engine/pdi/pdi1394_rpc.c`<br>`engine/pdi/pdistd_misc.c` |
| u128 | 0x0026F538..0x00270628 | `engine/pdi/pdi1394_internal.h`<br>`engine/pdi/T541_pdi1394.cpp` |
| u129 | 0x00270628..0x00272B40 | `sdk/libcdvd/libcdvd_internal.h`<br>`sdk/libcdvd/libcdvd.c`<br>`sdk/libpad/libpad_part1.c` |
| u130 | 0x00272B40..0x00274AD0 | `sdk/libmc/libmc_internal.h`<br>`sdk/libmc/libmc.c`<br>`sdk/libpad/libpad_part2.c`<br>`sdk/libscf/libscf.c` |
| u131 | 0x00274AD0..0x002776C0 | `runtime/libgcc/libgcc2_part1_internal.h`<br>`runtime/libgcc/libgcc2_part1.c`<br>`runtime/libm/libm.c` |
| u132 | 0x002776C0..0x00278630 | `runtime/libgcc/eh_internal.h`<br>`runtime/libgcc/T542_eh.c` |
| u133 | 0x00278630..0x00279CA8 | `runtime/libgcc/libgcc2_part2_internal.h`<br>`runtime/libgcc/dp_bit.c`<br>`runtime/libgcc/libgcc2_part2.c` |
| u134 | 0x00279CA8..0x0027A400 | `runtime/cxx/exception_internal.h`<br>`runtime/cxx/T543_exception.cpp` |
| u135 | 0x0027A400..0x0027C010 | `runtime/libgcc/fp_bit_internal.h`<br>`runtime/libgcc/fp_bit.c`<br>`runtime/libgcc/frame.c` |
| u136 | 0x0027C010..0x0027D7A0 | `runtime/cxx/cxx_runtime_internal.h`<br>`runtime/cxx/T544_op_delete.cpp`<br>`runtime/cxx/T545_op_new.cpp`<br>`runtime/cxx/T546_op_delete_arr.cpp`<br>`runtime/cxx/T547_op_new_arr.cpp`<br>`runtime/cxx/T548_tinfo.cpp`<br>`runtime/cxx/T549_tinfo2.cpp`<br>`runtime/cxx/T550_new.cpp` |
| u137 | 0x0027D7A0..0x00281088 | `runtime/newlib/newlib_part1_internal.h`<br>`runtime/newlib/sscanf.c`<br>`runtime/newlib/stdio_part1.c`<br>`runtime/newlib/stdlib.c`<br>`runtime/newlib/string.c`<br>`runtime/newlib/vfprintf.c`<br>`runtime/newlib/vfscanf.c` |
| u138 | 0x00281088..0x00283D50 | `runtime/newlib/newlib_part2_internal.h`<br>`runtime/newlib/freer.c`<br>`runtime/newlib/locale.c`<br>`runtime/newlib/mallocr_part1.c`<br>`runtime/newlib/memchr.c`<br>`runtime/newlib/refill.c`<br>`runtime/newlib/stdio_part2.c`<br>`runtime/newlib/strtod.c`<br>`runtime/newlib/strtoul_r.c` |
| u139 | 0x00283D50..0x002869E8 | `runtime/newlib/newlib_part3_internal.h`<br>`runtime/newlib/infconst.c`<br>`runtime/newlib/mallocr_part2.c`<br>`runtime/newlib/mprec.c`<br>`runtime/newlib/readr.c`<br>`runtime/newlib/strtoul.c`<br>`runtime/newlib/syscalls_r.c`<br>`runtime/newlib/ungetc.c`<br>`sdk/libkernl/initsys_glue.c`<br>`sdk/libkernl/kernel.c`<br>`sdk/libkernl/syscalls.S` |
| u140 | 0x002869E8..0x0028A950 | `sdk/libkernl/libkernl_part1_internal.h`<br>`sdk/libkernl/fileio_part1.c`<br>`sdk/libkernl/kprintf.c`<br>`sdk/libkernl/sifcmd.c`<br>`sdk/libkernl/sifrpc.c`<br>`sdk/libkernl/thread.c`<br>`sdk/libkernl/tty.c` |
| u141 | 0x0028A950..0x0028D7C0 | `sdk/libkernl/libkernl_part2_internal.h`<br>`sdk/libkernl/deci2_part1.c`<br>`sdk/libkernl/fileio_part2.c`<br>`sdk/libkernl/initsys.c`<br>`sdk/libkernl/iopheap.c`<br>`sdk/libkernl/iopreboot.c`<br>`sdk/libkernl/loadfile.c` |
| u142 | 0x0028D7C0..0x0028E760 | `sdk/libkernl/deci2_libgraph_internal.h`<br>`engine/pgl/vu0_microcode.c`<br>`runtime/libgcc/libgcc2_part3.c`<br>`sdk/libgraph/libgraph.c`<br>`sdk/libkernl/deci2_part2.c` |
