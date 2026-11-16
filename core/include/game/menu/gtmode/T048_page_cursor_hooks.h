/* T048_page_cursor_hooks.h - the FocusFuncData table of the GT-mode menu
 * (translation unit T048, unit u008), 0x00117320..0x00117C58.
 *
 * g_00295380 (92 entries, sorted by page name, filled by T048_static_init)
 * gives the cursor a page starts on: the licence pages "lc/lc_*", the race
 * pages "r/<league>/<series>" and "root".  GtmodeControl::begin (T043)
 * gives it to its PageManager (focus table +0xB30, count g_00350FF4, ctx the
 * GtmodeControl); PageManager::startPage (T211) calls func(ctx, pm) and falls
 * back to the previous page's button and the cursor memory when the page has
 * no entry or the result is negative (these hooks never return one).
 *
 * The race and licence hooks read the number at the end of the menu status's
 * event code (MenuStatus::event, GameStatus +0x307C8 +0x44: the race or test
 * last entered) and then clear it (MenuStatus::setEvent("")).
 *
 * The first pass's PageCursorHook (u008) is T211's FocusFuncData; the alias
 * is kept below. */
#ifndef GAME_T048_PAGE_CURSOR_HOOKS_H
#define GAME_T048_PAGE_CURSOR_HOOKS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::FocusFuncData (g_00295380) */

namespace gt2k_menu {
typedef FocusFuncData PageCursorHook;       /* u008: {page, cursor function}, 8 bytes */
}

/* The FocusFuncData functions (ctx: the GtmodeControl; pm is not used). */
/* "root": GtmodeControl::mode (+0xE48, how the root page was entered). */
int  cursor_from_owner(void *ctx, gt2k_menu::PageManager *pm);             /* 0x00117320 */
/* The licence pages: the event's last character - '1' if 0..7, else 0. */
int  cursor_from_last_char_1(void *ctx, gt2k_menu::PageManager *pm);       /* 0x00117328 */
/* Most race pages: the last character - '0' (negative: 0). */
int  cursor_from_last_digit(void *ctx, gt2k_menu::PageManager *pm);        /* 0x001173A0 */
/* The other race pages: the last two characters as a number, + 1 unless
 * MenuStatus::raceMode is 2 (a championship: T037's
 * action_set_race_code_champ stores its code with the last digit + 1)
 * (negative: 0). */
int  cursor_from_last_two_digits(void *ctx, gt2k_menu::PageManager *pm);   /* 0x00117418 */

/* T048 .data 0x00295380..0x00295660: {"lc/lc_a", cursor_from_last_char_1}
 * .. {"root", cursor_from_owner}; compiler-generated static initialiser in
 * the original (.ctors 0x0028F05C). */
extern gt2k_menu::FocusFuncData g_00295380[92];                             /* 0x00295380 */
extern const int g_00350FF4;                /* 0x00350FF4 gp-0x69FC its entries (92; const:
                                               see T016_ending_menu.h) */
void T048_static_init(void);                /* 0x001174B0 */

#endif /* GAME_T048_PAGE_CURSOR_HOOKS_H */
