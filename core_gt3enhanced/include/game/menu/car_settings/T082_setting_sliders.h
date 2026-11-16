/* T082_setting_sliders.h - the slider functions of the car setting screen
 * (translation unit T082, unit u035), 0x001638F8..0x00166F98.
 *
 * T082_static_init fills g_002ABAB0 with 40 MenuSliderFunc entries {name,
 * get} sorted by name (count g_00351970); SettingControl::begin (T080, 0x0015CA98) gives the
 * table to both layouts of its PageManager (PageLayout::setSliderFuncs on
 * common and own, ctx the SettingControl).  ButtonList::createSlider (T244) builds a Slider
 * (T238) for an element "slider_<name>" with the SliderDesc that get(ctx)
 * returns; the Slider calls desc.set(ctx, value, commit) when the player
 * moves it.
 *
 * Every get reads the range (and mostly the value) from the record of the
 * part fitted to GameStatus's setting car (MenuStatus::raceCar[0], the
 * first pass's settingCar); without a setting car it returns {0, 1, 0, 1,
 * set, 0, 0, 0}.  min..max is 1..levels or min..max + 1 of the record (max
 * is exclusive), step 1, mode 0.  The setters write the SettingControl's
 * working copy of the car (+0xE40) and return car_meets_event_regulations
 * of it (T084: the regulations of the event still hold); in mode 0 a 0
 * refuses the value, so the knob stops where the car would leave the
 * regulations.  The gear setters return 1.
 *
 * Signatures: the setters take T238's SliderSetFn signature (the first pass
 * had (SettingControl *ctl, int value); commit is not read).  The getters
 * return the SliderDesc by value, as the owner defines them: the code
 * returns the hidden result pointer in $v0 (0x00163A04), which a
 * void (SliderDesc *out, void *ctx) function would not do; ctx is the
 * SettingControl.
 *
 * AeroRecord and VcdRecord, planned here, are T482's records of tables 0x05
 * and 0x0D: the aliases below keep the first pass's names. */
#ifndef GAME_T082_SETTING_SLIDERS_H
#define GAME_T082_SETTING_SLIDERS_H

#include "game/game.h"
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuSliderFunc (g_002ABAB0) */

class GarageCar;                            /* T036 (pointers only) */
struct RacingModifyRecord;                  /* T482: table 0x05 */
struct DrivetrainRecord;                    /* T482: table 0x0D */
namespace gt2k_menu {
class SettingControl;                       /* T080 (pointers only) */
struct SliderDesc;                          /* T238 (returned by value: the callers include T238) */
}

/* u035 / u036's AeroRecord is table 0x05's record (getRacingmodifyRecord,
 * 0x002349D8): frontMin / frontMax / rearMin / rearMax are aeroFMin +0x11,
 * aeroFMax +0x12, aeroRMin +0x3A, aeroRMax +0x3B. */
typedef RacingModifyRecord AeroRecord;
/* u035 / u036's VcdRecord is table 0x0D's record (getDrivetrainRecord,
 * 0x002349A8): min / max (u036: levelMin / levelMax) are vcdMin +0x11,
 * vcdMax +0x12. */
typedef DrivetrainRecord VcdRecord;

/* The first pass's name of the table record (u035; its get is func). */
typedef gt2k_menu::MenuSliderFunc SettingSliderEntry;  /* {name, func}, 8 bytes */

/* ---- the sliders, by element name ("slider_<name>") ----------------------- */
/* "damper_f": bound and rebound together (the four front words +0x16C ..
 * +0x178); 1..the suspension's front bound levels. */
