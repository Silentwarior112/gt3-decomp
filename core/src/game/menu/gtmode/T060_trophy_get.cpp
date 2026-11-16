/* T060 - gt2k_menu::TrophyGetControl, the championship cup screen.
 * 0x0013A8D0..0x0013B380.
 *
 * trophy_get(prize, trophy) (called by T053's run_championship once a
 * championship is won, with trophy = -1) runs the page "trophy_get" of
 * "menu/trophy_get.mbl" on a 1 MB menu heap: the cup model (a random
 * "champ_NN" cup unless trophy is 0..9), the messages "mess0" (3 s, then
 * its exit script), "get_mess", and once the prize counter is done "mess1"
 * and the pad.  Afterwards the prize goes to the money and to the total
 * prize money and the trophy count goes up.  The cup is recorded at load
 * time (GameStatus largeTrophy bit 6 + cup).
 *
 * The end of the unit has unreferenced copies of the inline / implicit
 * destructors and the typeinfo functions of PrizeCounter, PrizeControlBase
 * and ControlBaseWithPageManager (their vtable copies 0x00318AF0, 0x00318B00,
 * 0x00318B18 point at the owners' definitions). */
/* The includes, in the original's class order (egcs writes the type names of
 * the classes it sees in the reverse order of their definitions).  Retail
 * T060's .rodata (0x003189C0..0x00318F4F): "bad_alloc" (<new>, read first),
 * "%s Cr. %s" (T099's inline PrizeCounter items), the functions' literals,
 * load's jump table, the script g_00318A38, "get_mess", the item names, the
 * four vtables, then the type names GetMoneyItem, DelayItem,
 * GTmodeTrophyControl, ArcadeTrophyControl, MiniPrizeControlBase,
 * TrophyGetControl, the three PrizeCounter items, PrizeCounter,
 * PrizeControlBase, ControlBaseWithPageManager, SmartIndexT<int>,
 * CallBackWhenPrizeRendering, PrizeModelSetContext, the OperatorHolder
 * group, UnitArena<64>, RegurationResult, RecordsMenuArg, the garage lists,
 * Operator (as u019's T056 without ConsolationControl; no LicenseGetControl,
 * NotCarGetControl).  Its .sdata (0x00351690): T099's inline "" and
 * "Cr. %s", "Get" and g_003516A4, "mess0", "mess1", "count".
 * ref/data/u020/notes.md. */
// DATA_START 0x003189C0
// DATA_START 0x00351690
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* getGameStatus, gt2k_game::GameStatus (the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the OperatorHolder group) */
#include "game/menu/gtmode/T460_prize_base.h"           /* PrizeBase (PrizeModelSetContext, CallBackWhenPrizeRendering) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (SmartIndexT<int>) */
#include "game/menu/gtmode/T060_trophy_get.h"           /* own (ControlBaseWithPageManager, PrizeControlBase,
                                               PrizeCounter and its items, TrophyGetControl) */
#define T056_NO_CONSOLATION                 /* (no use: MiniPrizeControlBase, ArcadeTrophyControl, */
#include "game/menu/gtmode/T056_gtmode_trophy_control.h" /* GTmodeTrophyControl; not ConsolationControl) */
#undef T056_NO_CONSOLATION

namespace gt2k_menu {
/* Known only by their type names, the first two of retail T060's .rodata
 * (0x00318B30 "Q29gt2k_menu12GetMoneyItem", 0x00318B50
 * "Q29gt2k_menu9DelayItem"): polymorphic classes without a key function
 * that the original T060 defined last and never used (? leftovers of the
 * cup screen's items; their bases and members are unknown, no code of
 * theirs exists).  Declared so that egcs writes the two names in front of
 * the others, as in retail; nothing else uses them. */
class DelayItem {
public:
    virtual ~DelayItem() {}
};
class GetMoneyItem {
public:
    virtual ~GetMoneyItem() {}
};
} /* namespace gt2k_menu */

/* The rest adds no type name. */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/gtmode/T058_trophy_item.h"          /* gt2k_menu::PrizeModelSpec, create_trophy_championship */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (attachTo, disappearAll), RootObject */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuHook, MenuHookFn */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase, TRANSITION_FADEOUT */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (attach, detach, waitFrame) */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "game/menu/framework/T262_menu_bgm.h"             /* menu_bgm_play_if_changed */
#include "game/race/core/T414_race_random.h"          /* raceRandomShuffle, raceRandomSkip */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#define U020_T060                           /* trophy_get_internal.h: this unit's constants */
#include "game/menu/gtmode/trophy_get_internal.h"

/* TrophyGetControl's item factories, filled by T060_static_init (.ctors).
 * A file static: retail has it in .bss (a C++ global would be zeros in
 * .data), the only item of T060's .bss (0x0049BDF0..0x0049BE50, then T107's
 * g_0049BE50). */
