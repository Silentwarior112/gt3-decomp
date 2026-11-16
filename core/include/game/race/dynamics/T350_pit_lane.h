/* T350_pit_lane.h - the pit lane of a car (translation unit T350, unit u083),
 * 0x001ED9F0..0x001EE288.
 *
 * The pit lane state of a car is DynCarState.ctrlMode (u083 pitState: 0
 * the driver, 1 the computer, 2 in the pit lane towards the box, 3 at the
 * box - the stop runs DynCarState.pitTimer frames (u083, 300) - 4 leaving),
 * with DynCarState.flags (u083 pitFlags: 1 going backwards, 4 a worn tyre
 * wants a stop (T296), 8 (?) the computer takes the car in, 0x10 (?) a wheel
 * on the counted ground, the pit lane's).  The positions come from the
 * course's pit gates
 * (DynamicsConductor::pitGateDistance: 1 the pit entry, -1 / -2 / -3 the exit
 * side, 3, 0) and boxes (pitBoxDistance).  A position below half a lap counts
 * one course length further, so that the pit lane is continuous across the
 * line. */
#ifndef GAME_T350_PIT_LANE_H
#define GAME_T350_PIT_LANE_H

#include "game/game.h"

class DynamicsConductor;                    /* T305_dynamics_conductor.h (pointers only) */
class RaceCarState;                         /* T302_car_state.h */

/* RaceDisplayInformationEvents of the car (RaceDisplay::pollInformationEvents,
 * T154): 1 at the pit entry, shown as "<maker> has pitted in"; 6 at the pit
 * exit, the start display and the start sound for the player's car. */
void dc_post_pit_in(DynamicsConductor *dc, int car);           /* 0x001ED9F0 */
void dc_post_pit_out(DynamicsConductor *dc, int car);          /* 0x001EDA40 */
/* 1 while the car is in the pit lane (state 2..4 and between the entry gate
 * 1 and the exit gate -1).  Caller: DesignerCamera (T398). */
int  car_in_pit_lane(RaceCarState *car);                       /* 0x001EDA90 */
/* The pit lane state machine after the car moved from prev to pos (the
 * course positions of this and the last frame; DynamicsConductor::updateCar
 * and T341).  The car wants service when a tyre is worn (its wear differs
 * from the wear curve's x0), a wheel is damaged or slot 41 asks for a stop. */
void car_update_pit(RaceCarState *car, f32 pos, f32 prev);     /* 0x001EDB40 */

#endif /* GAME_T350_PIT_LANE_H */
