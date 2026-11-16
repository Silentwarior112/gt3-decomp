/* T061 - gt2k_menu::CarGetControl, the prize car screen ("menu/car_get.mbl",
 * page "car_get"), and its flow.  0x0013E918..0x0013F940.
 *
 * F_0013F508 (called after the races of the arcade and GT modes) runs
 * car_get_prizes, which asks T419 for the prize sets the race gives
 * (PrizeCarList) and, for each one whose reason is valid, loads the models of
 * the set's cars (car_get_load_car: up to four, 0xC8000 bytes each) and runs
 * a CarGetControl over them; the car the roulette stops on is added to the
 * garage (a set of one car is also marked as won).  With 200 cars in the
 * garage, T105's not_car_get shows instead.  Then, once each, the ending
 * movies.
 *
 * Also here: the "eventname" item, the hook table g_0049BE90 and T061's
 * unreferenced copies of ControlBaseWithPageManager's typeinfo function and
 * destructor (the vtable copy 0x003199A8 points at T056's 0x00138C28). */
/* The includes, in the original's class order (egcs writes the type names of
 * the classes it sees in the reverse order of their definitions).  Retail
 * T061's .rodata (0x00319650..0x00319D06): "bad_alloc" (<new>, read first),
 * SimpleCameraBase::print's string (T412's inline function), the event
 * names, car_get_event_name's jump table, the literals and the weights'
 * initialiser, the vtables of CarGetControl and ControlBaseWithPageManager
 * (T061's own copy), then the type names EventNameItem, TheCar,
 * CarGetControl, PrizeControlBase, ControlBaseWithPageManager,
 * CallBackWhenPrizeRendering, PrizeModelSetContext, SmartIndexT<int>, the
 * OperatorHolder group, UnitArena<64>, RegurationResult, RecordsMenuArg, the
 * garage lists, EventNameItem's vtable (create_event_name, an inline
 * function written at the end), Operator.  ref/data/u021/notes.md. */
// DATA_START 0x00319650
#include <new>                              /* first: "bad_alloc" */
#include "game/camera/T412_simple_camera_base.h"   /* SimpleCameraBase::print's string (the cameras' base) */
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus (the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the OperatorHolder group) gt2k_menu::Transitive (buildLayer's
                                               result, attachTo) */
#include "game/menu/framework/T211_page_manager.h"         /* (SmartIndexT<int>) gt2k_menu::PageManager */
#include "game/menu/gtmode/T061_car_get_control.h"      /* own: (PrizeBase's classes, ControlBaseWithPageManager,
                                               PrizeControlBase) gt2k_menu::CarGetControl, CarGetEntry */
#include "game/menu/gtmode/T107_cars_item.h"            /* (TheCar, EventNameItem) CarsItem::create,
                                               create_car_get_message */
/* The rest adds no type name. */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync, tex1_variation_count, Tex1Image */
#include "game/app/T000_main.h"                 /* MovieParam */
#include "game/menu/framework/T051_movie_entry.h"          /* play_movie (only retail T051 has MovieControl's /
                                               MovieRender's type names: u013's data pass) */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T205_blink.h"                /* create_blink ("blink") */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuHook, MenuHookFn */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase, TRANSITION_FADEOUT */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (attach, detach, waitFrame) */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "game/menu/framework/T262_menu_bgm.h"             /* menu_bgm_play_if_changed */
#include "game/race/core/T414_race_random.h"          /* raceRandomShuffle, raceRandomSkip, raceRandomFloat */
#include "game/save/T419_prize_cars.h"           /* PrizeCarList, prize_reason_valid */
#include "game/save/T425_event_results.h"        /* EventResults (the league flags) */
#include "game/data/T435_product_config.h"       /* ProductConfig (the prize sets), product_config */
#include "game/car/model/T446_car_model.h"            /* car_model_path */
#include "engine/gfx/T451_model_set.h"            /* ModelSet (the body's textures) */
#include "game/car/model/T456_car_model_file.h"       /* CarModelFile, carmodelfile_relocate */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "game/menu/gtmode/car_get_internal.h"

