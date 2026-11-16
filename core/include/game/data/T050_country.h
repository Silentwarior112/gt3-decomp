/* T050_country.h - the country setting (translation unit T050, unit u013),
 * 0x00127898..0x001279C8.
 *
 * The country (T516 COUNTRY_*: 0 Japan, 1 USA, 2 Britain, 3 France, 4
 * Germany, 5 Italy, 6 Spain; ? 7 another: the switches have 8 cases, 7 doing
 * what the default does)
 * selects the real-time texts (T516, the country word g_00352FD8), the
 * course data folder (T179 course_set_region: 1 courses/JP, 2 courses/US, 3
 * courses/UK, 4 courses/EU) and the default units (T518, preset country + 1).
 * C++ functions (T050 is a C++ translation unit).  set_units_for_country
 * 0x00127908 is used only inside the unit. */
#ifndef GAME_T050_COUNTRY_H
#define GAME_T050_COUNTRY_H

#include "game/game.h"

/* The course folder of the country (unchanged for 7 and out-of-range
 * values).  Caller: u059. */
void set_course_region(int country);        /* 0x00127898 */
/* The country word, the course folder and the units.  Caller: main (T000),
 * with the saved country after a game is loaded. */
void set_country(int country);              /* 0x00127988 */

#endif /* GAME_T050_COUNTRY_H */
