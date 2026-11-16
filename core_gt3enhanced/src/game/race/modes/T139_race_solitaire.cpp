/* T139_race_solitaire.cpp - the one-player race base (0x00186968..0x00187F98):
 * RaceSolitaireEntry, RaceSolitaireInformation, RaceSolitaire.
 *
 * RaceSolitaire is the base of RaceTimeTrial (T140), RaceFreeRun (T142),
 * RaceLicense (T144) and RaceMachineTest (T148).  It has four inputs:
 * input0 is the pad (slot 0x14 of the pad manager) and records the player's
 * controls into record buffer 0; two more records, the lap being driven and
 * the best lap (one slot of record buffer 1 each), hang off it; input1 is
 * the second car (the ghost), played from record buffer 1; input2 / input3
 * play the two cars of the data logger replay (RaceOrganization::
 * loggerMode) out of the replay memory: three 0xA8000-byte blocks of the
 * arena g_002C2F50 (the lowest of them starts a 0x1F8000-byte RaceMemPool).
 * In logger mode the race is drawn as two side-by-side views
 * (RaceSplitDisplay).
 *
 * The RaceInfoBlock slots (11, 13, 14, 15, 17, 20..22, 58, 59) save and load
 * the race description and, after it (+0x191C), the control records.
 *
 * RaceSolitaireInformation describes a one-car race; RaceSolitaireEntry is
 * its entry list, which can be switched to show only the first car.
 *
 * The operator new / delete pairs are this unit's copies of the inline
 * race-pool operators (the pool g_002B3BF0).
 *
 * Includes (u047's data pass): the classes retail's T139 saw, in its order.
 * Its .rodata (0x0032AD30..0x0032B110) is "bad_alloc" (<new>), the two
 * literals, the three vtables, then the type names in reverse definition
 * order: the own three, the race display's (T154: RaceMiniMap ..
 * RaceMeterBase, RaceCameraBase, RaceValueDisplayBase, RaceDisplayBase) and
 * UnitArena<64> last (T153 read first, with its type-name switch: T138 /
 * T132 have it inside the display group, where T154 reads T153), no menu
 * class, no garage list.  Its .sdata (0x00351E88..0x00351E98) is the "" of
 * T157's inline RaceMessageDisplay::clear (read with the race display), then
 * the registry node g_00351E90.
 */
// DATA_START 0x0032AD30
// DATA_START 0x00351E88
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* placement new (race_solitaire_construct); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf functions) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "libc.h"                           /* memset */
#include "libpad.h"                         /* SCE_PADselect (Control::curButtons.raw[0], T477) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, first defined */
#define GAME_T180_INLINE_BLOCK_ACCESSORS    /* RaceCarModel::allocBlock / freeBlock inline (T180's header,
                                               through T154's; retail expands them) */
#include "game/race/core/T153_race_course.h"          /* UnitArena<64> (T471 _UnitArenaBase::alloc / release) */
#include "game/race/display/T154_race_display.h"         /* (the race display classes, before RaceBasic's headers) */
#include "game/race/modes/T139_race_solitaire.h"       /* own: RaceSolitaire ..; T132 RaceBasic, T130 RaceEntryPS2,
                                               T124 RaceInformation, T000 RaceEntry, T155
                                               RaceSplitDisplay, T318 AutomobileControlRecord, T174
                                               RaceMemPool */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar, RaceEntryBase, RaceEntryInformation;
                                               T128 RACE_TIME_INVALID */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool (T530 pdistd::Allocator) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, loggerMode, eventQueue, ...) */
#include "game/car/model/T180_race_car_model.h"       /* RaceCarModel::allocBlock / freeBlock (the replay memory
                                               blocks of the arena g_002C2F50) */
#include "game/debug/T290_logger_sectors.h"       /* dc_sector_restart */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor (isDataLoggerMultiLap, virtual_29) */
#include "game/race/dynamics/T313_dc_free_run.h"          /* OneLapGhostInfo (the head of a lap slot) */
#include "game/camera/T377_camera_target_info.h"   /* CameraTargetInfo */
#include "game/camera/T397_variable_camera_base.h" /* VariableCameraBase::getMount */
#include "engine/input/T475_pad_manager.h"          /* PadManager g_002EA7F8 */
#include "game/data/T487_race_mode_database.h"   /* RaceModeDatabase g_002F00C0 */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/race/modes/race_battles_internal.h"                           /* the unit's constants and data */
#include "game/enhanced.h"                  /* ENH_RACE_GHOST_CAR */

/* The control records saved after a RaceInfoBlock. */
static inline const u32 *race_info_records(const RaceInfoBlock *blk)
{
    return (const u32 *)((const u8 *)blk + RACE_INFO_BLOCK_SIZE);
}

