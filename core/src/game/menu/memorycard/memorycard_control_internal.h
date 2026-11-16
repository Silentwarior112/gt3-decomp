/* memorycard_control_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u026,
 * 0x001499B8..0x0014B610, second pass.
 *
 * Translation unit T068: gt2k_menu::MemorycardControl, the controller of the
 * memory card screens ("menu/memorycard.mbl"), and F_0014AE00, their entry
 * point (main, the arcade / GT-mode menus, the race menus, the logger, the
 * setting screen).  The class, its modes and screen states are in the
 * canonical header game/menu/memorycard/T068_memorycard_control_class.h.
 *
 * Every class is defined in a canonical header; T068's source includes the
 * headers it needs itself, in the order that gives retail's type names (u026's
 * data pass).  This header adds what only this unit uses: the heap size and
 * the typeinfo storage of the unit's own class (the .sdata words it reads are
 * declared in T069 / T070).
 */
#ifndef U026_H
#define U026_H

#include "types.h"

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* F_0014AE00's menu heap: memalign(0x40, MEMORYCARD_HEAP_SIZE), pushed with
 * menu_heap_push for the controller's lifetime. */
enum { MEMORYCARD_HEAP_SIZE = 0x60000 };    /* 384 KB */

/* The name entry (MemorycardControl::comment, run_text_input's maxLen). */
enum { MEMORYCARD_COMMENT_LEN = 0x10 };

/* ======================================================================== */
/* Data no canonical header declares                                        */
/* ======================================================================== */

/* (g_00351F70 "MaxSpeed": T150_serialize_replay_data.h.) */

/* ======================================================================== */
/* RTTI                                                                     */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
extern "C" {
/* The __si_type_info of MemorycardControl, filled by its __tf function. */
extern u32 __tiQ29gt2k_menu17MemorycardControl[3]; /* 0x004B1990 (.bss) */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U026_H */
