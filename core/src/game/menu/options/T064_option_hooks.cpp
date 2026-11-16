/* T064 - the layout object hooks of the options screen.
 * 0x00140060..0x00141E18.
 *
 * T064_static_init fills g_002A0AD0 with 105 MenuHooks {name, create, p8,
 * pC, p10, p14} sorted by name; OptionControl::begin (T062) gives the table
 * to both layouts of its PageManager (setHooks, ctx the OptionControl), and
 * PageLayout::createObject (T229) calls create(ctx, layout, parent, item,
 * p8, pC, p10, p14) for an object item whose type is an entry's name.  Most
 * entries are BoolItems (create_bool_item, T234) of the option buttons,
 * drawn while one of the tests below holds: "<option>_on_on" /
 * "<option>_off_off" while the option is on, "<option>_off_on" /
 * "<option>_on_off" while it is off (the lit and unlit pictures of the two
 * buttons).  The others are slider bars (create_slider_item, T228), texts
 * (create_callback_text, T239), "blink" (T205), the song lists (T085) and
 * the factories of this file.  Every `new' below is Object::operator new
 * (T220, the menu heap); the landing pads in the binary only give the block
 * back (Object::operator delete) if the constructor throws.
 *
 * The includes are in the order that gives retail's type names (.rodata
 * 0x0031AAB8, reverse definition order): Cell<Button>, SmartIndexT<int>,
 * OperatorList, List / OperatorHolder / Cell<OperatorHolder>, the garage
 * lists (wheel list .. Cell<GarageCar>), Operator.  So the original defined
 * the garage classes first (T429), then the menu classes (T216), then the
 * page manager (T211's SmartIndexT<int>, through T062), then the buttons
 * (T206, for that name only). */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::getGameOptions */
#include "game/menu/framework/T216_menu_transitive.h"      /* Transitive (the hooks' result); OperatorHolder */
#include "game/menu/options/T062_option_control.h"       /* OptionControl (ctx); T211 PageManager */
#include "game/menu/framework/T206_button.h"               /* (Cell<Button>) */
#include "game/menu/options/T064_option_hooks.h"         /* g_002A0AD0, g_00351708; T229 MenuHook, MenuHookFn */
#include "game/menu/options/T101_key_config_item.h"      /* KeyConfigItem */
#include "game/menu/arcade/T098_handicap_item.h"        /* HandicapItem */
#include "game/menu/options/T100_calibration_item.h"     /* CalibrationItem */
#include "game/menu/options/T085_play_list.h"            /* PlayListBox::factory ("song_edit", "song_list") */
#include "game/menu/framework/T205_blink.h"                /* create_blink ("blink") */
#include "game/menu/framework/T213_menu_item.h"            /* Item, MenuTexture */
#include "game/menu/framework/T214_menu_control.h"         /* MenuControl (control1 / control2) */
#include "game/menu/framework/T220_object.h"               /* Object (operator new / delete) */
#include "game/menu/framework/T228_slider_item.h"          /* create_slider_item */
#include "game/menu/framework/T234_bool_item.h"            /* create_bool_item */
#include "game/menu/framework/T239_callback_text.h"        /* create_callback_text */
#include "game/save/T426_controller_config.h"    /* CONFIG_ANALOG */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/options/options_internal.h"                           /* the unit's constants and callbacks */

using namespace gt2k_menu;

/* ------------------------------------------------------------------------ */
/* create_bool_item tests (p8; called with the OptionControl)               */
/* ------------------------------------------------------------------------ */

// FUNC 0x00140060 option_wide_mode_on
int option_wide_mode_on(OptionControl *ctl)
{
    return ctl->options.wide == 1;
}

// FUNC 0x00140070 option_wide_mode_off
int option_wide_mode_off(OptionControl *ctl)
{
    return ctl->options.wide != 1;
}

// FUNC 0x00140080 option_menu_bgm_on
int option_menu_bgm_on(OptionControl *ctl)
{
    return ctl->options.menuBgm == 1;
}

// FUNC 0x00140090 option_menu_bgm_off
int option_menu_bgm_off(OptionControl *ctl)
{
    return ctl->options.menuBgm != 1;
}

// FUNC 0x001400A0 option_menu_se_on
int option_menu_se_on(OptionControl *ctl)
{
    return ctl->options.menuSe == 1;
}

// FUNC 0x001400B0 option_menu_se_off
int option_menu_se_off(OptionControl *ctl)
{
    return ctl->options.menuSe != 1;
}

// FUNC 0x001400C0 option_race_bgm_on
int option_race_bgm_on(OptionControl *ctl)
{
    return ctl->options.raceBgm == 1;
}

// FUNC 0x001400D0 option_race_bgm_off
int option_race_bgm_off(OptionControl *ctl)
{
    return ctl->options.raceBgm != 1;
}

// FUNC 0x001400E0 option_race_se_on
int option_race_se_on(OptionControl *ctl)
{
    return ctl->options.raceSe == 1;
}

