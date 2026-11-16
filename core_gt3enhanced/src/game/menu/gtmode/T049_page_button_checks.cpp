/* T049 - the SelectableFuncData table of the GT-mode menu: per-button
 * availability checks.  0x001190D0..0x00119200.
 *
 * g_002959E8 (filled by the static initialiser) gives two pages their
 * button checks: "r/pro/pro" 25 (g_0030F508: 24 x event_check_unlocked,
 * then event_check_false) and "ts/ts_ot" 8 (g_0030F570: event_check_true,
 * event_check_has_wtr_1, event_check_has_wtr_2, 4 x event_check_true,
 * event_check_false).  ButtonList::setButtonChecks (T244) stores check i in
 * button i (the last one for the rest); a button is available when
 * check(ctx, its record) != 0.  The checks read the record as its name
 * (ButtonData::name, the event code of a race button); ctx (the
 * GtmodeControl) is not used. */
/* Retail's .rodata (0x0030F2B8..0x0030F590): the page names, the type names
 * in the reverse order of the classes' definitions (List / Cell<Button>,
 * OperatorList, List / Cell<OperatorHolder>, OperatorHolder,
 * SmartIndexT<int>, the GameStatus's garage lists, Operator: a base, last),
 * then the two check lists. */
/* Retail's .sdata: g_00351040 (the table count), read only by T043's code
 * (u009), so its address is anchored here (ref/data/u008/pending.md). */
// DATA_START 0x00351040
#include "game/save/T429_game_status.h"          /* getGameStatus, isLicenseSPassed, getRunningCar;
                                               T036 GarageCar */
#include "game/menu/framework/T211_page_manager.h"         /* SelectableFuncData (SmartIndexT<int>) */
#include "game/menu/framework/T206_button.h"               /* (seen, not used: Cell<Button>, the operator classes) */
#include "game/menu/framework/T244_button_list.h"          /* ButtonData::name (List<Button>) */
#include "game/data/T435_product_config.h"       /* product_config, eventValue */
#include "game/data/T483_car_information.h"      /* CarInformation::hasPart; T482 PART_LIGHTWEIGHT_* */
#include "game/menu/gtmode/T049_page_button_checks.h"
#include "game/menu/gtmode/gtmode_buttons_internal.h"

/* The table, .data 0x002959E8..0x00295A00 (2 entries of 0xC bytes), zero in
 * the image: T049_static_init fills it at start-up.  A C++ global without
 * an initialiser: zeros at the end of the file's .data, before the frame
 * info (as T048's g_00295380). */
gt2k_menu::SelectableFuncData g_002959E8[2];                                /* 0x002959E8 */

// FUNC 0x001190D0 event_check_false
int event_check_false(void *ctx, gt2k_menu::ButtonData *data)
{
    return 0;
}

// FUNC 0x001190D8 event_check_true
int event_check_true(void *ctx, gt2k_menu::ButtonData *data)
{
    return 1;
}

// FUNC 0x001190E0 event_check_unlocked
/* The event's requirement in the event data base (ProductConfig::
 * eventValue) <= GameStatus::isLicenseSPassed() (0 / 1); the same test as
 * T036's event_unlocked. */
int event_check_unlocked(void *ctx, gt2k_menu::ButtonData *data)
{
    int passed = getGameStatus()->isLicenseSPassed();
    return passed >= product_config()->eventValue(data->name);
}

// FUNC 0x00119130 event_check_has_wtr_1
/* Check 1 of "ts/ts_ot": the running car owns weight reduction stage 1
 * (part 0x1B, CarInformation::hasPart on the GarageCar's base at +0x10;
 * T036's text_price_wtr_1 prices it); 0 without a running car.  (? the
 * stage 2 button: the stages are bought in order.) */
int event_check_has_wtr_1(void *ctx, gt2k_menu::ButtonData *data)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car == 0)
        return 0;
    return car->hasPart(PART_LIGHTWEIGHT_STAGE1);
}

// FUNC 0x00119170 event_check_has_wtr_2
/* Check 2 of "ts/ts_ot": the same with stage 2 (part 0x1C,
 * text_price_wtr_2; ? the stage 3 button). */
int event_check_has_wtr_2(void *ctx, gt2k_menu::ButtonData *data)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car == 0)
        return 0;
    return car->hasPart(PART_LIGHTWEIGHT_STAGE2);
}

/* The table's entry count, .sdata 0x00351040 (a constant word, 2), read by
 * GtmodeControl::begin (T043): T049's only .sdata, defined here as T048's
 * g_00350FF4. */
const int g_00351040 = sizeof(g_002959E8) / sizeof(g_002959E8[0]);

/* The check lists of the two pages, .rodata 0x0030F508 and 0x0030F570.
 * File-local consts: retail has them after T049's type names, where egcs
 * writes a const with internal linkage (u004's notes, fact 1; one with an
 * extern declaration would come at its definition, before them). */
typedef int (*ButtonCheckFunc)(void *ctx, gt2k_menu::ButtonData *data);   /* (the address scanner
                                               reads the definitions below only so) */
static const ButtonCheckFunc g_0030F508[25] = {  /* 0x0030F508 */
    event_check_unlocked, event_check_unlocked, event_check_unlocked, event_check_unlocked,
    event_check_unlocked, event_check_unlocked, event_check_unlocked, event_check_unlocked,
    event_check_unlocked, event_check_unlocked, event_check_unlocked, event_check_unlocked,
    event_check_unlocked, event_check_unlocked, event_check_unlocked, event_check_unlocked,
    event_check_unlocked, event_check_unlocked, event_check_unlocked, event_check_unlocked,
    event_check_unlocked, event_check_unlocked, event_check_unlocked, event_check_unlocked,
    event_check_false
};
static const ButtonCheckFunc g_0030F570[8] = {   /* 0x0030F570 */
    event_check_true, event_check_has_wtr_1, event_check_has_wtr_2, event_check_true,
    event_check_true, event_check_true, event_check_true, event_check_false
};

/* One table entry: the inline constructor of the original's
 * SelectableFuncData (see T049_static_init).  The lists are const; the
 * table's member is not (T211), hence the cast (no code). */
static inline void set_checks(gt2k_menu::SelectableFuncData *e, const char *page,
                              const ButtonCheckFunc *checks, int count)
{
    e->page = page;
    e->checks = (ButtonCheckFunc *)checks;
    e->count = count;
}

// FUNC 0x001191B0 T049_static_init
/* Static initialiser (.ctors entry 0x0028F060): fills g_002959E8.
 * Compiler-generated in the original, as T048's: the table was defined here
 * with SelectableFuncData objects built by an inline constructor, which egcs
 * builds at run time (build_vec_init).  Written out the way build_vec_init
 * expands it (a walking cursor, a copy of it per element, the dead count of
 * the elements left): an exact form, not the original; it stores the same two
 * entries. */
void T049_static_init(void)
{
    gt2k_menu::SelectableFuncData *base = g_002959E8;
    int left = 1;                           /* build_vec_init's iterator: elements - 1 */

    { gt2k_menu::SelectableFuncData *e = base; set_checks(e, "r/pro/pro", g_0030F508, 25); }
    base++;
    left--;
    { gt2k_menu::SelectableFuncData *e = base; set_checks(e, "ts/ts_ot", g_0030F570, 8); }
    base++;
    left--;
}
