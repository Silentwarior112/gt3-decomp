/* T076 - the dialogs of the memory card screens and their cancel item.
 * 0x00158CD0..0x00159EE8.
 *
 * MemorycardErrorDialog is a MemorycardLayerMenu (layer `name' of
 * menu/memorycard.mbl) with a single item "<name>_m_ok", a
 * MemorycardCancelItem {MC_RESULT_CLOSE}; its kinds add their texts:
 *   MemorycardReadErrorDialog     "readerror" (+ "file damage" or
 *                                 "version error")
 *   MemorycardWriteErrorDialog    "writeerror"
 *   MemorycardNoSpaceDialog       "nospace" (+ "nospace_text_need" with the
 *                                 KB missing)
 *   MemorycardWriteProtectDialog  "writeprotect"
 * MemorycardFormatDialog ("format") asks whether to format the card in a
 * slot; OK (FormatWorkingMenu) formats it under the "formatworking" window.
 * MemorycardCancelItem returns its action code when decided.
 *
 * From 0x00159548: egcs's deferred output, per class (the reverse of the
 * order in which the code first uses their vtables): inline members,
 * typeinfo function, implicit destructor (with MemorycardCancelItem's the
 * unreferenced copy of its inline constructor); then unreferenced copies of
 * MemorycardMenuItem::render and of
 * Cell<MemorycardMenuItem>'s inline destructor and typeinfo function (T073
 * has the ones the vtables use).  `egcs -O2' on this file gives the
 * binary's functions in the binary's order.
 *
 * Matching (u031): FormatWorkingMenu::makeSubMenu is defined before
 * MemorycardFormatDialog's constructor although its code comes last: egcs
 * writes a string literal into .rodata when it expands the function that
 * uses it first, and the binary has the "formatworking" strings before the
 * "format" ones.  The menu results are built by MemorycardMenuResult's
 * constructors (sw; sw; ld from a stack temporary, as in the binary); the
 * items go into the menus through List<T>::append (menu_list.h: the
 * argument is evaluated before the list's `this').
 *
 * Includes (data pass): the original's .rodata has, after T076's own type
 * names (FormatWorkingMenu .. MemorycardCancelItem), T073's
 * (List<MemorycardMenuItem>, MemorycardMenuItem, Cell<MemorycardMenuItem>),
 * MemorycardControl, the garage car list's (GarageCarList, List<GarageCar>,
 * GarageCar, Cell<GarageCar>: no GarageWheelList, T076 did not see the
 * GameStatus), MemorycardFileManagerBase with List / Cell<MemorycardFile>,
 * SmartIndexT<int> and the operator holders (T216), Operator last
 * (0x0031FE58..0x00320280), T074's order.  The first five headers complete
 * those classes in the reverse of that order. */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (MemorycardLayerMenu::root);
                                               OperatorHolder, OperatorList */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager; SmartIndexT<int> */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* MemorycardFileManagerBase, List / Cell<MemorycardFile> */
#include "game/save/T429_garage_car_list.h"      /* GarageCarList, List / Cell<GarageCar>, GarageCar
                                               (type names only) */
#include "game/menu/memorycard/T075_memorycard_game_slots.h"    /* T068 (gt2k_menu::MemorycardControl), T073 (the
                                               menu classes, MemorycardMenuResult) and T076's
                                               header (the dialogs, MemorycardCancelItem) */
/* the rest */
#include "libmc.h"                          /* sceMcTypePS2 (mc_slot_type) */
#include <stddef.h>
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (setText) */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (decide) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (kill) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuText */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T495_menu_list.h"            /* gt2k_menu::pList::append */
#include "engine/io/T501_memorycard_manager.h"   /* mc_slot_type, mc_format */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/memorycard/memorycard_game_slots_internal.h"                           /* the unit's constants, the RTTI storage (GCC 14) */

using namespace gt2k_menu;

