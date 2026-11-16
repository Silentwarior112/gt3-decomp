/* T036_garage.h - the layout hooks of the GT-mode menu and GarageCar
 * (translation unit T036, 0x0011C9C8..0x00125B50: units u010
 * 0x0011C9C8..0x0011F0E0, u011 ..0x001218F0, u012 ..0x001258E8, u013
 * ..0x00125B50; one source file, src/game/menu/gtmode/T036_garage.cpp, since the T036
 * merge).  T036's DecayItem is in T036_decay_item.h.
 *
 * T036_static_init fills g_00296B90 with 221 MenuHook entries (sorted by
 * name); GtmodeControl::begin (T043) gives it to both layouts of its
 * PageManager (PageLayout::setHooks, ctx the GtmodeControl), and
 * PageLayout::createObject (T229) calls create(ctx, layout, parent, item, p8,
 * pC, p10, p14) for an object item whose type is an entry's name (NULL: no
 * object).  The entries are
 *   - the create functions below (make_*: pictures of the garage, shop,
 *     status, licence, event and regulation pages, the 3D car, ...),
 *   - the menu library's hooks: "blink" (T205), "scroll_bar" (T230
 *     create_scroll_bar, p8 = scroll_bar_list), "is_ride_car" (T234, p8 =
 *     is_ride_car, pC 1: tested once), "car_color_box" (T235, p8 / pC =
 *     car_color_box_colors / car_color_box_select) and, for most entries,
 *     create_callback_text (T239: p8 = a text_* callback below, called
 *     fn(ctl, buf, 0x80); pC == 1: once).
 * p10 and p14 are 0 in every entry.  Functions and callbacks are named after
 * their entry: make_<name>, text_<name>.
 *
 * The create functions take the MenuHookFn signature (T229; the first pass
 * had (GtmodeControl *ctl, MenuLayout *layout, void *parent,
 * MenuLayoutElement *elem, ...) with fewer arguments): ctx is the
 * GtmodeControl, item the layout item (u010 / u012's MenuLayoutElement, u011's
 * MenuElement).  The text callbacks take the GtmodeControl as u010 / u012
 * define them (u011's had void *owner); none reads size.
 *
 * The rest of the TU is DecayItem::virtual_4 and the code egcs emits at the
 * end of the unit for the classes whose vtables it writes there: DecayItem's
 * typeinfo function and destructor, the GarageCar / Cell<GarageCar>
 * destructors and typeinfo functions every GarageCar vtable uses (more copies
 * in T053, T080, T096, T427, T429, T490), and unreferenced copies of
 * RegurationResult's (0x001259A0, 0x001259D0; T045_gtmode_page_checks.h). */
#ifndef GAME_T036_GARAGE_H
#define GAME_T036_GARAGE_H

#include "game/game.h"
#include "game/menu/framework/menu_list.h"                 /* base gt2k_menu::Cell<GarageCar> */
#include "game/data/T483_car_information.h"      /* base CarInformation */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (DecayItem's base, when this header
                                               defined it; its includers' class order) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (GtmodeHook, g_00296B90) */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie (MenuMovie) */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class Car;                                  /* T212 */
class ButtonList;                           /* T244 */
class GtmodeControl;                        /* T043 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct OperatorCode;                        /* T217 */
}

/* The first pass's names of library types (u012 GtmodeHook = the hook
 * record, u009 / u013 MenuMovie = the menu background movie g_002CDC10). */
typedef gt2k_menu::MenuHook GtmodeHook;     /* 0x18 bytes: name, create, p8, pC, p10, p14 */
typedef MenuBgMovie MenuMovie;              /* 0x64 bytes (T253; stream at +0x60) */

/* GarageCar: in its own header (the GameStatus and the MenuStatus see it
 * without the rest of this one), included where this header defined it. */
#include "game/menu/gtmode/T036_garage_car.h"

