/* T238_slider.h - gt2k_menu::Slider and its value range
 * gt2k_menu::SliderDesc (translation unit T238, unit u069),
 * 0x001C6410..0x001C6D58.
 *
 * A Button of a ButtonList with a knob ("pinch") on a value range: the
 * SliderDesc the page's slider table returned for the element
 * (PageLayout::findSliderFunc, T229).  Left / right move the value by step
 * (mode 0 also: L1 / R1 by 10 steps); every move goes to desc.set(ctx, value,
 * 0), which in mode 0 may refuse it (the value goes back).  "pinch_prev"
 * marks the initial value while the knob is elsewhere, "pinch_fix" replaces
 * the knob when the range has a single value.  Built by
 * ButtonList::createSlider (T244, operator new(0x4B0)) for the elements
 * named "slider...".  Slider::hidePrev() sets g_003526B0 (gp-0x5340, T238
 * .sdata, below). */
#ifndef GAME_T238_SLIDER_H
#define GAME_T238_SLIDER_H

#include "game/game.h"
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (pinch, pinchPrev, pinchFix) */
#include "game/common/T266_smart_index_double_range.h" /* gt2k_menu::SmartIndexDoubleRange (index) */

namespace gt2k_menu {
class ButtonList;                           /* T244 (pointers only) */
class PageLayout;                           /* T229 */
class MenuControl;                          /* T214 */
struct ButtonData;                          /* T244 */
struct MenuLayoutItem;                      /* T229 (T235: MenuResource) */

/* Stores a new value: commit 0 while the knob moves (in mode 0 a 0 return
 * refuses the value), 1 for Slider::setValue. */
typedef int SliderSetFn(void *ctx, int value, int commit);

/* SliderDesc::mode (u069's names; u022 / u035 had local copies).  An enum
 * type (its own alias set): Slider's constructor copies desc with mode's
 * load moved up like set's, as retail. */
enum SliderMode {
    SLIDER_MODE_IMMEDIATE = 0,              /* left / right by step, L1 / R1 by 10 steps, every move
                                               offered to set(), which may refuse it (inputImmediate) */
    SLIDER_MODE_STEP      = 1               /* left / right by step only (inputStep) */
};

/* The value range of a slider, 0x20 bytes: what the page's slider function
 * fills (MenuSliderFunc, T229).  The same record as SliderDesc of
 * T065_option_sliders.h (u022 / u035).  The constructor with every field is
 * how T065's functions build it straight into the caller's buffer, `return
 * SliderDesc(min, max, value, ...)': a local returned by value is copied
 * (ldl / sdl pairs) instead (u022 notes). */
struct SliderDesc {
    int          min;                       /* 0x00 */
    int          max;                       /* 0x04 exclusive */
    int          value;                     /* 0x08 the initial value */
    int          step;                      /* 0x0C */
    SliderSetFn *set;                       /* 0x10 */
    SliderMode   mode;                      /* 0x14 SLIDER_MODE_* */
    int          limitMin;                  /* 0x18 the inner range the value may take
                                               (SmartIndexDoubleRange::setInnerRange) */
    int          limitMax;                  /* 0x1C */

