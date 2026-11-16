/* T207 - gt2k_menu::MapButtonList, MapButton, MagScrBegin, MagScrEnd.
 * 0x001B4900..0x001B56D0.
 *
 * A map of buttons placed freely on the screen.  The d-pad moves the
 * selection to the button that lies best in the pushed direction
 * (findInDirection: cos^2 of the angle over the distance); the Pointer
 * picture glides to it (Pointer::targetX/Y), the button under the pointer
 * gets the focus, and the view (viewX, viewY) follows the pointer while the
 * zoom goes to 30 / the selected button's width, kept in [0.5, 2].
 * MagScrBegin / MagScrEnd (z -1 and 20) bracket the map's drawing with the
 * zoom transform (and the same scale / offset for the system font); nothing
 * in this build makes them, so only the decide effect follows the zoom.
 *
 * Data (retail): .rodata 0x00332D88..0x003330DE: the four classes'
 * vtables, then the type names MapButtonList, MapButton, MagScrEnd,
 * MagScrBegin, List<Button>, Cell<Button>, the operator list classes
 * (Item's), SmartIndexT<int>, Operator: reverse definition order, so the
 * original instantiated SmartIndexT<int> first (before Item's classes; the
 * non-polymorphic base Operator always comes last), then read the Button
 * header (Cell<Button>) and ButtonList's (List<Button>).  .sdata
 * 0x00352518..0x00352530 ("pointer", the two base lists), .lit4
 * 0x0035000C..0x00350014. */
#include "game/common/T251_smart_index.h"          /* gt2k_menu::SmartIndexT<int> (instantiated here) */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button (Item, Cell<Button>) */
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList (List<Button>), ButtonData */
#include "game/menu/framework/T207_map_button_list.h"
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (attachTo, operator new / delete) */
#include "game/menu/framework/T222_decide_effect.h"        /* gt2k_menu::DecideEffect */
#include "game/menu/framework/T223_pointer.h"              /* gt2k_menu::Pointer */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T259_menu_drawable.h"        /* MENU_SCREEN_CX / _CY */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync, the matrix stack, vu0_scale */
#include "game/menu/framework/menu_buttons_internal.h"

namespace gt2k_menu {

/* The centre of the 640 x 448 menu screen: T259's MENU_SCREEN_CX / _CY. */

/* One caller.  The pointer picture is the hidden layout item "pointer"; the
 * view starts at the screen centre with zoom 1. */
// FUNC 0x001B4900 gt2k_menu::MapButtonList::MapButtonList
MapButtonList::MapButtonList(ButtonData **items, int count, PageLayout *layout_, int index_)
    : ButtonList(0, 0, layout_), pointer(0), focusedButton(0), selected(0), unk134(1.0f),
      targetX(MENU_SCREEN_CX), targetY(MENU_SCREEN_CY), magBegin(0), unk144(0),
      viewX(MENU_SCREEN_CX), viewY(MENU_SCREEN_CY), zoom(1.0f)
{
    ButtonList::createButtons(items, count, 0);     /* a direct call in the original */
    resetIndex();
    index.set(index_);

    MenuLayoutItem *item = layout_->findHiddenItem("pointer");
    MenuTexture *tex = 0;
    if (item != 0)
        tex = layout_->getTexture(item);
    pointer = new Pointer(item, tex);       /* Object::operator new (0x11C bytes) */ /* implicit: operator new, operator delete (new-expression) */

    pointer->setPosition(targetX, targetY);  /* inline (T223) */
    pointer->attachTo(this);
}

/* Focus from the index (as ButtonList's slot 9): the button at index.value
 * gets the focus (unless the list's focus is off) and becomes the selected
 * one; the pointer and the view jump to it. */
// FUNC 0x001B4AE0 gt2k_menu::MapButtonList::refresh
void MapButtonList::refresh()
{
    Button *sel = 0;
    int i = 0;
    ListIterator<Button> it(this);
    Button *b;
    while ((b = it.next()) != 0) {
        if (i == index.value && focusDisabled == 0) {
            b->setFocus(1);
            sel = b;
        } else {
            b->setFocus(0);
        }
        i++;
    }
    if (sel != 0) {
        targetX = sel->x.value;
        targetY = sel->y.value;
        selected = sel;
        pointer->setPosition(targetX, targetY);
        viewX.value = pointer->x.get();
        viewY.value = pointer->y.get();
    }
}

/* Creates a MapButton (0x16C bytes) bound to the view, depth = index;
 * offPoint unused. */
// FUNC 0x001B4BF0 gt2k_menu::MapButtonList::createButton
Button *MapButtonList::createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                    int index_)
{
    MapButton *b = new MapButton(this, layout, data, elem, index_, &viewX, &viewY, &zoom); /* implicit: operator new, operator delete (new-expression) */
                                            /* Object::operator new */
    b->attachTo(this);
    return b;
}

