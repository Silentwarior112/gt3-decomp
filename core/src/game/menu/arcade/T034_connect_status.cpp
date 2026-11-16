/* T034 - gt2k_menu::ConnectStatus.  0x001036D0..0x00103B70.
 *
 * The i.LINK connection panel of "ar/connect" (made by the item factory
 * arcade_create_connect_status, T030): eight slot pictures "entry_no_1" ..
 * "entry_no_8", the "entry" picture drawn over the slot of every console
 * that has entered (its key message arrived: pdi1394::pollKey) and the
 * "message" shown while some of the consoles on the bus (Pdi1394Status
 * nodeCount) have not.  It updates and draws only its children, not its own
 * picture.
 *
 * Not arcade_select_internal.h: the object's .rodata (0x003094F8..0x00309656) is
 * "entry_no_%d", the vtable, then the type names ConnectStatus, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>, pdi1394,
 * Operator (reverse definition order; Operator, a base the typeinfo of
 * OperatorHolder needs, comes last).  So the file includes only what it
 * uses, pdi1394 first, with the original's inline destructor (its type name
 * "7pdi1394": T541_pdi1394.h). */
#include "types.h"
#include <stddef.h>
#include "engine/pdi/T541_pdi1394.h"              /* pdi1394::pollKey */
#include "game/menu/arcade/T034_connect_status.h"
#include "engine/pdi/G003_pdi1394_rpc.h"          /* Pdi1394Status, pdi1394_get_status */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "engine/system/T473_link_session.h"         /* F_0022D3B8 (the pdi1394 driver) */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
extern "C" {
extern u32 __tiQ29gt2k_menu13ConnectStatus[3];         /* 0x004B1480 its __si_type_info */
}
#endif

namespace gt2k_menu {

/* Item(res, layout), then the children from the layout's hidden items:
 * each a new Item (0x100 bytes, Object::operator new on the menu heap; a
 * constructor that throws frees it with Object::operator delete, and this
 * constructor then runs Item::~Item(this, 0)). */
// FUNC 0x001036D0 gt2k_menu::ConnectStatus::ConnectStatus
ConnectStatus::ConnectStatus(MenuLayoutItem *res, PageLayout *layout)
    : Item(res, layout)
{
    char name[0x40];
    for (int i = 0; i < 8; i++) {
        sprintf(name, "entry_no_%d", i + 1);
        entries[i] = new Item(name, layout);    /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    }
    entry = new Item("entry", layout);
    message = new Item("message", layout);
}

/* Deletes the children (their virtual deleting destructors, flags 3), then
 * Item::~Item(this, 0) and, if (flags & 1), Object::operator delete. */
// FUNC 0x00103850 gt2k_menu::ConnectStatus::~ConnectStatus
ConnectStatus::~ConnectStatus()
{
    if (message != NULL) {
        delete message;
        message = NULL;
    }
    if (entry != NULL) {
        delete entry;
        entry = NULL;
    }
    for (int i = 0; i < 8; i++) {
        if (entries[i] != NULL) {
            delete entries[i];
            entries[i] = NULL;
        }
    }
    /* then ~Item; the deleting form calls Object::operator delete */
    /* implicit: operator delete (deleting destructor) */
}

/* Update: the children's (Item::virtual_1 is not called for the panel). */
// FUNC 0x00103938 gt2k_menu::ConnectStatus::virtual_1
void ConnectStatus::virtual_1()
{
    for (int i = 0; i < 8; i++) {
        if (entries[i] != NULL)
            entries[i]->virtual_1();
    }
    if (entry != NULL)
        entry->virtual_1();
    if (message != NULL)
        message->virtual_1();
}

/* Draw: the slots of the first `connected' consoles (no bound check against
 * the 8 entries), the "entry" picture moved (at rest) onto each of the
 * first `entered' slots and drawn there, then the message while
 * entered < connected.  One `i' for both loops (retail's registers: with
 * an index per loop the second loop's counter and pointer swap s0 / s1). */
// FUNC 0x001039E0 gt2k_menu::ConnectStatus::virtual_2
void ConnectStatus::virtual_2()
{
    Pdi1394Status status;
    pdi1394 *link = F_0022D3B8();
    pdi1394_get_status(&status);
    int connected = status.nodeCount;
    int entered = link->pollKey();
    int i;

    for (i = 0; i < connected; i++) {
        if (entries[i] != NULL)
            entries[i]->virtual_2();
    }
    for (i = 0; i < entered; i++) {
        if (entry != NULL && entries[i] != NULL) {
            Item *slot = entries[i];
            entry->x.set(slot->x.value);
            entry->y.set(slot->y.value);
            entry->virtual_2();
        }
    }
    if (message != NULL && entered < connected)
        message->virtual_2();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base Item). */
// FUNC 0x00103B20 __tfQ29gt2k_menu13ConnectStatus
extern "C" void *__tfQ29gt2k_menu13ConnectStatus(void)
{
    if (__tiQ29gt2k_menu13ConnectStatus[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu13ConnectStatus, "Q29gt2k_menu13ConnectStatus",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu13ConnectStatus;
}
#endif
