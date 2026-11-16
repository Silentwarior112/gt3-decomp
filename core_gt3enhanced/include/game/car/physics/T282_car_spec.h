/* T282_car_spec.h - SetupCarSpec, the car parameter record of a CarEquipments
 * (translation unit T282, unit u076), 0x001DA308..0x001DA708.
 *
 * memset, the race's tyres fitted first (fitTyres), CarEquipments::getSpec,
 * then the defaults of the fields the car data leaves at 0 (the steering
 * curves steer[1] / steer[2] from the local .sdata 0x00352900 / 0x00352908,
 * the tyre blocks, tyreSet), and no driving aids on a dirt course.  The
 * callers pass their own name as `caller' ("ShowRoomCar::SetupShowRoomCar()",
 * "DynamicsConductor::initialize()", "ValuateAutomobilePerformance()", ...),
 * which is not used. */
#ifndef GAME_T282_CAR_SPEC_H
#define GAME_T282_CAR_SPEC_H

#include "game/game.h"

class CarEquipments;                        /* T482_car_equipments.h (pointers only) */
struct CarSpec;

/* courseKind: DynamicsConductor::getCourseKind() (0 from the menus).
 * fitTyres (0 from every caller in this build): fits the course's tyres to
 * equip first (the TIRE_KIND_DIRT parts of tables CARDB_FRONT_TIRE /
 * CARDB_REAR_TIRE on course kind 2, dirt; none on kind 1; else
 * TIRE_KIND_ARCADE_RACING if tyreFlag). */
void SetupCarSpec(CarSpec *spec, CarEquipments *equip, int tyreFlag, int courseKind,
                  int fitTyres, const char *caller);       /* 0x001DA308 */

/* T282's .sdata: the default curve of the tyre blocks (CarSpecTyre.curve4).
 * Not const: SetupCarSpec reads g_00352918[j] after its store of
 * curve4.x[j] (a const table's load is moved above the store). */
extern u8 g_00352910[6];                    /* 0x00352910 {0x00, 0x1F, 0x3E, 0x69, 0xA5, 0xFF} */
extern u8 g_00352918[6];                    /* 0x00352918 {0x00, 0x1C, 0x3D, 0x72, 0xB4, 0xC4} */

#endif /* GAME_T282_CAR_SPEC_H */
