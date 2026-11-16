/* T137_race_split_battle.cpp - RaceSplitBattleInformation, RaceSplitBattle
 * and the instances RaceBattleInformationT<2> / RaceBattleT<2>
 * (0x00183240..0x001848A8); the classes are declared in
 * include/game/race/modes/T137_race_split_battle.h.
 *
 * The 2-player battle on one console ("RACE_MODE_2P_BATTLE"): two RaceInputs
 * (pad slots 0x14 and 0x15, inputIds 0x100 / 0x101, ports 0 / 1), one camera
 * view per player (CameraManager viewCount 2, view i follows car i and reads
 * input i), the 2-player result (Result2PBattle), the race / replay pause
 * menus (the replay one in mode 1), the split race display (one display per
 * screen; RaceSplitDisplay::target selects it) and
 * DynamicsConductorBattle2P.  The render draws the two views one above the
 * other (each h / 2 high, mirror at 200,16 / 200,8, zoom 1.2) or, on a wide
 * screen (g_002F424C), side by side (each w / 2 wide; the 3D views h * 8 /
 * 11 high from h / 6, the overlays full height, mirror at 90,5, zoom 1.0 /
 * mirror 1.3); a replay is drawn full screen (one view) unless the game
 * option replaySplit asks for the split screen.  Both inputs' records go
 * into the replay data (virtual_13 / 14 / 15).
 *
 * RaceSplitBattleInformation (RaceBattleInformationT<2>) gives the two cars
 * their equipments and inputIds 0x100 + i; the grid order is the entry
 * order (no opponents are drawn).
 *
 * Includes (u046's data pass): the classes retail's T137 saw, in its order.
 * Its .rodata (0x00329AF8..0x0032A360) is "bad_alloc" (<new>), the literals,
 * the four vtables, then the type names in reverse definition order: the
 * garage's (T429, read after the own header), its own four (RaceSplitBattle,
 * RaceBattleT<2>, RaceSplitBattleInformation, RaceBattleInformationT<2>), the
 * result bars (T200), ReplayPauseMenu (T170), PauseMenu (T169), the windows
 * (T199), RaceMenuButton (T198), the menu operator classes (T216), the race
 * display's (T154, UnitArena<64> among them) and Operator (a non-polymorphic
 * base: last).  Its .sdata (0x00351E60..0x00351E70) is T157's "" (the race
 * display), then the node g_00351E68.
 */
