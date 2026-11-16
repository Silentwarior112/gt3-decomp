/* T135_race_arcade_demo.cpp - RaceArcadeDemoInformation and RaceArcadeDemo
 * (0x00182850..0x00182E58); the classes are declared in
 * include/game/race/modes/T135_race_arcade_demo.h.
 *
 * RaceArcadeDemo is an arcade race run as a demonstration (the attract
 * demo, or with a "shop car" the test drive of a dealer's car: the
 * information's setShopCar / setShopCarPos, Lupo 2 names).  The shop car,
 * when set, is loaded into every car setupEntry is given (colour 0 of the
 * car) and keeps slot 0; the opponents fill the rest, and the car is moved
 * to grid position shopCarPos (1-based; 0 = the last).  The race has no
 * time limit by default (getTimeLimitOr(0)) and no laps (getNumberOfLaps
 * 0).
 *
 * The race itself: the pause menu closes itself after 30 s, event 7 (the
 * time limit) ends the race with code 5 (virtual_34), and an idle timer of
 * 1800 frames (30 s at 60 Hz) posts event 1 (the end) unless a button other
 * than left / right is held (or in a replay).  The race display is created
 * without its inner panel.
 *
 * Includes (u046's data pass): the classes retail's T135 saw, in its order.
 * Its .rodata (0x00329200..0x003296D8) is "bad_alloc" (<new>), the class
 * name, the two vtables, then the type names in reverse definition order:
 * its own two, the result bars (T200), ReplayPauseMenu (T170), PauseMenu
 * (T169), the windows (T199), RaceMenuButton (T198), the menu operator
 * classes (T216), the race display's (T154, UnitArena<64> among them) and
 * Operator (a non-polymorphic base: last).  Its .sdata
 * (0x00351E40..0x00351E50) is T157's "" (the race display), then the node
 * g_00351E48.
 */
// DATA_START 0x003291F0
// DATA_START 0x00351E40
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (the construct function); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/race/display/T154_race_display.h"         /* (the race display: retail's type-name order) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the menu operator classes: retail's type-name order) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (RaceMenuButton: retail's type-name order) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (the windows: retail's type-name order) */
#include "game/race/pause/T169_race_pause.h"           /* RacePause */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause */
#include "game/menu/records/T200_result_board.h"         /* (the result bars: retail's type-name order) */
#include "game/race/modes/T135_race_arcade_demo.h"     /* own: RaceArcadeDemo(Information); + T134 RaceArcade.., T133,
                                               T129 RaceBase (RaceClassInfo / RaceClassNode,
                                               race_class_register, RACE_LIVE, RACE_END_DEMO_TIME_UP) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::find / post, RACE_EVENT_* */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, eventQueue, the entry order) */
#include "game/race/pause/T168_pause_base.h"           /* PauseBase::setTimeout */
#include "game/race/core/T413_race_input.h"           /* RaceInput */
#include "engine/input/T474_control.h"              /* Control, CONTROL_LEFT / CONTROL_RIGHT */
#include "game/data/T482_car_equipments.h"       /* CarEquipments */
#include "game/data/T485_car_color.h"            /* CarColorDataBase g_00352D98 */
#include "game/race/modes/race_modes_internal.h"                           /* the unit's constants */

/* ======================================================================== */
/* RaceArcadeDemoInformation                                                */
/* ======================================================================== */

// FUNC 0x00182850 RaceArcadeDemoInformation::RaceArcadeDemoInformation
//   RaceArcadeInformation::RaceArcadeInformation, the vptr, no shop car.
//   Callers: run_menu_demo_race, run_attract_demo (T000).
RaceArcadeDemoInformation::RaceArcadeDemoInformation()
{
    shopCar = 0;
    shopCarPos = 0;

    /* implicit: RaceArcadeInformation (the base's / members' constructors) */
}

// FUNC 0x00182890 RaceArcadeDemoInformation::~RaceArcadeDemoInformation
//   The vptr, RaceArcadeInformation::~RaceArcadeInformation with the
//   caller's flags.
RaceArcadeDemoInformation::~RaceArcadeDemoInformation()
{
    /* implicit: ~RaceArcadeInformation (the base / member destructors) */
}

// FUNC 0x001828B8 RaceArcadeDemoInformation::getTimeLimit
//   (slot 6) The time limit, 0 (none) by default (getTimeLimitOr).
int RaceArcadeDemoInformation::getTimeLimit()
{
    return getTimeLimitOr(0);
}

