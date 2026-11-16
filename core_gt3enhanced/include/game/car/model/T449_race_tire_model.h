/* T449_race_tire_model.h - RaceTireModel, RaceTirePacket and its Callback,
 * the brake file "race/brake.bin" (translation unit T449, unit u101).
 *
 * A race car has four RaceTireModels (RaceCarModel, T180).  Each holds three
 * RaceTirePackets, the prebuilt GS packets of the tyre at three levels of
 * detail (18 / 12 / 8 segments), built once into the car's work space and
 * drawn by draw(); level 3 is a plain scaled cylinder.  On top of
 * TirePacket's tyre a RaceTirePacket adds the brake disc and caliper
 * (textures of the brake file), the glow of a heat brake, the wheel model
 * (through RaceTirePacket::Callback, which gives it the level) or a hexagon
 * at the lowest level, and the motion blur of the wheel and of the tread
 * (rendered into small GS buffers). */
#ifndef GAME_T449_RACE_TIRE_MODEL_H
#define GAME_T449_RACE_TIRE_MODEL_H

#include "game/game.h"
#include "game/car/model/T447_tire_model_base.h"      /* base TireModelBase */
#include "game/car/model/T462_tire_packet.h"          /* base TirePacket, TireCircle */
#include "engine/gfx/T451_model_set.h"            /* base ModelSet::Callback (of RaceTirePacket::Callback) */

struct Tex1Image;                           /* G001 (pointers only) */
class GSBuffer;                             /* T514 */
class TexSet;                               /* T515 */
class TStripBuilder;                        /* T448 */
struct VifModel;                            /* G001 (TStripBuilder::finish's packets) */
class RaceTireModel;

/* "race/brake.bin": a disc record (stride 0x10). */
struct BrakeDisc {
    u32 flags;                              /* 0x00 bit 0: a heat brake (glows) */
    u8  unk_04[0xC];                        /* 0x04 */
};
EGCS_SIZE(BrakeDisc, 0x10);

/* A caliper record (stride 0x20). */
struct BrakeCaliper {
    f32 halfU;                              /* 0x00 the half size of the quad */
    f32 halfV;                              /* 0x04 */
    f32 centerU;                            /* 0x08 its centre on the disc plane */
    f32 centerV;                            /* 0x0C */
    u8  unk_10[0x10];                       /* 0x10 */
};
EGCS_SIZE(BrakeCaliper, 0x20);

/* The brake file (one, gp-0x4F24): its pointers are addresses for the file
 * at `base' (where it was built); relocate() moves them by this - base. */
struct BrakeFile {
    void relocate();                        /* 0x00221E08 also relocates the texture */
    void loadTexture();                     /* 0x00221E78 makes tex resident (T513) */

    u8            unk_00[4];                /* 0x00 */
    BrakeFile    *base;                     /* 0x04 */
    u8            unk_08[8];                /* 0x08 */
    Tex1Image    *tex;                      /* 0x10 the textures */
    int           discTex;                  /* 0x14 the first disc texture */
    int           caliperTex;               /* 0x18 the first caliper texture */
    int           glowTex;                  /* 0x1C the heat glow texture */
    BrakeDisc    *discs;                    /* 0x20 */
    BrakeCaliper *calipers;                 /* 0x24 */
};

/* vtable _vt$14RaceTirePacket 0x00340F30 (6 slots), typeinfo
 * __tf14RaceTirePacket 0x00223FF0 (si, base TirePacket; __si_type_info
 * 0x004B29E0).  Size 0x4C (RaceTireModel::packets[3]). */
