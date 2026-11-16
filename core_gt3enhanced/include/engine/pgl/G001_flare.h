/* G001_flare.h - canonical header of translation unit G001 (no-FDE region,
 * units u113 u114 u115 u116 u117 u118; plain C, tex1.cpp is C++ with
 * C linkage).
 *
 * G001 is Polyphony's graphics library "pgl" ("__pglShading" is its own name):
 * the GS / VIF1 / GIF driver also linked into the bootstrap (there
 * include/gs_driver.h, src/gs_driver.cpp), here in a later, larger version
 * with an OpenGL-like layer on VU0 macro mode and the VU1 microcode, plus its
 * helpers (weather particles, lens flare, the 'Tex1' texture sets, bitmap
 * fonts, a printf engine, colour filters).  It keeps
 *   - GsContext, 0xBC0 bytes in scratchpad RAM at 0x70003000 (GS_CTX):
 *     shadowed GS registers with a dirty mask, the VIF1 / GIF packet writer,
 *     OpenGL-style light / material state compiled for the VU1 microcode,
 *     the VU1 viewport constants;
 *   - GfxDriverState g_gfx, 0xC0 bytes in .bss: vblank counter, per DMA
 *     channel busy flags and waiter lists, display register shadows.
 * VU0 conventions: vf28..vf31 the current matrix (GsContext.matrixMode),
 * vf24..vf27 the combined matrix (rebuilt by pgl_sync_matrices when
 * GsContext.matrixDirty is set), vi11..vi13 the matrix stack pointers.
 * The VU0 micro subroutines (microcode 0x00349600, vcallms byte addresses):
 * 0x000 vf28..31 = vf1..4 x vf28..31, 0x090 vf28..31 = vf28..31 x vf1..4,
 * 0x120 vf24..27 = vf28..31 x the projection (VU0 qwords 8..11), 0x1D0
 * vf1 = sin(vf1) per lane (Taylor polynomial to x^9), 0x2D0 the arctangent
 * of vu0_atan2f.
 *
 * Types used only through pointers stay forward declarations; they are
 * defined by their unit: Weather, WeatherDrop, FlareWork, ClutAnimTable (u113),
 * GsFrameHeader, GsSubPacketHeader (u116). */
#ifndef GAME_G001_FLARE_H
#define GAME_G001_FLARE_H

#include "game/game.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

struct GsTexRegs;
struct GsFrameHeader;
struct GsSubPacketHeader;
struct GsReadbackRec;
struct ClutAnimTable;
struct Weather;
struct WeatherDrop;
struct FlareWork;
struct PglFont;
struct FontSet;
struct GsLibState;

/* ======================================================================== */
/* Vectors, render targets, lights, materials                              */
/* ======================================================================== */

/* A VU0 / VU1 quadword (lqc2 / sqc2). */
typedef struct GsVec4 {
    f32 x;                  /* 0x00 */
    f32 y;                  /* 0x04 */
    f32 z;                  /* 0x08 */
    f32 w;                  /* 0x0C */
} __attribute__((aligned(16))) GsVec4;
EGCS_SIZE(GsVec4, 0x10);

/* A GsVec4 passed by value (gs_set_light_param_v): the binary keeps it in ONE
 * 128-bit GPR ($a3, qmfc2 / sq); egcs does that for a u128 argument, GCC 14
 * passes the struct. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
typedef u128 GsVec4Arg;
#else
typedef GsVec4 GsVec4Arg;
#endif

/* Render-target descriptor (GsContext.tgtB @0x170, tgtA @0x184). */
typedef struct GsRenderTarget {
    u16 base;               /* 0x00 frame buffer base in 64-word blocks (FRAME.FBP = base >> 5) */
    u8  fbw;                /* 0x02 FRAME.FBW */
    u8  psm;                /* 0x03 FRAME.PSM */
    s32 width;              /* 0x04 pixels */
    s32 height;             /* 0x08 pixels */
    u16 xoff;               /* 0x0C XYOFFSET.OFX (12.4) = (0x800 - width/2) << 4 */
    u16 yoff;               /* 0x0E XYOFFSET.OFY */
    u16 xend;               /* 0x10 xoff + width*16 */
    u16 yend;               /* 0x12 yoff + height*16 */
} GsRenderTarget;
EGCS_SIZE(GsRenderTarget, 0x14);

/* One light (8 at GsContext.lights, stride 0xD0): the glLight state and the
 * values derived from it for the VU1 microcode (u115) and the CPU lighting
 * (pgl_shade_vertices).  gs_set_light_param_f / _v pnames: 0 ambient,
 * 1 diffuse, 4 specular, 5 spotDir, 6 spotExponent, 7 spotCutoff,
 * 8..10 attenuation, 13 position.  gfx_init_lights: light 0 gets the OpenGL
 * defaults (spotCutoff 180 = not a spot light). */
typedef struct GsLight {
    struct GsLight *next;   /* 0x00 next enabled light (GsContext.lightList, built by pgl_build_light_list 0x0024EE80) */
    u8     unk_04[4];       /* 0x04 */
    u8     enable;          /* 0x08 gfx_enable / gfx_disable (0x65 + i); a change sets lightFlags |= 1 */
    s8     vuIndex;         /* 0x09 slot 0..3 in the VU1 light block, -1 = none */
    u8     unk_0A[2];       /* 0x0A */
    u32    spotExpEnc;      /* 0x0C float_to_gs_param_encode(spotExponent) */
    GsVec4 ambient;         /* 0x10 */
    GsVec4 diffuse;         /* 0x20 */
    GsVec4 diffuseLit;      /* 0x30 diffuse * material diffuse (pgl_update_light_colors 0x0024F218) */
    GsVec4 specular;        /* 0x40 */
    GsVec4 specularLit;     /* 0x50 specular * material specular */
    GsVec4 position;        /* 0x60 times the modelview when set; w == 0: directional */
    GsVec4 eyePosition;     /* 0x70 position in model space (normalised if directional) */
    GsVec4 halfVector;      /* 0x80 normalize(eyePosition - view direction) (directional) */
    GsVec4 spotDir;         /* 0x90 normalised, times the modelview when set */
    GsVec4 eyeSpotDir;      /* 0xA0 normalised spot direction in model space (positional) */
    f32    spotExponent;    /* 0xB0 clamped to [0, 128] */
    const f32 *spotTable;   /* 0xB4 pow(x, spotExponent) table, NULL: float_pow_encoded; reset with spotExponent */
    f32    spotTableScale;  /* 0xB8 table index = (int)(clamp(x, 0, 1) * scale) */
    f32    spotCutoff;      /* 0xBC degrees, 180.0: not a spot light */
    f32    spotCosCutoff;   /* 0xC0 pgl_cos_deg(spotCutoff) */
    f32    constAtten;      /* 0xC4 attenuation 1 / (c + d (l + d q)) */
    f32    linearAtten;     /* 0xC8 */
    f32    quadAtten;       /* 0xCC */
} GsLight;
EGCS_SIZE(GsLight, 0xD0);

/* GsMaterial word 0x44: in the context's materials the pow table of the CPU
 * lighting (read by pgl_shade_vertices, cleared by gs_set_material_param and
 * the init); in the material table records loaded by material_load (model
 * data, the tables of texmat_select) a flags byte. */
typedef union GsMaterialX44 {
    const f32 *powTable;    /* pow(x, shininess) table, NULL: float_pow_encoded */
    u8         flags;       /* bit 0: scaled by the library's matScale (material_load) */
} GsMaterialX44;

/* GsMaterial word 0x48: in the context's materials the pow table's index
 * scale; in the model files' material records (T451 ModelSet, T454
 * SpriteModel) an offset in the file, relocated with the model. */
typedef union GsMaterialX48 {
    f32   powTableScale;    /* table index = (int)(x * scale + 0.5) */
    void *offset;           /* model file records: relocated if not NULL */
} GsMaterialX48;

/* Material parameters, one block per face (GsContext.material, stride 0x50;
 * also the records of the material tables).  gs_set_material_param kinds:
 * 0 ambient, 1 diffuse, 2 both, 3 emission, 4 specular, 11 shininess. */
typedef struct GsMaterial {
    GsVec4 ambient;         /* 0x00 kind 0 (and 2) */
    GsVec4 diffuse;         /* 0x10 kind 1 (and 2) */
    GsVec4 specular;        /* 0x20 kind 4 */
    GsVec4 emission;        /* 0x30 kind 3 */
    f32    shininess;       /* 0x40 kind 11, clamped 0..128; picks the VU1 specular program (< 6, < 12, else) */
    GsMaterialX44 x44;      /* 0x44 powTable (context) / flags (table records) */
    GsMaterialX48 x48;      /* 0x48 powTableScale (context) / offset (model file records) */
    u32    shininessEnc;    /* 0x4C float_to_gs_param_encode(shininess) */
} GsMaterial;
EGCS_SIZE(GsMaterial, 0x50);

/* ======================================================================== */
/* The drawing context (scratchpad 0x70003000)                             */
/* ======================================================================== */

/* GsContext.gen[] - general registers, emitted in this order (GS register
 * numbers 46 44 45 3D 49 1A 22 3B 1C 3F); dirty bit 32 + index. */
enum {
    GS_GEN_COLCLAMP = 0, GS_GEN_DIMX, GS_GEN_DTHE, GS_GEN_FOGCOL, GS_GEN_PABE,
    GS_GEN_PRMODECONT, GS_GEN_SCANMSK, GS_GEN_TEXA, GS_GEN_TEXCLUT, GS_GEN_TEXFLUSH,
    GS_GEN_COUNT                                  /* 10 */
};
/* GsContext.reg[] - per-context registers: ONE shadow serves both GS
 * contexts, dirty bit i emits it to the context-1 register, 16 + i to the
 * context-2 register. */
enum {
    GS_REG_FRAME = 0, GS_REG_XYOFFSET, GS_REG_FBA, GS_REG_ZBUF, GS_REG_SCISSOR,
    GS_REG_ALPHA, GS_REG_TEST, GS_REG_CLAMP, GS_REG_TEX2, GS_REG_TEX1, GS_REG_TEX0,
    GS_REG_MIPTBP1, GS_REG_MIPTBP2,
    GS_REG_COUNT                                  /* 13 */
};

/* The primitive buffers gfx_alloc_tail hands out for pgl_draw_strip (xyz,
 * uvs) and the strip / sprite helpers: one vertex, 12 bytes (z is stored 0
 * by the 2D users: the menus' quads, T035, T259, the logger's strips), and
 * one texture coordinate pair, 8 bytes. */
typedef struct PrimVertex {
    f32 x;                  /* 0x0 */
    f32 y;                  /* 0x4 */
    f32 z;                  /* 0x8 */
} PrimVertex;
EGCS_SIZE(PrimVertex, 0xC);

typedef struct PrimUV {
    f32 u;                  /* 0x0 */
    f32 v;                  /* 0x4 */
} PrimUV;
EGCS_SIZE(PrimUV, 0x8);

/* Callbacks of the immediate-mode primitive path (pgl_draw_strip_vu1
 * 0x0024FEA8) and of the prebuilt models. */
typedef void (*PglTexGenFunc)(void *dst, const void *xyz, const void *normals,
                              const void *uvs, int n);
typedef void (*PglColorGenFunc)(void *dst, const void *xyz, const void *normals,
                                const void *colors, int n);
/* Called per model part with a texture index (VifPart.tex != 0); set by
 * pgl_set_part_tex_func (texmat_select in u102). */
typedef void (*PglPartTexFunc)(s32 tex);
/* GsReadbackRec.callback, copied from GsContext.readbackCallback by
 * gs_download_image; called with the destination and readbackArg. */
typedef void (*GsReadbackFunc)(u32 madr, u32 arg);

/* Drawing context, 0xBC0 bytes, at scratchpad 0x70003000 (GS_CTX).  The VU1
 * lighting block 0x1E0..0x2DF is sent by the u115 lighting modes
 * (pgl_send_lights_* 0x0024E8F8..): up to 4 directional lights, their
 * vectors transposed (x, y, z of lights 0..3). */
