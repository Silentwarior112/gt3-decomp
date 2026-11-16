/* T063_option_calib_checks.h - the page checks of the options screen
 * (translation unit T063, unit u022), 0x0013FF98..0x00140060.
 *
 * g_002A0A70 is the ValidFuncData table (T211) OptionControl::begin (T062)
 * gives its PageManager (+0xB24, count g_003516F0, ctx the OptionControl).
 * Both entries, "gt_force1/calib_steer" and "gt_force2/calib_steer", are
 * option_check_calib_steer (0x0013FF98, only reached through the table): the
 * wheel calibration page of the port being set up (calibPort) needs a device
 * in USB slot 10 + port; without one menu sound 4 and the page
 * "gt_force<n>/set_gtforce" instead.  As a ValidFuncData function it takes
 * (void *ctx, ButtonData *rec) and returns the page to open, NULL: none (the
 * first pass had (OptionControl *, const PageItem *)). */
#ifndef GAME_T063_OPTION_CALIB_CHECKS_H
#define GAME_T063_OPTION_CALIB_CHECKS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ValidFuncData (g_002A0A70) */

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
}

/* T063 .data, filled by T063_static_init (2 entries, sorted by name). */
extern gt2k_menu::ValidFuncData g_002A0A70[2];  /* 0x002A0A70 */
extern int g_003516F0;                      /* 0x003516F0 gp-0x6300 its entries (2) */
void T063_static_init(void);                /* 0x00140028 (.ctors 0x0028F080) */

#endif /* GAME_T063_OPTION_CALIB_CHECKS_H */