namespace gt2k_menu {

/* Decided: sound 1, {action, 0}; else {0, 0}. */
// FUNC 0x00158CD0 gt2k_menu::MemorycardCancelItem::select
MemorycardMenuResult MemorycardCancelItem::select(MemorycardControl &control)
{
    if (control.control1.decide != 0) {
        menu_play_se(MENU_SE_CANCEL);
        return MemorycardMenuResult(action, 0);
    }
    return MemorycardMenuResult(MC_RESULT_NONE, 0);
}

/* The window of layer `name' (anchor `window'), moved with up / down, not
 * cancelable, centred on (x, y), with the item "<name>_m_ok"
 * {MC_RESULT_CLOSE}. */
// FUNC 0x00158D28 gt2k_menu::MemorycardErrorDialog::MemorycardErrorDialog
MemorycardErrorDialog::MemorycardErrorDialog(MemorycardControl &control, int x, int y,
                                             const char *name, const char *window)
    : MemorycardLayerMenu(1, 0, control.pages.getFragmentManager(), control.pages.getLayer(), name,
                          window)
{
    char okName[0x40];                          /* sp+0x00 */
    PageLayout *layout = control.pages.getFragmentManager();
    Transitive *parent = root;

    sprintf(okName, "%s_m_ok", name);
    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions and their landing pads),
     * MemorycardMenuItemItem (MemorycardCancelItem's inline constructor) */
    append(new MemorycardCancelItem(layout, parent, okName, MC_RESULT_CLOSE));
    setPosition((f32)x, (f32)y);
}

/* "readerror": damaged adds "readerror_text_file_damage", else versionError
 * "readerror_text_version_error". */
// FUNC 0x00158EA0 gt2k_menu::MemorycardReadErrorDialog::MemorycardReadErrorDialog
MemorycardReadErrorDialog::MemorycardReadErrorDialog(MemorycardControl &control, int x, int y,
                                                     int damaged, int versionError)
    : MemorycardErrorDialog(control, x, y, "readerror", "readerror_window")
{
    PageLayout *layout = control.pages.current;

    /* implicit: operator new, operator delete (Object's, for the Items),
     * ~MemorycardLayerMenu (the bases' cleanup on a throw) */
    if (damaged != 0)
        attachObject(new Item("readerror_text_file_damage", layout));
    else if (versionError != 0)
        attachObject(new Item("readerror_text_version_error", layout));
}

// FUNC 0x00159020 gt2k_menu::MemorycardWriteErrorDialog::MemorycardWriteErrorDialog
MemorycardWriteErrorDialog::MemorycardWriteErrorDialog(MemorycardControl &control, int x, int y)
    : MemorycardErrorDialog(control, x, y, "writeerror", "writeerror_window")
{
}

/* "nospace"; with needKB != 0 the item "nospace_text_need" of the layer
 * "hidden" becomes an Item whose text is the item's text (a format) with
 * needKB. */
// FUNC 0x00159070 gt2k_menu::MemorycardNoSpaceDialog::MemorycardNoSpaceDialog
MemorycardNoSpaceDialog::MemorycardNoSpaceDialog(MemorycardControl &control, int x, int y,
                                                 int needKB)
    : MemorycardErrorDialog(control, x, y, "nospace", "nospace_window")
{
    /* implicit: operator new, operator delete (Object's, for the Item),
     * ~MemorycardLayerMenu (the bases' cleanup on a throw) */
    if (needKB != 0) {
        PageLayout *layout = control.pages.current;
        MenuLayoutItem *res = layout->findHiddenItem("nospace_text_need");
        const char *format = res->text->text;
        Item *need = new Item(res, layout->getTexture(res));

        sprintf(text, format, needKB);
        need->setText(text);
        attachObject(need);
    }
}

/* (No caller.) */
// FUNC 0x001591D0 gt2k_menu::MemorycardWriteProtectDialog::MemorycardWriteProtectDialog
MemorycardWriteProtectDialog::MemorycardWriteProtectDialog(MemorycardControl &control, int x,
                                                           int y)
    : MemorycardErrorDialog(control, x, y, "writeprotect", "writeprotect_window")
{
}

/* OK of the format dialog: the "formatworking" window at (x, y) becomes the
 * sub menu, the reader threads pause, the progress bar goes to 1 and the
 * card is formatted.  Failed: {MC_RESULT_NEXT_MENU, the write error dialog}
 * (the threads stay paused).  Done: the progress bar to 1 again, sound 7,
 * the texts give way to "formatworking_text_complete" and an OK item
 * {MC_RESULT_CLOSE}; the threads run again; {0, MC_ARG_SUB_MENU_OPENED}.
 * Inline: egcs emits it with the class's typeinfo function and destructor
 * (deferred output) and emits no copy of the class's inline constructor,
 * as in the binary. */
// FUNC 0x00159548 gt2k_menu::FormatWorkingMenu::makeSubMenu
inline MemorycardMenuResult FormatWorkingMenu::makeSubMenu(MemorycardControl &control)
{
    PageLayout *layout = control.pages.getFragmentManager();
    Item *message;
    Item *warning;
    Transitive *parent;
    f32 prev;

    control.progressPrev = 0.0f;
    control.progress = 0.0f;
    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions and their landing pads),
     * MemorycardMenuItemItem (MemorycardCancelItem's inline constructor) */
    subMenu = new MemorycardLayerMenu(0, 0, layout, control.pages.getLayer(), "formatworking",
                                      "formatworking_window");
    subMenu->setPosition((f32)x, (f32)y);
    message = new Item("formatworking_text_format", layout);
    warning = new Item("formatworking_text_warning", layout);
    subMenu->attachObject(message);
    subMenu->attachObject(warning);
    control.pauseThreads();

    /* memorycard_set_progress(1.0f, &control), inline */
    prev = control.progress;
    control.progress = 1.0f;
    control.progressPrev = prev;
    if (mc_format(port) != MC_STATUS_OK)
        return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                    new MemorycardWriteErrorDialog(control, x, y));