/* T105's not_car_get without its class (retail T061 has no NotCarGetControl
 * name; T105_not_car_get.h declares the same). */
int not_car_get(void);                      /* 0x0013B5C8 (T105) */

/* The item factories of menu/car_get.mbl, sorted by name (begin gives both
 * layouts the 4 entries): T061 .bss, a file static (a local symbol in the
 * original), filled by T061_static_init. */
static gt2k_menu::MenuHook g_0049BE90[4];   /* 0x0049BE90 */

using namespace gt2k_menu;

/* Loads car `code' of a prize set into buf (its menu model file, relocated)
 * and fills out: the colour is `color' (a colour id, -1: none) if its
 * index is one of the model's colour variations, else a random one
 * ((int)(variations * random + 0.5), 0 if that is past the last).  Returns
 * code, or NULL if the car is not in the database. */
// FUNC 0x0013E918 car_get_load_car
const char *car_get_load_car(const char *code, CarModelFile *buf, u32 color, CarGetEntry *out)
{
    CarEquipments equip;                    /* sp+0x000 CarEquipments::CarEquipments */
    char path[0x40];                        /* sp+0x1F0 */

    if (equip.setCar(nameToCode(code)) != 1)
        return NULL;                        /* (CarEquipments::~CarEquipments, flags 2, on both returns) */
    car_model_path(equip.getModelCode(), path, CAR_MODEL_MENU);
    file_read_blocking(path, buf, CAR_MODEL_BUF_SIZE, 1, &vol_device_object);
    carmodelfile_relocate(buf);
    ModelSet *body = buf->body;
    u32 index = 0;
    if (color != (u32)-1) {
        equip.setColorCode(color);
        index = equip.colorIndex;
    }
    u32 variations = 0;
    if (body->tex != NULL && body->tex[0] != NULL)
        variations = tex1_variation_count(body->tex[0]);
    if (color == (u32)-1 || !(index < variations)) {
        f32 r = raceRandomFloat(1);
        index = (int)((f32)variations * r + 0.5f);
        if (!(index < variations))
            index = 0;
    }
    equip.setColorIndex(index);
    out->model = buf;
    out->code = code;
    out->name = equip.getName1();
    out->color = equip.colorIndex;
    out->colorId = equip.colorCode;
    return code;
}

