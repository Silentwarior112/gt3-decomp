/* T182_replay_blur.h - ReplayBlur, the depth of field of the replays
 * (translation unit T182, unit u061), 0x001AC870..0x001AD818.
 *
 * A thin-lens camera (film diagonal 35 mm, focal length from the projection
 * matrix, f-number, focus distance) gives, for each circle of confusion of
 * 2^(k / steps) pixels, the distance in front of / behind the focus where
 * the image is that blurred.  The frame is downsampled into a chain of 5
 * levels (320x224, 320x112, 160x112, 160x56, 160x56) packed in one 640x224
 * buffer, and each level is drawn back over the whole screen, depth-tested
 * against its distance, with the alpha of the fraction of the octave.
 * RacePS2Base (T130, +0x97CC) holds it; drawView sets it from the course
 * environment (CourseEnv +0x930.., T178) and draws it in replays (course
 * flag 0x100).  ? name (u045's). */
#ifndef GAME_T182_REPLAY_BLUR_H
#define GAME_T182_REPLAY_BLUR_H

#include "game/game.h"
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer (held by value) */

/* Not polymorphic.  Size 0x64 (RacePS2Base +0x97CC .. +0x9830). */
class ReplayBlur {
public:
    /* levels constructed; the film 35 mm at 1.333, f-number 2, focal
     * length 0.035, focus 10, farMax 8, nearMax 4, steps 2 / 1, zFar 0.1,
     * zNear 1000. */
    ReplayBlur();                           /* 0x001AC870 */
    /* The film's diagonal (mm) and aspect: its height in metres. */
    void setFilm(f32 diagMm, f32 aspect);   /* 0x001AC948 */
    /* The clip distances and the focal length from the projection matrix. */
    void setFromProjection();               /* 0x001AC970 */
    /* Both sides: the far one with float depths, then (the depth buffer
     * inverted by invertDepth) the near one with GS Z values 0xFFFFFF - Z;
     * the levels go into work (drawFrameDepth's). */
    void drawZ(GSBuffer *dst, GSBuffer *src, GSBuffer *depth, GSBuffer *work);  /* 0x001ACA00 */
    /* No callers: the depth from the current frame copy, src over dst. */
    void drawFrameDepth(GSBuffer *dst, GSBuffer *src, int mode, f32 alpha);     /* 0x001ACE00 */
    /* The one in use: the near side from the most blurred level down, each
     * with the depth of the level drawn before, then src over dst (mode,
     * alpha), then the far side. */
    void draw(GSBuffer *dst, GSBuffer *src, GSBuffer *depth, GSBuffer *work,
              int mode, f32 alpha);         /* 0x001ACF38 */
    /* The current frame copy drawn 640 x 448 into dst. */
    void copyFrameTo(GSBuffer *dst);        /* 0x001AD2F0 */
    /* Inverts the depth buffer, Z' = 0xFFFFFF - Z: seen as PSMCT24, a white
     * 640 x 448 rectangle with blend (Cs - Cd) * FIX (0xA4, FIX 1.0); psm
     * back to 0x31 (u061 pass 2: an invertDepth). */
    void invertDepth(GSBuffer *depth);      /* 0x001AD3B0 Z' = 0xFFFFFF - Z (u061 invertDepth) */
    /* The chain: src into level 0, each level from the one before. */
    void buildLevels(GSBuffer *src);        /* 0x001AD440 */
    /* Level `level' stretched over the screen (10 strips of 64 x 448). */
    void drawLevel(int level, f32 alpha);   /* 0x001AD650 */

    f32      filmDiag;                      /* 0x00 mm (35) */
    f32      filmHeight;                    /* 0x04 m */
    f32      fNumber;                       /* 0x08 (2; u045 f08: CourseEnv's) */
    f32      focalLength;                   /* 0x0C m (0.035, then from the projection) */
    f32      focus;                         /* 0x10 the focus distance (10; u045 f10) */
    /* the clip distances: proj m[3][2] / (m[2][2] + 1) and m[3][2] /
     * (m[2][2] - 1) (setFromProjection; a GL-style projection gives the
     * far and the near clip), 0.1 and 1000 before */
    f32      zFar;                          /* 0x14 */
    f32      zNear;                         /* 0x18 */
    f32      farMax;                        /* 0x1C the largest blur behind the focus, pixels (8) */
    f32      nearMax;                       /* 0x20 the largest blur in front of it (4) */
    int      farSteps;                      /* 0x24 levels per doubling behind (2) */
    int      nearSteps;                     /* 0x28 levels per doubling in front (1) */
    GSBuffer levels;                        /* 0x2C the downsampled chain (640 x 224) */
};
EGCS_SIZE(ReplayBlur, 0x64);

#endif /* GAME_T182_REPLAY_BLUR_H */
