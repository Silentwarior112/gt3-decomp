/* memorycard_menu_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u028,
 * 0x0014E880..0x00151AD0, second pass.
 *
 * Five translation units of the memory card screens (menu/memorycard.mbl):
 *   T070  the page's ExecuteFuncData table {"input_text", text_box_key}
 *         (its static initialiser)
 *   T110  MemorycardDirCache: the card directory listing of a file manager
 *         (T109), read page by page, the least recently used page reused
 *   T071  MemorycardSlot / MemorycardSlotSet: the two card slots of the file
 *         screens (the icon grids of T069), the cursor, the info panel of the
 *         selected file and the operation menu (T074) opened on it; one frame
 *         of the screen is MemorycardSlotSet::update (from MemorycardControl::
 *         fileOperation, T068)
 *   T072  MemorycardIconSprites: the 14 built-in icon images and how to draw
 *         one
 *   T073  the menu classes the operation menus / dialogs of T074..T076 are
 *         built from: MemorycardMenuItem, MemorycardMenuItemItem,
 *         MemorycardLayerMenuItem, MemorycardMenu, MemorycardLayerMenu
 *
 * Every class is defined in a canonical header; since the data pass
 * (2026-10-17) each source includes the canonical headers it needs itself,
 * in the order that gives the original's type names (its comment says
 * which), and this header adds only what the unit's files share: the
 * constants of the menu protocol and of the screens, the declarations of the
 * unit's data no other unit reads (each defined in its file) and the
 * type_info storage / base lists only T073's typeinfo functions use (GCC 14
 * only).  It includes no class header (types.h, libc.h, T217's
 * OperatorCode: no type names). */
#ifndef U028_H
#define U028_H

#include "types.h"
#include "libc.h"                           /* strcpy, strcat, free */
#include <stddef.h>
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode (g_0031DD80) */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_class, __class_type_info::base_info */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* menu_play_se's sounds of the memory card menus: T233's MENU_SE_* (the decide
 * sound is MENU_SE_CONFIRM, 7). */

/* Drawable::anchorType: placed relative to the anchor in x (bit 0) and y
 * (bit 1). */
enum { MC_ANCHOR_XY = 3 };

/* MemorycardDir::id of the entries that are not files on the card (T108). */
enum {
    MC_DIR_ID_NEW         = -1,             /* the file to save (MemorycardSlot::newFile) */
    MC_DIR_ID_NOT_READING = -2              /* a MemorycardDummyFile: not read yet */
};

/* MemorycardControl::getCurrentPort of the demo replays of the disc
 * (MemorycardDemoFileManager, T079: the files load from the volume). */
enum { MC_PORT_DEMO_VOLUME = 0x100 };

/* The built-in icon images (g_002A6578, MemorycardIconSprites::sprites). */
enum {
    MC_SPRITE_NEW   = 0,                    /* "icon_new", drawn over the file to save */
    MC_SPRITE_COUNT = 14
};

/* The menu's virtual screen is 448 lines; layout items measure y up from
 * its bottom. */
#define MC_SCREEN_HEIGHT 448.0f

/* ======================================================================== */
/* Data only this unit reads (each defined in its file)                     */
/* ======================================================================== */

/* T071 (.sdata): the elements of the two slots' icon grids,
 * {"icon_view_slot1", "icon_view_slot2"}. */
extern const char *g_00351820[2];           /* 0x00351820 gp-0x61D0 */
/* (T071's comment buffer g_0049C0B8, backupComment / restoreComment, is a
 * file static of T071: retail's .bss.) */
/* T072 (.data): the names of the icon images, "icon_new" .. "icon_cursor"
 * (see T072_memorycard_icon_sprites.h). */
extern const char *g_002A6578[MC_SPRITE_COUNT];     /* 0x002A6578 */

namespace gt2k_menu {
/* T073 (.rodata): the fade scripts of MemorycardLayerMenu::setShown, two
 * operator codes each (T217), the second opRemove (the operator ends): [0]
 * fade in (opFadeAlpha +4.0 / s for 0.25 s), [1] alpha 1 at once
 * (opSetAlpha), [2] fade out (opFadeAlpha -4.0 / s for 0.25 s), [3] alpha 0
 * at once. */
extern const OperatorCode g_0031DD80[4][2]; /* 0x0031DD80 */
} /* namespace gt2k_menu */

/* ======================================================================== */
/* RTTI (modern compiler only; egcs generates these itself)                 */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
extern "C" {
/* The type_info storage of T073's classes that no other unit uses (the
 * other four are in T073_memorycard_menu.h). */
extern u32 __tiQ29gt2k_menu18MemorycardMenuItem[3];             /* 0x004B19A0 __si_type_info */
extern u32 __tiQ29gt2k_menut4List1ZQ29gt2k_menu18MemorycardMenuItem[3];   /* 0x004B19B0 */
extern u32 __tiQ29gt2k_menu14MemorycardMenu[4];                 /* 0x004B1A90 __class_type_info */
}
/* The base lists of the two __class_type_info nodes. */
extern const __class_type_info::base_info g_00351840[1];   /* 0x00351840 {MemorycardMenu, public +0} */
extern const __class_type_info::base_info g_0031E158[2];   /* 0x0031E158 {MemorycardMenuItem, public +0},
                                               {List<MemorycardMenuItem>, public +0xC} */
#endif /* !BOOT_OLD_GCC */

#endif /* U028_H */
