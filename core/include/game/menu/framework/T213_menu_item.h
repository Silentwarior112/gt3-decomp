/* T213_menu_item.h - gt2k_menu::Item, MenuTexture, MenuUV (translation unit
 * T213, unit u065), 0x001B9318..0x001BA970.
 *
 * The image / text element of a menu page.  An Item is built from one
 * layout item of a PageLayout (T229 MenuLayoutItem: u065 / u069's
 * MenuResource, u063 / u067's MenuElement, u066's PicData): its rectangle, an
 * optional TEX1 texture (gzipped when the item flag 0x40000 is set; then
 * inflated onto the menu heap), optional vertex colours and an optional text,
 * which can scroll (text flags 0x10) or appear one character a frame (0x20).
 * The base of the buttons (T206), the arrows (T210), the pointer (T223) and
 * most pictures of the pages.
 */
#ifndef GAME_T213_MENU_ITEM_H
#define GAME_T213_MENU_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T257_menu_image.h"           /* base gt2k_menu::Image */
#include "game/menu/framework/T213_menu_texture.h"         /* gt2k_menu::MenuTexture, MenuUV (plain records; their
                                               own header since u069's data pass) */

namespace gt2k_menu {
struct MenuLayoutItem;                      /* T229 (pointers only) */
struct MenuText;                            /* T229 */
struct OperatorCode;                        /* T217 */
class PageLayout;                           /* T229 */

/* vtable 0x00333E68 (8 slots: [3] Object's 0x001BFF00, [5] Image's
 * 0x001CF358, [6] / [7] Transitive's).  typeinfo __tfQ29gt2k_menu4Item
 * 0x001BA920 (__si_type_info 0x004B24E0, base Image).  Size 0x100 (the data
 * of Button, Arrow, Pointer, ScrollBar start at +0x100). */
class Item : public Image {
public:
    /* res / tex may be NULL (an empty Item: setResource later). */
    Item(MenuLayoutItem *res = 0, MenuTexture *tex = 0);           /* 0x001B98C0 */
    Item(MenuLayoutItem *res, PageLayout *layout);                 /* 0x001B9998 tex: layout->getTexture(res) */
    Item(const char *name, PageLayout *layout);                    /* 0x001B9A80 layout->findHiddenItem(name) */
    Item(const char *name, PageLayout *layout, MenuLayoutItem *pos); /* 0x001B9B58 the same, centred on pos */
    virtual ~Item();                        /* [0] +0x0C 0x001B9C90 frees texBuf and text */
    virtual void virtual_1();               /* [1] +0x14 0x001B9F98 update: Transitive's, then the text
                                               (scroll / typewriter) */
    virtual void virtual_2();               /* [2] +0x1C 0x001BA780 draw: drawImage(), then the text */
    /* Close: script, else (item flags 0x0F000000) the fade-out g_00333E08,
     * else kill() at once. */
    virtual void virtual_4(const OperatorCode *script);  /* [4] +0x2C 0x001BA898 */

    /* res->uv not the whole texture: Image type 2 (TILED), f[0] / f[1] =
     * the extents u1 - u0 / v1 - v0. */
    void setUVExtents(MenuLayoutItem *res); /* 0x001B9318 */
    void setFilter(MenuLayoutItem *res);    /* 0x001B93B8 (F_001B93B8) item flag 0x80000: drawFlags |= 3 (the own render
                                               state, bit 1 = bilinear filtering: TEX1 MMIN / MMAG linear) */
    void setBlend(MenuLayoutItem *res);     /* 0x001B93E0 drawFlags from the item's blend (flags & 0xF0):
                                               0x00 -> 0x01 alpha, 0x10 -> 0x11 additive, 0x20 -> 0x31 */
    void setRect(MenuLayoutItem *res);      /* 0x001B9448 z, centre (y up from 448) and size of res */
    void setVertexColors(MenuLayoutItem *res); /* 0x001B94B0 res->colors (flag 2), else grey 0x80 */
    void fadeIn();                          /* 0x001B9798 alphaRatio 0, script g_00333D80 */
    void appear(MenuLayoutItem *res);       /* 0x001B97C0 fadeIn() if the item fades in (0xF00000), else alphaRatio 1 */
    void setTexture(MenuTexture *tex);      /* 0x001B9800 (inflates a gzipped texture into texBuf) */
    void setResource(MenuLayoutItem *res, MenuTexture *tex);  /* 0x001B9D30 */
    void setResource(const char *name, PageLayout *layout);   /* 0x001B9EA8 nothing if there is no such item */
    void setText(const char *s);            /* 0x001B9F08 own copy on the menu heap; NULL: none */
    void setTextPosition(MenuText *t);      /* 0x001BA090 textX / textY from the anchor (flags & 0xF) */
    void beginText();                       /* 0x001BA2A0 font style, position, colour, clip */
    void endText();                         /* 0x001BA570 */
    void drawImage();                       /* 0x001BA5B8 by the corner type (item flags & 0xF00) */
    void halfTone(bool on);                 /* 0x001BA900 */
    void setColor(u32 rgba);                /* 0x001BA908 textColor */
    f32  get_current_text_y() const;        /* 0x001BA910 */
    f32  get_current_text_x() const;        /* 0x001BA918 */

    f32  textPos;                           /* 0xD8 scroll offset / characters shown */
    f32  textSpeed;                         /* 0xDC scroll step per frame */
    f32  textWidth;                         /* 0xE0 width of the scrolled text */
    MenuLayoutItem *res;                    /* 0xE4 */
    void *texBuf;                           /* 0xE8 the inflated texture (menu heap), NULL if not gzipped */
    char *text;                             /* 0xEC setText (menu heap); NULL: res->text->text */
    u32  textColor;                         /* 0xF0 setColor, RGBA8; 0: res->text->color */
    f32  textX;                             /* 0xF4 */
    f32  textY;                             /* 0xF8 */
    int  half;                              /* 0xFC halfTone: drawn half-toned (a button not available) */
};

/* Only used here: the scripts of fadeIn (g_00333D80: alpha +2.5 / s for
 * 0.4 s, then the operator ends) and of the fade-out of virtual_4
 * (g_00333E08: alpha -6.667 / s for 0.15 s, then kill()), T213 .rodata; the
 * vertex order {3, 2, 0, 1} of res->colors (g_00352568). */

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::MenuTexture, 0x8);
EGCS_SIZE(gt2k_menu::Item, 0x100);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu4Item(void);          /* 0x001BA920 */
extern u32 __tiQ29gt2k_menu4Item[3];        /* 0x004B24E0 its __si_type_info, filled by it (Image's
                                               and Drawable's are in T257 / T259) */
}
#endif

#endif /* GAME_T213_MENU_ITEM_H */
