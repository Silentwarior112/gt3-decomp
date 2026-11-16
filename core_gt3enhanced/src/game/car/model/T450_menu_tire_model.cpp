/* T450_menu_tire_model.cpp - MenuTireModel (unit u101, 0x00224090..0x00224600).
 *
 * The tyre of a menu car (TheCar in the car lists, the menu car model): one
 * TirePacket of 30 segments, and the wheel drawn as part 2 (front) or 3
 * (rear) of the car's own model set.  Its view test also decides whether
 * the outer face is turned to the camera (faceVisible).
 *
 * Data (retail, all EXACT at these places): .rodata 0x003410C0..0x003410F8
 * (the origin, the vtable, the type name: no other class seen, no <new>),
 * .lit4 0x003509B4..0x003509BC, .data 0x002E70AC..0x002E721C (frame info).
 */
#include "game/car/model/tire_model_internal.h"
#include "game/car/model/T450_menu_tire_model.h"

// FUNC 0x00224090 MenuTireModel::MenuTireModel
//   TireModelBase(), the vptr 0x003410D0, TirePacket() of the packet, no
//   wheel model (with the EH cleanup ~TireModelBase).  Callers:
//   gt2k_menu::CarsItem::CarsItem, gt2k_menu::Car::Car.
MenuTireModel::MenuTireModel()
{
    /* implicit: TireModelBase, ~TireModelBase, TirePacket (compiler-generated: bases, members,
     * deleting form) */
    wheelModel = NULL;
}

