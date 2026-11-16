/* T462_tire_packet.h - TirePacket: the geometry of a tyre built once into
 * GS strip packets (translation unit T462, unit u100).
 *
 * virtual_1 builds, with a TStripBuilder (T448) on the stack and a table of
 * nseg + 1 points of the unit circle (TireCircle), three packets: the tread
 * side (virtual_3: tread, shoulder bevels, sidewalls; shader 2, PRIM 0x3C),
 * the faces (virtual_4: shader 0, PRIM 0x2C) and, if `shadow', the shadow
 * strip under the tyre (virtual_5: PRIM 0x6C).  The packets are G001
 * VifModels (model_draw); the shader of a block is its part texture index,
 * given to the part-texture callback (pgl_set_part_tex_func:
 * tire_packet_pass_callback -> virtual_2), which sets up the tread's texture
 * wrap, blend and material.  Derived: RaceTirePacket (T449); embedded in
 * MenuTireModel (T450). */
#ifndef GAME_T462_TIRE_PACKET_H
#define GAME_T462_TIRE_PACKET_H

#include "game/game.h"

class TireModelBase;                        /* T447 (pointers only) */
class TStripBuilder;                        /* T448 */
struct VifModel;                            /* G001 (the packets TStripBuilder::finish writes) */

/* A point of the unit circle of the tread: (sin, cos) of one of nseg + 1
 * angles (makeCircle; a stack table of virtual_1). */
struct TireCircle {
    f32 x;                                  /* 0x0 sin */
    f32 y;                                  /* 0x4 cos */
};
EGCS_SIZE(TireCircle, 0x8);

/* vtable _vt$10TirePacket 0x00340CF8 (6 slots), typeinfo __tf10TirePacket
 * 0x00221298 (__user_type_info 0x00353448).  0x2C bytes of data, vptr
 * +0x2C.  Size 0x30 (RaceTirePacket's members start at +0x30). */
class TirePacket {
public:
    TirePacket();                           /* 0x0021FB30 nseg 30, bevel 1, a random texOffset */
    virtual ~TirePacket();                  /* [0] +0x0C 0x0021FBA0 */
    virtual void *virtual_1(void *buf);     /* [1] +0x14 0x0021FD70 builds the packets into buf; returns the end */
    virtual int   virtual_2(int pass);      /* [2] +0x1C 0x0021FCC8 the per-pass setup; 1 if the pass is known */
    virtual void  virtual_3(TStripBuilder *sb, TireCircle *c);  /* [3] +0x24 0x0021FEB8 the tread side */
    virtual void  virtual_4(TStripBuilder *sb, TireCircle *c);  /* [4] +0x2C 0x00220020 the faces, the inside */
    virtual void  virtual_5(TStripBuilder *sb, TireCircle *c);  /* [5] +0x34 0x00220150 the shadow */

    void setup(TireModelBase *model, int nseg, int bevel, int inside, int shadow,
               f32 x0);                     /* 0x0021FBD0 */
    void draw();                            /* 0x0021FBF0 the tread texture, packetOut, packetIn, the wheel */
    void drawShadow();                      /* 0x0021FC58 packetShadow if built */
    void bindPassCallback();                /* 0x0021FD48 this gets the passes (tire_packet_pass_callback) */
    f32  bevelRatio();                      /* 0x002201B8 0.25, F1 tyres 0.3 */
    void buildTread(TStripBuilder *sb, TireCircle *c, int uv, int color, int swap);                /* 0x002201E8 */
    void buildSidewall(TStripBuilder *sb, TireCircle *c, int uv, int color, int rev, int mirror);  /* 0x00220600 */
    void buildShoulder(TStripBuilder *sb, TireCircle *c, int uv, int color, int rev, int mirror);  /* 0x00220A00 */
    void buildFace(TStripBuilder *sb, TireCircle *c, int rev);  /* 0x00220DF0 */
    void buildShadow(TStripBuilder *sb, TireCircle *c);         /* 0x00220F68 */
    void makeCircle(TireCircle *c);         /* 0x002211A0 nseg + 1 points */
    int  isBuilt();                         /* 0x00221290 */

    f32            x0;                      /* 0x00 the inner x of the tyre */
    f32            texOffset;               /* 0x04 a random v offset of the tread texture */
    int            nseg;                    /* 0x08 segments around the tyre (30) */
    int            bevel;                   /* 0x0C 1: shoulder bevels */
    int            shadow;                  /* 0x10 builds packetShadow */
    int            inside;                  /* 0x14 builds the inner faces */
    TireModelBase *model;                   /* 0x18 */
    VifModel      *packetOut;               /* 0x1C the tread side (virtual_3) */
    VifModel      *packetIn;                /* 0x20 the faces (virtual_4) */
    VifModel      *packetShadow;            /* 0x24 the shadow (virtual_5) */
    int            built;                   /* 0x28 */
    /*             vptr                        0x2C */
};
EGCS_SIZE(TirePacket, 0x30);

/* The part-texture callback (G001 calls it as a PglPartTexFunc, void (*)(s32):
 * the result is dropped). */
int  tire_packet_pass_callback(int pass);   /* 0x0021FB00 the bound packet's virtual_2 (gp-0x4F28) */
void F_00221260(void);                      /* 0x00221260 draws the prebuilt packet 0x00340CE0 (a hexagonal prism,
                                               width 1, radius 1: a coarse unit tyre), gfx state 0xF off */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function and its storage itself (and rejects
 * hand declarations). */
extern "C" {
void *__tf10TirePacket(void);               /* 0x00221298 */
extern u32 __ti10TirePacket[2];             /* 0x00353448 gp-0x45A8 __user_type_info */
}
#endif

#endif /* GAME_T462_TIRE_PACKET_H */