typedef struct GsContext {
    u8     matrixDirty;     /* 0x000 a matrix changed: pgl_sync_matrices rebuilds vf24..vf27 */
    u8     matrixMode;      /* 0x001 matrix in vf28..vf31: 0 modelview, 1 projection, 2 texture; mode m parked at VU0 qword 4 + 4m */
    u8     vu1MatrixSent;   /* 0x002 0: the combined matrix and viewport must be sent to VU1 (pgl_send_matrix) */
    u8     curList;         /* 0x003 packet list being written: 0 VIF1, 1 GIF */
    u16    primAttr;        /* 0x004 PRIM attribute bits (IIP 8, TME 0x10, FGE 0x20, ABE 0x40, AA1 0x80); init 8 */
    u8     texGenEnable;    /* 0x006 texture coordinates from texGenFunc */
    u8     colorGenEnable;  /* 0x007 colours from colorGenFunc (gfx_enable 0x0B) */
    u32    vu1Prog;         /* 0x008 VU1 MSCAL address of the primitive program (0x320 / 0x33E + 0xA / 0x14, gs_select_vu1_prog), 0 = none */
    u32    vu1LightProg;    /* 0x00C lighting variant of the model program (sent with ITOP) */
    s32    vu1LightOfs;     /* 0x010 added to it: 0 / -2 / -4 / -6 by vertex format */
    u32    f014;            /* 0x014 cleared by vifmodel_setup_lighting (0x0024DF28) */
    u32    stmask;          /* 0x018 the VIF STMASK last sent (0xC0C0C0C0 at the list starts) */
    s32    lodBias;         /* 0x01C added to TEX1.K (<< TEX1.L) by gfx_set_texture_regs; gfx_tex_param 10 */
    u8     lightFlags;      /* 0x020 1 lights, 2 modelview, 4 material changed; 8 / 0x10 material / a light specular, 0x20 a positional light; 0x40 local viewer; 0x80 CPU lighting */
    u8     lightMode;       /* 0x021 VU1 lighting / __pglShading mode; init 2 */
    u8     colorMaterial;   /* 0x022 gfx_enable 0x0D: colours also set the material (colorMaterialFaces / colorMaterialKind) */
    u8     halfHeight;      /* 0x023 field rendering: y coordinates in 12.3 */
    u8     flag24;          /* 0x024 gfx_enable PGL_CAP_FLAG24 (no G001 code reads it) */
    u8     vu1ProgAlt;      /* 0x025 init 1; PGL_CAP_VU1_PROG_33E: vu1Prog base 0x33E, else 0x320 (the bootstrap's "pal"; not a video mode) */
    u8     cullEnable;      /* 0x026 face culling on (PGL_CAP_CULL_FACE): vu1Prog by cullFace / frontFace */
    u8     flag27;          /* 0x027 gfx_enable PGL_CAP_FLAG27 (no G001 code reads it) */
    u8     texMatrixEnable; /* 0x028 texture coordinates through the texture matrix (pgl_xform_texcoords) */
    u8     scaleEnable;     /* 0x029 the combined matrix columns are scaled by scale */
    u8     normalMtxEnable; /* 0x02A pgl_build_env_matrix premultiplies by normalMtx */
    u8     unk_02B[5];      /* 0x02B */
    u64    gen[GS_GEN_COUNT];   /* 0x030 general register shadows (GS_GEN_*) */
    u64    reg[GS_REG_COUNT];   /* 0x080 per-context register shadows (GS_REG_*) */
    u64    dirty;           /* 0x0E8 registers to emit on the next flush */
    u64    written;         /* 0x0F0 registers emitted (since the sub-packet start) */
    u64    enable;          /* 0x0F8 emit mask of the current list */
    u64    enableList0;     /* 0x100 saved emit mask of list 0 (VIF1) */
    u64    enableList1;     /* 0x108 saved emit mask of list 1 (GIF) */
    u8    *bufBase;         /* 0x110 packet buffer */
    u32    bufSize;         /* 0x114 */
    u32   *cur;             /* 0x118 packet write pointer (qword aligned, | addrBits) */
    u32   *end;             /* 0x11C packet end; gfx_alloc_tail takes scratch buffers from it, downwards */
    u32    uncachedBuf;     /* 0x120 buffer access: 0 cached, 1 uncached, 2 uncached-accelerated */
    u32    addrBits;        /* 0x124 segment bits ORed into buffer addresses */
    u32   *tail0;           /* 0x128 reserved next-tag slot of list 0 */
    u32   *tail1;           /* 0x12C reserved next-tag slot of list 1 */
    struct GsFrameHeader *frameHdr;     /* 0x130 frame packet header (gfx_begin_packet) */
    struct GsSubPacketHeader *subHdr;   /* 0x134 sub-packet being built (gfx_begin_subpacket) */
    PglTexGenFunc   texGenFunc;     /* 0x138 */
    PglColorGenFunc colorGenFunc;   /* 0x13C */
    PglPartTexFunc  partTexFunc;    /* 0x140 NULL: model_draw always takes the general path */
    GsReadbackFunc  readbackCallback; /* 0x144 word 2 of the gs_download_image readback record */
    u32    readbackArg;     /* 0x148 word 3 of the readback record */
    u32    clearColor;      /* 0x14C RGBA of the clears of tgtA */
    u32    clearColorB;     /* 0x150 RGBA of the clears of tgtB */
    f32    clearDepth;      /* 0x154 depth clear value = clearDepth * depthScale */
    f32    depthNear;       /* 0x158 (glDepthRange near) init 0 */
    f32    depthFar;        /* 0x15C (glDepthRange far) init 1.0 */
    f32    depthScale;      /* 0x160 max Z of the Z-buffer format */
    f32    depthOffset;     /* 0x164 gfx_set_depth_offset */
    f32    depthBias;       /* 0x168 gs_set_depth_bias_at */
    u32    spriteZF;        /* 0x16C XYZF high word (Z | F << 24) of the 2D primitives */
    GsRenderTarget tgtB;    /* 0x170 secondary target (effects, gs_clear_buffers bit 2) */
    GsRenderTarget tgtA;    /* 0x184 draw buffer (gfx_set_draw_buffer) */
    u8     unk_198[0x14];   /* 0x198 */
    s32    winX;            /* 0x1AC draw window (gfx_set_draw_area) */
    s32    winY;            /* 0x1B0 */
    s32    winW;            /* 0x1B4 */
    s32    winH;            /* 0x1B8 */
    s8     field;           /* 0x1BC current field (from the vblank handler) */
    u8     fieldShift;      /* 0x1BD field mode 9 (PGL_FIELD_FRAME_SHIFT) active (u116: f1BD); gfx_frame_effect 7 / 8 test it */
    u8     packetOpen;      /* 0x1BE \ tested together as one u16 ("packet active") */
    u8     subPacketOpen;   /* 0x1BF / */
    u8     path3Masked;     /* 0x1C0 */
    s8     clutPsm;         /* 0x1C1 CLUT format of gs_set_clut (1 = CT24: TCC off) */
    u8     unk_1C2[2];      /* 0x1C2 */
    u32    cullFace;        /* 0x1C4 the culled faces, PGL_CULL_* (gfx_cull_face = glCullFace), init 2 BACK */
    u32    frontFace;       /* 0x1C8 the front face winding, PGL_FRONT_* (gfx_front_face = glFrontFace), init 0 CCW */
    u32    zTestMethod;     /* 0x1CC TEST.ZTST used while depth testing (gfx_depth_func) */
    u8     zMask;           /* 0x1D0 ZBUF.ZMSK, 1 = no depth writes (gfx_depth_mask; also the clear packets) */
    u8     depthTest;       /* 0x1D1 depth test enabled (gfx_enable / gfx_disable 5) */
    u8     unk_1D2[2];      /* 0x1D2 */
    f32    fogStart;        /* 0x1D4 gfx_set_fog_params */
    f32    fogAmount;       /* 0x1D8 init 1.0 */
    u8     unk_1DC[4];      /* 0x1DC */
    GsVec4 lightDirX;       /* 0x1E0 VU1 light block: directions, transposed */
    GsVec4 lightDirY;       /* 0x1F0 */
    GsVec4 lightDirZ;       /* 0x200 */
    GsVec4 ambientColor;    /* 0x210 emission + global ambient x material ambient + light ambients */
    GsVec4 lightDiffuse[4]; /* 0x220 */
    GsVec4 halfX;           /* 0x260 half vectors, transposed */
    GsVec4 halfY;           /* 0x270 */
    GsVec4 halfZ;           /* 0x280 */
    GsVec4 eyePos;          /* 0x290 the eye in model space */
    GsVec4 lightSpecular[4];/* 0x2A0 */
    GsLight    lights[8];   /* 0x2E0 */
    GsMaterial material[2]; /* 0x960 front, back */
    GsVec4 eyeWorld;        /* 0xA00 the viewer {0,0,0,1}; pgl_light_modelfv 3 */
    GsVec4 eyeModel;        /* 0xA10 eyeWorld in model space (local viewer) */
    GsVec4 viewDir;         /* 0xA20 normalised (0,0,-1) in model space (infinite viewer) */
    GsVec4 globalAmbient;   /* 0xA30 {0.2,0.2,0.2,1}; pgl_light_modelfv 0 */
    GsVec4 constVector;     /* 0xA40 pgl_set_const_vector; sent by lighting mode 5 (pgl_send_mode5) */
    GsVec4 constColor;      /* 0xA50 pgl_set_const_color: rgb * 255, a * 128; sent by lighting mode 5 */
    f32    colorScale;      /* 0xA60 lit colour -> GS colour: 128 or 255 */
    f32    alphaScale;      /* 0xA64 128 or 255 */
    GsLight *lightList;     /* 0xA68 enabled lights */
    u16    nLights;         /* 0xA6C */
    s16    nVuLights;       /* 0xA6E lights given a VU1 slot (directional, coloured, max 4) */
    GsVec4 envMtx[4];       /* 0xA70 lighting mode 2 (pgl_build_env_matrix) */
    GsVec4 normalMtx[4];    /* 0xAB0 pgl_set_normal_matrix (4x4); rows 0..2 premultiply the env matrix */
    GsVec4 qAF0;            /* 0xAF0 pgl_set_vec_param / _scalar_param kind 0, init {1,1,1,1} */
    GsVec4 qB00;            /* 0xB00 kind 2, init {1,1,1,1} */
    GsVec4 qB10;            /* 0xB10 kind 3, init {0,0,0,0} */
    GsVec4 scale;           /* 0xB20 pgl_set_scale: column scale of the combined matrix (scaleEnable) */
    u8     unk_B30[0x10];   /* 0xB30 */
    f32    normal[3];       /* 0xB40 the normal of primitives without normals, init (0, 0, 1) */
    u32    fB4C;            /* 0xB4C init 0 */
    u8     unk_B50[0x10];   /* 0xB50 */
    u32    color;           /* 0xB60 pgl_color: vertex colour of untextured primitives, init 0xFFFFFFFF */
    u32    colorTex;        /* 0xB64 color rgb * 129/256, alpha kept: textured primitives, init 0x80808080 */
    s32    colorMaterialKind; /* 0xB68 material kind of colorMaterial, init 2 */
    s32    colorMaterialFaces; /* 0xB6C material face mask of colorMaterial, init 1 */
    f32    morphWeight;     /* 0xB70 pgl_set_morph_weight: the morph weight (modelset_run_part op 0x35, the fraction towards the next packet); unpacked to VU1 0xF2 by vifmodel_send_lighting */
    u32    nVerts;          /* 0xB74 statistics: vertices sent */
    u32    nPrims;          /* 0xB78 statistics: triangles */
    u32    nParts;          /* 0xB7C statistics: VU1 kicks (model parts, strip batches) */
    GsVec4 viewScale;       /* 0xB80 VU1 viewport scale (winW/2, -winH/2, depth, 255 - fog); vf23 */
    GsVec4 viewCenter;      /* 0xB90 VU1 viewport centre (x, y, depth, colour max); vf22 */
    GsVec4 limits;          /* 0xBA0 clamp of the clip coordinates (FLT_MAX x 4; .w fog start); vf21 */
    GsVec4 vBB0;            /* 0xBB0 {0.5, 0.5, 1, 1}; vf20 */
} GsContext;
EGCS_SIZE(GsContext, 0xBC0);

#define GS_CTX ((GsContext *)0x70003000)

/* GsContext.lightFlags bits. */
enum {
    PGL_LF_LIGHTS       = 0x01,     /* light enables / colours changed: rebuild the list */
    PGL_LF_MODELVIEW    = 0x02,     /* modelview changed: eye-space light vectors */
    PGL_LF_MATERIAL     = 0x04,     /* material changed: light x material colours */
    PGL_LF_DIRTY        = 0x07,
    PGL_LF_MAT_SPECULAR = 0x08,     /* the material has a specular colour */
    PGL_LF_SPECULAR     = 0x10,     /* a light has a specular colour */
    PGL_LF_POINT        = 0x20,     /* a positional light has a colour */
    PGL_LF_LOCAL_VIEWER = 0x40,     /* pgl_light_modelfv 1 (GL_LIGHT_MODEL_LOCAL_VIEWER) */
    PGL_LF_CPU_LIGHTING = 0x80      /* immediate-mode vertices are lit on the CPU (PGL_CAP_CPU_LIGHTING) */
};

