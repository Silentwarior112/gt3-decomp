/* T046 - gt2k_menu::GtmodeRender, the renderer of GtmodeControl.
 * 0x00127368..0x001274E0.
 *
 * A RenderBase that takes the menu background movie's next frame
 * (MenuBgMovie g_002CDC10, T253) before it draws the menu tree.
 *
 * Includes: the original's type names (.rodata 0x003125B0..0x00312716,
 * after GtmodeRender's vtable 0x00312588; reverse definition order) are
 * GtmodeRender, SmartIndexT<int>, List<Button>, Cell<Button>, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>, Operator: the
 * operator classes (T216), Button (T206), List<Button>, SmartIndexT<int>,
 * then GtmodeRender.  No GameStatus, no <new>. */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen: the operator classes) */
#include "game/menu/framework/T206_button.h"               /* (seen: Cell<Button>) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member. */
#include "game/menu/framework/T244_button_list.h"          /* (seen: ButtonList, SmartIndexT<int>) */
#include "game/menu/gtmode/T046_gtmode_render.h"
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si (GCC 14's __tf) */
#include "game/menu/gtmode/gtmode_main_internal.h"

namespace gt2k_menu {

/* RenderBase(), then the vptr (0x00312588).  Caller: the GtmodeControl
 * constructor (T043). */
// FUNC 0x00127368 gt2k_menu::GtmodeRender::GtmodeRender
GtmodeRender::GtmodeRender()
{
    /* implicit: RenderBase (base constructor) */
}

/* No callers. */
// FUNC 0x001273A0 gt2k_menu::GtmodeRender::begin
void GtmodeRender::begin()
{
    attach();
}

/* Caller: GtmodeControl::end (T043). */
// FUNC 0x001273C0 gt2k_menu::GtmodeRender::end
void GtmodeRender::end()
{
    detach();
}

/* Slot 1 (index.txt: virtual_1). */
// FUNC 0x001273E0 gt2k_menu::GtmodeRender::update
void GtmodeRender::update()
{
    RenderBase::update();
}

/* Slot 2 (index.txt: virtual_2). */
// FUNC 0x00127400 gt2k_menu::GtmodeRender::render
void GtmodeRender::render(GSBuffer &buf)
{
    g_002CDC10.update();
    RenderBase::render(buf);
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Builds the __si_type_info of GtmodeRender (base RenderBase) on the first
 * call. */
// FUNC 0x00127448 __tfQ29gt2k_menu12GtmodeRender
extern "C" void *__tfQ29gt2k_menu12GtmodeRender(void)
{
    if (__tiQ29gt2k_menu12GtmodeRender[0] == 0) {       /* _name not set yet */
        __tfQ29gt2k_menu10RenderBase();                 /* the base's node first */
        __rtti_si(__tiQ29gt2k_menu12GtmodeRender, "Q29gt2k_menu12GtmodeRender",
                  (const type_info *)__tiQ29gt2k_menu10RenderBase);
    }
    return __tiQ29gt2k_menu12GtmodeRender;
}
#endif

// FUNC 0x00127498 gt2k_menu::GtmodeRender::~GtmodeRender
/* implicit in game/menu/gtmode/T046_gtmode_render.h (index.txt: virtual_0); egcs emits
 * this copy for the vtable: no vptr store; RenderBase's inline destructor:
 * root's gt2k_menu::RootObject::~RootObject (flags 2), then
 * GameObject::~GameObject(this, flags), which does the delete. */
/* implicit: ~RootObject, ~GameObject (the inline ~RenderBase) */
