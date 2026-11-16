/* T027_level_button_list.h - gt2k_menu::LevelButtonList /
 * gt2k_menu::GoodiesButtonList and their MenuFuncData table (translation unit
 * T027, unit u005), 0x0010F5C0..0x0010FB40.
 *
 * Two button lists of the arcade menu (ArcadeControl, T010), made by name
 * through the MenuFuncData table g_002925A0 ("menu_goodies", "menu_level"),
 * which ArcadeControl::setup gives its PageManager (T211).  The level list
 * starts on ArcadeControl::level (hard for professional) and writes the
 * decided level back (0 easy, 1 normal, 2 hard, 3 professional); while the
 * buttons 0x2000 and 0x10000 (MENU_BUTTON_L1 / R1, T214) are held its third
 * button (List<Button>::at(2)) shows "pro" / "pro_a" instead of "hard" /
 * "hard_a" (the T027 .sdata strings 0x00350E68..0x00350E80) and decides
 * level 3.
 *
 * The typed list templates Cell<T> / List<T> are in menu_list.h (the first
 * pass had them here). */
/* The MenuFuncData table has a section of its own at the end: T010 (u007)
 * includes only it (T027_TABLE_ONLY).  Retail T010's .rodata has no
 * typeinfo name of ButtonList's classes (List<Button>), and it must see
 * SmartIndexT<int> (T244's index member) only with ArcadeControl, so that
 * ArcadeControl's and SmartIndexT<int>'s names come first (0x0030D9C0). */
#if !defined(GAME_T027_LEVEL_BUTTON_LIST_H) && !defined(T027_TABLE_ONLY)
#define GAME_T027_LEVEL_BUTTON_LIST_H

#include "game/game.h"
#include "game/menu/framework/menu_list.h"                 /* gt2k_menu::Cell<T>, List<T> */
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::MenuFuncData (g_002925A0) */
/* For the inline overrides (only T027 includes this part): */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl::level (decide) */
#include "game/menu/framework/T206_button.h"               /* Button (doControl's third button) */
#include "game/menu/framework/T214_menu_control.h"         /* MenuControl, MENU_BUTTON_L1 / R1 */

namespace gt2k_menu {
class Object;                               /* T220 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct LinkPage;                            /* T241 */
struct ButtonData;                          /* T244 */

/* The buttons of mask held (? an inline of the original): as an inline
 * call the mask is a pseudo set before the virtual getButtons call, so
 * retail keeps 0x10000 (R1) in $s0 across it (LevelButtonList::doControl). */
static inline u32 heldButtons(const MenuControl &ctl, u32 mask)
{
    return ctl.getButtons() & mask;
}

/* Bases: ButtonList (Drawable at +0x00, List<Button> at +0xB0).  vtables
 * 0x0030BA58 (13 slots: [1]..[10] ButtonList's / Drawable's / Object's) and
 * 0x0030BA40 (the List<Button> part, 2 slots, this-delta -0xB0: [1] pList's
 * sort 0x0023C580).  typeinfo __tfQ29gt2k_menu15LevelButtonList 0x0010FA88
 * (class, base list g_00350E90 {ButtonList, public +0}; storage 0x004B14F0).
 * Size 0x130 (operator new in create_level_button_list). */
class LevelButtonList : public ButtonList {
public:
    /* Inline (expanded in create_level_button_list): the buttons of the
     * page's records, the cursor on `level' (2 for 3). */
    LevelButtonList(ArcadeControl *ctl, int level, ButtonData **items, int count,
                    int controlType, int loop, PageLayout *layout)
        : ButtonList(controlType, loop, layout)
    {
        owner = ctl;
        pro = 1;
        relative = 0;
        createButtons(items, count, NULL);
        resetIndex();
        index.set(level < 3 ? level : 2);
    }
    /* The destructor is implicit ([0] +0x0C 0x0010FAE0) and the two
     * overrides inline: the class has no key function, so egcs writes the
     * vtable, these functions, the type name and __tf where the inline
     * constructor is used (T027), after the static initialiser, as retail's
     * T027 has them (0x0010F920..0x0010FB40: decide, doControl, __tf,
     * ~LevelButtonList: inline members in reverse declaration order, __tf,
     * the implicit members; then GoodiesButtonList's, defined later, come
     * first).  Defined out of class (in T027) they would all come before
     * the __tf / implicit groups. */

