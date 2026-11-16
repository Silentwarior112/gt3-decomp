/* T046_gtmode_render.h - gt2k_menu::GtmodeRender, the renderer of the GT
 * mode menu (translation unit T046, unit u013), 0x00127368..0x001274E0.
 *
 * The RenderBase (T246) of GtmodeControl (T043): every frame it takes the
 * menu background movie's next frame (MenuBgMovie g_002CDC10, T253
 * update) and then draws the menu tree like RenderBase. */
#ifndef GAME_T046_GTMODE_RENDER_H
#define GAME_T046_GTMODE_RENDER_H

#include "game/game.h"
#include "game/menu/framework/T246_render_base.h"          /* base gt2k_menu::RenderBase */

class GSBuffer;                             /* T514 (references only) */

namespace gt2k_menu {

/* vtable 0x00312588 (4 slots: [3] GameObject's 0x00101A38).  typeinfo
 * __tfQ29gt2k_menu12GtmodeRender 0x00127448 (si, base RenderBase; storage
 * 0x004B15F0).  No data of its own: size 0x58.  The destructor is implicit
 * in the original: 0x00127498 does not store the vptr (egcs stores it in
 * every user-declared destructor): the inline ~RenderBase (root's
 * ~RootObject with flags 2, ~GameObject with the flags). */
class GtmodeRender : public RenderBase {
public:
    GtmodeRender();                         /* 0x00127368 RenderBase(), then the vptr (caller: the
                                               GtmodeControl constructor, T043) */
    /*  [0] +0x0C 0x00127498 ~GtmodeRender: implicit (see above) */
    virtual void update();                  /* [1] +0x14 0x001273E0 RenderBase's */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x00127400 the movie frame (MenuBgMovie
                                               update), then RenderBase's */

    /* (? names: ShortmenuRender's begin / end, T112, are the same code.) */
    void begin();                           /* 0x001273A0 RenderBase::attach() (no callers) */
    void end();                             /* 0x001273C0 RenderBase::detach() (GtmodeControl::end) */
};
EGCS_SIZE(gt2k_menu::GtmodeRender, 0x58);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu12GtmodeRender(void);     /* 0x00127448 */
#endif

#endif /* GAME_T046_GTMODE_RENDER_H */
