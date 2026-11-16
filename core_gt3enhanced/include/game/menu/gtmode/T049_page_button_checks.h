/* T049_page_button_checks.h - the SelectableFuncData table of the GT-mode
 * menu (translation unit T049, unit u008), 0x001190D0..0x00119200.
 *
 * g_002959E8 (filled by T049_static_init) gives two pages the availability
 * check of each button: "r/pro/pro" 25 checks (g_0030F508: 24 x
 * event_check_unlocked, then event_check_false) and "ts/ts_ot" 8
 * (g_0030F570: event_check_true, event_check_has_wtr_1,
 * event_check_has_wtr_2, 4 x event_check_true, event_check_false).
 * GtmodeControl::begin (T043) gives it to its PageManager (selectable table
 * +0xB00, count g_00351040, ctx the GtmodeControl); ButtonList::
 * setButtonChecks (T244) stores check i in button i (the last one for the
 * rest), and a button is available when check(ctx, its record) != 0.
 *
 * The checks take the SelectableFuncData signature (the first pass had
 * (void *ctx, const char *linkMenu)): they read the record as its name
 * (ButtonData::name, +0x00), the event code of a race button.  The first
 * pass's PageButtonChecks (u008) is T211's SelectableFuncData; the alias is
 * kept below. */
#ifndef GAME_T049_PAGE_BUTTON_CHECKS_H
#define GAME_T049_PAGE_BUTTON_CHECKS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::SelectableFuncData (g_002959E8) */

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
typedef SelectableFuncData PageButtonChecks;    /* u008: {page, checks, count}, 0xC bytes */
}

/* The checks (ctx is not used). */
int  event_check_false(void *ctx, gt2k_menu::ButtonData *data);    /* 0x001190D0 0: not available */
int  event_check_true(void *ctx, gt2k_menu::ButtonData *data);     /* 0x001190D8 1 */
/* The event data base's requirement of the event (T435 F_0021D810 of
 * data->name) <= GameStatus F_00219D80() (0 / 1); the test of T036's
 * event_unlocked. */
int  event_check_unlocked(void *ctx, gt2k_menu::ButtonData *data); /* 0x001190E0 */
/* The ridden car owns part 0x1B / 0x1C (CarInformation::hasPart; the parts
 * T036's text_price_wtr_1 / text_price_wtr_2 price: T482's
 * PART_LIGHTWEIGHT_STAGE1 / STAGE2, weight reduction); 0 without a ridden
 * car.  (? checks 1 / 2 of "ts/ts_ot" are the stage 2 / 3 buttons: a stage
 * needs the one before.) */
int  event_check_has_wtr_1(void *ctx, gt2k_menu::ButtonData *data);    /* 0x00119130 */
int  event_check_has_wtr_2(void *ctx, gt2k_menu::ButtonData *data);    /* 0x00119170 */

/* T049 .data: {"r/pro/pro", g_0030F508, 25}, {"ts/ts_ot", g_0030F570, 8};
 * compiler-generated static initialiser in the original (.ctors
 * 0x0028F060).  The check arrays (.rodata 0x0030F508 "r/pro/pro",
 * 0x0030F570 "ts/ts_ot") are file-local consts of T049 (they follow its
 * type names), not declared here. */
extern gt2k_menu::SelectableFuncData g_002959E8[2];                     /* 0x002959E8 */
extern const int g_00351040;                /* 0x00351040 gp-0x69B0 its entries (2; const:
                                               see T016_ending_menu.h) */
void T049_static_init(void);                /* 0x001191B0 */

#endif /* GAME_T049_PAGE_BUTTON_CHECKS_H */