// FUNC 0x00224100 MenuTireModel::~MenuTireModel
//   (slot 0) The vptr, ~TirePacket of the packet (flags 2), then
//   ~TireModelBase with the flags (the deleting form's operator delete is
//   TireModelBase's).
MenuTireModel::~MenuTireModel()
{
    /* implicit: ~TireModelBase, ~TirePacket (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x00224150 MenuTireModel::build
//   Sets up the packet (segs segments, bevel, inside faces and shadow strip,
//   x0 0.001: TirePacket::setup) and builds it at work (TirePacket::
//   virtual_1, called directly); returns the end.  Callers:
//   gt2k_menu::TheCar::setup, gt2k_menu::Car::setupTires (with 30).
void *MenuTireModel::build(void *work, int segs)
{
    TirePacket *p = &packet;
    p->setup(this, segs, 1, 1, 1, 0.001f);
    return p->TirePacket::virtual_1(work);
}

/* The origin (0, 0, 0, 1): T450's first .rodata item (0x003410C0, before its
 * vtable and type name), so external linkage (tire_model_internal.h's extern): egcs writes
 * it where it is defined.  Defined after the file's first function, so the
 * frame info keeps its name. */
const f32 g_003410C0[4] = { 0.0f, 0.0f, 0.0f, 1.0f };     /* 0x003410C0 */

// FUNC 0x002241A8 MenuTireModel::virtual_1
//   (slot 1) The view test: 0 if the bounding box is out of the view
//   (clipTest).  Else the eye in the tyre's space (vu0_invert_rigid of the
//   pushed matrix applied to the origin), seen from the face plane x =
//   -width / 2: faceVisible = its direction cosine along x (rsqrt.s) <
//   0.05; returns 1.
int MenuTireModel::virtual_1()
{
    if (clipTest() == 0)
        return 0;
    f32 eye[4];                             /* sp+0x00 */
    pgl_push_matrix();
    vu0_invert_rigid();
    vu0_apply_matrix(eye, g_003410C0);
    pgl_pop_matrix();
    f32 halfW = width * 0.5f;
    f32 yy = eye[1] * eye[1];
    halfW = -halfW;
    f32 zz = eye[2] * eye[2];
    f32 x = eye[0] - halfW;
    eye[0] = x;                             /* a dead store, as in the code */
    f32 cosine = ee_rsqrt(x, x * x + yy + zz);
    faceVisible = cosine < 0.05f ? 1 : 0;
    return 1;
}

// FUNC 0x00224260 MenuTireModel::draw
//   In the current matrix: rotated by steer (+ 180 on the odd side) about
//   y and camber about z (degrees; the third float is not read).  Nothing
//   if the view test (virtual_1, a virtual call) fails.  Moved by -width /
//   2, scaled by the rim radius (pgl_set_scale), mirrored if the wheel file
//   says so (and the other cull face); the tyre (withTire, once built:
//   TirePacket::draw, texture alpha off around it); with the face seen and
//   a wheel model, the model's part 3 (rear: position / 2 != 0) or 2
//   (modelset_draw_part), mirrored once more when the side and the
//   mirroring differ (so it is mirrored on the odd side only).  That flip
//   sets the other cull face again, not the one found: on the even side
//   of a mirrored wheel file the model is drawn unmirrored with the
//   mirrored cull face (? a bug).  Then, with a tyre file that is not an F1 tyre,
//   withTire, withShadow and a built packet: the shadow strip
//   (TirePacket::drawShadow) without the steer, colour only.  Callers:
//   gt2k_menu::TheCar::drawTires, CarRender::drawModel.
void MenuTireModel::draw(f32 steer, f32 camber, f32 unused, int withTire, int withShadow)
{
    pgl_push_matrix();
    int side = position & 1;
    vu0_rotate_y(steer + (side ? 180.0f : 0.0f));
    vu0_rotate_z(camber);
    if (virtual_1() == 0) {
        pgl_pop_matrix();
        return;
    }
    if (clipped == 0)
        gfx_disable(PGL_CAP_VU1_PROG_33E);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    int otherMode = 1;
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_set_texture_function(PGL_TFX_MODULATE);
    gfx_depth_mask(1);
    gfx_set_frame_write_mask(GS_WRITE_RGBA);
    pgl_push_matrix();
    gfx_enable(PGL_CAP_FLAG24);
    gfx_enable(PGL_CAP_SCALE);
    pgl_translate(-width * 0.5f, 0.0f, 0.0f);
    int mode = gfx_get_cull_face();
    if ((mode ^ 1) == 0)                    /* gfx_other_cull_mode(mode) */
        otherMode = 2;
    int mirror = wheel_file_mirrored(wheelFile, position);
    f32 r = rim;
    pgl_set_scale(r, r, r);                 /* F_00257F40 */
    if (mirror != 0) {
        vu0_scale(1.0f, 1.0f, -1.0f);
        gfx_cull_face(otherMode);
    }
    if (withTire != 0) {
        TirePacket *p = &packet;
        if (p->built != 0) {
            gfx_tex_param(PGL_TEXPARAM_TCC, PGL_TCC_RGB);
            p->draw();
            gfx_tex_param(PGL_TEXPARAM_TCC, PGL_TCC_RGBA);
        }
    }
    if (faceVisible != 0 && wheelModel != NULL) {
        /* flipped again when the side and the mirroring differ: a ?: as
         * the condition (a jump from each arm, retail's sltu / xori) */
        bool mirrored = mirror;
        if ((side ? mirrored ^ 1 : mirrored) != 0) {
            vu0_scale(1.0f, 1.0f, -1.0f);
            gfx_cull_face(otherMode);
        }
        if (position / 2 != 0)
            modelset_draw_part(wheelModel, 3);
        else
            modelset_draw_part(wheelModel, 2);
    }
    gfx_cull_face(mode);
    gfx_disable(PGL_CAP_SCALE);
    gfx_disable(PGL_CAP_FLAG24);
    pgl_pop_matrix();
    gfx_enable(PGL_CAP_VU1_PROG_33E);
    gfx_depth_mask(1);
    gfx_set_frame_write_mask(GS_WRITE_RGBA);
    pgl_pop_matrix();
    if (tireFile == NULL)
        return;
    if (tireFile->isF1())
        return;
    if (withTire == 0)
        return;
    if (withShadow == 0)
        return;
    TirePacket *p = &packet;
    if (p->built == 0)
        return;
    pgl_push_matrix();
    vu0_rotate_y(side ? 180.0f : 0.0f);
    vu0_rotate_z(camber);
    gfx_set_frame_write_mask(GS_WRITE_RGB);
    p->drawShadow();
    gfx_set_frame_write_mask(GS_WRITE_RGBA);
    pgl_pop_matrix();
}

// FUNC 0x002245A8 MenuTireModel::setWheelModel
/* inline in game/car/model/T450_menu_tire_model.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __si_type_info
 * __ti13MenuTireModel (0x004B29F0, name 0x003410E8) on the first call,
 * after __tf13TireModelBase (base __ti13TireModelBase, 0x00353440). */
// FUNC 0x002245B0 __tf13MenuTireModel
extern "C" void *__tf13MenuTireModel(void)
{
    if (__ti13MenuTireModel[0] == 0) {
        __tf13TireModelBase();
        __rtti_si(__ti13MenuTireModel, "13MenuTireModel", (const type_info *)__ti13TireModelBase);
    }
    return __ti13MenuTireModel;
}
#endif
