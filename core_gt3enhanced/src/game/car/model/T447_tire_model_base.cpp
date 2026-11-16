/* T447 - TireModelBase: the size, the view test and the files of a tyre.
 * 0x0021F3F0..0x0021FB00.
 *
 * The size: width w, rim radius r, aspect ratio a (the sidewall height is
 * w * a): outer radius R = r + w * a, the bounding sphere's radius
 * sqrt(R^2 + w^2 / 4), the bounding box x = -w/2 .. +w/2, y, z = -R .. R.
 * (Default 0.235 / 0.21335 / 0.3: a 235/30 tyre on a 16.8" rim.)
 *
 * The tyre file "tire/n%d" carries the tread texture (+0x1C) and the F1
 * flag; the wheel file "wheel/%s" carries the wheel model (+0x1C) and how
 * its variations are picked (front / rear part, left / right, colour).
 * Both are relocated in place after loading (their +4 is the address they
 * were saved at). */
#include "game/car/model/car_model_internal.h"
#include "vu0asm.h"

/* ------------------------------------------------------------------------ */
/* The tyre and wheel files                                                 */
/* ------------------------------------------------------------------------ */

/* The two relocators wrap the pointer's relocation in a statement block
 * (`do { } while (0)', ? a macro): retail aligns the label after it to 8
 * bytes (the nop at 0x0021F41C / 0x0021F4FC), which only a loop note gives. */
// FUNC 0x0021F3F0 tire_file_relocate
void tire_file_relocate(TireFile2 *t)
{
    TireFile2 *savedAt = t->base;
    Tex1Image *tex = t->tex;
    t->base = t;
    int delta = (u8 *)t - (u8 *)savedAt;
    do {
        if (tex != NULL) {
            t->tex = (Tex1Image *)((u8 *)tex + delta);
            tex1_relocate(t->tex);
        }
    } while (0);
}

// FUNC 0x0021F430 tire_file_load_texture
void tire_file_load_texture(TireFile2 *t)
{
    g_002F4250.loadTex1(t->tex);
}

/* The GS registers of the texture's first image. */
// FUNC 0x0021F458 tire_file_send_texture
void tire_file_send_texture(TireFile2 *t)
{
    gfx_set_texture_regs(t->tex->tex);
}

// FUNC 0x0021F478 tire_file_path
char *tire_file_path(char *buf, int n, int menu)
{
    if (menu)
        sprintf(buf, "menu/tire/n%d", n);
    else
        sprintf(buf, "tire/n%d", n);
    return buf;
}

// FUNC 0x0021F4D0 wheel_file_relocate
void wheel_file_relocate(WheelFile2 *w)
{
    WheelFile2 *savedAt = w->base;
    ModelSet *m = w->model;
    w->base = w;
    int delta = (u8 *)w - (u8 *)savedAt;
    do {
        if (m != NULL) {
            w->model = (ModelSet *)((u8 *)m + delta);
            modelset_relocate(w->model);
        }
    } while (0);
}

/* The part of the wheel model to draw: flags bit 0, the rear wheels
 * (positions 2, 3) use part 1. */
// FUNC 0x0021F510 wheel_file_lod
int wheel_file_lod(WheelFile2 *w, int pos)
{
    if (w->flags & 1)
        return pos / 2;
    return 0;
}

/* The model's texture frame and material set together (an inline: retail
 * reloads w->model for each pair and keeps it in s0). */
static inline void wheel_select_variation(ModelSet *m, int v)
{
    modelset_select_tex_frame(m, v);
    modelset_select_material_set(m, v);
}

/* Selects the wheel model's variation (texture frame and material set):
 * none (flags bit 3), the side (bit 1: left / right variations), else the
 * colour. */
// FUNC 0x0021F538 wheel_file_select
void wheel_file_select(WheelFile2 *w, int color, int pos)
{
    /* implicit: modelset_select_tex_frame, modelset_select_material_set (wheel_select_variation) */
    int flags = w->flags;
    if (flags & 8)
        return;
    if (flags & 2)
        wheel_select_variation(w->model, pos & 1);
    else
        wheel_select_variation(w->model, color);
}

// FUNC 0x0021F5B8 wheel_file_variation
int wheel_file_variation(WheelFile2 *w, int color, int pos)
{
    int flags = w->flags;
    if (flags & 8)
        return -1;
    if (flags & 2)
        return pos & 1;
    return color;
}

/* An odd position (one side: ? the right) of a wheel with flags bit 2. */
// FUNC 0x0021F5E8 wheel_file_mirrored
int wheel_file_mirrored(WheelFile2 *w, int pos)
{
    return (pos & 1) && (w->flags & 4);
}

// FUNC 0x0021F608 wheel_file_path
char *wheel_file_path(char *buf, const char *name, int menu)
{
    if (menu)
        sprintf(buf, "menu/wheel/%s", name);
    else
        sprintf(buf, "wheel/%s", name);
    return buf;
}

/* ------------------------------------------------------------------------ */
/* TireModelBase                                                            */
/* ------------------------------------------------------------------------ */