// FUNC 0x001400F0 option_race_se_off
int option_race_se_off(OptionControl *ctl)
{
    return ctl->options.raceSe != 1;
}

// FUNC 0x00140100 option_replay_bgm_on
int option_replay_bgm_on(OptionControl *ctl)
{
    return ctl->options.replayBgm == 1;
}

// FUNC 0x00140110 option_replay_bgm_off
int option_replay_bgm_off(OptionControl *ctl)
{
    return ctl->options.replayBgm != 1;
}

// FUNC 0x00140120 option_replay_se_on
int option_replay_se_on(OptionControl *ctl)
{
    return ctl->options.replaySe == 1;
}

// FUNC 0x00140130 option_replay_se_off
int option_replay_se_off(OptionControl *ctl)
{
    return ctl->options.replaySe != 1;
}

// FUNC 0x00140140 option_vib1_on
int option_vib1_on(OptionControl *ctl)
{
    return ctl->options.vibration1p == 1;
}

// FUNC 0x00140150 option_vib1_off
int option_vib1_off(OptionControl *ctl)
{
    return ctl->options.vibration1p != 1;
}

// FUNC 0x00140160 option_vib2_on
int option_vib2_on(OptionControl *ctl)
{
    return ctl->options.vibration2p == 1;
}

// FUNC 0x00140170 option_vib2_off
int option_vib2_off(OptionControl *ctl)
{
    return ctl->options.vibration2p != 1;
}

// FUNC 0x00140180 option_tire_damage_on
int option_tire_damage_on(OptionControl *ctl)
{
    return ctl->options.tireDamage == 1;
}

// FUNC 0x00140190 option_tire_damage_off
int option_tire_damage_off(OptionControl *ctl)
{
    return ctl->options.tireDamage != 1;
}

// FUNC 0x001401A0 option_ghost_on
int option_ghost_on(OptionControl *ctl)
{
    return ctl->options.ghost == 1;
}

// FUNC 0x001401B0 option_ghost_off
int option_ghost_off(OptionControl *ctl)
{
    return ctl->options.ghost != 1;
}

// FUNC 0x001401C0 option_assist_asm_on
int option_assist_asm_on(OptionControl *ctl)
{
    return ctl->options.assistAsm == 1;
}

// FUNC 0x001401D0 option_assist_asm_off
int option_assist_asm_off(OptionControl *ctl)
{
    return ctl->options.assistAsm != 1;
}

// FUNC 0x001401E0 option_assist_tcs_on
int option_assist_tcs_on(OptionControl *ctl)
{
    return ctl->options.assistTcs == 1;
}

// FUNC 0x001401F0 option_assist_tcs_off
int option_assist_tcs_off(OptionControl *ctl)
{
    return ctl->options.assistTcs != 1;
}

// FUNC 0x00140200 option_split_tire_damage_on
int option_split_tire_damage_on(OptionControl *ctl)
{
    return ctl->options.splitBattle.tireDamage == 1;
}

// FUNC 0x00140210 option_split_tire_damage_off
int option_split_tire_damage_off(OptionControl *ctl)
{
    return ctl->options.splitBattle.tireDamage != 1;
}

// FUNC 0x00140220 option_split_boost_none
int option_split_boost_none(OptionControl *ctl)
{
    return ctl->options.splitBattle.boost == SPLIT_BOOST_NONE;
}

// FUNC 0x00140230 option_split_boost_not_none
int option_split_boost_not_none(OptionControl *ctl)
{
    return ctl->options.splitBattle.boost != SPLIT_BOOST_NONE;
}

// FUNC 0x00140240 option_split_boost_low
int option_split_boost_low(OptionControl *ctl)
{
    return ctl->options.splitBattle.boost == SPLIT_BOOST_LOW;
}

// FUNC 0x00140250 option_split_boost_not_low
int option_split_boost_not_low(OptionControl *ctl)
{
    return ctl->options.splitBattle.boost != SPLIT_BOOST_LOW;
}

// FUNC 0x00140260 option_split_boost_high
int option_split_boost_high(OptionControl *ctl)
{
    return ctl->options.splitBattle.boost == SPLIT_BOOST_HIGH;
}

// FUNC 0x00140270 option_split_boost_not_high
int option_split_boost_not_high(OptionControl *ctl)
{
    return ctl->options.splitBattle.boost != SPLIT_BOOST_HIGH;
}

/* "assist_typeN_on_1p" / "assist_typeN_off_1p": steering assist type N of
 * player 1 (stored as N - 1). */
// FUNC 0x00140280 option_assist_1p_type1
int option_assist_1p_type1(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p == STEERING_ASSIST_TYPE1;
}

// FUNC 0x00140290 option_assist_1p_not_type1
int option_assist_1p_not_type1(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p != STEERING_ASSIST_TYPE1;
}

