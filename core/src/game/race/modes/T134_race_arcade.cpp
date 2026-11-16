/* T134_race_arcade.cpp - RaceArcadeInformation, RaceArcade and
 * RaceArcadeSingle (0x00182118..0x00182850); the classes are declared in
 * include/game/race/modes/T134_race_arcade.h.
 *
 * RaceArcade is the arcade race: RaceSinglePlayer with one CarIcon per car
 * (7), the arcade result board (ResultArcade), the race pause menu, the
 * replay pause menu and its car dynamics (DynamicsConductorSinglePlayer,
 * which RaceBase::dc points to).  At the race start (virtual_8) it hands
 * RaceBasic the race pause menu and the result, or in a replay (mode 1) the
 * replay pause menu and no result.  virtual_43 (before the race runs) makes
 * the icons and gives them to the result board and the race display.
 * RaceArcadeSingle adds nothing; both are registered race classes.
 *
 * Includes (u046's data pass): the classes retail's T134 saw, in its order.
 * Its .rodata (0x00328B18..0x00329200) is "bad_alloc" (<new>), the literals,
 * the three vtables, then the type names in reverse definition order: its
 * own three, the result bars (T200), ReplayPauseMenu (T170), PauseMenu
 * (T169), the windows (T199), RaceMenuButton (T198), the menu operator
 * classes (T216), the race display's (T154), UnitArena<64> (T153, read
 * first: before RaceDisplayBase here, unlike T133's) and Operator (a
 * non-polymorphic base: last).  Its .sdata (0x00351E28..0x00351E40) is T157's
 * "" (the race display), then the nodes g_00351E30 / g_00351E38.
 */
// DATA_START 0x00328B18
// DATA_START 0x00351E28
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (the construct functions); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, first (T153_race_course.h) */
#include "game/race/core/T153_race_course.h"
#include "game/race/display/T154_race_display.h"         /* RaceDisplay::setCarIcons */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the menu operator classes: retail's type-name order) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (RaceMenuButton: retail's type-name order) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (the windows: retail's type-name order) */
#include "game/race/pause/T169_race_pause.h"           /* RacePause */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (F_001A3248) */
#include "game/menu/records/T200_result_board.h"         /* (the result bars: retail's type-name order) */
#include "game/menu/records/T117_result_arcade.h"        /* ResultArcade::setDisplays */
#include "game/race/modes/T134_race_arcade.h"          /* own: RaceArcade(Information), RaceArcadeSingle; + T133
                                               RaceSinglePlayer.., T129 RaceBase (RaceClassInfo /
                                               RaceClassNode, race_class_register, RACE_REPLAY) */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/car/model/T184_car_icon_maker.h"       /* CarIcon, car_icons_make */
#include "game/race/modes/race_modes_internal.h"                           /* the unit's constants */

/* ======================================================================== */
/* RaceArcadeInformation                                                    */
/* ======================================================================== */

// FUNC 0x00182118 RaceArcadeInformation::RaceArcadeInformation
//   RaceSinglePlayerInformation::RaceSinglePlayerInformation and the vptr.
//   Callers: run_arcade_race (T052) and RaceArcadeDemoInformation's
//   constructor.
RaceArcadeInformation::RaceArcadeInformation()
{
    /* implicit: RaceSinglePlayerInformation (the base's / members'
     * constructors) */
}

// FUNC 0x00182150 RaceArcadeInformation::~RaceArcadeInformation
//   The vptr, then RaceSinglePlayerInformation::
//   ~RaceSinglePlayerInformation with the caller's flags.
RaceArcadeInformation::~RaceArcadeInformation()
{
    /* implicit: ~RaceSinglePlayerInformation (the base / member destructors) */
}

/* ======================================================================== */
/* RaceArcade                                                               */
/* ======================================================================== */

// FUNC 0x00182178 RaceArcade::operator new
//   The race pool, 16-byte aligned (pdistd::Allocator::allocate).
void *RaceArcade::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x001821A0 RaceArcade::operator delete
//   The size (0x34580) is ignored (pdistd::Allocator::free).
void RaceArcade::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x001821C8 RaceArcade::virtual_24
//   The class name.
const char *RaceArcade::virtual_24()
{
    return "RaceArcade";
}

// FUNC 0x001821D8 RaceArcade_create
//   RaceClassInfo::create (g_002B6A58 +8): new RaceArcade (0x34580;
//   RaceArcade::operator new, the constructor, operator delete if it
//   throws).
RaceBase *RaceArcade_create(void)
{
    return new RaceArcade; /* implicit: RaceArcade::operator delete (if the constructor throws) */
}