namespace gt2k_menu {

/* The image name of the "eventname" item (PrizeReason, T419).  The jump
 * table is 0x00319880 (23 cases); other values give "" (.sdata
 * 0x003516C8). */
// FUNC 0x0013EB10 gt2k_menu::car_get_event_name
const char *car_get_event_name(const s8 *reason)
{
    switch (*reason) {
    case PRIZE_REASON_SOLO_RACE_CLEAR:          return "solo_race_clear";
    case PRIZE_REASON_CHAMPIONSHIP_WIN:         return "championship_win";
    case PRIZE_REASON_ENDURO_WIN:               return "enduro_win";
    case PRIZE_REASON_LICENSE_B_ALL_GOLD:       return "license_b_all_gold";
    case PRIZE_REASON_LICENSE_A_ALL_GOLD:       return "license_a_all_gold";
    case PRIZE_REASON_LICENSE_IB_ALL_GOLD:      return "license_ib_all_gold";
    case PRIZE_REASON_LICENSE_IA_ALL_GOLD:      return "license_ia_all_gold";
    case PRIZE_REASON_LICENSE_S_ALL_GOLD:       return "license_s_all_gold";
    case PRIZE_REASON_LICENSE_D_ALL_GOLD:       return "license_r_all_gold";
    case PRIZE_REASON_ARCADE_EASY_COMPLETE:     return "arcade_easy_complete";
    case PRIZE_REASON_ARCADE_NORMAL_COMPLETE:   return "arcade_normal_complete";
    case PRIZE_REASON_ARCADE_HARD_COMPLETE:     return "arcade_hard_complete";
    case PRIZE_REASON_ARCADE_PRO_COMPLETE:      return "arcade_pro_complete";
    case PRIZE_REASON_GTMODE_BEGINNER_COMPLETE: return "gtmode_beginner_complete";
    case PRIZE_REASON_GTMODE_AMA_COMPLETE:      return "gtmode_ama_complete";
    case PRIZE_REASON_GTMODE_PRO_COMPLETE:      return "gtmode_pro_complete";
    case PRIZE_REASON_GTMODE_DIRT_COMPLETE:     return "gtmode_dirt_complete";
    case PRIZE_REASON_GTMODE_ENDURO_COMPLETE:   return "gtmode_enduro_complete";
    case PRIZE_REASON_ACHIEVE_50:               return "achieve50";
    case PRIZE_REASON_ACHIEVE_75:               return "achieve75";
    case PRIZE_REASON_ACHIEVE_100:              return "achieve100";
    case PRIZE_REASON_GTMODE_COMPLETE:          return "gtmode_complete";
    case PRIZE_REASON_ARCADE_COMPULSOLY_COMPLETE: return "arcade_complusoly_complete";  /* (sic) */
    }
    return "";
}

// FUNC 0x0013EC58 gt2k_menu::CarGetControl::load
void CarGetControl::load()
{
    pages.loadFile("menu/car_get.mbl");
}

// FUNC 0x0013EC80 gt2k_menu::CarGetControl::unload
void CarGetControl::unload()
{
    pages.unloadFile();
}

/* The draw state reset, the pads, one frame, the render (no mask, the
 * fade-out transition when it is detached), the page manager over the
 * render's tree, the hook table g_0049BE90 on both layouts, no
 * ExecuteFuncData; the per-frame screen clear off. */
// FUNC 0x0013ECA0 gt2k_menu::CarGetControl::begin
void CarGetControl::begin()
{
    menu_draw_reset();
    attach();
    waitFrame();                            /* (virtual: slot 1) */
    render.attach();
    render.setTransition(true, TRANSITION_FADEOUT);
    render.setMask(false);
    pages.initialize(render.getRootObject(), NULL);
    pages.setHooks(g_0049BE90, 4, this);
    pages.setExecuteFuncData(NULL, 0, this);
    g_00352750 = 0;
    g_0035274C = 0;
}

/* The music stops, the pages are released, two frames, the render is
 * detached (its transition), the pads. */
// FUNC 0x0013ED68 gt2k_menu::CarGetControl::end
void CarGetControl::end()
{
    bgm_stop();
    pages.finalize();
    gfx_vsync(2);
    render.detach();
    detach();
}

/* Page "car_get" until it is left (or done is set); once the cars are
 * shown (carsShown), layer "after" of the current layout is built once and
 * attached to the current popup level's layer (the message). */
// FUNC 0x0013EDB0 gt2k_menu::CarGetControl::mainLoop
void CarGetControl::mainLoop()
{
    int afterShown = 0;

    menu_bgm_play_if_changed(BGM_GROUP_CAR_GET, 0, 0);
    waitFrame();
    pages.setPage("car_get", NULL);
    pages.startPage(NULL);
    while (pages.exited == 0 && done == 0) {
        raceRandomSkip(1);
        if (carsShown != 0 && afterShown == 0) {
            afterShown = 1;
            Transitive *after = pages.getFragmentManager()->buildLayer("after", 0);
            if (after != NULL) {
                after->attachTo(pages.getLayer());
                gfx_vsync(1);
            }
        }
        pages.update();
        waitFrame();
    }
    pages.closePage();
}

// FUNC 0x0013EEF8 gt2k_menu::CarGetControl::run
void CarGetControl::run()
{
    load();
    begin();
    mainLoop();
    end();
    unload();
}

} /* namespace gt2k_menu */

/* The prize car screens of race `race' (arg: passed on to PrizeCarList).
 * Returns 0 if there is no prize set, a set has more than 4 cars or the
 * memory cannot be had (memalign 0x40: the models of the biggest set +
 * 0x19000 for "menu/prize_get.img", and a 0x80000-byte menu heap), else
 * 1.  The cars' weights start at 1 (and stay at least 1). */