/* ======================================================================== */
/* RaceSolitaireEntry                                                       */
/* ======================================================================== */

// FUNC 0x00186968 RaceSolitaireEntry::operator new
//   From the race pool, 16-byte aligned.
void *RaceSolitaireEntry::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00186990 RaceSolitaireEntry::operator delete
//   The size (0x60) is ignored.
void RaceSolitaireEntry::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x001869B8 RaceSolitaireEntry::RaceSolitaireEntry
//   RaceEntryPS2::RaceEntryPS2, the vptr (+0x1C), the two counts cleared.
//   Caller: RaceSolitaireInformation::createEntries.
RaceSolitaireEntry::RaceSolitaireEntry()
{
    savedCount = 0;
    soloCount = 0;

    /* implicit: RaceEntryPS2 (the base's / members' constructors) */
}

// FUNC 0x001869F8 RaceSolitaireEntry::~RaceSolitaireEntry
//   Slot 0.  The vptr, RaceEntryPS2::~RaceEntryPS2 (flags 0), operator
//   delete(this, 0x60) when flags & 1.
RaceSolitaireEntry::~RaceSolitaireEntry()
{
    /* implicit: ~RaceEntryPS2 (the base / member destructors) */
}

// FUNC 0x00186A50 RaceSolitaireEntry::virtual_4
//   Slot 4 (RaceOrganization::setLoggerMode passes its mode).  on: the list
//   shows only the first car (cars -> soloSlots; the count shown is 1 when
//   there is a car, worked out the first time; the real count is kept);
//   off: the full list again (the count shown is kept for the next time).
void RaceSolitaireEntry::virtual_4(int on)
{
    if (on) {
        savedCount = count;
        if (soloCount == 0)
            soloCount = count != 0;
        soloSlots[0] = slots[0];
        count = soloCount;
        cars = soloSlots;
        return;
    }
    soloCount = count;
    count = savedCount;
    cars = slots;
}

/* ======================================================================== */
/* RaceSolitaireInformation                                                 */
/* ======================================================================== */

// FUNC 0x00186AA8 RaceSolitaireInformation::RaceSolitaireInformation
//   RaceInformation::RaceInformation, the vptr (+0x54), the entry's
//   CarEquipments::CarEquipments, then the entry cleared (no car, colour
//   -1; the landing pad: RaceInformation::~RaceInformation).  Callers:
//   run_time_trial, RaceFreeRunInformation's constructor.
RaceSolitaireInformation::RaceSolitaireInformation()
{
    /* implicit: RaceInformation, CarEquipments (the base's / members'
     * constructors), ~RaceInformation (their cleanup on a throw) */
    RaceEntry *e = &entry;                  /* the field order, as RaceBattleInformationT's */
    e->car = 0;
    e->color = -1;
    e->transmission = 0;
    e->tyre = 0;
    e->vibration = 0;
}

// FUNC 0x00186B38 RaceSolitaireInformation::~RaceSolitaireInformation
//   Slot 0.  The vptr, the entry's CarEquipments::~CarEquipments (flags 2),
//   RaceInformation::~RaceInformation with the caller's flags.  Callers:
//   run_time_trial, RaceFreeRunInformation's destructor.
RaceSolitaireInformation::~RaceSolitaireInformation()
{
    /* implicit: ~RaceInformation, ~CarEquipments (the base / member
     * destructors) */
}

// FUNC 0x00186B88 RaceSolitaireInformation::createEntries
//   Slot 4 (also RaceFreeRunInformation's): RaceSolitaireEntry::operator
//   new(0x60), RaceSolitaireEntry::RaceSolitaireEntry (operator delete(p,
//   0x60) if it throws).
RaceEntryBase *RaceSolitaireInformation::createEntries()
{
    return new RaceSolitaireEntry; /* implicit: RaceSolitaireEntry::operator delete (if the constructor throws) */
}

// FUNC 0x00186BE0 RaceSolitaireInformation::getEntryCount
//   Slot 8: one car.
int RaceSolitaireInformation::getEntryCount()
{
    return 1;
}

// FUNC 0x00186BE8 RaceSolitaireInformation::setupEntry
//   Slot 10 (also RaceFreeRunInformation's): the car gets the entry's
//   equipments (CarEquipments::operator=), input id 0x100 for car 0 (1, the
//   ghost, otherwise) and, when the equipments hold a car (presetCar, set by
//   setCar), the entry's tyre and transmission; its start order is 0.
void RaceSolitaireInformation::setupEntry(RaceEntryCar *car)
{
    RaceEntry *e = &entry;
    RaceEntryInformation *info = &car->info;
    int n = car->number;
    info->CarEquipments::operator=(e->equip);
    info->inputId = n != 0 ? INPUT_ID_SECOND : INPUT_ID_PLAYER;
    if (info->presetCar != 0) {
        car->tyre = e->tyre;
        car->transmission = e->transmission;
    }
    car->order = 0;
}

