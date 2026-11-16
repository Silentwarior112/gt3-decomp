/* T066 - the multi button functions of the options screen.
 * 0x00141F70..0x001425F8.
 *
 * T066_static_init fills g_002A1CD8 with 21 MenuMultiFuncs sorted by name;
 * OptionControl::begin (T062) gives the table to both layouts of its
 * PageManager (setMultiFuncs, ctx the OptionControl).  ButtonList::
 * createMultiButton (T244) calls func(ctx) for an element named "multi..."
 * and builds a MultiButton (T269) on the MultiButtonInfo it returns (by
 * value: the caller's buffer is the hidden first argument and comes back in
 * $v0): the initial choice, read from the edited GameOptions, and the
 * function the button calls with the choice (changed), which stores it.
 * The on / off options list "on" first: index = 1 - flag, and a choice
 * stores flag = (index != MULTI_CHOICE_OFF).  The wide mode lists "off"
 * first (index = flag) and is stored through GameOptions::setWide, which
 * applies it at once.  The steering assist types and the boost store the
 * choice itself.
 *
 * The includes are in the order that gives retail's type names (.rodata
 * 0x0031B0E0, reverse definition order): SmartIndexT<int>, Cell<Button>,
 * OperatorList, List / OperatorHolder / Cell<OperatorHolder>, the garage
 * lists (wheel list .. Cell<GarageCar>), Operator.  So the original defined
 * the garage classes first (T429), then the multi button (T269: the menu
 * classes, the buttons; for those names only), then the page manager
 * (T211's SmartIndexT<int>, through T062). */
#include "game/save/T429_game_status.h"          /* (the garage classes) */
#include "game/menu/framework/T269_multi_button.h"         /* (Cell<Button>, OperatorHolder) */
#include "game/menu/options/T066_option_multis.h"        /* g_002A1CD8, g_0035171C; T229 MultiButtonInfo */
#include "game/menu/options/T062_option_control.h"       /* OptionControl (ctx): options; T428 setWide */
#include "game/menu/options/options_internal.h"                           /* the unit's constants and callbacks */

/* .sdata opens with g_0035171C, which only T062's code reads. */
// DATA_START 0x0035171C

using namespace gt2k_menu;

/* "multi_wide_mode", and also "multi_cfg_steering_1p" (sic: that entry
 * shows and sets the wide mode too). */
// FUNC 0x00141F70 option_multi_set_wide_mode
void option_multi_set_wide_mode(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.setWide(index != 0);
}

// FUNC 0x00141F90 option_multi_wide_mode
MultiButtonInfo option_multi_wide_mode(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_wide_mode, ctl->options.wide);
}

// FUNC 0x00141FB0 option_multi_set_menu_bgm
void option_multi_set_menu_bgm(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.menuBgm = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00141FC0 option_multi_menu_bgm
MultiButtonInfo option_multi_menu_bgm(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_menu_bgm, 1 - ctl->options.menuBgm);
}

// FUNC 0x00141FE8 option_multi_set_menu_se
void option_multi_set_menu_se(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.menuSe = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00141FF8 option_multi_menu_se
MultiButtonInfo option_multi_menu_se(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_menu_se, 1 - ctl->options.menuSe);
}

// FUNC 0x00142020 option_multi_set_race_bgm
void option_multi_set_race_bgm(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.raceBgm = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00142030 option_multi_race_bgm
MultiButtonInfo option_multi_race_bgm(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_race_bgm, 1 - ctl->options.raceBgm);
}

// FUNC 0x00142058 option_multi_set_race_se
void option_multi_set_race_se(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.raceSe = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00142068 option_multi_race_se
MultiButtonInfo option_multi_race_se(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_race_se, 1 - ctl->options.raceSe);
}

// FUNC 0x00142090 option_multi_set_replay_bgm
void option_multi_set_replay_bgm(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.replayBgm = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x001420A0 option_multi_replay_bgm
MultiButtonInfo option_multi_replay_bgm(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_replay_bgm, 1 - ctl->options.replayBgm);
}

