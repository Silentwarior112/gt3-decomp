/* T074 - the operation menu of the memory card file screens, the items that
 * open its confirm windows and the "working" windows that do the card
 * operations.  0x00151AD0..0x00156A80 (work units u029 and u030: one object
 * in the original, merged here on 2026-10-16).
 *
 * T071 builds a MemorycardOperationMenu when a file of the list is decided
 * (and again after the name entry of a comment edit).  Its items are
 * MemorycardLayerMenuItems: deciding one calls makeSubMenu, which opens a
 * confirm window ("confirm" / "confirm2" of menu/memorycard.mbl) whose OK
 * item, a *WorkingMenu, does the work inside its own makeSubMenu: it opens a
 * "working" window and runs the whole card operation, the reader threads
 * paused and the window's progress bar fed by the progress callbacks; then
 * it shows "..._text_complete" with an OK item.  An error returns
 * {MC_RESULT_NEXT_MENU, a read / write error dialog (T076)} for T071 to
 * open.  The confirm windows are centred on g_00351848 / g_0035184C (the
 * centre of "confirm_window", set by the operation menu).
 * MemorycardOperationMenu is in game/menu/memorycard/T074_memorycard_operation_menu.h (T071
 * builds it); the other classes are T074's own and defined below.
 *
 * Every `new' of a window, an item or a dialog below is MemorycardMenuItem's
 * operator new (the menu heap), with MemorycardMenuItem's operator delete on
 * the exception path while the constructor runs; a `new Item' is Object's
 * operator new / delete.  The items' constructors are inline: each `new' of
 * one expands MemorycardLayerMenuItem's (the menus and *WorkingMenus) or
 * MemorycardMenuItemItem's (MemorycardCancelItem) constructor and stores the
 * vptr and the members.
 *
 * Source order: the original's, as the binary shows it.  Only four functions
 * are compiled where they are defined (memorycard_set_progress, the
 * MemorycardOperationMenu constructor and select, memorycard_set_confirm_
 * center), at the end of this file and the start of the text.  Every other
 * function is an inline member (no class here has a key method but
 * MemorycardOperationMenu), which egcs writes at the end of the unit in
 * reverse order of definition, each class's after its typeinfo function and
 * implicit destructor (one round for the classes the constructor builds,
 * then one for the classes their makers build: the working menus and
 * ConfirmMenu): so the classes and their members below run backwards
 * through the addresses.  The order of the definitions is also the binary's
 * data order:
 *   - .sdata 0x00351848: g_00351848, g_0035184C, then "working", "hidden"
 *     (CopyWorkingMenu's maker), "confirm" (CopyMenu's), then egcs's base
 *     list of ConfirmMenu / MemorycardOperationMenu (g_00351868);
 *   - .rodata 0x0031E420: the makers' strings, written as egcs parses each
 *     inline body (CopyWorkingMenu, CopyMenu, DeleteWorkingMenu, DeleteMenu,
 *     CommentChangeWorkingMenu, CommentChangeMenu, SaveWorkingMenu,
 *     SaveMenu, OverwriteMenu, LoadWorkingMenu, LoadMenu), then the layer
 *     tables' strings ("operation1".."op4_window"), then the constructor's
 *     ("op1_m_write"..), then the vtables of the classes the constructor
 *     builds (egcs walks the classes in reverse order of definition), the
 *     type names (the same order; then the classes of memorycard_operation_menu_internal.h's headers),
 *     the working menus' and ConfirmMenu's vtables (only the makers' code,
 *     written at the end, refers to them) and LoadWorkingMenu's jump table. */
#include "game/menu/memorycard/memorycard_operation_menu_internal.h"

using namespace gt2k_menu;

/* ======================================================================== */
/* Data                                                                     */
/* ======================================================================== */

/* The centre of the confirm windows (y up, 448 lines), until the operation
 * menu sets it from "confirm_window" (memorycard_set_confirm_center).  T074's
 * first .sdata items, before the makers' short strings. */
f32 g_00351848 = 320.0f;                    /* 0x00351848 */
f32 g_0035184C = 180.0f;                    /* 0x0035184C */

static void memorycard_set_confirm_center(f32 x, f32 y);