// FUNC 0x00186C68 RaceSolitaireInformation::setupOrganization
//   Slot 11 (also RaceFreeRunInformation's): empty (RaceInformation's
//   resets the start order).
void RaceSolitaireInformation::setupOrganization(RaceOrganization *org)
{
}

// FUNC 0x00186C70 RaceSolitaireInformation::setCar
//   ? name (unreferenced).  Retail copies code into a fresh register, as the
//   class-inline CarEquipments::setPresetCar(true) gives (its copy
//   0x00237790 has no callers; T482 merge: inline in
//   game/data/T482_car_equipments.h).
void RaceSolitaireInformation::setCar(s64 code)
{
    entry.car = code;
    entry.equip.setCar(code);
    entry.equip.setPresetCar(true);
}

// FUNC 0x00186CA8 RaceSolitaireInformation::setColor
//   ? name (unreferenced).  The entry's colour is an index of the car's
//   colours (setColorIndex).
void RaceSolitaireInformation::setColor(int color)
{
    entry.color = color;
    entry.equip.setColorIndex(color);
}

// FUNC 0x00186CD0 RaceSolitaireInformation::setEquipments
//   ? name.  CarEquipments::operator=.  The argument is passed by value:
//   egcs passes the address of the caller's copy and the callee destroys it
//   (CarEquipments::~CarEquipments, flags 2, also on the exception path).
//   Callers: run_free_run, run_time_trial.
void RaceSolitaireInformation::setEquipments(CarEquipments e)
{
    entry.equip = e; /* implicit: CarEquipments::operator= */
}

/* ======================================================================== */
/* RaceSolitaire: allocation, factory, construction, destruction            */
/* ======================================================================== */

// FUNC 0x00186D28 RaceSolitaire::operator new
//   From the race pool, 16-byte aligned.
void *RaceSolitaire::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x00186D50 RaceSolitaire::operator delete
//   The size (0x21800) is ignored.
void RaceSolitaire::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x00186D78 RaceSolitaire::virtual_24
//   Slot 24: the class name (0x0032AD40).
const char *RaceSolitaire::virtual_24()
{
    return "RaceSolitaire";
}

// FUNC 0x00186D88 race_solitaire_create
//   RaceClassInfo::create of "RaceSolitaire" (g_002B8180 +8): operator
//   new(0x21800), the constructor (operator delete(p, 0x21800) if it
//   throws).
RaceBase *race_solitaire_create(void)
{
    return new RaceSolitaire; /* implicit: RaceSolitaire::operator delete (if the constructor throws) */
}

// FUNC 0x00186DE8 race_solitaire_construct
//   RaceClassInfo::construct (g_002B8180 +0xC): the constructor in buf (none
//   for NULL); arg is not used.  The global placement new of <new> (as
//   u045's race_basic_construct): egcs tests its result for NULL itself
//   (retail's daddu v0,a0 / beqz v0); GCC 14 does not, hence the test.
RaceBase *race_solitaire_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)
        return NULL;
#endif
    return ::new (buf) RaceSolitaire;
}

/* The registry record of "RaceSolitaire" (.data, before the frame info; its
 * "RaceSolitaire" is virtual_24's literal, emitted once) and its node (.sdata
 * zeros after the "", linked in at start-up; written at its definition, as
 * T138's). */
RaceClassInfo g_002B8180 = {                /* 0x002B8180 */
    "RaceSolitaire", sizeof(RaceSolitaire), race_solitaire_create, race_solitaire_construct
};
RaceClassNode g_00351E90 = { NULL, NULL };  /* 0x00351E90 */

