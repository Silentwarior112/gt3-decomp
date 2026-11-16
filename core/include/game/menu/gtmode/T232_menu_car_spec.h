/* T232_menu_car_spec.h - MenuCarSpec: the car on show in the menus
 * (translation unit T232, unit u068).
 *
 * (? name, as in u010 / u021 / u064.)  One instance (menu_car_spec(), made
 * at start-up by menu_car_spec_create with the global operator new).  setCar
 * sets up its ShowRoomCar `car' (T278 SetupShowRoomCar, 0x001D93C0) from a
 * car code (its own CarEquipments and a fresh CarCondition) or from a garage
 * car (the CarInformation's equipments copied, its own condition); the
 * getters read the equipments in use and the colour database (T485).  Units
 * u037, u061, u108 and u109 used the name in the first pass for a bare T278
 * ShowRoomCar (their 0x1350-byte locals), this class's first member. */
#ifndef GAME_T232_MENU_CAR_SPEC_H
#define GAME_T232_MENU_CAR_SPEC_H

#include "game/game.h"
#include "game/data/T482_car_equipments.h"       /* CarEquipments, held by value */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar (car), held by value */

class CarInformation;                       /* T483 (pointers only) */

/* Not polymorphic.  Size 0x1538 (operator new in menu_car_spec_create
 * 0x001C4500). */
struct MenuCarSpec {
    MenuCarSpec();                          /* 0x001C4210 no car */
    void        clear();                    /* 0x001C4240 no car */
    void        setCar(const char *name);   /* 0x001C4248 nothing if the car does not load */
    void        setCar(CarInformation *car); /* 0x001C42B0 */
    const char *getName();                  /* 0x001C4308 */
    s64         getCode();                  /* 0x001C4328 */
    const char *getMakerName();             /* 0x001C4348 CarEquipments::getMakerName (F_001C4348) */
    const char *getBrandName();             /* 0x001C4368 CarEquipments::getBrandName (getMaker) */
    int         getColorCount();            /* 0x001C4388 */
    void        getColors(u32 *out, u32 orMask); /* 0x001C43B0 out[i] = RGBA of colour i | orMask */
    const char *getColorName(int i);        /* 0x001C4450 NULL out of range */
    int         hasDirtTires();             /* 0x001C44C0 1 if the front tyres are the dirt tyres
                                               (getFronttirePart() == part id 0x42; F_001C44C0) */
    CarEquipments *getEquipments();         /* 0x001C44E8 current */

    /* The menus read car.spec (CarSpec: length +0x1A, height +0x22, the
     * treads +0x26 / +0x28 (unk_026 / unk_028), weight +0x2A, the handling /
     * acceleration ratings +0x0C / +0x0D (handlingRating / accelRating, T032)), the tyre
     * size per axle car.dyn.gear.tyreRadius / tyreWidth / tyreSidewall
     * (+0x658 / +0x668 / +0x670: setSizeProfile's outer radius, width and
     * profile height, T107 / T212) and car's figures (power, torque,
     * maxSpeed) and wheel accessors.  (The sweep replaced the spelled-out
     * bytes unk_0000 .. tyre658 / 668 / 670 .. unk_0678 with the member.) */
    ShowRoomCar    car;                     /* 0x0000 set up by setCar */
    /*             4 bytes of padding          0x1344 (CarEquipments is 8-byte aligned) */
    CarEquipments  equip;                   /* 0x1348 the car of setCar */
    CarEquipments *current;                 /* 0x1530 the equipments in use; NULL: no car */
    u8             unk_1534[0x4];
};
EGCS_SIZE(MenuCarSpec, 0x1538);

void         menu_car_spec_create(void);    /* 0x001C44F0 at start-up (game_init, T007) */
MenuCarSpec *menu_car_spec(void);           /* 0x001C4548 the instance (0x00352644) */

#endif /* GAME_T232_MENU_CAR_SPEC_H */
