/* T011 - gt2k_menu::ArcadeRender.  0x00115618..0x001156F8.
 *
 * The arcade menu's renderer: the background movie frame (g_002CDC10) under
 * the normal RenderBase drawing.
 *
 * Retail's .rodata (0x0030DC10..0x0030DC58) is ArcadeRender's vtable and
 * type name only (its own header and the movie); the vtable is referenced
 * only by T010's constructor, hence the anchor. */
// DATA_START 0x0030DC10
#include "game/menu/arcade/T011_arcade_render.h"
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "cxxrt.h"                          /* __rtti_si (GCC 14's typeinfo function) */
#include "game/menu/arcade/arcade_control_internal.h"

namespace gt2k_menu {

/* GameObject slot 2 (index.txt: ArcadeRender::virtual_2). */
// FUNC 0x00115618 gt2k_menu::ArcadeRender::render
void ArcadeRender::render(GSBuffer &buf)
{
    g_002CDC10.update();
    RenderBase::render(buf);
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base RenderBase). */
// FUNC 0x00115660 __tfQ29gt2k_menu12ArcadeRender
extern "C" void *__tfQ29gt2k_menu12ArcadeRender(void)
{
    if (__tiQ29gt2k_menu12ArcadeRender[0] == 0) {
        __tfQ29gt2k_menu10RenderBase();
        __rtti_si(__tiQ29gt2k_menu12ArcadeRender, "Q29gt2k_menu12ArcadeRender",
                  (const type_info *)__tiQ29gt2k_menu10RenderBase);
    }
    return __tiQ29gt2k_menu12ArcadeRender;
}
#endif

// FUNC 0x001156B0 gt2k_menu::ArcadeRender::~ArcadeRender
/* implicit in game/menu/arcade/T011_arcade_render.h; egcs emits this copy: the inline
 * ~RenderBase (RootObject::~RootObject(&root, 2), then
 * GameObject::~GameObject(this, flags), which does the delete); no vptr
 * store. */
/* implicit: ~RootObject, ~GameObject (inline ~RenderBase) */