class RaceTirePacket : public TirePacket {
public:
    /* vtable 0x00340F68 (3 slots), typeinfo __tfQ214RaceTirePacket8Callback
     * 0x00224040 (si, base ModelSet::Callback; __si_type_info 0x004B29C0).
     * vptr +0x00.  Size 8.  The ModelSet callback while the wheel model is
     * drawn. */
    class Callback : public ModelSet::Callback {
    public:
        Callback();                         /* 0x00223DB0 lod 0 */
        virtual ~Callback();                /* [0] +0x0C 0x00223DC8 */
        virtual int selectSwitch(int id);   /* [1] +0x14 0x00223DF0 returns 0 */
        virtual int selectLOD(f32 size);    /* [2] +0x1C 0x00223DF8 returns lod (index.txt: lupo2m
                                               TStripBuilder::getCurrentAddress, the same code) */

        /*  vptr                               0x00 */
        int lod;                            /* 0x04 the wheel level */
    };

    RaceTirePacket();                       /* 0x00222690 */
    virtual ~RaceTirePacket();              /* [0] +0x0C 0x00222718 */
    virtual void *virtual_1(void *buf);     /* [1] +0x14 0x002237C0 TirePacket's, then the disc and caliper */
    virtual int   virtual_2(int pass);      /* [2] +0x1C 0x00223730 TirePacket's, 0xC the glow, 0xD the hexagon */
    virtual void  virtual_3(TStripBuilder *sb, TireCircle *c);  /* [3] +0x24 0x002238C0 TirePacket's, the hexagon */
    virtual void  virtual_4(TStripBuilder *sb, TireCircle *c);  /* [4] +0x2C 0x00223920 TirePacket's, the glow */
    /*            virtual_5                    [5] +0x34 0x00220150 TirePacket's */

    void setup(RaceTireModel *m, int nseg, int bevel, int shadow, int inside, int lod,
               f32 x0);                     /* 0x00222768 TirePacket::setup; withBrake = inside */
    void draw(int blurPasses, f32 spinRad, f32 glow, f32 blur); /* 0x002227E0 glow: the FIX of the heat
                                               glow's additive blend; blur: the spin (radians) to blur over */
    void setDiscVertices(f32 spinRad);      /* 0x00222B68 */
    void setCaliperVertices(f32 spinRad);   /* 0x00222CE0 */
    void drawTreadBlur(GSBuffer *buf, int blurPasses, f32 blur);  /* 0x00222F00 */
    void drawWheelTexture(GSBuffer *buf, TexSet *set, int blurPasses, f32 blur); /* 0x00223320 */
    void buildHexagon(TStripBuilder *sb);   /* 0x002239A8 */
    void buildDisc(TStripBuilder *sb);      /* 0x00223A70 */
    void buildCaliper(TStripBuilder *sb);   /* 0x00223B70 */
    void buildGlow(TStripBuilder *sb);      /* 0x00223C70 */
    /* Inline in the original (no callers; the key function, ~RaceTirePacket,
     * makes egcs write them in T449, after __tf13RaceTireModel: the class's
     * end-of-file group, in reverse declaration order). */
    void setWheelLOD(int lod) { callback.lod = lod; }   /* 0x00223FE8 */
    int  getWheelLOD() { return callback.lod; }         /* 0x00223FE0 */

    int            withBrake;               /* 0x30 the disc and caliper (levels 0, 1) */
    int            hexagon;                 /* 0x34 a hexagon instead of the wheel model (level 2) */
    RaceTireModel *owner;                   /* 0x38 */
    VifModel      *caliperPacket;           /* 0x3C */
    VifModel      *discPacket;              /* 0x40 */
    Callback       callback;                /* 0x44 */
};
EGCS_SIZE(RaceTirePacket::Callback, 0x8);
EGCS_SIZE(RaceTirePacket, 0x4C);

/* vtable _vt$13RaceTireModel 0x00340F18 (2 slots: [1] TireModelBase's
 * virtual_1), typeinfo __tf13RaceTireModel 0x00223F90 (si, base
 * TireModelBase; __si_type_info 0x004B29D0).  Size 0x18C (RaceCarModel has
 * four). */