    SliderDesc() {}
    SliderDesc(int min_, int max_, int value_, int step_, SliderSetFn *set_, SliderMode mode_,
               int limitMin_, int limitMax_)
        : min(min_), max(max_), value(value_), step(step_), set(set_), mode(mode_),
          limitMin(limitMin_), limitMax(limitMax_) {}
    /* Field by field (Slider's constructor: lw / sw per field through
     * &desc, where the implicit copy moves ldl / sdl pairs).  The users in
     * u022 / u035 / u070 compile the same with it. */
    SliderDesc(const SliderDesc &o)
        : min(o.min), max(o.max), value(o.value), step(o.step), set(o.set), mode(o.mode),
          limitMin(o.limitMin), limitMax(o.limitMax) {}
};
EGCS_SIZE(gt2k_menu::SliderDesc, 0x20);

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T238_slider.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* vtables 0x003367E0 (18 slots: [3] .. [7] as Item's, [9] [10] [12] .. [17]
 * Button's) and 0x003367C0 (the Cell<Button> part, 3 slots, this-delta
 * -0x100).  typeinfo __tfQ29gt2k_menu6Slider 0x001C6C78 (class, base Button
 * public +0, base list 0x003526C0; __class_type_info 0x004B2630).  Size 0x4B0
 * (operator new in ButtonList::createSlider). */
class Slider : public Button {
public:
    Slider(ButtonList *owner, int controlType, PageLayout *layout, f32 offPoint,
           ButtonData *data, MenuLayoutItem *elem, int relative, int depth, void *ctx,
           const SliderDesc &desc);         /* 0x001C6410 the knob pictures "pinch", "pinch_prev",
                                               "pinch_fix" of the layout's hidden layer */
    /* [0] +0x0C 0x001C6CD0 ~Slider: implicit (egcs emits it after __tf) */
    virtual void virtual_1();               /* [1] +0x14 0x001C6A50 Button's */
    virtual void virtual_2();               /* [2] +0x1C 0x001C6AD0 draw: Button's, then "pinch_prev"
                                               (mode 0, away from the initial value) and the knob */
    virtual void virtual_8();               /* [8] +0x4C 0x001C6A30 back to the initial value (not
                                               committed) */
    virtual int  virtual_11(const MenuControl &ctl); /* [11] +0x64 0x001C6798 input by desc.mode; 1:
                                               the list handles it (up / down) */

    void  setValue(int value);              /* 0x001C6690 moves there and commits: desc.set(ctx, v, 1)
                                               (if the range has 2 values or more) */
    void  setRange(int min, int max, int lmin, int lmax); /* 0x001C6700 new range and inner range */
    int   inputImmediate(const MenuControl &ctl); /* 0x001C67E0 mode 0 */
    int   inputStep(const MenuControl &ctl);      /* 0x001C6958 mode 1 */
    static f32 knobX(int index, int knobW, int count, f32 x, f32 w); /* 0x001C6A70 the knob's centre for
                                               value number index of count (also SliderItem, T228) */
    int   isMovable();                      /* 0x001C6C28 the range has 2 values or more */
    SmartIndexDoubleRange *getIndex();      /* 0x001C6C50 */
    int   getValue();                       /* 0x001C6C58 */
    void  setShowPrev(bool on);             /* 0x001C6C60 */
    static void hidePrev();                 /* 0x001C6C68 no slider draws "pinch_prev" until the next
                                               Slider is built (g_003526B0 = 1) */

    void                 *ctx;              /* 0x160 desc.set's first argument */
    Item                  pinch;            /* 0x164 "pinch": the knob */
    Item                  pinchPrev;        /* 0x264 "pinch_prev": the initial value */
    Item                  pinchFix;         /* 0x364 "pinch_fix": the knob of a single-value range */
    SliderDesc            desc;             /* 0x464 */
    int                   range;            /* 0x484 desc.max - desc.min */
    /* bool (4 bytes in egcs, its own alias set): the constructor's
     * showPrev(1) store is not held behind desc's int copies (retail). */
    bool                  showPrev;         /* 0x488 1: "pinch_prev" is drawn */
    SmartIndexDoubleRange index;            /* 0x48C the value (clamped) */
    int                   initial;          /* 0x4AC desc.value */
};
EGCS_SIZE(gt2k_menu::Slider, 0x4B0);

} /* namespace gt2k_menu */

/* T238 .sdata (gp-0x5340, initially 0): 1 = no Slider draws its
 * "pinch_prev" picture (the initial value) until the next Slider is built.
 * Set by Slider::hidePrev (0x001C6C68) and T083's commit_slider, cleared by
 * the Slider constructor (0x001C65E0), read by Slider::virtual_2
 * (0x001C6B18). */
extern int g_003526B0;                      /* 0x003526B0 */

/* The inline accessors 0x001C6C28..0x001C6C68: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
namespace gt2k_menu {

inline void Slider::hidePrev() { g_003526B0 = 1; }  /* 0x001C6C68 */
inline void Slider::setShowPrev(bool on) { showPrev = on; }  /* 0x001C6C60 */
inline int Slider::getValue() { return index.value; }  /* 0x001C6C58 */
inline SmartIndexDoubleRange *Slider::getIndex() { return &index; }  /* 0x001C6C50 */
inline int Slider::isMovable() { return index.size() >= 2; }  /* 0x001C6C28 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu6Slider(void);        /* 0x001C6C78 */
}
#endif

#endif /* GAME_T238_SLIDER_H */