int setting_slider_set_damper_f(void *ctx, int value, int commit);          /* 0x001638F8 */
gt2k_menu::SliderDesc setting_slider_damper_f(void *ctx);                   /* 0x00163970 */
int setting_slider_set_damper_r(void *ctx, int value, int commit);          /* 0x00163A20 */
gt2k_menu::SliderDesc setting_slider_damper_r(void *ctx);                   /* 0x00163A98 */
int setting_slider_set_bound_f(void *ctx, int value, int commit);           /* 0x00163B48 */
gt2k_menu::SliderDesc setting_slider_bound_f(void *ctx);                    /* 0x00163BB8 */
int setting_slider_set_bound_r(void *ctx, int value, int commit);           /* 0x00163C68 */
gt2k_menu::SliderDesc setting_slider_bound_r(void *ctx);                    /* 0x00163CD8 */
int setting_slider_set_rebound_f(void *ctx, int value, int commit);         /* 0x00163D88 */
gt2k_menu::SliderDesc setting_slider_rebound_f(void *ctx);                  /* 0x00163DF8 */
int setting_slider_set_rebound_r(void *ctx, int value, int commit);         /* 0x00163EA8 */
gt2k_menu::SliderDesc setting_slider_rebound_r(void *ctx);                  /* 0x00163F18 */
/* brake balance (the brake controller's countF / countR levels) */
int setting_slider_set_brake_f(void *ctx, int value, int commit);           /* 0x00163FC8 */
gt2k_menu::SliderDesc setting_slider_brake_f(void *ctx);                    /* 0x00164030 */
int setting_slider_set_brake_r(void *ctx, int value, int commit);           /* 0x001640E0 */
gt2k_menu::SliderDesc setting_slider_brake_r(void *ctx);                    /* 0x00164148 */
int setting_slider_set_camber_f(void *ctx, int value, int commit);          /* 0x001641F8 */
gt2k_menu::SliderDesc setting_slider_camber_f(void *ctx);                   /* 0x00164260 */
int setting_slider_set_camber_r(void *ctx, int value, int commit);          /* 0x00164320 */
gt2k_menu::SliderDesc setting_slider_camber_r(void *ctx);                   /* 0x00164388 */
int setting_slider_set_height_f(void *ctx, int value, int commit);          /* 0x00164448 ride height */
gt2k_menu::SliderDesc setting_slider_height_f(void *ctx);                   /* 0x001644B0 */
int setting_slider_set_height_r(void *ctx, int value, int commit);          /* 0x00164570 */
gt2k_menu::SliderDesc setting_slider_height_r(void *ctx);                   /* 0x001645D8 */
int setting_slider_set_rate_f(void *ctx, int value, int commit);            /* 0x00164698 spring rate */
gt2k_menu::SliderDesc setting_slider_rate_f(void *ctx);                     /* 0x00164700 */
int setting_slider_set_rate_r(void *ctx, int value, int commit);            /* 0x001647C0 */
gt2k_menu::SliderDesc setting_slider_rate_r(void *ctx);                     /* 0x00164828 */
int setting_slider_set_stabilizer_f(void *ctx, int value, int commit);      /* 0x001648E8 */
gt2k_menu::SliderDesc setting_slider_stabilizer_f(void *ctx);               /* 0x00164950 1..levels */
int setting_slider_set_stabilizer_r(void *ctx, int value, int commit);      /* 0x00164A00 */
gt2k_menu::SliderDesc setting_slider_stabilizer_r(void *ctx);               /* 0x00164A68 */
int setting_slider_set_toe_f(void *ctx, int value, int commit);             /* 0x00164B18 stored biased by 0x80 */
gt2k_menu::SliderDesc setting_slider_toe_f(void *ctx);                      /* 0x00164B80 */
int setting_slider_set_toe_r(void *ctx, int value, int commit);             /* 0x00164C40 */
gt2k_menu::SliderDesc setting_slider_toe_r(void *ctx);                      /* 0x00164CA8 */
/* LSD: initial torque, acceleration, deceleration sensitivity */
int setting_slider_set_lsd_init_f(void *ctx, int value, int commit);        /* 0x00164D68 */
gt2k_menu::SliderDesc setting_slider_lsd_init_f(void *ctx);                 /* 0x00164DD0 */
int setting_slider_set_lsd_accl_f(void *ctx, int value, int commit);        /* 0x00164E90 */
gt2k_menu::SliderDesc setting_slider_lsd_accl_f(void *ctx);                 /* 0x00164EF8 */
int setting_slider_set_lsd_accl_r(void *ctx, int value, int commit);        /* 0x00164FB8 */
gt2k_menu::SliderDesc setting_slider_lsd_accl_r(void *ctx);                 /* 0x00165020 */
int setting_slider_set_lsd_decl_f(void *ctx, int value, int commit);        /* 0x001650E0 */
gt2k_menu::SliderDesc setting_slider_lsd_decl_f(void *ctx);                 /* 0x00165148 */
int setting_slider_set_lsd_decl_r(void *ctx, int value, int commit);        /* 0x00165208 */
gt2k_menu::SliderDesc setting_slider_lsd_decl_r(void *ctx);                 /* 0x00165270 */
/* the same field as "elec_aycc" (the rear initial torque, +0x198) */
int setting_slider_set_lsd_init_r(void *ctx, int value, int commit);        /* 0x00165330 */
gt2k_menu::SliderDesc setting_slider_lsd_init_r(void *ctx);                 /* 0x00165398 no range with the AYC
                                                                               (LSD part 0x23) */

/* The gear ratios of auto setting level `level` (CarSpec of the car, T294
 * 0x001E28A8) into ratios[0..7]. */
void setting_gear_auto_ratios(GarageCar *car, int level, s16 *ratios);      /* 0x00165470 */
/* The range of gear `gear` at the car's auto level and final gear (T294
 * 0x001E2C58): the whole scale lo[top gear]..hi[1] (*min, *max), the gear's
 * own lo[gear]..hi[gear] (*limitMin, *limitMax). */
void setting_gear_range(GarageCar *car, int gear, int *min, int *max,
                        int *limitMin, int *limitMax);                      /* 0x001654C8 */
/* "gear_ratio_<n>": CarSaveEquipments::gearRatio[n] of the working copy
 * (0 = reverse), offered when the setting car's transmission has n gears or
 * more (always for 0); range setting_gear_range, inner range the gear's own. */
