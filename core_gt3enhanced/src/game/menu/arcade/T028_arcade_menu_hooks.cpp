/* T028 - the ExecuteFuncData hooks of the arcade menu.  0x0010FB40..0x0010FE58.
 *
 * g_002927B8 maps the exec names of the arcade pages' buttons (sorted, for
 * the PageManager's binary search) to their functions; ArcadeControl::setup
 * (T010) gives it to its PageManager with the ArcadeControl as ctx.  The
 * PageManager calls func(ctx, rec, index, layout) for a decided button whose
 * ButtonData::exec matches; the hooks read only ctx and rec.
 *
 * Includes: retail's .rodata (0x0030BD38) has the literals, then the type
 * names pdi1394, SmartIndexT<int> and the garage lists (GarageWheelList ..
 * Cell<GarageCar>), nothing of the menu items (no OperatorHolder, no
 * Operator): the GameStatus first, ArcadeControl (SmartIndexT<int>), the
 * i.LINK class (its type name: T541_pdi1394.h's inline destructor), and of
 * T037 only its actions' prototypes. */
#include "game/save/T429_game_status.h"          /* getGameStatus; T427 MenuStatus (nextMode) */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl::setModeByName, g_00350FC8; SmartIndexT<int> */
#include "game/menu/arcade/T028_arcade_menu_hooks.h"    /* the hooks, g_002927B8; T211 ExecuteFuncData */
#include "engine/pdi/T541_pdi1394.h"              /* pdi1394::sendByte */
#include "engine/system/T473_link_session.h"         /* F_0022D3B8: the pdi1394 driver */
#include "game/menu/framework/T244_button_data.h"          /* ButtonData::exec */
#include "game/common/T494_text_utils.h"           /* str_equal */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_0035274C / g_00352750 (cancel_select_game) */
#include "game/menu/gtmode/T037_gtmode_action_funcs.h"  /* action_set_garage_car ("set_garage_car") */
#include "game/menu/arcade/arcade_course_select_internal.h"

/* The ExecuteFuncData table (T028's .data, before its frame info), filled
 * by T028_static_init: zeros in the image (egcs writes a C++ global without
 * an initialiser as zeros in .data). */
gt2k_menu::ExecuteFuncData g_002927B8[15];  /* 0x002927B8 (13 in GT3; the enhanced engine adds
                                               "pod_race", "pod_race_2p") */

using gt2k_menu::ArcadeControl;
using gt2k_menu::ButtonData;
using gt2k_menu::PageLayout;

/* "compulsory", "goodies", "i_link_battle", "load_guest_garage",
 * "single_race", "time_attack", "two_player_battle": the mode of the button's
 * exec name. */
// FUNC 0x0010FB40 select_game_hook
void select_game_hook(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    static_cast<ArcadeControl *>(ctx)->setModeByName(rec->exec);
}

namespace gt2k_menu {

/* ArcadeControl::mode from the exec name (str_equal); an unknown name
 * leaves it unchanged.  The tests in a do { ... break; } while (0): its loop
 * notes let loop.c hoist the strings' lui into s-registers and stmt.c moves
 * the first tests to the bottom (the entry `b'), as retail; a plain
 * if / else chain has neither. */
// FUNC 0x0010FB60 gt2k_menu::ArcadeControl::setModeByName
void ArcadeControl::setModeByName(const char *name)
{
    do {
        if (str_equal(name, "single_race") != 0) {
            mode = ARCADE_SINGLE_RACE;
            break;
        }
        if (str_equal(name, "compulsory") != 0) {
            mode = ARCADE_COMPULSORY;
            break;
        }
        if (str_equal(name, "time_attack") != 0) {
            mode = ARCADE_TIME_ATTACK;
            break;
        }
        if (str_equal(name, "two_player_battle") != 0) {
            mode = ARCADE_TWO_PLAYER;
            break;
        }
        if (str_equal(name, "goodies") != 0) {
            mode = ARCADE_GOODIES;
            break;
        }
        if (str_equal(name, "load_guest_garage") != 0) {
            mode = ARCADE_LOAD_GUEST_GARAGE;
            break;
        }
        if (str_equal(name, "i_link_battle") != 0) {
            mode = ARCADE_I_LINK;
            break;
        }
        /* The enhanced engine (Lupo 2's setPodRace, L2 0x002B2640; menus_a.md
         * 14): the Pod race for one or two players.  Only a layout with
         * these buttons offers it; without the Pod's data the mode is
         * refused (ArcadeControl::mainLoop, podRaceAvailable). */
        if (str_equal(name, "pod_race") != 0) {
            mode = ARCADE_POD_RACE;
            podPlayers = 1;
            break;
        }
        if (str_equal(name, "pod_race_2p") != 0) {
            mode = ARCADE_POD_RACE;
            podPlayers = 2;
            break;
        }
    } while (0);
}

} /* namespace gt2k_menu */

