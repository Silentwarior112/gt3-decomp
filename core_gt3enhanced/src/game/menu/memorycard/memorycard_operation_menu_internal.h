/* memorycard_operation_menu_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, translation
 * unit T074, 0x00151AD0..0x00156A80: work units u029 (0x00151AD0..0x00153B70)
 * and u030 (0x00153B70..0x00156A80), one object in the original, merged
 * into src/u029/T074_memorycard_operation_menu.cpp.
 *
 * The operation menu of the memory card file screens.  When a file of the
 * list is decided, T071 (MemorycardSlotSet::openMenu) builds a
 * MemorycardOperationMenu next to its icon ("operation1".."operation4" of
 * menu/memorycard.mbl).  Its items open confirm windows:
 *   LoadMenu           "confirm"   OK (LoadWorkingMenu) / Cancel  "confirm_text_load"
 *   OverwriteMenu      "confirm2"  OK (SaveWorkingMenu) / Cancel  "confirm2_text_overwrite",
 *                                  or the "not enough space" dialog
 *   SaveMenu           "confirm2"  OK (SaveWorkingMenu) / Cancel  "confirm2_text_save" or
 *                                  the format question, or the "not enough space" dialog
 *   CommentChangeMenu  "confirm"   OK (CommentChangeWorkingMenu) / Cancel
 *   DeleteMenu         "confirm"   OK (DeleteWorkingMenu) / Cancel  "confirm_text_delete"
 *   CopyMenu           "confirm"   OK (CopyWorkingMenu) / Cancel; the text says
 *                                  "copy", "same file" + "overwrite", or "format
 *                                  the card in slot N" + "approve"; or the read
 *                                  error / "nospace" dialog replaces the window
 * The OK items do the card operation in their own windows:
 *   LoadWorkingMenu  "working"   loads the selected save and deserialises it
 *                                (Serialize, T509)
 *   SaveWorkingMenu  "working2"  writes the new save (an overwrite deletes the
 *                                old one first)
 *   CommentChangeWorkingMenu, DeleteWorkingMenu, CopyWorkingMenu  "working"
 * OverwriteMenu, SaveMenu and CommentChangeMenu are CommentMenus: deciding
 * one first runs the name entry for the save's comment (result {9, 1}); T071
 * then rebuilds the operation menu with afterNameEntry set, the item waits
 * four frames, stores the comment in the file and opens its confirm window.
 *
 * MemorycardOperationMenu is in the canonical header
 * game/menu/memorycard/T074_memorycard_operation_menu.h (T071 builds it); the items, the
 * working menus and ConfirmMenu are T074's own classes, defined in the
 * source (see there and the header).  This header adds what only T074 uses:
 * the result codes and constants it names and the typeinfo storage of
 * T074's classes (GCC 14's typeinfo functions).
 *
 * Include order (the merge, 2026-10-16): egcs writes the type names of the
 * classes an object completes into its .rodata in reverse order of
 * definition; T074's .rodata has, after its own (0x0031EB68..), the dialogs'
 * (T076), T073's (List<MemorycardMenuItem>, MemorycardMenuItem,
 * Cell<MemorycardMenuItem>), MemorycardControl's, the garage car list's
 * (GarageCarList, List<GarageCar>, GarageCar, Cell<GarageCar>; no
 * GarageWheelList), MemorycardFileManagerBase's (with List /
 * Cell<MemorycardFile>), SmartIndexT<int>'s and the operator holders'
 * (T216), 0x0031ED20..0x0031F098; the working menus' vtables follow them.
 * The headers below complete those classes in that order (checked byte for
 * byte against the binary; a header that adds a class without a key method
 * to this set moves the vtables after it and every maker's data
 * references).
 */
#ifndef U029_H
#define U029_H

#include "types.h"
#include "libc.h"                           /* strcmp, strcpy, free */
#include <stddef.h>

