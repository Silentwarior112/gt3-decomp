/* T057_consolation_control.h - the methods of gt2k_menu::ConsolationControl,
 * the consolation prize screen of the GT mode (translation unit T057, unit
 * u019), 0x00138C48..0x00138FC8.
 *
 * "menu/consolation_prize.mbl", page "consolation_prize": the trophy model,
 * the prize counter, the place.  The class (with its inline constructor and
 * destructor, the vtable and the RTTI, which are T056's) and its T057
 * methods load .. run (0x00138C48..0x00138EE0) are declared in
 * T056_gtmode_trophy_control.h; trophy_prize_screen (T056) builds it. */
#ifndef GAME_T057_CONSOLATION_CONTROL_H
#define GAME_T057_CONSOLATION_CONTROL_H

#include "game/game.h"
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (g_0049BD60) */

namespace gt2k_menu {
class ConsolationControl;                   /* T056 (the class of T057's methods) */
}

/* .ctors entry 0x0028F06C: g_0049BD60 = "count", "count_shadow",
 * "get_money_disp" (T099's _T057 hooks), "rank_disp" (T056
 * create_rank_disp), "render_model" (T058 create_trophy_consolation). */
void T057_static_init(void);                /* 0x00138EE0 */

/* g_0049BD60 (0x0049BD60, .bss) ConsolationControl's item factories: a file
 * static of T057 (retail has it in .bss), defined there (u019's data
 * pass). */

#endif /* GAME_T057_CONSOLATION_CONTROL_H */
