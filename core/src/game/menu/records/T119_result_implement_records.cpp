/* T119: ResultImplementRecords - base of the result screens that look up
 * the place the player's result takes in a record table of the GameStatus
 * (machine test, time trial, free run).
 *
 * finish (the player's car reached the goal) asks checkRecords() for the
 * place (the find functions of T415..T417 only search: the table is not
 * changed here); render() shows the place ("rank" font, race_rank_name) and
 * a blinking title ("NEW RECORD" / "RECORD TIME"), then drawRecord() the
 * value.  After `delay' seconds the pad's decide button or the end of
 * `timer' posts RACE_EVENT_END.
 */
/* Retail's .rodata (0x00326440..0x0032665E): "bad_alloc" (<new>'s inline
 * what(), written when the header is read; unreferenced: the anchor),
 * "sw_20#1*130,130", the vtable, then the type names in the reverse order
 * of the classes' definitions: ResultImplementRecords, ResultTermBar,
 * ResultTitleBar, RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton,
 * OperatorList, List / Cell<OperatorHolder>, OperatorHolder, RaceCameraBase,
 * UnitArena<64>, Operator (T117's set: the original saw the result board). */
// DATA_START 0x00326440
#include <new>
#include "game/menu/framework/T217_operator.h"             /* (seen) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen, not used) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (seen, not used) */
#include "game/menu/records/T200_result_board.h"         /* (seen, not used) */
#include "game/menu/records/T116_race_result_base.h"     /* base RaceResultBase */
#include "game/menu/records/T119_result_implement_records.h"
#include "engine/pgl/G001_flare.h"                /* FONT_ALIGN_* (Font::setAlign) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (lapTime) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, eventQueue) */
#include "game/race/core/T126_race_lap_time.h"        /* RaceLapTime (total) */
#include "game/race/display/T158_race_display_draw.h"    /* race_rank_name ("1st" .. "12th") */
#include "game/data/messages/T186_msg_race_display.h"     /* g_002C43E8, the RACEDISPLAY texts */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn, CONTROL_DECIDE */
#include "game/common/T494_text_utils.h"           /* format_time */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "game/menu/records/results_internal.h"                           /* RESULT_FRAME_TIME, RESULT_*_COLOR */

// FUNC 0x001759C8 ResultImplementRecords::start
//   enabled unless the race is a replay; 5 s delay, 10 s timer.
void ResultImplementRecords::start(RaceOrganization &org, int replay)
{
    RaceResultBase::start(org, replay);
    enabled = (replay == 0);
    delay = 5.0f;
    timer = 10.0f;
    recordPlace = 0;
    blink = 0;
}

// FUNC 0x00175A20 ResultImplementRecords::cleanup
//   Nothing.
void ResultImplementRecords::cleanup()
{
}

// FUNC 0x00175A28 ResultImplementRecords::finish
//   The player finished: RaceResultBase::finish (done = 1), then the place
//   of the result.
void ResultImplementRecords::finish(RaceOrganization &org)
{
    if (enabled != 0) {
        RaceResultBase::finish(org);
        recordPlace = checkRecords(org);
    }
}

// FUNC 0x00175A80 ResultImplementRecords::update
//   Per frame, once done: first the delay runs out, then the decide button
//   or a negative timer posts RACE_EVENT_END (every frame from then on).
void ResultImplementRecords::update(RaceOrganization &org, Control *pad)
{
    RaceResultBase::update(org, pad);
    if (enabled == 0 || done == 0)
        return;
    if (0.0f < delay) {
        delay = delay - RESULT_FRAME_TIME;
        return;
    }
    if ((pad->getButtonsOn() & CONTROL_DECIDE) || timer < 0.0f)
        org.eventQueue.post(RACE_EVENT_END, 0, 0);
    timer = timer - RESULT_FRAME_TIME;
}

// FUNC 0x00175B50 ResultImplementRecords::drawShadowText
//   Prints `text' (as a format) twice with the system font: in `shadow' at
//   (x + dx, y + dy), then in `color' at (x, y).  `this' is unused.
void ResultImplementRecords::drawShadowText(const char *text, int x, int y, int dx, int dy,
                                            u32 color, u32 shadow)
{
    g_002F1968.setColor(shadow);
    g_002F1968.setPos(x + dx, y + dy);
    g_002F1968.print(text);
    g_002F1968.setColor(color);
    g_002F1968.setPos(x, y);
    g_002F1968.print(text);
}

