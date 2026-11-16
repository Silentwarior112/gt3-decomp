/* T016 - the page hooks of the arcade "ar/select_ending" page.
 * 0x00103038..0x00103218.
 *
 * g_00350B78 = {select_ending_check0, select_ending_check1} and the tables'
 * counts are static data; the static initialiser fills in the names and
 * the other pointers.  The checks take the PageManager's arguments (T211)
 * and use none of them. */
#include "game/save/T429_game_status.h"          /* getGameStatus, EventResults, ArcadeRecords */
#include "game/menu/framework/T016_ending_menu.h"

/* g_00350B78[0]: button 0 is always selectable. */
// FUNC 0x00103038 select_ending_check0
int select_ending_check0(void *ctx, gt2k_menu::ButtonData *data)
{
    return 1;
}

/* g_00350B78[1]: the amateur or professional league won, or every arcade
 * race of the normal, hard or professional level won. */
// FUNC 0x00103040 select_ending_check1
int select_ending_check1(void *ctx, gt2k_menu::ButtonData *data)
{
    int won = 0;
    if (getGameStatus()->events.amateurLeagueWon() != 0 ||
        getGameStatus()->events.professionalLeagueWon() != 0 ||
        getGameStatus()->arcade.allWonNormal() != 0 ||
        getGameStatus()->arcade.allWonHard() != 0 ||
        getGameStatus()->arcade.allWonProfessional() != 0)
        won = 1;
    return won;
}

/* g_00350B68[0].func: NULL (no popup) if any league or any arcade level is
 * won, else the popup page "ar/cannot_ending". */
// FUNC 0x001030F0 select_ending_valid
const char *select_ending_valid(void *ctx, gt2k_menu::ButtonData *rec)
{
    int won = 0;
    if (getGameStatus()->events.beginnerLeagueWon() != 0 ||
        getGameStatus()->events.amateurLeagueWon() != 0 ||
        getGameStatus()->events.professionalLeagueWon() != 0 ||
        getGameStatus()->arcade.allWonEasy() != 0 ||
        getGameStatus()->arcade.allWonNormal() != 0 ||
        getGameStatus()->arcade.allWonHard() != 0 ||
        getGameStatus()->arcade.allWonProfessional() != 0)
        won = 1;
    if (won != 0)
        return NULL;
    return "ar/cannot_ending";
}

/* Static initialiser (.ctors entry 0x0028F030), in the tables' member
 * order: SelectableFuncData's, then ValidFuncData's. */
// FUNC 0x001031D8 T016_static_init
void T016_static_init(void)
{
    g_002902E0[0].page   = "ar/select_ending";
    g_002902E0[0].checks = g_00350B78;
    g_002902E0[0].count  = 2;
    g_00350B68[0].name   = "ar/select_ending";
    g_00350B68[0].func   = select_ending_valid;
}

/* The tables (zero until T016_static_init; the counts are static): .sdata
 * 0x00350B60..0x00350B80 in this order, the first table in .data.  The
 * static initialiser addresses g_00350B68 with lui / addiu although it is in
 * the .sdata: egcs decides a variable's $gp addressing at its first
 * declaration, the header's unsized one (whichever side of the functions
 * the definition is). */
// DATA_START 0x00350B60
gt2k_menu::SelectableFuncData g_002902E0[1] = { { NULL, NULL, 0 } };     /* 0x002902E0 */
const int g_00350B60 = 1;                   /* 0x00350B60 gp-0x6E90 */
gt2k_menu::ValidFuncData g_00350B68[1] = { { NULL, NULL } };             /* 0x00350B68 */
const int g_00350B70 = 1;                   /* 0x00350B70 gp-0x6E80 */
SelectEndingCheck *g_00350B78[2] = { select_ending_check0, select_ending_check1 };   /* 0x00350B78 gp-0x6E78 */
