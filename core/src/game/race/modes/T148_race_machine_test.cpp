/* T148 - RaceMachineTest, RaceMachineTestInformation (u049).
 *
 * The machine test (the races "M400" / "M1000" / "MMAX" of the car settings
 * menus, run_machine_test): a RaceSolitaire registered as "RaceMachineTest", with
 * the test kind (0 0-400 m, 1 0-1000 m, 2 the max speed challenge), seven
 * car icons (T184), the result sequence (ResultMachineTest), a retry pause
 * menu (race) and a replay pause menu, a plain RaceDisplay and a
 * DynamicsConductorMachineTest (RaceBase::dc).  The race mode name is
 * "RACE_MODE_MAX_SPEED_CHALLENGE" for the race code "MMAX", else
 * "RACE_MODE_0_400".
 *
 * RaceMachineTestInformation is the race information the menu builds on
 * the stack: two entries, the car of the test (CarEquipments) and the
 * vibration / transmission words run_machine_test takes from the GameStatus.
 *
 * Data (u049's data pass): retail's .rodata 0x0032CBD8..0x0032D1F0 is
 * "bad_alloc" (<new>), virtual_24's "RaceMachineTest" (also the registry
 * record's name), virtual_8's two mode names, the vtables of RaceMachineTest
 * and RaceMachineTestInformation, then the type names RaceMachineTest,
 * RaceMachineTestInformation, ResultTermBar, ResultTitleBar,
 * ReplayPauseMenu, PauseMenuWithRetry, RaceMenuSelectWindow .. the
 * OperatorHolder group (the race menus), RaceMiniMap .. RaceMeterBase,
 * RaceCameraBase, UnitArena<64>, RaceValueDisplayBase, RaceDisplayBase (T154
 * read on its own: it emits UnitArena<64>), the GameStatus's eight and
 * Operator (the non-polymorphic base, last): in reverse definition order
 * the GameStatus first, the race display, the menus (T216, T198, T199), the
 * retry and replay pause menus, the result board (T200), T148's header.
 * .data: the registry record g_002BA298, then the frame info.  .sdata
 * 0x00351F28..0x00351F3D: the "" of T157's inline RaceMessageDisplay::clear
 * (read with the race display), the registry node g_00351F30 (initialised:
 * written at its definition, before virtual_8's "MMAX" 0x00351F38).
 * "bad_alloc" and the "" are not referenced: anchors.
 */
// DATA_START 0x0032CBD8
// DATA_START 0x00351F28
#include "types.h"
#include <new>                              /* "bad_alloc"; the placement new of race_machine_test_construct */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si (egcs's <new> and cxxrt.h conflict) */
#endif
#include "game/save/T429_game_status.h"          /* (no use: the GameStatus's type names, which retail's T148 has) */
#include "game/race/display/T154_race_display.h"         /* RaceDisplay (it emits UnitArena<64>'s name: read before T153) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the race menus' classes, defined before the pause menus) */
#include "game/menu/race_menu/T198_race_menu_button.h"
#include "game/menu/race_menu/T199_race_menu_layer.h"
#include "game/race/pause/T171_race_pause_with_retry.h" /* RacePauseWithRetry */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause */
#include "game/menu/records/T200_result_board.h"         /* (ResultTitleBar / ResultTermBar: after the pause menus) */
#include "game/race/modes/T148_race_machine_test.h"
#include "game/save/T415_machine_test_records.h"  /* MACHINE_TEST_* (RaceMachineTest::kind) */
#include "game/race/core/T129_race_base.h"            /* RaceClassInfo / RaceClassNode, race_class_register, RACE_END_* */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar */
#include "game/race/core/T130_race_ps2_base.h"        /* RaceEntryPS2 (createEntries) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (raceCode, entries, eventQueue) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post, RACE_EVENT_GOAL */
#include "game/race/pause/T168_pause_base.h"           /* PAUSE_RESULT_* */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState (st.finished, topSpeed), car_finish */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T485_car_color.h"            /* the colour table g_00352D98 */
#include "game/race/core/T123_race_setup.h"           /* the race pool g_002B3BF0 (pdistd::Allocator) */
#include "game/race/display/race_license_internal.h"

/* ------------------------------------------------------------------------ */
/* RaceMachineTestInformation                                               */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018C7E8 RaceMachineTestInformation::RaceMachineTestInformation
//   RaceInformation, the vptr (+0x54), the car (CarEquipments 0x00231AB0),
//   transmission = 0, two entries.  vibration is left as it is.  Callers:
//   run_machine_test, run_license_test.
RaceMachineTestInformation::RaceMachineTestInformation()
{
    /* implicit: RaceInformation (base constructor), CarEquipments (member constructor), ~RaceInformation (exception cleanup) */
    transmission = 0;
    entryCount = 2;
}

