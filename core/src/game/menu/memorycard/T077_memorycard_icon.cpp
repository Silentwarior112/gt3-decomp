/* T077 - the 3D save icons of the memory card screens
 * (game/menu/memorycard/T077_memorycard_icon.h).
 *
 * A PS2 icon file (.ico), as read here:
 *   u32 0x00010000, u32 shapes (1, 2, 4, 6, 8), u32 texture type (bit 0 ?,
 *   bit 2 textured, bit 3 RLE-compressed), u32 (unused), u32 vertices
 *   (a multiple of 3, at most 600 / 550 / 500 / 450 / 400 triangles for
 *   1 / 2 / 4 / 6 / 8 shapes);
 *   per vertex: shapes x {s16 x, y, z, w}, {s16 nx, ny, nz, nw} (1.0 =
 *   4096), {s16 u, v}, u32 RGBA;
 *   u32 animations (at most 256), then per animation: u32 frame length,
 *   f32 speed, u32 play offset, u32 frames (at most 8), per frame {u32 shape,
 *   u32 keys (at most 10), keys x {f32 time, f32 value}};
 *   the texture: 128 x 128 16-bit texels, or u32 size + RLE data.
 * Only the first animation is kept.
 *
 * Data: .rodata 0x00320280..0x00320330: <new>'s "bad_alloc" (unreferenced:
 * DATA_START), the three quads of icon_model_render (g_00320290,
 * g_003202B0, g_003202E0), icon_model_draw's jump table; no type names (the
 * original saw no class whose virtual functions are all inline).  .sdata
 * 0x003518D0..0x003518E8: g_iconHeapReady, g_iconSlotUsed, g_iconSlot
 * (zeros: globals without an initialiser).  .data: g_iconHeap and the frame
 * info.
 */
// DATA_START 0x00320280
#include <new>                              /* (seen: "bad_alloc", T077's first .rodata) */
#include "libgraph.h"                       /* SCE_GS_PSMCT32 / PSMCT16 / PSMZ24 */
#include "pdistd/allocator.h"               /* pdistd::Allocator (the icon heaps) */
#include "game/menu/memorycard/T077_memorycard_icon.h"
#include "engine/pgl/G001_flare.h"                /* gfx_* / gs_* / pgl_* / vu0_*, tri_draw16, PGL_* */
#include "engine/gfx/T512_render_context.h"       /* g_002F4250 (the VRAM allocator) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/menu/memorycard/memorycard_icon_internal.h"

/* An 8-byte load from an unaligned address (egcs: ldl / ldr). */
struct U64Unaligned {
    u64 v;
} __attribute__((packed));

static inline u64 load64u(const void *p)
{
    return ((const U64Unaligned *)p)->v;
}

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
/* icon_model_draw's mesh loops, one instruction per asm (volatile: they stay
 * in this order), as the original's hand-written MMI code has them (see the
 * function).  The weights are 128-bit values (u128): egcs then saves the
 * registers that held them with sq / lq in every later function of this
 * file (icon_anim_weights, the static constructor), as retail does. */
#define ICON_LW(w, m)       __asm__ __volatile__("lw %0, %1" : "=r"(w) : "m"(m))
#define ICON_PCPYH(w)       __asm__ __volatile__("pcpyh %0, %1" : "=r"(w) : "r"(w))
#define ICON_ULD(v, p)      __asm__ __volatile__("uld %0, %1" : "=r"(v) \
                                                 : "m"(*(const U64Unaligned *)(p)))
#define ICON_PMULTH(d, a, b) __asm__ __volatile__("pmulth %0, %1, %2" : "=&r"(d) \
                                                  : "r"(a), "r"(b) : "hi", "lo")
#define ICON_PMADDH(d, a, b) __asm__ __volatile__("pmaddh %0, %1, %2" : "=r"(d) \
                                                  : "r"(a), "r"(b), "0"(d) : "hi", "lo")
#define ICON_PMFLO(d)       __asm__ __volatile__("pmflo %0" : "=r"(d))
#define ICON_SD(m, v)       __asm__ __volatile__("sd %1, %0" : "=m"(m) : "r"(v))
#define ICON_SH(m, v)       __asm__ __volatile__("sh %1, %0" : "=m"(m) : "r"(v))
#else
/* MMI pcpyh: the low halfword of x in all four halfwords (of each half). */
static inline u64 mmi_pcpyh(s32 x)
{
    u64 r;
    __asm__("pcpyh %0, %1" : "=r"(r) : "r"((u64)(s64)x));
    return r;
}
#endif

/* Checks an icon file and returns the bytes icon_model_build needs for it,
 * -1 if it is not a usable icon. */
// FUNC 0x00159EE8 icon_model_size
int icon_model_size(const u32 *ico)
{
    const u32 *in = ico;
    int shapes, texType, verts, tris, uvSize, size;
    u32 animCount, frames, keys;
    int i, j;

    if (*in++ != 0x10000)
        return -1;
    shapes = *in++;
    if (!((u32)(shapes - 1) < 2 || shapes == 4 || shapes == 6 || shapes == 8))
        return -1;
    texType = *in;
    in += 2;
    verts = *in++;
    if (verts % 3 != 0)
        return -1;
    if (verts < 3)
        return -1;
    /* (a second division: egcs shares the div with the % above but keeps
     * its divide-by-zero check, as retail) */
    tris = verts / 3;
    if (tris >= 601 && shapes == 1)
        return -1;
    if (tris >= 551 && shapes == 2)
        return -1;
    if (tris >= 501 && shapes == 4)
        return -1;
    if (tris >= 451 && shapes == 6)
        return -1;
    if (tris >= 401 && shapes == 8)
        return -1;

    in += (shapes * 2 + 4) * verts;     /* the vertices */
    uvSize = (verts * 4 + 15) & ~15;
    /* the positions, the McIconModel header (0x50), the uvs, the colours */
    size = ((verts * shapes * 6 + 15) & ~15) + 0x50;
    size += uvSize;
    size += uvSize;

    animCount = *in++;
    if (animCount > 0x100)
        return -1;
    for (i = 0; i < (int)animCount; i++) {
        in += 3;                        /* frame length, speed, play offset */
        frames = *in++;
        if (frames > 8)
            return -1;
        for (j = 0; j < (int)frames; j++) {
            in++;                       /* shape */
            keys = *in++;
            in += keys * 2;
            if (keys > 10)
                return -1;
            if (i == 0) {               /* only the first animation is kept */
                size += 0xC;
                size += keys * 8;
            }
        }
    }

    if (texType & 4) {
        if (texType & 8) {
            if (*in > 0x10004)          /* the compressed size */
                return -1;
        }
        size = (size + 15) & ~15;
        size += 0x8000;                 /* 128 x 128 16-bit texels */
    }
    return size;
}

