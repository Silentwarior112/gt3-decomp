/* T074_memorycard_operation_menu.h - the operation menu of the memory card
 * file screens (translation unit T074, 0x00151AD0..0x00156A80: work units
 * u029 and u030, one source file, src/game/menu/memorycard/T074_memorycard_operation_menu.cpp).
 *
 * When a file of the list is decided, T071 builds a MemorycardOperationMenu
 * next to its icon (layers "operation1".."operation4" of
 * menu/memorycard.mbl).  Its items are MemorycardLayerMenuItems (T073):
 * deciding one runs makeSubMenu, which opens a confirm window ("confirm" /
 * "confirm2") whose OK item, a *WorkingMenu, does the card operation inside
 * its own makeSubMenu ("working" / "working2" with the progress bar, then
 * "..._text_complete" and an OK MemorycardCancelItem, T076); on an error it
 * returns {0xB, the read / write error dialog (T076)}.
 *   LoadMenu           "confirm"   OK LoadWorkingMenu
 *   OverwriteMenu      "confirm2"  OK SaveWorkingMenu (newSave 0), or the
 *                                  "nospace" dialog
 *   SaveMenu           "confirm2"  OK SaveWorkingMenu (newSave 1), the format
 *                                  question, or the "nospace" dialog
 *   CommentChangeMenu  "confirm"   OK CommentChangeWorkingMenu
 *   DeleteMenu         "confirm"   OK DeleteWorkingMenu
 *   CopyMenu           "confirm"   OK CopyWorkingMenu (to the other slot), or
 *                                  the read error / "nospace" dialog
 * OverwriteMenu, SaveMenu and CommentChangeMenu are CommentMenus: deciding
 * one first runs the name entry for the save's comment (result {9, 1}); T071
 * then rebuilds the operation menu with afterNameEntry set, the item waits
 * four frames, stores the comment and opens its confirm window.
 *
 * Only MemorycardOperationMenu is used outside T074 (T071).  The items, the
 * working menus and ConfirmMenu are T074's own classes, defined in its
 * source (since the merge, 2026-10-16): the binary has their inline members
 * (every virtual method, no key method), strings and vtables in T074 only,
 * each class's members written right after its typeinfo function and
 * implicit destructor, which egcs does only when the class is completed
 * right before its members (a class defined here, before all of them,
 * would put every typeinfo function and destructor after all the methods).
 *
 * Hierarchy (egcs offsets; T073 owns the bases): MemorycardLayerMenuItem
 * (0x24, vptr +0x08) <- the items (0x24..0x30); MemorycardLayerMenu (0x3C,
 * List<MemorycardMenuItem> at +0x0C) <- ConfirmMenu (0x3C),
 * MemorycardOperationMenu (0x44). */
#ifndef GAME_T074_MEMORYCARD_OPERATION_MENU_H
#define GAME_T074_MEMORYCARD_OPERATION_MENU_H

#include "game/game.h"
#include "game/menu/memorycard/T073_memorycard_menu.h"      /* bases gt2k_menu::MemorycardLayerMenu,
                                               MemorycardLayerMenuItem; MemorycardMenuResult */

namespace gt2k_menu {
class MemorycardControl;                    /* T068 (references only) */
class MemorycardFile;                       /* T108 (pointers only) */
class Object;                               /* T220 (pointers only) */
class PageLayout;                           /* T229 */

/* vtables 0x0031EB28 (primary, 7 slots: [1] / [2] pCell's, [4] / [5] / [6]
 * MemorycardLayerMenu's 0x00150CA8 / 0x001511C8 / 0x00150D58) and 0x0031EB10
 * (the List<MemorycardMenuItem> part, 2 slots, this-delta -0xC: [1] pList's
 * sort 0x0023C580).  typeinfo __tfQ29gt2k_menu23MemorycardOperationMenu
 * 0x00154618 (class, base list g_00351868 {MemorycardLayerMenu, public +0};
 * storage 0x004B1B00).  Size 0x44 (operator new in T071).  select is the
 * key method (the first virtual one defined out of line): T074 writes the
 * vtables and the inline members (getIconDY / getIconDX, the destructor).
 * type (TYPE, chosen by T071's MemorycardSlotSet::openMenu):
 *   0  a file of the screen's kind: Overwrite (save mode) or Load (load mode),
 *      else Copy, Delete, Comment
 *   1  another GT3 file / the file utility: Copy, Delete, Comment
 *   2  a game file or another game's file: Copy, Delete
 *   3  the new-file entry: Save (save mode) */
class MemorycardOperationMenu : public MemorycardLayerMenu {
public:
    /* The constructor's type (? names; T071 passes the numbers). */
    enum TYPE {
        TYPE_MATCHING = 0,                  /* a GT3 save that passes the screen's filter (not in the
                                               file utility, mode 0x18) */
        TYPE_GT3      = 1,                  /* another GT3 save, or any GT3 save in the file utility */
        TYPE_OTHER    = 2,                  /* game data (MemorycardGameFile) or not a GT3 save */
        TYPE_NEW_FILE = 3                   /* the new-file entry (dir.id -1) */
    };

