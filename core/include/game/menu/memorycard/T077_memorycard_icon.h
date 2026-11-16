/* T077_memorycard_icon.h - the 3D save icons of the memory card screens
 * (translation unit T077, unit u032), 0x00159EE8..0x0015BA98.
 *
 * A PS2 save icon (.ico: magic 0x00010000, shapes, texture type, vertices,
 * animations, texture) is checked (icon_model_size 0x00159EE8) and unpacked
 * (icon_model_build 0x0015A0E0) into a McIconModel: morph shapes, UVs,
 * colours lit by icon.sys's lights, the first animation, the 128 x 128
 * texture (RLE or raw).  A model lives in one of two pdistd::Allocator heaps
 * (g_iconHeap 0x002A8328, assigned by MemorycardControl over its work
 * buffer, T068) or in one of two fixed 0x25800-byte slots taken from them.
 * icon_model_render draws the background gradient and the morphed mesh into
 * a 256 x 256 GS buffer, then that buffer as a sprite.  The static
 * constructor / destructor (.ctors 0x0028F09C / .dtors 0x0028F1EC) build /
 * destroy the two heaps.  The other functions of T077 (the unpacking, the
 * lighting of a vertex, the animation weights) are used only here.
 *
 * (The plan put globals of other units here: the counts of the setting
 * menu's tables, g_00351950 / 70 / 78 / 94, now declared with their tables
 * in T081 .. T084, whose .sdata holds them; g_00352EE8, now in T488.) */
#ifndef GAME_T077_MEMORYCARD_ICON_H
#define GAME_T077_MEMORYCARD_ICON_H

#include "game/game.h"

#ifdef __cplusplus                          /* a C++ translation unit (C++ linkage) */


/* The lights of icon.sys as the icon models use them, built by
 * icon_light_init from icon.sys's vec4 arrays.  0x54 bytes (the callers keep
 * it in a 0x60-byte buffer: u023's 0x60 is that buffer). */
struct McIconLight {
    f32 dir[3][3];                          /* 0x00 the light directions, normalised */
    f32 color[3][3];                        /* 0x24 */
    f32 ambient[3];                         /* 0x48 */
};
EGCS_SIZE(McIconLight, 0x54);

/* One key of an animation frame: the weight value of the frame's shape at
 * time. */
struct McIconKey {
    f32 time;                               /* 0x0 */
    f32 value;                              /* 0x4 */
};
EGCS_SIZE(McIconKey, 0x8);

/* One animation frame: the keys of one shape's weight. */
struct McIconFrame {
    int        shape;                       /* 0x0 */
    int        keyCount;                    /* 0x4 */
    McIconKey *keys;                        /* 0x8 */
};
EGCS_SIZE(McIconFrame, 0xC);

/* The (first) animation of an icon (McIconModel +0x24). */
struct McIconAnim {
    int          frameLength;               /* 0x00 */
    f32          speed;                     /* 0x04 */
    f32          offset;                    /* 0x08 the play offset (an int in the file) */
    int          frameCount;                /* 0x0C */
    McIconFrame *frames;                    /* 0x10 */
};
EGCS_SIZE(McIconAnim, 0x14);

/* An unpacked icon (? name).  0x50 bytes of header; the arrays follow at
 * +0x50 in the same block (positions, uvs, colors, frames, keys, texture,
 * each 16-aligned). */
struct McIconModel {
    int        refCount;                    /* 0x00 icon_model_release frees it below 0 */
    int        ready;                       /* 0x04 drawn only when nonzero */
    int        shapes;                      /* 0x08 morph shapes: 1, 2, 4, 6 or 8 */
    int        flag0C;                      /* 0x0C bit 0 of the texture type (? meaning) */
    /* A bool (4 bytes in egcs): icon_model_build's load of it is hoisted
     * above its int stores into the frames (another alias set), as in
     * retail. */
    bool       textured;                    /* 0x10 bit 2 of the texture type */
    int        vertexCount;                 /* 0x14 a multiple of 3 */
    s16       *positions;                   /* 0x18 vertexCount x shapes x (x, y, z) */
    s16       *uvs;                         /* 0x1C vertexCount x (u, v) */
    u32       *colors;                      /* 0x20 vertexCount lit colours, alpha 0x80 */
    McIconAnim anim;                        /* 0x24 */
    u16       *texture;                     /* 0x38 128 x 128 16-bit texels (bit 15 set), or NULL */
    int        heap;                        /* 0x3C the g_iconHeap it was allocated from */
    u32        bgColor[4];                  /* 0x40 the background's corner colours (RGBA8) */
};
EGCS_SIZE(McIconModel, 0x50);

