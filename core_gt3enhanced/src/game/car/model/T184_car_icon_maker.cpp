/* T184 - CarIcon and CarIconMaker: the 64 x 32 icons of the cars of a race.
 * 0x001ADA68..0x001AE4E0.
 *
 * car_icons_make puts a CarIconMaker on its stack, gives it to the render
 * thread (render_set_object) and sleeps.  In the render thread, render()
 * draws every icon in the first frame (frame 0); update() counts the frames
 * and from the third one hands the render thread back and wakes the
 * caller. */
/* Retail's .rodata (0x00330C20..0x00330C79): "bad_alloc" (<new>'s inline
 * what(), written when the header is read: DATA_START), the vtable, then
 * the type names CarIconMaker, UnitArena<64> (reverse definition order).
 * .data 0x002C3EF8: the light (four vectors), then the frame info; .sdata
 * 0x00352248: drawIcon's constant pool (the int 32 it converts to float). */
// DATA_START 0x00330C20
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include <new>
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (setModelLod), RaceEntryBase */
#ifdef BOOT_OLD_GCC
/* drawIcon's fixed LOD goes through RaceEntryCar::setModelLod (T121), whose
 * body calls F_00179460, an inline function in the original whose body
 * came after it: the call stays (retail calls T121's copy 0x00179460) and
 * the unit writes a weak copy, last of its deferred functions (after
 * __tf12CarIconMaker and the implicit destructor: the frame info's last
 * FDE), as T129 / T180 do.  Its body here, before CarIconMaker is defined
 * (T184_car_icon_maker.h): egcs writes the deferred functions in the
 * reverse of the order it saved them.  GCC 14 calls T121's F_00179460. */
// FUNC 0x001AE4D8 F_00179460
//   T184's dead weak copy (no callers; 0x00179460, T121's, is the one
//   linked): returns its argument.
inline CarModel *F_00179460(CarModel *m)
{
    return m;
}
#endif
#include "game/car/model/T184_car_icon_maker.h"
#include "eekernel.h"                       /* GetThreadId, SleepThread, (Cancel)WakeupThread */
#include "libgraph.h"                       /* SCE_GS_PSMCT32 / PSMZ24 */
#include "engine/pgl/G001_flare.h"                /* gfx_*, pgl_*, gs_*, vu0_*, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* tanf, sincosf */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization */
#include "game/race/core/T131_race_ps2_render.h"      /* CarState (= RaceCarState) */
#include "game/car/model/T180_race_car_model.h"       /* RaceCarModel */
#include "game/car/model/T446_car_model.h"            /* CarModel::callback */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState::params (CarDynamics, T279) */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* MenuCarSpec (RaceCarModel::drawMenuTires' parameter) */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::getLength / getHeight */
#include "engine/gfx/T451_model_set.h"            /* modelset_set_env_tex */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/effects/replay_effects_internal.h"                           /* g_002C3EF8 .. g_002C3F28 */

/* The work buffers of drawIcon: the model is drawn at twice the icon size. */
enum {
    ICON_W = 64, ICON_H = 32,               /* CarIcon's image */
    ICON_MODEL_W = 128, ICON_MODEL_H = 64,  /* the model, its mask */
    ICON_ENV_SIZE = 128                     /* the (black) reflection texture */
};
/* The frame-buffer write masks (gfx_set_frame_write_mask: the bits written). */
#define WRITE_ALPHA 0xFF000000u
#define WRITE_RGB   0x00FFFFFFu
#define WRITE_ALL   0xFFFFFFFFu

/* Callers: the race modes' constructors (array construction of the 7
 * icons). */
// FUNC 0x001ADA68 CarIcon::CarIcon
CarIcon::CarIcon()
{
    pixels = (u8 *)((u32)(image + 0x3F) & 0xFFFFFFC0);
}

// FUNC 0x001ADA88 CarIconMaker::CarIconMaker
/* Caller: car_icons_make. */
CarIconMaker::CarIconMaker(int count, CarIcon *icons, RaceOrganization *org)
    : GameObject(), count(count), icons(icons), org(org), frame(0)
{
}

/* Hands itself to the render thread and sleeps until finish() wakes it
 * (a wake-up already pending is cancelled first), then waits a vsync. */
// FUNC 0x001ADAF0 CarIconMaker::run
void CarIconMaker::run()
{
    int tid = GetThreadId();
    frame = 0;
    threadId = tid;
    CancelWakeupThread(GetThreadId());
    render_set_object(this);
    SleepThread();
    gfx_vsync(1);
}

