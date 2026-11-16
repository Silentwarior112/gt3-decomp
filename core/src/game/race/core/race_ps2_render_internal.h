/* race_ps2_render_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u045,
 * 0x0017E6C0..0x001817F8.
 *
 *   T131  RacePS2Base, rendering half: the per-frame draw of a race view
 *         (environment map, course, shadows, cars, rear-view mirror, lens
 *         flare, replay blur, colour correction, afterimage) - GameObject's
 *         render and vtable slots 44..55 (declared in T130_race_ps2_base.h).
 *   T132  RaceBasic (the race mode "RaceBasic": a pause menu, a result
 *         sequence and a race display on top of RacePS2Base) and
 *         RaceBasicWithRaceDisplay (a RaceBasic with its RaceDisplay
 *         embedded).
 *
 * Every class and function this unit uses is declared in the canonical
 * headers, which each file includes itself (since u045's data pass, in the
 * order that gives retail's type names: the files' comments).  This header
 * keeps only what is local to the unit: the projection constants, and named
 * constants for the flag bits and codes T131 tests.
 *
 * Offsets in comments are the ORIGINAL ones (egcs / GCC 2.9x ee layout).
 */
#ifndef U045_H
#define U045_H

#include "types.h"

/* ------------------------------------------------------------------------ */
/* Constants                                                                */
/* ------------------------------------------------------------------------ */

/* The projection set-up of every view:
 *   F_00257EF0(PROJ_Y_SCALE) after the camera's projection (0x3F8C8B44);
 *   the sub-rectangle views scale x by h / w * PROJ_ASPECT * zoom
 *   (0x3FB6D98C, close to 640 / 448: the full screen's w / h).
 * Float literals here have 9 significant digits rounded UP, so that both
 * compilers give the retail bits: GCC 14 converts decimal float literals for
 * the R5900 toward zero, egcs to nearest (and egcs C++ has no hex floats); a
 * decimal in [f, f + ulp/2) gives f with both.  The hex spelling is in the
 * comment. */
#define PROJ_Y_SCALE 1.09800005f            /* 0x3F8C8B44 0x1.191688p+0f: 1.098 */
#define PROJ_ASPECT  1.42851401f            /* 0x3FB6D98C 0x1.6db318p+0f: 1.428514 */

/* CourseEnv::flags bits tested here (T178 lists the others). */
enum {
    ENV_WEATHER          = 0x00000020,      /* rain: weather_draw where the camera shows it (CAM_EFFECT_WEATHER) */
    ENV_VIGNETTE         = 0x00000040,      /* replays: race_draw_vignette */
    ENV_COLOUR_CORRECT   = 0x00000080,      /* live race: the colour correction pass (replays: the camera's
                                               levelOverride / colourOverride) */
    ENV_REPLAY_BLUR      = 0x00000100,      /* replays: the depth-of-field blur (ReplayBlur) */
    ENV_LAMPS            = 0x00008000,      /* the cars' lamps into the flare (virtual_54) */
    ENV_REFL_CARS        = 0x00010000,      /* the road reflection draws the cars (virtual_51) */
    ENV_REFL_LAMPS       = 0x00020000,      /* ... and their lamps' reflections (virtual_52) */
    ENV_SUB_NO_ENVMAP    = 0x00100000,      /* a sub view (virtual_47) gets a blank environment map */
    ENV_COURSE_LIGHTS    = 0x00200000,      /* the course's lights into the flare (virtual_55) */
    ENV_SPLIT_NO_LIGHTS  = 0x00400000       /* virtual_47: neither lamps nor course lights */
};

/* CameraManager::virtual_11: the current camera's effect bits (the course
 * camera record's effects, T172 / T397). */
enum {
    CAM_EFFECT_WEATHER   = 0x2000           /* the camera shows the rain (with ENV_WEATHER) */
};

/* The results of PauseBase::virtual_2 RaceBasic::virtual_30 acts on are
 * T168's PAUSE_RESULT_*, the end codes it gives RaceBase::virtual_34
 * (RaceBase::endCode) T129's RACE_END_*; RaceBase::replayMode RACE_LIVE ..
 * (T129). */

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

#endif /* U045_H */
