/* T070 - the ExecuteFuncData table of the memory card page
 * (menu/memorycard.mbl).  0x0014E880..0x0014E8A8.
 *
 * MemorycardControl::begin (T068) installs g_00351810 (and its count
 * g_00351818) as the page manager's execute functions: the "input_text"
 * buttons of the name entry run text_box_key (T231).
 *
 * Includes (data pass): retail's T070 .rodata (0x0031D518..0x0031D708) is
 * "input_text", then the type names OperatorList, List / OperatorHolder /
 * Cell<OperatorHolder>, SmartIndexT<int>, the GameStatus's garage lists
 * (wheel list first) and Operator last: the GameStatus, then T211
 * (SmartIndexT<int>), then T231 (the operator holders), in that order.
 * Compiled -G8 (ref/compile_flags.json): the table and its count are retail's
 * .sdata. */
#include "game/save/T429_game_status.h"          /* (no use: retail's T070 saw the GameStatus) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ExecuteFuncData; SmartIndexT<int> */
#include "game/menu/framework/T231_text_box.h"             /* gt2k_menu::text_box_key */
#include "game/menu/memorycard/T070_memorycard_actions.h"
#include "game/menu/memorycard/memorycard_menu_internal.h"

/* The table, one entry: zeros in the file (.sdata), filled by the static
 * initialiser below.  In the original an ExecuteFuncData built by a
 * constructor (u005's T028 notes), which egcs writes as zeros where it is
 * defined and fills at run time; the explicit zero initialiser gives the
 * same bytes in the same place (an uninitialised C++ global would come
 * after the count, at the end of the file's data). */
gt2k_menu::ExecuteFuncData g_00351810[1] = { { NULL, NULL } };     /* 0x00351810 */
int g_00351818 = sizeof(g_00351810) / sizeof(g_00351810[0]);       /* 0x00351818 */

/* Static initialiser (.ctors entry 0x0028F098).  Compiler-generated in the
 * original (_GLOBAL_$I$...: the table is in .sdata, zero in the file);
 * written out here as the stores it makes. */
// FUNC 0x0014E880 T070_static_init
void T070_static_init(void)
{
    g_00351810[0].name = "input_text";
    g_00351810[0].func = gt2k_menu::text_box_key;
}
