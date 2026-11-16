/* T531_random.h - Random, a 32-bit pseudo-random generator (translation unit T531, unit u125). */
#ifndef GAME_T531_RANDOM_H
#define GAME_T531_RANDOM_H

#include "game/game.h"

/* Random (? name) - not polymorphic, state = one word.
 * Step: s = s * 17 + 17; value = s ^ rol(s, 16).  Floats put 23 random bits
 * into the mantissa of 1.0f.  Owners keep one as a member (the cameras, the
 * race code; reseeded from the clock by some), ComboutTransitionObject::init
 * (T005) a stack one seeded with 100.  One file-static instance (g_random
 * 0x003532E0 .sbss, gp-0x4710, T531) backs random_int / random_float.
 * T005's lone store of 100 (0x001024D4) needs no Random(u32): egcs -O2 turns
 * `Random r; r.seed = 100;` into exactly that code (the inline ctor's
 * 0x87654321 store is deleted as dead; checked with ee-gcc 2.9-991111b). */
class Random {
public:
    /* Inline: every owner's constructor stores 0x87654321, and T531's static
     * initialiser (random_static_init) does the same for the global one. */
    Random() : seed(0x87654321) {}
    /* (u092 matching: CameraManager::init stores the seed through &rng, not
     * folded into this + 0x961C as a member store would be.) */
    void setSeed(u32 s) { seed = s; }

    u32 next();                                 /* 0x0026C118 */
    f32 next12();                               /* 0x0026C140 [1, 2) */
    f32 next01();                               /* 0x0026C178 [0, 1): next12() - 1 */
    f32 range(f32 lo, f32 hi);                  /* 0x0026C1A0 next01() * (hi - lo) + lo */
    int chance(f32 p);                          /* 0x0026C1E0 next01() < p (unreferenced) */

    u32 seed;                                   /* 0x00 */
};
EGCS_SIZE(Random, 0x4);

extern "C" {
int  random_int(void);                          /* 0x0026C0C0 (int)(next01() * 2^31) of the global */
f32  random_float(void);                        /* 0x0026C0F8 next01() of the global */
/* 0x0026C218: T531's static initialiser (_GLOBAL_$I$, global seed =
 * 0x87654321), compiler-generated from the global's definition. */
}

#endif /* GAME_T531_RANDOM_H */
