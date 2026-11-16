/* T112 - gt2k_menu::ShortmenuRender, the renderer of the short menu.
 * 0x0012C978..0x0012CB70.
 *
 * A RenderBase that also runs (update) and draws (render) the race the
 * menu is shown over, when one is set (run_short_menu's race argument).
 *
 * Includes (data pass): retail's .rodata (0x00314C08..0x00314CD0) is
 * "bad_alloc", the vtable, then the type names ShortmenuControl,
 * SmartIndexT<int>, ShortmenuRender, RaceCameraBase, UnitArena<64> (reverse
 * definition order: the race base first, this file's class, then the
 * control's header); no garage, race display or menu class, no .sdata.
 * "bad_alloc" is unreferenced, hence the anchor. */
// DATA_START 0x00314C08
#include <new>                             /* "bad_alloc" (retail's first .rodata item) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/race/core/T130_race_ps2_base.h"        /* RacePS2Base (update, drawView), RaceCameraBase */
#include "game/menu/race_menu/T112_shortmenu_render.h"
#include "game/menu/race_menu/T111_shortmenu_control.h"    /* (seen, not used: ShortmenuControl, SmartIndexT<int>) */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_00352750 / g_0035274C */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_si, type_info */
#endif

namespace gt2k_menu {

/* RenderBase(), the vptr, then the two members (raceActive is stored
 * first). */
// FUNC 0x0012C978 gt2k_menu::ShortmenuRender::ShortmenuRender
ShortmenuRender::ShortmenuRender()
    : RenderBase()
{
    raceActive = 1;
    race = NULL;
}

/* Called by ShortmenuControl::begin: the render thread draws this. */
// FUNC 0x0012C9B8 gt2k_menu::ShortmenuRender::begin
void ShortmenuRender::begin()
{
    attach();
}

/* Called by ShortmenuControl::end: the transition, then the tree released. */
// FUNC 0x0012C9D8 gt2k_menu::ShortmenuRender::end
void ShortmenuRender::end()
{
    detach();
}

/* GameObject slot 1 (index.txt: ShortmenuRender::virtual_1).  A frame of
 * the race while raceActive (RaceBase::update, a virtual call), then the
 * object tree (RootObject::virtual_1 called directly on the member: unlike
 * RenderBase::update, no pause check). */
// FUNC 0x0012C9F8 gt2k_menu::ShortmenuRender::update
void ShortmenuRender::update()
{
    if (race != NULL && raceActive != 0)
        race->update();
    root.virtual_1();
}

/* GameObject slot 2 (index.txt: ShortmenuRender::virtual_2).  The
 * per-frame screen clear off (g_00352750, its colour g_0035274C), the race
 * view under the menu (RacePS2Base::drawView with the buffer), then
 * RenderBase's drawing of the menu. */
// FUNC 0x0012CA50 gt2k_menu::ShortmenuRender::render
void ShortmenuRender::render(GSBuffer &buf)
{
    g_00352750 = 0;
    g_0035274C = 0;
    if (race != NULL)
        race->drawView(&buf);
    RenderBase::render(buf);
}

// FUNC 0x0012CAB0 gt2k_menu::ShortmenuRender::setRace
void ShortmenuRender::setRace(RacePS2Base *r)
{
    race = r;
}

// FUNC 0x0012CAB8 gt2k_menu::ShortmenuRender::clearRace
void ShortmenuRender::clearRace()
{
    race = NULL;
}

/* No callers (ShortmenuControl::mainLoop does the same store itself). */
// FUNC 0x0012CAC0 gt2k_menu::ShortmenuRender::stopRace
void ShortmenuRender::stopRace()
{
    raceActive = 0;
}

/* No callers. */
// FUNC 0x0012CAC8 gt2k_menu::ShortmenuRender::startRace
void ShortmenuRender::startRace()
{
    raceActive = 1;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base RenderBase). */
// FUNC 0x0012CAD8 __tfQ29gt2k_menu15ShortmenuRender
extern "C" void *__tfQ29gt2k_menu15ShortmenuRender(void)
{
    if (__tiQ29gt2k_menu15ShortmenuRender[0] == 0) {
        __tfQ29gt2k_menu10RenderBase();
        __rtti_si(__tiQ29gt2k_menu15ShortmenuRender, "Q29gt2k_menu15ShortmenuRender",
                  (const type_info *)__tiQ29gt2k_menu10RenderBase);
    }
    return __tiQ29gt2k_menu15ShortmenuRender;
}
#endif

// FUNC 0x0012CB28 gt2k_menu::ShortmenuRender::~ShortmenuRender
/* implicit in game/menu/race_menu/T112_shortmenu_render.h; egcs emits this copy (slot
 * [0]): the inline ~RenderBase (RootObject::~RootObject(&root, 2), then
 * GameObject::~GameObject(this, flags), which does the delete); no vptr
 * store. */
/* implicit: ~RootObject, ~GameObject (inline base destructor) */
