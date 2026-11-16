/* T013 - the i.LINK pages of the arcade menu.  0x001157D0..0x00115E10.
 *
 * Both talk to the pdi1394 driver object (F_0022D3B8, T473).  g_00350FCC is
 * the "live" (1) / "play" (0) choice made on "ar/connect", g_00353200 this
 * console's slot (its link slot % 3), g_00352CB0 the session's copy of the
 * live flag, g_00352CA8 the session's view mode of this console (0: it
 * drives a car).
 *
 * Retail's .rodata (0x0030DD60..0x0030DDE8): the literals, the jump table of
 * link_view_mode, then the type names pdi1394 and SmartIndexT<int> only:
 * the ButtonData record without ButtonList (no List<Button>), and pdi1394
 * (its inline destructor: T541_pdi1394.h) defined after
 * T010_arcade_control.h.  Its .sdata (0x00350FC8..0x00350FD0) is
 * g_00350FC8 / g_00350FCC (below), its .sbss g_00353200. */
#include "game/menu/framework/T244_button_data.h"          /* ButtonData (the select buffer of "ar/connect") */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "engine/system/T473_link_session.h"         /* F_0022D360 .. F_0022D850, g_00352CA8 / B0 / DC */
#include "game/common/T494_text_utils.h"           /* str_equal */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "engine/pdi/G003_pdi1394_rpc.h"          /* Pdi1394Status, pdi1394_get_status, F_0026EB90 */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl (SmartIndexT<int>) */
#include "engine/pdi/T541_pdi1394.h"              /* last: pdi1394, PDI1394_* */
#include "game/menu/arcade/arcade_control_internal.h"

/* The "ar/connect" countdown and the live / play choice (declared in
 * T010_arcade_control.h: T010, T028 and T030 use them too).  Retail has
 * them in T013's .sdata, after T010's: only T013 references g_00350FC8 of
 * this unit's code, and the code's references put both in T013's range. */
int g_00350FC8 = 0;                         /* 0x00350FC8 */
int g_00350FCC = 0;                         /* 0x00350FCC */

/* This console's slot on "ar/connect" (pdi1394::fromSlot % 3 of the first
 * byte received), -1 until then.  Only T013 uses it: a file static (.sbss,
 * -G8). */
static int g_00353200;                      /* 0x00353200 gp-0x47F0 */

