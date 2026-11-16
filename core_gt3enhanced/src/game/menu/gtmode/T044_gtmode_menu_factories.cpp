/* T044 - the MenuFuncData factories of the GT mode pages (the "menu_*" button
 * lists), gt2k_menu::DealerButtonList and gt2k_menu::CarGoodsButtonList.
 * 0x0011A2B0..0x0011BC18.
 *
 * PageManager::callMenuFunc (T211) builds a page's button list with the
 * first entry of g_00295FB8 (filled by T044_static_init) whose layout object
 * the page has: create(ctx = the GtmodeControl, the page record, the
 * object's rectangle, the layer, the page's controlType / loop, the layout,
 * the cursor).  The lists are centred on the rectangle (the layout's y goes
 * up from the bottom of the 448-line screen).  The lists of run-time records
 * (garage, wheels) hang a FreeAgent holding their MenuItemArray block under
 * themselves.  `parent' is only used by menu_regulated_car.
 *
 * The DealerButtonList / CarGoodsButtonList constructors are inline:
 * create_menu_dealer / create_menu_car_goods expand them, and their
 * out-of-line copies (0x0011B7D8 / 0x0011B9B0) have no callers.
 *
 * Includes: retail's type names (.rodata 0x0030FD00..0x0030FF90, reverse
 * definition order) are DealerButtonList, CarGoodsButtonList, List<Button>,
 * Cell<Button>, OperatorList, List<OperatorHolder>, OperatorHolder,
 * Cell<OperatorHolder>, SmartIndexT<int>, RegurationResult, the garage lists
 * (GarageWheelList .. Cell<GarageCar>) and Operator: so the GameStatus first,
 * RegurationResult (no use here: retail's T044 saw it, as T045 did), then
 * SmartIndexT<int>, the menu item classes (T206: T216's operator classes,
 * Button's base Cell<Button>), ButtonList (List<Button>) and this file's two
 * classes; the other headers add no class. */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus (garage lists, menu),
                                               MenuStatus, GarageCar, GarageWheel */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (RegurationResult: its type name, see above) */
#include "game/common/T251_smart_index.h"          /* SmartIndexT<int> (GtmodeControl::infoMode) */
#include "game/menu/framework/T206_button.h"               /* Button::virtual_9 (visible), Cell<Button> */
#include "game/menu/framework/T244_button_list.h"          /* ButtonList (List<Button>), ButtonData */
#include "game/menu/gtmode/T044_gtmode_menu_factories.h" /* DealerButtonList, CarGoodsButtonList, the factories,
                                               MenuFuncData g_00295FB8 / g_00351088 */
#include "game/menu/gtmode/T043_gtmode_control.h"       /* GtmodeControl::getInfoMode / getSortType */
#include "game/app/T004_game_status_holder.h"   /* getGameStatusHolder (the guest garage) */
#include "game/menu/framework/T220_object.h"               /* Object::attachTo, operator new / delete, z */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout::findPageItem / firstPageItem,
                                               MenuLayoutItem */
#include "game/menu/framework/T241_link_manager.h"         /* LinkPage (count, items: the page's records) */
#include "game/menu/framework/T213_menu_item.h"            /* Item("no_regulated_car", layout) */
#include "game/menu/framework/T236_free_agent.h"           /* FreeAgent (inline constructor) */
#include "game/menu/framework/T225_menu_item_array.h"      /* MenuItemArray, setEntry */
#include "game/menu/gtmode/T219_garage_button_list.h"   /* GarageButtonList */
#include "game/menu/gtmode/T224_wheel_button_list.h"    /* WheelButtonList */
#include "game/menu/gtmode/T208_wheel_shop_button_list.h" /* WheelShopButtonList */
#include "game/menu/framework/T207_map_button_list.h"      /* MapButtonList */
#include "game/menu/gtmode/T047_top_button_list.h"      /* TopButtonList */
#include "game/menu/framework/T209_name_button_list.h"     /* create_name_button_list */
#include "game/menu/framework/menu_list.h"                 /* ListIterator<GarageCar / GarageWheel> */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::getName1 / getName2 */
#include "game/data/T484_race_database.h"        /* RaceDataBase */
#include "game/data/T489_car_database.h"         /* db_car_is_locked */
#include "game/common/T494_text_utils.h"           /* str_equal */
#include "game/data/T516_country.h"              /* Country g_00352FD8, COUNTRY_BR */
#include "game/menu/framework/T261_menu_draw_state.h"      /* MENU_SCREEN_H */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "game/menu/gtmode/gtmode_control_internal.h"

