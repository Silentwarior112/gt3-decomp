/* T107_cars_item.h - gt2k_menu::TheCar, gt2k_menu::CarsItem and
 * gt2k_menu::CarGetMessageItem: the cars of the prize car screen
 * ("render_model" and "car_get_message" of menu/car_get.mbl, T061)
 * (translation unit T107, unit u021), 0x0013C8F0..0x0013E918.
 *
 * CarsItem holds the cars of one prize set (1..4) on the turntable of its
 * owner (CarGetControl::turntable, T106) and runs the screen's show
 * (virtual_1, state):
 *   0  timer = 210 frames (one car) or 120;
 *   1  waits; one car: straight to 3, else the roulette starts (60 frames);
 *   2  waits, then the roulette stops on the car won (picked by the cars'
 *      weights in the constructor);
 *   3  waits for it to stop;
 *   4  the car won's highlight fades (1 -> 0), then the table slows down;
 *   5  the other cars sink to -2 * turntable.maxHeight, then the message
 *      shows (owner->carsShown) and the page is unfrozen;
 *   6  done.
 * While the other cars are still visible virtual_2 draws them all
 * (drawAll), then only the car won, lit by the camera's light set
 * (drawSelected).  Both draw into a colour buffer of their own over a copy
 * of the screen and blend it back (GSBuffer::blitField, 0.8).  In drawAll
 * the car won comes last, after the screen as it was before the cars is
 * drawn over the others with 1 - highlight (so they fade out in state 4),
 * and the cars reflect a black 32 x 32 texture; in drawSelected the car won
 * reflects the first texture of tex ("menu/prize_get.img"), its
 * environment map fading in with fade.  The prize car scene's lights are
 * T107 .bss (g_0049BE50..g_0049BE80, file-local). */
#ifndef GAME_T107_CARS_ITEM_H
#define GAME_T107_CARS_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/gtmode/T106_car_get_camera.h"       /* gt2k_menu::CarRoulette (roulette), CarGetMenuCamera (camera) */
#include "game/car/model/T446_car_model.h"            /* CarModel (TheCar::body) */
#include "game/car/model/T450_menu_tire_model.h"      /* MenuTireModel (TheCar::tires) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new: CarsItem::create) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (getTexture), MenuLayoutItem */
#include "game/menu/gtmode/T061_car_get_control.h"      /* gt2k_menu::CarGetControl (CarsItem::create reads it;
                                               defined before TheCar, as retail's type names show) */

struct Tex1Image;                           /* G001 (pointers only) */
struct CarModelFile;                        /* T456 */
class MenuCarCamera;                        /* T411 (via T106) */

namespace gt2k_menu {
class PageManager;                          /* T211 */
class Transitive;                           /* T216 */
struct MenuTexture;                         /* T213 */

/* One car of the prize screen: its model file, body and tyres.  vtable
 * 0x00319430 (1 slot).  typeinfo __tfQ29gt2k_menu6TheCar 0x0013E8D8 (no
 * bases; __user_type_info 0x00353348 gp-0x46A8).  0x368 bytes of data,
 * vptr +0x368; size 0x36C (the stride of CarsItem::cars). */
class TheCar {
public:
    /* Inline (CarsItem's constructor expands it). */
    TheCar() : code(NULL), model(NULL), alpha(0.0f), selected(0) {}
    /* [0] +0x0C 0x0013E820 the tyres (through their vtable), ~CarModel.
     * Inline: TheCar has no key function (its type name is in T061 too,
     * where nothing else of TheCar is); T107 emits the out-of-line copy
     * with the vtable. */
    virtual ~TheCar() {}
    /* Takes the model file of car `code' (relocated), the body, the four
     * tyres (sizes from the car's MenuCarSpec; their data after the file)
     * and the colour. */
    void setup(const char *code, CarModelFile *model, int color);  /* 0x0013C8F0 */
    void updateAlpha();                     /* 0x0013CA30 alpha 1 while selected, else fades */
    /* Light 0 of the showroom record of cam's mode (showroom_index, T215), its
     * colours times fade, and the CPU lighting on (this unused). */
    void setLights(MenuCarCamera *cam, f32 fade);   /* 0x0013CA78 */
    void resetLights();                     /* 0x0013CC20 the CPU lighting and light 0 off (this unused) */
    void F_0013CC48();                      /* 0x0013CC48 body pass 6 in black, then in the shadow colour */
    void drawBody();                        /* 0x0013CC78 raised so that the wheels touch the floor */
    void drawTires();                       /* 0x0013CCF8 at the MenuCarSpec wheel positions */
    /* Inline (CarsItem::virtual_1: retail computes the car's address for
     * this store apart from the one of updateAlpha's call). */
    void setSelected(u8 on) { selected = on; }
    /* Inline (defined in T107, its only user: setup, drawBody,
     * CarsItem::setupLayout): menu_car_spec()->setCar(code). */
    inline void selectSpec();

