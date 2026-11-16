/* T061_car_get_control.h - gt2k_menu::CarGetControl, the prize car screen
 * ("menu/car_get.mbl", page "car_get"), and the loaded car model file
 * (translation unit T061, unit u021), 0x0013E918..0x0013F940.
 *
 * F_0013F508 (after the races of the arcade and GT modes) runs
 * car_get_prizes, which asks T419 for the prize sets the race gives and, for
 * each one whose reason is valid, loads the models of the set's cars
 * (car_get_load_car: up to four, 0xC8000 bytes each) and runs a
 * CarGetControl over them on a 512 KB menu heap; the car the roulette stops
 * on goes to the garage (a set of one car is also marked as won).  With 200
 * cars in the garage, T105's not_car_get shows instead.  Then, once each,
 * the ending movies.
 *
 * Also in T061: the item table of car_get.mbl (g_0049BE90: "blink",
 * "car_get_message" and "render_model" of T107, "eventname"), and
 * unreferenced copies of ControlBaseWithPageManager's destructor and typeinfo
 * function (0x0013F888 / 0x0013F838, declared with the class in T056).
 *
 * The plan puts a shared record here:
 *   - gt2k_menu::PageList, which is T229's PageLayout (the first pass's
 *     name in u007, u019..u021, u026, u028, u030..u032): a typedef. */
#ifndef GAME_T061_CAR_GET_CONTROL_H
#define GAME_T061_CAR_GET_CONTROL_H

#include "game/game.h"
/* The prize classes the originals T061 / T107 saw before CarGetControl (their
 * type names, in reverse definition order): PrizeBase's (T460), then
 * ControlBaseWithPageManager and PrizeControlBase (T059's base part, which
 * includes T056's base part); none of T056's / T059's screens. */
#include "game/menu/gtmode/T460_prize_base.h"           /* PrizeModelSetContext, CallBackWhenPrizeRendering */
#define T059_BASE_ONLY
#include "game/menu/gtmode/T059_license_get_control.h"  /* base gt2k_menu::ControlBaseWithPageManager (T056's base
                                               part), gt2k_menu::PrizeControlBase */
#undef T059_BASE_ONLY
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/gtmode/T106_car_get_camera.h"       /* gt2k_menu::CarTurntable (turntable) */
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (PageList), MenuHook (g_0049BE90) */

struct Tex1Image;                           /* G001 (pointers only) */
struct CarModelFile;                        /* T456_car_model_file.h */

namespace gt2k_menu {

/* The first pass's name of T229's PageLayout (the item / hook tables at
 * +0x60 / +0x6C / +0x78 are setHooks / setSliderFuncs / setMultiFuncs; its
 * F_001C2998 is buildLayer).  New code should use PageLayout. */
typedef PageLayout PageList;

/* One car of a prize set, as car_get_load_car fills it.  (? name) */
struct CarGetEntry {
    const char   *code;                     /* 0x00 car code */
    const char   *name;                     /* 0x04 the car's name (CarEquipments) */
    CarModelFile *model;                    /* 0x08 its model, in a 0xC8000-byte buffer */
    int           color;                    /* 0x0C colour index */
    int           colorId;                  /* 0x10 CarEquipments +0x08: the colour the garage gets */

    /* Member by member (retail copies the entries into CarGetControl word
     * by word, interleaved, where a plain struct copy gives ldl/ldr pairs). */
    CarGetEntry &operator=(const CarGetEntry &o)
    {
        code = o.code;
        name = o.name;
        model = o.model;
        color = o.color;
        colorId = o.colorId;
        return *this;
    }
};
EGCS_SIZE(gt2k_menu::CarGetEntry, 0x14);

/* The image name of the "eventname" item: why the car is given (reason 0
 * "solo_race_clear" .. 22 "arcade_complusoly_complete" (sic), jump table
 * 0x00319880); other reasons give "". */
const char *car_get_event_name(const s8 *reason);  /* 0x0013EB10 */

/* vtable 0x00319990 (2 slots: [1] ControlBase::waitFrame).  typeinfo
 * __tfQ29gt2k_menu13CarGetControl 0x0013F798 (si, base
 * ControlBaseWithPageManager; storage 0x004B1840).  Size 0xED8
 * (sp+0xA0 .. sp+0xF78 in car_get_prizes, which expands the inline
 * constructor and the destructor). */
class CarGetControl : public ControlBaseWithPageManager {
public:
    /* The reason by reference (retail's car_get_prizes gives `reason' its
     * stack slot after the controller's: the address is taken when the
     * constructor's arguments are converted), unk the -1 that retail holds
     * in a register of its own from the constructor's start. */
    CarGetControl(int n, const CarGetEntry *src, Tex1Image *image, f32 *w, const s8 &reason, s8 unk = -1)
        : render(), count(n), selected(0), texBuf(image), weights(w),
          eventName(car_get_event_name(&reason)), unk_ED4(unk)
    {
        turntable.count = n;
        turntable.stopping = 0;
        carsShown = 0;
        done = 0;
        for (int i = 0; i < n; i++)
            entries[i] = src[i];
    }
    /* [0] +0x0C 0x0013F7E8 the implicit destructor (no vptr store): render's
     * (~RootObject, GameObject::~GameObject), then ~ControlBase. */

