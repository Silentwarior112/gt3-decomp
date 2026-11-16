/* T138_race_link_battle.cpp - the i.LINK battle race (0x001848A8..0x00186968):
 * RaceLinkBattleInformation, RaceLinkBattleInput, RaceLinkBattleNodeInput,
 * RaceLinkBattle and the instances RaceBattleInformationT<8> / RaceBattleT<8>
 * of the T137 templates.
 *
 * RaceLinkBattle runs one console of an i.LINK battle of up to 8 PS2s.  Every
 * node of the session has a RaceLinkBattleNodeInput that reads the input the
 * node published with its GameOptions (GameStatus menu.options[node]),
 * attached to the pad manager as slots 0xC..0x13.  A console with viewMode 0
 * (g_00352CA8) drives a car with its own RaceLinkBattleInput (slot 0x14);
 * the view modes 1..9 make the consoles one screen wall: setupView places
 * this console's screen from its position among the nodes (the playing
 * nodes first; they still drive, the others show a car).
 *
 * Every drawn frame (virtual_46 -> updateLink) a state machine follows the
 * session: running (the local pause button asks the others for a command
 * exchange; one they asked for pauses the race), our pause menu, waiting for
 * the others to take the exchange, the connection error.  virtual_27 resets
 * the link after 601 frames of a waiting exchange (g_0035322C).
 *
 * RaceLinkBattleInformation sets the race up from what the host (node 0)
 * published: its race options, 6 entries, the car of every playing node;
 * the other entries get cars from RaceInformation::assignCars, and the
 * playing nodes come first in the start order.
 *
 * The operator new / delete pairs are this unit's copies of the inline
 * race-pool operators (the pool g_002B3BF0).  egcs passes `this + delta' to
 * the virtual calls (delta 0 everywhere here).
 *
 * Includes (u047's data pass): the classes retail's T138 saw, in its order.
 * Its .rodata (0x0032A350..0x0032AD30) is "bad_alloc" (<new>), the two
 * literals, setupView's four jump tables, the six vtables, then the type
 * names in reverse definition order: the GameStatus's garage (T429, read
 * after the own header), the own six (RaceLinkBattle .. the <8> instances,
 * no <2> one), ResultTermBar, ResultTitleBar (T191), ReplayPauseMenu
 * (T170), PauseMenu (T169), the race menu windows (T199) and button (T198),
 * the OperatorHolder group (T216), the race display's (T154: RaceMiniMap ..
 * RaceCameraBase, UnitArena<64>, RaceValueDisplayBase, RaceDisplayBase;
 * T154 is the first header that reads T153, with its type-name switch),
 * and Operator (the non-polymorphic base, after all the others).  Its
 * .sdata (0x00351E70..0x00351E88) is the "" of T157's inline
 * RaceMessageDisplay::clear (read with the race display), the registry node
 * g_00351E78 and "sw_20".
 */
// DATA_START 0x0032A350
// DATA_START 0x00351E70
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (race_link_battle_construct); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/race/display/T154_race_display.h"         /* RaceDisplay (first: retail's type-name order) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the menu operator classes retail's T138 saw) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (RaceMenuButton, before the windows) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (RaceMenuWindow, RaceMenuSelectWindow) */
#include "game/race/pause/T169_race_pause.h"           /* RacePause */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause */
#include "game/menu/records/T191_result_link_battle.h"   /* ResultLinkBattle */
#include "game/race/modes/T138_race_link_battle.h"     /* own: RaceLinkBattle .., the T137 templates' <8> instances;
                                               T413 RaceInput, T310 DynamicsConductorBattleMP, T184
                                               CarIcon, T132 RaceBasic, T130 RaceEntryPS2, T129 RaceBase /
                                               the race class registry, T383 CameraManager / ScreenLayout */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::menu (T427), ::GameOptions (T428);
                                               after the own header: the garage names first */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar, RaceEntryBase, RaceEntryInformation */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool (T530 pdistd::Allocator) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, eventQueue, ...) */
#include "game/camera/T377_camera_target_info.h"   /* CameraTargetInfo::setCar */
#include "game/camera/T397_variable_camera_base.h" /* VariableCameraBase::getMount */
#include "engine/input/T475_pad_manager.h"          /* PadManager g_002EA7F8 */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/data/messages/T186_msg_race_display.h"     /* g_002C43E8, the RACEDISPLAY texts */
#include "engine/system/T473_link_session.h"         /* the i.LINK session: F_0022CCB0 .., g_00352CA8, g_00352CB0 */
#include "game/data/T485_car_color.h"            /* CarColorDataBase g_00352D98 */
#include "game/common/T494_text_utils.h"           /* draw_rect */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/pgl/G001_flare.h"                /* gfx_enable, PGL_CAP_BLEND, pgl_color */
#include "engine/pdi/G003_pdi1394_rpc.h"          /* F_0026EB90 (the link reset) */
#include "game/race/modes/race_battles_internal.h"                           /* the unit's constants and data */

/* The instances <8> of the T137 templates are defined here (explicit
 * specializations of the members, declared before their first use). */
template <> RaceBattleInformationT<8>::RaceBattleInformationT();
template <> RaceBattleInformationT<8>::~RaceBattleInformationT();
template <> void RaceBattleInformationT<8>::addMissingEntry(int i);
template <> RaceBattleT<8>::RaceBattleT();
template <> RaceBattleT<8>::~RaceBattleT();

/* RaceLinkBattle::virtual_27: the frames a command exchange has waited
 * (.sbss). */
static int g_0035322C;                      /* 0x0035322C gp-0x47C4 */

/* ======================================================================== */
/* RaceLinkBattleInformation                                                */
/* ======================================================================== */