// FUNC 0x00182238 RaceArcade_construct
//   RaceClassInfo::construct (g_002B6A58 +0xC): a RaceArcade in buf (NULL:
//   nothing).  arg is not used.
RaceBase *RaceArcade_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)                        /* egcs tests the result of <new>'s throw() placement
                                               new itself; GCC 14 does not */
        return NULL;
#endif
    return ::new (buf) RaceArcade;
}

/* The registry record of "RaceArcade" (.data, before the frame info; its
 * "RaceArcade" is virtual_24's literal, emitted once). */
RaceClassInfo g_002B6A58 = {                /* 0x002B6A58 */
    "RaceArcade", sizeof(RaceArcade), RaceArcade_create, RaceArcade_construct
};

// FUNC 0x00182260 RaceArcade::RaceArcade
//   RaceSinglePlayer::RaceSinglePlayer, the vptr, then the members: the 7
//   icons (CarIcon::CarIcon each), the result (ResultArcade's implicit
//   constructor: done = player = 0, the vptr, ResultBoard::ResultBoard), the
//   pause menu (RacePause::RacePause), the replay pause menu
//   (ReplayPause::ReplayPause, mode 0) and the dynamics
//   (DynamicsConductorSinglePlayer::DynamicsConductorSinglePlayer, never
//   destroyed).  The body names the mode and installs the dynamics.  The
//   landing pads destroy what was built (ReplayPause::~ReplayPause,
//   RacePause::~RacePause, the result's ResultBoard::~ResultBoard with
//   flags 2 and the RaceResultBase vptr back, RaceSinglePlayer::
//   ~RaceSinglePlayer).  Callers: run_arcade_race, run_gtmode_race,
//   create / construct, RaceArcadeSingle, RaceArcadeDemo.
RaceArcade::RaceArcade()
    : replayPause(0)
{
    /* implicit: RaceSinglePlayer, RacePause, ReplayPause, CarIcon,
     * ResultBoard, DynamicsConductorSinglePlayer (the base's / members'
     * constructors), ~RaceSinglePlayer, ~RacePause, ~ReplayPause, ~ResultBoard
     * (their cleanup on a throw) */
    setRaceMode("RACE_MODE_SINGLE");
    dc = &dcSingle;
}

// FUNC 0x00182408 RaceArcade::~RaceArcade
//   The vptr, then the members in reverse order (flags 2):
//   ReplayPause::~ReplayPause, RacePause::~RacePause, the result (implicit:
//   ResultBoard::~ResultBoard, the vptr = RaceResultBase's);
//   RaceSinglePlayer::~RaceSinglePlayer (flags 0); deleting form: operator
//   delete(this, 0x34580).
RaceArcade::~RaceArcade()
{
    /* implicit: ~RaceSinglePlayer, ~RacePause, ~ReplayPause, ~ResultBoard (the
     * base / member destructors) */
}

// FUNC 0x001824B8 RaceArcade::virtual_8
//   (index: RaceArcadeSingle::virtual_8; the slot of RaceArcade's vtable.)
//   Race start: RaceSinglePlayer's, then the menus: in a replay the replay
//   pause menu (given the organization first, F_001A3248) and no result,
//   otherwise the race pause menu and the arcade result.
void RaceArcade::virtual_8(int replay)
{
    RaceSinglePlayer::virtual_8(replay);
    if (replay != RACE_REPLAY) {
        setPauseMenu(&pause);
        setSubDisplay(&resultArcade);
    } else {
        replayPause.F_001A3248(org);
        setPauseMenu(&replayPause);
        setSubDisplay(NULL);
    }
}

// FUNC 0x00182550 RaceArcade::virtual_43
//   (index: RaceArcadeSingle::virtual_43.)  Before the race runs: the car
//   icons of this organization (car_icons_make), given to the result board
//   (ResultArcade::setDisplays) and to the race display (the inline
//   RaceDisplay::setCarIcons: retail stores through the display's address,
//   its `this').
void RaceArcade::virtual_43()
{
    car_icons_make(icons, org);
    resultArcade.setDisplays(icons);
    display.setCarIcons(icons);
}

/* ======================================================================== */
/* RaceArcadeSingle                                                         */
/* ======================================================================== */

// FUNC 0x001825B0 RaceArcadeSingle::operator new
//   The race pool, 16-byte aligned (pdistd::Allocator::allocate).
void *RaceArcadeSingle::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x001825D8 RaceArcadeSingle::operator delete
//   The size (0x34580) is ignored (pdistd::Allocator::free).
void RaceArcadeSingle::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00182600 RaceArcadeSingle::virtual_24
//   The class name.
const char *RaceArcadeSingle::virtual_24()
{
    return "RaceArcadeSingle";
}

