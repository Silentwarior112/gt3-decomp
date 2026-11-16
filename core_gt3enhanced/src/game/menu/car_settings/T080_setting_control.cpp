/* T080 - gt2k_menu::SettingControl and setting_menu: the car setting menu
 * (game/menu/car_settings/T080_setting_control.h), and T080's copies of GarageCar's /
 * Cell<GarageCar>'s inline members (T036).
 *
 * Data: .rodata 0x00320A78..0x00320D8E: <new>'s "bad_alloc" (unreferenced:
 * DATA_START), the functions' strings, the vtables (SettingControl, then
 * T080's copies of GarageCar's two and Cell<GarageCar>'s), then the type
 * names (reverse definition order): Cell<Button>, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * SettingControl, UnitArena<64>, RecordsMenuArg, SmartIndexT<int>,
 * RegurationResult, the garage's eight (GarageCar's base list g_00320D48
 * after its name), Operator.  So <new>, the GameStatus, RegurationResult,
 * SmartIndexT<int> (T251), RecordsMenuArg (T089), T153's UnitArena<64>,
 * then the own header (SettingControl; it includes T036_garage.h, the
 * operator classes, after the class), Cell<Button> (T206).  The memory card
 * screens through their entry header (retail T080 has none of
 * MemorycardControl's names).  .sdata: "root". */
// DATA_START 0x00320A78
#include <new>                              /* (seen: "bad_alloc", T080's first .rodata) */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus (menu.raceCar, menu.event) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#include "game/common/T251_smart_index.h"          /* (seen: SmartIndexT<int>) */
#include "game/menu/records/T089_records_menu_args.h"    /* (seen: RecordsMenuArg) */
#define GAME_T153_EMITS_TYPEINFO            /* (seen: UnitArena<64>'s type name) */
#include "game/race/core/T153_race_course.h"
#include "game/menu/car_settings/T080_setting_control.h"      /* SettingControl, then the operator classes */
#include "game/menu/framework/T206_button.h"               /* (seen: Cell<Button>) */
#include "game/menu/memorycard/T068_memorycard_entry.h"     /* F_0014AE00, gt2k_menu::MEMCARD_SETTING_* */
#include "game/menu/car_settings/T081_setting_hooks.h"        /* g_002A9080, g_00351950 */
#include "game/menu/car_settings/T082_setting_sliders.h"      /* g_002ABAB0, g_00351970 */
#include "game/menu/car_settings/T083_setting_actions.h"      /* g_002ACCC8, g_00351978 */
#include "game/menu/car_settings/T084_setting_checks.h"       /* g_002AD058, g_00351994 */
#include "game/menu/car_settings/T096_setting_checks.h"       /* g_0029AEA0, g_003512DC */
#include "game/menu/race_menu/T102_race_menu_helpers.h"    /* MemCardRaceInfo, BlockPool (_UnitArenaBase) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization::unloadCourseData */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "engine/pdistd/T471_unit_arena.h"           /* _UnitArenaBase::alloc / release */
#include "game/data/T482_car_equipments.h"       /* CarEquipments */
#include "game/data/T483_car_information.h"      /* CarInformation, __tf14CarInformation */
#include "game/save/T488_setting_serialize.h"    /* SettingSerialize, g_00352EE8 */
#include "game/data/T489_car_database.h"         /* race_name_split, race_kind_text */
#include "game/menu/framework/T495_menu_list.h"            /* __tfQ29gt2k_menu5pCell, __tiQ29gt2k_menu5pCell */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "libc.h"                           /* memalign, free */
#include "game/menu/memorycard/memorycard_icon_internal.h"

namespace gt2k_menu {

// FUNC 0x0015C998 gt2k_menu::SettingControl::SettingControl
SettingControl::SettingControl() : pages(this)
{
    /* egcs: ControlBase::ControlBase(), the vptr, the members
     * (RenderBase::RenderBase() render, PageManager::PageManager(this) pages,
     * GarageCar::GarageCar() car), then the two words.  If a member's
     * constructor throws: render's ~RootObject and GameObject::~GameObject,
     * then ControlBase::~ControlBase. */
    /* implicit: ControlBase, RenderBase, PageManager, GarageCar (the base and
     * members), ~RootObject, ~GameObject, ~ControlBase (their cleanup on a
     * throw) */
    request = REQUEST_NONE;
    changed = 0;
}

// FUNC 0x0015CA50 gt2k_menu::SettingControl::load
void SettingControl::load()
{
    pages.loadFile("menu/setting.mbl");
}

// FUNC 0x0015CA78 gt2k_menu::SettingControl::unload
void SettingControl::unload()
{
    pages.unloadFile();
}

/* Starts the pages: the render (no transition but a fade-out on detach),
 * the tables of the setting pages, all with this as their context: T083's
 * page actions, T096's ValidFuncData, T081's hooks and T082's sliders on
 * both layouts, T084's button checks (PageManager's inline set*FuncData /
 * setHooks / setSliderFuncs: each count is read once for both layouts). */
// FUNC 0x0015CA98 gt2k_menu::SettingControl::begin
void SettingControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    render.setTransition(true, TRANSITION_FADEOUT);
    pages.initialize(&render.root, NULL);
    pages.setExecuteFuncData(g_002ACCC8, g_00351978, this);
    pages.setValidFuncData(g_0029AEA0, g_003512DC, this);
    pages.setHooks(g_002A9080, g_00351950, this);
    pages.setSliderFuncs(g_002ABAB0, g_00351970, this);
    pages.setSelectableFuncData(g_002AD058, g_00351994, this);
    g_00352750 = 0;                     /* no per-frame screen clear */
    g_0035274C = 0;
}

