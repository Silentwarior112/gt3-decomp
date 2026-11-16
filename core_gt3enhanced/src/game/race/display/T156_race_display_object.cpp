/* T156_race_display_object.cpp - RaceDisplayObjectBase, the base class of
 * the race display elements, and helpers of the race display
 * (0x00196490..0x00196910).
 *
 * An object has an image pack and one image of it (setImage looks a name
 * up), a rectangle (location / size, stored as 16-bit), an alignment per
 * axis (default RACE_ALIGN_CENTER) and an enabled flag (default off).  The
 * race display (RaceDisplay::virtual_3) sets the 2D render state
 * (race_display_setup_2d) and calls the draw slot (draw, or the
 * meters' drawMeter with its GSBuffer) of every enabled object.
 */
#include "game/race/display/race_display_object_internal.h"

/* ======================================================================== */
/* RaceDisplayObjectBase: construction                                      */
/* ======================================================================== */

// FUNC 0x00196490 RaceDisplayObjectBase::RaceDisplayObjectBase
//   19 callers (the derived constructors).  setLocation / setSize are
//   virtual calls (the first through the just-stored vtable 0x0032E798, the
//   second through a reloaded vptr): the inline setRegion, integrated here
//   (egcs calls a constructor's own virtuals directly, but inside the
//   integrated body `this' is setRegion's).
RaceDisplayObjectBase::RaceDisplayObjectBase()
{
    imagePack = 0;
    image = 0;
    setRegion(0, 0, 0, 0);
    alignX = RACE_ALIGN_CENTER;
    alignY = RACE_ALIGN_CENTER;
    enabled = false;
}

// FUNC 0x00196510 RaceDisplayObjectBase::~RaceDisplayObjectBase
//   Restores the vptr and calls virtual_2 DIRECTLY (jal 0x00196568, not
//   through the vtable); the deleting form (flags & 1) then calls
//   operator delete.  41 callers (the derived destructors).
RaceDisplayObjectBase::~RaceDisplayObjectBase()
{
    RaceDisplayObjectBase::virtual_2();
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x00196560 RaceDisplayObjectBase::virtual_1
//   No class overrides it.
int RaceDisplayObjectBase::virtual_1()
{
    return 0;
}

// FUNC 0x00196568 RaceDisplayObjectBase::virtual_2
//   No class overrides it; the destructor calls it.
void RaceDisplayObjectBase::virtual_2()
{
}

// FUNC 0x00196570 RaceDisplayObjectBase::update
//   The per-frame update (RaceDisplay::virtual_2); empty in the base.
void RaceDisplayObjectBase::update()
{
}

// FUNC 0x00196578 RaceDisplayObjectBase::draw
//   The draw method; empty in the base.
void RaceDisplayObjectBase::draw()
{
}

// FUNC 0x00196580 RaceDisplayObjectBase::drawMeter
//   The meters' draw.  Ignores the buffer ($a1 is overwritten by the
//   function pointer) and calls draw through the vtable.
void RaceDisplayObjectBase::drawMeter(GSBuffer &buf)
{
    draw();
}

// FUNC 0x001965A8 RaceDisplayObjectBase::setImage
//   image = the named image of the pack (image_pack_find: else
//   "00no_texture", else NULL), when there is a pack.  Callers:
//   RaceDisplay::virtual_0 ("boost"), RaceBackPanel::setPanel ("panel_%d"),
//   RaceRemoteTachometer::virtual_12, RaceRoundTachometerBase::setImage,
//   RaceOnboardSpeedmeter::setupScale.
void RaceDisplayObjectBase::setImage(const char *name)
{
    if (imagePack != 0)
        image = image_pack_find(imagePack, name);
}

/* ======================================================================== */
/* Race display helpers                                                     */
/* ======================================================================== */

// FUNC 0x001965E0 race_display_set_ortho
//   No callers.  The projection of a 2D screen rectangle:
//   pgl_ortho(left, right, bottom, top, -1, 1), the integers converted to
//   float (from $a0, $a2, $a3, $a1).
void race_display_set_ortho(int left, int top, int right, int bottom)
{
    pgl_ortho((f32)left, (f32)right, (f32)bottom, (f32)top, -1.0f, 1.0f);
}

// FUNC 0x00196630 race_display_setup_2d
//   The render state of the race display's 2D drawing: opaque white
//   (0x80FFFFFF: GS alpha 0x80 = 1.0) for untextured primitives, blending
//   on, fog / depth test / alpha test / destination alpha test off, the
//   usual alpha blend (Cs - Cd) * As + Cd, linear texture filtering, the
//   system font left aligned at size 1.  Caller: RaceDisplay::virtual_3
//   (twice).
void race_display_setup_2d(void)
{
    pgl_color(0x80FFFFFF);
    gfx_enable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_FOG);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 0.0f);  /* 0x44 */
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    g_002F1968.setAlign(0);                 /* G001 font bits: left, default vertical */
    g_002F1968.setSize(1.0f, 1.0f);
}

// FUNC 0x001966D0 race_align_shift_x
//   No callers.  The argument is an alignment value, not a display: the x
//   shift race_align_posf applies to it on the wide screen (g_002F424C):
//   RACE_ALIGN_WIDE | LEFT (4) -RACE_WIDE_SHIFT, RACE_ALIGN_WIDE | RIGHT
//   (5) +RACE_WIDE_SHIFT; 0 for any other value and when the mode is off.
//   The whole value is compared (not align & 3).
f32 race_align_shift_x(int align)
{
    f32 shift = 0.0f;

    if (g_002F424C != 0)
        shift = RACE_WIDE_SHIFT;
    if (align == (RACE_ALIGN_WIDE | RACE_ALIGN_LEFT))
        return -shift;
    if (align == (RACE_ALIGN_WIDE | RACE_ALIGN_RIGHT))
        return shift;
    return 0.0f;
}