// DATA_START 0x00329AF8
// DATA_START 0x00351E60
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (the construct function); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/race/display/T154_race_display.h"         /* race_display_poll / drop_events (first: retail's type-name
                                               order) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the menu operator classes: retail's type-name order) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (RaceMenuButton: retail's type-name order) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (the windows: retail's type-name order) */
#include "game/race/pause/T169_race_pause.h"           /* RacePause */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (F_001A3248) */
#include "game/menu/records/T200_result_board.h"         /* (the result bars: retail's type-name order) */
#include "game/race/modes/T137_race_split_battle.h"    /* own: RaceSplitBattle(Information), the templates; + T132
                                               RaceBasic, T129 RaceBase (RaceClassInfo / RaceClassNode,
                                               race_class_register, RACE_LIVE ..), T155
                                               RaceSplitDisplay, T192 Result2PBattle */
#include "game/save/T429_game_status.h"          /* getGameStatus (render); after the own header: egcs
                                               writes type names in reverse definition order, and
                                               retail's garage names (0x00329FC0..) come before
                                               T137's own (0x0032A0A8) */
#include "engine/pgl/G001_flare.h"                /* gs_set_clear_color, gs_clear_buffers, PGL_CLEAR_COLOR */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar, RaceEntryInformation */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post, RACE_EVENT_* */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue, the entry order) */
#include "game/car/model/T184_car_icon_maker.h"       /* CarIcon, car_icons_make */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord::Manager (the replay records) */
#include "game/camera/T377_camera_target_info.h"   /* CameraTargetInfo::setCar */
#include "game/camera/T383_camera_manager.h"       /* CameraManager, CameraView */
#include "game/camera/T397_variable_camera_base.h" /* VariableCameraBase::getMount */
#include "game/race/core/T413_race_input.h"           /* RaceInput */
#include "game/save/T428_game_options.h"         /* GameOptions::replaySplit */
#include "game/data/T482_car_equipments.h"       /* CarEquipments */
#include "engine/gfx/T512_render_context.h"       /* g_002F424C (g_002F4230.wide) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/race/modes/race_modes_internal.h"                           /* the unit's constants */
#include "game/race/display/T155_race_pod2p_display.h"   /* the enhanced engine: RacePOD2PDisplay (RacePOD2P's) */

/* The instances <2> of the templates are defined here (explicit
 * specializations of the members, declared before their first use, as
 * T138's <8>): they are emitted where they are defined, before the
 * registration, as in retail's frame info (there egcs's _GLOBAL_$I$
 * followed the pending template instances; the C's registration is a plain
 * function).  u046's data pass. */
template <> RaceBattleInformationT<2>::RaceBattleInformationT();
template <> RaceBattleInformationT<2>::~RaceBattleInformationT();
template <> RaceBattleT<2>::RaceBattleT();
template <> RaceBattleT<2>::~RaceBattleT();

/* ======================================================================== */
/* RaceSplitBattleInformation                                               */
/* ======================================================================== */

// FUNC 0x00183240 RaceSplitBattleInformation::RaceSplitBattleInformation
//   RaceBattleInformationT<2>::RaceBattleInformationT, the vptr, 2 cars.
//   Caller: run_split_battle (T054).
RaceSplitBattleInformation::RaceSplitBattleInformation()
{
    entryCount = 2;

    /* implicit: RaceBattleInformationT (the base's / members' constructors) */
}

// FUNC 0x00183280 RaceSplitBattleInformation::~RaceSplitBattleInformation
//   The vptr, then RaceBattleInformationT<2>::~RaceBattleInformationT with
//   the caller's flags.
RaceSplitBattleInformation::~RaceSplitBattleInformation()
{
    /* implicit: ~RaceBattleInformationT (the base / member destructors) */
}

// FUNC 0x001832A8 RaceSplitBattleInformation::createEntries
//   (slot 4) new RaceEntryPS2 (as RaceSinglePlayerInformation's:
//   RaceEntryPS2::operator new, RaceEntryPS2::RaceEntryPS2, operator delete
//   if it throws).
RaceEntryBase *RaceSplitBattleInformation::createEntries()
{
    return new RaceEntryPS2; /* implicit: RaceEntryPS2::operator delete (if the constructor throws) */
}

// FUNC 0x00183300 RaceSplitBattleInformation::setupEntry
//   (slot 10) Car n gets entries[n]'s equipments (CarEquipments::operator=)
//   and inputId 0x100 + n and, when the equipments hold a preset car, the
//   entry's tyre and transmission.  No range check of n.  The car's
//   information through a pointer (retail: its own s-register beside car's).
void RaceSplitBattleInformation::setupEntry(RaceEntryCar *car)
{
    int n = car->number;
    RaceEntry *e = getEntry(n);
    RaceEntryInformation *info = &car->info;
    static_cast<CarEquipments &>(*info) = e->equip; /* implicit: CarEquipments::operator= */
    info->inputId = n + INPUT_ID_PLAYER;
    if (info->presetCar != 0) {
        car->tyre = e->tyre;
        car->transmission = e->transmission;
    }
}

// FUNC 0x00183378 RaceSplitBattleInformation::setupOrganization
//   (slot 11) Only the grid order reset: both cars are players.
void RaceSplitBattleInformation::setupOrganization(RaceOrganization *org)
{
    org->resetEntryOrder();
}

/* ======================================================================== */
/* RaceSplitBattle                                                          */
/* ======================================================================== */

// FUNC 0x00183398 RaceSplitBattle::operator new
//   The race pool, 16-byte aligned (pdistd::Allocator::allocate).
void *RaceSplitBattle::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x001833C0 RaceSplitBattle::operator delete
//   The size (0x3EC80) is ignored (pdistd::Allocator::free).
void RaceSplitBattle::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x001833E8 RaceSplitBattle::virtual_24
//   The class name.
const char *RaceSplitBattle::virtual_24()
{
    return "RaceSplitBattle";
}

// FUNC 0x001833F8 RaceSplitBattle_create
//   RaceClassInfo::create (g_002B72B8 +8): new RaceSplitBattle (0x3EC80;
//   RaceSplitBattle::operator new, the constructor, operator delete if it
//   throws).
RaceBase *RaceSplitBattle_create(void)
{
    return new RaceSplitBattle; /* implicit: RaceSplitBattle::operator delete (if the constructor throws) */
}

// FUNC 0x00183458 RaceSplitBattle_construct
//   RaceClassInfo::construct (g_002B72B8 +0xC): a RaceSplitBattle in buf
//   (NULL: nothing).  arg is not used.
RaceBase *RaceSplitBattle_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)                        /* egcs tests the result of <new>'s throw() placement
                                               new itself; GCC 14 does not */
        return NULL;