/* Unpacks an icon file into mem (icon_model_size bytes, 16-aligned; size is
 * not used).  The colours are lit with `light' (NULL: the file's colours).
 * Returns mem, NULL if it is not an icon file. */
// FUNC 0x0015A0E0 icon_model_build
McIconModel *icon_model_build(void *mem, int size, const u32 *ico, const McIconLight *light)
{
    McIconModel *model = (McIconModel *)mem;
    int p, stride;
    s16 *positions, *uvs;
    u32 *colors;
    s16 *pos, *uv;
    u32 *color;
    McIconAnim *anim;
    McIconFrame *frames;
    McIconKey *keys;
    const u32 *in;                      /* the file, a word at a time */
    const s16 *vtx;                     /* the vertex records */
    const f32 *keyIn;
    int shapes, verts, uvSize, animCount, keyCount, frameCount, i, j;
    u32 texType, rgba;
    f32 normal[3];

    /* `new (mem) McIconModel': egcs tests the pointer before the inline
     * constructor. */
    if (model != NULL) {
        model->refCount    = 0;
        model->ready       = 1;
        model->positions   = NULL;
        model->uvs         = NULL;
        model->colors      = NULL;
        model->anim.frames = NULL;
        model->texture     = NULL;
        model->heap        = 0;
    }

    p = (int)(model + 1);               /* the arrays follow the header (+0x50) */
    positions = (s16 *)p;
    in = ico;
    if (*in++ != 0x10000)
        return NULL;
    shapes = *in++;
    model->positions = (s16 *)p;
    model->shapes = shapes;
    texType = *in;
    in += 2;                            /* texture type, (unused) */
    vtx = (const s16 *)(in + 1);
    verts = *in;
    model->textured = ((int)texType >> 2) & 1;
    model->vertexCount = verts;
    model->flag0C = texType & 1;
    uvSize = (verts * 4 + 15) & ~15;
    stride = shapes * 6;
    p = (p + ((verts * stride + 15) & ~15) + 15) & ~15;
    uvs = (s16 *)p;
    model->uvs = uvs;
    p = (p + uvSize + 15) & ~15;
    colors = (u32 *)p;
    model->colors = colors;
    frames = (McIconFrame *)((p + uvSize + 15) & ~15);
    pos = positions;
    uv = uvs;
    color = colors;

    /* Per vertex: the shapes' (x, y, z) (w dropped), then the normal, the
     * (u, v) and the colour. */
    for (i = 0; i < model->vertexCount; i++) {
        for (j = 0; j < model->shapes; j++) {
            s16 x = vtx[0], y = vtx[1], z = vtx[2];

            pos += 3;
            pos[-3] = x;
            pos[-2] = y;
            pos[-1] = z;
            vtx += 4;
        }
        {
            s16 nx = vtx[0], ny = vtx[1], nz = vtx[2];
            s16 u = vtx[4], v = vtx[5];

            rgba = *(const u32 *)(vtx + 6);
            vtx += 8;
            uv[1] = v;
            normal[2] = (f32)nz * (1.0f / 4096.0f);
            uv[0] = u;
            normal[0] = (f32)nx * (1.0f / 4096.0f);
            uv += 2;
            normal[1] = (f32)ny * (1.0f / 4096.0f);
        }
        if (light != NULL)
            *color++ = icon_light_vertex(light, normal, rgba);
        else
            *color++ = (rgba & 0xFFFFFF) | 0x80000000;
    }

    /* The first animation. */
    anim = &model->anim;
    in = (const u32 *)vtx;
    animCount = *in++;
    anim->frames = frames;
    model->anim.frameLength = *in++;
    anim->speed = *(const f32 *)in++;
    anim->offset = (f32)*(const int *)in++;
    frameCount = *in++;
    anim->frameCount = frameCount;
    keys = (McIconKey *)&frames[frameCount];
    for (i = 0; i < anim->frameCount; i++) {
        anim->frames[i].shape = *in++;
        anim->frames[i].keys = keys;
        keyIn = (const f32 *)(in + 1);
        keyCount = *in;
        anim->frames[i].keyCount = keyCount;
        keys += keyCount;
        for (j = 0; j < keyCount; j++) {
            anim->frames[i].keys[j].time = *keyIn++;
            anim->frames[i].keys[j].value = *keyIn++;
        }
        in = (const u32 *)keyIn;
    }
    /* The others are skipped. */
    for (i = 1; i < animCount; i++) {
        int count;

        in += 3;                        /* frame length, speed, play offset */
        count = *in++;
        for (j = 0; j < count; j++) {
            in++;                       /* shape */
            keyCount = *in++;
            in += keyCount * 2;
        }
    }

    if (model->textured != 0) {
        u16 *tex = (u16 *)(((u32)keys + 15) & ~15);

        model->texture = tex;
        if (texType & 8) {
            icon_texture_unpack((s16 *)tex, (const s16 *)(in + 1), in[0]);
        } else {
            const u16 *texel = (const u16 *)in;

            for (i = 0x3FFF; i >= 0; i--)
                *tex++ = *texel++ | 0x8000;
        }
        return model;
    }
    model->texture = NULL;
    return model;
}

