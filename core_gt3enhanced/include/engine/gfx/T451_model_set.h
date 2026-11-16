/* T451_model_set.h - ModelSet, the "GTM1" model (translation unit T451, unit u102).
 *
 * A ModelSet holds parts (byte-code command streams), prebuilt VIF packets,
 * materials, textures (with optional texture sets: variations of the whole
 * table), one bounding sphere per part and the animation tracks (T452, an
 * AnimSet).  The file stores offsets relative to `base', the address it was
 * built at; modelset_relocate turns them into pointers.  The interpreter
 * (modelset_run_part / modelset_exec) runs a part's stream: draw a packet or
 * another part, select a texture, frustum tests, matrix and render-state
 * commands, and the switch (0x06) and LOD (0x07) branches, which the global
 * ModelSet::Callback decides (modelset_set_callback; none: switch 0,
 * modelset_default_lod).
 *
 * ModelSet::Callback: vtable 0x003412B0 (3 slots), typeinfo
 * __tfQ28ModelSet8Callback 0x00225D50 (__user_type_info 0x00353458).  No
 * data: vptr +0x00, 4 bytes.  Derived:
 * CarModel::Callback (T446), RaceTirePacket::Callback (T449).
 */
#ifndef GAME_T451_MODEL_SET_H
#define GAME_T451_MODEL_SET_H

#include "game/game.h"

struct Tex1Image;                               /* G001_flare.h */
struct GsMaterial;                              /* G001_flare.h */
struct VifModel;                                /* G001_flare.h: a prebuilt VU1 packet */
struct GsTexRegs;                               /* G001_flare.h */
struct AnimSet;                                 /* T452 (u102): the animation tracks */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function and its storage itself (and rejects
 * hand declarations). */
extern "C" {
void *__tfQ28ModelSet8Callback(void);       /* 0x00225D50 */
extern u32 __tiQ28ModelSet8Callback[2];     /* 0x00353458 gp-0x4598 __user_type_info */
}
#endif

/* A material record of the file, 0x50 bytes: G001's GsMaterial as
 * material_load reads it from a table (matsel_set_table, texmat_select):
 * the colours and shininess +0x00..+0x43, the flags byte x44.flags, an
 * offset in the file x48.offset (relocated). */
typedef struct GsMaterial ModelMaterial;

/* A part's bounding sphere, 16 bytes (also inline, unaligned, in the
 * command streams). */
struct ModelSphere {
    f32 x;                                      /* 0x00 centre */
    f32 y;                                      /* 0x04 */
    f32 z;                                      /* 0x08 */
    f32 r;                                      /* 0x0C radius */
};
EGCS_SIZE(ModelSphere, 0x10);

/* The "GTM1" model file. */
class ModelSet {
public:
    class Callback {
    public:
        virtual ~Callback();                    /* [0] +0x0C 0x00225CF8 */
        /* Command 0x06: the branch of switch `id' (0 here). */
        virtual int selectSwitch(int id);       /* [1] +0x14 0x00225D28 */
        /* Command 0x07: the LOD for a projected sphere size (here
         * modelset_default_lod). */
        virtual int selectLOD(f32 size);        /* [2] +0x1C 0x00225D30 */
    };

    u32             magic;                      /* 0x00 'GTM1' */
    ModelSet       *base;                       /* 0x04 the address the offsets are for */
    u8              unk_08[8];                  /* 0x08 */
    u16             nParts;                     /* 0x10 */
    u16             nPackets;                   /* 0x12 */
    u16             nMaterials;                 /* 0x14 */
    u16             nTex;                       /* 0x16 */
    u16             nTexSets;                   /* 0x18 texture LOD levels (RacePS2Base forces 2 for the mirror / sub views) */
    u16             nMaterialSets;              /* 0x1A */
    u8              unk_1C[4];                  /* 0x1C */
    u8            **parts;                      /* 0x20 [nParts] command streams */
    VifModel      **packets;                    /* 0x24 [nPackets] prebuilt packets (vif_model_relocate, model_draw) */
    ModelMaterial  *materials;                  /* 0x28 [nMaterials] the current set */
    Tex1Image     **tex;                        /* 0x2C [nTex] */
    ModelSphere    *bounds;                     /* 0x30 [nParts] */
    Tex1Image    ***texSets;                    /* 0x34 [nTexSets][nTex] by LOD (modelset_tex_lod); a set may be tex itself */
    ModelMaterial **materialSets;               /* 0x38 [nMaterialSets] */
    AnimSet        *anim;                       /* 0x3C NULL: none */
};
EGCS_SIZE(ModelSet::Callback, 0x4);