// FUNC 0x001848A8 RaceLinkBattleInformation::RaceLinkBattleInformation
//   RaceBattleInformationT<8>::RaceBattleInformationT, the vptr (+0x54),
//   then 6 entries and the race options of the host's game options, in
//   this order (its landing pad: ~RaceBattleInformationT, flags 0).
//   courseCode is not initialised here, and main_arcade (the only caller,
//   a stack object) does not call setCourseCode: getCourseCode reads
//   whatever the stack held (as in the code).
RaceLinkBattleInformation::RaceLinkBattleInformation()
{
    entryCount = 6;
    GameOptions *host = &getGameStatus()->menu.options[0];

    /* implicit: RaceBattleInformationT (the base's / members' constructors),
     * ~RaceBattleInformationT (their cleanup on a throw) */
    option.raceLaps = host->raceLaps;
    option.tireDamage = host->tireDamage;
    option.splitBattle.tireDamage = host->splitBattle.tireDamage;
    option.splitBattle.raceLaps = host->splitBattle.raceLaps;
    option.splitBattle.handicap = host->splitBattle.handicap;
    option.splitBattle.boost = host->splitBattle.boost;
}

// FUNC 0x00184950 RaceLinkBattleInformation::~RaceLinkBattleInformation
//   Slot 0.  The vptr, then RaceBattleInformationT<8>::~RaceBattleInformationT
//   with the caller's flags (it does the delete).  Caller: main_arcade.
RaceLinkBattleInformation::~RaceLinkBattleInformation()
{
    /* implicit: ~RaceBattleInformationT (the base / member destructors) */
}

// FUNC 0x00184978 RaceLinkBattleInformation::createEntries
//   Slot 4: RaceEntryPS2::operator new(0x3C), RaceEntryPS2::RaceEntryPS2
//   (operator delete(p, 0x3C) if it throws).
RaceEntryBase *RaceLinkBattleInformation::createEntries()
{
    return new RaceEntryPS2; /* implicit: RaceEntryPS2::operator delete (if the constructor throws) */
}

// FUNC 0x001849D0 RaceLinkBattleInformation::setupEntry
//   Slot 10.  Entry n is node n: a playing node's car gets its entry's car
//   and colour (index -> colour code), transmission, tyre and vibration,
//   input id 0x100 + n and three of the node's own game options; the
//   others are queued for assignCars.
//   Matching (96.6%): retail loads opts->steeringAssist1p ahead of the
//   transmission and inputId stores (another alias set than int); an enum
//   type for GameOptions::steeringAssist1p (and RaceEntryCar's) moves that
//   load but not e->tyre's (95.1%); the 720 orders of the six stores: no
//   better.
void RaceLinkBattleInformation::setupEntry(RaceEntryCar *car)
{
    int node = car->number;
    /* (retail adds the menu's offset first: the inline getMenuStatus) */
    GameOptions *opts = &getGameStatus()->getMenuStatus()->options[node];

    if (F_0022D7D0(node) != 0) {            /* the node plays */
        RaceEntry *e = &entries[node];
        RaceEntryInformation *info = &car->info;
        info->setCar(e->car);
        info->setColorCode(g_00352D98.getColor(info->getModelCode(), e->color));
        car->transmission = e->transmission;
        info->inputId = node + INPUT_ID_PLAYER;
        car->steeringAssist = opts->steeringAssist1p;
        car->tyre = e->tyre;
        car->vibration = e->vibration;
        car->assistAsm = opts->assistAsm;
        car->assistTcs = opts->assistTcs != 0;
        return;
    }
    addMissingEntry(node);
}

// FUNC 0x00184AD8 RaceLinkBattleInformation::setupOrganization
//   Slot 11.  The random seed the nodes share (F_0022DA08, the sum of their
//   random bytes), the cars of the queued entries (assignCars, slot 12),
//   then the start order: the playing nodes first (0, 1, ...), the other
//   entries after them.  The order table is a stack array of count words
//   (the code takes (count * 4 + 15) & ~15 bytes off $sp): an alloca (a VLA
//   gives egcs's `sll 5; srl 3' size in bits).
void RaceLinkBattleInformation::setupOrganization(RaceOrganization *org)
{
    int playing = 0;
    int i;

    setRandomSeed(F_0022DA08());
    RaceEntryBase *list = org->entries;
    assignCars(org, missing, missingCount);
    org->resetEntryOrder();

    int *order = (int *)__builtin_alloca(list->count * sizeof(int));
    int other = F_0022D850();               /* the playing nodes' count */
    for (i = 0; i < list->count; i++) {
        int pos;
        if (F_0022D7D0(i) != 0)
            pos = playing++;
        else
            pos = other++;
        order[i] = pos;
    }
    for (i = 0; i < list->count; i++)
        org->entries->cars[i]->order = order[i];
}

// FUNC 0x00184C28 RaceLinkBattleInformation::getCourseCode
//   Slot 2: the course set by setCourseCode, else the race database's
//   (RaceInformation::getCourseCode).
u64 RaceLinkBattleInformation::getCourseCode()
{
    if (courseCode == 0)
        return RaceInformation::getCourseCode();
    return courseCode;
}

// FUNC 0x00184C50 RaceLinkBattleInformation::setCourseCode
//   Slot 3.  A dirt course (a rally) is run with two cars: the kind itself
//   is stored as the entry count (it is 2).
void RaceLinkBattleInformation::setCourseCode(u64 code)
{
    courseCode = code;
    int kind = db.getCourseKind(code);
    if (kind == COURSE_KIND_DIRT)
        entryCount = kind;
}

/* ======================================================================== */
/* RaceLinkBattleInput / RaceLinkBattleNodeInput                            */
/* ======================================================================== */

// FUNC 0x00184C90 RaceLinkBattleInput::RaceLinkBattleInput
//   RaceInput::RaceInput, then the vptr (+0xC0).  Caller: the RaceLinkBattle
//   constructor.
RaceLinkBattleInput::RaceLinkBattleInput()
{
    /* implicit: RaceInput (the base's / members' constructors) */
}

// FUNC 0x00184CC8 RaceLinkBattleInput::~RaceLinkBattleInput
//   Slot 0.  The vptr, then RaceInput::~RaceInput with the caller's flags.
//   Callers: the RaceLinkBattle constructor's landing pad (flags 0) and
//   destructor (flags 2).
RaceLinkBattleInput::~RaceLinkBattleInput()
{
    /* implicit: ~RaceInput (the base / member destructors) */
}