// FUNC 0x0013EF40 car_get_prizes
int car_get_prizes(const char *race, int arg)
{
    int most = 0;
    int i;

    raceRandomShuffle();
    f32 weights[CAR_GET_MAX_CARS] = {1.0f, 1.0f, 1.0f, 1.0f};  /* sp+0x00 (a copy of .rodata 0x003198F8) */
    PrizeCarList prizes(race, arg);         /* sp+0x10 PrizeCarList::PrizeCarList */
    int nSets = prizes.count();
    if (nSets <= 0)
        return 0;
    for (i = 0; i < nSets; i++) {
        const char *set = prizes.getSet(i);
        int cars = product_config()->prizeCarCount(set);
        if (most < cars)
            most = cars;
    }
    if (most < 1 || most > CAR_GET_MAX_CARS)    /* (one unsigned compare of most - 1) */
        return 0;

    CarModelFile *models[CAR_GET_MAX_CARS]; /* sp+0x40 */
    memset(models, 0, sizeof(models));
    void *modelMem = memalign(0x40, most * CAR_MODEL_BUF_SIZE + PRIZE_GET_IMG_SIZE);  /* sp+0xF8C */
    if (modelMem == NULL)
        return 0;
    for (i = 0; i < most; i++) {
        if (i == 0)
            models[0] = (CarModelFile *)modelMem;
        else
            models[i] = (CarModelFile *)((u8 *)models[i - 1] + CAR_MODEL_BUF_SIZE);
    }
    Tex1Image *envImage = (Tex1Image *)((u8 *)models[most - 1] + CAR_MODEL_BUF_SIZE);  /* sp+0xF90 */
    void *heap = memalign(0x40, CAR_GET_HEAP_SIZE); /* sp+0xF94 */
    if (heap == NULL) {
        free(modelMem);
        return 0;
    }
    file_read_blocking("menu/prize_get.img", envImage, PRIZE_GET_IMG_SIZE, 1, &vol_device_object);

    for (i = 0; i < nSets; i++) {
        if (getGameStatus()->getCarNum() >= GARAGE_MAX_CARS) {
            not_car_get();                  /* ($a0 still holds &prizes: not read) */
            gfx_vsync(1);
            continue;
        }
        s8 reason = prizes.getReason(i);    /* sp+0xF80 */
        if (prize_reason_valid(reason) == 0)
            continue;
        menu_heap_push(heap, CAR_GET_HEAP_SIZE);
        int ok = 1;                         /* sp+0xF98 */
        const char *set = prizes.getSet(i);
        int cars = product_config()->prizeCarCount(set);
        CarGetEntry entries[CAR_GET_MAX_CARS];  /* sp+0x50 */
        for (int j = 0; j < cars; j++) {
            int replaced = 0;               /* sp+0xF84 */
            u32 color = (u32)-1;            /* a replaced (regional) car: a random colour */
            const char *code = product_config()->prizeCarCode(set, j, &replaced);
            if (replaced != 1)
                color = product_config()->prizeCarColor(set, j);
            f32 weight = (f32)product_config()->prizeCarWeight(set, j);
            weights[j] = weight;
            if (weight < 1.0f)
                weights[j] = 1.0f;
            if (car_get_load_car(code, models[j], color, &entries[j]) == NULL) {
                ok = 0;
                break;
            }
            gfx_vsync(1);
        }
        if (ok == 1) {
            /* The inline constructor: ControlBase::ControlBase, the
             * ControlBaseWithPageManager vptr, PageManager::PageManager(this),
             * the CarGetControl vptr, RenderBase::RenderBase, the fields
             * (eventName: car_get_event_name) and the entries; the landing
             * pads undo it (RootObject::~RootObject, GameObject::~GameObject,
             * ControlBase::~ControlBase). */
            /* implicit: ControlBase, PageManager, RenderBase, car_get_event_name (the inline CarGetControl constructor), ~RootObject, ~GameObject, ~ControlBase (its implicit destructor, inline, and landing pads) */
            CarGetControl ctl(cars, entries, envImage, weights, reason);   /* sp+0xA0 */
            ctl.run();
            gfx_vsync(1);
            CarGetEntry *won = &ctl.entries[ctl.selected];
            getGameStatus()->presentCar(won->code, won->colorId);
            if (cars == 1)
                getGameStatus()->prizeSets.set(set);
        }   /* ~ctl: RootObject::~RootObject, GameObject::~GameObject (render), ControlBase::~ControlBase (flags 2) */
        menu_heap_pop();
    }
    free(modelMem);
    free(heap);
    return 1;
}