/* Update (the ButtonList update is not called): the view follows the
 * pointer (rate 0.1), the zoom follows 30 / the selected button's width
 * (rate 0.05), clamped to [0.5, 2]; then the MagScrBegin gets the view. */
// FUNC 0x001B4CB0 gt2k_menu::MapButtonList::virtual_1
void MapButtonList::virtual_1()
{
    viewX.approach(pointer->x.value, 0.1f);
    viewY.approach(pointer->y.value, 0.1f);
    if (selected != 0)
        zoom.approach(30.0f / selected->w, 0.05f);
    zoom.clampMax(2.0f);
    zoom.clampMin(0.5f);
    if (magBegin != 0)
        magBegin->set(zoom.get(), viewX.get(), viewY.get());
}

// FUNC 0x001B4DB8 gt2k_menu::MapButtonList::focusButton
Button *MapButtonList::focusButton()
{
    return focusedButton;
}

/* 1 when the pointer is within 12 pixels of the target. */
// FUNC 0x001B4DC0 gt2k_menu::MapButtonList::isPointerOnTarget
int MapButtonList::isPointerOnTarget()
{
    f32 dx = targetX - pointer->x.get();
    f32 dy = targetY - pointer->y.get();
    if (dx * dx + dy * dy < 144.0f)
        return 1;
    return 0;
}

/* The button that lies best in direction (dirX, dirY) from the target:
 * the largest cos^2 / distance among the buttons in front (cos >= 0); NULL
 * if none. */
// FUNC 0x001B4E10 gt2k_menu::MapButtonList::findInDirection
Button *MapButtonList::findInDirection(f32 dirX, f32 dirY)
{
    Button *best = 0;
    f32 fromX = targetX;
    f32 fromY = targetY;
    f32 bestScore = 0.0f;
    ListIterator<Button> it(this);
    Button *b;
    while ((b = it.next()) != 0) {
        f32 dx = b->x.value - fromX;
        f32 dy = b->y.value - fromY;
        f32 dist2 = dx * dx + dy * dy;
        if (dist2 <= 0.0f)
            continue;
        f32 invDist = 1.0f / boot_sqrtf(dist2);
        f32 cosAngle = dirX * (dx * invDist) + dirY * (dy * invDist);
        if (cosAngle < 0.0f)
            continue;
        f32 score = cosAngle * cosAngle * invDist;
        if (bestScore < score) {
            bestScore = score;
            best = b;
        }
    }
    return best;
}

// FUNC 0x001B4EF0 gt2k_menu::MapButtonList::moveSelection
void MapButtonList::moveSelection(f32 dirX, f32 dirY)
{
    Button *b = findInDirection(dirX, dirY);
    if (b != 0)
        selected = b;
}

/* Input: the pointer's target is the selected button (with none, the
 * pointer jumps to the screen centre); the focus goes to the selected
 * button once the pointer is on it; up / down / left / right move the
 * selection (sound 3: the cursor), decide decides the focused button.
 * (Quirk: with no selection and the pointer on the old target, selected
 * (NULL) gets setFocus(1).) */
