/* T016_ending_menu.h - the page hooks of the arcade "ar/select_ending" page
 * (translation unit T016, unit u001), 0x00103038..0x00103218.
 *
 * ArcadeControl::setup (T010) gives two one-entry tables of this unit to its
 * PageManager (T211): g_002902E0 as the SelectableFuncData table (hooks[0])
 * and g_00350B68 as the ValidFuncData table (hooks[3]).  On the page, button
 * 0 is always selectable, the others once an amateur or professional league
 * or every arcade race of the normal, hard or professional level is won; a
 * button linking to the page opens the popup "ar/cannot_ending" instead
 * while no league and no arcade level is won.  The check array g_00350B78
 * is static data; T016_static_init fills in the names and the other
 * pointers. */
#ifndef GAME_T016_ENDING_MENU_H
#define GAME_T016_ENDING_MENU_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::SelectableFuncData, ValidFuncData */

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
}

/* A button check of a SelectableFuncData entry (T211). */
typedef int SelectEndingCheck(void *ctx, gt2k_menu::ButtonData *data);

/* The table counts are `const int' (T016, T019, T027, T028, T030): the
 * original's, which T010's ArcadeControl::setup, their only reader, shows:
 * a load of unchanging memory has no anti-dependence on the stores after
 * it, and only so does sched2 load them as late as retail (as `int' they
 * are hoisted).  Each count is .sdata either way (-G8). */
/* hooks[0]: {"ar/select_ending", g_00350B78, 2}. */
extern gt2k_menu::SelectableFuncData g_002902E0[1];    /* 0x002902E0 */
extern const int g_00350B60;                /* 0x00350B60 gp-0x6E90 its entries (1) */
/* hooks[3]: {"ar/select_ending", select_ending_valid}.  Declared without a
 * size, as the original: T010 builds its address with lui / addiu, not
 * through $gp (with the size, 8 bytes, it would be small data under -G8). */
extern gt2k_menu::ValidFuncData g_00350B68[];          /* 0x00350B68 (1 entry) */
extern const int g_00350B70;                /* 0x00350B70 gp-0x6E80 its entries (1) */
/* The checks of g_002902E0[0]: {select_ending_check0, select_ending_check1}. */
extern SelectEndingCheck *g_00350B78[2];    /* 0x00350B78 gp-0x6E78 */

/* The checks and the popup function take the tables' arguments; the
 * definitions use none of them. */
int  select_ending_check0(void *ctx, gt2k_menu::ButtonData *data);  /* 0x00103038 1 */
/* EventResults amateur / professional league won, or ArcadeRecords all won
 * on normal / hard / professional (GameStatus, T425 / T424). */
int  select_ending_check1(void *ctx, gt2k_menu::ButtonData *data);  /* 0x00103040 */
/* NULL if any league (beginner, amateur, professional) or any arcade level
 * (easy .. professional) is won, else the popup page "ar/cannot_ending". */
const char *select_ending_valid(void *ctx, gt2k_menu::ButtonData *rec);    /* 0x001030F0 */
void T016_static_init(void);                /* 0x001031D8 (.ctors 0x0028F030) */

#endif /* GAME_T016_ENDING_MENU_H */
