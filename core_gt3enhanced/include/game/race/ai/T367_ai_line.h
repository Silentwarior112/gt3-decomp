/* T367_ai_line.h - the computer driver's set-up, its line tables and line
 * changes, and the revving at the start (translation unit T367, unit u086),
 * 0x001F8638..0x001F8BA0.
 *
 * The line changes come from the traffic (T365) and from T341's mode
 * requests (car_mode_driver .. car_mode_stop_brake ask for line 7 / 6 / 3 / 4 through
 * ai_change_line; u079 read them as gear requests). */
#ifndef GAME_T367_AI_LINE_H
#define GAME_T367_AI_LINE_H

#include "game/game.h"

struct AIDriver;                            /* T361_drive_params.h (pointers only) */
struct LineNode;                            /* T368_enemy_lines.h */
class DynamicsConductor;                    /* T305_dynamics_conductor.h */
class RaceCarState;                         /* T302_car_state.h */

/* car, conductor, lines = conductor + 0xBBA8 (car_restore, T351). */
void ai_init(AIDriver *ai, DynamicsConductor *dc, RaceCarState *car);           /* 0x001F8638 */
/* ai_init: car_init's set-up after its memset (T302; a constructor or a
 * function, the code cannot tell). */
void ai_construct(AIDriver *ai, DynamicsConductor *dc, RaceCarState *car);      /* 0x001F8650 */
/* Of lines 6, 2, 1 (g_002DA030) that exist, the one whose point at the car's
 * course distance is nearest the car, under 100 m; else 6. */
int  ai_nearest_line(AIDriver *ai);                                             /* 0x001F8670 */
/* The start: line `line' (-1: the nearest), the corner speeds the top speed,
 * the counters, the zones and the gear cleared (DynCar_initialize: 6, 7 or
 * -1). */
void ai_reset(AIDriver *ai, int line);                                          /* 0x001F8780 */
/* ai->line == 4 (no callers). */
int  ai_on_line4(AIDriver *ai);                                                 /* 0x001F8838 */
/* lines->data->line[line]->count. */
int  ai_corner_count(AIDriver *ai, int line);                                   /* 0x001F8848 */
/* lines->data->line[line]->node. */
LineNode *ai_corners(AIDriver *ai, int line);                                   /* 0x001F8868 */
/* speedStep += delta within -2..2 (0 at the bound), then the corners again
 * (no callers). */
int  ai_step_speed(AIDriver *ai, int delta);                                    /* 0x001F8888 */
/* unk_99 += delta within -2..2 (0 at the bound), then F_001927D8(unk_99),
 * as ai_step_speed (no callers; nothing reads unk_99). */
int  ai_step_unk99(AIDriver *ai, int delta);                                    /* 0x001F88F8 */
/* Switches to `line' at once (no blend). */
void ai_set_line(AIDriver *ai, int line);                                       /* 0x001F8958 */
/* Moves to `line' (or its fallback, lines_select); 0 when there is none.
 * now == 0: blended from the old line (lineBlend 1). */
int  ai_change_line(AIDriver *ai, int line, int now);                           /* 0x001F8980 */
/* The throttle before the start (count: DynamicsConductor::getCountDown, or
 * the run state's pitTimer, car_input_ai): from 13 on, revving between 3/4
 * and 15/16 of the rev limit (CarDynamics.gear.revLimit; faster for the
 * lower ranks); under 13, holding the rev limit. */
void ai_rev_engine(AIDriver *ai, int count);                                    /* 0x001F8A00 */

#endif /* GAME_T367_AI_LINE_H */
