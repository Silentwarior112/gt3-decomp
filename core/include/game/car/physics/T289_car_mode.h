/* T289_car_mode.h - the display level of a car and the sound reset it guards
 * (translation unit T289, unit u078), 0x001E0668..0x001E0740.
 *
 * car_display_level tells how a car is shown: in full, as a ghost
 * (RacePS2Base::render draws a level-2 car with the scan mask, gs_set_scanmask_mode(2)),
 * only as a faded minimap dot, or not at all.  Only the cars without a
 * controller (DynCarState.noController: the free run's ghost, the data
 * logger's other runs) are below CAR_SHOW_FULL. */
#ifndef GAME_T289_CAR_MODE_H
#define GAME_T289_CAR_MODE_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */

/* car_display_level's levels (the callers compare: drawn from 2, sparks /
 * lamps from 2, the minimap dot faded below 3, sound and the camera targets
 * only at 3). */
enum {
    CAR_SHOW_HIDDEN = 0,                    /* not drawn, no minimap dot */
    CAR_SHOW_MAP    = 1,                    /* only the minimap dot (faded) */
    CAR_SHOW_GHOST  = 2,                    /* drawn as a ghost (scan mask), faded dot */
    CAR_SHOW_FULL   = 3                     /* a car with a controller */
};

/* CAR_SHOW_FULL when DynCarState.noController is 0; CAR_SHOW_MAP when
 * neither the organization's loggerMode (RaceOrganization +0x70) nor its
 * race's +0x217A4 (RaceSolitaire::showGhost, the ghost switch of button
 * 0x100) is set; else CAR_SHOW_HIDDEN when the driver or ctrlMode is 1,
 * CAR_SHOW_GHOST otherwise.  Callers: RacePS2Base::render, RaceMiniMap::
 * drawCar, RaceCarModel::emitSparks / addLamps, CameraManager::isWatchable,
 * F_0020E1F0 (T393), car_silence_hidden.  (First pass F_001E0668.) */
int  car_display_level(RaceCarState *car);  /* 0x001E0668 */
/* Unless car_display_level is CAR_SHOW_FULL: silences the tyres (smoke 0,
 * squeal volume 0, pitch 1) and the three engine sound levels.  Called by
 * car_step (T302).  (First pass F_001E06D8.) */
void car_silence_hidden(RaceCarState *car); /* 0x001E06D8 */

#endif /* GAME_T289_CAR_MODE_H */
