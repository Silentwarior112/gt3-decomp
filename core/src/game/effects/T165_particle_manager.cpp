/* T165 - ParticleManager: 128 particles (sparks, wheel debris) in a pool
 * with an active and a free list.  0x0019E6F0..0x0019F898.
 *
 * A particle moves with drag (-size^2 * drag * v|v| / mass), gravity and
 * an occasional lift early in its life, bounces off its ground plane, and
 * dies when its life runs out (a bounce shortens it).  It is drawn as a
 * streak from last frame's screen position to this frame's, coloured by
 * its remaining life (g_002C03B0.keys: yellow -> orange -> red), with
 * additive blending; the colour given at birth is stored but never read.
 *
 * rsqrt.s fd, fs, ft computes fs / sqrt(ft): particle_rsqrt below (the
 * instruction under egcs, the division under GCC 14: the same value up to
 * the instruction's precision).  sqrt.s: boot_sqrtf_v (the volatile form,
 * chosen per call site: both sites are exact only with it).
 *
 * The headers it needs, not race_meters_internal.h: retail's T165 has only its own type
 * name (no race display class, no "bad_alloc", no .sdata).
 */
#include "types.h"
#include "libc.h"                           /* free */
#include "cxxrt.h"                          /* type_info, __rtti_user (GCC 14) */
#include "game/effects/T165_particle_manager.h"     /* Particle, ParticleSource(Type), ParticleManager, the helpers */
#include "game/effects/T166_particle_params.h"      /* ParticleParams, ParticleColorKey, g_002C03B0 */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "engine/gfx/T512_render_context.h"       /* g_002F4250 (the VRAM) */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadTex1 */
#include "engine/pgl/G001_flare.h"                /* Tex1Image, tex1_relocate, vu0_rand, the pgl_* / gfx_* /
                                               vu0_* calls, strip_batch_*, strip_draw, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* cosf, sinf */

#define PARTICLE_POOL_SIZE      128
#define PARTICLE_TRAIL          0x0001      /* Particle::flags: screen[] holds last frame's position */
#define PARTICLE_SPARK_COLOR    0xFF229EFFu /* emitSparks (stored, never read) */

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft), a COP1 instruction egcs never
 * emits: a volatile asm, as the original's (u032's icon_rsqrt).  GCC 14:
 * the division. */
static inline f32 particle_rsqrt(f32 num, f32 sq)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 r;

    __asm__ __volatile__("rsqrt.s %0, %1, %2" : "=f"(r) : "f"(num), "f"(sq));
    return r;
#else
    return num / boot_sqrtf(sq);
#endif
}

/* ======================================================================== */
/* Particle                                                                 */
/* ======================================================================== */

// FUNC 0x0019E6F0 Particle::init
//   Callers: ParticleManager::add / emitDebris / emitSparks.  speed =
//   |vel| * 1024 (truncated), at most 0x7FFF; flags cleared.  The fields in
//   order; the parameter order is the one the callers' argument set-up
//   shows (add is exact only with it; EABI: the same registers).
void Particle::init(const f32 *startPos, const f32 *startVel, f32 mass, f32 startSize,
                    const f32 *groundPlane, f32 lifeTime, u32 rgba)
{
    int scaled;

    flags = 0;
    pos[0] = startPos[0];
    pos[1] = startPos[1];
    pos[2] = startPos[2];
    pos[3] = 1.0f;
    vel[0] = startVel[0];
    vel[1] = startVel[1];
    vel[2] = startVel[2];
    invMass = 1.0f / mass;
    size = startSize;
    plane[0] = groundPlane[0];
    plane[1] = groundPlane[1];
    plane[2] = groundPlane[2];
    plane[3] = groundPlane[3];
    life = lifeTime;
    lifeMax = lifeTime;
    color = rgba;
    scaled = (int)(boot_sqrtf_v(startVel[0] * startVel[0] + startVel[1] * startVel[1]
                                + startVel[2] * startVel[2]) * 1024.0f);
    if (scaled < 0x8000)
        speed = scaled;
    else
        speed = 0x7FFF;
}

