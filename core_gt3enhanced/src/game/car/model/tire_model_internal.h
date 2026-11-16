/* tire_model_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u101,
 * 0x00221E08..0x00224600: translation units T449, T450.
 *
 *   T449  RaceTireModel / RaceTirePacket: the tyres of the race cars.  A
 *         RaceTireModel (TireModelBase, T447) holds three RaceTirePackets
 *         (TirePacket, T462), the prebuilt GS packets of the tyre at three
 *         levels of detail (18 / 12 / 8 segments); a fourth "level" draws
 *         a plain cylinder.  On top of TirePacket's tyre, a RaceTirePacket
 *         adds the brake disc and caliper (textures of "race/brake.bin"),
 *         the glow of a heat brake, the wheel model (ModelSet, through a
 *         RaceTirePacket::Callback that gives it the level of detail) or a
 *         hexagon in its place at the lowest level, and the motion blur of
 *         the wheel and of the tread (both rendered into small GS buffers).
 *   T450  MenuTireModel: the tyre of a menu car (one TirePacket, 30
 *         segments; the wheel is a part of the car's own model).
 *
 * Every class is defined in its canonical header (include/game/T449, T450
 * and the ones below); this header adds what only this unit uses: the .data
 * / .bss objects of T449 and T450, the VU0 helpers and named constants of
 * the graphics library calls.  Offsets in comments are the original (egcs)
 * ones.
 */
#ifndef U101_H
#define U101_H

#include "types.h"
#include "libc.h"                       /* free, memcpy */
#include "cxxrt.h"                      /* type_info, __rtti_si */
/* (No <new>: its "bad_alloc" is not in either original file's .rodata.) */

/* own translation units */
#include "game/car/model/T449_race_tire_model.h"
#include "game/car/model/T450_menu_tire_model.h"

/* outside the unit */
#include "game/car/model/T447_tire_model_base.h"  /* TireModelBase, TireFile2, WheelFile2, tire_file_*, wheel_file_* */
#include "game/car/model/T448_strip_builder.h"    /* TStripBuilder */
#include "engine/gfx/T451_model_set.h"        /* ModelSet (+ Callback), modelset_draw_part, modelset_set_callback */
#include "game/car/model/T462_tire_packet.h"      /* TirePacket, TireCircle, F_00221260 */
#include "engine/io/T503_vol_thread.h"       /* file_read_blocking, vol_device_object */
#include "engine/gfx/T512_render_context.h"   /* g_002F4250 (= g_002F4230.vram, the texture manager) */
#include "engine/gfx/T513_vram_allocator.h"   /* VramAllocator::loadTex1 */
#include "engine/gfx/T514_gs_buffer.h"        /* GSBuffer */
#include "engine/gfx/T515_tex_set.h"          /* TexSet */
#include "game/effects/T159_flare.h"            /* Vec4 */
#include "engine/pgl/G001_flare.h"            /* the pgl graphics library, GsTexRegs, Tex1Image, GsMaterial */
#include "engine/media/G002_mpeg_frameq.h"      /* sincosf */

/* ======================================================================== */
/* EE / VU0 helpers                                                         */
/* ======================================================================== */

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft).  Volatile: retail loads the
 * constant it is compared with (0.05, from .lit4) only after it, which a
 * volatile asm (a barrier to memory accesses) gives (MenuTireModel::
 * virtual_1 exact; non-volatile, the load is hoisted above it). */
static inline f32 ee_rsqrt(f32 num, f32 sq)
{
    f32 r;
    __asm__ __volatile__("rsqrt.s %0, %1, %2" : "=f"(r) : "f"(num), "f"(sq));
    return r;
}