/* Entry point (the arcade and GT-mode race ends): the prize cars, then, the
 * first time the league results ask for them, the ending movies: the short
 * one once the beginner league is won, the long one once the amateur or
 * the professional league is.  Returns car_get_prizes'. */
// FUNC 0x0013F508 F_0013F508
int F_0013F508(const char *race, int arg)
{
    int playShort = 0;
    int playLong = 0;

    int result = car_get_prizes(race, arg);
    EventResults *events = getGameStatus()->getEventResults();
    if (events->beginnerLeagueWon())
        playShort = getGameStatus()->easyEnding == 0;
    if (events->amateurLeagueWon() || events->professionalLeagueWon())
        playLong = getGameStatus()->normalEnding == 0;
    if (playShort) {
        MovieParam movie;                   /* sp+0x00 */
        movie.file = "stream/ending_short_full.pss";
        movie.width = 640;
        movie.height = 448;
        movie.field = 0;
        movie.withSound = 1;
        movie.oneShot = 1;
        play_movie(&movie);
        gfx_vsync(2);
        getGameStatus()->easyEnding = 1;
    }
    if (playLong) {
        MovieParam movie;                   /* sp+0x00 */
        movie.file = "stream/ending_long_full.pss";
        movie.width = 640;
        movie.height = 448;
        movie.field = 0;
        movie.withSound = 1;
        movie.oneShot = 1;
        play_movie(&movie);
        gfx_vsync(2);
        getGameStatus()->normalEnding = 1;
    }
    return result;
}

/* "eventname": defined below (inline: retail emits it after the static
 * initialiser, at the end of the unit, and EventNameItem's vtable after the
 * type names, as egcs does for an inline function whose address the table
 * takes). */
inline Transitive *create_event_name(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                     int p8, int pC, int p10, int p14);

/* One table entry: the inline constructor of the original's MenuHook
 * (see T061_static_init); the fields in member order. */
