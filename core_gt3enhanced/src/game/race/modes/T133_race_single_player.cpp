/* T133_race_single_player.cpp - RaceSinglePlayerInformation and
 * RaceSinglePlayer (0x001817F8..0x00182118); the classes are declared in
 * include/game/race/modes/T133_race_single_player.h.
 *
 * RaceSinglePlayerInformation describes a race with one human player: the
 * player's car (code, colour index, equipments, transmission, tyre and
 * vibration) in entry slot 0.  It creates a RaceEntryPS2 list, gives the
 * player inputId 0x100, fills the other slots with opponents
 * (assignCarRange from 1) and puts the player last on the grid.  The number
 * of cars is 6, 4 on courses of kind 1 and 2 on the dirt courses.
 *
 * RaceSinglePlayer is RaceBasicWithRaceDisplay plus the player's RaceInput
 * (pad slot 0x14).  The input's replay record is what the replay data
 * holds after the 0x191C-byte race info block (virtual_13 / 14 / 15 load,
 * save and size it); in a race (replay mode 0) a full record posts event
 * 0x16, in a replay (mode 1) its end posts event 1 (the end).  Each frame it
 * hands the race display the car the current view follows.
 *
 * The create / construct functions are the RaceClassInfo entries of the
 * class registry (T129); the .ctors entry registers "RaceSinglePlayer".
 *
 * Includes (u046's data pass): the classes retail's T133 saw, in its order.
 * Its .rodata (0x003287A8..0x00328B18) is "bad_alloc" (<new>), the class
 * name, the two vtables, then the type names in reverse definition order:
 * its own two, then T154_race_display.h's (RaceMiniMap .. the meters,
 * RaceCameraBase, UnitArena<64>, RaceValueDisplayBase, RaceDisplayBase): the
 * race display header comes before the own one (which reads T130 / T129 and
 * their RaceCameraBase), as T132's.  No menu or garage class.  Its .sdata
 * (0x00351E18..0x00351E28) is the "" of T157's inline
 * RaceMessageDisplay::clear (read with the race display), then the registry
 * node g_00351E20 (an uninitialised global: zeros at the end).
 */
// DATA_START 0x003287A8
// DATA_START 0x00351E18
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (the construct function); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/race/display/T154_race_display.h"         /* RaceDisplayBase, race_display_poll / drop_events (first:
                                               retail's type-name order) */
#include "game/race/modes/T133_race_single_player.h"   /* own: RaceSinglePlayer(Information); + T132 RaceBasic.., T130
                                               RacePS2Base, RaceEntryPS2, T129 RaceBase (RaceClassInfo /
                                               RaceClassNode, race_class_register, RACE_LIVE ..), T124
                                               RaceInformation, T000 RaceEntry, T413 RaceInput */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar, RaceEntryInformation */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post, RACE_EVENT_* */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, eventQueue, the entry order) */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord::Manager (the replay record) */
#include "game/camera/T383_camera_manager.h"       /* CameraManager, CameraView */
#include "game/camera/T397_variable_camera_base.h" /* VariableCameraBase::getMount */
#include "engine/input/T475_pad_manager.h"          /* PadManager g_002EA7F8 */
#include "game/data/T482_car_equipments.h"       /* CarEquipments */
#include "game/data/T484_race_database.h"        /* RaceDataBase (getCourse, setCourse, getCourseKind) */
#include "game/race/modes/race_modes_internal.h"                           /* the unit's constants */

/* ======================================================================== */
/* RaceSinglePlayerInformation                                              */
/* ======================================================================== */

