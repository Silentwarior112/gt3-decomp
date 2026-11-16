/* T097 - gt2k_menu::NameEntryControl, the name entry screen.
 * 0x00128338..0x001287A8.
 *
 * name_entry(name) runs the page "name_entry" of "menu/name_entry.mbl" with
 * a 128 KB menu heap: a text box ("text_box", create_text_box, T231) typed
 * into by the key buttons ("input_text", text_box_key), starting from the
 * player's name (GameStatus::lastInputedName, at most 15 characters).  The
 * result goes to `name' and back to the player's name.
 *
 * Data: .rodata 0x00313278..0x00313536: <new>'s "bad_alloc" (unreferenced:
 * DATA_START), the functions' strings, the vtable, then the type names
 * (reverse definition order): NameEntryControl, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>, UnitArena<64>,
 * RegurationResult, RecordsMenuArg, SmartIndexT<int>, the garage's eight,
 * Operator.  So <new>, the GameStatus, SmartIndexT<int> (T251),
 * RecordsMenuArg (T089), RegurationResult, T153's UnitArena<64>, the
 * operator classes (T216), NameEntryControl last.  The two page tables are
 * file statics (.bss / .sbss). */
// DATA_START 0x00313278
#include <new>                              /* (seen: "bad_alloc", T097's first .rodata) */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::lastInputedName */
#include "game/common/T251_smart_index.h"          /* (seen: SmartIndexT<int>) */
#include "game/menu/records/T089_records_menu_args.h"    /* (seen: RecordsMenuArg) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#define GAME_T153_EMITS_TYPEINFO            /* (seen: UnitArena<64>'s type name) */
#include "game/race/core/T153_race_course.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen: the operator classes) */
#include "game/menu/framework/T211_page_manager.h"         /* PageManager, ExecuteFuncData */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout::setHooks, MenuHook */
#include "game/menu/framework/T227_text_input.h"           /* run_text_input */
#include "game/menu/framework/T231_text_box.h"             /* create_text_box, text_box_key */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push / pop */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "engine/pdistd/T532_heap.h"                 /* memalign, free */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si (GCC 14's __tf) */
#include "game/menu/framework/T097_name_entry.h"           /* NameEntryControl: the last class defined */
#include "game/menu/gtmode/gtmode_main_internal.h"

/* The two one-entry tables of the name entry page, filled by
 * T097_static_init: the hook {"text_box", create_text_box} of both layouts
 * and the execute table {"input_text", text_box_key}.  File statics: retail
 * has them in .bss / .sbss (a C++ global without an initialiser would be
 * zeros in .data). */
static gt2k_menu::MenuHook        g_0049BCA0[1];   /* 0x0049BCA0 */
static gt2k_menu::ExecuteFuncData g_00353208[1];   /* 0x00353208 gp-0x47E8 */

namespace gt2k_menu {

/* Caller: run. */
// FUNC 0x00128338 gt2k_menu::NameEntryControl::loadPages
void NameEntryControl::loadPages()
{
    pm.loadFile("menu/name_entry.mbl");
}

/* Caller: run. */
// FUNC 0x00128360 gt2k_menu::NameEntryControl::unloadPages
void NameEntryControl::unloadPages()
{
    pm.unloadFile();
}

/* The pads, a frame (the virtual waitFrame, ControlBase's), the render
 * thread draws render, the PageManager on render.root without a cursor
 * memory; the hook table g_0049BCA0 to both layouts and the execute table
 * g_00353208, each of one entry, with this as their context; no screen
 * clear (T261). */
// FUNC 0x00128380 gt2k_menu::NameEntryControl::begin
void NameEntryControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    pm.initialize(&render.root, NULL);
    pm.common.setHooks(g_0049BCA0, 1, this);
    pm.own.setHooks(g_0049BCA0, 1, this);
    pm.setExecuteFuncData(g_00353208, 1, this);
    g_00352750 = 0;
    g_0035274C = 0;
}