using namespace gt2k_menu;

/* "menu_garage": the cars of the garage, labelled "<name 1> <name 2>", in a
 * GarageButtonList (page "hm/hm_gr_panel"; the control's sortType and
 * infoMode).  (operator new: Object's, 0x33C bytes; the FreeAgent 0x24.) */
// FUNC 0x0011A2B0 create_menu_garage
ButtonList *create_menu_garage(void *ctx, LinkPage *page, MenuLayoutItem *rect, Object *parent,
                               int controlType, int loop, PageLayout *layout, int index)
{
    GtmodeControl *control = (GtmodeControl *)ctx;
    gt2k_game::GameStatus *status = getGameStatus();
    MenuLayoutItem *first = layout->firstPageItem();
    if (first == NULL)
        return NULL;
    GarageCarList *garage = status->getCarList();
    int count = garage->count();
    MenuItemArray entries(count);
    ButtonData *tmpl = page->items[0];
    ListIterator<GarageCar> it(garage);
    for (int i = 0; i < count; i++) {
        GarageCar *car = it.next();
        char label[0x80];
        sprintf(label, "%s %s", car->getName1(), car->getName2());
        entries.setEntry(i, tmpl, first, label);
    }
    /* (Both read before the allocation: retail loads sortType ahead of
     * operator new.) */
    SmartIndexT<int> *infoMode = control->getInfoMode();
    int sortType = control->getSortType();
    GarageButtonList *list = new GarageButtonList(status, "hm/hm_gr_panel", controlType, loop, /* implicit: Object::operator delete (if the constructor throws) */
                                                  layout, entries.getItems(), count, rect,
                                                  entries.getRects(), index, sortType, infoMode);
    FreeAgent *agent = new FreeAgent(entries.getBuffer());   /* inline: Object(), mem, the vptr */ /* implicit: Object::operator delete (if the constructor throws) */
    agent->attachTo(list);
    return list;
    /* (on an exception: Object::operator delete of the list / the agent) */
}

/* "menu_garage_trade": the same for the guest garage (the GameStatus of
 * getGameStatusHolder()), labelled with name 1 only. */
// FUNC 0x0011A4E8 create_menu_garage_trade
ButtonList *create_menu_garage_trade(void *ctx, LinkPage *page, MenuLayoutItem *rect,
                                     Object *parent, int controlType, int loop,
                                     PageLayout *layout, int index)
{
    GtmodeControl *control = (GtmodeControl *)ctx;
    gt2k_game::GameStatus *guest = getGameStatusHolder()->get();
    MenuLayoutItem *first = layout->firstPageItem();
    if (first == NULL)
        return NULL;
    GarageCarList *garage = guest->getCarList();
    int count = garage->count();
    MenuItemArray entries(count);
    ButtonData *tmpl = page->items[0];
    ListIterator<GarageCar> it(garage);
    for (int i = 0; i < count; i++) {
        GarageCar *car = it.next();
        entries.setEntry(i, tmpl, first, car->getName1());
    }
    /* (Both read before the allocation: retail loads sortType ahead of
     * operator new.) */
    SmartIndexT<int> *infoMode = control->getInfoMode();
    int sortType = control->getSortType();
    GarageButtonList *list = new GarageButtonList(guest, "hm/hm_gr_panel", controlType, loop, /* implicit: Object::operator delete (if the constructor throws) */
                                                  layout, entries.getItems(), count, rect,
                                                  entries.getRects(), index, sortType, infoMode);
    FreeAgent *agent = new FreeAgent(entries.getBuffer()); /* implicit: Object::operator delete (if the constructor throws) */
    agent->attachTo(list);
    return list;
    /* (on an exception: Object::operator delete of the list / the agent) */
}