// FUNC 0x001420C8 option_multi_set_replay_se
void option_multi_set_replay_se(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.replaySe = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x001420D8 option_multi_replay_se
MultiButtonInfo option_multi_replay_se(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_replay_se, 1 - ctl->options.replaySe);
}

// FUNC 0x00142100 option_multi_set_vibration_1p
void option_multi_set_vibration_1p(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.vibration1p = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00142110 option_multi_vibration_1p
MultiButtonInfo option_multi_vibration_1p(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_vibration_1p, 1 - ctl->options.vibration1p);
}

// FUNC 0x00142138 option_multi_set_vibration_2p
void option_multi_set_vibration_2p(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.vibration2p = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00142148 option_multi_vibration_2p
MultiButtonInfo option_multi_vibration_2p(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_vibration_2p, 1 - ctl->options.vibration2p);
}

// FUNC 0x00142170 option_multi_set_tire_damage
void option_multi_set_tire_damage(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.tireDamage = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00142180 option_multi_tire_damage
MultiButtonInfo option_multi_tire_damage(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_tire_damage, 1 - ctl->options.tireDamage);
}

// FUNC 0x001421A8 option_multi_set_split_tire_damage
void option_multi_set_split_tire_damage(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.splitBattle.tireDamage = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x001421B8 option_multi_split_tire_damage
MultiButtonInfo option_multi_split_tire_damage(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_split_tire_damage, 1 - ctl->options.splitBattle.tireDamage);
}

/* "multi_split_boost": SPLIT_BOOST_NONE / _LOW / _HIGH, as listed. */
// FUNC 0x001421E0 option_multi_set_split_boost
void option_multi_set_split_boost(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.splitBattle.boost = index;
}

// FUNC 0x001421E8 option_multi_split_boost
MultiButtonInfo option_multi_split_boost(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_split_boost, ctl->options.splitBattle.boost);
}

/* "multi_steering_assist_1p": assist type 1..5 as 0..4. */
// FUNC 0x00142208 option_multi_set_steering_assist_1p
void option_multi_set_steering_assist_1p(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.steeringAssist1p = index;
}

// FUNC 0x00142210 option_multi_steering_assist_1p
MultiButtonInfo option_multi_steering_assist_1p(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_steering_assist_1p, ctl->options.steeringAssist1p);
}

/* "multi_steering_force_1p": the force feedback flag. */
// FUNC 0x00142230 option_multi_set_steering_force_1p
void option_multi_set_steering_force_1p(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.forceFeedback1p = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00142240 option_multi_steering_force_1p
MultiButtonInfo option_multi_steering_force_1p(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_steering_force_1p, 1 - ctl->options.forceFeedback1p);
}

// FUNC 0x00142268 option_multi_set_steering_assist_2p
void option_multi_set_steering_assist_2p(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.steeringAssist2p = index;
}

// FUNC 0x00142270 option_multi_steering_assist_2p
MultiButtonInfo option_multi_steering_assist_2p(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_steering_assist_2p, ctl->options.steeringAssist2p);
}

// FUNC 0x00142290 option_multi_set_steering_force_2p
void option_multi_set_steering_force_2p(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.forceFeedback2p = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x001422A0 option_multi_steering_force_2p
MultiButtonInfo option_multi_steering_force_2p(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_steering_force_2p, 1 - ctl->options.forceFeedback2p);
}

/* "multi_gear_type": the automatic gear flag ("automatic" listed first). */
// FUNC 0x001422C8 option_multi_set_gear_type
void option_multi_set_gear_type(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.gearAuto = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x001422D8 option_multi_gear_type
MultiButtonInfo option_multi_gear_type(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_gear_type, 1 - ctl->options.gearAuto);
}

// FUNC 0x00142300 option_multi_set_assist_asm
void option_multi_set_assist_asm(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.assistAsm = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00142310 option_multi_assist_asm
MultiButtonInfo option_multi_assist_asm(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_assist_asm, 1 - ctl->options.assistAsm);
}

