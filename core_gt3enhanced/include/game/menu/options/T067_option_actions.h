/* T067_option_actions.h - the button actions of the options screen
 * (translation unit T067, unit u022), 0x001425F8..0x00142790.
 *
 * g_002A2038 is the ExecuteFuncData table (T211) OptionControl::begin
 * (T062) gives its PageManager (+0xB18, count g_00351720, ctx the
 * OptionControl): "commit" and "retrieve" (nothing: T062 copies the edited
 * options back itself), "set_default", "set_port_1" / "set_port_2" (and
 * "_jaguar": calibPort 0 / 1, calibWheel 2 / 3, for the USB wheel pages),
 * "stop_music".  The PageManager calls func(ctx, rec, index, layout) for a
 * decided button whose exec name matches.  The actions take the
 * ExecuteFuncData signature (the first pass had (OptionControl *, const
 * char *arg)); only ctx is used.  0x001425F8 and 0x00142608 are empty,
 * unreferenced functions. */
#ifndef GAME_T067_OPTION_ACTIONS_H
#define GAME_T067_OPTION_ACTIONS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ExecuteFuncData (g_002A2038) */

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
class PageLayout;                           /* T229 */
}

/* "set_default": ctx's edited options back to the defaults, applied at
 * once (GameOptions::reset, T428). */
void option_action_set_default(void *ctx, gt2k_menu::ButtonData *rec, int index,
                               gt2k_menu::PageLayout *layout);  /* 0x00142618 */

/* T067 .data, filled by T067_static_init (8 entries, sorted by name). */
extern gt2k_menu::ExecuteFuncData g_002A2038[8];   /* 0x002A2038 */
extern int g_00351720;                      /* 0x00351720 gp-0x62D0 its entries (8) */
void T067_static_init(void);                /* 0x001426A8 (.ctors 0x0028F090) */

#endif /* GAME_T067_OPTION_ACTIONS_H */
