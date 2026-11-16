/* results_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u041,
 * 0x001753C0..0x00177108, second pass: the race result screens and the
 * random colours of the cars.
 *
 *   T118  ResultChampionship      (ResultArcade)            the championship points board
 *         (include/game/menu/records/T118_result_championship.h)
 *   T119  ResultImplementRecords  (RaceResultBase)          the place a result takes in a
 *         record table, "NEW RECORD" (include/game/menu/records/T119_result_implement_records.h)
 *   T120  ResultMachineTest       (ResultImplementRecords)  machine test: 0-400 m, 0-1000 m,
 *         max speed (include/game/menu/records/T120_result_machine_test.h)
 *   T189  ResultTimeTrial         (ResultImplementRecords)  time trial
 *         (include/game/menu/records/T189_result_time_trial.h)
 *   T190  ResultFreeRun           (ResultImplementRecords)  free run: the best lap
 *         (include/game/menu/records/T190_result_free_run.h)
 *   T191  ResultLinkBattle        (ResultArcade)            i.LINK battle
 *         (include/game/menu/records/T191_result_link_battle.h)
 *   T192  Result2PBattle          (ResultArcade)            split screen battle
 *         (include/game/menu/records/T192_result_2p_battle.h)
 *   T193  (no class)              the random colours of the cars of a race
 *         (include/game/menu/race_menu/T193_race_color_select.h)
 *
 * Every class is defined in its canonical header (include/game/).  Each
 * source file includes the canonical headers it needs itself, in the order
 * that gives retail's type names in its .rodata (the include set and order
 * decide them: ref/data/u041/notes.md; each file's comment lists retail's
 * names), then this header for the named constants only this unit uses.
 */
#ifndef U041_H
#define U041_H

#include "types.h"
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si (the GCC 14 __tf functions;
                                               egcs's <new> and cxxrt.h conflict) */
#endif

/* ======================================================================== */
/* Constants (only used here)                                               */
/* ======================================================================== */

/* One frame of the result timers: 1/60 s (each translation unit has its own
 * copies in .sdata, gp-0x7D54 .. gp-0x7D2C). */
static const f32 RESULT_FRAME_TIME = 0.016666668f;

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

/* The RACEDISPLAY texts the result screens print: g_002C43E8[RACEDISPLAY_*] (T186). */

/* Font::setAlign: G001 font_set_align's FONT_ALIGN_* (G001_flare.h). */

/* The colours of the record texts (0xAABBGGRR). */
enum {
    RESULT_TEXT_COLOR   = 0x801880D0,       /* orange */
    RESULT_SHADOW_COLOR = 0x80000000,       /* black */
    RESULT_TITLE_COLOR  = 0x80D0D0D0        /* light grey (the title's other blink phase) */
};

/* ResultBoard::setup's mode: RESULT_BOARD_* (T200). */

/* ResultChampionship::state (T118).  The jump table 0x00326180 sends 0 and
 * 2 to the same code. */
enum {
    CHAMPIONSHIP_RESULT_RACE           = 0, /* ResultArcade's race result; the pad / the timer closes it */
    CHAMPIONSHIP_RESULT_OPEN_POINTS    = 1, /* once closed: the board of the points of the places */
    CHAMPIONSHIP_RESULT_POINTS         = 2, /* shown; the pad / the timer closes it */
    CHAMPIONSHIP_RESULT_OPEN_STANDINGS = 3, /* once closed: the board of the standings */
    CHAMPIONSHIP_RESULT_STANDINGS      = 4  /* shown; the pad / 15 s ends the result */
};

/* ResultMachineTest::kind: the machine test record tables, MACHINE_TEST_*
 * (T415). */

#endif /* U041_H */
