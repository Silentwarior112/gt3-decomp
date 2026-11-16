/* T332_surface_params.h - the 11 surface parameter records of the course
 * (translation unit T332, unit u077), 0x001DE070..0x001DE158.
 *
 * CourseEnv_initSurfaceSmoke (DynamicsConductor::virtual_20, with the conductor's
 * RaceCourse +0xB7A8) fills the course's smoke types 0..10, one per surface:
 * course->data->env->smoke[i] (SmokeType, CourseEnv +0x4D0 + i x 0x44, T178)
 * from the 8 bytes per surface of g_002D3510: density at least 0.01 (else
 * 1.0), size [0] / 100, grow [1] / 10, rise [2] / 100, spin [3], fade [4] /
 * 100, gravity [5] / 100, alphaScale [6] / 100, period (int) [7].  u077's
 * SurfaceParam is SmokeType seen from its +0x0C (density): its +0x00 /
 * +0x04 / +0x08 / +0x0C / +0x10 / +0x14 / +0x20 / +0x24 / +0x34 are those
 * members (h28); it is not declared. */
#ifndef GAME_T332_SURFACE_PARAMS_H
#define GAME_T332_SURFACE_PARAMS_H

#include "game/game.h"

class RaceCourse;                           /* T153_race_course.h (pointers only; u077 CourseRef) */

/* T332's .data (up to its .eh_frame at 0x002D3590): one record per surface
 * type, 16 as the course's smoke types (CourseEnv smoke[16], T178) and the
 * per-surface tables of T328; 0..5 set, 6..15 zero, 0..10 read. */
extern u8 g_002D3510[16][8];                /* 0x002D3510 */

void CourseEnv_initSurfaceSmoke(RaceCourse *course);  /* 0x001DE070 */

#endif /* GAME_T332_SURFACE_PARAMS_H */
