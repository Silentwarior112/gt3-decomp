/* T245 - CarRender (? name).  0x001CA550..0x001CB3F0.
 *
 * Draws the 3D car of the menu screens (gt2k_menu::Car, T212): the body
 * CarModel and its four MenuTireModels lit by the light set of the camera
 * (T215).  draw() renders `passes' jittered copies (the sub-pixel offsets of
 * g_002CCFC0[passes]) into buf0 (the draw buffer) and mixes each into buf1
 * (target B) with weight 1 / (i + 1): a running average, anti-aliasing by
 * accumulation.  The passes from `split' on use the previous body colour, so
 * the colour cross-fades with `blend'.  The average goes over the screen
 * with alpha; the mirror pass (the reflection on the floor) is drawn upside
 * down and added at 0.65 of the alpha. */
/* Retail's T245 data: .rodata 0x00336E00 is only the literal of
 * SimpleCameraBase's inline print() (no type names: the file saw no class
 * with inline virtual functions only); .data 0x002CCF50 the jitter tables of
 * draw() (writable, in definition order), then the frame info; .sdata
 * 0x003526F8 the one-pass table (8 bytes, -G8).  Only data points at the
 * literal and the one-pass table: */
// DATA_START 0x00336E00
// DATA_START 0x003526F8
#include "game/menu/framework/button_list_internal.h"
#include <stddef.h>
#include "game/car/model/T245_car_render.h"
#include "engine/pgl/G001_flare.h"                /* gfx_* / pgl_* / vu0_* / gs_* and their PGL_* constants,
                                               Tex1Image */
#include "engine/media/G002_mpeg_frameq.h"          /* atan2f */
#include "game/menu/gtmode/T215_showroom_data.h"        /* the showroom light sets showroom_index .. showroom_table */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* MenuCarSpec (car: the wheel accessors, the tyre radius) */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_00352758 (the screen buffer being drawn) */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar (the wheel accessors; MenuCarSpec's first
                                               0x1344 bytes) */
#include "game/camera/T410_camera_base.h"          /* CameraBase */
#include "game/camera/T411_menu_car_camera.h"      /* MenuCarCamera */
#include "game/camera/T412_simple_camera_base.h"   /* no use: retail's T245 .rodata is SimpleCameraBase::print's
                                               literal, written where the header is read (the
                                               other menu car files T212 / T215 include it too) */
#include "game/car/model/T446_car_model.h"            /* CarModel, car_shadow_set_dir */
#include "game/car/model/T450_menu_tire_model.h"      /* MenuTireModel */
#include "engine/gfx/T451_model_set.h"            /* modelset_set_env_tex */
#include "engine/gfx/T512_render_context.h"       /* g_002F4230 (wide, bufB38) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "engine/gfx/T515_tex_set.h"              /* TexSet */

// FUNC 0x001CA550 CarRender::CarRender
CarRender::CarRender(MenuCarSpec *spec, MenuCarCamera *camera)
    : spec(spec), camera(camera), envImage(NULL), unk0C(1), unk10(1)
{
    /* implicit: GSBuffer (member constructors), ~GSBuffer (exception cleanup) */
    unk84 = 0.5f;                           /* in declaration order (the scheduler then gives
                                               retail's 0x90 first) */
    unk88 = 0.5f;
    unk8C = 0.5f;
    unk90 = 0.5f;
    vpX = 320.0f;
    vpY = 112.0f;
    vpW = 640.0f;
    vpH = 224.0f;
    car_shadow_set_dir(0.0f, 0.0f, 0.0f);
}

/* The colour variation of the body and of the four tyres' wheels (this
 * unused). */
// FUNC 0x001CA658 CarRender::setColor
void CarRender::setColor(CarModel *model, MenuTireModel *tires, int color)
{
    model->color = color;
    if (tires != NULL) {
        MenuTireModel *tire = tires;
        do {                                /* no entry test, as retail */
            tire->color = color;
            tire++;
        } while (tire < tires + 4);
    }
}