/* The RLE texture of an icon file: a count n > 0 and one texel (n copies),
 * or a count n < 0 and -n texels; at most 0x8000 bytes are written, size is
 * the size of the packed data. */
// FUNC 0x0015A558 icon_texture_unpack
void icon_texture_unpack(s16 *dst, const s16 *src, int size)
{
    const s16 *end = (const s16 *)((const u8 *)src + (size & ~1));
    s16 *dstEnd = (s16 *)((u8 *)dst + 0x8000);
    int n, i;
    s16 c;

    while (src < end && dst < dstEnd) {
        n = *src++;
        if (n > 0) {
            c = *src++ | 0x8000;
            for (i = 0; i < n; i++) {
                *dst++ = c;
                if (!(dst < dstEnd))
                    return;
            }
        } else if (n < 0) {
            n = -n;
            for (i = 0; i < n; i++) {
                *dst++ = *src | 0x8000;
                if (!(dst < dstEnd))
                    return;
                src++;
                if (!(src < end))
                    return;
            }
        }
    }
}

/* An icon model in an icon heap.  *error: ICON_OK, ICON_ERROR_NOT_ICON
 * (not an icon, or not unpackable), ICON_ERROR_NO_MEMORY (heap not assigned
 * or full). */
// FUNC 0x0015A648 icon_model_create
McIconModel *icon_model_create(int unused, const u32 *ico, const McIconLight *light, int *error)
{
    int heap;
    int size;
    void *mem;
    McIconModel *model;

    /* Quirk: the heap index is an uninitialised variable in the original:
     * egcs keeps it in $s1 and normalises whatever the caller left there
     * (sltu $s1, $zero, $s1); the only caller (MemorycardOtherPS2File::
     * readIcon, T108) has its `this' there, so heap 1.  GCC 14 reads $s1
     * explicitly (an uninitialised read is undefined there).  The first
     * argument (the caller passes its port) is never read. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    heap = heap != 0;
#else
    {
        register int callerS1 __asm__("$17");
        __asm__ volatile("" : "=r"(callerS1));
        heap = callerS1 != 0;
    }
#endif
    *error = ICON_OK;
    if (g_iconHeapReady[heap] == 0) {
        *error = ICON_ERROR_NO_MEMORY;
        return NULL;
    }
    size = icon_model_size(ico);
    if (size < 0) {
        *error = ICON_ERROR_NOT_ICON;
        return NULL;
    }
    mem = g_iconHeap[heap].allocate(size, 0x10);
    if (mem == NULL) {
        *error = ICON_ERROR_NO_MEMORY;
        return NULL;
    }
    model = icon_model_build(mem, size, ico, light);
    if (model == NULL) {
        *error = ICON_ERROR_NOT_ICON;
        /* Bug: frees the result (NULL), not mem: the block stays allocated
         * (unreachable: icon_model_size has checked the magic). */
        g_iconHeap[heap].free(model);
        return NULL;
    }
    model->heap = heap;
    return model;
}

/* Drops a reference to an icon model of icon_model_create; below 0 it is
 * hidden, (unless noWait) two frames are waited for, and it is freed. */
// FUNC 0x0015A738 icon_model_release
void icon_model_release(McIconModel *icon, int noWait)
{
    if (icon != NULL) {
        if (--icon->refCount < 0) {
            icon->ready = 0;
            if (noWait == 0)
                gfx_vsync(2);
            g_iconHeap[icon->heap].free(icon);
        }
    }
}

/* An icon model in fixed slot `slot' (replacing the one there); NULL if the
 * slot has no memory or the icon does not fit / is not one. */
// FUNC 0x0015A7A0 icon_slot_create
McIconModel *icon_slot_create(int slot, const u32 *ico, const McIconLight *light)
{
    McIconModel *mem = g_iconSlot[slot];
    u32 size;

    if (mem == NULL)
        return NULL;
    icon_slot_release(slot);
    size = icon_model_size(ico);
    if (size > ICON_SLOT_SIZE)          /* unsigned: -1 too */
        return NULL;
    if (icon_model_build(mem, size, ico, light) == NULL)
        return NULL;
    g_iconSlotUsed[slot] = 1;
    return mem;
}

/* Empties slot `slot' (hiding its model and waiting two frames if it had
 * one). */
// FUNC 0x0015A848 icon_slot_release
void icon_slot_release(int slot)
{
    McIconModel *model = g_iconSlot[slot];

    if (model != NULL) {
        if (g_iconSlotUsed[slot] != 0) {
            model->ready = 0;
            gfx_vsync(2);
        }
        g_iconSlotUsed[slot] = 0;
    }
}

// FUNC 0x0015A8A0 icon_slot_get
McIconModel *icon_slot_get(int slot)
{
    if (g_iconSlotUsed[slot] != 0)
        return g_iconSlot[slot];
    return NULL;
}

/* Builds icon heap `heap' over mem (MemorycardControl::assignIconHeaps). */
// FUNC 0x0015A8D0 icon_heap_assign
void icon_heap_assign(int heap, void *mem, int size)
{
    g_iconHeap[heap].reset();
    g_iconHeap[heap].assign(mem, size);
    g_iconHeapReady[heap] = 1;
}

// FUNC 0x0015A938 icon_heap_unassign
void icon_heap_unassign(int heap)
{
    if (g_iconHeapReady[heap] != 0) {
        g_iconHeap[heap].unassign();
        g_iconHeap[heap].reset();
        g_iconHeapReady[heap] = 0;
    }
}

