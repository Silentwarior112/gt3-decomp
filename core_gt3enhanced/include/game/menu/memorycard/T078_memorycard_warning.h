/* T078_memorycard_warning.h - gt2k_menu::MemorycardWarningControl, the memory
 * card warning at boot (translation unit T078, unit u032),
 * 0x0015BA98..0x0015C360.
 *
 * main (T000) runs memorycard_warning once at boot: unless a slot holds a
 * PS2 memory card with room for a game save (the "GameStatus"
 * serialisation, game_save_size 0x0015BB78, plus menu/icon/ICON_a_spec.ico
 * plus 5 KB) or one with a save already, the page "warning" of
 * "menu/memorycard.mbl" shows text_no_card / text_lack1 / text_lack2 /
 * text_lack12 and text_need ("%d" KB) until a usable card is inserted or the
 * page is left. */
#ifndef GAME_T078_MEMORYCARD_WARNING_H
#define GAME_T078_MEMORYCARD_WARNING_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pages) */

namespace gt2k_menu {

/* vtable 0x003203D8 (2 slots: [1] ControlBase's waitFrame 0x001CBD30).
 * typeinfo __tfQ29gt2k_menu24MemorycardWarningControl 0x0015C2C0 (si, base
 * ControlBase; storage 0x004B1BD0).  ControlBase's data +0x000 .. +0x288,
 * vptr +0x288.  Size 0xE40 (memorycard_warning's stack object). */
class MemorycardWarningControl : public ControlBase {
public:
    /* Inline (memorycard_warning expands it): ControlBase(), the vptr,
     * render, pages(this). */
    MemorycardWarningControl() : pages(this) {}
    /* No vptr store: render's inline ~RenderBase (the root, then
     * ~GameObject, flags 2), then ~ControlBase with the flags (which does the
     * delete); pages has no destructor. */
    /* [0] +0x0C 0x0015C310 ~MemorycardWarningControl: implicit (no vptr store) */

    void load();                            /* 0x0015BA98 pages.loadFile("menu/memorycard.mbl") */
    void unload();                          /* 0x0015BAC0 */
    /* attach(), a frame, render.attach(), pages.initialize(&render.root,
     * NULL); g_00352750 / g_0035274C (T246) cleared. */
    void begin();                           /* 0x0015BAE0 */
    void end();                             /* 0x0015BB38 */
    /* Nothing while a slot is usable; else the page "warning" with the
     * texts that fit, until a card is (the page is then left) or the page
     * is left. */
    void warningLoop();                     /* 0x0015BD00 */
    void run();                             /* 0x0015C108 load, begin, warningLoop, end, unload */

    RenderBase  render;                     /* 0x28C */
    PageManager pages;                      /* 0x2E4 */
};
EGCS_SIZE(gt2k_menu::MemorycardWarningControl, 0xE40);

} /* namespace gt2k_menu */

/* The card in port (0 slot 1, 4 slot 2): 0 usable (a PS2 card with needKB
 * + 2 KB free, an unformatted one, or one that already has the game data),
 * 1 none (no PS2 card, or an error), 2 a PS2 card without the room and
 * without the game data.  The card type and space are read with the
 * interrupts off.  Only warningLoop calls it (the plan's "used by u024" is
 * a comment of u024 naming it as a caller of a T108 function). */
int warning_check_card(int port, int needKB);   /* 0x0015BBC8 */
/* memorycard_warning: in its own header (main did not see the class). */
#include "game/menu/memorycard/T078_memorycard_warning_entry.h"

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu24MemorycardWarningControl(void); /* 0x0015C2C0 */
#endif

#endif /* GAME_T078_MEMORYCARD_WARNING_H */
