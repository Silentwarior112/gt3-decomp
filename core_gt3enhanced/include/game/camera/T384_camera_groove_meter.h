/* T384_camera_groove_meter.h - CameraGrooveMeter (translation unit T384, unit
 * u092), 0x0020A2E0..0x0020A9C0.
 *
 * Eight arcs round the screen centre (320, 224) with random radii, start
 * angles, sweeps and spins (CameraManager +0x963C, T383).  start() runs them
 * for 180 frames, during which draw() shrinks them towards their radii and
 * fades them out.  Plus the arc drawing helpers (F_0020A360 draws with u117's
 * gs_draw_line_f; its only caller is draw).  Unused in the game: update and
 * draw have no callers, and start's only caller is reset (count 0 again).
 */
#ifndef GAME_T384_CAMERA_GROOVE_METER_H
#define GAME_T384_CAMERA_GROOVE_METER_H

#include "game/game.h"
#include "engine/pdistd/T531_random.h"               /* Random (held by value) */

/* One arc.  Not polymorphic.  Size 0x10 (the rings[] stride). */
class GrooveRing {
public:
    f32 radius;                             /* 0x00 50 + 10 i + 0..2 */
    f32 angle;                              /* 0x04 the start angle, degrees */
    f32 sweep;                              /* 0x08 80..280 degrees */
    f32 speed;                              /* 0x0C +-1..3: the share of the meter's angle it turns by */
};
EGCS_SIZE(GrooveRing, 0x10);

/* vtable 0x0033F060 (1 slot).  typeinfo __tf17CameraGrooveMeter 0x0020A980
 * (__rtti_user: no bases; __user_type_info 0x00353430, gp-0x45C0).  The data,
 * then the vptr (+0x98, egcs).  Size 0x9C (CameraManager +0x963C, the next
 * member at +0x96D8). */
class CameraGrooveMeter {
public:
    CameraGrooveMeter();                    /* 0x0020A500 reset() */
    virtual ~CameraGrooveMeter();           /* [0] +0x0C 0x0020A540 */
    void reset();                           /* 0x0020A570 reseeds rng from the clock, start(0, 0), count 0 */
    /* a, b clamped to 0..1; 180 frames; spin = b * 5 + 0.2; new random rings. */
    void start(f32 a, f32 b);               /* 0x0020A5B8 */
    void update();                          /* 0x0020A798 count - 1, angle + spin (wrapped to 0..360) */
    /* While running: the radius scale 2 (count / 180)^8 + 1, alpha count / 90
     * (at most 1), the colour from levelB. */
    void draw();                            /* 0x0020A7E0 */

    int        count;                       /* 0x00 frames left (180 on start) */
    f32        angle;                       /* 0x04 degrees */
    f32        spin;                        /* 0x08 degrees per frame: b * 5 + 0.2 */
    f32        levelA;                      /* 0x0C start()'s a, 0..1 (? name; not read here) */
    f32        levelB;                      /* 0x10 start()'s b, 0..1: the colour (? name) */
    Random     rng;                         /* 0x14 */
    GrooveRing rings[8];                    /* 0x18 */
    /*  vptr                                   0x98 */
};
EGCS_SIZE(CameraGrooveMeter, 0x9C);

f32  F_0020A2E0(f32 cx, f32 r, f32 a);      /* 0x0020A2E0 cx + r cos a */
f32  F_0020A320(f32 cy, f32 r, f32 a);      /* 0x0020A320 cy - r sin a */
/* An arc of radius r round (cx, cy) from startDeg over sweepDeg (and one step
 * more) in lines: the step count doubles from 16 until r tan(2 pi / n) < 8. */
void F_0020A360(f32 cx, f32 cy, f32 r, f32 startDeg, f32 sweepDeg);  /* 0x0020A360 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself (and rejects hand declarations) */
extern "C" {
void *__tf17CameraGrooveMeter(void);        /* 0x0020A980 */
extern u32 __ti17CameraGrooveMeter[2];      /* 0x00353430 gp-0x45C0 its __user_type_info (only T384 uses it) */
}
#endif

#endif /* GAME_T384_CAMERA_GROOVE_METER_H */
