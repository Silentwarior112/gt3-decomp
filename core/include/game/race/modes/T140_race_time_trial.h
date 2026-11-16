/* T140_race_time_trial.h - RaceTimeTrial (translation unit T140, unit
 * u048), 0x00187F98..0x00188478.
 *
 * The time trial: a RaceSolitaire (one car) registered as "RaceTimeTrial"
 * (g_002B89D8; mode name "RACE_MODE_TIME_TRIAL").  Its members: the trial
 * index (the record table of ResultTimeTrial), seven car icons (T184), the
 * result sequence, a retry pause menu (race) and a replay pause menu, the
 * RaceTimeTrialDisplay (race display and disqualification sequence, T141)
 * and the DynamicsConductorTimeTrial that becomes RaceBase::dc.
 *
 * The dossier class "T184Object" (u047 / u048 / u049) is CarIcon
 * (T184_car_icon_maker.h): the same constructor 0x001ADA68 and 0x2044-byte
 * layout; it is not redeclared here.
 */
#ifndef GAME_T140_RACE_TIME_TRIAL_H
#define GAME_T140_RACE_TIME_TRIAL_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */
#include "game/race/modes/T139_race_solitaire.h"       /* base RaceSolitaire */
#include "game/car/model/T184_car_icon_maker.h"       /* CarIcon (held by value) */
#include "game/menu/records/T189_result_time_trial.h"    /* ResultTimeTrial (held by value) */
#include "game/race/pause/T171_race_pause_with_retry.h"    /* RacePauseWithRetry (held by value) */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (held by value) */
#include "game/race/display/T141_race_time_trial_display.h"  /* RaceTimeTrialDisplay (held by value) */
#include "game/race/dynamics/T314_dc_time_trial.h"        /* DynamicsConductorTimeTrial (held by value) */

class RaceBase;                             /* pointers only */
class type_info;

/* The typeinfo function, declared before the class (see T132): the
 * __si_type_info at 0x004B1FF0 (base __ti13RaceSolitaire 0x004B1FE0).
 * Not for egcs: an earlier declaration is the one it uses, and then it does
 * not generate the function (u048 matching). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf13RaceTimeTrial(void);     /* 0x00188428 */
}
#endif

/* vtable 0x0032B148 (60 slots: [0] [8] [24] [43] its own, the others
 * RaceSolitaire's).  typeinfo __tf13RaceTimeTrial 0x00188428 (si, base
 * RaceSolitaire).  vptr +0.  Size 0x48A80 (race_time_trial_create; the
 * deleting destructor). */
class RaceTimeTrial : public RaceSolitaire {
public:
    static void *operator new(size_t size);                 /* 0x00187F98 the race pool */
    static void  operator delete(void *p, size_t size);     /* 0x00187FC0 */
    /* The members (replayPause in mode 0), setRaceMode
     * ("RACE_MODE_TIME_TRIAL"), the display as the race display with the
     * pad input (input0), dc = &dcTimeTrial.  Also called by
     * run_time_trial (F_00130A50: a RaceTimeTrial built in place). */
    RaceTimeTrial(int index);                               /* 0x00188088 */
    virtual ~RaceTimeTrial();                               /* [ 0] +0x0C  0x00188268 */
    /* The result gets the trial index, then RaceSolitaire's; in a race the
     * retry pause menu and the result, in a replay the replay pause menu
     * (told the organization) and no result. */
    virtual void virtual_8(int replay);                     /* [ 8] +0x4C  0x00188318 */
    virtual const char *virtual_24();                       /* [24] +0xCC  0x00187FE8 "RaceTimeTrial" */
    /* The icons of the race's cars (car_icons_make; RacePS2Base's is not called). */
    virtual void virtual_43();                              /* [43] +0x164 0x001883D8 */

    int                        index;       /* 0x21800 the time trial (record table) */
    CarIcon                    icons[7];    /* 0x21804 (u048: T184Object) */
    ResultTimeTrial            result;      /* 0x2F9E0 */
    RacePauseWithRetry         retryPause;  /* 0x2FA04 */
    ReplayPause                replayPause; /* 0x2FCD4 (mode 0) */
    RaceTimeTrialDisplay       display;     /* 0x30214 */
    DynamicsConductorTimeTrial dcTimeTrial; /* 0x3A6C0 RaceBase::dc points here (never destroyed) */
    /*  0x48A6C .. 0x48A7F: alignment */
};
EGCS_SIZE(RaceTimeTrial, 0x48A80);

/* RaceClassInfo "RaceTimeTrial" (g_002B89D8: size 0x48A80, these two). */
RaceBase *race_time_trial_create(void);                     /* 0x00187FF8 new RaceTimeTrial(0) */
RaceBase *race_time_trial_construct(void *buf, int arg);    /* 0x00188060 a RaceTimeTrial(0) in buf (NULL: none) */
/* .ctors entry 0x0028F110: race_class_register(&g_00351EA0, &g_002B89D8). */
void      race_time_trial_static_init(void);                /* 0x00188400 */

/* T140's registry data (RaceClassInfo / RaceClassNode: T129). */
extern RaceClassInfo g_002B89D8;            /* 0x002B89D8 {"RaceTimeTrial", 0x48A80, race_time_trial_create, race_time_trial_construct} */
extern RaceClassNode g_00351EA0;            /* 0x00351EA0 gp-0x5B50 RaceTimeTrial's node in the registry */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The __si_type_info {vptr, name, base} __tf13RaceTimeTrial fills (u048
 * g_004B1FF0). */
extern "C" u32 __ti13RaceTimeTrial[3];      /* 0x004B1FF0 */
#endif

/* (The plan put more data of other translation units here; they are
 * declared with their owners now: g_00351FB8 and __ti11RaceDisplay in
 * T154, __ti20ExaminationDisplayer in T141, __ti9MiniPrize in T145,
 * __ti18RacePauseWithRetry in T171, __ti9PrizeBase in T460.) */

#endif /* GAME_T140_RACE_TIME_TRIAL_H */