/* ======================================================================== */
/* Packet building: DMA tags, VIF codes, GIF tags, register shadows         */
/* ======================================================================== */

/* DMA source-chain tags (word 0: ID << 28 | QWC; word 1 the address). */
#define DMATAG_CNT(qwc)     (0x10000000u | (qwc))
#define DMATAG_REF(qwc)     (0x30000000u | (qwc))
#define DMATAG_CALL         0x50000000u
#define DMATAG_RET          0x60000000u
#define DMATAG_IRQ          0x80000000u     /* the IRQ bit: the channel stops at the tag */

/* VIF codes (CMD << 24 | NUM << 16 | IMMEDIATE). */
#define VIF_NOP             0x00000000u
#define VIF_STCYCL(cl, wl)  (0x01000000u | ((wl) << 8) | (cl))
#define VIF_ITOP(a)         (0x04000000u | (a))
#define VIF_FLUSH           0x11000000u
#define VIF_FLUSHA          0x13000000u
#define VIF_MSCAL(a)        (0x14000000u | (a))
#define VIF_STMASK          0x20000000u
#define VIF_DIRECT(qwc)     (0x50000000u | (qwc))
#define VIF_DIRECTHL(qwc)   (0x51000000u | (qwc))
/* UNPACK with FLG (addresses relative to the double-buffer base) and USN;
 * _M: with the STMASK write mask. */
#define VIF_UNPACK(fmt, num, addr)   (((u32)(fmt) << 24) | ((u32)(num) << 16) | 0xC000u | (addr))
#define VIF_UNPACK_M(fmt, num, addr) (VIF_UNPACK(fmt, num, addr) | 0x10000000u)

/* GIF tags: the low dword (NLOOP | EOP), the high dword (PRE, PRIM, FLG,
 * NREG) and its register list; GIF_HI_*: the same fields in the high
 * dword's upper word (as a u32). */
#define GIF_EOP             0x8000u
#define GIF_PRE             (1ull << 46)
#define GIF_FLG_REGLIST     (1ull << 58)
#define GIF_FLG_IMAGE       (2ull << 58)
#define GIF_NREG(n)         ((u64)(n) << 60)
#define GIF_REG(i, r)       ((u64)(r) << (4 * (i)))
#define GIF_REG_AD          0xEull          /* PACKED A+D: the data, then the register address */
#define GIF_HI_PRE          0x00004000u
#define GIF_HI_PRIM(p)      ((u32)(p) << 15)
#define GIF_HI_NREG(n)      ((u32)(n) << 28)

/* GsContext.dirty / written / enable: per-context register r (GS_REG_*) is
 * bit r (context 1) and 16 + r (context 2), general register g (GS_GEN_*)
 * bit 32 + g. */
#define GS_DIRTY_CTX1(r)        (1ull << (r))
#define GS_DIRTY_BOTH(r)        (GS_DIRTY_CTX1(r) | (1ull << (16 + (r))))
#define GS_DIRTY_REG_BOTH(r)    (0x10001ull << (r))
#define GS_DIRTY_GEN(g)         (1ull << (32 + (g)))
#define GS_DIRTY_ALL            0x000003FF1FFF1FFFull

/* Register fields of the shadows: f is a "shift, width" pair (GS_TEST_ATE
 * and the like, defined by the units that use them). */
#define GS_FIELD_MASK_(s, w)        (((1ull << (w)) - 1) << (s))
#define GS_FIELD_VAL_(s, w, v)      ((u64)((v) & ((1 << (w)) - 1)) << (s))
#define GS_FIELD_SET_(r, s, w, v)   (((r) & ~GS_FIELD_MASK_(s, w)) | GS_FIELD_VAL_(s, w, v))
#define GS_FIELD_GET_(r, s, w)      (((r) >> (s)) & ((1ull << (w)) - 1))
#define GS_FIELD_MASK(f)            GS_FIELD_MASK_(f)
#define GS_FIELD_VAL(f, v)          GS_FIELD_VAL_(f, v)
#define GS_FIELD_SET(r, f, v)       GS_FIELD_SET_(r, f, v)
#define GS_FIELD_GET(r, f)          GS_FIELD_GET_(r, f)

/* ======================================================================== */
/* Interrupt / DMA / display state                                          */
/* ======================================================================== */

/* Sleeping-thread record linked into the waiter lists (lives on the stack). */
typedef struct GfxWaitNode {
    struct GfxWaitNode *next;   /* 0x00 */
    int    tid;                 /* 0x04 the thread to wake */
} GfxWaitNode;
EGCS_SIZE(GfxWaitNode, 0x8);

/* Per DMA channel record (0 VIF0, 1 VIF1, 2 GIF) of GfxDriverState. */
typedef struct GfxDmaChan {
    volatile u8 tagIrq;         /* 0x00 channel stopped on a tag without IRQ bit */
    volatile u8 busy;           /* 0x01 transfer started, completion pending */
    u8     unk_02[2];           /* 0x02 */
    GfxWaitNode * volatile waiters; /* 0x04 threads sleeping in gfx_dma_sync */
    int    sema;                /* 0x08 channel mutex (gfx_dma_lock / gfx_dma_unlock) */
} GfxDmaChan;
EGCS_SIZE(GfxDmaChan, 0xC);

/* GS -> EE download record in the VIF1 chain, built by gs_download_image
 * after its transfer header.  The header's tag has the IRQ bit, so the
 * VIF1 chain stops in front of the record; once that DMAC event and the GS
 * FINISH have both come (GfxDriverState.finishCount == 2) gs_readback_begin
 * reads the record uncached at D1_TADR and starts the download of qwc
 * qwords to madr; gs_readback_end calls the callback and resumes the chain
 * at rec + 0x10 (the tag word itself is never run).  qwc is a union member as in the
 * bootstrap's gs_driver.h (egcs gives a union-member access alias set 0,
 * which keeps the qwc load ahead of the g_gfx.readback store as in the
 * binary). */
typedef struct GsReadbackRec {
    union {
        u16  qwc;           /* 0x00 tag QWC: the image size */
        u32  tag0;          /*      tag word 0 (QWC | cnt | IRQ) */
    } h;
    u32    madr;            /* 0x04 tag ADDR: the destination (SPR bit for scratchpad) */
    GsReadbackFunc callback; /* 0x08 GsContext.readbackCallback, NULL: none */
    u32    arg;             /* 0x0C GsContext.readbackArg */
} GsReadbackRec;
EGCS_SIZE(GsReadbackRec, 0x10);

/* GfxDriverState.vsyncCallback */
typedef void (*GfxVsyncFunc)(int count);

/* Interrupt / display state, 0xC0 bytes (g_gfx). */
typedef struct GfxDriverState {
    volatile u64 vblankCount;   /* 0x00 incremented by the vblank handler */
    volatile u64 lastVsync;     /* 0x08 count at the last gfx_vsync */
    GfxVsyncFunc vsyncCallback; /* 0x10 */
    volatile u8 field;          /* 0x14 GS CSR.FIELD sampled at vblank */
    volatile s8 finishCount;    /* 0x15 FINISH / VIF1 marker events of a readback */
    volatile s8 pathSync16;     /* 0x16 VIF1 <-> GIF chain rendezvous counters */
    volatile s8 pathSync17;     /* 0x17 */
    volatile s8 pathSync18;     /* 0x18 */
    u8     unk_19[3];           /* 0x19 */
    GfxDmaChan chan[3];         /* 0x1C VIF0, VIF1, GIF */
    GfxWaitNode * volatile vsyncWaiters; /* 0x40 threads in gfx_wait_vblank_event */
    struct GsReadbackRec * volatile readback; /* 0x44 download in progress */
    volatile u64 perfCount0;    /* 0x48 accumulated COP0 PCR0 (per vblank) */
    volatile u64 perfCount1;    /* 0x50 accumulated COP0 PCR1 */
    u64    pmode;               /* 0x58 PMODE shadow (0x66) */
    u64    smode2;              /* 0x60 SMODE2 shadow */
    u64    dispfb2;             /* 0x68 DISPFB2 shadow */
    u64    display2;            /* 0x70 DISPLAY2 shadow */
    u64    bgcolor;             /* 0x78 BGCOLOR shadow */
    u64    imr;                 /* 0x80 GS IMR shadow */
    u16    dby;                 /* 0x88 DISPFB2.DBY */
    u16    dbx;                 /* 0x8A DISPFB2.DBX */
    s16    dispDX;              /* 0x8C DISPLAY2 x offset */
    s16    dispDY;              /* 0x8E DISPLAY2 y offset */
    u16    dispW;               /* 0x90 display width */
    u16    dispH;               /* 0x92 display height */
    u16    screenW;             /* 0x94 screen width (MAGH divisor) */
    u16    screenH;             /* 0x96 screen height (MAGV divisor) */
    u8     gsRevision;          /* 0x98 GS CSR bits 23:16 */
    u8     inter;               /* 0x99 */
    u8     omode;               /* 0x9A */
    u8     ffmd;                /* 0x9B */
    u8     unk_9C[4];           /* 0x9C */
    int    intcIdVblank;        /* 0xA0 AddIntcHandler(INTC_VBLANK_S) id */
    int    dmacId[3];           /* 0xA4 AddDmacHandler ids, channels 0..2 */
    u8     unk_B0[4];           /* 0xB0 */
    int    intcIdVif1;          /* 0xB4 AddIntcHandler(INTC_VIF1) id */
    int    intcIdGs;            /* 0xB8 AddIntcHandler(INTC_GS) id */
    u8     unk_BC[4];           /* 0xBC */
} GfxDriverState;
EGCS_SIZE(GfxDriverState, 0xC0);

/* One EE DMAC channel register block (Dn_CHCR at 0x00; g_gsDmaChan[]). */
typedef struct EeDmaChanRegs {
    volatile u32 chcr;          /* 0x00 Dn_CHCR */
    u32    pad04[3];
    volatile u32 madr;          /* 0x10 Dn_MADR */
    u32    pad14[3];
    volatile u32 qwc;           /* 0x20 Dn_QWC */
    u32    pad24[3];
    volatile u32 tadr;          /* 0x30 Dn_TADR */
    u32    pad34[3];
    volatile u32 asr0;          /* 0x40 Dn_ASR0 */
    u32    pad44[3];
    volatile u32 asr1;          /* 0x50 Dn_ASR1 */
    u32    pad54[3];
    u32    pad60[8];            /* 0x60 */
    volatile u32 sadr;          /* 0x80 Dn_SADR */
} EeDmaChanRegs;

/* ======================================================================== */
/* Prebuilt VU1 models, 'Tex1' texture sets                                 */
/* ======================================================================== */

/* One VIF1 packet of a model (8 bytes). */
typedef struct VifPart {
    u32 *data;              /* 0x00 VIF codes (relocated by vif_model_relocate; data + 4: a GIFtag) */
    u16  qwc;               /* 0x04 */
    u16  tex;               /* 0x06 1-based texture index for partTexFunc (0 = none) */
} VifPart;
EGCS_SIZE(VifPart, 0x8);

/* A prebuilt VU1 model ("race/flare.bin", the sun discs, model data): per
 * part a VIF1 packet for the VU1 program at GsContext.vu1Prog. */
typedef struct VifModel {
    struct VifModel *self;  /* 0x00 address the pointers are relative to */
    void  *ptr04;           /* 0x04 relocated if non-NULL */
    u8     mode;            /* 0x08 bits 0..4 lighting mode, 5..7 vertex format; 0 = unlit (model_draw's fast path) */
    u8     scaleFlags;      /* 0x09 bit 0: colour scale 255 (else 128), bit 1: alpha scale 255 */
    u16    nParts;          /* 0x0A */
    u16    nVerts;          /* 0x0C statistics */
    u16    nPrims;          /* 0x0E statistics */
    VifPart part[1];        /* 0x10 nParts entries */
} VifModel;

/* The GS registers of one texture (bootstrap gs_driver.h): gfx_set_texture_regs
 * 0x0024CD48, texregs_*; Tex1Image.tex points to an array of them. */
