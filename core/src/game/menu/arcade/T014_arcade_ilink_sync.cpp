/* T014 - the i.LINK selection exchange of the arcade menu.
 * 0x00115E10..0x00115F70.
 *
 * Retail's .rodata (0x0030DDE8..0x0030DE30): "ar/wait_other", then the type
 * names pdi1394 and SmartIndexT<int> only (T013's view without the button
 * record); no .sdata. */
#include "game/menu/arcade/T029_arcade_movies.h"        /* MovieParam g_00292930[8] */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "engine/system/T473_link_session.h"         /* F_0022D3B8, F_0022D3C0, g_00352CB0 */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl (SmartIndexT<int>) */
#include "engine/pdi/T541_pdi1394.h"              /* last: pdi1394, PDI1394_GROUP_ACTIVE */
#include "game/menu/arcade/arcade_control_internal.h"

namespace gt2k_menu {

/* "ar/wait_other" over the background movie "stream/menu_bg2.pss": waits
 * for barrier PDI1394_GROUP_ACTIVE of every node (or the page exited / timed
 * out, which gives 0), then sends player 1's car, class, colour, racing and
 * transmission choices with the course (0xFF, "take a node's", in live mode)
 * through F_0022D3C0, which may replace the course with the agreed one; 10
 * more frames, then the course index is set to it.  The argument is not
 * used. */
// FUNC 0x00115E10 gt2k_menu::ArcadeControl::runWaitSyncPage
int ArcadeControl::runWaitSyncPage(int prev)
{
    int ok = 0;

    g_002CDC10.start(&g_00292930[ARCADE_MOVIE_BG2]);
    pages.setPage("ar/wait_other", NULL);
    pages.startPage(NULL);
    transitionIn();
    pdi1394 *link = F_0022D3B8();
    /* isExit (inline) puts &pages in a register of its own at the loop
     * head, which update then shares: gcse makes it a copy of the first
     * &pages, retail's second register for the update call. */
    while (!pages.isExit()) {
        if (link->pollBarrier(PDI1394_GROUP_ACTIVE) != 0) {
            ok = 1;
            break;
        }
        pages.update();
        waitFrame();
        if (timedOut != 0)
            break;
    }
    transitionOut();
    pages.closeAndExit();

    int courseSel = (g_00352CB0 != 0) ? 0xFF : course.value;   /* sp+0x00 */
    if (ok != 0) {
        if (F_0022D3C0(car1.value, class1.value, color1.value, racing1.value,
                       transmission1.value, &courseSel) == 0)
            ok = 0;
    }
    for (int frames = 9; frames >= 0; frames--)
        waitFrame();
    course.set(courseSel);
    return ok;
}

} /* namespace gt2k_menu */
