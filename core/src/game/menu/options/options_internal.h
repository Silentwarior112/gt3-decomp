/* options_internal.h - unit u022 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0013FF98..0x00142790, second pass.
 *
 * Five small translation units of the options screen, run by
 * gt2k_menu::OptionControl (T062, unit u021).  Each one is a set of
 * callbacks plus a static initialiser that fills one table sorted by name;
 * OptionControl::begin (0x0013FAD0) hands the tables over, ctx = the
 * OptionControl:
 *   T063  g_002A0A70, 2 ValidFuncData (PageManager::setValidFuncData): the
 *         USB wheel calibration pages refuse to open without a wheel.
 *   T064  g_002A0AD0, 105 MenuHooks (PageManager::setHooks, both layouts):
 *         the on / off indicators of the options (create_bool_item with a
 *         predicate), the slider bars (create_slider_item with a range), the
 *         texts (create_callback_text), "blink", the song lists and the
 *         factories of the key configuration, handicap, wheel calibration
 *         and steering assist items.
 *   T065  g_002A1C38, 3 MenuSliderFuncs (PageManager::setSliderFuncs).
 *   T066  g_002A1CD8, 21 MenuMultiFuncs (PageManager::setMultiFuncs).
 *   T067  g_002A2038, 8 ExecuteFuncData (PageManager::setExecuteFuncData).
 * None of the functions has a direct caller.  The tables are .data, filled
 * at run time by each TU's compiler-generated static initialiser (.ctors
 * entries 0x0028F080..0x0028F090; each file defines its table without an
 * initialiser, so its zeros are in .data before the frame info, as
 * retail's); their entry counts are .sdata words
 * (g_003516F0 = 2, g_00351708 = 105, g_00351718 = 3, g_0035171C = 21,
 * g_00351720 = 8).
 *
 * Every shared class, function and global is in its canonical header
 * (include/game/).  Each file includes the ones it needs itself, in the
 * order that gives retail's type names (each file's comment lists them),
 * and this header last: it adds what only this unit uses (constants, the
 * callbacks the tables hold that no other unit names) and includes nothing
 * that defines a class with a type name.
 */
#ifndef U022_H
#define U022_H

#include "types.h"
#include <stddef.h>                         /* NULL */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuHookFn, MultiButtonInfo */

namespace gt2k_menu {
class OptionControl;                        /* T062 (every callback's ctx) */
struct ButtonData;                          /* T244 (pointers only) */
struct SliderDesc;                          /* T238 (T065's slider functions return it) */
}

/* ======================================================================== */
/* Constants (only used here)                                               */
/* ======================================================================== */

/* menu_play_se (T233): the sound of a refused choice is MENU_SE_REFUSED. */

/* The pad manager slots of the two USB wheels (PadManager, T475: 10-11 are
 * usb[]; u014's names). */
enum { PAD_SLOT_USB1 = 10, PAD_SLOT_USB2 = 11 };

/* OptionControl::calibPort: the port the USB wheel pages are for (T067
 * set_port_1 / set_port_2). */
enum { CALIB_PORT_1 = 0, CALIB_PORT_2 = 1 };

/* KeyConfigItem::mode (T101) is a ControllerConfig mode (T426): CONFIG_ANALOG
 * the pad pages; OptionControl::calibWheel holds the mode of the USB wheel
 * pages: CONFIG_WHEEL_GTFORCE ("set_port_N"), CONFIG_WHEEL_C294
 * ("set_port_N_jaguar"). */

/* CalibrationItem::axis, the hook's p8, by the hook names
 * ("calib_<axis>_bar"); u014 has CALIB_STEERING / _CENTRE / _PEDAL1 / 2. */
enum {
    CALIB_STEER  = 0,                       /* "calib_steer_bar": the steering range */
    CALIB_CENTER = 1,                       /* "calib_center_bar": the steering centre */
    CALIB_ACCEL  = 2,                       /* "calib_accel_bar": channel 1 */
    CALIB_BRAKE  = 3                        /* "calib_brake_bar": channel 2 */
};

/* GameOptions::splitBattle.boost ("split_boost_none" / "_low" / "_high"). */
enum { SPLIT_BOOST_NONE = 0, SPLIT_BOOST_LOW = 1, SPLIT_BOOST_HIGH = 2 };

/* GameOptions::steeringAssist1p / 2p: "assist_type1".."assist_type5" are
 * stored as 0..4. */
enum {
    STEERING_ASSIST_TYPE1 = 0,
    STEERING_ASSIST_TYPE2 = 1,
    STEERING_ASSIST_TYPE3 = 2,
    STEERING_ASSIST_TYPE4 = 3,
    STEERING_ASSIST_TYPE5 = 4
};

