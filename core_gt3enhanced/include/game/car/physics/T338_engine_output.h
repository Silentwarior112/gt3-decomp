/* T338_engine_output.h - the engine output per frame (translation unit
 * T338, unit u078), 0x001E1768..0x001E1B28.
 *
 * engine_output (0x001E1768, inside T338; in GT3 only engine_update calls
 * it, the enhanced engine's Dualnote too): the rev limiter (cuts at the redline, back 500 rpm
 * below), then the torque through the power limit at the engine speed.
 * engine_update: the throttle with the turbo spool (DynCarState turboSpool /
 * turboSpoolMax, turboThrottle), the blow-off (blowOffState / blowOffLevel), the
 * turbo's engine sound level (engineSnd[2]) and engine_output; returns the
 * engine torque.  Called by car_drive_torque (T344) and car_engine_speed
 * (T296). */
#ifndef GAME_T338_ENGINE_OUTPUT_H
#define GAME_T338_ENGINE_OUTPUT_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */

f32 engine_update(RaceCarState *car, f32 throttle);    /* 0x001E17F0 */
/* The enhanced engine: the Dualnote's front motors read the engine's net
 * output too (Lupo 2 L2 0x001E0900; hybrid_front_motors, T344). */
f32 engine_output(RaceCarState *car, f32 load);        /* 0x001E1768 */

#endif /* GAME_T338_ENGINE_OUTPUT_H */