/* "menu_wheel": the wheels of the garage ("default" for an empty name: the
 * car's own wheel) in a WheelButtonList. */
// FUNC 0x0011A700 create_menu_wheel
ButtonList *create_menu_wheel(void *ctx, LinkPage *page, MenuLayoutItem *rect, Object *parent,
                              int controlType, int loop, PageLayout *layout, int index)
{
    MenuLayoutItem *first = layout->firstPageItem();
    if (first == NULL)
        return NULL;
    int count = getGameStatus()->getWheelNum();
    MenuItemArray entries(count);
    ButtonData *tmpl = page->items[0];
    ListIterator<GarageWheel> it(getGameStatus()->getWheelList());
    for (int i = 0; i < count; i++) {
        GarageWheel *wheel = it.next();
        const char *name = wheel->getName();
        if (name == NULL)
            name = "default";
        entries.setEntry(i, tmpl, first, name);
    }
    /* (Through MenuItemArray's inline accessors here: retail keeps &entries
     * in $s4 for them, set before the loop and on its bypass.) */
    WheelButtonList *list = new WheelButtonList(controlType, loop, layout, entries.getItems(), /* implicit: Object::operator delete (if the constructor throws) */
                                                count, rect, entries.getRects(), index);
    FreeAgent *agent = new FreeAgent(entries.getBuffer()); /* implicit: Object::operator delete (if the constructor throws) */
    agent->attachTo(list);
    return list;
    /* (on an exception: Object::operator delete of the list / the agent) */
}

/* "menu_wheel_shop": a WheelShopButtonList of the page's records. */
// FUNC 0x0011A8E0 create_menu_wheel_shop
ButtonList *create_menu_wheel_shop(void *ctx, LinkPage *page, MenuLayoutItem *rect,
                                   Object *parent, int controlType, int loop,
                                   PageLayout *layout, int index)
{
    f32 height = rect->h;
    f32 width = rect->w;
    f32 centreX = rect->x + width * 0.5f;
    f32 centreY = MENU_SCREEN_H - (rect->y + height * 0.5f);
    /* relative 1 ($a6) and index ($a7), after the floats: the constructor
     * does not read them */
    return new WheelShopButtonList(controlType, loop, layout, page->items, page->count, /* implicit: Object::operator delete (if the constructor throws) */
                                   centreX, centreY, width, height, 1, index);
    /* (on an exception: Object::operator delete) */
}

/* The label callback of "menu_regulated_car" (create_name_button_list): the
 * n-th distinct car of the current event's car list (a name counts at its
 * last occurrence), "--" if there is none.  `size' (0x80) is not read. */
// FUNC 0x0011A9F0 regulated_car_name
void regulated_car_name(char *out, int size, int n)
{
    const char *event = getGameStatus()->menu.event;
    RaceDataBase race;
    char name[0x40];        /* sp+0x30 */
    char other[0x40];       /* sp+0x70 */

    race.select(nameToCode(event));
    int count = race.getNumRegulatedCars();
    int found = 0;
    for (int i = 0; i < count; i++) {
        race.getRegulatedCarFullName(name, i);
        int j;
        for (j = i + 1; j < count; j++) {
            race.getRegulatedCarFullName(other, j);
            if (str_equal(name, other) != 0)
                break;
        }
        if (j >= count) {
            if (found == n) {
                strcpy(out, name);
                return;
            }
            found++;
        }
    }
    strcpy(out, "--");
    /* (~RaceDataBase on every return and on an exception) */
}

/* "menu_regulated_car": the distinct cars of the current event's car list
 * (create_name_button_list with regulated_car_name; the RaceDataBase is gone
 * before it is called); without any (any car may enter), the item
 * "no_regulated_car" is put under parent instead, 100 nearer in z. */