#endif
    return ::new (buf) RaceSplitBattle;
}

/* The registry record of "RaceSplitBattle" (.data, before the frame info;
 * its "RaceSplitBattle" is virtual_24's literal, emitted once). */
RaceClassInfo g_002B72B8 = {                /* 0x002B72B8 */
    "RaceSplitBattle", sizeof(RaceSplitBattle), RaceSplitBattle_create, RaceSplitBattle_construct
};

// FUNC 0x00183480 RaceSplitBattle::RaceSplitBattle
//   RaceBattleT<2>::RaceBattleT, the vptr, then the members: the dynamics
//   (DynamicsConductorBattle2P::DynamicsConductorBattle2P, never
//   destroyed), input (+0x1AC6C, RaceInput::RaceInput), inputs[2] (array
//   construction with its unwinding loop of virtual RaceInput::~RaceInput),
//   the 7 icons (CarIcon::CarIcon), the result (Result2PBattle's implicit
//   constructor: ResultArcade's vptr, ResultBoard::ResultBoard, then
//   Result2PBattle's vptr 0x00327140), the pause menu (RacePause::
//   RacePause), the replay pause menu (ReplayPause::ReplayPause, mode 1)
//   and the split display (RaceSplitDisplay::RaceSplitDisplay).  The body
//   names the mode, makes the split display the race display, installs the
//   dynamics and attaches the two inputs to pad slots 0x14 + i
//   (Control::attach), their records covering buffer i in mode 0
//   (Manager::attachBuffer), port = player.  The landing pads run
//   RaceSplitDisplay::~RaceSplitDisplay, ReplayPause::~ReplayPause,
//   RacePause::~RacePause, ResultBoard::~ResultBoard, the inputs'
//   destructors and RaceBattleT<2>::~RaceBattleT.  Callers:
//   run_split_battle, create / construct.
RaceSplitBattle::RaceSplitBattle()
    : replayPause(1)
{
    /* implicit: RaceBattleT, RaceSplitDisplay, RacePause, ReplayPause,
     * CarIcon, ResultBoard, DynamicsConductorBattle2P, RaceInput (the base's /
     * members' constructors), ~RaceBattleT, ~RaceSplitDisplay, ~RacePause,
     * ~ReplayPause, ~ResultBoard, ~RaceInput (their cleanup on a throw) */
    setRaceMode("RACE_MODE_2P_BATTLE");
    setRaceDisplay(&splitDisplay);
    podDisplay = NULL;                      /* the enhanced engine: splitDisplay (RacePOD2P sets its own) */
    dc = &dc2P;
    for (int i = 0; i < 2; i++) {
        RaceInput *in = &inputs[i];          /* as RaceSinglePlayer's: the input and its record
                                                are two loop pointers in retail */
        in->attach(PAD_SLOT_PLAYER + i);
        AutomobileControlRecord::Manager *rec = &in->rec;
        rec->setMode(0);
        rec->attachBuffer(i);
        in->port = (i != 0);
    }
}

// FUNC 0x001837F0 RaceSplitBattle::~RaceSplitBattle
//   The vptr; the two inputs leave the pad manager (Control::detach); then
//   the members in reverse order: RaceSplitDisplay::~RaceSplitDisplay,
//   ReplayPause::~ReplayPause, RacePause::~RacePause (flags 2), the result
//   (implicit: ResultBoard::~ResultBoard, the vptr = RaceResultBase's),
//   inputs[1], inputs[0] (virtual destructors, flags 0, through the vptr at
//   +0xC0), input (RaceInput::~RaceInput, flags 2);
//   RaceBattleT<2>::~RaceBattleT (flags 0); deleting form: operator
//   delete(this, 0x3EC80).
RaceSplitBattle::~RaceSplitBattle()
{
    inputs[0].detach();
    inputs[1].detach();
    /* implicit: ~RaceBattleT, ~RaceSplitDisplay, ~RacePause, ~ReplayPause,
     * ~ResultBoard, ~RaceInput (the base / member destructors) */
}

