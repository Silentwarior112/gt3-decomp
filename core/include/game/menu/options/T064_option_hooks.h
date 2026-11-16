/* T064_option_hooks.h - the layout object hooks of the options screen
 * (translation unit T064, unit u022), 0x00140060..0x00141E18.
 *
 * g_002A0AD0 maps the object types of the option layouts (sorted for the
 * binary search) to their create functions; OptionControl::begin (T062)
 * gives it to both layouts of its PageManager (PageLayout::setHooks, count
 * g_00351708, ctx the OptionControl).  PageLayout::createObject (T229)
 * calls create(ctx, layout, parent, item, p8, pC, p10, p14).  The entries
 * are the menu library's generic factories with a T064 callback in p8 (the
 * on / off indicators: create_bool_item, T234, with a test on the edited
 * GameOptions; the slider bars create_slider_item, T228, with a range; the
 * texts create_callback_text, T239 0x001C6E70), "blink",
 * the song lists (T085) and T064's own factories of the key configuration
 * pages (KeyConfigItem, T101), the steering assist type, the handicap bar
 * (HandicapItem, T098) and the wheel calibration bars (CalibrationItem,
 * T100).  Those callbacks and factories are only reached through the table;
 * the factories u014 refers to are declared here, with the MenuHookFn
 * signature (the first pass had (OptionControl *, MenuFile *, void *parent,
 * MenuElement *, ...)). */
#ifndef GAME_T064_OPTION_HOOKS_H
#define GAME_T064_OPTION_HOOKS_H

#include "game/game.h"
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (g_002A0AD0), MenuHookFn */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
}

/* "key_config_1": player 1's analog pad page, editable (a KeyConfigItem,
 * operator new 0x1180, of ctx's control1 and pages; mode 1, port 0). */
gt2k_menu::Transitive *option_create_key_config_1(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001404F8 */
/* "handicap_1p" (p8 1) / "handicap_2p" (p8 2): a HandicapItem (0x228) of
 * ctx's options for player p8. */
gt2k_menu::Transitive *option_create_handicap(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00140AC0 */
/* "calib_steer_bar" (p8 0), "calib_center_bar" (1), "calib_accel_bar" (2),
 * "calib_brake_bar" (3): a CalibrationItem (0x2F0) of axis p8 for ctx's
 * calibPort and calibWheel. */
gt2k_menu::Transitive *option_create_calib_bar(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00140B58 */

/* T064 .data, filled by T064_static_init (105 entries, sorted by name;
 * every word of every entry is stored), 0x002A0AD0..0x002A14A8. */
extern gt2k_menu::MenuHook g_002A0AD0[105]; /* 0x002A0AD0 */
extern int g_00351708;                      /* 0x00351708 gp-0x62E8 its entries (105) */
void T064_static_init(void);                /* 0x00140C20 (.ctors 0x0028F084) */

#endif /* GAME_T064_OPTION_HOOKS_H */
