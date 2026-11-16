/* T183_race_vignette.h - race_draw_vignette (? name), translation unit T183,
 * unit u061, 0x001AD818..0x001ADA68.
 *
 * A ring of 32 segments (a strip of 64 vertices) between the two radii of
 * the course environment's vignette, its first colour at the first radius
 * and its second at the second, drawn over the screen with an orthographic
 * -320..320 x -224..224 view (CourseEnv vignetteRadius0 .. vignetteColor1,
 * +0x950..+0x977, T178).  Caller: RacePS2Base::drawView (replays, course
 * flag 0x40). */
#ifndef GAME_T183_RACE_VIGNETTE_H
#define GAME_T183_RACE_VIGNETTE_H

#include "game/game.h"

class GSBuffer;                             /* T514_gs_buffer.h (pointers only) */
class CourseEnv;                            /* T178_course_env.h */

/* buf becomes the draw buffer; the ring is drawn into it. */
void race_draw_vignette(GSBuffer *buf, CourseEnv *env);    /* 0x001AD818 */

#endif /* GAME_T183_RACE_VIGNETTE_H */
