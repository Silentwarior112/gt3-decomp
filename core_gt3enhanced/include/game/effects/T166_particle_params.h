/* T166_particle_params.h - the particle parameters g_002C03B0 (translation
 * unit T166, unit u056), 0x0019F898..0x0019F9F8.
 *
 * g_002C03B0 is T166's .data (0xAC bytes, right before its frame block
 * 0x002C045C), built by T166's static constructor.  The
 * particles of T165 read the motion, life and colour fields; the car
 * effects of T180 (u060) the spark fields (+0x98..+0xA8).  Most values are
 * a hair under the round number (as stored).
 */
#ifndef GAME_T166_PARTICLE_PARAMS_H
#define GAME_T166_PARTICLE_PARAMS_H

#include "game/game.h"

/* A colour of the particles by remaining life (Particle::getColor blends
 * the two keys around it).  0x14 bytes. */
struct ParticleColorKey {
    f32 r;                                  /* 0x00 1.0 = 0x80 */
    f32 g;                                  /* 0x04 */
    f32 b;                                  /* 0x08 */
    f32 a;                                  /* 0x0C */
    f32 life;                               /* 0x10 the key applies from this life on */
};
EGCS_SIZE(ParticleColorKey, 0x14);

/* ? names (u056's / u060's).  Not polymorphic, 0xAC bytes (u056 / u060:
 * struct). */
class ParticleParams {
public:
    /* The values (drag 0.35, gravity 9.81, spread 10, density 500, ...);
     * the colour keys (1, 0.6, 0.2) from life 0.8, (1, 0.5, 0) from 0.4,
     * (0.8, 0, 0) from 0.15, (0.2, 0, 0) from 0, alpha 1. */
    ParticleParams();                       /* 0x0019F898 */

    f32 drag;                               /* 0x00 */
    f32 gravity;                            /* 0x04 */
    f32 bounce;                             /* 0x08 restitution on the plane */
    f32 friction;                           /* 0x0C */
    f32 spread;                             /* 0x10 degrees (emitSparks) */
    f32 speedMin;                           /* 0x14 */
    f32 speedMax;                           /* 0x18 */
    f32 upJitter;                           /* 0x1C */
    f32 sizeMin;                            /* 0x20 */
    f32 sizeMax;                            /* 0x24 */
    f32 density;                            /* 0x28 mass = size^3 * density */
    f32 lifeMin;                            /* 0x2C */
    f32 lifeMax;                            /* 0x30 */
    f32 bounceLife;                         /* 0x34 life *= this on a bounce */
    ParticleColorKey keys[4];               /* 0x38 by decreasing life */
    f32 liftAgeMin;                         /* 0x88 */
    f32 liftAgeMax;                         /* 0x8C */
    f32 liftChance;                         /* 0x90 */
    f32 lift;                               /* 0x94 */
    f32 sparkSpeed;                         /* 0x98 100: km/h, no sparks below it (T180; u056 unk98) */
    int sparkMin;                           /* 0x9C 5: the spark count clamp (u056 unk9C) */
    int sparkMax;                           /* 0xA0 20 (u056 unkA0) */
    f32 sparkPerDepth;                      /* 0xA4 200: sparks per unit of depth under the road (u056 unkA4) */
    int sparkAlways;                        /* 0xA8 0; != 0: also above the road (u056 unkA8) */
};
EGCS_SIZE(ParticleParams, 0xAC);

extern ParticleParams g_002C03B0;           /* 0x002C03B0 (u056, u060) */

/* Its static constructor (.ctors entry 0x0028F138, 0x0019F9D8) is egcs's
 * own _GLOBAL_$I$__14ParticleParams: T166 defines g_002C03B0 (u056 matching
 * pass; the first pass wrote it out as T166_static_init). */

#endif /* GAME_T166_PARTICLE_PARAMS_H */
