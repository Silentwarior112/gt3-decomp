/* T132 - RaceBasic, RaceBasicWithRaceDisplay (u045).
 *
 * RaceBasic is the registered race mode "RaceBasic" (mode name
 * "RACE_MODE_SINGLE"): a RacePS2Base with three optional parts the derived
 * modes plug in -
 *   +0xC880 a pause menu (PauseBase): START (CONTROL_START of the newly
 *           pressed buttons) opens it when there is one (in a demo it posts
 *           RACE_EVENT_END instead); while paused virtual_30 runs the menu,
 *           whose timeout / exit / retry end the race through virtual_34
 *           and continue resumes it;
 *   +0xC884 the result sequence of the mode (RaceResultBase), started /
 *           updated / drawn with the race;
 *   +0xC888 the race display (RaceDisplayBase), started with the mode id.
 * RaceBasicWithRaceDisplay embeds a RaceDisplay at +0xC8C0 and makes it the
 * race display.
 *
 * Includes (u045's data pass): the classes retail's T132 saw, in its order.
 * Its .rodata (0x003282E8..0x003287A8) is "bad_alloc" (<new>), the two
 * literals, the two vtables, then the type names in reverse definition
 * order: its own two, then T154_race_display.h's (RaceMiniMap .. the meters,
 * RaceCameraBase, UnitArena<64>, RaceValueDisplayBase, RaceDisplayBase), and
 * neither the GameStatus's garage nor the menu operator classes (T131 has
 * them): the race display header comes first, T130 / T129 after it.  Its
 * .sdata (0x00351E08..0x00351E18) is the "" of T157's inline
 * RaceMessageDisplay::clear (read with the race display), then the registry
 * node g_00351E10 (an uninitialised global: zeros at the end).
 */
// DATA_START 0x003282E8
// DATA_START 0x00351E08
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (race_basic_construct); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/race/display/T154_race_display.h"         /* RaceDisplayBase, RaceDisplay (first: retail's type-name order) */
#include "game/race/core/T132_race_basic.h"           /* own: RaceBasic, RaceBasicWithRaceDisplay, __tf9RaceBasic, __ti*;
                                               + T130 RacePS2Base, T129 RaceBase (RaceClassInfo /
                                               RaceClassNode, race_class_register) */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue) */
#include "game/camera/T383_camera_manager.h"       /* CameraManager (views) */
#include "game/race/core/T413_race_input.h"           /* RaceInput (virtual_25) */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn, CONTROL_START */
#include "game/data/T487_race_mode_database.h"   /* RaceModeDatabase g_002F00C0 */
#include "game/menu/records/T116_race_result_base.h"     /* RaceResultBase (RaceBasic::subDisplay) */
#include "game/race/pause/T168_pause_base.h"           /* PauseBase */

// FUNC 0x00180EB8 RaceBasic::operator new
//   The race pool, 16-byte aligned (the same code as RaceBase::operator new
//   0x0017BA18; an out-of-line copy emitted in T132).
void *RaceBasic::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00180EE0 RaceBasic::operator delete
//   Back to the race pool (copy of RaceBase::operator delete 0x0017BA40; the
//   size is not used).  Also called by the deleting destructors of
//   RaceBasicWithRaceDisplay, RaceBattleT<2> and RaceBattleT<8>.
void RaceBasic::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00180F08 RaceBasic::virtual_24
//   The class name (index.txt: RaceBasicWithRaceDisplay::virtual_24).
const char *RaceBasic::virtual_24()
{
    return "RaceBasic";
}

// FUNC 0x00180F18 race_basic_create
//   RaceClassInfo::create of "RaceBasic": new RaceBasic (size 0xC8C0; the
//   EH landing pad gives the memory back with operator delete).
RaceBase *race_basic_create(void)
{
    return new RaceBasic; /* implicit: RaceBasic::operator delete (if the constructor throws) */
}

// FUNC 0x00180F70 race_basic_construct
//   RaceClassInfo::construct: a RaceBasic in buf (nothing if buf is NULL).
//   arg is not used.  The global placement new of <new>: egcs's is inline
//   and throw(), so egcs tests its result for NULL itself (retail's daddu
//   v0,a0 / beqz v0 around the constructor) and emits no copy of it (a
//   class member placement new would get an out-of-line copy here, which
//   retail does not have).  GCC 14 does not test the standard placement
//   form: the explicit test keeps the behaviour.
RaceBase *race_basic_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)
        return NULL;
#endif
    return ::new (buf) RaceBasic;
}

