/* T084_setting_checks.h - the SelectableFuncData table of the car setting
 * screen and its checks (translation unit T084, unit u036),
 * 0x001677A0..0x00169780.
 *
 * g_002AD058 (23 pages sorted by name, count g_00351994; filled by
 * T084_static_init) gives the
 * setting pages the availability check of each button.  SettingControl::
 * begin (T080, 0x0015CA98) gives it to its PageManager (selectable table
 * +0xB00, ctx the SettingControl); ButtonList::setButtonChecks (T244) stores
 * check i in button i (the last one for the rest), and a button is available
 * when check(ctx, its record) != 0.
 *   - "chg/change_*" pages: one setting_owns_<part> per item, 1 if
 *     GameStatus's setting car (MenuStatus::settingCar) owns the part
 *     (CarInformation::hasPart; the part ids of T482 in the comments);
 *   - "set/adjast_*" [sic] pages: one setting_adjust_<setting> per item, 1 if
 *     the setting of the setting car can be adjusted: the fitted part's
 *     record has a range that is not a single value (can_adjust_spring_rate ..
 *     can_adjust_vcd, which T081 calls directly).
 * 0 without a setting car.  None of the checks reads its arguments.
 *
 * The checks take the SelectableFuncData signature (the first pass had
 * (SettingControl *ctl, const char *item)).  The first pass's
 * SettingPageChecks (u036) is T211's SelectableFuncData.
 *
 * The rest of the TU is code egcs emitted for RegurationResult (T045,
 * T045_gtmode_page_checks.h): unreferenced copies of its inline destructor
 * (0x00169710) and of __tf16RegurationResult (0x00169740). */
#ifndef GAME_T084_SETTING_CHECKS_H
#define GAME_T084_SETTING_CHECKS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::SelectableFuncData (g_002AD058) */

class GarageCar;                            /* T036 (pointers only) */
namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
}

/* The first pass's name of the record (u036). */
typedef gt2k_menu::SelectableFuncData SettingPageChecks;    /* {page, checks, count}, 0xC bytes */

/* 1 if car meets the regulations of the current event (MenuStatus::event,
 * T045's RegurationResult of T489 0x0023A438), 1 when there is no event
 * ("").  The code converts car to its CarInformation base without a NULL
 * test, as egcs does for a reference: the original probably took
 * `GarageCar &car` (or check_regulation a CarInformation &); the calls
 * compile the same. */
int car_meets_event_regulations(GarageCar *car);    /* 0x001677A0 (first pass: F_001677A0) */