/* Lighting on: the ambient colour (value 0 of the showroom record of the
 * camera's mode) and its three lights (values 4 + 16 k: ambient, diffuse,
 * specular, position; a light the record lacks is disabled); the lights are
 * placed mirrored in y for the reflection. */
// FUNC 0x001CA690 CarRender::setLights
void CarRender::setLights(int mirror)
{
    gfx_enable(PGL_CAP_CPU_LIGHTING);
    int set = showroom_index(camera);
    pgl_light_modelfv(PGL_LIGHTMODEL_AMBIENT, showroom_value_ptr(showroom_table(), 0, set));
    pgl_push_matrix();
    if (mirror != 0)
        vu0_scale(1.0f, -1.0f, 1.0f);
    if (showroom_flag(showroom_table(), 0, set)) {
        gfx_enable(PGL_CAP_LIGHT0 + 0);
        pgl_lightfv(PGL_CAP_LIGHT0 + 0, PGL_LIGHT_AMBIENT, showroom_value_ptr(showroom_table(), 0x04, set));
        pgl_lightfv(PGL_CAP_LIGHT0 + 0, PGL_LIGHT_DIFFUSE, showroom_value_ptr(showroom_table(), 0x08, set));
        pgl_lightfv(PGL_CAP_LIGHT0 + 0, PGL_LIGHT_SPECULAR, showroom_value_ptr(showroom_table(), 0x0C, set));
        pgl_lightfv(PGL_CAP_LIGHT0 + 0, PGL_LIGHT_POSITION, showroom_value_ptr(showroom_table(), 0x10, set));
    } else {
        gfx_disable(PGL_CAP_LIGHT0 + 0);
    }
    if (showroom_flag(showroom_table(), 1, set)) {
        gfx_enable(PGL_CAP_LIGHT0 + 1);
        pgl_lightfv(PGL_CAP_LIGHT0 + 1, PGL_LIGHT_AMBIENT, showroom_value_ptr(showroom_table(), 0x14, set));
        pgl_lightfv(PGL_CAP_LIGHT0 + 1, PGL_LIGHT_DIFFUSE, showroom_value_ptr(showroom_table(), 0x18, set));
        pgl_lightfv(PGL_CAP_LIGHT0 + 1, PGL_LIGHT_SPECULAR, showroom_value_ptr(showroom_table(), 0x1C, set));
        pgl_lightfv(PGL_CAP_LIGHT0 + 1, PGL_LIGHT_POSITION, showroom_value_ptr(showroom_table(), 0x20, set));
    } else {
        gfx_disable(PGL_CAP_LIGHT0 + 1);
    }
    if (showroom_flag(showroom_table(), 2, set)) {
        gfx_enable(PGL_CAP_LIGHT0 + 2);
        pgl_lightfv(PGL_CAP_LIGHT0 + 2, PGL_LIGHT_AMBIENT, showroom_value_ptr(showroom_table(), 0x24, set));
        pgl_lightfv(PGL_CAP_LIGHT0 + 2, PGL_LIGHT_DIFFUSE, showroom_value_ptr(showroom_table(), 0x28, set));
        pgl_lightfv(PGL_CAP_LIGHT0 + 2, PGL_LIGHT_SPECULAR, showroom_value_ptr(showroom_table(), 0x2C, set));
        pgl_lightfv(PGL_CAP_LIGHT0 + 2, PGL_LIGHT_POSITION, showroom_value_ptr(showroom_table(), 0x30, set));
    } else {
        gfx_disable(PGL_CAP_LIGHT0 + 2);
    }
    pgl_pop_matrix();
}

/* The wheel whose direction from the car centre (atan2 of its x and z, in
 * degrees) is nearest to the camera's orbit yaw turned by 180 (every angle
 * brought into [-180, 180]); -1 if none is below 360.  No callers: perhaps
 * once the wheel drawModel leaves out (its `hidden' is -1, see there). */