// FUNC 0x001402A0 option_assist_1p_type2
int option_assist_1p_type2(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p == STEERING_ASSIST_TYPE2;
}

// FUNC 0x001402B0 option_assist_1p_not_type2
int option_assist_1p_not_type2(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p != STEERING_ASSIST_TYPE2;
}

// FUNC 0x001402C0 option_assist_1p_type3
int option_assist_1p_type3(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p == STEERING_ASSIST_TYPE3;
}

// FUNC 0x001402D0 option_assist_1p_not_type3
int option_assist_1p_not_type3(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p != STEERING_ASSIST_TYPE3;
}

// FUNC 0x001402E0 option_assist_1p_type4
int option_assist_1p_type4(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p == STEERING_ASSIST_TYPE4;
}

// FUNC 0x001402F0 option_assist_1p_not_type4
int option_assist_1p_not_type4(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p != STEERING_ASSIST_TYPE4;
}

// FUNC 0x00140300 option_assist_1p_type5
int option_assist_1p_type5(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p == STEERING_ASSIST_TYPE5;
}

// FUNC 0x00140310 option_assist_1p_not_type5
int option_assist_1p_not_type5(OptionControl *ctl)
{
    return ctl->options.steeringAssist1p != STEERING_ASSIST_TYPE5;
}

// FUNC 0x00140320 option_force_feedback_1p_on
int option_force_feedback_1p_on(OptionControl *ctl)
{
    return ctl->options.forceFeedback1p == 1;
}

// FUNC 0x00140330 option_force_feedback_1p_off
int option_force_feedback_1p_off(OptionControl *ctl)
{
    return ctl->options.forceFeedback1p != 1;
}

/* The same for player 2. */
// FUNC 0x00140340 option_assist_2p_type1
int option_assist_2p_type1(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p == STEERING_ASSIST_TYPE1;
}

// FUNC 0x00140350 option_assist_2p_not_type1
int option_assist_2p_not_type1(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p != STEERING_ASSIST_TYPE1;
}

// FUNC 0x00140360 option_assist_2p_type2
int option_assist_2p_type2(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p == STEERING_ASSIST_TYPE2;
}

// FUNC 0x00140370 option_assist_2p_not_type2
int option_assist_2p_not_type2(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p != STEERING_ASSIST_TYPE2;
}

// FUNC 0x00140380 option_assist_2p_type3
int option_assist_2p_type3(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p == STEERING_ASSIST_TYPE3;
}

// FUNC 0x00140390 option_assist_2p_not_type3
int option_assist_2p_not_type3(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p != STEERING_ASSIST_TYPE3;
}

// FUNC 0x001403A0 option_assist_2p_type4
int option_assist_2p_type4(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p == STEERING_ASSIST_TYPE4;
}

// FUNC 0x001403B0 option_assist_2p_not_type4
int option_assist_2p_not_type4(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p != STEERING_ASSIST_TYPE4;
}

// FUNC 0x001403C0 option_assist_2p_type5
int option_assist_2p_type5(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p == STEERING_ASSIST_TYPE5;
}

// FUNC 0x001403D0 option_assist_2p_not_type5
int option_assist_2p_not_type5(OptionControl *ctl)
{
    return ctl->options.steeringAssist2p != STEERING_ASSIST_TYPE5;
}

// FUNC 0x001403E0 option_force_feedback_2p_on
int option_force_feedback_2p_on(OptionControl *ctl)
{
    return ctl->options.forceFeedback2p == 1;
}

// FUNC 0x001403F0 option_force_feedback_2p_off
int option_force_feedback_2p_off(OptionControl *ctl)
{
    return ctl->options.forceFeedback2p != 1;
}

// FUNC 0x00140400 option_gear_auto_on
int option_gear_auto_on(OptionControl *ctl)
{
    return ctl->options.gearAuto == 1;
}

// FUNC 0x00140410 option_gear_auto_off
int option_gear_auto_off(OptionControl *ctl)
{
    return ctl->options.gearAuto != 1;
}

/* ------------------------------------------------------------------------ */
/* create_slider_item ranges (p8, a SliderGetter; called with the           */
/* OptionControl at every draw): the bar's range [min, end) and the value,  */
/* the same as the T065 slider functions of the same names                  */
/* ------------------------------------------------------------------------ */

/* "slider_race_laps": [1, 100), i.e. 1..99 laps. */
// FUNC 0x00140420 option_range_race_laps
void option_range_race_laps(OptionControl *ctl, int *min, int *end, int *value)
{
    *min   = 1;
    *end   = 100;
    *value = ctl->options.raceLaps;
}

/* "slider_split_race_laps": [1, 100). */
// FUNC 0x00140440 option_range_split_race_laps
void option_range_split_race_laps(OptionControl *ctl, int *min, int *end, int *value)
{
    *min   = 1;
    *end   = 100;
    *value = ctl->options.splitBattle.raceLaps;
}

