/* T244_button_data.h - gt2k_menu::ButtonData, the button record of a menu
 * page (.mbl data), split from T244_button_list.h (u007's data pass).
 *
 * The record alone, without ButtonList: a header that defines ButtonList
 * instantiates its List<Button> base, and egcs then writes the type name
 * "Q29gt2k_menut4List1ZQ29gt2k_menu6Button" into every object including
 * it.  Retail's T013 (ArcadeControl::runConnectPage, a ButtonData select
 * buffer on the stack) has no such name: it saw the record only.
 * T244_button_list.h includes this header where the record was, so its
 * includers see the same. */
#ifndef GAME_T244_BUTTON_DATA_H
#define GAME_T244_BUTTON_DATA_H

#include "game/game.h"

namespace gt2k_menu {

/* The button record of a page (.mbl data, T241 / T242), 0x14C bytes: the
 * entries of a page's button list (LinkPage::items) and of a sub list. */
struct ButtonData {
    char         name[0x40];                /* 0x000 the layout element of the button */
    char         help2[0x40];               /* 0x040 the layer shown while focused (showHelpLayer2) */
    char         help[0x40];                /* 0x080 the layer shown while focused (showHelpLayer) */
    char         exec[0x40];                /* 0x0C0 the ExecuteFuncData / ValidFuncData key */
    char         link[0x40];                /* 0x100 the page it leads to */
    u16          unk140;                    /* 0x140 */
    u16          flags;                     /* 0x142 BUTTON_*: & 0xF the action; bit 4 / 5: the sub
                                               list's controlType / loop */
    int          subCount;                  /* 0x144 sub list: the entries */
    ButtonData **subItems;                  /* 0x148 */
};
EGCS_SIZE(gt2k_menu::ButtonData, 0x14C);

/* ButtonData::flags (u064's and u070's names, merged by the sweep). */
enum {
    BUTTON_ACTION_MASK       = 0x000F,      /* what deciding the button does (PageManager::onButton): */
    BUTTON_JUMP              = 0,           /*   to its link (the page closes) */
    BUTTON_POPUP             = 1,           /*   its link opens over the page */
    BUTTON_DECIDE            = 2,           /*   the screen is decided and exits */
    BUTTON_BACK              = 3,           /*   PageManager::back() */
    BUTTON_SUB_LIST          = 4,           /*   a sub list (subCount / subItems): the sub-list buttons
                                               (T267 / T268 / T269); nothing in onButton */
    BUTTON_SUB_CONTROL_SHIFT = 4,           /* bit 4: the sub list's controlType */
    BUTTON_SUB_LOOP_SHIFT    = 5            /* bit 5: the sub list's loop */
};

} /* namespace gt2k_menu */

#endif /* GAME_T244_BUTTON_DATA_H */