// FUNC 0x001828D8 RaceArcadeDemoInformation::setupEntry
//   (slot 10) With a shop car: the car's equipments load it
//   (CarEquipments::setCar) and take colour 0 of the model (getModelCode;
//   the colour table's getColor, then setColorCode).  No check of the
//   entry number and no inputId (unlike RaceSinglePlayerInformation's):
//   every entry gets the shop car here, then setupOrganization
//   (race_organization_create calls it after the entries) gives slots 1..
//   their opponents, so slot 0 keeps it.
void RaceArcadeDemoInformation::setupEntry(RaceEntryCar *car)
{
    u64 code = shopCar;
    if (code != 0) {
        CarEquipments *equip = &car->info;  /* inside the test: sched2 saves $ra first, as retail */
        equip->setCar(code);
        equip->setColorCode(g_00352D98.getColor(equip->getModelCode(), 0));
    }
}

// FUNC 0x00182930 RaceArcadeDemoInformation::setupOrganization
//   (slot 11) The opponents from slot 1 with a shop car, from slot 0
//   without (virtual assignCarRange); then the grid order: slot 0 goes to
//   shopCarPos - 1, or to the last place.
void RaceArcadeDemoInformation::setupOrganization(RaceOrganization *org)
{
    RaceEntryBase *entries = org->entries;
    assignCarRange(org, shopCar != 0, entries->count);
    org->resetEntryOrder();
    if (shopCarPos != 0)
        org->moveEntryOrder(0, shopCarPos - 1);
    else
        org->moveEntryOrder(0, entries->count - 1);
}

/* ======================================================================== */
/* RaceArcadeDemo                                                           */
/* ======================================================================== */

// FUNC 0x001829C0 RaceArcadeDemo::operator new
//   The race pool, 16-byte aligned (pdistd::Allocator::allocate).
void *RaceArcadeDemo::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x001829E8 RaceArcadeDemo::operator delete
//   The size (0x34D80) is ignored (pdistd::Allocator::free).
void RaceArcadeDemo::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00182A10 RaceArcadeDemo::virtual_24
//   The class name.
const char *RaceArcadeDemo::virtual_24()
{
    return "RaceArcadeDemo";
}

// FUNC 0x00182A20 RaceArcadeDemo_create
//   RaceClassInfo::create (g_002B6DD0 +8): new RaceArcadeDemo (0x34D80;
//   RaceArcadeDemo::operator new, the constructor, operator delete if it
//   throws).
RaceBase *RaceArcadeDemo_create(void)
{
    return new RaceArcadeDemo; /* implicit: RaceArcadeDemo::operator delete (if the constructor throws) */
}

// FUNC 0x00182A80 RaceArcadeDemo_construct
//   RaceClassInfo::construct (g_002B6DD0 +0xC): a RaceArcadeDemo in buf
//   (NULL: nothing).  arg is not used.
RaceBase *RaceArcadeDemo_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)                        /* egcs tests the result of <new>'s throw() placement
                                               new itself; GCC 14 does not */
        return NULL;
#endif
    return ::new (buf) RaceArcadeDemo;
}

/* The registry record of "RaceArcadeDemo" (.data, before the frame info;
 * its "RaceArcadeDemo" is virtual_24's literal, emitted once). */
RaceClassInfo g_002B6DD0 = {                /* 0x002B6DD0 */
    "RaceArcadeDemo", sizeof(RaceArcadeDemo), RaceArcadeDemo_create, RaceArcadeDemo_construct
};

// FUNC 0x00182AA8 RaceArcadeDemo::RaceArcadeDemo
//   RaceArcade::RaceArcade, the vptr, a second pause menu
//   (RacePause::RacePause, +0x34584) and replay pause menu
//   (ReplayPause::ReplayPause, +0x34830, mode 0) of its own, then the race
//   display's inner panel off (RaceBasic::setInnerPanel, inline: slot 4 of
//   raceDisplay, no NULL check).  The landing pads run
//   ReplayPause::~ReplayPause, RacePause::~RacePause and
//   RaceArcade::~RaceArcade.  The idle timer is set by virtual_8.  Callers:
//   run_menu_demo_race, run_attract_demo, create / construct.
RaceArcadeDemo::RaceArcadeDemo()
    : demoReplayPause(0)
{
    /* implicit: RaceArcade, RacePause, ReplayPause (the base's / members'
     * constructors), ~RaceArcade, ~RacePause, ~ReplayPause (their cleanup on a
     * throw) */
    setInnerPanel(false);
}