/* "slider_split_handicap": [-150, 151), i.e. -150..150 metres. */
// FUNC 0x00140460 option_range_split_handicap
void option_range_split_handicap(OptionControl *ctl, int *min, int *end, int *value)
{
    *min   = -150;
    *end   = 151;
    *value = ctl->options.splitBattle.handicap;
}

/* ------------------------------------------------------------------------ */
/* create_callback_text texts (p8; called with the OptionControl before     */
/* every draw, buf 0x80 bytes)                                              */
/* ------------------------------------------------------------------------ */

/* "race_laps".  (The value is read into a local first: written as the
 * argument, egcs copies ctl to $v0 before $a0 is overwritten.) */
// FUNC 0x00140480 option_text_race_laps
void option_text_race_laps(OptionControl *ctl, char *buf, int size)
{
    int laps = ctl->options.raceLaps;

    sprintf(buf, "%d laps", laps);         /* 0x003516F8 */
}

/* "split_race_laps" */
// FUNC 0x001404A8 option_text_split_race_laps
void option_text_split_race_laps(OptionControl *ctl, char *buf, int size)
{
    int laps = ctl->options.splitBattle.raceLaps;

    sprintf(buf, "%d laps", laps);         /* 0x003516F8 */
}

/* "split_handicap": the handicap in metres. */
// FUNC 0x001404D0 option_text_split_handicap
void option_text_split_handicap(OptionControl *ctl, char *buf, int size)
{
    int metres = ctl->options.splitBattle.handicap;

    sprintf(buf, "%d m", metres);          /* 0x00351700 */
}

/* ------------------------------------------------------------------------ */
/* Factories (MenuHookFn; parent and the unused p* are not read)            */
/* ------------------------------------------------------------------------ */
/* The key configuration pages: KeyConfigItem(owner, item, texture, pad,
 * pages, layout, mode, port, editable, usb).  The item's texture is looked
 * up before the allocation; the pad's and the PageManager's addresses are
 * taken into locals after it (retail keeps them in callee-saved registers
 * from there: written in the argument list they are computed after the
 * `new').  The constructor's return value (this, egcs) is the result. */

/* "key_config_1": player 1's analog pad page, editable with player 1's pad. */
// FUNC 0x001404F8 option_create_key_config_1
Transitive *option_create_key_config_1(void *ctx, PageLayout *layout, Object *parent,
                                       MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    OptionControl *ctl = (OptionControl *)ctx;
    MenuTexture *texture = layout->getTexture(item);
    MenuControl *pad = &ctl->control1;
    PageManager *pages = &ctl->pages;

    /* Object::operator new(0x1180); Object::operator delete if it throws */
    return new KeyConfigItem(ctl, item, texture, pad, pages, layout, /* implicit: Object::operator delete (if the constructor throws) */
                             CONFIG_ANALOG, 0, 1, 0);
}

/* "key_config_2": player 2's analog pad page, editable with player 2's pad. */
// FUNC 0x001405C0 option_create_key_config_2
Transitive *option_create_key_config_2(void *ctx, PageLayout *layout, Object *parent,
                                       MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    OptionControl *ctl = (OptionControl *)ctx;
    MenuTexture *texture = layout->getTexture(item);
    MenuControl *pad = &ctl->control2;
    PageManager *pages = &ctl->pages;

    /* Object::operator new(0x1180); Object::operator delete if it throws */
    return new KeyConfigItem(ctl, item, texture, pad, pages, layout, /* implicit: Object::operator delete (if the constructor throws) */
                             CONFIG_ANALOG, 1, 1, 0);
}

/* "key_config_1_display": player 1's page, display only. */
// FUNC 0x00140698 option_create_key_config_1_display
Transitive *option_create_key_config_1_display(void *ctx, PageLayout *layout, Object *parent,
                                               MenuLayoutItem *item, int p8, int pC, int p10,
                                               int p14)
{
    OptionControl *ctl = (OptionControl *)ctx;
    MenuTexture *texture = layout->getTexture(item);
    MenuControl *pad = &ctl->control1;
    PageManager *pages = &ctl->pages;

    /* Object::operator new(0x1180); Object::operator delete if it throws */
    return new KeyConfigItem(ctl, item, texture, pad, pages, layout, /* implicit: Object::operator delete (if the constructor throws) */
                             CONFIG_ANALOG, 0, 0, 0);
}

/* "key_config_2_display": player 2's page, display only. */
// FUNC 0x00140760 option_create_key_config_2_display
Transitive *option_create_key_config_2_display(void *ctx, PageLayout *layout, Object *parent,
                                               MenuLayoutItem *item, int p8, int pC, int p10,
                                               int p14)
{
    OptionControl *ctl = (OptionControl *)ctx;
    MenuTexture *texture = layout->getTexture(item);
    MenuControl *pad = &ctl->control2;
    PageManager *pages = &ctl->pages;

    /* Object::operator new(0x1180); Object::operator delete if it throws */
    return new KeyConfigItem(ctl, item, texture, pad, pages, layout, /* implicit: Object::operator delete (if the constructor throws) */
                             CONFIG_ANALOG, 1, 0, 0);
}

