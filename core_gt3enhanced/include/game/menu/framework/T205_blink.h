/* T205_blink.h - the "blink" element factory (translation unit T205, unit
 * u063), 0x001B3B58..0x001B3BE8.
 *
 * A page hook (T229 MenuHook, by the name "blink" in the screens' hook
 * tables: u006, u012, u021, u022, u034): an Item of the layout item running
 * the operator script g_003329D8 (T205 .rodata, 3 instructions: opFadeColors
 * alpha +10 / s for 0.1 s, opFadeColors alpha -2.5 / s for 0.4 s, opJump -2:
 * the item flashes in and fades out every 0.5 s, for as long as it lives).
 * No direct callers.
 */
#ifndef GAME_T205_BLINK_H
#define GAME_T205_BLINK_H

#include "game/game.h"

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
}

/* A gt2k_menu::MenuHookFn (T229): new Item(item, layout), start(g_003329D8).
 * ctx, parent and p8..p14 are not used (the page attaches the result). */
gt2k_menu::Transitive *create_blink(void *ctx, gt2k_menu::PageLayout *layout,
                                    gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
                                    int p8, int pC, int p10, int p14);  /* 0x001B3B58 */

#endif /* GAME_T205_BLINK_H */