/* The choices of an on / off multi button (T066): "on" is listed first.
 * The wide mode lists "off" first: its choice is the flag itself. */
enum { MULTI_CHOICE_ON = 0, MULTI_CHOICE_OFF = 1 };

/* gt2k_menu::SliderDesc::mode: all three option sliders use T238's
 * SLIDER_MODE_STEP (left / right by step only). */

/* ======================================================================== */
/* Callbacks of this unit that no other unit names                          */
/* ======================================================================== */
/* The callbacks the tables hold in an int word (MenuHook::p8) are written
 * with the OptionControl their caller passes as ctx (create_bool_item's
 * test, create_slider_item's getter: void *ctx; create_callback_text's
 * text: ControlBase *owner, OptionControl's base at +0). */

/* p8 of create_bool_item: the item is drawn while it returns non-zero (pC 0:
 * tested at every draw). */
typedef int OptionTestFn(gt2k_menu::OptionControl *ctl);
/* p8 of create_slider_item: the range [min, end) and the value (SliderGetter). */
typedef void OptionRangeFn(gt2k_menu::OptionControl *ctl, int *min, int *end, int *value);
/* p8 of create_callback_text: writes the text into buf (CallbackTextFn;
 * size 0x80, not checked). */
typedef void OptionTextFn(gt2k_menu::OptionControl *ctl, char *buf, int size);

/* T063 */
const char *option_check_calib_steer(void *ctx, gt2k_menu::ButtonData *rec); /* 0x0013FF98 */

/* T064: create_bool_item tests ("<option>_on_on" / "<option>_off_off" while
 * the option is on, "<option>_off_on" / "<option>_on_off" while it is off) */
OptionTestFn option_wide_mode_on;                   /* 0x00140060 */
OptionTestFn option_wide_mode_off;                  /* 0x00140070 */
OptionTestFn option_menu_bgm_on;                    /* 0x00140080 */
OptionTestFn option_menu_bgm_off;                   /* 0x00140090 */
OptionTestFn option_menu_se_on;                     /* 0x001400A0 */
OptionTestFn option_menu_se_off;                    /* 0x001400B0 */
OptionTestFn option_race_bgm_on;                    /* 0x001400C0 */
OptionTestFn option_race_bgm_off;                   /* 0x001400D0 */
OptionTestFn option_race_se_on;                     /* 0x001400E0 */
OptionTestFn option_race_se_off;                    /* 0x001400F0 */
OptionTestFn option_replay_bgm_on;                  /* 0x00140100 */
OptionTestFn option_replay_bgm_off;                 /* 0x00140110 */
OptionTestFn option_replay_se_on;                   /* 0x00140120 */
OptionTestFn option_replay_se_off;                  /* 0x00140130 */
OptionTestFn option_vib1_on;                        /* 0x00140140 */
OptionTestFn option_vib1_off;                       /* 0x00140150 */
OptionTestFn option_vib2_on;                        /* 0x00140160 */
OptionTestFn option_vib2_off;                       /* 0x00140170 */
OptionTestFn option_tire_damage_on;                 /* 0x00140180 */
OptionTestFn option_tire_damage_off;                /* 0x00140190 */
OptionTestFn option_ghost_on;                       /* 0x001401A0 */
OptionTestFn option_ghost_off;                      /* 0x001401B0 */
OptionTestFn option_assist_asm_on;                  /* 0x001401C0 */
OptionTestFn option_assist_asm_off;                 /* 0x001401D0 */
OptionTestFn option_assist_tcs_on;                  /* 0x001401E0 */
OptionTestFn option_assist_tcs_off;                 /* 0x001401F0 */
OptionTestFn option_split_tire_damage_on;           /* 0x00140200 */
OptionTestFn option_split_tire_damage_off;          /* 0x00140210 */
OptionTestFn option_split_boost_none;               /* 0x00140220 */
OptionTestFn option_split_boost_not_none;           /* 0x00140230 */
OptionTestFn option_split_boost_low;                /* 0x00140240 */
OptionTestFn option_split_boost_not_low;            /* 0x00140250 */
OptionTestFn option_split_boost_high;               /* 0x00140260 */
OptionTestFn option_split_boost_not_high;           /* 0x00140270 */
OptionTestFn option_assist_1p_type1;                /* 0x00140280 */
OptionTestFn option_assist_1p_not_type1;            /* 0x00140290 */
OptionTestFn option_assist_1p_type2;                /* 0x001402A0 */
OptionTestFn option_assist_1p_not_type2;            /* 0x001402B0 */
OptionTestFn option_assist_1p_type3;                /* 0x001402C0 */
OptionTestFn option_assist_1p_not_type3;            /* 0x001402D0 */
OptionTestFn option_assist_1p_type4;                /* 0x001402E0 */
OptionTestFn option_assist_1p_not_type4;            /* 0x001402F0 */
OptionTestFn option_assist_1p_type5;                /* 0x00140300 */
OptionTestFn option_assist_1p_not_type5;            /* 0x00140310 */
OptionTestFn option_force_feedback_1p_on;           /* 0x00140320 */
OptionTestFn option_force_feedback_1p_off;          /* 0x00140330 */
OptionTestFn option_assist_2p_type1;                /* 0x00140340 */
OptionTestFn option_assist_2p_not_type1;            /* 0x00140350 */
OptionTestFn option_assist_2p_type2;                /* 0x00140360 */
OptionTestFn option_assist_2p_not_type2;            /* 0x00140370 */
OptionTestFn option_assist_2p_type3;                /* 0x00140380 */
OptionTestFn option_assist_2p_not_type3;            /* 0x00140390 */
OptionTestFn option_assist_2p_type4;                /* 0x001403A0 */
OptionTestFn option_assist_2p_not_type4;            /* 0x001403B0 */
OptionTestFn option_assist_2p_type5;                /* 0x001403C0 */
OptionTestFn option_assist_2p_not_type5;            /* 0x001403D0 */
OptionTestFn option_force_feedback_2p_on;           /* 0x001403E0 */
OptionTestFn option_force_feedback_2p_off;          /* 0x001403F0 */
OptionTestFn option_gear_auto_on;                   /* 0x00140400 */
OptionTestFn option_gear_auto_off;                  /* 0x00140410 */
/* T064: slider ranges, texts, the other factories (MenuHookFn) */
OptionRangeFn option_range_race_laps;               /* 0x00140420 */
OptionRangeFn option_range_split_race_laps;         /* 0x00140440 */
OptionRangeFn option_range_split_handicap;          /* 0x00140460 */
OptionTextFn  option_text_race_laps;                /* 0x00140480 */
OptionTextFn  option_text_split_race_laps;          /* 0x001404A8 */
OptionTextFn  option_text_split_handicap;           /* 0x001404D0 */
gt2k_menu::MenuHookFn option_create_key_config_2;           /* 0x001405C0 */
gt2k_menu::MenuHookFn option_create_key_config_1_display;   /* 0x00140698 */
gt2k_menu::MenuHookFn option_create_key_config_2_display;   /* 0x00140760 */
gt2k_menu::MenuHookFn option_create_key_config_steer_1;     /* 0x00140838 */
gt2k_menu::MenuHookFn option_create_key_config_steer_2;     /* 0x00140918 */
gt2k_menu::MenuHookFn option_create_steering_assist_type;   /* 0x001409F8 */