typedef struct GsTexRegs {
    u64 tex0;               /* 0x00 TEX0 (bits 26-29: TW, log2 width) */
    u64 tex1;               /* 0x08 TEX1 */
    u64 miptbp1;            /* 0x10 MIPTBP1 (used when TEX1.MXL != 0) */
    u64 miptbp2;            /* 0x18 MIPTBP2 (used when TEX1.MXL >= 4) */
    u64 clamp;              /* 0x20 CLAMP: MAXU bits 14..23, MAXV bits 34..43 */
} GsTexRegs;
EGCS_SIZE(GsTexRegs, 0x28);

/* One upload record of a 'Tex1' texture set (Tex1Image.xfer, stride 0xC):
 * the pixels and the upper word of their BITBLTBUF (tex1_set_vram_base /
 * tex1_upload_*; u023 builds one on the stack for an icon upload). */
typedef struct Tex1Xfer {
    void *data;             /* 0x00 pixel data (relocated) */
    u16   dbp;              /* 0x04 BITBLTBUF.DBP (64-word blocks, relative to Tex1Image.vramBase) */
    u8    dbw;              /* 0x06 BITBLTBUF.DBW */
    u8    dpsm;             /* 0x07 BITBLTBUF.DPSM */
    u16   w;                /* 0x08 */
    u16   h;                /* 0x0A */
} Tex1Xfer;
EGCS_SIZE(Tex1Xfer, 0xC);

/* 'Tex1' texture set header (bootstrap startup_screen.h).  In the file every
 * pointer is an offset from the start (self = 0) and VRAM addresses are
 * relative to block 0 (vramBase = 0); the data start at 0x30. */
typedef struct Tex1Image {
    u32    magic;               /* 0x00 'Tex1' (0x31786554; never checked) */
    struct Tex1Image *self;     /* 0x04 address the pointers are relative to */
    void  *ptr08;               /* 0x08 relocated if non-NULL */
    u32    size;                /* 0x0C bytes to the next image of a pack */
    u16    vramBase;            /* 0x10 VRAM base the set is linked for (64-word blocks) */
    u16    vramSize;            /* 0x12 VRAM footprint in 64-word blocks */
    u16    nTex;                /* 0x14 GsTexRegs entries */
    u16    nXfer;               /* 0x16 Tex1Xfer entries */
    struct GsTexRegs *tex;      /* 0x18 per-texture GS register blocks (stride 0x28) */
    struct Tex1Xfer *xfer;      /* 0x1C upload list */
    u32   *ptrTable;            /* 0x20 {count, ptr[count]}: picture / CLUT variations */
    struct ClutAnimTable *clutAnim; /* 0x24 */
    u8     unk_28[8];           /* 0x28 zero in startupa.img */
} Tex1Image;
EGCS_SIZE(Tex1Image, 0x30);

/* One glyph of a bitmap font (u114), 16 bytes. */
typedef struct FontGlyph {
    s8  bearingX;           /* 0x00 x offset of the cell (lbu + sign extension) */
    s8  bearingY;           /* 0x01 y offset of the cell */
    u8  w;                  /* 0x02 cell width (0: nothing drawn) */
    u8  h;                  /* 0x03 cell height */
    u8  advance;            /* 0x04 */
    u8  u;                  /* 0x05 texel position in the font texture */
    u16 v;                  /* 0x06 */
    const s8 *left;         /* 0x08 left kerning profile, one byte per row (relocated) */
    const s8 *right;        /* 0x0C right kerning profile (relocated) */
} FontGlyph;
EGCS_SIZE(FontGlyph, 0x10);

/* A bitmap font face, one entry of a FontSet (font_set selects it).  u114
 * called it Font, u109 FontFace (T496 Font keeps them in a dictionary by
 * name; Font +0x244 is the current one); T499 reads width / lineHeight. */
typedef struct PglFont {
    const char *name;       /* 0x00 relocated; the key of T496's dictionary */
    FontGlyph  *glyphs;     /* 0x04 glyph of code first..last (relocated) */
    u16  first;             /* 0x08 */
    u16  last;              /* 0x0A */
    u8   width;             /* 0x0C character width (T499 text_char_width) */
    u8   lineHeight;        /* 0x0D character (line) height */
    s8   ascent;            /* 0x0E line top to baseline: font_apply_align's top alignment moves the pen down by it */
    u8   kernRows;          /* 0x0F rows of the kerning profiles */
    u8   kanjiW;            /* 0x10 kanji cell width (font_put_kanji) */
    u8   kanjiH;            /* 0x11 kanji cell height */
    s8   kanjiX;            /* 0x12 kanji cell x offset */
    s8   kanjiY;            /* 0x13 kanji cell y offset */
    u8   unk_14[8];         /* 0x14 */
    Tex1Image *tex;         /* 0x1C its texture (FontSet.tex[i]); u109 passes &tex->tex[palette] to font_set */
} PglFont;

/* A font-set file as loaded (fontset_relocate relocates it in place; u109:
 * FontFile, T496 Font.files[]). */
typedef struct FontSet {
    u32  magic;             /* 0x00 */
    struct FontSet *self;   /* 0x04 load address it was relocated for */
    void *unk_08;           /* 0x08 relocated if non-NULL */
    u32  unk_0C;            /* 0x0C */
    u16  count;             /* 0x10 faces (and textures) */
    u16  unk_12;            /* 0x12 */
    PglFont   **fonts;      /* 0x14 count faces (entries may be NULL) */
    Tex1Image **tex;        /* 0x18 count Tex1 images (tex1_relocate) */
} FontSet;

/* The kanji font: 4-bit glyphs uploaded one at a time into a VRAM cell
 * (font_put_kanji).  It is record 0 of the kanji font file (T496
 * Font::texBase), passed to font_set_kanji by Font::begin (T498). */
typedef struct KanjiFont {
    u16        lo;          /* 0x00 first code (JIS row/cell & 0x7F7F) */
    u16        hi;          /* 0x02 last code */
    const u8  *data;        /* 0x04 glyph images (relocated) */
    s32        glyphBytes;  /* 0x08 bytes per glyph */
    u16        w;           /* 0x0C glyph width (pixels) */
    u16        h;           /* 0x0E glyph height */
    const u16 *map;         /* 0x10 code - lo -> glyph index (relocated) */
    const void *unk_14;     /* 0x14 relocated by kanjifile_relocate, not read by G001 */
} KanjiFont;
EGCS_SIZE(KanjiFont, 0x18);

/* The kanji font file as loaded (kanjifile_relocate relocates it in place;
 * T496 Font.kanjiFile, read by Font::loadKanjiFont).  The header has FontSet's
 * layout. */
typedef struct KanjiFontFile {
    u32    magic;           /* 0x00 */
    struct KanjiFontFile *self; /* 0x04 load address it was relocated for */
    void  *unk_08;          /* 0x08 relocated if non-NULL */
    u32    unk_0C;          /* 0x0C */
    u16    count;           /* 0x10 */
    u16    unk_12;          /* 0x12 */
    KanjiFont **fonts;      /* 0x14 count records (entries may be NULL); [0] -> T496 Font::texBase */
    const u32  *cluts;      /* 0x18 16-colour CLUTs, 0x40 bytes each (Font::clutBase; Font::begin
                             *      passes cluts + palette * 0x10 to font_set_kanji) */
} KanjiFontFile;
EGCS_SIZE(KanjiFontFile, 0x1C);

/* The library state block g_002F72C0 (.data, initial values in brackets).
 * One object: every access goes through the base 0x002F72C0, and the next
 * datum (u115's 10^18 table, 0x002F73A8) follows at +0xE8, so the block is
 * 8-aligned (matScale is not a GsVec4 array: that would make it 0xF0).
 *   +0x00 the texture / PRIM override, the bootstrap's GsTexOverride
 *         (s_gsTexOverride): gs_tex_override_enable / _disable (GS_OVR_*),
 *         applied by gfx_set_texture_regs and vifmodel_put_part (u115);
 *   +0x20 the material colour scales (mat_scale_set), used by material_load;
 *   +0x68 the bitmap text renderer (font_*). */
typedef struct GsLibState {
    u64    tex0Mask;        /* 0x00 TEX0 bits kept from the context [0xE000001800000000: CLD, TFX] */
    u64    tex1Mask;        /* 0x08 TEX1 bits kept [0x1E0: MMAG, MMIN] */
    u64    clampMask;       /* 0x10 CLAMP bits kept [0] */
    u16    primMask;        /* 0x18 PRIM bits of prebuilt models taken from GsContext.primAttr [0x60] */
    u16    unk_1A;          /* 0x1A [1] */
    u8     enable;          /* 0x1C the override on (GS_OVR_ENABLE) [0] */
    u8     unk_1D[3];       /* 0x1D */
    f32    matScale[4][4];  /* 0x20 rgba scale of ambient, diffuse, specular, emission [1.0] (lqc2: 16-aligned by position) */
    u8     matScaleOn;      /* 0x60 GS_OVR_MAT_SCALE: material_load scales flagged materials [0] */
    u8     unk_61[7];       /* 0x61 */
    u64    fontTex0;        /* 0x68 TEX0_1 of the font texture (font_set) */
    u64    fontTex1;        /* 0x70 TEX1_1 */
    const PglFont *font;    /* 0x78 the current face */
    s32    align;           /* 0x7C font_set_align: FONT_ALIGN_* (bits 0-1 horizontal, bits 2-3 vertical) */
    s32    lastChar;        /* 0x80 previous glyph code (kerning), -1 none [-1] */
    u32    lastPrim;        /* 0x84 PRIM of the open sprite run */
    u32    lastColor;       /* 0x88 RGBAQ of the open sprite run [0x80808080] */
    u64   *pkt;             /* 0x8C GIF data of the open text packet (GsContext.cur + 0x10), NULL none */
    u64   *run;             /* 0x90 REGLIST GIF tag of the open sprite run, NULL none */
    u64   *pktEnd;          /* 0x94 packet write pointer */
    f32    maxX;            /* 0x98 widest line end so far */
    f32    spacingX;        /* 0x9C font_set_param 0x1102 */
    f32    spacingY;        /* 0xA0 font_set_param 0x1103 */
    f32    scaleX;          /* 0xA4 font_set_param 0x1100 [1.0] */
    f32    scaleY;          /* 0xA8 font_set_param 0x1101 [1.0] */
    f32    startX;          /* 0xAC line start (GS pixel units) */
    f32    startY;          /* 0xB0 */
    f32    startZ;          /* 0xB4 written by font_locate_3d */
    f32    startW;          /* 0xB8 */
    f32    curX;            /* 0xBC pen */
    f32    curY;            /* 0xC0 */
    u8     unk_C4[8];       /* 0xC4 */
    u8     open;            /* 0xCC a text packet is open (font_begin / font_end) */
    u8     unk_CD[3];       /* 0xCD */
    const struct KanjiFont *kfont; /* 0xD0 font_set_kanji */
    const void *kclut;      /* 0xD4 kanji CLUT, 16 colours (8x2 PSMCT32, 0x40 bytes) */
    s32    kvram;           /* 0xD8 kanji cell VRAM word address, its CLUT at +0x7C0 [-1] */
    u8     kinit;           /* 0xDC the kanji CLUT / context-2 registers are in the current packet */
    u8     unk_DD[3];       /* 0xDD */
    char  *out;             /* 0xE0 output pointer of fmt_vsprintf (sprintf_putc) */
} GsLibState;
EGCS_SIZE(GsLibState, 0xE8);

/* ======================================================================== */
/* Globals                                                                  */
/* ======================================================================== */

extern GfxDriverState g_gfx;        /* 0x004A69C0 .bss, the driver state */
extern u8 *g_pglAttribSp;           /* 0x003530DC gp-0x4914: attribute stack pointer of pgl_push_attrib /
                                     * pgl_pop_attrib (byte stack, grows up), set to g_002FB1E8 by gfx_init */
/* G001's .data (no FDEs, so it lies before T528's frame block; only u113 ..
 * u118 use these). */
extern struct GsLibState g_002F72C0;            /* 0x002F72C0 the library state block (GsLibState) */
extern EeDmaChanRegs *g_gsDmaChan[3];           /* 0x002FC1E8 {D0, D1, D2} register bases */

/* ======================================================================== */
/* Functions (address order, by file)                                       */
/* ======================================================================== */

/* The printf engine's character output. */
typedef int (*PglPutcFunc)(int c);

/* vu0_math.c */
f32  vu0_atan2f(f32 y, f32 x);                              /* 0x00245C10 */
f32  vu0_rand(void);                                        /* 0x00245D5C [0, 1) (vrnext) */
void vu0_rand_seed(u32 lcgSeed, f32 seed);                  /* 0x00245D78 */
void lcg_rand_seed(u32 seed);                               /* 0x00245D88 */
f32  lcg_rand12(void);                                      /* 0x00245D90 [1, 2) */
f32  lcg_rand_range(f32 lo, f32 hi);                        /* 0x00245DC8 */

