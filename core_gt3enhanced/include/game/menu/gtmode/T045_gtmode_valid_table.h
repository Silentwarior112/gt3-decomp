/* T045_gtmode_valid_table.h - g_00296750, the ValidFuncData table of the
 * GT-mode menu, and its entry count (translation unit T045, unit u009; the
 * checks and RegurationResult are in T045_gtmode_page_checks.h, which
 * includes this header where the declarations were).
 *
 * In its own header since u009's data pass (2026-10-17): GtmodeControl::begin
 * (T043) gives the table to its PageManager, and retail's T043 has no
 * RegurationResult type name in its .rodata (the checks' header brings the
 * class; egcs writes the type name of every class it sees whose virtual
 * functions are all inline). */
#ifndef GAME_T045_GTMODE_VALID_TABLE_H
#define GAME_T045_GTMODE_VALID_TABLE_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ValidFuncData (g_00296750) */

/* T045 .data, filled by T045_static_init (.ctors 0x0028F058; compiler-
 * generated in the original): {"00cmn/cd_buy_sure", check_cd_buy_sure} ..
 * {"wash_car", check_wash_car}, sorted by name.  GtmodeControl::begin (T043)
 * gives it to its PageManager (valid table +0xB24, ctx the GtmodeControl). */
extern gt2k_menu::ValidFuncData g_00296750[34];    /* 0x00296750 */
extern int g_00351100;                      /* 0x00351100 gp-0x68F0 its entries (34; defined by
                                               T045's C) */

#endif /* GAME_T045_GTMODE_VALID_TABLE_H */
