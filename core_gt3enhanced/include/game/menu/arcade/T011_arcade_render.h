/* T011_arcade_render.h - gt2k_menu::ArcadeRender, the renderer of the
 * arcade menu (translation unit T011, unit u007), 0x00115618..0x001156F8.
 *
 * The RenderBase (T246) of ArcadeControl (its member render at +0x28C):
 * draws the background movie frame (MenuBgMovie g_002CDC10, T253) under the
 * menu tree. */
#ifndef GAME_T011_ARCADE_RENDER_H
#define GAME_T011_ARCADE_RENDER_H

#include "game/game.h"
#include "game/menu/framework/T246_render_base.h"          /* base gt2k_menu::RenderBase */

class GSBuffer;                             /* T514 (references only) */

namespace gt2k_menu {

/* vtable 0x0030DC10 (4 slots: [1] RenderBase's update 0x001CB870, [3]
 * GameObject's 0x00101A38).  typeinfo __tfQ29gt2k_menu12ArcadeRender
 * 0x00115660 (si, base RenderBase; storage 0x004B1510).  No data of its own:
 * size 0x58 (ArcadeControl+0x28C .. +0x2E4).  The destructor is implicit in
 * the original: 0x001156B0 does not store the vptr (egcs stores it in every
 * user-declared destructor, not in the one it generates for a class with
 * one base): the inline ~RenderBase (~RootObject, ~GameObject with the
 * flags). */
class ArcadeRender : public RenderBase {
public:
    /*  ArcadeRender(): implicit (ArcadeControl's constructor expands it:
     *  RenderBase(), then the vptr).
     *  [0] +0x0C 0x001156B0 ~ArcadeRender: implicit (see above) */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x00115618 the movie frame (MenuBgMovie
                                               F_001CD1E0), then RenderBase's */
};
EGCS_SIZE(gt2k_menu::ArcadeRender, 0x58);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu12ArcadeRender(void); /* 0x00115660 */
extern u32 __tiQ29gt2k_menu12ArcadeRender[3];   /* 0x004B1510 its __si_type_info */
}
#endif

#endif /* GAME_T011_ARCADE_RENDER_H */
