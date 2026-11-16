/* T035 - gt2k_menu::SelectBox.  0x001095A8..0x00109F20.
 *
 * The colour chip list of the arcade car select screen (created by
 * ArcadeControl::createColorChips, T023).  One chip per entry of a
 * SmartIndexT<int> (the colour index): a filled quad in the chip's colour
 * with a one pixel frame in the box's own colour, the list scrolled so that
 * the current entry sits at the box's position (scroll.value eases towards
 * the index at rate 0.3).  The fills are clipped to the current row, so only
 * the current chip shows its colour; the frames to a 7 row window (1.5 rows
 * above the current one to 5.5 below).  arrow_up / arrow_down are drawn
 * while a clamping index can still move up / down. */
#define U004_T035                           /* arcade_car_select_internal.h: this unit's declarations */
#include "game/menu/arcade/arcade_car_select_internal.h"
#include "game/menu/arcade/T035_select_box.h"

namespace gt2k_menu {

/* The colour of the chips past the index's size. */
static const u32 SELECT_BOX_NO_COLOR = 0x08000000;

/* A MotionValue's value alone, velocity and force kept (? an inline member
 * of the original, MotionValue::setValue): retail stores it through the
 * inline's `this', &scroll, kept in $s4 from the member's construction (a
 * plain scroll.value store folds into this + 0x20C).  Not added to T260's
 * header: a new inline member there renumbers egcs's labels in every
 * includer, which changed one exception table (u030's T074). */
static inline void motionSetValue(MotionValue *mv, f32 v)
{
    mv->value = v;
}

/* Item(image, layout) (Object::operator new by the caller), the members in
 * order: scroll 0 (MotionValue()), spacing 2, the two arrows; then frozen 0
 * and both arrows hidden until the first update.  The EH landing pads
 * destroy what was built (~Item of arrowDown / arrowUp / the base). */
// FUNC 0x001095A8 gt2k_menu::SelectBox::SelectBox
SelectBox::SelectBox(const char *image, SmartIndexT<int> *idx, const u32 *chipColors,
                     PageLayout *layout)
    : Item(image, layout), index(idx), scroll(), spacing(2.0f),
      arrowUp("arrow_up", layout), arrowDown("arrow_down", layout),
      frozen(0), atTop(1), atBottom(1)
{
    int i;

    for (i = 0; i < 64; i++) {
        if (i < index->size())
            colors[i] = chipColors[i];
        else
            colors[i] = SELECT_BOX_NO_COLOR;
    }
    motionSetValue(&scroll, (f32)index->value);
}

/* Update: Item's, the scroll, the arrows' updates (direct calls on the
 * member Items); unless frozen, atTop / atBottom from a clamping index at
 * its ends. */
// FUNC 0x00109748 gt2k_menu::SelectBox::virtual_1
void SelectBox::virtual_1()
{
    Item::virtual_1();
    scroll.approach((f32)index->value, 0.3f);
    arrowUp.virtual_1();
    arrowDown.virtual_1();
    if (frozen == 0) {
        SmartIndexT<int> *idx = index;
        atTop    = (idx->clamp == 1 && idx->value <= idx->begin);
        atBottom = (idx->clamp == 1 && idx->value + 1 >= idx->end);
    }
}

/* Draw.  Row i is at y - (h + spacing) * (scroll - i).  The chip colours are
 * RGBA8 with the alpha byte forced to 0x80, unpacked (the inline Color4f(u32))
 * and packed again with this box's alphaRatio. */
// FUNC 0x001097F0 gt2k_menu::SelectBox::virtual_2
void SelectBox::virtual_2()
{
    int i;
    int count;
    f32 screenX, screenY, top, rowH;

    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 1.0f);
    count = index->size();
    screenX = getScreenX();
    screenY = getScreenY();
    top     = screenY - (h + spacing) * scroll.value;

    /* the fills, clipped to the current row */
    menu_clip_rect(320.0f, screenY, 640.0f, h + 2.0f);
    for (i = 0; i < count; i++) {
        f32 rowY = top + (f32)i * (h + spacing);
        PrimVertex *quad = (PrimVertex *)gfx_alloc_tail(3);     /* 4 vertices */

        makeChipQuad(quad, screenX, rowY);
        Color4f chip((colors[i] & 0xFFFFFF) | 0x80000000);
        pgl_color(chip.toRGBA8(alphaRatio));
        pgl_draw_strip(quad, 0, 0, 0, 4);
    }

