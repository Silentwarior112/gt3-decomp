/* T059_license_get_entry.h - license_get_screen, the "licence acquired"
 * screen's entry point (translation unit T059, unit u019; the controller
 * classes are in T059_license_get_control.h, which includes this header
 * where it declared the function).
 *
 * In its own header since u017's data pass (2026-10-17): of the originals,
 * T058 / T059 have LicenseGetControl's type name (T056..T061, T099, T104,
 * T105, T107 PrizeControlBase's and ControlBaseWithPageManager's), the
 * caller T053 has none of them: it saw license_get_screen without the
 * classes (egcs writes the type name of every class it sees whose virtual
 * functions are all inline). */
#ifndef GAME_T059_LICENSE_GET_ENTRY_H
#define GAME_T059_LICENSE_GET_ENTRY_H

#include "game/game.h"

/* 0 if the menu heap cannot be allocated, else 1.  licence 0..5.  Caller:
 * T053's run_license_test. */
int  license_get_screen(u32 licence);       /* 0x0013A458 */

#endif /* GAME_T059_LICENSE_GET_ENTRY_H */