// FUNC 0x001817F8 RaceSinglePlayerInformation::RaceSinglePlayerInformation
//   RaceInformation (RaceInformation::RaceInformation), the vptr, entry.equip
//   (CarEquipments::CarEquipments); then 6 cars and an empty player entry.
//   The landing pad runs RaceInformation::~RaceInformation.  Caller:
//   RaceArcadeInformation's constructor.
RaceSinglePlayerInformation::RaceSinglePlayerInformation()
{
    /* implicit: RaceInformation, CarEquipments (the base's / members'
     * constructors), ~RaceInformation (their cleanup on a throw) */
    RaceEntry *e = &entry;                  /* retail: the entry's address in s1 (its stores),
                                               this order is the closest found (97.3%) */
    e->color = -1;
    entryCount = 6;
    e->car = 0;
    e->transmission = 0;
    e->tyre = 0;
    e->vibration = 0;
}

// FUNC 0x00181890 RaceSinglePlayerInformation::~RaceSinglePlayerInformation
//   The vptr, ~entry.equip (CarEquipments::~CarEquipments, flags 2), then
//   RaceInformation::~RaceInformation with the caller's flags (it does the
//   operator delete).
RaceSinglePlayerInformation::~RaceSinglePlayerInformation()
{
    /* implicit: ~RaceInformation, ~CarEquipments (the base / member
     * destructors) */
}

// FUNC 0x001818E0 RaceSinglePlayerInformation::getCourseCode
//   (slot 2) The same code as RaceInformation's: the race DB's course.
u64 RaceSinglePlayerInformation::getCourseCode()
{
    return db.getCourse();
}

// FUNC 0x00181900 RaceSinglePlayerInformation::setCourseCode
//   (slot 3) The course, then the number of cars from its kind (the stored
//   course word, read back): kind 1 -> 4 cars, dirt -> 2, else unchanged.
void RaceSinglePlayerInformation::setCourseCode(u64 code)
{
    db.setCourse(code);
    switch (db.getCourseKind(db.course)) {
    case COURSE_KIND_DIRT:                  /* retail's block order: reorg puts this case's store
                                               in a beql slot, the kind-1 case runs into the end */
        entryCount = 2;
        break;
    case COURSE_KIND_1:
        entryCount = 4;
        break;
    }
}

// FUNC 0x00181958 RaceSinglePlayerInformation::createEntries
//   (slot 4) new RaceEntryPS2 (0x3C bytes, its own pool operator new;
//   RaceEntryPS2::RaceEntryPS2; operator delete(p, 0x3C) if the constructor
//   throws).
RaceEntryBase *RaceSinglePlayerInformation::createEntries()
{
    return new RaceEntryPS2; /* implicit: RaceEntryPS2::operator delete (if the constructor throws) */
}

// FUNC 0x001819B0 RaceSinglePlayerInformation::setupEntry
//   (slot 10) Only the player's car (entry number 0): the entry's equipments
//   (CarEquipments::operator=), inputId 0x100 and, when the equipments hold
//   a preset car (setCar marks it), the entry's tyre and transmission.
//   The entry and the car's information through pointers (retail keeps
//   both in s-registers, as RaceSplitBattleInformation's).
void RaceSinglePlayerInformation::setupEntry(RaceEntryCar *car)
{
    if (car->number == 0) {
        RaceEntry *e = &entry;
        RaceEntryInformation *info = &car->info;
        static_cast<CarEquipments &>(*info) = e->equip; /* implicit: CarEquipments::operator= */
        info->inputId = INPUT_ID_PLAYER;
        if (info->presetCar != 0) {
            car->tyre = e->tyre;
            car->transmission = e->transmission;
        }
    }
}

// FUNC 0x00181A20 RaceSinglePlayerInformation::setupOrganization
//   (slot 11) The opponents fill slots 1..count (virtual assignCarRange),
//   then the grid order is reset and the player (slot 0) moved to the last
//   place.  org->entries is read once (its count twice).
void RaceSinglePlayerInformation::setupOrganization(RaceOrganization *org)
{
    RaceEntryBase *entries = org->entries;
    assignCarRange(org, 1, entries->count);
    org->resetEntryOrder();
    org->moveEntryOrder(0, entries->count - 1);
}