/* Once both heaps are assigned: ICON_SLOT_SIZE bytes of each for the two
 * slots. */
// FUNC 0x0015A9A8 icon_slots_alloc
void icon_slots_alloc(void)
{
    int i;

    if (g_iconHeapReady[0] != 0 && g_iconHeapReady[1] != 0) {
        for (i = 0; i < 2; i++) {
            g_iconSlot[i] = (McIconModel *)g_iconHeap[i & 1].allocate(ICON_SLOT_SIZE, 0x10);
            g_iconSlotUsed[i] = 0;
        }
    }
}

// FUNC 0x0015AA48 icon_slots_free
void icon_slots_free(void)
{
    int i;

    if (g_iconHeapReady[0] != 0 && g_iconHeapReady[1] != 0) {
        for (i = 0; i < 2; i++) {
            g_iconHeap[i & 1].free(g_iconSlot[i]);
            g_iconSlot[i] = NULL;
            g_iconSlotUsed[i] = 0;
        }
    }
}

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft), a COP1 instruction egcs never
 * emits.  A volatile asm, as the game code had it (a scheduling barrier:
 * retail keeps the loop counter's decrement after it). */
static inline f32 icon_rsqrt(f32 num, f32 sq)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 r;

    __asm__ __volatile__("rsqrt.s %0, %1, %2" : "=f"(r) : "f"(num), "f"(sq));
    return r;
#else
    return num / boot_sqrtf(sq);
#endif
}

/* The lights of icon.sys (vec4 arrays) for the icon models: the directions
 * normalised (rsqrt.s), the colours and the ambient copied.  Returns out. */
// FUNC 0x0015AAE0 icon_light_init
McIconLight *icon_light_init(McIconLight *out, const f32 (*dir)[4], const f32 (*color)[4],
                             const f32 *ambient)
{
    int i;
    f32 x, inv;

    for (i = 0; i < 3; i++) {
        x = dir[i][0];
        inv = icon_rsqrt(1.0f, x * x + dir[i][1] * dir[i][1] + dir[i][2] * dir[i][2]);
        out->dir[i][0] = x * inv;
        out->dir[i][1] = dir[i][1] * inv;
        out->dir[i][2] = dir[i][2] * inv;
        out->color[i][0] = color[i][0];
        out->color[i][1] = color[i][1];
        out->color[i][2] = color[i][2];
    }
    out->ambient[0] = ambient[0];
    out->ambient[1] = ambient[1];
    out->ambient[2] = ambient[2];
    return out;
}

/* x clamped to [lo, hi]: COP1 max.s then min.s.  Volatile asms, as the game
 * code has them (like boot_fabsf): retail clamps r, g and b strictly in
 * source order and only then converts them (b first); with the ?: forms
 * (MAX_EXPR / MIN_EXPR under -ffast-math) sched1 interleaves the three. */
static inline f32 icon_clampf(f32 x, f32 lo, f32 hi)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 t;

    __asm__ __volatile__("max.s %0, %1, %2" : "=f"(t) : "f"(x), "f"(lo));
    __asm__ __volatile__("min.s %0, %1, %2" : "=f"(x) : "f"(t), "f"(hi));
    return x;
#else
    x = x > lo ? x : lo;
    return x < hi ? x : hi;
#endif
}

/* The colour of a vertex with normal n: ambient plus each light facing it,
 * times the vertex colour, clamped to 0..255; alpha 0x80. */
// FUNC 0x0015ABB0 icon_light_vertex
u32 icon_light_vertex(const McIconLight *light, const f32 *n, u32 rgba)
{
    f32 r = light->ambient[0];
    f32 g = light->ambient[1];
    f32 b = light->ambient[2];
    f32 facing;
    int i;

    for (i = 0; i < 3; i++) {
        facing = -(n[0] * light->dir[i][0] + n[1] * light->dir[i][1] + n[2] * light->dir[i][2]);
        if (0.0f < facing) {
            r += facing * light->color[i][0];
            g += facing * light->color[i][1];
            b += facing * light->color[i][2];
        }
    }
    r *= (f32)(rgba & 0xFF);
    g *= (f32)((rgba & 0xFF00) >> 8);
    b *= (f32)((rgba & 0xFF0000) >> 16);
    r = icon_clampf(r, 0.0f, 255.0f);
    g = icon_clampf(g, 0.0f, 255.0f);
    b = icon_clampf(b, 0.0f, 255.0f);
    {
        u32 c = (int)r | 0x80000000;

        return c | ((int)b << 16 | (int)g << 8);
    }
}

/* The quads of icon_model_render (strips of 4 vertices): the sprite's (u, v)
 * and (x, y, z), the background's (x, y, z).  The background's top is
 * retail's 0x41D33334, one ulp above the float nearest 26.4 (egcs makes
 * 0x41D33333 of 26.4f; 22.0f * 1.2f folds to 0x41D33334, perhaps the
 * original's form): written as 26.400002f, which egcs and GCC 14 round to
 * retail's value (an exact form, not necessarily the original). */
const f32 g_00320290[4][2] = {                  /* 0x00320290 */
    { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }
};
const f32 g_003202B0[4][3] = {                  /* 0x003202B0 */
    { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }
};
const f32 g_003202E0[4][3] = {                  /* 0x003202E0 */
    { -22.0f, -22.0f, -50.0f }, { 33.0f, -22.0f, -50.0f },
    { -22.0f, 26.400002f, -50.0f }, { 33.0f, 26.400002f, -50.0f }
};

/* Draws an icon model at (x, y), w x h, tinted (r, g, b, a) x 2.  `time' is
 * the animation time in seconds (MemorycardIconRenderInfo::time, T108): the
 * model turns at -30 degrees a second and is morphed by its animation at that
 * time.  The background gradient and the mesh go into a 256 x 256 colour
 * buffer with its own depth buffer, which is shrunk into a 128 x 128 one,
 * then drawn as a texture. */