// FUNC 0x001B4F20 gt2k_menu::MapButtonList::doControl
void MapButtonList::doControl(const MenuControl &ctl)
{
    Button *sel = selected;
    if (sel != 0) {
        targetX = sel->x.value;
        targetY = sel->y.value;
        pointer->setTarget(targetX, targetY);   /* inline (T223); jump2 merges the */
    } else {                                    /* two target stores of the arms */
        pointer->setPosition(MENU_SCREEN_CX, MENU_SCREEN_CY);
    }

    Button *focus = focusedButton;
    if (focus != sel) {
        if (focus != 0) {
            focus->setFocus(0);
            focusedButton = 0;
        }
        if (isPointerOnTarget() != 0) {
            Button *s = selected;
            focusedButton = s;
            s->setFocus(1);
        }
    }

    if (ctl.up != 0) {
        moveSelection(0.0f, -1.0f);
        menu_play_se(MENU_SE_CURSOR);
    } else if (ctl.down != 0) {
        moveSelection(0.0f, 1.0f);
        menu_play_se(MENU_SE_CURSOR);
    } else if (ctl.left != 0) {
        moveSelection(-1.0f, 0.0f);
        menu_play_se(MENU_SE_CURSOR);
    } else if (ctl.right != 0) {
        moveSelection(1.0f, 0.0f);
        menu_play_se(MENU_SE_CURSOR);
    } else if (ctl.decide != 0) {
        if (focusButton() != 0)             /* slot 8 */
            decide();                       /* slot 11 */
    }
}

/* Button(owner, 0, layout, offPoint 0.0, linkMenu, elem, not relative,
 * depth), then the view of the list. */
// FUNC 0x001B50A8 gt2k_menu::MapButton::MapButton
MapButton::MapButton(ButtonList *owner_, PageLayout *layout_, ButtonData *linkMenu_,
                     MenuLayoutItem *elem, int depth_, MotionValue *viewX_, MotionValue *viewY_,
                     MotionValue *zoom_)
    : Button(owner_, 0, layout_, 0.0f, linkMenu_, elem, 0, depth_)
{
    viewX = viewX_;
    viewY = viewY_;
    zoom = zoom_;
}

/* Button::virtual_15 with the effect bound to the list's zoom and view. */
// FUNC 0x001B5148 gt2k_menu::MapButton::virtual_15
void MapButton::virtual_15()
{
    f32 effectX = posX;
    f32 effectY = posY;
    f32 effectW = w;
    f32 effectH = h;
    int done;
    DecideEffect *effect = new DecideEffect(&done, effectX, effectY, effectW, effectH); /* implicit: operator new, operator delete (new-expression) */
                                            /* Object::operator new (0xD4 bytes) */
    effect->setView(&zoom->value, &viewX->value, &viewY->value);
    effect->attachTo(this);
    while (done != 1)
        gfx_vsync(1);
}

/* Unused in this build: no callers, no table entry, the vtable has no
 * other reference (so MapButtonList::magBegin stays NULL). */
// FUNC 0x001B5248 gt2k_menu::MagScrBegin::MagScrBegin
MagScrBegin::MagScrBegin()
{
    /* implicit: Object (base constructor) */
    /* Object() first, then the vptr */
    zoom = 1.0f;
    x = MENU_SCREEN_CX;
    y = MENU_SCREEN_CY;
    z = -1.0f;
}

// FUNC 0x001B52B0 gt2k_menu::MagScrBegin::set
void MagScrBegin::set(f32 zoom_, f32 x_, f32 y_)
{
    zoom = zoom_;
    x = x_;
    y = y_;
}

/* Pushes scale(zoom) * translate(320 / zoom - x, 224 / zoom - y): (x, y)
 * comes to the screen centre; the system font gets the same scale and
 * offset. */
// FUNC 0x001B52C0 gt2k_menu::MagScrBegin::virtual_2
void MagScrBegin::virtual_2()
{
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    f32 scale = zoom;
    f32 offsetX = MENU_SCREEN_CX / scale - x;
    f32 offsetY = MENU_SCREEN_CY / scale - y;
    vu0_scale(scale, scale, 1.0f);
    pgl_translate(offsetX, offsetY, 0.0f);
    g_002F1968.setScale(zoom);
    g_002F1968.setOffset(offsetX, offsetY);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
}

