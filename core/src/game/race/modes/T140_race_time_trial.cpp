/* T140 - RaceTimeTrial (u048), 0x00187F98..0x00188478.
 *
 * The time trial: a RaceSolitaire (one car) registered as "RaceTimeTrial"
 * (mode name "RACE_MODE_TIME_TRIAL").  Its members: the trial index (the
 * record table of ResultTimeTrial), seven car icons (T184), the result
 * sequence, a retry pause menu (race) and a replay pause menu, the
 * RaceTimeTrialDisplay (race display + disqualification sequence) and the
 * DynamicsConductorTimeTrial that becomes RaceBase::dc.
 *
 * Includes (u048's data pass): the classes retail's T140 saw, in its order.
 * Its .rodata (0x0032B110..0x0032B5CE) is "bad_alloc" (<new>), the two
 * literals, the vtable, then the type names in reverse definition order:
 * RaceTimeTrial, ExaminationDisplayer, CallBackWhenPrizeRendering,
 * PrizeModelSetContext, ResultTermBar, ResultTitleBar, ReplayPauseMenu,
 * PauseMenuWithRetry, RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton,
 * the OperatorHolder group, RaceMiniMap .. RaceMeterBase, RaceCameraBase,
 * RaceValueDisplayBase, RaceDisplayBase, UnitArena<64>, Operator: T153 is
 * read first (UnitArena<64> before the race display, unlike T141 / T142),
 * then the race display, the menu classes, RaceMenuButton, the windows, the
 * pause menus, the result bars, the prize classes, ExaminationDisplayer.
 * Its .sdata (0x00351E98..0x00351EA8) is the "" of T157's inline
 * RaceMessageDisplay::clear (read with the race display), then the registry
 * node g_00351EA0 (an uninitialised global: zeros at the end).
 */
// DATA_START 0x0032B110
// DATA_START 0x00351E98
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (race_time_trial_construct); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"          /* UnitArena<64> (first: retail's type-name order) */
#include "game/race/display/T154_race_display.h"         /* the race display classes */
#include "game/menu/framework/T216_menu_transitive.h"      /* no use: the OperatorHolder classes retail's T140 saw */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* RaceMenuButton (before the windows: retail's order) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* RaceMenuWindow, RaceMenuSelectWindow */
#include "game/race/pause/T171_race_pause_with_retry.h"    /* RacePauseWithRetry (PauseMenuWithRetry) */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (ReplayPauseMenu) */
#include "game/menu/records/T200_result_board.h"         /* no use: ResultTitleBar, ResultTermBar (seen in retail) */
#include "game/menu/gtmode/T460_prize_base.h"           /* no use: PrizeModelSetContext, CallBackWhenPrizeRendering */
#include "game/race/display/T141_race_time_trial_display.h"  /* RaceTimeTrialDisplay (ExaminationDisplayer) */
#include "game/race/modes/T140_race_time_trial.h"      /* own: RaceTimeTrial, the registry externs;
                                               + T139 RaceSolitaire, T129 race_class_register */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/car/model/T184_car_icon_maker.h"       /* car_icons_make */
#include "game/race/modes/race_time_trial_internal.h"

// FUNC 0x00187F98 RaceTimeTrial::operator new
//   The race pool, 16-byte aligned (an out-of-line copy of RaceBase's
//   inline operator new emitted in T140).
void *RaceTimeTrial::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00187FC0 RaceTimeTrial::operator delete
//   Back to the race pool (the size is ignored).  Called by
//   race_time_trial_create when the constructor throws and by the deleting
//   destructor.
void RaceTimeTrial::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00187FE8 RaceTimeTrial::virtual_24
//   The class name.
const char *RaceTimeTrial::virtual_24()
{
    return "RaceTimeTrial";
}

// FUNC 0x00187FF8 race_time_trial_create
//   RaceClassInfo::create of "RaceTimeTrial": new RaceTimeTrial(0) (operator
//   new 0x48A80; operator delete if the constructor throws).
RaceBase *race_time_trial_create(void)
{
    return new RaceTimeTrial(0); /* implicit: RaceTimeTrial::operator delete (if the constructor throws) */
}

// FUNC 0x00188060 race_time_trial_construct
//   RaceClassInfo::construct: a RaceTimeTrial(0) in buf (nothing if buf is
//   NULL).  arg is not used.
RaceBase *race_time_trial_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)                        /* egcs tests the result of <new>'s throw() placement
                                               new itself (retail's daddu v0,a0 / beqz v0); GCC 14
                                               does not */
        return NULL;
