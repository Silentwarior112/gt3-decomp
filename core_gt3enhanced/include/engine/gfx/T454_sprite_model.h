/* T454_sprite_model.h - SpriteModel (? name): a model of camera-facing quads
 * (translation unit T454, unit u102).
 *
 * Each part is a list of entries: a sprite (a quad of the sprite's half
 * width and height standing at x, y, z, turned about the vertical to face
 * the camera) or, with bit 15 of the index, a prebuilt packet.  One TEX1
 * texture with CLUT animations.  The file stores offsets relative to `base'.
 * spriteset_begin_draw computes the camera's right axis in model space (xz,
 * normalised; T454 .bss 0x0049D1A0) that the quads are built along.
 */
#ifndef GAME_T454_SPRITE_MODEL_H
#define GAME_T454_SPRITE_MODEL_H

#include "game/game.h"

struct Tex1Image;                               /* G001_flare.h */
struct GsMaterial;                              /* G001_flare.h (T451's ModelMaterial) */
struct VifModel;                                /* G001_flare.h: a prebuilt VU1 packet */

/* A sprite, 0x10 bytes. */
typedef struct Sprite {
    f32  w;                                     /* 0x00 half width */
    f32  h;                                     /* 0x04 height */
    f32 *uvs;                                   /* 0x08 relocated if not NULL */
    u32 *colors;                                /* 0x0C relocated if not NULL */
} Sprite;
EGCS_SIZE(Sprite, 0x10);

/* An entry of a part, 0x10 bytes. */
typedef struct SpriteEntry {
    u16 idx;                                    /* 0x00 the sprite; bit 15: a packet number instead */
    /* tex: bits 0..8 the texture, 9..15 the material (1-based; 0: keep the current) */
    u16 tex;                                    /* 0x02 the texmat_select id */
    f32 x;                                      /* 0x04 the foot of the quad */
    f32 y;                                      /* 0x08 */
    f32 z;                                      /* 0x0C */
} SpriteEntry;
EGCS_SIZE(SpriteEntry, 0x10);

typedef struct SpritePart {
    u16         count;                          /* 0x00 */
    u16         unk_02;                         /* 0x02 */
    SpriteEntry e[1];                           /* 0x04 [count] */
} SpritePart;

typedef struct SpriteModel {
    u32                   magic;                /* 0x00 */
    struct SpriteModel   *base;                 /* 0x04 the address the offsets are for */
    void                 *unk_08;               /* 0x08 relocated if not NULL */
    u8                    unk_0C[4];            /* 0x0C */
    u16                   nParts;               /* 0x10 */
    u16                   nMaterials;           /* 0x12 */
    u16                   nSprites;             /* 0x14 */
    u16                   nPackets;             /* 0x16 */
    SpritePart          **parts;                /* 0x18 [nParts] */
    struct Tex1Image     *tex;                  /* 0x1C its GsTexRegs: the texture table (texsel_set_table) */
    struct GsMaterial *materials;               /* 0x20 [nMaterials] the material table (matsel_set_table) */
    Sprite               *sprites;              /* 0x24 [nSprites] */
    struct VifModel     **packets;              /* 0x28 [nPackets] prebuilt packets (vif_model_relocate, model_draw) */
} SpriteModel;

void spriteset_relocate(SpriteModel *m);                        /* 0x00226AA0 NULL: nothing */
/* The CLUT animations of the texture (id < 0: all). */
void spriteset_reset_clut_anims(SpriteModel *m);                /* 0x00226C80 */
void spriteset_step_clut_anims(SpriteModel *m, f32 dt, int id); /* 0x00226CB0 dt first, as modelset_step_anims */
void spriteset_apply_clut_anims(SpriteModel *m);                /* 0x00226CE0 */
void spriteset_release_clut_anims(SpriteModel *m, int id);      /* 0x00226D10 */
void spriteset_begin_draw(SpriteModel *m);                      /* 0x00226D40 */
void spriteset_end_draw(SpriteModel *m);                        /* 0x00226EB0 */
void spriteset_draw_part(SpriteModel *m, int part);             /* 0x00226EE8 */
void spriteset_draw_all(SpriteModel *m);                        /* 0x00227100 */

#endif /* GAME_T454_SPRITE_MODEL_H */
