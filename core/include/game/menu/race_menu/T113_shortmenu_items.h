/* T113_shortmenu_items.h - the qm_* page actions and hooks of the short menu
 * and gt2k_menu::ShortMenuText (translation unit T113, unit u015),
 * 0x0012CB70..0x0012E2C8.
 *
 * T113_static_init fills the two tables ShortmenuControl::begin (T111)
 * registers, both sorted by name: g_0029C6B0, the 21 qm_* actions
 * (ExecuteFuncData, T211: "qm_exit", "qm_replay", ..., "qm_yes" / "qm_no";
 * each stores its QM_* code, T102, or the yes / no answer in the running
 * session's ShortMenuArgs, g_003514F0), and g_0029C758, the 45 qm_* hooks
 * (MenuHook, T229: {name, create, p8, pC, 0, 0}; p8 the line 0..5 or the
 * licence target 1 gold .. 3 bronze, pC the place of qm_mt_1000_time_rank)
 * of both layouts.  A hook makes a ShortMenuText whose text is formatted
 * from the ShortMenuArgs (times, cars, points, ranks, the licence targets,
 * the session number); highlighted lines get colour 0x800080FF.  The
 * actions and hooks are used only through the tables; their counts are
 * g_00351454 / g_00351480 (T113 .sdata).  T103's
 * ShortmenuLicenseControl uses the same tables. */
#ifndef GAME_T113_SHORTMENU_ITEMS_H
#define GAME_T113_SHORTMENU_ITEMS_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ExecuteFuncData (g_0029C6B0), MenuHook (T229) */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */

/* vtable 0x00315100 (8 slots: [1] Item's update 0x001B9F98, [2] Item's draw
 * 0x001BA780, [3] .. [7] as Item's).  typeinfo
 * __tfQ29gt2k_menu13ShortMenuText 0x0012E230 (si, base Item; storage
 * 0x004B1730).  An Item whose text is formatted into its own buffer
 * (setText(buf)).  Size 0x180 (operator new in the hooks).  The destructor
 * is implicit in the original: 0x0012E280 does not store the vptr (~Item
 * with 0, Object's operator delete when bit 0 of the flags is set). */
class ShortMenuText : public Item {
public:
    /* Inline (the hooks expand it): Item(res, tex), then the vptr. */
    ShortMenuText(MenuLayoutItem *res, MenuTexture *tex) : Item(res, tex) {}
    /*  [0] +0x0C 0x0012E280 ~ShortMenuText: implicit (see above) */

    char buf[0x80];                         /* 0x100 */
};
EGCS_SIZE(gt2k_menu::ShortMenuText, 0x180);

} /* namespace gt2k_menu */

/* The tables g_0029C6B0 / g_0029C758 and their counts g_00351454 /
 * g_00351480 (T113 .data / .sdata): in their own header since u015's data
 * pass (the ShortmenuControls of T111 / T103 see them, not ShortMenuText). */
#include "game/menu/race_menu/T113_shortmenu_tables.h"

/* A test hook ("kjm"; in no table, no references): a plain Item showing
 * the TEX1 picture "database/kjmtest.img" (0x4000 bytes, memalign'd, not
 * freed when the file cannot be read: then NULL).  ctx, layout, parent,
 * p8 .. p14 are not used. */
gt2k_menu::Transitive *kjm_test_image(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0012D7F0 */
/* The static initialiser of the two tables (.ctors entry 0x0028F0DC; the
 * compiler's _GLOBAL_$I$ function in the original). */
void T113_static_init(void);                /* 0x0012D8C0 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu13ShortMenuText(void);       /* 0x0012E230 */
extern u32 __tiQ29gt2k_menu13ShortMenuText[3];     /* 0x004B1730 its __si_type_info */
}
#endif

#endif /* GAME_T113_SHORTMENU_ITEMS_H */