    prev = control.progress;
    control.progress = 1.0f;
    control.progressPrev = prev;
    menu_play_se(MENU_SE_CONFIRM);
    message->virtual_4(NULL);                   /* closed: its fade-out */
    warning->virtual_4(NULL);
    parent = subMenu->root;
    subMenu->append(new MemorycardCancelItem(layout, parent, "formatworking_m_ok",
                                             MC_RESULT_CLOSE));
    subMenu->attachObject(new Item("formatworking_text_complete", layout));
    control.restartThreads();
    return MemorycardMenuResult(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);
}

/* "format": OK (FormatWorkingMenu at the same position) / Cancel
 * {MC_RESULT_CLOSE}, the cursor on Cancel; "format_text_slot1" for slot 1,
 * else "format_text_slot2"; centred on (x, y).  (No caller.) */
// FUNC 0x00159220 gt2k_menu::MemorycardFormatDialog::MemorycardFormatDialog
MemorycardFormatDialog::MemorycardFormatDialog(MemorycardControl &control, int x, int y, int port)
    : MemorycardLayerMenu(0, 1, control.pages.getFragmentManager(), control.pages.getLayer(),
                          "format", "format_window")
{
    PageLayout *layout;
    Transitive *parent;

    this->port = port;
    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions and their landing pads),
     * MemorycardLayerMenuItem, MemorycardMenuItemItem (FormatWorkingMenu's and
     * MemorycardCancelItem's inline constructors) */
    layout = control.pages.getFragmentManager();
    parent = root;
    append(new FormatWorkingMenu(layout, parent, "format_m_ok", x, y, port));
    append(new MemorycardCancelItem(layout, parent, "format_m_cancel", MC_RESULT_CLOSE));
    selectLast();
    if (port == MC_PORT_SLOT1)
        attachObject(new Item("format_text_slot1", layout));
    else
        attachObject(new Item("format_text_slot2", layout));
    setPosition((f32)x, (f32)y);
}