/* Caller: run. */
// FUNC 0x00128430 gt2k_menu::NameEntryControl::end
void NameEntryControl::end()
{
    pm.finalize();
    gfx_vsync(2);
    render.detach();
    detach();
}

/* The text input page, starting from the player's name; the entered name
 * (into name) then becomes the player's name.  The page's result is not
 * used. */
// FUNC 0x00128470 gt2k_menu::NameEntryControl::runPage
void NameEntryControl::runPage(char *name)
{
    run_text_input(&pm, "name_entry", getGameStatus()->lastInputedName, name,
                   NAME_ENTRY_MAX_LEN, NULL, NULL);
    strcpy(getGameStatus()->lastInputedName, name);
}

/* Caller: name_entry. */
// FUNC 0x001284E8 gt2k_menu::NameEntryControl::run
void NameEntryControl::run(char *name)
{
    loadPages();
    begin();
    runPage(name);
    end();
    unloadPages();
}

} /* namespace gt2k_menu */

/* Callers: run_free_run, run_time_trial (T052), machine_test_new_record,
 * run_license_test (T053).  The NameEntryControl's inline constructor
 * (ControlBase(), the vptr, RenderBase(), PageManager(this)) and implicit
 * destructor (render.root, render's GameObject, ControlBase, flags 2) are
 * expanded here, with the constructor's landing pads. */
// FUNC 0x00128540 name_entry
void name_entry(char *name)
{
    menu_draw_reset();
    void *heap = memalign(MENU_HEAP_ALIGN, NAME_ENTRY_HEAP_SIZE);
    menu_heap_push(heap, NAME_ENTRY_HEAP_SIZE);
    {
        /* implicit: ControlBase, RenderBase, PageManager (the inline NameEntryControl constructor), ~RootObject, ~GameObject (the implicit destructor and the exception cleanup) */
        gt2k_menu::NameEntryControl ctl;    /* sp+0x000 */
        ctl.run(name);
        gfx_vsync(1);
    }
    menu_heap_pop();
    free(heap);
}

/* Static initialiser (.ctors entry 0x0028F0D0): the two one-entry tables of
 * the name entry page.  Compiler-generated in the original (_GLOBAL_$I$...:
 * the function pointers are not constant initialisers); written out here
 * because the sources define no data. */
// FUNC 0x001286B8 T097_static_init
void T097_static_init(void)
{
    g_0049BCA0[0].name   = "text_box";
    g_0049BCA0[0].create = gt2k_menu::create_text_box;
    g_0049BCA0[0].p8     = 0;
    g_0049BCA0[0].pC     = 0;
    g_0049BCA0[0].p10    = 0;
    g_0049BCA0[0].p14    = 0;
    g_00353208[0].name   = "input_text";
    g_00353208[0].func   = gt2k_menu::text_box_key;
}

#ifndef BOOT_OLD_GCC
/* Builds the __si_type_info of NameEntryControl (base ControlBase, whose
 * node is __tiQ29gt2k_menu11ControlBase gp-0x45F8) on the first call. */
// FUNC 0x00128708 __tfQ29gt2k_menu16NameEntryControl
extern "C" void *__tfQ29gt2k_menu16NameEntryControl(void)
{
    if (__tiQ29gt2k_menu16NameEntryControl[0] == 0) {   /* _name not set yet */
        __tfQ29gt2k_menu11ControlBase();                /* the base's node first */
        __rtti_si(__tiQ29gt2k_menu16NameEntryControl, "Q29gt2k_menu16NameEntryControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu16NameEntryControl;
}
#endif

// FUNC 0x00128758 gt2k_menu::NameEntryControl::~NameEntryControl
/* implicit in game/menu/framework/T097_name_entry.h; egcs emits this copy for the vtable:
 * no vptr store; render's inline RenderBase destructor (root's
 * RootObject::~RootObject, then GameObject::~GameObject, flags 2), then
 * gt2k_menu::ControlBase::~ControlBase(this, flags), which does the delete
 * (pm has no destructor). */
/* implicit: ~RootObject, ~GameObject (the inline ~RenderBase of render), ~ControlBase (base destructor) */
