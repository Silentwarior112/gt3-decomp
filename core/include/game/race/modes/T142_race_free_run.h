/* T142_race_free_run.h - RaceFreeRun (the free run / time attack,
 * "RACE_MODE_TIME_ATTACK") and RaceFreeRunInformation (translation unit
 * T142, unit u048), 0x00188FE0..0x00189998.
 *
 * RaceFreeRun is a RaceSolitaire registered in the race class registry as
 * "RaceFreeRun" (RaceClassInfo 0x002B8F90 {"RaceFreeRun", 0x3A880,
 * race_free_run_create, race_free_run_construct}, node 0x00351EC0 gp-0x5B30;
 * race_free_run_static_init, .ctors 0x0028F114).  Its members: the
 * ResultFreeRun (the course's lap records, keyed by the course name), a
 * RacePause menu (a race), a ReplayPause menu (a replay), the
 * RaceFreeRunDisplay (T143) and the DynamicsConductorFreeRun (RaceBase::dc).
 * Inputs 1..3 record their controls (record mode 2), so the second car can
 * replay a ghost (virtual_20 / 23 / 59).  The pause menu's exit (result 4)
 * ends the session through the goal and the result sequence once the first
 * car is past its first lap (onExit); otherwise the race ends at once.
 *
 * The virtual methods keep the slot names of the bases (RaceBase T129,
 * RacePS2Base T130, RaceSolitaire T139). */
#ifndef GAME_T142_RACE_FREE_RUN_H
#define GAME_T142_RACE_FREE_RUN_H

#include "game/game.h"
#include <stddef.h>                             /* size_t */
#include "game/race/modes/T139_race_solitaire.h"           /* bases RaceSolitaire, RaceSolitaireInformation */
#include "game/menu/records/T190_result_free_run.h"          /* ResultFreeRun (held by value) */
#include "game/race/pause/T169_race_pause.h"               /* RacePause (held by value) */
#include "game/race/pause/T170_replay_pause.h"             /* ReplayPause (held by value) */
#include "game/race/display/T143_race_free_run_display.h"    /* RaceFreeRunDisplay (held by value) */
#include "game/race/dynamics/T313_dc_free_run.h"              /* DynamicsConductorFreeRun (held by value) */

class RaceBase;                                 /* T129 */
struct RaceInfoBlock;                           /* T129 (pointers only) */
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
/* (__si_type_info 0x004B2020, base RaceSolitaire 0x004B1FE0) */
const type_info &__tf11RaceFreeRun(void);               /* 0x001898F8 */
/* (__si_type_info 0x004B2030, base RaceSolitaireInformation 0x004B1FC0) */
const type_info &__tf22RaceFreeRunInformation(void);    /* 0x00189948 */
}
#endif

/* vtable 0x0032BAE0 (15 slots; 0 and 8 its own), typeinfo
 * __tf22RaceFreeRunInformation 0x00189948.  The race information of a free
 * run (run_free_run, F_0012FF48, builds it).  No data of its own: vptr +0x54
 * (RaceInformation's); 0x258 bytes. */
class RaceFreeRunInformation : public RaceSolitaireInformation {
public:
    RaceFreeRunInformation();                   /* 0x00188FE0 */
    virtual ~RaceFreeRunInformation();          /* [0] +0x0C 0x00189018 */
    virtual int getEntryCount();                /* [8] +0x4C 0x00189040 2: the car and its ghost */
};
EGCS_SIZE(RaceFreeRunInformation, 0x258);

/* vtable 0x0032B8F8 (60 slots; 0, 8, 20, 23, 24, 30, 56 and 59 its own),
 * typeinfo __tf11RaceFreeRun 0x001898F8.  vptr +0x00 (GameObject's);
 * 0x3A880 bytes (the race classes are 64-byte aligned: T132). */
class RaceFreeRun : public RaceSolitaire {
public:
    /* T142's copies of RaceBase's race-pool allocation (0x0017BA18 /
     * 0x0017BA40): g_002B3BF0, 16-byte aligned; the size is ignored. */
    static void *operator new(size_t size);     /* 0x00189048 */
    static void operator delete(void *p, size_t size);  /* 0x00189070 */