/* Caller: update. */
// FUNC 0x001ADB40 CarIconMaker::finish
void CarIconMaker::finish()
{
    render_set_object(NULL);
    WakeupThread(threadId);
}

/* Slot 1 (index.txt: CarIconMaker::virtual_1).  Once the icons are drawn
 * (frame != 0): counts, and finishes when it was 2 or more. */
// FUNC 0x001ADB70 CarIconMaker::update
void CarIconMaker::update()
{
    int f = frame;
    if (f != 0) {
        frame = f + 1;
        if (f >= 2)
            finish();
    }
}

/* Slot 2 (index.txt: CarIconMaker::virtual_2).  buf is not used: each icon
 * is drawn into a 64 x 32 buffer of its own and downloaded into the icon. */
// FUNC 0x001ADBA8 CarIconMaker::render
void CarIconMaker::render(GSBuffer &buf)
{
    if (frame != 0)
        return;
    for (int i = 0; i < org->entries->count && i < count; i++) {
        GSBuffer icon(SCE_GS_PSMCT32, ICON_W, ICON_H);  /* sp+0x00 */
        drawIcon(i, &icon);
        icon.download(icons[i].pixels, SCE_GS_PSMCT32);
    }
    frame = frame + 1;
}

/* Car `car' of the race into out (64 x 32):
 *   - its ShowRoomCar record (the wheel geometry) built from its equipments
 *     in a new condition;
 *   - the reflection: a 128 x 128 buffer cleared black, given to the car
 *     models as their environment texture;
 *   - the model into a 128 x 64 colour buffer with depth, a perspective of
 *     0.157 rad (tanf: distance = 64 / tan), scaled 35, raised by the front
 *     tyre radius less the wheel centre height, less half the car's height,
 *     turned 158 degrees, lit by setLights, LOD 0, the body then the tyres
 *     posed by the record;
 *   - the alpha normalised: where the destination alpha bit is set
 *     (dest alpha test GEQUAL), alpha written 0x80 (write mask alpha only);
 *   - the mask: the colour buffer drawn into a PSMCT32 copy with alpha test
 *     GREATER 0x40 (an outline loop over radius r and 16 angles, but r runs
 *     over 0 only: one sprite);
 *   - the colour buffer drawn onto itself shifted by half a texel, bilinear
 *     (a 2 x 2 box filter);
 *   - both halved into out: the mask into its alpha (write mask
 *     0xFF000000), the colour into its RGB (0x00FFFFFF).
 *
 * Matching (u061): 99.8%.  The word 32 at 0x00352248 (`lui; addiu;
 * lwc1 0($v0); cvt.s.w`) is egcs's constant pool, not a variable: iconH is
 * (f32)halfH, the int 32 that egcs loads into an FPR from a pool entry
 * (.sdata under -G8, addressed with lui / addiu; the data pass, from u059's
 * hint).  Left: mask.setTexture() before the halving: retail recomputes
 * sp + 0x1480, ours reuses $s4. */
