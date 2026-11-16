/* race_organization_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u043,
 * 0x0017A038..0x0017CF30, second pass: the race core's organization, lap
 * times, records and base class.
 *
 * Translation units (link order), each with its canonical header:
 *   T125  RaceOrganization, RaceResult; out-of-line copies of RaceEntryBase's
 *         inline accessors (T125_race_organization.h)
 *   T126  RaceLapTime (T126_race_lap_time.h)
 *   T127  TimeRecord, SpeedRecord (T127_race_records.h)
 *   T128  LapRecord (T128_race_lap_record.h)
 *   T129  RaceBase, the registry of race classes, the race info block
 *         (T129_race_base.h)
 *
 * How the pieces fit: RaceOrganization is the race-wide state (options, the
 * entry list, the race database and its event record, the course, the start
 * countdown, the time limit, the event queue and the records); RaceBase (the
 * base of every race mode) runs a race on it: RaceBase::org -> organization,
 * RaceOrganization::race -> RaceBase.  The race code talks through the
 * organization's event queue (RaceEventQueue, T122; the types below).
 *
 * Every class is defined in its canonical header; each source includes the
 * headers it needs itself, in the order that gives retail's type names (the
 * data pass, ref/data/u043/notes.md).  This header has the constants only
 * this unit names.
 *
 * Notes for the GCC 14 (R5900) behaviour build: float literals are rounded
 * toward zero (999.9f is 0x4479F999 there, 0x4479F99A in the retail code), and
 * float arguments go in $f12 + argument position ($f13 for `void f(T *, float)`,
 * $f14 for the third argument) where the original compiler numbered them
 * consecutively from $f12.
 */
#ifndef U043_H
#define U043_H

#include "types.h"

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

/* RACE_EVENT_BGM codes: the negative ones are RaceBGMBase's stop / pause /
 * resume, any other is played (play(code, code == BGM_RACE)). */
enum {
    BGM_STOP   = -1,                        /* stop */
    BGM_PAUSE  = -2,                        /* pause (stopSounds: the cars' sounds stopped) */
    BGM_RESUME = -3,                        /* resume (startSounds: the cars' sounds started) */
    BGM_RACE   = 0x14                       /* (? the race music) at the start (start) and with no
                                               start countdown; its second argument is 1 */
    /* 0x1B / 0x12: the music of start actions 1 / 2 (4 / 10 seconds of
     * countdown), not named. */
};

/* RaceOrganization::startAction (the race record's startAction, or set by
 * RaceBase::virtual_8 from the conductor's starting format). */
enum {
    START_ACTION_NONE     = -1,             /* no countdown; BGM_RACE at once */
    START_ACTION_DEFAULT  = 0,              /* the record's countdown, if any */
    START_ACTION_COUNT4   = 1,              /* a 4-second countdown, BGM 0x1B */
    START_ACTION_COUNT10  = 2,              /* a 10-second countdown, BGM 0x12 */
    START_ACTION_NO_COUNTDOWN = 3           /* no countdown and no BGM (u084's name) */
};

/* RaceBase::virtual_26's input ids (RaceEntryInformation::inputId, T121). */
enum {
    INPUT_GHOST = 1                         /* the second car's input (u084's name) */
};

/* car_model_path's kinds (T446) RaceEntryCar::loadModel gets, and the
 * CourseEnv::flags bits that choose them (RaceBase::virtual_8). */
enum {
    CAR_MODEL_DAY     = 0,
    CAR_MODEL_NIGHT   = 1,
    CAR_MODEL_EVENING = 3
};
enum {
    COURSE_ENV_NIGHT   = 0x8000,            /* the lamp lights (T178): the night model */
    COURSE_ENV_EVENING = 0x80000            /* (? not in T178's list) the evening model */
};

/* COURSE_KIND_* (RaceDataBase::getCourseKind): game/data/T484_race_database.h. */

/* The race info block (RaceBase::virtual_7). */
enum {
    RACE_INFO_SIZE    = 0x191C,
    RACE_INFO_MAGIC   = 0x4B325447,         /* 'GT2K' */
    RACE_INFO_VERSION = 0x30000
};

/* One 0x8000-byte slot of an AutomobileControlRecord buffer (T318: a
 * OneLapGhostInfo and a record), copied by RaceOrganization::setupGhost as a
 * struct (an inline 32-byte copy loop, no memcpy). */
struct ControlRecordSlot {
    u8 bytes[0x8000];
};

#endif /* U043_H */
