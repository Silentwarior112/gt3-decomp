/* T142 - RaceFreeRun, RaceFreeRunInformation (u048), 0x00188FE0..0x00189998.
 *
 * The free run (time attack): a RaceSolitaire registered as "RaceFreeRun"
 * (mode name "RACE_MODE_TIME_ATTACK").  Its members: the ResultFreeRun (the
 * course's lap records), a RacePause menu (race), a ReplayPause menu, the
 * RaceFreeRunDisplay and the DynamicsConductorFreeRun (RaceBase::dc).  The
 * inputs 1..3 record in one-slot mode, so the second car can replay a ghost
 * (the second input: virtual_20 / 23 / 59).  The pause menu's exit (result
 * 4) ends the session with the result sequence once the first car is past
 * its first lap (onExit); before that the race ends at once.
 *
 * Includes (u048's data pass): the classes retail's T142 saw, in its order.
 * Its .rodata (0x0032B8C0..0x0032BDC6) is "bad_alloc" (<new>), the two
 * literals, the two vtables, then the type names in reverse definition
 * order: RaceFreeRun, RaceFreeRunInformation, ResultTermBar, ResultTitleBar,
 * ReplayPauseMenu, PauseMenu, RaceMenuSelectWindow, RaceMenuWindow,
 * RaceMenuButton, the OperatorHolder group, then T154_race_display.h's
 * (RaceMiniMap .. RaceMeterBase, RaceCameraBase, UnitArena<64>,
 * RaceValueDisplayBase, RaceDisplayBase), Operator: the race display first,
 * then the menu classes, RaceMenuButton, the windows, the pause menus and
 * the result bars.  Its .sdata (0x00351EB8..0x00351EC8) is the "" of T157's
 * inline RaceMessageDisplay::clear, then the registry node g_00351EC0 (an
 * uninitialised global: zeros at the end).
 */
// DATA_START 0x0032B8C0
// DATA_START 0x00351EB8
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (race_free_run_construct); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/race/display/T154_race_display.h"         /* RaceDisplay (first: retail's type-name order) */
#include "game/menu/framework/T216_menu_transitive.h"      /* no use: the OperatorHolder classes retail's T142 saw */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* RaceMenuButton (before the windows: retail's order) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* RaceMenuWindow, RaceMenuSelectWindow */
#include "game/race/pause/T169_race_pause.h"           /* RacePause (PauseMenu) */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (ReplayPauseMenu) */
#include "game/menu/records/T200_result_board.h"         /* no use: ResultTitleBar, ResultTermBar (seen in retail) */
#include "game/race/modes/T142_race_free_run.h"        /* own: RaceFreeRun, RaceFreeRunInformation, the registry
                                               externs; + T139 RaceSolitaire, T129 race_class_register */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, startAction, loggerMode, eventQueue) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase / RaceEntryCar (state, transmission) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post, RACE_EVENT_* */
#include "game/race/pause/T168_pause_base.h"           /* PauseBase, PAUSE_RESULT_* */
#include "game/race/core/T413_race_input.h"           /* RaceInput (rec, bestLap, hold), stopLap */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord::Manager / Recorder */
#include "game/car/physics/T302_car_state.h"            /* car_lap_display, car_finish, CAR_FINISH_* */
#include "game/debug/T290_logger_sectors.h"       /* dc_logger_start */
#include "game/race/modes/race_time_trial_internal.h"

/* ------------------------------------------------------------------------ */
/* RaceFreeRunInformation                                                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x00188FE0 RaceFreeRunInformation::RaceFreeRunInformation
//   RaceSolitaireInformation, then its vptr (+0x54).  Called by run_free_run.
RaceFreeRunInformation::RaceFreeRunInformation()
{
    /* implicit: RaceSolitaireInformation (the base's / members' constructors) */
}

// FUNC 0x00189018 RaceFreeRunInformation::~RaceFreeRunInformation
//   (index.txt: virtual_0) The vptr, then ~RaceSolitaireInformation with the
//   caller's flags (which deletes).
RaceFreeRunInformation::~RaceFreeRunInformation()
{
    /* implicit: ~RaceSolitaireInformation (the base / member destructors) */
}

// FUNC 0x00189040 RaceFreeRunInformation::getEntryCount
//   (index.txt: virtual_8) Two entries: the car and its ghost.
int RaceFreeRunInformation::getEntryCount()
{
    return 2;
}

/* ------------------------------------------------------------------------ */
/* RaceFreeRun                                                              */
/* ------------------------------------------------------------------------ */

// FUNC 0x00189048 RaceFreeRun::operator new
//   The race pool, 16-byte aligned (T142's copy of RaceBase's inline one).
void *RaceFreeRun::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00189070 RaceFreeRun::operator delete
//   The size is ignored.
void RaceFreeRun::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00189098 RaceFreeRun::virtual_24
//   The class name.
const char *RaceFreeRun::virtual_24()
{
    return "RaceFreeRun";
}

