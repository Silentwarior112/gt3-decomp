/* T111_shortmenu_control.h - gt2k_menu::ShortmenuControl, the controller of
 * the short menu (translation unit T111, unit u015), 0x0012C300..0x0012C978.
 *
 * A stack object of run_short_menu (T102): loadPages gives its PageManager
 * the availability checks of page "root" (SelectableFuncData g_0029C2C8),
 * begin the qm_* actions (ExecuteFuncData g_0029C6B0, T113) and both
 * layouts the qm_* hooks (MenuHook g_0029C758, T113), run opens a page and
 * mainLoop runs it until the page manager is done.  The ShortmenuRender at
 * +0x290 draws and runs the race behind the menu.  The menu heap is pushed
 * by setHeap (memory of the caller's stack). */
#ifndef GAME_T111_SHORTMENU_CONTROL_H
#define GAME_T111_SHORTMENU_CONTROL_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/race_menu/T112_shortmenu_render.h"     /* gt2k_menu::ShortmenuRender (render) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pm), SelectableFuncData */

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */

/* vtable 0x00314890 (2 slots: [1] ControlBase's waitFrame 0x001CBD30).
 * typeinfo __tfQ29gt2k_menu16ShortmenuControl 0x0012C8D8 (si, base
 * ControlBase; storage 0x004B1310).  ControlBase's data +0x000 .. +0x288,
 * vptr +0x288.  Size 0xE50 (run_short_menu's frame).  The destructor is
 * implicit in the original: 0x0012C928 does not store the vptr (render's
 * inline destructor, then ~ControlBase with the flags; pm has none);
 * run_short_menu expands it. */
class ShortmenuControl : public ControlBase {
public:
    /* ControlBase(), the vptr, render, pm(this); heap, startPage NULL. */
    ShortmenuControl();                     /* 0x0012C300 */
    /*  [0] +0x0C 0x0012C928 ~ShortmenuControl: implicit (see above) */

    /* Pushes the menu heap mem (size bytes), popping the one pushed before. */
    void setHeap(void *mem, int size);      /* 0x0012C3B0 */
    void releaseHeap();                     /* 0x0012C408 pops it */
    /* pm.initialize(&render.root, NULL), pm.loadFile(file), the "root"
     * checks with this as context. */
    void loadPages(const char *file);       /* 0x0012C5C8 */
    void unloadPages();                     /* 0x0012C638 pm.finalize(), pm.unloadFile() */
    /* attach(), a frame, render.begin() (its detach transition: a
     * fade-out), the actions and the hooks (context this). */
    void begin();                           /* 0x0012C668 */
    void end();                             /* 0x0012C730 */
    /* Builds the page set by run (pm.startPage) and startPage as a popup
     * over it, then the page loop (pm.update, a frame) until pm.exited;
     * then the race stops (render.raceActive = 0) and the render thread
     * draws nothing. */
    void mainLoop();                        /* 0x0012C770 */
    /* begin, pm.setPage(page, arg), mainLoop, end.  arg: the page imported
     * from the other layout (LinkManager::setImportPage; NULL: none), which
     * T211 declares as an int. */
    void run(const char *page, const char *arg);    /* 0x0012C848 */
    /* (? name) Inline, no out-of-line copy: mainLoop stores raceActive
     * through it (retail keeps this + 0x290 in $s1 from before the page
     * loop), as the other controls' getRender (GtmodeControl). */
    ShortmenuRender *getRender() { return &render; }

    void           *heap;                 /* 0x28C the menu heap pushed by setHeap, NULL: none */
    ShortmenuRender render;                 /* 0x290 */
    PageManager     pm;                     /* 0x2F0 */
    const char     *startPage;              /* 0xE4C a page opened over the first one (e.g. "ghost_dlg"),
                                               NULL: none */
};
EGCS_SIZE(gt2k_menu::ShortmenuControl, 0xE50);

} /* namespace gt2k_menu */

/* The SelectableFuncData of a ShortmenuControl (T111 .data; one entry,
 * {"root", g_00314848, 18}: 18 x shortmenu_root_check).  T103's
 * ShortmenuLicenseControl uses it too (count g_00351440). */
extern gt2k_menu::SelectableFuncData g_0029C2C8[1];    /* 0x0029C2C8 */

/* The availability of the buttons of page "root" from the running
 * session's ShortMenuArgs (g_003514F0): "replay" / "save_replay"
 * need canReplay, "save_ghost" canSaveGhost, "data_logger" canLogger;
 * noSetting hides "setting" / "save_setting" / "load_setting", noExit
 * "exit" / "save_game", noFreeSession "free_session".  ctx is not used.
 * (The first pass took the record for its name: const char *item.) */
int  shortmenu_root_check(void *ctx, gt2k_menu::ButtonData *data);  /* 0x0012C430 */
/* The static initialiser of g_0029C2C8 (.ctors entry 0x0028F0D8; the
 * compiler's _GLOBAL_$I$ function in the original). */
void T111_static_init(void);                /* 0x0012C8A8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu16ShortmenuControl(void);    /* 0x0012C8D8 */
extern u32 __tiQ29gt2k_menu16ShortmenuControl[3];  /* 0x004B1310 its __si_type_info */
}
#endif

#endif /* GAME_T111_SHORTMENU_CONTROL_H */
