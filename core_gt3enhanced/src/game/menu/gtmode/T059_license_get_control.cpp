/* T059 - gt2k_menu::LicenseGetControl, the "licence acquired" screen
 * ("menu/lc_get.mbl", page "license_get").  0x0013A088..0x0013A8D0.
 *
 * license_get_screen (called by T053's run_license_test) runs it on a 1 MB
 * menu heap with the controller on the stack, then records the licence
 * trophy (GameStatus large-trophy bit 1 << licence, licences 0..5 = B, A,
 * IB, IA, S, and "d").  The page shows the licence trophy model
 * ("render_model", T058's create_trophy_license) and the licence's two
 * message layers: the first for 240 frames, then faded out; the second 120
 * frames later.
 *
 * Also: PrizeControlBase's destructor and RTTI, and unreferenced copies of
 * ControlBaseWithPageManager's (T056 has the ones in the vtables). */
/* The includes, in the original's class order (egcs writes the type names of
 * the classes it sees in the reverse order of their definitions).  Retail
 * T059's .rodata (0x00318580..0x003189B6): "bad_alloc" (<new>), the
 * functions' literals and mainLoop's data, the three vtables, then the type
 * names LicenseGetControl, PrizeControlBase, ControlBaseWithPageManager,
 * SmartIndexT<int>, CallBackWhenPrizeRendering, PrizeModelSetContext, the
 * OperatorHolder group, UnitArena<64>, RegurationResult, RecordsMenuArg, the
 * garage lists, Operator (no PrizeCounter, no trophy screens).  Its .sdata
 * (0x00351650): the 8-byte message names of g_0029F2C8, which only that
 * table references.  ref/data/u019/notes.md. */
// DATA_START 0x00318580
// DATA_START 0x00351650
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus (the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the OperatorHolder group) */
#include "game/menu/gtmode/T460_prize_base.h"           /* PrizeBase (PrizeModelSetContext, CallBackWhenPrizeRendering) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (SmartIndexT<int>) */
#include "game/menu/gtmode/T059_license_get_control.h"  /* LicenseGetControl (ControlBaseWithPageManager, PrizeControlBase) */
/* The rest adds no type name. */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/gtmode/T058_trophy_item.h"          /* PrizeModelSpec, create_trophy_license */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (buildLayer) */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C / g_00352750 */
#include "game/menu/framework/T262_menu_bgm.h"             /* menu_bgm_play_if_changed */
#include "game/save/T420_license_records.h"      /* LICENSE_B .. LICENSE_R */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "engine/pdistd/T532_heap.h"                 /* memalign, free */
#include "game/menu/gtmode/trophy_internal.h"

using namespace gt2k_menu;

/* LicenseGetControl's item factory, filled by T059_static_init (.ctors).
 * A file static: retail has it in .bss, the only item of T059's .bss
 * (0x0049BDD8..0x0049BDF0, then T060's g_0049BDF0). */
static MenuHook g_0049BDD8[1];              /* 0x0049BDD8 */

namespace gt2k_menu {

/* The menu file, and the licence trophy "menu/lic_*.prm" (PrizeModelSpec,
 * its inline constructor: kind -1, index 0) into a buffer of its own
 * (PrizeBase::load called directly, not through the vtable). */
// FUNC 0x0013A088 gt2k_menu::LicenseGetControl::load
void LicenseGetControl::load()
{
    pages.loadFile("menu/lc_get.mbl");
    PrizeModelSpec spec;                    /* sp+0x00 */
    spec.setLicense(licence);
    char path[0x80];                        /* sp+0x10 */
    spec.getPath(path);
    prize.load(path, NULL, 0, 0);
}

// FUNC 0x0013A100 gt2k_menu::LicenseGetControl::unload
void LicenseGetControl::unload()
{
    pages.unloadFile();
    prize.unload();                         /* PrizeBase::unload, called directly */
}

/* As GTmodeTrophyControl::begin (the same inline accessors expanded), with
 * the licence music and the one-entry factory table. */
// FUNC 0x0013A130 gt2k_menu::LicenseGetControl::begin
void LicenseGetControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    render.setTransition(true, TRANSITION_FADEOUT);
    render.setMask(false);
    menu_bgm_play_if_changed(BGM_LICENSE_GET, 0, 1);
    pages.initialize(&render.root, NULL);
    pages.setHooks(g_0049BDD8, 1, this);
    pages.setExecuteFuncData(NULL, 0, this);
    g_00352750 = 0;
    g_0035274C = 0;
}

// FUNC 0x0013A200 gt2k_menu::LicenseGetControl::end
void LicenseGetControl::end()
{
    bgm_stop();
    pages.finalize();
    render.detach();
    gfx_vsync(2);
    detach();
}

} /* namespace gt2k_menu */

