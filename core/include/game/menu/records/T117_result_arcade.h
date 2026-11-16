/* T117_result_arcade.h - ResultArcade, the result sequence of the arcade
 * race modes (translation unit T117, unit u040), 0x00174D50..0x001753C0.
 *
 * A ResultBoard (T200) lists the cars in rank order; a car's row gets its
 * time (or, in a time-limit race, its laps) when it reaches the goal.  Once
 * every car is in, a 15 s timer runs (1/60 s a frame); when it expires or
 * the pad's skip bit (0x40 CONTROL_DECIDE, Control::getButtonsOn: slot 5) is
 * pressed, event 1 (the end of the result sequence) is posted to the
 * organization's queue.
 *
 * Held by value by RaceArcade (+0x2509C, T134); base of ResultChampionship
 * (T118), ResultLinkBattle (T191) and Result2PBattle (T192).  The
 * constructor and the destructor are implicit (egcs synthesizes them where
 * used; the destructor's copy for the vtable follows the typeinfo
 * function). */
#ifndef GAME_T117_RESULT_ARCADE_H
#define GAME_T117_RESULT_ARCADE_H

#include "game/game.h"
#include "game/menu/records/T116_race_result_base.h"     /* base RaceResultBase */
#include "game/menu/records/T200_result_board.h"         /* ResultBoard (held by value) */

class RaceOrganization;                     /* T125 (pointers / references only) */
class RaceEntryCar;                         /* T121 */
class Control;                              /* T474 */
class GSBuffer;                             /* T514 */
struct CarIcon;                             /* T184 */

/* vtable 0x00325F88 (10 slots).  typeinfo __tf12ResultArcade 0x00175318
 * (si, base RaceResultBase; storage 0x004B1DF0).  Size 0x94C (RaceArcade:
 * the next member is at +0x259E8). */
class ResultArcade : public RaceResultBase {
public:
    /* [0] +0x0C 0x00175368 ~ResultArcade: implicit (~board, flags 2; the
     * base's destructor inlined). */
    /* The base's start, the board for the entries' count (setup(n, 0, 5.0)),
     * active = !replay, lapsOnly = the race has a time limit (minutes *
     * 60000 != 0), no car finished, timer 15 s. */
    virtual void start(RaceOrganization &org, int replay);     /* [1] +0x14 0x00174D50 */
    virtual void cleanup();                 /* [2] +0x1C 0x00174E18 board.stop() when active,
                                               board.clearRows() */
    /* The base's update (the goal events); once the player's car is in:
     * updateBoard, board.update(), waitEnd(org, the pad's skip bit). */
    virtual void update(RaceOrganization &org, Control *pad);  /* [3] +0x24 0x00174E58 */
    virtual void render(GSBuffer &buf, RaceOrganization &org); /* [4] +0x2C 0x001750A0 board.draw
                                                                   when active */
    /* Car `car' reached the goal: its row in the (rebuilt) rank order is
     * filled for good (name, icon, setRowResult, the player's mark) and
     * allFinished set when no entry is left running. */
    virtual void goal(RaceOrganization &org, int car);          /* [5] +0x34 0x001750C8 */
    virtual void finish(RaceOrganization &org);                 /* [6] +0x3C 0x00175258 the base's
                                                                   finish, board.start(), when active */
    /*           [7] +0x44 RaceResultBase::isPlayer */
    /* Once the board is in place: with every car in the timer runs down
     * 1/60 s; then on `skip' or an expired timer event 1 is posted. */
    virtual void waitEnd(RaceOrganization &org, int skip);      /* [8] +0x4C 0x00174EF8 */
    /* The race time and the laps of the car (laps only in a time-limit
     * race). */
    virtual void setRowResult(int row, RaceEntryCar *car);      /* [9] +0x54 0x00175298 */
    /* The cars' icons of the race mode (e.g. RaceArcade +0x16EC0) for the
     * board (board.displays).  Callers: the virtual_43 of RaceArcadeSingle,
     * RaceSplitBattle, RaceLinkBattle, RaceChampionship. */
    void setDisplays(CarIcon *displays);    /* 0x00174E50 */
    /* The rows of the cars still running, in the current rank order (the
     * list rebuilt first); nothing before the player's car is in or once
     * every car is. */
    void updateBoard(RaceOrganization &org);    /* 0x00174F90 */

    ResultBoard board;                      /* 0x00C */
    int         active;                     /* 0x920 the board is used: start's replay was 0 (u041: visible) */
    /* The two flags are bools (4 bytes under egcs, an alias set of their
     * own): goal's store of finished[number] does not hold back the load of
     * the entries' count, which retail schedules after the constant 1 (u040
     * notes; ints: 99.0%).  ResultChampionship (u041) is exact either way. */
    bool        finished[7];                /* 0x924 entry i reached the goal (is on the board) */
    bool        allFinished;                /* 0x940 every entry reached the goal */
    f32         timer;                      /* 0x944 seconds left before the end (15.0) */
    int         lapsOnly;                   /* 0x948 a time-limit race: the rows show laps */
};
EGCS_SIZE(ResultArcade, 0x94C);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tf12ResultArcade(void);             /* 0x00175318 */
extern u32 __ti12ResultArcade[3];           /* 0x004B1DF0 (__si_type_info; u041: g_004B1DF0) */
}
#endif

#endif /* GAME_T117_RESULT_ARCADE_H */
