/* T414_race_random.h - the race random generator (translation unit T414,
 * unit u095), 0x00211A00..0x00211B58.  (? names: no symbols.)
 *
 * A generator of T531 (F_0026C118 steps it, F_0026C178 gives a float in
 * [0, 1)) at gp-0x47A0, seeded from the COP0 Random register when the static
 * constructor builds it.  The race code skips / draws values from it;
 * GameStatus (T429) saves its seed with the game and restores it on load. */
#ifndef GAME_T414_RACE_RANDOM_H
#define GAME_T414_RACE_RANDOM_H

#include "game/game.h"
#include "engine/pdistd/T531_random.h"                   /* base Random */

/* The state of g_00353250 (? name).  No vtable.  Size 8 (gp-0x47A0, the next
 * .sbss word gp-0x4798 belongs to u102).
 * Its first word is a T531 Random: the constructor stores Random's
 * 0x87654321, and every step is Random::next / next01 on the object itself
 * (F_0026C118 / F_0026C178 with $a0 = &g_00353250).  A base or a first
 * member compile to the same code (? which); as a base, `seed' stays the
 * word's name. */
class RaceRandom : public Random {
public:
    RaceRandom();                               /* 0x00211A00 Random() (seed 0x87654321), initialSeed = 0, then randomize() */
    void randomize();                           /* 0x00211A38 both words = COP0 Random, then one step */

    /*  seed (Random)                              0x0 the generator state (T531) */
    u32 initialSeed;                            /* 0x4 the COP0 Random value it was seeded with */
};
EGCS_SIZE(RaceRandom, 8);

/* The generator itself, g_00353250 (.sbss 0x00353250, gp-0x47A0), is a file
 * static of T414 (u095's data pass: retail has it in .sbss, local); the
 * other units reach it through the functions below. */

void raceRandomShuffle(void);                   /* 0x00211A68 skips 1..12 values ((Random + rand()) % 13) */
void raceRandomSkip(int n);                     /* 0x00211AC0 */
f32  raceRandomFloat(int n);                    /* 0x00211B00 the n-th next value, in [0, 1) */
u32  raceRandomGetSeed(void);                   /* 0x00211B28 */
void raceRandomSetSeed(u32 seed);               /* 0x00211B30 */

/* The static constructor of g_00353250 (.ctors entry 0x0028F16C, 0x00211B38)
 * is egcs's _GLOBAL_$I$__10RaceRandom: T414 defines the object (u095
 * matching). */

#endif /* GAME_T414_RACE_RANDOM_H */