    const char   *code;                     /* 0x000 car code */
    CarModelFile *model;                    /* 0x004 */
    CarModel      body;                     /* 0x008 */
    MenuTireModel tires[4];                 /* 0x040 */
    f32           alpha;                    /* 0x360 of its floor image: 1 while selected, then fades */
    u8            selected;                 /* 0x364 the roulette points at it */
    /*            vptr                         0x368 */
};
EGCS_SIZE(gt2k_menu::TheCar, 0x36C);

/* "render_model": the cars of the owner's prize set.  vtable 0x003193E8
 * (8 slots: [3] .. [7] inherited).  typeinfo __tfQ29gt2k_menu8CarsItem
 * 0x0013E7D0 (si, base Item; storage 0x004B1860).  Size 0x1048 (operator
 * new in CarsItem::create). */
class CarsItem : public Item {
public:
    /* n cars from owner's entries; picks the car won (a random point of
     * the sum of the weights); freezes pages; camera light set 9.  ctl and
     * owner are both the owner. */
    CarsItem(int n, MenuLayoutItem *res, MenuTexture *tex, PageManager *pages,
             CarGetControl *ctl, Tex1Image *image, CarGetControl *owner);  /* 0x0013CE58 */
    virtual void virtual_1();               /* [1] +0x14 0x0013D498 update: Item's, the show, the
                                               roulette, the turntable, the camera */
    virtual void virtual_2();               /* [2] +0x1C 0x0013D750 draw: drawAll while the others are
                                               visible, then drawSelected */

    void setupLayout();                     /* 0x0013D368 the turntable from the cars' sizes */
    int  othersGone();                      /* 0x0013D468 */
    int  othersVisible();                   /* 0x0013D730 !othersGone() */
    void drawFloor(s8 i, f32 a, int place); /* 0x0013D798 the floor picture under car i (the float
                                               before place: the callers' argument order) */
    void drawAll();                         /* 0x0013D938 */
    void drawSelected();                    /* 0x0013DF30 */

    /* The three inline members, in this order: virtual_1 is the key
     * function, so T107 writes every inline member of CarsItem out of line
     * at the end of the unit, in the reverse order of their bodies (create
     * 0x0013E638, ~CarsItem 0x0013E710, getFade 0x0013E7C8, then the
     * typeinfo function: retail T107's function order and frame info). */
    /* 0x0013E7C8 drawSelected reads fade through it (retail's registers for
     * the ambient colour); the out-of-line copy has no callers. */
    f32 getFade() { return fade; }
    /* [0] +0x0C 0x0013E710 ~CameraBase (camera), the cars (through their
     * vtable), ~Item; Object::operator delete. */
    virtual ~CarsItem() {}
    /* "render_model" of car_get.mbl (T061's hook table; ctx: the
     * CarGetControl): Object::operator new(0x1048), a CarsItem of the
     * owner's cars and environment map, its turntable set up from the cars'
     * sizes.  The landing pad gives the block back with Object::operator
     * delete.  parent and p8 .. p14 are not used.  0x0013E638 (the first
     * pass's create_cars_item: a static member, as its place among
     * CarsItem's inline members shows). */
    static Transitive *create(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                              int p8, int pC, int p10, int p14)
    {
        CarGetControl *owner = (CarGetControl *)ctx;
        PageManager *pages = &owner->pages;
        int n = owner->count;
        Tex1Image *envImage = owner->texBuf;
        MenuTexture *tex = layout->getTexture(item);
        CarsItem *cars = new CarsItem(n, item, tex, pages, owner, envImage, owner); /* implicit: Object::operator delete (if the constructor throws) */
        cars->setupLayout();
        return cars;
    }