// FUNC 0x0019E7C8 Particle::update
//   Caller: ParticleManager::update.  cut != 0 (a camera change) forgets the
//   trail.  0 when the life ran out.  The position moves with the old
//   velocity; the drag terms use the old velocity's signs.  Below the plane
//   (depth < 0): pushed back onto it, life *= bounceLife, and the velocity w
//   becomes  t * (1 - 2 * bounce * friction * (w.n) / |t|) - bounce * w
//   (t: w's part along the plane; only -bounce * w when the factor is
//   negative).  Quirk: the second term is the whole velocity, not its
//   normal part.
//   Matching (90.4%, 193/193): everything up to the bounce block is retail's
//   (the params pointer taken after the early return; vel[] read in place,
//   so it is reloaded after vu0_rand; dvy as `(vy < 0 ? -t : t) - gdt' with
//   gdt computed before the test: one sub at the join, into dvy's register;
//   tx *= factor in place).  Left: sched2 order and registers inside the
//   bounce block (ns * n1 in $f2, retail $f8; tz in $f8, retail $f6; the
//   -bounce * w products scheduled early).  Tried: 378 orders of the block's
//   statements (all the same code), the products before / after / inside
//   the arms, num / sq locals, declaration orders, the reference compiler
//   (the same 90.4%).
int Particle::update(f32 dt, int cut)
{
    const ParticleParams *p;
    f32 t, gdt, dtOverMass, drag, age, dvx, dvy, dvz, depth;

    if (cut != 0)
        resetTrail();
    life = life - dt;
    if (life < 0.0f)
        return 0;
    p = &g_002C03B0;

    dtOverMass = invMass * dt;
    drag = -size * size * p->drag * dtOverMass;
    pos[0] = pos[0] + vel[0] * dt;
    pos[1] = pos[1] + vel[1] * dt;
    pos[2] = pos[2] + vel[2] * dt;
    age = lifeMax - life;

    dvx = drag * vel[0] * vel[0];
    if (vel[0] < 0.0f)
        dvx = -dvx;
    gdt = p->gravity * dt;
    t = drag * vel[1] * vel[1];
    dvy = (vel[1] < 0.0f ? -t : t) - gdt;
    dvz = drag * vel[2] * vel[2];
    if (vel[2] < 0.0f)
        dvz = -dvz;

    if (p->liftAgeMin < age && age < p->liftAgeMax && vu0_rand() < p->liftChance)
        dvy = dvy + p->lift * (f32)speed * (1.0f / 1024.0f) * dtOverMass;

    vel[0] = vel[0] + dvx;
    vel[1] = vel[1] + dvy;
    vel[2] = vel[2] + dvz;

    depth = pos[0] * plane[0] + pos[1] * plane[1] + pos[2] * plane[2] + plane[3];
    if (depth < 0.0f) {
        f32 normalSpeed = vel[0] * plane[0] + vel[1] * plane[1] + vel[2] * plane[2];
        f32 tx, ty, tz, bounce, factor, bx, by, bz;

        life = life * p->bounceLife;
        pos[0] = pos[0] - depth * plane[0];
        pos[1] = pos[1] - depth * plane[1];
        pos[2] = pos[2] - depth * plane[2];
        tx = vel[0] - normalSpeed * plane[0];
        ty = vel[1] - normalSpeed * plane[1];
        tz = vel[2] - normalSpeed * plane[2];
        bounce = p->bounce;
        bx = -bounce * vel[0];
        by = -bounce * vel[1];
        bz = -bounce * vel[2];
        factor = 1.0f - particle_rsqrt((bounce + bounce) * p->friction * normalSpeed,
                                       tx * tx + ty * ty + tz * tz);
        if (factor < 0.0f) {
            vel[0] = bx;
            vel[1] = by;
            vel[2] = bz;
        } else {
            tx *= factor;
            ty *= factor;
            tz *= factor;
            vel[0] = tx + bx;
            vel[1] = ty + by;
            vel[2] = tz + bz;
        }
    }
    return 1;
}

// FUNC 0x0019EAD0 particle_draw_line
//   ? name.  No callers.  A line from a to b (pgl_draw_line_3d) when both
//   are in front of the camera (z < -0.1).
void particle_draw_line(const f32 *a, const f32 *b)
{
    int aInFront = a[2] < -0.1f;
    int bInFront = b[2] < -0.1f;

    if (aInFront && bInFront)
        pgl_draw_line_3d(a, b);
}

/* The streak's texture coordinates (0,0) (1,0) (0,1) (1,1): the first item
 * of T165's .rodata, before the constructor's "race/particle.img"
 * (external linkage, T165_particle_manager.h declares it: written here). */