// FUNC 0x0015CBE8 gt2k_menu::SettingControl::end
void SettingControl::end()
{
    pages.finalize();
    gfx_vsync(2);
    render.detach();
    detach();
}

/* Copies the GameStatus's setting car (if any), opens "root" (and
 * "load_warning" over it if asked) and runs the pages until they are
 * left. */
// FUNC 0x0015CC28 gt2k_menu::SettingControl::mainLoop
void SettingControl::mainLoop(int loadWarning)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL)
        car.assign(*settingCar);
    pages.setPage("root", NULL);
    pages.startPage(NULL);
    if (loadWarning != 0) {
        pages.pushPopup(NULL);
        pages.setPage("load_warning", NULL);
        pages.startPage(NULL);
    }
    while (pages.exited == 0) {
        pages.update();
        waitFrame();
    }
}

// FUNC 0x0015CD08 gt2k_menu::SettingControl::run
int SettingControl::run(int loadWarning)
{
    load();
    begin();
    mainLoop(loadWarning);
    end();
    unload();
    return changed;
}

} /* namespace gt2k_menu */

/* The car setting menu (callers: the race modes of T052 / T053): runs
 * SettingControl over a 256 KB menu heap until its pages neither load
 * (REQUEST_LOAD) nor save (REQUEST_SAVE) the setting car's setting on a memory
 * card (F_0014AE00 MEMCARD_SETTING_LOAD / MEMCARD_SETTING_SAVE with a
 * SettingSerialize that g_00352EE8 points at; the save gets the race kind's
 * name and the index of the current event and a work block of org's course
 * arena).  After a load the menu shows "load_warning" when the loaded
 * setting differed from the car (applySettings != 0: another car, or part
 * groups that differ).  Nonzero if the menu reported a change or a setting
 * was loaded. */
/* The enhanced engine: the course of the race the setting menu runs for,
 * for the settings' dirt-tyre check of an arcade event (T096; Lupo 2 reads
 * its menu's course name instead, MenuStatus +0x84, which GT3's arcade does
 * not fill: menus_b.md 5).  0 outside setting_menu. */
static u64 s_settingCourse;

u64 setting_menu_course(void)
{
    return s_settingCourse;
}