    /*               Item                      0x000 .. 0x100 */
    CarRoulette      roulette;              /* 0x100 */
    TheCar           cars[4];               /* 0x11C */
    CarGetControl   *ctl;                   /* 0xECC (= owner) */
    PageManager     *pages;                 /* 0xED0 the owner's (frozen while the roulette runs) */
    CarGetMenuCamera camera;                /* 0xED4 */
    Tex1Image       *tex;                   /* 0x1028 "menu/prize_get.img" (owner->texBuf) */
                                            /* (the car won's environment map, drawSelected) */
    CarGetControl   *owner;                 /* 0x102C */
    int              timer;                 /* 0x1030 frames */
    f32              highlight;             /* 0x1034 1 -> 0 in state 4; the car won's overlay: 1 - it */
    f32              fade;                  /* 0x1038 light of the car won, 0 -> 1 once the others are gone */
    f32              drop;                  /* 0x103C height of the other cars, 0 -> -2 * maxHeight */
    int              unk_1040;              /* 0x1040 */
    s8               count;                 /* 0x1044 */
    s8               state;                 /* 0x1045 0 .. 6 */
};
EGCS_SIZE(gt2k_menu::CarsItem, 0x1048);

/* The "eventname" item of T061: the hidden picture named owner->eventName,
 * centred on the layout item.  vtable 0x00319CB8 (8 slots: [1] .. [7]
 * inherited).  typeinfo __tfQ29gt2k_menu13EventNameItem 0x0013F8A8 (si,
 * base Item; storage 0x004B1850).  Size 0x100.  Defined here, between
 * CarsItem and CarGetMessageItem, not in T061_car_get_control.h: the
 * original defined the car_get classes in the order CarGetControl,
 * TheCar, CarsItem, EventNameItem, CarGetMessageItem (the type names of
 * T107 and T061, written in the reverse order, show it), and only T107 and
 * T061 saw them. */
class EventNameItem : public Item {
public:
    /* Inline (create_event_name expands it). */
    EventNameItem(const char *name, PageLayout *layout, MenuLayoutItem *pos)
        : Item(name, layout, pos) {}
    /* [0] +0x0C 0x0013F8F8 the implicit destructor (no vptr store): ~Item, then
     * Object::operator delete. */
};
EGCS_SIZE(gt2k_menu::EventNameItem, 0x100);

/* "car_get_message": an Item that updates and draws only once owner's
 * carsShown is set.  vtable 0x003193A0 (8 slots: [3] .. [7] inherited).
 * typeinfo __tfQ29gt2k_menu17CarGetMessageItem 0x0013E5A0 (si, base Item;
 * storage 0x004B1830).  Size 0x104 (operator new in
 * create_car_get_message). */
class CarGetMessageItem : public Item {
public:
    /* Inline (create_car_get_message expands it); 0x0013E550 is an
     * out-of-line copy without callers. */
    CarGetMessageItem(CarGetControl *o, MenuLayoutItem *res, MenuTexture *tex)
        : Item(res, tex), owner(o) {}       /* 0x0013E550 */
    /* [0] +0x0C 0x0013E5F0 the implicit destructor (no vptr store): ~Item, then
     * Object::operator delete. */
    virtual void virtual_1();               /* [1] +0x14 0x0013E438 Item's once owner->carsShown */
    virtual void virtual_2();               /* [2] +0x1C 0x0013E470 the same */

    /*             Item                        0x000 .. 0x100 */
    CarGetControl *owner;                   /* 0x100 */
};
EGCS_SIZE(gt2k_menu::CarGetMessageItem, 0x104);

} /* namespace gt2k_menu */

/* The item factories of car_get.mbl in T107 (T061's hook table g_0049BE90;
 * ctx: the CarGetControl).  "car_get_message": a CarGetMessageItem of the
 * item; "render_model" is CarsItem::create (above). */
gt2k_menu::Transitive *create_car_get_message(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0013E4A8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu17CarGetMessageItem(void);    /* 0x0013E5A0 */
void *__tfQ29gt2k_menu8CarsItem(void);              /* 0x0013E7D0 */
void *__tfQ29gt2k_menu6TheCar(void);                /* 0x0013E8D8 */
void *__tfQ29gt2k_menu13EventNameItem(void);        /* 0x0013F8A8 (T061) */
}
#endif

#endif /* GAME_T107_CARS_ITEM_H */