const f32 g_0032FE10[8] = {                 /* 0x0032FE10 */
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
};

// FUNC 0x0019EB30 particle_draw_streak
//   ? name.  Caller: Particle::draw.  A quad (a strip of 4 vertices, the
//   texture coordinates g_0032FE10) from `from' to `to' (screen positions),
//   `width' to each side and beyond each end; a square around `from' when
//   they coincide in x / y.  One colour for all four corners.  The colours
//   and vertices come from the tail of the prim buffer.
void particle_draw_streak(const f32 *from, const f32 *to, u32 color, f32 width)
{
    u32 *colors = (u32 *)gfx_alloc_tail(1);
    f32 *v = (f32 *)gfx_alloc_tail(3);
    f32 ax = from[0], ay = from[1], az = from[2];
    f32 bx = to[0], by = to[1], bz = to[2];
    f32 dx = bx - ax;
    f32 dy = by - ay;

    colors[0] = color;
    colors[1] = color;
    colors[2] = color;
    colors[3] = color;
    if (dx * dx + dy * dy == 0.0f) {
        v[0] = ax + width;  v[1] = ay + width;  v[2] = az;
        v[3] = ax - width;  v[4] = ay + width;  v[5] = az;
        v[6] = ax + width;  v[7] = ay - width;  v[8] = az;
        v[9] = ax - width;  v[10] = ay - width; v[11] = az;
    } else {
        /* rsqrt.s: width / sqrt(dx^2 + dy^2); (dx, dy) becomes the
         * streak's direction, `width' long */
        f32 k = particle_rsqrt(width, dx * dx + dy * dy);

        dx *= k;
        dy *= k;
        v[0] = ax - dx - dy;  v[1] = ay - dy + dx;  v[2] = az;
        v[3] = ax - dx + dy;  v[4] = ay - dy - dx;  v[5] = az;
        v[6] = bx + dx - dy;  v[7] = by + dy + dx;  v[8] = bz;
        v[9] = bx + dx + dy;  v[10] = by + dy - dx; v[11] = bz;
    }
    strip_draw(v, NULL, colors, g_0032FE10, 4);
}

// FUNC 0x0019ECA8 Particle::draw
//   Caller: ParticleManager::draw.  Transforms the position; draws the
//   streak from last frame's screen position once there is one
//   (PARTICLE_TRAIL; under an identity matrix: the positions are already
//   transformed), and keeps this frame's.
void Particle::draw()
{
    f32 view[4] __attribute__((aligned(16)));   /* sp+0x0 */

    vu0_apply_matrix(view, pos);
    if (flags & PARTICLE_TRAIL) {
        pgl_push_matrix();
        vu0_load_identity_regs();
        particle_draw_streak(screen, view, getColor(), size);
        pgl_pop_matrix();
    } else {
        flags = flags | PARTICLE_TRAIL;
    }
    screen[0] = view[0];
    screen[1] = view[1];
    screen[2] = view[2];
}

// FUNC 0x0019ED48 particle_pack_color
//   ? name.  Caller: Particle::getColor.  RGBA (1.0 = 0x80, scaled by 127.5
//   and truncated) to the GS byte order R | G << 8 | B << 16 | A << 24.
u32 particle_pack_color(f32 r, f32 g, f32 b, f32 a)
{
    return (int)(r * 127.5f) | (int)(g * 127.5f) << 8 | (int)(b * 127.5f) << 16 | (int)(a * 127.5f) << 24;
}

/* ? name.  A colour key as particle_pack_color packs it; inline (retail
 * expands it in Particle::getColor, whose blended colour calls
 * particle_pack_color itself). */
static inline u32 key_color(const ParticleColorKey &k)
{
    return (int)(k.r * 127.5f) | (int)(k.g * 127.5f) << 8 | (int)(k.b * 127.5f) << 16 | (int)(k.a * 127.5f) << 24;
}