namespace gt2k_menu {

/* "ar/connect": waits for the other consoles.  The focused button (its exec
 * name, copied into a select buffer by PageManager::update) picks play /
 * live.  Every frame the session key goes out; decide is accepted only once
 * every node on the bus (Pdi1394Status::nodeCount) has answered it (pad 1's
 * decide buttons are 0 until then, CONTROL_DECIDE from then on and always on
 * exit).  Once a byte came in from the link (this console then takes its
 * slot), a 540-frame countdown (g_00350FC8) runs; at its end the result is
 * whether every node's key matched (checkKeys).  Exiting the page (a 0 byte
 * goes out), the link going away or a 0 byte received give 0, which also
 * resets the link (F_0026EB90).  The argument is not used. */
// FUNC 0x001157D0 gt2k_menu::ArcadeControl::runConnectPage
int ArcadeControl::runConnectPage(int prev)
{
    ButtonData focused;                     /* sp+0x000 the select buffer */
    Pdi1394Status status;                   /* sp+0x150 (a 0x50-byte slot) */
    u8 byte;                                /* sp+0x1A0 */
    int ok = 0;
    int received = 0;

    waitFrame();
    g_002CDC10.suspend(1);
    g_00350FC8 = ARCADE_CONNECT_COUNTDOWN;
    g_00353200 = -1;
    g_00350FCC = 0;
    pages.setPage("ar/connect", NULL);
    pages.startPage(NULL);
    transitionIn();
    pages.setSelectBuffer(&focused);
    pdi1394 *link = F_0022D3B8();
    F_0022D360();
    link->init();
    link->setGroup(PDI1394_STATE_NONE);     /* the 8 IOP nodes directly */
    link->ackKey();
    control1.decideButtons = 0;

    do {
        if (str_equal(focused.exec, "i_link_play"))
            g_00350FCC = 0;
        else if (str_equal(focused.exec, "i_link_live"))
            g_00350FCC = 1;
        link->sendKey();
        pdi1394_get_status(&status);
        if (link->pollKey() == status.nodeCount)
            control1.decideButtons = CONTROL_DECIDE;
        g_00352CB0 = g_00350FCC;
        if (link->updateTopology() == 0)
            goto out;                       /* the link went away */
        /* isExit (inline) puts &pages in a register of its own, which
         * update shares: gcse makes it a copy of the first &pages, retail's
         * $fp (as runWaitSyncPage's) */
        if (pages.isExit()) {
            link->sendByte(0);
            goto out;
        }
        pages.update();
        waitFrame();
        if (link->recvByte(&byte) != 0) {
            if (byte == 0)
                goto out;
            if (g_00353200 == -1)
                g_00353200 = link->fromSlot % 3;
            received = 1;
        }
        if (received != 0)
            g_00350FC8 = g_00350FC8 - 1;
    } while (g_00350FC8 > 0);
    /* the countdown ran out: every node's key must match */
    ok = (link->checkKeys() != 0);

out:
    control1.decideButtons = CONTROL_DECIDE;
    pages.setSelectBuffer(NULL);
    transitionOut();
    pages.closeAndExit();
    g_002CDC10.suspend(0);
    if (ok == 0)
        F_0026EB90();
    g_00352CDC = 0;
    timedOut = 0;
    return ok;
}

static void link_view_mode(int *viewMode, int nodes, int slot);   /* below */

/* "ar/wait_other": waits (page frozen) for barrier 0 of every node, then
 * acknowledges for 10 frames, announces play / live (PDI1394_STATE_PLAY /
 * _LIVE), waits up to 10 tries for every node's announcement, and checks
 * that the linked nodes are all playing or live (the counts of group 0 and
 * PDI1394_GROUP_ACTIVE agree).  With 1..6 play nodes (F_0022D850) that is
 * enough (g_00352CA8 = 0: this console drives); with none the view mode of
 * this console's slot (link_view_mode) is set with the nodes' link modes
 * (F_00115C80).  Failure resets the link.  The argument is not used. */
// FUNC 0x00115A60 gt2k_menu::ArcadeControl::runWaitOtherPage
int ArcadeControl::runWaitOtherPage(int prev)
{
    int ok = 0;
    PageManager *pm = &pages;
    pdi1394 *link = F_0022D3B8();
    int tries;

    pm->setPage("ar/wait_other", NULL);
    pm->startPage(NULL);
    pm->setFreeze(1);
    transitionIn();
    while (link->pollBarrier(0) == 0) {
        pm->update();
        waitFrame();
        if (timedOut != 0)
            goto out;
    }
    if (link->barrier(0) == 0) {
        link->ackByte();
        goto out;
    }

    tries = 10;
    do {
        tries--;
        link->ackByte();
        link->ackKey();
        waitFrame();
    } while (tries > 0);

    link->sendState(g_00350FCC != 0 ? PDI1394_STATE_LIVE : PDI1394_STATE_PLAY);
    tries = 10;
    while (tries > 0) {
        if (link->waitPackets(0, PDI1394_WAIT_ALL | PDI1394_WAIT_TIMEOUT) != 0)
            break;
        tries--;
    }
    if (tries == 0)
        goto out;

    link->collectStates();
    {
        int playing = F_0022D850();
        int linked = link->getGroupCount(PDI1394_STATE_LINKED);
        int active = link->getGroupCount(PDI1394_GROUP_ACTIVE);
        if (active == linked) {
            if (playing == 0) {
                /* Quirk: link_view_mode leaves viewMode unset for 6 nodes
                 * and a slot other than 0..2: the original then tests stale
                 * stack. */
                int viewMode;
                link_view_mode(&viewMode, active, g_00353200);
                if (viewMode != 0) {
                    F_00115C80(viewMode);
                    ok = 1;
                }
            } else if (playing < 7) {
                g_00352CA8 = 0;
                ok = 1;
            }
        }
    }
    if (ok == 0)
        F_0026EB90();

out:
    transitionOut();
    pm->setFreeze(0);
    pm->closeAndExit();
    gfx_vsync(2);
    return ok;
}

/* The link modes of nodes 0 and 1 for view mode `sel' (node 0 plays; node 1
 * plays for view mode 5, else is live), then g_00352CA8 = sel. */
// FUNC 0x00115C80 gt2k_menu::ArcadeControl::F_00115C80
void ArcadeControl::F_00115C80(int sel)
{
    setLinkMode(0, PDI1394_STATE_PLAY);
    if (sel == 5)
        setLinkMode(1, PDI1394_STATE_PLAY);
    else
        setLinkMode(1, PDI1394_STATE_LIVE);
    g_00352CA8 = sel;
}

/* Sets a node's link mode (pdi1394::setSlotGroup); for this console's own
 * slot also the live flags (mode 2 = live).  `this' is not used. */
// FUNC 0x00115CE8 gt2k_menu::ArcadeControl::setLinkMode
void ArcadeControl::setLinkMode(int node, int linkMode)
{
    if (node == F_0022D830()) {
        int live = (linkMode == PDI1394_STATE_LIVE);
        g_00350FCC = live;
        g_00352CB0 = live;
    }
    F_0022D3B8()->setSlotGroup(node, linkMode);
}

/* The view mode of this console in a live session of `nodes' consoles (the
 * PDI1394_GROUP_ACTIVE count), by its slot (0 = none; 1 .. 9 the views of
 * g_00352CA8, T473).  With 6 nodes and a slot other than 0..2 *viewMode is
 * left as it was.  A file-local helper (one caller; `this' is not passed).
 * Jump table 0x0030DDA0 on nodes - 2 (5 entries). */
// FUNC 0x00115D48 link_view_mode
static void link_view_mode(int *viewMode, int nodes, int slot)
{
    switch (nodes) {
    default:
        *viewMode = 0;
        return;
    case 2:
        if (slot != 0)
            *viewMode = 1;
        else
            *viewMode = 3;
        return;
    case 3:
        *viewMode = 4;
        return;
    case 4:
        *viewMode = 7;
        return;
    case 5:
        *viewMode = 8;
        return;
    case 6:
        switch (slot) {
        case 0:
            *viewMode = 9;
            return;
        case 1:
            *viewMode = 5;
            return;
        case 2:
            *viewMode = slot;               /* 2 (stores the slot register) */
            return;
        }
        return;
    }
}

} /* namespace gt2k_menu */