/* particles.c */
int  weather_draw_quad(f32 x, f32 y, f32 z, f32 size);      /* 0x00245F30 */
void weather_init(struct Weather *w, struct WeatherDrop *drops, int count); /* 0x002461D8 */
void weather_set(struct Weather *w, f32 size, f32 spreadXZ, f32 spreadY, f32 ahead,
                 f32 velXZ, f32 velY);                      /* 0x00246290 */
void weather_update(struct Weather *w);                     /* 0x00246320 */
/* addr: not read (u045's drawView passes the target buffer's VRAM address,
 * buf->addr, in $a1). */
void weather_draw(struct Weather *w, u32 addr);             /* 0x00246380 */

/* flare.c */
VifModel *flare_sun_model_a(void);                          /* 0x002466F0 */
VifModel *flare_sun_model_b(void);                          /* 0x00246700 */
void flare_build_disc(void);                                /* 0x00246710 */
VifModel *flare_disc_model(void);                           /* 0x002469E0 */
void flare_build_rays(void);                                /* 0x002469F0 */
VifModel *flare_rays_model(void);                           /* 0x00246D28 */
/* results: a group of 4 occlusion result texels (UNPACK V4-8 x4), index
 * the one of the group; (r, g, b) a colour (? every caller passes one). */
void vif_model_draw(struct FlareWork *fw, VifModel *m, const void *results, u32 index,
                    f32 r, f32 g, f32 b);                   /* 0x00247690 */
/* screen*: the screen buffer (FlareClass::begin: GSBuffer addr, width,
 * displayHeight), whose alpha the occlusion queries draw into. */
void flare_init(struct FlareWork *fw, f32 aspect, int screenVram, int screenW, int screenH,
                int occVram, int occH, int workVram, int workW,
                int workH, Tex1Image *tex0, Tex1Image *tex1); /* 0x00247948 */
void flare_readback(struct FlareWork *fw, u32 *results);    /* 0x00247D50 */
int  flare_query_screen(struct FlareWork *fw, const f32 *screen, f32 scale, f32 size); /* 0x00247EF0 */
void flare_draw_glow(struct FlareWork *fw, int flags, const f32 *pos, VifModel *m,
                     f32 near, f32 r, f32 g, f32 b, f32 a, f32 scale,
                     f32 haloA, f32 haloScale);             /* 0x00247FE8 */
void flare_draw_streak(struct FlareWork *fw, const f32 *pos, const f32 *axes,
                       VifModel *unused, f32 near, f32 r, f32 g, f32 b, f32 a); /* 0x002481F0 */
int  flare_query_sun(struct FlareWork *fw, const f32 *pos, f32 scale, f32 size); /* 0x00248378 */
void flare_draw_sun(struct FlareWork *fw, const f32 *pos, VifModel *m, f32 scale,
                    f32 r, f32 g, f32 b);                   /* 0x00248458 */
void flare_draw_sun_disc(struct FlareWork *fw, const f32 *pos, f32 r, f32 g, f32 b,
                         f32 a, f32 scale);                 /* 0x00248538 */
void vu0_sync_state(void);                                  /* 0x00248680 */
void vu0_load_screen_regs(void);                            /* 0x002486B8 vf20..vf23 = vBB0, limits, viewCenter, viewScale */
void vu0_load_screen_regs_nosync(void);                     /* 0x00248700 */
void vu0_load_modelview(void);                                 /* 0x00248720 vf20..vf23 = the MODELVIEW matrix (current or parked) */
void vu0_load_projection(void);                                 /* 0x00248768 vf20..vf23 = the projection matrix */

/* tex1.cpp */
/* gs_tex_override_enable / _disable switches (the head of g_002F72C0). */
enum {
    GS_OVR_ENABLE    = 0x1000,  /* the override on: the masks below apply */
    GS_OVR_MAT_SCALE = 0x1001,  /* material_load scales flagged materials (matScaleOn) */
    GS_OVR_PRIM_IIP  = 0x1100,  /* PRIM bits prebuilt models take from GsContext.primAttr */
    GS_OVR_PRIM_TME  = 0x1101,
    GS_OVR_PRIM_FGE  = 0x1102,
    GS_OVR_PRIM_ABE  = 0x1103,
    GS_OVR_PRIM_AA1  = 0x1104,
    GS_OVR_TEX0_TFX  = 0x1105,  /* texture register fields gfx_set_texture_regs keeps from the context */
    GS_OVR_TEX1_MMAG = 0x1106,
    GS_OVR_TEX1_MMIN = 0x1107,
    GS_OVR_CLAMP_U   = 0x1108,  /* CLAMP WMS, MINU, MAXU */
    GS_OVR_CLAMP_V   = 0x1109   /* CLAMP WMT, MINV, MAXV */
};
Tex1Image *tex1_set_vram_base(Tex1Image *img, u32 vramWordAddr); /* 0x002487B8 */
void gs_tex_override_enable(u32 what);                      /* 0x00248A30 GS_OVR_* */
void gs_tex_override_disable(u32 what);                     /* 0x00248BB8 */
VifModel *vif_model_relocate(VifModel *m);                  /* 0x00248D48 */
Tex1Image *tex1_relocate(Tex1Image *img);                   /* 0x00248DA8 */
void tex1_select_variation(Tex1Image *img, int index);      /* 0x00248EC8 */
void clut_anim_apply_all(Tex1Image *img);                   /* 0x00249110 */
void clut_anim_step(Tex1Image *img, float dt, int id);      /* 0x00249180 (u102's callers set $f12 before $a1: dt is declared first) */
void clut_anim_release(Tex1Image *img, int id);             /* 0x002494A0 */
/* The enhanced engine (rendering.md 1, backend.md 5; Lupo 2 L2 0x00264720):
 * resets the channels of animation `id' (every channel when id < 0) and
 * marks them dirty; with id < 0 it also applies them at once, as GT3 does
 * (the prize models rely on it), else the next clut_anim_apply_all does.
 * C++ callers that pass no id get -1 (GT3's call). */
#ifdef __cplusplus
void clut_anim_reset_all(Tex1Image *img, int id = -1);      /* 0x00249518 */
#else
void clut_anim_reset_all(Tex1Image *img, int id);           /* 0x00249518 */
#endif
u32  tex1_variation_count(Tex1Image *img);                  /* 0x002496B0 */
void tex1_upload_packet(Tex1Image *img);                    /* 0x002496D0 */
void tex1_upload_to_vram(Tex1Image *img);                   /* 0x002497F0 */
void ring_draw(u32 colorInner, u32 colorOuter, int n, f32 deg0, f32 deg1,
               f32 width);                                  /* 0x002498C8 the ring of gt2k_menu::CircleGauge */

/* font.c */
struct FontSet *fontset_relocate(struct FontSet *fs);       /* 0x00249B40 */
struct KanjiFontFile *kanjifile_relocate(struct KanjiFontFile *f);          /* 0x00249CE8 relocates the kanji font file (Font::loadKanjiFont) */
void font_set(const struct PglFont *font, const struct GsTexRegs *tex); /* 0x00249DB8 font and its TEX0 / TEX1 */
void font_set_kanji(const struct KanjiFont *kf, const void *clut, s32 vram); /* 0x00249DE8 */
void font_set_param(u32 param, f32 v);                      /* 0x00249E00 0x1100 scaleX, 0x1101 scaleY, 0x1102 / 0x1103 spacing */
f32  font_get_param(u32 param);                             /* 0x00249E68 */
/* font_set_align bits (GsLibState.align; T496 Font::setAlign passes them
 * on): bits 0-1 horizontal, bits 2-3 vertical.  0: the pen is the left end
 * of the baseline (font_apply_align). */
enum {
    FONT_ALIGN_LEFT    = 0x0,
    FONT_ALIGN_RIGHT   = 0x2,       /* the pen is the right end */
    FONT_ALIGN_HCENTER = 0x3,
    FONT_ALIGN_H_MASK  = 0x3,
    FONT_ALIGN_TOP     = 0x4,       /* the pen is the top of the first line */
    FONT_ALIGN_BOTTOM  = 0x8,       /* the pen is the bottom of the last line */
    FONT_ALIGN_VCENTER = 0xC,
    FONT_ALIGN_V_MASK  = 0xC,
    FONT_ALIGN_CENTER  = 0xF        /* centred both ways */
};
void font_set_align(s32 align);                             /* 0x00249EE0 FONT_ALIGN_* */
void font_locate(s32 x, s32 y);                             /* 0x0024A1D0 */
void font_locate_f(f32 x, f32 y);                           /* 0x0024A208 */
int  font_locate_3d(const f32 *pos);                        /* 0x0024A378 */
void font_begin(void);                                      /* 0x0024A4E8 */
void font_end(void);                                        /* 0x0024A5F8 */
void font_puts(const u8 *s);                                /* 0x0024A658 */
int  font_putc(int c);                                      /* 0x0024AB50 */
void font_vprintf(const char *fmt, va_list ap);             /* 0x0024B0D8 */
void font_apply_align(struct GsLibState *st, const char *fmt, va_list ap); /* 0x0024B178 */
int  fmt_sprintf(char *buf, const char *fmt, ...);          /* 0x0024B3C8 (F_0024B3C8) sprintf on fmt_vprintf */
int  fmt_vsprintf(char *buf, const char *fmt, va_list ap);  /* 0x0024B400 (F_0024B400) vsprintf */
f32  font_text_width(const u8 *s);                          /* 0x0024B440 */
f32  font_text_height(const s8 *s);                         /* 0x0024B538 */

/* vecmath.c */
void glu_look_at(f32 ex, f32 ey, f32 ez, f32 cx, f32 cy, f32 cz,
                 f32 ux, f32 uy, f32 uz);                   /* 0x0024B658 */
void mat_reflect(f32 px, f32 py, f32 pz, f32 nx, f32 ny, f32 nz); /* 0x0024B7B0 */
void proj_shift_depth(f32 d);                               /* 0x0024B888 */

/* texregs.c */
void texregs_init(struct GsTexRegs *t);                     /* 0x0024B930 */
void texregs_set_buffer(struct GsTexRegs *t, u32 level, u32 addr, s32 psm, s32 width); /* 0x0024B958 */
void texregs_set_size(u64 *tex0, s32 w, s32 h);             /* 0x0024BBA0 */
void texregs_set_mode(struct GsTexRegs *t, u32 what, s32 v); /* 0x0024BC68 */
void texregs_set_clamp_region(struct GsTexRegs *t, s32 axis, s32 lo, s32 hi); /* 0x0024BDC8 */
s32  tex_vram_size(u32 psm, s32 w, s32 h, s32 *alignedW);   /* 0x0024BE68 */

/* render.c */
int  box_visible(const f32 *lo, const f32 *hi);             /* 0x0024BF80 corners (x, y, z), read as words */
void mat_scale_set_grey(u32 kind, f32 v);                   /* 0x0024C098 */
void draw_poly2d(const f32 *xy, const f32 *uv, const u32 *rgba, u32 n); /* 0x0024C148 */
void strip_batch_begin(void);                               /* 0x0024C488 */
void strip_batch_end(void);                                 /* 0x0024C4B0 */
/* The buffers go to VU1 by DMA ref tags (their physical addresses); NULL:
 * none (xyz2, st, rgba). */
void strip_batch_add(const void *xyz, const void *xyz2, const void *rgba, const void *st,
                     s32 n, const u8 *strips);           /* 0x0024C6B0 */
void strip_draw(const void *xyz, const void *xyz2, const void *rgba, const void *st,
                s32 n);                                   /* 0x0024C940 */
void tri_draw16(const void *xyzw16, const void *unused, const void *rgba, const void *st16,
                s32 n);                                   /* 0x0024C9A8 */
void pgl_set_part_tex_func(PglPartTexFunc fn);              /* 0x0024CCB0 (F_0024CCB0) GsContext.partTexFunc = fn */
void material_load(const GsMaterial *m);                    /* 0x0024CCC0 */
void gfx_set_texture_regs(const struct GsTexRegs *t);       /* 0x0024CD48 */
struct GsTexRegs *texsel_get_table2(void);                  /* 0x0024CEE8 */
void matsel_set_base(s32 base);                             /* 0x0024CEF0 */
void matsel_set_table2(GsMaterial *t);                      /* 0x0024CEF8 */
void matsel_set_table(GsMaterial *t);                       /* 0x0024CF08 */
void texsel_set_base(s32 base);                             /* 0x0024CF18 */
void texsel_set_table2(struct GsTexRegs *t);                /* 0x0024CF20 */
void texsel_set_table(struct GsTexRegs *t);                 /* 0x0024CF30 */
void texmat_select(s32 id);                                 /* 0x0024CF40 a PglPartTexFunc */
void model_draw(VifModel *m);                               /* 0x0024D048 */
u32 *vu1_unpack_xyz(s32 n);                                 /* 0x0024D0C0 */

