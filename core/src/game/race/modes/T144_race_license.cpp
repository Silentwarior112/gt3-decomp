/* T144 - RaceLicenseBGM, RaceLicense, LicensePause (u048),
 * 0x00189AE0..0x0018A0E8.
 *
 * The licence test: a RaceSolitaire registered as "RaceLicense" (mode name
 * "RACE_MODE_LICENSE").  Its members: a LicensePause menu (a
 * RacePauseWithRetry), a ReplayPause menu, its own BGM (RaceLicenseBGM,
 * which becomes RaceBase::bgm: no race music), the RaceLicenseDisplay (T147)
 * and the DynamicsConductorLicense (RaceBase::dc).  No result sequence here:
 * the licence display does the judging (LicenseExaminationMessages, T146).
 *
 * Includes (u048's data pass): the classes retail's T144 saw, in its order.
 * Its .rodata (0x0032BF70..0x0032C5AE) is "bad_alloc" (<new>), the two
 * literals, the three vtables, then the type names in reverse definition
 * order: RaceLicense, RaceLicenseBGM, LicensePause, ExaminationDisplayer,
 * CallBackWhenPrizeRendering, PrizeModelSetContext, ResultTermBar,
 * ResultTitleBar, ReplayPauseMenu, PauseMenuWithRetry, RaceMenuSelectWindow,
 * RaceMenuWindow, RaceMenuButton, the OperatorHolder group, then
 * T154_race_display.h's (RaceMiniMap .. RaceMeterBase, RaceCameraBase,
 * UnitArena<64>, RaceValueDisplayBase, RaceDisplayBase), the GameStatus's
 * garage names (GarageWheelList .. Cell<GarageCar>), Operator: the
 * GameStatus first, then the race display, the menu classes,
 * RaceMenuButton, the windows, the pause menus, the result bars, the prize
 * classes and ExaminationDisplayer.  Its .sdata (0x00351ED0..0x00351EE0) is
 * the "" of T157's inline RaceMessageDisplay::clear, then the registry node
 * g_00351ED8 (an uninitialised global: zeros at the end).
 */
// DATA_START 0x0032BF70
// DATA_START 0x00351ED0
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (race_license_construct); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/save/T429_game_status.h"          /* no use: the garage classes retail's T144 saw (first) */
#include "game/race/display/T154_race_display.h"         /* the race display classes */
#include "game/menu/framework/T216_menu_transitive.h"      /* no use: the OperatorHolder classes retail's T144 saw */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* RaceMenuButton (before the windows: retail's order) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* RaceMenuWindow, RaceMenuSelectWindow */
#include "game/race/pause/T171_race_pause_with_retry.h"    /* base RacePauseWithRetry (PauseMenuWithRetry) */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (ReplayPauseMenu) */
#include "game/menu/records/T200_result_board.h"         /* no use: ResultTitleBar, ResultTermBar (seen in retail) */
#include "game/menu/gtmode/T460_prize_base.h"           /* PrizeModelSetContext, CallBackWhenPrizeRendering */
#include "game/race/display/T141_race_time_trial_display.h"  /* ExaminationDisplayer (T146's base) */
#include "game/race/modes/T144_race_license.h"         /* own: RaceLicense, RaceLicenseBGM, LicensePause, the
                                               registry externs; + T139 RaceSolitaire, T130
                                               RaceBGMPS2, T147 RaceLicenseDisplay, T129
                                               race_class_register */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post, RACE_EVENT_GO */
#include "game/race/modes/race_time_trial_internal.h"

/* ------------------------------------------------------------------------ */
/* RaceLicenseBGM                                                           */
/* ------------------------------------------------------------------------ */

// FUNC 0x00189AE0 RaceLicenseBGM::RaceLicenseBGM
//   RaceBGMPS2 (0x0017DCA0), then its vptr (+4).
RaceLicenseBGM::RaceLicenseBGM()
{
    /* implicit: RaceBGMPS2 (the base's / members' constructors) */
}

