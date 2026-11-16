/* T427 (0x00217C30..0x00218250): gt2k_game::MenuStatus (? name, as in u001 /
 * u013), the menu state at GameStatus+0x307C8: the page the menus go to
 * next, the current event, the cars of the shop / garage screens and of the
 * race modes, remembered cursors and arcade choices.
 *
 * Then T427's out-of-line copies of the header-inline GarageCar /
 * Cell<GarageCar> destructors and typeinfo functions (? why egcs emitted
 * them here; the constructor's cleanups expand the destructor inline).  All
 * four are unreferenced: the vtables use T036's copies
 * (0x00125A10..0x00125B00).  The typeinfo functions are weak here, so T036's
 * definitions win, as at the original link.
 *
 * Retail's .rodata (0x003403E0..0x003404A0): the GarageCar vtables (its
 * CarInformation part's, its own, Cell<GarageCar>'s: weak copies, unreferenced
 * since T036's win), then the type names GarageCarList, List<GarageCar>,
 * GarageCar, Cell<GarageCar> (with GarageCar's base list g_00340470): the
 * original saw the garage car list, not the GameStatus.  .sdata: "root". */
// DATA_START 0x003403E0
#include "game/save/game_options_internal.h"
#include "cxxrt.h"                      /* type_info, __rtti_si (GCC 14's typeinfo functions) */
#include "game/menu/gtmode/T232_menu_car_spec.h"    /* MenuCarSpec, menu_car_spec */
#include "game/save/T429_garage_car_list.h"  /* (seen, not used: GarageCarList's type names) */
#include "game/save/T427_menu_status.h"
#ifndef BOOT_OLD_GCC
#include "game/menu/gtmode/T036_garage.h"           /* __ti9GarageCar, __tf..Cell1Z9GarageCar (GCC 14 only:
                                           its menu classes are no part of retail's T427) */
#include "runtime/cxx/T548_tinfo.h"            /* __rtti_class, __class_type_info::base_info */
#endif

/* A GarageCar's CarInformation base (+0x10) without the null test of a
 * pointer conversion (arcade_car_select_internal.h / u005's T026 have the same helper): egcs tests
 * a static_cast of the dereferenced car for null (movz), a reference bound
 * to it not; as an inline call the conversion is done after
 * menu_car_spec(), as retail (MenuStatus::setCar). */
static inline CarInformation *garageCarInfo(GarageCar *car)
{
    CarInformation &info = *car;

    return &info;
}

#ifndef BOOT_OLD_GCC
/* T427's copy of GarageCar's base list (the same words as T036's
 * g_00311EA0): {Cell<GarageCar> 0x004B1350, offset 0, public},
 * {CarInformation 0x004B2A50, offset 0x10, public}. */
extern const __class_type_info::base_info g_00340470[2];   /* 0x00340470 */
#endif

