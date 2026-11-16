/* T165_particle_manager.h - ParticleManager, the sparks and wheel debris of
 * a race (translation unit T165, unit u056), 0x0019E6F0..0x0019F898.
 *
 * 128 particles (pool) on an active and a free list.  A particle moves with
 * drag, gravity and an occasional lift early in its life (the parameters
 * g_002C03B0, T166), bounces off its ground plane (a bounce shortens its
 * life) and dies when its life runs out.  It is drawn as a streak from last
 * frame's screen position to this one's, coloured by its remaining life
 * (g_002C03B0.keys); the colour given at birth is stored, never read.
 * RacePS2Base (T130) holds the manager (built by its constructor
 * 0x0017D070); RaceCourse (T153) keeps a pointer to it (emitDebris).
 */
#ifndef GAME_T165_PARTICLE_MANAGER_H
#define GAME_T165_PARTICLE_MANAGER_H

#include "game/game.h"

struct Tex1Image;                           /* G001 (pointers only: forward declarations) */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" const type_info &__tf15ParticleManager(void);   /* 0x0019F858 (user) */
#endif

/* One particle, 0x54 bytes (ParticleManager::pool[128], new[] 0x2A00). */
struct Particle {
    Particle() : next(0) {}                 /* inline (the pool's new[]) */

    /* speed = |vel| * 1024 (at most 0x7FFF); flags cleared.  Callers: add,
     * emitDebris, emitSparks.  The parameter order is the callers' argument
     * order (EABI: the same registers as with the floats last; u056
     * matching pass). */
    void init(const f32 *pos, const f32 *vel, f32 mass, f32 size,
              const f32 *plane, f32 life, u32 color);                /* 0x0019E6F0 */
    int  update(f32 dt, int cut);                           /* 0x0019E7C8 0: expired (the caller's
                                                               argument order; EABI: the same registers) */
    void draw();                                            /* 0x0019ECA8 */
    u32  getColor();                                        /* 0x0019EDA0 by the remaining life */
    void resetTrail();                                      /* 0x0019EEE8 */

    Particle *next;                         /* 0x00 the active / free list */
    u16       flags;                        /* 0x04 bit 0: screen[] holds last frame's position */
    s16       speed;                        /* 0x06 |initial velocity| * 1024, at most 0x7FFF */
    f32       pos[4];                       /* 0x08 x y z, w = 1 */
    f32       vel[3];                       /* 0x18 */
    f32       invMass;                      /* 0x24 */
    f32       size;                         /* 0x28 */
    f32       plane[4];                     /* 0x2C the ground plane: normal, d */
    f32       life;                         /* 0x3C time left */
    f32       lifeMax;                      /* 0x40 */
    u32       color;                        /* 0x44 (never read) */
    f32       screen[3];                    /* 0x48 the last transformed position */
};
EGCS_SIZE(Particle, 0x54);

/* ? name (u056's).  The wheel record emitDebris reads (RaceCourse::
 * emitDebris, T153, passes it on). */
struct ParticleSource {
    u8  unk_00[4];                          /* 0x00 */
    int type;                               /* 0x04 index of g_0032FE70 (? the surface) */
    f32 pos[3];                             /* 0x08 */
    f32 yaw;                                /* 0x14 */
    u8  unk_18[4];                          /* 0x18 */
    f32 plane[4];                           /* 0x1C the ground plane: normal, d */
    u8  unk_2C[8];                          /* 0x2C */
    f32 speed0;                             /* 0x34 */
    f32 speed1;                             /* 0x38 speed1 - speed0: the slip */
};

/* One debris type: particles per call, the two colours mixed. */
struct ParticleSourceType {
    int count;                              /* 0x0 */
    u32 color0;                             /* 0x4 */
    u32 color1;                             /* 0x8 */
};
EGCS_SIZE(ParticleSourceType, 0xC);

/* vtable 0x0032FE48 (1 slot), typeinfo __tf15ParticleManager 0x0019F858
 * (user).  0x10 bytes of data, egcs vptr +0x10.  Size 0x14. */
class ParticleManager {
public:
    /* The pool (new Particle[128]), the texture "race/particle.img" (read
     * whole, relocated), clear(). */
    ParticleManager();                                      /* 0x0019EEF8 */
    /* free(tex), delete[] pool. */
    virtual ~ParticleManager();                             /* [0] +0x0C 0x0019EF90 */

    /* No live particle; all on the free list in pool order (RacePS2Base::
     * virtual_8, the constructor). */
    void clear();                                           /* 0x0019EFF8 */
    /* One particle, if one is free (no callers). */
    void add(const f32 *pos, const f32 *vel, const f32 *plane, u32 color,
             f32 mass, f32 size, f32 life);                 /* 0x0019F040 ? name */
    /* When the wheel's slip is at least 2: g_0032FE70[type].count particles
     * thrown along its yaw from around the wheel, life 1.5.  Caller:
     * RaceCourse::emitDebris (T153). */
    void emitDebris(const ParticleSource *src);             /* 0x0019F0D8 ? name */
    /* count sparks from pos along dir (spread: g_002C03B0.spread).  Caller:
     * RaceCarModel::emitSparks (T180). */
    void emitSparks(const f32 *pos, const f32 *dir, const f32 *plane, int count);   /* 0x0019F450 ? name */
    Particle *alloc();                                      /* 0x0019F640 NULL when none is free */
    void release(Particle *p, Particle *prev);              /* 0x0019F670 */
    void update(int cut, f32 dt);                           /* 0x0019F698 */
    /* The live particles' streaks in one strip batch, additive blending
     * (PGL_BLEND_SEL(CS, ZERO, FIX, CD), fix 1.0), no depth writes. */
    void draw();                                            /* 0x0019F738 */
    void resetTrails();                                     /* 0x0019F820 ? name */

    Particle  *pool;                        /* 0x00 new Particle[128] */
    Particle  *active;                      /* 0x04 the live list (newest first) */
    Particle  *freeList;                    /* 0x08 */
    Tex1Image *tex;                         /* 0x0C "race/particle.img" */
    /*         vptr                            0x10 */
};
EGCS_SIZE(ParticleManager, 0x14);

/* A line from a to b (view space; pgl_draw_line_3d) when both are in front
 * of the camera (z < -0.1).  No callers (u117 only names it as a caller of
 * pgl_draw_line_3d). */
void particle_draw_line(const f32 *a, const f32 *b);        /* 0x0019EAD0 ? name */
/* A quad from `from' to `to' (screen positions), width to each side and
 * beyond each end; a square when they coincide.  Caller: Particle::draw. */
void particle_draw_streak(const f32 *from, const f32 *to, u32 color, f32 width);    /* 0x0019EB30 ? name */
u32  particle_pack_color(f32 r, f32 g, f32 b, f32 a);       /* 0x0019ED48 ? name */

/* The debris type table g_0032FE70 (by ParticleSource::type) is a file
 * static of T165 (internal linkage: retail has it after T165's type name,
 * where egcs writes deferred internal-linkage tables; u056 data pass). */
/* the streak's texture coordinates (0,0) (1,0) (0,1) (1,1) */
extern const f32 g_0032FE10[8];                             /* 0x0032FE10 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti15ParticleManager[2];    /* 0x003533B0 gp-0x4640 __user_type_info (u056 g_003533B0) */
#endif

#endif /* GAME_T165_PARTICLE_MANAGER_H */
