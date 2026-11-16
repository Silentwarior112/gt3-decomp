/* T075_memorycard_game_slots.h - the game load / save screens of the memory
 * card and the autoload at boot (translation unit T075, unit u031),
 * 0x00156A80..0x00158CD0.
 *
 * MemorycardControl::gameMenu (T068; modes 0x19 save, 0x1A..0x1D loads)
 * runs a MemorycardSlotMenuRoot: the "root" window of menu/memorycard.mbl
 * with one MemorycardSlotMenu per slot.  Deciding a slot with a PS2 memory
 * card opens the "confirm" window; its OK (WorkingMenu) shows the "working"
 * window, loads the game (Serialize "GameStatus" into the control's arg) or
 * saves the new file, then "working_text_complete".  A load leaves the
 * screen with result 1; after a save the OK item rebuilds the root window.
 * The port used is remembered in the GameStatus menu state (+0x5F0 / +0x5F4
 * / +0x5F8 by mode) and the root window opens on that slot.
 * MemorycardControl::autoLoad runs a MemorycardAutoLoad: the same load from
 * slot 1 at boot, without the menus.
 *
 * Menu results (MemorycardMenuResult {code, arg}, T073) used here: 5 leave
 * the menu with result 1 and rebuild the root, 6 leave the screen with
 * result 1, 8 the card went away, 0xA / 0xC a window's Cancel / OK (the root
 * is rebuilt), 0xB: arg is the menu that replaces the root (a dialog).
 *
 * MemorycardSlotMenu's and WorkingMenu's virtual methods are inline in the
 * original: they are egcs's deferred output (each class's group: its
 * methods, its typeinfo function, its implicit destructor; no key method,
 * so no copy of the inline constructors either).  T075's source defines
 * them `inline' (egcs -O2 then gives the binary's functions in the binary's
 * order), so only T075 may build these two classes.
 *
 * Order of this header (u031 matching): MemorycardSlotMenuRoot and
 * MemorycardAutoLoad first (game/menu/memorycard/T075_memorycard_slot_menu_root.h, which
 * T068's class header includes too; game/menu/memorycard/T068_memorycard_control.h is this
 * header plus that class header), then
 * MemorycardControl (T068), the menu classes (T073) and the dialogs (T076),
 * then WorkingMenu and MemorycardSlotMenu.  egcs writes the type names of
 * an object's classes into its .rodata in reverse order of definition; T075
 * and T076 (each includes this header after the ones that complete the
 * garage, memory card file and operator classes) have their own names
 * first, then the dialogs', then T073's, then MemorycardControl's, as in
 * the binary.  Includers of T068 see the same
 * classes as before, the dialogs now too.
 *
 * File-local (static) helpers of T075, not declared here: the progress
 * callback 0x00156A80 (T074 has its own, memorycard_set_progress) and
 * memorycard_game_file_exists 0x00156A98 (a MemorycardGameFile on the stack
 * asks the card; callers 0x0015716C / 0x00157A50, both in T075: the plan's
 * "used by u024" is a comment of u024 naming it as a caller of a T108
 * function). */
#ifndef GAME_T075_MEMORYCARD_GAME_SLOTS_H
#define GAME_T075_MEMORYCARD_GAME_SLOTS_H

#include "game/game.h"

namespace gt2k_menu {
class MemorycardControl;                    /* T068 (pointers / references only) */
class MemorycardLayerMenu;                  /* T073 */
class MemorycardSlotMenu;                   /* below */
class Object;                               /* T220 */
class Item;                                 /* T213 */
} /* namespace gt2k_menu */

/* MemorycardSlotMenuRoot and MemorycardAutoLoad (plain structs): in their
 * own header since u026's data pass (retail's T068 saw them without T073's
 * and T076's classes); T068's class header includes only that one. */
#include "game/menu/memorycard/T075_memorycard_slot_menu_root.h"

/* The rest after these (see the top).  Included from T068 (which includes
 * this header first), the T068 below is skipped and T068 goes on after it. */
#include "game/menu/memorycard/T068_memorycard_control.h"   /* gt2k_menu::MemorycardControl */
#include "game/menu/memorycard/T073_memorycard_menu.h"      /* base gt2k_menu::MemorycardLayerMenuItem;
                                               MemorycardMenuResult */
#include "game/menu/memorycard/T076_memorycard_dialogs.h"   /* the dialogs T075 builds (type name order) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuLayoutItem (MenuElement) */