// FUNC 0x0011AB78 create_menu_regulated_car
ButtonList *create_menu_regulated_car(void *ctx, LinkPage *page, MenuLayoutItem *rect,
                                      Object *parent, int controlType, int loop,
                                      PageLayout *layout, int index)
{
    int distinct;
    {
        const char *event = getGameStatus()->menu.event;
        RaceDataBase race;
        char name[0x40];
        char other[0x40];

        race.select(nameToCode(event));
        int count = race.getNumRegulatedCars();
        distinct = 0;
        for (int i = 0; i < count; i++) {
            race.getRegulatedCarFullName(name, i);
            int j;
            for (j = i + 1; j < count; j++) {
                race.getRegulatedCarFullName(other, j);
                if (str_equal(name, other) != 0)
                    break;
            }
            if (j >= count)
                distinct++;
        }
        /* (~RaceDataBase) */
    }
    ButtonList *list = create_name_button_list(page, rect, parent, controlType, loop, layout,
                                               index, distinct, regulated_car_name);
    if (distinct == 0) {
        Item *item = new Item("no_regulated_car", layout);  /* Object::operator new (0x100) */ /* implicit: Object::operator delete (if the constructor throws) */
        item->z += 100.0f;
        item->attachTo(parent);
    }
    return list;
    /* (on an exception: Object::operator delete of the item) */
}

/* "menu_map": a MapButtonList of the page's records. */
// FUNC 0x0011AD70 create_menu_map
ButtonList *create_menu_map(void *ctx, LinkPage *page, MenuLayoutItem *rect, Object *parent,
                            int controlType, int loop, PageLayout *layout, int index)
{
    return new MapButtonList(page->items, page->count, layout, index); /* implicit: Object::operator delete (if the constructor throws) */
    /* (on an exception: Object::operator delete) */
}

namespace gt2k_menu {

/* A button per record that is not locked (db_car_is_locked) and has a page
 * item of its name, one after the other along the list, spacing apart (the
 * index is the record's, counting the skipped ones).  rects is not read.
 * Matching: items[i] (loop.c's pointer giv set after the guard) and the
 * `controlType == 0 ? h : w' form are retail's; retail sets $f12 before $a3,
 * which egcs does with the virtual declared (elem, data, offPoint, index)
 * (T244, changed with every override and caller by u070: exact, 97.2%
 * before). */
// FUNC 0x0011ADF8 gt2k_menu::CarGoodsButtonList::createButtons
void CarGoodsButtonList::createButtons(ButtonData **items, int count, MenuLayoutItem *rects)
{
    f32 offPoint = 0.0f;
    for (int i = 0; i < count; i++) {
        if (db_car_is_locked(nameToCode(items[i]->name)) != 0)
            continue;
        MenuLayoutItem *elem = layout->findPageItem(items[i]->name);
        if (elem == NULL)
            continue;
        offPoint = offPoint + (controlType == 0 ? elem->h : elem->w) * 0.5f;
        append(createButton(elem, items[i], offPoint, i));
        offPoint += (controlType == 0 ? elem->h : elem->w) * 0.5f;
        offPoint += spacing;
    }
}

} /* namespace gt2k_menu */

/* "menu_car_goods": a CarGoodsButtonList of the page's records (relative 1,
 * no rects). */
