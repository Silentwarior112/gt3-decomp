/* T147 - RaceLicenseDisplay (u049).
 *
 * The race display of a licence test (RaceLicense+0x229C8): the normal
 * RaceDisplay plus the result sequence of LicenseExaminationMessages
 * (T146).  update (virtual_2) runs phase[0]:
 *   0  racing: event 9 (the test is failed) -> 3; a result > 0 in the
 *      organization (result.rank: the trophy) -> 1;
 *   1  passed: the pass sequence (drawn with renderWhenPassing), then 2: updatePass
 *      every frame (0.1 s, the messages (at most 30 s), then 5 s or the pad
 *      button 0x40);
 *   3  failed: the BGM event 0x14 (code 0x20), then 4: updateFail every
 *      frame (1 s, the messages ("time over" and the time if result.rank is
 *      0, else "disqualified"; at most 30 s), then 0.8 s);
 *   5  the end, then 6: endExamination (event 1: the race ends).
 * phase[1] is the step of updatePass / updateFail.  The panel setters are
 * empty: a licence test shows none of the race panels.
 *
 * Data (u049's data pass): retail's .rodata 0x0032C9C0..0x0032CBD2 is
 * "bad_alloc" (<new>), virtual_2's jump table, the vtable, then the type
 * names RaceLicenseDisplay, ExaminationDisplayer, CallBackWhenPrizeRendering,
 * PrizeModelSetContext (T147's header: T146's, whose LicensePrize and
 * LicenseExaminationMessages have their key functions in T146), RaceMiniMap
 * .. RaceMeterBase, RaceValueDisplayBase, RaceDisplayBase (T154),
 * RaceCameraBase, UnitArena<64>: in reverse definition order UnitArena<64>
 * (T153, emitted) and RaceCameraBase (T383) first, then the race display,
 * then T147's header.  Its .sdata (0x00351F20) is the "" of T157's inline
 * RaceMessageDisplay::clear (-G8: retail's "" is not in T147's .rodata,
 * whose jump table follows "bad_alloc" at 0x0032C9D0).  Neither is
 * referenced: anchors.
 */
// DATA_START 0x0032C9C0
// DATA_START 0x00351F20
#include "types.h"
#include <new>                              /* "bad_alloc" (retail's first .rodata string) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si (egcs's <new> and cxxrt.h conflict) */
#endif
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, the last one */
#include "game/race/core/T153_race_course.h"
#include "game/camera/T383_race_camera_base.h"     /* (RaceCameraBase: defined before the race display) */
#include "game/race/display/T154_race_display.h"         /* base RaceDisplay */
#include "game/race/display/T147_race_license_display.h"
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (result, eventQueue) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::find / post, RACE_EVENT_* */
#include "game/data/messages/T186_msg_race_display.h"     /* the race texts g_002C43E8 */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn */
#include "game/race/display/race_license_internal.h"

// FUNC 0x0018BE40 RaceLicenseDisplay::RaceLicenseDisplay
//   RaceDisplay, the vptr, the messages (0x0018AF98), timer = -1, then
//   reset() (which sets timer to 0 again).  On an exception the members
//   built so far are destroyed (~LicenseExaminationMessages, ~RaceDisplay).
//   Caller: RaceLicense's constructor.
//   97.8%: retail computes `this + 0x8000' before the timer's store, egcs
//   after it (a sched1 tie: every order of the four stores, in the body or
//   the initialiser list, and FrameTimer() / FrameTimer(int) tried; also
//   991111-01).
RaceLicenseDisplay::RaceLicenseDisplay()
    : timer(-1)
{
    /* implicit: RaceDisplay (base constructor), LicenseExaminationMessages (member constructor), ~LicenseExaminationMessages, ~RaceDisplay (exception cleanup) */
    phase[1] = 0;
    input = NULL;
    passed = 0;
    reset();
}

// FUNC 0x0018BEF8 RaceLicenseDisplay::~RaceLicenseDisplay
//   (index.txt: RaceLicenseDisplay::virtual_21) The vptr, the messages'
//   destructor (~LicenseExaminationMessages, flags 2), then ~RaceDisplay
//   with the caller's flags (the deleting form is there).
RaceLicenseDisplay::~RaceLicenseDisplay()
{
    /* implicit: ~LicenseExaminationMessages (member destructor), ~RaceDisplay (base destructor) */
}

// FUNC 0x0018BF50 RaceLicenseDisplay::reset
//   The messages are not shown, timer 0, phase 0 / step 0.  Retail stores
//   `active' through &msgs in a register: an inline member's `this', i.e.
//   ExaminationDisplayer's inline deactivate, not LicenseExaminationMessages'
//   (which also hides the prize and is called directly elsewhere).
//   68.2%: retail's stores are msgs, timer, phase[1], phase[0] with the
//   two addresses in v1 / a1; every order of the four statements (with
//   setSubPhase, a local reference or pointer to msgs, timer = 0 through an
//   operator=) gives other registers or another order (991111-01 too).
void RaceLicenseDisplay::reset()
{
    timer.set(0);
    msgs.ExaminationDisplayer::deactivate();
    phase[0] = 0;
    phase[1] = 0;
}