/* "i_link_live", "i_link_play": the "ar/connect" countdown g_00350FC8 (T010)
 * cleared, then the byte 1 to the link's current group (pdi1394::sendByte). */
// FUNC 0x0010FC78 i_link_start_hook
void i_link_start_hook(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_00350FC8 = 0;
    F_0022D3B8()->sendByte(1);
}

/* "cancel_select_game": no screen clear before the next frames
 * (RenderBase::beginRender's flag g_00352750 and its colour g_0035274C,
 * T261, which the page manager's fade timer sets). */
// FUNC 0x0010FCA8 cancel_select_game_hook
void cancel_select_game_hook(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_00352750 = 0;
    g_0035274C = 0;
}

/* "go_ending_easy" / "go_ending_normal": main() runs mode 14 / 15 next (the
 * ending movies, T015 arcade_menu). */
// FUNC 0x0010FCB8 go_ending_easy_hook
void go_ending_easy_hook(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = 14;
}

// FUNC 0x0010FCE8 go_ending_normal_hook
void go_ending_normal_hook(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = 15;
}

/* Static initialiser (.ctors entry 0x0028F03C).  In the original a
 * compiler-generated _GLOBAL_$I$ function: the table defined here with
 * ExecuteFuncData objects built by a constructor, which egcs fills at run
 * time through a walking cursor (cp/init.c build_vec_init; see u006's
 * T030_static_init).  The decomp keeps the table at its original address,
 * so the walk is written out: an exact form, not the original (it stores
 * the same 13 entries). */
// FUNC 0x0010FD18 T028_static_init
void T028_static_init(void)
{
    gt2k_menu::ExecuteFuncData *p = g_002927B8;

    p->name = "cancel_select_game";
    p->func = cancel_select_game_hook;
    p++;
    p->name = "compulsory";
    p->func = select_game_hook;
    p++;
    p->name = "go_ending_easy";
    p->func = go_ending_easy_hook;
    p++;
    p->name = "go_ending_normal";
    p->func = go_ending_normal_hook;
    p++;
    p->name = "goodies";
    p->func = select_game_hook;
    p++;
    p->name = "i_link_battle";
    p->func = select_game_hook;
    p++;
    p->name = "i_link_live";
    p->func = i_link_start_hook;
    p++;
    p->name = "i_link_play";
    p->func = i_link_start_hook;
    p++;
    p->name = "load_guest_garage";
    p->func = select_game_hook;
    p++;
    p->name = "pod_race";                   /* the enhanced engine: the Pod race (menus_a.md 14) */
    p->func = select_game_hook;
    p++;
    p->name = "pod_race_2p";
    p->func = select_game_hook;
    p++;
    p->name = "set_garage_car";
    p->func = action_set_garage_car;
    p++;
    p->name = "single_race";
    p->func = select_game_hook;
    p++;
    p->name = "time_attack";
    p->func = select_game_hook;
    p++;
    p->name = "two_player_battle";
    p->func = select_game_hook;
}

/* The table's entry count (.sdata 0x00350EA0, after setModeByName's
 * "goodies"; read by ArcadeControl::setup, T010). */
const int g_00350EA0 = sizeof(g_002927B8) / sizeof(g_002927B8[0]);