/* ---- "chg/change_*": the setting car owns part N -------------------------- */
int setting_owns_cpu_off(void *ctx, gt2k_menu::ButtonData *data);          /* 0x00167840 0x08 */
int setting_owns_cpu_on(void *ctx, gt2k_menu::ButtonData *data);           /* 0x00167888 0x09 */
int setting_owns_muf_normal(void *ctx, gt2k_menu::ButtonData *data);       /* 0x001678D0 0x24 */
int setting_owns_muf_sports(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167918 0x25 */
int setting_owns_muf_semi_racing(void *ctx, gt2k_menu::ButtonData *data);  /* 0x00167960 0x26 */
int setting_owns_muf_racing(void *ctx, gt2k_menu::ButtonData *data);       /* 0x001679A8 0x27 */
int setting_owns_tur_off(void *ctx, gt2k_menu::ButtonData *data);          /* 0x001679F0 0x4E */
int setting_owns_tur_stage1(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167A38 0x4F */
int setting_owns_tur_stage2(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167A80 0x50 */
int setting_owns_tur_stage3(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167AC8 0x51 */
int setting_owns_tur_stage4(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167B10 0x52 */
int setting_owns_coo_normal(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167B58 0x17 */
int setting_owns_coo_1(void *ctx, gt2k_menu::ButtonData *data);            /* 0x00167BA0 0x18 */
int setting_owns_coo_2(void *ctx, gt2k_menu::ButtonData *data);            /* 0x00167BE8 0x19 */
int setting_owns_nat_off(void *ctx, gt2k_menu::ButtonData *data);          /* 0x00167C30 0x28 */
int setting_owns_nat_stage1(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167C78 0x29 */
int setting_owns_nat_stage2(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167CC0 0x2A */
int setting_owns_nat_stage3(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167D08 0x2B */
int setting_owns_sus_normal(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167D50 0x32 */
int setting_owns_sus_sports(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167D98 0x33 */
int setting_owns_sus_semi_racing(void *ctx, gt2k_menu::ButtonData *data);  /* 0x00167DE0 0x34 */
int setting_owns_sus_racing(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167E28 0x35 */
int setting_owns_brc_off(void *ctx, gt2k_menu::ButtonData *data);          /* 0x00167E70 0x02 */
int setting_owns_brc_on(void *ctx, gt2k_menu::ButtonData *data);           /* 0x00167EB8 0x03 */
int setting_owns_brk_normal(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167F00 0x00 */
int setting_owns_brk_racing(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167F48 0x01 */
int setting_owns_msn_normal(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00167F90 0x13 */
int setting_owns_msn_close(void *ctx, gt2k_menu::ButtonData *data);        /* 0x00167FD8 0x14 */
int setting_owns_msn_super_close(void *ctx, gt2k_menu::ButtonData *data);  /* 0x00168020 0x15 */
int setting_owns_msn_variable(void *ctx, gt2k_menu::ButtonData *data);     /* 0x00168068 0x16 */
int setting_owns_clt_normal(void *ctx, gt2k_menu::ButtonData *data);       /* 0x001680B0 0x04 */
int setting_owns_clt_big(void *ctx, gt2k_menu::ButtonData *data);          /* 0x001680F8 0x05 */
int setting_owns_clt_twin(void *ctx, gt2k_menu::ButtonData *data);         /* 0x00168140 0x06 */
int setting_owns_clt_triple(void *ctx, gt2k_menu::ButtonData *data);       /* 0x00168188 0x07 */
int setting_owns_fly_normal(void *ctx, gt2k_menu::ButtonData *data);       /* 0x001681D0 0x0F */
int setting_owns_fly_light(void *ctx, gt2k_menu::ButtonData *data);        /* 0x00168218 0x10 */
int setting_owns_fly_crmo(void *ctx, gt2k_menu::ButtonData *data);         /* 0x00168260 0x11 */
int setting_owns_fly_light_crmo(void *ctx, gt2k_menu::ButtonData *data);   /* 0x001682A8 0x12 */
int setting_owns_lsd_normal(void *ctx, gt2k_menu::ButtonData *data);       /* 0x001682F0 0x1E */
int setting_owns_lsd_2way(void *ctx, gt2k_menu::ButtonData *data);         /* 0x00168338 0x20 */
int setting_owns_lsd_15way(void *ctx, gt2k_menu::ButtonData *data);        /* 0x00168380 0x21 */
int setting_owns_lsd_1way(void *ctx, gt2k_menu::ButtonData *data);         /* 0x001683C8 0x1F */
int setting_owns_lsd_variable(void *ctx, gt2k_menu::ButtonData *data);     /* 0x00168410 0x22 */
int setting_owns_lsd_aycc(void *ctx, gt2k_menu::ButtonData *data);         /* 0x00168458 0x23 */
int setting_owns_shf_normal(void *ctx, gt2k_menu::ButtonData *data);       /* 0x001684A0 0x2E */
int setting_owns_shf_propeller(void *ctx, gt2k_menu::ButtonData *data);    /* 0x001684E8 0x2F */
int setting_owns_tir_f_normal(void *ctx, gt2k_menu::ButtonData *data);     /* 0x00168530 0x38 */
int setting_owns_tir_f_control(void *ctx, gt2k_menu::ButtonData *data);    /* 0x00168578 0x41 */
int setting_owns_tir_f_sports(void *ctx, gt2k_menu::ButtonData *data);     /* 0x001685C0 0x39 */
int setting_owns_tir_f_racing_sh(void *ctx, gt2k_menu::ButtonData *data);  /* 0x00168608 0x3A */
int setting_owns_tir_f_racing_h(void *ctx, gt2k_menu::ButtonData *data);   /* 0x00168650 0x3B */
int setting_owns_tir_f_racing_mh(void *ctx, gt2k_menu::ButtonData *data);  /* 0x00168698 0x3C */
int setting_owns_tir_f_racing_m(void *ctx, gt2k_menu::ButtonData *data);   /* 0x001686E0 0x3D */
int setting_owns_tir_f_racing_ms(void *ctx, gt2k_menu::ButtonData *data);  /* 0x00168728 0x3E */
int setting_owns_tir_f_racing_s(void *ctx, gt2k_menu::ButtonData *data);   /* 0x00168770 0x3F */
int setting_owns_tir_f_racing_ss(void *ctx, gt2k_menu::ButtonData *data);  /* 0x001687B8 0x40 */
int setting_owns_ayc_off(void *ctx, gt2k_menu::ButtonData *data);          /* 0x00168800 0x1E (the AYC page's copy) */
int setting_owns_ayc_on(void *ctx, gt2k_menu::ButtonData *data);           /* 0x00168848 0x23 */
int setting_owns_asm_off(void *ctx, gt2k_menu::ButtonData *data);          /* 0x00168890 0x53 */
int setting_owns_asm_on(void *ctx, gt2k_menu::ButtonData *data);           /* 0x001688D8 0x54 */
int setting_owns_tcs_off(void *ctx, gt2k_menu::ButtonData *data);          /* 0x00168920 0x36 */
int setting_owns_tcs_on(void *ctx, gt2k_menu::ButtonData *data);           /* 0x00168968 0x37 */
int setting_owns_vcd_off(void *ctx, gt2k_menu::ButtonData *data);          /* 0x001689B0 0x55 */
int setting_owns_vcd_on(void *ctx, gt2k_menu::ButtonData *data);           /* 0x001689F8 0x56 */

/* ---- can a setting of the setting car be adjusted? -------------------------
 * The fitted part's record (CarEquipments get*Record, T482; the record
 * getter is called again for every byte) has two different limits.  T081
 * calls them directly (u033 passed an argument that is not read). */
/* (first pass: F_00168A40 .. F_001691A0, in this order) */
int can_adjust_spring_rate(void);           /* 0x00168A40 spring rate (SuspensionRecord) */
int can_adjust_ride_height(void);           /* 0x00168AD0 ride height (16-bit limits) */
int can_adjust_damper(void);                /* 0x00168B60 damper: only the sports / semi racing
                                               suspensions (0x33 / 0x34) */
int can_adjust_bound_rebound(void);         /* 0x00168BC8 bound / rebound: only the racing
                                               suspension (0x35) */
int can_adjust_camber(void);                /* 0x00168C10 camber */
int can_adjust_toe(void);                   /* 0x00168CA0 toe */
int can_adjust_stabiliser(void);            /* 0x00168D30 stabiliser: unless both level counts are 1 */
int can_adjust_brake_balance(void);         /* 0x00168DA8 brake balance: unless both counts are 1
                                               (BrakeControllerRecord) */
int can_adjust_lsd_initial(void);           /* 0x00168E20 LSD initial torque (LsdRecord) */
int can_adjust_lsd_accel(void);             /* 0x00168EB0 LSD acceleration */
int can_adjust_lsd_decel(void);             /* 0x00168F40 LSD deceleration */
int can_adjust_final_gear(void);            /* 0x00168FD0 the final gear (the gear ratio pages): the
                                               transmission's finalGearRatioMax / Min differ */
int can_adjust_aero(void);                  /* 0x00169038 aero (RacingModifyRecord) */
int can_adjust_ayc(void);                   /* 0x001690C8 AYC: the LSD fitted is the AYC (0x23) */
int can_adjust_asm(void);                   /* 0x00169110 ASM: AsccRecord::count0 is not 1 */
int can_adjust_tcs(void);                   /* 0x00169158 TCS: TcsRecord::count is not 1 */
int can_adjust_vcd(void);                   /* 0x001691A0 VCD (DrivetrainRecord vcdMin / vcdMax) */

/* ---- "set/adjast_*": out-of-line wrappers of the checks above ------------- */
int setting_adjust_spring(void *ctx, gt2k_menu::ButtonData *data);         /* 0x00169208 can_adjust_spring_rate */
int setting_adjust_height(void *ctx, gt2k_menu::ButtonData *data);         /* 0x00169228 can_adjust_ride_height */
int setting_adjust_damper(void *ctx, gt2k_menu::ButtonData *data);         /* 0x00169248 can_adjust_damper */
int setting_adjust_bound_rebound(void *ctx, gt2k_menu::ButtonData *data);  /* 0x00169268 can_adjust_bound_rebound */
int setting_adjust_camber(void *ctx, gt2k_menu::ButtonData *data);         /* 0x00169288 can_adjust_camber */
int setting_adjust_toe(void *ctx, gt2k_menu::ButtonData *data);            /* 0x001692A8 can_adjust_toe */
int setting_adjust_stabiliser(void *ctx, gt2k_menu::ButtonData *data);     /* 0x001692C8 can_adjust_stabiliser */
int setting_adjust_brake_balance(void *ctx, gt2k_menu::ButtonData *data);  /* 0x001692E8 can_adjust_brake_balance */
int setting_adjust_lsd_initial(void *ctx, gt2k_menu::ButtonData *data);    /* 0x00169308 can_adjust_lsd_initial */
int setting_adjust_lsd_accel(void *ctx, gt2k_menu::ButtonData *data);      /* 0x00169328 can_adjust_lsd_accel */
int setting_adjust_lsd_decel(void *ctx, gt2k_menu::ButtonData *data);      /* 0x00169348 can_adjust_lsd_decel */
int setting_adjust_gear_ratio(void *ctx, gt2k_menu::ButtonData *data);     /* 0x00169368 can_adjust_final_gear */
int setting_adjust_aero(void *ctx, gt2k_menu::ButtonData *data);           /* 0x00169388 can_adjust_aero */
int setting_adjust_ayc(void *ctx, gt2k_menu::ButtonData *data);            /* 0x001693A8 can_adjust_ayc */
int setting_adjust_asm(void *ctx, gt2k_menu::ButtonData *data);            /* 0x001693C8 can_adjust_asm */
int setting_adjust_tcs(void *ctx, gt2k_menu::ButtonData *data);            /* 0x001693E8 can_adjust_tcs */
int setting_adjust_vcd(void *ctx, gt2k_menu::ButtonData *data);            /* 0x00169408 can_adjust_vcd */

/* T084 .data 0x002AD058..0x002AD16C: {"chg/change_brk_balance", g_003519A8,
 * 2} .. {"set/adjast_sus", g_00322FF8, 8}.  Filled by T084_static_init
 * (.ctors entry 0x0028F0AC; compiler-generated in the original).
 * "chg/change_tir_rear" gets g_00322FD0, which holds the same ten front-tyre
 * checks as g_00322FA8. */
extern gt2k_menu::SelectableFuncData g_002AD058[23];   /* 0x002AD058 */
/* Its entry count (T084 .sdata; SettingControl::begin, T080). */
extern const int g_00351994;                /* 0x00351994 gp-0x605C (=0x17) */
void T084_static_init(void);                /* 0x00169428 */

/* The check arrays of the pages (g_00351998 .. g_003519D8 in T084's .sdata,
 * g_00322F08 .. g_00323028 in its .rodata, after the type names) are
 * file-local constants of T084 (src/game/menu/car_settings/T084_setting_checks.cpp): retail
 * has them where egcs writes a const with internal linkage, so the original
 * declared no extern (u036's data pass).  Only T084_static_init names them. */

#endif /* GAME_T084_SETTING_CHECKS_H */