// FUNC 0x00189B18 RaceLicenseBGM::~RaceLicenseBGM
//   (index.txt: virtual_0) The vptr, ~RaceBGMPS2 (flags 0); deleting form:
//   RaceBGMPS2::operator delete(this, 0x9B8).
RaceLicenseBGM::~RaceLicenseBGM()
{
    /* implicit: ~RaceBGMPS2, operator delete (the base / member destructors,
     * flags & 1) */
}

// FUNC 0x00189B70 RaceLicenseBGM::play
//   Play: code 0x14 (the next song of the play list, the race's music)
//   stops the music instead (stop, through the vtable); every other
//   code goes to RaceBGMPS2.
void RaceLicenseBGM::play(int code, int arg)
{
    if (code == BGM_CODE_NEXT_SONG)
        stop();
    else
        RaceBGMPS2::play(code, arg);
}

/* ------------------------------------------------------------------------ */
/* RaceLicense                                                              */
/* ------------------------------------------------------------------------ */

// FUNC 0x00189BB8 RaceLicense::operator new
//   The race pool, 16-byte aligned (T144's copy of RaceBase's inline one).
void *RaceLicense::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00189BE0 RaceLicense::operator delete
//   The size is ignored.
void RaceLicense::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00189C08 RaceLicense::virtual_24
//   The class name.
const char *RaceLicense::virtual_24()
{
    return "RaceLicense";
}

// FUNC 0x00189C18 race_license_create
//   RaceClassInfo::create of "RaceLicense": new RaceLicense (operator new
//   0x3B440; operator delete if the constructor throws).
RaceBase *race_license_create(void)
{
    return new RaceLicense; /* implicit: RaceLicense::operator delete (if the constructor throws) */
}

// FUNC 0x00189C78 race_license_construct
//   RaceClassInfo::construct: a RaceLicense in buf (nothing if buf is
//   NULL).  arg is not used.
RaceBase *race_license_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)                        /* egcs tests the result of <new>'s throw() placement
                                               new itself (retail's daddu v0,a0 / beqz v0); GCC 14
                                               does not */
        return NULL;
#endif
    return ::new (buf) RaceLicense;
}

/* The registry record of "RaceLicense" (.data, before the frame info; its
 * "RaceLicense" is virtual_24's literal, written once). */
RaceClassInfo g_002B93D0 = {                /* 0x002B93D0 */
    "RaceLicense", sizeof(RaceLicense), race_license_create, race_license_construct
};

// FUNC 0x00189CA0 RaceLicense::RaceLicense
//   RaceSolitaire, the pause menu (RacePauseWithRetry 0x001A3A08, then the
//   inline LicensePause: its vptr), the replay pause menu (ReplayPause
//   0x001A30C0, mode 0), the BGM (RaceLicenseBGM 0x00189AE0), the display
//   (RaceLicenseDisplay 0x0018BE40) and the conductor
//   (DynamicsConductorLicense 0x001F0AD8).  The display gets the pad input;
//   the conductor and the BGM become RaceBase::dc and RaceBase::bgm.
//   Unwinding: ~RaceLicenseDisplay, ~RaceLicenseBGM, ~ReplayPause,
//   ~RacePauseWithRetry, ~RaceSolitaire.  Also called directly by
//   run_license_test.
RaceLicense::RaceLicense()
    : replayPause(0)
{
    /* implicit: RaceSolitaire, RaceLicenseBGM, RaceLicenseDisplay,
     * ReplayPause, RacePauseWithRetry, DynamicsConductorLicense (the base's /
     * members' constructors), ~RaceSolitaire, ~RaceLicenseBGM,
     * ~RaceLicenseDisplay, ~ReplayPause, ~RacePauseWithRetry (their cleanup on
     * a throw) */
    setRaceMode("RACE_MODE_LICENSE");
    setRaceDisplay(&display);
    display.setInput(&input0);              /* inline (T147): the store goes through &display */
    dc = &dcLicense;
    bgm = &licenseBgm;
}