/* T065: the slider functions (MenuSliderFunc::func) and their setters
 * (SliderSetFn) */
int option_slider_set_race_laps(void *ctx, int value, int commit);         /* 0x00141E18 */
gt2k_menu::SliderDesc option_slider_race_laps(void *ctx);                  /* 0x00141E28 */
int option_slider_set_split_race_laps(void *ctx, int value, int commit);   /* 0x00141E68 */
gt2k_menu::SliderDesc option_slider_split_race_laps(void *ctx);            /* 0x00141E78 */
int option_slider_set_split_handicap(void *ctx, int value, int commit);    /* 0x00141EB8 */
gt2k_menu::SliderDesc option_slider_split_handicap(void *ctx);             /* 0x00141EC8 */

/* T066: the multi button functions (MenuMultiFunc::func) and their setters
 * (MultiButtonInfo::changed) */
void option_multi_set_wide_mode(void *ctx, int index);                     /* 0x00141F70 */
gt2k_menu::MultiButtonInfo option_multi_wide_mode(void *ctx);              /* 0x00141F90 */
void option_multi_set_menu_bgm(void *ctx, int index);                      /* 0x00141FB0 */
gt2k_menu::MultiButtonInfo option_multi_menu_bgm(void *ctx);               /* 0x00141FC0 */
void option_multi_set_menu_se(void *ctx, int index);                       /* 0x00141FE8 */
gt2k_menu::MultiButtonInfo option_multi_menu_se(void *ctx);                /* 0x00141FF8 */
void option_multi_set_race_bgm(void *ctx, int index);                      /* 0x00142020 */
gt2k_menu::MultiButtonInfo option_multi_race_bgm(void *ctx);               /* 0x00142030 */
void option_multi_set_race_se(void *ctx, int index);                       /* 0x00142058 */
gt2k_menu::MultiButtonInfo option_multi_race_se(void *ctx);                /* 0x00142068 */
void option_multi_set_replay_bgm(void *ctx, int index);                    /* 0x00142090 */
gt2k_menu::MultiButtonInfo option_multi_replay_bgm(void *ctx);             /* 0x001420A0 */
void option_multi_set_replay_se(void *ctx, int index);                     /* 0x001420C8 */
gt2k_menu::MultiButtonInfo option_multi_replay_se(void *ctx);              /* 0x001420D8 */
void option_multi_set_vibration_1p(void *ctx, int index);                  /* 0x00142100 */
gt2k_menu::MultiButtonInfo option_multi_vibration_1p(void *ctx);           /* 0x00142110 */
void option_multi_set_vibration_2p(void *ctx, int index);                  /* 0x00142138 */
gt2k_menu::MultiButtonInfo option_multi_vibration_2p(void *ctx);           /* 0x00142148 */
void option_multi_set_tire_damage(void *ctx, int index);                   /* 0x00142170 */
gt2k_menu::MultiButtonInfo option_multi_tire_damage(void *ctx);            /* 0x00142180 */
void option_multi_set_split_tire_damage(void *ctx, int index);             /* 0x001421A8 */
gt2k_menu::MultiButtonInfo option_multi_split_tire_damage(void *ctx);      /* 0x001421B8 */
void option_multi_set_split_boost(void *ctx, int index);                   /* 0x001421E0 */
gt2k_menu::MultiButtonInfo option_multi_split_boost(void *ctx);            /* 0x001421E8 */
void option_multi_set_steering_assist_1p(void *ctx, int index);            /* 0x00142208 */
gt2k_menu::MultiButtonInfo option_multi_steering_assist_1p(void *ctx);     /* 0x00142210 */
void option_multi_set_steering_force_1p(void *ctx, int index);             /* 0x00142230 */
gt2k_menu::MultiButtonInfo option_multi_steering_force_1p(void *ctx);      /* 0x00142240 */
void option_multi_set_steering_assist_2p(void *ctx, int index);            /* 0x00142268 */
gt2k_menu::MultiButtonInfo option_multi_steering_assist_2p(void *ctx);     /* 0x00142270 */
void option_multi_set_steering_force_2p(void *ctx, int index);             /* 0x00142290 */
gt2k_menu::MultiButtonInfo option_multi_steering_force_2p(void *ctx);      /* 0x001422A0 */
void option_multi_set_gear_type(void *ctx, int index);                     /* 0x001422C8 */
gt2k_menu::MultiButtonInfo option_multi_gear_type(void *ctx);              /* 0x001422D8 */
void option_multi_set_assist_asm(void *ctx, int index);                    /* 0x00142300 */
gt2k_menu::MultiButtonInfo option_multi_assist_asm(void *ctx);             /* 0x00142310 */
void option_multi_set_assist_tcs(void *ctx, int index);                    /* 0x00142338 */
gt2k_menu::MultiButtonInfo option_multi_assist_tcs(void *ctx);             /* 0x00142348 */
void option_multi_set_ghost_type(void *ctx, int index);                    /* 0x00142370 */
gt2k_menu::MultiButtonInfo option_multi_ghost_type(void *ctx);             /* 0x00142380 */