// FUNC 0x00186E10 RaceSolitaire::RaceSolitaire
//   RaceBasic::RaceBasic, the vptr, then the members: the four inputs
//   (RaceInput::RaceInput), splitDisplay (RaceSplitDisplay::
//   RaceSplitDisplay), bestLapRecord and lapRecord
//   (AutomobileControlRecord::Manager::Manager), pool (RaceMemPool::
//   RaceMemPool).  The landing pads destroy what is built (flags 0):
//   RaceMemPool::~RaceMemPool, Manager::~Manager, RaceSplitDisplay::
//   ~RaceSplitDisplay, RaceInput::~RaceInput, RaceBasic::~RaceBasic.
//   The body: input0 joins the pad manager and records into buffer 0; the
//   lap records are slots 1 (the lap) and 2 (the best lap, its head cleared
//   to "no time") of buffer 1; input1 plays buffer 1; inputs 1..3 are held.
//   Last the replay memory is taken (virtual_57, called directly here).
//   Callers: race_solitaire_create / construct and the constructors of
//   RaceTimeTrial, RaceFreeRun, RaceLicense, RaceMachineTest.
//   Matching: the inline RaceInput / Manager accessors (T413 / T318) give
//   retail's &inputN registers; inputs 1..3 held in input order, each with
//   its record mode first (Lupo 2's constructor has this order too).
RaceSolitaire::RaceSolitaire()
{
    /* implicit: RaceBasic, RaceSplitDisplay, RaceMemPool, Manager, RaceInput
     * (the base's / members' constructors), ~RaceBasic, ~RaceSplitDisplay,
     * ~RaceMemPool, ~Manager, ~RaceInput (their cleanup on a throw) */
    g_002EA7F8.attach(&input0, PAD_SLOT_STEERING1);
    input0.getRecord()->setMode(RECORD_MODE_BUFFER);
    input0.getRecord()->attachBuffer(0);
    lapRecord.setMode(RECORD_MODE_SLOT);
    lapRecord.attachBuffer(1);
    lapRecord.selectSlot(1);
    input0.setLap(&lapRecord);
    bestLapRecord.setMode(RECORD_MODE_SLOT);
    bestLapRecord.attachBuffer(1);
    bestLapRecord.selectSlot(2);
    OneLapGhostInfo *best = (OneLapGhostInfo *)bestLapRecord.getSlot();
    memset((void *)best, 0, sizeof(OneLapGhostInfo));
    input0.setBestLap(&bestLapRecord);
    best->time = RACE_TIME_INVALID;
    input1.getRecord()->setMode(RECORD_MODE_BUFFER);
    input1.getRecord()->attachBuffer(1);
    input1.setHold(1);
    input2.setRecordMode(RECORD_MODE_BUFFER);
    input2.setHold(1);
    input3.setRecordMode(RECORD_MODE_BUFFER);
    input3.setHold(1);
    showGhost = 1;
    field_217A8 = 0;
    replayMem = NULL;
    replayMemSize = 0;
    firstTransmission = 0;
    ghostKept = 0;                          /* the enhanced engine: no ghost car yet (M1) */
    virtual_57();
}

// FUNC 0x00187120 RaceSolitaire::~RaceSolitaire
//   Slot 0.  The vptr; input0 leaves the pad manager; the three replay
//   blocks go back to the arena; then the members, last first (flags 2):
//   RaceMemPool::~RaceMemPool, Manager::~Manager (lapRecord, bestLapRecord),
//   RaceSplitDisplay::~RaceSplitDisplay, RaceInput::~RaceInput (input3 ..
//   input0); RaceBasic::~RaceBasic (flags 0); operator delete(this,
//   0x21800) when flags & 1.  Callers: the destructors of the four derived
//   modes.
RaceSolitaire::~RaceSolitaire()
{
    g_002EA7F8.detach(&input0);
    if (replayMem != NULL) {
        int step = REPLAY_BLOCK_SIZE;       /* (a variable: retail keeps 0xA8000 in $s0 for the
                                               second block, the third is a constant) */
        RaceCarModel::freeBlock(replayMem); /* implicit: release (the inline's call) */
        RaceCarModel::freeBlock(replayMem + step);
        RaceCarModel::freeBlock(replayMem + step * 2);
    }
    /* implicit: ~RaceBasic, ~RaceSplitDisplay, ~RaceMemPool, ~Manager,
     * ~RaceInput (the base / member destructors) */
}

// FUNC 0x00187288 race_input_set_record_buffer
//   ? name.  Caller: virtual_57.
void race_input_set_record_buffer(RaceInput *input, u8 *buf)
{
    input->rec.setBuffer(buf);
}

// FUNC 0x001872A8 RaceSolitaire::virtual_4
//   Slot 4 (also the derived modes'): the replay memory pool.
RaceMemPool *RaceSolitaire::virtual_4()
{
    return &pool;
}

// FUNC 0x001872B8 RaceSolitaire::virtual_57
//   Slot 57: the replay memory, once: three blocks of the arena g_002C2F50
//   (the lowest address starts the 0x1F8000 bytes: the blocks are taken to
//   be adjacent, nothing checks it), the RaceMemPool over them, and the
//   logger inputs' record buffers (the pool's small buffers).  Callers: the
//   constructor (directly), virtual_8, virtual_23.
//   The three blocks are sorted by address (swaps; only the lowest is
//   used): the swaps' dead stores keep the compares as branches until
//   flow deletes them (no if-conversion: retail's bnel), the third swap
//   goes after reload, and with it the only use of the copy of the first
//   block allocBlock's inline expansion left in $s5 (saved, never used).
void RaceSolitaire::virtual_57()
{
    if (replayMem != NULL)
        return;
    u8 *first = (u8 *)RaceCarModel::allocBlock();   /* implicit: alloc (the inline's call) */
    u8 *second = (u8 *)RaceCarModel::allocBlock();
    u8 *third = (u8 *)RaceCarModel::allocBlock();
    u8 *t;
    if (second < first) { t = first; first = second; second = t; }
    if (third < first) { t = first; first = third; third = t; }
    if (third < second) { t = second; second = third; third = t; }
    replayMem = first;
    replayMemSize = REPLAY_MEM_SIZE;
    RaceMemPool *p = &pool;                 /* (retail keeps &pool in $s0 for the two small buffers) */
    p->F_001A61F0(first, REPLAY_MEM_SIZE);
    race_input_set_record_buffer(&input2, (u8 *)p->bufs[0].small);
    race_input_set_record_buffer(&input3, (u8 *)p->bufs[1].small);
}

