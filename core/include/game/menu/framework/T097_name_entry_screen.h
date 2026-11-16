/* T097_name_entry_screen.h - name_entry, the name entry screen's entry point
 * (translation unit T097, unit u013; the controller class is in
 * T097_name_entry.h, which includes this header where it declared the
 * function).
 *
 * In its own header since u016's data pass (2026-10-17): of the originals,
 * T097 has NameEntryControl's type name, the race modes' T052 does not: it
 * saw name_entry without the class (egcs writes the type name of every
 * class it sees whose virtual functions are all inline). */
#ifndef GAME_T097_NAME_ENTRY_SCREEN_H
#define GAME_T097_NAME_ENTRY_SCREEN_H

#include "game/game.h"

/* The name entry screen; name: at least 16 bytes.  Callers: T052
 * (run_free_run, run_time_trial), T053. */
void name_entry(char *name);                /* 0x00128540 */

#endif /* GAME_T097_NAME_ENTRY_SCREEN_H */
