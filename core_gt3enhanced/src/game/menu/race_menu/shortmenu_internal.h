/* shortmenu_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u015,
 * 0x0012C300..0x0012F840, second pass.
 *
 * Translation units (address order; the T numbers follow the FDE table):
 *   T111  gt2k_menu::ShortmenuControl, the controller of the "short menu"
 *         (the quick menu of the race modes: pause / result / retry), a
 *         page file run by a PageManager over a menu heap carved out of the
 *         caller's stack (include/game/menu/race_menu/T111_shortmenu_control.h).
 *   T112  gt2k_menu::ShortmenuRender, its renderer: it keeps the race behind
 *         the menu running (RaceBase::update) and drawn (RacePS2Base
 *         virtual_44) (T112_shortmenu_render.h).
 *   T113  the page actions "qm_*" (ExecuteFuncData: each stores a QM_* code
 *         in the session's ShortMenuArgs), the layout hooks "qm_*" (MenuHook:
 *         texts with the race results), gt2k_menu::ShortMenuText and the
 *         static initialiser of both tables (T113_shortmenu_items.h).
 *   T102  run_short_menu / run_short_menu_license and the race-result
 *         helpers of the race modes (T052 ..): grid and result orders, the
 *         result lines of ShortMenuArgs, licence and event results, the car
 *         condition moved between the garage car and the race, the memory
 *         card screens, the ranking code, the demo setting loader
 *         (T102_race_menu_helpers.h).
 *
 * How a session runs (run_short_menu): ShortMenuArgs.result = QM_EXIT and
 * g_003514F0 = &args; a ShortmenuControl on the stack loads params->file and
 * gives its PageManager three tables: the availability checks of page
 * "root" (g_0029C2C8: 18 x shortmenu_root_check, which hides "replay",
 * "save_ghost", ... from the flags of the args), the 21 qm_* actions
 * (g_0029C6B0) and, in both layouts, the 45 qm_* hooks (g_0029C758); a
 * 0x40040-byte alloca, 64-byte aligned, becomes the menu heap; the page
 * params->page runs until the page manager has exited; the result is the
 * code an action stored (QM_EXIT if none).
 *
 * Every shared class is defined in a canonical header (include/game/);
 * each source includes the headers it needs itself, in the order that gives
 * retail's type names (data pass, 2026-10-17: egcs writes the type name of
 * every class a file sees whose virtual functions are all inline, in
 * reverse definition order; each source's comment lists retail's).  This
 * header brings no class: it has what only this unit uses, the session
 * pointer g_003514F0, the check table g_00314848, the qm_* functions (used
 * only through T113's tables) and a few named constants.
 */
#ifndef U015_H
#define U015_H

#include "types.h"
#include "game/menu/race_menu/T102_race_menu_helpers.h"    /* ShortMenuArgs (g_003514F0; no type names) */

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
class Object;                               /* T220 */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
}

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */

/* The menu heap of a session: an alloca of this size, aligned to 64 bytes
 * (the rest after the alignment is the heap). */
#define SHORT_MENU_HEAP_ALLOC   0x40040

/* The text colour of a highlighted result line (Item::textColor, RGBA8). */
#define SHORT_MENU_HIGHLIGHT    0x800080FF

/* The unit tag ShowRoomCar::power takes ('ps', not read by it). */
#define UNIT_TAG_PS             0x7073

/* The course kind of a dirt course (RaceDataBase::getCourseKind). */
#define COURSE_KIND_DIRT        2

/* The tyre kinds (CarEquipments::getFronttireKind, T482): 13..24
 * (TIRE_KIND_SECOND_SET..) are the second set of 0..11, folded back below 13
 * for the ranking code; TIRE_KIND_DIRT the dirt tyre. */

/* make_ranking_code's kind: RankingKind (T431). */

/* The demo files ("demo/<name>"): at most this many bytes. */
#define DEMO_FILE_SIZE          0x41940

/* The entries of the SHORTMENU texts g_002B1B30 (T114; the keys
 * SESSION_BRFORE (sic) .. POINT_PLURAL). */
enum {
    SHORTMENU_TEXT_SESSION_BEFORE = 0,      /* EUC-JP "No." before the session number */
    SHORTMENU_TEXT_SESSION_AFTER  = 1,      /* EUC-JP "race" after it */
    SHORTMENU_TEXT_LAPS_SINGLE    = 2,      /* "Lap" */
    SHORTMENU_TEXT_LAPS_PLURAL    = 3,      /* "Laps" */
    SHORTMENU_TEXT_POINT_SINGLE   = 4,      /* "pt" */
    SHORTMENU_TEXT_POINT_PLURAL   = 5       /* "pts" */
};