/* The registry record of "RaceBasic" (.data, before the frame info; its
 * "RaceBasic" is virtual_24's literal, emitted once). */
RaceClassInfo g_002B6218 = {                /* 0x002B6218 */
    "RaceBasic", sizeof(RaceBasic), race_basic_create, race_basic_construct
};

// FUNC 0x00180F98 RaceBasic::RaceBasic
//   No result, display or pause menu; the mode "RACE_MODE_SINGLE".  (EH
//   landing pad: ~RacePS2Base.)
RaceBasic::RaceBasic() : RacePS2Base()
{
    raceDisplay = NULL;
    pauseMenu = NULL;
    subDisplay = NULL;
    setRaceMode("RACE_MODE_SINGLE");
}

// FUNC 0x00181020 RaceBasic::~RaceBasic
//   (RaceBasic::virtual_0) Only the base destructor; deleting form: operator
//   delete(this, 0xC8C0).
RaceBasic::~RaceBasic()
{
    /* implicit: ~RacePS2Base (the base / member destructors) */
}

// FUNC 0x00181078 RaceBasic::virtual_8
//   (index.txt: RaceBasicWithRaceDisplay::virtual_8) Race start: the base,
//   then the race display (organization, mode id, replay; the inner panel
//   only outside replays), the pause menu and the result sequence.
void RaceBasic::virtual_8(int replay)
{
    RacePS2Base::virtual_8(replay);
    if (raceDisplay != NULL) {
        raceDisplay->virtual_0(org, raceModeId, replay);
        raceDisplay->innerPanel(replay == 0);
    }
    if (pauseMenu != NULL)
        pauseMenu->virtual_1();
    if (subDisplay != NULL)
        subDisplay->start(*org, replay);
}

// FUNC 0x00181140 RaceBasic::virtual_35
//   (index.txt: RaceBasicWithRaceDisplay::virtual_35) The end: the race
//   display's virtual_1, the result's cleanup, then the base.
void RaceBasic::virtual_35()
{
    if (raceDisplay != NULL)
        raceDisplay->virtual_1();
    if (subDisplay != NULL)
        subDisplay->cleanup();
    RacePS2Base::virtual_35();
}

// FUNC 0x001811B8 RaceBasic::virtual_28
//   (index.txt: RaceBasicWithRaceDisplay::virtual_28) The update: the base,
//   the result sequence, then START outside background mode: pause, or in
//   a demo the end of the race.  (o: retail reads org before the
//   virtual_25 call and keeps it in $s2; egcs evaluates the call argument
//   first otherwise.)
void RaceBasic::virtual_28(f32 dt)
{
    RacePS2Base::virtual_28(dt);
    if (subDisplay != NULL) {
        RaceOrganization *o = org;
        subDisplay->update(*o, virtual_25());
    }
    if ((virtual_25()->getButtonsOn() & CONTROL_START) && backgroundMode == 0) {
        if (replayMode == RACE_DEMO) {
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
        } else {
            setPause(1);
            stopSounds();
        }
    }
}

// FUNC 0x001812D8 RaceBasic::virtual_30
//   (index.txt: RaceBasicWithRaceDisplay::virtual_30) The update while
//   paused: the pause menu's result.  No NULL check: paused is only set with
//   a menu (setPause).
void RaceBasic::virtual_30()
{
    PauseBase *menu = pauseMenu;
    switch (menu->virtual_2(virtual_25(), org)) {
    case PAUSE_RESULT_TIMEOUT:
        virtual_34(RACE_END_TIMEOUT);
        break;
    case PAUSE_RESULT_EXIT:
        virtual_34(RACE_END_EXIT);
        break;
    case PAUSE_RESULT_CONTINUE:
        setPause(0);
        startSounds();
        break;
    case PAUSE_RESULT_RETRY:
        virtual_34(RACE_END_RETRY);
        break;
    }
}

// FUNC 0x00181400 RaceBasic::virtual_45
//   (index.txt: RaceBasicWithRaceDisplay::virtual_45) The 2D overlays: the
//   base's, the race display, the result and, while paused, the pause menu
//   (with view 0's camera).
void RaceBasic::virtual_45(GSBuffer *buf)
{
    RacePS2Base::virtual_45(buf);
    RaceOrganization *o = org;
    CameraManager *cm = virtual_6();
    if (raceDisplay != NULL)
        raceDisplay->virtual_3(*buf);
    if (subDisplay != NULL)
        subDisplay->render(*buf, *o);
    if (paused != 0)
        pauseMenu->virtual_3(buf, o, cm->views[0].camera);
}

