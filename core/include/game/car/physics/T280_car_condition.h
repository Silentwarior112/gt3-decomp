/* T280_car_condition.h - CarCondition, the wear of a car (translation unit
 * T280, unit u076), 0x001D9AF0..0x001D9FB8.
 *
 * The odometer, the engine and oil mileages and the dirt of a car.  The
 * mileages are bounded by the thresholds of ConditionModel g_002D2B00
 * (T283): the engine runs from newEngineMileage() (negative: run-in) up to
 * maxEngineMileage(); the oil from -maxOilMileage() (new) up to it.  It is
 * held by value in CarSaveInformation (+0x0C, T483), DynCarState (+0x3F8,
 * RaceCarState +0x137C) and RaceEntryCar (+0x2104), and built on the stack by
 * ValuateAutomobilePerformance / SelectMeterScale (T279).
 * CarInformation's condition wrappers (T483) are named after these methods. */
#ifndef GAME_T280_CAR_CONDITION_H
#define GAME_T280_CAR_CONDITION_H

#include "game/game.h"

/* Not polymorphic.  Size 0x20 (clear() memsets 0x20; the DynCarState field
 * +0x3F8 is followed by +0x418). */
class CarCondition {
public:
    CarCondition();                         /* 0x001D9B10 clear() */
    void clear();                           /* 0x001D9AF0 memset(this, 0, 0x20) */
    void setOdometer(u32 v);                /* 0x001D9B38 at most 0xEE6B27A6 */
    u32  odometerDistance();                /* 0x001D9B60 tenths of a km (odometer / 50) or of a mile (/ 80, the
                                               imperial setting; T219 prints it with "%.1t") */
    f32  addOdometer(f32 d);                /* 0x001D9BB0 d metres: counts d / 2 whole units; returns the rest of d */
    void setEngineMileage(f32 v);           /* 0x001D9C20 at most maxEngineMileage() */
    void addEngineMileage(f32 d);           /* 0x001D9C80 */
    void resetEngineMileage();              /* 0x001D9CE8 newEngineMileage() */
    void setOilMileage(f32 v);              /* 0x001D9D18 at most maxOilMileage() */
    void addOilMileage(f32 d);              /* 0x001D9D78 */
    int  oilNeedsChange();                  /* 0x001D9E20 oilChangeMileage() <= oilMileage */
    f32  oilDirtiness();                    /* 0x001D9E68 0 new .. 1 used up */
    void setDirt(f32 v);                    /* 0x001D9F18 at most 0.8 */
    void addDirt(f32 d);                    /* 0x001D9F38 at most 0.8 (the loose surfaces) */
    void resetEngine();                     /* 0x001D9F60 resetEngineMileage() (a new car: GameStatus::buyCar,
                                               CarInformation::resetEngine) */
    void changeOil();                       /* 0x001D9F80 setOilMileage(newOilMileage()) */

    u32 odometer;                           /* 0x00 in units of 2 m (/ 50 = 0.1 km, / 80 = 0.1 mile; u066) */
    f32 engineMileage;                      /* 0x04 (u076 engine; u082 f04): the run-in and the wear */
    f32 oilMileage;                         /* 0x08 (u076 oil; u082 f08) */
    f32 dirt;                               /* 0x0C 0..0.8 */
    u8  unk_10[0x10];                       /* 0x10 (only cleared) */
};
EGCS_SIZE(CarCondition, 0x20);

#endif /* GAME_T280_CAR_CONDITION_H */
