/* T037_tumble_car.h - gt2k_menu::TumbleCar, the car wash animation
 * (translation unit T037, unit u013): action_wash_car new's it.
 *
 * Split from T037_gtmode_actions.h (u013's data pass): the original class
 * has no key function (its update, virtual_1, is inline), so egcs writes its
 * type name into every object that sees the class; of the originals only
 * T037 has "Q29gt2k_menu9TumbleCar" (0x003122F0), so only T037 includes
 * this header (T043 includes T037_gtmode_actions.h for the action table).
 * Evidence of the inline update: T037's frame info (0x0029A330) has
 * virtual_1 0x00127220, __tf 0x001272D0, ~TumbleCar 0x00127320 at the end
 * of the unit (egcs's group of a key-less class's inline members, __tf and
 * implicit members) and no out-of-line constructor, which egcs would write
 * (a global) if virtual_1 were the key function. */
#ifndef GAME_T037_TUMBLE_CAR_H
#define GAME_T037_TUMBLE_CAR_H

#include "game/game.h"
#include "game/menu/framework/T220_object.h"               /* base gt2k_menu::Object */
#include "game/menu/framework/T212_menu_car.h"             /* gt2k_menu::Car (dirt) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager::setFreeze */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se, MENU_SE_CAR_WASH */

namespace gt2k_menu {

/* The car wash animation (an Object under PageManager::root, new'ed by
 * action_wash_car): 30 frames, menu sound 2, 180 frames of spin (10000 a
 * frame on the car camera's yaw force), then the car clean (Car::dirt
 * 0), PageManager::setFreeze(0) and kill().  vtable 0x003122C0 (5 slots:
 * [2] Object's 0x001BFEF8, [3] / [4] Object's 0x001BFF00 / 0x001BFF08).
 * typeinfo __tfQ29gt2k_menu9TumbleCar 0x001272D0 (si, base Object; storage
 * 0x004B15E0).  Size 0x34 (operator new in action_wash_car).
 *
 * The destructor is implicit in the original: 0x00127320 does not store the
 * vptr (~Object with flags 0, then Object's operator delete). */
class TumbleCar : public Object {
public:
    /* Inline (expanded in action_wash_car): Object(), the vptr.  The spun
     * value comes from the caller: retail computes car + 0x174 before
     * Object() (in a saved register), so it is an argument, not an
     * initialiser computed from car. */
    TumbleCar(Car *car, f32 *spin, PageManager *pages)
        : car(car), pages(pages), spin(spin), state(0), timer(30) {}
    /*  [0] +0x0C 0x00127320 ~TumbleCar: implicit (see above) */

    /* [1] +0x14 0x00127220 update: 30 frames of wait, then the car wash
     * sound and 180 frames of spin (10000 a frame on the car camera's yaw
     * force); then the car is clean (Car::dirt 0, if there is a 3D car), the
     * PageManager takes input again (setFreeze(0); "ignore_control" froze
     * it) and the animation kills itself.  Inline: egcs writes it at the end
     * of T037 (its FUNC marker is in T037_gtmode_actions.cpp). */
    virtual void virtual_1()
    {
        switch (state) {
        case 0:                             /* wait */
            if (--timer < 0) {
                state = 1;
                timer = 180;
                menu_play_se(MENU_SE_CAR_WASH);
            }
            break;
        case 1:                             /* spinning */
            *spin += 10000.0f;
            if (--timer < 0) {
                if (car != NULL)
                    car->dirt = 0.0f;
                pages->setFreeze(0);
                kill();
            }
            break;
        }
    }

    Car         *car;                       /* 0x20 MenuStatus::carItem, the 3D car (NULL: none) */
    PageManager *pages;                     /* 0x24 the GtmodeControl's */
    f32         *spin;                      /* 0x28 &car->camera.tumbleY.force (Car +0x174) */
    int          state;                     /* 0x2C 0 wait, 1 spinning */
    int          timer;                     /* 0x30 frames left in the state */
};
EGCS_SIZE(gt2k_menu::TumbleCar, 0x34);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu9TumbleCar(void);  /* 0x001272D0 */
#endif

#endif /* GAME_T037_TUMBLE_CAR_H */