// FUNC 0x001ADCB8 CarIconMaker::drawIcon
void CarIconMaker::drawIcon(int car, GSBuffer *out)
{
    f32 mm = 0.001f;                                    /* f21, loaded first */
    RaceEntryCar *entry = org->entries->cars[car];
    ShowRoomCar spec;                                   /* sp+0x0000 */
    CarCondition cond;                                  /* sp+0x1350 (CarCondition::CarCondition) */

    spec.SetupShowRoomCar(&entry->info, &cond);
    RaceCarModel *model = &entry->model;
    CarState *state = entry->state;
    entry->info.getLength();                            /* result unused */
    f32 carHeight = (f32)entry->info.getHeight() * mm;  /* mm -> m */
    f32 tyre = state->params.gear.tyreRadius[0];
#if ENH_PHYSICS_REAR_RIDE_HEIGHT
    /* The enhanced engine (from the PC port, its ROADMAP 68): pitched for
     * the rear axle's own height (ShowRoomCar::restLift; turned below, in
     * the car's frame after its lights) */
    f32 pitch;
    f32 lift = spec.restLift(tyre, &pitch);
#else
    f32 lift = tyre - spec.wheelHeight(0, 0.0f, 0.0f);
#endif

    /* the reflection: a cleared 128 x 128 buffer */
    GSBuffer env(SCE_GS_PSMCT32, ICON_ENV_SIZE, ICON_ENV_SIZE);    /* sp+0x1370 */
    env.setDrawBuffer();
    gs_set_clear_color_f(0.0f, 0.0f, 0.0f, 1.0f);
    gs_clear_buffers(PGL_CLEAR_COLOR);
    GsTexPacket envTex;                                 /* sp+0x13B0 */
    env.makeTexRegs(&envTex.regs);
    modelset_set_env_tex(&envTex.regs);

    /* the model, 128 x 64 with depth */
    /* Plain ints, not constants: retail keeps halfW in fp and rematerialises
     * halfH (addiu 0x20) where it is used, then shifts / converts them. */
    int halfW = ICON_W;                                 /* fp: the icon width, also half the model's */
    int halfH = ICON_H;
    GSBuffer color(SCE_GS_PSMCT32, ICON_MODEL_W, ICON_MODEL_H);    /* sp+0x1400 */
    color.setDrawBuffer();
    GSBuffer depth(&color, SCE_GS_PSMZ24);              /* sp+0x1440 */
    depth.setZBuffer();
    gs_set_clear_color_f(0.0f, 0.0f, 0.0f, 0.0f);
    gs_clear_buffers(PGL_CLEAR_COLOR | PGL_CLEAR_DEPTH);
    int w = color.width;
    int h = color.displayHeight();
    gfx_set_draw_area(0, 0, w, h);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    f32 dist = (f32)h / tanf(0.157f);
    pgl_frustum((f32)-w * 0.5f, (f32)w * 0.5f, (f32)-h * 0.5f, (f32)h * 0.5f,
                0.01f, dist + (f32)w * 0.5f, dist);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();
    int radius = 0;                                     /* the mask's outline radius: s0 = 0 from here */
    pgl_translate(0.0f, 0.0f, -dist);
    vu0_scale(35.0f, 35.0f, 35.0f);
    pgl_translate(0.0f, lift - carHeight * 0.5f, 0.0f);
    vu0_rotate_y(158.0f);
    setLights();
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_set_fog_params(0.0f, 1.0f);
    entry->setModelLod(0);                              /* a fixed LOD (u061 CarDrawParam mode) */
    /* implicit: getCarModel, F_00179460 (setModelLod's inline body: the call to F_00179460 stays) */
#if ENH_PHYSICS_REAR_RIDE_HEIGHT
    vu0_rotate_x(pitch);                                /* the enhanced engine: the rest pitch (above) */
#endif
    model->drawBody(0.0f, 0);                           /* in the current matrix */
    model->drawMenuTires(&spec);

    /* the alpha normalised: 0x80 where the destination alpha bit is set */
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_FOG);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    color.setTexture();
    gfx_enable(PGL_CAP_DEST_ALPHA);
    gfx_dest_alpha_func(PGL_ATEST_GEQUAL, 0.5f);    /* ref: not read */
    gfx_set_frame_write_mask(WRITE_ALPHA);
    gs_draw_flat_sprite(0, 0, ICON_MODEL_W, ICON_MODEL_H);
    gfx_disable(PGL_CAP_DEST_ALPHA);

    /* the mask: the colour buffer where its alpha is above 0x40 */
    GSBuffer mask(&color, SCE_GS_PSMCT32);              /* sp+0x1480 */
    mask.setDrawBuffer();
    color.setTexture();
    gs_set_clear_color_f(0.0f, 0.0f, 0.0f, 0.0f);
    gs_clear_buffers(PGL_CLEAR_COLOR);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    gfx_enable(PGL_CAP_ALPHA_TEST);
    gfx_alpha_func_raw(PGL_ATEST_GREATER, 0x40);
    {
        int maskW = ICON_MODEL_W;                       /* s3 */
        int maskH = ICON_MODEL_H;                       /* s1 */
        do {
            for (f32 angle = 0.0f; angle < 6.28f; angle += 0.3925f) {
                if (radius != 0 || angle == 0.0f) {
                    f32 dx, dy;                         /* sp+0x14C0, sp+0x14C4 */
                    sincosf(angle, &dx, &dy);
                    dx = dx * (f32)radius;
                    dy = dy * (f32)radius;
                    gs_draw_textured_sprite_f(dx, dy, (f32)maskW + dx, (f32)maskH + dy,
                                              0.0f, 0.0f, (f32)maskW, (f32)maskH);
                }
            }
            radius++;
        } while (radius <= 0);
    }
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_set_frame_write_mask(WRITE_ALL);

    /* the colour buffer smoothed onto itself (half a texel, bilinear) */
    color.setDrawBuffer();
    color.setTexture();
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    {
        f32 modelW = (f32)(halfW * 2);
        f32 modelH = (f32)(halfH * 2);
        f32 u1 = modelW + 0.5f;                         /* f22 */
        f32 v1 = modelH + 0.5f;                         /* f21 */
        gs_draw_textured_sprite_f(0.0f, 0.0f, modelW, modelH, 0.5f, 0.5f, u1, v1);

        /* halved into the icon: alpha from the mask, colour from the model */
        out->setDrawBuffer();
        mask.setTexture();
        gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
        gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
        gfx_set_frame_write_mask(WRITE_ALPHA);
        f32 iconW = (f32)halfW;                         /* f25 */
        f32 iconH = (f32)halfH;                         /* f24: 32 from the constant pool */
        gs_draw_textured_sprite_f(0.0f, 0.0f, iconW, iconH, 0.5f, 0.5f, u1, v1);
        color.setTexture();
        gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
        gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
        gfx_set_frame_write_mask(WRITE_RGB);
        gs_draw_textured_sprite_f(0.0f, 0.0f, iconW, iconH, 0.5f, 0.5f, u1, v1);
        gfx_set_frame_write_mask(WRITE_ALL);
    }
    /* the GSBuffers destroyed (~GSBuffer, flags 2) from mask to env */
}

