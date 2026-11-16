/* T290_logger_sectors.h - DynamicsConductor::virtual_7 and the data logger's
 * sector records of a car (translation unit T290, unit u078),
 * 0x001E0888..0x001E0BC0.
 *
 * T290 also defines DynamicsConductor::virtual_7(RaceCarState *car, int flag)
 * (0x001E0938: the car's DynCarState.section; flag is not read), which
 * T305_dynamics_conductor.h declares in the class.
 *
 * The sector records are the car's RaceInput's snapshots (RaceCarState +0xC
 * -> RaceInput, T413: +0x12C, 0x1B00 bytes each: OneLapGhostInfo, T313); the
 * logger position is its record's recorder (+0xCC, +0x30), the mode
 * rec.mode (+0xC8).  u078 called the RaceInput CarDynamics. */
#ifndef GAME_T290_LOGGER_SECTORS_H
#define GAME_T290_LOGGER_SECTORS_H

#include "game/game.h"

class DynamicsConductor;                    /* T305_dynamics_conductor.h (pointers only) */
class RaceCarState;                         /* T302_car_state.h */

int  dc_section_count(DynamicsConductor *dc);           /* 0x001E0888 the organization's getLoggerSectorCount() + 1 */
f32  dc_section_value(DynamicsConductor *dc, int idx);  /* 0x001E08A8 the organization's getLoggerSectorPosition(idx) */
/* 1 when the conductor supports the data logger (slot 4) and the car's
 * ctrlMode and driver are 0. */
int  car_logger_active(RaceCarState *car);              /* 0x001E08C8 */
/* Opens sector record `sector': DynamicsConductor::saveSector, then frac50
 * and t of the start (OneLapGhostInfo.frac50 / t; u078 kind / value), the
 * logger position (minus sector 0's start in record mode 2) and (clock +
 * 50) % 3.  The last two arguments are not read (countLap passes the
 * section's value and the course position).  t before frac50: countLap
 * (u083) loads $f12 before $a2 (the argument registers are set in parameter
 * order); the registers are the same either way (EABI). */
void car_sector_begin(RaceCarState *car, int sector, f32 t, int frac50,
                      f32 unused1, f32 unused2);        /* 0x001E0940 */
/* Posts event 0xD (the car's index); with `record' (countLap passes the
 * organization's loggerCreating), closes the sector: its time (ms) and the
 * logger positions it spans. */
void car_sector_end(RaceCarState *car, int sector, int time, int record); /* 0x001E0A18 */
void car_sector_restore(RaceCarState *car, int sector); /* 0x001E0AA0 restoreSector on the car's sector record */
/* car_sector_restore, then the logger stepped to the sector's start (0 for
 * sector 0 in mode 2) from the record's beginning (Recorder::skip(n, 0)). */
void dc_sector_restart(DynamicsConductor *dc, int car, int sector);     /* 0x001E0AD8 */
/* Unless the car's RaceInput is held (+0x140): ghost_restart (lap 1, t 1),
 * then sector 0 opened. */
void dc_logger_start(DynamicsConductor *dc, int car);   /* 0x001E0B50 */

#endif /* GAME_T290_LOGGER_SECTORS_H */