/* ======================================================================== */
/* Data only this unit uses                                                 */
/* ======================================================================== */

/* gp-0x6500: the ShortMenuArgs of the running session (set by
 * run_short_menu / run_short_menu_license, NULL otherwise); the qm_*
 * actions and hooks and shortmenu_root_check read it. */
extern ShortMenuArgs *g_003514F0;           /* 0x003514F0 */

/* T111 .data: the 18 availability checks of page "root" (all
 * shortmenu_root_check), g_0029C2C8[0].checks. */
typedef int ShortMenuButtonCheck(void *ctx, gt2k_menu::ButtonData *data);
extern ShortMenuButtonCheck *const g_00314848[18];  /* 0x00314848 (T111 .rodata) */

/* ======================================================================== */
/* T113: the qm_* functions (used only through T113's tables)               */
/* ======================================================================== */

/* The actions (ExecuteFuncData, T211): ctx is the ShortmenuControl; none
 * reads its arguments. */
#define QM_ACTION_ARGS void *ctx, gt2k_menu::ButtonData *rec, int index, gt2k_menu::PageLayout *layout
void qm_replay(QM_ACTION_ARGS);             /* 0x0012CB70 */
void qm_exit(QM_ACTION_ARGS);               /* 0x0012CB80 */
void qm_try_again(QM_ACTION_ARGS);          /* 0x0012CB90 */
void qm_setting(QM_ACTION_ARGS);            /* 0x0012CBA0 */
void qm_go_race(QM_ACTION_ARGS);            /* 0x0012CBB0 (also "qm_start") */
void qm_free_run(QM_ACTION_ARGS);           /* 0x0012CBC0 */
void qm_option(QM_ACTION_ARGS);             /* 0x0012CBD0 */
void qm_save_replay(QM_ACTION_ARGS);        /* 0x0012CBE0 */
void qm_save_ghost(QM_ACTION_ARGS);         /* 0x0012CBF0 */
void qm_load_ghost(QM_ACTION_ARGS);         /* 0x0012CC00 */
void qm_logger(QM_ACTION_ARGS);             /* 0x0012CC10 */
void qm_load_setting(QM_ACTION_ARGS);       /* 0x0012CC20 */
void qm_save_setting(QM_ACTION_ARGS);       /* 0x0012CC30 */
void qm_save_game(QM_ACTION_ARGS);          /* 0x0012CC40 */
void qm_next_session(QM_ACTION_ARGS);       /* 0x0012CC50 */
void qm_records(QM_ACTION_ARGS);            /* 0x0012CC60 */
void qm_yes(QM_ACTION_ARGS);                /* 0x0012CC70 */
void qm_no(QM_ACTION_ARGS);                 /* 0x0012CC80 */
void qm_demo(QM_ACTION_ARGS);               /* 0x0012CC90 */
void qm_dummy(QM_ACTION_ARGS);              /* 0x0012CCA0 */

/* The hooks (MenuHookFn, T229): ctx is the ShortmenuControl; p8 / pC are
 * the table's words (the line, the licence target, the place), p10 / p14
 * are 0. */
#define QM_HOOK_ARGS void *ctx, gt2k_menu::PageLayout *layout, gt2k_menu::Object *parent, \
                     gt2k_menu::MenuLayoutItem *item, int p8, int pC, int p10, int p14
gt2k_menu::Transitive *qm_best_time(QM_HOOK_ARGS);          /* 0x0012CCA8 */
gt2k_menu::Transitive *qm_rank_speed(QM_HOOK_ARGS);         /* 0x0012CD60 */
gt2k_menu::Transitive *qm_rank_time(QM_HOOK_ARGS);          /* 0x0012CE68 */
gt2k_menu::Transitive *qm_rank_name(QM_HOOK_ARGS);          /* 0x0012CF50 */
gt2k_menu::Transitive *qm_list_car_name(QM_HOOK_ARGS);      /* 0x0012D040 */
gt2k_menu::Transitive *qm_mt_1000_time_rank(QM_HOOK_ARGS);  /* 0x0012D158 */
gt2k_menu::Transitive *qm_list_time(QM_HOOK_ARGS);          /* 0x0012D240 */
gt2k_menu::Transitive *qm_pt(QM_HOOK_ARGS);                 /* 0x0012D328 */
gt2k_menu::Transitive *qm_laps(QM_HOOK_ARGS);               /* 0x0012D450 */
gt2k_menu::Transitive *qm_license(QM_HOOK_ARGS);            /* 0x0012D540 */
gt2k_menu::Transitive *qm_session_num(QM_HOOK_ARGS);        /* 0x0012D720 */
void F_0012D8B8(void);                      /* 0x0012D8B8 empty, no references */

#endif /* U015_H */