// FUNC 0x00181A88 RaceSinglePlayerInformation::setCar
//   The player's car code; the equipments load it (CarEquipments::setCar)
//   and are marked a preset car whatever setCar returns (as set_entry_car,
//   T000).  Unreferenced (inline at the callers).
void RaceSinglePlayerInformation::setCar(u64 code)
{
    entry.car = code;
    CarEquipments &eq = entry.equip;        /* retail keeps &entry.equip in s0 */
    eq.setCar(code);
    eq.presetCar = 1;
}

// FUNC 0x00181AC0 RaceSinglePlayerInformation::setColor
//   The colour, also given to the equipments as their colour index
//   (CarEquipments::setColorIndex; T000 calls entry.color a colour id).
//   Unreferenced.
void RaceSinglePlayerInformation::setColor(int color)
{
    entry.color = color;
    entry.equip.setColorIndex(color);
}

// FUNC 0x00181AE8 RaceSinglePlayerInformation::setEquipments
//   entry.equip = eq (CarEquipments::operator=); eq is passed by value: egcs
//   destroys it here (CarEquipments::~CarEquipments flags 2, also on the
//   exception path).  Caller: run_arcade_race (T052).
void RaceSinglePlayerInformation::setEquipments(CarEquipments eq)
{
    entry.equip = eq; /* implicit: CarEquipments::operator= */
}

/* ======================================================================== */
/* RaceSinglePlayer: allocation, registry entries, construction             */
/* ======================================================================== */

// FUNC 0x00181B40 RaceSinglePlayer::operator new
//   The race pool, 16-byte aligned (pdistd::Allocator::allocate).
void *RaceSinglePlayer::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00181B68 RaceSinglePlayer::operator delete
//   The size (0x16EC0) is ignored (pdistd::Allocator::free).
void RaceSinglePlayer::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00181B90 RaceSinglePlayer::virtual_24
//   The class name.
const char *RaceSinglePlayer::virtual_24()
{
    return "RaceSinglePlayer";
}

// FUNC 0x00181BA0 RaceSinglePlayer_create
//   RaceClassInfo::create (g_002B65D8 +8): new RaceSinglePlayer (0x16EC0;
//   RaceSinglePlayer::operator new, the constructor, operator delete if it
//   throws).
RaceBase *RaceSinglePlayer_create(void)
{
    return new RaceSinglePlayer; /* implicit: RaceSinglePlayer::operator delete (if the constructor throws) */
}

// FUNC 0x00181C00 RaceSinglePlayer_construct
//   RaceClassInfo::construct (g_002B65D8 +0xC): a RaceSinglePlayer in buf,
//   nothing when buf is NULL (egcs's null-checked placement new).  arg is
//   not used.
RaceBase *RaceSinglePlayer_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)                        /* egcs tests the result of <new>'s throw() placement
                                               new itself; GCC 14 does not */
        return NULL;
#endif
    return ::new (buf) RaceSinglePlayer;
}

/* The registry record of "RaceSinglePlayer" (.data, before the frame info;
 * its "RaceSinglePlayer" is virtual_24's literal, emitted once). */
RaceClassInfo g_002B65D8 = {                /* 0x002B65D8 */
    "RaceSinglePlayer", sizeof(RaceSinglePlayer), RaceSinglePlayer_create, RaceSinglePlayer_construct
};