// FUNC 0x00184CF0 RaceLinkBattleInput::updateSync
//   Slot 2: empty (Control::updateSync is not run for the local player).
void RaceLinkBattleInput::updateSync()
{
}

// FUNC 0x00184CF8 RaceLinkBattleInput::getControllerConfig
//   Slot 10: the controller settings this node published.
ControllerConfig *RaceLinkBattleInput::getControllerConfig()
{
    return &status->config;
}

// FUNC 0x00184D08 RaceLinkBattleNodeInput::getControllerConfig
//   Slot 10: the controller settings the node published.
ControllerConfig *RaceLinkBattleNodeInput::getControllerConfig()
{
    return &status->config;
}

/* ======================================================================== */
/* RaceLinkBattle: allocation, factory, construction, destruction           */
/* ======================================================================== */

// FUNC 0x00184D18 RaceLinkBattle::operator new
//   From the race pool, 16-byte aligned.
void *RaceLinkBattle::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00184D40 RaceLinkBattle::operator delete
//   The size (0x35000) is ignored.
void RaceLinkBattle::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00184D68 RaceLinkBattle::virtual_24
//   Slot 24: the class name (the registry's, 0x0032A360).
const char *RaceLinkBattle::virtual_24()
{
    return "RaceLinkBattle";
}

// FUNC 0x00184D78 race_link_battle_create
//   RaceClassInfo::create of "RaceLinkBattle" (g_002B7960 +8): operator
//   new(0x35000), the constructor (operator delete(p, 0x35000) if it
//   throws).
RaceBase *race_link_battle_create(void)
{
    return new RaceLinkBattle; /* implicit: RaceLinkBattle::operator delete (if the constructor throws) */
}

// FUNC 0x00184DD8 race_link_battle_construct
//   RaceClassInfo::construct (g_002B7960 +0xC): the constructor in buf
//   (none for NULL); arg is not used.  The global placement new of <new> (as
//   u045's race_basic_construct): egcs tests its result for NULL itself
//   (retail's daddu v0,a0 / beqz v0); GCC 14 does not, hence the test.
RaceBase *race_link_battle_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)
        return NULL;
#endif
    return ::new (buf) RaceLinkBattle;
}

/* The registry record of "RaceLinkBattle" (.data, before the frame info;
 * its "RaceLinkBattle" is virtual_24's literal, emitted once). */
RaceClassInfo g_002B7960 = {                /* 0x002B7960 */
    "RaceLinkBattle", sizeof(RaceLinkBattle), race_link_battle_create, race_link_battle_construct
};
/* Its node in the registry (zeros, linked in at start-up).  Retail's .sdata
 * has it before "sw_20" (drawConnectionError's): written here, at its
 * definition, not with the uninitialised globals at the end of the file.
 * The zero initialiser does that (an exact form: the original's node was
 * possibly an object whose constructor is the static initialiser). */
RaceClassNode g_00351E78 = { NULL, NULL };  /* 0x00351E78 */

// FUNC 0x00184E00 RaceLinkBattle::RaceLinkBattle
//   RaceBattleT<8>::RaceBattleT, the vptr, then the members in order:
//   DynamicsConductorBattleMP::DynamicsConductorBattleMP, input
//   (RaceLinkBattleInput::RaceLinkBattleInput), the 8 node inputs (inline:
//   RaceInput::RaceInput and the vptr), the 7 icons (CarIcon::CarIcon),
//   result (inline: RaceResultBase's done / player 0, the ResultArcade vptr,
//   ResultBoard::ResultBoard, the ResultLinkBattle vptr),
//   RacePause::RacePause, ReplayPause::ReplayPause(0),
//   RaceDisplay::RaceDisplay.  The landing pads destroy what is built
//   (flags 0): RaceDisplay::~RaceDisplay, ReplayPause::~ReplayPause,
//   RacePause::~RacePause, ResultBoard::~ResultBoard (flags 2, and the
//   RaceResultBase vptr), the node inputs through their virtual destructor,
//   RaceLinkBattleInput::~RaceLinkBattleInput, RaceBattleT<8>::~RaceBattleT.
//   A driving console (viewMode 0) uses its own input (virtual_25 is called
//   directly here); the view modes are set up by setupView.  Callers:
//   main_arcade, race_link_battle_create / construct.
RaceLinkBattle::RaceLinkBattle()
    : replayPause(0)
{
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();

    /* implicit: RaceLinkBattleInput, RaceBattleT, RaceDisplay, RacePause,
     * ReplayPause, CarIcon, ResultBoard, DynamicsConductorBattleMP, RaceInput
     * (the base's / members' constructors), ~RaceLinkBattleInput,
     * ~RaceBattleT, ~RaceDisplay, ~RacePause, ~ReplayPause, ~ResultBoard
     * (their cleanup on a throw) */
    setRaceMode("RACE_MODE_SINGLE");
    dc = &dcBattle;
    nodeId = F_0022D830();
    viewMode = g_00352CA8;
    if (g_00352CA8 == 0) {
        setActiveInput(&input);
        GameOptions *opts = &menu->options[nodeId];
        input.setStatus(opts);
        g_002EA7F8.attach(virtual_25(), PAD_SLOT_STEERING1);
    } else {
        setupView();
    }
    for (int node = 0; node < LINK_NODES; node++) {
        RaceLinkBattleNodeInput *in = &nodeInputs[node];
        in->setStatus(&menu->options[node]);
        g_002EA7F8.attach(in, PAD_SLOT_LINK + node);
        in->rec.attachBuffer(node);
    }
    setRaceDisplay(&display);
}