// FUNC 0x00182610 RaceArcadeSingle_create
//   RaceClassInfo::create (g_002B6A68 +8): new RaceArcadeSingle (0x34580;
//   RaceArcadeSingle::operator new, the constructor, operator delete if it
//   throws).
RaceBase *RaceArcadeSingle_create(void)
{
    return new RaceArcadeSingle; /* implicit: RaceArcadeSingle::operator delete (if the constructor throws) */
}

// FUNC 0x00182670 RaceArcadeSingle_construct
//   RaceClassInfo::construct (g_002B6A68 +0xC): a RaceArcadeSingle in buf
//   (NULL: nothing).  arg is not used.
RaceBase *RaceArcadeSingle_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)                        /* egcs tests the result of <new>'s throw() placement
                                               new itself; GCC 14 does not */
        return NULL;
#endif
    return ::new (buf) RaceArcadeSingle;
}

/* The registry record of "RaceArcadeSingle" (.data, after RaceArcade's). */
RaceClassInfo g_002B6A68 = {                /* 0x002B6A68 */
    "RaceArcadeSingle", sizeof(RaceArcadeSingle), RaceArcadeSingle_create, RaceArcadeSingle_construct
};

// FUNC 0x00182698 RaceArcadeSingle::RaceArcadeSingle
//   RaceArcade::RaceArcade and the vptr.
RaceArcadeSingle::RaceArcadeSingle()
{
    /* implicit: RaceArcade (the base's / members' constructors) */
}

// FUNC 0x001826D0 RaceArcadeSingle::~RaceArcadeSingle
//   The vptr, RaceArcade::~RaceArcade (flags 0); deleting form: operator
//   delete(this, 0x34580).
RaceArcadeSingle::~RaceArcadeSingle()
{
    /* implicit: ~RaceArcade (the base / member destructors) */
}

/* The two classes' nodes in the registry (.sdata zeros, linked in at
 * start-up). */
RaceClassNode g_00351E30;                   /* 0x00351E30 */
RaceClassNode g_00351E38;                   /* 0x00351E38 */

// FUNC 0x00182728 race_arcade_static_ctor
//   .ctors entry (0x0028F0F8): registers "RaceArcade" (g_002B6A58, node
//   gp-0x5BC0) and "RaceArcadeSingle" (g_002B6A68, node gp-0x5BB8).
void race_arcade_static_ctor(void)
{
    race_class_register(&g_00351E30, &g_002B6A58);
    race_class_register(&g_00351E38, &g_002B6A68);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x00182760 __tf16RaceArcadeSingle
//   The __si_type_info __ti16RaceArcadeSingle, base __ti10RaceArcade.
extern "C" const type_info &__tf16RaceArcadeSingle(void)
{
    if (__ti16RaceArcadeSingle[0] == 0) {
        __tf10RaceArcade();
        __rtti_si(__ti16RaceArcadeSingle, "16RaceArcadeSingle",
                  (const type_info *)__ti10RaceArcade);
    }
    return *(const type_info *)__ti16RaceArcadeSingle;
}

// FUNC 0x001827B0 __tf10RaceArcade
//   The __si_type_info __ti10RaceArcade, base __ti16RaceSinglePlayer.
//   Callers: __tf16RaceArcadeSingle, __tf14RaceArcadeDemo.
extern "C" const type_info &__tf10RaceArcade(void)
{
    if (__ti10RaceArcade[0] == 0) {
        __tf16RaceSinglePlayer();
        __rtti_si(__ti10RaceArcade, "10RaceArcade",
                  (const type_info *)__ti16RaceSinglePlayer);
    }
    return *(const type_info *)__ti10RaceArcade;
}

// FUNC 0x00182800 __tf21RaceArcadeInformation
//   The __si_type_info __ti21RaceArcadeInformation, base
//   __ti27RaceSinglePlayerInformation.  Caller:
//   __tf25RaceArcadeDemoInformation.
extern "C" const type_info &__tf21RaceArcadeInformation(void)
{
    if (__ti21RaceArcadeInformation[0] == 0) {
        __tf27RaceSinglePlayerInformation();
        __rtti_si(__ti21RaceArcadeInformation, "21RaceArcadeInformation",
                  (const type_info *)__ti27RaceSinglePlayerInformation);
    }
    return *(const type_info *)__ti21RaceArcadeInformation;
}
#endif