namespace gt2k_menu {
class PageLayout;                           /* T229 */

/* The 0x6C-byte layout item of T229 (name, flags, x / y / z / w / h, type,
 * active, uv, colours, text): u027 / u028 / u030 / u031 / u063 / u070 /
 * u019 called it MenuElement.  (u040's MenuElement, count +0x104 / items
 * +0x108, is T241's LinkPage, not this.) */
/* (No gt2k_menu::MenuElement typedef: T224 declares ::MenuElement, and egcs
 * calls the two ambiguous under `using namespace gt2k_menu`.) */

/* OK of a slot's confirm window: loads the game from / saves the new file
 * to the card in port.  vtable 0x0031F960 (10 slots: [4]
 * MemorycardMenuItemItem::isSelectable, [5] / [6] / [8] / [9]
 * MemorycardLayerMenuItem's).  typeinfo __tfQ29gt2k_menu11WorkingMenu
 * 0x00158C38 (si, base MemorycardLayerMenuItem; storage 0x004B1B80).  Size
 * 0x30 (operator new in MemorycardSlotMenu::makeSubMenu).  Defined before
 * MemorycardSlotMenu, which builds it (the binary's type name order). */
class WorkingMenu : public MemorycardLayerMenuItem {
public:
    /* Inline (MemorycardSlotMenu::makeSubMenu expands it). */
    WorkingMenu(PageLayout *layout, Object *parent, const char *name, int port, bool formatted)
        : MemorycardLayerMenuItem(layout, parent, name), port(port), done(false),
          formatted(formatted) {}
    /* [0] +0x0C 0x00158C88 ~WorkingMenu: implicit (no vptr store) */
    /* The two methods below: inline, defined in T075's source. */
    /* {6, 0} once the game is loaded, else MemorycardLayerMenuItem::select. */
    virtual MemorycardMenuResult select(MemorycardControl &control); /* [3] +0x24 0x001583A0 */
    /* The "working" window; a load ({0xB, the read error dialog} on an
     * error) or the save of control's new file with its thumbnail (comment
     * emptied), formatting the card first when !formatted ({0xB, the write
     * error dialog} on an error); then sound 7, "working_text_complete" and,
     * after a save, an OK item {5}.  The port is remembered.  {0, 0xA}. */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x001583D8 */

    /*  MemorycardLayerMenuItem                0x00 */
    int port;                               /* 0x24 */
    /* bool: makeSubMenu's `!done' is egcs's xori 1. */
    bool done;                              /* 0x28 the game was loaded: select() leaves */
    /* A bool: save gets !formatted, egcs's xori 1. */
    bool formatted;                         /* 0x2C false: the save formats the card first */
};
EGCS_SIZE(gt2k_menu::WorkingMenu, 0x30);

/* A slot of the "root" window ("root_m_slot1" port 0, "root_m_slot2" port
 * 4).  vtable 0x0031F4D0 (10 slots: [1] / [2] pCell's, [5] / [6] / [8]
 * MemorycardLayerMenuItem's).  typeinfo __tfQ29gt2k_menu18MemorycardSlotMenu
 * 0x00158308 (si, base MemorycardLayerMenuItem; storage 0x004B1B90).  Size
 * 0x34 (operator new in MemorycardSlotMenuRoot::open). */
class MemorycardSlotMenu : public MemorycardLayerMenuItem {
public:
    /* Inline (open expands it; no out-of-line copy). */
    MemorycardSlotMenu(PageLayout *layout, Object *parent, const char *name, int port)
        : MemorycardLayerMenuItem(layout, parent, name), selectable(true), port(port),
          connected(false), formatted(true) {}
    /* [0] +0x0C 0x00158358 ~MemorycardSlotMenu: implicit (no vptr store) */
    /* The four methods below: inline, defined in T075's source. */
    /* The card was taken out with the window open: the window is deleted,
     * {1, 0xB}.  Else MemorycardLayerMenuItem::select. */
    virtual MemorycardMenuResult select(MemorycardControl &control); /* [3] +0x24 0x00158238 */
    virtual bool isSelectable(MemorycardControl &control); /* [4] +0x2C 0x00158300 selectable */
    /* Nothing ({0, 0}) without a PS2 memory card; else the "confirm" window
     * centred on (320, 224): a load offers OK (WorkingMenu) when the card has
     * the game data ("confirm_text_nofile" when not, the read error dialog
     * when asking failed); the save checks the space (the "nospace" dialog)
     * and says whether the game data is overwritten or the card formatted.
     * {0, 0xA}. */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00157938 */
    /* connected / formatted from the card server (T501); selectable =
     * connected, dim = !connected. */
    virtual void updateItem(MemorycardControl &control, bool sel); /* [9] +0x54 0x001582B0 */

    /* makeSubMenu's inline helpers (? names; defined in T075's source, no
     * out-of-line copies): the dialog that replaces the confirm window (its
     * cleanups come right after each expansion, as in the binary) and the
     * load / save cases. */
    void showReadError(MemorycardControl &control, int damaged);
    void showNoSpace(MemorycardControl &control, int needKB);
    void makeLoadConfirm(MemorycardControl &control);
    void makeSaveConfirm(MemorycardControl &control);

    /*  MemorycardLayerMenuItem                0x00 */
    /* bool: isSelectable returns it as is (lw; jr). */
    bool selectable;                        /* 0x24 a PS2 memory card is in the slot */
    int port;                               /* 0x28 0 slot 1, 4 slot 2 */
    /* Both bools: updateItem reloads connected and negates it with xori 1
     * (egcs's bool negation) for dim, and formatted goes as is into
     * WorkingMenu's bool formatted (mc_slot_formatted, T501, returns a
     * bool). */
    bool connected;                         /* 0x2C the card is a PS2 memory card (type 2) */
    bool formatted;                         /* 0x30 */
};
EGCS_SIZE(gt2k_menu::MemorycardSlotMenu, 0x34);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu18MemorycardSlotMenu(void);   /* 0x00158308 */
void *__tfQ29gt2k_menu11WorkingMenu(void);          /* 0x00158C38 */
}
#endif

#endif /* GAME_T075_MEMORYCARD_GAME_SLOTS_H */