// FUNC 0x00182B98 RaceArcadeDemo::~RaceArcadeDemo
//   The vptr, ReplayPause::~ReplayPause and RacePause::~RacePause of its
//   own menus (flags 2), RaceArcade::~RaceArcade (flags 0); deleting form:
//   operator delete(this, 0x34D80).
RaceArcadeDemo::~RaceArcadeDemo()
{
    /* implicit: ~RaceArcade, ~RacePause, ~ReplayPause (the base / member
     * destructors) */
}

// FUNC 0x00182C20 RaceArcadeDemo::virtual_8
//   Race start: RaceArcade's (which installs RaceArcade's menus, not the
//   two of this class), the installed pause menu closes itself after 30 s
//   (PauseBase::setTimeout), the idle timer starts at 1800 frames.
void RaceArcadeDemo::virtual_8(int replay)
{
    RaceArcade::virtual_8(replay);
    pauseMenu->setTimeout(30);
    idleTimer = DEMO_IDLE_FRAMES;
}

// FUNC 0x00182C68 RaceArcadeDemo::virtual_27
//   Event 7 (the time limit; taken from the queue, RaceEventQueue::find)
//   ends the race with code 5 (virtual_34), then RacePS2Base's event
//   handling, whose result is returned.
int RaceArcadeDemo::virtual_27()
{
    if (org->eventQueue.find(RACE_EVENT_TIME_UP, NULL, 1) != 0)
        virtual_34(RACE_END_DEMO_TIME_UP);
    return RacePS2Base::virtual_27();
}

// FUNC 0x00182CC8 RaceArcadeDemo::virtual_28
//   The update (RaceSinglePlayer's), then the idle timer: at 0 it posts
//   event 1 (the end); a replay, or any held button of the player's input
//   but left / right (Control::getButtons, slot 4), restarts it at 1800.
void RaceArcadeDemo::virtual_28(float dt)
{
    RaceSinglePlayer::virtual_28(dt);
    if (--idleTimer == 0)
        org->eventQueue.post(RACE_EVENT_END, 0, 0);
    if (replayMode != RACE_LIVE
        || (virtual_25()->getButtons() & ~(u32)(CONTROL_LEFT | CONTROL_RIGHT)) != 0)
        idleTimer = DEMO_IDLE_FRAMES;
}

/* RaceArcadeDemo's node in the registry (.sdata zeros, linked in at
 * start-up). */
RaceClassNode g_00351E48;                   /* 0x00351E48 */

// FUNC 0x00182D78 race_arcade_demo_static_ctor
//   .ctors entry (0x0028F0FC): registers "RaceArcadeDemo" (g_002B6DD0,
//   node gp-0x5BA8).
void race_arcade_demo_static_ctor(void)
{
    race_class_register(&g_00351E48, &g_002B6DD0);
}

// FUNC 0x00182DA0 RaceArcadeDemo::getNumberOfLaps
//   (slot 40, Lupo 2 name) No laps.
/* inline in game/race/modes/T135_race_arcade_demo.h; egcs emits this copy (this unit
 * defines the class's first virtual function) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00182DA8 __tf14RaceArcadeDemo
//   The __si_type_info __ti14RaceArcadeDemo, base __ti10RaceArcade.
extern "C" const type_info &__tf14RaceArcadeDemo(void)
{
    if (__ti14RaceArcadeDemo[0] == 0) {
        __tf10RaceArcade();
        __rtti_si(__ti14RaceArcadeDemo, "14RaceArcadeDemo",
                  (const type_info *)__ti10RaceArcade);
    }
    return *(const type_info *)__ti14RaceArcadeDemo;
}
#endif

// FUNC 0x00182DF8 RaceArcadeDemoInformation::setShopCarPos
//   (Lupo 2 name; unreferenced)
/* inline in game/race/modes/T135_race_arcade_demo.h; egcs emits this copy */

// FUNC 0x00182E00 RaceArcadeDemoInformation::setShopCar
//   (Lupo 2 name; a 64-bit store; unreferenced)
/* inline in game/race/modes/T135_race_arcade_demo.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x00182E08 __tf25RaceArcadeDemoInformation
//   The __si_type_info __ti25RaceArcadeDemoInformation, base
//   __ti21RaceArcadeInformation.
extern "C" const type_info &__tf25RaceArcadeDemoInformation(void)
{
    if (__ti25RaceArcadeDemoInformation[0] == 0) {
        __tf21RaceArcadeInformation();
        __rtti_si(__ti25RaceArcadeDemoInformation, "25RaceArcadeDemoInformation",
                  (const type_info *)__ti21RaceArcadeInformation);
    }
    return *(const type_info *)__ti25RaceArcadeDemoInformation;
}
#endif