// FUNC 0x00185208 RaceLinkBattle::~RaceLinkBattle
//   Slot 0.  The vptr; the own input leaves the pad manager when it is the
//   active one (virtual_25 called directly, twice), then the 8 node inputs.
//   Then the members, last first (flags 2): RaceDisplay::~RaceDisplay,
//   ReplayPause::~ReplayPause, RacePause::~RacePause, result (inline:
//   ResultBoard::~ResultBoard, the RaceResultBase vptr), the node inputs
//   (their virtual destructor, flags 0, the last first),
//   RaceLinkBattleInput::~RaceLinkBattleInput; RaceBattleT<8>::~RaceBattleT
//   (flags 0); operator delete(this, 0x35000) when flags & 1.  dcBattle and
//   the icons have no destructor.  Caller: main_arcade.
RaceLinkBattle::~RaceLinkBattle()
{
    if (virtual_25() == &input)
        g_002EA7F8.detach(virtual_25());
    for (int node = 0; node < LINK_NODES; node++)
        g_002EA7F8.detach(&nodeInputs[node]);
    /* implicit: ~RaceLinkBattleInput, ~RaceBattleT, ~RaceDisplay, ~RacePause,
     * ~ReplayPause, ~ResultBoard (the base / member destructors) */
}

// FUNC 0x00185398 RaceLinkBattle::virtual_5
//   Slot 5: 1.
int RaceLinkBattle::virtual_5()
{
    return 1;
}

// FUNC 0x001853A0 RaceLinkBattle::virtual_8
//   Slot 8, the race start.  The argument is not used.  A driving console
//   whose session only watches (g_00352CB0) runs the race as a replay
//   (RaceBasic::virtual_8(1), the display's "LIVE", the replay pause menu,
//   no result, the input in replay mode 1, the camera on the leader); else
//   the race itself, the camera on the own car or, on a screen of the wall,
//   on viewCar, with the wall layout and the race display's panels of the
//   camera manager.  The result's player is the own node.  Then the session
//   starts (F_0022D870).
void RaceLinkBattle::virtual_8(int replay)
{
    int watchOnly = g_00352CB0;
    result.setCarNumber(nodeId);
    int onWall = viewMode != 0;
    int asReplay;
    if (!onWall && watchOnly != 0) {
        asReplay = 1;
        display.setLive(true);              /* inline (its copy 0x00195840 has no callers) */
    } else {
        asReplay = 0;
    }
    RaceBasic::virtual_8(asReplay);
    if (asReplay != 1) {
        setPauseMenu(&racePause);
        setSubDisplay(&result);
    } else {
        replayPause.F_001A3248(org);
        setPauseMenu(&replayPause);
        setSubDisplay(NULL);
    }
    pauseRequest = 0;
    state = LINK_BATTLE_RUNNING;
    raceDisplay->setPlayerCarNumber(nodeId);

    RaceOrganization *o = org;
    CameraManager *cam = virtual_6();
    if (onWall) {
        virtual_25()->setReplayMode(0);
        cam->views[cam->current].targetInfo.setCar(viewCar, 0);
        cam->setScreenLayout(&viewParams);
        raceDisplay->setTopLeftPanel(cam->isCenterTopLeft());
        raceDisplay->setTopRightPanel(cam->isCenterTopRight());
        raceDisplay->setBottomLeftPanel(cam->isCenterBottomLeft());
        raceDisplay->setBottomRightPanel(cam->isCenterBottomRight());
        raceDisplay->setCenterPanel(cam->isCenterScreen());
    } else if (watchOnly == 0) {
        virtual_25()->setReplayMode(0);
        cam->views[cam->current].targetInfo.setCar(nodeId, 0);
    } else {
        virtual_25()->setReplayMode(1);
        cam->views[cam->current].targetInfo.setCar(o->entries->rankTop->number, 0);
    }
    F_0022D870();
}

// FUNC 0x001856F0 RaceLinkBattle::virtual_43
//   Slot 43 (before the race runs): the cars' icons are made and given to
//   the result board; the race display shows none.
void RaceLinkBattle::virtual_43()
{
    car_icons_make(icons, org);
    result.setDisplays(icons);
    display.setCarIcons(NULL);              /* inline (as RaceArcade::virtual_43) */
}

// FUNC 0x00185758 RaceLinkBattle::virtual_33
//   Slot 33: sends the end to the others (F_0022DA10, 0x80), then
//   RacePS2Base::virtual_33.
void RaceLinkBattle::virtual_33()
{
    F_0022DA10();
    RacePS2Base::virtual_33();
}

// FUNC 0x00185788 RaceLinkBattle::virtual_35
//   Slot 35 (the end of the race): RaceBasic::virtual_35, then the session
//   ends (F_0022D9A8).
void RaceLinkBattle::virtual_35()
{
    RaceBasic::virtual_35();
    F_0022D9A8();
}

// FUNC 0x001857B0 RaceLinkBattle::setActiveInput
//   Callers: the constructor, setupView.
void RaceLinkBattle::setActiveInput(RaceInput *in)
{
    activeInput = in;
}

// FUNC 0x001857C8 RaceLinkBattle::virtual_25
//   Slot 25: the input this console uses.  Also called directly by the
//   constructor and the destructor.
RaceInput *RaceLinkBattle::virtual_25()
{
    return activeInput;
}

// FUNC 0x001857E0 RaceLinkBattle::virtual_26
//   Slot 26: input id 0x100 + n is node n's input while its port is ready
//   (Control::isPortReady); anything else gets the inert input g_002DFB58.
//   (The id is only checked against 0x100, not against the 8 nodes.)
RaceInput *RaceLinkBattle::virtual_26(int id)
{
    if (id >= INPUT_ID_PLAYER) {
        int node = id - INPUT_ID_PLAYER;    /* (its own variable: retail does not fold -0x100 * 0x14C) */
        RaceLinkBattleNodeInput *in = &nodeInputs[node];
        if (in->isPortReady() != 0)
            return in;
    }
    return &g_002DFB58;
}

// FUNC 0x00185848 RaceLinkBattle::virtual_27
//   Slot 27, once per frame (0: no frame).  While RacePS2Base's holds the
//   race, the frames a command exchange waits for us (F_0022CD18) are
//   counted and the link is reset (F_0026EB90) after 601 of them.
//   Otherwise the session's state: a bus reset (2 / 3) ends the race; else
//   the frame runs when the pad manager's link exchange went through.
int RaceLinkBattle::virtual_27()
{
    if (RacePS2Base::virtual_27() == 0) {
        if (F_0022CD18() != 0)
            g_0035322C++;
        if (g_0035322C >= LINK_WAIT_RESET_FRAMES) {
            g_0035322C = 0;
            F_0026EB90();
        }
        linkActive = 0;
    } else {
        g_0035322C = 0;
        int session = F_0022CD28();
        linkResult = session;
        if ((u32)(session - LINK_STATE_RESET) < 2) {    /* LINK_STATE_RESET, LINK_STATE_RESET48 */
            linkActive = 1;
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
        } else {
            linkActive = g_002EA7F8.updateLink();
        }
    }
    return linkActive;
}