// FUNC 0x001814D8 RaceBasic::setPause
//   paused = on, but only when there is a pause menu.
void RaceBasic::setPause(int on)
{
    paused = (pauseMenu != NULL) ? on : 0;
}

// FUNC 0x001814F0 RaceBasic::setPauseMenu
//   Ignored in background mode; opens the menu (virtual_1).
void RaceBasic::setPauseMenu(PauseBase *m)
{
    if (backgroundMode == 0) {
        pauseMenu = m;
        if (m != NULL)
            m->virtual_1();
    }
}

// FUNC 0x00181538 RaceBasic::setSubDisplay
//   Ignored in background mode; starts the result sequence with the replay
//   mode.
void RaceBasic::setSubDisplay(RaceResultBase *r)
{
    if (backgroundMode == 0) {
        subDisplay = r;
        if (r != NULL)
            r->start(*org, replayMode);
    }
}

// FUNC 0x00181588 RaceBasic::setRaceDisplay
void RaceBasic::setRaceDisplay(RaceDisplayBase *d)
{
    raceDisplay = d;
}

// FUNC 0x00181598 RaceBasic::setRaceMode
//   The id of the mode name in the race mode database g_002F00C0 (-1 if
//   unknown).
void RaceBasic::setRaceMode(const char *name)
{
    raceModeId = g_002F00C0.getModeId(name);
}

// FUNC 0x001815D0 RaceBasicWithRaceDisplay::RaceBasicWithRaceDisplay
//   RaceBasic, the embedded RaceDisplay, which becomes the race display.
//   (EH landing pads: ~RaceDisplay, ~RaceBasic.)
RaceBasicWithRaceDisplay::RaceBasicWithRaceDisplay()
{
    /* implicit: RaceBasic, RaceDisplay (the base's and the member's
     * constructors), ~RaceDisplay, ~RaceBasic (their cleanup on a throw) */
    setRaceDisplay(&display);
}

// FUNC 0x00181678 RaceBasicWithRaceDisplay::~RaceBasicWithRaceDisplay
//   (RaceBasicWithRaceDisplay::virtual_0) The RaceDisplay (flags 2), then
//   RaceBasic; deleting form: operator delete(this, 0x16D40).
RaceBasicWithRaceDisplay::~RaceBasicWithRaceDisplay()
{
    /* implicit: ~RaceBasic, ~RaceDisplay, operator delete (the base / member
     * destructors, flags & 1) */
}

/* RaceBasic's node in the registry (.sdata zeros, linked in at start-up). */
RaceClassNode g_00351E10;                   /* 0x00351E10 */

// FUNC 0x001816E8 race_basic_static_init
//   .ctors entry (0x0028F0F0): registers "RaceBasic" (g_002B6218) with its
//   node g_00351E10.
void race_basic_static_init(void)
{
    race_class_register(&g_00351E10, &g_002B6218);
}

#ifndef BOOT_OLD_GCC                        /* egcs generates the typeinfo functions itself */
// FUNC 0x00181710 __tf24RaceBasicWithRaceDisplay
//   Builds the __si_type_info __ti24RaceBasicWithRaceDisplay (0x004B1EC0;
//   base: RaceBasic) on the first call.
extern "C" const type_info &__tf24RaceBasicWithRaceDisplay(void)
{
    void *node = __ti24RaceBasicWithRaceDisplay;

    if (__ti24RaceBasicWithRaceDisplay[0] == 0) {
        __tf9RaceBasic();
        __rtti_si(node, "24RaceBasicWithRaceDisplay", (const type_info *)(void *)__ti9RaceBasic);
    }
    return *(const type_info *)node;
}
#endif

// FUNC 0x00181760 RaceBasic::setInnerPanel
/* inline in game/race/core/T132_race_basic.h; egcs emits this copy */

// FUNC 0x00181798 RaceBasic::setRaceModeId
/* inline in game/race/core/T132_race_basic.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x001817A8 __tf9RaceBasic
//   Builds the __si_type_info __ti9RaceBasic (0x004B1ED0; base: RacePS2Base,
//   __ti11RacePS2Base 0x004B1E90) on the first call.
extern "C" const type_info &__tf9RaceBasic(void)
{
    void *node = __ti9RaceBasic;

    if (__ti9RaceBasic[0] == 0) {
        __tf11RacePS2Base();
        __rtti_si(node, "9RaceBasic", (const type_info *)(void *)__ti11RacePS2Base);
    }
    return *(const type_info *)node;
}
#endif
