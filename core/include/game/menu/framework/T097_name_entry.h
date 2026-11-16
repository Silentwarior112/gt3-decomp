/* T097_name_entry.h - name_entry and gt2k_menu::NameEntryControl, the name
 * entry screen (translation unit T097, unit u013), 0x00128338..0x001287A8.
 *
 * name_entry(name) runs the page "name_entry" of "menu/name_entry.mbl" with
 * a 128 KB menu heap (run_text_input, T227): a text box ("text_box", hook
 * create_text_box, T231) typed into by the key buttons ("input_text",
 * ExecuteFuncData text_box_key), starting from the player's name
 * (GameStatus::lastInputedName, +0x78, at most 15 characters).  The
 * result goes to `name' and back to the player's name.
 * The page tables (g_0049BCA0 MenuHook[1], g_00353208 ExecuteFuncData[1])
 * are used only by T097. */
#ifndef GAME_T097_NAME_ENTRY_H
#define GAME_T097_NAME_ENTRY_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pm) */

namespace gt2k_menu {

/* vtable 0x003132D0 (2 slots: [1] ControlBase's waitFrame 0x001CBD30).
 * typeinfo __tfQ29gt2k_menu16NameEntryControl 0x00128708 (si, base
 * ControlBase; storage 0x004B1620).  ControlBase's data +0x000 .. +0x288,
 * vptr +0x288.  Size 0xE40 (name_entry's stack object).  The destructor
 * is implicit in the original: 0x00128758 does not store the vptr
 * (render's inline ~RenderBase, then ~ControlBase with the flags; pm has
 * no destructor). */
class NameEntryControl : public ControlBase {
public:
    /* Inline (name_entry expands it): ControlBase(), the vptr, render,
     * pm(this). */
    NameEntryControl() : pm(this) {}
    /*  [0] +0x0C 0x00128758 ~NameEntryControl: implicit (see above) */

    void loadPages();                       /* 0x00128338 pm.loadFile("menu/name_entry.mbl") */
    void unloadPages();                     /* 0x00128360 */
    /* attach(), a frame, render.attach(), pm.initialize(&render.root,
     * NULL), the hook table to both layouts and the execute table, both
     * with this as context. */
    void begin();                           /* 0x00128380 */
    void end();                             /* 0x00128430 */
    /* The page, then the entered name becomes the player's name. */
    void runPage(char *name);               /* 0x00128470 */
    void run(char *name);                   /* 0x001284E8 loadPages, begin, runPage, end, unloadPages */

    RenderBase  render;                     /* 0x28C */
    PageManager pm;                         /* 0x2E4 */
};
EGCS_SIZE(gt2k_menu::NameEntryControl, 0xE40);

} /* namespace gt2k_menu */

/* name_entry, the name entry screen: in its own header (the race modes'
 * T052 did not see NameEntryControl: u016's data pass). */
#include "game/menu/framework/T097_name_entry_screen.h"
/* The static initialiser of the two page tables (.ctors entry 0x0028F0D0;
 * the compiler's _GLOBAL_$I$ function in the original). */
void T097_static_init(void);                /* 0x001286B8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu16NameEntryControl(void); /* 0x00128708 */
#endif

#endif /* GAME_T097_NAME_ENTRY_H */
