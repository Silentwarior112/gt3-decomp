/* T197_race_menu_page.h - the race menus' common helpers and RaceMenuPage
 * (translation unit T197, unit u062), 0x001AFA60..0x001B02F8.
 *
 * The race menus (the pause menus T169..T171, the replay option T196, the
 * result board T200) run on a 128 KB menu heap of their own (pushed on the
 * menu heap stack, T204), load their layouts from
 * "menu/<country>/<file>.pmb" (gt2k_menu::PageLayout, T229) and draw with a
 * 640 x 448 2D projection that is widened to 852.9 x 448 (the system font
 * squeezed by 0.75) when the first game option word is set (? 16:9).
 *
 * RaceMenuPage (? name, u062): a MenuControl and the PageLayout of one page. */
#ifndef GAME_T197_RACE_MENU_PAGE_H
#define GAME_T197_RACE_MENU_PAGE_H

#include "game/game.h"
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (held by value) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (held by value) */

class GSBuffer;                             /* T514_gs_buffer.h (pointers only) */
namespace gt2k_menu {
class Drawable;                             /* T259_menu_drawable.h */
class Transitive;                           /* T216_menu_transitive.h */
class Item;                                 /* T213_menu_item.h */
}

/* ? name.  An RGBA colour (0..1) of the race menus.  The constructor is
 * inline (ReplayOption's five and ResultBoard's two are set to 0.502 in
 * their constructors). */
struct RaceMenuColor {
    RaceMenuColor() : r(0.502f), g(0.502f), b(0.502f), a(0.502f) {}

    f32 r;                                  /* 0x0 */
    f32 g;                                  /* 0x4 */
    f32 b;                                  /* 0x8 */
    f32 a;                                  /* 0xC */
};
EGCS_SIZE(RaceMenuColor, 0x10);

/* Not polymorphic (its control is).  Size 0x1BC (PauseMenu +0x54,
 * ReplayOption +0x34, ResultBoard's base). */
class RaceMenuPage {
public:
    /* The MenuControl, the layout (owner: this), loaded = 0. */
    RaceMenuPage();                         /* 0x001B01B8 */
    /* unload(), then the control's destructor. */
    ~RaceMenuPage();                        /* 0x001B0220 */
    /* Page `page' of "menu/<country>/<file>.pmb", once.  1 when it is
     * loaded (now or before), 0 on failure. */
    int  load(const char *file, const char *page);  /* 0x001B0270 */
    void unload();                          /* 0x001B02C0 the layout file released, loaded = 0 */

    gt2k_menu::MenuControl control;         /* 0x000 */
    gt2k_menu::PageLayout  layout;          /* 0x134 (u057 / u058 / u061 archive: their ImageArchive) */
    int                    loaded;          /* 0x1B8 */
};
EGCS_SIZE(RaceMenuPage, 0x1BC);

/* The race menu heap: memalign(0x40, 0x20000) once (game init / exit, u001;
 * g_00352398, gp-0x5658). */
void race_menu_heap_alloc(void);            /* 0x001AFA60 */
void race_menu_heap_free(void);             /* 0x001AFA90 */
/* Pushed / popped on the menu heap stack (RacePS2Base's constructor /
 * destructor, u044). */
void race_menu_heap_push(void);             /* 0x001AFAB8 */
void race_menu_heap_pop(void);              /* 0x001AFAD8 */

/* The object of layer `name' of the current page (PageLayout::buildLayer,
 * front 1: its objects attachSorted by z), or an empty Transitive when the
 * page has no such layer. */
gt2k_menu::Transitive *race_menu_build_layer(gt2k_menu::PageLayout *layout,
                                             const char *name);     /* 0x001AFBA0 */
/* A new Item of element `name', attached under root (race_menu_attach). */
gt2k_menu::Item *race_menu_new_item(gt2k_menu::PageLayout *layout, gt2k_menu::Transitive *root,
                                    const char *name, int sorted);  /* 0x001AFC28 */
/* item under root: sorted, among root's children by z (attachSorted); else
 * as root's next sibling (linkAfter: a child of root's parent).  Either way
 * its position is relative to root (anchor = root, anchorType 3); returns
 * item. */
gt2k_menu::Transitive *race_menu_attach(gt2k_menu::Transitive *root, gt2k_menu::Transitive *item,
                                        int sorted);                /* 0x001AFCC8 */
/* Colour `index' of the hidden item `name' (PageLayout::findHiddenItem:
 * "color_mine", ...): its RGBA bytes / 255 * 0.5 into out.  0 when there is
 * no such item (out unchanged). */
int  race_menu_get_color(RaceMenuColor *out, gt2k_menu::PageLayout *layout, const char *name,
                         int index);        /* 0x001AFD28 */
/* The four vertex colours of d = c. */
void race_menu_set_colors(const RaceMenuColor *c, gt2k_menu::Drawable *d);     /* 0x001AFE10 */
/* item's text colour: (2r, 2g, 2b, a) packed. */
void race_menu_set_text_color(const RaceMenuColor *c, gt2k_menu::Item *item);  /* 0x001AFEA8 */
/* A menu draw into buf: the draw area, the 640 x 448 (or widened)
 * orthographic projection, identity model view. */
void race_menu_begin_draw(GSBuffer *buf);   /* 0x001AFF10 */
/* The font back to scale 1 / offset 0, the normal alpha blend
 * (gfx_blend_func 0x44: (Cs - Cd) * As + Cd), blending off (gfx_disable
 * PGL_CAP_BLEND). */
void race_menu_end_draw(void);              /* 0x001B0038 */
/* The scissor rectangle (x, y, w, h) in menu coordinates (squeezed when
 * wide); the whole screen. */
void race_menu_set_scissor(f32 x, f32 y, f32 w, f32 h);    /* 0x001B0158 */
void race_menu_reset_scissor(void);         /* 0x001B0190 */

#endif /* GAME_T197_RACE_MENU_PAGE_H */
