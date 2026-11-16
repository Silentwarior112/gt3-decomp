#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/menu/framework/T206_button.cpp"
#pragma implementation "src/game/menu/framework/T206_button.cpp"
#pragma implementation "T206_button.h"
#endif
/* T206 - gt2k_menu::Button and the Cell<Button> destructor instance.
 * 0x001B3BE8..0x001B4900.
 *
 * The button of a ButtonList: an Item (its picture) and a list cell.  It
 * keeps its position along the list (offPoint); each frame update works out
 * its screen position (posX, posY) from the list's position and scroll
 * offset, and the draw places the Item there for the time of the draw
 * (beginDraw / endDraw), clipped to the list.  A focused button draws its
 * "active" picture (or a grey quad) and the list's cursor picture over it,
 * blinking (blink); a button whose check function says it is not available
 * is drawn half-toned, with the list's "disable" picture over it.
 *
 * Data (retail .rodata 0x00332B40..0x00332D86): Button's two vtables and
 * Cell<Button>'s, then the type names SmartIndexT<int>, List<Button>,
 * Button (its base list 0x00332C78), Cell<Button>, the operator list
 * classes (Item's), Operator: reverse definition order, so the original
 * read the Button header (Item, Cell<Button>, Button) and then ButtonList's
 * (its List<Button> base, then the SmartIndexT<int> index member).  .lit4
 * 0x0034FFF4..0x0035000C. */
#include "game/menu/framework/T206_button.h"               /* gt2k_menu::Button (Item, Cell<Button>) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail 0x00332C38 / 0x00332C18). */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList (the owner), ButtonData */
#include "game/menu/framework/T222_decide_effect.h"        /* gt2k_menu::DecideEffect */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, ITEM_* */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable::setRectVertices */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_clip_x / _y / _full, menu_is_clipped */
#include "engine/pgl/G001_flare.h"                /* gfx_enable, gfx_blend_func, gfx_alloc_tail, gfx_vsync,
                                               pgl_color4f, pgl_draw_strip */
#include "game/menu/framework/menu_buttons_internal.h"

