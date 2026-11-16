/* T045_gtmode_page_checks.h - the ValidFuncData table of the GT-mode menu
 * and RegurationResult (translation unit T045, unit u009),
 * 0x0011BC18..0x0011C9C8.
 *
 * g_00296750 (34 entries, sorted by name, filled by T045_static_init) maps
 * the links and exec names of the GT-mode buttons to checks run before the
 * button acts.  GtmodeControl::begin (T043) gives it to its PageManager
 * (valid table +0xB24, count g_00351100, ctx the GtmodeControl);
 * PageManager::callValidFunc (T211) calls func(ctx, rec) for the button's
 * link, then for its exec name.  A check returns NULL to go on, or the page
 * to open as a popup instead (a message: "00cmn/no_money", ...); most play
 * menu sound 4 (refused) first.  The buying checks compare with the credits
 * (GameStatus +0x18, 64-bit).
 *
 * The checks take the ValidFuncData signature (the first pass had
 * (GtmodeControl *ctl, const char *arg)): ctx is not used, and the string
 * they read is the record's name (ButtonData::name, +0x00: a licence letter,
 * a part, a wheel, a course). */
#ifndef GAME_T045_GTMODE_PAGE_CHECKS_H
#define GAME_T045_GTMODE_PAGE_CHECKS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ValidFuncData (g_00296750) */

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
}

/* RegurationResult: in its own header (units see it without the checks),
 * included where this header defined it. */
#include "game/menu/gtmode/T045_reguration_result.h"

/* The ValidFuncData checks (keys in comments). */
/* "00cmn/cd_buy_sure": the dealer's car (MenuStatus::car): sold here (car
 * flag bit 0), the credits, fewer than 200 cars. */
const char *check_cd_buy_sure(void *ctx, gt2k_menu::ButtonData *rec);        /* 0x0011BC18 */
/* "hm/hm_buy_sure": the same at the used car shop (flag bit 1). */
const char *check_hm_buy_sure(void *ctx, gt2k_menu::ButtonData *rec);        /* 0x0011BCE0 */
/* "hm/hm_sell_sure": MenuStatus::car may be sold (flag bit 1). */
const char *check_hm_sell_sure(void *ctx, gt2k_menu::ButtonData *rec);       /* 0x0011BDA8 */
/* "at/at", "go_machine_test", "hm/hm_fr", "mt/mt", "ts/ts", "ts/ts_bs",
 * "ts/ts_dr", "ts/ts_eg", "ts/ts_ot", "ts/ts_tr": a car is ridden. */
const char *check_ride_car(void *ctx, gt2k_menu::ButtonData *rec);           /* 0x0011BE00 */
/* "at/ws/ws": the ridden car takes other wheels (flag bit 2). */
const char *check_ws(void *ctx, gt2k_menu::ButtonData *rec);                 /* 0x0011BE40 */
/* "hm/hm_wheel": the same for MenuStatus::car. */
const char *check_hm_wheel(void *ctx, gt2k_menu::ButtonData *rec);           /* 0x0011BEA8 */
/* "ts/ts_buy_sure1" .. "5": the part rec->name (T203 part_id_of_name) for
 * the ridden car: not owned, exists, the credits. */
const char *check_ts_buy_sure(void *ctx, gt2k_menu::ButtonData *rec);        /* 0x0011BF00 */
/* "ts/ts_buy_sure_all_tire": the tyres 0x3A..0x40 the ridden car can have
 * and does not own, their total price. */
const char *check_ts_buy_sure_all_tire(void *ctx, gt2k_menu::ButtonData *rec); /* 0x0011BFE0 */
/* "00cmn/ws_buy_sure": the wheel rec->name (cut at its last '_') is not
 * owned, the credits for the T435 price "wheel". */
const char *check_ws_buy_sure(void *ctx, gt2k_menu::ButtonData *rec);        /* 0x0011C0C8 */
/* "lc/lc_a" .. "lc/lc_s": the licences before rec->name in g_002966B0 are
 * complete ("d" always open); else the entry's message. */
const char *check_license(void *ctx, gt2k_menu::ButtonData *rec);            /* 0x0011C160 */
/* "go_license": the last test of a licence (g_002966F8) needs its first
 * seven passed ("lc/no_last_exam"). */
const char *check_go_license(void *ctx, gt2k_menu::ButtonData *rec);         /* 0x0011C258 */
/* "go_race": the ridden car meets the event's regulations
 * ("00cmn/license_violation" for flags[1], else
 * "00cmn/regulation_violation"). */
const char *check_go_race(void *ctx, gt2k_menu::ButtonData *rec);            /* 0x0011C308 */
/* "go_free_run": a dirt course (course type 2 in race "ATAF0001") needs
 * dirt tyres (part 0x42). */
const char *check_go_free_run(void *ctx, gt2k_menu::ButtonData *rec);        /* 0x0011C3F8 */
/* "oil_change" / "wash_car": the credits for the T435 price "oil_change" /
 * "car_wash". */
const char *check_oil_change(void *ctx, gt2k_menu::ButtonData *rec);         /* 0x0011C4F8 */
const char *check_wash_car(void *ctx, gt2k_menu::ButtonData *rec);           /* 0x0011C558 */
/* "check_ride_car": asks first ("00cmn/cd_ride_sure", sound 7) if a car is
 * ridden, else rides garage car MenuStatus::garageIndex at once. */
const char *check_ride_sure(void *ctx, gt2k_menu::ButtonData *rec);          /* 0x0011C5B8 */

/* T045 .data, filled by T045_static_init (.ctors 0x0028F058; compiler-
 * generated in the original): g_00296750 and its count g_00351100, in their
 * own header (T043 sees them without RegurationResult). */
#include "game/menu/gtmode/T045_gtmode_valid_table.h"
void T045_static_init(void);                /* 0x0011C628 */

/* The licence order of check_license / check_go_license (T045 .data,
 * terminated by {NULL, -1}): g_002966B0 {"b", 0, NULL}, {"a", 1,
 * "lc/no_license_a"}, "ib", "ia", "s"; g_002966F8 {"L0B0008", 0} ..
 * {"L0R0008", 5}. */
struct LicenseEntry {
    const char *name;                       /* 0x0 */
    int         license;                    /* 0x4 0 B, 1 A, 2 IB, 3 IA, 4 S, 5 R */
    const char *message;                    /* 0x8 the popup when it is missing */
};
EGCS_SIZE(LicenseEntry, 0xC);
extern LicenseEntry g_002966B0[6];          /* 0x002966B0 */
extern LicenseEntry g_002966F8[7];          /* 0x002966F8 */

/* (__tf16RegurationResult / __ti16RegurationResult: T045_reguration_result.h) */

#endif /* GAME_T045_GTMODE_PAGE_CHECKS_H */