/* out = vf31 / vf28 of VU0 (the translation / the x column of the current
 * matrix, G001's convention).  The original's asm moves the quadword through
 * $2 / $3 (vnop, qmfc2.ni + pcpyud + sdl / sdr into the unaligned Vec4, a
 * memory operand: retail folds the frame offset into the sdl / sdr), as
 * T159's vu0_get_translation; GCC 14 (#else) goes through an aligned
 * temporary (sqc2, the same values). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline void vu0_get_vf31(Vec4 *out)
{
    __asm__ __volatile__("vnop\n\tqmfc2.ni $2, $vf31\n\tpcpyud $3, $2, $2\n\t"
                         "sdl $2, 7+%0\n\tsdr $2, %0\n\tsdl $3, 15+%0\n\tsdr $3, 8+%0"
                         : "=m"(*out) : : "$2", "$3");
}
static inline void vu0_get_vf28(Vec4 *out)
{
    __asm__ __volatile__("vnop\n\tqmfc2.ni $2, $vf28\n\tpcpyud $3, $2, $2\n\t"
                         "sdl $2, 7+%0\n\tsdr $2, %0\n\tsdl $3, 15+%0\n\tsdr $3, 8+%0"
                         : "=m"(*out) : : "$2", "$3");
}
#else
static inline void vu0_get_vf31(Vec4 *out)
{
    f32 v[4] __attribute__((aligned(16)));
    __asm__ volatile(
        "vnop\n\t"
        "sqc2         $vf31, 0(%0)\n\t"
        : : "r"(v) : "memory");
    out->x = v[0]; out->y = v[1]; out->z = v[2]; out->w = v[3];
}
static inline void vu0_get_vf28(Vec4 *out)
{
    f32 v[4] __attribute__((aligned(16)));
    __asm__ volatile(
        "vnop\n\t"
        "sqc2         $vf28, 0(%0)\n\t"
        : : "r"(v) : "memory");
    out->x = v[0]; out->y = v[1]; out->z = v[2]; out->w = v[3];
}
#endif

/* ======================================================================== */
/* Constants of the graphics library calls                                 */
/* ======================================================================== */

/* The matrix modes (PGL_MTX_*), the gfx_enable switches (PGL_CAP_*; the
 * tyres turn PGL_CAP_FLAG24 on around their packets, ? what it selects, and
 * PGL_CAP_VU1_PROG_33E off when no corner of the bounding box crosses the
 * guard band: ? the clipping variant of the VU1 program), the gfx_tex_param
 * pnames and values, the alpha test / fail values and the blend modes:
 * G001_flare.h. */

/* gfx_cull_face (glCullFace; GsContext.cullFace, init PGL_CULL_BACK): with
 * PGL_CAP_CULL_FACE the VU1 program culls by the (face, winding) pair.  The
 * tyres swap FRONT and BACK while their matrix is mirrored (z scaled by -1),
 * which flips the winding, and restore the mode gfx_get_cull_face gave. */
static inline int gfx_other_cull_mode(int mode) { return mode == PGL_CULL_FRONT ? PGL_CULL_BACK : PGL_CULL_FRONT; }

/* gfx_set_frame_write_mask: the bits written. */
enum {
    GS_WRITE_RGBA = (int)0xFFFFFFFF,
    GS_WRITE_RGB  = 0x00FFFFFF          /* the alpha (destination alpha) kept */
};

/* gs_set_scanmask_mode: PGL_SCANMASK_* (G001_flare.h). */

/* pgl_push_attrib: everything of the render target (0x887), pushed while a
 * tyre draws into its own small buffer. */
enum {
    PGL_ATTRIB_TARGET = PGL_ATTR_SCISSOR | PGL_ATTR_VIEWPORT | PGL_ATTR_TARGET_B |
                        PGL_ATTR_DEPTH | PGL_ATTR_DRAW_BUFFER
};

/* GS PRIM of the strips (TStripBuilder::setPrim). */
enum {
    PRIM_TRISTRIP = 0x04,
    PRIM_IIP      = 0x08,               /* Gouraud */
    PRIM_TME      = 0x10,               /* textured */
    PRIM_FGE      = 0x20,               /* fogged */
    PRIM_ABE      = 0x40                /* alpha blended */
};

/* Shaders (TStripBuilder::setShader) of RaceTirePacket's strips: given to
 * the part-texture callback (tire_packet_pass_callback -> virtual_2) when
 * the block is drawn; 0 calls nothing. */
