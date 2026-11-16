#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/menu/framework/T238_slider.cpp"
#pragma implementation "src/game/menu/framework/T238_slider.cpp"
#pragma implementation "T238_slider.h"
#endif
/* T238 - gt2k_menu::Slider (0x001C6410..0x001C6D58).
 *
 * A Button of a ButtonList with a knob ("pinch") on a value range: the
 * SliderDesc the page's slider function filled (min / max / value / step,
 * the inner range limitMin..limitMax and the setter).  Left / right move the
 * value by step (mode 0 also: L1 / R1 by 10 steps); every move is handed to
 * desc.set(ctx, value, 0), which in mode 0 may refuse it (the value goes
 * back).  "pinch_prev" marks the initial value while the knob is elsewhere,
 * "pinch_fix" replaces the knob when the range has a single value.
 *
 * Data (retail .rodata 0x003367A0..0x003369D6): "pinch_prev", "pinch_fix",
 * Slider's vtable (the Cell<Button> part first), then the type names Slider,
 * SmartIndexT<int>, List<Button>, Cell<Button>, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>, Operator: the
 * original read the Button header (Item, Cell<Button>) and ButtonList's
 * (List<Button>, SmartIndexT<int>) before Slider's.  .sdata
 * 0x003526B0..0x003526C8: g_003526B0, "pinch", Slider's base list.
 */
#include "game/menu/framework/T206_button.h"               /* (data layout only) gt2k_menu::Button: Cell<Button> */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail 0x003368B0 / 0x00336890). */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList (index: SmartIndexT<int>) */
#include "game/menu/framework/T238_slider.h"
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/menu_links_internal.h"                           /* (T229: PageLayout::findHiddenItem, getTexture) */

/* (gp-0x5340, initially 0) 1: no Slider draws its "pinch_prev" picture; see
 * game/menu/framework/T238_slider.h.  Defined before the first function: retail has it
 * first in T238's .sdata, before the constructor's "pinch". */
int g_003526B0 = 0;                         /* 0x003526B0 */

