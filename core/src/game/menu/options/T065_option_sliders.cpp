/* T065 - the slider functions of the options screen.
 * 0x00141E18..0x00141F70.
 *
 * T065_static_init fills g_002A1C38 with three MenuSliderFuncs sorted by
 * name; OptionControl::begin (T062) gives the table to both layouts of its
 * PageManager (setSliderFuncs, ctx the OptionControl).  ButtonList::
 * createSlider (T244) calls func(ctx) for an element named "slider..." and
 * builds a Slider (T238) on the SliderDesc it returns (by value: the
 * caller's buffer is the hidden first argument and comes back in $v0).  The
 * ranges are [min, max): laps 1..99, handicap -150..150 metres by 5; the
 * set function stores the value the player picks in the edited GameOptions
 * and accepts it.  The T064 hooks of the same names draw the same ranges
 * (option_range_*).
 *
 * The includes are in the order that gives retail's type names (.rodata
 * 0x0031AD08, reverse definition order): SmartIndexT<int>, Cell<Button>,
 * OperatorList, List / OperatorHolder / Cell<OperatorHolder>, the garage
 * lists (wheel list .. Cell<GarageCar>), Operator.  So the original defined
 * the garage classes first (T429), then the slider (T238: the menu classes,
 * the buttons, its SmartIndexDoubleRange's SmartIndexT<int>). */
#include "game/save/T429_game_status.h"          /* (the garage classes) */
#include "game/menu/options/T065_option_sliders.h"       /* g_002A1C38, g_00351718; T238 SliderDesc, T229 */
#include "game/menu/options/T062_option_control.h"       /* OptionControl (ctx): options */
#include "game/menu/options/options_internal.h"                           /* the unit's constants and callbacks */

/* .sdata opens with g_00351718, which only T062's code reads. */
// DATA_START 0x00351718

using namespace gt2k_menu;

/* The setters (SliderSetFn): commit is not used (mode 1 never refuses). */
// FUNC 0x00141E18 option_slider_set_race_laps
int option_slider_set_race_laps(void *ctx, int value, int commit)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.raceLaps = value;
    return 1;
}

/* "slider_race_laps": [1, 100) by 1. */
// FUNC 0x00141E28 option_slider_race_laps
SliderDesc option_slider_race_laps(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return SliderDesc(1, 100, ctl->options.raceLaps, 1,
                      option_slider_set_race_laps, SLIDER_MODE_STEP, 0, 0);
}

// FUNC 0x00141E68 option_slider_set_split_race_laps
int option_slider_set_split_race_laps(void *ctx, int value, int commit)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.splitBattle.raceLaps = value;
    return 1;
}

/* "slider_split_race_laps": [1, 100) by 1. */
// FUNC 0x00141E78 option_slider_split_race_laps
SliderDesc option_slider_split_race_laps(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return SliderDesc(1, 100, ctl->options.splitBattle.raceLaps, 1,
                      option_slider_set_split_race_laps, SLIDER_MODE_STEP, 0, 0);
}

// FUNC 0x00141EB8 option_slider_set_split_handicap
int option_slider_set_split_handicap(void *ctx, int value, int commit)
{
    OptionControl *ctl = (OptionControl *)ctx;
    ctl->options.splitBattle.handicap = value;
    return 1;
}

/* "slider_split_handicap": [-150, 151) by 5, i.e. -150..150 metres. */
// FUNC 0x00141EC8 option_slider_split_handicap
SliderDesc option_slider_split_handicap(void *ctx)
{
    OptionControl *ctl = (OptionControl *)ctx;
    return SliderDesc(-150, 151, ctl->options.splitBattle.handicap, 5,
                      option_slider_set_split_handicap, SLIDER_MODE_STEP, 0, 0);
}

/* The table, .data 0x002A1C38: no initialiser, so egcs writes its zeros in
 * .data in front of the frame info (retail's are zero too: the static
 * initialiser below fills it at boot). */
MenuSliderFunc g_002A1C38[3];               /* 0x002A1C38 */

/* The table's entry count, .sdata 0x00351718: a constant word (3), read by
 * OptionControl::begin (T062); defined here, before the initialiser. */
int g_00351718 = sizeof(g_002A1C38) / sizeof(g_002A1C38[0]);

/* One table entry: the inline constructor of the original's MenuSliderFunc
 * (see T065_static_init). */
static inline void set_slider(MenuSliderFunc *e, const char *name, SliderDesc (*func)(void *ctx))
{
    e->name = name;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it (u006's form): the
 * constructor on a copy of the cursor, then the cursor and the count of
 * the elements left stepped. */
#define SLIDER(name, func)                                                        { MenuSliderFunc *e = base; set_slider(e, name, func); }                      base++;                                                                       left--

/* Static initialiser of g_002A1C38 (.ctors entry 0x0028F088): a
 * compiler-generated _GLOBAL_$I$ function in the original, which defined
 * the table here with MenuSliderFunc objects built by an inline constructor
 * (built at run time by cp/init.c build_vec_init; retail's .data is zero).
 * That needs a constructor in T229's MenuSliderFunc, a plain struct other
 * units share; the table is defined above without one, so the function is
 * written out the way build_vec_init expands it (a walking cursor, a copy
 * of it per element, the dead count of the elements left): an exact form,
 * not the original; it stores the same three entries. */
// FUNC 0x00141F10 T065_static_init
void T065_static_init(void)
{
    MenuSliderFunc *base = g_002A1C38;
    int left = 2;                           /* build_vec_init's iterator: elements - 1 */

    SLIDER("slider_race_laps",       option_slider_race_laps);
    SLIDER("slider_split_handicap",  option_slider_split_handicap);
    SLIDER("slider_split_race_laps", option_slider_split_race_laps);
}