enum {
    RACE_SHADER_NONE    = 0,            /* the disc, the caliper (their textures are set by draw) */
    RACE_SHADER_GLOW    = 0xC,          /* the heat glow: the brake file's glow texture, no depth writes */
    RACE_SHADER_HEXAGON = 0xD           /* the hexagon: white, the wheel texture of drawWheelTexture */
};

/* TStripBuilder::finish(a, b): a = the lighting mode of the packet (G001
 * VifModel.mode), b = its colour / alpha scale flags. */
enum { STRIP_LIT = 1, STRIP_SCALE_NONE = 0 };

/* GSBuffer.psm of the blur buffers. */
enum { GS_PSMCT32 = 0 };

/* The wheel model's texture table (ModelSet.tex, or texSets[part * 3 +
 * level] when it has sets): the entries of the wheel face.  (? entry 0: the
 * rest of the wheel; it is never picked here.) */
enum {
    WHEEL_TEX_FACE         = 1,         /* the still face */
    WHEEL_TEX_FACE_BLURRED = 2          /* the face drawn blurred (fast spin), optional */
};

/* The register block of G001's second texture table (texsel_get_table2, the
 * models' environment table: modelset_set_env_tex) that receives the wheel
 * face of this frame: drawWheelTexture fills it, the hexagon (shader 0xD)
 * draws with it.  (? the wheel model selects it too: texture 0x1FF with
 * base 1.) */
enum { TEX2_WHEEL_FACE = 1 };

/* The blur buffer of the tread (drawTreadBlur): 128 x 64. */
enum { TREAD_BLUR_W = 0x80, TREAD_BLUR_H = 0x40 };
/* The wheel face blur buffer: w x 3w, w = the face texture's width, at most
 * 64 (drawWheelTexture). */
enum { WHEEL_BLUR_MAX_W = 0x40 };

/* BrakeDisc.flags. */
enum { BRAKE_DISC_HEAT = 1 };               /* a heat brake: the glow strip (virtual_4) */

/* TireFile2.flags (bit 1, the F1 tyre: TireFile2::isF1). */
enum { TIRE_TREAD_BLUR = 1 };               /* the tread is motion blurred (drawTreadBlur) */

/* The levels of detail of RaceTireModel (selectLOD; draw). */
enum {
    RACE_TIRE_LEVELS   = 3,             /* the RaceTirePackets (0 the finest) */
    RACE_TIRE_HEXAGON  = 2,             /* the level with a hexagon for the wheel model */
    RACE_TIRE_CYLINDER = 3              /* draw: the plain cylinder */
};

/* ======================================================================== */
/* T449: the brake file and the tables of RaceTireModel                     */
/* ======================================================================== */

/* Defined in T449 (its .sdata, zeros: uninitialised C++ globals, written at
 * the end of the file in the order of these declarations). */
extern BrakeFile *g_00352ACC;               /* 0x00352ACC gp-0x4F24 (T449 .sdata) the brake file (race_begin
                                               .. race_end) */
extern int        g_00352AD0;               /* 0x00352AD0 gp-0x4F20 (T449 .sdata) 1 while its textures are
                                               linked in the brake textures (T449's static g_0049D160, .bss) */

/* The hexagon of RaceTirePacket::buildHexagon (T449 .rodata, its first item:
 * external linkage, written where it is defined).  T449's other tables
 * (0x00340FD0..0x003410BF: the levels of detail, the white material, the
 * square, the normal) are file statics there: egcs writes them after the
 * vtables and type names. */
extern const f32 g_00340ED0[6][3];          /* 0x00340ED0 a hexagon in the y-z plane (radius 1.1547) */

/* T450 .rodata (its first item) */
extern const f32 g_003410C0[4];             /* 0x003410C0 the origin (0, 0, 0, 1) */

/* The __si_type_info objects of the typeinfo functions (.bss). */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __tiQ214RaceTirePacket8Callback[3];  /* 0x004B29C0 */
extern u32 __ti13RaceTireModel[3];          /* 0x004B29D0 */
extern u32 __ti14RaceTirePacket[3];         /* 0x004B29E0 */
extern u32 __ti13MenuTireModel[3];          /* 0x004B29F0 (T450) */
}
#endif

#endif /* U101_H */
