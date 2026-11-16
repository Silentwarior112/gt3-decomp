/* T080_setting_entry.h - setting_menu, the car setting menu's entry point
 * (translation unit T080, unit u032; the controller class is in
 * T080_setting_control.h, which includes this header where it declared the
 * function).
 *
 * In its own header since u016's data pass (2026-10-17): of the originals,
 * T080 and T096 have SettingControl's type name, the race modes' T052 does
 * not: it saw setting_menu without the class (egcs writes the type name of
 * every class it sees whose virtual functions are all inline). */
#ifndef GAME_T080_SETTING_ENTRY_H
#define GAME_T080_SETTING_ENTRY_H

#include "game/game.h"

class RaceOrganization;                     /* T125 (pointers only) */

/* The car setting menu (callers: the race modes of T052 0x0013084C and T053
 * 0x00131DC0 / 0x001331B8 / 0x00133B98; u043 / u108 only name it in
 * comments): SettingControl over a 256 KB menu heap until
 * its pages neither load (1) nor save (2) the setting car's setting on a
 * memory card (F_0014AE00 mode 0x17 / 0x16 with a SettingSerialize; the save
 * gets the licence / test of the current event and a work block of org's
 * course pool).  Nonzero if the menu reported a change or a setting was
 * loaded. */
int setting_menu(u64 course, RaceOrganization *org); /* 0x0015CD60 */

#endif /* GAME_T080_SETTING_ENTRY_H */