// FUNC 0x0018C860 RaceMachineTestInformation::~RaceMachineTestInformation
//   (index.txt: virtual_0) The vptr, the car (~CarEquipments, flags 2),
//   then ~RaceInformation with the caller's flags (the deleting form is
//   there).
RaceMachineTestInformation::~RaceMachineTestInformation()
{
    /* implicit: ~CarEquipments (member destructor), ~RaceInformation (base destructor) */
}

// FUNC 0x0018C8B0 RaceMachineTestInformation::createEntries
//   (index.txt: virtual_4) new RaceEntryPS2 (its pool operator new, 0x3C
//   bytes; operator delete(p, 0x3C) if the constructor throws).
RaceEntryBase *RaceMachineTestInformation::createEntries()
{
    return new RaceEntryPS2; /* implicit: RaceEntryPS2::operator delete (if the constructor throws) */
}

// FUNC 0x0018C908 RaceMachineTestInformation::setupEntry
//   (index.txt: virtual_10) Every entry gets the test car, input id 0x100 +
//   its number and order 0.
void RaceMachineTestInformation::setupEntry(RaceEntryCar *entry)
{
    int number = entry->number;
    (CarEquipments &)entry->info = car;     /* CarEquipments::operator= 0x00231B90 */ /* implicit: CarEquipments::operator= */
    entry->info.setInputID(number + 0x100);  /* inline: &entry->info stays in a register */
    entry->order = 0;
}

// FUNC 0x0018C958 RaceMachineTestInformation::setupOrganization
//   (index.txt: virtual_11) Nothing.
void RaceMachineTestInformation::setupOrganization(RaceOrganization *org)
{
}

// FUNC 0x0018C960 RaceMachineTestInformation::setCar
//   (u049: F_0018C960) car = c.  Callers: run_machine_test, run_license_test.
void RaceMachineTestInformation::setCar(const CarEquipments &c)
{
    car = c; /* implicit: CarEquipments::operator= */
}

// FUNC 0x0018C980 RaceMachineTestInformation::setCar
//   Unreferenced (as RaceSolitaireInformation::setCar): loads the car
//   `code', then its colour code of colour id 0 (the colour table
//   g_00352D98).
void RaceMachineTestInformation::setCar(u64 code)
{
    car.setCar(code);
    car.setColorCode(g_00352D98.getColor(car.getModelCode(), 0));
}

/* ------------------------------------------------------------------------ */
/* RaceMachineTest                                                          */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018C9D0 RaceMachineTest::operator new
//   The race pool, 16-byte aligned (an out-of-line copy of RaceBase's
//   inline operator new emitted in T148).
void *RaceMachineTest::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x0018C9F8 RaceMachineTest::operator delete
//   Back to the race pool (the size, 0x48A80, is not used).  Called by
//   race_machine_test_create when the constructor throws and by the
//   deleting destructor.
void RaceMachineTest::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x0018CA20 RaceMachineTest::virtual_24
//   The class name.
const char *RaceMachineTest::virtual_24()
{
    return "RaceMachineTest";
}

// FUNC 0x0018CA30 race_machine_test_create
//   RaceClassInfo::create of "RaceMachineTest" (0x002BA298+8): new
//   RaceMachineTest(0) (size 0x48A80; operator delete if the constructor
//   throws).
RaceBase *race_machine_test_create(void)
{
    return new RaceMachineTest(MACHINE_TEST_400M); /* implicit: RaceMachineTest::operator delete (if the constructor throws) */
}

// FUNC 0x0018CA98 race_machine_test_construct
//   RaceClassInfo::construct (0x002BA298+0xC): a RaceMachineTest(0) in buf
//   (nothing if buf is NULL).  arg is not used.  The global placement new
//   of <new> (as u045's race_basic_construct): egcs's is inline and
//   throw(), so egcs tests its result for NULL itself (retail's daddu v0,a0
//   / beqz v0); GCC 14 does not test the standard form, hence the explicit
//   test there.
RaceBase *race_machine_test_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)
        return NULL;
#endif
    return ::new (buf) RaceMachineTest(MACHINE_TEST_400M);
}

/* The registry record of "RaceMachineTest" (.data, before the frame info;
 * its "RaceMachineTest" is virtual_24's literal, written once).
 * sizeof(RaceMachineTest) is 0x48A80 under egcs (EGCS_SIZE) and GCC 14's
 * own size there, what the registry's users allocate for construct. */