    void load();                            /* 0x0013EC58 "menu/car_get.mbl" */
    void unload();                          /* 0x0013EC80 */
    void begin();                           /* 0x0013ECA0 the render (mask off, a transition), the item
                                               table g_0049BE90 on both layouts, no actions */
    void end();                             /* 0x0013ED68 */
    void mainLoop();                        /* 0x0013EDB0 page "car_get" until it is left or done; once
                                               carsShown, layer "after" of the current layout */
    void run();                             /* 0x0013EEF8 load, begin, mainLoop, end, unload */

    /*          ControlBaseWithPageManager     0x000 .. 0xDE8 */
    RenderBase   render;                    /* 0xDE8 */
    CarGetEntry  entries[4];                /* 0xE40 */
    int          count;                     /* 0xE90 the cars of the set */
    int          selected;                  /* 0xE94 the car the roulette shows (CarsItem::virtual_1) */
                                            /* (at the end the car won) */
    Tex1Image   *texBuf;                    /* 0xE98 "menu/prize_get.img" */
                                            /* (CarsItem::tex: the car won's environment map) */
    f32         *weights;                   /* 0xE9C the cars' chances (>= 1) */
    const char  *eventName;                 /* 0xEA0 the image of the "eventname" item */
    CarTurntable turntable;                 /* 0xEA4 */
    s8           unk_ED4;                   /* 0xED4 (-1) */
    /* 0xED5 carsShown: set by CarsItem once the others have sunk (then
     * layer "after", the message); 0xED6 done: ends the main loop (not set
     * in T061).  8-bit fields of an unsigned int (bit-fields), as
     * CarGetMenuCamera::next (T106): their stores are not char accesses
     * (CarsItem::virtual_1 loads `pages' after the carsShown store, as
     * retail); the loads stay lbu. */
    unsigned int carsShown : 8;             /* (no offsetof for a bit-field) */
    unsigned int done : 8;                  /* (no offsetof for a bit-field) */
};
EGCS_SIZE(gt2k_menu::CarGetControl, 0xED8);

/* EventNameItem (the "eventname" item, whose functions T061 holds) is
 * defined in T107_cars_item.h, after CarsItem: see there. */

} /* namespace gt2k_menu */

/* Loads car `code' of a prize set into buf (its model file, relocated) and
 * fills out: the colour is `color' if it is one of the model's colours,
 * else a random one (also for color 0xFFFFFFFF, none: unsigned, retail
 * compares it with a lui/ori 0xFFFFFFFF).  Returns code, or NULL if the car
 * does not exist. */
const char *car_get_load_car(const char *code, CarModelFile *buf, u32 color,
                             gt2k_menu::CarGetEntry *out);  /* 0x0013E918 */
/* The prize car screens of race `race' (arg: passed on to T419's
 * PrizeCarList).  0 if there is no prize set, a set has more than 4 cars or
 * the memory cannot be had, else 1.  For each set: not_car_get (T105)
 * instead while the garage holds 200 cars; nothing for an invalid reason;
 * else the cars' models (car_get_load_car), the screen, and the car won
 * goes to the garage (GameStatus::presentCar). */
int  car_get_prizes(const char *race, int arg);             /* 0x0013EF40 */
/* F_0013F508 (car_get_prizes, then the ending movies): in its own header
 * (its callers did not see the classes above: u016's data pass). */
#include "game/menu/gtmode/T061_car_get_entry.h"
/* create_event_name ("eventname", 0x0013F700) is an inline function of
 * T061 (declared there). */

/* T061's static initialiser: fills the item factories of menu/car_get.mbl
 * (g_0049BE90, T061 .bss, file-local: defined in T061). */
void T061_static_init(void);                /* 0x0013F648 (.ctors 0x0028F07C) */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu13CarGetControl(void);    /* 0x0013F798 */
}
#endif

#endif /* GAME_T061_CAR_GET_CONTROL_H */
