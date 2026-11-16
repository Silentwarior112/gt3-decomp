/* T365_ai_traffic.h - the cars around the computer driver (translation unit
 * T365, unit u086), 0x001F6B48..0x001F7078.
 *
 * Every frame dc_car_pairs (T349) clears the zones (ai_clear_traffic), then
 * reports each car within 20 m (ai_note_car) and the closing speed to the car
 * ahead (ai_note_closing); ai_drive (T366) acts on them (ai_avoid_traffic).
 * The eight zones are AIDriver.traffic[] (T361): 0 ahead, 1 ahead -side, 2
 * ahead +side, 3 beside -, 4 beside +, 5 behind -side, 6 behind +side, 7
 * behind; 2 a car in the zone, 1 a car leaning to that side. */
#ifndef GAME_T365_AI_TRAFFIC_H
#define GAME_T365_AI_TRAFFIC_H

#include "game/game.h"

struct AIDriver;                            /* T361_drive_params.h (pointers only) */

/* 50 m: the distance before the braking point that counts as near it. */
f32  ai_brake_margin(struct AIDriver *ai);                                      /* 0x001F6B48 */
/* caution = 0, the zones cleared. */
void ai_clear_traffic(struct AIDriver *ai);                                     /* 0x001F6B58 */
/* A car at dist (ignored beyond 20 m), `along' ahead (+) / behind (-),
 * `side' across: ahead beyond `front', behind beyond `rear', else beside;
 * a side zone beyond +-`width'. */
void ai_note_car(struct AIDriver *ai, f32 dist, f32 along, f32 side,
                 f32 front, f32 rear, f32 width);                               /* 0x001F6B90 */
/* The caution for a car `gap' m ahead closing at `closing' m/s: 1 above
 * 10 m/s, 0.5 under 3 s to contact, 0.25 under 2 m; the largest is kept. */
void ai_note_closing(struct AIDriver *ai, f32 gap, f32 closing);                /* 0x001F6CC0 */
/* While driving forward: a free side line (T369) when a car closes in, then
 * 30 frames' wait; else the throttle lifted by the caution, back to line 6
 * with nobody around, or away from a car beside.  Line 3 only slows behind
 * cars; line 4 does nothing. */
void ai_avoid_traffic(struct AIDriver *ai);                                     /* 0x001F6D70 */

#endif /* GAME_T365_AI_TRAFFIC_H */