// FUNC 0x001CA9A8 CarRender::hiddenWheel
int CarRender::hiddenWheel()
{
    int wheel;                              /* one variable for both loops ($s0) */
    f32 view = camera->tumbleY.value + 180.0f;
    while (view < -180.0f)
        view += 360.0f;
    while (view > 180.0f)
        view -= 360.0f;
    f32 diff[4];
    for (wheel = 0; wheel < 4; wheel++) {
        /* spec->car (MenuCarSpec's first member: the wheel accessors) read for
         * each call, as retail */
        f32 wx = spec->car.wheelLateralPos(wheel, 0.0f, 0.0f);
        /* worked out in diff[wheel] step by step, as the wraps below: radians,
         * degrees, the difference.  The two first stores are dead (flow
         * deletes them before scheduling), but until then they make the
         * wheel loop 65 insns for loop2, over its move threshold of 64: pi's
         * and 180's literals stay in the loop, as retail (pi reloaded
         * -0x7968($gp) each pass).  As one expression the loop has 63 insns,
         * loop2 hoists pi into $f24 and the frame grows by its save. */
        diff[wheel] = atan2f(wx, spec->car.wheelAxleX(wheel, 0.0f, 0.0f));
        diff[wheel] = diff[wheel] / 3.141592f * 180.0f;
        diff[wheel] = view - diff[wheel];
        while (diff[wheel] < -180.0f)
            diff[wheel] += 360.0f;
        while (diff[wheel] > 180.0f)
            diff[wheel] -= 360.0f;
        diff[wheel] = boot_fabsf(diff[wheel]);
    }
    f32 best = 360.0f;
    int nearest = -1;
    for (wheel = 0; wheel < 4; wheel++) {
        if (diff[wheel] < best) {
            best = diff[wheel];
            nearest = wheel;
        }
    }
    return nearest;
}

/* One pass of the car: culling, the lights, the view (camera slot 2), the
 * body (for the reflection mirrored in y through the current matrix, with
 * its normal matrix and the front faces clockwise; else part 6 in black
 * first), lifted so that the front wheels touch the floor, then each tyre at
 * its place with its steer and camber in degrees.  `hidden' is -1, so no
 * tyre is ever left out (? leftover: hiddenWheel would give one). */
// FUNC 0x001CABE0 CarRender::drawModel
void CarRender::drawModel(CarModel *model, MenuTireModel *tires, int mirror, int withTire)
{
    gfx_enable(PGL_CAP_CULL_FACE);
    gfx_front_face(PGL_FRONT_CCW);
    pgl_push_matrix();
    model->beginFade();
    setLights(mirror);
    camera->applyView();
    if (mirror != 0) {
        f32 m[16] __attribute__((aligned(16)));     /* sp+0x00 */
        gfx_enable(PGL_CAP_NORMAL_MATRIX);
        pgl_get_matrix(m);
        vu0_scale(1.0f, -1.0f, 1.0f);
        pgl_push_matrix();
        vu0_invert_rigid();
        pgl_mult_matrix_90(m);
        pgl_get_matrix(m);
        pgl_set_normal_matrix(m);
        pgl_pop_matrix();
        gfx_front_face(PGL_FRONT_CW);
    } else {
        model->F_0021F328();
    }
    /* spec->car read for every call, as in hiddenWheel */
    f32 radius = spec->car.dyn.gear.tyreRadius[0];
#if ENH_PHYSICS_REAR_RIDE_HEIGHT
    /* The enhanced engine (from the PC port, its ROADMAP 68): pitched for
     * the rear axle's own height, the tyres with it (ShowRoomCar::restLift) */
    f32 pitch;
    pgl_translate(0.0f, spec->car.restLift(radius, &pitch), 0.0f);
    vu0_rotate_x(pitch);
#else
    pgl_translate(0.0f, radius - spec->car.wheelHeight(0, 0.0f, 0.0f), 0.0f);
#endif
    model->draw();
    if (tires != NULL) {
        int hidden = -1;
        MenuTireModel *tire = tires;
        for (int wheel = 0; wheel < 4; wheel++, tire++) {
            if (mirror == 0 && hidden == wheel)
                continue;
            pgl_push_matrix();
            f32 wx = spec->car.wheelLateralPos(wheel, 0.0f, 0.0f);
            f32 wy = spec->car.wheelHeight(wheel, 0.0f, 0.0f);
            f32 wz = spec->car.wheelAxleX(wheel, 0.0f, 0.0f);
            pgl_translate(wx, wy, wz);
            f32 steer = spec->car.wheelSteerAngle(wheel, 0.0f, 0.0f) * 57.29578f;
            f32 camber = spec->car.wheelCamber(wheel, 0.0f, 0.0f) * 57.29578f;
            tire->draw(steer, camber, 0.0f, withTire, 1);
            pgl_pop_matrix();
        }
    }
    model->endFade();
    pgl_pop_matrix();
    gfx_front_face(PGL_FRONT_CCW);
    gfx_disable(PGL_CAP_NORMAL_MATRIX);
}