// FUNC 0x001873B8 RaceSolitaire::seekRecords
//   ? name.  Clears the organization's startFlag and puts car 0 (and car 1
//   while the second input plays) back to logger sector `sector'
//   (dc_sector_restart).  Caller: gt2k_menu::LoggerControl::runPage.
void RaceSolitaire::seekRecords(int sector)
{
    DynamicsConductor *conductor = dc;
    org->startFlag = 0;
    dc_sector_restart(conductor, 0, sector);
    if (getSecondInput()->hold == 0)
        dc_sector_restart(conductor, 1, sector);
}

// FUNC 0x00187430 RaceSolitaire::virtual_23
//   Slot 23: the replay memory (virtual_57), input2 plays a copy of input0's
//   record (Manager::copyFrom) and is no longer held, and firstTransmission takes
//   the first car's transmission.
void RaceSolitaire::virtual_23()
{
    virtual_57();
    input2.getRecord()->copyFrom(input0.getRecord());
    input2.setHold(0);
    if (org != NULL) {
        firstTransmission = org->entries->cars[0]->transmission;
#if ENH_RACE_GHOST_CAR
        keepGhostCar();                     /* Lupo 2 (L2 0x00184630) */
#endif
    }
}

/* New in the enhanced engine (race_logic.md M1; Lupo 2 L2 0x00184630..
 * 0x00184750, also in RaceFreeRun::virtual_23): the first car of the lap
 * that has just become the ghost - the CarSaveEquipments part of its
 * equipments (0x1D0 bytes), its condition and its transmission, steering
 * assist, ASM and TCS - for virtual_21's ghost slot.  So the saved ghost
 * replays with the car that set the lap, even when the car or its settings
 * changed later.  Caller: virtual_23 (both). */
void RaceSolitaire::keepGhostCar()
{
    RaceEntryCar *car = org->entries->cars[0];

    ghostInfo = static_cast<const CarSaveEquipments &>(car->info);
    ghostCondition = car->condition;
    ghostTransmission = car->transmission;
    ghostSteeringAssist = car->steeringAssist;
    ghostAsm = car->assistAsm;
    ghostTcs = car->assistTcs;
    ghostKept = 1;
}

// FUNC 0x001874B0 RaceSolitaire::getMainInput
//   ? name.  input2 in the logger (or with field_217A8 set), else input0.
//   Callers: virtual_26, virtual_9, virtual_28, RaceFreeRun::onExit,
//   RaceMachineTest::onExit.
RaceInput *RaceSolitaire::getMainInput()
{
    if (field_217A8 != 0 || org->loggerMode != 0)
        return &input2;
    return &input0;
}

// FUNC 0x001874E8 RaceSolitaire::getSecondInput
//   ? name.  input3 in the logger (or with field_217A8 set), else input1.
//   8 callers (also RaceFreeRun's).
RaceInput *RaceSolitaire::getSecondInput()
{
    if (field_217A8 != 0 || org->loggerMode != 0)
        return &input3;
    return &input1;
}

// FUNC 0x00187520 RaceSolitaire::virtual_25
//   Slot 25: the player's input, input2 in the logger (field_217A8 is not
//   looked at here).
RaceInput *RaceSolitaire::virtual_25()
{
    if (org->loggerMode != 0)
        return &input2;
    return &input0;
}

// FUNC 0x00187548 RaceSolitaire::virtual_26
//   Slot 26: input id 1 (the ghost car) is the second input, 0x100 the main
//   one, anything else as in RaceBase.
RaceInput *RaceSolitaire::virtual_26(int id)
{
    switch (id) {
    case INPUT_ID_PLAYER:
        return getMainInput();
    case INPUT_ID_SECOND:
        return getSecondInput();
    }
    return RaceBase::virtual_26(id);
}