// FUNC 0x00142338 option_multi_set_assist_tcs
void option_multi_set_assist_tcs(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.assistTcs = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00142348 option_multi_assist_tcs
MultiButtonInfo option_multi_assist_tcs(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_assist_tcs, 1 - ctl->options.assistTcs);
}

/* "multi_ghost_type": the ghost flag. */
// FUNC 0x00142370 option_multi_set_ghost_type
void option_multi_set_ghost_type(void *ctx, int index)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.ghost = (index != MULTI_CHOICE_OFF);
}

// FUNC 0x00142380 option_multi_ghost_type
MultiButtonInfo option_multi_ghost_type(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return MultiButtonInfo(option_multi_set_ghost_type, 1 - ctl->options.ghost);
}

/* The table, .data 0x002A1CD8: no initialiser, so egcs writes its zeros in
 * .data in front of the frame info (retail's are zero too: the static
 * initialiser below fills it at boot). */
MenuMultiFunc g_002A1CD8[21];               /* 0x002A1CD8 */

/* The table's entry count, .sdata 0x0035171C: a constant word (21), read
 * by OptionControl::begin (T062); defined here, before the initialiser. */
int g_0035171C = sizeof(g_002A1CD8) / sizeof(g_002A1CD8[0]);

/* One table entry: the inline constructor of the original's MenuMultiFunc
 * (see T066_static_init). */
static inline void set_multi(MenuMultiFunc *e, const char *name, MultiButtonInfo (*func)(void *ctx))
{
    e->name = name;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it (u006's form): the
 * constructor on a copy of the cursor, then the cursor and the count of
 * the elements left stepped. */
#define MULTI(name, func)                                                         { MenuMultiFunc *e = base; set_multi(e, name, func); }                        base++;                                                                       left--

/* Static initialiser of g_002A1CD8 (.ctors entry 0x0028F08C): a
 * compiler-generated _GLOBAL_$I$ function in the original, which defined
 * the table here with MenuMultiFunc objects built by an inline constructor
 * (built at run time by cp/init.c build_vec_init; retail's .data is zero).
 * That needs a constructor in T229's MenuMultiFunc, a plain struct other
 * units share; the table is defined above without one, so the function is
 * written out the way build_vec_init expands it (a walking cursor, a copy
 * of it per element, the dead count of the elements left): an exact form,
 * not the original; it stores the same 21 entries.
 * Entry i is at 0x002A1CD8 + 8 * i. */
// FUNC 0x001423A8 T066_static_init
void T066_static_init(void)
{
    MenuMultiFunc *base = g_002A1CD8;
    int left = 20;                          /* build_vec_init's iterator: elements - 1 */

    MULTI("multi_assist_asm",         option_multi_assist_asm);
    MULTI("multi_assist_tcs",         option_multi_assist_tcs);
    MULTI("multi_cfg_steering_1p",    option_multi_wide_mode);   /* sic */
    MULTI("multi_gear_type",          option_multi_gear_type);
    MULTI("multi_ghost_type",         option_multi_ghost_type);
    MULTI("multi_menu_bgm",           option_multi_menu_bgm);
    MULTI("multi_menu_se",            option_multi_menu_se);
    MULTI("multi_race_bgm",           option_multi_race_bgm);
    MULTI("multi_race_se",            option_multi_race_se);
    MULTI("multi_replay_bgm",         option_multi_replay_bgm);
    MULTI("multi_replay_se",          option_multi_replay_se);
    MULTI("multi_split_boost",        option_multi_split_boost);
    MULTI("multi_split_tire_damage",  option_multi_split_tire_damage);
    MULTI("multi_steering_assist_1p", option_multi_steering_assist_1p);
    MULTI("multi_steering_assist_2p", option_multi_steering_assist_2p);
    MULTI("multi_steering_force_1p",  option_multi_steering_force_1p);
    MULTI("multi_steering_force_2p",  option_multi_steering_force_2p);
    MULTI("multi_tire_damage",        option_multi_tire_damage);
    MULTI("multi_vibration_1p",       option_multi_vibration_1p);
    MULTI("multi_vibration_2p",       option_multi_vibration_2p);
    MULTI("multi_wide_mode",          option_multi_wide_mode);
}
