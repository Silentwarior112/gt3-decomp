/* T070_memorycard_actions.h - the action table of the memory card page
 * (translation unit T070, unit u028), 0x0014E880..0x0014E8A8.
 *
 * T070 is only its static initialiser and its data: the one-entry
 * ExecuteFuncData table g_00351810 (.sdata, zero in the file; the plan put
 * it in T068_memorycard_control.h), filled with {"input_text", text_box_key
 * 0x001C40F0 of T231}, which MemorycardControl::begin (T068) installs as the
 * page manager's execute functions.
 *
 * (The type_info storage of four T073 classes, which the plan also put
 * here, is declared with their __tf functions in T073_memorycard_menu.h.) */
#ifndef GAME_T070_MEMORYCARD_ACTIONS_H
#define GAME_T070_MEMORYCARD_ACTIONS_H

#include "game/game.h"

#ifdef __cplusplus                          /* a C++ translation unit (C++ linkage) */

#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ExecuteFuncData (g_00351810) */

/* The page's ExecuteFuncData table (PageManager executeFuncs, hooks[2] of
 * the first pass), one entry; its count g_00351818 (=1) follows it.  Size
 * not declared: T068's begin loads its address with lui / addiu (-G8), not
 * $gp-relative as a known 8-byte object would be. */
extern gt2k_menu::ExecuteFuncData g_00351810[];    /* 0x00351810 gp-0x61E0 */
extern int g_00351818;                      /* 0x00351818 gp-0x61D8 (=1) the entries of g_00351810
                                               (T068 begin stores both into the page manager,
                                               executeFuncs / executeCount) */

void T070_static_init(void);                /* 0x0014E880 (.ctors 0x0028F098) g_00351810[0] = {"input_text",
                                               text_box_key} */

#endif /* __cplusplus */

#endif /* GAME_T070_MEMORYCARD_ACTIONS_H */
