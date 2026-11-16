/* T072 - gt2k_menu::MemorycardIconSprites, the built-in icon images of the
 * memory card screens.  0x001504D0..0x001507F8.
 *
 * Built by the icon views of T069 (SingleIconView +0x104, IconView +0x10C)
 * from the page's layout.  The files' render (T108) set x .. a and call
 * draw(n) for the icons they have no model for; MemorycardSlot::renderFile
 * (T071) draws MC_SPRITE_NEW over the file to save.  The images
 * (g_002A6578): 0 new, 1 game, 2 replay, 3 best_replay, 4 setting, 5 other,
 * 6 ps1, 7 damage, 8 unknown, 9 reading, 10 license, 11 demo, 12 blank, 13
 * cursor.
 *
 * Includes (data pass): retail's T072 .rodata (0x0031DAB8..0x0031DD80) is
 * the 14 image names (g_002A6578's), then the type names SmartIndexT<int>,
 * OperatorList, List / OperatorHolder / Cell<OperatorHolder>, the
 * GameStatus's garage lists (wheel list first) and Operator last: the
 * GameStatus, then T072's own header (T213's Item: the operator holders),
 * then T251 (SmartIndexT<int>), in that order. */
#include "game/save/T429_game_status.h"          /* (no use: retail's T072 saw the GameStatus) */
#include "game/menu/memorycard/T072_memorycard_icon_sprites.h"
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (by value) */
#include "game/common/T251_smart_index.h"          /* (no use: retail's T072 has SmartIndexT<int>'s name) */
#include "game/menu/memorycard/memorycard_menu_internal.h"

// DATA_START 0x0031DAB8

/* The 14 images' names (.data, in front of the frame info; the names are
 * the first strings of T072's .rodata). */
const char *g_002A6578[MC_SPRITE_COUNT] = {    /* 0x002A6578 */
    "icon_new", "icon_game", "icon_replay", "icon_best_replay", "icon_setting", "icon_other",
    "icon_ps1", "icon_damage", "icon_unknown", "icon_reading", "icon_license", "icon_demo",
    "icon_blank", "icon_cursor"
};

namespace gt2k_menu {

/* layout is passed by value (the callers copy it, and egcs copies it again
 * on entry: the ldl / ldr loop).  The 14 Items are built empty first
 * (Item(NULL, NULL) each; the cleanup destroys the ones built, last first,
 * if one throws), then each gets its image by name. */
// FUNC 0x001504D0 gt2k_menu::MemorycardIconSprites::MemorycardIconSprites
MemorycardIconSprites::MemorycardIconSprites(PageLayout layout)
{
    int i;

    /* implicit: Item (the 14 members built empty) */
    for (i = 0; i < MC_SPRITE_COUNT; i++)
        sprites[i].setResource(g_002A6578[i], &layout);
    /* chained: the constants go through $f registers (swc1), not `sw $zero' */
    x = y = 0.0f;
    w = h = 64.0f;
    r = g = b = a = 1.0f;
}

/* Draws image `icon' at x, y (w x h) in the colour r, g, b, a (all four
 * corners): sets them on the Item (the position at rest: MotionValue::set),
 * then its draw (Item::virtual_2).  Through the inline members
 * Drawable::setRect and Color4f::set on sprites[icon] (retail adds icon * 256
 * + this, the order of an inline call's argument; a local Item pointer gives
 * this + icon * 256); Item hides Drawable's setRect, hence the
 * qualification. */
// FUNC 0x00150700 gt2k_menu::MemorycardIconSprites::draw
void MemorycardIconSprites::draw(int icon)
{
    sprites[icon].Drawable::setRect(x, y, w, h);
    sprites[icon].color[0].set(r, g, b, a);
    sprites[icon].color[1].set(r, g, b, a);
    sprites[icon].color[2].set(r, g, b, a);
    sprites[icon].color[3].set(r, g, b, a);
    sprites[icon].virtual_2();
}

} /* namespace gt2k_menu */
