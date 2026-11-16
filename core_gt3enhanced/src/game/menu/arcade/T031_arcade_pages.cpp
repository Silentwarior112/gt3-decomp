/* T031 - three pages of the arcade menu.  0x00113540..0x00113A38.
 *
 * Each runs one page of menu/arcade.mbl the same way: background movie, set
 * and start the page, transitionIn, then one PageManager::update plus one
 * waitFrame until the page is exited; transitionOut, closeAndExit.  The int
 * argument is the previous page's result and is not used.
 *
 * Retail's .rodata (0x0030D570..0x0030D7F0): the history names, "ar/level",
 * "ar/goodies", then the type names SmartIndexT<int> and the GameStatus's
 * garage lists (nothing else): the GameStatus first, T010_arcade_control.h
 * last.  "ar/top" is .sdata (0x00350F68, -G8). */
#include "game/save/T429_game_status.h"          /* first: the garage lists; getGameStatus, MenuStatus */
#include "game/app/T002_render_thread.h"        /* render_set_object */
#include "game/menu/arcade/T029_arcade_movies.h"        /* MovieParam g_00292930[8] */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout::findHiddenItem, MenuLayoutItem */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_00352748 */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync, gs_get_field_mode */
#include "game/menu/arcade/T010_arcade_control.h"       /* last: ArcadeControl (SmartIndexT<int>) */
#include "game/menu/arcade/arcade_control_internal.h"
#include "game/enhanced.h"                  /* ENH_MENU_REMEMBER_CURSOR */

#if ENH_MENU_REMEMBER_CURSOR
/* The enhanced engine (Lupo 2's top menu, L2 0x00108AB8 / 0x00108BF4..
 * 0x00108C38; menus_a.md 15): the cursor of "ar/top" when it was last
 * decided, -1 before.  Lupo 2 keeps its top menu's page and cursor in its
 * MenuStatus (+0x35F54 / +0x35F50), memory only; GT3's save layout does not
 * change, so a file static here (the arcade menu is built anew after every
 * race, this survives it; a reset or a new boot forgets it). */
static int s_topCursor = -1;
#endif

