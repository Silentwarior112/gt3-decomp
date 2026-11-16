/* T209 - a factory of ButtonLists labelled by a callback.
 * 0x001B5E88..0x001B60B0.
 *
 * Data (retail .rodata 0x003334C0..0x00333606, unreferenced): only type
 * names, SmartIndexT<int>, List<Button>, Cell<Button>, the operator list
 * classes (Item's), Operator: the original read the Button header (Item,
 * Cell<Button>), then ButtonList's (List<Button>, SmartIndexT<int>).  The
 * floats are lui / mtc1 (-G0). */
// DATA_START 0x003334C0
#include "game/menu/framework/T206_button.h"               /* gt2k_menu::Button (Item, Cell<Button>) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail 0x003334E0 / 0x003334C0). */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList, ButtonData */
#include "game/menu/framework/T225_menu_item_array.h"      /* ::MenuItemArray */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "game/menu/framework/T236_free_agent.h"           /* gt2k_menu::FreeAgent */
#include "game/menu/framework/T241_link_manager.h"         /* gt2k_menu::LinkPage */
#include "game/menu/framework/T209_name_button_list.h"
#include "game/menu/framework/menu_buttons_internal.h"

using namespace gt2k_menu;

/* One caller (T044 0x0011AB78).  NULL if the layout has no item.
 * Otherwise `count` entries, each a copy of the page's first record
 * (page->items[0]) with the layout's first item and the label
 * name(buf, 0x80, i); a ButtonList (relative 1, spacing 4.0) centred in
 * rect (y turned from bottom-up to top-down: 448 - centre); a FreeAgent
 * holding the entries' block hangs under the list.  `parent` is not
 * used. */
// FUNC 0x001B5E88 create_name_button_list
ButtonList *create_name_button_list(LinkPage *page, MenuLayoutItem *rect, void *parent,
                                    int controlType, int loop, PageLayout *layout, int index,
                                    int count, void (*name)(char *buf, int size, int i))
{
    MenuLayoutItem *first = layout->firstPageItem();
    if (first == 0)
        return 0;

    MenuItemArray entries(count);
    char label[0x80];
    ButtonData *tmpl = page->items[0];
    for (int i = 0; i < count; i++) {
        name(label, 0x80, i);
        entries.setEntry(i, tmpl, first, label);
    }

    f32 height = rect->h;
    f32 width = rect->w;
    f32 centreX = rect->x + width * 0.5f;
    f32 centreY = 448.0f - (rect->y + height * 0.5f);
    /* Through MenuItemArray's inline accessors (retail keeps &entries in
     * $fp for them, as u009's T044).  Retail sets $f12..$f15 before count
     * ($t1): T244's 13-argument constructor declares its floats right after
     * layout (u070: exact so, 97.4% before). */
    ButtonList *list = new ButtonList(controlType, loop, layout, centreX, centreY, width, height, 4.0f, /* implicit: operator new, operator delete (new-expression) */
                                      entries.getItems(), count, 1, index, entries.getRects());
                                            /* Object::operator new (0x128 bytes) */
    FreeAgent *agent = new FreeAgent(entries.getBuffer());   /* 0x24 bytes; the inline constructor:
                                                           Object(), mem, the vptr */
                                            /* implicit: Object (FreeAgent's inline constructor) */
    agent->attachTo(list);
    return list;
}
