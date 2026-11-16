/* T067 - the button actions of the options screen.  0x001425F8..0x00142790.
 *
 * T067_static_init fills g_002A2038 with eight ExecuteFuncData entries
 * sorted by name; OptionControl::begin (T062) gives them to its PageManager
 * (setExecuteFuncData, ctx the OptionControl).  PageManager::callExecuteFunc
 * calls an entry for a decided button whose exec name is the entry's name
 * (rec the button, index its place in the list, layout the current one);
 * only ctx is used here.  "commit" and "retrieve" do nothing: OptionControl
 * itself copies the edited options back to the game when the screen ends.
 *
 * The includes are in the order that gives retail's type names (.rodata
 * 0x0031B368, reverse definition order): Cell<Button>, OperatorList,
 * List / OperatorHolder / Cell<OperatorHolder>, SmartIndexT<int>, the garage
 * lists (wheel list .. Cell<GarageCar>), Operator: as T063, the garage
 * classes (T429), the page manager (T211's SmartIndexT<int>), the menu
 * classes (T216), the buttons (T206); T216 / T206 for those names only. */
#include "game/save/T429_game_status.h"          /* (the garage classes) */
#include "game/menu/options/T067_option_actions.h"       /* g_002A2038, g_00351720; T211 ExecuteFuncData */
#include "game/menu/options/T062_option_control.h"       /* OptionControl (ctx): options, calibPort / Wheel */
#include "game/menu/framework/T216_menu_transitive.h"      /* (OperatorHolder, OperatorList) */
#include "game/menu/framework/T206_button.h"               /* (Cell<Button>) */
#include "game/save/T426_controller_config.h"    /* CONFIG_WHEEL_GTFORCE, CONFIG_WHEEL_C294 */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "game/menu/options/options_internal.h"                           /* the unit's constants and callbacks */

/* .sdata opens with g_00351720, which only T062's code reads. */
// DATA_START 0x00351720

using namespace gt2k_menu;

/* Empty and unreferenced (no table entry, no call); its signature is
 * unknown (? a leftover beside "commit"). */
// FUNC 0x001425F8 F_001425F8
void F_001425F8(void)
{
}

/* "commit": nothing. */
// FUNC 0x00142600 option_action_commit
void option_action_commit(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
}

/* Empty and unreferenced, as F_001425F8 (? a leftover beside "retrieve"). */
// FUNC 0x00142608 F_00142608
void F_00142608(void)
{
}

/* "retrieve": nothing. */
// FUNC 0x00142610 option_action_retrieve
void option_action_retrieve(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
}

/* "set_default": the edited options back to their defaults, applied at
 * once (GameOptions::reset). */
// FUNC 0x00142618 option_action_set_default
void option_action_set_default(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.reset();
}

/* "set_port_1" .. "set_port_2_jaguar": the port and the wheel the USB wheel
 * pages are for (T064's key_config_steer_* and calib_*_bar items, the T063
 * checks): calibWheel is the KeyConfigItem mode of the wheel page. */
// FUNC 0x00142638 option_action_set_port_1
void option_action_set_port_1(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->calibPort  = CALIB_PORT_1;
    ctl->calibWheel = CONFIG_WHEEL_GTFORCE;
}

// FUNC 0x00142648 option_action_set_port_2
void option_action_set_port_2(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->calibPort  = CALIB_PORT_2;
    ctl->calibWheel = CONFIG_WHEEL_GTFORCE;
}

// FUNC 0x00142660 option_action_set_port_1_jaguar
void option_action_set_port_1_jaguar(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->calibPort  = CALIB_PORT_1;
    ctl->calibWheel = CONFIG_WHEEL_C294;
}

// FUNC 0x00142670 option_action_set_port_2_jaguar
void option_action_set_port_2_jaguar(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->calibPort  = CALIB_PORT_2;
    ctl->calibWheel = CONFIG_WHEEL_C294;
}

/* "stop_music" */
// FUNC 0x00142688 option_action_stop_music
void option_action_stop_music(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    bgm_stop();
}

/* The table, .data 0x002A2038: no initialiser, so egcs writes its zeros in
 * .data in front of the frame info (retail's are zero too: the static
 * initialiser below fills it at boot). */
ExecuteFuncData g_002A2038[8];              /* 0x002A2038 */

/* The table's entry count, .sdata 0x00351720: a constant word (8), read by
 * OptionControl::begin (T062).  Defined here, before the initialiser: its
 * place in .sdata, before the initialiser's "commit" (0x00351728), shows
 * it. */
int g_00351720 = sizeof(g_002A2038) / sizeof(g_002A2038[0]);

/* One table entry: the inline constructor of the original's
 * ExecuteFuncData (see T067_static_init). */
static inline void set_action(ExecuteFuncData *e, const char *name,
                              void (*func)(void *ctx, ButtonData *rec, int index,
                                           PageLayout *layout))
{
    e->name = name;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it (u006's form): the
 * constructor on a copy of the cursor, then the cursor and the count of
 * the elements left stepped. */
#define ACTION(name, func)                                                        { ExecuteFuncData *e = base; set_action(e, name, func); }                     base++;                                                                       left--

/* Static initialiser of g_002A2038 (.ctors entry 0x0028F090): a
 * compiler-generated _GLOBAL_$I$ function in the original, which defined
 * the table here with ExecuteFuncData objects built by an inline
 * constructor (built at run time by cp/init.c build_vec_init; retail's .data
 * is zero).  That needs a constructor in T211's ExecuteFuncData, a plain
 * struct other units share; the table is defined above without one, so the
 * function is written out the way build_vec_init expands it (a walking
 * cursor, a copy of it per element, the dead count of the elements left):
 * an exact form, not the original; it stores the same eight entries. */
// FUNC 0x001426A8 T067_static_init
void T067_static_init(void)
{
    ExecuteFuncData *base = g_002A2038;
    int left = 7;                           /* build_vec_init's iterator: elements - 1 */

    ACTION("commit",            option_action_commit);
    ACTION("retrieve",          option_action_retrieve);
    ACTION("set_default",       option_action_set_default);
    ACTION("set_port_1",        option_action_set_port_1);
    ACTION("set_port_1_jaguar", option_action_set_port_1_jaguar);
    ACTION("set_port_2",        option_action_set_port_2);
    ACTION("set_port_2_jaguar", option_action_set_port_2_jaguar);
    ACTION("stop_music",        option_action_stop_music);
}
