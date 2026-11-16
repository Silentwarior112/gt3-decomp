/* T268_free_button.h - gt2k_menu::FreeButton (translation unit T268, unit
 * u068).
 *
 * Made by ButtonList::createFreeButton (T244) for a button record with
 * sub-entries: the button carries a ButtonList of them (a child object,
 * placed at the button's screen position every frame), which gets the input
 * while the button has it.  When the button loses the focus the sub list's
 * focus is switched off; when it gets it back, the sub list is scrolled back
 * to the position the update saved (three statics shared by every
 * FreeButton: 0x00352648 position, 0x0035264C count, 0x00352650 moved) if
 * its cursor had moved or its count did not grow. */
#ifndef GAME_T268_FREE_BUTTON_H
#define GAME_T268_FREE_BUTTON_H

#include "game/game.h"
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button */

namespace gt2k_menu {
class ButtonList;                           /* T244 (pointers / references only) */
struct ButtonData;                          /* T244 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
class MenuControl;                          /* T214 */

/* vtables 0x00335DA0 (18 slots: [2] / [8] / [15..17] Button's, [3] Object's,
 * [4] Item's, [5] ImageGZ's, [6] / [7] Transitive's) and 0x00335D80
 * (Cell<Button> part, this-delta -0x100, 3 slots).  typeinfo
 * __tfQ29gt2k_menu10FreeButton 0x001C4920 (__class_type_info 0x004B25D0,
 * base list 0x00352658: Button, public, +0).  Size 0x164 (operator new in
 * T244 0x001C82C0). */
class FreeButton : public Button {
public:
    FreeButton(ButtonList *owner, int controlType, int loop, PageLayout *layout,
               f32 offPoint, ButtonData *linkMenu, MenuLayoutItem *elem, ButtonData **items,
               int count, int subControlType, int subLoop, int relative, int depth);               /* 0x001C4550 loop (the owner's) is not used; the sub
                                               list on the button's rectangle, spacing 4; depth 0
                                               starts with the focus */
    virtual ~FreeButton();                  /* [0] +0x0C 0x001C46B0 */
    virtual void virtual_1();               /* [1] +0x14 0x001C4778 Button's; the sub list to the button;
                                               saves its position and count while it has the focus */
    virtual void virtual_9(int visible);    /* [9] +0x54 0x001C4710 Button's; the sub list's
                                               hideButtons(visible ^ 1) */
    /* [10] +0x5C 0x001C4758 the sub list's setDecideFunc (ButtonList, T244) */
    virtual void virtual_10(void *ctx, void (*func)(void *ctx, ButtonData *data, int index,
                                                    ButtonList *list, int a4));
    virtual int  virtual_11(const MenuControl &ctl); /* [11] +0x64 0x001C47F0 the sub list's
                                               doControl (a move sets the moved static); returns 1 */
    virtual void virtual_12();              /* [12] +0x6C 0x001C4870 the sub list's decide() */
    virtual void virtual_13();              /* [13] +0x74 0x001C48A0 the focus comes: scroll back, the
                                               sub list's focus on */
    virtual void virtual_14();              /* [14] +0x7C 0x001C4900 the focus goes: its focus off */

    ButtonList *subList;                    /* 0x160 (a child object) */
};
EGCS_SIZE(gt2k_menu::FreeButton, 0x164);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu10FreeButton(void);    /* 0x001C4920 */
#endif

#endif /* GAME_T268_FREE_BUTTON_H */
