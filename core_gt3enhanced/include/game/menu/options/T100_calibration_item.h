/* T100_calibration_item.h - gt2k_menu::CalibrationItem, one bar of the USB
 * wheel calibration page (translation unit T100, unit u014),
 * 0x0012A2B0..0x0012AB90.
 *
 * Built by the option page hook option_create_calib_bar (T064 0x00140B58;
 * operator new 0x2F0) as CalibrationItem(item, layout, owner,
 * &owner->options, owner->calibPort, owner->calibWheel, arg).  It attaches a Control of its own to the USB
 * slot of its port (pad manager slot 10 / 11, T475) and stores what that
 * wheel reports in the options' ControllerConfig (T426): axis 0 the
 * steering range (channel 0), 1 the steering centre, 2 / 3 the ranges of
 * the pedals (channels 1 / 2), each once it is wider than `threshold'.  The
 * item draws the range seen so far: steering 0 .. 1024 centred on 512
 * across the item's width, pedals 0 .. 256 up its height.  While any
 * CalibrationItem exists (g_003512FC counts them) the pedal map flag
 * g_00352CE0 (T474_control.h) is 0 and the calibration is marked not
 * valid (setValid); the last one marks it valid again when the same wheel
 * (pjoy_get_event_count of the port unchanged since the first one,
 * g_003512F8) is still there. */
#ifndef GAME_T100_CALIBRATION_ITEM_H
#define GAME_T100_CALIBRATION_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item, held by value (pinch) */
#include "engine/input/T474_control.h"              /* Control (control) */

class GameOptions;                          /* T428 (pointers only) */

namespace gt2k_menu {
class OptionControl;                        /* T062 (pointers only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */

/* vtable 0x00313EF8 (8 slots: [3] .. [7] as Item's).  typeinfo
 * __tfQ29gt2k_menu15CalibrationItem 0x0012AB40 (si, base Item; storage
 * 0x004B1700).  Size 0x2F0 (operator new in option_create_calib_bar). */
class CalibrationItem : public Item {
public:
    /* Item(res, layout); control attached to slot 10 + (port != 0); the
     * starting range: axis 1 the stored steering range and the "pinch"
     * picture, 0 an empty range at 512, 2 / 3 lo 0xFF, hi 0. */
    CalibrationItem(MenuLayoutItem *res, PageLayout *layout, OptionControl *owner,
                    ::GameOptions *options, int port, int wheel, int axis); /* 0x0012A2B0 */
    /* The last one marks the calibration valid again if the wheel is the
     * same; control detached; g_00352CE0 = 1 once none is left. */
    virtual ~CalibrationItem();             /* [0] +0x0C 0x0012A528 */
    virtual void virtual_1();               /* [1] +0x14 0x0012A608 update: Item's, then widens the range */
    /* draw: the item becomes the range's rectangle (and pinch the centre,
     * axis 1), Item's draw, pinch's. */
    virtual void virtual_2();               /* [2] +0x1C 0x0012A7E8 */

    OptionControl *owner;                   /* 0x100 (not used) */
    ::GameOptions *options;                 /* 0x104 owner +0xE58 */
    int            port;                    /* 0x108 owner->calibPort */
    int            wheel;                   /* 0x10C owner->calibWheel (the tables take wheel - 2) */
    int            axis;                    /* 0x110 the hook's p8: 0 steering, 1 centre, 2 / 3 pedals */
    Control        control;                 /* 0x114 the wheel */
    u32            lo;                      /* 0x1D8 smallest value seen */
    u32            hi;                      /* 0x1DC largest value seen */
    u32            centre;                  /* 0x1E0 */
    f32            pos;                     /* 0x1E4 x (steering) or bottom y (pedals) of the bar */
    f32            size;                    /* 0x1E8 width or height of the bar */
    Item           pinch;                   /* 0x1EC "pinch", the centre marker (axis 1) */
    int            threshold;               /* 0x2EC 0x40: the smallest range stored */
};
EGCS_SIZE(gt2k_menu::CalibrationItem, 0x2F0);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu15CalibrationItem(void);  /* 0x0012AB40 */
#endif

#endif /* GAME_T100_CALIBRATION_ITEM_H */