int setting_slider_set_gear_ratio_0(void *ctx, int value, int commit);      /* 0x001655B0 */
gt2k_menu::SliderDesc setting_slider_gear_ratio_0(void *ctx);               /* 0x00165618 */
int setting_slider_set_gear_ratio_1(void *ctx, int value, int commit);      /* 0x00165710 */
gt2k_menu::SliderDesc setting_slider_gear_ratio_1(void *ctx);               /* 0x00165770 */
int setting_slider_set_gear_ratio_2(void *ctx, int value, int commit);      /* 0x00165860 */
gt2k_menu::SliderDesc setting_slider_gear_ratio_2(void *ctx);               /* 0x001658C8 */
int setting_slider_set_gear_ratio_3(void *ctx, int value, int commit);      /* 0x001659C0 */
gt2k_menu::SliderDesc setting_slider_gear_ratio_3(void *ctx);               /* 0x00165A28 */
int setting_slider_set_gear_ratio_4(void *ctx, int value, int commit);      /* 0x00165B20 */
gt2k_menu::SliderDesc setting_slider_gear_ratio_4(void *ctx);               /* 0x00165B88 */
int setting_slider_set_gear_ratio_5(void *ctx, int value, int commit);      /* 0x00165C80 */
gt2k_menu::SliderDesc setting_slider_gear_ratio_5(void *ctx);               /* 0x00165CE8 */
int setting_slider_set_gear_ratio_6(void *ctx, int value, int commit);      /* 0x00165DE0 */
gt2k_menu::SliderDesc setting_slider_gear_ratio_6(void *ctx);               /* 0x00165E48 */
int setting_slider_set_gear_ratio_7(void *ctx, int value, int commit);      /* 0x00165F40 */
gt2k_menu::SliderDesc setting_slider_gear_ratio_7(void *ctx);               /* 0x00165FA8 */
int setting_slider_set_gear_ratio_final(void *ctx, int value, int commit);  /* 0x001660A0 */
gt2k_menu::SliderDesc setting_slider_gear_ratio_final(void *ctx);           /* 0x001660F0 finalGearRatioMin ..
                                                                               Max of the working copy (no + 1) */
/* A new auto setting level: every gear slider of the page (gear i at
 * SettingControl::gearSliders[i - 1], i = 1..gears) gets the new ranges
 * (Slider::setRange) and the ratio of the level (Slider::setValue). */
void setting_gear_apply_auto_level(gt2k_menu::SettingControl *ctl, int level);   /* 0x001661B0 */
/* "gear_auto_level": stores the level (CarSaveEquipments::maxSpeed),
 * applies it, then keeps the final gear it was set with
 * (lastFinalGearRatio, +0x1AC); 0 without a setting car. */
int setting_slider_set_gear_auto_level(void *ctx, int value, int commit);   /* 0x001662F0 */
gt2k_menu::SliderDesc setting_slider_gear_auto_level(void *ctx);            /* 0x00166368 */
int setting_slider_set_aero_f(void *ctx, int value, int commit);            /* 0x00166430 */
gt2k_menu::SliderDesc setting_slider_aero_f(void *ctx);                     /* 0x00166498 aeroFMin .. aeroFMax + 1 */
int setting_slider_set_aero_r(void *ctx, int value, int commit);            /* 0x00166558 */
gt2k_menu::SliderDesc setting_slider_aero_r(void *ctx);                     /* 0x001665C0 */
/* the AYC level is the rear LSD initial torque */
int setting_slider_set_elec_aycc(void *ctx, int value, int commit);         /* 0x00166680 */
gt2k_menu::SliderDesc setting_slider_elec_aycc(void *ctx);                  /* 0x001666E8 */
int setting_slider_set_elec_ascc(void *ctx, int value, int commit);         /* 0x001667A8 */
gt2k_menu::SliderDesc setting_slider_elec_ascc(void *ctx);                  /* 0x00166810 1..levels */
/* reversed: the stored level is TcsRecord::count - (value - 1) */
int setting_slider_set_elec_tcsc(void *ctx, int value, int commit);         /* 0x001668C0 */
gt2k_menu::SliderDesc setting_slider_elec_tcsc(void *ctx);                  /* 0x00166940 */
int setting_slider_set_elec_vcdc(void *ctx, int value, int commit);         /* 0x00166A08 */
gt2k_menu::SliderDesc setting_slider_elec_vcdc(void *ctx);                  /* 0x00166A70 vcdMin .. vcdMax + 1 */

/* T082 .data 0x002ABAB0..0x002ABBF0: {"slider_aero_f", setting_slider_aero_f}
 * .. Filled by T082_static_init (.ctors entry 0x0028F0A4; compiler-generated
 * in the original). */
extern gt2k_menu::MenuSliderFunc g_002ABAB0[40];   /* 0x002ABAB0 */
/* Its entry count (T082 .sdata; SettingControl::begin, T080). */
extern const int g_00351970;                /* 0x00351970 gp-0x6080 (=0x28) */
void T082_static_init(void);                /* 0x00166B30 */

#endif /* GAME_T082_SETTING_SLIDERS_H */