// FUNC 0x001890A8 race_free_run_create
//   RaceClassInfo::create of "RaceFreeRun": new RaceFreeRun(NULL) (operator
//   new 0x3A880; operator delete if the constructor throws).
RaceBase *race_free_run_create(void)
{
    return new RaceFreeRun(NULL); /* implicit: RaceFreeRun::operator delete (if the constructor throws) */
}

// FUNC 0x00189110 race_free_run_construct
//   RaceClassInfo::construct: a RaceFreeRun(NULL) in buf (nothing if buf is
//   NULL).  arg is not used.
RaceBase *race_free_run_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)                        /* egcs tests the result of <new>'s throw() placement
                                               new itself (retail's daddu v0,a0 / beqz v0); GCC 14
                                               does not */
        return NULL;
#endif
    return ::new (buf) RaceFreeRun(NULL);
}

/* The registry record of "RaceFreeRun" (.data, before the frame info; its
 * "RaceFreeRun" is virtual_24's literal, written once). */
RaceClassInfo g_002B8F90 = {                /* 0x002B8F90 */
    "RaceFreeRun", sizeof(RaceFreeRun), race_free_run_create, race_free_run_construct
};

// FUNC 0x00189138 RaceFreeRun::RaceFreeRun
//   RaceSolitaire, the result (inline: done = player = 0), the pause menu
//   (RacePause 0x001A2A78), the replay pause menu (ReplayPause 0x001A30C0,
//   mode 0), the display (RaceFreeRunDisplay 0x00189998) and the conductor
//   (DynamicsConductorFreeRun 0x001F1190).  Inputs 1..3 record in one-slot
//   mode; the result keeps the course name.  Unwinding: the inline
//   ~RaceFreeRunDisplay (~RaceDisplay), ~ReplayPause, ~RacePause, the inline
//   ~ResultFreeRun, ~RaceSolitaire.  Also called directly by run_free_run.
//   Each input's mode goes through the inline Manager::setMode (retail
//   stores it through &inputN.rec; u085 made it inline in T318's header).
RaceFreeRun::RaceFreeRun(const char *course)
    : replayPause(0)
{
    /* implicit: RaceSolitaire, RaceFreeRunDisplay, RacePause, ReplayPause,
     * DynamicsConductorFreeRun (the base's / members' constructors),
     * ~RaceSolitaire, ~RaceDisplay, ~RacePause, ~ReplayPause (their cleanup on
     * a throw) */
    setRaceMode("RACE_MODE_TIME_ATTACK");
    setRaceDisplay(&display);
    input1.rec.setMode(RECORD_MODE_ONE_SLOT);
    input2.rec.setMode(RECORD_MODE_ONE_SLOT);
    dc = &dcFreeRun;
    input3.rec.setMode(RECORD_MODE_ONE_SLOT);
    result.setCourse(course);
}

// FUNC 0x00189318 RaceFreeRun::~RaceFreeRun
//   (index.txt: virtual_0) The display (RaceFreeRunDisplay's destructor
//   inline: its vptr, then ~RaceDisplay flags 2), the replay pause, the
//   pause, the inline ~ResultFreeRun, RaceSolitaire; deleting form:
//   operator delete(this, 0x3A880).
RaceFreeRun::~RaceFreeRun()
{
    /* implicit: ~RaceSolitaire, ~RaceDisplay, ~RacePause, ~ReplayPause (the
     * base / member destructors) */
}

// FUNC 0x001893D8 RaceFreeRun::virtual_8
//   Race start (exited = 0, then RaceSolitaire's).  The second input, when
//   held, goes back to the start of its record.  A race gets the pause menu
//   and the result sequence; a replay the replay pause menu (told the
//   organization), no result and, when inputs 2 / 3 are used outside the
//   logger, virtual_56 and no start action.
void RaceFreeRun::virtual_8(int replay)
{
    exited = 0;
    RaceSolitaire::virtual_8(replay);
    RaceOrganization *o = org;
    RaceInput *second = getSecondInput();
    if (second->hold != 0)
        second->rec.selectSlot(0);
    if (replay != RACE_REPLAY) {
        setPauseMenu(&pause);
        setSubDisplay(&result);
        return;
    }
    replayPause.F_001A3248(o);
    setPauseMenu(&replayPause);
    setSubDisplay(NULL);
    if (field_217A8 != 0 && o->loggerMode == 0) {
        virtual_56();
        o->startAction = -1;
    }
}

// FUNC 0x001894E8 RaceFreeRun::virtual_56
//   RaceSolitaire's, then the logger starts car 0 and, if the second input
//   is not held, car 1 (dc_logger_start).
void RaceFreeRun::virtual_56()
{
    RaceSolitaire::virtual_56();
    dc_logger_start(dc, 0);
    if (getSecondInput()->hold == 0)
        dc_logger_start(dc, 1);
}

