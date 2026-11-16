/* T447_tire_model_base.h - TireModelBase, the tyre and wheel files
 * (translation unit T447, unit u100).
 *
 * TireModelBase: the size of a tyre (width w, rim radius r, aspect ratio a:
 * outer radius R = r + w * a, the bounding sphere and box), its VU0 view
 * test, and the tyre / wheel files it draws with.  Derived: RaceTireModel
 * (T449), MenuTireModel (T450).
 *
 * The tyre file "tire/n%d" ("menu/tire/n%d") carries the tread texture and
 * the F1 flag; the wheel file "wheel/%s" ("menu/wheel/%s") the wheel model
 * and how its variations are picked.  Both are relocated in place after
 * loading (their +4 is the address they were saved at). */
#ifndef GAME_T447_TIRE_MODEL_BASE_H
#define GAME_T447_TIRE_MODEL_BASE_H

#include "game/game.h"

struct Tex1Image;                           /* G001 (pointers only) */
class ModelSet;                             /* T451 */

/* A loaded tyre file (its size is the file's). */
struct TireFile2 {
    u8         unk_00[4];                   /* 0x00 */
    TireFile2 *base;                        /* 0x04 the address its offsets are relative to */
    u8         unk_08[8];                   /* 0x08 */
    u32        texHeight;                   /* 0x10 the tread texture's s range once around the tyre: its repeats
                                               around it (u101: treadRepeat); s wraps (CLAMP.WMS repeat) */
    int        flags;                       /* 0x14 bit 0: tread blur (RaceTireModel::needTreadBlur); bit 1: F1 tyre */
    f32        power;                       /* 0x18 the specular power of the tyre material */
    Tex1Image *tex;                         /* 0x1C the tread texture */

    int isF1() const { return (flags >> 1) & 1; }   /* (inline: no code of its own) */
};

/* A loaded wheel file (its size is the file's). */
struct WheelFile2 {
    u8          unk_00[4];                  /* 0x00 */
    WheelFile2 *base;                       /* 0x04 the address its offsets are relative to */
    u8          unk_08[4];                  /* 0x08 */
    int         size;                       /* 0x0C the file's size in bytes (carmodelfile_work_area, u102) */
    int         flags;                      /* 0x10 1: the rear wheels use the model's part 1; 2: left / right
                                               variations; 4: mirrored on the odd side; 8: no colours */
    u8          unk_14[8];                  /* 0x14 */
    ModelSet   *model;                      /* 0x1C the wheel model */
};

void  tire_file_relocate(TireFile2 *t);                         /* 0x0021F3F0 also relocates the texture */
void  tire_file_load_texture(TireFile2 *t);                     /* 0x0021F430 makes tex resident (T513) */
void  tire_file_send_texture(TireFile2 *t);                     /* 0x0021F458 sends tex's GS registers */
char *tire_file_path(char *buf, int n, int menu);               /* 0x0021F478 "tire/n%d", "menu/tire/n%d" */
void  wheel_file_relocate(WheelFile2 *w);                       /* 0x0021F4D0 also relocates the model */
int   wheel_file_lod(WheelFile2 *w, int pos);                   /* 0x0021F510 flags & 1 ? pos / 2 : 0 (the part drawn) */
void  wheel_file_select(WheelFile2 *w, int color, int pos);     /* 0x0021F538 selects the model's variation */
int   wheel_file_variation(WheelFile2 *w, int color, int pos);  /* 0x0021F5B8 -1 (flags & 8), color, or pos & 1 */
int   wheel_file_mirrored(WheelFile2 *w, int pos);              /* 0x0021F5E8 (pos & 1) && (flags & 4) */
char *wheel_file_path(char *buf, const char *name, int menu);   /* 0x0021F608 "wheel/%s", "menu/wheel/%s" */

/* vtable _vt$13TireModelBase 0x00340CB0 (2 slots), typeinfo
 * __tf13TireModelBase 0x0021FAC0 (__user_type_info 0x00353440).  0x90 bytes
 * of data, vptr +0x90.  Size 0x94 (the derived classes' members start at
 * +0x94). */
class TireModelBase {
public:
    TireModelBase();                        /* 0x0021F660 a 235/30 tyre on a 16.8" rim */
    virtual ~TireModelBase();               /* [0] +0x0C 0x0021F6B0 */
    virtual int virtual_1();                /* [1] +0x14 0x0021F940 the view test: 0 outside; sets faceVisible */

    void setSize(f32 w, f32 rim, f32 aspect);           /* 0x0021F6E0 */
    void setSizeOuter(f32 w, f32 outer, f32 aspect);    /* 0x0021F7B8 */
    void setSizeProfile(f32 w, f32 outer, f32 h);       /* 0x0021F7D8 */
    int  clipTest();                        /* 0x0021F810 the corners through VU0: 0 if all are outside one plane;
                                               clipped = one is outside the guard band (x, y beyond 2 w) */
    void setVisible();                      /* 0x0021F9D0 clipped = faceVisible = 1 */
    void drawWheel();                       /* 0x0021F9E0 the wheel model's part wheel_file_lod (modelset_draw_part) */
    int  isF1Tire();                        /* 0x0021FA38 */
    int  getWheelTexLevelBias();            /* 0x0021FA50 */
    int  getWheelVariation();               /* 0x0021FA78 */
    /* Inline in the original: the copies 0x0021FAA0..0x0021FAB8 egcs
     * emitted have no callers; RaceCarModel / Car / CarsItem (T180, T212,
     * T107) expand them (u060, u064). */
    void setColor(int c) { color = c; }                 /* 0x0021FAA0 */
    void setPosition(int pos) { position = pos; }       /* 0x0021FAA8 */
    void setWheelFile(WheelFile2 *w) { wheelFile = w; } /* 0x0021FAB0 */
    void setTireFile(TireFile2 *t) { tireFile = t; }    /* 0x0021FAB8 */

    int         position;                   /* 0x00 0..3: bit 0 the side, / 2 front / rear */
    f32         aspect;                     /* 0x04 the aspect ratio (sidewall height / width) */
    f32         width;                      /* 0x08 */
    f32         rim;                        /* 0x0C the rim radius */
    f32         outer;                      /* 0x10 the outer radius: rim + width * aspect */
    f32         diameter;                   /* 0x14 2 * bound */
    f32         bound;                      /* 0x18 the bounding sphere's radius */
    f32         box[8][3];                  /* 0x1C the corners of the bounding box */
    int         clipped;                    /* 0x7C a corner is outside the guard band (clipping needed) */
    int         faceVisible;                /* 0x80 the camera sees the outer face */
    TireFile2  *tireFile;                   /* 0x84 */
    WheelFile2 *wheelFile;                  /* 0x88 */
    int         color;                      /* 0x8C the wheel colour */
    /*          vptr                           0x90 */
};
EGCS_SIZE(TireModelBase, 0x94);

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function and its storage itself (and rejects
 * hand declarations). */
extern "C" {
void *__tf13TireModelBase(void);            /* 0x0021FAC0 */
extern u32 __ti13TireModelBase[2];          /* 0x00353440 gp-0x45B0 __user_type_info */
}
#endif

#endif /* GAME_T447_TIRE_MODEL_BASE_H */
