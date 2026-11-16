/* T347_ground_surface.h - the surface type of a ground query (translation
 * unit T347, unit u081), 0x001E8A48..0x001E8AE0.
 *
 * GroundQuery is u077 / u081's name for RunwayContact (T153): a typedef in
 * T330_dyn_car_init.h, so it is included here, not forward-declared. */
#ifndef GAME_T347_GROUND_SURFACE_H
#define GAME_T347_GROUND_SURFACE_H

#include "game/game.h"
#include "game/car/physics/T330_dyn_car_init.h"         /* GroundQuery (a typedef of RunwayContact) */

struct RunwayContact;                       /* T153_race_course.h (pointers only) */

/* The query's surface code; with bit 1 of its flags (+0x33) the codes 0 / 7
 * become 12, 1 becomes 13 and 9 becomes 14. */
u8 ground_surface_type(GroundQuery *q);     /* 0x001E8A48 */

#endif /* GAME_T347_GROUND_SURFACE_H */