namespace gt2k_game {

// FUNC 0x00217C30 gt2k_game::MenuStatus::MenuStatus
MenuStatus::MenuStatus()
    : pageArg(0), nextMode(0), raceMode(0), runMode(8),
#ifdef BOOT_OLD_GCC
      /* the arrays from compound literals (a GNU extension of egcs; retail
       * stores them in member order, before focusCar / after carItem) */
      raceCarKind((int[2]){ RACE_CAR_NONE, RACE_CAR_NONE }),
#else
      raceCarKind{ RACE_CAR_NONE, RACE_CAR_NONE },
#endif
      car(&focusCar), carItem(0),
#ifdef BOOT_OLD_GCC
      raceCar((GarageCar *[2]){ 0, 0 }),
#else
      raceCar{ 0, 0 },
#endif
      color(0), garageIndex(0), wheelIndex(0),
      parts(0x58), arcadeMode(0), arcadeLevel(0), arcadeCourse(0), arcadeClass(0),
      lastArcadeCar(0), gameCardPort(0), gameCardPort1D(1), gameCardPort1C(1)
{
    /* implicit: CursorMemory, GameOptions, ~CarInformation, GarageCar, ~pCell (compiler-generated:
     * bases, members, deleting form) */
    /* Members in order: the scalars before focusCar (+0x40, +0xC4..+0xD4),
     * the two GarageCars (GarageCar::GarageCar), car .. wheelIndex, cursor
     * (CursorMemory::CursorMemory), parts .. gameCardPort1C, the 8 options
     * (GameOptions::GameOptions); the body clears the three strings.  If a
     * GameOptions constructor throws, the cleanups run the two GarageCars'
     * inline destructors (both vptrs, CarInformation::~CarInformation(flags
     * 0), Cell<GarageCar>'s vptr, gt2k_menu::pCell::~pCell(flags 0)),
     * raceCarBuf2 first, and rethrow. */


    page[0] = 0;
    event[0] = 0;
    wheelName[0] = 0;
}

/* Back to the start state (str84, the cursor memory, the game card ports and
 * the GameOptions are kept).  (Retail's store order: replayCount after
 * wheelIndex, lastArcadeCar last.) */
// FUNC 0x00217DF8 gt2k_game::MenuStatus::reset
void MenuStatus::reset()
{
    page[0] = 0;
    pageArg = 0;
    event[0] = 0;
    nextMode = 0;
    raceMode = 0;
    runMode = 8;
    raceCarKind[0] = RACE_CAR_NONE;
    raceCarKind[1] = RACE_CAR_NONE;
    car = &focusCar;
    carItem = 0;
    raceCar[0] = 0;
    raceCar[1] = 0;
    wheelName[0] = 0;
    color = 0;
    garageIndex = 0;
    wheelIndex = 0;
    replayCount = 0;
    parts = 0x58;
    arcadeMode = 0;
    arcadeLevel = 0;
    arcadeCourse = 0;
    arcadeClass = 0;
    replayViewMode = -1;
    lastArcadeCar = 0;
}

/* NULL clears the page (pageArg -1). */
// FUNC 0x00217E70 gt2k_game::MenuStatus::setPage
void MenuStatus::setPage(const char *name, int arg)
{
    if (name != 0) {
        strcpy(page, name);
        pageArg = arg;
    } else {
        page[0] = 0;
        pageArg = -1;
    }
}

/* Copies the page name to `out' ("root" if there is none; the strcpy of
 * the literal is inline). */
// FUNC 0x00217EC0 gt2k_game::MenuStatus::getPage
const char *MenuStatus::getPage(char *out)
{
    if (page[0] == 0)
        strcpy(out, "root");
    else
        strcpy(out, page);
    return page;
}

/* The car of player 1 (player1 != 0) or 2 becomes car `code', built in that
 * player's buffer. */
// FUNC 0x00217F20 gt2k_game::MenuStatus::setRaceCarByCode
void MenuStatus::setRaceCarByCode(const char *code, int player1)
{
    GarageCar  *buffer = player1 ? &focusCar : &raceCarBuf2;
    GarageCar **slot   = player1 ? &raceCar[0] : &raceCar[1];
    int        *kind   = player1 ? &raceCarKind[0] : &raceCarKind[1];

    buffer->setCar(code);
    *slot = buffer;
    *kind = RACE_CAR_BY_CODE;
}

/* The car of player 1 / 2 is `car' itself (a garage entry). */
// FUNC 0x00217F88 gt2k_game::MenuStatus::setRaceCar
void MenuStatus::setRaceCar(GarageCar *garageCar, int player1)
{
    GarageCar **slot = player1 ? &raceCar[0] : &raceCar[1];
    int        *kind = player1 ? &raceCarKind[0] : &raceCarKind[1];

    *slot = garageCar;
    *kind = RACE_CAR_GARAGE;
}

/* The car of player 1 / 2 is a copy of `car' in that player's buffer (u017:
 * a guest garage entry). */
// FUNC 0x00217FB0 gt2k_game::MenuStatus::setRaceCarCopy
void MenuStatus::setRaceCarCopy(const GarageCar *source, int player1)
{
    GarageCar  *buffer = player1 ? &focusCar : &raceCarBuf2;
    GarageCar **slot   = player1 ? &raceCar[0] : &raceCar[1];
    int        *kind   = player1 ? &raceCarKind[0] : &raceCarKind[1];

    buffer->assign(*source);
    *slot = buffer;
    *kind = RACE_CAR_COPY;
}

// FUNC 0x00218018 gt2k_game::MenuStatus::setEvent
void MenuStatus::setEvent(const char *code)
{
    if (code != 0)
        strcpy(event, code);
    else
        event[0] = 0;
}

// FUNC 0x00218048 gt2k_game::MenuStatus::setStr84
void MenuStatus::setStr84(const char *s)
{
    if (s != 0)
        strcpy(str84, s);
    else
        str84[0] = 0;
}

/* Also recomputes the menu car spec for it (its CarInformation, car + 0x10,
 * with no NULL check: a reference conversion, not a pointer one). */
// FUNC 0x00218078 gt2k_game::MenuStatus::setCar
void MenuStatus::setCar(GarageCar *garageCar)
{
    car = garageCar;
    menu_car_spec()->setCar(garageCarInfo(garageCar));
}

// FUNC 0x002180B0 gt2k_game::MenuStatus::setFocusCar
void MenuStatus::setFocusCar(const char *code)
{
    if (code != 0)
        focusCar.setCar(code);
    setCar(&focusCar);
}

// FUNC 0x00218100 gt2k_game::MenuStatus::clearFocusCar
void MenuStatus::clearFocusCar()
{
    car = &focusCar;
    car->valid = 0;
}

} /* namespace gt2k_game */