namespace gt2k_menu {

/* The button kinds of the layout item flags: T229's ITEM_BUTTON_KIND_*. */

/* With `relative`, offPoint is the offset along the list: x for a
 * horizontal list, y for a vertical one. */
// FUNC 0x001B3BE8 gt2k_menu::Button::placeOffPoint
void Button::placeOffPoint()
{
    if (relative != 0) {
        if (controlType == 0) {
            x.set(0.0f);
            y.set(offPoint);
        } else {
            x.set(offPoint);
            y.set(0.0f);
        }
    }
}

/* Item(elem, layout), the Cell<Button> part (next = prev = 0), then the two
 * Button vtables.  An element with flag 8 gets its "active" picture
 * (elem->active).  The z goes up by 100 per layout level and down by 0.001
 * per depth unit. */
// FUNC 0x001B3C50 gt2k_menu::Button::Button
Button::Button(ButtonList *owner_, int controlType_, PageLayout *layout_, f32 offPoint_,
               ButtonData *linkMenu_, MenuLayoutItem *elem, int relative_, int depth_)
    : Item(elem, layout_)
{
    /* implicit: ~pCell (exception cleanup of the Cell part) */
    /* (the landing pads unwind the Cell part with pCell::~pCell, then
     * Item::~Item) */
    depth = depth_;
    u32 flags = elem->flags;                /* read here: retail loads it before the int stores
                                               below (they cannot pass an int load) */
    controlType = controlType_;
    layout = layout_;
    owner = owner_;
    linkMenu = linkMenu_;
    offPoint = offPoint_;
    baseZ = z;
    relative = relative_;
    blink = 0.5f;
    element = elem;
    activeItem = 0;
    focused = 0;
    disableFocus = 0;
    decide = 0;
    check = 0;
    checkCtx = 0;
    effectRunning = 0;
    savedX = 0.0f;
    savedY = 0.0f;
    posX = 0.0f;
    posY = 0.0f;
    if (flags & ITEM_HAS_ACTIVE) {
        MenuLayoutItem *activeElem = elem->active;
        activeItem = new Item(activeElem, layout);    /* Object::operator new */ /* implicit: operator new, operator delete (new-expression) */
    }
    placeOffPoint();
    z = z + (f32)layout->level * 100.0f + -(f32)depth * 0.001f;
    /* Left (96.4%): sched1 puts the flags load before the 0.5 constant and
     * z gets $f1 / 0.5 $f0 (retail: z $f0, 0.5 $f1, the load after both);
     * the same with the reference compiler 991111-01.  Tried: depth / blink
     * as mem-initializers, 12 orders of depth, flags, blink and baseZ,
     * the flags test as a local, offPoint at every parameter position; a
     * local `half' set before the flags load gives retail's registers
     * (96.9%), not kept. */
}

/* Deletes the active picture (cleared first). */
// FUNC 0x001B3E60 gt2k_menu::Button::~Button
Button::~Button()
{
    /* egcs then runs the inlined ~Cell<Button> (its vptr, pCell::~pCell(0)),
     * Item::~Item(0), and Object::operator delete if flags & 1. */
    Item *active = activeItem;
    if (active != 0) {
        activeItem = 0;
        delete active;
    }
    /* implicit: operator delete (deleting destructor) */
}

/* Availability check: fn(ctx, linkMenu); one caller (ButtonList::
 * setButtonChecks). */
// FUNC 0x001B3F08 gt2k_menu::Button::setCheck
void Button::setCheck(int (*fn)(void *ctx, ButtonData *data), void *ctx)
{
    check = fn;
    checkCtx = ctx;
}

// FUNC 0x001B3F18 gt2k_menu::Button::isAvailable
int Button::isAvailable()
{
    if (check != 0)
        return check(checkCtx, linkMenu);
    return 1;
}

// FUNC 0x001B3F50 gt2k_menu::Button::getElement
MenuLayoutItem *Button::getElement()
{
    return element;
}

/* Slot 13 when the focus comes, slot 14 when it goes; a focused button
 * makes the list show the record's two help layers. */
// FUNC 0x001B3F58 gt2k_menu::Button::setFocus
void Button::setFocus(int focus)
{
    if (focus != 0) {
        if (focused == 0)
            virtual_13();
    } else if (focused != 0) {
        virtual_14();
    }
    focused = focus;
    if (focus != 0) {
        owner->showHelpLayer(owner, linkMenu->help);
        owner->showHelpLayer2(owner, linkMenu->help2);
    }
}

/* Update: the active picture, the Item, the blink value of the focus frame
 * (kind 0x2000 buttons count it down by 1/60 a frame while focused in an
 * active list), then the screen position.  The blink case falls through
 * into the frame case (retail: one 0.5 store for the three paths).  Its
 * count-down test is an exact form, not the original: retail branches out
 * on !(blink < 0) (c.lt.s; bc1f), which -ffast-math folds to c.le.s when
 * written as `if (blink >= 0) break;' or `if (!(blink < 0)) break;'; the
 * empty then-branch keeps retail's test (also for a NaN blink). */
// FUNC 0x001B4008 gt2k_menu::Button::virtual_1
void Button::virtual_1()
{
    if (activeItem != 0)
        activeItem->virtual_1();
    Item::virtual_1();

    if (effectRunning == 0) {
        switch (element->flags & ITEM_BUTTON_KIND_MASK) {
        case ITEM_BUTTON_KIND_BLINK:
            if (owner->active == 0) {
                blink = 0.25f;
                break;
            }
            if (focused != 0) {
                blink -= 0.016666668f;      /* 1/60 */
                if (blink < 0.0f) {
                    /* run out: start again at 0.5 */
                } else {
                    break;
                }
            }
            /* fall through (not focused, or the count-down ran out) */
        case ITEM_BUTTON_KIND_FRAME:
            blink = 0.5f;
            break;
        default:
            blink = 0.0f;
            break;
        }
    } else {
        blink = 0.25f;
    }

    if (relative != 0) {
        f32 originX, originY;
        if (controlType == 0) {
            originX = owner->x.value;
            originY = owner->y.value + owner->displayY.value;
        } else {
            originX = owner->x.value + owner->displayX.value;
            originY = owner->y.value;
        }
        posX = x.value + originX;
        posY = y.value + originY;
    } else {
        posX = x.value;
        posY = y.value;
    }
}

/* Clips to the list (relative buttons), saves the position and moves the
 * Item (and its active picture) to the screen position. */
// FUNC 0x001B4130 gt2k_menu::Button::beginDraw
void Button::beginDraw()
{
    placeOffPoint();
    if (relative != 0) {
        if (controlType == 0)
            menu_clip_y(owner->y.value, owner->h);
        else
            menu_clip_x(owner->x.value, owner->w);
    }
    savedX = x.get();
    savedY = y.get();
    x.set(posX);
    y.set(posY);
    Item *active = activeItem;
    if (active != 0) {
        active->x.set(x.get());
        active->y.set(y.get());
    }
}

/* The picture: the active one while focused (half-toned if not available),
 * else the button's own (half-toned if not available and the list has no
 * "disable" picture). */
// FUNC 0x001B41F8 gt2k_menu::Button::drawItem
void Button::drawItem()
{
    if (activeItem != 0 && focused != 0) {
        int available = isAvailable();
        Item *active = activeItem;
        active->half = available ^ 1;
        active->virtual_2();
    } else {
        if (owner->disableItem == 0 && isAvailable() == 0)
            half = 1;
        Item::virtual_2();
    }
}

/* Focus frame: without an active picture an additive quad of grey level
 * `blink` over the button; with one, the button's own picture again (kind
 * 0x2000: with alpha 2 a (0.5 - blink), a cross-fade), then the list's
 * cursor picture at alpha blink + 0.5.  Then the list's "disable" picture
 * over an unavailable button. */
// FUNC 0x001B4288 gt2k_menu::Button::drawFocus
void Button::drawFocus()
{
    if (focused != 0) {
        if (activeItem == 0) {
            gfx_enable(PGL_CAP_BLEND);
            gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_AS, PGL_SEL_CD),
                           1.0f);           /* 0x48: Cs * As + Cd */
            PrimVertex *quad = (PrimVertex *)gfx_alloc_tail(3);    /* 4 vertices */
            setRectVertices(quad, x.value, y.value, w, h);
            pgl_color4f(blink, blink, blink, 1.0f);
            pgl_draw_strip(quad, 0, 0, 0, 4);
        } else {
            if (effectRunning == 0) {
                if ((element->flags & ITEM_BUTTON_KIND_MASK) == ITEM_BUTTON_KIND_BLINK) {
                    f32 alpha = alphaRatio;
                    alphaRatio = alpha * (0.5f - blink) * 2.0f;
                    Item::virtual_2();
                    alphaRatio = alpha;
                }
            } else {
                Item::virtual_2();
            }
            if ((element->flags & ITEM_BUTTON_KIND_MASK) != 0) {
                Item *cursor = owner->focusItem;
                if (cursor != 0) {
                    cursor->x.set(posX);
                    cursor->y.set(posY);
                    cursor->alphaRatio = blink + 0.5f;
                    cursor->virtual_2();
                }
            }
        }
    }
    Item *disable = owner->disableItem;
    if (disable != 0 && isAvailable() == 0) {
        disable->x.set(posX);
        disable->y.set(posY);
        disable->w = w;
        disable->h = h;
        disable->virtual_2();
    }
}

