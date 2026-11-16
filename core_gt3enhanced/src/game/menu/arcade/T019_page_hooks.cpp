/* T019 - the FocusFuncData table of the arcade menu.  0x00104EE0..0x00104F30.
 *
 * g_00290940 (this unit's .data, right before its .eh_frame block) holds two
 * {page, func} entries that the static initialiser fills in at startup:
 * {"ar/connect", arcade_connect_focus}, {"ar/top", arcade_top_focus}.
 * ArcadeControl::setup (T010) hands it to its PageManager as the focus
 * table (hooks[4], count g_00350C08 = 2, ctx the ArcadeControl): the
 * initial cursor of each page.
 *
 * Data: .data g_00290940 (zeros), .sdata g_00350C08 then "ar/top", .rodata
 * "ar/connect" then the one type name of the original, SmartIndexT<int>
 * (T010's T251): no menu or garage header. */
#include "types.h"
#include "game/menu/arcade/T019_page_hooks.h"
#include "game/menu/arcade/T010_arcade_control.h"       /* gt2k_menu::ArcadeControl::mode */

/* The table: no initialiser (the static initialiser below fills it in), so
 * egcs writes it as zeros at the end of the file's .data, before the frame
 * info, where the original has it. */
gt2k_menu::FocusFuncData g_00290940[2];     /* 0x00290940 */
/* Its entries: .sdata at the definition, in front of "ar/top" (T019_static_init's). */
// DATA_START 0x00350C08
const int g_00350C08 = 2;                   /* 0x00350C08 gp-0x6DE8 */

/* "ar/top": the cursor starts on the current mode (ArcadeControl::mode). */
// FUNC 0x00104EE0 arcade_top_focus
int arcade_top_focus(void *ctx, gt2k_menu::PageManager *pm)
{
    return ((gt2k_menu::ArcadeControl *)ctx)->mode;
}

/* "ar/connect": the cursor starts on button 1. */
// FUNC 0x00104EE8 arcade_connect_focus
int arcade_connect_focus(void *ctx, gt2k_menu::PageManager *pm)
{
    return 1;
}

/* Static initialiser (.ctors entry 0x0028F034), compiler-generated in the
 * original: the table had a dynamic initialiser (its .data is 16 zero
 * bytes; egcs makes a constant aggregate initialiser static data).  Defined
 * as FocusFuncData("ar/connect", arcade_connect_focus), ... with an inline
 * two-argument constructor, egcs generates exactly this function (tested):
 * the elements built in place by a pointer walking the array
 * (build_vec_init: `sw 4($a1)', then `addiu $a1, 8').  Written out here
 * (the matching pass, when the table was not defined by the C; the original
 * form needs a constructor in T211's FocusFuncData, which u006's T030 shares):
 * an exact form, not the original (the same two entries; u006's
 * T030_static_init has the same shape).  The walking pointer gives the
 * generated code (indexing the array folds every address into its own
 * %lo).  "ar/top" (7 bytes) is .sdata (0x00350C10, after the count
 * g_00350C08): -G8. */
// FUNC 0x00104EF0 T019_static_init
void T019_static_init(void)
{
    gt2k_menu::FocusFuncData *p = g_00290940;
    p->page = "ar/connect";
    p->func = arcade_connect_focus;
    p++;
    p->page = "ar/top";
    p->func = arcade_top_focus;
}