/* "key_config_steer_1": the USB wheel page of the port and wheel set by the
 * set_port_* actions (mode calibWheel, port calibPort, both read after the
 * texture), editable with player 1's pad. */
// FUNC 0x00140838 option_create_key_config_steer_1
Transitive *option_create_key_config_steer_1(void *ctx, PageLayout *layout, Object *parent,
                                             MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    OptionControl *ctl = (OptionControl *)ctx;
    MenuTexture *texture = layout->getTexture(item);
    MenuControl *pad = &ctl->control1;
    PageManager *pages = &ctl->pages;
    int wheel = ctl->calibWheel;
    int port  = ctl->calibPort;

    /* Object::operator new(0x1180); Object::operator delete if it throws */
    return new KeyConfigItem(ctl, item, texture, pad, pages, layout, /* implicit: Object::operator delete (if the constructor throws) */
                             wheel, port, 1, 1);
}

/* "key_config_steer_2": the same code as key_config_steer_1 (player 1's pad
 * too; the port comes from calibPort). */
// FUNC 0x00140918 option_create_key_config_steer_2
Transitive *option_create_key_config_steer_2(void *ctx, PageLayout *layout, Object *parent,
                                             MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    OptionControl *ctl = (OptionControl *)ctx;
    MenuTexture *texture = layout->getTexture(item);
    MenuControl *pad = &ctl->control1;
    PageManager *pages = &ctl->pages;
    int wheel = ctl->calibWheel;
    int port  = ctl->calibPort;

    /* Object::operator new(0x1180); Object::operator delete if it throws */
    return new KeyConfigItem(ctl, item, texture, pad, pages, layout, /* implicit: Object::operator delete (if the constructor throws) */
                             wheel, port, 1, 1);
}

/* "steering_assist_type_1p" (p8 0) / "_2p" (p8 1): the hidden picture
 * "steering_type<N>" of the player's steering assist type N, centred on the
 * item.  It reads the options in force (GameStatus), not the copy being
 * edited. */
// FUNC 0x001409F8 option_create_steering_assist_type
Transitive *option_create_steering_assist_type(void *ctx, PageLayout *layout, Object *parent,
                                               MenuLayoutItem *item, int p8, int pC, int p10,
                                               int p14)
{
    char name[0x40];                        /* sp+0x00 */
    int type;

    if (p8 == 0)
        type = getGameStatus()->getGameOptions()->steeringAssist1p;
    else
        type = getGameStatus()->getGameOptions()->steeringAssist2p;
    sprintf(name, "steering_type%d", type + 1);
    /* Object::operator new(0x100); Object::operator delete if it throws */
    return new Item(name, layout, item); /* implicit: Object::operator delete (if the constructor throws) */
}

/* "handicap_1p" (p8 1) / "handicap_2p" (p8 2): the handicap bar of player
 * p8 over the edited options. */
// FUNC 0x00140AC0 option_create_handicap
Transitive *option_create_handicap(void *ctx, PageLayout *layout, Object *parent,
                                   MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ::GameOptions *options = &ctl->options;
    int player = p8;

    /* Object::operator new(0x228); Object::operator delete if it throws */
    return new HandicapItem(options, player, item, layout); /* implicit: Object::operator delete (if the constructor throws) */
}

/* "calib_steer_bar" (p8 CALIB_STEER), "calib_center_bar" (CALIB_CENTER),
 * "calib_accel_bar" (CALIB_ACCEL), "calib_brake_bar" (CALIB_BRAKE): one bar
 * of the wheel calibration page, for the port and wheel set by the
 * set_port_* actions (both read before the allocation). */
// FUNC 0x00140B58 option_create_calib_bar
Transitive *option_create_calib_bar(void *ctx, PageLayout *layout, Object *parent,
                                    MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ::GameOptions *options = &ctl->options;
    int port  = ctl->calibPort;
    int wheel = ctl->calibWheel;
    int axis  = p8;

    /* Object::operator new(0x2F0); Object::operator delete if it throws */
    return new CalibrationItem(item, layout, ctl, options, port, wheel, axis); /* implicit: Object::operator delete (if the constructor throws) */
}

/* ------------------------------------------------------------------------ */
/* The hook table                                                           */
/* ------------------------------------------------------------------------ */

/* The table, .data 0x002A0AD0..0x002A14A8: no initialiser, so egcs writes
 * its zeros in .data in front of the frame info (retail's are zero too: the
 * static initialiser below fills it at boot). */
MenuHook g_002A0AD0[105];                   /* 0x002A0AD0 */

/* The table's entry count, .sdata 0x00351708: a constant word (105), read
 * by OptionControl::begin (T062).  Defined here, after the factories, where
 * the original defined it: its place in .sdata, between the texts' "%d m"
 * and the initialiser's "blink" (0x00351710), shows it. */