/* Restores the position and the clip rectangle (relative buttons). */
// FUNC 0x001B4478 gt2k_menu::Button::endDraw
void Button::endDraw()
{
    if (relative != 0) {
        x.set(savedX);
        y.set(savedY);
        menu_clip_full();
    }
}

// FUNC 0x001B44C0 gt2k_menu::Button::isClipped
int Button::isClipped()
{
    return menu_is_clipped(x.value, y.value, w, h);
}

// FUNC 0x001B44E8 gt2k_menu::Button::virtual_2
void Button::virtual_2()
{
    if (visible != 0) {
        beginDraw();
        if (isClipped() == 0) {
            drawItem();
            drawFocus();
        }
        endDraw();
    }
}

// FUNC 0x001B4548 gt2k_menu::Button::virtual_8
void Button::virtual_8()
{
}

// FUNC 0x001B4550 gt2k_menu::Button::virtual_9
void Button::virtual_9(int v)
{
    visible = v;
}

/* Empty: only the buttons with a sub list (T268 / T269) take the list's
 * decide callback. */
// FUNC 0x001B4558 gt2k_menu::Button::virtual_10
void Button::virtual_10(void *ctx, void (*func)(void *ctx, ButtonData *data, int index,
                                                ButtonList *list, int a4))
{
}

/* The focused button's input frame: a plain button lets the list go on. */
// FUNC 0x001B4560 gt2k_menu::Button::virtual_11
int Button::virtual_11(const MenuControl &ctl)
{
    return 1;
}

// FUNC 0x001B4568 gt2k_menu::Button::virtual_12
void Button::virtual_12()
{
}

// FUNC 0x001B4570 gt2k_menu::Button::virtual_13
void Button::virtual_13()
{
}

// FUNC 0x001B4578 gt2k_menu::Button::virtual_14
void Button::virtual_14()
{
}

/* Decide effect: a DecideEffect (0xD4 bytes) over the button, then waits
 * (one vsync at a time) until it sets `done` to 1. */
// FUNC 0x001B4580 gt2k_menu::Button::virtual_15
void Button::virtual_15()
{
    f32 effectX = posX;
    f32 effectY = posY;
    f32 effectW = w;
    f32 effectH = h;
    int done;
    DecideEffect *effect = new DecideEffect(&done, effectX, effectY, effectW, effectH); /* implicit: operator new, operator delete (new-expression) */
                                            /* Object::operator new (0xD4 bytes) */
    effect->attachTo(this);
    while (done != 1)
        gfx_vsync(1);
}

