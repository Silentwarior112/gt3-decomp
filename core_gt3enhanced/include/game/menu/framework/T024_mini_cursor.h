/* T024_mini_cursor.h - gt2k_menu::MiniCursor, the strip of small pictures
 * under the arcade selects (translation unit T024, unit u005),
 * 0x0010C9E0..0x0010DFF0.
 *
 * Built by ArcadeControl::createMiniCursor (T020) over a SmartIndexT<int>
 * (the courses, the classes, the cars), one icon per index.  update
 * (virtual_1) keeps the current icon inside the strip's width (easing at
 * 0.3 a frame, jumping when the index wrapped) and blinks it (1.0 down to
 * 0.25 by 1/15 a frame, restarted when the index changes); draw (virtual_2)
 * places the valid icons, tints the current one, darkens the marked ones
 * (or draws `mark' over them) and draws `cursor' over the current one.
 *
 * The globals the plan gave this header are declared where they lie: the
 * i.LINK connect countdown g_00350FC8 in T010_arcade_control.h, the screen
 * clear words g_0035274C / g_00352750 in T261_menu_draw_state.h. */
#ifndef GAME_T024_MINI_CURSOR_H
#define GAME_T024_MINI_CURSOR_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue scroll */

namespace gt2k_menu {
template <class T> class SmartIndexT;       /* T251 (pointers only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */

/* vtable 0x0030B2A0 (8 slots: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [6] / [7] Transitive's).  typeinfo
 * __tfQ29gt2k_menu10MiniCursor 0x0010DFA0 (si, base Item; storage
 * 0x004B14B0).  Size 0x138 (operator new in ArcadeControl::createMiniCursor). */
class MiniCursor : public Item {
public:
    /* Item(res, NULL); no icons yet (count from the index's range, both
     * arrays on the menu heap, 16-aligned, zeroed); icons 16 x 16, gap 4. */
    MiniCursor(MenuLayoutItem *res, SmartIndexT<int> *list);        /* 0x0010C9E0 */
    virtual ~MiniCursor();                  /* [0] +0x0C 0x0010CB50 deletes the icons, cursor, mark */
    virtual void virtual_1();               /* [1] +0x14 0x0010D050 update: scroll, blink */
    virtual void virtual_2();               /* [2] +0x1C 0x0010D1B8 draw */
    /* Icon i (0 <= i < count) = a new Item of res; the strip takes its size. */
    void setItem(int i, MenuLayoutItem *res, PageLayout *layout);   /* 0x0010CCA0 */
    void setCursor(const char *name, PageLayout *layout);           /* 0x0010CDA8 (no callers) */
    void setMark(const char *name, PageLayout *layout);             /* 0x0010CE50 */
    void setMarked(int i, bool on);                                 /* 0x0010CEF8 (the locked entries) */
    f32  totalWidth();                      /* 0x0010CF28 size() icons and the gaps between */
    f32  widthBefore();                     /* 0x0010CF80 x of the current icon from the start */
    f32  widthAfter();                      /* 0x0010CFD0 the icons after the current one */

    SmartIndexT<int> *list;                 /* 0x100 */
    MotionValue scroll;                     /* 0x104 x of the first icon */
    f32    gap;                             /* 0x110 4.0 */
    f32    itemW;                           /* 0x114 16.0, then the icons' width */
    f32    itemH;                           /* 0x118 16.0, then the icons' height */
    int    lastValue;                       /* 0x11C list->value seen last (-1) */
    f32    blink;                           /* 0x120 1.0 down to 0.25 */
    int    count;                           /* 0x124 list->end - list->begin */
    Item **items;                           /* 0x128 count icons (menu heap) */
    Item  *cursor;                          /* 0x12C drawn over the current icon */
    Item  *mark;                            /* 0x130 drawn over the marked icons */
    bool  *marked;                          /* 0x134 count flags (menu heap; egcs' bool is 4 bytes:
                                               the zeroing loop keeps `count' in a register only
                                               when the flags are not ints) */
};
EGCS_SIZE(gt2k_menu::MiniCursor, 0x138);

} /* namespace gt2k_menu */

/* (newlib's _ctype_, which the plan put here, is declared in G007_stdio.h.) */

/* (g_00350FC8, the "ar/connect" countdown that the plan put here, lies
 * among T010's .sdata words: T010_arcade_control.h.) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu10MiniCursor(void);       /* 0x0010DFA0 */
#endif

#endif /* GAME_T024_MINI_CURSOR_H */
