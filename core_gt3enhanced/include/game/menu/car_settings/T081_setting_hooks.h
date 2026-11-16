/* T081_setting_hooks.h - the layout hooks of the car setting screen
 * (translation unit T081, units u033 0x0015D448..0x0015FC70 and u034
 * ..0x001638F8; one source file, src/game/menu/car_settings/T081_setting_hooks.cpp).
 *
 * T081_static_init fills g_002A9080 with 161 MenuHook entries (sorted by
 * name, count g_00351950; 162 in the enhanced engine: "gear_max_speed");
 * SettingControl::begin (T080, 0x0015CA98) gives the table to both layouts
 * of its PageManager (PageLayout::setHooks on common and own, ctx the
 * SettingControl), and
 * PageLayout::createObject (T229) calls create(ctx, layout, parent, item, p8,
 * pC, p10, p14) for an object item whose type is an entry's name.  The
 * entries are
 *   - create_callback_text (T239, 0x001C6E70) with p8 = a text_* callback
 *     below, called fn(ctl, buf, 0x80): the value of a setting or the name
 *     of a fitted part of the control's working copy of the setting car
 *     (SettingControl +0xE40), "----" when GameStatus has no setting car
 *     (MenuStatus::raceCar[0], read through getMenuStatus()) or the setting
 *     cannot be adjusted on it (the T084 checks can_adjust_spring_rate ..);
 *   - create_bool_item (T234, 0x001C50D8) with p8 = an attach_* predicate
 *     (the image of the part fitted to the working copy) or a disable_*
 *     predicate (the setting cannot be adjusted);
 *   - the gear chart hook (0x001C62C8, T237's GearChart) with p8 =
 *     gear_chart_car, "blink" (T205) and the two create functions below.
 * pC, p10 and p14 are 0 in every entry.
 *
 * Names: the hooks of u033 are named after their entry (text_<name>,
 * attach_<name>), u034's keep u034's setting_* names.  The text callbacks and
 * predicates take the SettingControl, as the owners define them (the table
 * stores them as ints; T036 does the same); the two create functions take
 * the MenuHookFn signature (T229; the first pass had (SettingControl *,
 * MenuFile *, void *parent, MenuElement *, ...) returning Object *). */
#ifndef GAME_T081_SETTING_HOOKS_H
#define GAME_T081_SETTING_HOOKS_H

#include "game/game.h"
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (SettingHook, g_002A9080) */

class GarageCar;                            /* T036 (pointers only) */
namespace gt2k_menu {
class SettingControl;                       /* T080 (pointers only) */
class Object;                               /* T220 */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
}

/* The first pass's name of the hook record (u034). */
typedef gt2k_menu::MenuHook SettingHook;    /* 0x18 bytes: name, create, p8, pC, p10, p14 */

/* ---- u033: the texts (p8 of create_callback_text) --------------------------
 * The setting values of the working copy (SettingControl::car); the units
 * are the SETTING texts g_002B1940 (T093: "kgf/mm", "mm", "Level", the
 * EUC-JP degree sign), formatted with fmt_sprintf (G001, 0x0024B3C8: %t
 * prints an int with `precision' decimals). */
