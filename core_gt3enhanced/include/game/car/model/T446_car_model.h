/* T446_car_model.h - CarModel and CarModel::Callback, CarModelLamps,
 * the car list (translation unit T446, unit u100).
 *
 * CarModel: the drawn body of a car, a ModelSet (T451) with a
 * ModelSet::Callback that answers the model's switches (the lamps; the lamp
 * brightnesses as morph positions) and picks its LOD.  The parts drawn
 * (modelset_draw_part): draw (part 0, the body, colour variation `color'),
 * drawShadow (part 1 squashed onto the ground along the shadow direction),
 * and the parts 4..7, some with the ground-pass matrices.  Users:
 * RaceCarModel (T180, u042), the menu cars (u021, u060, u064, u070).
 *
 * The car list: the directory "cars/day" of the volume holds one file per
 * car model; car_list_load turns it into (code, name) pairs sorted by code
 * (code = nameToCode of the name, T481).  The model files are "cars/day/",
 * "cars/night/", "menu/cars/" and "cars/eve/" + the name (kinds 0..3). */
#ifndef GAME_T446_CAR_MODEL_H
#define GAME_T446_CAR_MODEL_H

#include "game/game.h"
#include "engine/gfx/T451_model_set.h"            /* base ModelSet::Callback (of CarModel::Callback) */

/* CarModel+0x34: the last four states of the lamps (pushLamps shifts one in
 * per call); fraction() is their share that is on. */
struct CarModelLamps {
    void fill(int on);                      /* 0x0021ED50 all four */
    void push(int on);                      /* 0x0021ED80 shifts in a new state */
    f32  fraction();                        /* 0x0021EDB0 the fraction of them set */

    s8   v[4];                              /* 0x0 */
};
EGCS_SIZE(CarModelLamps, 0x4);

/* Not polymorphic.  Size 0x38 in GT3 (embedded in RaceCarModel at +0x4,
 * u042); 0x54 in the enhanced engine (the render inputs at the end). */
class CarModel {
public:
    /* vtable _vt$Q28CarModel8Callback 0x00340C28 (3 slots), typeinfo
     * __tfQ28CarModel8Callback 0x0021F3A0 (si, base ModelSet::Callback,
     * __si_type_info 0x004B29B0).  vptr +0x00 (the base has no data).
     * Size 0x24.  The current ModelSet callback while the body is drawn. */
    class Callback : public ModelSet::Callback {
    public:
        Callback();                         /* 0x0021EC08 lod -1, lodDist 220, steer 0.5 */
        virtual ~Callback();                /* [0] +0x0C 0x0021EC68 */
        virtual int selectSwitch(int what); /* [1] +0x14 0x0021EC90 0: brakeLamps; 1..4: wheelShake[what - 1],
                                               5: steer as the morph position (modelset_set_morph), branch 0;
                                               the enhanced engine (L2 0x0022D9E8): 6..10 from the
                                               CarModel's render inputs (the callback is its first member) */
        virtual int selectLOD(f32 dist);    /* [2] +0x1C 0x0021ED08 lod, else 0 beyond lodDist, 1 beyond
                                               0.33 lodDist, else 2 */

        /*  vptr                               0x00 */
        int brakeLamps;                     /* 0x04 the brake lamps are on (T180: st.brake > 0) */
        int lod;                            /* 0x08 a fixed LOD, or -1 */
        f32 lodDist;                        /* 0x0C 220.0 */
        f32 wheelShake[4];                  /* 0x10 per wheel (switches 1..4): car_wheel_shake_at, 0.5 = rest (T180) */
        f32 steer;                          /* 0x20 switch 5: car_steer_view, 0.5 = straight (T180) */
    };

    CarModel();                             /* 0x0021EB50 */
    ~CarModel();                            /* 0x0021EBC0 */
    void setModel(ModelSet *m, int relocate);   /* 0x0021EA90 relocate: modelset_relocate first */
    void clearModel();                      /* 0x0021EB48 */
    void pushLamps(int on);                 /* 0x0021EDF8 */
    void setLamps(int on);                  /* 0x0021EE20 */
    void setWheelShake(u32 wheel, f32 v);   /* 0x0021EE48 wheel < 4 */
    void setSteer(f32 v);                   /* 0x0021EE68 */
    void setDirt(f32 v);                    /* 0x0021EE70 */
    void beginFade();                       /* 0x0021EE78 pgl_set_scalar_param(0, 1 - dirt): qAF0, the scale of
                                               the environment-map colours (lighting mode 2, u115) */
    void endFade();                         /* 0x0021EEA8 the same, 1 */
    void draw();                            /* 0x0021EED0 part 0, the body; the lamps' fraction is the emission
                                               scale of the flagged materials (GS_OVR_MAT_SCALE) */
    void F_0021EFC0();                      /* 0x0021EFC0 empty */
    void F_0021EFC8();                      /* 0x0021EFC8 part 7 */
    void F_0021F010();                      /* 0x0021F010 part 4 */
    void beginGroundPass();                 /* 0x0021F0B0 both matrices pushed, a depth bias, y + 0.02 */
    void endGroundPass();                   /* 0x0021F108 */
    void drawShadow();                      /* 0x0021F148 part 1 */
    void F_0021F258();                      /* 0x0021F258 part 6 in the shadow colour, ground pass */
    void F_0021F2E0();                      /* 0x0021F2E0 part 5, ground pass */
    void F_0021F328();                      /* 0x0021F328 part 6 in black */