// FUNC 0x00183948 RaceSplitBattle::virtual_8
//   Race start: RaceBasic's (0x00181078), the menus (as RaceArcade's, with
//   the 2P result), then two camera views (CameraManager::setViewCount):
//   view i reads inputs[i] (RaceInput::setReplayMode with the argument, not
//   replayMode) and, made the current view (setCurrentView), follows car i
//   (CameraTargetInfo::setCar, no blend).
void RaceSplitBattle::virtual_8(int replay)
{
    RaceBattleT<2>::virtual_8(replay);
    if (replay != RACE_REPLAY) {
        setPauseMenu(&pause);
        setSubDisplay(&result2P);
    } else {
        replayPause.F_001A3248(org);
        setPauseMenu(&replayPause);
        setSubDisplay(NULL);
    }
    camera.setViewCount(2);
    for (int i = 0; i < 2; i++) {
        CameraManager *cm = &camera;        /* a loop-local copy of the argument register */
        inputs[i].setReplayMode(replay);
        cm->views[i].input = &inputs[i];
        cm->setCurrentView(i);
        cm->views[cm->current].targetInfo.setCar(i, 0);
    }
}

// FUNC 0x00183A90 RaceSplitBattle::virtual_43
//   Before the race runs: the car icons (car_icons_make), given to the
//   result board (ResultArcade::setDisplays) only.
void RaceSplitBattle::virtual_43()
{
    car_icons_make(icons, org);
    result2P.setDisplays(icons);
}

// FUNC 0x00183AE8 RaceSplitBattle::virtual_25
//   Player 1's input.
RaceInput *RaceSplitBattle::virtual_25()
{
    return &inputs[0];
}

// FUNC 0x00183AF8 RaceSplitBattle::virtual_26
//   inputIds 0x100 / 0x101 are the two players' inputs (an unsigned range
//   check), anything else RaceBase::virtual_26 (called with the original
//   id).
RaceInput *RaceSplitBattle::virtual_26(int id)
{
    u32 player = id - INPUT_ID_PLAYER;
    if (player < 2)
        return &inputs[player];
    return RaceBase::virtual_26(id);
}

// FUNC 0x00183B40 RaceSplitBattle::virtual_28
//   The update: RaceBasic's, the split display (updateDisplay), then the
//   record checks of both players: in a race a full record posts event 0x16
//   with the player (delay 2); in a replay the end of either record posts
//   event 1.  The inputs are fetched through virtual_26 each time.
void RaceSplitBattle::virtual_28(float dt)
{
    RaceBattleT<2>::virtual_28(dt);
    updateDisplay();
    switch (replayMode) {
    case RACE_LIVE:
        if (virtual_26(INPUT_ID_PLAYER)->rec.rec.finished != 0)
            org->eventQueue.post(RACE_EVENT_RECORD_FULL, 0, 2);
        if (virtual_26(INPUT_ID_PLAYER + 1)->rec.rec.finished != 0)
            org->eventQueue.post(RACE_EVENT_RECORD_FULL, 1, 2);
        break;
    case RACE_REPLAY:
        if (virtual_26(INPUT_ID_PLAYER)->rec.rec.finished != 0
            || virtual_26(INPUT_ID_PLAYER + 1)->rec.rec.finished != 0)
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
        break;
    case RACE_DEMO:                         /* retail's case tree tests 1, then < 2, then 0 */
        break;
    }
}

// FUNC 0x00183C70 RaceSplitBattle::virtual_30
//   The update while paused: RaceBasic's (the pause menu), then the split
//   display (updateDisplay).
void RaceSplitBattle::virtual_30()
{
    RaceBattleT<2>::virtual_30();
    updateDisplay();
}

