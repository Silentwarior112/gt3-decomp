/* records_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u040,
 * 0x00171CA8..0x001753C0, second pass.
 *
 * Translation units (address order):
 *   T089  the record sources of the records screen: gt2k_menu::RecordsMenuArg
 *         and its subclasses, one per record table of the GameStatus
 *         (game/menu/records/T089_records_menu_args.h; SplitBattleOption:
 *         game/menu/records/T089_split_battle_option.h).
 *   T090  gt2k_menu::RecordsControl and run_records_menu, the records screen
 *         (game/menu/records/T090_records_control.h).
 *   T091  its page handlers and their tables (game/menu/records/T091_records_items.h).
 *   T092  gt2k_menu::RecordsButtonList / RecordsButton, the list of places
 *         (game/menu/records/T092_records_button_list.h).
 *   T093, T094, T114, T095  four GT2KRealTimeText tables and their global
 *         instances: "SETTING", "PRIZE_RT_MSG", "SHORTMENU", "MEMORYCARDTEXT"
 *         (game/data/messages/T093_msg_setting.h, T094_msg_prize_rt.h, T114_msg_shortmenu.h,
 *         T095_msg_memorycard.h).
 *   T188  Color8, an 8-bit RGBA colour (game/menu/framework/T188_color.h).
 *   T115  RaceOption from the game options (game/menu/race_menu/T115_race_option.h).
 *   T116  RaceResultBase, the result sequence of a race mode
 *         (game/menu/records/T116_race_result_base.h).
 *   T117  ResultArcade, the arcade result board (game/menu/records/T117_result_arcade.h).
 *
 * Every class is defined in a canonical header.  Each source file includes
 * the canonical headers it needs itself, in the order that gives retail's
 * type names: egcs writes the type names of the classes a unit sees into its
 * .rodata in the reverse order of the classes' definitions, and retail's
 * name lists say which classes each original file saw
 * (ref/data/u040/notes.md).  This header has the unit's constants only (T089,
 * T090 and T117 include it). */
#ifndef U040_H
#define U040_H

#include "types.h"

/* The places of the record tables (getCount): 12, the licence tests 6. */
enum {
    RECORDS_PLACES         = 12,
    RECORDS_LICENSE_PLACES = 6
};

/* The machine test tables (MachineTestRecords kind): MACHINE_TEST_* (T415). */

/* The records screen's menu heap, on run_records_menu's stack. */
#define RECORDS_HEAP_SIZE 0x80000

/* The menu sound effect of a panel being opened (menu_play_se, T233). */

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

/* ResultArcade: the board opens 5 s into the result sequence (setup's t),
 * the race ends 15 s after the last car reached the goal, the timer runs
 * down 1/60 s a frame. */
#define RESULT_BOARD_OPEN_TIME 5.0f
#define RESULT_END_DELAY       15.0f
#define RESULT_FRAME_TIME      (1.0f / 60.0f)

#endif /* U040_H */
