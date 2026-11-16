/* T223 - gt2k_menu::Pointer.  0x001C0A00..0x001C0D18.
 *
 * The pointer of a MapButtonList (T207).  Its position (x, y) eases towards
 * the target, 0.3 of the way per frame.  It is drawn at disp: the position
 * seen through the map view (scale `zoom', the view centre viewX / viewY
 * shown at the screen centre 320, 224), plus half its size. */
/* Retail's .rodata (0x00334D10..0x00334E46): the vtable, then the type names
 * Pointer, OperatorList, List / Cell<OperatorHolder>, OperatorHolder,
 * Operator. */
#include "game/menu/framework/T223_pointer.h"              /* (T213: Item; T259: MENU_SCREEN_*; T260: MotionValue) */
#include "game/menu/framework/menu_widgets_internal.h"

namespace gt2k_menu {

// FUNC 0x001C0A00 gt2k_menu::Pointer::Pointer
Pointer::Pointer(MenuLayoutItem *item, MenuTexture *tex)
    : Item(item, tex)
{
    targetY = MENU_SCREEN_CY;
    targetX = MENU_SCREEN_CX;
    viewX = MENU_SCREEN_CX;
    viewY = MENU_SCREEN_CY;
    zoom = 1.0f;
    x.set(MENU_SCREEN_CX);
    y.set(targetY);
    calcDisp();
}

// FUNC 0x001C0AB8 gt2k_menu::Pointer::setView
void Pointer::setView(float viewZoom, float centreX, float centreY)
{
    zoom = viewZoom;
    viewX = centreX;
    viewY = centreY;
}

// FUNC 0x001C0AC8 gt2k_menu::Pointer::calcDisp
void Pointer::calcDisp()
{
    f32 scale = zoom;
    dispX = (x.value + (MENU_SCREEN_CX / scale - viewX)) * scale + w * 0.5f;
    dispY = (y.value + (MENU_SCREEN_CY / scale - viewY)) * scale + h * 0.5f;
}

// FUNC 0x001C0B48 gt2k_menu::Pointer::virtual_1
void Pointer::virtual_1()
{
    Item::virtual_1();
    x.approach(targetX, 0.3f);
    y.approach(targetY, 0.3f);
    calcDisp();
}

/* Draw at disp, then put the position back. */
// FUNC 0x001C0BA0 gt2k_menu::Pointer::virtual_2
void Pointer::virtual_2()
{
    f32 posX = x.get();
    f32 posY = y.get();
    x.set(dispX);
    y.set(dispY);
    Item::virtual_2();
    x.set(posX);
    y.set(posY);
}

// FUNC 0x001C0C30 gt2k_menu::Pointer::getDispY(void) const
float Pointer::getDispY() const
{
    return dispY;
}

// FUNC 0x001C0C38 gt2k_menu::Pointer::getDispX(void) const
float Pointer::getDispX() const
{
    return dispX;
}

// FUNC 0x001C0C40 gt2k_menu::Pointer::setTarget(float, float)
/* inline in game/menu/framework/T223_pointer.h (T207 expands it); egcs emits this copy */

// FUNC 0x001C0C50 gt2k_menu::Pointer::setPosition
/* inline in game/menu/framework/T223_pointer.h (T207 expands it); egcs emits this copy:
 * the position and the target both */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001C0C80 __tfQ29gt2k_menu7Pointer
extern "C" void *__tfQ29gt2k_menu7Pointer(void)
{
    if (__tiQ29gt2k_menu7Pointer[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu7Pointer, "Q29gt2k_menu7Pointer",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu7Pointer;
}
#endif

// FUNC 0x001C0CD0 gt2k_menu::Pointer::~Pointer
/* the implicit destructor (game/menu/framework/T223_pointer.h): Item::~Item (flags 0)
 * without a vptr store, then Object::operator delete if flags & 1 */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