    /* The layer "operation<type + 1>" / "op<type + 1>_window" of the open
     * page.  afterNameEntry: rebuilt after the name entry (the cursor on the
     * CommentMenu that ran it; hidden while that is pending).  Sets the
     * centre of the confirm windows (g_00351848 / g_0035184C) from
     * "confirm_window" and stops the 3D icon of slot 0 (icon_slot_release,
     * T077). */
    MemorycardOperationMenu(MemorycardControl &control, int type, int afterNameEntry); /* 0x00151AE8 */
    /* [0] +0x0C 0x00154670 ~MemorycardOperationMenu: implicit (egcs writes it
     * after the typeinfo function; it stores both vptrs) */
    /* {8, 0} once the current slot's card cannot be used, else the menu's. */
    virtual MemorycardMenuResult select(MemorycardControl &control); /* [3] +0x24 0x00152508 */
    /* Inline accessors (defined in T074's source): out-of-line copies, no
     * callers; T071 reads the fields. */
    f32 getIconDY();                        /* 0x00154608 */
    f32 getIconDX();                        /* 0x00154610 */

    /*  MemorycardLayerMenu                    0x00 */
    f32 iconDX;                             /* 0x3C "opN_icon_face" centre - window centre */
    f32 iconDY;                             /* 0x40 the same, y up (448 lines) */
};
EGCS_SIZE(gt2k_menu::MemorycardOperationMenu, 0x44);

} /* namespace gt2k_menu */

/* The progress callback (T108's MemorycardProgressFunc) the working menus
 * pass to the card operations, arg the MemorycardControl: its progress bar
 * goes to progress (progressPrev = the old value).  (T075 has its own static
 * copy, 0x00156A80.) */
void memorycard_set_progress(f32 progress, void *arg); /* 0x00151AD0 */

/* The centre of the confirm windows (y up, 448 lines): set by the
 * MemorycardOperationMenu constructor from "confirm_window" (static
 * memorycard_set_confirm_center 0x00152570); the confirm windows and the
 * dialogs (as ints) open there.  Defined in T074 (320, 180 until then). */
extern f32 g_00351848;                      /* 0x00351848 gp-0x61A8 (=320.0) x */
extern f32 g_0035184C;                      /* 0x0035184C gp-0x61A4 (=180.0) y */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu8LoadMenu(void);                  /* 0x001528B0 */
void *__tfQ29gt2k_menu13OverwriteMenu(void);            /* 0x00152CD0 */
void *__tfQ29gt2k_menu8SaveMenu(void);                  /* 0x00153188 */
void *__tfQ29gt2k_menu17CommentChangeMenu(void);        /* 0x00153568 */
void *__tfQ29gt2k_menu11CommentMenu(void);              /* 0x001537A8 */
void *__tfQ29gt2k_menu10DeleteMenu(void);               /* 0x00153B90 */
void *__tfQ29gt2k_menu8CopyMenu(void);                  /* 0x00154570 */
void *__tfQ29gt2k_menu23MemorycardOperationMenu(void);  /* 0x00154618 */
void *__tfQ29gt2k_menu15LoadWorkingMenu(void);          /* 0x00154ED8 */
void *__tfQ29gt2k_menu15SaveWorkingMenu(void);          /* 0x00155620 */
void *__tfQ29gt2k_menu24CommentChangeWorkingMenu(void); /* 0x00155C58 */
void *__tfQ29gt2k_menu17DeleteWorkingMenu(void);        /* 0x00156248 */
void *__tfQ29gt2k_menu15CopyWorkingMenu(void);          /* 0x00156930 */
void *__tfQ29gt2k_menu11ConfirmMenu(void);              /* 0x001569C8 */
}
#endif

#endif /* GAME_T074_MEMORYCARD_OPERATION_MENU_H */