// FUNC 0x00187598 RaceSolitaire::virtual_8
//   Slot 8, the race start: the replay memory, RaceBasic::virtual_8; in a
//   race (not a replay) both lap records rewound; the split display set up
//   for the mode "RACE_MODE_LOGGER" (live); no time step.  Callers: the
//   virtual_8 of the four derived modes.
void RaceSolitaire::virtual_8(int replay)
{
    virtual_57();
    RaceBasic::virtual_8(replay);
    if (replayMode == 0) {
        lapRecord.reset();
        bestLapRecord.reset();
    }
    RaceOrganization *o = org;
    splitDisplay.virtual_0(o, g_002F00C0.getModeId("RACE_MODE_LOGGER"), 0);
    timeStep = 0;
}

// FUNC 0x00187660 RaceSolitaire::virtual_9
//   Slot 9.  input0 / input2 get the replay mode; the pool's lastSector is
//   the course's (RaceOrganization::getLoggerSectorCount) or, when the conductor's
//   data logger runs over several laps, its lap count (virtual_29); the main
//   and the second input take the pool's two sector arrays as their
//   snapshots (the second in replay mode 1); RaceBase::virtual_9 in
//   between.
void RaceSolitaire::virtual_9()
{
    input0.setReplayMode(replayMode);
    input2.setReplayMode(replayMode);
    RaceOrganization *o = org;
    RaceMemPool *mem = virtual_4();
    mem->lastSector = o->getLoggerSectorCount();
    getMainInput()->snapshots = mem->bufs[0].sectors;
    RaceInput *second = getSecondInput();
    second->setReplayMode(1);
    second->snapshots = mem->bufs[1].sectors;
    RaceBase::virtual_9();
    DynamicsConductor *conductor = dc;
    if (conductor->isDataLoggerMultiLap() != 0)
        mem->lastSector = conductor->virtual_29();
}

// FUNC 0x00187760 RaceSolitaire::virtual_56
//   Slot 56: the pool's sector records cleared (RaceMemPool::F_001A62B0).
//   Caller: RaceFreeRun::virtual_56.
void RaceSolitaire::virtual_56()
{
    virtual_4()->F_001A62B0();
}

// FUNC 0x00187790 RaceSolitaire::virtual_35
//   Slot 35 (the end of the race): RaceBasic::virtual_35, then in a race
//   both lap records' recorders are closed (Recorder::stop).  Callers:
//   run_free_run, run_time_trial, RaceMachineTest::virtual_35.
void RaceSolitaire::virtual_35()
{
    RaceBasic::virtual_35();
    if (replayMode == 0) {
        lapRecord.rec.stop();
        bestLapRecord.rec.stop();
    }
}

// FUNC 0x001877E8 RaceSolitaire::virtual_11
//   Slot 11: RaceBase::virtual_11, then the block holds one car.
void RaceSolitaire::virtual_11(RaceInfoBlock *blk)
{
    RaceBase::virtual_11(blk);
    blk->carCount = 1;
}

// FUNC 0x00187818 RaceSolitaire::virtual_58
//   Slot 58: input0 plays the record saved after the block (all its slots).
void RaceSolitaire::virtual_58(RaceInfoBlock *blk)
{
    input0.rec.load(race_info_records(blk), 1);
}

// FUNC 0x00187840 RaceSolitaire::virtual_13
//   Slot 13: input0 plays the record saved after the block, input1 is held,
//   then virtual_23.  (RaceInput::setHold, inline: retail stores through
//   &input1 + 0x140.)
void RaceSolitaire::virtual_13(RaceInfoBlock *blk)
{
    input0.rec.load(race_info_records(blk), 1);
    input1.setHold(1);
    virtual_23();
}

// FUNC 0x00187898 RaceSolitaire::virtual_14
//   Slot 14: marks the block (a record follows) and appends input0's record
//   at blk + blk->size (Manager::save).
void RaceSolitaire::virtual_14(RaceInfoBlock *blk)
{
    blk->recordCount = 1;
    input0.rec.save((int *)blk, 1);
}

// FUNC 0x001878C8 RaceSolitaire::virtual_15
//   Slot 15: the size of input0's record (Manager::saveSize).
int RaceSolitaire::virtual_15()
{
    return input0.rec.saveSize(1);
}

// FUNC 0x001878F0 RaceSolitaire::virtual_17
//   Slot 17: RaceBase::virtual_17, then the block holds one car.
void RaceSolitaire::virtual_17(RaceInfoBlock *blk)
{
    RaceBase::virtual_17(blk);
    blk->carCount = 1;
}

// FUNC 0x00187920 RaceSolitaire::virtual_20
//   Slot 20: the second input plays the record saved after the block and is
//   no longer held.
void RaceSolitaire::virtual_20(RaceInfoBlock *blk)
{
    RaceInput *in = getSecondInput();
    in->rec.load(race_info_records(blk), 1);
    in->hold = 0;
}