    /* the frames, clipped to 7 rows centred 2 rows below the current one */
    rowH = h + spacing;
    menu_clip_rect(320.0f, screenY + (rowH + rowH), 640.0f, rowH * 7.0f);
    for (i = 0; i < count; i++) {
        f32 rowY = top + (f32)i * (h + spacing);
        PrimVertex *frame = (PrimVertex *)gfx_alloc_tail(8);    /* 10 vertices */

        makeChipFrame(frame, screenX, rowY);
        pgl_color(color[0].toRGBA8(alphaRatio));
        pgl_draw_strip(frame, 0, 0, 0, 10);
    }
    menu_clip_full();

    if (atTop == 0) {
        arrowUp.setX(screenX);
        arrowUp.setY(screenY - h - spacing);
        arrowUp.setAlphaRatio(alphaRatio);
        arrowUp.virtual_2();
    }
    if (atBottom == 0) {
        arrowDown.setX(screenX);
        arrowDown.setY(screenY + h + spacing);
        arrowDown.setAlphaRatio(alphaRatio);
        arrowDown.virtual_2();
    }
}

/* The chip: a w x h rectangle around (px, py) as a 4 vertex triangle
 * strip. */
// FUNC 0x00109C18 gt2k_menu::SelectBox::makeChipQuad
void SelectBox::makeChipQuad(PrimVertex *v, f32 px, f32 py)
{
    v[0].x = px - w * 0.5f;
    v[0].y = py - h * 0.5f;
    v[0].z = 0.0f;
    v[1].x = px + w * 0.5f;
    v[1].y = py - h * 0.5f;
    v[1].z = 0.0f;
    v[2].x = px - w * 0.5f;
    v[2].y = py + h * 0.5f;
    v[2].z = 0.0f;
    v[3].x = px + w * 0.5f;
    v[3].y = py + h * 0.5f;
    v[3].z = 0.0f;
}

/* The chip's frame: the same rectangle's outline, one pixel wide, as a
 * 10 vertex triangle strip (outer / inner corner pairs, back to the start). */
// FUNC 0x00109CB8 gt2k_menu::SelectBox::makeChipFrame
void SelectBox::makeChipFrame(PrimVertex *v, f32 px, f32 py)
{
    v[0].x = px - w * 0.5f;
    v[0].y = py - h * 0.5f;
    v[0].z = 0.0f;
    v[1].x = (px - w * 0.5f) + 1.0f;
    v[1].y = (py - h * 0.5f) + 1.0f;
    v[1].z = 0.0f;
    v[2].x = px + w * 0.5f;
    v[2].y = py - h * 0.5f;
    v[2].z = 0.0f;
    v[3].x = (px + w * 0.5f) - 1.0f;
    v[3].y = (py - h * 0.5f) + 1.0f;
    v[3].z = 0.0f;
    v[4].x = px + w * 0.5f;
    v[4].y = py + h * 0.5f;
    v[4].z = 0.0f;
    v[5].x = (px + w * 0.5f) - 1.0f;
    v[5].y = (py + h * 0.5f) - 1.0f;
    v[5].z = 0.0f;
    v[6].x = px - w * 0.5f;
    v[6].y = py + h * 0.5f;
    v[6].z = 0.0f;
    v[7].x = (px - w * 0.5f) + 1.0f;
    v[7].y = (py + h * 0.5f) - 1.0f;
    v[7].z = 0.0f;
    v[8].x = px - w * 0.5f;
    v[8].y = py - h * 0.5f;
    v[8].z = 0.0f;
    v[9].x = (px - w * 0.5f) + 1.0f;
    v[9].y = (py - h * 0.5f) + 1.0f;
    v[9].z = 0.0f;
}

/* No callers: the out-of-line copy of an inline accessor. */
// FUNC 0x00109E60 gt2k_menu::SelectBox::setFrozen
void SelectBox::setFrozen(bool f)
{
    frozen = f;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00109E68 __tfQ29gt2k_menu9SelectBox
extern "C" void *__tfQ29gt2k_menu9SelectBox(void)
{
    if (__tiQ29gt2k_menu9SelectBox[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu9SelectBox, "Q29gt2k_menu9SelectBox",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu9SelectBox;
}
#endif

// FUNC 0x00109EB8 gt2k_menu::SelectBox::~SelectBox
/* implicit in game/menu/arcade/T035_select_box.h; egcs emits this copy (slot [0]):
 * Item::~Item of arrowDown and arrowUp (flags 2), Item::~Item(this, 0),
 * then Object::operator delete with flags & 1; no vptr store. */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