// FUNC 0x0015AD48 icon_model_render
void icon_model_render(McIconModel *icon, f32 x, f32 y, f32 w, f32 h, f32 r, f32 g,
                       f32 b, f32 a, f32 time)
{
    if (icon->ready == 0)
        return;

    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    vu0_load_identity_regs();
    pgl_frustum(-11.0f, 11.0f, -11.0f, 11.0f, 1.0f, 100.0f, 50.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    vu0_load_identity_regs();
    pgl_push_attrib(PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B |
                    PGL_ATTR_VIEWPORT | PGL_ATTR_SCISSOR);   /* 0x887 */
    {
        GSBuffer frame(SCE_GS_PSMCT32, 0x100, 0x100);
        GSBuffer depth(&frame, SCE_GS_PSMZ24);

        frame.setDrawBuffer();
        depth.setZBuffer();
        gs_set_clear_color(0);
        gs_clear_buffers(PGL_CLEAR_COLOR | PGL_CLEAR_DEPTH);
        gfx_enable(PGL_CAP_VU1_PROG_33E);
        pgl_draw_strip(g_003202E0, NULL, icon->bgColor, NULL, 4);   /* the background */
        pgl_translate(0.0f, -10.0f, -50.0f);
        vu0_rotate_y(-time * 30.0f);
        icon_model_draw(icon, time);
        {
            GSBuffer small(SCE_GS_PSMCT32, 0x80, 0x80);

            /* frame -> small, filtered */
            frame.setTexture();
            small.setDrawBuffer();
            gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
            gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
            gfx_disable(PGL_CAP_DEPTH_TEST);
            gfx_disable(PGL_CAP_BLEND);
            pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
            gs_draw_textured_sprite_f(0.0f, 0.0f, 128.0f, 128.0f, 0.5f, 0.5f, 256.5f, 256.5f);
            /* back to the screen: small as a quad at (x, y) */
            pgl_pop_attrib();
            pgl_matrix_mode(PGL_MTX_PROJECTION);
            pgl_pop_matrix();
            pgl_matrix_mode(PGL_MTX_MODELVIEW);
            pgl_pop_matrix();
            pgl_push_matrix();
            gfx_enable(PGL_CAP_BLEND);
            gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 1.0f);
            pgl_translate(x, y, 0.0f);
            vu0_scale(w * 0.5f, h * 0.5f, 1.0f);
            pgl_color4f(r + r, g + g, b + b, a + a);
            small.setTexture();
            pgl_draw_strip(g_003202B0, NULL, NULL, g_00320290, 4);
            pgl_pop_matrix();
        }   /* ~GSBuffer: small, then depth and frame (their VRAM freed) */
    }
}

/* Draws the mesh of an icon model: the shapes morphed with the animation's
 * weights at `time' (12-bit fixed point, MMI pmulth / pmaddh) into the
 * scratchpad, then drawn in batches of 60 vertices. */
// FUNC 0x0015B0E0 icon_model_draw
/* The mesh loops are the original's MMI code: per vertex, the shapes'
 * (x, y, z, w) halfwords (unaligned ldl / ldr) times the pcpyh'd weights,
 * summed in HI / LO (pmulth, pmaddh), x and z taken from the result
 * register, y from pmflo.  Retail names the temporaries itself: the vertex
 * loads in $2, $3, ..., the sum in the next register, pmflo into $2, and its
 * loads and stores are never put in a delay slot (asm).  Under egcs this is
 * an exact form, not the original: one volatile asm per instruction with
 * register variables for those temporaries (the dst[-k] stores after the
 * increment give retail's -0x8000-biased base).  Case 8 applies pcpyh to
 * weight 0 three times, as retail does (harmless: pcpyh of a pcpyh'd value
 * is the same value).  GCC 14 keeps the C form below (64-bit weights: only
 * the low four halfwords are used). */