int g_00351708 = sizeof(g_002A0AD0) / sizeof(g_002A0AD0[0]);

/* One table entry: the inline constructor of the original's MenuHook (see
 * T064_static_init). */
static inline void set_entry(MenuHook *e, const char *name, MenuHookFn *create,
                             int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* The library hooks take fewer or typed parameters (p8 / pC as what they
 * are): the table holds them as MenuHookFn. */
#define HOOK_BOOL_ITEM      create_bool_item        /* T234 p8 a test, pC 0: every draw */
#define HOOK_SLIDER_ITEM    create_slider_item      /* T228 p8 a range */
#define HOOK_CALLBACK_TEXT  create_callback_text    /* T239 p8 a text, pC 0: every draw */
#define HOOK_BLINK          create_blink            /* T205 */
#define HOOK_PLAY_LIST      ((MenuHookFn *)PlayListBox::factory)    /* T085 p8 PlayListBox's p0 */

/* One element as egcs's build_vec_init expands it (u006's form): the
 * constructor called on a copy of the cursor (`this'), then the cursor and
 * the count of the elements left stepped.  pC, p10, p14 are 0 in every
 * entry.  (No do-while wrapper: its loop notes would change the code.) */
#define HOOK(name, create, p8)                                                    { MenuHook *e = base; set_entry(e, name, create, p8, 0, 0, 0); }            base++;                                                                       left--

/* Static initialiser of the hook table g_002A0AD0 (.ctors entry
 * 0x0028F084): a compiler-generated _GLOBAL_$I$ function in the original.
 * All six words of every entry are stored.  The names are sorted
 * (PageLayout::findHook binary-searches them); entry i is at 0x002A0AD0 +
 * 0x18 * i.  p8 of the factories of this file: see each one.
 *
 * The original defined the table here, with MenuHook objects built by an
 * inline constructor, e.g. MenuHook("blink", create_blink, 0, 0, 0, 0):
 * egcs builds such a table at run time, in this function (cp/init.c
 * build_vec_init), and leaves .data zero (as retail's 0x002A0AD0..
 * 0x002A14A8).  That needs a constructor in T229's MenuHook, a plain struct
 * every menu unit shares; the table is defined above without one, so the
 * function is written out the way build_vec_init expands it (a walking
 * cursor, a copy of it per element, the dead count of the elements left):
 * an exact form, not the original.  It stores the same 105 entries as a
 * plain walk or indexing would. */
// FUNC 0x00140C20 T064_static_init
void T064_static_init(void)
{
    MenuHook *base = g_002A0AD0;
    int left = 104;                         /* build_vec_init's iterator: elements - 1 */

    HOOK("assist_asm_off",            HOOK_BOOL_ITEM,                     (int)option_assist_asm_off);
    HOOK("assist_asm_on",             HOOK_BOOL_ITEM,                     (int)option_assist_asm_on);
    HOOK("assist_tcs_off",            HOOK_BOOL_ITEM,                     (int)option_assist_tcs_off);
    HOOK("assist_tcs_on",             HOOK_BOOL_ITEM,                     (int)option_assist_tcs_on);
    HOOK("assist_type1_off_1p",       HOOK_BOOL_ITEM,                     (int)option_assist_1p_not_type1);
    HOOK("assist_type1_off_2p",       HOOK_BOOL_ITEM,                     (int)option_assist_2p_not_type1);
    HOOK("assist_type1_on_1p",        HOOK_BOOL_ITEM,                     (int)option_assist_1p_type1);
    HOOK("assist_type1_on_2p",        HOOK_BOOL_ITEM,                     (int)option_assist_2p_type1);
    HOOK("assist_type2_off_1p",       HOOK_BOOL_ITEM,                     (int)option_assist_1p_not_type2);
    HOOK("assist_type2_off_2p",       HOOK_BOOL_ITEM,                     (int)option_assist_2p_not_type2);
    HOOK("assist_type2_on_1p",        HOOK_BOOL_ITEM,                     (int)option_assist_1p_type2);
    HOOK("assist_type2_on_2p",        HOOK_BOOL_ITEM,                     (int)option_assist_2p_type2);
    HOOK("assist_type3_off_1p",       HOOK_BOOL_ITEM,                     (int)option_assist_1p_not_type3);
    HOOK("assist_type3_off_2p",       HOOK_BOOL_ITEM,                     (int)option_assist_2p_not_type3);
    HOOK("assist_type3_on_1p",        HOOK_BOOL_ITEM,                     (int)option_assist_1p_type3);
    HOOK("assist_type3_on_2p",        HOOK_BOOL_ITEM,                     (int)option_assist_2p_type3);
    HOOK("assist_type4_off_1p",       HOOK_BOOL_ITEM,                     (int)option_assist_1p_not_type4);
    HOOK("assist_type4_off_2p",       HOOK_BOOL_ITEM,                     (int)option_assist_2p_not_type4);
    HOOK("assist_type4_on_1p",        HOOK_BOOL_ITEM,                     (int)option_assist_1p_type4);
    HOOK("assist_type4_on_2p",        HOOK_BOOL_ITEM,                     (int)option_assist_2p_type4);
    HOOK("assist_type5_off_1p",       HOOK_BOOL_ITEM,                     (int)option_assist_1p_not_type5);
    HOOK("assist_type5_off_2p",       HOOK_BOOL_ITEM,                     (int)option_assist_2p_not_type5);
    HOOK("assist_type5_on_1p",        HOOK_BOOL_ITEM,                     (int)option_assist_1p_type5);
    HOOK("assist_type5_on_2p",        HOOK_BOOL_ITEM,                     (int)option_assist_2p_type5);
    HOOK("blink",                     HOOK_BLINK,                         0);
    HOOK("calib_accel_bar",           option_create_calib_bar,            CALIB_ACCEL);
    HOOK("calib_brake_bar",           option_create_calib_bar,            CALIB_BRAKE);
    HOOK("calib_center_bar",          option_create_calib_bar,            CALIB_CENTER);
    HOOK("calib_steer_bar",           option_create_calib_bar,            CALIB_STEER);
    HOOK("force_feedback_off_1p",     HOOK_BOOL_ITEM,                     (int)option_force_feedback_1p_off);
    HOOK("force_feedback_off_2p",     HOOK_BOOL_ITEM,                     (int)option_force_feedback_2p_off);
    HOOK("force_feedback_on_1p",      HOOK_BOOL_ITEM,                     (int)option_force_feedback_1p_on);
    HOOK("force_feedback_on_2p",      HOOK_BOOL_ITEM,                     (int)option_force_feedback_2p_on);
    HOOK("gear_auto_off",             HOOK_BOOL_ITEM,                     (int)option_gear_auto_off);
    HOOK("gear_auto_on",              HOOK_BOOL_ITEM,                     (int)option_gear_auto_on);
    HOOK("ghost_off",                 HOOK_BOOL_ITEM,                     (int)option_ghost_off);
    HOOK("ghost_on",                  HOOK_BOOL_ITEM,                     (int)option_ghost_on);
    HOOK("handicap_1p",               option_create_handicap,             1);
    HOOK("handicap_2p",               option_create_handicap,             2);
    HOOK("key_config_1",              option_create_key_config_1,         0);
    HOOK("key_config_1_display",      option_create_key_config_1_display, 0);
    HOOK("key_config_2",              option_create_key_config_2,         0);
    HOOK("key_config_2_display",      option_create_key_config_2_display, 0);
    HOOK("key_config_steer_1",        option_create_key_config_steer_1,   0);
    HOOK("key_config_steer_2",        option_create_key_config_steer_2,   0);
    HOOK("menu_bgm_off_off",          HOOK_BOOL_ITEM,                     (int)option_menu_bgm_on);
    HOOK("menu_bgm_off_on",           HOOK_BOOL_ITEM,                     (int)option_menu_bgm_off);
    HOOK("menu_bgm_on_off",           HOOK_BOOL_ITEM,                     (int)option_menu_bgm_off);
    HOOK("menu_bgm_on_on",            HOOK_BOOL_ITEM,                     (int)option_menu_bgm_on);
    HOOK("menu_se_off_off",           HOOK_BOOL_ITEM,                     (int)option_menu_se_on);
    HOOK("menu_se_off_on",            HOOK_BOOL_ITEM,                     (int)option_menu_se_off);
    HOOK("menu_se_on_off",            HOOK_BOOL_ITEM,                     (int)option_menu_se_off);
    HOOK("menu_se_on_on",             HOOK_BOOL_ITEM,                     (int)option_menu_se_on);
    HOOK("race_bgm_off_off",          HOOK_BOOL_ITEM,                     (int)option_race_bgm_on);
    HOOK("race_bgm_off_on",           HOOK_BOOL_ITEM,                     (int)option_race_bgm_off);
    HOOK("race_bgm_on_off",           HOOK_BOOL_ITEM,                     (int)option_race_bgm_off);
    HOOK("race_bgm_on_on",            HOOK_BOOL_ITEM,                     (int)option_race_bgm_on);
    HOOK("race_laps",                 HOOK_CALLBACK_TEXT,                 (int)option_text_race_laps);
    HOOK("race_se_off_off",           HOOK_BOOL_ITEM,                     (int)option_race_se_on);
    HOOK("race_se_off_on",            HOOK_BOOL_ITEM,                     (int)option_race_se_off);
    HOOK("race_se_on_off",            HOOK_BOOL_ITEM,                     (int)option_race_se_off);
    HOOK("race_se_on_on",             HOOK_BOOL_ITEM,                     (int)option_race_se_on);
    HOOK("replay_bgm_off_off",        HOOK_BOOL_ITEM,                     (int)option_replay_bgm_on);
    HOOK("replay_bgm_off_on",         HOOK_BOOL_ITEM,                     (int)option_replay_bgm_off);
    HOOK("replay_bgm_on_off",         HOOK_BOOL_ITEM,                     (int)option_replay_bgm_off);
    HOOK("replay_bgm_on_on",          HOOK_BOOL_ITEM,                     (int)option_replay_bgm_on);
    HOOK("replay_se_off_off",         HOOK_BOOL_ITEM,                     (int)option_replay_se_on);
    HOOK("replay_se_off_on",          HOOK_BOOL_ITEM,                     (int)option_replay_se_off);
    HOOK("replay_se_on_off",          HOOK_BOOL_ITEM,                     (int)option_replay_se_off);
    HOOK("replay_se_on_on",           HOOK_BOOL_ITEM,                     (int)option_replay_se_on);
    HOOK("slider_race_laps",          HOOK_SLIDER_ITEM,                   (int)option_range_race_laps);
    HOOK("slider_split_handicap",     HOOK_SLIDER_ITEM,                   (int)option_range_split_handicap);
    HOOK("slider_split_race_laps",    HOOK_SLIDER_ITEM,                   (int)option_range_split_race_laps);
    HOOK("song_edit",                 HOOK_PLAY_LIST,                     0);
    HOOK("song_list",                 HOOK_PLAY_LIST,                     1);
    HOOK("split_boost_high_off",      HOOK_BOOL_ITEM,                     (int)option_split_boost_not_high);
    HOOK("split_boost_high_on",       HOOK_BOOL_ITEM,                     (int)option_split_boost_high);
    HOOK("split_boost_low_off",       HOOK_BOOL_ITEM,                     (int)option_split_boost_not_low);
    HOOK("split_boost_low_on",        HOOK_BOOL_ITEM,                     (int)option_split_boost_low);
    HOOK("split_boost_none_off",      HOOK_BOOL_ITEM,                     (int)option_split_boost_not_none);
    HOOK("split_boost_none_on",       HOOK_BOOL_ITEM,                     (int)option_split_boost_none);
    HOOK("split_handicap",            HOOK_CALLBACK_TEXT,                 (int)option_text_split_handicap);
    HOOK("split_race_laps",           HOOK_CALLBACK_TEXT,                 (int)option_text_split_race_laps);
    HOOK("split_tire_damage_off_off", HOOK_BOOL_ITEM,                     (int)option_split_tire_damage_on);
    HOOK("split_tire_damage_off_on",  HOOK_BOOL_ITEM,                     (int)option_split_tire_damage_off);
    HOOK("split_tire_damage_on_off",  HOOK_BOOL_ITEM,                     (int)option_split_tire_damage_off);
    HOOK("split_tire_damage_on_on",   HOOK_BOOL_ITEM,                     (int)option_split_tire_damage_on);
    HOOK("steering_assist_type_1p",   option_create_steering_assist_type, 0);
    HOOK("steering_assist_type_2p",   option_create_steering_assist_type, 1);
    HOOK("tire_damage_off_off",       HOOK_BOOL_ITEM,                     (int)option_tire_damage_on);
    HOOK("tire_damage_off_on",        HOOK_BOOL_ITEM,                     (int)option_tire_damage_off);
    HOOK("tire_damage_on_off",        HOOK_BOOL_ITEM,                     (int)option_tire_damage_off);
    HOOK("tire_damage_on_on",         HOOK_BOOL_ITEM,                     (int)option_tire_damage_on);
    HOOK("vib1_off_off",              HOOK_BOOL_ITEM,                     (int)option_vib1_on);
    HOOK("vib1_off_on",               HOOK_BOOL_ITEM,                     (int)option_vib1_off);
    HOOK("vib1_on_off",               HOOK_BOOL_ITEM,                     (int)option_vib1_off);
    HOOK("vib1_on_on",                HOOK_BOOL_ITEM,                     (int)option_vib1_on);
    HOOK("vib2_off_off",              HOOK_BOOL_ITEM,                     (int)option_vib2_on);
    HOOK("vib2_off_on",               HOOK_BOOL_ITEM,                     (int)option_vib2_off);
    HOOK("vib2_on_off",               HOOK_BOOL_ITEM,                     (int)option_vib2_off);
    HOOK("vib2_on_on",                HOOK_BOOL_ITEM,                     (int)option_vib2_on);
    HOOK("wide_mode_off_off",         HOOK_BOOL_ITEM,                     (int)option_wide_mode_on);
    HOOK("wide_mode_off_on",          HOOK_BOOL_ITEM,                     (int)option_wide_mode_off);
    HOOK("wide_mode_on_off",          HOOK_BOOL_ITEM,                     (int)option_wide_mode_off);
    HOOK("wide_mode_on_on",           HOOK_BOOL_ITEM,                     (int)option_wide_mode_on);
}
