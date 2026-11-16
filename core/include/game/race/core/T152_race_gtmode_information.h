/* T152_race_gtmode_information.h - RaceGTmodeInformation, the race
 * information of a GT mode race (translation unit T152, unit u050),
 * 0x0018F3E0..0x0018F688.
 *
 * RaceInformation with the player's car (a CarEquipments at +0x60), which
 * goes to entry 0 with input id 0x100; the other entries get random
 * opponents of the race's car list and the player is moved to the back of
 * the grid.  6 cars, the entry list a RaceEntryPS2.  The vibration /
 * transmission words are filled from the GameStatus options (u017:
 * GameOptions::unk14 / unk24 ^ 1).  run_gtmode_race (T053) builds one on the stack;
 * the base of RaceChampionshipInformation (T151).
 *
 * The virtual methods keep RaceInformation's slot names (T124). */
#ifndef GAME_T152_RACE_GTMODE_INFORMATION_H
#define GAME_T152_RACE_GTMODE_INFORMATION_H

#include "game/game.h"
#include "game/race/core/T124_race_information.h"         /* base RaceInformation */
#include "game/data/T482_car_equipments.h"           /* CarEquipments (held by value) */

class RaceEntryBase;                            /* T121 (pointers only) */
class RaceEntryCar;                             /* T121 (pointers only) */
class RaceOrganization;                         /* T125 (pointers only) */
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* (__si_type_info 0x004B2140, base RaceInformation 0x00353378) */
extern "C" const type_info &__tf21RaceGTmodeInformation(void); /* 0x0018F638 */
#endif

/* vtable 0x0032DAC8 (15 slots; 0, 4, 10 and 11 its own), typeinfo
 * __tf21RaceGTmodeInformation 0x0018F638.  vptr +0x54 (RaceInformation's);
 * 0x248 bytes (RaceChampionshipInformation's data start there). */
class RaceGTmodeInformation : public RaceInformation {
public:
    /* The car; transmission 0 (vibration is left as it is), 6 cars.
     * Callers: run_gtmode_race (T053), RaceChampionshipInformation's constructor. */
    RaceGTmodeInformation();                    /* 0x0018F3E0 */
    virtual ~RaceGTmodeInformation();           /* [ 0] +0x0C 0x0018F458 */
    /* new RaceEntryPS2 (also RaceChampionshipInformation's slot 4) */
    virtual RaceEntryBase *createEntries();     /* [ 4] +0x2C 0x0018F4A8 */
    /* Entry number 0 only: the car (CarEquipments::operator=) and input
     * id 0x100. */
    virtual void setupEntry(RaceEntryCar *entry);           /* [10] +0x5C 0x0018F500 */
    /* Random opponents for the cars 1..count-1 (assignCarRange), the entry
     * order reset, the player moved from the front to the back. */
    virtual void setupOrganization(RaceOrganization *org);  /* [11] +0x64 0x0018F540 */

    /* car = c.  Callers: run_gtmode_race (T053), RaceChampionshipInformation::
     * setPlayerCar. */
    void setCar(const CarEquipments &c);        /* 0x0018F5A8 */
    /* Loads car `code', then its colour of colour id 0. */
    void setCar(u64 code);                      /* 0x0018F5C8 */
    /* (symbols; no callers) */
    int  getTransmission();                     /* 0x0018F618 */
    void setTransmission(bool v);               /* 0x0018F620 */
    int  getVibrartion();                       /* 0x0018F628 (sic) */
    void setVibration(bool v);                  /* 0x0018F630 */

    int           vibration;                    /* 0x58 */
    int           transmission;                 /* 0x5C */
    CarEquipments car;                          /* 0x60 the player's car */
};
EGCS_SIZE(RaceGTmodeInformation, 0x248);

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* The __si_type_info {vptr, name, base} __tf21RaceGTmodeInformation fills. */
extern "C" u32 __ti21RaceGTmodeInformation[3]; /* 0x004B2140 (u050 g_004B2140) */
#endif

#endif /* GAME_T152_RACE_GTMODE_INFORMATION_H */