/* fontfmt.c */
int  fmt_put_string(PglPutcFunc put, u32 flags, s32 width, s32 prec, const u8 *s); /* 0x0024D4F8 */
int  fmt_put_number(PglPutcFunc put, u32 flags, s32 width, s32 prec, s8 sign,
                    const char *digits, s32 ndigits, s32 base); /* 0x0024D670 */

/* format_part2.c */
int  fmt_put_decimal(PglPutcFunc put, int flags, int width, int prec, u64 v,
                     int sign, int fixed);                  /* 0x0024D8C0 */
int  fmt_put_octal(PglPutcFunc put, int flags, int width, int prec, u64 v,
                   int sign);                               /* 0x0024D9C0 */
int  fmt_put_hex(PglPutcFunc put, int flags, int width, int prec, u64 v,
                 int conv);                                 /* 0x0024DA38 */
int  fmt_vprintf(PglPutcFunc put, const u8 *fmt, const u8 *args); /* 0x0024DB30 %d %u %t %o %i %x %X %c %s */

/* vu1_lighting.c */
void vifmodel_draw_lit(const VifModel *m, u32 prog);        /* 0x0024E2B8 */
void vifmodel_draw(const VifModel *m, u32 prog);            /* 0x0024E4A0 */
void pgl_sync_matrices(GsContext *ctx);                     /* 0x0024E670 */
void pgl_send_matrix(GsContext *ctx, u32 vuAddr);           /* 0x0024E7F0 */
void pgl_send_dirty_regs(GsContext *ctx, u32 vuAddr);       /* 0x0024E858 */
void pgl_set_vec_param(u32 kind, const f32 *v);             /* 0x0024F4E0 */
void pgl_set_scalar_param(u32 kind, f32 v);                 /* 0x0024F580 */
void pgl_set_envmap_params(f32 a, f32 b);                   /* 0x0024F7A8 */
void pgl_envmap_rotate(f32 x, f32 y, f32 z);                /* 0x0024F7B8 */
void pgl_envmap_point_at(f32 x, f32 y, f32 z);              /* 0x0024F900 */
void pgl_envmap_set_basis(f32 ax, f32 ay, f32 az, f32 bx, f32 by, f32 bz); /* 0x0024F928 */

/* vu1_prim.c */
void *pgl_light_vertices(GsContext *ctx, const void *xyz, const void *normals,
                         const void *colors, const void *uvs, int n); /* 0x0024FC48 (u115: pgl_shade_vertices) */
void pgl_put_texcoords(GsContext *ctx, const void *uvs, int n); /* 0x0024FE00 */
void pgl_draw_strip(const void *xyz, const void *normals, const void *colors,
                    const void *uvs, int n);                /* 0x002501A8 */

/* gs_driver_core.c */
void gs_flush_dirty_regs(void);                             /* 0x00250208 */
u64  gfx_perf_counter0(void);                               /* 0x002502C8 */
void gfx_init_vu0_constants(GsContext *ctx);                /* 0x002503E0 */
void gfx_init_lights(GsContext *ctx);                       /* 0x002504E8 */
void gfx_init_material(GsContext *ctx);                     /* 0x002505C0 */
void gfx_reset_texture_state(GsContext *ctx);               /* 0x00250690 */
void gfx_init_default(int omode);                           /* 0x00250808 */
void gfx_init(int fullReset, int inter, int omode, int ffmd); /* 0x00250820 */

/* gs_display_driver.c */
/* gfx_enable / gfx_disable capabilities (glEnable style); 0, 0x15..0x63 and
 * >= 0x6D do nothing.  A register switch marks its register dirty. */
enum {
    PGL_CAP_PATH3          = 0x01,  /* on: PATH3 unmasked, off: masked (path3Masked; VIF MSKPATH3 in an active packet) */
    PGL_CAP_TEXTURE        = 0x02,  /* PRIM.TME (primAttr) */
    PGL_CAP_DEST_ALPHA     = 0x03,  /* TEST.DATE, the destination alpha test (gfx_dest_alpha_func) */
    PGL_CAP_ALPHA_TEST     = 0x04,  /* TEST.ATE (gfx_alpha_func) */
    PGL_CAP_DEPTH_TEST     = 0x05,  /* depthTest; on: ZTST zTestMethod, ZMSK zMask; off: ZTST ALWAYS, ZMSK 1 */
    PGL_CAP_BLEND          = 0x06,  /* PRIM.ABE (gfx_blend_func) */
    PGL_CAP_DITHER         = 0x07,  /* DTHE */
    PGL_CAP_FOG            = 0x08,  /* PRIM.FGE */
    PGL_CAP_ANTIALIAS      = 0x09,  /* PRIM.AA1 */
    PGL_CAP_TEXGEN         = 0x0A,  /* texGenEnable */
    PGL_CAP_COLORGEN       = 0x0B,  /* colorGenEnable */
    PGL_CAP_COLOR_CLAMP    = 0x0C,  /* COLCLAMP */
    PGL_CAP_COLOR_MATERIAL = 0x0D,  /* colorMaterial */
    PGL_CAP_FLAG24         = 0x0E,  /* flag24 */
    PGL_CAP_CULL_FACE      = 0x0F,  /* cullEnable (then gs_select_vu1_prog) */
    PGL_CAP_FLAG27         = 0x10,  /* flag27 */
    PGL_CAP_VU1_PROG_33E   = 0x11,  /* vu1ProgAlt (then gs_select_vu1_prog) */
    PGL_CAP_SCALE          = 0x12,  /* scaleEnable; on and off set matrixDirty */
    PGL_CAP_TEX_MATRIX     = 0x13,  /* texMatrixEnable */
    PGL_CAP_NORMAL_MATRIX  = 0x14,  /* normalMtxEnable */
    PGL_CAP_CPU_LIGHTING   = 0x64,  /* lightFlags 0x80 */
    PGL_CAP_LIGHT0         = 0x65   /* + i (0..7): lights[i].enable, lightFlags |= 1 */
};
/* gfx_cull_face (glCullFace) and gfx_front_face (glFrontFace).
 * With culling on, gs_select_vu1_prog adds 0xA to the program for (BACK,
 * CCW) and (FRONT, CW), 0x14 for (FRONT, CCW) and (BACK, CW); FRONT_AND_BACK
 * gives program 0 (nothing drawn).  The rear-view mirror (T131
 * RacePS2Base::drawMirror) draws with PGL_FRONT_CW. */
enum { PGL_CULL_FRONT = 1, PGL_CULL_BACK = 2, PGL_CULL_FRONT_AND_BACK = 3 };
enum { PGL_FRONT_CCW = 0, PGL_FRONT_CW = 1 };
/* gfx_set_packet_access modes (GsContext.uncachedBuf, segment addrBits 0,
 * 0x20000000, 0x30000000). */
enum { PGL_PKT_CACHED = 0, PGL_PKT_UNCACHED = 1, PGL_PKT_UNCACHED_ACCEL = 2 };
/* gfx_select_list lists (GsContext.curList). */
enum { PGL_LIST_VIF1 = 0, PGL_LIST_GIF = 1 };
/* gfx_put_irq_marker codes, counted by u118's VIF1 / DMAC handlers in
 * g_gfx.pathSync16 / 17: a WAIT stalls its list until the other list has
 * passed a SIGNAL.  READBACK (pathSync18), put on both lists by
 * gs_download_image: the GIF list stops at it until the download is over
 * (gs_readback_end restarts GIF DMA), the VIF1 list waits at it until the
 * GIF list has got there. */
enum { PGL_MARK_WAIT = 0x10000, PGL_MARK_SIGNAL = 0x20000, PGL_MARK_READBACK = 0x40000 };
/* gfx_depth_func: GL-style functions, mapped to TEST.ZTST by the table at
 * 0x002FC1F8 {NEVER, ALWAYS, GREATER, GEQUAL} (GS Z grows towards the
 * viewer: gs_set_clear_depth stores 1 - d). */
enum { PGL_DEPTHFUNC_NEVER = 0, PGL_DEPTHFUNC_ALWAYS = 1, PGL_DEPTHFUNC_LESS = 2, PGL_DEPTHFUNC_LEQUAL = 3 };
/* gfx_alpha_func / gfx_alpha_func_raw: TEST.ATST.  gfx_dest_alpha_func takes
 * GEQUAL (pass where the frame buffer alpha bit is 1: DATM 1) or LESS (0). */
enum {
    PGL_ATEST_NEVER = 0, PGL_ATEST_ALWAYS = 1, PGL_ATEST_LESS = 2, PGL_ATEST_LEQUAL = 3,
    PGL_ATEST_EQUAL = 4, PGL_ATEST_GEQUAL = 5, PGL_ATEST_GREATER = 6, PGL_ATEST_NOTEQUAL = 7
};
/* gfx_alpha_fail: TEST.AFAIL, what a pixel failing the alpha test writes. */
enum { PGL_AFAIL_KEEP = 0, PGL_AFAIL_FB_ONLY = 1, PGL_AFAIL_ZB_ONLY = 2, PGL_AFAIL_RGB_ONLY = 3 };
/* gfx_blend_func / gfx_blend_func_raw selectors: the low byte of GS ALPHA,
 * colour = (A - B) * C / 128 + D; A, B, D: PGL_SEL_CS (source), _CD (frame
 * buffer), _ZERO; C: PGL_SEL_AS (source alpha), _AD (frame buffer alpha),
 * _FIX (the FIX byte, gfx_blend_func's fix * 128). */
#define PGL_BLEND_SEL(a, b, c, d)  ((a) | (b) << 2 | (c) << 4 | (d) << 6)
enum { PGL_SEL_CS = 0, PGL_SEL_CD = 1, PGL_SEL_ZERO = 2 };
enum { PGL_SEL_AS = 0, PGL_SEL_AD = 1, PGL_SEL_FIX = 2 };
/* The blend modes the callers use, by their formula. */
enum {
    PGL_BLEND_ALPHA      = 0x44,    /* (CS, CD, AS, CD): (Cs - Cd) * As + Cd */
    PGL_BLEND_ADD        = 0x48,    /* (CS, ZERO, AS, CD): Cs * As + Cd */
    PGL_BLEND_ADD_DEST_A = 0x58,    /* (CS, ZERO, AD, CD): Cs * Ad + Cd */
    PGL_BLEND_FIX        = 0x64,    /* (CS, CD, FIX, CD): (Cs - Cd) * FIX + Cd */
    PGL_BLEND_ADD_FIX    = 0x68,    /* (CS, ZERO, FIX, CD): Cs * FIX + Cd */
    PGL_BLEND_SRC_ALPHA  = 0x88     /* (CS, ZERO, AS, ZERO): Cs * As */
};
/* gfx_set_field_mode: XYOFFSET.OFY = tgtA.yoff + an offset (8: half a pixel
 * in 12.4, one frame line in a half-height field buffer), and halfHeight. */
enum {
    PGL_FIELD_FRAME       = 0,  /* full height, + 0 */
    PGL_FIELD_HALF_CUR    = 1,  /* half height, + field * 8 */
    PGL_FIELD_HALF_OTHER  = 2,  /* half height, + (1 - field) * 8 */
    PGL_FIELD_HALF_EVEN   = 3,  /* half height, + 0 */
    PGL_FIELD_HALF_ODD    = 4,  /* half height, + 8 */
    PGL_FIELD_FRAME_SHIFT = 9   /* full height, + field * 8; fieldShift = 1 */
};
/* gs_clear_buffers mask */
enum { PGL_CLEAR_COLOR = 1, PGL_CLEAR_DEPTH = 2, PGL_CLEAR_TARGET_B = 4 };
/* gfx_frame_effect kinds: A = tgtA (the draw buffer), B = tgtB, f = amount
 * (FIX = f * 128, clamped to 0..255). */
