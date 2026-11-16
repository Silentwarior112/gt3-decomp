/* model_set_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u102,
 * 0x00224600..0x00227378: translation units T451..T456, the model formats.
 *
 *   T451  ModelSet, the "GTM1" model file: relocation, the CLUT animation
 *         hooks, and the draw interpreter (each part is a byte stream of
 *         commands: draw a prebuilt packet or another part, select a
 *         texture, frustum test, LOD and switch branches, matrix, render
 *         state and fog colour commands).  The LOD / switch branches are
 *         decided by the global ModelSet::Callback.
 *   T452  the model's animation tracks (ModelSet +0x3C): typed tracks that
 *         write an angle, a frame number or a position into the command
 *         streams.
 *   T453  VisionList: a grid of cells with lists of points (the parts
 *         visible from each), nearest point.
 *   T454  SpriteModel (? name): a model of camera-facing quads.
 *   T455  CarModelInfo relocation and the tyre table lookup.
 *   T456  CarModelFile relocation (info, body ModelSet, tyre, wheel).
 *
 * The classes and records are in the canonical headers of the own
 * translation units (include/game/T451, T453..T456); this header adds what
 * only this unit uses: T452's track records and functions (T451
 * forward-declares AnimSet), the .sdata / .sbss / .bss words of T451 and
 * T454, named constants of the graphics library calls, and small helpers.
 *
 * All file formats store offsets relative to a "base" word that holds the
 * address the file was built at; relocating adds (this - base) to each
 * offset and stores this into base.
 */
#ifndef U102_H
#define U102_H

#include "types.h"
#include "cxxrt.h"                      /* __rtti_user */

/* own translation units */
#include "engine/gfx/T451_model_set.h"        /* ModelSet (+ Callback), ModelMaterial, ModelSphere, modelset_* */
#include "game/course/T453_vision_list.h"      /* VisionList, VisionPoint, vision_* */
#include "engine/gfx/T454_sprite_model.h"     /* SpriteModel, Sprite, SpriteEntry, SpritePart, spriteset_* */
#include "game/car/model/T455_car_model_info.h"   /* CarModelInfo, carmodelinfo_* */
#include "game/car/model/T456_car_model_file.h"   /* CarModelFile, carmodelfile_* */

/* outside the unit */
#include "engine/pgl/G001_flare.h"            /* the pgl graphics library: Tex1Image, GsTexRegs, GsMaterial, VifModel, GsVec4 */
#include "game/car/model/T447_tire_model_base.h"  /* TireFile2, WheelFile2, tire_file_relocate, wheel_file_relocate */
#include "engine/gfx/T512_render_context.h"   /* g_002F4250 (= g_002F4230.vram, the texture manager) */
#include "engine/gfx/T513_vram_allocator.h"   /* VramAllocator::loadTex1 */

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */
/* A 16-byte VU0 transfer buffer (lqc2 / sqc2), read as floats or as the
 * raw words (the sign of w of the frustum test). */
typedef union {
    f32 f[4];
    u32 u[4];
    s32 i[4];
} __attribute__((aligned(16))) U102Vec;

/* Unaligned reads of the command streams (T451).  The original reads them
 * with the assembler's unaligned macros in volatile inline asm, expanded at
 * each use: ulw (lwl / lwr; a float goes through $2 with the mtc1 in the
 * same asm), ulhu / ulh (lbu or lb, lbu $at, sll, or: the macro's $at shows
 * in retail; egcs itself reads packed fields as lbu / lbu / dsll / or).
 * Macros, not inline functions: an asm records its source line, and two
 * reads at different lines are not cross-jumped (modelset_exec keeps the
 * equal tails of commands 0x06 / 0x07 / 0x09 apart).  A volatile asm is a
 * scheduling barrier, which keeps each read's mtc1 and the store of the
 * value ahead of the pointer increment as retail.  GCC 14 (and NON_MATCHING)
 * reads the bytes in C. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define u102_rd_f32(p) ({ f32 v_;                                               \
        __asm__ volatile("ulw $2, %1\n\tmtc1 $2, %0"                            \
                         : "=f"(v_) : "m"(*(const u32 *)(p)) : "$2");           \
        v_; })
#define u102_rd_u32(p) ({ u32 v_;                                               \
        __asm__ volatile("ulw %0, %1" : "=r"(v_) : "m"(*(const u32 *)(p)));     \
        v_; })
#define u102_rd_u16(p) ({ int v_;                                               \
        __asm__ volatile("ulhu %0, %1" : "=r"(v_) : "m"(*(const u16 *)(p)));    \
        v_; })
#define u102_rd_s16(p) ({ int v_;                                               \
        __asm__ volatile("ulh %0, %1" : "=r"(v_) : "m"(*(const s16 *)(p)));     \
        v_; })
#else
struct __attribute__((packed)) U102UnalignedF32 { f32 v; };
struct __attribute__((packed)) U102UnalignedU32 { u32 v; };
static inline f32 u102_rd_f32(const u8 *p) { return ((const U102UnalignedF32 *)p)->v; }
static inline u32 u102_rd_u32(const u8 *p) { return ((const U102UnalignedU32 *)p)->v; }
static inline int u102_rd_u16(const u8 *p) { return (p[1] << 8) | p[0]; }
static inline int u102_rd_s16(const u8 *p) { return ((s8)p[1] << 8) | p[0]; }  /* a signed offset */
#endif
/* Read an operand and step over it (the interpreter's usual form). */
#define U102_GET_F32(p) ({ f32 g_ = u102_rd_f32(p); (p) += 4; g_; })
#define U102_GET_U32(p) ({ u32 g_ = u102_rd_u32(p); (p) += 4; g_; })
#define U102_GET_U16(p) ({ int g_ = u102_rd_u16(p); (p) += 2; g_; })
/* A branch of the command stream: p += the offset (read at p), a
 * statement macro: its loop notes align the label after it (retail's nops
 * in modelset_exec and modelset_run_part). */