void icon_model_draw(McIconModel *icon, f32 time)
{
    s32 weights[8];
    McIconVertex *mesh, *dst;
    const u8 *src;                      /* the shapes' positions: 3 s16 per shape and vertex */
    const u32 *colors;
    const s16 *uvs;
    int left, count;

    if (icon->vertexCount <= 0)
        return;

    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gfx_enable(PGL_CAP_CULL_FACE);
    gfx_front_face(PGL_FRONT_CCW);
    pgl_push_matrix();
    vu0_scale(1.0f / 1024.0f, -1.0f / 1024.0f, -1.0f / 1024.0f);
    if (icon->textured != 0 && icon->texture != NULL) {
        gfx_enable(PGL_CAP_TEXTURE);
        g_002F4250.loadImage(SCE_GS_PSMCT16, 0x80, 0x80, icon->texture);
        gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REGION_CLAMP);
        gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_REGION_CLAMP);
        gfx_set_texture_region(PGL_REGION_U, 0, 0x7F);
        gfx_set_texture_region(PGL_REGION_V, 0, 0x7F);
        gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
        gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
        gfx_tex_param(PGL_TEXPARAM_MXL, 0);
        gfx_set_texture_function(PGL_TFX_MODULATE);
    } else {
        gfx_disable(PGL_CAP_TEXTURE);
    }

    icon_anim_weights(&icon->anim, weights, time);
    left = icon->vertexCount;
    mesh = (McIconVertex *)gfx_alloc_tail((left * 8 + 15) >> 4);
    dst = mesh;
    src = (const u8 *)icon->positions;

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    switch (icon->shapes) {
    case 1:
        /* Three vertices at a time, 8 bytes each: w is the next vertex's x. */
        do {
            u64 a, b, c;

            ICON_ULD(a, src + 0);
            ICON_ULD(b, src + 6);
            ICON_ULD(c, src + 12);
            dst += 3;
            ICON_SD(((u64 *)dst)[-3], a);
            src += 18;
            ICON_SD(((u64 *)dst)[-2], b);
            left -= 3;
            ICON_SD(((u64 *)dst)[-1], c);
        } while (left != 0);
        break;
    case 2: {
        u128 w0, w1;

        ICON_LW(w0, weights[0]);
        ICON_LW(w1, weights[1]);
        ICON_PCPYH(w0);
        ICON_PCPYH(w1);
        do {
            register u64 v0 __asm__("$2");
            register u64 v1 __asm__("$3");
            register u64 acc __asm__("$4");
            register u64 lo __asm__("$2");

            ICON_ULD(v0, src + 0);
            ICON_ULD(v1, src + 6);
            ICON_PMULTH(acc, v0, w0);
            ICON_PMADDH(acc, v1, w1);
            dst++;
            src += 12;
            ICON_PMFLO(lo);
            acc >>= 12;
            ICON_SH(dst[-1].x, acc);
            ICON_SH(dst[-1].y, lo >> 44);
            ICON_SH(dst[-1].z, acc >> 32);
        } while (--left != 0);
        break;
    }
    case 4: {
        u128 w0, w1, w2, w3;

        ICON_LW(w0, weights[0]);
        ICON_LW(w1, weights[1]);
        ICON_LW(w2, weights[2]);
        ICON_LW(w3, weights[3]);
        ICON_PCPYH(w0);
        ICON_PCPYH(w1);
        ICON_PCPYH(w2);
        ICON_PCPYH(w3);
        do {
            register u64 v0 __asm__("$2");
            register u64 v1 __asm__("$3");
            register u64 v2 __asm__("$4");
            register u64 v3 __asm__("$5");
            register u64 acc __asm__("$6");
            register u64 lo __asm__("$2");

            ICON_ULD(v0, src + 0);
            ICON_ULD(v1, src + 6);
            ICON_ULD(v2, src + 12);
            ICON_ULD(v3, src + 18);
            ICON_PMULTH(acc, v0, w0);
            ICON_PMADDH(acc, v1, w1);
            ICON_PMADDH(acc, v2, w2);
            ICON_PMADDH(acc, v3, w3);
            dst++;
            src += 24;
            ICON_PMFLO(lo);
            acc >>= 12;
            ICON_SH(dst[-1].x, acc);
            ICON_SH(dst[-1].y, lo >> 44);
            ICON_SH(dst[-1].z, acc >> 32);
        } while (--left != 0);
        break;
    }
    case 6: {
        u128 w0, w1, w2, w3, w4, w5;

        ICON_LW(w0, weights[0]);
        ICON_LW(w1, weights[1]);
        ICON_LW(w2, weights[2]);
        ICON_LW(w3, weights[3]);
        ICON_LW(w4, weights[4]);
        ICON_LW(w5, weights[5]);
        ICON_PCPYH(w0);
        ICON_PCPYH(w1);
        ICON_PCPYH(w2);
        ICON_PCPYH(w3);
        ICON_PCPYH(w4);
        ICON_PCPYH(w5);
        do {
            register u64 v0 __asm__("$2");
            register u64 v1 __asm__("$3");
            register u64 v2 __asm__("$4");
            register u64 v3 __asm__("$5");
            register u64 v4 __asm__("$6");
            register u64 v5 __asm__("$7");
            register u64 acc __asm__("$8");
            register u64 lo __asm__("$2");

            ICON_ULD(v0, src + 0);
            ICON_ULD(v1, src + 6);
            ICON_ULD(v2, src + 12);
            ICON_ULD(v3, src + 18);
            ICON_ULD(v4, src + 24);
            ICON_ULD(v5, src + 30);
            ICON_PMULTH(acc, v0, w0);
            ICON_PMADDH(acc, v1, w1);
            ICON_PMADDH(acc, v2, w2);
            ICON_PMADDH(acc, v3, w3);
            ICON_PMADDH(acc, v4, w4);
            ICON_PMADDH(acc, v5, w5);
            dst++;
            src += 36;
            ICON_PMFLO(lo);
            acc >>= 12;
            ICON_SH(dst[-1].x, acc);
            ICON_SH(dst[-1].y, lo >> 44);
            ICON_SH(dst[-1].z, acc >> 32);
        } while (--left != 0);
        break;
    }
    case 8: {
        u128 w0, w1, w2, w3, w4, w5, w6, w7;

        ICON_LW(w0, weights[0]);
        ICON_LW(w1, weights[1]);
        ICON_LW(w2, weights[2]);
        ICON_LW(w3, weights[3]);
        ICON_LW(w4, weights[4]);
        ICON_LW(w5, weights[5]);
        ICON_LW(w6, weights[6]);
        ICON_LW(w7, weights[7]);
        ICON_PCPYH(w0);
        ICON_PCPYH(w0);
        ICON_PCPYH(w0);
        ICON_PCPYH(w1);
        ICON_PCPYH(w2);
        ICON_PCPYH(w3);
        ICON_PCPYH(w4);
        ICON_PCPYH(w5);
        ICON_PCPYH(w6);
        ICON_PCPYH(w7);
        do {
            register u64 v0 __asm__("$2");
            register u64 v1 __asm__("$3");
            register u64 v2 __asm__("$4");
            register u64 v3 __asm__("$5");
            register u64 v4 __asm__("$6");
            register u64 v5 __asm__("$7");
            register u64 v6 __asm__("$8");
            register u64 v7 __asm__("$9");
            register u64 acc __asm__("$10");
            register u64 lo __asm__("$2");

            ICON_ULD(v0, src + 0);
            ICON_ULD(v1, src + 6);
            ICON_ULD(v2, src + 12);
            ICON_ULD(v3, src + 18);
            ICON_ULD(v4, src + 24);
            ICON_ULD(v5, src + 30);
            ICON_ULD(v6, src + 36);
            ICON_ULD(v7, src + 42);
            ICON_PMULTH(acc, v0, w0);
            ICON_PMADDH(acc, v1, w1);
            ICON_PMADDH(acc, v2, w2);
            ICON_PMADDH(acc, v3, w3);
            ICON_PMADDH(acc, v4, w4);
            ICON_PMADDH(acc, v5, w5);
            ICON_PMADDH(acc, v6, w6);
            ICON_PMADDH(acc, v7, w7);
            dst++;
            src += 48;
            ICON_PMFLO(lo);
            acc >>= 12;
            ICON_SH(dst[-1].x, acc);
            ICON_SH(dst[-1].y, lo >> 44);
            ICON_SH(dst[-1].z, acc >> 32);
        } while (--left != 0);
        break;
    }
    default:
        /* (other shape counts: the buffer is drawn as it is) */
        break;
    }