// FUNC 0x0015CD60 setting_menu
int setting_menu(u64 course, RaceOrganization *org)
{
    int result = 0;
    int state = gt2k_menu::SettingControl::REQUEST_NONE;
    int done = 0;
    int loadWarning = 0;

    s_settingCourse = course;               /* the enhanced engine (setting_menu_course) */
    do {
        switch (state) {
        case gt2k_menu::SettingControl::REQUEST_NONE: {
            void *heap;

            menu_draw_reset();
            heap = memalign(0x40, SETTING_HEAP_SIZE);
            menu_heap_push(heap, SETTING_HEAP_SIZE);
            {
                /* ~SettingControl (implicit) at the end of the scope: car's
                 * inline ~GarageCar (CarInformation::~CarInformation, the
                 * Cell vptr, pCell::~pCell), render's ~RootObject and
                 * GameObject::~GameObject, ControlBase::~ControlBase. */
                /* implicit: ~CarInformation, ~GameObject (~SettingControl,
                 * implicit) */
                /* implicit: ~pCell, ~RootObject, ~ControlBase (~SettingControl, implicit) */
                gt2k_menu::SettingControl control;

                result = control.run(loadWarning) != 0 || result != 0;
                gfx_vsync(1);
                state = control.request;
                if (state == gt2k_menu::SettingControl::REQUEST_NONE)
                    done = 1;
            }
            menu_heap_pop();
            free(heap);
            break;
        }
        case gt2k_menu::SettingControl::REQUEST_LOAD: {
            GarageCar *car = getGameStatus()->getMenuStatus()->raceCar[0];

            if (car != NULL) {
                /* The inline constructor / destructor: Serialize::Base::Base,
                 * the vptr, CarEquipments::CarEquipments; the vptr,
                 * CarEquipments::~CarEquipments, Serialize::Base::~Base. */
                /* implicit: Base, CarEquipments, ~CarEquipments, ~Base
                 * (SettingSerialize's inline constructor and destructor) */
                SettingSerialize ser;

                g_00352EE8 = &ser;
                if (F_0014AE00(gt2k_menu::MEMCARD_SETTING_LOAD, course,
                               car->carCode, NULL, NULL, 0, 0, 0, NULL) != 0) {
                    loadWarning = ser.applySettings(car) != 0;
                    result = 1;
                }
            }
            state = gt2k_menu::SettingControl::REQUEST_NONE;
            gfx_vsync(1);
            break;
        }
        case gt2k_menu::SettingControl::REQUEST_SAVE: {
            GarageCar *car = getGameStatus()->getMenuStatus()->raceCar[0];

            if (car != NULL) {
                SettingSerialize ser;
                MemCardRaceInfo info;           /* (info.licence is left unset) */
                int kind;
                BlockPool *arena;
                void *work;

                ser.eq = *car;                  /* CarEquipments::operator= */
                /* implicit: CarEquipments::operator= */
                g_00352EE8 = &ser;
                race_name_split(getGameStatus()->menu.event, &kind, &info.test);
                info.licenceName = race_kind_text(kind);
                arena = org->unloadCourseData();    /* the course data's arena (0x002BB5B8) */
                work = arena->alloc(0);
                F_0014AE00(gt2k_menu::MEMCARD_SETTING_SAVE, course, car->carCode,
                           &info, work, arena->unitSize, 0, 0, NULL);
                arena->release(work);
            }
            state = gt2k_menu::SettingControl::REQUEST_NONE;
            gfx_vsync(1);
            break;
        }
        }
    } while (done == 0);
    s_settingCourse = 0;
    return result;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0015D210 __tfQ29gt2k_menu14SettingControl
extern "C" void *__tfQ29gt2k_menu14SettingControl(void)
{
    if (__tiQ29gt2k_menu14SettingControl[0] == 0) {             /* _name not set yet */
        __tfQ29gt2k_menu11ControlBase();                        /* the base's node first */
        __rtti_si(__tiQ29gt2k_menu14SettingControl, "Q29gt2k_menu14SettingControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu14SettingControl;
}
#endif

// FUNC 0x0015D260 gt2k_menu::SettingControl::~SettingControl(void)
/* implicit (game/menu/car_settings/T080_setting_control.h); egcs emits this copy, slot 0 of
 * the vtable 0x00320AB0: no vptr store; car's inline ~GarageCar (its vptrs,
 * CarInformation::~CarInformation(flags 0), the Cell vptr,
 * gt2k_menu::pCell::~pCell(flags 2)), render's inline ~RenderBase
 * (gt2k_menu::RootObject::~RootObject, GameObject::~GameObject), then
 * gt2k_menu::ControlBase::~ControlBase with the flags, which does the
 * delete. */
/* implicit: ~CarInformation, ~pCell, ~RootObject, ~GameObject, ~ControlBase
 * (compiler-generated destructor) */

/* ======================================================================== */
/* T080's out-of-line copies of inline functions (unreferenced: the calls   */
/* and the vtables use T036's)                                              */
/* ======================================================================== */

// FUNC 0x0015D308 gt2k_menu::Cell<GarageCar>::~Cell
/* inline in game/menu/framework/menu_list.h; egcs emits this copy (T080's; T036's is
 * 0x00125A10): the vptr (+0x8) = 0x00311C50, then
 * gt2k_menu::pCell::~pCell(this, flags), which does the delete. */
/* implicit: ~pCell (the inline destructor's copy) */

#ifndef BOOT_OLD_GCC
/* T080's copy (T036's is 0x00125A38; the type_info 0x004B1230 is shared,
 * the base list g_00320D48 is T080's): CarInformation's node and
 * Cell<GarageCar>'s first.  Weak: T036's definition wins at link time. */
// FUNC 0x0015D330 __tf9GarageCar
extern "C" __attribute__((weak)) void *__tf9GarageCar(void)
{
    if (__ti9GarageCar[0] == 0) {                               /* _name not set yet */
        __tf14CarInformation();                                 /* the bases' nodes first */
        __tfQ29gt2k_menut4Cell1Z9GarageCar();
        __rtti_class(__ti9GarageCar, "9GarageCar", g_00320D48, 2);
    }
    return __ti9GarageCar;
}
#endif

// FUNC 0x0015D390 GarageCar::~GarageCar
/* inline in game/menu/gtmode/T036_garage.h; egcs emits this copy (T080's; T036's is
 * 0x00125A98): the vptrs +0x1EC = 0x00311C20 and +0x8 = 0x00311C30,
 * CarInformation::~CarInformation(this + 0x10, 0), then the inline
 * Cell<GarageCar> destructor (vptr 0x00311C50,
 * gt2k_menu::pCell::~pCell(this, flags), which does the delete). */
/* implicit: ~CarInformation, ~pCell (the inline destructor's copy) */

#ifndef BOOT_OLD_GCC
/* T080's copy (T036's is 0x00125B00): single base pCell.  Weak, as above. */
// FUNC 0x0015D3F8 __tfQ29gt2k_menut4Cell1Z9GarageCar
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut4Cell1Z9GarageCar(void)
{
    if (__tiQ29gt2k_menut4Cell1Z9GarageCar[0] == 0) {           /* _name not set yet */
        __tfQ29gt2k_menu5pCell();                               /* the base's node first */
        __rtti_si(__tiQ29gt2k_menut4Cell1Z9GarageCar, "Q29gt2k_menut4Cell1Z9GarageCar",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1Z9GarageCar;
}
#endif