static gt2k_menu::MenuHook g_0049BDF0[4];   /* 0x0049BDF0 */

namespace gt2k_menu {

/* The menu file and the cup.  The cup's bit is set here, before the screen
 * is shown. */
// FUNC 0x0013A8D0 gt2k_menu::TrophyGetControl::load
void TrophyGetControl::load(int trophy)
{
    pages.loadFile("menu/trophy_get.mbl");
    PrizeModelSpec cup;                     /* sp+0x00 (inline constructor: kind -1, index 0) */
    cup.setChampionship(trophy);
    switch (cup.index) {                    /* jump table 0x00318A00, 10 cases */
    case 0:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 0);
        break;
    case 1:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 1);
        break;
    case 2:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 2);
        break;
    case 3:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 3);
        break;
    case 4:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 4);
        break;
    case 5:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 5);
        break;
    case 6:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 6);
        break;
    case 7:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 7);
        break;
    case 8:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 8);
        break;
    case 9:
        getGameStatus()->setLargeTrophy(LARGE_TROPHY_CUP0 << 9);
        break;
    }
    char path[0x80];                        /* sp+0x10 */
    cup.getPath(path);
    prize.load(path, NULL, 0, 0);           /* a direct call (PrizeBase::load): a buffer of its own */
}

// FUNC 0x0013AA40 gt2k_menu::TrophyGetControl::unload
void TrophyGetControl::unload()
{
    pages.unloadFile();
    prize.unload();                         /* a direct call (PrizeBase::unload) */
}

/* The pads, one frame, the render on the render thread with the fade-out
 * transition and no mask, the cup music, the page manager on the render's
 * tree (no cursor memory) with the item factories g_0049BDF0 in both
 * layouts and no ExecuteFuncData, no screen clear.  The inline accessors
 * are expanded (no calls in the listing); retail keeps &render in $s0 and
 * &pages in $s2 for them (the inline members' `this'). */
// FUNC 0x0013AA70 gt2k_menu::TrophyGetControl::begin
void TrophyGetControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    render.setTransition(true, TRANSITION_FADEOUT); /* before setMask: retail's store order with
                                                       T246's on-first setTransition */
    render.setMask(false);
    menu_bgm_play_if_changed(BGM_TROPHY_GET, 0, 1);
    pages.initialize(render.getRootObject(), NULL); /* no cursor memory */
    pages.setHooks(g_0049BDF0, 4, this);
    pages.setExecuteFuncData(NULL, 0, this);
    g_00352750 = 0;                                 /* no screen clear, colour 0 */
    g_0035274C = 0;
}

// FUNC 0x0013AB40 gt2k_menu::TrophyGetControl::end
void TrophyGetControl::end()
{
    bgm_stop();
    pages.finalize();
    render.detach();
    gfx_vsync(2);
    detach();
}

/* Page "trophy_get", pad frozen.  "mess0" for 180 frames, then its exit
 * script (disappearAll) and 90 frames; "get_mess", 60 frames.  Then a frame
 * loop (the race random generator stepped once a frame): once the counter
 * is done, 15 frames and "mess1" (attached without a NULL check), then the
 * pad works until the page is left. */
// FUNC 0x0013AB88 gt2k_menu::TrophyGetControl::mainLoop
void TrophyGetControl::mainLoop()
{
    PageManager *pm = &pages;
    pm->setFreeze(1);
    pm->setPage("trophy_get", NULL);
    pm->startPage(NULL);
    PageLayout *layout = pm->current;

    Transitive *message = layout->buildLayer(g_0029F588[0], 0);    /* "mess0" */
    if (message != NULL) {
        message->attachTo(pm->layers[pm->popupLevel]);
        gfx_vsync(180);
        message->disappearAll(g_00318A38);
        gfx_vsync(90);
    }
    message = layout->buildLayer(g_0029F588[1], 0);                /* "get_mess" */
    if (message != NULL) {
        message->attachTo(pm->layers[pm->popupLevel]);
        gfx_vsync(60);
    }

    int phase = 0;                          /* 0 counting, 1 counted, 2 the pad works */
    while (pages.exited == 0) {
        raceRandomSkip(1);
        switch (phase) {
        case 0:
            if (counter.isDone())           /* (not ?: : retail sets 0 and movn's the 1) */
                phase = 1;
            else
                phase = 0;
            break;
        case 1:
            phase = 2;
            gfx_vsync(15);
            layout->buildLayer(g_0029F588[2], 0)->attachTo(pm->layers[pm->popupLevel]); /* "mess1" */
            break;
        case 2:
            pm->setFreeze(0);
            break;
        }
        pm->update();
        waitFrame();
    }
    pm->closePage();
}

