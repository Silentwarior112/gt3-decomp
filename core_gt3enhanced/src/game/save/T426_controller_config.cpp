/* T426 (0x00217538..0x00217C30): the controller settings of the game
 * options (GameOptions+0x638, ? class name as in option_items_internal.h): the key
 * assignment of each function (10) for 4 controller modes x 2 ports (an
 * index into the key code table g_002E2CF8), and the calibration of the USB
 * wheels (modes 2, 3) per port.  The calibration accessors are used by the
 * options' CalibrationItem (T062) and the wheel driver
 * (usbApplyCalibration, T479).
 *
 * Retail's T426 has no .rodata (no literal, no type name): the original saw
 * no class with a type name.  Its .data is the key code table, then the
 * frame info. */
#include "game/save/game_options_internal.h"
#include "game/save/T426_controller_config.h"
#include "game/enhanced.h"                  /* ENH_MENU_WHEEL_CALIBRATION */

/* The key codes ControllerConfig::keys[][][] index: {button, axis, pressure}
 * (-1 = none; values from the original's .data). */
KeyCodeInfo g_002E2CF8[34] = {  /* 0x002E2CF8 */
    { -1, -1, -1 },                             /*  0 none */
    { 0, 2, 2 },                                /*  1 */
    { 0, 0, 0 },                                /*  2 */
    { 0, 0x103, 0x103 },                        /*  3 */
    { 0, 0x203, 0x203 },                        /*  4 */
    { 0, 0x102, 0x102 },                        /*  5 */
    { 0, 0x202, 0x202 },                        /*  6 */
    { 0, 0x101, 0x101 },                        /*  7 */
    { 0, 0x201, 0x201 },                        /*  8 */
    { 0, 0x100, 0x100 },                        /*  9 */
    { 0, 0x200, 0x200 },                        /* 10 */
    { 32, -1, 9 },                              /* 11 */
    { 64, -1, 10 },                             /* 12 */
    { 16, -1, 8 },                              /* 13 */
    { 128, -1, 11 },                            /* 14 */
    { 8, -1, 13 },                              /* 15 */
    { 2, -1, 15 },                              /* 16 */
    { 4, -1, 12 },                              /* 17 */
    { 1, -1, 14 },                              /* 18 */
    { 0x800, -1, -1 },                          /* 19 */
    { 0, 0, -1 },                               /* 20 */
    { 1, -1, -1 },                              /* 21 */
    { 2, -1, -1 },                              /* 22 */
    { 4, -1, -1 },                              /* 23 */
    { 8, -1, -1 },                              /* 24 */
    { 16, -1, -1 },                             /* 25 */
    { 32, -1, -1 },                             /* 26 */
    { 0, 1, -1 },                               /* 27 */
    { 0, 2, -1 },                               /* 28 */
    { -1, -1, -1 },                             /* 29 */
    { 0x400, -1, -1 },                          /* 30 */
    { 0x800, -1, -1 },                          /* 31 */
    { 0x1000, -1, -1 },                         /* 32 */
    { 0x2000, -1, -1 },                         /* 33 */
};  /* 0x002E2CF8 */

// FUNC 0x00217538 ControllerConfig::ControllerConfig
ControllerConfig::ControllerConfig()
{
    reset();
}

/* The compiler's inline struct copy (0x240 bytes in 32-byte blocks, then
 * 0x10; ldl/ldr + sdl/sdr when either pointer is not 8-aligned, ld/sd
 * otherwise).  Called by GameOptions::operator=. */
// FUNC 0x00217560 ControllerConfig::operator=
ControllerConfig &ControllerConfig::operator=(const ControllerConfig &o)
{
    memcpy(this, &o, sizeof(ControllerConfig));
    return *this;
}

// FUNC 0x00217630 ControllerConfig::reset
void ControllerConfig::reset()
{
    resetKeys(CONFIG_DIGITAL, 0);
    resetKeys(CONFIG_DIGITAL, 1);
    resetKeys(CONFIG_ANALOG, 0);
    resetKeys(CONFIG_ANALOG, 1);
    resetKeys(CONFIG_WHEEL_GTFORCE, 0);
    resetKeys(CONFIG_WHEEL_GTFORCE, 1);
    resetCalibration(CONFIG_WHEEL_GTFORCE, 0);
    resetCalibration(CONFIG_WHEEL_GTFORCE, 1);
    resetKeys(CONFIG_WHEEL_C294, 0);
    resetKeys(CONFIG_WHEEL_C294, 1);
    resetCalibration(CONFIG_WHEEL_C294, 0);
    resetCalibration(CONFIG_WHEEL_C294, 1);
}

