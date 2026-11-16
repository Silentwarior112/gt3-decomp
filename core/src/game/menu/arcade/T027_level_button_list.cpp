/* T027 - gt2k_menu::LevelButtonList / GoodiesButtonList and their
 * MenuFuncData table.  0x0010F5C0..0x0010FB40.
 *
 * Two button lists of the arcade menu, made by name through g_002925A0
 * ("menu_goodies", "menu_level"), which ArcadeControl::setup gives its
 * PageManager.  The level list starts on ArcadeControl::level and writes
 * the decided level back; while L1 and R1 are held its third button shows
 * "pro" instead of "hard" and decides level 3 (professional). */
/* Includes: only what T027 uses, its own header last: egcs writes the
 * typeinfo names of the classes a file sees in the reverse order of their
 * definition, and retail T027's .rodata (0x0030BAC8..) has GoodiesButtonList,
 * LevelButtonList, List<Button>, Cell<Button>, OperatorList ..
 * Cell<OperatorHolder>, SmartIndexT<int>, the garage lists, Operator: the
 * GameStatus first (Operator and the garage classes), ArcadeControl
 * (SmartIndexT<int>), the buttons, the own classes. */
#include "game/save/T429_game_status.h"          /* (seen, not used) */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl::level */
#include "game/menu/framework/T206_button.h"               /* Button (LevelButtonList's third button) */
#include "game/menu/framework/T214_menu_control.h"         /* MenuControl, MENU_BUTTON_L1 / R1 */
#include "game/menu/framework/T241_link_manager.h"         /* LinkPage (the factories' page: items, count) */
#include "game/menu/arcade/T027_level_button_list.h"    /* own translation unit, last */
#include "game/menu/arcade/arcade_course_select_internal.h"

/* The MenuFuncData table (T027's .data, before its frame info), filled by
 * T027_static_init: zeros in the image (egcs writes a C++ global without an
 * initialiser as zeros in .data). */
gt2k_menu::MenuFuncData g_002925A0[2];      /* 0x002925A0 */

/* "menu_level": a LevelButtonList of the page's records (its inline
 * constructor).  The level is read before the allocation.  rect, parent and
 * index are unused. */
// FUNC 0x0010F5C0 create_level_button_list
gt2k_menu::ButtonList *create_level_button_list(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index)
{
    gt2k_menu::ArcadeControl *arcade = static_cast<gt2k_menu::ArcadeControl *>(ctx);
    int level = arcade->level;
    /* Object::operator new(0x130), the inline LevelButtonList(): ButtonList(),
     * createButtons, resetIndex, index.set(level < 3 ? level : 2); if it
     * throws, ~ButtonList, then operator delete. */
    /* implicit: set (index.set in the inline LevelButtonList constructor) */
    /* implicit: createButtons, resetIndex (the inline LevelButtonList constructor) */
    /* implicit: Object::operator delete (if the constructor throws) */
    gt2k_menu::ButtonList *list = new gt2k_menu::LevelButtonList(arcade, level, page->items, page->count,
                                                                 controlType, loop, layout);
    return list;                            /* (a direct `return new' lays the EH blocks out
                                               in another order) */
}

/* "menu_goodies": a GoodiesButtonList of the page's records (its inline
 * constructor).  ctx, rect, parent and index are unused. */
// FUNC 0x0010F700 create_goodies_button_list
gt2k_menu::ButtonList *create_goodies_button_list(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index)
{
    /* Object::operator new(0x128), the inline GoodiesButtonList(): ButtonList(),
     * createButtons, resetIndex; if it throws, ~ButtonList, then operator
     * delete. */
    /* implicit: createButtons, resetIndex (the inline GoodiesButtonList constructor) */
    /* implicit: Object::operator delete (if the constructor throws) */
    gt2k_menu::ButtonList *list = new gt2k_menu::GoodiesButtonList(page->items, page->count,
                                                                   controlType, loop, layout);
    return list;
}

