/* T363_ai_gear.h - the computer driver's gear for its target speed
 * (translation unit T363, unit u086), 0x001F6578..0x001F6678.
 *
 * AIDriver.gear / gearSet (+0xAC / +0xAD, shown by the race display, u051):
 * the gear for the speed aimed at (car_gear_for_speed, T343), once the car is
 * not slower than it.  Callers: DynCar_initialize (T330), car_input_update
 * (T342). */
#ifndef GAME_T363_AI_GEAR_H
#define GAME_T363_AI_GEAR_H

#include "game/game.h"

struct AIDriver;                            /* T361_drive_params.h (pointers only) */

/* gearSet = 0, gear = -1. */
void ai_clear_gear(struct AIDriver *ai);                                        /* 0x001F6578 */
/* The gear for nextSpeed (state 4: the lower of segSpeed / nextSpeed) when
 * the car is not slower; braking (state 3 / 4) also sets gearSet. */
void ai_select_gear(struct AIDriver *ai);                                       /* 0x001F6588 */
/* ai_brake_factor (its result dropped), then ai_select_gear. */
void ai_update_gear(struct AIDriver *ai);                                       /* 0x001F6648 */

#endif /* GAME_T363_AI_GEAR_H */
