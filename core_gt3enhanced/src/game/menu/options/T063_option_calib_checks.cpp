/* T063 - the page checks of the options screen.  0x0013FF98..0x00140060.
 *
 * T063_static_init fills g_002A0A70 with two ValidFuncData entries;
 * OptionControl::begin (T062) gives them to its PageManager
 * (setValidFuncData, ctx the OptionControl).  PageManager::callValidFunc
 * calls an entry for a decided button whose link (or exec name) is the
 * entry's name; a non-NULL result is the page to open as a popup instead.
 * Both entries guard the USB wheel calibration pages.
 *
 * The includes are in the order that gives retail's type names (.rodata
 * 0x0031A028, reverse definition order): Cell<Button>, OperatorList,
 * List / OperatorHolder / Cell<OperatorHolder>, SmartIndexT<int>, the garage
 * lists (wheel list .. Cell<GarageCar>), Operator.  So the original defined
 * the GameStatus' garage classes first (T429), then the page manager (T211's
 * SmartIndexT<int>), then the menu classes (T216), then the buttons (T206);
 * T216 / T206 for those names only. */
#include "game/save/T429_game_status.h"          /* (the garage classes) */
#include "game/menu/options/T063_option_calib_checks.h"  /* g_002A0A70, g_003516F0; T211 ValidFuncData */
#include "game/menu/options/T062_option_control.h"       /* OptionControl (ctx): calibPort */
#include "game/menu/framework/T216_menu_transitive.h"      /* (OperatorHolder, OperatorList) */
#include "game/menu/framework/T206_button.h"               /* (Cell<Button>) */
#include "engine/input/T475_pad_manager.h"          /* PadManager g_002EA7F8 (getPortStatus) */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se, MENU_SE_REFUSED */
#include "game/menu/options/options_internal.h"                           /* the unit's constants and callbacks */

/* .sdata opens with g_003516F0, which only T062's code reads. */
// DATA_START 0x003516F0

using namespace gt2k_menu;

/* "gt_force1/calib_steer", "gt_force2/calib_steer": the calibration page
 * of the wheel port being set up (calibPort, set by T067's set_port_*
 * actions) needs a device in that port's USB slot.  Without one: the
 * refused sound and the "set_gtforce" page of the port instead.  Any other
 * calibPort: NULL.  rec is not used.
 *
 * Not exact (93.6%): retail reads calibPort again at the second test on
 * both paths into it (the reload fills both bnel slots).  egcs's GCSE (PRE)
 * here keeps the first load's value for the path from the first test and
 * reloads only after the getPortStatus call.  Tried: if / else-if, nested
 * ifs, early returns, a result variable, a switch for either test, an
 * inline accessor or an inline helper for either or both tests, ctx / ctl
 * mixed, a local for the port, do-while(0): all 75..94%; the reference
 * compiler 991111-01 gives the same 93.6%.  Exact only with -fno-gcse (no
 * evidence for it elsewhere) or with a second pointer `OptionControl *c2 =
 * ctl;' for the second test (its copy keeps the two loads apart for PRE),
 * which no original would have; neither is kept (u022 notes). */
// FUNC 0x0013FF98 option_check_calib_steer
const char *option_check_calib_steer(void *ctx, ButtonData *rec)
{
    OptionControl *ctl = (OptionControl *)ctx;

    if (ctl->calibPort == CALIB_PORT_1 && g_002EA7F8.getPortStatus(PAD_SLOT_USB1) == 0) {
        menu_play_se(MENU_SE_REFUSED);
        return "gt_force1/set_gtforce";
    }
    if (ctl->calibPort == CALIB_PORT_2 && g_002EA7F8.getPortStatus(PAD_SLOT_USB2) == 0) {
        menu_play_se(MENU_SE_REFUSED);
        return "gt_force2/set_gtforce";
    }
    return NULL;
}

/* The table, .data 0x002A0A70: no initialiser, so egcs writes its zeros in
 * .data in front of the frame info (retail's are zero too: the static
 * initialiser below fills it at boot). */
ValidFuncData g_002A0A70[2];                /* 0x002A0A70 */

/* The table's entry count, .sdata 0x003516F0: a constant word (2), read
 * by OptionControl::begin (T062); defined here, before the initialiser. */
int g_003516F0 = sizeof(g_002A0A70) / sizeof(g_002A0A70[0]);

/* One table entry: the inline constructor of the original's ValidFuncData
 * (see T063_static_init). */
static inline void set_check(ValidFuncData *e, const char *name,
                             const char *(*func)(void *ctx, ButtonData *rec))
{
    e->name = name;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it (u006's form): the
 * constructor on a copy of the cursor, then the cursor and the count of
 * the elements left stepped. */
#define CHECK(name, func)                                                         { ValidFuncData *e = base; set_check(e, name, func); }                        base++;                                                                       left--

/* Static initialiser of g_002A0A70 (.ctors entry 0x0028F080): a
 * compiler-generated _GLOBAL_$I$ function in the original, which defined
 * the table here with ValidFuncData objects built by an inline constructor
 * (egcs builds such a table at run time, cp/init.c build_vec_init; retail's
 * .data 0x002A0A70..0x002A0A80 is zero).  The original's dynamic
 * initialiser needs a constructor in T211's ValidFuncData, a plain struct
 * other units share; the table is defined above without one, so the
 * function is written out the way build_vec_init expands it (a walking
 * cursor, a copy of it per element, the dead count of the elements left):
 * an exact form, not the original; it stores the same two entries. */
// FUNC 0x00140028 T063_static_init
void T063_static_init(void)
{
    ValidFuncData *base = g_002A0A70;
    int left = 1;                           /* build_vec_init's iterator: elements - 1 */

    CHECK("gt_force1/calib_steer", option_check_calib_steer);
    CHECK("gt_force2/calib_steer", option_check_calib_steer);
}