enum {
    PGL_FX_ADD_A_TO_B     = 0,  /* B += A * f (f <= 0: B -= A * -f) */
    PGL_FX_COPY_A_TO_B    = 1,  /* B = A * f */
    PGL_FX_ADD_WHITE      = 2,  /* B += white * f (f <= 0: B -= white * -f) */
    PGL_FX_SCALE_B        = 3,  /* B = B * f */
    PGL_FX_COPY_B_TO_A    = 4,  /* A = B * f (at the integer XYOFFSET) */
    PGL_FX_MIX_A_INTO_B   = 5,  /* B = lerp(B, A, f) */
    PGL_FX_ALPHA_A_INTO_B = 6,  /* B = lerp(B, A, A's alpha) */
    PGL_FX_FIELD_BLEND    = 7,  /* B = A with lines r, r + 1 averaged, then lerp(B, A, f) (? deflicker); the pairing by field / fieldShift */
    PGL_FX_FIELD_BLEND_INV = 8  /* the same for the other field */
};
/* gfx_tex_param pnames (the register field, its width). */
enum {
    PGL_TEXPARAM_TCC      = 0,  /* TEX0.TCC (1) */
    PGL_TEXPARAM_TFX      = 1,  /* TEX0.TFX (2), PGL_TFX_* */
    PGL_TEXPARAM_MMIN     = 2,  /* TEX1.MMIN (3), PGL_FILTER_* (2..5: mipmap filters) */
    PGL_TEXPARAM_MMAG     = 3,  /* TEX1.MMAG (1), PGL_FILTER_* */
    PGL_TEXPARAM_WMS      = 4,  /* CLAMP.WMS (2), PGL_WRAP_* */
    PGL_TEXPARAM_WMT      = 5,  /* CLAMP.WMT (2), PGL_WRAP_* */
    PGL_TEXPARAM_LCM      = 6,  /* TEX1.LCM (1) */
    PGL_TEXPARAM_MXL      = 7,  /* TEX1.MXL (3) */
    PGL_TEXPARAM_K        = 8,  /* TEX1.K = value >> 4 (12) */
    PGL_TEXPARAM_L        = 9,  /* TEX1.L (2) */
    PGL_TEXPARAM_LOD_BIAS = 10, /* GsContext.lodBias (no register) */
    PGL_TEXPARAM_TA0      = 11, /* TEXA.TA0 (8) */
    PGL_TEXPARAM_TA1      = 12, /* TEXA.TA1 (8) */
    PGL_TEXPARAM_AEM      = 13  /* TEXA.AEM (1) */
};
enum { PGL_TCC_RGB = 0, PGL_TCC_RGBA = 1 };     /* RGBA: the texture alpha is used */
enum { PGL_TFX_MODULATE = 0, PGL_TFX_DECAL = 1, PGL_TFX_HIGHLIGHT = 2, PGL_TFX_HIGHLIGHT2 = 3 };
enum { PGL_FILTER_NEAREST = 0, PGL_FILTER_LINEAR = 1 };
enum { PGL_WRAP_REPEAT = 0, PGL_WRAP_CLAMP = 1, PGL_WRAP_REGION_CLAMP = 2, PGL_WRAP_REGION_REPEAT = 3 };
/* gfx_set_texture_region axes (CLAMP MINU / MAXU, MINV / MAXV). */
enum { PGL_REGION_U = 0, PGL_REGION_V = 1 };
/* gfx_set_clut_mode: TEX2.PSM and CSA. */
enum {
    PGL_CLUT_FROM_TEX0 = 1,     /* PSM and CSA of TEX0 */
    PGL_CLUT_T4        = 2,     /* PSMT4, CSA of TEX0 */
    PGL_CLUT_T4_CSA0   = 3,     /* PSMT4, CSA 0 */
    PGL_CLUT_T8        = 4,     /* PSMT8, CSA of TEX0 */
    PGL_CLUT_T8_CSA0   = 5      /* PSMT8, CSA 0 */
};
void gfx_init_lights_material_spr(void);                    /* 0x00250A80 */
void gfx_reset_texture_state_spr(void);                     /* 0x00250AB8 */
u8   gfx_packet_is_open(void);                              /* 0x00250AE0 */
void gfx_change_display_mode(int inter, int omode, int ffmd); /* 0x00250AF0 */
void gfx_store_display_mode(int inter, int omode, int ffmd); /* 0x00250B58 */
void gfx_set_display_enable(u8 on);                         /* 0x00250BC0 */
void gfx_set_display_area(int dbx, int dby, int w, int h);  /* 0x00250CF8 */
void gfx_set_screen_size(int w, int h);                     /* 0x00250D18 */
void gfx_set_display_fb(u32 fbWordAddr, int psm, int fbw, int fbh); /* 0x00250D30 */
void gfx_dma_init(void);                                    /* 0x00250E68 */
void gfx_dma_lock(int ch);                                  /* 0x00250F60 */
void gfx_dma_unlock(int ch);                                /* 0x00250F80 */
int  gfx_dma_sync(int ch);                                  /* 0x00250FA0 0 ok, -1 timeout */
void gfx_dma_send(int ch, const void *madr, u32 qwc);       /* 0x00251150 */
void gfx_kick_frame_lists(struct GsFrameHeader *hdr);       /* 0x00251210 */
void vu_upload_microcode(int ch, u32 vuAddr, int byteSize, const void *code); /* 0x002512D0 */
void gfx_enable(u32 what);                                  /* 0x002513E0 */
void gfx_disable(u32 what);                                 /* 0x00251670 */
void gfx_cull_face(u32 mode);                          /* 0x00251918 */
void gfx_front_face(u32 mode);                          /* 0x00251930 */
void gfx_set_packet_buffer(void *buf, u32 size);            /* 0x00251948 */
void gfx_set_packet_access(u32 mode);                       /* 0x002519E0 */
struct GsFrameHeader *gfx_begin_packet(void);               /* 0x00251B10 */
void gfx_end_packet(void);                                  /* 0x00251C08 */
struct GsSubPacketHeader *gfx_begin_subpacket(void);        /* 0x00251CB8 */
void gfx_close_subpacket(void);                             /* 0x00251DB8 */
void gfx_call_subpacket(const struct GsSubPacketHeader *sp); /* 0x00251EB8 */
int  gfx_select_list(int which);                            /* 0x00251FA8 */
u32 *gfx_put_irq_marker(u32 code);                          /* 0x00252078 */
void gfx_sync_pipeline(void);                               /* 0x00252140 */
int  gfx_vsync(int n);                                      /* 0x00252338 */
void gfx_dma_sync_frame(void);                              /* 0x00252460 */
void gfx_set_target_b(u32 fbWordAddr, int psm, int w, int h); /* 0x00252480 */
void gfx_set_draw_buffer(u32 fbWordAddr, int psm, int w, int h); /* 0x002524E8 */
void gs_update_depth_range(GsContext *ctx);                 /* 0x002525A0 */
void gfx_set_depth_buffer(u32 zbWordAddr, int psm);         /* 0x00252600 */
void gfx_depth_func(int func);                              /* 0x002526D0 */
void gfx_alpha_func(int atst, float ref);                   /* 0x00252730 */
void gfx_alpha_fail(int afail);                             /* 0x002527A8 */
void gfx_blend_func(int abcd, float fix);                   /* 0x002527E8 abcd: only its low byte is stored; int, not u8:
                                                               u045's drawReflections passes CourseEnv::reflBlend with lw */
void gfx_dest_alpha_func(int mode, float ref);              /* 0x00252838 ref: not read (every caller passes 0.5) */
void gfx_alpha_func_raw(int atst, u8 ref);                  /* 0x00252890 */
void gfx_blend_func_raw(u8 abcd, u8 fix);                   /* 0x002528E0 */
void gfx_color_mask(u8 r, u8 g, u8 b, u8 a);                /* 0x00252908 */
void gfx_set_frame_write_mask(u32 mask);                    /* 0x00252950 */
void gfx_depth_mask(u8 write);                              /* 0x00252978 */
void gfx_set_field_mode(u32 mode);                          /* 0x002529E0 */
void gfx_offset_xy(float dx, float dy);                     /* 0x00252B40 */
void gfx_set_draw_area(int x, int y, int w, int h);         /* 0x00252BB8 */
void gs_update_scissor_and_viewport(GsContext *ctx);        /* 0x00252BE8 */
void gfx_set_scissor(int x, int y, int w, int h);           /* 0x00252D30 */
void gfx_set_depth_offset(float ofs);                       /* 0x00252DF8 */
void gs_set_depth_bias_at(float dz, float x, float y, float z); /* 0x00252E08 */
void gs_set_clear_color_f(float r, float g, float b, float a); /* 0x00252F58 */
void gs_set_clear_color(u32 rgba);                          /* 0x00252F90 */
void gs_set_clear_depth(float d);                           /* 0x00252FA0 */
void gs_clear_buffers(int mask);                            /* 0x00253138 */
void gfx_frame_effect(u32 kind, float amount);              /* 0x00253828 */
u32  gfx_set_fog_color_f(float r, float g, float b);        /* 0x00253DD0 returns the previous FOGCOL (gs_get_fog_color) */
u32  gfx_set_fog_color(u32 rgb);                            /* 0x00253E90 returns the previous FOGCOL */
void gfx_set_fog_params(float start, float amount);         /* 0x00253EF0 */
void gfx_set_texture_buffer(int level, u32 addr, int psm, int width); /* 0x00253F38 */
void gfx_set_texture_size(int w, int h);                    /* 0x00254228 */
void gfx_set_texture_function(int tfx);                     /* 0x00254308 */
void gfx_tex_param(int pname, int value);                   /* 0x00254358 */
void gfx_set_texture_region(int axis, u32 min, u32 max);    /* 0x002545D8 */
void gfx_texflush(void);                                    /* 0x00254690 */
void gfx_set_clut_mode(int mode);                           /* 0x002546B0 */

/* pgl.c */
/* pgl_matrix_mode modes (GsContext.matrixMode); the matrix of a mode that
 * is not current is parked at VU0 qword 4 + 4 * mode. */
enum { PGL_MTX_MODELVIEW = 0, PGL_MTX_PROJECTION = 1, PGL_MTX_TEXTURE = 2 };
/* pgl_push_attrib groups (glPushAttrib-like): pushed in this order, popped
 * in reverse and re-applied. */
enum {
    PGL_ATTR_DRAW_BUFFER = 0x001,   /* XYOFFSET, SCANMSK, fieldShift, halfHeight, tgtA (pop: gfx_set_draw_buffer) */
    PGL_ATTR_DEPTH       = 0x002,   /* ZBUF, depthScale, depthTest, zMask (pop: gs_update_depth_range, PGL_CAP_DEPTH_TEST) */
    PGL_ATTR_TARGET_B    = 0x004,   /* tgtB, clearColorB (only restored) */
    PGL_ATTR_FOG         = 0x040,   /* fogStart, fogAmount, FOGCOL (pop: gfx_set_fog_params) */
    PGL_ATTR_VIEWPORT    = 0x080,   /* winX..winH, depthNear, depthFar (pop: gs_update_scissor_and_viewport) */
    PGL_ATTR_SCISSOR     = 0x800    /* SCISSOR */
};
/* pgl_lightfv pnames (gs_set_light_param_v: the vectors, _f: the scalars);
 * the light is PGL_CAP_LIGHT0 + i.  One GL-like numbering with the
 * gs_set_material_param kinds (2 ambient and diffuse, 3 emission, 11
 * shininess). */
enum {
    PGL_LIGHT_AMBIENT        = 0,
    PGL_LIGHT_DIFFUSE        = 1,
    PGL_LIGHT_SPECULAR       = 4,
    PGL_LIGHT_SPOT_DIRECTION = 5,   /* normalised, times the modelview */
    PGL_LIGHT_SPOT_EXPONENT  = 6,   /* clamped to [0, 128] */
    PGL_LIGHT_SPOT_CUTOFF    = 7,   /* degrees; 180: not a spot light */
    PGL_LIGHT_CONST_ATTEN    = 8,
    PGL_LIGHT_LINEAR_ATTEN   = 9,
    PGL_LIGHT_QUAD_ATTEN     = 10,
    PGL_LIGHT_POSITION       = 13   /* w == 0: directional; times the modelview */
};
/* pgl_light_modelfv pnames (2 is accepted and ignored). */
enum {
    PGL_LIGHTMODEL_AMBIENT      = 0,    /* GsContext.globalAmbient */
    PGL_LIGHTMODEL_LOCAL_VIEWER = 1,    /* params[0] != 0: lightFlags 0x40 */
    PGL_LIGHTMODEL_EYE          = 3     /* GsContext.eyeWorld, the viewer position */
};
/* gs_set_scanmask_mode modes (SCANMSK.MSK); gs_get_scanmask_state returns
 * the mode of the current mask (EVEN, ODD or OFF). */
