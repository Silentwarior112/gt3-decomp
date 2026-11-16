/* race_entry_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u042,
 * 0x00177108..0x0017A038, second pass: the race entries, the event queue,
 * the race object pool / organization factory and the race information.
 *
 * Translation units (link order), each with its canonical header:
 *   T121  RaceEntryInformation, RaceEntryCar (one car of a race: car data,
 *         lap times, records, model, sound), RaceEntryBase (the car list);
 *         CarSound's compiler-generated operator= (include/game/race/core/T121_race_entry.h)
 *   T122  RaceEventQueue (T122_race_event_queue.h)
 *   T123  the race object pool g_002B3BF0 and race_organization_create
 *         (T123_race_setup.h)
 *   T124  RaceInformation, the description of a race to set up
 *         (T124_race_information.h)
 *
 * Every class is defined in a canonical header.  Each source includes the
 * canonical headers it needs itself, in the order that gives retail's type
 * names (data pass: each file's comment lists them); this header has only
 * the two data words only this unit reads and the constants of the car
 * sound frame RaceEntryCar::updateSound builds (T121 includes it last).
 */
#ifndef U042_H
#define U042_H

#include "types.h"

/* ======================================================================== */
/* Data only this unit reads                                                */
/* ======================================================================== */

/* {0, 0, 0, 1}: the listener's own position (.data, T121 defines it; the
 * on-board voices of RaceEntryCar::updateSound sound there). */
extern f32 g_002B2CC0[4];                   /* 0x002B2CC0 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it with __tf20RaceEntryInformation */
#include "runtime/cxx/T548_tinfo.h"                /* __class_type_info::base_info */
/* The base list of RaceEntryInformation's __class_type_info: one public
 * base, CarEquipments at offset 0 ({__ti13CarEquipments, 0x40000000}). */
extern const __class_type_info::base_info g_00351DD8[1];   /* 0x00351DD8 */
#endif

/* ======================================================================== */
/* The car sound frame (T121 CarSoundFrame) as RaceEntryCar::updateSound    */
/* fills it                                                                 */
/* ======================================================================== */

/* The listening modes: the rows of the gain table g_002D33A8 (T285, 5 rows).
 * The camera kind is CameraManager::virtual_14, from the current camera's
 * virtual_9: kind 0 (virtual_9 0 / 1) the onboard camera on mount 0 or a
 * mount without flag bit 1; kind 1 (virtual_9 2) onboard mount 1, the
 * mounts with flag bit 1, the logger camera; kind 2 the others (3: the
 * outside cameras).  "own" = CameraManager::isOwnCar: the current view
 * looks from this car. */
enum {
    LISTEN_INSIDE_OWN   = 0,                /* camera kind 0, own car: the voices sound at the listener */
    LISTEN_INSIDE_OTHER = 1,                /* camera kind 0, another car */
    LISTEN_MOUNT_OWN    = 2,                /* camera kind 1, own car */
    LISTEN_MOUNT_OTHER  = 3,                /* camera kind 1, another car */
    LISTEN_OUTSIDE      = 4                 /* camera kind 2 */
};

/* The columns of the gain table: one gain per sound of the frame.  The
 * wheel voices are T287's evaluators (their surface classes in brackets). */
enum {
    SND_GAIN_ENGINE0    = 0x0,              /* engine[0], DynCarState.engineSnd[0] / 64 */
    SND_GAIN_ENGINE1    = 0x1,              /* engine[1], engineSnd[1] / 64 */
    SND_GAIN_TURBO      = 0x2,              /* voice[SND_VOICE_TURBO], engineSnd[2] / 128 */
    SND_GAIN_BLOWOFF    = 0x3,              /* oneShot[0] x DynCarState.blowOffLevel (? the blow-off, T288) */
    SND_GAIN_GEAR_WHINE = 0x4,              /* voice[SND_VOICE_GEAR_WHINE], car_sound_gear_whine */
    SND_GAIN_CRASH      = 0x5,              /* oneShot[1 + kind], car_crash_sound_get */
    SND_GAIN_WHEEL3     = 0x6,              /* wheel voice 3, tyre_sound_squeal_1_13 [1, 13] */
    SND_GAIN_ROAD       = 0x7,              /* voice[SND_VOICE_ROAD], car_sound_road */
    SND_GAIN_WIND       = 0x8,              /* engine[2], car_sound_wind */
    SND_GAIN_WHEEL5     = 0x9,              /* wheel voice 5, tyre_sound_roll_0789 [0, 7, 8, 9] the tyre roll */
    SND_GAIN_WHEEL6     = 0xA,              /* wheel voice 6, tyre_sound_squeal_6_12_14 [6, 12, 14] */
    SND_GAIN_WHEEL1     = 0xB,              /* wheel voice 1, tyre_sound_squeal_0789 [0, 7, 8, 9] */
    SND_GAIN_WHEEL0     = 0xC,              /* wheel voice 0, tyre_sound_none (empty) */
    SND_GAIN_WHEEL4     = 0xD,              /* wheel voice 4, tyre_sound_squeal_2_4 [2, 3, 4] */
    SND_GAIN_WHEEL2     = 0xE               /* wheel voice 2, tyre_sound_squeal_5 [5] */
};

/* CarSoundFrame::engine[] (the streamed voices of CarSound::engines[]). */
enum {
    SND_ENGINE_0    = 0,                    /* param: the rpm */
    SND_ENGINE_1    = 1,                    /* param: the rpm */
    SND_ENGINE_WIND = 2                     /* param: car_sound_wind's (integer) pitch */
};

/* CarSoundFrame::voice[] (CarSound::slots[]): four car voices, then seven
 * per wheel. */
enum {
    SND_VOICE_GEAR_WHINE = 0,
    SND_VOICE_TURBO      = 1,
    SND_VOICE_ROAD       = 2,
    SND_VOICE_SILENT     = 3,               /* always volume 0 */
    SND_VOICE_WHEEL      = 4,               /* + 7 x wheel + the wheel voice */
    SND_WHEEL_VOICES     = 7
};

/* CarSoundFrame::oneShot[] (CarSound's one-shot sounds 13, 15, 16, 17). */
enum {
    SND_ONESHOT_BLOWOFF = 0,
    SND_ONESHOT_CRASH   = 1                 /* + the crash kind 0..2 (car_crash_sound_add) */
};

/* The speed of sound per frame, about 331.45 m/s / 60 (0x40B0C5FA, a .lit4
 * word; the Doppler factor of updateSound). */
#define SOUND_SPEED_PER_FRAME 5.524167f

#endif /* U042_H */
