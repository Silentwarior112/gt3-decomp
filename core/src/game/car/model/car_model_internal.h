/* car_model_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u100,
 * 0x0021E7D8..0x00221E08: translation units T446, T447, T462, T448.
 *
 *   T446  CarModel: the drawn body of a car (a ModelSet, T451, with a
 *         ModelSet::Callback that answers its switches with the lamp state
 *         and morph positions and picks its LOD), the car list of the volume
 *         (cars/day/: model code -> file name, sorted for a binary search)
 *         and the model file paths (cars/day/, cars/eve/, cars/night/,
 *         menu/cars/); the body's parts (0 the body, 1 the shadow projected
 *         on the ground, 4..7).
 *   T447  TireModelBase: the size of a tyre (width, rim radius, aspect
 *         ratio -> outer radius, bounding box and sphere), its VU0 view
 *         test, the tyre file (tire/n%d: the tread texture) and the wheel
 *         file (wheel/%s: the wheel model and its variations).
 *   T462  TirePacket: builds the tyre geometry once into strip packets
 *         (TStripBuilder): the tread (a cylinder of nseg segments), the
 *         shoulder bevels, the sidewalls, the faces, the shadow strip; and
 *         draws them.
 *   T448  TStripBuilder: collects triangle strips (position, normal, uv,
 *         colour per vertex; up to 64 vertices per block) and writes them
 *         as VIF UNPACK blocks behind a GIF tag; or only measures the size.
 *
 * Every class is defined in its canonical header (include/game/T446, T447,
 * T448, T462); this header adds the outside declarations they need and what
 * only this unit uses: the car list's storage, the data objects of T447 /
 * T462 (each defined in its file), and named constants of the graphics
 * library calls.
 */
#ifndef U100_H
#define U100_H

#include "types.h"
#include "libc.h"                       /* strcpy, strcat, realloc */
#include "cxxrt.h"                      /* type_info, __rtti_si, __rtti_user */

/* own translation units */
#include "game/car/model/T446_car_model.h"
#include "game/car/model/T447_tire_model_base.h"
#include "game/car/model/T448_strip_builder.h"
#include "game/car/model/T462_tire_packet.h"

/* outside the unit */
#include "engine/gfx/T451_model_set.h"        /* ModelSet (+ Callback), modelset_* */
#include "game/data/T481_car_data_base.h"    /* nameToCode */
#include "engine/io/T500_vol_toc.h"          /* VolDir, VolToc, vol_toc_lookup */
#include "engine/io/T503_vol_thread.h"       /* vol_device_object, file_read_blocking */
#include "engine/gfx/T512_render_context.h"   /* g_002F4250 (= g_002F4230.vram, the texture manager) */
#include "engine/gfx/T513_vram_allocator.h"   /* VramAllocator::loadTex1 */
#include "engine/pdistd/T534_sprintf.h"          /* sprintf */
#include "engine/pgl/G001_flare.h"            /* the pgl graphics library, GsMaterial, VifModel, Tex1Image */
#include "engine/media/G002_mpeg_frameq.h"      /* sincosf */
#include "runtime/newlib/G007_stdio.h"            /* qsort, rand */

/* ======================================================================== */
/* Constants of the graphics library calls                                 */
/* ======================================================================== */

/* The matrix modes (PGL_MTX_*), mat_scale_set_grey's kinds (PGL_MAT_*, as
 * gs_set_material_param's), the gfx_enable switches (PGL_CAP_*), the
 * gfx_tex_param pnames and values and the blend modes: G001_flare.h. */

/* pgl_set_scalar_param kind 0: qAF0 = (v, v, v, 1), the scale of the
 * environment-map colours of lighting mode 2 (u115). */
enum { PGL_PARAM_ENVMAP_SCALE = 0 };

/* GS PRIM: a triangle strip and its attributes. */
enum {
    PRIM_TRISTRIP = 0x04,
    PRIM_IIP      = 0x08,               /* Gouraud */
    PRIM_TME      = 0x10,               /* textured */
    PRIM_FGE      = 0x20,               /* fogged */
    PRIM_ABE      = 0x40                /* alpha blended */
};

/* ======================================================================== */
/* T446: CarModel                                                           */
/* ======================================================================== */

/* The car list (cars/day/): one entry per model file, sorted by code. */
struct CarListEntry {
    u64         code;                       /* 0x0 nameToCode(name) */
    const char *name;                       /* 0x8 the file name in the TOC */
    u32         unk_0C;                     /* 0xC (never written) */
};
EGCS_SIZE(CarListEntry, 0x10);
extern int           g_00352AB8;            /* 0x00352AB8 gp-0x4F38 (T446 .sdata, defined there) the number of entries */
extern CarListEntry *g_00352ABC;            /* 0x00352ABC gp-0x4F34 (T446 .sdata, defined there) the entries (realloc'd) */

/* The car shadow's direction and colour: T446's file statics g_0049D140,
 * g_0049D150 (.bss). */

/* ======================================================================== */
/* T447: TireModelBase                                                      */
/* ======================================================================== */

extern const f32 g_00340CA0[4];             /* 0x00340CA0 (T447 .rodata, defined there) {0, 0, 0, 1}: the origin */

/* ======================================================================== */
/* T462: TirePacket                                                         */
/* ======================================================================== */

/* The tyre material (T462 .data, defined there): ambient 0.3, diffuse 0.15,
 * specular 0.9, emission (0, 0, 0, 1), shininess 16 (TirePacket::virtual_2
 * replaces it with the tyre file's power). */
extern GsMaterial g_002E5E90;               /* 0x002E5E90 */

/* A prebuilt packet (T462 .rodata, defined there; a VifModel as
 * TStripBuilder::finish writes them): one part of 0x17 quadwords at
 * 0x00340D40 (positions and colours 0x80000000), 20 vertices, 16 triangles,
 * PRIM 0x2C: a hexagonal prism x 0..1, radius 1, as two strips, the side
 * (14 vertices) and the cap at x = 0 (6) - a coarse unit tyre. */
extern const VifModel g_00340CE0;           /* 0x00340CE0 */

/* The packet tire_packet_pass_callback forwards to (bindPassCallback). */
extern TirePacket *g_00352AC8;              /* 0x00352AC8 gp-0x4F28 (T462 .sdata, defined there) */

/* Shaders (TStripBuilder::setShader): a block's VifPart.tex, given to the
 * part-texture callback (tire_packet_pass_callback -> virtual_2) when the
 * packet is drawn; 0 calls nothing, so the state set before stays. */
enum {
    TIRE_SHADER_NONE  = 0,                  /* the faces, the shadow */
    TIRE_SHADER_1     = 1,                  /* known to virtual_2, nothing to set (no builder of u100 uses it) */
    TIRE_SHADER_TREAD = 2                   /* the tread side: its texture wrap, blend and material */
};

#endif /* U100_H */
