/* T116_race_result_base.h - RaceResultBase, the end-of-race result
 * sequence of a race mode (translation unit T116, unit u040),
 * 0x00174B48..0x00174D50.
 *
 * The race mode keeps one (RaceBasic::subDisplay, T132: a ResultArcade,
 * ResultChampionship, ResultImplementRecords, ... held by value) and
 * drives it: start(org, replay) with the race, cleanup(), update(org, pad)
 * every frame, render(buf, org).  update takes the "goal" events (type 5,
 * the car's entry index as argument) of the organization's event queue:
 * goal(org, car) for each, and finish(org) once the player's car is among
 * them.
 *
 * Lupo 2's symbols name cleanup, render, goal, setCarNumber, getCarNumber
 * and isStartResultSequence; they are its linkonce (inline) functions, with
 * the constructor and the destructor.  T116 holds their out-of-line copies,
 * after the out-of-line methods (0x00174C90..0x00174D10): in-class inline
 * definitions here. */
#ifndef GAME_T116_RACE_RESULT_BASE_H
#define GAME_T116_RACE_RESULT_BASE_H

#include "game/game.h"

class RaceOrganization;                     /* T125 (references only: forward declarations) */
class Control;                              /* T474 */
class GSBuffer;                             /* T514 */

/* No base.  vtable 0x00325EE8 (8 slots).  typeinfo __tf14RaceResultBase
 * 0x00174D10 (__user_type_info 0x00353358, gp-0x4698).  Data, then the
 * vptr at +0x08 (egcs); size 0xC. */
class RaceResultBase {
public:
    /* Inline (the race modes and ResultChampionship / ResultMachineTest
     * expand it); 0x00174CF0 is T116's copy (no callers). */
    RaceResultBase() : done(0), player(0) {}
    virtual ~RaceResultBase() {}            /* [0] +0x0C 0x00174CC0 (inline: the derived
                                               destructors expand it) */
    /* done = 0.  `replay' (RaceBasic's): the race is a replay. */
    virtual void start(RaceOrganization &org, int replay);     /* [1] +0x14 0x00174B48 */
    virtual void cleanup() {}               /* [2] +0x1C 0x00174CB8 */
    /* For each pending goal event (counts[5] of the queue, read once): its
     * car, goal(org, car), and when it is the player's car (isPlayer) and
     * done is 0, finish(org).  `pad' is not used here. */
    virtual void update(RaceOrganization &org, Control *pad);  /* [3] +0x24 0x00174B50 */
    virtual void render(GSBuffer &buf, RaceOrganization &org) {}   /* [4] +0x2C 0x00174CB0 */
    void setCarNumber(int car) { player = car; }        /* 0x00174CA8 (no callers) */
    int  getCarNumber() { return player; }              /* 0x00174CA0 (no callers) */
    int  isStartResultSequence() { return done; }       /* 0x00174C98 (no callers) */
    virtual void goal(RaceOrganization &org, int car) {}   /* [5] +0x34 0x00174C90 */
    virtual void finish(RaceOrganization &org);          /* [6] +0x3C 0x00174C40 done = 1 */
    virtual int  isPlayer(int car);         /* [7] +0x44 0x00174C30 car == player */
    /* The player's place in the rank order list of the entries (0 = first),
     * -1 if the player's car is not in it. */
    int  getPlayerRank(RaceOrganization &org);          /* 0x00174C50 */

    /* done: the player's car reached the goal, the result sequence started
     * (finish; Lupo 2: isStartResultSequence).  player: the player's car,
     * its entry index (Lupo 2: the car number). */
    int done;                               /* 0x0 */
    int player;                             /* 0x4 */
    /*  vptr                                   0x8 */
};
EGCS_SIZE(RaceResultBase, 0xC);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tf14RaceResultBase(void);           /* 0x00174D10 (__rtti_user) */
extern u32 __ti14RaceResultBase[2];         /* 0x00353358 gp-0x4698 (__user_type_info; u041:
                                               g_00353358) */
}
#endif

#endif /* GAME_T116_RACE_RESULT_BASE_H */