// FUNC 0x0019EDA0 Particle::getColor
//   Caller: Particle::draw.  The first key whose life is below the
//   particle's: key 0 as it is (key_color, inlined: its r load goes through
//   the params' address, retail's 0x38($a3)), a later key blended with the
//   one before by where the life lies between them (a call of
//   particle_pack_color).  0 when no key is below (life <= 0).  The life
//   test reads params.keys[i], the colours keys[i]: two strength-reduced
//   cursors, as retail.
u32 Particle::getColor()
{
    const ParticleParams &params = g_002C03B0;
    const ParticleColorKey *keys = params.keys;
    int i;

    for (i = 0; i < 4; i++) {
        if (params.keys[i].life < life) {
            if (i == 0)
                return key_color(params.keys[0]);   /* inlined */
            else {
                /* blended with the key before by where the life lies
                 * between theirs (particle_pack_color, not inlined) */
                const ParticleColorKey *next = &keys[i];
                f32 t = (params.keys[i - 1].life - life) / (params.keys[i - 1].life - params.keys[i].life);
                f32 u = 1.0f - t;

                return particle_pack_color(next[-1].r * u + next->r * t, next[-1].g * u + next->g * t,
                                           next[-1].b * u + next->b * t, next[-1].a * u + next->a * t);
            }
        }
    }
    return 0;
}

// FUNC 0x0019EEE8 Particle::resetTrail
//   Callers: Particle::update (cut), ParticleManager::resetTrails.
void Particle::resetTrail()
{
    flags = flags & (u16)~PARTICLE_TRAIL;
}

/* ======================================================================== */
/* ParticleManager                                                          */
/* ======================================================================== */

// FUNC 0x0019EEF8 ParticleManager::ParticleManager
//   Caller: RacePS2Base's constructor (0x0017D070).  The vptr, the pool
//   (operator new[](0x2A00), each next = 0: Particle's inline constructor),
//   the texture "race/particle.img" read whole (unpacked) and relocated,
//   the lists (clear).
ParticleManager::ParticleManager()
{
    pool = new Particle[PARTICLE_POOL_SIZE]; /* implicit: operator new[] (new-expression) */
    tex = (Tex1Image *)file_read_blocking("race/particle.img", NULL, 0, 1, &vol_device_object);
    tex1_relocate(tex);
    clear();
}

// FUNC 0x0019EF90 ParticleManager::~ParticleManager
//   Callers: RacePS2Base's constructor (EH) and destructor.  The vptr,
//   free(tex), operator delete[](pool) when non-null; the deleting form then
//   calls operator delete(this).
ParticleManager::~ParticleManager()
{
    free(tex);
    delete[] pool; /* implicit: operator delete[] (delete-expression), operator delete (deleting destructor) */
}

// FUNC 0x0019EFF8 ParticleManager::clear
//   Callers: RacePS2Base::virtual_8, the constructor.  No live particle;
//   every one on the free list in pool order.
void ParticleManager::clear()
{
    int i;

    active = NULL;
    freeList = pool;
    for (i = 0; i < PARTICLE_POOL_SIZE - 1; i++)
        pool[i].next = &pool[i] + 1;
    pool[PARTICLE_POOL_SIZE - 1].next = NULL;
}

// FUNC 0x0019F040 ParticleManager::add
//   ? name.  No callers.  One particle, if one is free (alloc, init).
void ParticleManager::add(const f32 *pos, const f32 *vel, const f32 *plane, u32 color,
                          f32 mass, f32 size, f32 life)
{
    Particle *p = alloc();

    if (p != NULL)
        p->init(pos, vel, mass, size, plane, life, color);
}

/* The debris types by ParticleSource::type: particles per call and the two
 * colours mixed.  Internal linkage: egcs writes it at the very end of
 * T165's .rodata, after the vtable and the type name (retail
 * 0x0032FE70..0x0032FEC4; no other file reads it). */
static const ParticleSourceType g_0032FE70[7] = {   /* 0x0032FE70 */
    { 0, 0x30404040, 0x20303030 },
    { 1, 0x40808080, 0x40404040 },
    { 1, 0x40208020, 0x40000000 },
    { 7, 0x506080B0, 0x505D7A96 },
    { 7, 0x506080B0, 0x505D7A96 },
    { 8, 0x506080B0, 0x505D7A96 },
    { 5, 0x50FF0000, 0x50FFFFFF },
};