// FUNC 0x0021F660 TireModelBase::TireModelBase
TireModelBase::TireModelBase()
{
    this->tireFile = NULL;
    this->wheelFile = NULL;
    this->color = 0;
    this->position = 0;
    setSize(0.235f, 0.21335f, 0.3f);
}

// FUNC 0x0021F6B0 TireModelBase::~TireModelBase
TireModelBase::~TireModelBase()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr; the deleting form (flags & 1) calls operator delete(this) */
}

/* The corners: per face (x = -w/2, then +w/2) (x,-R,-R) (x,R,-R) (x,-R,R)
 * (x,R,R); R is read back from the object for every store.  The sqrt is a
 * barrier (boot_sqrtf_v: retail computes -w/2 after it); the loop steps by
 * the 12 floats of a face (retail's reversed counter runs 12, 0). */
// FUNC 0x0021F6E0 TireModelBase::setSize
void TireModelBase::setSize(f32 w, f32 rim, f32 aspect)
{
    this->width = w;
    this->rim = rim;
    this->aspect = aspect;
    f32 outerR = w * aspect + rim;
    this->outer = outerR;
    f32 radius = boot_sqrtf_v(outerR * outerR + w * w * 0.25f);
    this->bound = radius;
    this->diameter = radius + radius;
    f32 x = -w * 0.5f;
    for (int i = 0; i < 8 * 3; i += 4 * 3) {
        f32 *b = &this->box[0][0] + i;
        b[0] = x;
        b[1] = -this->outer;
        b[2] = -this->outer;
        b[3] = x;
        b[4] = this->outer;
        b[5] = -this->outer;
        b[6] = x;
        b[7] = -this->outer;
        b[8] = this->outer;
        b[9] = x;
        b[10] = this->outer;
        b[11] = this->outer;
        x += this->width;
    }
}

/* From the outer radius. */
// FUNC 0x0021F7B8 TireModelBase::setSizeOuter
void TireModelBase::setSizeOuter(f32 w, f32 outer, f32 aspect)
{
    setSize(w, outer - w * aspect, aspect);
}

/* From the outer radius and a profile height h, 3/4 of which is the
 * sidewall. */
// FUNC 0x0021F7D8 TireModelBase::setSizeProfile
void TireModelBase::setSizeProfile(f32 w, f32 outer, f32 h)
{
    f32 sidewall = h * 0.75f;
    setSize(w, outer - sidewall, sidewall / w);
}

/* VU0's clip results through the stack (sqc2); w (v[3]) < 0: behind the
 * eye. */
struct ClipQ { s32 v[4]; } __attribute__((aligned(16)));

/* The clip-flag read (vnop; cfc2 $vi18).  Retail has it as an asm sched1
 * moves the w load above (not volatile; ordered after its vclipw by the
 * "m" input of the clip result).  GCC 14 keeps it volatile, so it can never
 * move past the next vclipw. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define CLIP_FLAGS_ASM __asm__
#else
#define CLIP_FLAGS_ASM __asm__ volatile
#endif

/* The view test of the bounding box with VU0: each corner through the
 * combined matrix vf24..vf27 (vu0_load_screen_regs syncs it and loads vf20
 * = vBB0 {0.5, 0.5, 1, 1}), clipped against w (the view frustum) and, with
 * x, y halved, against w again (the guard band: twice the view).  A corner
 * behind the eye (w < 0) has its flags swapped and inverted (+x <-> -x,
 * ...).  All corners outside one plane of the frustum: 0 (culled).  Else 1,
 * clipped = some corner is outside the guard band (it must be clipped).
 * 95.7%: retail's corner pointer runs from `this' (operand 0x1C(p), end
 * this + 0x60), egcs gives this + 0x1C (a variable index expands as
 * &box + i * 12; tried box[i][0], *(box[0] + i * 3), struct casts, a local
 * pointer, two operands); the first w load stays below the flag read (the
 * statement block is a barrier; a w local read before it takes a0 / a1). */
