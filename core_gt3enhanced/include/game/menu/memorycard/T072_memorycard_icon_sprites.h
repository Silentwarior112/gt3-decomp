/* T072_memorycard_icon_sprites.h - gt2k_menu::MemorycardIconSprites, the
 * built-in icon images of the memory card screens (translation unit T072,
 * unit u028), 0x001504D0..0x001507F8.
 *
 * The 14 hidden items of the page's layout named by g_002A6578 (0
 * "icon_new", 1 "icon_game", 2 "icon_replay", 3 "icon_best_replay", 4
 * "icon_setting", 5 "icon_other", 6 "icon_ps1", 7 "icon_damage", 8
 * "icon_unknown", 9 "icon_reading", 10 "icon_license", 11 "icon_demo", 12
 * "icon_blank", 13 "icon_cursor"), then where and how the next one is drawn.
 * Built by the icon views of T069 (SingleIconView +0x104, IconView +0x10C);
 * the files' render (T108) set x .. a and call draw(n) for the icons they
 * have no model for; MemorycardSlot::renderFile (T071) draws 0 over the new
 * file. */
#ifndef GAME_T072_MEMORYCARD_ICON_SPRITES_H
#define GAME_T072_MEMORYCARD_ICON_SPRITES_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (sprites) */

namespace gt2k_menu {
class PageLayout;                           /* T229 (the constructor's argument) */

/* Not polymorphic.  Size 0xE20.  No destructor of its own: the owners'
 * destructors (~SingleIconView, ~IconView) expand the implicit one (the 14
 * Items in reverse order, through their vtables). */
class MemorycardIconSprites {
public:
    /* The layout is passed by value (0x84 bytes; the callers copy it with ld
     * / sd, egcs copies it again on entry).  The 14 Items are built empty,
     * then each gets its image by name; x = y = 0, w = h = 64, colour 1. */
    MemorycardIconSprites(PageLayout layout);   /* 0x001504D0 */
    /* sprites[icon] at (x, y), w x h, colour r, g, b, a (the four corners),
     * then its draw (Item::virtual_2).  Seen: 12 a place beyond the end of
     * a list, 13 the cursor frame. */
    void draw(int icon);                    /* 0x00150700 */
    /* inline (? names): T108's render() functions set x .. a through them
     * (each group's loads come before its stores, the arguments of an
     * inline call) */
    void setPosition(f32 px, f32 py) { x = px; y = py; }
    void setSize(f32 pw, f32 ph) { w = pw; h = ph; }
    void setColor(f32 pr, f32 pg, f32 pb, f32 pa) { r = pr; g = pg; b = pb; a = pa; }

    Item sprites[14];                      /* 0x000 (u027: icons; [1] gives the list icon size) */
    f32  x;                                 /* 0xE00 centre */
    f32  y;                                 /* 0xE04 */
    f32  w;                                 /* 0xE08 */
    f32  h;                                 /* 0xE0C */
    f32  r;                                 /* 0xE10 */
    f32  g;                                 /* 0xE14 */
    f32  b;                                 /* 0xE18 */
    f32  a;                                 /* 0xE1C */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::MemorycardIconSprites, 0xE20);

#endif /* GAME_T072_MEMORYCARD_ICON_SPRITES_H */