#else
    switch (icon->shapes) {
    case 1: {
        /* Three vertices at a time, 8 bytes each: w is the next vertex's x. */
        u64 *d = (u64 *)dst;
        int i = left;
        do {
            d[0] = load64u(src + 0);
            d[1] = load64u(src + 6);
            d[2] = load64u(src + 12);
            d += 3;
            src += 18;
            i -= 3;
        } while (i != 0);
        break;
    }
    case 2: {
        u64 w0 = mmi_pcpyh(weights[0]);
        u64 w1 = mmi_pcpyh(weights[1]);
        int i = left;
        do {
            u64 acc, lo;
            __asm__("pmulth %0, %2, %4\n\t"
                    "pmaddh %0, %3, %5\n\t"
                    "pmflo  %1"
                    : "=&r"(acc), "=r"(lo)
                    : "r"(load64u(src)), "r"(load64u(src + 6)), "r"(w0), "r"(w1)
                    : "hi", "lo");
            dst->x = (s16)(acc >> 12);
            dst->y = (s16)(lo >> 44);
            dst->z = (s16)((acc >> 12) >> 32);
            dst++;
            src += 12;
        } while (--i != 0);
        break;
    }
    case 4: {
        u64 w0 = mmi_pcpyh(weights[0]);
        u64 w1 = mmi_pcpyh(weights[1]);
        u64 w2 = mmi_pcpyh(weights[2]);
        u64 w3 = mmi_pcpyh(weights[3]);
        int i = left;
        do {
            u64 acc, lo;
            __asm__("pmulth %0, %2, %6\n\t"
                    "pmaddh %0, %3, %7\n\t"
                    "pmaddh %0, %4, %8\n\t"
                    "pmaddh %0, %5, %9\n\t"
                    "pmflo  %1"
                    : "=&r"(acc), "=r"(lo)
                    : "r"(load64u(src)), "r"(load64u(src + 6)), "r"(load64u(src + 12)),
                      "r"(load64u(src + 18)), "r"(w0), "r"(w1), "r"(w2), "r"(w3)
                    : "hi", "lo");
            dst->x = (s16)(acc >> 12);
            dst->y = (s16)(lo >> 44);
            dst->z = (s16)((acc >> 12) >> 32);
            dst++;
            src += 24;
        } while (--i != 0);
        break;
    }
    case 6: {
        u64 w0 = mmi_pcpyh(weights[0]);
        u64 w1 = mmi_pcpyh(weights[1]);
        u64 w2 = mmi_pcpyh(weights[2]);
        u64 w3 = mmi_pcpyh(weights[3]);
        u64 w4 = mmi_pcpyh(weights[4]);
        u64 w5 = mmi_pcpyh(weights[5]);
        int i = left;
        do {
            u64 acc, lo;
            __asm__("pmulth %0, %2, %8\n\t"
                    "pmaddh %0, %3, %9\n\t"
                    "pmaddh %0, %4, %10\n\t"
                    "pmaddh %0, %5, %11\n\t"
                    "pmaddh %0, %6, %12\n\t"
                    "pmaddh %0, %7, %13\n\t"
                    "pmflo  %1"
                    : "=&r"(acc), "=r"(lo)
                    : "r"(load64u(src)), "r"(load64u(src + 6)), "r"(load64u(src + 12)),
                      "r"(load64u(src + 18)), "r"(load64u(src + 24)), "r"(load64u(src + 30)),
                      "r"(w0), "r"(w1), "r"(w2), "r"(w3), "r"(w4), "r"(w5)
                    : "hi", "lo");
            dst->x = (s16)(acc >> 12);
            dst->y = (s16)(lo >> 44);
            dst->z = (s16)((acc >> 12) >> 32);
            dst++;
            src += 36;
        } while (--i != 0);
        break;
    }
    case 8: {
        u64 w0 = mmi_pcpyh(weights[0]);
        u64 w1 = mmi_pcpyh(weights[1]);
        u64 w2 = mmi_pcpyh(weights[2]);
        u64 w3 = mmi_pcpyh(weights[3]);
        u64 w4 = mmi_pcpyh(weights[4]);
        u64 w5 = mmi_pcpyh(weights[5]);
        u64 w6 = mmi_pcpyh(weights[6]);
        u64 w7 = mmi_pcpyh(weights[7]);
        int i = left;
        do {
            u64 acc, lo;
            __asm__("pmulth %0, %2, %10\n\t"
                    "pmaddh %0, %3, %11\n\t"
                    "pmaddh %0, %4, %12\n\t"
                    "pmaddh %0, %5, %13\n\t"
                    "pmaddh %0, %6, %14\n\t"
                    "pmaddh %0, %7, %15\n\t"
                    "pmaddh %0, %8, %16\n\t"
                    "pmaddh %0, %9, %17\n\t"
                    "pmflo  %1"
                    : "=&r"(acc), "=r"(lo)
                    : "r"(load64u(src)), "r"(load64u(src + 6)), "r"(load64u(src + 12)),
                      "r"(load64u(src + 18)), "r"(load64u(src + 24)), "r"(load64u(src + 30)),
                      "r"(load64u(src + 36)), "r"(load64u(src + 42)),
                      "r"(w0), "r"(w1), "r"(w2), "r"(w3), "r"(w4), "r"(w5), "r"(w6), "r"(w7)
                    : "hi", "lo");
            dst->x = (s16)(acc >> 12);
            dst->y = (s16)(lo >> 44);
            dst->z = (s16)((acc >> 12) >> 32);
            dst++;
            src += 48;
        } while (--i != 0);
        break;
    }
    default:
        /* (other shape counts: the buffer is drawn as it is) */
        break;
    }
