/* T393_target_chooser.h - TargetChooser (translation unit T393, unit u094),
 * 0x0020E020..0x0020E930.
 *
 * ? name.  The automatic choice of the car a camera view follows in view
 * mode 2 (CameraView +0x274, T383; u092 CameraSwitcher, u093
 * CameraTargetPlanner).  `target' is the view's camera target (CameraViewKind
 * variant, T383): 0 the view's own car (score 1200), 1 the leader, otherwise
 * automatic: every car has a score 0..1200 from its place (g_002DF108) plus
 * random gains, place changes and a bonus for the state of its
 * CameraTargetSelector slot (T392); F_0020E1F0 picks the best car now and
 * then (after 240 frames the chance grows to 1 over 1560 frames, tested every
 * 30 frames; only cars drawn in full, car_display_level == 3), F_0020E790 takes it
 * when the camera switches (CameraManager::updateMode2, T389).
 */
#ifndef GAME_T393_TARGET_CHOOSER_H
#define GAME_T393_TARGET_CHOOSER_H

#include "game/game.h"
#include "engine/pdistd/T531_random.h"               /* Random (held by value) */

class CameraTargetInfo;                     /* T377 (pointers only) */
class CameraTargetSelector;                 /* T392 */
class RaceOrganization;                     /* T125 */

/* One car's score.  Not polymorphic.  Size 0xC (the car[] stride). */
class TargetScore {
public:
    f32 score;                              /* 0x0 0..1200 */
    f32 bonus;                              /* 0x4 the slot-state bonus (scaled by the place) */
    int hold;                               /* 0x8 frames the place changes are ignored (only counted down) */
};
EGCS_SIZE(TargetScore, 0xC);

/* Not polymorphic.  Size 0x60 (CameraView +0x274, the next member at
 * +0x2D4).  rng: CameraView's constructor stores 0x87654321 at +0x2C8,
 * Random's inline constructor; F_0020E020 reseeds it from the clock.
 *
 * pending is a byte-wide bool bit-field (u094 matching pass): its byte
 * accesses are not char accesses, so F_0020E790 loads and stores the score
 * around `pending = 0' as retail does (a u8 orders them, 77.4%), while
 * F_0020E1F0's test of it stays after the score loops (an `unsigned : 8'
 * field is hoisted above them).  The layout is the same under both
 * compilers (bool is 1 byte in GCC 14; the field takes the byte 0x5E). */
class TargetChooser {
public:
    TargetScore car[7];                     /* 0x00 */
    Random      rng;                        /* 0x54 */
    int         frames;                     /* 0x58 frames since the last switch */
    s8          next;                       /* 0x5C the car chosen */
    s8          shown;                      /* 0x5D the car shown */
    bool        pending : 8;                /* 0x5E 1: next changed and was not taken yet */
    u8          unk_5F;                     /* 0x5F */
};
EGCS_SIZE(TargetChooser, 0x60);

/* T393 .data 0x002DF108 (defined in T393; retail's is in .data, so not
 * const): per 0-based place (5 = 6th and below) {400, 1/60, 1.5},
 * {300, 0.6/60, 0.95}, {200, 0.4/60, 0.85}, {100, 0.34/60, 0.75},
 * {100, 0.28/60, 0.7}, {100, 0.2/60, 0.65}. */
class RankWeight {
public:
    f32 score;                              /* 0x0 the score a car starts / restarts with */
    f32 chance;                             /* 0x4 the chance per frame of +10 */
    f32 scale;                              /* 0x8 the scale of the slot-state bonus */
};
EGCS_SIZE(RankWeight, 0xC);
extern RankWeight g_002DF108[6];            /* 0x002DF108 (only T393 uses it) */

/* The owner's free functions (u093 declared F_0020E790 as a method).
 * sel is the view's CameraTargetSelector (T392): its slots' states and rank
 * tables (u094: RaceEvents). */
/* CameraManager::init / F_0020EBA0 (T394): restarts the scores; car (the car
 * the view follows) is next and shown. */
void F_0020E020(TargetChooser *tc, RaceOrganization *org, int car, int target);   /* 0x0020E020 */
/* CameraManager::update, once a frame per view: ti is the view's target info
 * (index[0] = the car followed). */
void F_0020E1F0(TargetChooser *tc, RaceOrganization *org, CameraTargetInfo *ti,
                CameraTargetSelector *sel, int target);                          /* 0x0020E1F0 */
/* updateMode2 (T389), when the view switches to next: next +200, the others
 * scaled by next's slot state, the car left restarts from its place's score. */
void F_0020E790(TargetChooser *tc, RaceOrganization *org, CameraTargetSelector *sel); /* 0x0020E790 */

#endif /* GAME_T393_TARGET_CHOOSER_H */
