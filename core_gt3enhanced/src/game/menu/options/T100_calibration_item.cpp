/* T100 - gt2k_menu::CalibrationItem.  0x0012A2B0..0x0012AB90.
 *
 * One bar of the USB wheel calibration page, built by the T064 hook
 * option_create_calib_bar (Object::operator new 0x2F0) as
 * CalibrationItem(item, layout, owner, &owner->options, owner->calibPort,
 * owner->calibWheel, p8).  While any of these items exists (count
 * g_003512FC), the pedal map flag g_00352CE0 (T474) is 0 and the
 * calibration is marked not valid (setValid); the last one marks it valid
 * again when the port's pjoy_get_event_count is still the one the first
 * item saw (g_003512F8; ? the wheel was not unplugged / replaced in the
 * meantime). */
/* Retail T100's .rodata (0x00313EF8..0x0031411E): the vtable, then the type
 * names CalibrationItem, OperatorList, List / OperatorHolder /
 * Cell<OperatorHolder>, the garage's (GarageWheelList .. Cell<GarageCar>),
 * Operator (reverse definition order: the GameStatus first, then the menu
 * classes of T100's header); no SmartIndexT<int>, no prize counter (option_items_internal.h
 * used to bring both).  Its .sdata (0x003512F0..0x00351300): "pinch", then
 * g_003512F8 and g_003512FC, zeros (C++ globals without an initialiser are
 * written at the end of the file's output, in the order of their first
 * declaration: option_items_internal.h's). */
#include "game/save/T429_game_status.h"          /* (the garage's type names) */
#include "game/menu/options/T100_calibration_item.h"
#include "game/save/T428_game_options.h"         /* GameOptions (config) */
#include "game/save/T426_controller_config.h"    /* ControllerConfig */
#include "engine/input/T474_control.h"              /* Control, g_00352CE0 (the pedal map flag) */
#include "engine/input/T475_pad_manager.h"          /* PadManager g_002EA7F8 (attach / detach) */
#include "engine/media/G002_mpeg_frameq.h"          /* pjoy_get_event_count */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue (x, y) */
#include "game/menu/options/option_items_internal.h"

int g_003512F8;                             /* 0x003512F8 pjoy_get_event_count of the port when the first
                                               CalibrationItem was built */
int g_003512FC;                             /* 0x003512FC CalibrationItems alive */

