/* T274_raw_scroll_bar.h - gt2k_menu::RawScrollBar (translation unit T274,
 * unit u074).
 *
 * Only the constructor (with the constructor of its 0x14-byte member at
 * +0x1E4), the destructor and the typeinfo function are in T274; slot 8 (the
 * class name) and the registry entry are in T272.  Nothing in the image reads
 * the members (? an unfinished class): the names are the first pass's. */
#ifndef GAME_T274_RAW_SCROLL_BAR_H
#define GAME_T274_RAW_SCROLL_BAR_H

#include "game/game.h"
#include "game/menu/framework/T264_raw_window.h"           /* base gt2k_menu::RawWindow */

namespace gt2k_menu {
struct RawWindowArgs;                       /* T264 (references / pointers only) */
struct MenuLayoutItem;                      /* T229 */
class PageLayout;                           /* T229 */

/* vtable 0x00339060 (24 slots: [0] and [8] its own, the rest RawWindow's).
 * typeinfo __tfQ29gt2k_menu12RawScrollBar 0x001D5D10 (si, base RawWindow;
 * __si_type_info 0x004B2800).  Size 0x204 (operator new in T272's create,
 * 0x001D3B08). */
class RawScrollBar : public RawWindow {
public:
    /* (? a scroll range) a member with its own constructor: 0, 0, 100.0f,
     * 10.0f, 0.  Not polymorphic, 0x14 bytes. */
    struct Range {
        Range();                            /* 0x001D5C10 */

        int unk_00;                         /* 0x00 */
        int unk_04;                         /* 0x04 */
        f32 max;                            /* 0x08 (? ) 100 */
        f32 page;                           /* 0x0C (? ) 10 */
        int unk_10;                         /* 0x10 */
    };

    RawScrollBar(const RawWindowArgs &args);   /* 0x001D5C40 */
    virtual ~RawScrollBar();                /* [0] +0x0C 0x001D5CB8 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3AC0 (T272) "RawScrollBar" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3AC8 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3B08 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3B80 (T272) */

    Range range;                            /* 0x1E4 */
    int   unk_1F8;                          /* 0x1F8 0 */
    int   unk_1FC;                          /* 0x1FC 0 */
    int   unk_200;                          /* 0x200 0 */
};
EGCS_SIZE(gt2k_menu::RawScrollBar::Range, 0x14);
EGCS_SIZE(gt2k_menu::RawScrollBar, 0x204);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu12RawScrollBar(void);     /* 0x001D5D10 */
#endif

#endif /* GAME_T274_RAW_SCROLL_BAR_H */