static inline void set_hook(MenuHook *e, const char *name, MenuHookFn *create,
                            int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* One element as egcs's build_vec_init expands it (u006 / u012 / u013 /
 * u015's form): the constructor on a copy of the cursor, then the cursor
 * and the count of the elements left stepped. */
#define HOOK(name, create)                                                        { MenuHook *e = hooks; set_hook(e, name, create, 0, 0, 0, 0); }               hooks++;                                                                      hooksLeft--

/* Static initialiser (.ctors entry 0x0028F07C): the hooks of
 * menu/car_get.mbl, sorted by name.  Compiler-generated in the original
 * (_GLOBAL_$I$...): the table was defined in this unit with MenuHook
 * objects built by an inline constructor, which egcs builds at run time
 * (cp/init.c build_vec_init: a walking cursor).  The decomp defines the
 * table without a constructor (MenuHook is a plain record of T229, which
 * other units' tables brace-initialise; the static above, .bss 0x0049BE90),
 * so the function is written out the way build_vec_init expands it (a cursor, a
 * copy of it per element, the dead count of the elements left): an exact
 * form, not the original; it stores the same 4 entries {name, create, 0, 0,
 * 0, 0}. */
// FUNC 0x0013F648 T061_static_init
void T061_static_init(void)
{
    MenuHook *hooks = g_0049BE90;
    int hooksLeft = 3;                      /* build_vec_init's iterator: elements - 1 (dead) */

    HOOK("blink", create_blink);            /* "blink" .sdata 0x003516E0; T205 */
    HOOK("car_get_message", create_car_get_message);
    HOOK("eventname", create_event_name);
    HOOK("render_model", CarsItem::create);
}

#undef HOOK

/* "eventname" (ctx: the CarGetControl): Object::operator new(0x100), then
 * EventNameItem's inline constructor: Item::Item(name, layout, pos) - the
 * hidden picture named owner->eventName, centred on the layout item - and
 * the vptr; the landing pad gives the block back with Object::operator
 * delete.  parent and p8 .. p14 are not used. */
// FUNC 0x0013F700 create_event_name
inline Transitive *create_event_name(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                              int p8, int pC, int p10, int p14)
{
    CarGetControl *owner = (CarGetControl *)ctx;
    /* implicit: Item (base constructor in the inline EventNameItem constructor) */
    EventNameItem *name = new EventNameItem(owner->eventName, layout, item);   /* Item::Item(name, layout, pos), the vptr */ /* implicit: Object::operator delete (if the constructor throws) */
    return name;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * ControlBaseWithPageManager). */
// FUNC 0x0013F798 __tfQ29gt2k_menu13CarGetControl
extern "C" void *__tfQ29gt2k_menu13CarGetControl(void)
{
    if (__tiQ29gt2k_menu13CarGetControl[0] == 0) {
        __tfQ29gt2k_menu26ControlBaseWithPageManager();
        __rtti_si(__tiQ29gt2k_menu13CarGetControl, "Q29gt2k_menu13CarGetControl",
                  (const type_info *)__tiQ29gt2k_menu26ControlBaseWithPageManager);
    }
    return __tiQ29gt2k_menu13CarGetControl;
}
#endif

// FUNC 0x0013F7E8 gt2k_menu::CarGetControl::~CarGetControl
/* implicit in game/menu/gtmode/T061_car_get_control.h (no vptr store); egcs emits it
 * for the vtable: render's members (RootObject::~RootObject +0xDEC, flags
 * 2; GameObject::~GameObject +0xDE8, flags 2), pages has none, then
 * ControlBase::~ControlBase(this, flags), which does the operator delete */
/* implicit: ~RootObject, ~GameObject (member destructors of render), ~ControlBase (base destructor) */

// FUNC 0x0013F838 __tfQ29gt2k_menu26ControlBaseWithPageManager
/* T061's copy of ControlBaseWithPageManager's typeinfo function (no
 * callers; its own copy of the name string, 0x00319A38): generated by egcs
 * for the class of game/menu/gtmode/T056_gtmode_trophy_control.h, whose T056 copy
 * (0x00138BD8) is the one used.  __tfQ29gt2k_menu11ControlBase, then
 * __rtti_si(__tiQ29gt2k_menu26ControlBaseWithPageManager, ..., ControlBase's
 * node) */
/* implicit: __tfQ29gt2k_menu11ControlBase, __rtti_si (compiler-generated typeinfo function) */

// FUNC 0x0013F888 gt2k_menu::ControlBaseWithPageManager::~ControlBaseWithPageManager
/* T061's copy of ControlBaseWithPageManager's implicit destructor (no
 * callers; T056's 0x00138C28 is the vtable's): a tail call of
 * ControlBase::~ControlBase(this, flags) */
/* implicit: ~ControlBase (base destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x0013F8A8 __tfQ29gt2k_menu13EventNameItem
extern "C" void *__tfQ29gt2k_menu13EventNameItem(void)
{
    if (__tiQ29gt2k_menu13EventNameItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu13EventNameItem, "Q29gt2k_menu13EventNameItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu13EventNameItem;
}
#endif

// FUNC 0x0013F8F8 gt2k_menu::EventNameItem::~EventNameItem
/* implicit in game/menu/gtmode/T061_car_get_control.h (no vptr store); egcs emits it
 * for the vtable: Item::~Item (flags 0), then Object::operator delete if
 * flags & 1 */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