// FUNC 0x00189E50 RaceLicense::~RaceLicense
//   (index.txt: virtual_0) The display (0x0018BEF8), the BGM, the replay
//   pause, the pause (LicensePause's implicit destructor expanded:
//   ~RacePauseWithRetry), then RaceSolitaire; deleting form: operator
//   delete(this, 0x3B440).
RaceLicense::~RaceLicense()
{
    /* implicit: ~RaceSolitaire, ~RaceLicenseBGM, ~RaceLicenseDisplay,
     * ~ReplayPause, ~RacePauseWithRetry (the base / member destructors) */
}

// FUNC 0x00189F00 RaceLicense::virtual_8
//   Race start: RaceSolitaire's, then the pause menu (a race) or the replay
//   pause menu told the organization (a replay).  No result sequence.
void RaceLicense::virtual_8(int replay)
{
    RaceSolitaire::virtual_8(replay);
    if (replay != RACE_REPLAY) {
        setPauseMenu(&pause);
    } else {
        replayPause.F_001A3248(org);
        setPauseMenu(&replayPause);
    }
}

// FUNC 0x00189F78 RaceLicense::start
//   The start countdown is over: the start event only (RaceBase's then also
//   posts the race music, event 0x14 code 0x14).
void RaceLicense::start()
{
    org->eventQueue.post(RACE_EVENT_GO, 0, 0);
}

// FUNC 0x00189FA8 F_00189FA8
//   Empty; no callers, no data references (? an empty inline method emitted
//   in T144).
void F_00189FA8(void)
{
}

/* RaceLicense's node in the registry (.sdata zeros, linked in at start-up). */
RaceClassNode g_00351ED8;                   /* 0x00351ED8 */

// FUNC 0x00189FB0 race_license_static_init
//   .ctors entry (0x0028F118): registers "RaceLicense" (g_002B93D0) with
//   its node g_00351ED8.
void race_license_static_init(void)
{
    race_class_register(&g_00351ED8, &g_002B93D0);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x00189FD8 __tf11RaceLicense
//   Builds the __si_type_info __ti11RaceLicense (base RaceSolitaire) on the
//   first call.
extern "C" const type_info &__tf11RaceLicense(void)
{
    if (__ti11RaceLicense[0] == 0) {
        __tf13RaceSolitaire();
        __rtti_si(__ti11RaceLicense, "11RaceLicense", (const type_info *)__ti13RaceSolitaire);
    }
    return *(const type_info *)__ti11RaceLicense;
}

// FUNC 0x0018A028 __tf14RaceLicenseBGM
//   Builds the __si_type_info __ti14RaceLicenseBGM (base RaceBGMPS2) on the
//   first call.
extern "C" const type_info &__tf14RaceLicenseBGM(void)
{
    if (__ti14RaceLicenseBGM[0] == 0) {
        __tf10RaceBGMPS2();
        __rtti_si(__ti14RaceLicenseBGM, "14RaceLicenseBGM", (const type_info *)__ti10RaceBGMPS2);
    }
    return *(const type_info *)__ti14RaceLicenseBGM;
}

// FUNC 0x0018A078 __tf12LicensePause
//   Builds the __si_type_info __ti12LicensePause (base RacePauseWithRetry)
//   on the first call.
extern "C" const type_info &__tf12LicensePause(void)
{
    if (__ti12LicensePause[0] == 0) {
        __tf18RacePauseWithRetry();
        __rtti_si(__ti12LicensePause, "12LicensePause", (const type_info *)__ti18RacePauseWithRetry);
    }
    return *(const type_info *)__ti12LicensePause;
}
#endif

// FUNC 0x0018A0C8 LicensePause::~LicensePause
/* the implicit destructor (game/race/modes/T144_race_license.h declares no member);
 * egcs emits this copy for the vtable: ~RacePauseWithRetry with the
 * caller's flags (which deletes), no vptr store */
/* implicit: ~RacePauseWithRetry (the base / member destructors) */