/* The default key assignment of (mode, port); other modes are left alone.
 * Modes 0 and 1 differ only in function 0. */
// FUNC 0x00217710 ControllerConfig::resetKeys
void ControllerConfig::resetKeys(int mode, int port)
{
    int *key = keys[mode][port];

    switch (mode) {
    case CONFIG_DIGITAL:
        key[0] = 0;
        key[1] = 0x0C;
        key[2] = 0x0E;
        key[3] = 0x0D;
        key[4] = 0x0B;
        key[5] = 0x10;
        key[6] = 0x12;
        key[7] = 0x11;
        key[8] = 0x0F;
        key[9] = 0x13;
        break;
    case CONFIG_ANALOG:
        key[0] = 1;
        key[1] = 0x0C;
        key[2] = 0x0E;
        key[3] = 0x0D;
        key[4] = 0x0B;
        key[5] = 0x10;
        key[6] = 0x12;
        key[7] = 0x11;
        key[8] = 0x0F;
        key[9] = 0x13;
        break;
    case CONFIG_WHEEL_GTFORCE:
        key[0] = 0x14;
        key[1] = 0x1B;
        key[2] = 0x1C;
        key[3] = 0x1A;
        key[4] = 0x19;
        key[5] = 0x18;
        key[6] = 0x15;
        key[7] = 0x16;
        key[8] = 0x1D;
        key[9] = 0x17;
        break;
    case CONFIG_WHEEL_C294:
        key[0] = 0x14;
        key[1] = 0x1B;
        key[2] = 0x1C;
        key[3] = 0x1E;
        key[4] = 0x21;
        key[5] = 0x1A;
        key[6] = 0x17;
        key[7] = 0x15;
        key[8] = 0x18;
        key[9] = 0x13;
        break;
    }
}

/* The default calibration of (wheel, port): the steering's full range, the
 * pedals' 0 .. WHEEL_PEDAL_RANGE, not valid (the options' calibration
 * screen measures them).
 * The enhanced engine (Lupo 2 L2 0x00225DC0, menus_b.md 11): the two USB
 * wheels get measured pedal ranges and are marked valid, so they work
 * without the calibration screen: the GT Force (wheel 2) accelerator
 * 0x29..0xD5 and brake 0x2B..0xDA, the C294 (wheel 3) 0x25..0xB4 and
 * 0x48..0xDF (setValid, L2 0x002261A0).  It is what a new or reset set of
 * options holds; a loaded save keeps its own calibration.  GT3's wheel
 * driver applies the calibration to the GT Force only: the C294's needs the
 * input port's change of backend.md B4 c (USBSteeringPort::update,
 * src/engine/input: the race area's). */
// FUNC 0x00217880 ControllerConfig::resetCalibration
void ControllerConfig::resetCalibration(int wheel, int port)
{
    WheelCalibration *c = &calib[wheel - CONFIG_WHEEL_FIRST][port];

    c->valid       = 0;
    c->steerMin    = 0;
    c->steerMax    = WHEEL_STEER_RANGE;
    c->steerCentre = WHEEL_STEER_RANGE / 2;
#if ENH_MENU_WHEEL_CALIBRATION
    if (wheel == CONFIG_WHEEL_GTFORCE) {
        c->axis1Min = 0x29;
        c->axis1Max = 0xD5;
        c->axis2Min = 0x2B;
        c->axis2Max = 0xDA;
        setValid(port, wheel, 1);
        return;
    }
    if (wheel == CONFIG_WHEEL_C294) {
        c->axis1Min = 0x25;
        c->axis1Max = 0xB4;
        c->axis2Min = 0x48;
        c->axis2Max = 0xDF;
        setValid(port, wheel, 1);
        return;
    }
#endif
    c->axis1Min    = 0;
    c->axis1Max    = WHEEL_PEDAL_RANGE;
    c->axis2Min    = 0;
    c->axis2Max    = WHEEL_PEDAL_RANGE;
}

// FUNC 0x002178C8 ControllerConfig::keyAxis
int ControllerConfig::keyAxis(int mode, int port, int func)
{
    return g_002E2CF8[getKey(mode, port, func)].axis;
}

// FUNC 0x00217900 ControllerConfig::keyButton
int ControllerConfig::keyButton(int mode, int port, int func)
{
    return g_002E2CF8[getKey(mode, port, func)].button;
}

// FUNC 0x00217938 ControllerConfig::keyPressure
int ControllerConfig::keyPressure(int mode, int port, int func)
{
    return g_002E2CF8[getKey(mode, port, func)].pressure;
}

// FUNC 0x00217970 ControllerConfig::setKey
void ControllerConfig::setKey(int mode, int port, int func, int key)
{
    keys[mode][port][func] = key;
}

