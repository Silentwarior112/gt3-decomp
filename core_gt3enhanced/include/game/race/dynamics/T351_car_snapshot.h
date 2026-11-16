/* T351_car_snapshot.h - a snapshot of a RaceCarState (translation unit T351,
 * unit u083), 0x001EE288..0x001EE638.
 *
 * The car is copied whole into / out of a byte buffer (0x16DC bytes, memcpy:
 * ldl / ldr pairs when either pointer is not 8-aligned, lwl / lwr for the
 * last word).  A restore (the data logger's sector
 * records, T306, and the replay snapshots) keeps the car's four links and
 * points the curves of its parameter block (RaceCarState +0x10) back into
 * itself. */
#ifndef GAME_T351_CAR_SNAPSHOT_H
#define GAME_T351_CAR_SNAPSHOT_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */
class RaceOrganization;                     /* T125_race_organization.h */
class DynamicsConductor;                    /* T305_dynamics_conductor.h */
class RaceCourse;                           /* T153_race_course.h */
class RaceInput;                            /* T413_race_input.h */
class CarDynamics;                          /* T279_meter_scale.h */

/* The four links at the head of a RaceCarState (org, dc, course, input),
 * which a restore keeps; copied member by member (four lw / sw).  Not polymorphic.
 * Size 0x10. */
class CarLinks {
public:
    RaceOrganization  *org;                 /* 0x0 */
    DynamicsConductor *dc;                  /* 0x4 */
    RaceCourse        *course;              /* 0x8 */
    RaceInput         *input;               /* 0xC (u083 CarDynamics *cd) */
};
EGCS_SIZE(CarLinks, 0x10);

/* Each Curve of the parameter block gets its x / y pointing at the arrays
 * that follow it: the engine's (+0x138), steer0..2 (+0x5C / +0x98 / +0xD4),
 * the five of each of the six TyreData (+0x398, two share one x array) and
 * camberCurve[2] (+0x340).  (The first pass's parameter was u8 *.) */
void car_relink_curves(CarDynamics *params);                   /* 0x001EE288 */
void car_links_save(CarLinks *src, CarLinks *dst);             /* 0x001EE3A0 *dst = *src */
void car_links_restore(CarLinks *dst, CarLinks *src);          /* 0x001EE3C8 *dst = *src */
/* The car into a snapshot buffer (OneLapGhostInfo::car, T313). */
void car_copy(u8 *dst, const RaceCarState *src);               /* 0x001EE3F0 */
/* The snapshot over the car, keeping the car's links, then the curves
 * relinked and the computer driver bound to the car again (ai_init, T367). */
void car_restore(RaceCarState *car, const u8 *snap);           /* 0x001EE4D8 */

#endif /* GAME_T351_CAR_SNAPSHOT_H */