namespace gt2k_menu {

// FUNC 0x0012A2B0 gt2k_menu::CalibrationItem::CalibrationItem
CalibrationItem::CalibrationItem(MenuLayoutItem *res, PageLayout *layout, OptionControl *owner,
                                 ::GameOptions *options, int port, int wheel, int axis)
    : Item(res, layout), owner(owner), options(options), port(port), wheel(wheel), axis(axis)
{
    /* the members: control (Control::Control), pinch (Item(NULL, NULL)); the
     * landing pads run ~Item(pinch), Control::~Control and ~Item(this),
     * flags 0 */
    /* implicit: Control, ~Control (member constructor, landing pad) */
    threshold = 0x40;
    /* the parameter options (retail: $s5 + 0x638), the members port / wheel
     * (reloaded); one attach call per slot, cross-jumped */
    ControllerConfig *config = &options->config;
    if (++g_003512FC == 1)
        g_003512F8 = pjoy_get_event_count(this->port != 0);
    g_00352CE0 = 0;
    config->setValid(this->port, this->wheel, 0);
    if (this->port == 0)
        g_002EA7F8.attach(&control, PAD_SLOT_USB1);
    else
        g_002EA7F8.attach(&control, PAD_SLOT_USB2);
    switch (this->axis) {
    case CALIB_STEERING:                    /* an empty range at the middle */
        lo = 0x200;
        hi = 0x200;
        centre = 0x200;
        pos = x.value;
        size = w;
        break;
    case CALIB_CENTRE:                      /* the stored steering range, the centre marker */
        lo = config->getSteerMin(this->port, this->wheel);
        hi = config->getSteerMax(this->port, this->wheel);
        centre = 0x200;
        pos = x.value;
        size = w;
        pinch.setResource("pinch", layout);
        break;
    case CALIB_PEDAL1:
    case CALIB_PEDAL2:                      /* an empty range, measured from the bottom */
        lo = 0xFF;
        centre = 0x80;
        size = h;
        hi = 0;
        pos = y.value + h * 0.5f;
        break;
    }
}

// FUNC 0x0012A528 gt2k_menu::CalibrationItem::~CalibrationItem
CalibrationItem::~CalibrationItem()
{
    /* the vptr is stored first */
    ControllerConfig *config = &options->config;
    if (--g_003512FC == 0) {
        int events = pjoy_get_event_count(port != 0);
        config->setValid(port, wheel, g_003512F8 == events);
    }
    g_002EA7F8.detach(&control);
    g_00352CE0 = 0;                         /* sw $zero in the delay slot, then 1 when none is left */
    if (g_003512FC == 0)
        g_00352CE0 = 1;
    /* then the members: pinch.~Item() (flags 2), control's Control::~Control
     * (flags 2); Item::~Item(this, 0); Object::operator delete if flags & 1 */
    /* implicit: ~Control, ~Item, operator delete (member / base destructor, deleting destructor) */
}

/* update: Item's, then widens [lo, hi] with the axis value and stores it
 * once the range is wider than threshold; the centre item stores the
 * current steering value. */
// FUNC 0x0012A608 gt2k_menu::CalibrationItem::virtual_1
void CalibrationItem::virtual_1()
{
    Item::virtual_1();
    ControllerConfig *config = &options->config;
    switch (axis) {
    case CALIB_STEERING: {
        u32 value = control.channels[0];
        if (hi < value)
            hi = value;
        if (value < lo)
            lo = value;
        if (threshold < (int)(hi - lo)) {
            config->setSteerMin(lo, port, wheel);
            config->setSteerMax(hi, port, wheel);
        }
        break;
    }
    case CALIB_CENTRE:
        centre = control.channels[0];
        config->setSteerCentre(centre, port, wheel);
        break;
    case CALIB_PEDAL2: {
        u32 value = control.channels[2];
        if (hi < value)
            hi = value;
        if (value < lo)
            lo = value;
        if (threshold < (int)(hi - lo)) {
            config->setAxis2Min(lo, port, wheel);
            config->setAxis2Max(hi, port, wheel);
        }
        break;
    }
    case CALIB_PEDAL1: {
        u32 value = control.channels[1];
        if (hi < value)
            hi = value;
        if (value < lo)
            lo = value;
        if (threshold < (int)(hi - lo)) {
            config->setAxis1Min(lo, port, wheel);
            config->setAxis1Max(hi, port, wheel);
        }
        break;
    }
    }
}

/* draw: the item becomes the range rectangle: steering values map to
 * pos + size * (v - 512) / 1024 horizontally, pedal values to
 * pos - size * (256 - v) / 256 vertically (v converted as unsigned).
 * Other axis values keep the rectangle (the x / y stores still clear the
 * MotionValues' force and velocity).  Then Item's draw and, for the centre
 * item, pinch's (both direct calls). */
// FUNC 0x0012A7E8 gt2k_menu::CalibrationItem::virtual_2
void CalibrationItem::virtual_2()
{
    /* x / y through MotionValue's inline get / Drawable's setX / setY:
     * retail keeps &x / &y in $a1 / $a2 for the whole function */
    f32 itemY = y.get();                    /* the pinch's y */
    f32 newY = y.get();
    f32 newX = x.get();
    f32 newW = w;
    f32 newH = h;
    switch (axis) {
    case CALIB_STEERING: {
        f32 left  = pos + size * (((f32)lo - 512.0f) * 0.0009765625f);
        f32 right = pos + size * (((f32)hi - 512.0f) * 0.0009765625f);
        newW = right - left;
        newX = (left + right) * 0.5f;
        break;
    }
    case CALIB_CENTRE: {
        f32 left  = pos + size * (((f32)lo - 512.0f) * 0.0009765625f);
        f32 right = pos + size * (((f32)hi - 512.0f) * 0.0009765625f);
        newW = right - left;
        newX = (left + right) * 0.5f;
        pinch.x.set(pos + size * (((f32)centre - 512.0f) * 0.0009765625f));
        pinch.y.set(itemY);
        break;
    }
    case CALIB_PEDAL1:
    case CALIB_PEDAL2: {
        f32 top    = pos - size * ((256.0f - (f32)lo) * 0.00390625f);
        f32 bottom = pos - size * ((256.0f - (f32)hi) * 0.00390625f);
        newH = bottom - top;
        newY = (top + bottom) * 0.5f;
        break;
    }
    }
    setX(newX);
    setY(newY);
    w = newW;
    h = newH;
    Item::virtual_2();
    if (axis == CALIB_CENTRE)
        pinch.virtual_2();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x0012AB40 __tfQ29gt2k_menu15CalibrationItem
extern "C" void *__tfQ29gt2k_menu15CalibrationItem(void)
{
    if (__tiQ29gt2k_menu15CalibrationItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu15CalibrationItem, "Q29gt2k_menu15CalibrationItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu15CalibrationItem;
}
#endif