// FUNC 0x0018BF80 RaceLicenseDisplay::setPhase
void RaceLicenseDisplay::setPhase(int i, u8 v)
{
    phase[i] = v;
}

// FUNC 0x0018BF98 RaceLicenseDisplay::updatePass
//   One frame of the pass: a 6-frame wait; the messages are set up with the
//   organization's result (only if it is > 0), its time, innerPanel() as
//   `report', no text; then the pass sequence (slots 0 and 2, through the
//   vtable) until finished or 1800 frames; then 300 more frames (or the
//   pad's button 0x40) before the messages are hidden and phase 5.
//   Retail computes &msgs in each case (a local per case) and keeps
//   &organization->result in a register across innerPanel() (a reference).
void RaceLicenseDisplay::updatePass(RaceOrganization *organization)
{
    switch (phase[1]) {
    case 0:
        timer.set(6);
        phase[1]++;
        /* fallthrough */
    case 1: {
        if (timer.get() > 0)
            break;
        timer.set(1800);
        RaceResult &result = organization->result;
        if (result.rank > 0)
            msgs.setup(result.rank, result.time, innerPanel(), NULL, true);
        phase[1]++;
        break;
    }
    case 2: {
        ExaminationDisplayer *displayer = &msgs;
        displayer->updateWhenPassing(*organization);
        displayer->updateAlways();
        if (displayer->finished || timer.isOver()) {
            timer.set(300);
            phase[1]++;
        }
        break;
    }
    case 3: {
        ExaminationDisplayer *displayer = &msgs;
        displayer->updateWhenPassing(*organization);
        displayer->updateAlways();
        int skipped = 0;
        if (input != NULL) {                /* retail: li 0; movn 1 (other forms: sra / andi or movz) */
            if (!(input->getButtonsOn() & LICENSE_SKIP_BUTTON))
                skipped = 0;
            else
                skipped = 1;
        }
        if (skipped || timer.isOver()) {
            msgs.deactivate();              /* a direct call */
            setPhase(0, 5);
        }
        break;
    }
    }
    timer.tick();
}

// FUNC 0x0018C1E0 RaceLicenseDisplay::updateFail
//   One frame of the failure: a 60-frame wait; the messages are set up as
//   failed (-1) with "time over" and the organization's time when its
//   result is 0, else "disqualified" without a time line; then the fail
//   sequence (slots 1 and 2, through the vtable) until finished or 1800
//   frames, the messages are hidden; 48 frames later phase 5.  The text is
//   read before innerPanel() (retail keeps it in s2 across the call).
void RaceLicenseDisplay::updateFail(RaceOrganization *organization)
{
    switch (phase[1]) {
    case 0:
        timer.set(60);
        phase[1]++;
        /* fallthrough */
    case 1: {
        if (timer.get() > 0)
            break;
        timer.set(1800);
        RaceResult &result = organization->result;
        const char *text;
        if (result.rank == LICENSE_RESULT_TIME_OVER) {
            text = g_002C43E8[RACEDISPLAY_EXAM_TIME_OVER];          /* 0x002C4404 */
            msgs.setup(LICENSE_RESULT_FAILED, result.time, innerPanel(), text, true);
        } else {
            text = g_002C43E8[RACEDISPLAY_EXAM_FAIL];               /* 0x002C4400 */
            msgs.setup(LICENSE_RESULT_FAILED, 0, innerPanel(), text, false);
        }
        phase[1]++;
        break;
    }
    case 2: {
        ExaminationDisplayer *displayer = &msgs;
        displayer->updateWhenDisqualifying(*organization);
        displayer->updateAlways();
        if (displayer->finished || timer.isOver()) {
            timer.set(48);
            msgs.deactivate();              /* a direct call */
            phase[1]++;
        }
        break;
    }
    case 3:
        if (timer.isOver())
            setPhase(0, 5);
        break;
    }
    timer.tick();
}

// FUNC 0x0018C400 RaceLicenseDisplay::endExamination
//   ? name.  Back to phase 0 and posts event 1 (the end of the race).
void RaceLicenseDisplay::endExamination(RaceOrganization *organization)
{
    setPhase(0, 0);
    phase[1] = 0;
    organization->eventQueue.post(RACE_EVENT_END, 0, 0);
}

// FUNC 0x0018C458 RaceLicenseDisplay::virtual_2
//   (index.txt: virtual_2) The update: RaceDisplay's, then phase[0] (see
//   the head of the file).  In phase 0 event 9 is taken from the queue;
//   a result > 0 wins over it (and clears unkA470).
void RaceLicenseDisplay::virtual_2()
{
    RaceDisplay::virtual_2();
    RaceOrganization *organization = org;

    switch (phase[0]) {
    case 0:
        if (organization->eventQueue.find(RACE_EVENT_LICENSE_FAILED, NULL, 1) != 0)
            setPhase(0, 3);
        if (organization->result.rank > 0) {
            unkA470 = 0;
            setPhase(0, 1);
        }
        break;
    case 1:
        passed = 1;
        setPhase(0, 2);
        phase[1] = 0;
        /* fallthrough */
    case 2:
        updatePass(organization);
        break;
    case 3:
        passed = 0;
        setPhase(0, 4);
        phase[1] = 0;
        organization->eventQueue.post(RACE_EVENT_BGM, BGM_CODE_FAILED, 0);
        /* fallthrough */
    case 4:
        updateFail(organization);
        break;
    case 5:
        setPhase(0, 6);
        phase[1] = 0;
        /* fallthrough */
    case 6:
        endExamination(organization);
        break;
    }
}

