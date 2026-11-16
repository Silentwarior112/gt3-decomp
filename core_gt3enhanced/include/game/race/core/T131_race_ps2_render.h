/* T131_race_ps2_render.h - the rendering half of RacePS2Base (translation
 * unit T131, unit u045), 0x0017E6C0..0x00180EB8: Matrix44 and the helpers;
 * the CarState / CarDrawParam names of u045 / u061.
 *
 * T131 defines RacePS2Base's render (GameObject slot 2) and slots 44..55 -
 * the environment map, the course, the cars, the lens flare, the rear-view
 * mirror, the reflections on the road (drawReflections: the cars mirrored in
 * the ground, u045's first pass called it the shadows), the replay blur and
 * the afterimage effect.  They are declared with the class in
 * T130_race_ps2_base.h.
 *
 * (The plan put the __si_type_info storage of RaceBasic and
 * RaceBasicWithRaceDisplay here; it is declared in T132_race_basic.h.)
 */
#ifndef GAME_T131_RACE_PS2_RENDER_H
#define GAME_T131_RACE_PS2_RENDER_H

#include "game/game.h"

class GSBuffer;                             /* T514 (pointers only: forward declarations) */
class RaceCarState;                         /* T302 */
class CarModel;                             /* T446 */

/* A VU0 matrix as the G001 matrix functions store and load it (F_00257C34 /
 * F_00257E30: sqc2 when 16-byte aligned, sdl / sdr otherwise).  Not
 * polymorphic.  Size 0x40 (u045, u055, u057..u059, u061). */
struct Matrix44 {
    f32 m[4][4];                            /* 0x00 */
};
EGCS_SIZE(Matrix44, 0x40);

/* u045 / u061's CarState: RaceEntryCar +0x18, the car of the
 * DynamicsConductor (their 0x136E and 0x284 are inside its DynCarState and
 * CarDynamics members). */
typedef RaceCarState CarState;

/* u045 / u061's CarDrawParam: what F_00179460 returns, the CarModel of a
 * RaceCarModel (+4).  Their mode (+8) is CarModel::callback.lod and scale
 * (+0xC) callback.lodDist (T446; RaceBase::virtual_8 sets 200). */
typedef CarModel CarDrawParam;

/* Whether the 8 m box around the car's interpolated position (t: the
 * interpolation argument of the RaceCarState accessors) is in view
 * (F_0024BF80).  Callers: RacePS2Base::drawCars, u114. */
int  race_car_in_view(CarState *cs, f32 t);     /* 0x0017F688 */
/* Clears the alpha channel of the frame to rgba's alpha (write mask
 * 0xFF000000: gfx_set_frame_write_mask takes the bits written), then makes
 * every channel writable again.  (u045 F_001807B0.) */
void clear_frame_alpha(u32 rgba);           /* 0x001807B0 */
/* Before the course: the depth cleared, then buf (the buffer being drawn)
 * as a full-size texture sprite with no colour written, depth test ALWAYS
 * and alpha test NOTEQUAL 0: only the depth of its pixels with alpha.
 * Leaves the depth test LEQUAL and the write mask 0x00FFFFFF (colour, not
 * alpha).  (u045 F_001807F8.) */
void write_alpha_depth(GSBuffer *buf);      /* 0x001807F8 */
void *F_00180EB0(void *p);                  /* 0x00180EB0 returns p (an inline function emitted here) */

#endif /* GAME_T131_RACE_PS2_RENDER_H */
