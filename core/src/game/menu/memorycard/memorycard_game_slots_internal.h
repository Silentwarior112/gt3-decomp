/* memorycard_game_slots_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u031,
 * 0x00156A80..0x00159EE8, second pass.
 *
 * Two translation units of the memory card screens (menu/memorycard.mbl):
 *   T075  the game load / save screens (MemorycardControl::gameMenu, modes
 *         MODE_GAME_SAVE 0x19, MODE_GAME_LOAD 0x1A .. MODE_GAME_LOAD_GTMODE
 *         0x1D) and the autoload at boot (MemorycardControl::autoLoad):
 *           MemorycardSlotMenuRoot  the "root" window: two MemorycardSlotMenu
 *                                   items ("root_m_slot1" / "root_m_slot2")
 *           MemorycardSlotMenu      a slot; decided, it opens the "confirm"
 *                                   window (OK = WorkingMenu, Cancel)
 *           WorkingMenu             OK: the "working" window, loads
 *                                   "GameStatus" (the game) from the card or
 *                                   saves the new file, then "complete"
 *           MemorycardAutoLoad      the same load without the menus
 *   T076  the dialogs (MemorycardErrorDialog and its four kinds,
 *         MemorycardFormatDialog with its FormatWorkingMenu) and
 *         MemorycardCancelItem, used by T074 / T075 too.
 *
 * Every class is defined in a canonical header (the unit's own are
 * game/menu/memorycard/T075_memorycard_game_slots.h and game/menu/memorycard/T076_memorycard_dialogs.h);
 * each source includes the headers it needs itself, in the order that
 * gives the original's type names (data pass: the two originals saw
 * different classes, T075 the GameStatus with its wheel list, T076 only
 * the garage car list).  This header adds what only this unit uses: the
 * result codes and constants it names, the type_info storage of T075 /
 * T076's classes and the two __rtti_class base lists; the sources include
 * it after their headers. */
#ifndef U031_H
#define U031_H

#include "types.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_class, __class_type_info::base_info */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* menu_play_se's sounds: T233's MENU_SE_CANCEL (a Cancel / OK item decided) and
 * MENU_SE_CONFIRM (a window's work is done). */

/* The card ids of the two slots (T501: port * 4 + slot). */
enum { MC_PORT_SLOT1 = 0, MC_PORT_SLOT2 = 4 };

/* mc_slot_status of the last operation: MC_STATUS_* (T501). */

/* What a save keeps free on the card besides its own size: mc_slot_free's
 * clusters (1 KB) - 2 must cover it (u029's name; ? the reason for the
 * margin). */
enum { MC_FREE_MARGIN_KB = 2 };

/* MemorycardDir::id (T108).  A new file (not on the card) makes saveFiles
 * create the directory, its icons and icon.sys; any id >= 0 marks a file
 * that is on the card: load() refuses a negative id and saveFiles then only
 * rewrites the data and thumbnail files.  The game data's name is fixed
 * ("BASCUS-97102GAMEDATA"), so T075 needs no listing entry number and
 * stores an arbitrary one. */
enum {
    MC_DIR_ID_NEW     = -1,
    MC_DIR_ID_ON_CARD = 0x01000000
};

/* Where the windows and dialogs of the game screens go: the screen centre
 * (640 x 448). */
enum { MC_CENTRE_X = 320, MC_CENTRE_Y = 224 };

/* Frames shown before the window goes: a game load from the autoload, from
 * the load screen, and before the OK item of a save. */
enum { MC_AUTOLOAD_WAIT = 90, MC_LOAD_WAIT = 60, MC_SAVE_WAIT = 4 };

/* ======================================================================== */
/* RTTI (modern compiler only; egcs generates these itself)                 */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
extern "C" {
/* The type_info nodes of T075 / T076's classes, filled by their __tf
 * functions (__si_type_info 3 words, __class_type_info 4). */
extern u32 __tiQ29gt2k_menu23MemorycardNoSpaceDialog[4];        /* 0x004B1A10 */
extern u32 __tiQ29gt2k_menu21MemorycardErrorDialog[4];          /* 0x004B1A20 */
extern u32 __tiQ29gt2k_menu28MemorycardWriteProtectDialog[4];   /* 0x004B1A30 */
extern u32 __tiQ29gt2k_menu26MemorycardWriteErrorDialog[4];     /* 0x004B1A40 */
extern u32 __tiQ29gt2k_menu25MemorycardReadErrorDialog[4];      /* 0x004B1A50 */
extern u32 __tiQ29gt2k_menu11WorkingMenu[3];                    /* 0x004B1B80 */
extern u32 __tiQ29gt2k_menu18MemorycardSlotMenu[3];             /* 0x004B1B90 */
extern u32 __tiQ29gt2k_menu22MemorycardFormatDialog[4];         /* 0x004B1BA0 */
extern u32 __tiQ29gt2k_menu17FormatWorkingMenu[3];              /* 0x004B1BB0 */
extern u32 __tiQ29gt2k_menu20MemorycardCancelItem[3];           /* 0x004B1BC0 */
}
/* The base lists of the __class_type_info nodes (.sdata): MemorycardMenu
 * has two bases, so its subclasses' nodes are __class_type_info too. */
extern const __class_type_info::base_info g_003518C0[1];   /* 0x003518C0 {MemorycardLayerMenu, public +0} */
extern const __class_type_info::base_info g_003518C8[1];   /* 0x003518C8 {MemorycardErrorDialog, public +0} */
#endif /* !BOOT_OLD_GCC */

#endif /* U031_H */
