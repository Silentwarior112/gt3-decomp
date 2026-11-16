/* T034_connect_status.h - gt2k_menu::ConnectStatus, the i.LINK connection
 * panel (translation unit T034, unit u002), 0x001036D0..0x00103B70.
 *
 * Made by the item factory arcade_create_connect_status (T030) for the
 * layout object "connect_status": eight slot pictures "entry_no_1" ..
 * "entry_no_8", the "entry" picture drawn over the slot of every console
 * that has entered (its key message is queued: pdi1394::pollKey, T541), and
 * the "message" shown while some of the consoles on the bus
 * (Pdi1394Status::nodeCount, pdi1394_get_status, G003) have not.  It
 * updates and draws only its children, not its own picture. */
#ifndef GAME_T034_CONNECT_STATUS_H
#define GAME_T034_CONNECT_STATUS_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

namespace gt2k_menu {
class PageLayout;                           /* T229 (pointers only) */
struct MenuLayoutItem;                      /* T229 */

/* vtable 0x00309508 (8 slots: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [6] / [7] Transitive's).  typeinfo
 * __tfQ29gt2k_menu13ConnectStatus 0x00103B20 (si, base Item).  Size 0x128
 * (operator new in arcade_create_connect_status). */
class ConnectStatus : public Item {
public:
    /* Item(res, layout), then the children from the layout's hidden items. */
    ConnectStatus(MenuLayoutItem *res, PageLayout *layout); /* 0x001036D0 */
    virtual ~ConnectStatus();               /* [0] +0x0C 0x00103850 deletes the children */
    virtual void virtual_1();               /* [1] +0x14 0x00103938 update: the children's */
    virtual void virtual_2();               /* [2] +0x1C 0x001039E0 draw: the connected slots, "entry"
                                               on each console in, "message" while some are not */

    Item *entries[8];                       /* 0x100 "entry_no_1" .. "entry_no_8" */
    Item *entry;                            /* 0x120 "entry" */
    Item *message;                          /* 0x124 "message" */
};
EGCS_SIZE(gt2k_menu::ConnectStatus, 0x128);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu13ConnectStatus(void);    /* 0x00103B20 */
#endif

#endif /* GAME_T034_CONNECT_STATUS_H */