class RaceTireModel : public TireModelBase {
public:
    RaceTireModel();                        /* 0x00221EA0 */
    virtual ~RaceTireModel();               /* [0] +0x0C 0x00221F98 */
    /*      virtual_1                          [1] +0x14 0x0021F940 TireModelBase's */

    /* (Parameter order as the original's: the same registers in any order,
     * but RaceCarModel::setupTires / drawTires / drawMenuTires (u060) load
     * the arguments in this order; draw itself is exact only so.) */
    void  setBrake(f32 radius, f32 offset, f32 angleDeg, int discNo, int caliperNo); /* 0x00222090 */
    void *build(void *work);                /* 0x002220B8 the three levels; returns the rest of work */
    f32   screenSize();                     /* 0x002221C8 the projected size of the bounding sphere */
    int   selectLOD();                      /* 0x00222258 */
    void  draw(int lod, f32 steer, f32 camber, f32 spin, f32 glow,
               int blurPasses, f32 blur);   /* 0x002222B8 lod < 0: selectLOD(); 3: the cylinder; angles in
                                               degrees, blur in radians; glow: RaceTirePacket::draw */
    void  drawCylinder(f32 steer, f32 camber, f32 spin);    /* 0x00222550 */
    /* The eight below are inline in the original (no callers; egcs writes
     * them in T449 after the static initialiser, before __tf13RaceTireModel):
     * defined `inline' in T449_race_tire_model.cpp, where the brake file and
     * the graphics library are declared. */
    int   isHeatBrake();                    /* 0x00223E48 no callers */
    int   existCaliper();                   /* 0x00223E68 no callers */
    void  setGlowTexture();                 /* 0x00223E78 (? name) does not use this; no callers */
    void  setCaliperTexture();              /* 0x00223EB0 (? name) no callers */
    void  setDiscTexture();                 /* 0x00223EF0 (? name) no callers */
    BrakeCaliper *getCaliper();             /* 0x00223F30 (? name) NULL without one; no callers */
    int   needWheelMotionBlur();            /* 0x00223F58 faceVisible; no callers */
    int   needTreadBlur();                  /* 0x00223F60 (? name) no callers */

    RaceTirePacket packets[3];              /* 0x094 levels 0..2 */
    int            disc;                    /* 0x178 the brake disc (setBrake discNo - 1) */
    int            caliper;                 /* 0x17C the caliper (caliperNo - 2; < 0: none) */
    f32            discRadius;              /* 0x180 */
    f32            discOffset;              /* 0x184 */
    f32            caliperAngle;            /* 0x188 radians */
};
EGCS_SIZE(RaceTireModel, 0x18C);

void brake_file_load(const char *path);     /* 0x00222028 once ("race/brake.bin"), relocated */
void brake_file_free(void);                 /* 0x00222068 */
void brake_textures_link(void);             /* 0x00222638 the brake textures into VRAM for the race */
void brake_textures_unlink(void);           /* 0x00222668 */
/* One pass of the wheel face blur (drawWheelTexture): the texture's (0, 0)..
 * (texSize, texSize) turned by angle about its centre into the rows
 * size..3 size of the size x 3 size blur buffer at double height, from row
 * yOffset (0: every row; 1 with the even rows masked, so that shrinking the
 * rows back averages a still and a turned pass). */
void F_002231D0(f32 angle, f32 size, f32 texSize, f32 yOffset);   /* 0x002231D0 */
/* 0x00223E00 / 0x00223E28: egcs's static destructor / initialiser of the
 * brake textures (g_0049D160, defined in T449); their FUNC markers name them
 * T449_static_dtor / _ctor. */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand declarations). */
extern "C" {
void *__tf13RaceTireModel(void);            /* 0x00223F90 */
void *__tf14RaceTirePacket(void);           /* 0x00223FF0 */
void *__tfQ214RaceTirePacket8Callback(void);    /* 0x00224040 */
}
#endif

#endif /* GAME_T449_RACE_TIRE_MODEL_H */