/* outside the unit: first the headers that complete the classes whose type
 * names T074's .rodata has, in the binary's order (see the top) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (MemorycardLayerMenu::root, the bar);
                                               OperatorHolder, OperatorList */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (getFragmentManager, getLayer);
                                               SmartIndexT<int> (T240's PageHistory) */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* gt2k_menu::MemorycardFileManagerBase::updateFileId /
                                               updateFileIdForDelete; List / Cell<MemorycardFile> */
#include "game/save/T429_garage_car_list.h"      /* GarageCarList, List / Cell<GarageCar>, GarageCar
                                               (type names only; not GameStatus's wheel list) */
#include "game/menu/memorycard/T075_memorycard_game_slots.h"    /* (T068's MemorycardControl before T073's and T076's
                                               classes: T075's header includes T068 first) */
/* the rest */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/memorycard/T068_memorycard_control.h"   /* gt2k_menu::MemorycardControl, MemorycardLoadInfo */
#include "game/menu/memorycard/T071_memorycard_slots.h"     /* gt2k_menu::MemorycardSlot::isReady, MemorycardSlotSet::
                                               releaseInfo / resetInfo */
#include "game/menu/memorycard/T073_memorycard_menu.h"      /* gt2k_menu::MemorycardLayerMenu, MemorycardLayerMenuItem,
                                               MemorycardMenuResult */
#include "game/menu/memorycard/T076_memorycard_dialogs.h"   /* gt2k_menu::MemorycardCancelItem, MemorycardReadErrorDialog,
                                               MemorycardWriteErrorDialog, MemorycardNoSpaceDialog */
#include "game/menu/memorycard/T077_memorycard_icon.h"      /* icon_slot_release */
#include "game/menu/memorycard/T108_memorycard_file.h"      /* gt2k_menu::MemorycardFile, MemorycardGT3File */
#include "game/race/core/T150_serialize_replay_data.h"    /* g_00351F40 "ReplayData", g_00351F54 "GhostData", g_00351F60 "TrackData" */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object::kill */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuLayer */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (control1.decide) */
#include "engine/io/T501_memorycard_manager.h"   /* mc_slot_refresh, mc_slot_status, mc_slot_free,
                                               mc_slot_formatted, mc_slot_read_size */
#include "engine/io/T509_serialize.h"            /* Serialize::Base, Header; serialize_create / _destroy */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_class, __class_type_info::base_info */
#endif

/* own translation unit (last: T074's classes are completed after all the
 * others, MemorycardOperationMenu first) */
#include "game/menu/memorycard/T074_memorycard_operation_menu.h"    /* gt2k_menu::MemorycardOperationMenu;
                                               memorycard_set_progress, g_00351848 / g_0035184C */

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* The decide sound of the memory card menus (T073 checkDecide plays it too),
 * also played when an operation is complete: T233's MENU_SE_CONFIRM. */

/* What a save / copy keeps free on the card besides its own size:
 * mc_slot_free's clusters (1 KB) - 2 must cover it (? the reason for the
 * margin). */
enum { MC_FREE_MARGIN_KB = 2 };

/* CommentMenu::select takes the edited comment on its fourth call after the
 * operation menu came back from the name entry (? why four). */
enum { COMMENT_MENU_WAIT = 4 };

/* MemorycardControl::commentMode: which CommentMenu ran the name entry (its
 * preCommentEdit), so that the rebuilt operation menu puts its cursor on
 * that item again.  Only this unit reads or writes it. */
enum {
    COMMENT_MODE_SAVE      = 0,             /* SaveMenu (the new file) */
    COMMENT_MODE_OVERWRITE = 1,             /* OverwriteMenu */
    COMMENT_MODE_CHANGE    = 2              /* CommentChangeMenu */
};

/* MemorycardControl::getCurrentPort() of the disc's demo replays (u028's
 * name): their files come from the volume, not from a card. */
enum { MC_PORT_DEMO_VOLUME = 0x100 };

/* MemorycardFile::readIcon's which: the copy icon of icon.sys. */
enum { MC_ICON_COPY = 1 };

/* T501's mc_slot_status codes: MC_STATUS_* (T501_memorycard_manager.h). */

/* Serialize::Base::checkHeader's result from which the data is refused
 * (the version differs; 1, only the sub-version, is accepted). */
enum { SERIALIZE_VERSION_DIFFERS = 2 };

/* The frames the working windows wait with their "..._text_complete" shown
 * before going on: the load (no OK item, it leaves at once) and the rest. */
enum { MC_LOAD_COMPLETE_WAIT = 0x3C, MC_COMPLETE_WAIT = 4 };

/* ======================================================================== */
/* Data (.data)                                                             */
/* ======================================================================== */

/* The layers of the operation menus by MemorycardOperationMenu::TYPE and
 * their anchor elements (defined in the source: their strings sit in T074's
 * .rodata between the makers' and the constructor's). */
extern const char *g_002A6D88[4];           /* 0x002A6D88 (.data, T074) "operation1".."operation4" */
extern const char *g_002A6D98[4];           /* 0x002A6D98 (.data, T074) "op1_window".."op4_window" */

/* (g_00351F60 "TrackData", by which LoadWorkingMenu loads the logger's
 * replays (mode 0x15): T150_serialize_replay_data.h.) */

/* ======================================================================== */
/* RTTI                                                                     */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
extern "C" {
/* The type_info nodes the __tf functions of T074 fill: __si_type_info
 * (3 words) / __class_type_info (4 words); MemorycardLayerMenuItem's and
 * MemorycardLayerMenu's: T073's header. */
extern u32 __tiQ29gt2k_menu17CommentChangeMenu[3];         /* 0x004B1AA0 (.bss) */
extern u32 __tiQ29gt2k_menu24CommentChangeWorkingMenu[3];  /* 0x004B1AB0 (.bss) */
extern u32 __tiQ29gt2k_menu15CopyWorkingMenu[3];           /* 0x004B1AC0 (.bss) */
extern u32 __tiQ29gt2k_menu13OverwriteMenu[3];             /* 0x004B1AD0 (.bss) */
extern u32 __tiQ29gt2k_menu8SaveMenu[3];                   /* 0x004B1AE0 (.bss) */
extern u32 __tiQ29gt2k_menu15SaveWorkingMenu[3];           /* 0x004B1AF0 (.bss) */
extern u32 __tiQ29gt2k_menu23MemorycardOperationMenu[4];   /* 0x004B1B00 (.bss) */
extern u32 __tiQ29gt2k_menu10DeleteMenu[3];                /* 0x004B1B10 (.bss) */
extern u32 __tiQ29gt2k_menu8LoadMenu[3];                   /* 0x004B1B20 (.bss) */
extern u32 __tiQ29gt2k_menu11ConfirmMenu[4];               /* 0x004B1B30 (.bss) */
extern u32 __tiQ29gt2k_menu17DeleteWorkingMenu[3];         /* 0x004B1B40 (.bss) */
extern u32 __tiQ29gt2k_menu15LoadWorkingMenu[3];           /* 0x004B1B50 (.bss) */
extern u32 __tiQ29gt2k_menu8CopyMenu[3];                   /* 0x004B1B60 (.bss) */
extern u32 __tiQ29gt2k_menu11CommentMenu[3];               /* 0x004B1B70 (.bss) */
}
/* The base list of ConfirmMenu's and MemorycardOperationMenu's
 * __class_type_info (egcs writes it itself: one .sdata copy, after the
 * strings). */
extern const __class_type_info::base_info g_00351868[1];   /* 0x00351868 (.sdata) {MemorycardLayerMenu,
                                               public +0} */
#endif /* !BOOT_OLD_GCC */

#endif /* U029_H */