// FUNC 0x0011AF18 create_menu_car_goods
ButtonList *create_menu_car_goods(void *ctx, LinkPage *page, MenuLayoutItem *rect,
                                  Object *parent, int controlType, int loop,
                                  PageLayout *layout, int index)
{
    /* (The inline constructor is expanded: ButtonList(controlType, loop,
     * layout), the centre and size, the vptrs, createButtons (called
     * directly), resetIndex, getGameStatus()->menu.car->getName (GarageCar),
     * the walk with db_car_is_locked / nameToCode / str_equal, index.set;
     * ~ButtonList and Object::operator delete on an exception.) */
    /* implicit: getGameStatus, getName, db_car_is_locked, nameToCode, str_equal, set (the inline CarGoodsButtonList constructor) */
    f32 height = rect->h;
    f32 width = rect->w;
    f32 centreX = rect->x + width * 0.5f;
    f32 centreY = MENU_SCREEN_H - (rect->y + height * 0.5f);
    /* implicit: CarGoodsButtonList::createButtons, resetIndex (the inline CarGoodsButtonList constructor) */
    CarGoodsButtonList *list = new CarGoodsButtonList(controlType, loop, layout, page->items, /* implicit: Object::operator delete (if the constructor throws) */
                                                      page->count, 1, index, centreX, centreY,
                                                      width, height, NULL);
    return list;
}

namespace gt2k_menu {

/* ButtonList's, with the element of record i taken from rects[i] when rects
 * is given (else the page item of its name), without "opel" in Britain
 * (g_00352FD8 COUNTRY_BR, where the cars are Vauxhalls) and without
 * "vauxhall" elsewhere (the country is read again for the second test); each
 * new button is hidden first (virtual_9(0)).  Matching: as
 * CarGoodsButtonList::createButtons (exact since T244's virtual is declared
 * (elem, data, offPoint, index); 98.0% before). */
// FUNC 0x0011B138 gt2k_menu::DealerButtonList::createButtons
void DealerButtonList::createButtons(ButtonData **items, int count, MenuLayoutItem *rects)
{
    f32 offPoint = 0.0f;
    for (int i = 0; i < count; i++) {
        MenuLayoutItem *elem = (rects != NULL) ? &rects[i] : layout->findPageItem(items[i]->name);
        if (elem == NULL)
            continue;
        if ((g_00352FD8.value == COUNTRY_BR && str_equal(items[i]->name, "opel") != 0) ||
            (g_00352FD8.value != COUNTRY_BR && str_equal(items[i]->name, "vauxhall") != 0))
            continue;
        offPoint = offPoint + (controlType == 0 ? elem->h : elem->w) * 0.5f;
        Button *button = createButton(elem, items[i], offPoint, i);
        button->virtual_9(0);
        append(button);
        offPoint += (controlType == 0 ? elem->h : elem->w) * 0.5f;
        offPoint += spacing;
    }
}

} /* namespace gt2k_menu */

/* "menu_dealer": a DealerButtonList of the page's records (relative 1, no
 * rects). */
// FUNC 0x0011B2D0 create_menu_dealer
ButtonList *create_menu_dealer(void *ctx, LinkPage *page, MenuLayoutItem *rect, Object *parent,
                               int controlType, int loop, PageLayout *layout, int index)
{
    /* (The inline constructor of T044's header is expanded:
     * ButtonList(controlType, loop, layout), the centre and size, the vptrs,
     * createButtons (called directly), resetIndex, index.set(cursor);
     * ~ButtonList and Object::operator delete on an exception.) */
    /* implicit: set (index.set in the inline DealerButtonList constructor) */
    f32 height = rect->h;
    f32 width = rect->w;
    f32 centreX = rect->x + width * 0.5f;
    f32 centreY = MENU_SCREEN_H - (rect->y + height * 0.5f);
    /* implicit: DealerButtonList::createButtons, resetIndex (the inline DealerButtonList constructor) */
    DealerButtonList *list = new DealerButtonList(controlType, loop, layout, page->items, /* implicit: Object::operator delete (if the constructor throws) */
                                                  page->count, 1, index, centreX, centreY,
                                                  width, height, NULL);
    return list;
}

/* "menu_top": a TopButtonList of the page's records (relative 1). */
// FUNC 0x0011B468 create_menu_top
ButtonList *create_menu_top(void *ctx, LinkPage *page, MenuLayoutItem *rect, Object *parent,
                            int controlType, int loop, PageLayout *layout, int index)
{
    f32 height = rect->h;
    f32 width = rect->w;
    f32 centreX = rect->x + width * 0.5f;
    f32 centreY = MENU_SCREEN_H - (rect->y + height * 0.5f);
    return new TopButtonList(controlType, loop, layout, page->items, page->count, /* implicit: Object::operator delete (if the constructor throws) */
                             centreX, centreY, width, height, 1, index);
    /* (on an exception: Object::operator delete) */
}

