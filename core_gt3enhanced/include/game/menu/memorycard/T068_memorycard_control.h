/* T068_memorycard_control.h - gt2k_menu::MemorycardControl, the controller
 * of the memory card screens ("menu/memorycard.mbl"), with the screens of
 * T075 (and through them T073's menu classes and T076's dialogs) in front of
 * it (translation unit T068, unit u026), 0x001499B8..0x0014B610.
 *
 * Since u026's data pass (2026-10-17) the class is in
 * T068_memorycard_control_class.h: retail's T068 saw it without T076's
 * dialogs (its .rodata has none of their type names), but this header gave
 * T075_memorycard_game_slots.h (which includes T073 and T076) before the
 * class to every includer.  This header keeps that view: the same classes,
 * defined in the same order as before the split (T075's header, then the
 * class), for the units that include it (T069..T074, T080).  A file whose
 * original saw only the class includes the class header instead (T068).
 *
 * F_0014AE00, the entry point, and its modes: T068_memorycard_entry.h (the
 * callers that do not see the class). */
#ifndef GAME_T068_MEMORYCARD_CONTROL_H
#define GAME_T068_MEMORYCARD_CONTROL_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pages), ExecuteFuncData */
#include "game/app/T079_demo_file_manager.h"    /* gt2k_menu::MemorycardDemoFileManager (demo) */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* gt2k_menu::MemorycardFileManager (files) */
#include "game/menu/memorycard/T071_memorycard_slots.h"     /* gt2k_menu::MemorycardSlotSet (slots) */
#include "game/menu/memorycard/T075_memorycard_game_slots.h"    /* gt2k_menu::MemorycardSlotMenuRoot (slotMenu); T073's
                                               menu classes, T076's dialogs */
#include "game/menu/memorycard/T068_memorycard_control_class.h" /* gt2k_menu::MemorycardControl, MemorycardLoadInfo,
                                               F_0014AE00 */

#endif /* GAME_T068_MEMORYCARD_CONTROL_H */