#endif

    /* 20 triangles at a time */
    left = icon->vertexCount;
    uvs = icon->uvs;
    colors = icon->colors;
    do {
        count = left < 61 ? left : 60;
        left -= count;
        tri_draw16(mesh, NULL, colors, uvs, count / 3);
        uvs += 60 * 2;
        colors += 60;
        mesh += 60;
    } while (left != 0);
    pgl_pop_matrix();
    gfx_disable(PGL_CAP_CULL_FACE);
}

/* The background corner colours (icon.sys: r, g, b ints) with alpha. */
// FUNC 0x0015B650 icon_model_set_bg
void icon_model_set_bg(McIconModel *icon, const int *c0, const int *c1, const int *c2,
                       const int *c3, int alpha)
{
    u32 a = alpha << 24;

    /* RGBA8, R in the low byte */
    icon->bgColor[0] = c0[0] | a | (c0[2] << 16 | c0[1] << 8);
    icon->bgColor[1] = c1[0] | a | (c1[2] << 16 | c1[1] << 8);
    icon->bgColor[2] = c2[0] | a | (c2[2] << 16 | c2[1] << 8);
    icon->bgColor[3] = c3[0] | a | (c3[2] << 16 | c3[1] << 8);
}

/* The weight of frame `frame' at t: its keys interpolated linearly (before
 * the first key: the first value; after the last: the last value; 1.0 for a
 * frame out of range or without keys). */
// FUNC 0x0015B6E8 icon_anim_key_value
f32 icon_anim_key_value(McIconAnim *anim, int frame, f32 t)
{
    McIconFrame *fr;
    McIconKey *key, *prev;
    int keyCount, i;
    f32 u;

    if (!(frame < anim->frameCount))
        return 1.0f;
    fr = &anim->frames[frame];
    keyCount = fr->keyCount;
    if (keyCount == 0)
        return 1.0f;
    for (i = 0; i < keyCount; i++) {
        key = &fr->keys[i];
        if (t < key->time) {
            if (i == 0)
                return key->value;
            prev = &fr->keys[i - 1];
            u = (t - prev->time) / (key->time - prev->time);
            return prev->value * (1.0f - u) + key->value * u;
        }
    }
    return fr->keys[keyCount - 1].value;
}

/* The animation frame at `time' (seconds: 60 frames a second, times the
 * animation's speed, plus its play offset), modulo the frame length. */
// FUNC 0x0015B7C0 icon_anim_time
f32 icon_anim_time(McIconAnim *anim, f32 time)
{
    f32 t = time * anim->speed * 60.0f + anim->offset;

    return t - (f32)((int)(t / (f32)anim->frameLength) * anim->frameLength);
}

/* The weights of the 8 shapes at `time', scaled to a sum of 4096 (all on
 * shape 0 if the sum is not positive). */
// FUNC 0x0015B810 icon_anim_weights
void icon_anim_weights(McIconAnim *anim, s32 *out, f32 time)
{
    f32 weight[8];
    f32 t, value, sum, scale;
    int i;

    t = icon_anim_time(anim, time);
    for (i = 7; i >= 0; i--)
        weight[i] = 0.0f;
    sum = 0.0f;
    for (i = 0; i < anim->frameCount; i++) {
        value = icon_anim_key_value(anim, i, t);
        sum += value;
        weight[anim->frames[i].shape] = value;
    }
    if (sum <= 0.0f) {
        out[0] = 0x1000;
        for (i = 1; i < 8; i++)
            out[i] = 0;
        return;
    }
    scale = 4096.0f / sum;
    for (i = 0; i < 8; i++)
        out[i] = (s32)(weight[i] * scale);
}

/* The two icon heaps, default-constructed: Allocator(0), best fit (.data
 * 0x002A8328: egcs puts a public object that has a constructor in .data,
 * zero-filled). */
pdistd::Allocator g_iconHeap[2];                /* 0x002A8328 */

/* The heaps' and slots' state: no initialiser (zeros at the end of the
 * .sdata, in memorycard_icon_internal.h's declaration order, as retail's 0x003518D0..). */
bool         g_iconHeapReady[2];                /* 0x003518D0 */
int          g_iconSlotUsed[2];                 /* 0x003518D8 */
McIconModel *g_iconSlot[2];                     /* 0x003518E0 */

/* Compiler-generated: egcs's static destructor and constructor of
 * g_iconHeap (.dtors entry 0x0028F1EC, .ctors entry 0x0028F09C), named after
 * the file's first global function and emitted at the end of the unit, the
 * destructor first.  The destructor runs the two heaps' virtual destructors
 * last first (behind a never-taken NULL test of the array); the constructor
 * is the array loop Allocator(0) x 2, its landing pad destroying the heaps
 * already built. */
// FUNC 0x0015B970 T077_static_fini (_GLOBAL_$D$icon_model_size__FPCUi)
/* implicit: ~Allocator (the static destructor of g_iconHeap) */

// FUNC 0x0015B9D8 T077_static_init (_GLOBAL_$I$icon_model_size__FPCUi)
/* implicit: Allocator (the static constructor of g_iconHeap) */