/* T067: the button actions (ExecuteFuncData::func) other than set_default */
void F_001425F8(void);                      /* 0x001425F8 empty, unreferenced */
void option_action_commit(void *ctx, gt2k_menu::ButtonData *rec, int index,
                          gt2k_menu::PageLayout *layout);              /* 0x00142600 */
void F_00142608(void);                      /* 0x00142608 empty, unreferenced */
void option_action_retrieve(void *ctx, gt2k_menu::ButtonData *rec, int index,
                            gt2k_menu::PageLayout *layout);            /* 0x00142610 */
void option_action_set_port_1(void *ctx, gt2k_menu::ButtonData *rec, int index,
                              gt2k_menu::PageLayout *layout);          /* 0x00142638 */
void option_action_set_port_2(void *ctx, gt2k_menu::ButtonData *rec, int index,
                              gt2k_menu::PageLayout *layout);          /* 0x00142648 */
void option_action_set_port_1_jaguar(void *ctx, gt2k_menu::ButtonData *rec, int index,
                                     gt2k_menu::PageLayout *layout);   /* 0x00142660 */
void option_action_set_port_2_jaguar(void *ctx, gt2k_menu::ButtonData *rec, int index,
                                     gt2k_menu::PageLayout *layout);   /* 0x00142670 */
void option_action_stop_music(void *ctx, gt2k_menu::ButtonData *rec, int index,
                              gt2k_menu::PageLayout *layout);          /* 0x00142688 */

#endif /* U022_H */