/* Static initialiser (.ctors entry 0x0028F038).  In the original a
 * compiler-generated _GLOBAL_$I$ function filling the table defined here
 * (MenuFuncData objects built by a constructor) through a walking cursor
 * (cp/init.c build_vec_init; see u006's T030_static_init).  The table stays
 * at its original address, so the walk is written out: an exact form, not
 * the original (the same two entries). */
// FUNC 0x0010F808 T027_static_init
void T027_static_init(void)
{
    gt2k_menu::MenuFuncData *p = g_002925A0;

    p->name = "menu_goodies";
    p->create = create_goodies_button_list;
    p++;
    p->name = "menu_level";
    p->create = create_level_button_list;
}

/* The table's entry count (.sdata 0x00350E88, after doControl's strings;
 * read by ArcadeControl::setup, T010). */
const int g_00350E88 = sizeof(g_002925A0) / sizeof(g_002925A0[0]);

/* The classes' virtual functions are inline in game/menu/arcade/T027_level_button_list.h
 * (it says why), their destructors implicit: egcs emits them, the __tf
 * functions and the destructors after the static initialiser,
 * GoodiesButtonList's first (the classes in reverse definition order). */

// FUNC 0x0010F848 gt2k_menu::GoodiesButtonList::doControl
/* inline in game/menu/arcade/T027_level_button_list.h; egcs emits this copy (the
 * vtable's): ButtonList::doControl */
/* implicit: doControl (ButtonList's, in the header's inline body) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated: builds the __class_type_info (base list g_00350E90:
 * ButtonList) on the first call. */
// FUNC 0x0010F868 __tfQ29gt2k_menu17GoodiesButtonList
extern "C" void *__tfQ29gt2k_menu17GoodiesButtonList(void)
{
    if (__tiQ29gt2k_menu17GoodiesButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu17GoodiesButtonList, "Q29gt2k_menu17GoodiesButtonList",
                     g_00350E90, 1);
    }
    return __tiQ29gt2k_menu17GoodiesButtonList;
}
#endif

// FUNC 0x0010F8C0 gt2k_menu::GoodiesButtonList::~GoodiesButtonList
/* implicit in game/menu/arcade/T027_level_button_list.h; egcs emits this copy: both
 * vptrs (0x0030B9D0 / the List<Button> part's 0x0030B9B8), ~ButtonList
 * (flags 0) and, for a deleting call, Object::operator delete */
/* implicit: ~ButtonList (base destructor), operator delete (deleting destructor) */

// FUNC 0x0010F920 gt2k_menu::LevelButtonList::decide
/* inline in game/menu/arcade/T027_level_button_list.h; egcs emits this copy (the
 * vtable's): ButtonList's decide, then owner->level = the cursor (3 for
 * button 2 showing "pro") */
/* implicit: decide (ButtonList's, in the header's inline body) */

// FUNC 0x0010F968 gt2k_menu::LevelButtonList::doControl
/* inline in game/menu/arcade/T027_level_button_list.h; egcs emits this copy (the
 * vtable's): button 2 "hard" <-> "pro" while L1 and R1 are held (the
 * header's heldButtons), then ButtonList's */
/* implicit: at, setResource, doControl (List<Button>::at, Item::setResource,
 * ButtonList::doControl in the header's inline body) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated: builds the __class_type_info (base list g_00350E90:
 * ButtonList) on the first call. */
// FUNC 0x0010FA88 __tfQ29gt2k_menu15LevelButtonList
extern "C" void *__tfQ29gt2k_menu15LevelButtonList(void)
{
    if (__tiQ29gt2k_menu15LevelButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu15LevelButtonList, "Q29gt2k_menu15LevelButtonList",
                     g_00350E90, 1);
    }
    return __tiQ29gt2k_menu15LevelButtonList;
}
#endif

// FUNC 0x0010FAE0 gt2k_menu::LevelButtonList::~LevelButtonList
/* implicit in game/menu/arcade/T027_level_button_list.h; egcs emits this copy: both
 * vptrs (0x0030BA58 / 0x0030BA40), ~ButtonList (flags 0) and, for a
 * deleting call, Object::operator delete */
/* implicit: ~ButtonList (base destructor), operator delete (deleting destructor) */
