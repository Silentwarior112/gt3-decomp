/* race_database_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u108,
 * 0x00236600..0x00239ED0: translation units T482 (its end), T483, T484,
 * T485, T486 (data only), T487, T488.
 *
 *   T482  CarEquipments, one file with the rest of T482 in u106's folder
 *         (src/game/data/T482_car_equipments.cpp: the tyre helpers, the save
 *         block copies, the class-inline accessors and part setters, the
 *         typeinfo functions at this unit's addresses 0x00236600..0x00237860).
 *         game/data/T482_car_equipments.h
 *   T483  CarInformation: a CarEquipments plus CarSaveInformation (the parts
 *         owned, one bit per part id, and the car's condition): prices,
 *         ownership bits, loading a car, the 0x1FC-byte save block.
 *         game/data/T483_car_information.h
 *   T484  RaceDataBase: a CarDataBase with the selected race (event) record
 *         of table 0x20: entry cars, regulated cars, prize money, times, the
 *         enemy and tyre wear parameters; the course records of table 0x22.
 *         game/data/T484_race_database.h
 *   T485  CarColorDataBase (? name: no RTTI): database/carcolor.db + .sdb,
 *         the colours of each car (the static object g_00352D98).
 *         game/data/T485_car_color.h
 *   T486  the maker name tables g_002EFF20 / g_002EFFD8 (no code; their
 *         externs are in src/game/data/car_equipments_internal.h, T482's getMakerName /
 *         getBrandName read them).  T486_maker_names.cpp
 *   T487  RaceModeDatabase: database/racemode.db + racedetail.db (the static
 *         object g_002F00C0).  game/data/T487_race_mode_database.h
 *   T488  SettingSerialize: a Serialize::Base holding a CarEquipments and a
 *         course code ("SettingSerialize" registration).
 *         game/save/T488_setting_serialize.h
 *
 * Every class is defined in its canonical header; this header includes them
 * and keeps what only the unit uses: the record of table 0x21, the unit's
 * data and the typeinfo storage of its classes.  Offsets in comments are the
 * original (egcs) ones.
 */
#ifndef U108_H
#define U108_H

#include "types.h"
#include <stddef.h>

/* own translation units (T482's file is in src/u106) */
#include "game/data/T482_car_equipments.h"           /* CarEquipments, CarSaveEquipments, the records, CarSpec */
#include "game/data/T483_car_information.h"          /* CarInformation, CarSaveInformation */
#include "game/data/T484_race_database.h"            /* RaceDataBase, RaceEvent, RaceTime, CourseRecord */
#include "game/data/T485_car_color.h"                /* CarColorDataBase g_00352D98, CarColorRecord */
#include "game/data/T487_race_mode_database.h"       /* RaceModeDatabase g_002F00C0, its records */
#include "game/save/T488_setting_serialize.h"        /* SettingSerialize, g_00352EE8 */

/* outside the unit */
#include "game/car/physics/T280_car_condition.h"            /* CarCondition */
#include "game/data/T481_car_data_base.h"            /* CarDataBase g_002EBD88, DataBlock, StringsDataBase,
                                                   nameToCode, __ti11CarDataBase */
#include "engine/io/T503_vol_thread.h"               /* file_read_blocking, vol_device_object */
#include "engine/io/T509_serialize.h"                /* Serialize::Base, ClassInfo, Factory, Header,
                                                   __tiQ29Serialize4Base */
#include "game/data/T516_country.h"                  /* Country g_00352FD8, COUNTRY_* */
#include "runtime/newlib/G007_stdio.h"                    /* memcpy, memset */
#include "engine/pdistd/T532_heap.h"                     /* free, memalign */
#include "engine/pdistd/T534_sprintf.h"                  /* sprintf */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                    /* __rtti_class / __rtti_si / __rtti_user,
                                                   __class_type_info::base_info */
#endif

/* ======================================================================== */
/* Names for the numbers                                                    */
/* ======================================================================== */

/* CARDB_* (the paramdb record tables): game/data/T482_car_equipments.h. */

/* PART_* (the part ids): game/data/T482_car_equipments.h. */

/* TIRE_KIND_* (tyre kinds): game/data/T482_car_equipments.h. */

/* ======================================================================== */
/* Records and classes only u108 uses                                       */
/* ======================================================================== */

/* A record of table 0x21: the cars a race allows (RaceEvent::carList; the
 * first pass called it PrizeCarsRecord; u109 has the same local copy, h11). */
struct CarListRecord {
    u64 code;                                   /* 0x00 */
    u64 cars[0x20];                             /* 0x08 car codes, "-----" ends the list */
};

#define POWER_UNIT_PS 0x7073                    /* 'ps': ShowRoomCar::power's unit tag */

/* GameThumbnail.type of a car setting save (T108: 0 game data, 1 car
 * setting, 2 replay, 3 best lap; u098 has THUMBNAIL_GAME_DATA). */
#define THUMBNAIL_SETTING 1

/* ======================================================================== */
/* The unit's data                                                          */
/* ======================================================================== */

/* T484 .sdata: the championship points of positions 1..8 (defined in
 * T484_race_database.cpp). */
extern const s8 g_00352D78[8];                  /* 0x00352D78 gp-0x4C78 {10, 6, 4, 3, 2, 1, 0, 0} */

/* T488 .sdata: the factory of the registration "SettingSerialize"
 * (g_002F03F8); both defined in T488_setting_serialize.cpp, the factory
 * first (this extern gives the const its external linkage). */
extern const Serialize::Factory g_00352EE0;     /* 0x00352EE0 gp-0x4B10 {setting_serialize_get,
                                                   setting_serialize_release} */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions and their storage itself; these are
 * for GCC 14 only.  The type_info storage of the unit's classes nobody else
 * names (__ti14CarInformation is in T483, __ti11CarDataBase in T481, T482's
 * in car_equipments_internal.h): __class_type_info = 4 words, __si_type_info = 3,
 * __user_type_info = 2. */
extern "C" {
extern u32 __ti18CarSaveInformation[2];         /* 0x003534A8 gp-0x4548 */
extern u32 __ti12RaceDataBase[3];               /* 0x004B2A60 */
extern u32 __ti16RaceModeDatabase[2];           /* 0x003534B0 gp-0x4540 */
extern u32 __ti16SettingSerialize[3];           /* 0x004B2A70 */
}

/* The __class_type_info base list (.rodata), both bases public. */
extern const __class_type_info::base_info g_003423B0[2];   /* 0x003423B0 CarInformation:
                                                               {CarEquipments, +0}, {CarSaveInformation, +0x1E8} */
#endif /* !BOOT_OLD_GCC */

#endif /* U108_H */
