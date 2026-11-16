/* T450_menu_tire_model.h - MenuTireModel (translation unit T450, unit u101).
 *
 * The tyre of a menu car (the car lists' car, the menu car model): one
 * TirePacket of 30 segments, the wheel drawn as part 2 (front) or 3 (rear)
 * of the car's own model set.  Its view test also decides whether the outer
 * face is turned to the camera (faceVisible).  Users: u021, u064, u070. */
#ifndef GAME_T450_MENU_TIRE_MODEL_H
#define GAME_T450_MENU_TIRE_MODEL_H

#include "game/game.h"
#include "game/car/model/T447_tire_model_base.h"      /* base TireModelBase */
#include "game/car/model/T462_tire_packet.h"          /* TirePacket, held by value */

class ModelSet;                             /* T451 (pointers only) */

/* vtable _vt$13MenuTireModel 0x003410D0 (2 slots), typeinfo
 * __tf13MenuTireModel 0x002245B0 (si, base TireModelBase; __si_type_info
 * 0x004B29F0).  Size 0xC8. */
class MenuTireModel : public TireModelBase {
public:
    MenuTireModel();                        /* 0x00224090 */
    virtual ~MenuTireModel();               /* [0] +0x0C 0x00224100 */
    virtual int virtual_1();                /* [1] +0x14 0x002241A8 the view test (clipTest), sets faceVisible */

    void *build(void *work, int segs);      /* 0x00224150 builds the packet at work; returns the end */
    /* The floats first (EABI numbers them apart from the ints, but egcs
     * moves the arguments in declaration order: retail T107 drawTires sets
     * $f12 / $f14 before $a1; u021). */
    void  draw(f32 steer, f32 camber, f32 unused, int withTire, int withShadow); /* 0x00224260 angles in degrees;
                                               withShadow: packet.packetShadow (not F1 tyres) */
    /* Inline in the original (? name): the copy egcs emitted has no
     * callers; Car / CarsItem (T212, T107) expand it. */
    void  setWheelModel(ModelSet *m) { wheelModel = m; }    /* 0x002245A8 */

    TirePacket packet;                      /* 0x094 */
    ModelSet  *wheelModel;                  /* 0x0C4 the car's model set (wheels: parts 2 front, 3 rear) */
};
EGCS_SIZE(MenuTireModel, 0xC8);

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tf13MenuTireModel(void);  /* 0x002245B0 */
#endif

#endif /* GAME_T450_MENU_TIRE_MODEL_H */