enum {
    PGL_SCANMASK_OFF         = 0,   /* every row drawn */
    PGL_SCANMASK_FIELD       = 1,   /* the rows of GsContext.field's parity not drawn */
    PGL_SCANMASK_OTHER_FIELD = 2,   /* the rows of the other parity not drawn */
    PGL_SCANMASK_EVEN        = 3,   /* even rows not drawn */
    PGL_SCANMASK_ODD         = 4    /* odd rows not drawn */
};
void gs_fix_tex0_tcc(GsContext *ctx);                       /* 0x00254828 */
void gs_set_clut(u32 cbpWordAddr, int cpsm);                /* 0x002548C0 */
void gs_set_clut_csa(int csa);                              /* 0x002549E0 */
int  gs_build_image_transfer_packet(u32 *pkt, const u32 *dstBitblt,
                                    int dx, int dy, int w, int h); /* 0x00254A30 returns the image qwc */
void gs_upload_image(u32 fbWordAddr, int psm, int fbw, int dx, int dy,
                     int w, int h, const void *src);        /* 0x00254B28 */
void gs_upload_image_sync(u32 fbWordAddr, int psm, int fbw, int dx, int dy,
                          int w, int h, const void *src);   /* 0x00254C00 */
void gs_download_image(u32 fbWordAddr, int psm, int fbw, int sx, int sy,
                       int w, int h, void *dst);            /* 0x00254CB0 */
void pgl_matrix_mode(int mode);                             /* 0x00254E20 PGL_MTX_* */
void pgl_push_matrix(void);                                 /* 0x00254F68 */
void pgl_pop_matrix(void);                                  /* 0x00254FE8 */
void pgl_push_attrib(u32 mask);                             /* 0x00255070 PGL_ATTR_* */
void pgl_pop_attrib(void);                                  /* 0x002552A0 */
void pgl_frustum(float l, float r, float b, float t, float n, float f,
                 float focal);                              /* 0x002555A8 */
void pgl_ortho(float l, float r, float b, float t, float n, float f); /* 0x00255680 */
void *gfx_alloc_tail(int qwc);                              /* 0x00255760 */
u32 *gfx_open_direct(int qwc);                              /* 0x00255780 */
u32  gs_prim_attr_with(u32 bits);                           /* 0x002557D0 */
void pgl_lightfv(int light, u32 pname, const float *params); /* 0x002557E8 PGL_LIGHT_* */
void pgl_light_modelfv(int pname, const float *params);     /* 0x00255898 PGL_LIGHTMODEL_* */
u32  float_to_gs_param_encode(float v);                     /* 0x00255940 */
float pgl_cos_deg(float deg);                               /* 0x00255A70 */
void pgl_sincos_deg(float deg, float *s, float *c);         /* 0x00255A98 */
float pseudo_angle(float a, float b);                       /* 0x00255AE0 octant-ratio pseudo angle */
void gs_set_scanmask_mode(u32 mode);                        /* 0x00255B70 PGL_SCANMASK_* */
void pgl_set_const_vector(float x, float y, float z, float w);        /* 0x00255C30 GsContext.constVector (lighting mode 5's vector) */
void pgl_set_const_color(float r, float g, float b, float a);        /* 0x00255C50 GsContext.constColor (lighting mode 5's colour) */
void gs_draw_flat_sprite(int x0, int y0, int x1, int y1);   /* 0x00255C90 */
void gs_draw_flat_sprite_strips(int x0, int y0, int x1, int y1); /* 0x00255DF0 */
void gs_draw_textured_sprite(int x0, int y0, int x1, int y1,
                             int u0, int v0, int u1, int v1); /* 0x00256020 */
void gs_draw_textured_sprite_f(float x0, float y0, float x1, float y1,
                               float u0, float v0, float u1, float v1); /* 0x00256188 */
void gs_draw_textured_sprite_strips_f(float x0, float y0, float x1, float y1,
                                      float u0, float v0, float u1, float v1); /* 0x00256360 */
void gs_draw_line(int x0, int y0, int x1, int y1);          /* 0x002566F0 */
void gs_draw_line_f(float x0, float y0, float x1, float y1); /* 0x00256838 */
void pgl_draw_line_strip_2d(const float *xy, int n);        /* 0x002569C0 */
void pgl_set_sprite_depth(float z);                         /* 0x00256B80 */
void gs_set_sprite_zf(int z, u32 fog);                      /* 0x00256C78 */
void pgl_draw_line_3d(const float *p0, const float *p1);    /* 0x00256E60 */
void pgl_draw_line_3f(float x0, float y0, float z0,
                      float x1, float y1, float z1);        /* 0x00256FF8 */
void gs_emit_stmask(GsContext *ctx, u32 mask);              /* 0x00257030 */
void pgl_shade_vertices(GsContext *ctx, u32 *out, const float *pos,
                        const float *nrm, const u32 *colors, int n); /* 0x002570C8 CPU lighting loop */
u32 *__pglShading(GsContext *ctx, const float *pos, const float *nrm,
                  const u32 *colors, int n);                /* 0x00257910 */

/* vu0_matrix_asm.c (hand-written; pgl_mult_matrix is glMultMatrix,
 * current = m x current in the row-vector layout, VU0 micro subroutine
 * 0x000; pgl_mult_matrix_90 is the other order, current = current x m:
 * m applied after the current matrix, subroutine 0x090) */
void vu0_sincos_deg(float deg);                             /* 0x00257A44 vf1.x = sin, vf1.y = cos (subroutine 0x1D0) */
void vu0_load_identity_regs(void);                          /* 0x00257B30 */
void vu0_reset_rotation(void);                              /* 0x00257B5C */
void vu0_reset_translation(void);                           /* 0x00257B84 */
void pgl_load_matrix(const float *m);                       /* 0x00257B9C */
void pgl_get_matrix(float *m);                              /* 0x00257C34 */
void pgl_rotate(float deg, float x, float y, float z);      /* 0x00257CBC */
void pgl_mult_matrix(const float *m);                       /* 0x00257D94 */
void pgl_mult_matrix_90(const float *m);                    /* 0x00257E30 */
void pgl_translate(float x, float y, float z);              /* 0x00257EB4 */
void F_00257EF0(float s);                                   /* 0x00257EF0 runs into vu0_scale_inv_y */

/* vu0_current_matrix.c */
void vu0_scale_inv_y(f32 s);                                /* 0x00257EF4 */
void vu0_scale(f32 x, f32 y, f32 z);                        /* 0x00257F04 */
void pgl_set_scale(f32 x, f32 y, f32 z);                    /* 0x00257F40 (F_00257F40) GsContext.scale */
void vu0_rotate_x(f32 deg);                                 /* 0x00257F60 */
void vu0_rotate_y(f32 deg);                                 /* 0x00257FA0 */
void vu0_rotate_z(f32 deg);                                 /* 0x00257FE0 */
void vu0_transpose(void);                                   /* 0x00258020 */
void vu0_invert_rigid(void);                                /* 0x00258078 */
void vu0_apply_matrix(f32 *dst, const f32 *src);            /* 0x002580EC */
void pgl_set_normal_matrix(const f32 *m);                   /* 0x00258138 (F_00258138) GsContext.normalMtx */

/* gs_context.c */
void pgl_set_morph_weight(f32 weight);                                /* 0x00258150 GsContext.morphWeight = weight, the morph weight */
int  gs_get_scanmask_state(void);                           /* 0x00258160 PGL_SCANMASK_EVEN / _ODD / _OFF of the SCANMSK shadow */
u32  gs_get_fog_color(void);                                /* 0x002581C8 FOGCOL r | g << 8 | b << 16 */
void pgl_get_fog_constants(f32 *out);                                  /* 0x002581F0 viewCenter.w, viewScale.w, limits.w (the fog constants of gfx_set_fog_params) */
int  gs_get_field_mode(void);                            /* 0x00258218 the PGL_FIELD_* mode of the current field (FRAME_SHIFT, HALF_ODD, HALF_EVEN; FRAME unless ffmd == 1) */
void gfx_get_draw_buffer(u32 *fbWordAddr, u32 *psm, s32 *w, s32 *h); /* 0x00258260 */
u32  gfx_get_cull_face(void);                              /* 0x00258290 GsContext.cullFace: the PGL_CULL_* mode (glCullFace) */
void gfx_get_draw_area(s32 *x, s32 *y, s32 *w, s32 *h);     /* 0x002582A0 */

/* color_filter.c */
/* workAddr: free VRAM (word address: one work page, the CLUT at +0x800, the
 * index texture at +0x900); r, g, b the tone-curve control points, [lo, hi]
 * the input range, saturation 1.0 = unchanged, 0.0 = grey.  Source: the
 * TEX0 texture, destination: the FRAME buffer. */
void pfx_color_correction(f32 r, f32 g, f32 b, f32 lo, f32 hi, f32 saturation,
                          u32 workAddr);                    /* 0x002582F8 workAddr last: the caller
                                                               (u045 drawView) sets $a0 after $f12..$f17 */
void pfx_build_identity_clut32(u32 clutAddr);               /* 0x00259240 256-entry CT32 CLUT, entry k = k in every byte */
/* Splits the TEX0 texture (the frame) into its red, green and blue bytes,
 * written as the alpha bytes of the planes at fbAddrR / G / B (w x h;
 * workAddr: 3 pages of work VRAM, the identity CLUT at +0x1800, the index
 * texture at +0x1900).  RacePS2Base::captureAfterimage. */
void pfx_split_rgb_planes(u32 fbAddrR, u32 fbAddrG, u32 fbAddrB,
                s32 w, s32 h, u32 workAddr);                /* 0x00259368 */

/* gs_driver.c */
void pgl_color(u32 rgba);                             /* 0x0025A038 */
u32  pack_rgba_floats_to_u32(float r, float g, float b, float a, float scale); /* 0x0025A090 */
void pgl_color4f(float r, float g, float b, float a); /* 0x0025A128 */
void pgl_color3f(float r, float g, float b);        /* 0x0025A150 */
void pgl_color4fv(const f32 *rgba);                    /* 0x0025A180 */
void xor_unscramble_with_lcg(u8 *buf, int len);             /* 0x0025A1F8 */
int  gs_collect_dirty_regs(GsContext *ctx, u64 *out);       /* 0x0025A290 */
int  intc_vblank_handler(int cause);                        /* 0x0025A360 INTC_VBLANK_S (installed by gfx_init) */
int  intc_vif1_handler(int cause);                          /* 0x0025A4A0 INTC_VIF1 */
int  intc_gs_finish_handler(int cause);                     /* 0x0025A5B8 INTC_GS */
int  dmac_channel_handler(int ch);                          /* 0x0025A618 DMAC channels 0..2 (installed by gfx_dma_init) */
void dma_start_source_chain(int ch, const void *tadr);      /* 0x0025A950 */
void gs_select_vu1_prog(GsContext *ctx);                    /* 0x0025AA00 (bootstrap: gs_compute_display_period) sets vu1Prog from vu1ProgAlt, the culling switch, cullFace, frontFace */
int  gs_build_image_readback_packet(u64 *pkt, const u32 *srcBitblt,
                                    int sx, int sy, int w, int h); /* 0x0025AA88 */

/* gs_lights.c */
/* gs_set_material_param kinds (the PGL_LIGHT_* numbering; also
 * GsContext.colorMaterialKind) and faceMask bits (GsContext.material[0] / [1];
 * GsContext.colorMaterialFaces).  Kind 11 takes the shininess as an integer, the
 * others an RGBA colour (bytes / 255). */
enum {
    PGL_MAT_AMBIENT             = 0,
    PGL_MAT_DIFFUSE             = 1,
    PGL_MAT_AMBIENT_AND_DIFFUSE = 2,
    PGL_MAT_EMISSION            = 3,
    PGL_MAT_SPECULAR            = 4,
    PGL_MAT_SHININESS           = 11    /* clamped to 0..128 */
};
enum { PGL_FACE_FRONT = 1, PGL_FACE_BACK = 2 };
void gs_set_material_param(int faceMask, int kind, u32 value); /* 0x0025AC70 PGL_FACE_* mask, PGL_MAT_* (vectors: face 0 only, see u118) */
void gs_set_light_param_f(GsContext *ctx, GsLight *l, int pname, f32 v); /* 0x0025ADB0 */
void gs_set_light_param_v(GsContext *ctx, GsLight *l, u32 pname, GsVec4Arg v); /* 0x0025AE60 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_G001_FLARE_H */