/* "menu_ride_sure": a plain ButtonList of the first record, or of the first
 * two when a car is ridden (the second asks to ride the new car); spacing
 * 54, relative 1, no rects.  Matching: exact since T244's 13-argument
 * constructor declares its floats right after layout (u070; 97.5% before). */
// FUNC 0x0011B578 create_menu_ride_sure
ButtonList *create_menu_ride_sure(void *ctx, LinkPage *page, MenuLayoutItem *rect,
                                  Object *parent, int controlType, int loop,
                                  PageLayout *layout, int index)
{
    int count = 1;
    if (getGameStatus()->getRunningCar() != NULL)
        count = 2;
    f32 height = rect->h;
    f32 width = rect->w;
    f32 centreX = rect->x + width * 0.5f;
    f32 centreY = MENU_SCREEN_H - (rect->y + height * 0.5f);
    return new ButtonList(controlType, loop, layout, centreX, centreY, width, height, 54.0f, /* implicit: Object::operator delete (if the constructor throws) */
                          page->items, count, 1, index, NULL);
    /* (on an exception: Object::operator delete) */
}

/* The table's entry count, .sdata 0x00351088 (10), read by
 * GtmodeControl::begin (T043).  Defined here, after the factories, as T048's
 * g_00350FF4: its place in .sdata, after createButtons' "opel" and before
 * the base list of the two classes' typeinfo (0x00351090), shows it.  Not
 * const, as T037's g_00351294: begin (T043) loads the five counts in
 * retail's order only when the five declarations agree (all const or, as
 * here, T048 / T049 const and T037 / T044 / T045 not; u009 notes). */
int g_00351088 = sizeof(g_00295FB8) / sizeof(g_00295FB8[0]);

/* The factory table (.data 0x00295FB8, 10 entries): zero in the original's
 * data, filled by T044_static_init at boot.  No initialiser, so egcs writes
 * the zeros in .data in front of the frame info, as retail has them. */
MenuFuncData g_00295FB8[10];                /* 0x00295FB8 */

/* One table entry: the inline constructor of the original's MenuFuncData
 * (see T044_static_init). */
static inline void set_menu(MenuFuncData *e, const char *name,
                            ButtonList *(*create)(void *ctx, LinkPage *page,
                                                  MenuLayoutItem *rect, Object *parent,
                                                  int controlType, int loop,
                                                  PageLayout *layout, int index))
{
    e->name = name;
    e->create = create;
}

/* One element as egcs's build_vec_init expands it (u006 / u008's form): the
 * constructor on a copy of the cursor, then the cursor and the count of the
 * elements left stepped. */
#define MENU(name, create)                                                    \
    { MenuFuncData *e = base; set_menu(e, name, create); }                    \
    base++;                                                                   \
    left--

/* The static initialiser (.ctors 0x0028F054): the factory table, sorted by
 * name.  Compiler-generated in the original, as T048's: the table was
 * defined here with MenuFuncData objects built by an inline constructor,
 * which egcs builds at run time (cp/init.c build_vec_init).  Written out the
 * way build_vec_init expands it (a walking cursor, a copy of it per element,
 * the dead count of the elements left): an exact form, not the original; it
 * stores the same ten entries. */
// FUNC 0x0011B6B8 T044_static_init
void T044_static_init(void)
{
    MenuFuncData *base = g_00295FB8;
    int left = 9;                           /* build_vec_init's iterator: elements - 1 */

    MENU("menu_car_goods",     create_menu_car_goods);
    MENU("menu_dealer",        create_menu_dealer);
    MENU("menu_garage",        create_menu_garage);
    MENU("menu_garage_trade",  create_menu_garage_trade);
    MENU("menu_map",           create_menu_map);
    MENU("menu_regulated_car", create_menu_regulated_car);
    MENU("menu_ride_sure",     create_menu_ride_sure);
    MENU("menu_top",           create_menu_top);
    MENU("menu_wheel",         create_menu_wheel);
    MENU("menu_wheel_shop",    create_menu_wheel_shop);
}

