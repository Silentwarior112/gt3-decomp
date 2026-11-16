/* T348 - the speed factor of each wheel (it scales the surface bumps,
 * car_ground_contact). */
#include "game/car/physics/T348_wheel_speed_factor.h"
#include "game/car/physics/suspension_internal.h"

// FUNC 0x001E9E18 car_wheel_speed_factors
/* min(|the wheel's velocity| x 0.02, 1): 1 from 50 m/s. */
void car_wheel_speed_factors(CarDynState *st)
{
    int i;

    for (i = 3; i >= 0; i--) {
        CarWheelState *w = &st->wheel[3 - i];
        f32 factor = boot_sqrtf(w->velX * w->velX + w->velY * w->velY);
        factor *= 0.019999998f;
        if (factor > 1.0f)
            factor = 1.0f;
        w->speedFactor = factor;
    }
}