/* icon_model_create's *error (T108's readIcon: NOT_ICON marks the icon
 * prepared, NO_MEMORY leaves it to be asked again; ? names). */
enum {
    ICON_OK              = 0,
    ICON_ERROR_NOT_ICON  = 1,               /* not an icon file (or not unpackable) */
    ICON_ERROR_NO_MEMORY = 2                /* the heap is not assigned, or full */
};

/* An icon model in an icon heap.  *error: 0 done, 1 not an icon (or not
 * unpackable), 2 no heap / no memory.  unused is never read; the heap is an
 * uninitialised local of the original (the caller's $s1 != 0: heap 1 from
 * the only caller, T108 MemorycardOtherPS2File). */
McIconModel *icon_model_create(int unused, const u32 *ico, const McIconLight *light,
                               int *error);     /* 0x0015A648 */
/* Drops a reference; below 0 the model is hidden, (unless noWait) two
 * frames are waited for, and it is freed. */
void         icon_model_release(McIconModel *icon, int noWait); /* 0x0015A738 */
/* An icon model in fixed slot `slot' (replacing the one there); NULL if the
 * slot has no memory or the icon does not fit / is not one. */
McIconModel *icon_slot_create(int slot, const u32 *ico, const McIconLight *light); /* 0x0015A7A0 */
/* Empties the slot (hiding its model and waiting two frames if it had one). */
void         icon_slot_release(int slot);   /* 0x0015A848 */
McIconModel *icon_slot_get(int slot);       /* 0x0015A8A0 NULL when the slot is empty */
/* Builds icon heap `heap' over [mem, mem + size) (MemorycardControl::
 * assignIconHeaps). */
void         icon_heap_assign(int heap, void *mem, int size); /* 0x0015A8D0 */
void         icon_heap_unassign(int heap);  /* 0x0015A938 */
/* Once both heaps are assigned: 0x25800 bytes of each for the two slots. */
void         icon_slots_alloc(void);        /* 0x0015A9A8 */
void         icon_slots_free(void);         /* 0x0015AA48 */
/* The lights of icon.sys (vec4 arrays): the directions normalised, the
 * colours and the ambient copied.  Returns out. */
McIconLight *icon_light_init(McIconLight *out, const f32 (*dir)[4], const f32 (*color)[4],
                             const f32 *ambient); /* 0x0015AAE0 */
/* Draws the model at (x, y), w x h, tinted (r, g, b, a) x 2, at time (the
 * animation time in seconds, MemorycardIconRenderInfo::time): turned by
 * -30 degrees a second and morphed at that time; nothing unless ready. */
void         icon_model_render(McIconModel *icon, f32 x, f32 y, f32 w, f32 h, f32 r, f32 g,
                               f32 b, f32 a, f32 time); /* 0x0015AD48 */
/* The mesh: the shapes morphed with the animation's weights at time into
 * the scratchpad, drawn in batches of 60 vertices (icon_model_render's). */
void         icon_model_draw(McIconModel *icon, f32 time); /* 0x0015B0E0 */
/* The background corner colours (icon.sys: r, g, b ints) with alpha. */
void         icon_model_set_bg(McIconModel *icon, const int *c0, const int *c1, const int *c2,
                               const int *c3, int alpha); /* 0x0015B650 */
/* The static destructor / constructor of g_iconHeap, 0x0015B970 (.dtors) /
 * 0x0015B9D8 (.ctors), are egcs's _GLOBAL_$D$ / _GLOBAL_$I$ functions of
 * T077 (compiler-generated from g_iconHeap's definition; nothing calls
 * them). */

#endif /* __cplusplus */

#endif /* GAME_T077_MEMORYCARD_ICON_H */
