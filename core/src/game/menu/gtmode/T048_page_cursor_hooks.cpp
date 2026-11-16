/* T048 - the FocusFuncData table of the GT-mode menu.  0x00117320..0x00117C58.
 *
 * g_00295380 (92 entries, sorted by page, filled by the static initialiser)
 * gives the cursor a page starts on: the licence pages "lc/lc_*", the race
 * pages "r/<league>/<series>" and "root".  GtmodeControl::begin (T043)
 * gives it to its PageManager (ctx: the GtmodeControl); PageManager::
 * startPage (T211) calls func(ctx, pm) and falls back to the previous
 * page's button and the cursor memory when the page has no entry or the
 * result is negative (these hooks never return one).  The race and licence
 * hooks read the number at the end of MenuStatus::event (the code of the
 * race or test last entered) and then clear it (setEvent("")).
 *
 * Retail keeps the menu status's offset (0x307C8) in a saved register for
 * both getGameStatus() calls: the inline GameStatus::getMenuStatus().  The
 * negative check is `cursor = 0; if (race >= 0) cursor = race;' (retail's
 * slti / movz; `race < 0 ? 0 : race' folds to a MAX_EXPR: li -1 / slt /
 * movn). */
/* Retail's .rodata (0x0030E668..0x0030EFE8): the page names, then the type
 * names in the reverse order of the classes' definitions: List / Cell<Button>,
 * OperatorList, List / Cell<OperatorHolder>, OperatorHolder, the
 * GameStatus's garage lists, SmartIndexT<int> (GtmodeControl's members: read
 * first), Operator (a base: last). */
#include "game/menu/gtmode/T043_gtmode_control.h"       /* GtmodeControl::mode */
#include "game/save/T429_game_status.h"          /* getGameStatus, getMenuStatus; T427 MenuStatus */
#include "game/menu/framework/T206_button.h"               /* (seen, not used: Cell<Button>, the operator classes) */
#include "game/menu/framework/T244_button_list.h"          /* (seen, not used: List<Button>) */
#include "engine/media/G002_mpeg_frameq.h"          /* strlen */
#include "game/menu/gtmode/T048_page_cursor_hooks.h"
#include "game/menu/gtmode/gtmode_buttons_internal.h"

/* The table, .data 0x00295380..0x00295660 (92 entries of 8 bytes), zero in
 * the image: T048_static_init fills it at start-up (in the original egcs's
 * code for the table's constructor-built initialiser, see there).  A C++
 * global without an initialiser: egcs writes it as zeros at the end of the
 * file's .data, before the frame info, as retail has it. */
gt2k_menu::FocusFuncData g_00295380[92];                                    /* 0x00295380 */

// FUNC 0x00117320 cursor_from_owner
/* "root": how the root page was entered (GtmodeControl::mode). */
int cursor_from_owner(void *ctx, gt2k_menu::PageManager *pm)
{
    return ((gt2k_menu::GtmodeControl *)ctx)->mode;
}

// FUNC 0x00117328 cursor_from_last_char_1
/* The licence pages: the event's last character - '1' (the test number) if
 * it is 0..7, else 0. */
int cursor_from_last_char_1(void *ctx, gt2k_menu::PageManager *pm)
{
    char *event = getGameStatus()->getMenuStatus()->event;
    int test = event[strlen(event) - 1] - '1';  /* signed char load */

    getGameStatus()->getMenuStatus()->setEvent("");
    return ((u32)test <= 7) ? test : 0;
}

// FUNC 0x001173A0 cursor_from_last_digit
/* Most race pages: the event's last character - '0' (negative: 0). */
int cursor_from_last_digit(void *ctx, gt2k_menu::PageManager *pm)
{
    char *event = getGameStatus()->getMenuStatus()->event;
    int race = event[strlen(event) - 1] - '0';
    int cursor;

    getGameStatus()->getMenuStatus()->setEvent("");
    cursor = 0;
    if (race >= 0)
        cursor = race;
    return cursor;
}

// FUNC 0x00117418 cursor_from_last_two_digits
/* The other race pages: the event's last two characters as a decimal
 * number, plus 1 unless MenuStatus::raceMode is 2 (a championship: T037
 * action_set_race_code_champ already gave its code the last digit + 1);
 * negative: 0. */
int cursor_from_last_two_digits(void *ctx, gt2k_menu::PageManager *pm)
{
    char *event = getGameStatus()->getMenuStatus()->event;
    int len = strlen(event);
    int race = (event[len - 2] - '0') * 10 + (event[len - 1] - '0');
    int cursor;

    if (getGameStatus()->getMenuStatus()->raceMode != RACE_MODE_CHAMPIONSHIP)
        race = race + 1;
    getGameStatus()->getMenuStatus()->setEvent("");
    cursor = 0;
    if (race >= 0)
        cursor = race;
    return cursor;
}

