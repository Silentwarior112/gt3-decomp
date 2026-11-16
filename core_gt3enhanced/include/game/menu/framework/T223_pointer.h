/* T223_pointer.h - gt2k_menu::Pointer (translation unit T223, unit u067).
 *
 * The map pointer (T207 MapButtonList): its position eases to the target
 * (0.3 per frame); it is drawn at disp, its position seen through the view
 * (a zoom about viewX / viewY onto the 640 x 448 screen). */
#ifndef GAME_T223_POINTER_H
#define GAME_T223_POINTER_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

namespace gt2k_menu {
struct MenuLayoutItem;                      /* T229 (pointers only) */
struct MenuTexture;                         /* T213 */

/* vtable 0x00334D10 (8 slots: [3] Object's, [4] Item's, [5] ImageGZ's, [6] /
 * [7] Transitive's).  typeinfo __tfQ29gt2k_menu7Pointer 0x001C0C80 (si, base
 * Item; __si_type_info 0x004B2560).  Size 0x11C (operator new in T207
 * 0x001B4A14). */
class Pointer : public Item {
public:
    Pointer(MenuLayoutItem *item, MenuTexture *tex);  /* 0x001C0A00 position, target and view at the
                                               screen centre (320, 224), zoom 1 */
    /*  [0] +0x0C 0x001C0CD0 the implicit destructor: only ~Item, no vptr store */
    virtual void virtual_1();               /* [1] +0x14 0x001C0B48 Item's; eases x / y to the target,
                                               calcDisp() */
    virtual void virtual_2();               /* [2] +0x1C 0x001C0BA0 Item's at disp */
    void  setView(float zoom, float x, float y); /* 0x001C0AB8 */
    void  calcDisp();                       /* 0x001C0AC8 disp = the position through the view */
    float getDispY() const;                 /* 0x001C0C30 */
    float getDispX() const;                 /* 0x001C0C38 */
    /* Inline in the original: T207 (MapButtonList's constructor, refresh,
     * doControl) has their expansions; 0x001C0C40 / 0x001C0C50 are
     * out-of-line copies with no callers.  The target is stored x first
     * (u063 matching): both copies and T207's refresh are exact only so,
     * although the scheduler puts targetY's store first in each.
     * setPosition is declared first: egcs writes a class's inline members
     * out in the reverse order of their declaration (T223's frame info,
     * 0x002C9E50: setTarget's copy first). */
    void  setPosition(float px, float py)   /* 0x001C0C50 the position and the target */
    {
        x.set(px);
        y.set(py);
        targetX = px;
        targetY = py;
    }
    void  setTarget(float tx, float ty)     /* 0x001C0C40 */
    {
        targetX = tx;
        targetY = ty;
    }

    float targetX;                          /* 0x100 */
    float targetY;                          /* 0x104 */
    float zoom;                             /* 0x108 */
    float viewX;                            /* 0x10C */
    float viewY;                            /* 0x110 */
    float dispX;                            /* 0x114 */
    float dispY;                            /* 0x118 */
};
EGCS_SIZE(gt2k_menu::Pointer, 0x11C);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu7Pointer(void);    /* 0x001C0C80 */
#endif

#endif /* GAME_T223_POINTER_H */
