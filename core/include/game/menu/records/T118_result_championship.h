/* T118_result_championship.h - ResultChampionship, the result board of a
 * championship race (translation unit T118, unit u041),
 * 0x001753C0..0x001759C8.
 *
 * After ResultArcade's race result (state 0) the board shows the points
 * every place earns (state 1 -> 2), then the standings: the points so far
 * plus the points of this race, sorted (state 3 -> 4); then it waits for
 * the pad's bit 0x40 or 15 s and posts event 1 (leave the result screen).
 * Held by value by RaceChampionship (+0x2509C, T151). */
#ifndef GAME_T118_RESULT_CHAMPIONSHIP_H
#define GAME_T118_RESULT_CHAMPIONSHIP_H

#include "game/game.h"
#include "game/menu/records/T117_result_arcade.h"        /* base ResultArcade */

class type_info;
class RaceOrganization;                     /* T125 (pointers / references only) */
class RaceEntryCar;                         /* T121 */
class Control;                              /* T474 */

/* The typeinfo function (vtable entry 0).  egcs synthesizes it itself
 * once the class is complete, unless an earlier declaration exists (then
 * the vtable refers to that one and the function is not emitted), so
 * only the GCC 14 build declares it.  It builds the __si_type_info at
 * 0x004B1E00 (name "18ResultChampionship" 0x003261F0, base
 * __ti12ResultArcade 0x004B1DF0). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf18ResultChampionship(void);    /* 0x00175920 */
}
#endif

/* (? name, u041.)  A car of the standings: a node of the list `head',
 * sorted by points.  0xC bytes. */
struct ChampionshipStanding {
    ChampionshipStanding *next;             /* 0x0 */
    int           points;                   /* 0x4 the points so far + the points of this race */
    RaceEntryCar *car;                      /* 0x8 */
};
EGCS_SIZE(ChampionshipStanding, 0xC);

/* vtable 0x00326198 (10 slots).  Size 0x9A8 (RaceChampionship: the next
 * member is at +0x25A44).  The destructor is implicit. */
class ResultChampionship : public ResultArcade {
public:
    /* Inline (Lupo 2: linkonce); 0x001758B0 is T118's copy (no callers). */
    ResultChampionship() : head(0), state(0) {}
    /* [0] +0x0C 0x00175970 ~ResultChampionship: implicit (the copy for the
     * vtable follows the typeinfo function). */
    /* ResultArcade's start, then state 0 and a 10 s timer. */
    virtual void start(RaceOrganization &org, int replay);     /* [1] +0x14 0x001753C0 */
    /*           [2] +0x1C ResultArcade::cleanup */
    /* RaceResultBase's update (not ResultArcade's); once active and the
     * board is in place, the state machine (jump table 0x00326180, cases
     * 0..4), then board.update(). */
    virtual void update(RaceOrganization &org, Control *pad);  /* [3] +0x24 0x001753F8 */
    /*           [4] +0x2C ResultArcade::render, [5] +0x34 ResultArcade::goal,
     *           [6] +0x3C ResultArcade::finish, [7] +0x44 RaceResultBase::isPlayer,
     *           [8] +0x4C ResultArcade::waitEnd, [9] +0x54 ResultArcade::setRowResult */
    int  countDown();                       /* 0x001756C0 timer -= 1/60, 1 once below 0 */
    void listRaceResult(RaceOrganization &org); /* 0x001756F0 the board's rows: the cars in
                                                   finishing (rank list) order */
    /* For the car of each place: points = its points so far (RaceEntryCar
     * +0x20FC) + the points of the place (RaceDataBase, T484), inserted into
     * `head' in descending order (after the cars with as many points). */
    void sortStandings(RaceOrganization &org);  /* 0x00175790 */

    /* state: 0 the race result, 1 -> 2 the points of the places, 3 -> 4 the
     * standings. */
    ChampionshipStanding  standings[7];     /* 0x94C */
    ChampionshipStanding *head;             /* 0x9A0 the first of the sorted standings */
    u32                   state;            /* 0x9A4 */
};
EGCS_SIZE(ResultChampionship, 0x9A8);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti18ResultChampionship[3];     /* 0x004B1E00 (__si_type_info; u041: g_004B1E00) */
}
#endif

#endif /* GAME_T118_RESULT_CHAMPIONSHIP_H */