/* Relocation (always in place; nothing when base is already the address). */
void  modelset_relocate(ModelSet *m);                       /* 0x00224600 */
/* The CLUT animations of every texture (texture sets other than the base
 * table too) and the animation tracks.  id < 0: all; the tracks only step
 * with id < 0.
 * modelset_reset_anims takes the id in the enhanced engine (Lupo 2 L2
 * 0x002321D8; rendering.md 1, backend.md 5): id < 0 resets (and applies)
 * every CLUT record and the tracks, as GT3; id >= 0 only the records of
 * that id, left for the next modelset_apply_clut_anims, and not the tracks
 * (CarModel::setAnimSelect, T446).  A caller that passes no id gets -1. */
void  modelset_reset_anims(ModelSet *m, int id = -1);       /* 0x002248D0 */
/* dt before id: u059's course_step_anims sets the float argument first
 * (EABI: the order changes no register). */
void  modelset_step_anims(ModelSet *m, f32 dt, int id);     /* 0x002249F8 */
/* The enhanced engine: modelset_step_anims without the tracks (the CLUT
 * records only; CarModel::animate with ENH_RENDER_CAR_ANIMS 0). */
void  modelset_step_clut_anims(ModelSet *m, f32 dt, int id);
void  modelset_apply_clut_anims(ModelSet *m);               /* 0x00224B48 */
void  modelset_release_clut_anims(ModelSet *m, int id);     /* 0x00224C60 */

/* Level of detail.  The depth (w, at least 0.0001) of a sphere centre
 * through vf24..vf27; the projected size of a sphere; the LOD of a size
 * (0 above the LOD size, 200 by default; 1 above half of it, 2 above a
 * quarter, else 3); the texture set of a part's sphere (the same with the
 * texture LOD size, 400). */
f32   modelset_sphere_depth(const void *centre);            /* 0x00224D88 */
f32   modelset_sphere_size(const u8 *sphere);               /* 0x00224DE8 */
int   modelset_default_lod(f32 size);                       /* 0x00224E30 */
int   modelset_tex_lod(const ModelSphere *s);               /* 0x00224E88 */

/* Drawing: the render states and m's material table, the parts, the
 * states back.  draw_part = begin, run_part, end. */
void  modelset_begin_draw(ModelSet *m);                     /* 0x00224F18 */
void  modelset_end_draw(ModelSet *m);                       /* 0x00224FE8 m unused */
void  modelset_draw_all(ModelSet *m);                       /* 0x00225028 */
void  modelset_draw_part(ModelSet *m, int part);            /* 0x00225090 */
void  modelset_run_part(ModelSet *m, int part);             /* 0x002250D8 */
/* The commands modelset_run_part does not handle itself: runs op with its
 * operands at p, returns the next command. */
u8   *modelset_exec(u8 op, u8 *p);                          /* 0x00225508 */

/* Settings */
void  modelset_select_tex_frame(ModelSet *m, int index);    /* 0x00225AF8 tex1_select_variation of the base table */
void  modelset_select_material_set(ModelSet *m, int i);     /* 0x00225B80 materials = materialSets[i] */
void  F_00225BB8(GsMaterial *t);                            /* 0x00225BB8 matsel_set_table2(t) */
/* The environment-map texture of the models (texsel_set_table2); NULL: none. */
void  modelset_set_env_tex(GsTexRegs *regs);                /* 0x00225BD8 */
void  modelset_set_callback(ModelSet::Callback *cb);        /* 0x00225BF8 NULL: none */
void  modelset_set_morph(f32 t);                            /* 0x00225C00 command 0x35's position, clamped to [0, 1] */
/* The frustum test (0x0A) also culls on clip flag 0x10. */
void  modelset_set_cull_far(int on);                        /* 0x00225C38 */
void  modelset_set_lod_size(f32 size);                      /* 0x00225C40 (200.0) */
void  modelset_set_state5(int on);                          /* 0x00225C48 render state 5 allowed (1) */
void  modelset_set_tex_lod(int on);                         /* 0x00225C50 texture sets by LOD */
void  modelset_set_tex_lod_size(f32 size);                  /* 0x00225C58 (400.0) */
void  modelset_set_tex_set(int set);                        /* 0x00225C60 the forced texture set, < 0: by LOD (-1) */
/* The projection scale (pixels per unit at w 1) from the current matrices,
 * times scale. */
void  modelset_set_projection_scale(f32 scale);             /* 0x00225C68 */

#endif /* GAME_T451_MODEL_SET_H */