// FUNC 0x00183CA0 RaceSplitBattle::updateDisplay
//   Between race_display_poll_events / race_display_drop_events: for each
//   screen i the camera's current view becomes i (virtual_6, then
//   CameraManager::setCurrentView), the split display's target display
//   becomes i and gets the player number i (RaceSplitDisplay::virtual_7,
//   setPlayerCarNumber), the car of the current view (virtual_8,
//   setTargetCarNumber) and view i's camera mount (virtual_9), all called
//   directly; then its update (RaceSplitDisplay::virtual_2, both displays).
void RaceSplitBattle::updateDisplay()
{
    race_display_poll_events(org);
    if (podDisplay != NULL) {
        /* The enhanced engine: the Pod 2P race's display, the same steps
         * (Lupo 2's RaceSplitBattleBase through getDisplay) */
        for (int i = 0; i < 2; i++) {
            CameraManager *cm = virtual_6();
            cm->setCurrentView(i);
            podDisplay->setTargetDisplay(i);
            podDisplay->setPlayerCarNumber(i);
            podDisplay->setTargetCarNumber(cm->F_00209F10());
            podDisplay->virtual_9(cm->views[i].camera->getMount());
        }
        podDisplay->virtual_2(RACE_DISPLAY_STEP);
        race_display_drop_events(org);
        return;
    }
    for (int i = 0; i < 2; i++) {
        CameraManager *cm = virtual_6();
        cm->setCurrentView(i);
        RaceSplitDisplay *sd = &splitDisplay;   /* loop.c hoists it (and sd + 0x10000 for target);
                                                   virtual_2 below recomputes the address */
        sd->target = i;
        sd->RaceSplitDisplay::setPlayerCarNumber(i);
        sd->RaceSplitDisplay::setTargetCarNumber(cm->F_00209F10());
        sd->RaceSplitDisplay::virtual_9(cm->views[i].camera->getMount());
    }
    splitDisplay.RaceSplitDisplay::virtual_2(RACE_DISPLAY_STEP);  /* the enhanced engine: a frame (hud.md C19) */
    race_display_drop_events(org);
}

/* The enhanced engine: the screen's display for render's two screens (the
 * Pod 2P race's when it has one). */
void RaceSplitBattle::setDisplayTarget(int i)
{
    splitDisplay.setTargetDisplay(i);
    if (podDisplay != NULL)
        podDisplay->setTargetDisplay(i);
}

// FUNC 0x00183DD0 RaceSplitBattle::virtual_33
//   RacePS2Base's (0x0017DBB0, which sets it too), then endRequested = 1
//   (the next virtual_27 ends the race).
void RaceSplitBattle::virtual_33()
{
    RaceBattleT<2>::virtual_33();
    endRequested = 1;
}

// FUNC 0x00183E08 RaceSplitBattle::render
//   (GameObject slot 2.)  Split screen in a race, or in a replay when the
//   game option replaySplit is set: a GSBuffer (GSBuffer::GSBuffer, then
//   GSBuffer::copy of buf) gets each screen's rectangle (GSBuffer::setRect);
//   per screen the camera's current view (CameraManager::setCurrentView),
//   the split display's target, the mirror rectangle, the 3D view
//   (virtual_47, sub 1) and the overlays (virtual_45).  On a wide screen
//   buf is first made the draw buffer (GSBuffer::setDrawBuffer) and
//   cleared to black (gs_set_clear_color, gs_clear_buffers colour); h stays
//   whole there (GSBuffer::displayHeight).  Otherwise (a replay) view 0
//   full screen (virtual_44, virtual_45).  Then the result
//   (RaceResultBase::render), the pause menu while paused (its draw slot,
//   with view 0's camera) and the error view (virtual_46).  org is read
//   once at the start.  The GSBuffer of either branch is destroyed by one
//   shared GSBuffer::~GSBuffer call (flags 2).
void RaceSplitBattle::render(GSBuffer &buf)
{
    RaceOrganization *o = org;

    if (replayMode == RACE_LIVE || getGameStatus()->getGameOptions()->replaySplit != 0) {
        int w = buf.width;
        int h = buf.displayHeight();
        if (g_002F424C == 0) {
            /* one screen above the other */
            h >>= 1;
            GSBuffer screen;
            screen.copy(&buf);
            camera.setCurrentView(0);
            setDisplayTarget(0);
            screen.setRect(0, 0, w, h);
            mirror.set(2.0f, 200.0f, 16.0f, 240.0f, 40.0f);
            drawSubView(&screen, 1, 1.2f);
            virtual_45(&screen);
            camera.setCurrentView(1);
            setDisplayTarget(1);
            screen.setRect(0, h, w, h);
            mirror.set(2.0f, 200.0f, 8.0f, 240.0f, 40.0f);
            drawSubView(&screen, 1, 1.2f);
            virtual_45(&screen);
        } else {
            /* side by side: the 3D views are h * 8 / 11 high from h / 6 */
            int viewY = h / 6;
            int viewH = (h * 8) / 11;
            w >>= 1;
            buf.setDrawBuffer();
            gs_set_clear_color(0);
            gs_clear_buffers(PGL_CLEAR_COLOR);
            GSBuffer screen;
            screen.copy(&buf);
            mirror.set(1.3f, 90.0f, 5.0f, 140.0f, 46.0f);
            camera.setCurrentView(0);
            setDisplayTarget(0);
            screen.setRect(0, viewY, w, viewH);
            drawSubView(&screen, 1, 1.0f);
            screen.setRect(0, 0, w, h);
            virtual_45(&screen);
            camera.setCurrentView(1);
            setDisplayTarget(1);
            screen.setRect(w, viewY, w, viewH);
            drawSubView(&screen, 1, 1.0f);
            screen.setRect(w, 0, w, h);
            virtual_45(&screen);
        }
    } else {
        camera.setCurrentView(0);
        setDisplayTarget(0);
        drawView(&buf);
        virtual_45(&buf);
    }
    if (subDisplay != NULL)
        subDisplay->render(buf, *o);
    if (paused != 0) {
        CameraManager *cm = &camera;        /* GCSE: the &camera register of the paths above */
        pauseMenu->virtual_3(&buf, o, cm->views[0].camera);
    }
    virtual_46(&buf);
}