// FUNC 0x00189538 RaceFreeRun::virtual_23
//   (RaceSolitaire's is not called.)  input2 takes the pad's best lap
//   record (slot 0) and plays; firstTransmission = the first car's transmission.
//   The pad's input through RaceSolitaire::getPadInput (inline in T139's
//   header since u047's pass): retail forms &input0 as a pointer (this +
//   0xC8C0, then +0x124).
void RaceFreeRun::virtual_23()
{
    RaceInput *in = &input2;
    in->rec.selectSlot(0);
    in->rec.copySlotFrom(getPadInput()->bestLap);
    in->hold = 0;
    if (org != NULL)
        firstTransmission = org->entries->cars[0]->transmission;
}

// FUNC 0x001895C0 RaceFreeRun::virtual_20
//   The second input loads the control record that follows blk (one slot)
//   and plays.
void RaceFreeRun::virtual_20(RaceInfoBlock *blk)
{
    RaceInput *second = getSecondInput();
    second->rec.load(race_info_block_records(blk), 0);
    second->hold = 0;
}

// FUNC 0x00189608 RaceFreeRun::virtual_59
//   virtual_57 (the replay memory), then which == 0: input2 loads the
//   record after blk and plays (field_217A8 = 1: inputs 2 / 3 from now
//   on); else virtual_19(blk) and virtual_20(blk).
void RaceFreeRun::virtual_59(int which, RaceInfoBlock *blk)
{
    virtual_57();
    if (which == 0) {
        field_217A8 = 1;
        RaceInput *in = &input2;
        in->rec.load(race_info_block_records(blk), 0);
        in->hold = 0;
    } else {
        virtual_19(blk);
        virtual_20(blk);
    }
}

// FUNC 0x001896B8 RaceFreeRun::virtual_30
//   The update while paused (RaceBasic::virtual_30 with another case 4):
//   the menu's timeout ends the race (virtual_34(3)), exit runs onExit and
//   resumes, continue resumes, retry ends it (virtual_34(4)).  No NULL
//   check of the menu.
void RaceFreeRun::virtual_30()
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

// FUNC 0x001897D8 RaceFreeRun::onExit
//   ? name: it handles the pause menu's exit (result 4).  Once per race,
//   outside replays, when the first car shows lap 2 or later: the goal
//   event (the result sequence shows the lap record),
//   car_finish(CAR_FINISH_AUTO), the main input's replay record and lap
//   record stopped, g_00351FB8 = 3 (event 0x16 seen: no "replay recorded"
//   message; RaceDisplay's inline static ackReplayRecorded, its copy
//   0x00195820); the caller resumes the race.  Otherwise virtual_34(2):
//   the race ends at once.
void RaceFreeRun::onExit()
{
    RaceOrganization *o = org;
    RaceCarState *car = o->entries->cars[0]->state;
    if (exited != 0 || replayMode != 0 || car_lap_display(car) < 2)
        virtual_34(RACE_END_EXIT);          /* first: retail's fall-through block */
    else {
        exited = 1;
        o->eventQueue.post(RACE_EVENT_GOAL, 0, 0);
        car_finish(car, CAR_FINISH_AUTO);
        getMainInput()->rec.rec.stop();
        getMainInput()->stopLap();
        RaceDisplay::ackReplayRecorded();
    }
}

/* RaceFreeRun's node in the registry (.sdata zeros, linked in at start-up). */
RaceClassNode g_00351EC0;                   /* 0x00351EC0 */

// FUNC 0x001898D0 race_free_run_static_init
//   .ctors entry (0x0028F114): registers "RaceFreeRun" (g_002B8F90) with
//   its node g_00351EC0.
void race_free_run_static_init(void)
{
    race_class_register(&g_00351EC0, &g_002B8F90);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001898F8 __tf11RaceFreeRun
//   Builds the __si_type_info __ti11RaceFreeRun (base RaceSolitaire) on the
//   first call.
extern "C" const type_info &__tf11RaceFreeRun(void)
{
    if (__ti11RaceFreeRun[0] == 0) {
        __tf13RaceSolitaire();
        __rtti_si(__ti11RaceFreeRun, "11RaceFreeRun", (const type_info *)__ti13RaceSolitaire);
    }
    return *(const type_info *)__ti11RaceFreeRun;
}

// FUNC 0x00189948 __tf22RaceFreeRunInformation
//   Builds the __si_type_info __ti22RaceFreeRunInformation (base
//   RaceSolitaireInformation) on the first call.
extern "C" const type_info &__tf22RaceFreeRunInformation(void)
{
    if (__ti22RaceFreeRunInformation[0] == 0) {
        __tf24RaceSolitaireInformation();
        __rtti_si(__ti22RaceFreeRunInformation, "22RaceFreeRunInformation",
                  (const type_info *)__ti24RaceSolitaireInformation);
    }
    return *(const type_info *)__ti22RaceFreeRunInformation;
}
#endif