/* Unused in this build: no callers, no table entry, the vtable has no
 * other reference (so MapButtonList::magBegin stays NULL). */
// FUNC 0x001B5390 gt2k_menu::MagScrEnd::MagScrEnd
MagScrEnd::MagScrEnd()
{
    /* implicit: Object (base constructor) */
    /* Object() first, then the vptr */
    z = 20.0f;
}

/* Resets the font scale / offset and pops the matrix. */
// FUNC 0x001B53D0 gt2k_menu::MagScrEnd::virtual_2
void MagScrEnd::virtual_2()
{
    g_002F1968.setScale(1.0f);
    g_002F1968.setOffset(0.0f, 0.0f);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x001B5430 __tfQ29gt2k_menu13MapButtonList
extern "C" void *__tfQ29gt2k_menu13MapButtonList(void)
{
    if (__tiQ29gt2k_menu13MapButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu13MapButtonList, "Q29gt2k_menu13MapButtonList",
                     g_00352520, 1);
    }
    return __tiQ29gt2k_menu13MapButtonList;
}
#endif

namespace gt2k_menu {

// FUNC 0x001B5488 gt2k_menu::MapButtonList::~MapButtonList
/* the implicit destructor (game/menu/framework/T207_map_button_list.h): both vptrs,
 * ButtonList::~ButtonList(0), Object::operator delete if flags & 1 */
/* implicit: ~ButtonList (base destructor), operator delete (deleting destructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x001B54E8 __tfQ29gt2k_menu9MapButton
extern "C" void *__tfQ29gt2k_menu9MapButton(void)
{
    if (__tiQ29gt2k_menu9MapButton[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu9MapButton, "Q29gt2k_menu9MapButton", g_00352528, 1);
    }
    return __tiQ29gt2k_menu9MapButton;
}
#endif

namespace gt2k_menu {

// FUNC 0x001B5540 gt2k_menu::MapButton::~MapButton
/* the implicit destructor (game/menu/framework/T207_map_button_list.h): both vptrs,
 * Button::~Button(0), Object::operator delete if flags & 1 */
/* implicit: ~Button (base destructor), operator delete (deleting destructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Object). */
// FUNC 0x001B55A0 __tfQ29gt2k_menu9MagScrEnd
extern "C" void *__tfQ29gt2k_menu9MagScrEnd(void)
{
    if (__tiQ29gt2k_menu9MagScrEnd[0] == 0) {
        __tfQ29gt2k_menu6Object();
        __rtti_si(__tiQ29gt2k_menu9MagScrEnd, "Q29gt2k_menu9MagScrEnd",
                  (const type_info *)__tiQ29gt2k_menu6Object);
    }
    return __tiQ29gt2k_menu9MagScrEnd;
}
#endif

// FUNC 0x001B55F0 gt2k_menu::MagScrEnd::~MagScrEnd
/* the implicit destructor (game/menu/framework/T207_map_button_list.h): Object::~Object(0)
 * without a vptr store, Object::operator delete if flags & 1 */
/* implicit: ~Object (base destructor), operator delete (deleting destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Object). */
// FUNC 0x001B5638 __tfQ29gt2k_menu11MagScrBegin
extern "C" void *__tfQ29gt2k_menu11MagScrBegin(void)
{
    if (__tiQ29gt2k_menu11MagScrBegin[0] == 0) {
        __tfQ29gt2k_menu6Object();
        __rtti_si(__tiQ29gt2k_menu11MagScrBegin, "Q29gt2k_menu11MagScrBegin",
                  (const type_info *)__tiQ29gt2k_menu6Object);
    }
    return __tiQ29gt2k_menu11MagScrBegin;
}
#endif

// FUNC 0x001B5688 gt2k_menu::MagScrBegin::~MagScrBegin
/* the implicit destructor (game/menu/framework/T207_map_button_list.h): Object::~Object(0)
 * without a vptr store, Object::operator delete if flags & 1 */
/* implicit: ~Object (base destructor), operator delete (deleting destructor) */
