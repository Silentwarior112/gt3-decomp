/* T075_memorycard_slot_menu_root.h - gt2k_menu::MemorycardSlotMenuRoot and
 * MemorycardAutoLoad, the two plain structs of T075 (unit u031) that
 * MemorycardControl (T068) holds / builds: its member slotMenu and
 * autoLoad's local.
 *
 * In their own header since u026's data pass (2026-10-17): retail's T068
 * .rodata has the type names of the menu items of T073 (List / Cell<
 * MemorycardMenuItem>, MemorycardMenuItem) but none of T076's dialogs, so
 * the original's T068 saw these two structs without the rest of T075's
 * header (which brings T073, T076 and T229).  Both are plain structs (no
 * virtual functions, no inline members), so the move changes no object:
 * T075_memorycard_game_slots.h includes this header where they were. */
#ifndef GAME_T075_MEMORYCARD_SLOT_MENU_ROOT_H
#define GAME_T075_MEMORYCARD_SLOT_MENU_ROOT_H

#include "game/game.h"

namespace gt2k_menu {
class MemorycardControl;                    /* T068 (pointers / references only) */
class MemorycardLayerMenu;                  /* T073 */
class MemorycardSlotMenu;                   /* T075 */
class Item;                                 /* T213 */

/* The slot menu of the game load / save screens (? name): MemorycardControl
 * +0x30694 (T068).  Not polymorphic; size 0x18 (the next member of the
 * control, mode, is at +0x306AC).  The destructor deletes the window. */
struct MemorycardSlotMenuRoot {
    MemorycardSlotMenuRoot();               /* 0x00156B08 the six words cleared */
    ~MemorycardSlotMenuRoot();              /* 0x00156B28 delete root */
    /* Builds the "root" window (unless one is open): the two slots, the
     * texts "root_text_insert_card" / "root_text_select_card" (hidden until
     * update), the cursor on slot 2 when that port was used last. */
    void open(MemorycardControl *c, int save);  /* 0x00156B88 */
    /* One frame: the window's select / update and its result (a dialog
     * replaces the window, the window is rebuilt, or the screen is left:
     * state 3); the texts; in the save screen button 0x10 leaves for the
     * file utility (state 1). */
    void update(MemorycardControl *c);      /* 0x00156E98 */

    MemorycardLayerMenu *root;              /* 0x00 the "root" window, or the dialog replacing it */
    MemorycardSlotMenu  *slot1;             /* 0x04 */
    MemorycardSlotMenu  *slot2;             /* 0x08 */
    int                  save;              /* 0x0C the save screen (open's save) */
    Item                *insertText;        /* 0x10 "root_text_insert_card" */
    Item                *selectText;        /* 0x14 "root_text_select_card" */
};
EGCS_SIZE(gt2k_menu::MemorycardSlotMenuRoot, 0x18);

/* The autoload at boot (? name; MemorycardControl::autoLoad's local): the
 * load of WorkingMenu from slot 1 without the menus, with a dialog on an
 * error.  Not polymorphic; one word.  The destructor deletes the dialog. */
struct MemorycardAutoLoad {
    MemorycardAutoLoad();                   /* 0x001570C8 menu = NULL */
    ~MemorycardAutoLoad();                  /* 0x001570D8 delete menu */
    /* 1: a formatted PS2 memory card in port has the game data, 0: not,
     * -1: asking failed.  (c is not used.) */
    int  checkPort(MemorycardControl *c, int port); /* 0x00157138 */
    /* The load into the control's arg under the "working" window: done,
     * "working_text_complete", result 1, the port remembered, 90 frames
     * (menu stays NULL); failed, the read error dialog becomes menu.
     * Returns 1. */
    int  load(MemorycardControl *c, int port);  /* 0x00157198 */
    int  check(MemorycardControl *c);       /* 0x001577B0 checkPort(c, 0) */
    /* r < 0: the read error dialog at (0x140, 0xE0), else load(c, 0); no
     * dialog left: the page is left (exitRequest). */
    void start(MemorycardControl *c, int r);    /* 0x001577D0 */
    /* One frame of the dialog; once it is left the screen is left (state 3). */
    void update(MemorycardControl *c);      /* 0x00157880 */

    MemorycardLayerMenu *menu;              /* 0x00 the dialog shown, NULL once done */
};
EGCS_SIZE(gt2k_menu::MemorycardAutoLoad, 0x4);

} /* namespace gt2k_menu */

#endif /* GAME_T075_MEMORYCARD_SLOT_MENU_ROOT_H */