/* Blink for 30 frames: alpha 0.4, 0.6, ... 1.0 (then 0.4 again); with an
 * active picture the button gets 1 - alpha and the picture 1. */
// FUNC 0x001B4660 gt2k_menu::Button::virtual_16
void Button::virtual_16()
{
    int frames = 0x1D;
    effectRunning = 1;
    f32 alpha = 0.4f;
    do {
        if (activeItem != 0) {
            alphaRatio = 1.0f - alpha;
            activeItem->alphaRatio = 1.0f;
        } else {
            alphaRatio = alpha;
        }
        alpha += 0.2f;
        frames--;
        if (1.0f < alpha)
            alpha = 0.4f;
        gfx_vsync(1);
    } while (frames >= 0);
    alphaRatio = 1.0f;
    if (activeItem != 0)
        activeItem->alphaRatio = 1.0f;
    effectRunning = 0;
}

/* Fade for 30 frames: alpha 1.0, 0.8, ... 0.2 (then 1.0 again), the active
 * picture the same. */
// FUNC 0x001B4718 gt2k_menu::Button::virtual_17
void Button::virtual_17()
{
    int frames = 0x1D;
    effectRunning = 1;
    f32 alpha = 1.0f;
    do {
        alphaRatio = alpha;
        if (activeItem != 0)
            activeItem->alphaRatio = alpha;
        alpha -= 0.2f;
        frames--;
        if (alpha < 0.2f)
            alpha = 1.0f;
        gfx_vsync(1);
    } while (frames >= 0);
    alphaRatio = 1.0f;
    if (activeItem != 0)
        activeItem->alphaRatio = 1.0f;
    effectRunning = 0;
}

// FUNC 0x001B47C0 gt2k_menu::Cell<gt2k_menu::Button>::~Cell
/* inline in game/menu/framework/menu_list.h; egcs emits this copy (vtable 0x00332BF8 slot
 * 0): the Cell<Button> vptr, then pCell::~pCell with the flags */
/* implicit: ~pCell (base destructor) */

/* Inline members emitted out of line (no callers). */
// FUNC 0x001B47E8 gt2k_menu::Button::getPosY
/* inline in game/menu/framework/T206_button.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001B47F0 gt2k_menu::Button::getPosX
/* inline in game/menu/framework/T206_button.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001B47F8 gt2k_menu::Button::getActiveItem
/* inline in game/menu/framework/T206_button.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001B4800 gt2k_menu::Button::setDecide
/* inline in game/menu/framework/T206_button.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001B4810 gt2k_menu::Button::getLinkMenu
/* inline in game/menu/framework/T206_button.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001B4818 gt2k_menu::Button::setOffPoint
/* inline in game/menu/framework/T206_button.h; egcs emits this copy (#pragma implementation) */
/* implicit: placeOffPoint (the inline body) */

// FUNC 0x001B4838 gt2k_menu::Button::isDisableFocus
/* inline in game/menu/framework/T206_button.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001B4840 gt2k_menu::Button::setDisableFocus
/* inline in game/menu/framework/T206_button.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001B4848 gt2k_menu::Button::isFocused
/* inline in game/menu/framework/T206_button.h; egcs emits this copy (#pragma implementation) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, two bases). */
// FUNC 0x001B4850 __tfQ29gt2k_menu6Button
extern "C" void *__tfQ29gt2k_menu6Button(void)
{
    if (__tiQ29gt2k_menu6Button[0] == 0) {
        __tfQ29gt2k_menut4Cell1ZQ29gt2k_menu6Button();
        __tfQ29gt2k_menu4Item();
        __rtti_class(__tiQ29gt2k_menu6Button, "Q29gt2k_menu6Button", g_00332C78, 2);
    }
    return __tiQ29gt2k_menu6Button;
}

/* Compiler-generated typeinfo function (__si_type_info, base pCell). */
// FUNC 0x001B48B0 __tfQ29gt2k_menut4Cell1ZQ29gt2k_menu6Button
extern "C" void *__tfQ29gt2k_menut4Cell1ZQ29gt2k_menu6Button(void)
{
    if (__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu6Button[0] == 0) {
        __tfQ29gt2k_menu5pCell();
        __rtti_si(__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu6Button,
                  "Q29gt2k_menut4Cell1ZQ29gt2k_menu6Button",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1ZQ29gt2k_menu6Button;
}
#endif
