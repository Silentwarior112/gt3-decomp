/* T269_multi_button.h - gt2k_menu::MultiButton (translation unit T269, unit
 * u068).
 *
 * Made by ButtonList::createMultiButton (T244) for a button record named in
 * the layout's multi-button table (PageLayout::findMultiFunc): the table's
 * function gives the changed callback and the initial choice
 * (MultiButtonInfo, T229).  The button carries a ButtonList of the record's
 * sub-entries (the choices), placed at the button every frame; every input
 * frame the callback gets the current choice, then the input goes to the
 * sub list. */
#ifndef GAME_T269_MULTI_BUTTON_H
#define GAME_T269_MULTI_BUTTON_H

#include "game/game.h"
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button */

namespace gt2k_menu {
class ButtonList;                           /* T244 (pointers / references only) */
struct ButtonData;                          /* T244 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct MultiButtonInfo;                     /* T229 */
class MenuControl;                          /* T214 */

/* vtables 0x00335FC0 (18 slots: [2] / [8] / [13..17] Button's, [3] Object's,
 * [4] Item's, [5] ImageGZ's, [6] / [7] Transitive's) and 0x00335FA0
 * (Cell<Button> part, this-delta -0x100, 3 slots).  typeinfo
 * __tfQ29gt2k_menu11MultiButton 0x001C4D20 (__class_type_info 0x004B25E0,
 * base list 0x00352660: Button, public, +0).  Size 0x170 (operator new in
 * T244 0x001C81B0). */
class MultiButton : public Button {
public:
    MultiButton(ButtonList *owner, int controlType, int loop, PageLayout *layout,
                f32 offPoint, ButtonData *linkMenu, MenuLayoutItem *elem, ButtonData **items,
                int count, int subControlType, int subLoop, int relative, int depth, void *ctx,
                const MultiButtonInfo &info); /* 0x001C4978 loop is not used; the sub list
                                               at info->index, its buttons relative (scrolling)
                                               when the first choice's page item is off screen */
    virtual ~MultiButton();                 /* [0] +0x0C 0x001C4B70 */
    virtual void virtual_1();               /* [1] +0x14 0x001C4C38 Button's; the sub list to the button */
    virtual void virtual_9(int visible);    /* [9] +0x54 0x001C4BD0 Button's; the sub list's
                                               hideButtons(visible ^ 1) */
    /* [10] +0x5C 0x001C4C18 the sub list's setDecideFunc (ButtonList, T244) */
    virtual void virtual_10(void *ctx, void (*func)(void *ctx, ButtonData *data, int index,
                                                    ButtonList *list, int a4));
    virtual int  virtual_11(const MenuControl &ctl); /* [11] +0x64 0x001C4C88 changed(ctx, choice),
                                               the sub list's doControl; 0 if decide was pressed */
    virtual void virtual_12();              /* [12] +0x6C 0x001C4CF0 the sub list's decide() */

    void      (*changed)(void *ctx, int index); /* 0x160 */
    int         initialIndex;               /* 0x164 */
    ButtonList *subList;                    /* 0x168 the choices (a child object) */
    void       *ctx;                        /* 0x16C */
};
EGCS_SIZE(gt2k_menu::MultiButton, 0x170);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu11MultiButton(void);   /* 0x001C4D20 */
#endif

#endif /* GAME_T269_MULTI_BUTTON_H */