// FUNC 0x00218110 gt2k_menu::Cell<GarageCar>::~Cell
/* implicit: ~pCell; inline in game/menu/framework/menu_list.h; egcs emits this copy (T427's): the vptr (+0x8)
 * = 0x00311C50, then gt2k_menu::pCell::~pCell(this, flags), which does the
 * delete. */

#ifndef BOOT_OLD_GCC
/* T427's copy (the type_info 0x004B1230 is shared; base list g_00340470 is
 * T427's).  It calls T036's __tfQ29gt2k_menut4Cell1Z9GarageCar (0x00125B00),
 * not T427's own copy. */
// FUNC 0x00218138 __tf9GarageCar
extern "C" __attribute__((weak)) void *__tf9GarageCar(void)
{
    if (__ti9GarageCar[0] == 0) {                       /* _name not set yet */
        __tf14CarInformation();                         /* the bases' nodes first (results unused) */
        __tfQ29gt2k_menut4Cell1Z9GarageCar();
        __rtti_class(__ti9GarageCar, "9GarageCar", g_00340470, 2);
    }
    return __ti9GarageCar;
}
#endif

// FUNC 0x00218198 GarageCar::~GarageCar
/* implicit: ~CarInformation, ~pCell; inline in game/menu/gtmode/T036_garage.h; egcs emits this copy (T427's): the vptrs
 * +0x1EC = 0x00311C20 and +0x8 = 0x00311C30,
 * CarInformation::~CarInformation(this + 0x10, 0), then the inline
 * Cell<GarageCar> destructor (vptr 0x00311C50, gt2k_menu::pCell::~pCell(this,
 * flags)). */

#ifndef BOOT_OLD_GCC
/* T427's copy. */
// FUNC 0x00218200 __tfQ29gt2k_menut4Cell1Z9GarageCar
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut4Cell1Z9GarageCar(void)
{
    if (__tiQ29gt2k_menut4Cell1Z9GarageCar[0] == 0) {  /* _name not set yet */
        __tfQ29gt2k_menu5pCell();                       /* the base's node first (result unused) */
        __rtti_si(__tiQ29gt2k_menut4Cell1Z9GarageCar, "Q29gt2k_menut4Cell1Z9GarageCar",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1Z9GarageCar;
}
#endif