RaceClassInfo g_002BA298 = {                /* 0x002BA298 */
    "RaceMachineTest", sizeof(RaceMachineTest), race_machine_test_create, race_machine_test_construct
};
/* Its node in the registry (.sdata zeros, linked in at start-up by
 * race_machine_test_static_init).  Initialised: written here, before
 * virtual_8's "MMAX", as retail's (an uninitialised global would come
 * after every literal; u047's T138 has the same). */
RaceClassNode g_00351F30 = { NULL, NULL };  /* 0x00351F30 */

// FUNC 0x0018CAC0 RaceMachineTest::RaceMachineTest
//   RaceSolitaire, the kind (stored before the vptr), the vptr, then the
//   members in order: the icons (7 x 0x2044, CarIcon 0x001ADA68), the
//   ResultMachineTest (inline: done = player = 0, the vptr, speedMode = 0),
//   the retry pause (0x001A3A08), the replay pause (0x001A30C0, mode 0), the
//   display (0x001909E0) and the conductor (0x001F05F8).  The display is
//   installed and the conductor becomes RaceBase::dc; exited is set by
//   virtual_8.  (On an exception the members built so far are destroyed:
//   ~RaceDisplay, ~ReplayPause, ~RacePauseWithRetry, ~RaceSolitaire.)
//   Also called directly by run_machine_test (built in place, kind 0 M400,
//   1 M1000, else 2).
RaceMachineTest::RaceMachineTest(int kind)
    : kind(kind), replayPause(0)
{
    /* implicit: RaceSolitaire (base constructor), CarIcon, RacePauseWithRetry, ReplayPause, RaceDisplay, DynamicsConductorMachineTest (member constructors), ~RaceDisplay, ~ReplayPause, ~RacePauseWithRetry, ~RaceSolitaire (exception cleanup) */
    setRaceDisplay(&display);
    dc = &dcMachineTest;
}

// FUNC 0x0018CC80 RaceMachineTest::~RaceMachineTest
//   (index.txt: virtual_0) The members in reverse order (the display
//   0x00191300, the replay pause 0x001A31A8, the retry pause 0x001A3AB8, the
//   inline ~ResultMachineTest (only RaceResultBase's vptr); the conductor
//   and the icons have no destructor), then ~RaceSolitaire (flags 0);
//   deleting form: operator delete(this, 0x48A80).
RaceMachineTest::~RaceMachineTest()
{
    /* implicit: ~RaceDisplay, ~ReplayPause, ~RacePauseWithRetry (member destructors), ~RaceSolitaire (base destructor) */
}

// FUNC 0x0018CD30 RaceMachineTest::virtual_8
//   Race start: the mode name from the race code ("MMAX": the max speed
//   challenge, else 0-400 m, also for 0-1000 m), the result gets the kind,
//   then the base.  In a race (replay != 1) the retry pause menu and the
//   result sequence are installed; in a replay the replay pause menu (given
//   the organization) and no result sequence.
void RaceMachineTest::virtual_8(int replay)
{
    exited = 0;
    u64 code = org->raceCode;               /* read before nameToCode (retail: in s0 across the call) */
    bool max = code == nameToCode("MMAX");  /* a value (xor; sltiu), then one setRaceMode call */
    setRaceMode(max ? "RACE_MODE_MAX_SPEED_CHALLENGE" : "RACE_MODE_0_400");
    result.setKind(kind);
    RaceSolitaire::virtual_8(replay);
    if (replay != 1) {
        setPauseMenu(&retryPause);
        setSubDisplay(&result);
    } else {
        replayPause.F_001A3248(org);
        setPauseMenu(&replayPause);
        setSubDisplay(NULL);
    }
}

// FUNC 0x0018CE48 RaceMachineTest::virtual_35
//   Only RaceSolitaire::virtual_35 (the arguments passed on).
void RaceMachineTest::virtual_35()
{
    RaceSolitaire::virtual_35();
}

// FUNC 0x0018CE68 RaceMachineTest::virtual_43
//   (RacePS2Base::virtual_43 is not called.)  Builds the icons of the
//   race's cars.
void RaceMachineTest::virtual_43()
{
    car_icons_make(icons, org);
}

