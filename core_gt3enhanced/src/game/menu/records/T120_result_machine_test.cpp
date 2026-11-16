/* T120: ResultMachineTest - the result screen of a machine test (held by
 * RaceMachineTest at +0x2F9E0).  The max speed test (MACHINE_TEST_MAX_SPEED)
 * looks up the car's top speed (RaceCarState::topSpeed), the others the
 * race time, in the machine test records of the GameStatus (T415, table
 * `kind').
 */
/* Retail's .rodata (0x00326660..0x0032695E): "bad_alloc" (<new>'s inline
 * what(), written when the header is read; unreferenced: the anchor),
 * "sw_20#1*130,130", the vtable, then the type names in the reverse order
 * of the classes' definitions: ResultMachineTest, ResultTermBar,
 * ResultTitleBar, RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton,
 * OperatorList, List / Cell<OperatorHolder>, OperatorHolder, RaceCameraBase,
 * UnitArena<64>, GarageWheelList .. Cell<GarageCar> (the GameStatus header
 * read first), Operator. */
// DATA_START 0x00326660
#include <new>
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus (machineTest) */
#include "game/menu/framework/T217_operator.h"             /* (seen) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen, not used) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (seen, not used) */
#include "game/menu/records/T200_result_board.h"         /* (seen, not used) */
#include "game/menu/records/T116_race_result_base.h"     /* RaceResultBase */
#include "game/menu/records/T119_result_implement_records.h" /* base ResultImplementRecords */
#include "game/menu/records/T120_result_machine_test.h"
#include "engine/pgl/G001_flare.h"                /* FONT_ALIGN_* (Font::setAlign) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (state, lapTime) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries) */
#include "game/race/core/T126_race_lap_time.h"        /* RaceLapTime (total) */
#include "game/data/messages/T186_msg_race_display.h"     /* g_002C43E8, the RACEDISPLAY texts */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState::topSpeed */
#include "game/save/T415_machine_test_records.h" /* MachineTestRecords::findTime / findSpeed */
#include "game/common/T494_text_utils.h"           /* format_speed */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "game/menu/records/results_internal.h"                           /* RESULT_*_COLOR */

// FUNC 0x00175F48 ResultMachineTest::setKind
//   Caller: RaceMachineTest::virtual_8.  The max speed test switches to
//   speed mode (never back here).
void ResultMachineTest::setKind(int k)
{
    kind = k;
    if (k == MACHINE_TEST_MAX_SPEED)
        speedMode = 1;
}

// FUNC 0x00175F68 ResultMachineTest::shortenDelay
//   Caller: RaceMachineTest::onExit: the screen reacts after 0.1 s instead
//   of 5 s.
void ResultMachineTest::shortenDelay()
{
    delay = 0.1f;
}

// FUNC 0x00175F78 ResultMachineTest::checkRecords
//   The place the top speed (speed mode) or the race time would take in
//   table `kind' (-1: none).
int ResultMachineTest::checkRecords(RaceOrganization &org)
{
    RaceEntryCar *car = org.entries->cars[player];
    if (speedMode != 0) {
        f32 speed = car->state->topSpeed;
        return getGameStatus()->machineTest.findSpeed(kind, speed);
    }
    u32 time = car->lapTime.total;
    return getGameStatus()->machineTest.findTime(kind, time);
}

// FUNC 0x00176018 ResultMachineTest::getRecordTitle
//   Speed mode: "NEW RECORD" for place 0, else "RECORD SPEED"; otherwise the
//   base's.
const char *ResultMachineTest::getRecordTitle()
{
    if (speedMode == 0)
        return ResultImplementRecords::getRecordTitle();
    if (recordPlace == 0)
        return g_002C43E8[RACEDISPLAY_NEW_RECORD];
    return g_002C43E8[RACEDISPLAY_RECORD_SPEED];
}

// FUNC 0x00176060 ResultMachineTest::drawRecord
//   Speed mode: "Max Speed" and the car's top speed (format_speed: in the
//   user's unit); otherwise the base's ("Lap" and the time).
void ResultMachineTest::drawRecord(RaceOrganization &org)
{
    char text[0x40];

    if (speedMode == 0) {
        ResultImplementRecords::drawRecord(org);
        return;
    }
    RaceCarState *state = org.entries->cars[player]->state;
    format_speed(text, state->topSpeed);
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
    g_002F1968.setStyle("sw_16#1");
    drawShadowText(g_002C43E8[RACEDISPLAY_MAX_SPEED], 160, 270, 2, 2,
                   RESULT_TEXT_COLOR, RESULT_SHADOW_COLOR);
    g_002F1968.setAlign(FONT_ALIGN_RIGHT);
    g_002F1968.setStyle("sw_20#1*130,130");
    drawShadowText(text, 480, 270, 2, 2, RESULT_TEXT_COLOR, RESULT_SHADOW_COLOR);
}

// FUNC 0x00176160 ResultMachineTest::ResultMachineTest
/* inline in game/menu/records/T120_result_machine_test.h; egcs emits this copy (no
 * callers): the inline RaceResultBase constructor (done = player = 0), vptr
 * _vt$17ResultMachineTest, speedMode = 0 (ResultImplementRecords' implicit
 * constructor stores no vptr of its own) */

/* egcs synthesizes the typeinfo function itself (the header declares it for
 * GCC 14 only).  Its name "17ResultMachineTest" follows the vtable as in
 * retail once the GameStatus header (the garage classes' names) is read
 * first (ref/data/u041/notes.md). */
#ifndef BOOT_OLD_GCC
// FUNC 0x00176180 __tf17ResultMachineTest
//   Builds the __si_type_info __ti17ResultMachineTest (name 0x003266E8, base
//   __ti22ResultImplementRecords) on the first call.
extern "C" const type_info &__tf17ResultMachineTest(void)
{
    if (__ti17ResultMachineTest[0] == 0) {
        __tf22ResultImplementRecords();
        __rtti_si(__ti17ResultMachineTest, "17ResultMachineTest",
                  (const type_info *)__ti22ResultImplementRecords);
    }
    return *(const type_info *)__ti17ResultMachineTest;
}
#endif

// FUNC 0x001761D0 ResultMachineTest::~ResultMachineTest
/* implicit (game/menu/records/T120_result_machine_test.h declares none); egcs emits this
 * copy for the vtable: ~RaceResultBase inlined (vptr _vt$14RaceResultBase),
 * operator delete(this) when flags & 1 */
/* implicit: operator delete (flags & 1) */