    /* [12] +0x6C 0x0010F968.  Button 2 (List<Button>::at(2), the "hard"
     * button) shows "pro" / "pro_a" (its active picture) while L1 and R1
     * (0x2000 and 0x10000) are held, else "hard" / "hard_a"; the pictures
     * change only when `pro' changes.  Then ButtonList's control. */
    virtual void doControl(const MenuControl &ctl)
    {
        Button *hard = static_cast<Button *>(static_cast<Cell<Button> *>(at(2)));
        if (hard != NULL) {
            if (heldButtons(ctl, MENU_BUTTON_L1) != 0 && heldButtons(ctl, MENU_BUTTON_R1) != 0) {
                if (pro == 0) {
                    hard->setResource("pro", layout);
                    if (hard->activeItem != NULL)
                        hard->activeItem->setResource("pro_a", layout);
                    pro = 1;
                }
            } else if (pro != 0) {
                hard->setResource("hard", layout);
                if (hard->activeItem != NULL)
                    hard->activeItem->setResource("hard_a", layout);
                pro = 0;
            }
        }
        ButtonList::doControl(ctl);
    }
    /* [11] +0x64 0x0010F920.  ButtonList's decide (the decide callback),
     * then owner->level = the cursor: 0 easy, 1 normal, 2 hard, 3
     * professional when button 2 shows "pro". */
    virtual void decide()
    {
        ButtonList::decide();
        int level = index.value;
        if (level == 2 && pro != 0)
            level = 3;
        owner->level = level;
    }

    ArcadeControl *owner;                   /* 0x128 */
    int            pro;                     /* 0x12C button 2 shows "pro" (1 at first: the first
                                               doControl without the buttons resets it) */
};
EGCS_SIZE(gt2k_menu::LevelButtonList, 0x130);

/* Bases: ButtonList.  vtables 0x0030B9D0 (13 slots) and 0x0030B9B8 (the
 * List<Button> part, 2 slots, this-delta -0xB0).  typeinfo
 * __tfQ29gt2k_menu17GoodiesButtonList 0x0010F868 (class, base list
 * g_00350E90; storage 0x004B14D0).  Size 0x128 (operator new in
 * create_goodies_button_list). */
class GoodiesButtonList : public ButtonList {
public:
    /* Inline (expanded in create_goodies_button_list). */
    GoodiesButtonList(ButtonData **items, int count, int controlType, int loop,
                      PageLayout *layout)
        : ButtonList(controlType, loop, layout)
    {
        relative = 0;
        createButtons(items, count, NULL);
        resetIndex();
    }
    /* The destructor is implicit ([0] +0x0C 0x0010F8C0), doControl inline:
     * no key function, as LevelButtonList. */
    virtual void doControl(const MenuControl &ctl)  /* [12] +0x6C 0x0010F848 ButtonList's */
    {
        ButtonList::doControl(ctl);
    }
};
EGCS_SIZE(gt2k_menu::GoodiesButtonList, 0x128);

} /* namespace gt2k_menu */

/* The MenuFuncData create functions (ctx: the ArcadeControl; rect, parent
 * and index unused): a list of page->items / page->count. */
gt2k_menu::ButtonList *create_level_button_list(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0010F5C0 */
gt2k_menu::ButtonList *create_goodies_button_list(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0010F700 */

void T027_static_init(void);                /* 0x0010F808 (.ctors 0x0028F038) */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu17GoodiesButtonList(void);  /* 0x0010F868 */
void *__tfQ29gt2k_menu15LevelButtonList(void);    /* 0x0010FA88 */
}
#endif

#endif /* GAME_T027_LEVEL_BUTTON_LIST_H */

#ifndef GAME_T027_LEVEL_BUTTON_LIST_TABLE
#define GAME_T027_LEVEL_BUTTON_LIST_TABLE

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::MenuFuncData */

/* T027 .data, filled by T027_static_init: {"menu_goodies",
 * create_goodies_button_list}, {"menu_level", create_level_button_list}. */
extern gt2k_menu::MenuFuncData g_002925A0[2];  /* 0x002925A0 */
extern const int g_00350E88;                /* 0x00350E88 gp-0x6B68 its entries (2; const: see T016_ending_menu.h) */

#endif /* GAME_T027_LEVEL_BUTTON_LIST_TABLE */
