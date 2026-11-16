/* T090_records_entry.h - run_records_menu, the records screen's entry point
 * (translation unit T090, unit u040; the controller class is in
 * T090_records_control.h, which includes this header where it declared the
 * function).
 *
 * In its own header since u016's data pass (2026-10-17): of the originals,
 * T090..T092 have RecordsControl's type name, the race modes' T052 does
 * not: it saw run_records_menu without the class (egcs writes the type name
 * of every class it sees whose virtual functions are all inline). */
#ifndef GAME_T090_RECORDS_ENTRY_H
#define GAME_T090_RECORDS_ENTRY_H

#include "game/game.h"

namespace gt2k_menu {
class RecordsMenuArg;                       /* T089 (pointers only) */
}

/* The records screen of arg: menu_draw_reset, the menu heap pushed over its
 * frame (sp+0), a RecordsControl (sp+0x80000) run, one vsync before its
 * destructor, the heap popped.  1.  Callers: T052 / T053's short menus. */
int run_records_menu(gt2k_menu::RecordsMenuArg *arg);  /* 0x00172F90 */

#endif /* GAME_T090_RECORDS_ENTRY_H */