// FUNC 0x00181C28 RaceSinglePlayer::RaceSinglePlayer
//   RaceBasicWithRaceDisplay::RaceBasicWithRaceDisplay, the vptr, the input
//   (RaceInput::RaceInput); the input goes to pad slot 0x14 (PadManager::
//   attach directly, not Control::attach) and its record covers buffer 0 in
//   mode 0 (Manager::attachBuffer).  The landing pads run
//   RaceInput::~RaceInput and RaceBasicWithRaceDisplay::
//   ~RaceBasicWithRaceDisplay.  Callers: the create / construct functions,
//   RaceArcade, RaceFreePractice and RaceChampionship (0x0018EF88).
RaceSinglePlayer::RaceSinglePlayer()
{
    /* implicit: RaceBasicWithRaceDisplay, RaceInput (the base's / members'
     * constructors), ~RaceBasicWithRaceDisplay, ~RaceInput (their cleanup on a
     * throw) */
    RaceInput *in = &input;                 /* retail: the constructor's argument register */
    g_002EA7F8.attach(in, PAD_SLOT_PLAYER);
    AutomobileControlRecord::Manager *rec = &in->rec;
    rec->setMode(0);
    rec->attachBuffer(0);
}

// FUNC 0x00181CF0 RaceSinglePlayer::~RaceSinglePlayer
//   The vptr, the input leaves the pad manager (PadManager::detach),
//   RaceInput::~RaceInput (flags 2), RaceBasicWithRaceDisplay::
//   ~RaceBasicWithRaceDisplay (flags 0); deleting form: operator
//   delete(this, 0x16EC0).
RaceSinglePlayer::~RaceSinglePlayer()
{
    g_002EA7F8.detach(&input);
    /* implicit: ~RaceBasicWithRaceDisplay, ~RaceInput (the base / member
     * destructors) */
}

// FUNC 0x00181D80 RaceSinglePlayer::virtual_25
//   The player's input.
RaceInput *RaceSinglePlayer::virtual_25()
{
    return &input;
}

// FUNC 0x00181D90 RaceSinglePlayer::virtual_26
//   The input of inputId `id': 0x100 is the player, anything else goes to
//   RaceBase::virtual_26 (its disabled input).
RaceInput *RaceSinglePlayer::virtual_26(int id)
{
    if (id == INPUT_ID_PLAYER)
        return &input;
    return RaceBase::virtual_26(id);
}

// FUNC 0x00181DC8 RaceSinglePlayer::virtual_13
//   Loads the input record from right after the race info block
//   (Manager::load, all its slots); the end pointer it returns is dropped.
void RaceSinglePlayer::virtual_13(RaceInfoBlock *blk)
{
    input.rec.load((const u32 *)((u8 *)blk + RACE_INFO_BLOCK_SIZE), 1);
}

// FUNC 0x00181DF8 RaceSinglePlayer::virtual_14
//   One recorded input; the record is appended to the block (Manager::save
//   writes at blk + blk->size and advances the size word).
void RaceSinglePlayer::virtual_14(RaceInfoBlock *blk)
{
    blk->recordCount = 1;
    input.rec.save(&blk->size, 1);
}

// FUNC 0x00181E28 RaceSinglePlayer::virtual_15
//   The size of the saved input record (Manager::saveSize).
int RaceSinglePlayer::virtual_15()
{
    return input.rec.saveSize(1);
}

// FUNC 0x00181E50 RaceSinglePlayer::virtual_8
//   Race start: RaceBasic's (0x00181078; index: RaceBasicWithRaceDisplay::
//   virtual_8), then the input follows the replay mode RaceBase stored
//   (RaceInput::setReplayMode(replayMode), not the argument).  Also called
//   by RaceArcade, RaceFreePractice and RaceChampionship (0x0018F1E0).
void RaceSinglePlayer::virtual_8(int replay)
{
    RaceBasicWithRaceDisplay::virtual_8(replay);
    input.setReplayMode(replayMode);
}