/* The data of mainLoop, defined here where the original had it: retail's
 * .rodata has the script and the four 9-byte message names between load's
 * "menu/lc_get.mbl" and mainLoop's "license_get" (the 8-byte names are in
 * .sdata after T058's "lic_*"), the table itself in .data. */

/* The first message's exit (Object::disappearAll): opFadeColors (code 20)
 * alpha -1 a second for 1 s, then opRelease (6). */
const OperatorCode g_003185A0[2] = {        /* 0x003185A0 */
    { 20 /* opFadeColors */, 0, 0, { 0, 0, 0, 0 }, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
    { 6 /* opRelease */, 0, 0, { 0, 0, 0, 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
};

/* The licences' two message layers (LicenseIndex order). */
const char *g_0029F2C8[6][2] = {            /* 0x0029F2C8 */
    { "b_mess0", "b_mess1" },
    { "a_mess0", "a_mess1" },
    { "ib_mess0", "ib_mess1" },
    { "ia_mess0", "ia_mess1" },
    { "s_mess0", "s_mess1" },
    { "d_mess0", "d_mess1" },
};

namespace gt2k_menu {

/* The page opens frozen; the licence's two message layers are built from
 * the current layout and attached to the current popup level's layer, with
 * fixed waits (gfx_vsync(n): n frames): the first is shown for 240 frames,
 * then faded out (Object::disappearAll with g_003185A0: alpha -1 / s for
 * 1 s, then killed), the second appears 120 frames later, and 60 frames
 * after that the page takes input until it is left.  A licence above 5
 * leaves `messages' NULL and the first lookup reads through it, as in the
 * original.  The PageManager is used directly (pages: retail's $s3) and
 * through pm (a second copy of &pages, $s5: getLayer's popupLevel and the
 * final setFreeze; getLayer's layers[] is folded to this + 0xD74). */
// FUNC 0x0013A248 gt2k_menu::LicenseGetControl::mainLoop
void LicenseGetControl::mainLoop()
{
    const char **messages = NULL;
    pages.setPage("license_get", NULL);
    pages.setFreeze(1);
    pages.startPage(NULL);
    switch ((u32)licence) {                 /* jump table 0x00318650 */
    case LICENSE_B:
        messages = g_0029F2C8[LICENSE_B];   /* "b_mess0", "b_mess1" */
        break;
    case LICENSE_A:
        messages = g_0029F2C8[LICENSE_A];   /* "a_mess*" */
        break;
    case LICENSE_IB:
        messages = g_0029F2C8[LICENSE_IB];  /* "ib_mess*" */
        break;
    case LICENSE_IA:
        messages = g_0029F2C8[LICENSE_IA];  /* "ia_mess*" */
        break;
    case LICENSE_S:
        messages = g_0029F2C8[LICENSE_S];   /* "s_mess*" */
        break;
    case LICENSE_R:
        messages = g_0029F2C8[LICENSE_R];   /* "d_mess*" */
        break;
    }
    PageManager *pm = &pages;
    PageLayout *layout = pages.getFragmentManager();
    Transitive *layer = layout->buildLayer(messages[0], 0);
    if (layer != NULL) {
        layer->attachTo(pm->getLayer());
        gfx_vsync(240);
        layer->disappearAll(g_003185A0);
        gfx_vsync(120);
    }
    layer = layout->buildLayer(messages[1], 0);
    if (layer != NULL) {
        layer->attachTo(pm->getLayer());
        gfx_vsync(60);
    }
    pm->setFreeze(0);
    while (!pages.isExit()) {
        pages.update();
        waitFrame();
    }
    pages.closePage();
}

// FUNC 0x0013A410 gt2k_menu::LicenseGetControl::run
void LicenseGetControl::run()
{
    load();
    begin();
    mainLoop();
    end();
    unload();
}

} /* namespace gt2k_menu */

/* Returns 0 if the menu heap cannot be allocated, else 1.  The controller
 * (0xED8 bytes) is at sp+0x000. */
// FUNC 0x0013A458 license_get_screen
int license_get_screen(u32 licence)
{
    /* The screen's inline constructor: ControlBase::ControlBase, the
     * ControlBaseWithPageManager vptr, PageManager::PageManager(this), the
     * PrizeControlBase vptr, PrizeBase::PrizeBase, then (after the
     * PrizeControlBase part, its own member) licence and the
     * LicenseGetControl vptr, RenderBase::RenderBase; its implicit
     * destructor at the end of the block: RootObject::~RootObject,
     * GameObject::~GameObject (render), PrizeBase::~PrizeBase,
     * ControlBase::~ControlBase (flags 2); the landing pads destroy what
     * was built. */
    /* implicit: ControlBase, PageManager, PrizeBase, RenderBase (the inline LicenseGetControl constructor), ~RootObject, ~GameObject, ~PrizeBase, ~ControlBase (its implicit destructor, inline, and landing pads) */
    menu_draw_reset();
    void *heap = memalign(0x40, LICENSE_MENU_HEAP_SIZE);
    if (heap == NULL)
        return 0;
    menu_heap_push(heap, LICENSE_MENU_HEAP_SIZE);
    {
        LicenseGetControl screen(licence);  /* sp+0x000 */
        screen.run();
        gfx_vsync(1);
    }
    switch (licence) {                      /* jump table 0x00318670 */
    case LICENSE_B:
        getGameStatus()->setLargeTrophy(0x1);
        break;
    case LICENSE_A:
        getGameStatus()->setLargeTrophy(0x2);
        break;
    case LICENSE_IB:
        getGameStatus()->setLargeTrophy(0x4);
        break;
    case LICENSE_IA:
        getGameStatus()->setLargeTrophy(0x8);
        break;
    case LICENSE_S:
        getGameStatus()->setLargeTrophy(0x10);
        break;
    case LICENSE_R:
        getGameStatus()->setLargeTrophy(0x20);
        break;
    }
    menu_heap_pop();
    free(heap);
    return 1;
}

/* Static initialiser (.ctors entry 0x0028F074): the one item factory of
 * LicenseGetControl, written as T056's (HOOK, trophy_internal.h; plain stores give the
 * same code here). */
// FUNC 0x0013A6E0 T059_static_init
void T059_static_init(void)
{
    MenuHook *base = g_0049BDD8;
    int left = 0;                           /* build_vec_init's iterator: elements - 1 */

    HOOK("render_model", create_trophy_license, 0, 0, 0, 0);
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * PrizeControlBase). */
// FUNC 0x0013A718 __tfQ29gt2k_menu17LicenseGetControl
extern "C" void *__tfQ29gt2k_menu17LicenseGetControl(void)
{
    if (__tiQ29gt2k_menu17LicenseGetControl[0] == 0) {
        __tfQ29gt2k_menu16PrizeControlBase();
        __rtti_si(__tiQ29gt2k_menu17LicenseGetControl, "Q29gt2k_menu17LicenseGetControl",
                  (const type_info *)__tiQ29gt2k_menu16PrizeControlBase);
    }
    return __tiQ29gt2k_menu17LicenseGetControl;
}
#endif

// FUNC 0x0013A768 gt2k_menu::LicenseGetControl::~LicenseGetControl
/* the implicit destructor (game/menu/gtmode/T059_license_get_control.h), no vptr store:
 * render (RootObject::~RootObject +0xE84, GameObject::~GameObject +0xE80),
 * PrizeBase::~PrizeBase (+0xDE8), all with flags 2, then
 * ControlBase::~ControlBase(this, flags), which does the operator delete */
/* implicit: ~RootObject, ~GameObject (member destructors of render), ~PrizeBase (member destructor), ~ControlBase (base destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * ControlBaseWithPageManager; also used by T060's TrophyGetControl). */
// FUNC 0x0013A7C8 __tfQ29gt2k_menu16PrizeControlBase
extern "C" void *__tfQ29gt2k_menu16PrizeControlBase(void)
{
    if (__tiQ29gt2k_menu16PrizeControlBase[0] == 0) {
        __tfQ29gt2k_menu26ControlBaseWithPageManager();
        __rtti_si(__tiQ29gt2k_menu16PrizeControlBase, "Q29gt2k_menu16PrizeControlBase",
                  (const type_info *)__tiQ29gt2k_menu26ControlBaseWithPageManager);
    }
    return __tiQ29gt2k_menu16PrizeControlBase;
}
#endif

// FUNC 0x0013A818 gt2k_menu::PrizeControlBase::~PrizeControlBase
/* the implicit destructor (game/menu/gtmode/T059_license_get_control.h), no vptr store:
 * PrizeBase::~PrizeBase (+0xDE8, flags 2), then
 * ControlBase::~ControlBase(this, flags).  In both vtables (this unit's
 * 0x003186B0 and T060's 0x00318B00). */
/* implicit: ~PrizeBase (member destructor), ~ControlBase (base destructor) */

#ifndef BOOT_OLD_GCC
/* T059's copy of ControlBaseWithPageManager's typeinfo function
 * (unreferenced; T056's 0x00138BD8 is the one in the vtables), with this
 * unit's copy of the name string (0x00318720).  Weak: T056's wins. */
// FUNC 0x0013A860 __tfQ29gt2k_menu26ControlBaseWithPageManager
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menu26ControlBaseWithPageManager(void)
{
    if (__tiQ29gt2k_menu26ControlBaseWithPageManager[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu26ControlBaseWithPageManager,
                  "Q29gt2k_menu26ControlBaseWithPageManager",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu26ControlBaseWithPageManager;
}
#endif

// FUNC 0x0013A8B0 gt2k_menu::ControlBaseWithPageManager::~ControlBaseWithPageManager
/* T059's copy of the implicit destructor (unreferenced; T056's 0x00138C28
 * is the one in the vtables): ControlBase::~ControlBase(this, flags) */
/* implicit: ~ControlBase (base destructor) */