// FUNC 0x00185920 RaceLinkBattle::virtual_28
//   Slot 28, the frame.  RacePS2Base::virtual_28 (RaceBasic's is skipped),
//   the display events (race_display_poll_events); an end received from
//   another node (F_0022DA30) ends the race.  The race display: the camera
//   view's target car, its update, the view camera's mount (the camera
//   manager is asked twice); the result sequence; the display events
//   dropped.  Last, on a console that does not only watch, START asks the
//   others for the pause (F_0022CD30, 0x81).
void RaceLinkBattle::virtual_28(float dt)
{
    RacePS2Base::virtual_28(dt);
    race_display_poll_events(org);
    if (F_0022DA30() != 0)
        org->eventQueue.post(RACE_EVENT_END, 0, 0);

    raceDisplay->setTargetCarNumber(virtual_6()->F_00209F10());   /* the current view's target */
    raceDisplay->virtual_2(RACE_DISPLAY_STEP);  /* the enhanced engine: a frame (hud.md C19) */
    raceDisplay->virtual_9(virtual_6()->views[0].camera->getMount());

    RaceResultBase *res = subDisplay;
    if (res != NULL) {
        RaceOrganization *o = org;          /* (read before the call: retail keeps it in $s2) */
        res->update(*o, virtual_25());
    }
    race_display_drop_events(org);

    if (g_00352CB0 == 0) {
        if (virtual_25()->getButtonsOn() & CONTROL_START) {
            pauseRequest = 1;
            F_0022CD30();
        }
    }
}

// FUNC 0x00185AF0 RaceLinkBattle::drawConnectionError
//   A 300 x 80 box (colour 0x40000010) centred on (320, 300) and the text
//   RACEDISPLAY PleaseConnect (g_002C43E8[3]) centred on it, in "sw_20",
//   colour 0x804040FF.  `this' is not used.  Caller: updateLink.
void RaceLinkBattle::drawConnectionError()
{
    gfx_enable(PGL_CAP_BLEND);
    pgl_color(0x40000010);
    draw_rect(320.0f, 300.0f, 300.0f, 80.0f);
    g_002F1968.setStyle("sw_20");
    g_002F1968.setColor(0x804040FF);
    g_002F1968.setPos(320, 300);
    g_002F1968.setAlign(FONT_ALIGN_HCENTER | FONT_ALIGN_VCENTER);
    g_002F1968.print(g_002C43E8[RACEDISPLAY_PLEASE_CONNECT]);
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
}

// FUNC 0x00185BA0 RaceLinkBattle::drawWaitingForResponse
//   ? name (unreferenced).  As drawConnectionError with a 380-wide box, the
//   colour 0x8040FF40 and the text WaitOther (g_002C43E8[4]).
void RaceLinkBattle::drawWaitingForResponse()
{
    gfx_enable(PGL_CAP_BLEND);
    pgl_color(0x40000010);
    draw_rect(320.0f, 300.0f, 380.0f, 80.0f);
    g_002F1968.setStyle("sw_20");
    g_002F1968.setColor(0x8040FF40);
    g_002F1968.setPos(320, 300);
    g_002F1968.setAlign(FONT_ALIGN_HCENTER | FONT_ALIGN_VCENTER);
    g_002F1968.print(g_002C43E8[RACEDISPLAY_WAIT_OTHER]);
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
}

// FUNC 0x00185C58 RaceLinkBattle::updateLink
//   The link state machine, once per drawn frame (virtual_46):
//     ERROR: draws the message, then as RUNNING;
//     RUNNING: when a command exchange waits for us (F_0022CD18): our own
//       pause request pauses the race (linkResult 1, setPause(1),
//       stopSounds) and opens our menu (PAUSED); else an end received
//       (F_0022DA30) ends the race, and a session state other than RUN
//       makes us WAIT: a bus reset also ends the race (and stores ERROR,
//       which WAITING overwrites at once, as in the code); the race pauses
//       (stopSounds) unless the state is RESET48;
//     PAUSED: the link lost (F_0022CCB0 0) ends the race (ERROR); the menu's
//       continue / exit (exit also sends the end, F_0022DA10) make us WAIT;
//       the menu is drawn (with view 0's camera);
//     WAITING: the link lost ends the race (ERROR); else once the others
//       took the exchange (F_0022CCD8, always 1) RUNNING again, and the race
//       resumes (startSounds) for the states RUN, COMMAND (also setPause(0))
//       and RESET.
void RaceLinkBattle::updateLink(GSBuffer &buf)
{
    int current = state;

    switch (current) {
    case LINK_BATTLE_ERROR:
        drawConnectionError();
        /* fall through */
    case LINK_BATTLE_RUNNING: {
        if (F_0022CD18() == 0)
            return;
        if (pauseRequest != 0) {
            linkResult = LINK_STATE_COMMAND;
            setPause(1);
            stopSounds();
            state = LINK_BATTLE_PAUSED;
            pauseRequest = 0;
            return;
        }
        if (F_0022DA30() != 0)
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
        int session = F_0022CD28();
        linkResult = session;
        if (session == LINK_STATE_RUN)
            return;
        if (session != LINK_STATE_COMMAND) {
            state = LINK_BATTLE_ERROR;
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
        }
        state = LINK_BATTLE_WAITING;
        if (linkResult == LINK_STATE_RESET48)
            return;
        stopSounds();
        return;
    }
    case LINK_BATTLE_PAUSED: {
        if (F_0022CCB0() == 0) {
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
            state = LINK_BATTLE_ERROR;
        }
        switch (pauseMenu->virtual_2(virtual_25(), org)) {
        case PAUSE_RESULT_EXIT:
            F_0022DA10();
            /* fall through */
        case PAUSE_RESULT_CONTINUE:
            state = LINK_BATTLE_WAITING;
            break;
        /* (the three empty cases shape retail's decision tree: == 3, then
         * <= 3, then != 4) */
        case PAUSE_RESULT_PAUSED:
        case PAUSE_RESULT_INPUT:
        case PAUSE_RESULT_TIMEOUT:
            break;
        }
        RaceOrganization *o = org;
        CameraManager *cm = virtual_6();
        pauseMenu->virtual_3(&buf, o, cm->views[0].camera);
        return;
    }
    case LINK_BATTLE_WAITING: {
        if (F_0022CCB0() == 0) {
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
            state = LINK_BATTLE_ERROR;
            return;
        }
        if (F_0022CCD8() == 0)
            return;
        if (F_0022DA30() != 0)
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
        state = LINK_BATTLE_RUNNING;
        switch (linkResult) {             /* (a switch: retail's tree == 1, < 2, == 0, == 2; the
                                               2 compared through the register holding `current') */
        case LINK_STATE_COMMAND:
            setPause(0);
            /* fall through */
        case LINK_STATE_RUN:
        case LINK_STATE_RESET:
            startSounds();
            break;
        }
        return;
    }
    }
}