// FUNC 0x00217998 ControllerConfig::getKey
int ControllerConfig::getKey(int mode, int port, int func)
{
    return keys[mode][port][func];
}

// FUNC 0x002179C0 ControllerConfig::setSteerMin
void ControllerConfig::setSteerMin(u32 v, int port, int wheel)
{
    calib[wheel - CONFIG_WHEEL_FIRST][port].steerMin = v;
}

// FUNC 0x002179E0 ControllerConfig::setSteerMax
void ControllerConfig::setSteerMax(u32 v, int port, int wheel)
{
    calib[wheel - CONFIG_WHEEL_FIRST][port].steerMax = v;
}

// FUNC 0x00217A00 ControllerConfig::setSteerCentre
void ControllerConfig::setSteerCentre(u32 v, int port, int wheel)
{
    calib[wheel - CONFIG_WHEEL_FIRST][port].steerCentre = v;
}

// FUNC 0x00217A20 ControllerConfig::setAxis1Min
void ControllerConfig::setAxis1Min(u32 v, int port, int wheel)
{
    calib[wheel - CONFIG_WHEEL_FIRST][port].axis1Min = v;
}

// FUNC 0x00217A40 ControllerConfig::setAxis1Max
void ControllerConfig::setAxis1Max(u32 v, int port, int wheel)
{
    calib[wheel - CONFIG_WHEEL_FIRST][port].axis1Max = v;
}

// FUNC 0x00217A60 ControllerConfig::setAxis2Min
void ControllerConfig::setAxis2Min(u32 v, int port, int wheel)
{
    calib[wheel - CONFIG_WHEEL_FIRST][port].axis2Min = v;
}

// FUNC 0x00217A80 ControllerConfig::setAxis2Max
void ControllerConfig::setAxis2Max(u32 v, int port, int wheel)
{
    calib[wheel - CONFIG_WHEEL_FIRST][port].axis2Max = v;
}

// FUNC 0x00217AA0 ControllerConfig::getSteerMin
u32 ControllerConfig::getSteerMin(int port, int wheel)
{
    return calib[wheel - CONFIG_WHEEL_FIRST][port].steerMin;
}

// FUNC 0x00217AC0 ControllerConfig::getSteerMax
u32 ControllerConfig::getSteerMax(int port, int wheel)
{
    return calib[wheel - CONFIG_WHEEL_FIRST][port].steerMax;
}

// FUNC 0x00217AE0 ControllerConfig::getSteerCentre
u32 ControllerConfig::getSteerCentre(int port, int wheel)
{
    return calib[wheel - CONFIG_WHEEL_FIRST][port].steerCentre;
}

// FUNC 0x00217B00 ControllerConfig::getAxis1Min
u32 ControllerConfig::getAxis1Min(int port, int wheel)
{
    return calib[wheel - CONFIG_WHEEL_FIRST][port].axis1Min;
}

// FUNC 0x00217B20 ControllerConfig::getAxis1Max
u32 ControllerConfig::getAxis1Max(int port, int wheel)
{
    return calib[wheel - CONFIG_WHEEL_FIRST][port].axis1Max;
}

// FUNC 0x00217B40 ControllerConfig::getAxis2Min
u32 ControllerConfig::getAxis2Min(int port, int wheel)
{
    return calib[wheel - CONFIG_WHEEL_FIRST][port].axis2Min;
}

// FUNC 0x00217B60 ControllerConfig::getAxis2Max
u32 ControllerConfig::getAxis2Max(int port, int wheel)
{
    return calib[wheel - CONFIG_WHEEL_FIRST][port].axis2Max;
}

// FUNC 0x00217B80 ControllerConfig::getValid
int ControllerConfig::getValid(int port, int wheel)
{
    return calib[wheel - CONFIG_WHEEL_FIRST][port].valid;
}

// FUNC 0x00217BA0 ControllerConfig::setValid
void ControllerConfig::setValid(int port, int wheel, int v)
{
    calib[wheel - CONFIG_WHEEL_FIRST][port].valid = v;
}

// FUNC 0x00217BC0 ControllerConfig::getSize
int ControllerConfig::getSize()
{
    return 0x250;
}

// FUNC 0x00217BC8 ControllerConfig::load
const u8 *ControllerConfig::load(const u8 *src)
{
    u32 n = 0x250;                              /* sizeof(ControllerConfig); in a variable, egcs calls memcpy */

    memcpy(this, src, n);
    return src + n;
}

// FUNC 0x00217BF8 ControllerConfig::save
u8 *ControllerConfig::save(u8 *dst)
{
    u32 n = 0x250;                              /* sizeof(ControllerConfig) */

    memcpy(dst, this, n);
    return dst + n;
}