#define U102_BRANCH(p, off) do { (p) += (off); } while (0)

/* Unaligned copies of 4 / 2 bytes (lbu / sb), T452's writes into the
 * command streams.  Statement macros (`do { } while (0)'): retail's loops
 * keep the index increment after the copy, and the source pointer is a
 * local of the block (its first byte is addressed from the base). */
#define U102_COPY4(dst, src) do {                                           \
        u8 *d_ = (u8 *)(dst);                                               \
        const u8 *s_ = (const u8 *)(src);                                   \
        d_[0] = s_[0]; d_[1] = s_[1]; d_[2] = s_[2]; d_[3] = s_[3];         \
    } while (0)
#define U102_COPY2(dst, src) do {                                           \
        u8 *d_ = (u8 *)(dst);                                               \
        const u8 *s_ = (const u8 *)(src);                                   \
        d_[0] = s_[0]; d_[1] = s_[1];                                       \
    } while (0)

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft). */
static inline f32 u102_rsqrt(f32 num, f32 sq)
{
    f32 r;
    __asm__("rsqrt.s %0, %1, %2" : "=f"(r) : "f"(num), "f"(sq));
    return r;
}

/* p += delta bytes (a stored offset becomes a pointer) */
#define U102_RELOC(p, d) ((p) = (__typeof__(p))((u8 *)(p) + (d)))
/* The same written delta + p (T453's vision_relocate: addu delta, p) */
#define U102_RELOC_DP(p, d) ((p) = (__typeof__(p))((d) + (u32)(p)))
/* The same as a statement macro (`do { } while (0)'), as T451's and T454's
 * relocators have it: its loop notes keep each store ahead of the loads
 * after it, align the label that follows (retail's nops) and weight
 * delta's references, which decides the registers. */
#define U102_RELOC_STMT(p, d) do { U102_RELOC(p, d); } while (0)
/* if (p != NULL) U102_RELOC_STMT(p, d), as one more statement block
 * (T454's optional offsets) */
#define U102_RELOC_NZ(p, d) do { if ((p) != NULL) U102_RELOC_STMT(p, d); } while (0)

/* ======================================================================== */
/* Constants of the graphics library calls                                 */
/* ======================================================================== */

/* The gfx_enable switches (PGL_CAP_*; the frustum test, command 0x0A, turns
 * PGL_CAP_VU1_PROG_33E on only when a point is beyond the guard band, so
 * the VU1 program 0x33E is the clipping one), the depth / alpha test and
 * fail values (gfx_depth_func(PGL_DEPTHFUNC_LEQUAL): TEST.ZTST GEQUAL, the GS
 * keeping the greater Z; gfx_dest_alpha_func(PGL_ATEST_GEQUAL): TEST.DATM
 * 1) and the blend modes: G001_flare.h. */

/* pgl_set_vec_param / pgl_set_scalar_param kinds (u115): the two vectors
 * sent with lighting mode 3 (the environment map). */
enum {
    PGL_PARAM_ENV_B00 = 2,              /* GsContext.qB00 */
    PGL_PARAM_ENV_B10 = 3               /* GsContext.qB10 */
};

/* ======================================================================== */
/* T452: animation tracks (ModelSet +0x3C)                                  */
/* ======================================================================== */
/* Track types.  1..4 are handled alike (what tells them apart is not in
 * this unit). */