/* gt2k_menu::DecayItem (the item of "slow_decay") is defined in
 * game/menu/gtmode/T036_decay_item.h: T036, its only user, defined it after every other
 * polymorphic class (the order of its type names), so T036_garage.cpp
 * includes that header last. */

/* ---- u010 ---------------------------------------------------------------
 * The garage / shop car is MenuStatus::car, its 3D view MenuStatus::carItem
 * (T427); the ridden car GameStatus::getRunningCar() (T429); the current
 * list ctl->getButtonList(); prices with thousands separators (T494). */
/* "mpeg": a MpegImage (T249) of the item. */
gt2k_menu::Transitive *make_mpeg(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011C9C8 */
void text_user_carname(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011CA40 name 1 of
                                               the ridden car, "" */
void text_user_carname_full(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x0011CA90 names 1 and 2 */
void text_user_day(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x0011CB18 the day
                                               counter (once) */
/* "user_day_unit": an Item of the item showing the day counter with the
 * item's own text as the format; NULL without a text. */
gt2k_menu::Transitive *make_user_day_unit(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011CB60 */
void text_user_money(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x0011CC20 */
void text_focused_carname(gt2k_menu::GtmodeControl *ctl, char *buf, int size);   /* 0x0011CC50 name 1 of
                                               the garage / shop car */
void text_car_buy_price(gt2k_menu::GtmodeControl *ctl, char *buf, int size);     /* 0x0011CC98 "----" with
                                               car flag bit 0 */
void text_car_sell_price(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x0011CD28 a quarter;
                                               "----" with flag bit 1 */
void text_car_kilometrage(gt2k_menu::GtmodeControl *ctl, char *buf, int size);   /* 0x0011CDD0 tenths, in
                                               the distance unit */
/* "car_current_torque" and "disp_ps_befor": the ridden car's power. */
void text_current_power(gt2k_menu::GtmodeControl *ctl, char *buf, int size);     /* 0x0011CE48 */
/* "car_previous_torque": GtmodeControl::prevPower. */
void text_previous_power(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x0011CEB8 */
/* "disp_ps_after" (once): the power of a copy of the ridden car with the
 * focused part (the current list's button) fitted; "----" if it cannot be. */
void text_ps_after(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x0011CF08 */
void text_car_drive_type(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x0011D188 */
void text_car_max_power(gt2k_menu::GtmodeControl *ctl, char *buf, int size);     /* 0x0011D1D0 */
void text_car_max_torque(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x0011D240 */
void text_car_weight(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x0011D2D0 a fixed
                                               placeholder */
/* "goods_car_price": "Cr. <price>" of the car named by the item (up to its
 * last '_'), "----" with car flag bit 0; NULL for an unknown car. */
gt2k_menu::Transitive *make_goods_car_price(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011D2F8 */
/* The status page (GameStatus counters, T429; licence medals, T420). */
void text_status_complete(gt2k_menu::GtmodeControl *ctl, char *buf, int size);       /* 0x0011D460 */
void text_status_ave_prize_car(gt2k_menu::GtmodeControl *ctl, char *buf, int size);  /* 0x0011D4F8 */
void text_status_ttl_wins(gt2k_menu::GtmodeControl *ctl, char *buf, int size);       /* 0x0011D530 */
void text_status_avg_win(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x0011D578 */
void text_status_ttl_races(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011D5C8 */
void text_status_no_cars(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x0011D610 */
void text_status_prize_money(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x0011D660 */
void text_status_no_lc_g(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x0011D690 */
void text_status_no_lc_s(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x0011D6E8 */
void text_status_no_lc_b(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x0011D740 */
void text_status_no_trophy(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011D798 */
void text_status_ttl_car_price(gt2k_menu::GtmodeControl *ctl, char *buf, int size);  /* 0x0011D7E0 */
void text_status_ttl_milage(gt2k_menu::GtmodeControl *ctl, char *buf, int size);     /* 0x0011D818 */
/* "license_mark" / "license_mark_rally": the hidden mark of the highest
 * licence passed / "lc_mark6" once the rally licence is passed, at the
 * item; NULL if none. */
gt2k_menu::Transitive *make_license_mark(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011D890 */
gt2k_menu::Transitive *make_license_mark_rally(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011D9D0 */
/* "user_license": the licence card of the highest licence passed, at the
 * hidden item's own place. */
gt2k_menu::Transitive *make_user_license(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011DAC0 */
/* "need_license": the mark of the licence the focused event needs. */
gt2k_menu::Transitive *make_need_license(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011DBF8 */
/* "disp_license_cup_<licence><n>" (test = licence << 4 | n - 1; licences b
 * 0, a 1, ib 2, ia 3, s 4, r 5): the cup won in that test, NULL if none. */
gt2k_menu::Transitive *make_license_cup(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int test, int pC, int p10, int p14);    /* 0x0011DDA0 */
/* "disp_time_gold" / "_silver" / "_bronze": that time of the licence test
 * named by the item (up to its last '_'), as an Item text. */
gt2k_menu::Transitive *make_time_gold(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011E0A8 */
gt2k_menu::Transitive *make_time_silver(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011E0D0 */
gt2k_menu::Transitive *make_time_bronze(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011E0F8 */
/* "license_car_name" / "license_launch_speed": names 1 and 2 of the test's
 * entry car 0 ("error" if none) / its start speed and the speed unit. */
gt2k_menu::Transitive *make_license_car_name(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011E120 */
gt2k_menu::Transitive *make_license_launch_speed(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011E2C0 */
/* A gt2k_menu::Car (T212) showing car in camera preset view (Car::
 * setCameraMode), placed on the item, its dirt from the car's condition, its
 * wheel getWheelModelName(), its colour; also MenuStatus::carItem.  The
 * item's texture goes through a MenuImageHolder (T226) on the stack.  parent
 * is not used.  (Not a hook: the car_render_* hooks call it.) */
gt2k_menu::Car *make_car_render(GarageCar *car, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item, int view); /* 0x0011E428 */
/* "car_render_car_wash" (8), "car_render_mine" / "_mine_side" (5),
 * "car_render_oil_change" (10), "car_render_wheel_shop" (7): the ridden
 * car, which also becomes MenuStatus::car; without one carItem = NULL. */
gt2k_menu::Transitive *make_car_render_current(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int view, int pC, int p10, int p14);    /* 0x0011E630 */
/* "car_render_wheel" (6): MenuStatus::car (the code of make_car_render_focused). */
gt2k_menu::Transitive *make_car_render_wheel(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int view, int pC, int p10, int p14);    /* 0x0011E700 */
/* "car_render_from" / "car_render_shop" (4), "car_render_garage" (5):
 * MenuStatus::car. */
gt2k_menu::Transitive *make_car_render_focused(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int view, int pC, int p10, int p14);    /* 0x0011E770 */
/* "car_render_buy" (11): MenuStatus::car in MenuStatus::color, at z 300. */
gt2k_menu::Transitive *make_car_render_buy(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int view, int pC, int p10, int p14);    /* 0x0011E7E0 */
/* "car_logo": the hidden item named after MenuStatus::car's code, at its
 * own place; NULL if none. */
gt2k_menu::Transitive *make_car_logo(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0011E888 */
/* "car_color_box": the colours of the car on show | 0x80000000, in the
 * static table g_0049BC20 (T036 .bss), and their count; the setter repaints
 * MenuStatus::carItem and sets MenuStatus::color. */
u32 *car_color_box_colors(int *count);      /* 0x0011E958 */
void car_color_box_select(gt2k_menu::GtmodeControl *ctl, int color);             /* 0x0011E9A8 */
void text_car_color_name(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x0011EA08 " " for a
                                               name starting with '-' */
void text_color_name_box(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x0011EA90 */
/* "scroll_bar": the current list (ctl->getButtonList(), expanded). */
gt2k_menu::ButtonList *scroll_bar_list(gt2k_menu::GtmodeControl *ctl);          /* 0x0011EB18 */
void text_focused_partsname(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x0011EB30 */
void text_focused_wheel_name(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x0011EB78 a placeholder
                                               (EUC-JP "hoge-hoge wheel") */

/* ---- u010 / u011: "price_<part>" -----------------------------------------
 * For the ridden car: "購入済" (purchased, g_003512C8[0]) when it owns the
 * part, else the part's price, else "----" (no car, or the car has no such
 * part).  One body (T036_PRICE_TEXT below) per part id, in parentheses. */

/* The body of every "price_<part>" text: 0xA4 bytes of the same code per
 * part, the part id the only difference (? an inline function or a macro of
 * the original).  A macro here because the headers of its callees include
 * this one (T429 getGameStatus / GameStatus::getRunningCar); the users
 * include them and T039 (g_003512C8), T494 (format_number), G007 (strcpy).
 * Calls: getGameStatus, GameStatus::getRunningCar, CarInformation::hasPart /
 * getPartPrice, format_number, strcpy ("----" is inlined as a 5-byte copy).
 * It returns from the (void) text callback whose whole body it is: the
 * original's early returns (u011 matching pass).  As `break' out of a
 * `do { } while (0)', egcs aligns the first label inside the phony loop
 * (getPartPrice's): one nop more than retail (95.8%); as a block, exact. */
#define T036_PRICE_TEXT(buf, part)                                          \
    {                                                                       \
        GarageCar *car_ = getGameStatus()->getRunningCar();                 \
        if (car_ != NULL) {                                                 \
            if (car_->hasPart(part) != 0) {                                 \
                strcpy((buf), g_003512C8[0]);                               \
                return;                                                     \
            }                                                               \
            int price_ = car_->getPartPrice(part);                          \
            if (price_ >= 0) {                                              \
                format_number((buf), price_);                               \
                return;                                                     \
            }                                                               \
        }                                                                   \
        strcpy((buf), "----");                                              \
    }

void text_price_muf_sports(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011EBA0 (0x25) */
void text_price_muf_semi_racing(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x0011EC48 (0x26) */
void text_price_muf_racing(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011ECF0 (0x27) */
void text_price_cpu_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x0011ED98 (0x09) */
void text_price_ptp_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x0011EE40 (0x2D) */
void text_price_egb_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x0011EEE8 (0x0E) */
void text_price_nat_1(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x0011EF90 (0x29) */
void text_price_nat_2(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x0011F038 (0x2A) */
void text_price_nat_3(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x0011F0E0 (0x2B) */
void text_price_dsp_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x0011F188 (0x0B) */
void text_price_tbk_1(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x0011F230 (0x4F) */
void text_price_tbk_2(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x0011F2D8 (0x50) */
void text_price_tbk_3(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x0011F380 (0x51) */
void text_price_tbk_4(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x0011F428 (0x52) */
void text_price_icl_sports(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011F4D0 (0x18) */
void text_price_icl_racing_big(gt2k_menu::GtmodeControl *ctl, char *buf, int size);  /* 0x0011F578 (0x19) */
void text_price_sus_sports(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011F620 (0x33) */
void text_price_sus_semi_racing(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x0011F6C8 (0x34) */
void text_price_sus_racing(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011F770 (0x35) */
void text_price_bkt_racing(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011F818 (0x01) */
void text_price_bbl_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x0011F8C0 (0x03) */
void text_price_tms_close(gt2k_menu::GtmodeControl *ctl, char *buf, int size);       /* 0x0011F968 (0x14) */
void text_price_tms_super_close(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x0011FA10 (0x15) */
void text_price_tms_variable(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x0011FAB8 (0x16) */
void text_price_clt_high(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x0011FB60 (0x05) */
void text_price_clt_twin(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x0011FC08 (0x06) */
void text_price_clt_triple(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x0011FCB0 (0x07) */
void text_price_fly_light(gt2k_menu::GtmodeControl *ctl, char *buf, int size);       /* 0x0011FD58 (0x10) */
void text_price_fly_cr_mo(gt2k_menu::GtmodeControl *ctl, char *buf, int size);       /* 0x0011FE00 (0x11) */
void text_price_fly_light_cr_mo(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x0011FEA8 (0x12) */
void text_price_prp_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x0011FF50 (0x2F) */
void text_price_lsd_1(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x0011FFF8 (0x1F) */
void text_price_lsd_15(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x001200A0 (0x21) */
void text_price_lsd_2(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x00120148 (0x20) */
void text_price_lsd_variable(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x001201F0 (0x22) */
void text_price_tir_control(gt2k_menu::GtmodeControl *ctl, char *buf, int size);     /* 0x00120298 (0x41) */
void text_price_tir_dirt(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x00120340 (0x42) */
void text_price_tir_sports(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x001203E8 (0x39) */
void text_price_tir_super_hard(gt2k_menu::GtmodeControl *ctl, char *buf, int size);  /* 0x00120490 (0x3A) */
void text_price_tir_hard(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x00120538 (0x3B) */
void text_price_tir_medium_hard(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x001205E0 (0x3C) */
void text_price_tir_medium(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x00120688 (0x3D) */
void text_price_tir_medium_soft(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x00120730 (0x3E) */
void text_price_tir_soft(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x001207D8 (0x3F) */
void text_price_tir_super_soft(gt2k_menu::GtmodeControl *ctl, char *buf, int size);  /* 0x00120880 (0x40) */
void text_price_wtr_1(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x00120928 (0x1B) */
void text_price_wtr_2(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x001209D0 (0x1C) */
void text_price_wtr_3(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x00120A78 (0x1D) */
void text_price_ayc_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x00120B20 (0x23) */
void text_price_tcs_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x00120BC8 (0x37) */
void text_price_sms_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x00120C70 (0x54) */
void text_price_vcd_on(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x00120D18 (0x56) */
/* The services, priced by name in the T435 data base. */
void text_price_oil_change(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x00120DC0 "oil_change" */
void text_price_car_wash(gt2k_menu::GtmodeControl *ctl, char *buf, int size);        /* 0x00120E00 "car_wash" */
void text_price_wheel(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x00120E40 "wheel" */

/* ---- u011: the event and regulation pages ---------------------------------
 * "The focused event": the name of the current list's focused record
 * (ButtonList::focusData()). */
/* "race_item": the hidden item named after MenuStatus::event, as an Item
 * (NULL if none). */
gt2k_menu::Transitive *make_race_item(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00120E80 */
/* Whether an event is open: GameStatus::isLicenseSPassed() (0 / 1) >= the
 * product database's requirement of it (ProductConfig::eventValue, T435).
 * EventHider::isOpen. */
int  event_unlocked(const char *event);     /* 0x00120F30 */
/* "event_hide": an EventHider (T042) over the current list. */
gt2k_menu::Transitive *make_event_hide(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00120F80 */
/* "event_flyer" / "event_text": the item, or the hidden "hide_plate" /
 * "hide_text" while the focused event is closed. */
gt2k_menu::Transitive *make_event_flyer(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00121038 */
gt2k_menu::Transitive *make_event_text(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00121130 */
/* "event_status": an EventStatusObject (T041) of the focused event. */
gt2k_menu::Transitive *make_event_status(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00121228 */
/* "event_status_button01" .. "11": an EventStatusButtonObject (T041) of the
 * button in cell `cell' of the current list. */
gt2k_menu::Transitive *make_event_status_button(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int cell, int pC, int p10, int p14);    /* 0x001212E0 */
/* "arrow_dec" (dec 1) / "arrow_inc" (0): an Arrow (T210) of the current list. */
gt2k_menu::Transitive *make_arrow(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int dec, int pC, int p10, int p14);     /* 0x001213A0 */
/* "event_course_doc" / "event_course_logo": the hidden picture
 * "<course>_doc" / "<course>" of the focused event's course, at the item. */
gt2k_menu::Transitive *make_event_course_doc(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00121450 */
gt2k_menu::Transitive *make_event_course_logo(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00121598 */
/* "regulation_license": the icon of the licence MenuStatus::event needs,
 * its text red (Item::textColor 0x804040FF) when the player has not passed
 * it. */
gt2k_menu::Transitive *make_regulation_license(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001216D0 */

/* ---- u012 ---------------------------------------------------------------
 * The regulation icons of MenuStatus::event, red (Item::textColor
 * 0x804040FF) when the ridden car breaks the rule (or none is ridden). */
gt2k_menu::Transitive *make_regulation_power(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001218F0 always "power_unlimited" */
gt2k_menu::Transitive *make_regulation_tuning(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001219F0 */
gt2k_menu::Transitive *make_regulation_dr_type(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00121BB8 */
gt2k_menu::Transitive *make_regulation_aspiration(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00121D98 */
gt2k_menu::Transitive *make_regulation_tire(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00121F38 RegurationResult::flags[0] */
/* "regulation_car": "car_limited" when the race allows only some cars (red
 * unless the ridden car is one), else "car_unlimited". */
gt2k_menu::Transitive *make_regulation_car(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001220E8 */
/* "violated_license": the picture of the licence the race needs ("error"
 * for none), never red. */
gt2k_menu::Transitive *make_violated_license(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00122260 */
/* "spec_*": the spec sheet.  Most read the catalogue data of MenuStatus::car
 * (its CarEquipments); "spec_max_power_real" its GarageCar::getPower; the
 * ratios, the treads, the weight and "spec_max_torque_real" the car on show
 * (T232 menu_car_spec(), g_00352644, a ShowRoomCar record).  "_real": the
 * figures computed for the car, else the catalogue's. */
void text_spec_aspiration(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x001223A8 */
void text_spec_displacement(gt2k_menu::GtmodeControl *ctl, char *buf, int size);    /* 0x001223F0 also
                                               "spec_displacement_real" */
void text_spec_drive_type(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x00122450 */
void text_spec_engine_type(gt2k_menu::GtmodeControl *ctl, char *buf, int size);     /* 0x00122498 */
void text_spec_height(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x00122518 */
void text_spec_length(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x00122578 */
void text_spec_max_power(gt2k_menu::GtmodeControl *ctl, char *buf, int size);       /* 0x001225D8 */
void text_spec_max_power_real(gt2k_menu::GtmodeControl *ctl, char *buf, int size);  /* 0x001226A0 */
void text_spec_max_torque(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x00122720 */
void text_spec_max_torque_real(gt2k_menu::GtmodeControl *ctl, char *buf, int size); /* 0x001227E8 */
void text_spec_p_w_ratio(gt2k_menu::GtmodeControl *ctl, char *buf, int size);       /* 0x00122878 */
void text_spec_p_w_ratio_real(gt2k_menu::GtmodeControl *ctl, char *buf, int size);  /* 0x00122908 */
void text_spec_t_w_ratio(gt2k_menu::GtmodeControl *ctl, char *buf, int size);       /* 0x00122998 */
void text_spec_t_w_ratio_real(gt2k_menu::GtmodeControl *ctl, char *buf, int size);  /* 0x00122A30 */
void text_spec_tread_f(gt2k_menu::GtmodeControl *ctl, char *buf, int size);         /* 0x00122AC8 */
void text_spec_tread_r(gt2k_menu::GtmodeControl *ctl, char *buf, int size);         /* 0x00122B00 */
void text_spec_weight(gt2k_menu::GtmodeControl *ctl, char *buf, int size);          /* 0x00122B38 */
void text_spec_wheel_base(gt2k_menu::GtmodeControl *ctl, char *buf, int size);      /* 0x00122B70 */
void text_spec_width(gt2k_menu::GtmodeControl *ctl, char *buf, int size);           /* 0x00122BD0 */
/* "race_prize_1" .. "6": the prize of that place in MenuStatus::event. */
void text_race_prize_1(gt2k_menu::GtmodeControl *ctl, char *buf, int size);         /* 0x00122C30 */
void text_race_prize_2(gt2k_menu::GtmodeControl *ctl, char *buf, int size);         /* 0x00122CD8 */
void text_race_prize_3(gt2k_menu::GtmodeControl *ctl, char *buf, int size);         /* 0x00122D80 */
void text_race_prize_4(gt2k_menu::GtmodeControl *ctl, char *buf, int size);         /* 0x00122E28 */
void text_race_prize_5(gt2k_menu::GtmodeControl *ctl, char *buf, int size);         /* 0x00122ED0 */
void text_race_prize_6(gt2k_menu::GtmodeControl *ctl, char *buf, int size);         /* 0x00122F78 */
/* "ignore_control": no object; the PageManager's setFreeze(1) (the
 * OilChangeCylinder sets it back when its animation ends). */
gt2k_menu::Transitive *make_ignore_control(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00123020 */
/* "oil_change_cylinder": an OilChangeCylinder (T040) as dirty as
 * MenuStatus::car's oil; also GtmodeControl::oilCylinder. */
gt2k_menu::Transitive *make_oil_change_cylinder(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00123048 */
/* "slow_decay": the item as a DecayItem. */
gt2k_menu::Transitive *make_slow_decay(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00123110 */
/* "tuner_logo": the hidden picture named after the ridden car's maker. */
gt2k_menu::Transitive *make_tuner_logo(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00123198 */
int  is_ride_car(gt2k_menu::GtmodeControl *ctl);    /* 0x00123248 "is_ride_car": a car is ridden */

/* T036 .data 0x00296B90..0x00298048: {"arrow_dec", make_arrow, 1, 0, 0, 0}
 * .. {"violated_license", make_violated_license, 0, 0, 0, 0}.  Filled by
 * T036_static_init (.ctors 0x0028F048; compiler-generated in the original:
 * the function pointers cast to int are not constant initialisers). */
extern gt2k_menu::MenuHook g_00296B90[221]; /* 0x00296B90 */
extern int g_00351278;                      /* 0x00351278 gp-0x6778 its entries (221; defined in
                                               T036_garage.cpp, .sdata) */
void T036_static_init(void);                /* 0x00123270 */

/* (g_003512C8, the texts of T039's "GTMODE" table, which the plan put here,
 * is declared in T039_gtmode_messages.h.) */

#ifndef BOOT_OLD_GCC                        /* egcs generates and places them itself */
extern "C" {
void *__tf9GarageCar(void);                        /* 0x00125A38 (copies in T053, T080, T096, T427,
                                                       T429, T490) */
void *__tfQ29gt2k_menut4Cell1Z9GarageCar(void);     /* 0x00125B00 (si, base pCell; the same
                                                       copies) */
/* type_info storage of this header's GarageCar and Cell<GarageCar> (T517's
 * GT2KRealTimeText node is declared in T517, T220's Object node in T220,
 * T495's pCell node in T495, T483's CarInformation node in T483); filled by
 * the __tf functions. */
extern u32 __ti9GarageCar[4];                       /* 0x004B1230 __class_type_info */
extern u32 __tiQ29gt2k_menut4Cell1Z9GarageCar[3];   /* 0x004B1350 __si_type_info */
}
#endif

#endif /* GAME_T036_GARAGE_H */
