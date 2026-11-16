/* T090 - gt2k_menu::RecordsControl: the records screen
 * (game/menu/records/T090_records_control.h).  0x00172D58..0x001731B0.
 *
 * run_records_menu (callers: run_free_run, run_time_trial, run_machine_test,
 * run_license_test: the short menus of T052 / T053) pushes a 0x80000-byte
 * menu heap on the stack, runs a RecordsControl over "menu/records.mbl", page
 * "records_free_tt" (whatever the kind of records), then pops the heap. */
/* Retail's .rodata (0x003251C8..0x00325390): "bad_alloc" (<new>'s inline
 * what(), written when the header is read), the literals, the vtable, then
 * the type names in the reverse order of the classes' definitions:
 * RecordsControl, UnitArena<64>, RegurationResult, RecordsMenuArg, the
 * GameStatus's garage lists, SmartIndexT<int>. */
// DATA_START 0x003251C8
#include <new>
#include "game/menu/framework/T211_page_manager.h"         /* PageManager (pm); SmartIndexT<int> */
#include "game/save/T429_game_status.h"          /* (seen, not used) */
#include "game/menu/records/T089_records_menu_args.h"    /* RecordsMenuArg */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen, not used) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/menu/records/T090_records_control.h"
#include "game/menu/records/T091_records_items.h"        /* the menu tables g_00351C88 .. g_00351CA4, g_002B1450 */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/records/records_internal.h"                           /* RECORDS_HEAP_SIZE */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

namespace gt2k_menu {

// FUNC 0x00172D58 gt2k_menu::RecordsControl::loadPages
void RecordsControl::loadPages()
{
    pm.loadFile("menu/records.mbl");
}

// FUNC 0x00172D80 gt2k_menu::RecordsControl::unloadPages
void RecordsControl::unloadPages()
{
    pm.unloadFile();
}

/* The pads attached, one frame (waitFrame, virtual), the renderer on the
 * render thread, the object tree to the PageManager; the PageManager gets
 * the "menu_records" factory (its MenuFuncData) and the "delete" action
 * (its ExecuteFuncData), both layouts the "entry_code1..3" hooks
 * (g_002B1450), all with this as their context; the per-frame screen clear
 * off.  The PageManager's inline setters (retail keeps &pm in $s1, the hook
 * count in $s3 for both layouts); T091's tables are declared unsized
 * (game/menu/records/T091_records_items.h), so their addresses are lui/addiu, not
 * $gp-relative, as retail. */
// FUNC 0x00172DA0 gt2k_menu::RecordsControl::begin
void RecordsControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    pm.initialize(&render.root, NULL);
    pm.setMenuFuncData(g_00351C88, g_00351C90, this);
    pm.setExecuteFuncData(g_00351C98, g_00351CA0, this);
    pm.setHooks(g_002B1450, g_00351CA4, this);
    g_00352750 = 0;                         /* no per-frame screen clear */
    g_0035274C = 0;
}

/* The pages released, two vsyncs, the renderer off the render thread, the
 * pads detached. */
// FUNC 0x00172E78 gt2k_menu::RecordsControl::end
void RecordsControl::end()
{
    pm.finalize();
    gfx_vsync(2);
    render.detach();
    detach();
}

/* The page "records_free_tt" until the PageManager is left, one frame of
 * input (update) and one waitFrame (virtual) at a time. */
// FUNC 0x00172EB8 gt2k_menu::RecordsControl::mainLoop
void RecordsControl::mainLoop()
{
    pm.setPage("records_free_tt", NULL);
    pm.startPage(NULL);
    while (pm.exited == 0) {
        pm.update();
        waitFrame();
    }
    pm.closePage();
}

// FUNC 0x00172F48 gt2k_menu::RecordsControl::run
void RecordsControl::run()
{
    loadPages();
    begin();
    mainLoop();
    end();
    unloadPages();
}

} /* namespace gt2k_menu */

/* The records screen of `records'.  The control is a stack object
 * (sp+0x80000) above the menu heap (sp+0); its inline constructor is
 * ControlBase::ControlBase, the vptr, arg, RenderBase::RenderBase,
 * PageManager::PageManager(this) (the landing pads: render's ~RootObject and
 * GameObject::~GameObject, ControlBase::~ControlBase); its implicit
 * destructor (the same three) runs after the final vsync.  Returns 1. */
// FUNC 0x00172F90 run_records_menu
int run_records_menu(gt2k_menu::RecordsMenuArg *records)
{
    u8 heap[RECORDS_HEAP_SIZE];             /* sp+0x00000 */

    menu_draw_reset();
    menu_heap_push(heap, RECORDS_HEAP_SIZE);
    {
        /* sp+0x80000: ControlBase::ControlBase, the vptr, arg,
         * RenderBase::RenderBase, PageManager::PageManager(this); at the end
         * of the scope ~RootObject, GameObject::~GameObject and
         * ControlBase::~ControlBase (flags 2) */
        /* implicit: ControlBase, RenderBase, PageManager (the inline
         * constructor), ~GameObject (the implicit destructor at the end of the
         * scope) */
        /* implicit: ~RootObject (render.root, the implicit destructor) */
        gt2k_menu::RecordsControl control(records);

        control.run();
        gfx_vsync(1);
    }
    menu_heap_pop();
    return 1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00173110 __tfQ29gt2k_menu14RecordsControl
extern "C" void *__tfQ29gt2k_menu14RecordsControl(void)
{
    if (__tiQ29gt2k_menu14RecordsControl[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu14RecordsControl, "Q29gt2k_menu14RecordsControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu14RecordsControl;
}
#endif

// FUNC 0x00173160 gt2k_menu::RecordsControl::~RecordsControl
/* implicit in game/menu/records/T090_records_control.h; egcs emits this copy (slot 0):
 * render's inline destructor (~RootObject of render.root with flags 2,
 * GameObject::~GameObject of render with flags 2), then
 * ControlBase::~ControlBase with this destructor's flags; the PageManager
 * has no destructor. */
/* implicit: ~GameObject, ~RootObject, ~ControlBase (the base / member
 * destructors) */