// FUNC 0x0021F810 TireModelBase::clipTest
int TireModelBase::clipTest()
{
    u32 outsideAll = 0x3F;                  /* the frustum planes every corner is outside of */
    u32 outsideAny = 0;                     /* the guard-band planes some corner is outside of */
    vu0_load_screen_regs();
    __asm__ volatile("vnop");
    for (int i = 0; i < 8; i++) {
        ClipQ clip[2];                      /* sp+0x00 vf1, sp+0x10 vf2 */
        u32 flags;
        /* the corner (unaligned: ldl/ldr x, y + lwl/lwr z, pcpyld, qmtc2.ni;
         * its w is not used, vmaddw takes vf0w) through vf24..vf27; vf2 =
         * vf1 * vBB0 (x, y halved); the frustum clip of vf1 */
        __asm__ volatile(
            "ldl          $2, 7+%1\n\t"
            "ldr          $2, %1\n\t"
            "lwl          $3, 11+%1\n\t"
            "lwr          $3, 8+%1\n\t"
            "pcpyld       $3, $3, $2\n\t"
            "qmtc2.ni     $3, $vf1\n\t"
            "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
            "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
            "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
            "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
            "vmul.xyzw    $vf2, $vf1, $vf20\n\t"
            ".word        0x4BE109FF\n\t"   /* vclipw.xyzw $vf1, $vf1w */
            "sqc2         $vf1, %0\n\t"
            "vnop\n\t"
            "vnop\n\t"
            "vnop"
            : "=m"(clip[0]) : "m"(this->box[i][0]) : "$2", "$3");
        CLIP_FLAGS_ASM("vnop\n\t"
            "cfc2.ni      %0, $vi18"
            : "=r"(flags) : "m"(clip[0]));
        u32 f;
        do {                                /* ? a macro: retail aligns the label after the if */
            if (clip[0].v[3] < 0)
                flags = ((flags >> 1) | 0x2A) ^ ((flags << 1) | 0x15);
            f = flags & 0x3F;
        } while (0);
        outsideAll &= f;
        /* the guard-band clip of vf2 */
        __asm__ volatile(
            ".word        0x4BE211FF\n\t"   /* vclipw.xyzw $vf2, $vf2w */
            "sqc2         $vf2, %0\n\t"
            "vnop\n\t"
            "vnop\n\t"
            "vnop"
            : "=m"(clip[1]));
        CLIP_FLAGS_ASM("vnop\n\t"
            "cfc2.ni      %0, $vi18"
            : "=r"(flags) : "m"(clip[1]));
        if (clip[1].v[3] < 0)
            flags = ((flags >> 1) | 0x2A) ^ ((flags << 1) | 0x15);
        u32 g = flags & 0x3F;
        outsideAny |= g;
    }
    if (outsideAll != 0)
        return 0;
    if (outsideAny == 0)
        this->clipped = 0;
    else
        this->clipped = 1;
    return 1;
}

/* The origin (T447 .rodata, after the file names, before the vtable). */
const f32 g_00340CA0[4] __attribute__((aligned(16))) = {   /* 0x00340CA0 */
    0.0f, 0.0f, 0.0f, 1.0f,
};

/* The view test; faceVisible = the eye (the origin through the inverse of
 * the current matrix: in the tyre's space) is beyond the outer face
 * (x < -w/2). */
// FUNC 0x0021F940 TireModelBase::virtual_1
int TireModelBase::virtual_1()
{
    if (!clipTest())
        return 0;
    f32 eye[4];                             /* sp+0x00 */
    pgl_push_matrix();
    vu0_invert_rigid();
    vu0_apply_matrix(eye, g_00340CA0);
    pgl_pop_matrix();
    this->faceVisible = (0.0f < -(this->width * 0.5f) - eye[0]) ? 1 : 0;
    return 1;
}

// FUNC 0x0021F9D0 TireModelBase::setVisible
void TireModelBase::setVisible()
{
    this->clipped = 1;
    this->faceVisible = 1;
}

// FUNC 0x0021F9E0 TireModelBase::drawWheel
void TireModelBase::drawWheel()
{
    wheel_file_select(this->wheelFile, this->color, this->position);
    WheelFile2 *w = this->wheelFile;
    modelset_draw_part(w->model, wheel_file_lod(w, this->position));
}

/* TireFile2::isF1, written out. */
// FUNC 0x0021FA38 TireModelBase::isF1Tire(void)
int TireModelBase::isF1Tire()
{
    return (this->tireFile->flags >> 1) & 1;
}

// FUNC 0x0021FA50 TireModelBase::getWheelTexLevelBias(void)
int TireModelBase::getWheelTexLevelBias()
{
    return wheel_file_lod(this->wheelFile, this->position) * 3;
}

// FUNC 0x0021FA78 TireModelBase::getWheelVariation
int TireModelBase::getWheelVariation()
{
    return wheel_file_variation(this->wheelFile, this->color, this->position);
}

// FUNC 0x0021FAA0 TireModelBase::setColor
/* inline in game/car/model/T447_tire_model_base.h; egcs emits this copy */

// FUNC 0x0021FAA8 TireModelBase::setPosition(int)
/* inline in game/car/model/T447_tire_model_base.h; egcs emits this copy */

// FUNC 0x0021FAB0 TireModelBase::setWheelFile(WheelFile2*)
/* inline in game/car/model/T447_tire_model_base.h; egcs emits this copy */

// FUNC 0x0021FAB8 TireModelBase::setTireFile(TireFile2*)
/* inline in game/car/model/T447_tire_model_base.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __user_type_info
 * __ti13TireModelBase (0x00353440, name 0x00340CC8) on the first call. */
// FUNC 0x0021FAC0 __tf13TireModelBase
extern "C" void *__tf13TireModelBase(void)
{
    if (__ti13TireModelBase[0] == 0)
        __rtti_user(__ti13TireModelBase, "13TireModelBase");
    return __ti13TireModelBase;
}
#endif