// FUNC 0x0013AD80 gt2k_menu::TrophyGetControl::run
void TrophyGetControl::run(int trophy)
{
    load(trophy);
    begin();
    mainLoop();
    end();
    unload();
}

} /* namespace gt2k_menu */

using namespace gt2k_menu;

/* mainLoop's data, defined here (MATCHING.md, "Defining data in C"): in
 * retail T060's .rodata the script follows mainLoop's "trophy_get" and
 * precedes the table's "get_mess" and T060_static_init's names, so the
 * original defined both after mainLoop.  T060_static_init's strings are at
 * their retail offsets only so. */

/* The script that takes "mess0" away (Object::disappearAll): alpha -1 a
 * second for 1 s, then the object is killed (u019's g_003185A0 is the same
 * script). */
const OperatorCode g_00318A38[2] = {        /* 0x00318A38 */
    { 20 /* opFadeColors */, 0, 0, { 0, 0, 0, 0 }, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
    { 6 /* opRelease */, 0, 0, { 0, 0, 0, 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
};

/* (? name, ? use) An unreferenced pointer to "Get" (.sdata 0x003516A0, the
 * only "Get" of the image), between T099's "Cr. %s" and mainLoop's message
 * names in retail T060's .sdata: a variable of the original file with an
 * initialiser, defined before g_0029F588 (nothing in the code reads it). */
const char *g_003516A4 = "Get";             /* 0x003516A4 */

/* mainLoop's message layers ("mess0", "mess1" in .sdata 0x003516A8 /
 * 0x003516B0, "get_mess" in .rodata 0x00318A98). */
const char *g_0029F588[3] = {               /* 0x0029F588 */
    "mess0", "get_mess", "mess1"
};

/* Returns 0 if the menu heap cannot be allocated, else 1.  The controller
 * is a stack object; the prize goes to the money only after the screen. */
// FUNC 0x0013ADC8 trophy_get
int trophy_get(int prize, int trophy)
{
    raceRandomShuffle();
    menu_draw_reset();
    void *heap = memalign(0x40, TROPHY_GET_HEAP_SIZE);
    if (heap == NULL)
        return 0;
    menu_heap_push(heap, TROPHY_GET_HEAP_SIZE);
    {
        /* sp+0x000.  The inline constructor is expanded: ControlBase(), the
         * ControlBaseWithPageManager vptr, PageManager(this), PrizeBase(),
         * param and the TrophyGetControl vptr, RenderBase(),
         * PrizeCounter(money, money + prize) (the landing pads: ~RootObject,
         * GameObject::~GameObject, ~PrizeBase, ~ControlBase, flags 0 or 2,
         * then __throw). */
        TrophyGetControl screen(prize);
        /* implicit: ControlBase, PageManager, PrizeBase, RenderBase, PrizeCounter (inline constructor) */
        screen.run(trophy);
        getGameStatus()->incMoney(prize);
        getGameStatus()->incPrize(prize);
        getGameStatus()->incTrophyNum(1);
        gfx_vsync(1);
        /* The implicit destructor is expanded: counter's vptr (PrizeCounter's
         * inline destructor), ~RootObject and GameObject::~GameObject
         * (render), ~PrizeBase, ~ControlBase, all with flags 2. */
        /* implicit: ~RootObject, ~GameObject, ~PrizeBase, ~ControlBase (member / base destructors) */
    }
    menu_heap_pop();
    free(heap);
    return 1;
}

/* One table entry: the inline constructor of the original's MenuHook (see
 * T060_static_init): name, create, p8 .. p14 = 0. */
static inline void set_hook(MenuHook *e, const char *name, MenuHookFn *create)
{
    e->name   = name;
    e->create = create;
    e->p8     = 0;
    e->pC     = 0;
    e->p10    = 0;
    e->p14    = 0;
}

/* One element as egcs's build_vec_init expands it (u006 / u013's form): the
 * constructor on a copy of the cursor, then the cursor and the count of the
 * elements left stepped.  (No do-while wrapper: its loop notes would change
 * the code.) */
#define HOOK(name, create)                                                    \
    { MenuHook *e = base; set_hook(e, name, create); }                        \
    base++;                                                                   \
    left--

/* Static initialiser (.ctors entry 0x0028F078): the item factories of
 * TrophyGetControl, sorted by name.  Compiler-generated in the original:
 * the table was defined here with MenuHook objects built by an inline
 * constructor, which egcs builds at run time (cp/init.c build_vec_init;
 * the table is in .bss).  The decomp does not define the table (it stays
 * at its original address), so the function is written out the way
 * build_vec_init expands it (a walking cursor, a copy of it per element,
 * the dead count of the elements left): an exact form, not the original;
 * it stores the same 4 entries (with g_0049BDF0[i] = ... stores: 50.5%). */
// FUNC 0x0013B088 T060_static_init
void T060_static_init(void)
{
    MenuHook *base = g_0049BDF0;
    int left = 3;                           /* build_vec_init's iterator: elements - 1 */

    HOOK("count",          create_count_T060);          /* "count" .sdata 0x003516B8 */
    HOOK("count_shadow",   create_count_shadow_T060);
    HOOK("get_money_disp", create_get_money_disp_T060);
    HOOK("render_model",   create_trophy_championship);
}

#undef HOOK

/* ------------------------------------------------------------------------ */
/* RTTI and the vtable destructor                                           */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * PrizeControlBase). */
// FUNC 0x0013B140 __tfQ29gt2k_menu16TrophyGetControl
extern "C" void *__tfQ29gt2k_menu16TrophyGetControl(void)
{
    if (__tiQ29gt2k_menu16TrophyGetControl[0] == 0) {
        __tfQ29gt2k_menu16PrizeControlBase();
        __rtti_si(__tiQ29gt2k_menu16TrophyGetControl, "Q29gt2k_menu16TrophyGetControl",
                  (const type_info *)__tiQ29gt2k_menu16PrizeControlBase);
    }
    return __tiQ29gt2k_menu16TrophyGetControl;
}
#endif

// FUNC 0x0013B190 gt2k_menu::TrophyGetControl::~TrophyGetControl
/* implicit (game/menu/gtmode/T060_trophy_get.h); egcs emits this copy for the vtable:
 * counter's vptr (+0xF08) = 0x00313B08 (PrizeCounter's inline destructor),
 * render (~RootObject +0xE84, GameObject::~GameObject +0xE80, flags 2),
 * ~PrizeBase (+0xDE8, flags 2), then ~ControlBase(this, flags), which does
 * the operator delete */
/* implicit: ~RootObject, ~GameObject (member destructor), ~PrizeBase, ~ControlBase (base destructor) */

/* ------------------------------------------------------------------------ */
/* T060's copies of inline / implicit functions (unreferenced: the calls    */
/* and the vtable copies use the owners' definitions).                      */
/* ------------------------------------------------------------------------ */

// FUNC 0x0013B208 gt2k_menu::PrizeCounter::~PrizeCounter
/* inline in game/menu/gtmode/T099_prize_counter.h; egcs emits this copy (T099's is
 * 0x0012A240): the vptr (+0x30) = 0x00313B08, then the global operator
 * delete (0x0027C0C0) if flags & 1 */
/* implicit: operator delete (deleting destructor) */

#ifndef BOOT_OLD_GCC
/* PrizeCounter's typeinfo function (T099's is 0x0012A270; this copy has its
 * own name string 0x00318C78).  No bases: __rtti_user.  Weak, so that the
 * owner's definition wins, as at the original link. */
// FUNC 0x0013B238 __tfQ29gt2k_menu12PrizeCounter
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menu12PrizeCounter(void)
{
    if (__tiQ29gt2k_menu12PrizeCounter[0] == 0)
        __rtti_user(__tiQ29gt2k_menu12PrizeCounter, "Q29gt2k_menu12PrizeCounter");
    return __tiQ29gt2k_menu12PrizeCounter;
}

/* PrizeControlBase's typeinfo function (T059's is 0x0013A7C8; weak). */
// FUNC 0x0013B278 __tfQ29gt2k_menu16PrizeControlBase
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menu16PrizeControlBase(void)
{
    if (__tiQ29gt2k_menu16PrizeControlBase[0] == 0) {
        __tfQ29gt2k_menu26ControlBaseWithPageManager();
        __rtti_si(__tiQ29gt2k_menu16PrizeControlBase, "Q29gt2k_menu16PrizeControlBase",
                  (const type_info *)__tiQ29gt2k_menu26ControlBaseWithPageManager);
    }
    return __tiQ29gt2k_menu16PrizeControlBase;
}
#endif

// FUNC 0x0013B2C8 gt2k_menu::PrizeControlBase::~PrizeControlBase
/* implicit (game/menu/gtmode/T059_license_get_control.h); egcs emits this copy (T059's
 * is 0x0013A818): ~PrizeBase (+0xDE8, flags 2), then ~ControlBase(this,
 * flags) */
/* implicit: ~PrizeBase, ~ControlBase (base destructor) */

#ifndef BOOT_OLD_GCC
/* ControlBaseWithPageManager's typeinfo function (T056's is 0x00138BD8;
 * this copy has its own name string 0x00318CB8; weak). */
// FUNC 0x0013B310 __tfQ29gt2k_menu26ControlBaseWithPageManager
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

// FUNC 0x0013B360 gt2k_menu::ControlBaseWithPageManager::~ControlBaseWithPageManager
/* implicit (game/menu/gtmode/T056_gtmode_trophy_control.h); egcs emits this copy (T056's
 * is 0x00138C28): pages has no destructor, ~ControlBase(this, flags) */
/* implicit: ~ControlBase (base destructor) */