enum {
    ANIM_ANGLE_1  = 1,                  /* 1..4: an angle in degrees, wrapped at 360 */
    ANIM_ANGLE_2  = 2,
    ANIM_ANGLE_3  = 3,
    ANIM_ANGLE_4  = 4,
    ANIM_FRAME    = 5,                  /* a 16-bit frame number in [min, max) */
    ANIM_BLEND    = 6,                  /* a position in [0, 1) over the frames min..max */
    ANIM_CYCLE    = 7                   /* a value in [0, 1) */
};

/* The common head of a track.  A track writes its value into nTargets
 * places of the command streams (unaligned: byte copies). */
struct AnimTrack {
    u16 type;                               /* 0x00 ANIM_* */
    u16 nTargets;                           /* 0x02 */
    f32 speed;                              /* 0x04 per unit of dt */
};
/* types 1..4 (degrees, wraps at 360) and 7 (wraps in [0, 1)) */
struct AnimTrackF {
    u16 type;                               /* 0x00 */
    u16 nTargets;                           /* 0x02 */
    f32 speed;                              /* 0x04 */
    f32 start;                              /* 0x08 */
    f32 cur;                                /* 0x0C */
    u8 *targets[1];                         /* 0x10 [nTargets] */
};
/* types 5 (a 16-bit frame) and 6 (a fraction f32 at +0, a 16-bit frame at +5) */
struct AnimTrackI {
    u16 type;                               /* 0x00 */
    u16 nTargets;                           /* 0x02 */
    f32 speed;                              /* 0x04 */
    u16 min;                                /* 0x08 first frame */
    u16 max;                                /* 0x0A end frame */
    f32 initial;                            /* 0x0C type 5: a frame, type 6: a position */
    f32 cur;                                /* 0x10 */
    u8 *targets[1];                         /* 0x14 [nTargets] */
};
/* ModelSet::anim (T451 forward-declares it). */
struct AnimSet {
    u32        count;                       /* 0x00 */
    AnimTrack *tracks[1];                   /* 0x04 [count] */
};

void anim_relocate_targets(void **p, u32 n, int delta); /* 0x00225D90 */
void anim_angle_relocate(AnimTrackF *t, int delta);     /* 0x00225DC0 */
void anim_angle_reset(AnimTrackF *t);                   /* 0x00225DE8 */
void anim_angle_step(AnimTrackF *t, f32 dt);            /* 0x00225E58 */
void anim_frame_relocate(AnimTrackI *t, int delta);     /* 0x00225F18 */
void anim_frame_reset(AnimTrackI *t);                   /* 0x00225F40 */
void anim_frame_step(AnimTrackI *t, f32 dt);            /* 0x00225FE0 */
void anim_blend_relocate(AnimTrackI *t, int delta);     /* 0x002260C0 */
void anim_blend_reset(AnimTrackI *t);                   /* 0x002260E8 */
void anim_blend_step(AnimTrackI *t, f32 dt);            /* 0x002261D8 */
void anim_cycle_relocate(AnimTrackF *t, int delta);     /* 0x00226378 */
void anim_cycle_reset(AnimTrackF *t);                   /* 0x002263A0 */
void anim_cycle_step(AnimTrackF *t, f32 dt);            /* 0x00226410 */
void animset_relocate(AnimSet *a, int delta);           /* 0x00226518 */
void animset_reset(AnimSet *a);                         /* 0x00226640 */
void animset_step(AnimSet *a, f32 dt);                  /* 0x00226748 */

/* ======================================================================== */
/* T451 .sdata (defined in T451)                                            */
/* ======================================================================== */
extern ModelSet::Callback *g_00352AD4;      /* 0x00352AD4 gp-0x4F1C the callback (NULL: none) */
extern int  g_00352AD8;                     /* 0x00352AD8 gp-0x4F18 the frustum test also culls on clip flag 0x10 (+z, beyond the far plane) */
extern int  g_00352ADC;                     /* 0x00352ADC gp-0x4F14 (=1) the depth test (PGL_CAP_DEPTH_TEST) allowed: modelset_set_state5 */
extern f32  g_00352AE0;                     /* 0x00352AE0 gp-0x4F10 (=200.0) model LOD size */
extern int  g_00352AE4;                     /* 0x00352AE4 gp-0x4F0C texture sets by LOD on */
extern f32  g_00352AE8;                     /* 0x00352AE8 gp-0x4F08 (=400.0) texture set LOD size */
extern int  g_00352AEC;                     /* 0x00352AEC gp-0x4F04 (=-1) the forced texture set, < 0: by size */
extern int  g_00352AF0;                     /* 0x00352AF0 gp-0x4F00 packets drawn (a counter) */
/* T451's .sbss words g_00353258 / g_0035325C and T454's .bss g_0049D1A0 are
 * file statics of those files (retail has them in .sbss / .bss: a C++
 * global without an initialiser would be zeros in .sdata / .data). */

#endif /* U102_H */
