/* prize_records_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u096,
 * 0x00213210..0x00216308.
 *
 * Translation units (all parts of the save data, gt2k_game::GameStatus):
 *   T417  TimeTrialRecords (GameStatus+0x246F8): the records of the 10 arcade
 *         time trial courses AT010001..AT100001, 12 places each (time, name,
 *         date, ranking code) and the best place ever taken.  reset() fills
 *         them with three preset records per course (Polyphony staff names).
 *   T418  BestPlace: one int, the best place taken (lower is better).
 *   T419  the prize cars: PrizeCarList (the prize sets a race gives, one per
 *         category) and the checks behind it, one per category of prize set
 *         (race / championship / licence clear, arcade and GT mode series,
 *         GT mode complete, 50 / 75 / 100 % achievement).
 *   T420  LicenseRecords (GameStatus+0x1F550): 6 licences x 8 tests, each a
 *         medal and a 6-place ranking (time, name, date, ranking code).
 *   T421  PrizeSetFlags (GameStatus+0x2EE60): one byte per prize set, set once
 *         the set (a single-car one) has been won.
 *   T422  ChampionshipSave (GameStatus+0x2EF28): the suspended championship.
 *   T423  CursorMemory: a page name and the button index last used on it.
 *
 * Every class is defined in its canonical header (include/game/T417..T423);
 * this header adds the outside declarations they need and the unit's local
 * constants.  Save data: every class has a static getSize(), load(src, arg)
 * (returns the end of what it read; arg is unused) and save(dst) (returns
 * the end).
 */
#ifndef U096_H
#define U096_H

#include "types.h"
#include "libc.h"                       /* memcpy, strcpy, strcmp, strlen (core 0x00269A68,
                                           the MMI strlen) */
#include <stddef.h>

#include "runtime/newlib/G007_stdio.h"            /* strstr */
#include "engine/pdistd/T534_sprintf.h"          /* sprintf */
#include "game/race/core/T128_race_lap_record.h"  /* RACE_TIME_INVALID (0x157529FF: "no time") */

/* own translation units */
#include "game/save/T417_time_trial_records.h"
#include "game/save/T418_best_place.h"
#include "game/save/T419_prize_cars.h"
#include "game/save/T420_license_records.h"
#include "game/save/T421_prize_set_flags.h"
#include "game/save/T422_championship_save.h"
#include "game/save/T423_cursor_memory.h"

/* outside the unit (none of these headers puts a type name or a literal into
 * an object: the GameStatus's header, which writes the garage lists' type
 * names, is included by T419 alone, the car list's by T420: the data pass,
 * ref/data/u096/notes.md) */
#include "game/data/T435_product_config.h"   /* product_config, the prize set queries */
#include "game/sound/T465_clock.h"            /* clock_seconds (the record dates) */
#include "game/data/T481_car_data_base.h"    /* nameToCode */
#include "game/data/T482_car_equipments.h"   /* CarEquipments (ChampionshipSave's cars) */
#include "game/data/T484_race_database.h"    /* RaceDataBase, RaceEvent (= RaceDataEntry), RaceTime */
#include "game/data/T489_car_database.h"     /* race_name_split */
#include "game/common/T494_text_utils.h"       /* str_equal */

/* The ranking code of an empty place: 30 dashes (RankingCode, T415).  The
 * clear functions copy it with an inline 31-byte strcpy. */
#define RANKING_CODE_EMPTY "------------------------------"

/* The date of every preset time trial record (TimeTrialCourse::getDate).  In
 * clock_seconds units (seconds since 2000-01-01 00:00) it is 2001-04-28
 * 00:01:00 (? the reading), the day GT3 came out in Japan. */
#define TIME_TRIAL_PRESET_DATE 0x027CC4BC

/* enum RaceKind (race_name_split's kinds): game/data/T489_car_database.h. */

#endif /* U096_H */