/* The table's entry count, .sdata 0x00350FF4 (a constant word, 92), read by
 * GtmodeControl::begin (T043).  Defined here, after the hooks, where the
 * original defined it: its place in .sdata, after the hooks' "" and before
 * the table's short page names, shows it (as u006's g_00350F48). */
const int g_00350FF4 = sizeof(g_00295380) / sizeof(g_00295380[0]);

/* One table entry: the inline constructor of the original's FocusFuncData
 * (see T048_static_init). */
static inline void set_focus(gt2k_menu::FocusFuncData *e, const char *page,
                             int (*func)(void *ctx, gt2k_menu::PageManager *pm))
{
    e->page = page;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it: the constructor called on
 * a copy of the cursor (`this'), then the cursor and the count of the
 * elements left stepped (u006's T030_static_init has the same form). */
#define FOCUS(page, func)                                                     \
    { gt2k_menu::FocusFuncData *e = base; set_focus(e, page, func); }        \
    base++;                                                                   \
    left--

// FUNC 0x001174B0 T048_static_init
/* Static initialiser (.ctors entry 0x0028F05C): fills g_00295380 (92
 * entries, sorted by page: PageManager binary-searches them; entry i at
 * 0x00295380 + 8 * i).  Compiler-generated in the original: the table was
 * defined here with FocusFuncData objects built by an inline constructor
 * (FocusFuncData("lc/lc_a", cursor_from_last_char_1), ...), which egcs builds
 * at run time (cp/init.c build_vec_init) in the _GLOBAL_$I$ function, leaving
 * .data zero.  The decomp defines the table as a plain zero global (above;
 * the same .data) and writes the function out as build_vec_init expands it:
 * an exact form, not the original (the count of the elements left, `left', is
 * dead; flow deletes it).  It stores the same 92 entries as a plain
 * g_00295380[i].page / .func run, which scores 11.6%: the walking cursor and
 * the store order are build_vec_init's. */
void T048_static_init(void)
{
    gt2k_menu::FocusFuncData *base = g_00295380;
    int left = 91;                          /* build_vec_init's iterator: elements - 1 */

    FOCUS("lc/lc_a",                    cursor_from_last_char_1);
    FOCUS("lc/lc_b",                    cursor_from_last_char_1);
    FOCUS("lc/lc_d",                    cursor_from_last_char_1);
    FOCUS("lc/lc_ia",                   cursor_from_last_char_1);
    FOCUS("lc/lc_ib",                   cursor_from_last_char_1);
    FOCUS("lc/lc_s",                    cursor_from_last_char_1);
    FOCUS("r/amateur/g_4wd_n",          cursor_from_last_digit);
    FOCUS("r/amateur/g_80sports_n",     cursor_from_last_digit);
    FOCUS("r/amateur/galtezza_n",       cursor_from_last_two_digits);
    FOCUS("r/amateur/gboxer_n",         cursor_from_last_digit);
    FOCUS("r/amateur/gdeu_touring_n",   cursor_from_last_two_digits);
    FOCUS("r/amateur/gdream_n",         cursor_from_last_two_digits);
    FOCUS("r/amateur/geurope_n",        cursor_from_last_two_digits);
    FOCUS("r/amateur/gevolution_n",     cursor_from_last_digit);
    FOCUS("r/amateur/gff_n",            cursor_from_last_digit);
    FOCUS("r/amateur/gfr_n",            cursor_from_last_digit);
    FOCUS("r/amateur/ggtallstar_n",     cursor_from_last_two_digits);
    FOCUS("r/amateur/ggtr_n",           cursor_from_last_digit);
    FOCUS("r/amateur/ggtworld_n",       cursor_from_last_two_digits);
    FOCUS("r/amateur/gjapan_n",         cursor_from_last_two_digits);
    FOCUS("r/amateur/gjgtc_n",          cursor_from_last_two_digits);
    FOCUS("r/amateur/gmercedes_n",      cursor_from_last_digit);
    FOCUS("r/amateur/gmr_n",            cursor_from_last_digit);
    FOCUS("r/amateur/gna_n",            cursor_from_last_digit);
    FOCUS("r/amateur/gstar_n",          cursor_from_last_digit);
    FOCUS("r/amateur/gtt_n",            cursor_from_last_two_digits);
    FOCUS("r/amateur/gturbo_n",         cursor_from_last_digit);
    FOCUS("r/amateur/gtype_r_n",        cursor_from_last_two_digits);
    FOCUS("r/amateur/gusa_n",           cursor_from_last_two_digits);
    FOCUS("r/beginner/g_4wd_e",         cursor_from_last_digit);
    FOCUS("r/beginner/g_80sports_e",    cursor_from_last_digit);
    FOCUS("r/beginner/galtezza_e",      cursor_from_last_two_digits);
    FOCUS("r/beginner/gbeetle_e",       cursor_from_last_two_digits);
    FOCUS("r/beginner/gclubman_e",      cursor_from_last_digit);
    FOCUS("r/beginner/gevolution_e",    cursor_from_last_digit);
    FOCUS("r/beginner/gff_e",           cursor_from_last_digit);
    FOCUS("r/beginner/gfr_e",           cursor_from_last_digit);
    FOCUS("r/beginner/ggtworld_e",      cursor_from_last_two_digits);
    FOCUS("r/beginner/gkcar_e",         cursor_from_last_digit);
    FOCUS("r/beginner/gmercedes_e",     cursor_from_last_digit);
    FOCUS("r/beginner/gmr_e",           cursor_from_last_digit);
    FOCUS("r/beginner/gna_e",           cursor_from_last_digit);
    FOCUS("r/beginner/gspider_e",       cursor_from_last_digit);
    FOCUS("r/beginner/gstar_e",         cursor_from_last_digit);
    FOCUS("r/beginner/gsunday_e",       cursor_from_last_digit);
    FOCUS("r/beginner/gtt_e",           cursor_from_last_two_digits);
    FOCUS("r/beginner/gturbo_e",        cursor_from_last_digit);
    FOCUS("r/beginner/gtype_r_e",       cursor_from_last_two_digits);
    FOCUS("r/beginner/gvitz_e",         cursor_from_last_two_digits);
    FOCUS("r/dirt/dr_route5wet_a",      cursor_from_last_digit);
    FOCUS("r/dirt/dr_smokymt_a",        cursor_from_last_digit);
    FOCUS("r/dirt/dr_swiss_a",          cursor_from_last_digit);
    FOCUS("r/dirt/dr_tahiticircuit_a",  cursor_from_last_digit);
    FOCUS("r/dirt/dr_tahitimaze_a",     cursor_from_last_digit);
    FOCUS("r/dirt/droute5wet_a",        cursor_from_last_digit);
    FOCUS("r/dirt/dsmokymt_a",          cursor_from_last_digit);
    FOCUS("r/dirt/dswiss_a",            cursor_from_last_digit);
    FOCUS("r/dirt/dtahiticircuit_a",    cursor_from_last_digit);
    FOCUS("r/dirt/dtahitimaze_a",       cursor_from_last_digit);
    FOCUS("r/end/eakasaka_n",           cursor_from_last_digit);
    FOCUS("r/end/egrandvalley300_n",    cursor_from_last_digit);
    FOCUS("r/end/elaguna200_n",         cursor_from_last_digit);
    FOCUS("r/end/emonaco_n",            cursor_from_last_digit);
    FOCUS("r/end/eroadster_n",          cursor_from_last_digit);
    FOCUS("r/end/erome2h_n",            cursor_from_last_digit);
    FOCUS("r/end/eroute11_n",           cursor_from_last_digit);
    FOCUS("r/end/eseattle100_n",        cursor_from_last_digit);
    FOCUS("r/end/essw150_n",            cursor_from_last_digit);
    FOCUS("r/end/etrialmt2h_n",         cursor_from_last_digit);
    FOCUS("r/pro/g_4wd_h",              cursor_from_last_digit);
    FOCUS("r/pro/gboxer_h",             cursor_from_last_digit);
    FOCUS("r/pro/gbritish_h",           cursor_from_last_digit);
    FOCUS("r/pro/gclio_h",              cursor_from_last_two_digits);
    FOCUS("r/pro/gdream_h",             cursor_from_last_two_digits);
    FOCUS("r/pro/gelise_h",             cursor_from_last_two_digits);
    FOCUS("r/pro/gf1_h",                cursor_from_last_two_digits);
    FOCUS("r/pro/gff_h",                cursor_from_last_digit);
    FOCUS("r/pro/gfr_h",                cursor_from_last_digit);
    FOCUS("r/pro/ggtallstar_h",         cursor_from_last_two_digits);
    FOCUS("r/pro/ggtr_h",               cursor_from_last_digit);
    FOCUS("r/pro/ggtworld_h",           cursor_from_last_two_digits);
    FOCUS("r/pro/gitaly_h",             cursor_from_last_digit);
    FOCUS("r/pro/gjgtc_h",              cursor_from_last_two_digits);
    FOCUS("r/pro/gmr_h",                cursor_from_last_digit);
    FOCUS("r/pro/gna_h",                cursor_from_last_digit);
    FOCUS("r/pro/gpolyphony_h",         cursor_from_last_two_digits);
    FOCUS("r/pro/gspider_h",            cursor_from_last_digit);
    FOCUS("r/pro/gturbo_h",             cursor_from_last_digit);
    FOCUS("r/pro/gtuscan_h",            cursor_from_last_two_digits);
    FOCUS("r/pro/gvitz_h",              cursor_from_last_two_digits);
    FOCUS("r/pro/gwind_h",              cursor_from_last_digit);
    FOCUS("root",                       cursor_from_owner);
}