// FUNC 0x00184310 RaceSplitBattle::virtual_45
//   The overlays: the split display draws its target display
//   (RaceSplitDisplay::virtual_17, drawTarget; buf passed through).
//   RaceBasic's virtual_45 is not called.
void RaceSplitBattle::virtual_45(GSBuffer *buf)
{
    if (podDisplay != NULL) {
        podDisplay->drawTarget(*buf);       /* the enhanced engine: the Pod 2P race's */
        return;
    }
    splitDisplay.RaceSplitDisplay::drawTarget(*buf);
}

// FUNC 0x00184338 RaceSplitBattle::virtual_46
//   Only RacePS2Base's (the same as the inherited slot).
void RaceSplitBattle::virtual_46(GSBuffer *buf)
{
    RaceBattleT<2>::virtual_46(buf);
}

// FUNC 0x00184358 RaceSplitBattle::virtual_13
//   Loads both players' input records from after the race info block, one
//   after the other (Manager::load returns the end of the first; the
//   inputs through virtual_26).
void RaceSplitBattle::virtual_13(RaceInfoBlock *blk)
{
    const u32 *data = (const u32 *)((u8 *)blk + RACE_INFO_BLOCK_SIZE);
    data = virtual_26(INPUT_ID_PLAYER)->rec.load(data, 1);
    virtual_26(INPUT_ID_PLAYER + 1)->rec.load(data, 1);
}

// FUNC 0x001843D8 RaceSplitBattle::virtual_14
//   Two recorded inputs; both records are appended to the block (each
//   Manager::save advances its size word).
void RaceSplitBattle::virtual_14(RaceInfoBlock *blk)
{
    blk->recordCount = 2;
    virtual_26(INPUT_ID_PLAYER)->rec.save(&blk->size, 1);
    virtual_26(INPUT_ID_PLAYER + 1)->rec.save(&blk->size, 1);
}

// FUNC 0x00184460 RaceSplitBattle::virtual_15
//   The size of both saved records (Manager::saveSize).
int RaceSplitBattle::virtual_15()
{
    int size = virtual_26(INPUT_ID_PLAYER)->rec.saveSize(1);
    return size + virtual_26(INPUT_ID_PLAYER + 1)->rec.saveSize(1);
}

/* ======================================================================== */
/* The template instances                                                   */
/* ======================================================================== */

// FUNC 0x001844D8 RaceBattleInformationT<2>::RaceBattleInformationT
//   RaceInformation::RaceInformation, the vptr, the entries' equipments
//   (CarEquipments::CarEquipments, array construction, unwound with
//   CarEquipments::~CarEquipments on exceptions), then every entry empty (no
//   car, colour -1) and no missing entry.  Caller:
//   RaceSplitBattleInformation's constructor.
template <>
RaceBattleInformationT<2>::RaceBattleInformationT()
{
    /* implicit: RaceInformation, CarEquipments (the base's / members'
     * constructors), ~RaceInformation, ~CarEquipments (their cleanup on a
     * throw) */
    for (int i = 0; i < 2; i++) {
        RaceEntry *e = &entries[i];         /* one pointer for the five stores, as retail */
        e->car = 0;
        e->color = -1;
        e->transmission = 0;
        e->tyre = 0;
        e->vibration = 0;
    }
    missingCount = 0;
}

