/* T103_shortmenu_license.h - gt2k_menu::ShortmenuLicenseControl /
 * ShortmenuLicenseRender, the licence-test variant of the short menu
 * (translation unit T103, unit u018), 0x00134630..0x00134EE8.
 *
 * A stack object of run_short_menu_license (T102, u015), the licence test's
 * short menu (u015's ShortmenuControl / ShortmenuRender, T111 / T112, are the
 * plain one).  The race behind the menu is started in replay mode
 * (RaceBase::virtual_8(replayMode)) and restarted the same way whenever it
 * signals its end (updateRace, once a frame); the render draws it in a
 * 280 x 210 sub-window of the frame (RacePS2Base::drawSubView) and nothing at
 * all while `hidden' (until the first page is open).  The page tables are
 * the short menu's (T111 / T113 .data, u015). */
#ifndef GAME_T103_SHORTMENU_LICENSE_H
#define GAME_T103_SHORTMENU_LICENSE_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T246_render_base.h"          /* base gt2k_menu::RenderBase */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pm) */

class GSBuffer;                             /* T514 (references only) */
class RacePS2Base;                          /* T130 (pointers only) */

namespace gt2k_menu {

/* vtable 0x003168F0 (4 slots: [3] GameObject's 0x00101A38).  typeinfo
 * __tfQ29gt2k_menu22ShortmenuLicenseRender 0x00134E50 (si, base
 * RenderBase; storage 0x004B1770).  vptr +0x00; size 0x60
 * (ShortmenuLicenseControl +0x290). */
class ShortmenuLicenseRender : public RenderBase {
public:
    ShortmenuLicenseRender();               /* 0x00134630 RenderBase(), hidden 1, race NULL */
    /* ShortmenuLicenseControl's (implicit) destructor expands it;
     * 0x00134EA0 is the copy for the vtable. */
    /* [0] +0x0C 0x00134EA0 ~ShortmenuLicenseRender: implicit (no vptr store) */
    /* Only the object tree (root's update); the race runs in
     * ShortmenuLicenseControl::updateRace. */
    virtual void update();                  /* [1] +0x14 0x001346B0 */
    /* Nothing while hidden; else the race (when its field_978C is set) in a
     * (32, 58, 280 x 210) copy of buf with its +0xC6F4 block {80, 10, 120,
     * 32, 1.3}, then RenderBase::render (the menu). */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x001346D0 */
    void begin();                           /* 0x00134670 attach() */
    void end();                             /* 0x00134690 detach() */
    void setRace(RacePS2Base *r);           /* 0x00134860 */
    void clearRace();                       /* 0x00134868 */
    /* Inline in the original: no callers, 0x00134E38..0x00134E48 are the
     * out-of-line copies egcs emits before the typeinfo function;
     * ShortmenuLicenseControl::run expands them (retail keeps &render in
     * an s-register and reads race / stores hidden through it). */
    RacePS2Base *getRace() { return race; }         /* 0x00134E38 */
    int  isHidden() { return hidden; }              /* 0x00134E40 */
    void setHidden(int v) { hidden = v; }           /* 0x00134E48 */

    RacePS2Base *race;                      /* 0x58 the race behind the menu, NULL none */
    int          hidden;                    /* 0x5C non-zero: render() draws nothing (? name) */
};
EGCS_SIZE(gt2k_menu::ShortmenuLicenseRender, 0x60);

/* vtable 0x003168D8 (2 slots: [1] ControlBase::waitFrame).  typeinfo
 * __tfQ29gt2k_menu23ShortmenuLicenseControl 0x00134D98 (si, base
 * ControlBase; storage 0x004B1740).  vptr +0x288 (ControlBase's); size
 * 0xE58 (a stack object of run_short_menu_license). */
class ShortmenuLicenseControl : public ControlBase {
public:
    /* ControlBase, render, pm(this); heap NULL, replayMode 2, startPage
     * NULL (bgmGroup is not set). */
    ShortmenuLicenseControl();              /* 0x00134870 */
    /* run_short_menu_license expands it (render, then ~ControlBase);
     * 0x00134DE8 is the copy for the vtable. */
    /* [0] +0x0C 0x00134DE8 ~ShortmenuLicenseControl: implicit (no vptr store) */
    void setHeap(void *mem, int size);      /* 0x00134928 pushes the menu heap (a pushed one is popped first) */
    void releaseHeap();                     /* 0x00134980 */
    /* The page file, the item checks of the short menu's page "root" (T111
     * g_0029C2C8) as the selectable table. */
    void loadPages(const char *file);       /* 0x001349A8 */
    void unloadPages();                     /* 0x00134A18 */
    void begin();                           /* 0x00134A48 the qm_* actions and item factories (T113) */
    void end();                             /* 0x00134B10 */
    void mainLoop();                        /* 0x00134B50 the page (and startPage over it) until left */
    void updateRace();                      /* 0x00134C30 one frame of the race; restarted at its end */
    /* The race started in replayMode and one frame of it, then the page:
     * begin, setPage(page, arg), mainLoop, end; the race stopped. */
    void run(const char *page, const char *arg);    /* 0x00134CD8 */

    /*                     ControlBase         0x000 .. 0x28C (vptr +0x288) */
    void                  *heap;            /* 0x28C the menu heap pushed by setHeap, NULL none */
    ShortmenuLicenseRender render;          /* 0x290 */
    PageManager            pm;              /* 0x2F0 */
    const char            *startPage;       /* 0xE4C a page opened over the first one, NULL none */
    int                    bgmGroup;        /* 0xE50 ShortMenuParams::bgmGroup: the music
                                               (menu_bgm_play_if_changed(bgmGroup, 0, 1); first pass: mode) */
    int                    replayMode;      /* 0xE54 RaceBase::virtual_8's argument: 2 (constructor), 1 when
                                               run_short_menu_license's flag is set */
};
EGCS_SIZE(gt2k_menu::ShortmenuLicenseControl, 0xE58);

} /* namespace gt2k_menu */

/* T103's .sdata copy of the size of the short menu's selectable table. */
extern int g_00351440;                      /* 0x00351440 gp-0x65B0 1, the entries of T111's g_0029C2C8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu23ShortmenuLicenseControl(void);  /* 0x00134D98 */
void *__tfQ29gt2k_menu22ShortmenuLicenseRender(void);   /* 0x00134E50 */
}
#endif

#endif /* GAME_T103_SHORTMENU_LICENSE_H */