namespace gt2k_menu {

/* "ar/top": the mode selection (its buttons' ExecuteFuncData set the mode,
 * T028).  Looks the 17 "history_*" banners of the page up once (cached in
 * historyLevel / historyArea / historyClass: T020's showHistory* slide them
 * in on the later pages).  Cancelling it (not decided) leaves without the
 * transition: it stops all menu drawing (g_00352748 = 1) and takes the
 * object off the render thread. */
// FUNC 0x00113540 gt2k_menu::ArcadeControl::runTopPage
int ArcadeControl::runTopPage(int prev)
{
    PageManager *pm = &pages;

    g_002CDC10.start(&g_00292930[ARCADE_MOVIE_TOP]);
    pm->setPage("ar/top", NULL);
    pm->startPage(NULL);
#if ENH_MENU_REMEMBER_CURSOR
    if (s_topCursor >= 0)                   /* Lupo 2: back on the mode chosen last */
        pm->setCursor(s_topCursor);
#endif
    transitionIn();

    PageLayout *layout = pm->current;
    if (historyLevel[0] == NULL)
        historyLevel[0] = layout->findHiddenItem("history_level_easy");
    if (historyLevel[1] == NULL)
        historyLevel[1] = layout->findHiddenItem("history_level_normal");
    if (historyLevel[2] == NULL)
        historyLevel[2] = layout->findHiddenItem("history_level_hard");
    if (historyLevel[3] == NULL)
        historyLevel[3] = layout->findHiddenItem("history_level_pro");
    if (historyArea[0] == NULL)
        historyArea[0] = layout->findHiddenItem("history_area_a");
    if (historyArea[1] == NULL)
        historyArea[1] = layout->findHiddenItem("history_area_b");
    if (historyArea[2] == NULL)
        historyArea[2] = layout->findHiddenItem("history_area_c");
    if (historyArea[3] == NULL)
        historyArea[3] = layout->findHiddenItem("history_area_d");
    if (historyArea[4] == NULL)
        historyArea[4] = layout->findHiddenItem("history_area_e");
    if (historyArea[5] == NULL)
        historyArea[5] = layout->findHiddenItem("history_area_f");
    if (historyClass[ARCADE_CLASS_C] == NULL)
        historyClass[ARCADE_CLASS_C] = layout->findHiddenItem("history_class_c");
    if (historyClass[ARCADE_CLASS_B] == NULL)
        historyClass[ARCADE_CLASS_B] = layout->findHiddenItem("history_class_b");
    if (historyClass[ARCADE_CLASS_A] == NULL)
        historyClass[ARCADE_CLASS_A] = layout->findHiddenItem("history_class_a");
    if (historyClass[ARCADE_CLASS_S] == NULL)
        historyClass[ARCADE_CLASS_S] = layout->findHiddenItem("history_class_s");
    if (historyClass[ARCADE_CLASS_DIRT] == NULL)
        historyClass[ARCADE_CLASS_DIRT] = layout->findHiddenItem("history_class_dirt");
    if (historyClass[ARCADE_CLASS_HOME_GARAGE] == NULL)
        historyClass[ARCADE_CLASS_HOME_GARAGE] = layout->findHiddenItem("history_class_home_garage");
    if (historyClass[ARCADE_CLASS_GUEST_GARAGE] == NULL)
        historyClass[ARCADE_CLASS_GUEST_GARAGE] = layout->findHiddenItem("history_class_guest_garage");

    /* A second pointer to the page manager for the loop and the result:
     * retail keeps it in its own register ($s3, a copy of $s2 made by GCSE
     * at the start); one pointer for everything gives $s2 throughout. */
    PageManager *loop = &pages;
    while (pages.exited == 0) {
        loop->update();
        waitFrame();
    }

    int decided = loop->decided;
    if (decided != 0) {
        transitionOut();
        pm->closeAndExit();
#if ENH_MENU_REMEMBER_CURSOR
        s_topCursor = pm->getLastCursor();  /* Lupo 2: the closed list's cursor */
#endif
    } else {
        /* not decided (cancelled): on the even field one vsync more */
        if (gs_get_field_mode() == PGL_FIELD_HALF_EVEN)
            gfx_vsync(1);
        g_00352748 = 1;                     /* RenderBase::render draws nothing (T261) */
        pm->closeAndExit();
        gfx_vsync(2);
        render_set_object(NULL);
    }
    /* one return after the join: retail returns decided ($s1) on both
     * paths, where a return inside the else knows it is 0 */
    return decided;
}

/* "ar/level": the cursor (the level: easy, normal, hard, professional) is
 * restored from / saved to the GameStatus menu state (arcadeLevel).
 * Returns whether the page was decided. */
// FUNC 0x00113870 gt2k_menu::ArcadeControl::runLevelPage
int ArcadeControl::runLevelPage(int prev)
{
    PageManager *pm = &pages;

    g_002CDC10.start(&g_00292930[ARCADE_MOVIE_LEVEL]);
    pm->setPage("ar/level", NULL);
    pm->startPage(NULL);
    pm->setCursor(getGameStatus()->getMenuStatus()->arcadeLevel);   /* retail keeps the MenuStatus address */
    transitionIn();
    while (pages.exited == 0) {
        pm->update();
        waitFrame();
    }
    transitionOut();
    pm->closeAndExit();
    gfx_vsync(2);
    gt2k_game::MenuStatus *menu = &getGameStatus()->menu;
    menu->arcadeLevel = pm->getLastCursor();
    return pm->decided;
}

/* "ar/goodies": runs with the background movie suspended. */
// FUNC 0x00113968 gt2k_menu::ArcadeControl::runGoodiesPage
int ArcadeControl::runGoodiesPage(int prev)
{
    g_002CDC10.suspend(1);
    PageManager *pm = &pages;
    pm->setPage("ar/goodies", NULL);
    pm->startPage(NULL);
    transitionIn();
    while (pages.exited == 0) {
        pm->update();
        waitFrame();
    }
    transitionOut();
    pm->closeAndExit();
    gfx_vsync(2);
    int decided = pm->decided;
    g_002CDC10.suspend(0);
    return decided;
}

} /* namespace gt2k_menu */