// FUNC 0x00185F78 RaceLinkBattle::virtual_46
//   Slot 46 (the drawn frame): RacePS2Base::virtual_46 (the error view),
//   then updateLink.
void RaceLinkBattle::virtual_46(GSBuffer *buf)
{
    RacePS2Base::virtual_46(buf);
    updateLink(*buf);
}

// FUNC 0x00185FB8 RaceLinkBattle::setupView
//   Called by the constructor when viewMode != 0.  rel is this node's
//   position after the playing nodes (F_0022D850); rel < 0: it is one of
//   them and drives a car.  The view mode (jump table 0x0032A390; the rel
//   tables 0x0032A3C0, 0x0032A3E0, 0x0032A400) gives the car shown and the
//   screen's place in the wall: (modeX, nX) 1 / n the n-th column from the
//   right (n = 2: left of the centre), 2 / n from the left; modeY 1 / nY 2
//   the row above; 0 the centre.  flags starts as WALL_TURN_45; a rel a
//   mode does not know leaves the place as it was.  A driving console then
//   uses its own input (its options, pad slot 0x14 through virtual_25), the
//   others node viewCar's input.
//   Matching: the cases in retail's block order (3, 4 / 5, 2, 1, 6..9;
//   jump2 cross-jumps the identical store tails), direct viewParams
//   stores, and the store order of every block searched (singly, and
//   jointly for the blocks that share a tail; build/scratch/u047
//   sv_search*.py).  99.7%: two cross-jumped tails store nY / modeY of 0
//   the other way round (a sched2 tie no order reached).
void RaceLinkBattle::setupView()
{
    int rel = nodeId - F_0022D850();
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    int drives = (u32)rel >> 31;

    viewParams.flags = WALL_TURN_45;
    switch (viewMode) {
    case 3:     /* the driver in the centre, the screen left of it */
        viewCar = 0;
        if (drives) {
            viewParams.modeX = 0;
            viewParams.nX = 0;
            viewParams.modeY = 0;
            viewParams.nY = 0;
        } else {
            viewParams.modeX = WALL_COUNT_FROM_RIGHT;
            viewParams.nX = 2;
            viewParams.nY = 0;
            viewParams.modeY = 0;
        }
        break;
    case 4:
    case 5:     /* two drivers, a screen left and right of each (cars 0 / 1) */
        viewParams.modeY = 0;
        viewParams.nY = 0;
        if (!drives) {
            viewParams.modeX = (rel & 1) ? WALL_COUNT_FROM_LEFT : WALL_COUNT_FROM_RIGHT;
            viewParams.nX = 2;
            viewCar = rel >= 2;
        } else {
            viewParams.modeX = 0;
            viewParams.nX = 0;
            viewCar = nodeId;
        }
        break;
    case 2:     /* three columns, each shifted + / - (? stereo pairs) */
        viewParams.modeY = 0;
        viewParams.nY = 0;
        viewCar = 0;
        switch (rel) {
        case -1:
            viewParams.modeX = 0;
            viewParams.nX = 0;
            viewParams.flags |= WALL_SHIFT_PLUS;
            break;
        case 0:
            viewParams.modeX = 0;
            viewParams.nX = 0;
            viewParams.flags |= WALL_SHIFT_MINUS;
            break;
        case 1:
            viewParams.modeX = WALL_COUNT_FROM_RIGHT;
            viewParams.nX = 2;
            viewParams.flags |= WALL_SHIFT_PLUS;
            break;
        case 2:
            viewParams.modeX = WALL_COUNT_FROM_LEFT;
            viewParams.nX = 2;
            viewParams.flags |= WALL_SHIFT_PLUS;
            break;
        case 3:
            viewParams.modeX = WALL_COUNT_FROM_RIGHT;
            viewParams.nX = 2;
            viewParams.flags |= WALL_SHIFT_MINUS;
            break;
        case 4:
            viewParams.modeX = WALL_COUNT_FROM_LEFT;
            viewParams.nX = 2;
            viewParams.flags |= WALL_SHIFT_MINUS;
            break;
        }
        break;
    case 1:     /* one screen, shifted + (driver) / - (? a stereo pair) */
        viewParams.modeX = 0;
        viewParams.nX = 0;
        viewParams.modeY = 0;
        viewParams.nY = 0;
        viewCar = 0;
        viewParams.flags = (viewParams.flags & ~WALL_TURN_45) | (drives ? WALL_SHIFT_PLUS : WALL_SHIFT_MINUS);
        break;
    case 6:     /* the driver and the screen above it */
        viewParams.modeX = 0;
        viewParams.nX = 0;
        viewParams.flags &= ~WALL_TURN_45;
        viewCar = 0;
        if (drives) {
            viewParams.nY = 0;
            viewParams.modeY = 0;
        } else {
            viewParams.modeY = WALL_COUNT_FROM_BOTTOM;
            viewParams.nY = 2;
        }
        break;
    case 7:     /* a 2 x 2 wall: the driver, left, above, left above */
        viewCar = 0;
        switch (rel) {
        case -1:
            viewParams.modeX = 0;
            viewParams.nX = 0;
            viewParams.modeY = 0;
            viewParams.nY = 0;
            break;
        case 0:
            viewParams.modeX = WALL_COUNT_FROM_RIGHT;
            viewParams.nX = 2;
            viewParams.nY = 0;
            viewParams.modeY = 0;
            break;
        case 1:
            viewParams.modeY = WALL_COUNT_FROM_BOTTOM;
            viewParams.modeX = 0;
            viewParams.nX = 0;
            viewParams.nY = 2;
            break;
        case 2:
            viewParams.modeX = WALL_COUNT_FROM_RIGHT;
            viewParams.modeY = WALL_COUNT_FROM_BOTTOM;
            viewParams.nX = 2;
            viewParams.nY = 2;
            break;
        }
        break;
    case 8:     /* five screens in a row, the wall turned a step a screen */
        viewCar = 0;
        viewParams.flags |= WALL_TURNED;
        switch (rel) {
        case -1:
            viewParams.modeX = 0;
            viewParams.nX = 0;
            viewParams.modeY = 0;
            viewParams.nY = 0;
            break;
        case 0:
            viewParams.modeX = WALL_COUNT_FROM_RIGHT;
            viewParams.nX = 2;
            viewParams.nY = 0;
            viewParams.modeY = 0;
            break;
        case 1:
            viewParams.nY = 0;
            viewParams.modeX = WALL_COUNT_FROM_LEFT;
            viewParams.modeY = 0;
            viewParams.nX = 2;
            break;
        case 2:
            viewParams.modeX = WALL_COUNT_FROM_RIGHT;
            viewParams.nX = 3;
            viewParams.nY = 0;
            viewParams.modeY = 0;
            break;
        case 3:
            viewParams.modeX = WALL_COUNT_FROM_LEFT;
            viewParams.nX = 3;
            viewParams.nY = 0;
            viewParams.modeY = 0;
            break;
        }
        break;
    case 9:     /* a 3 x 2 wall: the driver, above, left, right, left / right above */
        viewCar = 0;
        switch (rel) {
        case -1:
            viewParams.modeX = 0;
            viewParams.nX = 0;
            viewParams.modeY = 0;
            viewParams.nY = 0;
            break;
        case 0:
            viewParams.modeY = WALL_COUNT_FROM_BOTTOM;
            viewParams.modeX = 0;
            viewParams.nX = 0;
            viewParams.nY = 2;
            break;
        case 1:
            viewParams.modeX = WALL_COUNT_FROM_RIGHT;
            viewParams.nX = 2;
            viewParams.nY = 0;
            viewParams.modeY = 0;
            break;
        case 2:
            viewParams.nY = 0;
            viewParams.modeX = WALL_COUNT_FROM_LEFT;
            viewParams.modeY = 0;
            viewParams.nX = 2;
            break;
        case 3:
            viewParams.modeX = WALL_COUNT_FROM_RIGHT;
            viewParams.modeY = WALL_COUNT_FROM_BOTTOM;
            viewParams.nX = 2;
            viewParams.nY = 2;
            break;
        case 4:
            viewParams.modeY = WALL_COUNT_FROM_BOTTOM;
            viewParams.modeX = WALL_COUNT_FROM_LEFT;
            viewParams.nX = 2;
            viewParams.nY = 2;
            break;
        }
        break;
    }

    if (drives) {
        setActiveInput(&input);
        GameOptions *opts = &menu->options[nodeId];
        input.setStatus(opts);
        g_002EA7F8.attach(virtual_25(), PAD_SLOT_STEERING1);
        return;
    }
    setActiveInput(&nodeInputs[viewCar]);
}