// FUNC 0x0019F0D8 ParticleManager::emitDebris
//   ? name.  Caller: RaceCourse::emitDebris (T153; itself never called).
//   When the wheel's slip (speed1 - speed0, times 0.5..0.95) is at least 2:
//   g_0032FE70[type].count particles thrown at |slip| along the wheel's yaw
//   (+-15 degrees; against it for a negative slip), 70..85 degrees from the
//   ground normal (5..20 above the plane), from around the wheel (+-0.1),
//   size 0..0.02, mass size^3 * 1000, a random mix of the type's two
//   colours, life 1.5.  Stops when the pool is empty.
void ParticleManager::emitDebris(const ParticleSource *src)
{
    f32 slip, signedSlip;
    int made;

    slip = (src->speed1 - src->speed0) * (vu0_rand() * 0.45f + 0.5f);
    signedSlip = slip;
    if (slip < 0.0f)
        slip = -slip;
    if (slip < 2.0f)
        return;

    made = 0;
    while (made < g_0032FE70[src->type].count) {
        Particle *p = alloc();
        f32 yaw, cosYaw, sinYaw, normalH, normalY, invLen, pitch, cosPitch, sinPitch;
        f32 along, size, mass;
        f32 vel[3];                         /* sp+0x0 */
        f32 pos[3];                         /* sp+0x10 */
        u32 color0, color1, color;
        int mix;

        if (p == NULL)
            return;
        /* the yaw +- 15 degrees (0.5235984 = 30, 0.2617992 = 15 in radians) */
        yaw = src->yaw + vu0_rand() * 0.5235984f + -0.2617992f;
        cosYaw = cosf(yaw);
        sinYaw = sinf(yaw);
        /* the ground normal in the vertical plane of the yaw, normalised */
        normalH = src->plane[0] * sinYaw + src->plane[2] * cosYaw;
        normalY = src->plane[1];
        /* rsqrt.s: 1 / sqrt(h^2 + y^2) */
        invLen = particle_rsqrt(1.0f, normalH * normalH + normalY * normalY);
        normalH = normalH * invLen;
        normalY = normalY * invLen;
        /* -85 .. -70 degrees from the normal (0.2617991 = 15, 1.4835286 =
         * 85 degrees); the other way for a negative slip */
        pitch = vu0_rand() * 0.2617991f + -1.4835286f;
        if (signedSlip < 0.0f)
            pitch = -pitch;
        made++;
        cosPitch = cosf(pitch);
        sinPitch = sinf(pitch);
        /* the normal rotated by pitch in that plane, times |slip| */
        along = cosPitch * normalH - sinPitch * normalY;
        vel[0] = along * sinYaw * slip;
        vel[1] = (sinPitch * normalH + cosPitch * normalY) * slip;
        vel[2] = along * cosYaw * slip;
        pos[0] = src->pos[0] + (vu0_rand() - 0.5f) * 0.2f;
        pos[1] = src->pos[1] + (vu0_rand() - 0.5f) * 0.2f;
        pos[2] = src->pos[2] + (vu0_rand() - 0.5f) * 0.2f;
        size = vu0_rand() * 0.02f;
        mass = size * size * size * 1000.0f;
        color0 = g_0032FE70[src->type].color0;
        color1 = g_0032FE70[src->type].color1;
        mix = (int)(vu0_rand() * 255.0f);
        /* color0 * mix + color1 * (255 - mix), per byte (R and B together),
         * >> 8; color0's alpha */
        color = ((((color0 & 0xFF00FF) * mix + (color1 & 0xFF00FF) * (0xFF - mix)) >> 8) & 0xFF00FF)
              | ((((color0 & 0xFF00) * mix + (color1 & 0xFF00) * (0xFF - mix)) >> 8) & 0xFF00)
              | (color0 & 0xFF000000);
        p->init(pos, vel, mass, size, src->plane, 1.5f, color);
    }
}