// FUNC 0x0011B7D8 gt2k_menu::DealerButtonList::DealerButtonList
/* inline in game/menu/gtmode/T044_gtmode_menu_factories.h; egcs emits this copy (no
 * callers): ButtonList(controlType, loop, layout), the centre (x.set / y.set)
 * and size, the vptrs, DealerButtonList::createButtons, resetIndex,
 * index.set(cursor); ~ButtonList on an exception.  Matching (96.8%, the same
 * under 991111-01): only the callee-saved registers of items, count and rects
 * (retail s0 / s1 / s2, ours s1 / s2 / s0: local-alloc gives the stack
 * argument rects $s0 first).  Tried: the float parameters before relative /
 * cursor (both orders), the body out of the class (in the header and in this
 * file); none changes it. */
/* implicit: ButtonList, createButtons, resetIndex, set, ~ButtonList (the inline constructor in the header) */

#ifndef BOOT_OLD_GCC
/* Builds the __class_type_info __tiQ29gt2k_menu16DealerButtonList (one base,
 * ButtonList: __tfQ29gt2k_menu10ButtonList first; base list g_00351090) on
 * the first call. */
// FUNC 0x0011B8F8 __tfQ29gt2k_menu16DealerButtonList
extern "C" void *__tfQ29gt2k_menu16DealerButtonList(void)
{
    if (__tiQ29gt2k_menu16DealerButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu16DealerButtonList, "Q29gt2k_menu16DealerButtonList",
                     g_00351090, 1);
    }
    return __tiQ29gt2k_menu16DealerButtonList;
}
#endif

// FUNC 0x0011B950 gt2k_menu::DealerButtonList::~DealerButtonList
/* implicit in game/menu/gtmode/T044_gtmode_menu_factories.h; egcs emits this copy for the
 * vtable, after the class's __tf (the frame info's order): both vptrs,
 * ~ButtonList (flags 0), then Object::operator delete if flags & 1. */
/* implicit: ~ButtonList (base destructor), operator delete (deleting destructor) */

// FUNC 0x0011B9B0 gt2k_menu::CarGoodsButtonList::CarGoodsButtonList
/* inline in game/menu/gtmode/T044_gtmode_menu_factories.h; egcs emits this copy (no
 * callers; create_menu_car_goods expands it): as DealerButtonList's, then
 * the cursor on the menu status's car counted among the records that are not
 * locked, else on `cursor'. */
/* implicit: ButtonList, createButtons, resetIndex, getGameStatus, getName, db_car_is_locked, nameToCode, str_equal, set, ~ButtonList (the inline constructor in the header) */

#ifndef BOOT_OLD_GCC
/* The same for CarGoodsButtonList (base list g_00351090). */
// FUNC 0x0011BB60 __tfQ29gt2k_menu18CarGoodsButtonList
extern "C" void *__tfQ29gt2k_menu18CarGoodsButtonList(void)
{
    if (__tiQ29gt2k_menu18CarGoodsButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu18CarGoodsButtonList, "Q29gt2k_menu18CarGoodsButtonList",
                     g_00351090, 1);
    }
    return __tiQ29gt2k_menu18CarGoodsButtonList;
}
#endif

// FUNC 0x0011BBB8 gt2k_menu::CarGoodsButtonList::~CarGoodsButtonList
/* implicit in game/menu/gtmode/T044_gtmode_menu_factories.h; egcs emits this copy (as
 * ~DealerButtonList: both vptrs, ~ButtonList (flags 0), then
 * Object::operator delete if flags & 1). */
/* implicit: ~ButtonList (base destructor), operator delete (deleting destructor) */
