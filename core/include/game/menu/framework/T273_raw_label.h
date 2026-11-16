/* T273_raw_label.h - gt2k_menu::RawLabel (translation unit T273, unit u073),
 * 0x001D3E40..0x001D3F70.
 *
 * A list item that only sets its focus picture.  Its class name (slot 8),
 * layout factory and registry functions are in T272. */
#ifndef GAME_T273_RAW_LABEL_H
#define GAME_T273_RAW_LABEL_H

#include "game/game.h"
#include "game/menu/framework/T263_raw_list.h"             /* base gt2k_menu::RawListItem */

namespace gt2k_menu {
class RawWindow;                            /* T264 (pointers only) */
struct RawWindowArgs;                       /* T264 (references / pointers only) */
struct MenuLayoutItem;                      /* T229 (pointers only) */
class PageLayout;                           /* T229 (pointers only) */

/* vtable 0x00338038 (24 slots: [1]..[3] / [9]..[20] / [22] / [23]
 * RawWindow's, [21] RawListItem's, [4] Item's, [5] Image's, [6] / [7]
 * Transitive's).  typeinfo __tfQ29gt2k_menu8RawLabel 0x001D3F20
 * (__si_type_info 0x004B2730, base RawListItem).  No data of its own: size
 * 0x204 (operator new(0x204) in create). */
class RawLabel : public RawListItem {
public:
    RawLabel(const RawWindowArgs &args);    /* 0x001D3E40 RawListItem(args); the focus picture p0
                                               (NULL: "label_focus") */
    virtual ~RawLabel();                    /* [0] +0x0C 0x001D3EC8 only ~RawListItem */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3098 (T272) the class name "RawLabel" */

    /* The layout factory (no callers): the eight words as a RawWindowArgs, then create. */
    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D30A0 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D30E0 (T272) the registry's */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3158 (T272) 1 (args unused) */
};
EGCS_SIZE(gt2k_menu::RawLabel, 0x204);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu8RawLabel(void);      /* 0x001D3F20 */
}
#endif

#endif /* GAME_T273_RAW_LABEL_H */