// FUNC 0x0019F450 ParticleManager::emitSparks
//   ? name.  Caller: RaceCarModel::emitSparks (T180).  count sparks at pos:
//   dir turned about the vertical by +-spread/2 degrees and scaled to
//   speedMin..speedMax, the vertical part jittered by (the mean of two
//   randoms - 0.5) * |dir in xz| * upJitter; size sizeMin..sizeMax, mass
//   size^3 * density, life lifeMin..lifeMax, colour PARTICLE_SPARK_COLOR.
//   Stops when the pool is empty.
void ParticleManager::emitSparks(const f32 *pos, const f32 *dir, const f32 *plane, int count)
{
    f32 dirXZ = boot_sqrtf_v(dir[0] * dir[0] + dir[2] * dir[2]);   /* sqrt.s, volatile */
    int made = 0;

    while (made < count) {
        Particle *p = alloc();
        f32 vel[3];                         /* sp+0x0 */
        f32 angle, speed, sinSpeed, cosSpeed, jitter, size, mass, life;

        if (p == NULL)
            return;
        made++;
        angle = (vu0_rand() - 0.5f) * 0.017453292f * g_002C03B0.spread;   /* degrees -> radians */
        speed = vu0_rand() * (g_002C03B0.speedMax - g_002C03B0.speedMin) + g_002C03B0.speedMin;
        sinSpeed = sinf(angle) * speed;
        cosSpeed = cosf(angle) * speed;
        vel[0] = cosSpeed * dir[0] - sinSpeed * dir[2];
        jitter = vu0_rand();
        jitter = jitter + vu0_rand();
        vel[1] = dir[1] + (jitter * 0.5f - 0.5f) * dirXZ * g_002C03B0.upJitter;
        vel[2] = sinSpeed * dir[0] + cosSpeed * dir[2];
        size = vu0_rand() * (g_002C03B0.sizeMax - g_002C03B0.sizeMin) + g_002C03B0.sizeMin;
        mass = size * size * size * g_002C03B0.density;
        life = vu0_rand() * (g_002C03B0.lifeMax - g_002C03B0.lifeMin) + g_002C03B0.lifeMin;
        p->init(pos, vel, mass, size, plane, life, PARTICLE_SPARK_COLOR);
    }
}

// FUNC 0x0019F640 ParticleManager::alloc
//   Callers: add, emitDebris, emitSparks.  The first free particle, put at
//   the head of the live list; NULL when none is free.
Particle *ParticleManager::alloc()
{
    Particle *p = freeList;

    if (p == NULL)
        return NULL;
    freeList = p->next;
    p->next = active;
    active = p;
    return p;
}

// FUNC 0x0019F670 ParticleManager::release
//   Caller: update.  Unlinks p (prev: the live particle before it, NULL at
//   the head) and puts it on the free list.
void ParticleManager::release(Particle *p, Particle *prev)
{
    Particle *next = p->next;

    if (prev != NULL)
        prev->next = next;
    else
        active = next;
    p->next = freeList;
    freeList = p;
}

// FUNC 0x0019F698 ParticleManager::update
//   Caller: RacePS2Base::virtual_28 (cut: a byte of the camera, dt).  Each
//   live particle moves (Particle::update); the expired ones are released.
void ParticleManager::update(int cut, f32 dt)
{
    Particle *prev = NULL;
    Particle *p = active;

    while (p != NULL) {
        Particle *next = p->next;

        if (p->update(dt, cut) == 0)
            release(p, prev);
        else
            prev = p;
        p = next;
    }
}

// FUNC 0x0019F738 ParticleManager::draw
//   Caller: RacePS2Base::drawView.  When any particle is live: the
//   texture made resident (VramAllocator::loadTex1) and selected, depth
//   test, blending and texturing on, face culling off, no depth writes,
//   additive blending (Cs * 1.0 + Cd); one batch of streaks flushed under an
//   identity matrix, then depth writes and culling back on, texturing off.
void ParticleManager::draw()
{
    Particle *p;

    if (active == NULL)
        return;
    g_002F4250.loadTex1(tex);
    gfx_set_texture_regs(tex->tex);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_BLEND);
    gfx_enable(PGL_CAP_TEXTURE);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_depth_mask(0);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_CD), 1.0f);  /* 0x68 */
    strip_batch_begin();
    for (p = active; p != NULL; p = p->next)
        p->draw();
    pgl_push_matrix();
    vu0_load_identity_regs();
    strip_batch_end();
    pgl_pop_matrix();
    gfx_depth_mask(1);
    gfx_enable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_TEXTURE);
}

// FUNC 0x0019F820 ParticleManager::resetTrails
//   ? name.  No callers.  Every live particle forgets its trail.
void ParticleManager::resetTrails()
{
    Particle *p;

    for (p = active; p != NULL; p = p->next)
        p->resetTrail();
}

#ifndef BOOT_OLD_GCC
// FUNC 0x0019F858 __tf15ParticleManager
//   __user_type_info at 0x003533B0 (gp-0x4640).
extern "C" const type_info &__tf15ParticleManager(void)
{
    if (__ti15ParticleManager[0] == 0)
        __rtti_user(__ti15ParticleManager, "15ParticleManager");
    return *(const type_info *)__ti15ParticleManager;
}
#endif
