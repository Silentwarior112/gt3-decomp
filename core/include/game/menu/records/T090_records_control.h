/* T090_records_control.h - gt2k_menu::RecordsControl, the records screen
 * (translation unit T090, unit u040), 0x00172D58..0x001731B0.
 *
 * run_records_menu (the race modes' short menus: T052 / T053) pushes a
 * 0x80000-byte menu heap on its stack and runs a RecordsControl over the
 * records `arg' (T089): "menu/records.mbl", page "records_free_tt" whatever
 * the kind of records, with T091's "menu_records" list factory, "delete"
 * action and "entry_code1..3" texts; then pops the heap. */
#ifndef GAME_T090_RECORDS_CONTROL_H
#define GAME_T090_RECORDS_CONTROL_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render, held by value) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pm, held by value) */

namespace gt2k_menu {
class RecordsMenuArg;                       /* T089 (pointers only) */

/* vtable 0x00325200 (2 slots: [1] ControlBase::waitFrame).  typeinfo
 * __tfQ29gt2k_menu14RecordsControl 0x00173110 (si, base ControlBase;
 * __si_type_info 0x004B1D80).  vptr +0x288 (ControlBase's); size 0xE48 (a
 * stack object of run_records_menu, above its menu heap). */
class RecordsControl : public ControlBase {
public:
    /* Inline in run_records_menu (no out-of-line copy): ControlBase(), the
     * vptr, arg, render, pm(this); index is not set. */
    RecordsControl(RecordsMenuArg *a) : arg(a), render(), pm(this) {}
    /* render's destructor (inline: ~RootObject, ~GameObject), then
     * ~ControlBase; the PageManager has no destructor. */
    /* [0] +0x0C 0x00173160 ~RecordsControl: implicit (no vptr store) */

    void loadPages();                       /* 0x00172D58 pm.loadFile("menu/records.mbl") */
    void unloadPages();                     /* 0x00172D80 */
    /* attach, one waitFrame (virtual), the renderer attached, the object
     * tree to pm; pm's MenuFuncData g_00351C88 and ExecuteFuncData
     * g_00351C98 (ctx this), both layouts' hooks g_002B1450; the screen
     * clear off (g_00352750 = g_0035274C = 0). */
    void begin();                           /* 0x00172DA0 */
    void end();                             /* 0x00172E78 pm.finalize, 2 vsyncs, render detached, detach */
    /* The page "records_free_tt" until pm is done: one pm frame and one
     * waitFrame at a time. */
    void mainLoop();                        /* 0x00172EB8 */
    void run();                             /* 0x00172F48 loadPages, begin, mainLoop, end, unloadPages */

    /*              ControlBase                0x000 .. 0x28C (vptr +0x288) */
    RecordsMenuArg *arg;                    /* 0x28C the records shown */
    int             index;                  /* 0x290 the place the panel and "delete" act on
                                               (RecordsButtonList::openPanel) */
    RenderBase      render;                 /* 0x294 */
    PageManager     pm;                     /* 0x2EC */
};
EGCS_SIZE(gt2k_menu::RecordsControl, 0xE48);

} /* namespace gt2k_menu */

/* run_records_menu, the records screen: in its own header (the race modes'
 * T052 did not see RecordsControl: u016's data pass). */
#include "game/menu/records/T090_records_entry.h"

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand
 * declaration). */
extern "C" {
void *__tfQ29gt2k_menu14RecordsControl(void);   /* 0x00173110 */
extern u32 __tiQ29gt2k_menu14RecordsControl[3]; /* 0x004B1D80 (__si_type_info) */
}
#endif

#endif /* GAME_T090_RECORDS_CONTROL_H */