// FUNC 0x00187968 RaceSolitaire::virtual_21
//   Slot 21: marks the block, stores firstTransmission (the first car's
//   transmission, as a byte) in its first car slot and appends input2's
//   record.
//   The enhanced engine (race_logic.md M1, ENH_RACE_GHOST_CAR; Lupo 2 L2
//   0x00184FF8): once a ghost's car was kept, slot 0 gets it whole -
//   equipments, condition, transmission, steering assist, ASM, TCS - in the
//   fields virtual_7 fills (RaceInfoSlot): the block stays 0x191C bytes.
void RaceSolitaire::virtual_21(RaceInfoBlock *blk)
{
    blk->recordCount = 1;
#if ENH_RACE_GHOST_CAR
    if (ghostKept != 0) {
        /* Lupo 2 (L2 0x0018500C): the ghost's own car in slot 0 (the tyre
         * byte stays virtual_7's) */
        RaceInfoSlot *slot = &blk->slots[0];

        slot->info = ghostInfo;
        slot->condition = ghostCondition;
        slot->transmission = (s8)ghostTransmission;
        slot->steeringAssist = (s8)ghostSteeringAssist;
        slot->assistAsm = (s8)ghostAsm;
        slot->assistTcs = (s8)ghostTcs;
    } else
#endif
    blk->slots[0].transmission = (s8)firstTransmission;
    input2.rec.save((int *)blk, 1);
}

// FUNC 0x001879A8 RaceSolitaire::virtual_22
//   Slot 22: the size of input2's record.
int RaceSolitaire::virtual_22()
{
    return input2.rec.saveSize(1);
}

// FUNC 0x001879D0 RaceSolitaire::virtual_59
//   Slot 59 (main's replay of a saved RaceSolitaire): which 0 ->
//   virtual_13(blk), else virtual_19(blk) then virtual_20(blk).
void RaceSolitaire::virtual_59(int which, RaceInfoBlock *blk)
{
    if (which == 0) {
        virtual_13(blk);
        return;
    }
    virtual_19(blk);
    virtual_20(blk);
}

// FUNC 0x00187A50 RaceSolitaire::virtual_28
//   Slot 28, the frame.  timeStep (when non-zero) replaces dt for
//   RaceBasic::virtual_28.  SELECT newly pressed on the player's input flips
//   showGhost (Control::rawButtonDown(0), inline).
//   The display events (race_display_poll_events); the race display (when
//   there is one) gets the camera view's target car, the view camera's
//   mount and its update; the display events dropped; the split display's
//   two views show cars 0 and 1 (-1 for a car the race does not have) and
//   are updated.  Then the ends of the records: in a race the main record
//   full posts RACE_EVENT_RECORD_FULL (delay 2); in a replay (mode 1) the
//   main record played to its end (in the logger: both, the second only
//   while it plays) ends the race.
void RaceSolitaire::virtual_28(float dt)
{
    if (timeStep != 0.0f)
        dt = timeStep;
    RaceBasic::virtual_28(dt);

    RaceInput *in = virtual_25();
    if (in->rawButtonDown(0) & SCE_PADselect)
        showGhost = 1 - showGhost;

    race_display_poll_events(org);
    if (raceDisplay != NULL) {
        raceDisplay->setTargetCarNumber(virtual_6()->F_00209F10());   /* the current view's target */
        raceDisplay->virtual_9(virtual_6()->views[0].camera->getMount());
        raceDisplay->virtual_2(RACE_DISPLAY_STEP);  /* the enhanced engine: a frame (hud.md C19) */
    }
    race_display_drop_events(org);

    for (int i = 0; i < 2; i++) {
        splitDisplay.setTargetDisplay(i);
        int car = i;
        if (i >= (int)org->entries->count)
            car = -1;
        splitDisplay.setPlayerCarNumber(car);
        splitDisplay.setTargetCarNumber(car);
        splitDisplay.virtual_9(1);
    }
    splitDisplay.virtual_2(RACE_DISPLAY_STEP);  /* the enhanced engine: a frame (hud.md C19) */

    switch (replayMode) {
    case RACE_LIVE:
        if (getMainInput()->rec.rec.finished != 0)
            org->eventQueue.post(RACE_EVENT_RECORD_FULL, 0, 2);
        break;
    case RACE_REPLAY:
        if (org->loggerMode != 0) {
            int firstDone = getMainInput()->rec.rec.finished;
            int secondDone;
            if (getSecondInput()->hold != 0)
                secondDone = 1;
            else
                secondDone = getSecondInput()->rec.rec.finished;
            if (firstDone != 0 && secondDone != 0)
                org->eventQueue.post(RACE_EVENT_END, 0, 0);
        } else if (getMainInput()->rec.rec.finished != 0) {
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
        }
        break;
    case RACE_DEMO:                         /* (the empty cases shape retail's decision tree:
                                               == 1, then >= 2, then != 0) */
    case RACE_REPLAY_ALT:
        break;
    }
}

