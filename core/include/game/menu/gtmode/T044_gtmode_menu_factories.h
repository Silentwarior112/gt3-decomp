/* T044_gtmode_menu_factories.h - the MenuFuncData table of the GT-mode menu,
 * gt2k_menu::DealerButtonList and gt2k_menu::CarGoodsButtonList (translation
 * unit T044, unit u009), 0x0011A2B0..0x0011BC18.
 *
 * g_00295FB8 (10 entries, sorted by name, filled by T044_static_init) maps
 * the "menu_*" layout objects of the GT-mode pages to the factories of their
 * button lists.  GtmodeControl::begin (T043) gives it to its PageManager
 * (menu table +0xB0C, count g_00351088, ctx the GtmodeControl);
 * PageManager::callMenuFunc (T211) calls create(ctx, page, rect, parent,
 * controlType, loop, layout, index) for the first entry whose object the page
 * has.  The lists are centred on rect (y up from the bottom of the 448-line
 * screen: centre (x + w / 2, 448 - (y + h / 2))); the lists of run-time
 * records (garage, wheels) hang a FreeAgent (T236) holding the MenuItemArray
 * (T225) block under themselves.
 *
 * The factories take the MenuFuncData signature; u009's MenuElement is T241's
 * LinkPage (count +0x104, items +0x108). */
#ifndef GAME_T044_GTMODE_MENU_FACTORIES_H
#define GAME_T044_GTMODE_MENU_FACTORIES_H

#include "game/game.h"
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::MenuFuncData (g_00295FB8) */
#include "game/save/T429_game_status.h"          /* getGameStatus, MenuStatus::car (GarageCar::getName):
                                               CarGoodsButtonList's constructor */
#include "game/data/T481_car_data_base.h"        /* nameToCode (the same) */
#include "game/data/T489_car_database.h"         /* db_car_is_locked (the same) */
#include "game/common/T494_text_utils.h"           /* str_equal (the same) */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct LinkPage;                            /* T241 */
struct ButtonData;                          /* T244 */

/* The two classes in the original's order (u009's data pass): CarGoodsButtonList
 * first, DealerButtonList after it.  egcs writes their vtables and type names
 * in reverse definition order (retail: Dealer's at 0x0030FBF0 / 0x0030FD00
 * before CarGoods' 0x0030FC78 / 0x0030FD20) and their out-of-line inline
 * members likewise (the frame info: Dealer's constructor, __tf, destructor,
 * then CarGoods'), each class's members declared inline first, then its
 * __tf, then its implicit members: the destructors are implicit (the key
 * function is createButtons). */

/* The cars of a tuning shop ("menu_car_goods").  Bases: ButtonList.
 * vtables 0x0030FC90 (13 slots) and 0x0030FC78 (the List<Button> part, 2
 * slots, this-delta -0xB0).  typeinfo __tfQ29gt2k_menu18CarGoodsButtonList
 * 0x0011BB60 (class, base list g_00351090; storage 0x004B15B0).  No data of
 * its own: size 0x128 (operator new in create_menu_car_goods). */
class CarGoodsButtonList : public ButtonList {
public:
    /* Inline (expanded in create_menu_car_goods; the out-of-line copy
     * 0x0011B9B0 has no callers): as DealerButtonList's, then the cursor on
     * the menu status's car (MenuStatus::car->getName()) counted among the
     * cars that are not locked (T489 db_car_is_locked), else on `cursor'.
     * relative is not read.  In the class, as DealerButtonList's: defined
     * out of the class (T044's source had it so before u009's data pass) its
     * out-of-line copy came first in the end-of-unit group, ahead of
     * DealerButtonList's members (the frame info's order). */
    CarGoodsButtonList(int controlType, int loop, PageLayout *layout, ButtonData **items,
                       int count, int relative, int cursor, f32 cx, f32 cy, f32 cw, f32 ch,
                       MenuLayoutItem *rects)
        : ButtonList(controlType, loop, layout)
    {
        x.set(cx);
        y.set(cy);
        w = cw;
        h = ch;
        CarGoodsButtonList::createButtons(items, count, rects);
        resetIndex();
        const char *selected = getGameStatus()->getMenuStatus()->car->getName();
        if (selected != NULL) {
            int locked = 0;
            int i;
            for (i = 0; i < count; i++) {
                if (db_car_is_locked(nameToCode(items[i]->name)) != 0) {
                    locked++;
                    continue;
                }
                if (str_equal(items[i]->name, selected) != 0)
                    break;
            }
            if (i < count) {
                index.set(i - locked);
                return;
            }
        }
        index.set(cursor);
        /* (on an exception: ~ButtonList) */
    }                                       /* 0x0011B9B0 */
    /*  [0] +0x0C 0x0011BBB8 ~CarGoodsButtonList: implicit (both vptrs,
     *      ~ButtonList, operator delete if flags & 1) */
    /* A button per entry that is not locked and has a page item of its name
     * (rects is not read), one after the other, spacing apart. */
    virtual void createButtons(ButtonData **items, int count,
                               MenuLayoutItem *rects);  /* [6] +0x3C 0x0011ADF8 */
};
EGCS_SIZE(gt2k_menu::CarGoodsButtonList, 0x128);