void text_spring_rate_f(gt2k_menu::SettingControl *ctl, char *buf, int size);      /* 0x0015D448 "<rate>kgf/mm" */
void text_spring_rate_r(gt2k_menu::SettingControl *ctl, char *buf, int size);      /* 0x0015D4E0 */
void text_spring_height_f(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015D578 ride height, "<h>mm" */
void text_spring_height_r(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015D610 */
void text_spring_camber_f(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015D6A8 one decimal, degrees */
void text_spring_camber_r(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015D740 */
void text_spring_toe_f(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015D7D8 (toe - 0x80) * 5, degrees */
void text_spring_toe_r(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015D880 */
void text_spring_stabilizer_f(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015D928 "Level <n>" */
void text_spring_stabilizer_r(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015D9C0 */
void text_spring_brake_f(gt2k_menu::SettingControl *ctl, char *buf, int size);     /* 0x0015DA58 brake balance */
void text_spring_brake_r(gt2k_menu::SettingControl *ctl, char *buf, int size);     /* 0x0015DAF0 */
void text_lsd_int_f(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015DB88 LSD initial torque */
void text_lsd_int_r(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015DC20 0 with the AYC (LSD
                                               part 0x23) */
void text_lsd_acl_f(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015DCE8 acceleration */
void text_lsd_acl_r(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015DD80 */
void text_lsd_dec_f(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015DE18 deceleration */
void text_lsd_dec_r(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015DEB0 */
void text_spring_damper_f(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015DF48 "Level <n>" */
void text_spring_damper_r(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015DFE0 */
void text_spring_bound_f(gt2k_menu::SettingControl *ctl, char *buf, int size);     /* 0x0015E078 the damper's field,
                                               under the bound / rebound check */
void text_spring_bound_r(gt2k_menu::SettingControl *ctl, char *buf, int size);     /* 0x0015E110 */
void text_spring_rebound_f(gt2k_menu::SettingControl *ctl, char *buf, int size);   /* 0x0015E1A8 */
void text_spring_rebound_r(gt2k_menu::SettingControl *ctl, char *buf, int size);   /* 0x0015E240 */
/* Three decimals, while the setting car's transmission has gear N
 * (TransmissionRecord::gears; ratio 0, the reverse gear, and the final ratio
 * always); the enhanced engine: "----" for a CVT (Lupo 2, menus_b.md 1). */
void text_gear_ratio_0(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015E2D8 */
void text_gear_ratio_1(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015E370 */
void text_gear_ratio_2(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015E408 */
void text_gear_ratio_3(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015E4A0 */
void text_gear_ratio_4(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015E538 */
void text_gear_ratio_5(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015E5D0 */
void text_gear_ratio_6(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015E668 */
void text_gear_ratio_7(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015E700 */
void text_gear_ratio_final(gt2k_menu::SettingControl *ctl, char *buf, int size);   /* 0x0015E798 */
void text_gear_ratio_auto(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015E830 "Level <n>" when the
                                               transmission's auto level range (maxSpeedMin / Max) is not
                                               a single value */
/* The enhanced engine (new, Lupo 2 L2 0x0014A8F8; menus_b.md 2): hook
 * "gear_max_speed", the top speed of the working copy's gearing (unit, two
 * decimals), "----" for a CVT; shown only where the layout has the item. */
void text_gear_max_speed(gt2k_menu::SettingControl *ctl, char *buf, int size);
void text_aero_f(gt2k_menu::SettingControl *ctl, char *buf, int size);             /* 0x0015E8E0 downforce, two decimals */
void text_aero_r(gt2k_menu::SettingControl *ctl, char *buf, int size);             /* 0x0015E978 */
void text_elec_aycc(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015EA10 the rear LSD initial
                                               torque field (the AYC's level) */
void text_elec_ascc(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015EAA8 "Level <asc - 1>" */
void text_elec_tcsc(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015EB48 "Level <count -
                                               settingTCSC>" (TcsRecord, not checked for NULL) */
void text_elec_vcdc(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015EC10 "<vcd> %" */
/* "gear_chart" (p8 of the gear chart hook 0x001C62C8): the car whose gears
 * the GearChart (T237) draws, the working copy; NULL without a setting car. */
GarageCar *gear_chart_car(gt2k_menu::SettingControl *ctl);                         /* 0x0015ECA8 */
/* The names of the working copy's fitted parts (CarEquipments get*Name,
 * T482). */
void text_parts_eng_computer(gt2k_menu::SettingControl *ctl, char *buf, int size);     /* 0x0015ECE8 */
void text_parts_eng_muffler(gt2k_menu::SettingControl *ctl, char *buf, int size);      /* 0x0015ED68 */
void text_parts_eng_turbine(gt2k_menu::SettingControl *ctl, char *buf, int size);      /* 0x0015EDE8 */
void text_parts_eng_inter_cooler(gt2k_menu::SettingControl *ctl, char *buf, int size); /* 0x0015EE68 */
void text_parts_eng_displacement(gt2k_menu::SettingControl *ctl, char *buf, int size); /* 0x0015EEE8 */
void text_parts_eng_na_tune(gt2k_menu::SettingControl *ctl, char *buf, int size);      /* 0x0015EF68 */
void text_parts_eng_port_polish(gt2k_menu::SettingControl *ctl, char *buf, int size);  /* 0x0015EFE8 */
void text_parts_eng_engine_balance(gt2k_menu::SettingControl *ctl, char *buf, int size);   /* 0x0015F068 */
void text_parts_sus_s_kit(gt2k_menu::SettingControl *ctl, char *buf, int size);        /* 0x0015F0E8 */
void text_parts_sus_b_kit(gt2k_menu::SettingControl *ctl, char *buf, int size);        /* 0x0015F168 */
void text_parts_sus_b_balance(gt2k_menu::SettingControl *ctl, char *buf, int size);    /* 0x0015F1E8 */
void text_parts_drv_mission(gt2k_menu::SettingControl *ctl, char *buf, int size);      /* 0x0015F268 */
void text_parts_drv_clutch(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x0015F2E8 */
void text_parts_drv_fly(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015F368 */
void text_parts_drv_lsd(gt2k_menu::SettingControl *ctl, char *buf, int size);          /* 0x0015F3E8 */
void text_parts_drv_shaft(gt2k_menu::SettingControl *ctl, char *buf, int size);        /* 0x0015F468 */
void text_parts_oth_asm_s(gt2k_menu::SettingControl *ctl, char *buf, int size);        /* 0x0015F4E8 */
void text_parts_oth_tcs_s(gt2k_menu::SettingControl *ctl, char *buf, int size);        /* 0x0015F568 */
void text_parts_oth_vcd_s(gt2k_menu::SettingControl *ctl, char *buf, int size);        /* 0x0015F5E8 */
void text_parts_oth_weight_r(gt2k_menu::SettingControl *ctl, char *buf, int size);     /* 0x0015F668 */
/* The tyres; in a race on dirt tyres only (RaceEvent::needTire 9 of
 * MenuStatus::event) the full name of the dirt tyres, part 0x42, if the
 * working copy owns it, else "----" (the rear text tests and names 0x42
 * too). */
void text_parts_tir_f(gt2k_menu::SettingControl *ctl, char *buf, int size);            /* 0x0015F6E8 */
void text_parts_tir_r(gt2k_menu::SettingControl *ctl, char *buf, int size);            /* 0x0015F848 */
void text_parts_oth_ayc_s(gt2k_menu::SettingControl *ctl, char *buf, int size);        /* 0x0015F9A8 getPartName(0x54)
                                               "fitted" with the AYC (LSD part 0x23), else
                                               getPartName(0x53) "none" */

/* ---- the attach_* predicates (p8 of create_bool_item) ----------------------
 * 1 if the working copy's part of the category is the part id of the image;
 * 0 without a setting car (setting_attach_ayc_off: 1). */
int attach_cpu_off(gt2k_menu::SettingControl *ctl);         /* 0x0015FA40 computer 0x08 */
int attach_cpu_on(gt2k_menu::SettingControl *ctl);          /* 0x0015FA90 0x09 */
int attach_muf_normal(gt2k_menu::SettingControl *ctl);      /* 0x0015FAE0 muffler 0x24 */
int attach_muf_sports(gt2k_menu::SettingControl *ctl);      /* 0x0015FB30 0x25 */
int attach_muf_semi_racing(gt2k_menu::SettingControl *ctl); /* 0x0015FB80 0x26 */
int attach_muf_racing(gt2k_menu::SettingControl *ctl);      /* 0x0015FBD0 0x27 */
int attach_tur_off(gt2k_menu::SettingControl *ctl);         /* 0x0015FC20 turbine kit 0x4E */
/* u034 */
int setting_attach_tur_stage1(gt2k_menu::SettingControl *ctl);      /* 0x0015FC70 0x4F */
int setting_attach_tur_stage2(gt2k_menu::SettingControl *ctl);      /* 0x0015FCC0 0x50 */
int setting_attach_tur_stage3(gt2k_menu::SettingControl *ctl);      /* 0x0015FD10 0x51 */
int setting_attach_tur_stage4(gt2k_menu::SettingControl *ctl);      /* 0x0015FD60 0x52 */
int setting_attach_coo_normal(gt2k_menu::SettingControl *ctl);      /* 0x0015FDB0 intercooler 0x17 */
int setting_attach_coo_1(gt2k_menu::SettingControl *ctl);           /* 0x0015FE00 0x18 */
int setting_attach_coo_2(gt2k_menu::SettingControl *ctl);           /* 0x0015FE50 0x19 */
int setting_attach_nat_off(gt2k_menu::SettingControl *ctl);         /* 0x0015FEA0 NA tune 0x28 */
int setting_attach_nat_stage1(gt2k_menu::SettingControl *ctl);      /* 0x0015FEF0 0x29 */
int setting_attach_nat_stage2(gt2k_menu::SettingControl *ctl);      /* 0x0015FF40 0x2A */
int setting_attach_nat_stage3(gt2k_menu::SettingControl *ctl);      /* 0x0015FF90 0x2B */
int setting_attach_sus_normal(gt2k_menu::SettingControl *ctl);      /* 0x0015FFE0 suspension 0x32 */
int setting_attach_sus_sports(gt2k_menu::SettingControl *ctl);      /* 0x00160030 0x33 */
int setting_attach_sus_semi_racing(gt2k_menu::SettingControl *ctl); /* 0x00160080 0x34 */
int setting_attach_sus_racing(gt2k_menu::SettingControl *ctl);      /* 0x001600D0 0x35 */
int setting_attach_brk_normal(gt2k_menu::SettingControl *ctl);      /* 0x00160120 brakes 0x00 */
int setting_attach_brk_racing(gt2k_menu::SettingControl *ctl);      /* 0x00160168 0x01 */
int setting_attach_brc_off(gt2k_menu::SettingControl *ctl);         /* 0x001601B8 brake controller 0x02 */
int setting_attach_brc_on(gt2k_menu::SettingControl *ctl);          /* 0x00160208 0x03 */
int setting_attach_msn_normal(gt2k_menu::SettingControl *ctl);      /* 0x00160258 transmission 0x13 */
int setting_attach_msn_close(gt2k_menu::SettingControl *ctl);       /* 0x001602A8 0x14 */
int setting_attach_msn_super_close(gt2k_menu::SettingControl *ctl); /* 0x001602F8 0x15 */
int setting_attach_msn_variable(gt2k_menu::SettingControl *ctl);    /* 0x00160348 0x16 */
int setting_attach_clt_normal(gt2k_menu::SettingControl *ctl);      /* 0x00160398 clutch 0x04 */
int setting_attach_clt_big(gt2k_menu::SettingControl *ctl);         /* 0x001603E8 0x05 */
int setting_attach_clt_twin(gt2k_menu::SettingControl *ctl);        /* 0x00160438 0x06 */
int setting_attach_clt_triple(gt2k_menu::SettingControl *ctl);      /* 0x00160488 0x07 */
int setting_attach_fly_normal(gt2k_menu::SettingControl *ctl);      /* 0x001604D8 flywheel 0x0F */
int setting_attach_fly_light(gt2k_menu::SettingControl *ctl);       /* 0x00160528 0x10 */
int setting_attach_fly_crmo(gt2k_menu::SettingControl *ctl);        /* 0x00160578 0x11 */
int setting_attach_fly_light_crmo(gt2k_menu::SettingControl *ctl);  /* 0x001605C8 0x12 */
int setting_attach_lsd_normal(gt2k_menu::SettingControl *ctl);      /* 0x00160618 LSD 0x1E */
int setting_attach_lsd_2way(gt2k_menu::SettingControl *ctl);        /* 0x00160668 0x20 */
int setting_attach_lsd_15way(gt2k_menu::SettingControl *ctl);       /* 0x001606B8 0x21 */
int setting_attach_lsd_1way(gt2k_menu::SettingControl *ctl);        /* 0x00160708 0x1F */
int setting_attach_lsd_variable(gt2k_menu::SettingControl *ctl);    /* 0x00160758 0x22 */
int setting_attach_lsd_aycc(gt2k_menu::SettingControl *ctl);        /* 0x001607A8 0x23 */
int setting_attach_shf_normal(gt2k_menu::SettingControl *ctl);      /* 0x001607F8 propeller shaft 0x2E */
int setting_attach_shf_propeller(gt2k_menu::SettingControl *ctl);   /* 0x00160848 0x2F */
int setting_attach_tir_f_normal(gt2k_menu::SettingControl *ctl);    /* 0x00160898 front tyres 0x38 */
int setting_attach_tir_f_sports(gt2k_menu::SettingControl *ctl);    /* 0x001608E8 0x39 */
int setting_attach_tir_f_racing_sh(gt2k_menu::SettingControl *ctl); /* 0x00160938 0x3A */
int setting_attach_tir_f_racing_h(gt2k_menu::SettingControl *ctl);  /* 0x00160988 0x3B */
int setting_attach_tir_f_racing_mh(gt2k_menu::SettingControl *ctl); /* 0x001609D8 0x3C */
int setting_attach_tir_f_racing_m(gt2k_menu::SettingControl *ctl);  /* 0x00160A28 0x3D */
int setting_attach_tir_f_racing_ms(gt2k_menu::SettingControl *ctl); /* 0x00160A78 0x3E */
int setting_attach_tir_f_racing_s(gt2k_menu::SettingControl *ctl);  /* 0x00160AC8 0x3F */
int setting_attach_tir_f_racing_ss(gt2k_menu::SettingControl *ctl); /* 0x00160B18 0x40 */
int setting_attach_tir_f_control(gt2k_menu::SettingControl *ctl);   /* 0x00160B68 0x41 */
int setting_attach_tir_r_normal(gt2k_menu::SettingControl *ctl);    /* 0x00160BB8 rear tyres 0x43 */
int setting_attach_tir_r_sports(gt2k_menu::SettingControl *ctl);    /* 0x00160C08 0x44 */
int setting_attach_tir_r_racing_sh(gt2k_menu::SettingControl *ctl); /* 0x00160C58 0x45 */
int setting_attach_tir_r_racing_h(gt2k_menu::SettingControl *ctl);  /* 0x00160CA8 0x46 */
int setting_attach_tir_r_racing_mh(gt2k_menu::SettingControl *ctl); /* 0x00160CF8 0x47 */
int setting_attach_tir_r_racing_m(gt2k_menu::SettingControl *ctl);  /* 0x00160D48 0x48 */
int setting_attach_tir_r_racing_ms(gt2k_menu::SettingControl *ctl); /* 0x00160D98 0x49 */
int setting_attach_tir_r_racing_s(gt2k_menu::SettingControl *ctl);  /* 0x00160DE8 0x4A */
int setting_attach_tir_r_racing_ss(gt2k_menu::SettingControl *ctl); /* 0x00160E38 0x4B */
int setting_attach_tir_r_control(gt2k_menu::SettingControl *ctl);   /* 0x00160E88 0x4C */
int setting_attach_ayc_off(gt2k_menu::SettingControl *ctl);         /* 0x00160ED8 LSD not 0x23 (1 without a
                                                                       setting car) */
int setting_attach_ayc_on(gt2k_menu::SettingControl *ctl);          /* 0x00160F28 LSD 0x23 */
int setting_attach_sms_off(gt2k_menu::SettingControl *ctl);         /* 0x00160F78 ASM 0x53 */
int setting_attach_sms_on(gt2k_menu::SettingControl *ctl);          /* 0x00160FC8 0x54 */
int setting_attach_tcs_off(gt2k_menu::SettingControl *ctl);         /* 0x00161018 TCS 0x36 */
int setting_attach_tcs_on(gt2k_menu::SettingControl *ctl);          /* 0x00161068 0x37 */
int setting_attach_vcd_off(gt2k_menu::SettingControl *ctl);         /* 0x001610B8 VCD 0x55 */
int setting_attach_vcd_on(gt2k_menu::SettingControl *ctl);          /* 0x00161108 0x56 */

/* "gear_setup": no object (NULL); finds the gear sliders of the current
 * button list by name ("slider_gear_ratio_1".."_7", "_final": dynamic_cast to
 * Slider; "slider_gear_auto_level") and keeps them in SettingControl::
 * gearSliders / gearAutoLevel; ratio sliders 1..7 get showPrev 0. */
gt2k_menu::Transitive *setting_create_gear_setup(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00161158 */

/* ---- the disable_* predicates: !(the T084 check of the setting) -----------
 * The listing inverts with xori 1, egcs's `!' of a bool (of an int it is
 * sltiu): the T084 checks return bool in the original (T084 declares int). */
int setting_disable_spring_rate(gt2k_menu::SettingControl *ctl);    /* 0x00161530 !can_adjust_spring_rate */
int setting_disable_ride_height(gt2k_menu::SettingControl *ctl);    /* 0x00161550 !can_adjust_ride_height */
int setting_disable_damper(gt2k_menu::SettingControl *ctl);         /* 0x00161570 !can_adjust_damper */
int setting_disable_bound(gt2k_menu::SettingControl *ctl);          /* 0x00161590 !can_adjust_bound_rebound ("disable_bound"
                                                                       and "disable_rebound") */
int setting_disable_camber(gt2k_menu::SettingControl *ctl);         /* 0x001615B0 !can_adjust_camber */
int setting_disable_toe(gt2k_menu::SettingControl *ctl);            /* 0x001615D0 !can_adjust_toe */
int setting_disable_stabilizer(gt2k_menu::SettingControl *ctl);     /* 0x001615F0 !can_adjust_stabiliser */
int setting_disable_brake_balance(gt2k_menu::SettingControl *ctl);  /* 0x00161610 !can_adjust_brake_balance */
int setting_disable_lsd_initial(gt2k_menu::SettingControl *ctl);    /* 0x00161630 !can_adjust_lsd_initial */
int setting_disable_lsd_accel(gt2k_menu::SettingControl *ctl);      /* 0x00161650 !can_adjust_lsd_accel */
int setting_disable_lsd_decrease(gt2k_menu::SettingControl *ctl);   /* 0x00161670 !can_adjust_lsd_decel */
int setting_disable_gear_ratio(gt2k_menu::SettingControl *ctl);     /* 0x00161690 !can_adjust_final_gear */
int setting_disable_aero(gt2k_menu::SettingControl *ctl);           /* 0x001616B0 !can_adjust_aero */
int setting_disable_ayc(gt2k_menu::SettingControl *ctl);            /* 0x001616D0 !can_adjust_ayc */
int setting_disable_asm(gt2k_menu::SettingControl *ctl);            /* 0x001616F0 !can_adjust_asm */
int setting_disable_tcs(gt2k_menu::SettingControl *ctl);            /* 0x00161710 !can_adjust_tcs */
int setting_disable_vcd(gt2k_menu::SettingControl *ctl);            /* 0x00161730 !can_adjust_vcd */

/* "turbine_zero_name": an Item of the hidden picture "name_normal" if the
 * setting car's stock turbine kit is a turbo (isTurboByDefault), else
 * "name_none" (also without a setting car). */
gt2k_menu::Transitive *setting_create_turbine_zero_name(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00161750 */
/* "user_carname": name 1 of the setting car, "" without one. */
void setting_text_user_carname(gt2k_menu::SettingControl *ctl, char *buf, int size);       /* 0x00161840 */
/* "car_current_power": "<power> <unit>" of the working copy (GarageCar::
 * getPower; the unit of g_002F59D8, T518); "" without a setting car. */
void setting_text_car_current_power(gt2k_menu::SettingControl *ctl, char *buf, int size);  /* 0x00161898 */

/* T081 .data 0x002A9080..0x002A9F98: {"aero_f", create_callback_text,
 * text_aero_f, 0, 0, 0} .. Filled by T081_static_init (.ctors entry
 * 0x0028F0A0; compiler-generated in the original: the callbacks cast to int
 * are not constant initialisers). */
extern gt2k_menu::MenuHook g_002A9080[162]; /* 0x002A9080 (161 in GT3; the enhanced engine adds
                                               "gear_max_speed") */
/* Its entry count (T081 .sdata), given with it to the page manager by
 * SettingControl::begin (T080): const (begin loads it once for its two
 * calls). */
extern const int g_00351950;                /* 0x00351950 gp-0x60A0 (=0xA1) */
void T081_static_init(void);                /* 0x00161930 */

#endif /* GAME_T081_SETTING_HOOKS_H */
