/* T290 - DynamicsConductor::virtual_7 and the data logger's sector records:
 * 0x001E0888..0x001E0BC0.
 *
 * A car's sector records are its RaceInput's snapshots (RaceCarState +0xC ->
 * RaceInput +0x12C, OneLapGhostInfo, T313); the logger position is the frame
 * count of the input's control record (rec.rec.frames, RaceInput +0xFC).
 *
 * Data (retail): .rodata 0x0033C5A8..0x0033C5C9, nothing the code
 * references: <new>'s "bad_alloc", then UnitArena<64>'s type name (T153
 * with GAME_T153_EMITS_TYPEINFO); no "" (the race displays not seen). */
// DATA_START 0x0033C5A8
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (seen: UnitArena<64>) */
#include "game/car/physics/car_engine_internal.h"
#include "game/debug/T290_logger_sectors.h"

/* The organization's getLoggerSectorCount() + 1 (? the number of sections).  Callers:
 * DynamicsConductorLicense::virtual_7. */
// FUNC 0x001E0888 dc_section_count
int dc_section_count(DynamicsConductor *dc)
{
    return dc->org->getLoggerSectorCount() + 1;
}

/* The organization's getLoggerSectorPosition(idx).  Callers: DynamicsConductor::countLap
 * (with the car's section). */
// FUNC 0x001E08A8 dc_section_value
f32 dc_section_value(DynamicsConductor *dc, int idx)
{
    return dc->org->getLoggerSectorPosition(idx);
}

/* 1 when the conductor supports the data logger (slot 4), the car's driver
 * drives it (ctrlMode 0) and the driver is the pad.  Callers:
 * DynamicsConductor::countLap, DynamicsConductor::virtual_13. */
// FUNC 0x001E08C8 car_logger_active
int car_logger_active(RaceCarState *car)
{
    DynCarState *st = &car->st;
    int active = 0;
    if (car->dc->isDataLoggerSupported() != 0) {
        active = 0;
        if (st->ctrlMode == CTRL_DRIVER)
            active = st->driver == DRIVER_PAD;
    }
    return active;
}

/* Slot 7 (inherited by every conductor but the license one, which calls it
 * when its slot 6 returns 0): the car's section.  flag is not used. */
// FUNC 0x001E0938 DynamicsConductor::virtual_7
int DynamicsConductor::virtual_7(RaceCarState *car, int flag)
{
    return car->st.section;
}

/* Opens sector record `sector' of the car: saveSector (the car's snapshot),
 * then frac50, the logger position (in the one-slot record mode less sector
 * 0's start), t and (clock + 50) % 3.  The last two arguments are not read.
 * Callers: dc_logger_start, DynamicsConductor::countLap,
 * DynamicsConductor::virtual_13. */
// FUNC 0x001E0940 car_sector_begin
void car_sector_begin(RaceCarState *car, int sector, f32 t, int frac50, f32 unused1, f32 unused2)
{
    RaceInput *in = car->input;
    DynamicsConductor *dc = car->dc;
    OneLapGhostInfo *rec = in->getSnapshot(sector);
    int start = in->getFrames();

    if (in->getRecordMode() == REC_MODE_ONE_SLOT && sector != 0)
        start -= in->getSnapshot(0)->start;
    dc->saveSector(rec, car->st.carIndex);
    rec->frac50 = frac50;
    rec->start = start;
    rec->t = t;
    rec->clockRem = dc->clockRem(50);
}

/* Posts the logger sector event (arg: the car's index); with `record',
 * closes the sector: its time and the logger positions it spans.  Callers:
 * DynamicsConductor::countLap, DynamicsConductor::licenseCheckStop. */
// FUNC 0x001E0A18 car_sector_end
void car_sector_end(RaceCarState *car, int sector, int time, int record)
{
    car->org->eventQueue.post(RACE_EVENT_LOGGER_SECTOR, car->st.carIndex, 0);
    if (record != 0) {
        RaceInput *in = car->input;
        OneLapGhostInfo *rec = in->getSnapshot(sector);
        int frames = in->getFrames();
        rec->time = time;
        rec->length = frames - rec->start;
    }
}

/* restoreSector on the car's sector record: the car back as it was at the
 * sector's start. */
// FUNC 0x001E0AA0 car_sector_restore
void car_sector_restore(RaceCarState *car, int sector)
{
    car->dc->restoreSector(car->st.carIndex, car->input->getSnapshot(sector));
}

/* car_sector_restore, then the logger's record stepped to the sector's
 * start (0 for sector 0 in the one-slot mode) from its beginning
 * (Recorder::skip with keep 0).  Callers: RaceSolitaire::seekRecords. */
// FUNC 0x001E0AD8 dc_sector_restart
void dc_sector_restart(DynamicsConductor *dc, int car, int sector)
{
    RaceCarState *cs = dc->getCar(car);
    car_sector_restore(cs, sector);
    RaceInput *in = cs->input;
    int start = in->getSnapshot(sector)->start;
    int frames = 0;
    if (sector != 0)
        frames = start;
    if (in->getRecordMode() != REC_MODE_ONE_SLOT)
        frames = start;
    in->skip(frames, 0);
}

/* Unless the car's RaceInput is held: ghost_restart (lap 1, t 1), then
 * sector 0 opened (frac50 0, t 1).  Callers: RaceFreeRun::virtual_56. */
// FUNC 0x001E0B50 dc_logger_start
void dc_logger_start(DynamicsConductor *dc, int car)
{
    RaceCarState *cs = dc->getCar(car);
    RaceInput *in = cs->input;
    if (in->hold == 0) {
        ghost_restart(in, cs, 1, 1.0f);
        car_sector_begin(cs, 0, 1.0f, 0, 0.0f, cs->st.lapDistance);
    }
}
