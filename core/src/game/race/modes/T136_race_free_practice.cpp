/* T136_race_free_practice.cpp - RaceFreePractice (0x00182E58..0x00183240);
 * the class is declared in include/game/race/modes/T136_race_free_practice.h.
 *
 * The free practice race mode ("RACE_MODE_FREE_PRACTICE"): RaceSinglePlayer
 * with the 7 car icons, the race / replay pause menus and its own dynamics
 * (DynamicsConductorFreePractice).  Unlike RaceArcade it has no result
 * sequence: virtual_8 only installs a pause menu, and the icons go to the
 * race display alone.
 *
 * Includes (u046's data pass): the classes retail's T136 saw, in its order.
 * Its .rodata (0x003296C8..0x00329AF8) is "bad_alloc" (<new>), the literals,
 * the vtable, then the type names in reverse definition order: its own,
 * ReplayPauseMenu (T170), PauseMenu (T169), the windows (T199),
 * RaceMenuButton (T198), the menu operator classes (T216), the race
 * display's (T154, UnitArena<64> among them) and Operator (a non-polymorphic
 * base: last); no result bars (it has no result).  Its .sdata
 * (0x00351E50..0x00351E60) is T157's "" (the race display), then the node
 * g_00351E58.
 */
// DATA_START 0x003296C8
// DATA_START 0x00351E50
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (the construct function); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/race/display/T154_race_display.h"         /* RaceDisplay::setCarIcons (first: retail's type-name order) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the menu operator classes: retail's type-name order) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (RaceMenuButton: retail's type-name order) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (the windows: retail's type-name order) */
#include "game/race/pause/T169_race_pause.h"           /* RacePause */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (F_001A3248) */
#include "game/race/modes/T136_race_free_practice.h"   /* own: RaceFreePractice; + T133 RaceSinglePlayer, T129
                                               RaceBase (RaceClassInfo / RaceClassNode,
                                               race_class_register, RACE_REPLAY) */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/car/model/T184_car_icon_maker.h"       /* CarIcon, car_icons_make */
#include "game/race/modes/race_modes_internal.h"                           /* the unit's constants */

// FUNC 0x00182E58 RaceFreePractice::operator new
//   The race pool, 16-byte aligned (pdistd::Allocator::allocate).
void *RaceFreePractice::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00182E80 RaceFreePractice::operator delete
//   The size (0x33C40) is ignored (pdistd::Allocator::free).
void RaceFreePractice::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00182EA8 RaceFreePractice::virtual_24
//   The class name.
const char *RaceFreePractice::virtual_24()
{
    return "RaceFreePractice";
}

// FUNC 0x00182EB8 RaceFreePractice_create
//   RaceClassInfo::create (g_002B70D8 +8): new RaceFreePractice (0x33C40;
//   RaceFreePractice::operator new, the constructor, operator delete if it
//   throws).
RaceBase *RaceFreePractice_create(void)
{
    return new RaceFreePractice; /* implicit: RaceFreePractice::operator delete (if the constructor throws) */
}

// FUNC 0x00182F18 RaceFreePractice_construct
//   RaceClassInfo::construct (g_002B70D8 +0xC): a RaceFreePractice in buf
//   (NULL: nothing).  arg is not used.
RaceBase *RaceFreePractice_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)                        /* egcs tests the result of <new>'s throw() placement
                                               new itself; GCC 14 does not */
        return NULL;
#endif
    return ::new (buf) RaceFreePractice;
}

/* The registry record of "RaceFreePractice" (.data, before the frame info;
 * its "RaceFreePractice" is virtual_24's literal, emitted once). */
RaceClassInfo g_002B70D8 = {                /* 0x002B70D8 */
    "RaceFreePractice", sizeof(RaceFreePractice), RaceFreePractice_create, RaceFreePractice_construct
};

// FUNC 0x00182F40 RaceFreePractice::RaceFreePractice
//   RaceSinglePlayer::RaceSinglePlayer, the vptr, the 7 icons
//   (CarIcon::CarIcon), the pause menu (RacePause::RacePause), the replay
//   pause menu (ReplayPause::ReplayPause, mode 0) and the dynamics
//   (DynamicsConductorFreePractice::DynamicsConductorFreePractice, never
//   destroyed); the body names the mode and installs the dynamics.  The
//   landing pads run ReplayPause::~ReplayPause, RacePause::~RacePause and
//   RaceSinglePlayer::~RaceSinglePlayer.  Callers: run_gtmode_race,
//   run_championship (T053), create / construct.
RaceFreePractice::RaceFreePractice()
    : replayPause(0)
{
    /* implicit: RaceSinglePlayer, RacePause, ReplayPause, CarIcon,
     * DynamicsConductorFreePractice (the base's / members' constructors),
     * ~RaceSinglePlayer, ~RacePause, ~ReplayPause (their cleanup on a throw) */
    setRaceMode("RACE_MODE_FREE_PRACTICE");
    dc = &dcFree;
}

// FUNC 0x00183078 RaceFreePractice::~RaceFreePractice
//   The vptr, ReplayPause::~ReplayPause and RacePause::~RacePause (flags
//   2), RaceSinglePlayer::~RaceSinglePlayer (flags 0); deleting form:
//   operator delete(this, 0x33C40).
RaceFreePractice::~RaceFreePractice()
{
    /* implicit: ~RaceSinglePlayer, ~RacePause, ~ReplayPause (the base / member
     * destructors) */
}

// FUNC 0x00183100 RaceFreePractice::virtual_8
//   Race start: RaceSinglePlayer's, then the race pause menu, or in a
//   replay the replay pause menu (given the organization first,
//   F_001A3248).
void RaceFreePractice::virtual_8(int replay)
{
    RaceSinglePlayer::virtual_8(replay);
    if (replay != RACE_REPLAY) {
        setPauseMenu(&pause);
    } else {
        replayPause.F_001A3248(org);
        setPauseMenu(&replayPause);
    }
}

// FUNC 0x00183178 RaceFreePractice::virtual_43
//   Before the race runs: the car icons (car_icons_make), given to the race
//   display (the inline RaceDisplay::setCarIcons).
void RaceFreePractice::virtual_43()
{
    car_icons_make(icons, org);
    display.setCarIcons(icons);
}

/* RaceFreePractice's node in the registry (.sdata zeros, linked in at
 * start-up). */
RaceClassNode g_00351E58;                   /* 0x00351E58 */

// FUNC 0x001831C8 race_free_practice_static_ctor
//   .ctors entry (0x0028F100): registers "RaceFreePractice" (g_002B70D8,
//   node gp-0x5B98).
void race_free_practice_static_ctor(void)
{
    race_class_register(&g_00351E58, &g_002B70D8);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001831F0 __tf16RaceFreePractice
//   The __si_type_info __ti16RaceFreePractice, base __ti16RaceSinglePlayer.
extern "C" const type_info &__tf16RaceFreePractice(void)
{
    if (__ti16RaceFreePractice[0] == 0) {
        __tf16RaceSinglePlayer();
        __rtti_si(__ti16RaceFreePractice, "16RaceFreePractice",
                  (const type_info *)__ti16RaceSinglePlayer);
    }
    return *(const type_info *)__ti16RaceFreePractice;
}
#endif