/* ======================================================================== */
/* RaceBattleInformationT<8>, RaceBattleT<8>                                */
/* ======================================================================== */

// FUNC 0x001864C8 RaceBattleInformationT<8>::RaceBattleInformationT
//   RaceInformation::RaceInformation, the vptr (+0x54), the 8 entries'
//   CarEquipments (CarEquipments::CarEquipments), then every entry empty (no
//   car, colour -1) and no queued entry.  The landing pads destroy the
//   CarEquipments built so far (CarEquipments::~CarEquipments, flags 2) and
//   RaceInformation::~RaceInformation.  Caller: the RaceLinkBattleInformation
//   constructor.
template <>
RaceBattleInformationT<8>::RaceBattleInformationT()
{
    /* implicit: RaceInformation, CarEquipments (the base's / members'
     * constructors), ~RaceInformation, ~CarEquipments (their cleanup on a
     * throw) */
    for (int i = 0; i < 8; i++) {
        RaceEntry *e = &entries[i];         /* one pointer for the five stores, as retail (u046's <2>) */
        e->car = 0;
        e->color = -1;
        e->transmission = 0;
        e->tyre = 0;
        e->vibration = 0;
    }
    missingCount = 0;
}

// FUNC 0x001865F0 RaceBattleInformationT<8>::~RaceBattleInformationT
//   Slot 0.  The vptr, the 8 entries' CarEquipments::~CarEquipments from the
//   last (flags 2), then RaceInformation::~RaceInformation with the caller's
//   flags.  Callers: the RaceLinkBattleInformation constructor's landing pad
//   and destructor.
template <>
RaceBattleInformationT<8>::~RaceBattleInformationT()
{
    /* implicit: ~RaceInformation, ~CarEquipments (the base / member
     * destructors) */
}

// FUNC 0x00186678 RaceBattleInformationT<8>::addMissingEntry
//   ? name.  Queues entry i for assignCars (no bounds check).  Caller:
//   RaceLinkBattleInformation::setupEntry.
template <>
void RaceBattleInformationT<8>::addMissingEntry(int i)
{
    missing[missingCount++] = i;
}

// FUNC 0x00186698 RaceBattleT<8>::RaceBattleT
//   RaceBasic::RaceBasic, then the vptr.  Caller: the RaceLinkBattle
//   constructor.
template <>
RaceBattleT<8>::RaceBattleT()
{
    /* implicit: RaceBasic (the base's / members' constructors) */
}

