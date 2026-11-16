/* T166 - the particle parameters g_002C03B0 (0xAC bytes, this unit's
 * .data, right before its frame block 0x002C045C) and their static
 * initializer.  0x0019F898..0x0019F9F8.
 *
 * Its own header only (not race_meters_internal.h): retail's T166 has no .rodata and no
 * .sdata, so it read none of the race display headers (their type names,
 * "bad_alloc" from <new>, T157's ""); the "bad_alloc" at 0x0032FEC8 after
 * T165's table is T167's first item.
 */
#include "types.h"
#include "game/effects/T166_particle_params.h"      /* ParticleParams, ParticleColorKey, g_002C03B0 */

/* ? name.  Sets one colour key; inline (the stores go through the key's
 * address, its first field through the object's). */
static inline void color_key_set(ParticleColorKey &key, f32 r, f32 g, f32 b, f32 a, f32 life)
{
    key.r = r;
    key.g = g;
    key.b = b;
    key.a = a;
    key.life = life;
}

// FUNC 0x0019F898 ParticleParams::ParticleParams
//   Caller: the static constructor of g_002C03B0.  The values as written
//   (most a hair under the round number), in field order; egcs schedules
//   the stores.  Colour keys: (1, 0.6, 0.2) from life 0.8, (1, 0.5, 0) from
//   0.4, (0.8, 0, 0) from 0.15, (0.2, 0, 0) from 0; alpha 1.  The spark
//   fields (+0x98..+0xA8) are read by RaceCarModel (T180).
ParticleParams::ParticleParams()
{
    drag = 0.349999f;
    gravity = 9.81f;
    bounce = 0.499998f;
    friction = 0.499998f;
    spread = 10.0f;
    speedMin = 0.299998f;
    speedMax = 0.699998f;
    upJitter = 0.199999f;
    sizeMin = 0.009998f;
    sizeMax = 0.029998f;
    density = 500.0f;
    lifeMin = 0.599999f;
    lifeMax = 1.999997f;
    bounceLife = 0.599999f;
    liftAgeMin = 0.089998f;
    liftAgeMax = 0.109999f;
    liftChance = 0.899999f;
    lift = 0.024999f;
    sparkSpeed = 100.0f;
    sparkMin = 5;
    sparkMax = 20;
    sparkPerDepth = 200.0f;
    sparkAlways = 0;
    color_key_set(keys[0], 1.0f, 0.599999f, 0.199999f, 1.0f, 0.8f);
    color_key_set(keys[1], 0.999998f, 0.499998f, 0.0f, 1.0f, 0.399998f);
    color_key_set(keys[2], 0.799998f, 0.0f, 0.0f, 1.0f, 0.149998f);
    color_key_set(keys[3], 0.199999f, 0.0f, 0.0f, 1.0f, 0.0f);
}

/* The particle parameters, default-constructed (.data 0x002C03B0, 0xAC
 * bytes right before the unit's frame block: egcs puts a public object
 * that has a constructor in .data, zero-filled). */
ParticleParams g_002C03B0;                  /* 0x002C03B0 */

/* Compiler-generated: egcs's static constructor of g_002C03B0 (.ctors entry
 * 0x0028F138), named after the file's first global function and emitted at
 * the end of the unit: ParticleParams() on the object. */
// FUNC 0x0019F9D8 T166_static_init (_GLOBAL_$I$__14ParticleParams)
/* implicit: ParticleParams (the static constructor of g_002C03B0) */