    /* course: the ResultFreeRun's record key (NULL: "").  Also called
     * directly by run_free_run (F_0012FF48). */
    RaceFreeRun(const char *course);            /* 0x00189138 */
    /* (deleting form: operator delete(this, 0x3A880)) */
    virtual ~RaceFreeRun();                     /* [ 0] +0x00C 0x00189318 */
    /* Race start (exited = 0): a race gets the pause menu and the result
     * sequence; a replay the replay pause menu and, when inputs 2 / 3 are
     * used outside the logger, virtual_56 and no start action. */
    virtual void virtual_8(int replay);         /* [ 8] +0x04C 0x001893D8 */
    /* The second input loads the controls record that follows blk
     * (+0x191C) and drives. */
    virtual void virtual_20(RaceInfoBlock *blk);    /* [20] +0x0AC 0x001895C0 */
    /* input2 takes the pad's record and drives (RaceSolitaire's is not
     * called). */
    virtual void virtual_23();                  /* [23] +0x0C4 0x00189538 */
    virtual const char *virtual_24();           /* [24] +0x0CC 0x00189098 "RaceFreeRun" */
    /* The update while paused (RaceBasic's with another case 4): the menu's
     * result 2 (timeout) ends the race (virtual_34(3)), 4 (exit) onExit,
     * then resumes, 3 (continue) resumes, 5 (retry) virtual_34(4). */
    virtual void virtual_30();                  /* [30] +0x0FC 0x001896B8 */
    /* RaceSolitaire's, then starts car 0 and, if the second input drives,
     * car 1. */
    virtual void virtual_56();                  /* [56] +0x1CC 0x001894E8 */
    /* virtual_57, then which == 0: input2 replays blk's record (inputs 2 /
     * 3 from now on); else virtual_19(blk) and virtual_20(blk). */
    virtual void virtual_59(int which, RaceInfoBlock *blk); /* [59] +0x1E4 0x00189608 */

    /* The pause menu's exit (result 4; u048 onExit).  Once per race, outside replays, when the first car shows lap 2
     * or later (car_lap_display): the goal event 5 (the result sequence
     * shows the lap record), car_finish(CAR_FINISH_AUTO), the main input's
     * replay record and lap record stopped, g_00351FB8 = 3; the race goes
     * on.  Otherwise virtual_34(2): the race ends. */
    void onExit();                              /* 0x001897D8 */

    ResultFreeRun            result;            /* 0x21800 */
    RacePause                pause;             /* 0x21860 the pause menu of a race */
    ReplayPause              replayPause;       /* 0x21B0C (mode 0) */
    RaceFreeRunDisplay       display;           /* 0x2204C RaceBasic's race display */
    DynamicsConductorFreeRun dcFreeRun;         /* 0x2C4C8 RaceBase::dc */
    int                      exited;            /* 0x3A874 onExit ended the session (u048 exited) */
    /*  0x3A878 .. 0x3A87F: alignment */
};
EGCS_SIZE(RaceFreeRun, 0x3A880);

/* RaceClassInfo::create of "RaceFreeRun": new RaceFreeRun(NULL). */
RaceBase *race_free_run_create(void);                       /* 0x001890A8 */
/* RaceClassInfo::construct: a RaceFreeRun(NULL) in buf (NULL if buf is
 * NULL); arg is not used. */
RaceBase *race_free_run_construct(void *buf, int arg);      /* 0x00189110 */
/* .ctors entry 0x0028F114: registers "RaceFreeRun". */
void      race_free_run_static_init(void);                  /* 0x001898D0 */

/* T142's registry data (RaceClassInfo / RaceClassNode: T129). */
extern RaceClassInfo g_002B8F90;                /* 0x002B8F90 {"RaceFreeRun", 0x3A880, race_free_run_create, race_free_run_construct} */
extern RaceClassNode g_00351EC0;                /* 0x00351EC0 gp-0x5B30 RaceFreeRun's node in the registry */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
/* The __si_type_info {vptr, name, base} the two __tf functions fill (u048
 * g_004B2020 / g_004B2030). */
extern "C" u32 __ti11RaceFreeRun[3];            /* 0x004B2020 */
extern "C" u32 __ti22RaceFreeRunInformation[3]; /* 0x004B2030 */
#endif

#endif /* GAME_T142_RACE_FREE_RUN_H */