/* The cars of a dealer ("menu_dealer").  Bases: ButtonList (Drawable at
 * +0x00, List<Button> at +0xB0).  vtables 0x0030FC08 (13 slots: [1] / [5] /
 * [7]..[12] ButtonList's, [2] Drawable's, [3] / [4] Object's) and 0x0030FBF0
 * (the List<Button> part, 2 slots, this-delta -0xB0: [1] pList's sort
 * 0x0023C580).  typeinfo __tfQ29gt2k_menu16DealerButtonList 0x0011B8F8
 * (class, base list g_00351090 {ButtonList, public +0}; storage 0x004B15A0).
 * No data of its own: size 0x128 (operator new in create_menu_dealer). */
class DealerButtonList : public ButtonList {
public:
    /* Inline (expanded in create_menu_dealer; the out-of-line copy
     * 0x0011B7D8 has no callers): the centre (cx, cy) and size, the buttons
     * (createButtons, called directly), the cursor on `cursor'.  relative is
     * not read (create_menu_dealer passes 1). */
    DealerButtonList(int controlType, int loop, PageLayout *layout, ButtonData **items,
                     int count, int relative, int cursor, f32 cx, f32 cy, f32 cw, f32 ch,
                     MenuLayoutItem *rects)
        : ButtonList(controlType, loop, layout)
    {
        x.set(cx);
        y.set(cy);
        w = cw;
        h = ch;
        DealerButtonList::createButtons(items, count, rects);
        resetIndex();
        index.set(cursor);
    }
    /*  [0] +0x0C 0x0011B950 ~DealerButtonList: implicit (both vptrs,
     *      ~ButtonList, operator delete if flags & 1) */
    /* ButtonList's, with the element of entry i from rects[i] when rects is
     * given, without the "opel" entry in country 2 (UK: g_00352FD8) and the
     * "vauxhall" one elsewhere; every new button hidden (virtual_9(0)). */
    virtual void createButtons(ButtonData **items, int count,
                               MenuLayoutItem *rects);  /* [6] +0x3C 0x0011B138 */
};
EGCS_SIZE(gt2k_menu::DealerButtonList, 0x128);

} /* namespace gt2k_menu */

/* The MenuFuncData factories (ctx: the GtmodeControl; parent is not used). */
/* "menu_garage": a GarageButtonList (T219) of the garage's cars ("<name 1>
 * <name 2>"), page "hm/hm_gr_panel", sortType GtmodeControl::sortType,
 * infoMode &GtmodeControl::infoMode; NULL if the page has no item. */
gt2k_menu::ButtonList *create_menu_garage(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011A2B0 */
/* "menu_garage_trade": the same for the guest garage (the GameStatusHolder's
 * status, T004), labelled with name 1 only. */
gt2k_menu::ButtonList *create_menu_garage_trade(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011A4E8 */
/* "menu_wheel": a WheelButtonList (T224) of the garage's wheels ("default"
 * for the car's own). */
gt2k_menu::ButtonList *create_menu_wheel(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011A700 */
/* "menu_wheel_shop": a WheelShopButtonList (T208) of the page's records. */
gt2k_menu::ButtonList *create_menu_wheel_shop(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011A8E0 */
/* The label callback of "menu_regulated_car" (T209 create_name_button_list):
 * the n-th distinct entry car of the current event (a name counts at its
 * last occurrence), "--" if none; size is not read. */
void regulated_car_name(char *out, int size, int n);   /* 0x0011A9F0 */
/* "menu_regulated_car": create_name_button_list of the distinct entry cars;
 * without any, the hidden item "no_regulated_car" (z + 100) under parent. */
gt2k_menu::ButtonList *create_menu_regulated_car(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011AB78 */
/* "menu_map": a MapButtonList (T207). */
gt2k_menu::ButtonList *create_menu_map(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011AD70 */
/* "menu_car_goods": a CarGoodsButtonList (relative 1, no rects). */
gt2k_menu::ButtonList *create_menu_car_goods(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011AF18 */
/* "menu_dealer": a DealerButtonList (relative 1, no rects). */
gt2k_menu::ButtonList *create_menu_dealer(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011B2D0 */
/* "menu_top": a TopButtonList (T047, relative 1). */
gt2k_menu::ButtonList *create_menu_top(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011B468 */
/* "menu_ride_sure": a plain ButtonList of the first record, or the first two
 * when a car is ridden; spacing 54. */
gt2k_menu::ButtonList *create_menu_ride_sure(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x0011B578 */

/* T044 .data, filled by T044_static_init (.ctors 0x0028F054; compiler-
 * generated in the original): {"menu_car_goods", create_menu_car_goods} ..
 * {"menu_wheel_shop", create_menu_wheel_shop}. */
extern gt2k_menu::MenuFuncData g_00295FB8[10];     /* 0x00295FB8 */
extern int g_00351088;                      /* 0x00351088 gp-0x6968 its entries (10; defined by
                                               T044's C) */
void T044_static_init(void);                /* 0x0011B6B8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu16DealerButtonList(void);     /* 0x0011B8F8 */
void *__tfQ29gt2k_menu18CarGoodsButtonList(void);   /* 0x0011BB60 */
}
#endif

#endif /* GAME_T044_GTMODE_MENU_FACTORIES_H */