// FUNC 0x001866D0 RaceBattleT<8>::~RaceBattleT
//   Slot 0.  The vptr, RaceBasic::~RaceBasic (flags 0), RaceBasic's operator
//   delete(this, 0xC8C0) when flags & 1.  Callers: the RaceLinkBattle
//   constructor's landing pad and destructor.
template <>
RaceBattleT<8>::~RaceBattleT()
{
    /* implicit: ~RaceBasic, operator delete (the base / member destructors,
     * flags & 1) */
}

// FUNC 0x00186728 race_link_battle_static_init
//   .ctors entry 0x0028F108: registers "RaceLinkBattle".
void race_link_battle_static_init(void)
{
    race_class_register(&g_00351E78, &g_002B7960);
}

// FUNC 0x00186750 RaceLinkBattle::virtual_30
//   Slot 30 (RaceBasic's frame while paused): empty, the pause menu runs in
//   updateLink.
/* inline in game/race/modes/T138_race_link_battle.h; egcs emits this copy with the
 * vtable (empty) */

/* ======================================================================== */
/* RTTI and the small members emitted here                                  */
/* ======================================================================== */

#ifndef BOOT_OLD_GCC
// FUNC 0x00186758 __tf14RaceLinkBattle
//   The __si_type_info __ti14RaceLinkBattle (name 0x0032AA50, base
//   __tit11RaceBattleT1i8).
extern "C" const type_info &__tf14RaceLinkBattle(void)
{
    if (__ti14RaceLinkBattle[0] == 0) {
        __tft11RaceBattleT1i8();
        __rtti_si(__ti14RaceLinkBattle, "14RaceLinkBattle",
                  (const type_info *)__tit11RaceBattleT1i8);
    }
    return *(const type_info *)__ti14RaceLinkBattle;
}

// FUNC 0x001867A8 __tft11RaceBattleT1i8
//   The __si_type_info __tit11RaceBattleT1i8 (name 0x0032AA68, base
//   __ti9RaceBasic).  Caller: __tf14RaceLinkBattle.
extern "C" const type_info &__tft11RaceBattleT1i8(void)
{
    if (__tit11RaceBattleT1i8[0] == 0) {
        __tf9RaceBasic();
        __rtti_si(__tit11RaceBattleT1i8, "t11RaceBattleT1i8",
                  (const type_info *)__ti9RaceBasic);
    }
    return *(const type_info *)__tit11RaceBattleT1i8;
}
#endif

// FUNC 0x001867F8 RaceLinkBattleNodeInput::setStatus
/* inline in game/race/modes/T138_race_link_battle.h (the constructor expands it); egcs
 * emits this copy with the vtable: status = s */

#ifndef BOOT_OLD_GCC
// FUNC 0x00186800 __tf23RaceLinkBattleNodeInput
//   The __si_type_info __ti23RaceLinkBattleNodeInput (name 0x0032AA80, base
//   __ti9RaceInput).
extern "C" const type_info &__tf23RaceLinkBattleNodeInput(void)
{
    if (__ti23RaceLinkBattleNodeInput[0] == 0) {
        __tf9RaceInput();
        __rtti_si(__ti23RaceLinkBattleNodeInput, "23RaceLinkBattleNodeInput",
                  (const type_info *)__ti9RaceInput);
    }
    return *(const type_info *)__ti23RaceLinkBattleNodeInput;
}
#endif

// FUNC 0x00186850 RaceLinkBattleNodeInput::~RaceLinkBattleNodeInput
/* implicit in game/race/modes/T138_race_link_battle.h; egcs emits this copy with the
 * vtable (slot 0): no vptr store, only RaceInput::~RaceInput with the
 * caller's flags */
/* implicit: ~RaceInput (the base / member destructors) */

// FUNC 0x00186870 RaceLinkBattleInput::setStatus
/* inline in game/race/modes/T138_race_link_battle.h (the constructor and setupView
 * expand it); egcs emits this copy with the vtable: status = s */

#ifndef BOOT_OLD_GCC
// FUNC 0x00186878 __tf19RaceLinkBattleInput
//   The __si_type_info __ti19RaceLinkBattleInput (name 0x0032AAA0, base
//   __ti9RaceInput).
extern "C" const type_info &__tf19RaceLinkBattleInput(void)
{
    if (__ti19RaceLinkBattleInput[0] == 0) {
        __tf9RaceInput();
        __rtti_si(__ti19RaceLinkBattleInput, "19RaceLinkBattleInput",
                  (const type_info *)__ti9RaceInput);
    }
    return *(const type_info *)__ti19RaceLinkBattleInput;
}

// FUNC 0x001868C8 __tf25RaceLinkBattleInformation
//   The __si_type_info __ti25RaceLinkBattleInformation (name 0x0032AAB8,
//   base __tit22RaceBattleInformationT1i8).
extern "C" const type_info &__tf25RaceLinkBattleInformation(void)
{
    if (__ti25RaceLinkBattleInformation[0] == 0) {
        __tft22RaceBattleInformationT1i8();
        __rtti_si(__ti25RaceLinkBattleInformation, "25RaceLinkBattleInformation",
                  (const type_info *)__tit22RaceBattleInformationT1i8);
    }
    return *(const type_info *)__ti25RaceLinkBattleInformation;
}

// FUNC 0x00186918 __tft22RaceBattleInformationT1i8
//   The __si_type_info __tit22RaceBattleInformationT1i8 (name 0x0032AAD8,
//   base __ti15RaceInformation, gp-0x4678).  Caller:
//   __tf25RaceLinkBattleInformation.
extern "C" const type_info &__tft22RaceBattleInformationT1i8(void)
{
    if (__tit22RaceBattleInformationT1i8[0] == 0) {
        __tf15RaceInformation();
        __rtti_si(__tit22RaceBattleInformationT1i8, "t22RaceBattleInformationT1i8",
                  (const type_info *)__ti15RaceInformation);
    }
    return *(const type_info *)__tit22RaceBattleInformationT1i8;
}
#endif