/* Both work buffers become copies of the screen buffer, PSMCT32 (psm 0),
 * with VRAM of their own. */
// FUNC 0x001CAE58 CarRender::beginFrame
void CarRender::beginFrame()
{
    GSBuffer *screen = g_00352758;
    GSBuffer *b1 = &buf1;                   /* each through a pointer: retail stores psm through
                                               the buffer's register, &buf0 taken after the
                                               first copy */
    b1->copy(screen);
    b1->psm = 0;
    b1->allocVram();
    GSBuffer *b0 = &buf0;
    b0->copy(screen);
    b0->psm = 0;
    b0->allocVram();
}

// FUNC 0x001CAEC8 CarRender::endFrame
void CarRender::endFrame()
{
    buf1.freeVram();
    buf0.freeVram();
}

/* The jitter tables: per pass count n a table of n sub-pixel offsets {x, y}
 * (draw() scales them by 0.6).  Writable data in retail (.data / .sdata,
 * not .rodata), in this order; n = 1 is 8 bytes, so .sdata (-G8). */
static f32 g_003526F8[1 * 2] = {    /* 0x003526F8 */
    0.0f, 0.0f,
};
static f32 g_002CCF50[2 * 2] = {    /* 0x002CCF50 */
    0.25f, 0.75f,  0.75f, 0.25f,
};
static f32 g_002CCF60[3 * 2] = {    /* 0x002CCF60 */
    0.5033923f, 0.8317967f,  0.78060764f, 0.2504381f,  0.2261829f, 0.41315535f,
};
static f32 g_002CCF78[4 * 2] = {    /* 0x002CCF78 */
    0.375f, 0.25f,  0.125f, 0.75f,  0.875f, 0.25f,  0.625f, 0.75f,
};
static f32 g_002CCF98[5 * 2] = {    /* 0x002CCF98 */
    0.5f, 0.5f,  0.3f, 0.1f,  0.7f, 0.9f,  0.9f, 0.3f,  0.1f, 0.7f,
};
/* 0x002CCFC0, indexed by the pass count (button_list_internal.h's extern) */
const f32 *g_002CCFC0[6] = {        /* 0x002CCFC0 */
    NULL, g_003526F8, g_002CCF50, g_002CCF60, g_002CCF78, g_002CCF98,
};

/* See the top of the file.  Without mirror each pass starts from a copy of
 * the screen in buf0 (RGB only, its alpha kept: write mask 0x00FFFFFF);
 * with two passes the second (prevColor) one is weighted by blend / ratio. */