/* {MC_RESULT_CARD_GONE, 0} once the slot has no PS2 memory card, else the
 * menu's select. */
// FUNC 0x001594E8 gt2k_menu::MemorycardFormatDialog::select
MemorycardMenuResult MemorycardFormatDialog::select(MemorycardControl &control)
{
    if (mc_slot_type(port) != sceMcTypePS2)
        return MemorycardMenuResult(MC_RESULT_CARD_GONE, 0);
    return MemorycardMenu::select(control);
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00159890 __tfQ29gt2k_menu17FormatWorkingMenu
extern "C" void *__tfQ29gt2k_menu17FormatWorkingMenu(void)
{
    if (__tiQ29gt2k_menu17FormatWorkingMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu17FormatWorkingMenu, "Q29gt2k_menu17FormatWorkingMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu17FormatWorkingMenu;
}
#endif

// FUNC 0x001598E0 gt2k_menu::FormatWorkingMenu::~FormatWorkingMenu
/* implicit (game/menu/memorycard/T076_memorycard_dialogs.h); egcs emits this copy for the
 * vtable: ~MemorycardLayerMenuItem (0x00151488, flags 0), then operator
 * delete (0x00150870) when flags & 1. */
/* implicit: ~MemorycardLayerMenuItem, operator delete (compiler-generated
 * destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: __class_type_info (MemorycardMenu
 * has two bases), one base, g_003518C0 (MemorycardLayerMenu). */
// FUNC 0x00159928 __tfQ29gt2k_menu22MemorycardFormatDialog
extern "C" void *__tfQ29gt2k_menu22MemorycardFormatDialog(void)
{
    if (__tiQ29gt2k_menu22MemorycardFormatDialog[0] == 0) {
        __tfQ29gt2k_menu19MemorycardLayerMenu();
        __rtti_class(__tiQ29gt2k_menu22MemorycardFormatDialog, "Q29gt2k_menu22MemorycardFormatDialog",
                     g_003518C0, 1);
    }
    return __tiQ29gt2k_menu22MemorycardFormatDialog;
}
#endif

// FUNC 0x00159980 gt2k_menu::MemorycardFormatDialog::~MemorycardFormatDialog
/* implicit (game/menu/memorycard/T076_memorycard_dialogs.h); egcs emits this copy for the
 * vtable: the vptrs (+0x14 0x0031FC08, +0x08 0x0031FC20; the implicit
 * destructor of a class with two vptrs stores them), ~MemorycardLayerMenu
 * (0x00151158, flags 0), then operator delete (0x00150870) when flags & 1. */
/* implicit: ~MemorycardLayerMenu, operator delete (compiler-generated
 * destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (base list g_003518C8:
 * MemorycardErrorDialog). */
// FUNC 0x001599E0 __tfQ29gt2k_menu28MemorycardWriteProtectDialog
extern "C" void *__tfQ29gt2k_menu28MemorycardWriteProtectDialog(void)
{
    if (__tiQ29gt2k_menu28MemorycardWriteProtectDialog[0] == 0) {
        __tfQ29gt2k_menu21MemorycardErrorDialog();
        __rtti_class(__tiQ29gt2k_menu28MemorycardWriteProtectDialog,
                     "Q29gt2k_menu28MemorycardWriteProtectDialog", g_003518C8, 1);
    }
    return __tiQ29gt2k_menu28MemorycardWriteProtectDialog;
}
#endif

// FUNC 0x00159A38 gt2k_menu::MemorycardWriteProtectDialog::~MemorycardWriteProtectDialog
/* implicit (game/menu/memorycard/T076_memorycard_dialogs.h); egcs emits this copy for the
 * vtable: the implicit ~MemorycardErrorDialog expanded (its vptrs +0x14
 * 0x0031FDC0, +0x08 0x0031FDD8), ~MemorycardLayerMenu (0x00151158, flags
 * 0), then operator delete (0x00150870) when flags & 1. */
/* implicit: ~MemorycardLayerMenu, operator delete (compiler-generated
 * destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (base list g_003518C8). */
// FUNC 0x00159A98 __tfQ29gt2k_menu23MemorycardNoSpaceDialog
extern "C" void *__tfQ29gt2k_menu23MemorycardNoSpaceDialog(void)
{
    if (__tiQ29gt2k_menu23MemorycardNoSpaceDialog[0] == 0) {
        __tfQ29gt2k_menu21MemorycardErrorDialog();
        __rtti_class(__tiQ29gt2k_menu23MemorycardNoSpaceDialog,
                     "Q29gt2k_menu23MemorycardNoSpaceDialog", g_003518C8, 1);
    }
    return __tiQ29gt2k_menu23MemorycardNoSpaceDialog;
}
#endif

// FUNC 0x00159AF0 gt2k_menu::MemorycardNoSpaceDialog::~MemorycardNoSpaceDialog
/* implicit; as ~MemorycardWriteProtectDialog. */
/* implicit: ~MemorycardLayerMenu, operator delete (compiler-generated
 * destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (base list g_003518C8). */
// FUNC 0x00159B50 __tfQ29gt2k_menu26MemorycardWriteErrorDialog
extern "C" void *__tfQ29gt2k_menu26MemorycardWriteErrorDialog(void)
{
    if (__tiQ29gt2k_menu26MemorycardWriteErrorDialog[0] == 0) {
        __tfQ29gt2k_menu21MemorycardErrorDialog();
        __rtti_class(__tiQ29gt2k_menu26MemorycardWriteErrorDialog,
                     "Q29gt2k_menu26MemorycardWriteErrorDialog", g_003518C8, 1);
    }
    return __tiQ29gt2k_menu26MemorycardWriteErrorDialog;
}
#endif

// FUNC 0x00159BA8 gt2k_menu::MemorycardWriteErrorDialog::~MemorycardWriteErrorDialog
/* implicit; as ~MemorycardWriteProtectDialog. */
/* implicit: ~MemorycardLayerMenu, operator delete (compiler-generated
 * destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (base list g_003518C8). */
// FUNC 0x00159C08 __tfQ29gt2k_menu25MemorycardReadErrorDialog
extern "C" void *__tfQ29gt2k_menu25MemorycardReadErrorDialog(void)
{
    if (__tiQ29gt2k_menu25MemorycardReadErrorDialog[0] == 0) {
        __tfQ29gt2k_menu21MemorycardErrorDialog();
        __rtti_class(__tiQ29gt2k_menu25MemorycardReadErrorDialog,
                     "Q29gt2k_menu25MemorycardReadErrorDialog", g_003518C8, 1);
    }
    return __tiQ29gt2k_menu25MemorycardReadErrorDialog;
}
#endif

// FUNC 0x00159C60 gt2k_menu::MemorycardReadErrorDialog::~MemorycardReadErrorDialog
/* implicit; as ~MemorycardWriteProtectDialog. */
/* implicit: ~MemorycardLayerMenu, operator delete (compiler-generated
 * destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (base list g_003518C0:
 * MemorycardLayerMenu). */
// FUNC 0x00159CC0 __tfQ29gt2k_menu21MemorycardErrorDialog
extern "C" void *__tfQ29gt2k_menu21MemorycardErrorDialog(void)
{
    if (__tiQ29gt2k_menu21MemorycardErrorDialog[0] == 0) {
        __tfQ29gt2k_menu19MemorycardLayerMenu();
        __rtti_class(__tiQ29gt2k_menu21MemorycardErrorDialog, "Q29gt2k_menu21MemorycardErrorDialog",
                     g_003518C0, 1);
    }
    return __tiQ29gt2k_menu21MemorycardErrorDialog;
}
#endif

// FUNC 0x00159D18 gt2k_menu::MemorycardErrorDialog::~MemorycardErrorDialog
/* implicit (game/menu/memorycard/T076_memorycard_dialogs.h; the four kinds' implicit
 * destructors expand it); egcs emits this copy for the vtable: the vptrs
 * (+0x14 0x0031FDC0, +0x08 0x0031FDD8), ~MemorycardLayerMenu (0x00151158,
 * flags 0), then operator delete (0x00150870) when flags & 1. */
/* implicit: ~MemorycardLayerMenu, operator delete (compiler-generated
 * destructor) */

// FUNC 0x00159D78 gt2k_menu::MemorycardCancelItem::MemorycardCancelItem
/* inline in game/menu/memorycard/T076_memorycard_dialogs.h; egcs emits this unreferenced
 * copy: MemorycardMenuItemItem (0x00150E00), action, the vptr 0x0031FE18. */
/* implicit: MemorycardMenuItemItem (the inline constructor's copy) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00159DC0 __tfQ29gt2k_menu20MemorycardCancelItem
extern "C" void *__tfQ29gt2k_menu20MemorycardCancelItem(void)
{
    if (__tiQ29gt2k_menu20MemorycardCancelItem[0] == 0) {
        __tfQ29gt2k_menu22MemorycardMenuItemItem();
        __rtti_si(__tiQ29gt2k_menu20MemorycardCancelItem, "Q29gt2k_menu20MemorycardCancelItem",
                  (const type_info *)__tiQ29gt2k_menu22MemorycardMenuItemItem);
    }
    return __tiQ29gt2k_menu20MemorycardCancelItem;
}
#endif

// FUNC 0x00159E10 gt2k_menu::MemorycardCancelItem::~MemorycardCancelItem
/* implicit (game/menu/memorycard/T076_memorycard_dialogs.h); egcs emits this copy for the
 * vtable.  The implicit ~MemorycardMenuItemItem / ~MemorycardMenuItem are
 * empty here and Cell<MemorycardMenuItem>'s inline destructor stores its
 * vptr (+0x08 0x0031E0A0); then ~pCell (0x0023C0C0, flags 0) and operator
 * delete (0x00150870) when flags & 1. */
/* implicit: ~pCell, operator delete (compiler-generated destructor) */

// FUNC 0x00159E68 gt2k_menu::MemorycardMenuItem::render
/* inline in game/menu/memorycard/T073_memorycard_menu.h (empty); T076's unreferenced copy,
 * emitted for MemorycardCancelItem's vtable [6] (the vtables use T073's
 * 0x001519C8).  egcs -O2 puts it here, between ~MemorycardCancelItem and
 * the Cell<MemorycardMenuItem> copies, as in the binary. */

// FUNC 0x00159E70 gt2k_menu::Cell<gt2k_menu::MemorycardMenuItem>::~Cell
/* inline in game/menu/framework/menu_list.h; T076's unreferenced copy (the vtables use
 * T073's 0x00151748): the vptr (+0x08 = 0x0031E0A0), then ~pCell(this,
 * flags), which does the delete. */
/* implicit: ~pCell (the inline destructor's copy) */

#ifndef BOOT_OLD_GCC
/* T076's copy of the template instance's typeinfo function (unreferenced;
 * T073's is 0x00151A80, declared in game/menu/memorycard/T073_memorycard_menu.h; same node,
 * own copy of the name at 0x00320000).  Weak: the build links T073's. */
// FUNC 0x00159E98 __tfQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem(void)
{
    if (__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem[0] == 0) {
        __tfQ29gt2k_menu5pCell();
        __rtti_si(__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem,
                  "Q29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem;
}
#endif
