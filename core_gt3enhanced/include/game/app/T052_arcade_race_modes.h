/* T052_arcade_race_modes.h - the arcade race sessions and ShortMenuRankEntry
 * (translation unit T052, unit u016), 0x0012F840..0x00131410.
 *
 * Every session selects the race (select_race, T102), puts the menu's car
 * (an arcade car, or a garage car with its condition) into a race
 * information, builds the RaceOrganization and runs the race; then a loop
 * runs the short menu (run_short_menu, T102) over it and acts on the QM_*
 * code it returns, until QM_EXIT.  Stack layout (all three): ShortMenuArgs
 * sp+0x000, the information sp+0x190, a CarEquipments temporary /
 * ShortMenuParams sp+0x3F0, the race sp+0x5E0.
 *
 * Also here: 0x001313F0, T052's unreferenced copy of ShortMenuResultEntry's
 * inline constructor (T102's is 0x0012F820). */
#ifndef GAME_T052_ARCADE_RACE_MODES_H
#define GAME_T052_ARCADE_RACE_MODES_H

#include "game/game.h"

#ifdef __cplusplus                          /* a C++ translation unit */

#include "game/race/core/T128_race_lap_record.h"      /* RACE_TIME_INVALID */

struct MenuArgs;                           /* T000: the results of a menu (pointers only) */

/* (? name, u015) A line of the ranking of a short menu (the qm_rank_*
 * texts, T113): ShortMenuArgs::rank (T102).  Not polymorphic; 0x10 bytes.
 * The inline constructor calls the inline clear(), defined after it (the
 * shape of ShortMenuResultEntry, T102): egcs compiles the in-class bodies
 * in order, so the constructor keeps a real call to clear (the race modes'
 * array loops of ShortMenuArgs call 0x001313D0) and each unit that uses it
 * emits a copy of clear (7 instructions, no `this' returned: not a
 * constructor).  0x001313D0 is T052's copy, the first one linked;
 * unreferenced copies 0x00133DC8 T053, 0x001345F0 T054.  (u017 matching
 * pass, after u015's note.) */
struct ShortMenuRankEntry {
    ShortMenuRankEntry() { clear(); }
    void clear()                            /* (? name) 0x001313D0 the empty line */
    {
        time = RACE_TIME_INVALID;           /* in field order (the copies' stores are */
        name = NULL;                        /* scheduled +0xC, +0x4, +0x0, +0x8) */
        highlight = 0;
        speed = 0.0f;
    }

    int         time;                       /* 0x00 ms */
    const char *name;                       /* 0x04 NULL: the default name, "Polyphony Digital" */
    int         highlight;                  /* 0x08 non-zero: the player's line */
    f32         speed;                      /* 0x0C km/h (the speed rankings) */
};
EGCS_SIZE(ShortMenuRankEntry, 0x10);

/* Arcade single race (the arcade menu's state 2): RaceArcade with the
 * result menu "menu/qm_arc_result.mbl"; the replay after each race, the
 * trophy screen after a win, the prize-car check at the end.  Caller: the
 * arcade menu loop main_arcade (T000).  Returns 0. */
int run_arcade_race(MenuArgs *args);        /* 0x0012F840 */
/* Free run (arcade state 4, MenuArgs::selection 3 "time attack": RaceFreeRun
 * is "RACE_MODE_TIME_ATTACK"); main_free_run (T000) runs it from GT mode
 * with gtMode != 0 (the transmission from GameOptions::gearAuto).  The
 * free run records of the course (FreeRunRecords).  Returns 0. */
int run_free_run(MenuArgs *args, int gtMode);   /* 0x0012FF48 */
/* Time trial (arcade state 3, MenuArgs::selection 2 "compulsory": the
 * records of the test, RecordsMenuArgCompulsory / TimeTrialRecords); always
 * an arcade car.  Returns 0. */
int run_time_trial(MenuArgs *args);         /* 0x00130A50 */

#endif /* __cplusplus */

#endif /* GAME_T052_ARCADE_RACE_MODES_H */