// FUNC 0x0018CE90 RaceMachineTest::virtual_30
//   The update while paused (as RaceFreeRun's): the pause menu's result
//   (with the player's input) 2 (timeout) ends the race (virtual_34(3)), 4
//   (exit) onExit, then resumes, 3 (continue) resumes, 5 (retry) the race
//   again (virtual_34(4)).  No NULL check of the menu.
void RaceMachineTest::virtual_30()
{
    PauseBase *menu = pauseMenu;
    switch (menu->virtual_2(virtual_25(), org)) {
    case PAUSE_RESULT_TIMEOUT:
        virtual_34(RACE_END_TIMEOUT);
        break;
    case PAUSE_RESULT_EXIT:
        onExit();
        /* fallthrough */
    case PAUSE_RESULT_CONTINUE:
        setPause(0);
        startSounds();
        break;
    case PAUSE_RESULT_RETRY:
        virtual_34(RACE_END_RETRY);
        break;
    }
}

// FUNC 0x0018CFB0 RaceMachineTest::onExit
//   (? name: the pause menu's exit, result 4; onExit would fit, as
//   RaceFreeRun's 0x001897D8.)  In the max speed challenge, once per race,
//   outside replays, when car 0 has gone faster than 1.0 and has not
//   finished: the run ends here with its top speed (the goal event 5,
//   car_finish(CAR_FINISH_RACE_END), the main input's replay record and
//   lap recording stopped, g_00351FB8 = 3, the display's finishDisplay
//   off, the result sequence's delay shortened).  Otherwise virtual_34(2):
//   the race is left.  The caller resumes the race in both cases.
//   RaceDisplay's inline ackReplayRecorded (g_00351FB8 = 3) and
//   setFinishDisplay(false) (0x00195820 / 0x00195810 are their
//   unreferenced copies): retail keeps &display in a register (the inline
//   setter's `this').
void RaceMachineTest::onExit()
{
    RaceOrganization *organization = org;
    RaceCarState *car = organization->entries->cars[0]->state;

    if (kind == MACHINE_TEST_MAX_SPEED && exited == 0 && replayMode == 0
        && car->topSpeed > 1.0f && car->st.finished == 0) {
        exited = 1;
        organization->eventQueue.post(RACE_EVENT_GOAL, 0, 0);
        car_finish(car, CAR_FINISH_RACE_END);
        getMainInput()->rec.rec.stop();
        getMainInput()->stopLap();
        RaceDisplay::ackReplayRecorded();
        display.setFinishDisplay(false);
        result.shortenDelay();
        return;
    }
    virtual_34(RACE_END_EXIT);
}

// FUNC 0x0018D0F0 race_machine_test_static_init
//   .ctors entry (0x0028F120): registers "RaceMachineTest" (g_002BA298)
//   with its node g_00351F30.
void race_machine_test_static_init(void)
{
    race_class_register(&g_00351F30, &g_002BA298);
}

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x0018D118 __tf15RaceMachineTest
//   Builds the __si_type_info at 0x004B20C0 (base: RaceSolitaire,
//   0x004B1FE0) on the first call.
extern "C" const type_info &__tf15RaceMachineTest(void)
{
    if (__ti15RaceMachineTest[0] == 0) {
        __tf13RaceSolitaire();
        __rtti_si(__ti15RaceMachineTest, "15RaceMachineTest", (const type_info *)__ti13RaceSolitaire);
    }
    return *(const type_info *)__ti15RaceMachineTest;
}
#endif

// FUNC 0x0018D168 RaceMachineTestInformation::getTransmission
/* inline in game/race/modes/T148_race_machine_test.h; egcs emits this copy (no
 * callers; u049: getField5C): transmission */

// FUNC 0x0018D170 RaceMachineTestInformation::setTransmission
/* inline in game/race/modes/T148_race_machine_test.h; egcs emits this copy (no
 * callers; u049: setField5C; run_machine_test stores the field directly):
 * transmission = v */

// FUNC 0x0018D178 RaceMachineTestInformation::getVibrartion
/* inline in game/race/modes/T148_race_machine_test.h; egcs emits this copy (no
 * callers; u049: getField58; sic, the name of RaceGTmodeInformation's
 * twin): vibration */

// FUNC 0x0018D180 RaceMachineTestInformation::setVibration
/* inline in game/race/modes/T148_race_machine_test.h; egcs emits this copy (no
 * callers; u049: setField58): vibration = v */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x0018D188 __tf26RaceMachineTestInformation
//   Builds the __si_type_info at 0x004B20B0 (base: RaceInformation,
//   gp-0x4678) on the first call.
extern "C" const type_info &__tf26RaceMachineTestInformation(void)
{
    if (__ti26RaceMachineTestInformation[0] == 0) {
        __tf15RaceInformation();
        __rtti_si(__ti26RaceMachineTestInformation, "26RaceMachineTestInformation",
                  (const type_info *)__ti15RaceInformation);
    }
    return *(const type_info *)__ti26RaceMachineTestInformation;
}
#endif