#endif
    return ::new (buf) RaceTimeTrial(0);
}

/* The registry record of "RaceTimeTrial" (.data, before the frame info; its
 * "RaceTimeTrial" is virtual_24's literal, written once). */
RaceClassInfo g_002B89D8 = {                /* 0x002B89D8 */
    "RaceTimeTrial", sizeof(RaceTimeTrial), race_time_trial_create, race_time_trial_construct
};

// FUNC 0x00188088 RaceTimeTrial::RaceTimeTrial
//   RaceSolitaire, then the members in order: the icons (7 x CarIcon
//   0x001ADA68), the ResultTimeTrial (inline: done = player = 0), the retry
//   pause (RacePauseWithRetry 0x001A3A08), the replay pause (ReplayPause
//   0x001A30C0, mode 0), the display (RaceTimeTrialDisplay 0x00188828) and
//   the dynamics conductor (DynamicsConductorTimeTrial 0x001F1AD8).  The
//   display gets the pad input (input0) and the conductor becomes
//   RaceBase::dc.  When a member constructor throws, the members built so
//   far are destroyed (~RaceTimeTrialDisplay, ~ReplayPause,
//   ~RacePauseWithRetry, the inline ~ResultTimeTrial, ~RaceSolitaire).
//   Also called directly by run_time_trial (a RaceTimeTrial built in place).
RaceTimeTrial::RaceTimeTrial(int index)
    : index(index), replayPause(0)
{
    /* implicit: RaceSolitaire, RaceTimeTrialDisplay, ReplayPause,
     * RacePauseWithRetry, CarIcon, DynamicsConductorTimeTrial (the base's /
     * members' constructors), ~RaceSolitaire, ~RaceTimeTrialDisplay,
     * ~ReplayPause, ~RacePauseWithRetry (their cleanup on a throw) */
    setRaceMode("RACE_MODE_TIME_TRIAL");
    setRaceDisplay(&display);
    display.setInput(&input0);             /* inline: the store goes through &display ($s0) */
    dc = &dcTimeTrial;
}

// FUNC 0x00188268 RaceTimeTrial::~RaceTimeTrial
//   (index.txt: RaceTimeTrial::virtual_0) The members in reverse order (the
//   display 0x001888D8, the replay pause, the retry pause, the inline
//   ~ResultTimeTrial; the conductor and the icons have no destructor), then
//   RaceSolitaire; deleting form: operator delete(this, 0x48A80).
RaceTimeTrial::~RaceTimeTrial()
{
    /* implicit: ~RaceSolitaire, ~RaceTimeTrialDisplay, ~ReplayPause,
     * ~RacePauseWithRetry (the base / member destructors) */
}

// FUNC 0x00188318 RaceTimeTrial::virtual_8
//   Race start: the result gets the trial index, then RaceSolitaire's.  In
//   a race the retry pause menu and the result sequence are installed; in a
//   replay the replay pause menu (told the organization) and no result.
void RaceTimeTrial::virtual_8(int replay)
{
    result.setIndex(index);
    RaceSolitaire::virtual_8(replay);
    if (replay != RACE_REPLAY) {
        setPauseMenu(&retryPause);
        setSubDisplay(&result);
    } else {
        replayPause.F_001A3248(org);
        setPauseMenu(&replayPause);
        setSubDisplay(NULL);
    }
}

// FUNC 0x001883D8 RaceTimeTrial::virtual_43
//   Before the race runs: the icons of the race's cars (RacePS2Base's empty
//   one is not called).
void RaceTimeTrial::virtual_43()
{
    car_icons_make(icons, org);
}

/* RaceTimeTrial's node in the registry (.sdata zeros, linked in at start-up). */
RaceClassNode g_00351EA0;                   /* 0x00351EA0 */

// FUNC 0x00188400 race_time_trial_static_init
//   .ctors entry (0x0028F110): registers "RaceTimeTrial" (g_002B89D8) with
//   its node g_00351EA0.
void race_time_trial_static_init(void)
{
    race_class_register(&g_00351EA0, &g_002B89D8);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x00188428 __tf13RaceTimeTrial
//   Builds the __si_type_info __ti13RaceTimeTrial (base RaceSolitaire) on
//   the first call.
extern "C" const type_info &__tf13RaceTimeTrial(void)
{
    if (__ti13RaceTimeTrial[0] == 0) {
        __tf13RaceSolitaire();
        __rtti_si(__ti13RaceTimeTrial, "13RaceTimeTrial", (const type_info *)__ti13RaceSolitaire);
    }
    return *(const type_info *)__ti13RaceTimeTrial;
}
#endif