// FUNC 0x00175C10 ResultImplementRecords::getRecordTitle
//   Place 0 is a new record.
const char *ResultImplementRecords::getRecordTitle()
{
    if (recordPlace == 0)
        return g_002C43E8[RACEDISPLAY_NEW_RECORD];
    return g_002C43E8[RACEDISPLAY_RECORD_TIME];
}

// FUNC 0x00175C30 ResultImplementRecords::render
//   Nothing until the delay is over; the place and the title only for
//   places 0..11 (unsigned compare: -1, no place, skips them); then
//   drawRecord.  The title alternates every frame between orange and grey.
void ResultImplementRecords::render(GSBuffer &buf, RaceOrganization &org)
{
    if (enabled == 0)
        return;
    if (0.0f < delay)
        return;
    if ((u32)recordPlace < 12) {
        g_002F1968.setAlign(FONT_ALIGN_CENTER);
        g_002F1968.setSpacing(4.0f, 0.0f);
        g_002F1968.setStyle("rank");
        drawShadowText(race_rank_name(recordPlace + 1), 320, 150, 3, 3,
                       RESULT_TEXT_COLOR, RESULT_SHADOW_COLOR);
        g_002F1968.setSpacing(0.0f, 0.0f);
        blink = blink ^ 1;
        g_002F1968.setStyle("sw_20");
        g_002F1968.setPos(320, 200);
        if (blink != 0)
            g_002F1968.setColor(0.8f, 0.4f, 0.0f, 1.0f);
        else
            g_002F1968.setColor((u32)RESULT_TITLE_COLOR);
        g_002F1968.print(getRecordTitle());
    }
    drawRecord(org);
}

// FUNC 0x00175DC0 ResultImplementRecords::drawRecord
//   "Lap" on the left, the time getRecord() returns on the right.
void ResultImplementRecords::drawRecord(RaceOrganization &org)
{
    char text[0x40];

    format_time(text, getRecord(org));
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
    g_002F1968.setStyle("sw_16#1");
    drawShadowText(g_002C43E8[RACEDISPLAY_LAP_COUNT_LABEL], 200, 270, 2, 2,
                   RESULT_TEXT_COLOR, RESULT_SHADOW_COLOR);
    g_002F1968.setAlign(FONT_ALIGN_RIGHT);
    g_002F1968.setStyle("sw_20#1*130,130");
    drawShadowText(text, 440, 270, 2, 2, RESULT_TEXT_COLOR, RESULT_SHADOW_COLOR);
}

// FUNC 0x00175EA0 ResultImplementRecords::getRecord
//   The player's race time.
int ResultImplementRecords::getRecord(RaceOrganization &org)
{
    return org.entries->cars[player]->lapTime.total;
}

// FUNC 0x00175EC0 ResultImplementRecords::checkRecords
/* inline in game/menu/records/T119_result_implement_records.h (-1: no record table);
 * egcs emits this copy for the vtable (slot 8) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00175EC8 __tf22ResultImplementRecords
//   Builds the __si_type_info __ti22ResultImplementRecords (name 0x003264C8,
//   base __ti14RaceResultBase, gp-0x4698) on the first call.  Callers:
//   __tf17ResultMachineTest, __tf15ResultTimeTrial, __tf13ResultFreeRun.
extern "C" const type_info &__tf22ResultImplementRecords(void)
{
    if (__ti22ResultImplementRecords[0] == 0) {
        __tf14RaceResultBase();
        __rtti_si(__ti22ResultImplementRecords, "22ResultImplementRecords",
                  (const type_info *)__ti14RaceResultBase);
    }
    return *(const type_info *)__ti22ResultImplementRecords;
}
#endif

// FUNC 0x00175F18 ResultImplementRecords::~ResultImplementRecords
/* implicit (game/menu/records/T119_result_implement_records.h declares none); egcs emits
 * this copy for the vtable: ~RaceResultBase inlined (vptr
 * _vt$14RaceResultBase), operator delete(this) when flags & 1 */
/* implicit: operator delete (flags & 1) */
