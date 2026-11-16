/* T531_random.cpp - a small 32-bit pseudo-random generator (? class name
 * Random: the binary has no RTTI for it).  Class: include/engine/pdistd/T531_random.h.
 *
 * State = one word: s = s * 17 + 17, value = s ^ rol(s, 16).  Floats are made
 * by putting 23 random bits into the mantissa of 1.0f ([1, 2)) and
 * subtracting 1.  Users keep their own generator (the cameras, reseeded from
 * the clock; ComboutTransitionObject::init uses a stack one seeded with 100);
 * one file-static instance, g_random at 0x003532E0 (gp-0x4710), is seeded
 * 0x87654321 by this file's static initialiser and backs random_int /
 * random_float.
 */
#include "engine/pdistd/pdistd_internal.h"
#include "engine/pdistd/T531_random.h"

/* The global generator: an object with a constructor, built by the
 * compiler's _GLOBAL_$I$ function (random_static_init below).  A file
 * static: retail has it in .sbss (0x003532E0, after the .sdata), where egcs
 * puts a static object (-G8); a global one would be zero-filled .sdata. */
static Random g_random;                         /* 0x003532E0 */

// FUNC 0x0026C0C0 random_int
// A random int in [0, 2^31): (int)(next01() * 2147483648.0f) of the global
// generator (cvt.w.s truncates).  Callers: 0x00211A68, 0x0021ABB0.
extern "C" int random_int(void)
{
    return (int)(g_random.next01() * 2147483648.0f);
}

// FUNC 0x0026C0F8 random_float
// A random float in [0, 1) from the global generator.  Caller: 0x0013BA80.
extern "C" f32 random_float(void)
{
    return g_random.next01();
}

// FUNC 0x0026C118 Random::next
// Steps the generator: s = s * 17 + 17 (sll 4 + addu + 0x11), stores it and
// returns s ^ rol(s, 16).  23 callers outside (cameras, race code).
u32 Random::next()
{
    u32 s = seed * 17 + 0x11;

    seed = s;
    return s ^ ((s << 16) | (s >> 16));
}

// FUNC 0x0026C140 Random::next12
// A random float in [1, 2): the low 23 bits of next() as the mantissa of 1.0f.
f32 Random::next12()
{
    union {
        u32 u;
        f32 f;
    } v;

    v.u = (next() & 0x007FFFFF) | 0x3F800000;
    return v.f;
}

// FUNC 0x0026C178 Random::next01
// A random float in [0, 1): next12() - 1.0f.  27 callers.
f32 Random::next01()
{
    return next12() - 1.0f;
}

// FUNC 0x0026C1A0 Random::range
// A random float in [lo, hi): next01() * (hi - lo) + lo (the difference is
// computed after the call).  Caller: ComboutTransitionObject::init.
f32 Random::range(f32 lo, f32 hi)
{
    f32 r = next01();

    return r * (hi - lo) + lo;
}

// FUNC 0x0026C1E0 Random::chance
// 1 if next01() < p (c.lt.s), else 0.  Unreferenced.
int Random::chance(f32 p)
{
    return next01() < p ? 1 : 0;
}

/* Compiler-generated: egcs's static initialiser of g_random (named after the
 * unit's first public object, random_int), emitted at the end of the unit. */
// FUNC 0x0026C218 random_static_init (_GLOBAL_$I$random_int)
//   (.ctors entry 0x0028F1D0) the inline Random() : seed(0x87654321) on
//   g_random - a single store to gp-0x4710.
/* implicit: Random */