/* (? name) The y scale 1.098 and, on a 16:9 screen (rc->wide), x * 0.75: the
 * same two steps as T107's cars_projection (u021).  Inline: retail keeps
 * rc (&g_002F4230, as bufB38's address - 0xB38) in a register of its own for
 * the wide test. */
static inline void car_render_aspect(RenderContext *rc)
{
    F_00257EF0(1.098f);
    if (rc->wide != 0)
        vu0_scale(0.75f, 1.0f, 1.0f);
}

// FUNC 0x001CAEF8 CarRender::draw
void CarRender::draw(CarModel *model, MenuTireModel *tires, int color, int prevColor,
                     int mirror, int lamps, int withTire, f32 alpha, f32 blend, int passes)
{
    GSBuffer screen(*g_00352758);           /* sp+0x00 */  /* implicit: GSBuffer::copy (inline copy constructor) */
    screen.setField(PGL_FIELD_FRAME);
    pgl_push_attrib(PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B
                    | PGL_ATTR_VIEWPORT | PGL_ATTR_SCISSOR);
    model->setLamps(lamps);
    setColor(model, tires, color);
    GSBuffer *b1 = &buf1;                   /* target B: the average (through a pointer: retail's
                                               width / height loads use it) */
    b1->setTargetB();
    buf0.setDrawBuffer();
    g_002F4230.bufB38.setZBuffer();         /* g_002F4D68 */
    TexSet env;                             /* sp+0x40 */
    env.load(envImage);
    modelset_set_env_tex(envImage->tex);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    vu0_load_identity_regs();
    camera->applyProjection();
    car_render_aspect(&g_002F4230);         /* implicit: F_00257EF0, vu0_scale (car_render_aspect, inline) */
    vu0_scale(1.0f, vpW * 0.35f / vpH, 1.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    vu0_load_identity_regs();
    gfx_disable(PGL_CAP_FOG);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_depth_func(PGL_DEPTHFUNC_LEQUAL);
    gfx_color_mask(1, 1, 1, 1);
    int split = (int)((1.0f - blend) * (f32)(passes - 1)) + 1;  /* the first pass in prevColor */
    f32 ratio = (f32)split / (f32)passes;
    for (int i = 0; i < passes; i++) {
        if (i == split)
            setColor(model, tires, prevColor);
        gfx_depth_mask(1);
        gs_set_clear_color(0x80000000);
        gs_clear_buffers(PGL_CLEAR_COLOR | PGL_CLEAR_DEPTH);
        if (mirror == 0) {
            gfx_set_frame_write_mask(0x00FFFFFF);
            buf0.blit(&screen, PGL_FILTER_NEAREST, PGL_FILTER_NEAREST);
            gfx_set_frame_write_mask(0xFFFFFFFF);
        }
        gfx_offset_xy(g_002CCFC0[passes][i * 2] * 0.6f, g_002CCFC0[passes][i * 2 + 1] * 0.6f);
        drawModel(model, tires, mirror, withTire);
        f32 weight = 1.0f / (f32)(i + 1);
        if (passes == 2 && i >= split)
            weight *= blend / ratio;
        gfx_frame_effect(PGL_FX_MIX_A_INTO_B, weight);
    }
    screen.setDrawBuffer();
    b1->setTexture();
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_enable(PGL_CAP_BLEND);
    if (mirror != 0)    /* Cs * FIX + Cd: added */
        gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_CD),
                       alpha * 0.65f);
    else                /* (Cs - Cd) * FIX + Cd: blended */
        gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_FIX, PGL_SEL_CD), alpha);
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gs_draw_textured_sprite_strips_f(0.0f, 0.0f, (f32)screen.width, (f32)screen.displayHeight(),
                                     0.0f, 0.0f, (f32)b1->width, (f32)b1->height);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_pop_matrix();
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_CPU_LIGHTING);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_ANTIALIAS);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_disable(PGL_CAP_BLEND);
    pgl_pop_attrib();
}