namespace gt2k_menu {

/* (? name) The centre of a layout element, y up (448 lines).  ConfirmMenu's
 * constructor and the MemorycardOperationMenu constructor take the face's
 * centre relative to the window's (anchor's).  The four fields are read
 * into locals first, in field order: that RTL order is what lets sched1 keep
 * the window's h / y products alive longer, so local-alloc gives retail's
 * FPU registers (0.5 in $f5, the window's h / y in $f7 / $f8); computed in
 * the expressions, every form tried (u029: ~900) leaves $f6 / $f1 / $f7 and
 * the window's y add two instructions early. */
static inline void item_center(MenuLayoutItem *item, f32 *x, f32 *y)
{
    f32 ix = item->x;
    f32 iy = item->y;
    f32 iw = item->w;
    f32 ih = item->h;

    *x = ix + iw * 0.5f;
    *y = 448.0f - (iy + ih * 0.5f);
}

/* ------------------------------------------------------------------------ */
/* MemorycardOperationMenu's inline members                                 */
/* ------------------------------------------------------------------------ */

/* Out-of-line copies of two inline accessors (no callers; T071 reads the
 * fields directly): MemorycardOperationMenu has a key method (select, at the
 * end), so egcs writes every inline member of it here, after the members of
 * the classes below (defined after them). */
// FUNC 0x00154610 gt2k_menu::MemorycardOperationMenu::getIconDX
inline f32 MemorycardOperationMenu::getIconDX()
{
    return iconDX;
}

// FUNC 0x00154608 gt2k_menu::MemorycardOperationMenu::getIconDY
inline f32 MemorycardOperationMenu::getIconDY()
{
    return iconDY;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (a class with a base list:
 * MemorycardLayerMenu). */
// FUNC 0x00154618 __tfQ29gt2k_menu23MemorycardOperationMenu
extern "C" void *__tfQ29gt2k_menu23MemorycardOperationMenu(void)
{
    if (__tiQ29gt2k_menu23MemorycardOperationMenu[0] == 0) {
        __tfQ29gt2k_menu19MemorycardLayerMenu();
        __rtti_class(__tiQ29gt2k_menu23MemorycardOperationMenu,
                     "Q29gt2k_menu23MemorycardOperationMenu", g_00351868, 1);
    }
    return __tiQ29gt2k_menu23MemorycardOperationMenu;
}
#endif

// FUNC 0x00154670 gt2k_menu::MemorycardOperationMenu::~MemorycardOperationMenu
/* implicit in game/menu/memorycard/T074_memorycard_operation_menu.h; egcs emits this copy
 * after the typeinfo function: stores both vtables (0x0031EB10 at +0x14,
 * 0x0031EB28 at +0x08), then ~MemorycardLayerMenu (flags 0), then
 * MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: ~MemorycardLayerMenu, operator delete (compiler-generated
 * destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (a class with a base list:
 * MemorycardLayerMenu). */
// FUNC 0x001569C8 __tfQ29gt2k_menu11ConfirmMenu
extern "C" void *__tfQ29gt2k_menu11ConfirmMenu(void)
{
    if (__tiQ29gt2k_menu11ConfirmMenu[0] == 0) {
        __tfQ29gt2k_menu19MemorycardLayerMenu();
        __rtti_class(__tiQ29gt2k_menu11ConfirmMenu, "Q29gt2k_menu11ConfirmMenu", g_00351868, 1);
    }
    return __tiQ29gt2k_menu11ConfirmMenu;
}
#endif

// FUNC 0x00156A20 gt2k_menu::ConfirmMenu::~ConfirmMenu
/* implicit (below); egcs emits this copy after the typeinfo function: stores
 * both vtables (0x0031F250 at +0x14, 0x0031F268 at +0x08), then
 * ~MemorycardLayerMenu (flags 0), then MemorycardMenuItem's operator delete
 * when flags & 1 */
/* implicit: ~MemorycardLayerMenu, operator delete (compiler-generated
 * destructor) */

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* ConfirmMenu                                                              */
/* ------------------------------------------------------------------------ */

/* A window centred on (x, y) that also tells the control where the file's
 * icon goes (the element faceName of the layer, relative to the window).
 * vtables 0x0031F268 (primary, 7 slots: [1] / [2] pCell's, [3] / [4] / [5] /
 * [6] MemorycardLayerMenu's 0x00150A88 / 0x00150CA8 / 0x001511C8 /
 * 0x00150D58) and 0x0031F250 (the List<MemorycardMenuItem> part, 2 slots,
 * this-delta -0xC: [1] pList's sort 0x0023C580).  typeinfo
 * __tfQ29gt2k_menu11ConfirmMenu 0x001569C8 (class, base list g_00351868
 * {MemorycardLayerMenu, public +0}; storage 0x004B1B30).  Size 0x3C. */
class ConfirmMenu : public MemorycardLayerMenu {
public:
    /* The confirm windows: (1, "confirm", "confirm_window",
     * "confirm_icon_face"); the working windows: (0, "working",
     * "working_window", "working_icon_face") (SaveWorkingMenu's "working2"
     * is a plain MemorycardLayerMenu). */
    ConfirmMenu(MemorycardControl &control, int cancelable, const char *layerName,
                const char *windowName, const char *faceName, f32 x, f32 y);
    /* [0] +0x0C 0x00156A20 ~ConfirmMenu: implicit */
};
EGCS_SIZE(gt2k_menu::ConfirmMenu, 0x3C);

/* Inline (every maker expands it; no out-of-line copy: the class has no key
 * method).  MemorycardLayerMenu(0, cancelable, the open page's layout, the
 * root object of its popup level, layerName, windowName); then the centre
 * of faceName (PageLayout::findObject in the layer) minus the window's
 * centre (y up, 448 lines) is added to (x, y) and stored in the control's
 * confirm icon position (iconPos[4] / [5], +0x3075C), after setPosition(x,
 * y).  The page manager is read through its inline accessors
 * (getFragmentManager, getLayer): their `this' is &control.pages, which
 * retail keeps in a register (control + 0x2E4) for the whole maker, the
 * maker's own `layout' read included; direct field reads fold to control +
 * 0x4C0. */
inline ConfirmMenu::ConfirmMenu(MemorycardControl &control, int cancelable, const char *layerName,
                                const char *windowName, const char *faceName, f32 x, f32 y)
    : MemorycardLayerMenu(0, cancelable, control.pages.getFragmentManager(),
                          control.pages.getLayer(), layerName, windowName)
{
    /* (no NULL checks: the layout has the face and the window) */
    MenuLayoutItem *face = PageLayout::findObjectIn(faceName,
                                                    control.pages.getFragmentManager()->getLayer(layerName));
    /* the face's centre relative to the window's (y up, 448 lines) */
    f32 wx, wy, fx, fy;

    item_center(anchor, &wx, &wy);
    item_center(face, &fx, &fy);
    f32 dx = fx - wx;
    f32 dy = fy - wy;

    setPosition(x, y);
    control.iconPos[4] = x + dx;            /* where the file's icon goes while the window is open */
    control.iconPos[5] = y + dy;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* CopyWorkingMenu                                                          */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00156930 __tfQ29gt2k_menu15CopyWorkingMenu
extern "C" void *__tfQ29gt2k_menu15CopyWorkingMenu(void)
{
    if (__tiQ29gt2k_menu15CopyWorkingMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu15CopyWorkingMenu, "Q29gt2k_menu15CopyWorkingMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu15CopyWorkingMenu;
}
#endif

// FUNC 0x00156980 gt2k_menu::CopyWorkingMenu::~CopyWorkingMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: ~MemorycardLayerMenuItem, operator delete (compiler-generated
 * destructor) */

namespace gt2k_menu {

/* "confirm_m_ok" of CopyMenu.  vtable 0x0031F1F8 (10 slots: [3]
 * MemorycardLayerMenuItem::select, [4] MemorycardMenuItemItem::isSelectable).
 * typeinfo __tfQ29gt2k_menu15CopyWorkingMenu 0x00156930 (si, base
 * MemorycardLayerMenuItem; storage 0x004B1AC0).  Size 0x2C. */
class CopyWorkingMenu : public MemorycardLayerMenuItem {
public:
    CopyWorkingMenu(PageLayout *layout, Object *parent, const char *name, int replace, int format)
        : MemorycardLayerMenuItem(layout, parent, name), replace(replace), format(format) {}
    /* [0] +0x0C 0x00156980 ~CopyWorkingMenu: implicit (no vptr store) */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x001562E0 */

    /*  MemorycardLayerMenuItem                0x00 */
    int replace;                            /* 0x24 a file of the name is on the other card */
    int format;                             /* 0x28 the other card is not formatted */
};
EGCS_SIZE(gt2k_menu::CopyWorkingMenu, 0x2C);

/* "working": copies the selected file from the card of the current slot to
 * the other one (copyTo, with replace / format from CopyMenu).  A changed
 * source card gives the read error dialog, a changed destination the write
 * error one; a failed copy the read error dialog if the source's last
 * operation failed (mc_slot_status), else the write error one.  Then
 * "working_text_complete", 4 frames, the bar is released, "working_m_ok"
 * (MC_RESULT_FILE_DONE) added and the other card's list read again. */
// FUNC 0x001562E0 gt2k_menu::CopyWorkingMenu::makeSubMenu
inline MemorycardMenuResult CopyWorkingMenu::makeSubMenu(MemorycardControl &control)
{
    PageLayout *layout;                         /* s5 */
    Item *textCopy;                             /* s7 */
    Item *textWarning;                          /* s6 */
    Transitive *bar;                            /* fp */
    MemorycardFile *file;                       /* s4 */
    MemorycardFileManagerBase *manager;
    int src;                                    /* s1 the current slot's port */
    int dst;                                    /* s0 the other slot's port */

    /* implicit: operator new, operator delete, MemorycardLayerMenu,
     * setPosition, getLayer, findObjectIn, ~MemorycardLayerMenu (the inline ConfirmMenu constructor
     * and its cleanup), MemorycardMenuItemItem (MemorycardCancelItem's inline
     * constructor) */
    control.progressPrev = 0.0f;
    control.progress = 0.0f;
    /* expands ConfirmMenu's constructor: MemorycardLayerMenu's constructor,
     * PageLayout::getLayer / findObject, setPosition */
    subMenu = new ConfirmMenu(control, 0, "working", "working_window", "working_icon_face",
                              g_00351848, g_0035184C);
    layout = control.pages.getFragmentManager();
    textCopy = new Item("working_text_copy", layout);
    textWarning = new Item("working_text_warning", layout);
    bar = layout->createObject(layout->findObject("working_bar_base", "hidden"), subMenu->root, NULL);
    subMenu->attachObject(textCopy);
    subMenu->attachObject(textWarning);
    subMenu->attachObject(bar);
    control.pauseThreads();
    file = control.getSelectedFile();
    src = control.getCurrentPort();
    dst = control.getOtherPort();
    if (file != NULL && src >= 0 && dst >= 0) {
        if (mc_slot_refresh(src) != 0) {        /* the source card was changed */
            /* sp+0x00 */
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                        new MemorycardReadErrorDialog(control, (int)g_00351848, (int)g_0035184C, 0, 0));
        }
        if (mc_slot_refresh(dst) != 0) {        /* the destination card was changed */
            /* sp+0x08 */
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                        new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
        }
        if (file->copyTo(src, dst, replace, format, memorycard_set_progress, &control) == 0) {
            if (mc_slot_status(src) != MC_STATUS_OK) {
                /* sp+0x10 */
                return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                            new MemorycardReadErrorDialog(control, (int)g_00351848, (int)g_0035184C, 0, 0));
            }

            /* sp+0x18 */
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                        new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
        }
    }

    menu_play_se(MENU_SE_CONFIRM);
    textCopy->virtual_4(NULL);                  /* close (fade out) */
    textWarning->virtual_4(NULL);
    subMenu->attachObject(new Item("working_text_complete", layout));
    gfx_vsync(MC_COMPLETE_WAIT);
    bar->kill();
    /* (MemorycardCancelItem's inline constructor: MemorycardMenuItemItem's) */
    {
        Transitive *parent = subMenu->root;

        subMenu->append(new MemorycardCancelItem(layout, parent, "working_m_ok", MC_RESULT_FILE_DONE));
    }
    manager = control.getOtherManager();
    if (manager != NULL)
        manager->updateFileId();
    control.restartThreads();

    MemorycardMenuResult result(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);  /* sp+0x20 */

    return result;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* CopyMenu                                                                 */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00154570 __tfQ29gt2k_menu8CopyMenu
extern "C" void *__tfQ29gt2k_menu8CopyMenu(void)
{
    if (__tiQ29gt2k_menu8CopyMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu8CopyMenu, "Q29gt2k_menu8CopyMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu8CopyMenu;
}
#endif

// FUNC 0x001545C0 gt2k_menu::CopyMenu::~CopyMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "opN_m_copy": copies the selected file to the card of the other slot.
 * vtable 0x0031EAB8 (10 slots: [5] / [6] / [8] MemorycardLayerMenuItem's).
 * typeinfo __tfQ29gt2k_menu8CopyMenu 0x00154570 (si, base
 * MemorycardLayerMenuItem; storage 0x004B1B60).  Size 0x28. */
class CopyMenu : public MemorycardLayerMenuItem {
public:
    CopyMenu(PageLayout *layout, Object *parent, const char *name)
        : MemorycardLayerMenuItem(layout, parent, name), selectable(true) {}
    /* [0] +0x0C 0x001545C0 ~CopyMenu: implicit (no vptr store) */
    /* Not selectable with a window open: the window is deleted, {1, 0xB}.
     * Else MemorycardLayerMenuItem::select. */
    virtual MemorycardMenuResult select(MemorycardControl &control); /* [3] +0x24 0x00154448 */
    virtual bool isSelectable(MemorycardControl &control); /* [4] +0x2C 0x00154568 selectable */
    /* "confirm" (OK CopyWorkingMenu): "copy", "same file" + "overwrite", or
     * "format the card in slot N" + "approve"; or the read error / "nospace"
     * dialog instead. */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00153C28 */
    /* selectable: the other slot's card is ready and the selected file may
     * be copied (no sceMcFileAttrDupProhibit) and has a known size; dim =
     * !selectable. */
    virtual void updateItem(MemorycardControl &control, bool sel); /* [9] +0x54 0x001544C0 */
    /* (? names) makeSubMenu's checks, inline (no out-of-line copy), u030:
     * checkCard looks at the other slot's card (an unformatted card sets
     * format, a file of the name replace) and returns 0 when an error dialog
     * replaced subMenu (openReadError, the "nospace" dialog), else 1.
     * Retail's exception handlers of the dialogs' new sit right after each
     * openReadError and after checkCard's `return 0', where an inline body's
     * handlers go. */
    int  checkCard(MemorycardControl &control, int &replace, int &format);
    void openReadError(MemorycardControl &control);

    /*  MemorycardLayerMenuItem                0x00 */
    /* bool (4 bytes in egcs), as OverwriteMenu's: isSelectable (bool)
     * returns it with a bare lw (0x00154568; an int member would need
     * egcs's sltu) and updateItem dims with xori 1.  updateItem stores
     * MemorycardSlot::isReady()'s value unconverted, so isReady (T071, int
     * there) returns bool too (u030; was int). */
    bool selectable;                        /* 0x24 isSelectable() (updateItem) */
};
EGCS_SIZE(gt2k_menu::CopyMenu, 0x28);

// FUNC 0x00154568 gt2k_menu::CopyMenu::isSelectable
inline bool CopyMenu::isSelectable(MemorycardControl &control)
{
    return selectable;
}

/* Selectable (and not dimmed) while the card of the other slot is ready and
 * the selected file may be copied (no sceMcFileAttrDupProhibit) and has a
 * known size. */
// FUNC 0x001544C0 gt2k_menu::CopyMenu::updateItem
inline void CopyMenu::updateItem(MemorycardControl &control, bool sel)
{
    MemorycardSlot *other = control.getOtherSlot();
    MemorycardFile *file;

    if (other == NULL)
        selectable = false;
    else
        selectable = other->isReady();
    file = control.getSelectedFile();
    if (file == NULL) {
        selectable = false;
    } else {
        bool copyable = false;

        /* bit 3 of attr: sceMcFileAttrDupProhibit */
        if (selectable && ((file->dir.entry.attr >> 3) & 1) == 0) {
            /* (retail's slti / xori: a conditional store, which jump
             * turns into the sge expander; copyable = sizeKB >= 0
             * gives egcs's nor / srl) */
            if (file->sizeKB >= 0)
                copyable = true;
        }
        selectable = copyable;
    }
    dim = !selectable;
}

/* While not selectable (the other card went away, ...) an open window is
 * deleted: {1, 0xB}.  Else MemorycardLayerMenuItem::select. */
// FUNC 0x00154448 gt2k_menu::CopyMenu::select
inline MemorycardMenuResult CopyMenu::select(MemorycardControl &control)
{
    if (selectable || subMenu == NULL)
        return MemorycardLayerMenuItem::select(control);
    delete subMenu;

    MemorycardMenuResult r(MC_RESULT_USED, MC_ARG_SUB_MENU_CLOSED);  /* sp+0x00 */

    subMenu = NULL;
    return r;
}

/* "confirm" (cancelable): OK copies the selected file to the card of the
 * other slot (CopyWorkingMenu), the cursor starts on Cancel.  Releases the
 * 3D icon of slot 0 first.  With the reader threads paused it looks at the
 * other card:
 *   not formatted        -> "confirm_text_format_copy_<1|2>" + "approve"
 *                           (CopyWorkingMenu format)
 *   a file of the name   -> "confirm_text_same_file" + "overwrite"
 *                           (CopyWorkingMenu replace; the old file's size
 *                           counts as free)
 *   else                 -> "confirm_text_copy"
 * exists() or calcSize() failing replaces the window with the read error
 * dialog; lacking the room (2 KB are kept free) with the "nospace" one.  If
 * either card was changed meanwhile the window is deleted and {0, 0}
 * returned.  Finally the selected file reads its copy icon.
 *
 * The checks are inline members (checkCard, openReadError, as
 * SaveMenu::checkCard): the exception handlers of the dialogs' `new'
 * (operator delete, terminate) sit in the code right after each read error
 * and after the check's `return 0', where the handlers of an inline
 * function's body go; written in makeSubMenu itself, egcs puts them at the
 * end of the function and numbers the registers otherwise (file in $s1, dst
 * in $s0 are checkCard's own locals), and the flag `ok' is retail's `$s0 =
 * 1' in the branches' delay slots. */
// FUNC 0x00153C28 gt2k_menu::CopyMenu::makeSubMenu
/* The window replaced by the read error dialog (both read errors). */
inline void CopyMenu::openReadError(MemorycardControl &control)
{
    delete subMenu;
    subMenu = new MemorycardReadErrorDialog(control, (int)g_00351848, (int)g_0035184C, 0, 0);
}

/* 1: the confirm window stays (format / replace set for CopyWorkingMenu);
 * 0: it was replaced by an error dialog. */
inline int CopyMenu::checkCard(MemorycardControl &control, int &replace, int &format)
{
    MemorycardFile *file = control.getSelectedFile();     /* s1 */
    int dst = control.getOtherPort();                   /* s0 the other slot's port */
    int exists;
    int oldKB;                                          /* sp+0x40 */
    int freeKB;
    int needKB;

    if (file == NULL || dst < 0)
        return 1;
    if (mc_slot_formatted(dst) == 0) {
        format = 1;
        return 1;
    }
    exists = file->exists(dst);
    if (exists < 0) {
        openReadError(control);
        return 0;
    }
    replace = (exists == 1);
    oldKB = 0;
    if (replace && file->calcSize(dst, &oldKB) == 0) {
        openReadError(control);
        return 0;
    }
    freeKB = mc_slot_free(dst);
    needKB = file->sizeKB - oldKB;
    if (freeKB - MC_FREE_MARGIN_KB < needKB && needKB > 0) {
        delete subMenu;
        subMenu = new MemorycardNoSpaceDialog(control, (int)g_00351848, (int)g_0035184C, needKB);
        return 0;
    }
    return 1;
}

inline MemorycardMenuResult CopyMenu::makeSubMenu(MemorycardControl &control)
{
    int format;                                 /* s7 the other card is not formatted */
    int replace;                                /* s6 a file of the name is on it */
    int ok;                                     /* s0 the confirm window stays */
    MemorycardFile *file;
    int port;

    /* implicit: operator new, operator delete, MemorycardLayerMenu,
     * setPosition, getLayer, findObjectIn, ~MemorycardLayerMenu (the inline ConfirmMenu constructor
     * and its cleanup), MemorycardLayerMenuItem, MemorycardMenuItemItem (the
     * items' inline constructors) */
    icon_slot_release(0);
    /* expands ConfirmMenu's constructor: MemorycardLayerMenu's constructor,
     * PageLayout::getLayer / findObject, setPosition */
    subMenu = new ConfirmMenu(control, 1, "confirm", "confirm_window", "confirm_icon_face",
                              g_00351848, g_0035184C);
    format = 0;
    control.pauseThreads();
    /* (retail copies format's 0: `daddu s6, s7'; every form tried loads
     * the constant: replace = format, replace = format = 0, both before
     * the call, 64-bit types) */
    replace = 0;
    ok = checkCard(control, replace, format);

    /* a card changed meanwhile: no window */
    port = control.getOtherPort();
    if (port >= 0 && mc_slot_refresh(port) != 0) {
        delete subMenu;
        subMenu = NULL;
        control.restartThreads();

        MemorycardMenuResult result(MC_RESULT_NONE, 0);  /* sp+0x48 */

        return result;
    }
    port = control.getCurrentPort();
    if (port >= 0 && mc_slot_refresh(port) != 0) {
        delete subMenu;
        subMenu = NULL;
        control.restartThreads();

        MemorycardMenuResult result(MC_RESULT_NONE, 0);  /* sp+0x50 */

        return result;
    }
    control.restartThreads();

    if (ok) {
        PageLayout *layout = control.pages.getFragmentManager();
        Transitive *parent = subMenu->root;

        /* (the items' inline constructors: MemorycardLayerMenuItem's,
         * MemorycardMenuItemItem's) */
        subMenu->append(new CopyWorkingMenu(layout, parent, "confirm_m_ok", replace, format));
        subMenu->append(new MemorycardCancelItem(layout, parent, "confirm_m_cancel", MC_RESULT_CANCEL));
        subMenu->selectLast();
        if (!replace) {
            if (!format) {
                subMenu->attachObject(new Item("confirm_text_copy", layout));
            } else {
                char name[0x40];                /* sp+0x00 */

                /* the slot's number: port 0 is slot 1 */
                sprintf(name, "confirm_text_format_copy_%d", control.getOtherPort() == 0 ? 1 : 2);
                subMenu->attachObject(new Item(name, layout));
                subMenu->attachObject(new Item("confirm_text_approve", layout));
            }
        } else {
            subMenu->attachObject(new Item("confirm_text_same_file", layout));
            subMenu->attachObject(new Item("confirm_text_overwrite", layout));
        }
        file = control.getSelectedFile();
        port = control.getCurrentPort();
        if (file != NULL && port >= 0)
            file->readIcon(port, MC_ICON_COPY);
    }

    MemorycardMenuResult result(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);  /* sp+0x58 */

    return result;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* DeleteWorkingMenu                                                        */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00156248 __tfQ29gt2k_menu17DeleteWorkingMenu
extern "C" void *__tfQ29gt2k_menu17DeleteWorkingMenu(void)
{
    if (__tiQ29gt2k_menu17DeleteWorkingMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu17DeleteWorkingMenu, "Q29gt2k_menu17DeleteWorkingMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu17DeleteWorkingMenu;
}
#endif

// FUNC 0x00156298 gt2k_menu::DeleteWorkingMenu::~DeleteWorkingMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "confirm_m_ok" of DeleteMenu.  vtable 0x0031F1A0 (10 slots: [3]
 * MemorycardLayerMenuItem::select, [4] MemorycardMenuItemItem::isSelectable).
 * typeinfo __tfQ29gt2k_menu17DeleteWorkingMenu 0x00156248 (si, base
 * MemorycardLayerMenuItem; storage 0x004B1B40).  Size 0x24. */
class DeleteWorkingMenu : public MemorycardLayerMenuItem {
public:
    DeleteWorkingMenu(PageLayout *layout, Object *parent, const char *name)
        : MemorycardLayerMenuItem(layout, parent, name) {}
    /* [0] +0x0C 0x00156298 ~DeleteWorkingMenu: implicit (no vptr store) */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00155CF0 */
};
EGCS_SIZE(gt2k_menu::DeleteWorkingMenu, 0x24);

/* "working": deletes the selected file from the card of the current slot.
 * Write errors return {0xB, write error dialog}.  Then
 * "working_text_complete", 4 frames, the bar is released, "working_m_ok"
 * (MC_RESULT_FILE_DONE) added, the info panel released, the list
 * renumbered (updateFileIdForDelete) and the panel rebuilt. */
// FUNC 0x00155CF0 gt2k_menu::DeleteWorkingMenu::makeSubMenu
inline MemorycardMenuResult DeleteWorkingMenu::makeSubMenu(MemorycardControl &control)
{
    PageLayout *layout;                         /* s5 */
    Item *textDelete;                           /* s6 */
    Item *textWarning;                          /* s4 */
    Transitive *bar;                            /* s7 */
    MemorycardFile *file;                       /* s1 */
    MemorycardFileManagerBase *manager;
    int port;                                   /* s0 */

    /* implicit: operator new, operator delete, MemorycardLayerMenu,
     * setPosition, getLayer, findObjectIn, ~MemorycardLayerMenu (the inline ConfirmMenu constructor
     * and its cleanup), MemorycardMenuItemItem (MemorycardCancelItem's inline
     * constructor) */
    control.progressPrev = 0.0f;
    control.progress = 0.0f;
    /* expands ConfirmMenu's constructor: MemorycardLayerMenu's constructor,
     * PageLayout::getLayer / findObject, setPosition */
    subMenu = new ConfirmMenu(control, 0, "working", "working_window", "working_icon_face",
                              g_00351848, g_0035184C);
    layout = control.pages.getFragmentManager();
    textDelete = new Item("working_text_delete", layout);
    textWarning = new Item("working_text_warning", layout);
    bar = layout->createObject(layout->findObject("working_bar_base", "hidden"), subMenu->root, NULL);
    subMenu->attachObject(textDelete);
    subMenu->attachObject(textWarning);
    subMenu->attachObject(bar);
    control.pauseThreads();
    file = control.getSelectedFile();
    port = control.getCurrentPort();
    if (file != NULL && port >= 0) {
        if (mc_slot_refresh(port) != 0) {       /* the card was changed */
            /* sp+0x00 */
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                        new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
        }
        if (file->deleteDir(port, memorycard_set_progress, &control) == 0) {
            /* sp+0x08 */
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                        new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
        }
    }

    menu_play_se(MENU_SE_CONFIRM);
    textDelete->virtual_4(NULL);                /* close (fade out) */
    textWarning->virtual_4(NULL);
    subMenu->attachObject(new Item("working_text_complete", layout));
    gfx_vsync(MC_COMPLETE_WAIT);
    bar->kill();
    /* (MemorycardCancelItem's inline constructor: MemorycardMenuItemItem's) */
    {
        Transitive *parent = subMenu->root;

        subMenu->append(new MemorycardCancelItem(layout, parent, "working_m_ok", MC_RESULT_FILE_DONE));
    }
    control.slots.releaseInfo();
    manager = control.getCurrentManager();
    if (manager != NULL)
        manager->updateFileIdForDelete();
    control.slots.resetInfo();
    control.restartThreads();

    MemorycardMenuResult result(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);  /* sp+0x10 */

    return result;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* DeleteMenu                                                               */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00153B90 __tfQ29gt2k_menu10DeleteMenu
extern "C" void *__tfQ29gt2k_menu10DeleteMenu(void)
{
    if (__tiQ29gt2k_menu10DeleteMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu10DeleteMenu, "Q29gt2k_menu10DeleteMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu10DeleteMenu;
}
#endif

// FUNC 0x00153BE0 gt2k_menu::DeleteMenu::~DeleteMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "opN_m_delete".  vtable 0x0031EA60 (10 slots: [3] MemorycardLayerMenuItem::
 * select, [5] / [6] / [8] / [9] MemorycardLayerMenuItem's).  typeinfo
 * __tfQ29gt2k_menu10DeleteMenu 0x00153B90 (si, base MemorycardLayerMenuItem;
 * storage 0x004B1B10).  Size 0x24. */
class DeleteMenu : public MemorycardLayerMenuItem {
public:
    DeleteMenu(PageLayout *layout, Object *parent, const char *name)
        : MemorycardLayerMenuItem(layout, parent, name) {}
    /* [0] +0x0C 0x00153BE0 ~DeleteMenu: implicit (no vptr store) */
    virtual bool isSelectable(MemorycardControl &control); /* [4] +0x2C 0x00153B70 a file is
                                               selected */
    /* Stops icon slot 0; "confirm" (OK DeleteWorkingMenu, Cancel {0xA},
     * "confirm_text_delete"), the cursor on Cancel; the file reads its
     * delete icon (readIcon which 2). */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x001537F8 */
};
EGCS_SIZE(gt2k_menu::DeleteMenu, 0x24);

/* Delete: selectable when a file is selected. */
// FUNC 0x00153B70 gt2k_menu::DeleteMenu::isSelectable
inline bool DeleteMenu::isSelectable(MemorycardControl &control)
{
    return control.getSelectedFile() != NULL;
}

/* "confirm": OK deletes the file (DeleteWorkingMenu), the cursor starts on
 * Cancel.  Icon slot 0 (T077) is emptied first; then the selected file
 * reads its delete icon (readIcon which 2: icon.sys's FnameDel, a model in
 * that slot) for the window.  new ConfirmMenu expands its constructor here:
 * MemorycardLayerMenu's, PageLayout::getLayer / findObject, setPosition
 * (the centre block through item_center). */
// FUNC 0x001537F8 gt2k_menu::DeleteMenu::makeSubMenu
inline MemorycardMenuResult DeleteMenu::makeSubMenu(MemorycardControl &control)
{
    PageLayout *layout;
    Transitive *parent;
    MemorycardFile *file;
    int port;

    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions below and their landing pads),
     * MemorycardLayerMenu, setPosition, getLayer, findObjectIn, ~MemorycardLayerMenu (the inline
     * ConfirmMenu constructor and its cleanup), MemorycardLayerMenuItem,
     * MemorycardMenuItemItem (the items' inline constructors) */
    icon_slot_release(0);
    subMenu = new ConfirmMenu(control, 1, "confirm", "confirm_window", "confirm_icon_face",
                              g_00351848, g_0035184C);
    layout = control.pages.getFragmentManager();
    parent = subMenu->root;
    subMenu->append(new DeleteWorkingMenu(layout, parent, "confirm_m_ok"));
    subMenu->append(new MemorycardCancelItem(layout, parent, "confirm_m_cancel", MC_RESULT_CANCEL));
    subMenu->selectLast();
    subMenu->attachObject(new Item("confirm_text_delete", layout));
    file = control.getSelectedFile();
    port = control.getCurrentPort();
    if (file != NULL && port >= 0)
        file->readIcon(port, 2);

    MemorycardMenuResult r(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);
    return r;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* CommentMenu                                                              */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function of the base of OverwriteMenu,
 * SaveMenu and CommentChangeMenu (unnamed in index.txt; the name string
 * 'Q29gt2k_menu11CommentMenu' is at 0x0031EC40). */
// FUNC 0x001537A8 __tfQ29gt2k_menu11CommentMenu
extern "C" void *__tfQ29gt2k_menu11CommentMenu(void)
{
    if (__tiQ29gt2k_menu11CommentMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu11CommentMenu, "Q29gt2k_menu11CommentMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu11CommentMenu;
}
#endif

namespace gt2k_menu {

/* The items that edit the save's comment first.  No vtable of its own is
 * emitted (the inline constructor's vptr store is dead), nor its implicit
 * destructor; typeinfo __tfQ29gt2k_menu11CommentMenu 0x001537A8 (si, base
 * MemorycardLayerMenuItem; storage 0x004B1B70).  Size 0x2C. */
class CommentMenu : public MemorycardLayerMenuItem {
public:
    CommentMenu(PageLayout *layout, Object *parent, const char *name, int pending)
        : MemorycardLayerMenuItem(layout, parent, name), pending(pending), counter(0) {}
    /* [0] ~CommentMenu: implicit (no vptr store: the derived destructors call
     * ~MemorycardLayerMenuItem directly) */
    /* A sub menu open: MemorycardLayerMenuItem::select.  Back from the name
     * entry (pending): on the fourth call the edited comment goes into the
     * selected GT3 save and makeSubMenu opens the confirm window.  Else,
     * decided on a GT3 save: sound 7, preCommentEdit, the save's comment to
     * the name entry, {9, 1}. */
    virtual MemorycardMenuResult select(MemorycardControl &control); /* [3] +0x24 0x00153608 */
    virtual void updateItem(MemorycardControl &control, bool sel); /* [9] +0x54 0x00153600 (empty) */
    /* Sets control's commentMode (which item ran the name entry). */
    virtual void preCommentEdit(MemorycardControl &control) = 0; /* [10] +0x5C */

    /*  MemorycardLayerMenuItem                0x00 */
    int pending;                            /* 0x24 back from the name entry: finish the edit */
    int counter;                            /* 0x28 frames since then */
};
EGCS_SIZE(gt2k_menu::CommentMenu, 0x2C);

/* With a sub menu open: MemorycardLayerMenuItem::select.  Back from the name
 * entry (pending): on the fourth call the edited comment goes into the
 * selected GT3 save and the item's confirm window opens (makeSubMenu's
 * result).  Else, decided on a GT3 save: the decide sound, preCommentEdit
 * (which item it was), the save's comment is copied to the name entry and
 * the result asks for it ({9, 1}).  (No isSelectable check, unlike
 * checkDecide: the menu's cursor only stops on selectable items.) */
// FUNC 0x00153608 gt2k_menu::CommentMenu::select
inline MemorycardMenuResult CommentMenu::select(MemorycardControl &control)
{
    /* (the decide branch first: retail falls through into it and jumps to
     * the pending one; a `save' per branch: the decide one lives in an
     * s-register, the pending one dies at its strcpy) */
    if (subMenu != NULL)
        return MemorycardLayerMenuItem::select(control);
    if (!pending) {
        if (control.control1.decide) {
            MemorycardGT3File *save = dynamic_cast<MemorycardGT3File *>(control.getSelectedFile());

            if (save != NULL) {
                menu_play_se(MENU_SE_CONFIRM);
                preCommentEdit(control);
                strcpy(control.comment, save->thumbnail.comment);

                MemorycardMenuResult r(MC_RESULT_NAME_ENTRY, 1);
                return r;
            }
        }
    } else if (++counter == COMMENT_MENU_WAIT) {
        MemorycardGT3File *save = dynamic_cast<MemorycardGT3File *>(control.getSelectedFile());

        if (save != NULL)
            strcpy(save->thumbnail.comment, control.comment);
        pending = 0;
        return makeSubMenu(control);
    }

    MemorycardMenuResult r(MC_RESULT_NONE, 0);   /* sp+0x08 */
    return r;
}

// FUNC 0x00153600 gt2k_menu::CommentMenu::updateItem
inline void CommentMenu::updateItem(MemorycardControl &control, bool sel)
{
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* CommentChangeWorkingMenu                                                 */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00155C58 __tfQ29gt2k_menu24CommentChangeWorkingMenu
extern "C" void *__tfQ29gt2k_menu24CommentChangeWorkingMenu(void)
{
    if (__tiQ29gt2k_menu24CommentChangeWorkingMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu24CommentChangeWorkingMenu,
                  "Q29gt2k_menu24CommentChangeWorkingMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu24CommentChangeWorkingMenu;
}
#endif

// FUNC 0x00155CA8 gt2k_menu::CommentChangeWorkingMenu::~CommentChangeWorkingMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "confirm_m_ok" of CommentChangeMenu.  vtable 0x0031F148 (10 slots: [3]
 * MemorycardLayerMenuItem::select, [4] MemorycardMenuItemItem::isSelectable).
 * typeinfo __tfQ29gt2k_menu24CommentChangeWorkingMenu 0x00155C58 (si, base
 * MemorycardLayerMenuItem; storage 0x004B1AB0).  Size 0x24. */
class CommentChangeWorkingMenu : public MemorycardLayerMenuItem {
public:
    CommentChangeWorkingMenu(PageLayout *layout, Object *parent, const char *name)
        : MemorycardLayerMenuItem(layout, parent, name) {}
    /* [0] +0x0C 0x00155CA8 ~CommentChangeWorkingMenu: implicit (no vptr store) */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x001556B8 */
};
EGCS_SIZE(gt2k_menu::CommentChangeWorkingMenu, 0x24);

/* "working": writes the name entry's comment into the selected GT3 save
 * (changeComment).  Write errors return {0xB, write error dialog}.  Then
 * "working_text_complete", 4 frames, the bar is released, "working_m_ok"
 * (MC_RESULT_COMMENT_DONE) added, the file list read again and the info
 * panel rebuilt. */
// FUNC 0x001556B8 gt2k_menu::CommentChangeWorkingMenu::makeSubMenu
inline MemorycardMenuResult CommentChangeWorkingMenu::makeSubMenu(MemorycardControl &control)
{
    PageLayout *layout;                         /* s3 */
    Item *textSave;                             /* s7 */
    Item *textWarning;                          /* s6 */
    Transitive *bar;                            /* fp */
    MemorycardFile *file;                       /* s0 */
    MemorycardGT3File *save;                    /* s1 */
    MemorycardFileManagerBase *manager;
    int port;                                   /* s5 */

    /* implicit: operator new, operator delete, MemorycardLayerMenu,
     * setPosition, getLayer, findObjectIn, ~MemorycardLayerMenu (the inline ConfirmMenu constructor
     * and its cleanup), MemorycardMenuItemItem (MemorycardCancelItem's inline
     * constructor) */
    control.progressPrev = 0.0f;
    control.progress = 0.0f;
    /* expands ConfirmMenu's constructor: MemorycardLayerMenu's constructor,
     * PageLayout::getLayer / findObject, setPosition */
    subMenu = new ConfirmMenu(control, 0, "working", "working_window", "working_icon_face",
                              g_00351848, g_0035184C);
    layout = control.pages.getFragmentManager();
    textSave = new Item("working_text_save", layout);
    textWarning = new Item("working_text_warning", layout);
    bar = layout->createObject(layout->findObject("working_bar_base", "hidden"), subMenu->root, NULL);
    subMenu->attachObject(textSave);
    subMenu->attachObject(textWarning);
    subMenu->attachObject(bar);
    control.pauseThreads();
    file = control.getSelectedFile();
    port = control.getCurrentPort();
    save = dynamic_cast<MemorycardGT3File *>(file);
    if (save != NULL && file != NULL) {
        if (mc_slot_refresh(port) != 0) {       /* the card was changed */
            /* sp+0x00 */
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                        new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
        }
        if (save->changeComment(port, control.comment, memorycard_set_progress, &control) == 0) {
            /* sp+0x08 */
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                        new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
        }
    }

    menu_play_se(MENU_SE_CONFIRM);
    textSave->virtual_4(NULL);                  /* close (fade out) */
    textWarning->virtual_4(NULL);
    subMenu->attachObject(new Item("working_text_complete", layout));
    gfx_vsync(MC_COMPLETE_WAIT);
    bar->kill();
    /* (MemorycardCancelItem's inline constructor: MemorycardMenuItemItem's) */
    {
        Transitive *parent = subMenu->root;

        subMenu->append(new MemorycardCancelItem(layout, parent, "working_m_ok", MC_RESULT_COMMENT_DONE));
    }
    manager = control.getCurrentManager();
    if (manager != NULL)
        manager->updateFileId();
    control.restartThreads();
    control.slots.resetInfo();

    MemorycardMenuResult result(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);  /* sp+0x10 */

    return result;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* CommentChangeMenu                                                        */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00153568 __tfQ29gt2k_menu17CommentChangeMenu
extern "C" void *__tfQ29gt2k_menu17CommentChangeMenu(void)
{
    if (__tiQ29gt2k_menu17CommentChangeMenu[0] == 0) {
        __tfQ29gt2k_menu11CommentMenu();
        __rtti_si(__tiQ29gt2k_menu17CommentChangeMenu, "Q29gt2k_menu17CommentChangeMenu",
                  (const type_info *)__tiQ29gt2k_menu11CommentMenu);
    }
    return __tiQ29gt2k_menu17CommentChangeMenu;
}
#endif

// FUNC 0x001535B8 gt2k_menu::CommentChangeMenu::~CommentChangeMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "op1_m_comment" / "op2_m_comment".  vtable 0x0031EA00 (11 slots: [3]
 * CommentMenu::select, [4] MemorycardMenuItemItem::isSelectable, [9]
 * CommentMenu::updateItem).  typeinfo __tfQ29gt2k_menu17CommentChangeMenu
 * 0x00153568 (si, base CommentMenu; storage 0x004B1AA0).  Size 0x2C. */
class CommentChangeMenu : public CommentMenu {
public:
    CommentChangeMenu(PageLayout *layout, Object *parent, const char *name,
                      MemorycardControl &control, int afterNameEntry);
    /* [0] +0x0C 0x001535B8 ~CommentChangeMenu: implicit (no vptr store) */
    /* "confirm" (OK CommentChangeWorkingMenu). */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00153220 */
    virtual void preCommentEdit(MemorycardControl &control); /* [10] +0x5C 0x00153550 commentMode 2 */
};
EGCS_SIZE(gt2k_menu::CommentChangeMenu, 0x2C);

/* Back from the name entry (afterNameEntry), the item stays pending (its
 * confirm window comes up) only if the comment was changed: pending is
 * cleared unless the selected file is a MemorycardGT3File whose comment
 * differs from control's (strcmp).  Inline (the MemorycardOperationMenu
 * constructor expands it; no out-of-line copy). */
inline CommentChangeMenu::CommentChangeMenu(PageLayout *layout, Object *parent, const char *name,
                                            MemorycardControl &control, int afterNameEntry)
    : CommentMenu(layout, parent, name, afterNameEntry)
{
    if (afterNameEntry) {
        MemorycardGT3File *save = dynamic_cast<MemorycardGT3File *>(control.getSelectedFile());

        if (save == NULL || strcmp(save->thumbnail.comment, control.comment) == 0)
            pending = 0;
    }
}

// FUNC 0x00153550 gt2k_menu::CommentChangeMenu::preCommentEdit
inline void CommentChangeMenu::preCommentEdit(MemorycardControl &control)
{
    control.commentMode = COMMENT_MODE_CHANGE;
}

/* "confirm": OK writes the new comment (CommentChangeWorkingMenu), Cancel
 * gives code 3; the cursor starts on Cancel.  new ConfirmMenu expands its
 * constructor here: MemorycardLayerMenu's, PageLayout::getLayer /
 * findObject, setPosition (the centre block through item_center). */
// FUNC 0x00153220 gt2k_menu::CommentChangeMenu::makeSubMenu
inline MemorycardMenuResult CommentChangeMenu::makeSubMenu(MemorycardControl &control)
{
    PageLayout *layout;
    Transitive *parent;

    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions below and their landing pads),
     * MemorycardLayerMenu, setPosition, getLayer, findObjectIn, ~MemorycardLayerMenu (the inline
     * ConfirmMenu constructor and its cleanup), MemorycardLayerMenuItem,
     * MemorycardMenuItemItem (the items' inline constructors) */
    subMenu = new ConfirmMenu(control, 1, "confirm", "confirm_window", "confirm_icon_face",
                              g_00351848, g_0035184C);
    layout = control.pages.getFragmentManager();
    parent = subMenu->root;
    subMenu->append(new CommentChangeWorkingMenu(layout, parent, "confirm_m_ok"));
    subMenu->append(new MemorycardCancelItem(layout, parent, "confirm_m_cancel",
                                             MC_RESULT_COMMENT_CANCEL));
    subMenu->selectLast();
    subMenu->attachObject(new Item("confirm_text_name_change", layout));

    MemorycardMenuResult r(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);
    return r;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* SaveWorkingMenu                                                          */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00155620 __tfQ29gt2k_menu15SaveWorkingMenu
extern "C" void *__tfQ29gt2k_menu15SaveWorkingMenu(void)
{
    if (__tiQ29gt2k_menu15SaveWorkingMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu15SaveWorkingMenu, "Q29gt2k_menu15SaveWorkingMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu15SaveWorkingMenu;
}
#endif

// FUNC 0x00155670 gt2k_menu::SaveWorkingMenu::~SaveWorkingMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "confirm2_m_ok" of SaveMenu (newSave 1) and OverwriteMenu (newSave 0):
 * writes the new save ("working2"; an overwrite deletes the old one first).
 * vtable 0x0031F0F0 (10 slots: [3] MemorycardLayerMenuItem::select, [4]
 * MemorycardMenuItemItem::isSelectable).  typeinfo
 * __tfQ29gt2k_menu15SaveWorkingMenu 0x00155620 (si, base
 * MemorycardLayerMenuItem; storage 0x004B1AF0).  Size 0x2C. */
class SaveWorkingMenu : public MemorycardLayerMenuItem {
public:
    SaveWorkingMenu(PageLayout *layout, Object *parent, const char *name, int newSave, bool format)
        : MemorycardLayerMenuItem(layout, parent, name), newSave(newSave), format(format) {}
    /* [0] +0x0C 0x00155670 ~SaveWorkingMenu: implicit (no vptr store) */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00154F70 */

    /*  MemorycardLayerMenuItem                0x00 */
    int newSave;                            /* 0x24 0: overwrite the selected file */
    /* A bool: makeSubMenu passes it to MemorycardFile::save's bool format
     * with a bare lw (an int would need egcs's sltu), and SaveMenu stores
     * its flag as is.  T075's WorkingMenu passes save a bool too (its
     * !formatted is egcs's bool negation, xori 1). */
    bool format;                            /* 0x28 format the card first */
};
EGCS_SIZE(gt2k_menu::SaveWorkingMenu, 0x2C);

} /* namespace gt2k_menu */

/* The progress callbacks of an overwrite's delete (the first half of the
 * bar) and write (the second half).  Inline in the original: egcs writes
 * them at the end of the unit, after SaveWorkingMenu::makeSubMenu, the only
 * function that takes their address. */
// FUNC 0x001555F8 memorycard_progress_first_half
static inline void memorycard_progress_first_half(f32 progress, void *arg)
{
    MemorycardControl *control = (MemorycardControl *)arg;

    control->progressPrev = control->progress;
    control->progress = progress * 0.5f;
}

// FUNC 0x001555D0 memorycard_progress_second_half
static inline void memorycard_progress_second_half(f32 progress, void *arg)
{
    MemorycardControl *control = (MemorycardControl *)arg;

    control->progressPrev = control->progress;
    control->progress = progress * 0.5f + 0.5f;
}

namespace gt2k_menu {

/* "working2" (a plain MemorycardLayerMenu, no icon): writes the new save to
 * the card of the current slot.  The selected file must be a GT3 save.  A
 * new save is written as the selected entry itself (the new-file entry);
 * an overwrite takes control.newFile, deletes the old file first (progress
 * 0..0.5) and then writes (0.5..1).  The save gets the control's thumbnail
 * and the name entry's comment.  Write errors return {0xB, write error
 * dialog}.  Then "working2_text_complete", 4 frames, the bar is released and
 * "working2_m_ok" (MC_RESULT_SAVE_DONE) added; the file list is read again. */
// FUNC 0x00154F70 gt2k_menu::SaveWorkingMenu::makeSubMenu
inline MemorycardMenuResult SaveWorkingMenu::makeSubMenu(MemorycardControl &control)
{
    PageLayout *layout;                         /* s5 */
    Item *textSave;                             /* s7 */
    Item *textWarning;                          /* s6 */
    Transitive *bar;                            /* fp */
    MemorycardFile *file;                       /* s1 the file written */
    MemorycardGT3File *save;                    /* s0 */
    MemorycardFileManagerBase *manager;
    int port;                                   /* s4 */

    /* implicit: operator new, operator delete, MemorycardMenuItemItem
     * (MemorycardCancelItem's inline constructor) */
    control.progressPrev = 0.0f;
    control.progress = 0.0f;
    subMenu = new MemorycardLayerMenu(0, 0, control.pages.getFragmentManager(),
                                      control.pages.getLayer(),
                                      "working2", "working2_window");
    subMenu->setPosition(g_00351848, g_0035184C);
    layout = control.pages.getFragmentManager();
    textSave = new Item("working2_text_save", layout);
    textWarning = new Item("working2_text_warning", layout);
    bar = layout->createObject(layout->findObject("working2_bar_base", "hidden"), subMenu->root, NULL);
    subMenu->attachObject(textSave);
    subMenu->attachObject(textWarning);
    subMenu->attachObject(bar);
    control.pauseThreads();
    file = control.getSelectedFile();
    port = control.getCurrentPort();
    save = dynamic_cast<MemorycardGT3File *>(file);
    if (save != NULL && file != NULL) {
        if (newSave == 0) {
            save = dynamic_cast<MemorycardGT3File *>(control.newFile);
            if (save == NULL) {
                /* sp+0x00 */
                return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                            new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
            }
            if (mc_slot_refresh(port) != 0) {   /* the card was changed */
                /* sp+0x08 */
                return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                            new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
            }
            if (file->deleteDir(port, memorycard_progress_first_half, &control) == 0) {
                /* sp+0x10 */
                return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                            new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
            }
            file = save;
        }
        save->setThumbnail(control.thumbnail);
        strcpy(save->thumbnail.comment, control.comment);
        void *data = control.data;
        int size = control.dataSize;

        if (file->save(port, data, size, format,
                       newSave != 0 ? memorycard_set_progress : memorycard_progress_second_half,
                       &control) == 0) {
            /* sp+0x18 */
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                        new MemorycardWriteErrorDialog(control, (int)g_00351848, (int)g_0035184C));
        }
    }

    menu_play_se(MENU_SE_CONFIRM);
    textSave->virtual_4(NULL);                  /* close (fade out) */
    textWarning->virtual_4(NULL);
    subMenu->attachObject(new Item("working2_text_complete", layout));
    gfx_vsync(MC_COMPLETE_WAIT);
    bar->kill();
    /* (MemorycardCancelItem's inline constructor: MemorycardMenuItemItem's) */
    {
        Transitive *parent = subMenu->root;

        subMenu->append(new MemorycardCancelItem(layout, parent, "working2_m_ok", MC_RESULT_SAVE_DONE));
    }
    control.slots.releaseInfo();
    manager = control.getCurrentManager();
    if (manager != NULL)
        manager->updateFileId();
    control.restartThreads();
    control.slots.resetInfo();

    MemorycardMenuResult result(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);  /* sp+0x20 */

    return result;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* SaveMenu                                                                 */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00153188 __tfQ29gt2k_menu8SaveMenu
extern "C" void *__tfQ29gt2k_menu8SaveMenu(void)
{
    if (__tiQ29gt2k_menu8SaveMenu[0] == 0) {
        __tfQ29gt2k_menu11CommentMenu();
        __rtti_si(__tiQ29gt2k_menu8SaveMenu, "Q29gt2k_menu8SaveMenu",
                  (const type_info *)__tiQ29gt2k_menu11CommentMenu);
    }
    return __tiQ29gt2k_menu8SaveMenu;
}
#endif

// FUNC 0x001531D8 gt2k_menu::SaveMenu::~SaveMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "op4_m_save" (the new-file entry of the list).  vtable 0x0031E9A0 (11
 * slots: [3] CommentMenu::select, [4] MemorycardMenuItemItem::isSelectable,
 * [9] CommentMenu::updateItem).  typeinfo __tfQ29gt2k_menu8SaveMenu
 * 0x00153188 (si, base CommentMenu; storage 0x004B1AE0).  Size 0x2C. */
class SaveMenu : public CommentMenu {
public:
    SaveMenu(PageLayout *layout, Object *parent, const char *name, int pending)
        : CommentMenu(layout, parent, name, pending) {}
    /* [0] +0x0C 0x001531D8 ~SaveMenu: implicit (no vptr store) */
    /* "confirm2" (OK SaveWorkingMenu newSave 1, "confirm2_text_save"), the
     * format question for an unformatted card, or the "nospace" dialog. */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00152D68 */
    virtual void preCommentEdit(MemorycardControl &control); /* [10] +0x5C 0x00153178 commentMode 0 */
    /* (? name) makeSubMenu's card check, inline (no out-of-line copy): an
     * unformatted card sets format; a card without the room for newFile gets
     * the "nospace" dialog in place of subMenu and 0.  Its returns are
     * retail's flag in $v0 (set on each path just before the join, the
     * format path's `return 1' shared by the others), u029. */
    int checkCard(MemorycardControl &control, MemorycardFile *newFile, int port, bool &format);
};
EGCS_SIZE(gt2k_menu::SaveMenu, 0x2C);

// FUNC 0x00153178 gt2k_menu::SaveMenu::preCommentEdit
inline void SaveMenu::preCommentEdit(MemorycardControl &control)
{
    control.commentMode = COMMENT_MODE_SAVE;
}

/* "confirm2": OK saves the new file (SaveWorkingMenu newSave 1, format).
 * On an unformatted card the text asks to format the card in slot 1 / 2
 * first and the cursor starts on Cancel (no space check then); if the card
 * lacks the new save's size, the "nospace" dialog replaces the window
 * (checkCard). */
// FUNC 0x00152D68 gt2k_menu::SaveMenu::makeSubMenu
/* The card check: an unformatted card asks for the format first (no space
 * check then); a card without the room for the new save gets the "nospace"
 * dialog in place of the window, and 0. */
inline int SaveMenu::checkCard(MemorycardControl &control, MemorycardFile *newFile, int port, bool &format)
{
    if (port >= 0 && mc_slot_formatted(port) == 0) {
        format = true;
        return 1;
    }
    if (newFile != NULL && port >= 0) {
        int freeKB = mc_slot_free(port);
        int needKB = newFile->sizeKB;

        if (freeKB - MC_FREE_MARGIN_KB < needKB) {
            delete subMenu;
            subMenu = new MemorycardNoSpaceDialog(control, (int)g_00351848, (int)g_0035184C, needKB);
            return 0;
        }
    }
    return 1;
}

inline MemorycardMenuResult SaveMenu::makeSubMenu(MemorycardControl &control)
{
    bool format;                        /* the card is not formatted: format it first */
    MemorycardFile *newFile;
    int port;

    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions below and their landing pads),
     * MemorycardLayerMenuItem, MemorycardMenuItemItem (the items' inline
     * constructors) */
    subMenu = new MemorycardLayerMenu(0, 1, control.pages.getFragmentManager(),
                                      control.pages.getLayer(),
                                      "confirm2", "confirm2_window");
    format = false;
    subMenu->setPosition(g_00351848, g_0035184C);
    newFile = control.newFile;
    port = control.getCurrentPort();
    if (checkCard(control, newFile, port, format)) {
        PageLayout *layout = control.pages.getFragmentManager();
        Transitive *parent = subMenu->root;

        subMenu->append(new SaveWorkingMenu(layout, parent, "confirm2_m_ok", 1, format));
        subMenu->append(new MemorycardCancelItem(layout, parent, "confirm2_m_cancel", MC_RESULT_CANCEL));
        if (!format) {
            subMenu->attachObject(new Item("confirm2_text_save", layout));
        } else {
            char text[0x40];                    /* sp+0x00 */

            subMenu->selectLast();
            sprintf(text, "confirm2_text_format_save_%d", control.getCurrentPort() == 0 ? 1 : 2);
            subMenu->attachObject(new Item(text, layout));
            subMenu->attachObject(new Item("confirm2_text_approve", layout));
        }
    }

    MemorycardMenuResult r(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);  /* sp+0x40 */
    return r;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* OverwriteMenu                                                            */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00152CD0 __tfQ29gt2k_menu13OverwriteMenu
extern "C" void *__tfQ29gt2k_menu13OverwriteMenu(void)
{
    if (__tiQ29gt2k_menu13OverwriteMenu[0] == 0) {
        __tfQ29gt2k_menu11CommentMenu();
        __rtti_si(__tiQ29gt2k_menu13OverwriteMenu, "Q29gt2k_menu13OverwriteMenu",
                  (const type_info *)__tiQ29gt2k_menu11CommentMenu);
    }
    return __tiQ29gt2k_menu13OverwriteMenu;
}
#endif

// FUNC 0x00152D20 gt2k_menu::OverwriteMenu::~OverwriteMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0; CommentMenu's implicit destructor adds nothing), then
 * MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "op1_m_write" (save mode, over the selected file).  vtable 0x0031E940
 * (11 slots: [3] CommentMenu::select, [5] / [6] / [8]
 * MemorycardLayerMenuItem's).  typeinfo __tfQ29gt2k_menu13OverwriteMenu
 * 0x00152CD0 (si, base CommentMenu; storage 0x004B1AD0).  Size 0x30
 * (operator new in the MemorycardOperationMenu constructor). */
class OverwriteMenu : public CommentMenu {
public:
    OverwriteMenu(PageLayout *layout, Object *parent, const char *name, int pending)
        : CommentMenu(layout, parent, name, pending), selectable(1) {}
    /* [0] +0x0C 0x00152D20 ~OverwriteMenu: implicit (no vptr store) */
    virtual bool isSelectable(MemorycardControl &control); /* [4] +0x2C 0x00152CB0 selectable */
    /* "confirm2" (OK SaveWorkingMenu newSave 0, "confirm2_text_overwrite"),
     * or the "nospace" dialog (T076) when the card lacks the room. */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00152948 */
    virtual void updateItem(MemorycardControl &control, bool sel); /* [9] +0x54 0x00152C60
                                               selectable: the file's size is known */
    virtual void preCommentEdit(MemorycardControl &control); /* [10] +0x5C 0x00152CB8 commentMode 1 */

    /*  CommentMenu                            0x00 */
    /* bool (4 bytes in egcs): isSelectable returns it with a bare lw (an int
     * would need egcs's sltu for the bool result) and updateItem stores a
     * compare's 0 / 1 and dims with xori 1 (u029; was int). */
    bool selectable;                        /* 0x2C the selected file's size is known */
};
EGCS_SIZE(gt2k_menu::OverwriteMenu, 0x30);

// FUNC 0x00152CB8 gt2k_menu::OverwriteMenu::preCommentEdit
inline void OverwriteMenu::preCommentEdit(MemorycardControl &control)
{
    control.commentMode = COMMENT_MODE_OVERWRITE;
}

// FUNC 0x00152CB0 gt2k_menu::OverwriteMenu::isSelectable
inline bool OverwriteMenu::isSelectable(MemorycardControl &control)
{
    return selectable;
}

/* Selectable (and not dimmed) only while the selected file's size is
 * known (sizeKB -1 unknown, -2 error, -3 invalid). */
// FUNC 0x00152C60 gt2k_menu::OverwriteMenu::updateItem
inline void OverwriteMenu::updateItem(MemorycardControl &control, bool sel)
{
    MemorycardFile *file = control.getSelectedFile();

    if (file == NULL)
        selectable = false;
    else
        selectable = file->sizeKB >= 0;
    dim = !selectable;
}

/* "confirm2": OK overwrites the selected file with the new one
 * (SaveWorkingMenu newSave 0, no format), the cursor starts on Cancel.  If
 * the card lacks the room the new save needs beyond the old one, the
 * "nospace" dialog replaces the window. */
// FUNC 0x00152948 gt2k_menu::OverwriteMenu::makeSubMenu
inline MemorycardMenuResult OverwriteMenu::makeSubMenu(MemorycardControl &control)
{
    int noSpace = 0;
    MemorycardFile *newFile;
    MemorycardFile *file;
    int port;

    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions below and their landing pads),
     * MemorycardLayerMenuItem, MemorycardMenuItemItem (the items' inline
     * constructors) */
    subMenu = new MemorycardLayerMenu(0, 1, control.pages.getFragmentManager(),
                                      control.pages.getLayer(),
                                      "confirm2", "confirm2_window");
    subMenu->setPosition(g_00351848, g_0035184C);
    newFile = control.newFile;
    file = control.getSelectedFile();
    port = control.getCurrentPort();
    if (newFile != NULL && file != NULL && port >= 0) {
        int freeKB = mc_slot_free(port);
        int needKB = newFile->sizeKB - file->sizeKB;

        if (freeKB - MC_FREE_MARGIN_KB < needKB && needKB > 0) {
            delete subMenu;
            subMenu = new MemorycardNoSpaceDialog(control, (int)g_00351848, (int)g_0035184C, needKB);
            noSpace = 1;
        }
    }
    if (!noSpace) {
        PageLayout *layout = control.pages.getFragmentManager();
        Transitive *parent = subMenu->root;

        subMenu->append(new SaveWorkingMenu(layout, parent, "confirm2_m_ok", 0, false));
        subMenu->append(new MemorycardCancelItem(layout, parent, "confirm2_m_cancel", MC_RESULT_CANCEL));
        subMenu->selectLast();
        subMenu->attachObject(new Item("confirm2_text_overwrite", layout));
    }

    MemorycardMenuResult r(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);
    return r;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* LoadWorkingMenu                                                          */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00154ED8 __tfQ29gt2k_menu15LoadWorkingMenu
extern "C" void *__tfQ29gt2k_menu15LoadWorkingMenu(void)
{
    if (__tiQ29gt2k_menu15LoadWorkingMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu15LoadWorkingMenu, "Q29gt2k_menu15LoadWorkingMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu15LoadWorkingMenu;
}
#endif

// FUNC 0x00154F28 gt2k_menu::LoadWorkingMenu::~LoadWorkingMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "confirm_m_ok" of LoadMenu: loads the selected save and deserialises it
 * (Serialize, T509).  vtable 0x0031F098 (10 slots: [4] MemorycardMenuItemItem::
 * isSelectable, [5] / [6] / [8] / [9] MemorycardLayerMenuItem's).  typeinfo
 * __tfQ29gt2k_menu15LoadWorkingMenu 0x00154ED8 (si, base
 * MemorycardLayerMenuItem; storage 0x004B1B50).  Size 0x28. */
class LoadWorkingMenu : public MemorycardLayerMenuItem {
public:
    LoadWorkingMenu(PageLayout *layout, Object *parent, const char *name)
        : MemorycardLayerMenuItem(layout, parent, name), done(0) {}
    /* [0] +0x0C 0x00154F28 ~LoadWorkingMenu: implicit (no vptr store) */
    /* {6, 0} once done, else MemorycardLayerMenuItem::select. */
    virtual MemorycardMenuResult select(MemorycardControl &control); /* [3] +0x24 0x001546D0 */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00154708 */

    /*  MemorycardLayerMenuItem                0x00 */
    int done;                               /* 0x24 the load went through */
};
EGCS_SIZE(gt2k_menu::LoadWorkingMenu, 0x28);

/* "working": loads the selected save.  The Serialize class follows the
 * screen's mode (the demo / replay loads "ReplayData", the ghost load
 * "GhostData", the logger's load "TrackData", the setting load
 * "SettingSerialize"; other modes load nothing).  The disc's demo files
 * (port 0x100) come from the volume, the others from the card (a read
 * shorter than the Serialize header is "file damage").  The header is
 * checked (another version: "version error") and the data deserialised into
 * control.arg; the save's thumbnail and date go to control.loadInfo.  Then
 * "working_text_complete", a wait of 60 frames, and done is set (no OK
 * item: the next select leaves).  The bar is "working2_bar_base" (sic: the
 * other working windows use "working_bar_base") and is not released here. */
// FUNC 0x00154708 gt2k_menu::LoadWorkingMenu::makeSubMenu
inline MemorycardMenuResult LoadWorkingMenu::makeSubMenu(MemorycardControl &control)
{
    const char *className;                      /* s2 */
    PageLayout *layout;                         /* s6 */
    Item *textLoad;                             /* fp */
    Item *textWarning;                          /* s7 */
    Transitive *bar;
    MemorycardFile *file;                       /* s3 */
    int mode;
    int port;                                   /* s0 */
    void *data;                                 /* s1 the save's data (malloc) */

    /* implicit: operator new, operator delete, MemorycardLayerMenu,
     * setPosition, getLayer, findObjectIn, ~MemorycardLayerMenu (the inline ConfirmMenu constructor
     * and its cleanup) */
    control.progressPrev = 0.0f;
    control.progress = 0.0f;
    /* expands ConfirmMenu's constructor: MemorycardLayerMenu's constructor,
     * PageLayout::getLayer / findObject, setPosition */
    subMenu = new ConfirmMenu(control, 0, "working", "working_window", "working_icon_face",
                              g_00351848, g_0035184C);
    layout = control.pages.getFragmentManager();
    textLoad = new Item("working_text_load", layout);
    textWarning = new Item("working_text_warning", layout);
    className = NULL;
    bar = layout->createObject(layout->findObject("working2_bar_base", "hidden"), subMenu->root, NULL);
    subMenu->attachObject(textLoad);
    subMenu->attachObject(textWarning);
    subMenu->attachObject(bar);
    control.pauseThreads();
    file = control.getSelectedFile();
    mode = control.mode;
    port = control.getCurrentPort();
    switch (mode) {                             /* jump table 0x0031F2C0, modes 0x0C..0x17 */
    case MemorycardControl::MODE_DEMO_REPLAY_LOAD:
    case MemorycardControl::MODE_REPLAY_LOAD:
        className = g_00351F40;                 /* "ReplayData" */
        break;
    case MemorycardControl::MODE_GHOST_LOAD:
        className = g_00351F54;                 /* "GhostData" */
        break;
    case MemorycardControl::MODE_LOGGER_LOAD:
        className = g_00351F60;                 /* "TrackData" */
        break;
    case MemorycardControl::MODE_SETTING_LOAD:
        className = "SettingSerialize";
        break;
    }

    if (file != NULL && className != NULL) {
        if (port == MC_PORT_DEMO_VOLUME) {
            data = file->loadFromVolume(memorycard_set_progress, &control);
        } else {
            if (mc_slot_refresh(port) != 0)     /* the card was changed (sp+0x00) */
                return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                            new MemorycardReadErrorDialog(control, (int)g_00351848,
                                                                               (int)g_0035184C, 0, 0));
            data = file->load(port, memorycard_set_progress, &control);
            if (data != NULL && mc_slot_read_size(port) < (int)sizeof(Serialize::Header)) {
                free(data);
                /* damaged (sp+0x08) */
                return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                            new MemorycardReadErrorDialog(control, (int)g_00351848,
                                                                               (int)g_0035184C, 1, 0));
            }
        }
        if (data != NULL) {
            Serialize::Base *loader = serialize_create(className);  /* (no NULL check) */
            const void *end;

            if (loader->checkHeader(data, control.arg) >= SERIALIZE_VERSION_DIFFERS) {
                serialize_destroy(loader);
                free(data);
                /* version error (sp+0x10) */
                return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                            new MemorycardReadErrorDialog(control, (int)g_00351848,
                                                                               (int)g_0035184C, 0, 1));
            }
            end = loader->restore(data, control.arg);  /* NULL: bad checksum */
            serialize_destroy(loader);
            free(data);
            if (end == NULL)                    /* damaged (sp+0x18) */
                return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                            new MemorycardReadErrorDialog(control, (int)g_00351848,
                                                                               (int)g_0035184C, 1, 0));

            MemorycardGT3File *save = dynamic_cast<MemorycardGT3File *>(file);
            MemorycardLoadInfo *info = control.loadInfo;

            if (info != NULL && save != NULL) {
                info->thumbnail = save->thumbnail;
                info->date = save->dir.entry.create;
            }
        } else {
            int damaged = 0;
            /* (the demo port 0x100 is no card: status 1, not damaged) */
            int status = mc_slot_status(port);

            if (status == MC_STATUS_NO_ENTRY || status == MC_STATUS_OK)
                damaged = 1;
            /* (sp+0x20) */
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                                        new MemorycardReadErrorDialog(control, (int)g_00351848,
                                                                           (int)g_0035184C, damaged, 0));
        }
    }

    menu_play_se(MENU_SE_CONFIRM);
    textLoad->virtual_4(NULL);                  /* close (fade out) */
    textWarning->virtual_4(NULL);
    subMenu->attachObject(new Item("working_text_complete", layout));
    control.restartThreads();
    gfx_vsync(MC_LOAD_COMPLETE_WAIT);

    MemorycardMenuResult result(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);  /* sp+0x28 */

    done = 1;
    return result;
}

/* Once the load went through: {6, 0} (T071 leaves the screen with result
 * 1).  Else MemorycardLayerMenuItem::select. */
// FUNC 0x001546D0 gt2k_menu::LoadWorkingMenu::select
inline MemorycardMenuResult LoadWorkingMenu::select(MemorycardControl &control)
{
    if (done) {
        MemorycardMenuResult r(MC_RESULT_LEAVE, 0);
        return r;
    }
    return MemorycardLayerMenuItem::select(control);
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* LoadMenu                                                                 */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x001528B0 __tfQ29gt2k_menu8LoadMenu
extern "C" void *__tfQ29gt2k_menu8LoadMenu(void)
{
    if (__tiQ29gt2k_menu8LoadMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu8LoadMenu, "Q29gt2k_menu8LoadMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu8LoadMenu;
}
#endif

// FUNC 0x00152900 gt2k_menu::LoadMenu::~LoadMenu
/* implicit (below); egcs emits this copy: ~MemorycardLayerMenuItem (flags
 * 0), then MemorycardMenuItem's operator delete when flags & 1 */
/* implicit: operator delete (compiler-generated destructor) */

namespace gt2k_menu {

/* "op1_m_load".  vtable 0x0031E8E8 (10 slots: [1] / [2] pCell's, [3]
 * MemorycardLayerMenuItem::select 0x00151510, [4] MemorycardMenuItemItem::
 * isSelectable 0x00151860, [5] / [6] / [8] / [9] MemorycardLayerMenuItem's).
 * typeinfo __tfQ29gt2k_menu8LoadMenu 0x001528B0 (si, base
 * MemorycardLayerMenuItem; storage 0x004B1B20).  Size 0x24. */
class LoadMenu : public MemorycardLayerMenuItem {
public:
    LoadMenu(PageLayout *layout, Object *parent, const char *name)
        : MemorycardLayerMenuItem(layout, parent, name) {}
    /* [0] +0x0C 0x00152900 ~LoadMenu: implicit (no vptr store) */
    /* "confirm" (OK LoadWorkingMenu, Cancel {0xA}; "confirm_text_load"), the
     * cursor on OK; {0, 0xA}. */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00152580 */
};
EGCS_SIZE(gt2k_menu::LoadMenu, 0x24);

/* "confirm": OK loads the file (LoadWorkingMenu), the cursor starts on OK.
 * new ConfirmMenu expands its constructor here: MemorycardLayerMenu's,
 * PageLayout::getLayer / findObject, setPosition (its centre block through
 * item_center, as in CommentChangeMenu and DeleteMenu::makeSubMenu). */
// FUNC 0x00152580 gt2k_menu::LoadMenu::makeSubMenu
inline MemorycardMenuResult LoadMenu::makeSubMenu(MemorycardControl &control)
{
    PageLayout *layout;
    Transitive *parent;

    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions below and their landing pads),
     * MemorycardLayerMenu, setPosition, getLayer, findObjectIn, ~MemorycardLayerMenu (the inline
     * ConfirmMenu constructor and its cleanup), MemorycardLayerMenuItem,
     * MemorycardMenuItemItem (the items' inline constructors) */
    subMenu = new ConfirmMenu(control, 1, "confirm", "confirm_window", "confirm_icon_face",
                              g_00351848, g_0035184C);
    layout = control.pages.getFragmentManager();
    parent = subMenu->root;
    subMenu->append(new LoadWorkingMenu(layout, parent, "confirm_m_ok"));
    subMenu->append(new MemorycardCancelItem(layout, parent, "confirm_m_cancel", MC_RESULT_CANCEL));
    subMenu->attachObject(new Item("confirm_text_load", layout));

    MemorycardMenuResult r(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);
    return r;
}

} /* namespace gt2k_menu */

/* ======================================================================== */
/* The layer tables and the out-of-line functions                           */
/* ======================================================================== */

/* The layers of the operation menus by MemorycardOperationMenu::TYPE and
 * their anchor elements (.data; their strings follow the makers' in
 * .rodata). */
const char *g_002A6D88[4] = {               /* 0x002A6D88 */
    "operation1", "operation2", "operation3", "operation4"
};
const char *g_002A6D98[4] = {               /* 0x002A6D98 */
    "op1_window", "op2_window", "op3_window", "op4_window"
};

/* The progress callback (MemorycardProgressFunc) the working menus pass to
 * the card operations: the control's progress bar goes to `progress'. */
// FUNC 0x00151AD0 memorycard_set_progress
void memorycard_set_progress(f32 progress, void *arg)
{
    MemorycardControl *control = (MemorycardControl *)arg;
    f32 prev = control->progress;

    control->progress = progress;
    control->progressPrev = prev;
}

namespace gt2k_menu {

/* The menu of the file decided in the list (type: TYPE), on the layer
 * "operation<type + 1>" next to the file's icon.  afterNameEntry: the menu
 * is rebuilt after the name entry; its cursor goes to the CommentMenu that
 * ran it, and while that item is still pending the menu starts hidden.  Then
 * slot 0's 3D icon is released and the centre of the confirm windows is
 * taken from "confirm_window".  The items' constructors are inline (every
 * `new' below expands MemorycardLayerMenuItem's constructor, the vptr and
 * the members; CommentChangeMenu's with its strcmp).  The iconDX / iconDY
 * block goes through item_center (exact with its field loads first). */
// FUNC 0x00151AE8 gt2k_menu::MemorycardOperationMenu::MemorycardOperationMenu
MemorycardOperationMenu::MemorycardOperationMenu(MemorycardControl &control, int type, int afterNameEntry)
    : MemorycardLayerMenu(1, 1, control.pages.getFragmentManager(), control.pages.getLayer(),
                          g_002A6D88[type], g_002A6D98[type])
{
    PageLayout *layout = control.pages.getFragmentManager();
    Transitive *parent = root;
    int load = control.isLoadMode();
    int save = control.isSaveMode();
    CommentMenu *commentItem = NULL;            /* sp+0x00 the item of commentMode */
    int commentMode = control.commentMode;      /* sp+0x04 */
    MenuLayoutItem *face = NULL;
    MenuLayoutItem *confirm;

    switch (type) {
    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions below and their landing pads),
     * MemorycardLayerMenuItem, ~MemorycardLayerMenuItem (the items' inline
     * constructors and their cleanup), getSelectedFile, __dynamic_cast, strcmp
     * (CommentChangeMenu's inline constructor) */
    /* (`append(commentItem = new ...)': the item is stored and appended in
     * one expression, inside the new expression's cleanup region, as retail
     * clears the cleanup flag only after the store, just before append) */
    case TYPE_MATCHING:
        if (save) {
            if (commentMode == COMMENT_MODE_OVERWRITE) {
                append(commentItem = new OverwriteMenu(layout, parent, "op1_m_write", afterNameEntry));
            } else {
                append(new OverwriteMenu(layout, parent, "op1_m_write", 0));
            }
        } else if (load) {
            append(new LoadMenu(layout, parent, "op1_m_load"));
        }
        if (!load) {
            append(new CopyMenu(layout, parent, "op1_m_copy"));
            append(new DeleteMenu(layout, parent, "op1_m_delete"));
            if (commentMode == COMMENT_MODE_CHANGE) {
                append(commentItem = new CommentChangeMenu(layout, parent, "op1_m_comment", control,
                                                           afterNameEntry));
            } else {
                append(new CommentChangeMenu(layout, parent, "op1_m_comment", control, 0));
            }
        }
        face = PageLayout::findObjectIn("op1_icon_face", layout->getLayer(g_002A6D88[type]));
        break;
    case TYPE_GT3:
        append(new CopyMenu(layout, parent, "op2_m_copy"));
        append(new DeleteMenu(layout, parent, "op2_m_delete"));
        append(commentItem = new CommentChangeMenu(layout, parent, "op2_m_comment", control,
                                                   afterNameEntry));
        face = PageLayout::findObjectIn("op2_icon_face", layout->getLayer(g_002A6D88[type]));
        break;
    case TYPE_OTHER:
        append(new CopyMenu(layout, parent, "op3_m_copy"));
        append(new DeleteMenu(layout, parent, "op3_m_delete"));
        face = PageLayout::findObjectIn("op3_icon_face", layout->getLayer(g_002A6D88[type]));
        break;
    case TYPE_NEW_FILE:
        if (save)
            append(commentItem = new SaveMenu(layout, parent, "op4_m_save", afterNameEntry));
        face = PageLayout::findObjectIn("op4_icon_face", layout->getLayer(g_002A6D88[type]));
        break;
    }

    if (afterNameEntry) {
        current = commentItem;
        /* (no NULL check: only a CommentMenu's name entry rebuilds the menu) */
        if (commentItem->pending)
            setShown(0, 1);                     /* hidden (faded out) */
    }
    if (face != NULL) {
        f32 wx, wy, fx, fy;

        item_center(anchor, &wx, &wy);
        item_center(face, &fx, &fy);
        iconDX = fx - wx;
        iconDY = fy - wy;
    }
    icon_slot_release(0);
    confirm = PageLayout::findItemIn("confirm_window", layout->getLayer("confirm"));
    memorycard_set_confirm_center(confirm->x + confirm->w * 0.5f,
                                  448.0f - (confirm->y + confirm->h * 0.5f));
}

/* {8, 0} once the card of the current slot cannot be used (removed ...),
 * else MemorycardMenu's select.  The class's key method (its first virtual
 * function defined out of line): the vtables and the inline members of
 * MemorycardOperationMenu are written in this unit. */
// FUNC 0x00152508 gt2k_menu::MemorycardOperationMenu::select
MemorycardMenuResult MemorycardOperationMenu::select(MemorycardControl &control)
{
    MemorycardSlot *slot = control.getCurrentSlot();

    if (slot == NULL || !slot->isReady()) {
        MemorycardMenuResult r(MC_RESULT_CARD_GONE, 0);
        return r;
    }
    return MemorycardMenu::select(control);
}

} /* namespace gt2k_menu */

// FUNC 0x00152570 memorycard_set_confirm_center
static void memorycard_set_confirm_center(f32 x, f32 y)
{
    g_00351848 = x;
    g_0035184C = y;
}
