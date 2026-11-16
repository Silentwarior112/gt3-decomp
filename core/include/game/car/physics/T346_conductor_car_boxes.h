/* T346_conductor_car_boxes.h - the DynamicsConductor's car boxes
 * (translation unit T346, unit u081), 0x001E8730..0x001E8910.
 *
 * The cars' position / velocity / yaw / yaw rate / runway sector go to the
 * collision boxes (CarBox, T376, the conductor's car list) before the
 * collision step and come back after it (callers: DynamicsConductor slot 21
 * and the conductor's step, T317). */
#ifndef GAME_T346_CONDUCTOR_CAR_BOXES_H
#define GAME_T346_CONDUCTOR_CAR_BOXES_H

#include "game/game.h"

class DynamicsConductor;                    /* T305_dynamics_conductor.h (pointers only) */

void conductor_store_car_boxes(DynamicsConductor *dc);     /* 0x001E8730 the cars to their boxes */
void conductor_load_car_boxes(DynamicsConductor *dc);      /* 0x001E8828 the boxes back to the cars */

#endif /* GAME_T346_CONDUCTOR_CAR_BOXES_H */