namespace gt2k_menu {

/* The clamped value at desc.value inside [min, max) and the inner range; the
 * knob pictures from the layout's "hidden" layer. */
// FUNC 0x001C6410 gt2k_menu::Slider::Slider
Slider::Slider(ButtonList *owner, int controlType, PageLayout *layout, f32 offPoint,
               ButtonData *data, MenuLayoutItem *elem, int relative, int depth, void *ctx_,
               const SliderDesc &desc_)
    : Button(owner, controlType, layout, offPoint, data, elem, relative, depth),
      ctx(ctx_), pinch(), pinchPrev(), pinchFix(), desc(desc_), showPrev(1), index() /* implicit: Item, SmartIndexDoubleRange (member constructors), ~Item (exception cleanup) */
{
    /* Button(...), both vptrs; the three Items (Item::Item(NULL, NULL)), the
     * record copied, SmartIndexDoubleRange::SmartIndexDoubleRange() of index.
     * On a throw: Item::~Item of the Items built, then Button::~Button. */
    index.setClamp(true);                   /* through &index (retail sw 0xC(s0)) */
    initial = desc.value;
    range = desc.max - desc.min;
    index.setRange(desc.min, desc.max);
    index.set(initial);
    index.setInnerRange(desc.limitMin, desc.limitMax);

    MenuLayoutItem *picture;
    picture = layout->findHiddenItem("pinch");
    if (picture != 0)
        pinch.setResource(picture, layout->getTexture(picture));
    picture = layout->findHiddenItem("pinch_prev");
    if (picture != 0)
        pinchPrev.setResource(picture, layout->getTexture(picture));
    picture = layout->findHiddenItem("pinch_fix");
    if (picture != 0)
        pinchFix.setResource(picture, layout->getTexture(picture));

    g_003526B0 = 0;
}

/* Moves to `value` (when the range has 2 values or more) and commits it:
 * desc.set(ctx, value, 1).  Caller setting_gear_apply_auto_level (T082). */
// FUNC 0x001C6690 gt2k_menu::Slider::setValue
void Slider::setValue(int value)
{
    if (isMovable()) {                      /* implicit: size (the inline isMovable) */
        index.set(value);
        desc.set(ctx, index.value, 1);
    }
}

/* New range min..max and inner range lmin..lmax (when the range has 2
 * values or more).  Caller setting_gear_apply_auto_level (T082). */
// FUNC 0x001C6700 gt2k_menu::Slider::setRange
void Slider::setRange(int min, int max, int lmin, int lmax)
{
    if (isMovable()) {                      /* implicit: size (the inline isMovable) */
        index.setRange(min, max);
        index.setInnerRange(lmin, lmax);
        range = max - min;
    }
}

/* Input by desc.mode; 1: the list handles it (up / down, an unknown mode). */
// FUNC 0x001C6798 gt2k_menu::Slider::virtual_11
int Slider::virtual_11(const MenuControl &ctl)
{
    switch (desc.mode) {
    case SLIDER_MODE_IMMEDIATE:
        return inputImmediate(ctl);
    case SLIDER_MODE_STEP:
        return inputStep(ctl);
    }
    return 1;
}

/* One move of inputImmediate by delta: a move that desc.set refuses
 * (returns 0) is undone, and the old value set again.  Always 0.  (? an
 * inline helper of the original: retail has the move / set sequence once
 * per button, partly cross-jumped.) */
static inline int moveImmediate(Slider *s, int delta)
{
    SmartIndexDoubleRange *idx = &s->index;
    int oldValue = s->index.value;
    if (idx->SmartIndexDoubleRange::move(delta) != 0) {
        if (s->desc.set(s->ctx, s->index.value, 0) != 0) {
            menu_play_se(MENU_SE_VALUE);
        } else {
            idx->set(oldValue);
            s->desc.set(s->ctx, s->index.value, 0);
            menu_play_se(MENU_SE_REFUSED);
        }
    } else {
        menu_play_se(MENU_SE_REFUSED);
    }
    return 0;
}

/* Mode 0: left / right = -/+ step, L1 / R1 = -/+ 10 steps (moveImmediate);
 * 1 for up / down (the list handles them). */
// FUNC 0x001C67E0 gt2k_menu::Slider::inputImmediate
int Slider::inputImmediate(const MenuControl &ctl)
{
    int result = 0;
    if (ctl.up != 0 || ctl.down != 0)
        result = 1;
    else if (ctl.left != 0)
        result = moveImmediate(this, -desc.step);
    else if (ctl.right != 0)
        result = moveImmediate(this, desc.step);
    else if (ctl.l1 != 0)
        result = moveImmediate(this, -desc.step * 10);
    else if (ctl.r1 != 0)
        result = moveImmediate(this, desc.step * 10);
    return result;
}

/* One move of inputStep by delta; desc.set only when the range has 2 values
 * or more.  Always 0.  (? an inline helper, as moveImmediate: retail has the
 * move call once per button.) */
static inline int moveStep(Slider *s, int delta)
{
    if (s->index.SmartIndexDoubleRange::move(delta) != 0) {
        menu_play_se(MENU_SE_VALUE);
        if (s->isMovable())
            s->desc.set(s->ctx, s->index.value, 0);
    } else {
        menu_play_se(MENU_SE_REFUSED);
    }
    return 0;
}

/* Mode 1: left / right = -/+ step (moveStep); 1 otherwise (the list handles
 * the input).  Not exact (96.2%): retail's branch on `right' is a beql whose
 * annulled slot holds a dead load of ctl.decide (0xE8): the code of a
 * decide test that jump2 emptied (? `else if (ctl.decide) ...'); every
 * decide arm tried (result = 1 / 0, return 1, empty, with result
 * uninitialised and `else result = 1') is folded before reload or changes
 * the rest. */
// FUNC 0x001C6958 gt2k_menu::Slider::inputStep
int Slider::inputStep(const MenuControl &ctl)
{
    int result = 1;
    if (ctl.up != 0 || ctl.down != 0)
        result = 1;
    else if (ctl.left != 0)
        result = moveStep(this, -desc.step); /* implicit: move, menu_play_se, size (the inline moveStep) */
    else if (ctl.right != 0)
        result = moveStep(this, desc.step);
    return result;
}

/* Back to the initial value (not committed). */
// FUNC 0x001C6A30 gt2k_menu::Slider::virtual_8
void Slider::virtual_8()
{
    index.set(initial);
}

// FUNC 0x001C6A50 gt2k_menu::Slider::virtual_1
void Slider::virtual_1()
{
    Button::virtual_1();
}

/* The knob's centre for value number `valueIndex` of `count`: the knob
 * (knobW wide) slides inside the w-wide bar centred on x; x itself when
 * count < 2.  Also used by SliderItem::virtual_2 (0x001C20F0, T228). */
// FUNC 0x001C6A70 gt2k_menu::Slider::knobX
f32 Slider::knobX(int valueIndex, int knobW, int count, f32 x, f32 w)
{
    if (count < 2)
        return x;
    /* Not exact (95.8%): retail adds left + offset into the left's register
     * ($f1) and only the last sum into $f0; with `left + offset' first in
     * the source the registers of the two chains swap (87.5%).  Tried: every
     * order of the three terms, locals for each (all declaration orders),
     * in-place sums, / 2 for * 0.5; the reference compiler gives the same. */
    f32 knobWidth = (f32)knobW;
    return ((w - knobWidth) / (f32)(count - 1)) * (f32)valueIndex + (x - w * 0.5f)
           + knobWidth * 0.5f;
}

/* Button's draw; "pinch_prev" at the initial value (mode 0, away from it,
 * unless hidden); then the knob ("pinch_fix" for a single-value range) at
 * the value, through its virtual draw.  Retail reads index.begin, x and y
 * through their addresses ($s4, $s2, $s3: the pointer / the MotionValue
 * accessors) and the knobs' w through the Item (getW).
 * Not exact (90.9%): retail computes &index after Button's draw (in the
 * delay slot of the mode test), sched1 here moves the local's addiu above
 * the call, and &x / &y follow one branch later.  Tried: getIndex() at
 * each use (PRE then recomputes the addresses in the then-part, 85%), a
 * local after the if (70%), (&index)->begin, getDX() / getX() forms, the
 * size test as isMovable() or size() < 2 (54 combinations). */
// FUNC 0x001C6AD0 gt2k_menu::Slider::virtual_2
void Slider::virtual_2()
{
    Button::virtual_2();
    SmartIndexDoubleRange *idx = &index;

    if (desc.mode == SLIDER_MODE_IMMEDIATE && index.value != initial && showPrev != 0 &&
        g_003526B0 == 0) {
        f32 prevX = knobX(initial - idx->begin, (int)pinchPrev.getW(), range, x.get(), w);
        pinchPrev.setX(prevX);
        pinchPrev.setY(y.get());
        pinchPrev.Item::virtual_2();
    }

    Item *knob = isMovable() ? &pinch : &pinchFix;  /* implicit: size (the inline isMovable) */
    f32 knobPos = knobX(index.value - idx->begin, (int)knob->getW(), range, x.get(), w);
    knob->setX(knobPos);
    knob->setY(y.get());
    knob->virtual_2();
}

/* (no callers) */
// FUNC 0x001C6C28 gt2k_menu::Slider::isMovable
/* inline in game/menu/framework/T238_slider.h; egcs emits this copy (#pragma implementation) */
/* implicit: size (the inline body) */

/* (no callers) */
// FUNC 0x001C6C50 gt2k_menu::Slider::getIndex
/* inline in game/menu/framework/T238_slider.h; egcs emits this copy (#pragma implementation) */

/* (no callers) */
// FUNC 0x001C6C58 gt2k_menu::Slider::getValue
/* inline in game/menu/framework/T238_slider.h; egcs emits this copy (#pragma implementation) */

/* (no callers) */
// FUNC 0x001C6C60 gt2k_menu::Slider::setShowPrev
/* inline in game/menu/framework/T238_slider.h; egcs emits this copy (#pragma implementation) */

/* (no callers; static: no `this`) Turns pinch_prev off for every slider until
 * the next Slider is built. */
// FUNC 0x001C6C68 gt2k_menu::Slider::hidePrev
/* inline in game/menu/framework/T238_slider.h; egcs emits this copy (#pragma implementation) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one base:
 * Button, public, +0). */
// FUNC 0x001C6C78 __tfQ29gt2k_menu6Slider
extern "C" void *__tfQ29gt2k_menu6Slider(void)
{
    if (__tiQ29gt2k_menu6Slider[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu6Slider, "Q29gt2k_menu6Slider", g_003526C0, 1);
    }
    return __tiQ29gt2k_menu6Slider;
}
#endif

namespace gt2k_menu {

// FUNC 0x001C6CD0 gt2k_menu::Slider::~Slider
/* implicit (game/menu/framework/T238_slider.h); egcs emits this copy after __tf.  Both
 * vptrs; the members pinchFix, pinchPrev, pinch (Item::~Item, flags 2), then
 * Button::~Button(0); Object::operator delete if flags & 1. */
/* implicit: ~Item (member destructors), ~Button (base destructor), operator delete (deleting destructor) */

} /* namespace gt2k_menu */