// FUNC 0x00181E90 RaceSinglePlayer::virtual_28
//   The update: RaceBasic's (0x001811B8), then between
//   race_display_poll_events and race_display_drop_events the race display
//   gets the car of the current view (slot 8), view 0's camera mount
//   (slot 9) and its update (slot 2); the display pointer is reloaded for
//   each call.  Then the record checks: in a race a full record posts event
//   0x16 (arg 0, delay 2); in a replay its end posts event 1 (the end).
void RaceSinglePlayer::virtual_28(float dt)
{
    RaceBasicWithRaceDisplay::virtual_28(dt);
    race_display_poll_events(org);
    if (raceDisplay != NULL) {
        raceDisplay->setTargetCarNumber(virtual_6()->F_00209F10());
        raceDisplay->virtual_9(virtual_6()->views[0].camera->getMount());
        raceDisplay->virtual_2(RACE_DISPLAY_STEP);  /* the enhanced engine: a frame (hud.md C19) */
    }
    race_display_drop_events(org);
    switch (replayMode) {
    case RACE_LIVE:
        if (input.rec.isFinished() != 0)
            org->eventQueue.post(RACE_EVENT_RECORD_FULL, 0, 2);
        break;
    case RACE_REPLAY:
        if (input.rec.isFinished() != 0)
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
        break;
    case RACE_DEMO:                         /* retail's case tree tests 1, then < 2, then 0 */
        break;
    }
}

/* RaceSinglePlayer's node in the registry (.sdata zeros, linked in at
 * start-up). */
RaceClassNode g_00351E20;                   /* 0x00351E20 */

// FUNC 0x00182028 race_single_player_static_ctor
//   .ctors entry (0x0028F0F4, egcs _GLOBAL_$I$): registers
//   "RaceSinglePlayer" (g_002B65D8) with its node g_00351E20 (gp-0x5BD0).
void race_single_player_static_ctor(void)
{
    race_class_register(&g_00351E20, &g_002B65D8);
}

// FUNC 0x00182050 RaceSinglePlayer::getInput
//   The same as virtual_25, non-virtual (unreferenced).
/* inline in game/race/modes/T133_race_single_player.h; egcs emits this copy (this unit
 * defines the class's first virtual function) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00182060 __tf16RaceSinglePlayer
//   Builds the __si_type_info __ti16RaceSinglePlayer (name 0x00328A18, base
//   __ti24RaceBasicWithRaceDisplay) on the first call.  Callers:
//   __tf10RaceArcade, __tf16RaceFreePractice, __tf16RaceChampionship.
extern "C" const type_info &__tf16RaceSinglePlayer(void)
{
    if (__ti16RaceSinglePlayer[0] == 0) {
        __tf24RaceBasicWithRaceDisplay();
        __rtti_si(__ti16RaceSinglePlayer, "16RaceSinglePlayer",
                  (const type_info *)__ti24RaceBasicWithRaceDisplay);
    }
    return *(const type_info *)__ti16RaceSinglePlayer;
}
#endif

// FUNC 0x001820B0 RaceSinglePlayerInformation::setVibration
//   entry.vibration (unreferenced).
/* inline in game/race/modes/T133_race_single_player.h; egcs emits this copy */

// FUNC 0x001820B8 RaceSinglePlayerInformation::setTyre
//   entry.tyre (-> the car's tyre, setupEntry; unreferenced).
/* inline in game/race/modes/T133_race_single_player.h; egcs emits this copy */

// FUNC 0x001820C0 RaceSinglePlayerInformation::setTransmission
//   entry.transmission (-> the car's transmission, setupEntry;
//   unreferenced).
/* inline in game/race/modes/T133_race_single_player.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x001820C8 __tf27RaceSinglePlayerInformation
//   Builds the __si_type_info __ti27RaceSinglePlayerInformation (base
//   __ti15RaceInformation, gp-0x4678) on the first call.  Caller:
//   __tf21RaceArcadeInformation.
extern "C" const type_info &__tf27RaceSinglePlayerInformation(void)
{
    if (__ti27RaceSinglePlayerInformation[0] == 0) {
        __tf15RaceInformation();
        __rtti_si(__ti27RaceSinglePlayerInformation, "27RaceSinglePlayerInformation",
                  (const type_info *)__ti15RaceInformation);
    }
    return *(const type_info *)__ti27RaceSinglePlayerInformation;
}
#endif