    /* ---- The enhanced engine (rendering.md 1, 2; ref/enhanced/ported/
     * rendering.md).  Lupo 2's CarModel wrappers.  The race car model
     * (RaceCarModel::updateBodyState, T180) and the menu car
     * (gt2k_menu::Car::virtual_1, T212) call them every frame. */
    /* The model's animations stepped by 1/60 s: the CLUT records of
     * animSelect (all with -1) and, with -1, the model's tracks
     * (modelset_step_anims; ENH_RENDER_CAR_ANIMS 0: the CLUT records only). */
    void animate();                         /* L2 0x0022DAE8 */
    /* The dirty CLUT records written to the palettes
     * (modelset_apply_clut_anims; Lupo 2's CarModel::reset).  Once per frame:
     * RaceCarModel::applyAnims (from RacePS2Base::virtual_3), the menu car. */
    void applyAnims();                      /* L2 0x0022DB18 */
    /* A new animSelect rewinds the CLUT records of that id
     * (modelset_reset_anims(model, id)); the same id does nothing. */
    void setAnimSelect(int id);             /* L2 0x0022DC90 */
    void setWing(f32 v);                    /* L2 0x0022DC60 wing */
    void setPodTail(f32 lean, f32 bob);     /* L2 0x0022DC68 podLean, podBob */
    void setPodColor(int color, f32 blend); /* L2 0x0022DC78 podColor, podColorBlend */
    void setPodState(int state);            /* L2 0x0022DC88 podState */

    Callback       callback;                /* 0x00 */
    ModelSet      *model;                   /* 0x24 the body model */
    int            color;                   /* 0x28 the colour index */
    int            colorCount;              /* 0x2C the colour variations of the model's images */
    f32            dirt;                    /* 0x30 car_dirt / CarCondition::dirt (0..0.8): beginFade scales the
                                               environment map by 1 - dirt */
    CarModelLamps  lamps;                   /* 0x34 */

    /* ---- The enhanced engine: Lupo 2's render inputs (rendering.md 2;
     * ref/enhanced/INTERFACES.md), after GT3's 0x38 bytes.  Lupo 2 put
     * them in its CarModel::Callback at +0x24..+0x3C (its later CarModel
     * fields moved up by 0x1C); selectSwitch answers SWITCH ids 6..10 from
     * them (L2 0x0022D9E8; the callback is this object's first member).
     * Writer: rendering (RaceCarModel::updateBodyState from the car's
     * RaceCarState, the menu car).  The constructor sets Lupo 2's defaults
     * (its Callback constructor L2 0x0022D900): podLean / podBob 0.5,
     * animSelect -1 (every record), the others 0.  A car that is not a Pod
     * keeps them, so on a model with those switches ids 7..10 answer arm 1
     * (podBob), arm 0, arm 0 and none (-1: past the table).  GT3's car
     * models ask ids 0..5 only (checked: cars/day, night, eve, menu/cars). */
    f32            wing;                    /* 0x38 SWITCH 6 morph: the moving part 0..1 (RaceCarState.wingPosition;
                                               L2 setter 0x0022DC60; Lupo 2 +0x24) */
    f32            podLean;                 /* 0x3C SWITCH 7 morph, branch 0, when != 0.5 (the Pod's tail lean;
                                               L2 0x0022DC68; Lupo 2 +0x28) */
    f32            podBob;                  /* 0x40 SWITCH 7 morph, branch 1, when podLean == 0.5 (Lupo 2 +0x2C) */
    int            podColor;                /* 0x44 SWITCH 8 branch: the Pod's colour category (L2 0x0022DC78;
                                               Lupo 2 +0x30) */
    f32            podColorBlend;           /* 0x48 SWITCH 8 morph: the cross-fade to it (Lupo 2 +0x34) */
    int            podState;                /* 0x4C SWITCH 9 branch: the Pod's state (L2 0x0022DC88; Lupo 2 +0x38) */
    int            animSelect;              /* 0x50 SWITCH 10 branch and the CLUT animation id the model plays, -1
                                               every record (setAnimSelect L2 0x0022DC90: modelset_reset_anims of
                                               the new id; the Pod's face; Lupo 2 +0x3C) */
};
EGCS_SIZE(CarModel::Callback, 0x24);
EGCS_SIZE(CarModel, 0x54);

int         car_list_compare(const void *a, const void *b);     /* 0x0021E7D8 qsort: by code (unsigned) */
void        car_list_load(void);                                /* 0x0021E800 */
int         car_list_find(u64 code);                            /* 0x0021E910 the index; 0 if absent */
const char *car_list_name(u64 code);                            /* 0x0021E968 the model file's name */
char       *car_model_path(u64 code, char *path, int kind);     /* 0x0021E998 kind 0 day, 1 night, 2 menu, 3 eve */
void       *car_model_load(u64 code, void *buf, int menu);      /* 0x0021EA38 the day or menu file (0xA8000 bytes at most) */
void        car_shadow_set_dir(f32 x, f32 y, f32 z);            /* 0x0021F058 (x, 0, z) through the current matrix */
void        car_shadow_set_color(f32 r, f32 g, f32 b, f32 a);   /* 0x0021F090 */

/* The car shadow strength: drawShadow (0x0021F148) multiplies the shadow
 * colour's alpha by it; T107 sets 0.6 before the prize cars are drawn, T180
 * sets it while racing.  T446 .sdata (after the car list's words), initially
 * 0. */
extern f32 g_00352AC4;                      /* 0x00352AC4 gp-0x4F2C */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function and its storage itself (and rejects
 * hand declarations). */
extern "C" {
void *__tfQ28CarModel8Callback(void);       /* 0x0021F3A0 */
extern u32 __tiQ28CarModel8Callback[3];     /* 0x004B29B0 __si_type_info (T446 .bss) */
}
#endif

#endif /* GAME_T446_CAR_MODEL_H */
