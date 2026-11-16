/* T012 - the "ar/push_start" page of the arcade menu.  0x001156F8..0x001157D0.
 *
 * Retail's .rodata (0x0030DC58..0x0030DD60): "ar/push_start", then the type
 * names SmartIndexT<int> and the operator classes (OperatorList ..
 * Operator): the menu's Transitive header first, T010_arcade_control.h
 * last; no GameStatus. */
#include "game/menu/framework/T216_menu_transitive.h"      /* (no use: the operator classes retail's T012 saw) */
#include "game/menu/arcade/T029_arcade_movies.h"        /* MovieParam g_00292930[8] */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se, MENU_SE_DECIDE */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "game/menu/arcade/T010_arcade_control.h"       /* last: ArcadeControl (SmartIndexT<int>) */
#include "game/menu/arcade/arcade_control_internal.h"

namespace gt2k_menu {

/* The attract screen (runAttract): background movie "stream/menu_bg.pss",
 * then frames until pad 1 decides (start or decide, runAttract's decide
 * buttons: returns 1 after the decide sound) or the page times out (pad 1's
 * cancel forces the time-out; returns 0).  No PageManager::update here.  The
 * argument is not used. */
// FUNC 0x001156F8 gt2k_menu::ArcadeControl::runPushStartPage
int ArcadeControl::runPushStartPage(int prev)
{
    g_002CDC10.start(&g_00292930[ARCADE_MOVIE_BG]);
    PageManager *pm = &pages;
    pm->setPage("ar/push_start", NULL);
    int pushed = 1;
    pm->startPage(NULL);
    transitionIn();
    while (control1.decide == 0) {
        if (control1.cancel != 0)
            timedOut = 1;
        waitFrame();
        if (timedOut != 0) {
            pushed = 0;
            goto out;                       /* no decide sound */
        }
    }
    pushed = 1;
    menu_play_se(MENU_SE_DECIDE);
out:
    transitionOut();
    pm->closeAndExit();
    return pushed;
}

} /* namespace gt2k_menu */
