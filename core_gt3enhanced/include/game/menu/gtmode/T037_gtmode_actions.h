/* T037_gtmode_actions.h - the ExecuteFuncData table of the GT-mode menu
 * (translation unit T037, unit u013), 0x00125B50..0x00127368; TumbleCar, the
 * car wash, is in T037_tumble_car.h.
 *
 * g_0029A180 (54 entries, sorted by name, filled by T037_static_init) maps
 * the exec names of the GT-mode buttons to their actions.  GtmodeControl::
 * begin (T043) gives it to its PageManager (execute table +0xB18, count
 * g_00351294, ctx the GtmodeControl); PageManager::callExecuteFunc (T211)
 * calls func(ctx, rec, index, layout) for a decided button.  The go_*
 * actions set MenuStatus::nextMode, the mode main() (T000) runs once the menu
 * is left; the others set the menu status's race, car, wheel and part, buy,
 * sell, ride, tune and wash cars, and set the garage's sort order.
 *
 * The actions take the ExecuteFuncData signature (the first pass had
 * (GtmodeControl *ctl, const char *arg)): ctx is the GtmodeControl, the
 * string they read is rec->name (ButtonData +0x00: a race / test / car /
 * wheel / part / course code); index and layout are not used.  The first
 * pass's GtmodeActionEntry (u013) is T211's ExecuteFuncData. */
#ifndef GAME_T037_GTMODE_ACTIONS_H
#define GAME_T037_GTMODE_ACTIONS_H

#include "game/game.h"
#include "game/menu/framework/T220_object.h"               /* (kept for the includers: TumbleCar's base Object) */
#include "game/menu/framework/T212_menu_car.h"             /* (kept for the includers: gt2k_menu::Car) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ExecuteFuncData (g_0029A180) */

namespace gt2k_menu {
class PageManager;                          /* T211 (pointers only) */
class PageLayout;                           /* T229 */
struct ButtonData;                          /* T244 */

typedef ExecuteFuncData GtmodeActionEntry;  /* u013: {name, action}, 8 bytes */

/* gt2k_menu::TumbleCar, the car wash animation, is in game/menu/gtmode/T037_tumble_car.h
 * (split, u013's data pass): a class without a key function, whose type
 * name only T037 has; this header does not include it. */

} /* namespace gt2k_menu */

#include "game/menu/gtmode/T037_gtmode_action_funcs.h"  /* the actions (split: T028 includes only them) */

/* T037 .data 0x0029A180..0x0029A330: {"buy_all_tires", action_buy_all_tires}
 * .. {"wash_car", action_wash_car}.  Filled by T037_static_init (.ctors
 * 0x0028F04C; compiler-generated in the original).  Defined in T037 without
 * an initialiser (zeros before its frame info). */
extern gt2k_menu::ExecuteFuncData g_0029A180[54];  /* 0x0029A180 */
extern int g_00351294;                      /* 0x00351294 gp-0x675C its entries (54) */
void T037_static_init(void);                /* 0x00126C30 */

#endif /* GAME_T037_GTMODE_ACTIONS_H */
