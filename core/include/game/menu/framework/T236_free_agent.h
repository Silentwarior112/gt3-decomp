/* T236_free_agent.h - gt2k_menu::FreeAgent (translation unit T236, unit
 * u069), 0x001C5D40..0x001C5E50.
 *
 * An Object that owns one block of the menu heap (a list factory's entry
 * array, MenuItemArray::buffer) and frees it when the object tree releases
 * it: the list factories hang one under every list they build. */
#ifndef GAME_T236_FREE_AGENT_H
#define GAME_T236_FREE_AGENT_H

#include "game/game.h"
#include "game/menu/framework/T220_object.h"               /* base gt2k_menu::Object */

namespace gt2k_menu {

/* vtable 0x003365B0 (5 slots: [1] / [2] Object's, [3] / [4] 0x001BFF00 /
 * 0x001BFF08).  typeinfo __tfQ29gt2k_menu9FreeAgent 0x001C5E00 (si, base
 * Object; __si_type_info 0x004B2610).  Size 0x24. */
class FreeAgent : public Object {
public:
    /* Inline in the original: Object(), then mem = p, expanded by the list
     * factories (u009 T044, u040, u063, u067 T226).  T236 also has an
     * out-of-line copy with no callers: 0x001C5DB8. */
    FreeAgent(void *p) : mem(p) {}
    virtual ~FreeAgent();                   /* [0] +0x0C 0x001C5D40 frees mem on the current menu heap */

    void *mem;                              /* 0x20 a block of the menu heap (NULL: none) */
};
EGCS_SIZE(gt2k_menu::FreeAgent, 0x24);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu9FreeAgent(void);     /* 0x001C5E00 */
}
#endif

#endif /* GAME_T236_FREE_AGENT_H */
