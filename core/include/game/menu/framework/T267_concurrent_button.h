/* T267_concurrent_button.h - gt2k_menu::ConcurrentButton (translation unit
 * T267, unit u067).
 *
 * A button of a ButtonList with a ButtonList of its own, built by
 * ButtonList::createConcurrentButton (T244) from the button record's sub
 * list (record count / items, flags bits 4 / 5).  The sub list is a child of
 * the button, sits on the button's rectangle and follows it; visibility,
 * input and the decision go to the sub list.  The button itself never takes
 * the focus (disableFocus). */
#ifndef GAME_T267_CONCURRENT_BUTTON_H
#define GAME_T267_CONCURRENT_BUTTON_H

#include "game/game.h"
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button */

namespace gt2k_menu {
class ButtonList;                           /* T244 (pointers / references only) */
struct ButtonData;                          /* T244 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
class MenuControl;                          /* T214 */

/* vtables 0x00335388 (18 slots: [2] / [8] / [13..17] Button's, [3] Object's,
 * [4] Item's, [5] ImageGZ's, [6] / [7] Transitive's) and 0x00335368
 * (Cell<Button> part, this-delta -0x100, 3 slots).  typeinfo
 * __tfQ29gt2k_menu16ConcurrentButton 0x001C1F70 (__class_type_info
 * 0x004B2590, base list 0x00352608: Button, public, +0).  Size 0x164
 * (operator new in T244 0x001C83C8). */
class ConcurrentButton : public Button {
public:
    ConcurrentButton(ButtonList *owner, int controlType, int loop, PageLayout *layout,
                     f32 offPoint, ButtonData *linkMenu, MenuLayoutItem *elem,
                     ButtonData **items, int count, int listControlType, int listLoop,
                     int relative, int depth);         /* 0x001C1CC8 loop (the owner's) is not read; the sub
                                               list on the button's rectangle, spacing 4 */
    virtual ~ConcurrentButton();            /* [0] +0x0C 0x001C1DF8 */
    virtual void virtual_1();               /* [1] +0x14 0x001C1EC0 Button's, then the sub list goes to
                                               the button's position */
    virtual void virtual_9(int visible);    /* [9] +0x54 0x001C1E58 Button's; the sub list's
                                               hideButtons(visible ^ 1) */
    /* [10] +0x5C 0x001C1EA0 the sub list's setDecideFunc (ButtonList, T244) */
    virtual void virtual_10(void *ctx, void (*func)(void *ctx, ButtonData *data, int index,
                                                    ButtonList *list, int a4));
    virtual int  virtual_11(const MenuControl &ctl); /* [11] +0x64 0x001C1F10 the sub list's
                                               doControl; returns 1 */
    virtual void virtual_12();              /* [12] +0x6C 0x001C1F40 the sub list's decide() */

    ButtonList *list;                       /* 0x160 the sub list (a child object) */
};
EGCS_SIZE(gt2k_menu::ConcurrentButton, 0x164);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu16ConcurrentButton(void);  /* 0x001C1F70 */
#endif

#endif /* GAME_T267_CONCURRENT_BUTTON_H */
