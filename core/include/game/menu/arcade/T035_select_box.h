/* T035_select_box.h - gt2k_menu::SelectBox, the colour chips of the arcade
 * car select (translation unit T035, unit u004), 0x001095A8..0x00109F20.
 *
 * Made by ArcadeControl::createColorChips (T023) over a colour index: one
 * chip per entry of the SmartIndexT<int> (a filled quad in the colour, a
 * one-pixel frame in the box's own colour), scrolled so that the current
 * entry sits at the box's position (scroll eases towards index->value at
 * rate 0.3).  The fills are clipped to the current row, the frames to a
 * 7-row window (1.5 rows above the current one to 5.5 below); arrow_up /
 * arrow_down are drawn while a clamping index can still move that way. */
#ifndef GAME_T035_SELECT_BOX_H
#define GAME_T035_SELECT_BOX_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item, Item arrowUp / arrowDown */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue scroll */

struct PrimVertex;                          /* G001: {x, y, z} (pointers only) */

namespace gt2k_menu {
template <class T> class SmartIndexT;       /* T251 (pointers only) */
class PageLayout;                           /* T229 */

/* vtable 0x0030A620 (8 slots: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [6] / [7] Transitive's).  typeinfo
 * __tfQ29gt2k_menu9SelectBox 0x00109E68 (si, base Item; storage
 * 0x004B14A0).  Size 0x420 (operator new in ArcadeControl::createColorChips).
 * The destructor is implicit in the original: 0x00109EB8 does not store the
 * vptr (egcs stores it in every user-declared destructor, not in the one it
 * generates for a class with one base): ~arrowDown, ~arrowUp, ~Item, then
 * the deleting part. */
class SelectBox : public Item {
public:
    /* Item(image, layout); chip i gets chipColors[i] (0x08000000 past the
     * index's size); spacing 2, both arrows hidden until the first update. */
    SelectBox(const char *image, SmartIndexT<int> *index, const u32 *chipColors,
              PageLayout *layout);          /* 0x001095A8 */
    /*  [0] +0x0C 0x00109EB8 ~SelectBox: implicit (see above) */
    virtual void virtual_1();               /* [1] +0x14 0x00109748 update: scroll, the arrows,
                                               atTop / atBottom unless frozen */
    virtual void virtual_2();               /* [2] +0x1C 0x001097F0 draw: the chips (alpha 0x80), the
                                               arrows */
    /* width x height around (x, y): a 4-vertex strip / the one-pixel
     * outline as a 10-vertex strip. */
    void makeChipQuad(PrimVertex *v, f32 x, f32 y);     /* 0x00109C18 */
    void makeChipFrame(PrimVertex *v, f32 x, f32 y);    /* 0x00109CB8 */
    void setFrozen(bool f);                 /* 0x00109E60 (inline accessor: out-of-line copy, no callers;
                                               bool: a bare sw, an int would need egcs's sltu) */

    SmartIndexT<int> *index;                /* 0x100 */
    u32         colors[64];                 /* 0x104 chip colours RGBA8 (r in the low byte) */
    MotionValue scroll;                     /* 0x204 .value follows index->value */
    f32         spacing;                    /* 0x210 gap between two chips (2.0) */
    Item        arrowUp;                    /* 0x214 "arrow_up" */
    Item        arrowDown;                  /* 0x314 "arrow_down" */
    /* frozen is a bool (4 bytes in egcs): setFrozen stores its argument as
     * is.  virtual_1 keeps idx->clamp in a register across the atTop store:
     * the store's alias set is not clamp's, and since clamp is the bool
     * (T251; u002's updateArrows needs it), atTop / atBottom are ints. */
    bool        frozen;                     /* 0x414 1: atTop / atBottom no longer updated */
    int         atTop;                      /* 0x418 1: arrow_up hidden */
    int         atBottom;                   /* 0x41C 1: arrow_down hidden */
};
EGCS_SIZE(gt2k_menu::SelectBox, 0x420);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu9SelectBox(void);         /* 0x00109E68 */
#endif

#endif /* GAME_T035_SELECT_BOX_H */