// FUNC 0x00196710 race_display_align
//   No callers.  A wrapper of T158's race_align_pos (the arguments and the
//   result pass through unchanged).
int race_display_align(int pos, int size, int align)
{
    return race_align_pos(pos, size, align);
}

// FUNC 0x00196730 race_display_alignf
//   No callers.  A wrapper of T158's race_align_posf ($a0, $f12, $f13 and
//   $f0 pass through unchanged).
f32 race_display_alignf(int align, f32 pos, f32 size)
{
    return race_align_posf(pos, size, align);
}

/* ======================================================================== */
/* RaceSplitDisplayBase                                                     */
/* ======================================================================== */

// FUNC 0x00196750 RaceSplitDisplayBase::setTargetDisplay
//   No callers.
void RaceSplitDisplayBase::setTargetDisplay(int display)
{
    targetDisplay = display;
}

/* ======================================================================== */
/* RaceDisplayObjectBase: accessors                                         */
/* ======================================================================== */

// FUNC 0x00196758 RaceDisplayObjectBase::getImagePack
ImagePack *RaceDisplayObjectBase::getImagePack() const
{
    return imagePack;
}

// FUNC 0x00196760 RaceDisplayObjectBase::setImagePack
void RaceDisplayObjectBase::setImagePack(ImagePack *pack)
{
    imagePack = pack;
}

// FUNC 0x00196768 RaceDisplayObjectBase::getAlignY
u8 RaceDisplayObjectBase::getAlignY() const
{
    return alignY;
}

// FUNC 0x00196770 RaceDisplayObjectBase::getAlignX
u8 RaceDisplayObjectBase::getAlignX() const
{
    return alignX;
}

// FUNC 0x00196778 RaceDisplayObjectBase::getHeight
int RaceDisplayObjectBase::getHeight() const
{
    return h;
}

// FUNC 0x00196780 RaceDisplayObjectBase::getWidth
int RaceDisplayObjectBase::getWidth() const
{
    return w;
}

// FUNC 0x00196788 RaceDisplayObjectBase::getY
int RaceDisplayObjectBase::getY() const
{
    return y;
}

// FUNC 0x00196790 RaceDisplayObjectBase::getX
int RaceDisplayObjectBase::getX() const
{
    return x;
}

// FUNC 0x00196798 RaceDisplayObjectBase::isEnabled
//   Callers: RaceDisplay::virtual_3, drawIfEnabled (both).  Returns the
//   flag as it is (lbu, no sltu: `enabled' is a bool bit-field, whose value
//   needs no normalization; a u8 field returned as bool would add sltu).
bool RaceDisplayObjectBase::isEnabled() const
{
    return enabled;
}

// FUNC 0x001967A0 RaceDisplayObjectBase::setEnabled
//   No callers (the users store enabled themselves).
void RaceDisplayObjectBase::setEnabled(bool on)
{
    enabled = on;
}

/* ======================================================================== */
/* RaceDisplayObjectBase: the rectangle and the alignment                   */
/* ======================================================================== */

// FUNC 0x001967A8 RaceDisplayObjectBase::setRegion (setRegion__21RaceDisplayObjectBaseiiii)
//   Inline in game/race/display/T156_race_display_object.h; egcs emits this copy (the
//   vtable's): setLocation(x, y) then setSize(w, h), both virtual calls
//   (`this' is a parameter here).

// FUNC 0x00196810 RaceDisplayObjectBase::setAlignment (setAlignment__21RaceDisplayObjectBaseUcUc)
// FUNC 0x00196820 RaceDisplayObjectBase::setSize (setSize__21RaceDisplayObjectBaseii)
// FUNC 0x00196830 RaceDisplayObjectBase::setLocation (setLocation__21RaceDisplayObjectBaseii)
//   Inline in game/race/display/T156_race_display_object.h; egcs emits these copies (the
//   vtable's).  Written x then y (w then h): egcs emits the second store
//   first and the first one in the jr's delay slot, as retail; the sizes
//   are stored as 16-bit (sh).

// FUNC 0x00196840 RaceDisplayObjectBase::drawIfEnabled(GSBuffer &) (drawIfEnabled__21RaceDisplayObjectBaseR8GSBuffer)
/* implicit: isEnabled (the inline body in the header) */
// FUNC 0x00196890 RaceDisplayObjectBase::drawIfEnabled() (drawIfEnabled__21RaceDisplayObjectBase)
/* implicit: isEnabled (the inline body in the header) */
//   Inline in game/race/display/T156_race_display_object.h, declared before the setters;
//   egcs emits these copies after setLocation's (u053 data pass: retail's
//   function order).  No callers (RaceDisplay::virtual_3 has them
//   expanded): drawMeter(buf) / draw() when the object is enabled.

/* ======================================================================== */
/* RTTI                                                                     */
/* ======================================================================== */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001968D0 __tf21RaceDisplayObjectBase
//   Builds the __user_type_info __ti21RaceDisplayObjectBase (0x003533A0,
//   gp-0x4650; name 0x0032E7F0, no bases) on the first call.  21 callers:
//   the derived classes' __tf functions.
extern "C" const type_info &__tf21RaceDisplayObjectBase(void)
{
    if (__ti21RaceDisplayObjectBase[0] == 0)
        __rtti_user(__ti21RaceDisplayObjectBase, "21RaceDisplayObjectBase");
    return *(const type_info *)__ti21RaceDisplayObjectBase;
}
#endif
