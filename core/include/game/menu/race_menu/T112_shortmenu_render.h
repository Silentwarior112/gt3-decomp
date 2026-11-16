/* T112_shortmenu_render.h - gt2k_menu::ShortmenuRender, the renderer of the
 * short menu (translation unit T112, unit u015), 0x0012C978..0x0012CB70.
 *
 * The RenderBase (T246) of ShortmenuControl (T111, its member at +0x290):
 * it also runs (update) and draws (render) the race the menu is shown
 * over, when one is set (run_short_menu's race, T102). */
#ifndef GAME_T112_SHORTMENU_RENDER_H
#define GAME_T112_SHORTMENU_RENDER_H

#include "game/game.h"
#include "game/menu/framework/T246_render_base.h"          /* base gt2k_menu::RenderBase */

class GSBuffer;                             /* T514 (references only) */
class RacePS2Base;                          /* T130 (pointers only) */

namespace gt2k_menu {

/* vtable 0x00314C18 (4 slots: [3] GameObject's 0x00101A38).  typeinfo
 * __tfQ29gt2k_menu15ShortmenuRender 0x0012CAD8 (si, base RenderBase;
 * storage 0x004B1720).  Size 0x60 (ShortmenuControl +0x290 .. +0x2F0).  The
 * destructor is implicit in the original: 0x0012CB28 does not store the
 * vptr (the inline ~RenderBase: root's ~RootObject with flags 2,
 * ~GameObject with the flags); ShortmenuControl's destructor expands it. */
class ShortmenuRender : public RenderBase {
public:
    ShortmenuRender();                      /* 0x0012C978 RenderBase(), the vptr, raceActive 1, race NULL */
    /*  [0] +0x0C 0x0012CB28 ~ShortmenuRender: implicit (see above) */
    /* A frame of the race (RaceBase::update) while raceActive, then the
     * menu tree (root.virtual_1(), not RenderBase's: no pause check). */
    virtual void update();                  /* [1] +0x14 0x0012C9F8 */
    /* No screen clear (g_00352750 / g_0035274C = 0), the race view
     * (RacePS2Base virtual_44) under the menu, then RenderBase's. */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x0012CA50 */

    void begin();                           /* 0x0012C9B8 attach() (ShortmenuControl::begin) */
    void end();                             /* 0x0012C9D8 detach() (ShortmenuControl::end) */
    void setRace(RacePS2Base *r);           /* 0x0012CAB0 */
    void clearRace();                       /* 0x0012CAB8 race = NULL */
    void stopRace();                        /* 0x0012CAC0 raceActive = 0 (no callers: mainLoop stores it
                                               itself) */
    void startRace();                       /* 0x0012CAC8 raceActive = 1 (no callers) */

    RacePS2Base *race;                      /* 0x58 the race behind the menu, NULL: none */
    int          raceActive;                /* 0x5C non-zero: update() runs the race too */
};
EGCS_SIZE(gt2k_menu::ShortmenuRender, 0x60);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu15ShortmenuRender(void);     /* 0x0012CAD8 */
extern u32 __tiQ29gt2k_menu15ShortmenuRender[3];   /* 0x004B1720 its __si_type_info */
}
#endif

#endif /* GAME_T112_SHORTMENU_RENDER_H */