// FUNC 0x00187D30 RaceSolitaire::virtual_45
//   Slot 45, the 2D overlays.  Outside the logger RaceBasic's; in the logger
//   the split display's two views are drawn (drawTarget) into a stack
//   GSBuffer (GSBuffer::GSBuffer, then copy(buf); ~GSBuffer, flags 2, also
//   on the exception path) whose rectangle is 640 x 448 at x 0, then 0x230.
void RaceSolitaire::virtual_45(GSBuffer *buf)
{
    if (org->loggerMode == 0) {
        RaceBasic::virtual_45(buf);
        return;
    }
    GSBuffer view;
    view.copy(buf);
    int x = 0;
    for (int i = 0; i < 2; i++) {
        view.setRect(x, 0, 640, 448);
        splitDisplay.setTargetDisplay(i);
        splitDisplay.drawTarget(view);
        x += 0x230;
    }
}

// FUNC 0x00187E28 race_solitaire_static_init
//   .ctors entry 0x0028F10C: registers "RaceSolitaire".
void race_solitaire_static_init(void)
{
    race_class_register(&g_00351E90, &g_002B8180);
}

// FUNC 0x00187E50 RaceSolitaire::getPadInput
//   ? name (unreferenced).
/* inline in game/race/modes/T139_race_solitaire.h; egcs emits this copy with the
 * vtable: return &input0; */

// FUNC 0x00187E60 RaceSolitaire::getShowGhost
//   Unreferenced.
/* inline in game/race/modes/T139_race_solitaire.h; egcs emits this copy with the
 * vtable: return showGhost; */

// FUNC 0x00187E70 RaceSolitaire::setShowGhost
//   Unreferenced.
/* inline in game/race/modes/T139_race_solitaire.h; egcs emits this copy with the
 * vtable: showGhost = v; */

// FUNC 0x00187E80 RaceSolitaire::setTimeStep
//   ? name (unreferenced).
/* inline in game/race/modes/T139_race_solitaire.h; egcs emits this copy with the
 * vtable: timeStep = t; */

#ifndef BOOT_OLD_GCC
// FUNC 0x00187E90 __tf13RaceSolitaire
//   The __si_type_info __ti13RaceSolitaire (name 0x0032B000, base
//   __ti9RaceBasic).  Callers: the __tf functions of the four derived modes.
extern "C" const type_info &__tf13RaceSolitaire(void)
{
    if (__ti13RaceSolitaire[0] == 0) {
        __tf9RaceBasic();
        __rtti_si(__ti13RaceSolitaire, "13RaceSolitaire", (const type_info *)__ti9RaceBasic);
    }
    return *(const type_info *)__ti13RaceSolitaire;
}
#endif

// FUNC 0x00187EE0 RaceSolitaireInformation::setVibration
//   Unreferenced: the entry's vibration.
/* inline in game/race/modes/T139_race_solitaire.h; egcs emits this copy with the
 * vtable: entry.vibration = v; */

// FUNC 0x00187EE8 RaceSolitaireInformation::setTyre
//   Unreferenced: the entry's tyre.
/* inline in game/race/modes/T139_race_solitaire.h; egcs emits this copy with the
 * vtable: entry.tyre = v; */

// FUNC 0x00187EF0 RaceSolitaireInformation::setTransmission
//   Unreferenced: the entry's transmission.
/* inline in game/race/modes/T139_race_solitaire.h; egcs emits this copy with the
 * vtable: entry.transmission = v; */

#ifndef BOOT_OLD_GCC
// FUNC 0x00187EF8 __tf24RaceSolitaireInformation
//   The __si_type_info __ti24RaceSolitaireInformation (name 0x0032B010, base
//   __ti15RaceInformation, gp-0x4678).  Caller: __tf22RaceFreeRunInformation.
extern "C" const type_info &__tf24RaceSolitaireInformation(void)
{
    if (__ti24RaceSolitaireInformation[0] == 0) {
        __tf15RaceInformation();
        __rtti_si(__ti24RaceSolitaireInformation, "24RaceSolitaireInformation",
                  (const type_info *)__ti15RaceInformation);
    }
    return *(const type_info *)__ti24RaceSolitaireInformation;
}

// FUNC 0x00187F48 __tf18RaceSolitaireEntry
//   The __si_type_info __ti18RaceSolitaireEntry (name 0x0032B030, base
//   __ti12RaceEntryPS2).
extern "C" const type_info &__tf18RaceSolitaireEntry(void)
{
    if (__ti18RaceSolitaireEntry[0] == 0) {
        __tf12RaceEntryPS2();
        __rtti_si(__ti18RaceSolitaireEntry, "18RaceSolitaireEntry",
                  (const type_info *)__ti12RaceEntryPS2);
    }
    return *(const type_info *)__ti18RaceSolitaireEntry;
}
#endif
