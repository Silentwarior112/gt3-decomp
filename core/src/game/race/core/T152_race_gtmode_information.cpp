/* T152_race_gtmode_information.cpp - RaceGTmodeInformation
 * (0x0018F3E0..0x0018F688); the class is declared in
 * include/game/race/core/T152_race_gtmode_information.h.
 *
 * The information of a race of the GT mode: RaceInformation with the
 * player's car (a CarEquipments at +0x60), which goes to entry 0 with input
 * id 0x100; the other entries get random opponents of the race's car list
 * and the player is moved to the back of the grid.  The entry list is a
 * RaceEntryPS2; 6 cars.  The base of RaceChampionshipInformation (T151).
 *
 * Includes (u050's data pass): the classes retail's T152 saw, in its order.
 * Its .rodata (0x0032DAB8..0x0032DD0E) is "bad_alloc" (<new>), the vtable,
 * then the type names in reverse definition order: its own, the race
 * display's (T154, which reads T153 with UnitArena<64>'s name) and the
 * GameStatus's garage lists (T429, first); no menu classes, no Operator.
 * Its .sdata (0x00351F90) is T157's "" alone (read with the race display).
 */
// DATA_START 0x0032DAB8
// DATA_START 0x00351F90
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include <new>                              /* "bad_alloc" (retail's T152 has it first) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si (GCC 14's __tf functions) */
#endif
#include "game/save/T429_game_status.h"          /* (the garage lists: retail's type-name order) */
#include "game/race/display/T154_race_display.h"         /* (the race display classes: retail's type-name order) */
#include "game/race/core/T152_race_gtmode_information.h"  /* own; + T124 RaceInformation, T482 CarEquipments */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar, RaceEntryInformation */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, the entry order) */
#include "game/race/core/T130_race_ps2_base.h"        /* RaceEntryPS2 */
#include "game/data/T485_car_color.h"            /* CarColorDataBase g_00352D98 */
#include "game/race/core/race_championship_internal.h"                           /* the unit's constants */

// FUNC 0x0018F3E0 RaceGTmodeInformation::RaceGTmodeInformation
//   RaceInformation::RaceInformation, the vptr, the car
//   (CarEquipments::CarEquipments); transmission 0 (the vibration word is
//   left as it is), 6 cars.  The landing pad destroys the base
//   (RaceInformation::~RaceInformation, flags 0).  Callers: run_gtmode_race
//   (T053) and RaceChampionshipInformation's constructor.
RaceGTmodeInformation::RaceGTmodeInformation()
{
    /* implicit: RaceInformation (base constructor), CarEquipments (member constructor), ~RaceInformation (exception cleanup) */
    transmission = 0;
    entryCount = GTMODE_RACE_CARS;
}

// FUNC 0x0018F458 RaceGTmodeInformation::~RaceGTmodeInformation
//   (index: virtual_0.)  The vptr, ~car (CarEquipments::~CarEquipments,
//   flags 2), then RaceInformation::~RaceInformation with the caller's
//   flags (it does the operator delete).  Callers: run_gtmode_race (T053),
//   RaceChampionshipInformation's constructor (landing pad) and destructor.
RaceGTmodeInformation::~RaceGTmodeInformation()
{
    /* implicit: ~CarEquipments (member destructor), ~RaceInformation (base destructor) */
}

// FUNC 0x0018F4A8 RaceGTmodeInformation::createEntries
//   (index: virtual_4; also RaceChampionshipInformation's slot 4.)  new
//   RaceEntryPS2 (0x3C bytes: RaceEntryPS2::operator new, the
//   constructor, RaceEntryPS2::operator delete(p, 0x3C) if it throws).
RaceEntryBase *RaceGTmodeInformation::createEntries()
{
    return new RaceEntryPS2;                    /* implicit: RaceEntryPS2::operator delete (if the constructor throws) */
}

// FUNC 0x0018F500 RaceGTmodeInformation::setupEntry
//   (index: virtual_10; also RaceChampionshipInformation's.)  Only the
//   player's car (entry number 0): its equipments (only the CarEquipments
//   part, CarEquipments::operator=) and the player's input id.
void RaceGTmodeInformation::setupEntry(RaceEntryCar *entry)
{
    if (entry->number == 0) {
        RaceEntryInformation *info = &entry->info;
        static_cast<CarEquipments &>(*info) = car;  /* implicit: CarEquipments::operator= */
        info->inputId = INPUT_ID_PLAYER;
    }
}

// FUNC 0x0018F540 RaceGTmodeInformation::setupOrganization
//   (index: virtual_11.)  Random opponents for the cars 1..count-1
//   (RaceInformation slot 13, assignCarRange), the entry order reset, and
//   the player moved from the front to the back (moveEntryOrder(0,
//   count - 1); the count is read again through the same list pointer).
void RaceGTmodeInformation::setupOrganization(RaceOrganization *org)
{
    RaceEntryBase *list = org->entries;
    assignCarRange(org, 1, list->count);
    org->resetEntryOrder();
    org->moveEntryOrder(0, list->count - 1);
}

// FUNC 0x0018F5A8 RaceGTmodeInformation::setCar
//   car = c (CarEquipments::operator=).  Callers: run_gtmode_race (T053),
//   RaceChampionshipInformation::setPlayerCar.
void RaceGTmodeInformation::setCar(const CarEquipments &c)
{
    car = c;                                    /* implicit: CarEquipments::operator= */
}

// FUNC 0x0018F5C8 RaceGTmodeInformation::setCar
//   Loads car `code' ($a1 passed through to CarEquipments::setCar; its
//   result is not tested) and gives it colour 0 of its model's colour list
//   (CarColorDataBase::getColor of getModelCode(), then setColorCode).
//   Caller: RaceChampionshipInformation::setPlayerCar(u64).
void RaceGTmodeInformation::setCar(u64 code)
{
    car.setCar(code);
    car.setColorCode(g_00352D98.getColor(car.getModelCode(), 0));
}

// FUNC 0x0018F618 RaceGTmodeInformation::getTransmission
int RaceGTmodeInformation::getTransmission()
{
    return transmission;
}

// FUNC 0x0018F620 RaceGTmodeInformation::setTransmission
//   The bool is stored as a word.
void RaceGTmodeInformation::setTransmission(bool v)
{
    transmission = v;
}

// FUNC 0x0018F628 RaceGTmodeInformation::getVibrartion
//   (sic: the original's spelling.)
int RaceGTmodeInformation::getVibrartion()
{
    return vibration;
}

// FUNC 0x0018F630 RaceGTmodeInformation::setVibration
//   The bool is stored as a word.
void RaceGTmodeInformation::setVibration(bool v)
{
    vibration = v;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x0018F638 __tf21RaceGTmodeInformation
//   The __si_type_info __ti21RaceGTmodeInformation, base
//   __ti15RaceInformation (gp-0x4678).  Caller:
//   __tf27RaceChampionshipInformation.
extern "C" const type_info &__tf21RaceGTmodeInformation(void)
{
    if (__ti21RaceGTmodeInformation[0] == 0) {
        __tf15RaceInformation();
        __rtti_si(__ti21RaceGTmodeInformation, "21RaceGTmodeInformation",
                  (const type_info *)__ti15RaceInformation);
    }
    return *(const type_info *)__ti21RaceGTmodeInformation;
}
#endif