// FUNC 0x00184600 RaceBattleInformationT<2>::~RaceBattleInformationT
//   (known name) The vptr, the entries' equipments in reverse
//   (CarEquipments::~CarEquipments, flags 2), RaceInformation::
//   ~RaceInformation with the caller's flags.
template <>
RaceBattleInformationT<2>::~RaceBattleInformationT()
{
    /* implicit: ~RaceInformation, ~CarEquipments (the base / member
     * destructors) */
}

// FUNC 0x00184688 RaceBattleT<2>::RaceBattleT
//   RaceBasic::RaceBasic and the vptr.
template <>
RaceBattleT<2>::RaceBattleT()
{
    /* implicit: RaceBasic (the base's / members' constructors) */
}

// FUNC 0x001846C0 RaceBattleT<2>::~RaceBattleT
//   (known name) The vptr, RaceBasic::~RaceBasic (flags 0); deleting form:
//   RaceBasic's operator delete(this, 0xC8C0).
template <>
RaceBattleT<2>::~RaceBattleT()
{
    /* implicit: ~RaceBasic, operator delete (the base / member destructors,
     * flags & 1) */
}

/* RaceSplitBattle's node in the registry (.sdata zeros, linked in at
 * start-up). */
RaceClassNode g_00351E68;                   /* 0x00351E68 */

// FUNC 0x00184718 race_split_battle_static_ctor
//   .ctors entry (0x0028F104): registers "RaceSplitBattle" (g_002B72B8,
//   node gp-0x5B88).
void race_split_battle_static_ctor(void)
{
    race_class_register(&g_00351E68, &g_002B72B8);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x00184740 __tf15RaceSplitBattle
//   The __si_type_info __ti15RaceSplitBattle, base __tit11RaceBattleT1i2.
extern "C" const type_info &__tf15RaceSplitBattle(void)
{
    if (__ti15RaceSplitBattle[0] == 0) {
        __tft11RaceBattleT1i2();
        __rtti_si(__ti15RaceSplitBattle, "15RaceSplitBattle",
                  (const type_info *)__tit11RaceBattleT1i2);
    }
    return *(const type_info *)__ti15RaceSplitBattle;
}

// FUNC 0x00184790 __tft11RaceBattleT1i2
//   The __si_type_info __tit11RaceBattleT1i2, base __ti9RaceBasic.
extern "C" const type_info &__tft11RaceBattleT1i2(void)
{
    if (__tit11RaceBattleT1i2[0] == 0) {
        __tf9RaceBasic();
        __rtti_si(__tit11RaceBattleT1i2, "t11RaceBattleT1i2",
                  (const type_info *)__ti9RaceBasic);
    }
    return *(const type_info *)__tit11RaceBattleT1i2;
}
#endif

// FUNC 0x001847E0 RaceSplitBattleInformation::setEquipments
//   entries[i].equip = eq (CarEquipments::operator=, RaceEntry's inline
//   setEquipments; by reference; unreferenced).
/* inline in game/race/modes/T137_race_split_battle.h; egcs emits this copy (this unit
 * defines the class's first virtual function) */
/* implicit: CarEquipments::operator= (RaceEntry's inline setEquipments) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00184808 __tf26RaceSplitBattleInformation
//   The __si_type_info __ti26RaceSplitBattleInformation, base
//   __tit22RaceBattleInformationT1i2.
extern "C" const type_info &__tf26RaceSplitBattleInformation(void)
{
    if (__ti26RaceSplitBattleInformation[0] == 0) {
        __tft22RaceBattleInformationT1i2();
        __rtti_si(__ti26RaceSplitBattleInformation, "26RaceSplitBattleInformation",
                  (const type_info *)__tit22RaceBattleInformationT1i2);
    }
    return *(const type_info *)__ti26RaceSplitBattleInformation;
}

// FUNC 0x00184858 __tft22RaceBattleInformationT1i2
//   The __si_type_info __tit22RaceBattleInformationT1i2, base
//   __ti15RaceInformation (gp-0x4678).
extern "C" const type_info &__tft22RaceBattleInformationT1i2(void)
{
    if (__tit22RaceBattleInformationT1i2[0] == 0) {
        __tf15RaceInformation();
        __rtti_si(__tit22RaceBattleInformationT1i2, "t22RaceBattleInformationT1i2",
                  (const type_info *)__ti15RaceInformation);
    }
    return *(const type_info *)__tit22RaceBattleInformationT1i2;
}
#endif