/* The light of the icons (.data 0x002C3EF8, in front of the frame info):
 * a directional light from (1, 0.5, -10.5), grey ambient, bright diffuse,
 * white specular. */
f32 g_002C3EF8[4] = { 1.0f, 0.5f, -10.5f, 0.0f };   /* 0x002C3EF8 PGL_LIGHT_POSITION */
f32 g_002C3F08[4] = { 0.8f, 0.8f, 0.8f, 1.0f };     /* 0x002C3F08 PGL_LIGHT_AMBIENT */
f32 g_002C3F18[4] = { 1.5f, 1.5f, 1.5f, 1.0f };     /* 0x002C3F18 PGL_LIGHT_DIFFUSE */
f32 g_002C3F28[4] = { 1.0f, 1.0f, 1.0f, 1.0f };     /* 0x002C3F28 PGL_LIGHT_SPECULAR */

/* Light 1 (PGL_CAP_LIGHT0 + 1: ambient, diffuse, specular, a directional
 * position) on, lights 0, 2 and 3 off. */
// FUNC 0x001AE370 CarIconMaker::setLights
void CarIconMaker::setLights()
{
    pgl_lightfv(PGL_CAP_LIGHT0 + 1, PGL_LIGHT_AMBIENT, g_002C3F08);
    pgl_lightfv(PGL_CAP_LIGHT0 + 1, PGL_LIGHT_DIFFUSE, g_002C3F18);
    pgl_lightfv(PGL_CAP_LIGHT0 + 1, PGL_LIGHT_SPECULAR, g_002C3F28);
    pgl_lightfv(PGL_CAP_LIGHT0 + 1, PGL_LIGHT_POSITION, g_002C3EF8);
    gfx_disable(PGL_CAP_LIGHT0);
    gfx_enable(PGL_CAP_LIGHT0 + 1);
    gfx_disable(PGL_CAP_LIGHT0 + 2);
    gfx_disable(PGL_CAP_LIGHT0 + 3);
}

// FUNC 0x001AE3F8 car_icons_make
/* Makes the icons of the race's cars (as many as there are; the callers
 * have 7).  Callers: the race modes' virtual_43.  The maker's implicit
 * destructor, GameObject::~GameObject(&maker, 2), runs at the end. */
void car_icons_make(CarIcon *icons, RaceOrganization *org)
{
    CarIconMaker maker(org->entries->count, icons, org);    /* sp+0x00 */
    maker.run(); /* implicit: ~GameObject (the maker's implicit destructor) */
}

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001AE468 __tf12CarIconMaker
/* __si_type_info __ti12CarIconMaker (0x004B23A0), base __ti10GameObject. */
extern "C" const type_info &__tf12CarIconMaker(void)
{
    if (__ti12CarIconMaker[0] == 0) {
        __tf10GameObject();
        __rtti_si(__ti12CarIconMaker, "12CarIconMaker", (const type_info *)__ti10GameObject);
    }
    return *(const type_info *)__ti12CarIconMaker;
}
#endif

// FUNC 0x001AE4B8 CarIconMaker::~CarIconMaker
/* the implicit destructor (game/car/model/T184_car_icon_maker.h); egcs emits this
 * copy: GameObject::~GameObject(this, flags), no vptr of its own stored */
/* implicit: ~GameObject (base destructor) */

/* 0x001AE4D8: F_00179460's weak copy, written last (see the includes). */