// FUNC 0x0018C5C0 RaceLicenseDisplay::virtual_3
//   (index.txt: virtual_3) The draw: RaceDisplay's, then (inner panel on
//   and the messages shown) the pass screen (renderWhenPassing) or the fail screen
//   (slot 4), through the vtable.  passed is read before active is tested
//   (retail: its load in the delay slot of that test's branch).
void RaceLicenseDisplay::virtual_3(GSBuffer &buf)
{
    RaceDisplay::virtual_3(buf);
    if (!innerPanel())
        return;
    ExaminationDisplayer *displayer = &msgs;
    u8 pass = passed;
    if (displayer->active == 0)
        return;
    if (pass)
        displayer->renderWhenPassing(buf);
    else
        displayer->renderWhenDisqualifying(buf);
}

// FUNC 0x0018C660 RaceLicenseDisplay::virtual_0
//   (index.txt: virtual_0) RaceDisplay::virtual_0 (the arguments passed
//   on), then reset().
void RaceLicenseDisplay::virtual_0(RaceOrganization *organization, int raceMode, int replayMode)
{
    RaceDisplay::virtual_0(organization, raceMode, replayMode);
    reset();
}

// FUNC 0x0018C690 RaceLicenseDisplay::virtual_16
//   (index.txt: virtual_16) RaceDisplay's version is not called.  Only
//   when replay == 1: the BGM event 0x14 (code 0x13, the finish) and event
//   8 (the time limit stops); car / myCar are not read.
void RaceLicenseDisplay::virtual_16(RaceOrganization *organization, int car, int myCar)
{
    if (replay == 1) {
        organization->eventQueue.post(RACE_EVENT_BGM, BGM_CODE_FINISH, 0);
        organization->eventQueue.post(RACE_EVENT_STOP_TIMER, 0, 0);
    }
}

// FUNC 0x0018C6E8 RaceLicenseDisplay::showLapTime
//   (index.txt: virtual_20) RaceDisplay's (the time on the event board),
//   only when replay != 0.
void RaceLicenseDisplay::showLapTime(int time)
{
    if (replay != 0)
        RaceDisplay::showLapTime(time);
}

// FUNC 0x0018C710 RaceLicenseDisplay::setCenterPanel
void RaceLicenseDisplay::setCenterPanel(bool on)
{
}

// FUNC 0x0018C718 RaceLicenseDisplay::setBottomRightPanel
void RaceLicenseDisplay::setBottomRightPanel(bool on)
{
}

// FUNC 0x0018C720 RaceLicenseDisplay::setBottomLeftPanel
void RaceLicenseDisplay::setBottomLeftPanel(bool on)
{
}

// FUNC 0x0018C728 RaceLicenseDisplay::setTopRightPanel
void RaceLicenseDisplay::setTopRightPanel(bool on)
{
}

// FUNC 0x0018C730 RaceLicenseDisplay::setTopLeftPanel
void RaceLicenseDisplay::setTopLeftPanel(bool on)
{
}

// FUNC 0x0018C738 RaceLicenseDisplay::setInput
/* inline in game/race/display/T147_race_license_display.h; egcs emits this copy (no
 * callers; RaceLicense's constructor expands it): input = c */

// FUNC 0x0018C748 RaceLicenseDisplay::getSubPhase
/* inline in game/race/display/T147_race_license_display.h; egcs emits this copy (no
 * callers): phase[1] */

// FUNC 0x0018C758 RaceLicenseDisplay::nextSubPhase
/* inline in game/race/display/T147_race_license_display.h; egcs emits this copy (no
 * callers): phase[1]++ */

// FUNC 0x0018C770 RaceLicenseDisplay::setSubPhase
/* inline in game/race/display/T147_race_license_display.h; egcs emits this copy (no
 * callers): phase[1] = v */

// FUNC 0x0018C780 RaceLicenseDisplay::getPhase
/* inline in game/race/display/T147_race_license_display.h; egcs emits this copy (no
 * callers): phase[i] */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x0018C798 __tf18RaceLicenseDisplay
//   Builds the __si_type_info at 0x004B20A0 (base: RaceDisplay, 0x004B21D0)
//   on the first call.
extern "C" const type_info &__tf18RaceLicenseDisplay(void)
{
    if (__ti18RaceLicenseDisplay[0] == 0) {
        __tf11RaceDisplay();
        __rtti_si(__ti18RaceLicenseDisplay, "18RaceLicenseDisplay", (const type_info *)__ti11RaceDisplay);
    }
    return *(const type_info *)__ti18RaceLicenseDisplay;
}
#endif
