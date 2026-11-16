/* T426_controller_config.h - ControllerConfig, the controller settings of the
 * game options (translation unit T426, unit u097), 0x00217538..0x00217C30.
 * (? class name, as in u014.)
 *
 * GameOptions+0x638 (GameStatus+0x30570): the key assignment of each
 * function (10) for 4 controller modes (0 digital pad, 1 analog pad, 2 / 3 the
 * two USB wheel types) x 2 ports, as indexes into the key code table
 * g_002E2CF8; and the calibration of the USB wheels (modes 2, 3) per port.
 * RaceInput (T413) maps the pad / wheel through it; the calibration
 * accessors are used by the options' CalibrationItem (T062) and the wheel
 * driver (usbApplyCalibration, T479). */
#ifndef GAME_T426_CONTROLLER_CONFIG_H
#define GAME_T426_CONTROLLER_CONFIG_H

#include "game/game.h"

/* The controller modes: ControllerConfig's first key index (keys[mode]),
 * KeyConfigItem::mode (T101: its tables in g_0029BE98), the modes RaceInput
 * maps the pad / wheel by (T413).  The calibrations are of the wheel modes
 * (calib[mode - CONFIG_WHEEL_FIRST]). */
enum ControllerMode {
    CONFIG_DIGITAL       = 0,                   /* a digital pad */
    CONFIG_ANALOG        = 1,                   /* DualShock / DualShock 2 */
    CONFIG_WHEEL_GTFORCE = 2,                   /* USB wheel type 1 (USB_WHEEL_GTFORCE, T474) */
    CONFIG_WHEEL_C294    = 3,                   /* USB wheel type 3 (USB_WHEEL_C294: the "jaguar_*" pictures) */
    CONFIG_MODES         = 4,
    CONFIG_WHEEL_FIRST   = CONFIG_WHEEL_GTFORCE /* the first wheel mode (calib[]) */
};

/* The functions a key is assigned to (keys[mode][port][func]); RaceInput's
 * button bits of 1..8 are g_002DFB18 (T413). */
enum ControllerFunc {
    KEY_FUNC_STEER      = 0,                    /* analog only */
    KEY_FUNC_THROTTLE   = 1,
    KEY_FUNC_BRAKE      = 2,
    KEY_FUNC_THROTTLE2  = 3,                    /* CarInput.throttle2 (the reverse, T342) */
    KEY_FUNC_HANDBRAKE  = 4,
    KEY_FUNC_SHIFT_UP   = 5,
    KEY_FUNC_SHIFT_DOWN = 6,
    /* 7 and 8: RaceInput bits 0x10000 / 0x20000 (? use); 9 (? unused) */
    KEY_FUNC_COUNT      = 10
};

/* One entry of the key code table g_002E2CF8: the 6-byte {button, axis,
 * pressure} of a key (? field names).  Size 6 (the table's stride). */
class KeyCodeInfo {
public:
    s16 button;                                 /* 0x0 pad button bit mask (0 / -1 = none) */
    s16 axis;                                   /* 0x2 analog input index (< 0 none; RaceInput's sticks) */
    s16 pressure;                               /* 0x4 pressure input index (< 0 none; RaceInput's pressure pads) */
};
EGCS_SIZE(KeyCodeInfo, 6);

/* Defined in T426 (34 entries; .data in the original, so not const). */
extern KeyCodeInfo g_002E2CF8[];                /* 0x002E2CF8 */

/* The USB wheel calibration of one (wheel mode, port).  Size 0x20 (the
 * accessors' strides: port << 5, wheel << 6). */
class WheelCalibration {
public:
    int valid;                                  /* 0x00 the calibration is valid */
    u32 steerMin;                               /* 0x04 default 0 */
    u32 steerMax;                               /* 0x08 default 0x400 */
    u32 steerCentre;                            /* 0x0C default 0x200 */
    u32 axis1Min;                               /* 0x10 default 0 */
    u32 axis1Max;                               /* 0x14 default 0x100 */
    u32 axis2Min;                               /* 0x18 default 0 */
    u32 axis2Max;                               /* 0x1C default 0x100 */
};
EGCS_SIZE(WheelCalibration, 0x20);

/* No vtable.  Size 0x250 (getSize; load / save copy 0x250; GameOptions ends
 * with it at 0x638 + 0x250 = 0x888). */
class ControllerConfig {
public:
    ControllerConfig();                         /* 0x00217538 reset() */
    /* The compiler's inline struct copy (0x240 bytes in 32-byte blocks, then
     * 0x10).  Called by GameOptions::operator=. */
    ControllerConfig &operator=(const ControllerConfig &o); /* 0x00217560 */
    void reset();                               /* 0x00217630 the keys of modes 0..3, the calibrations of 2, 3 */
    void resetKeys(int mode, int port);         /* 0x00217710 */
    void resetCalibration(int wheel, int port); /* 0x00217880 */
    int  keyAxis(int mode, int port, int func);     /* 0x002178C8 g_002E2CF8[key].axis */
    int  keyButton(int mode, int port, int func);   /* 0x00217900 g_002E2CF8[key].button */
    int  keyPressure(int mode, int port, int func); /* 0x00217938 g_002E2CF8[key].pressure */
    void setKey(int mode, int port, int func, int key); /* 0x00217970 */
    int  getKey(int mode, int port, int func);  /* 0x00217998 */
    void setSteerMin(u32 v, int port, int wheel);    /* 0x002179C0 */
    void setSteerMax(u32 v, int port, int wheel);    /* 0x002179E0 */
    void setSteerCentre(u32 v, int port, int wheel); /* 0x00217A00 */
    void setAxis1Min(u32 v, int port, int wheel);    /* 0x00217A20 */
    void setAxis1Max(u32 v, int port, int wheel);    /* 0x00217A40 */
    void setAxis2Min(u32 v, int port, int wheel);    /* 0x00217A60 */
    void setAxis2Max(u32 v, int port, int wheel);    /* 0x00217A80 */
    u32  getSteerMin(int port, int wheel);      /* 0x00217AA0 */
    u32  getSteerMax(int port, int wheel);      /* 0x00217AC0 */
    u32  getSteerCentre(int port, int wheel);   /* 0x00217AE0 */
    u32  getAxis1Min(int port, int wheel);      /* 0x00217B00 */
    u32  getAxis1Max(int port, int wheel);      /* 0x00217B20 */
    u32  getAxis2Min(int port, int wheel);      /* 0x00217B40 */
    u32  getAxis2Max(int port, int wheel);      /* 0x00217B60 */
    int  getValid(int port, int wheel);         /* 0x00217B80 */
    void setValid(int port, int wheel, int v);  /* 0x00217BA0 */
    static int getSize();                       /* 0x00217BC0 0x250 */
    const u8 *load(const u8 *src);              /* 0x00217BC8 */
    u8 *save(u8 *dst);                          /* 0x00217BF8 */

    int              keys[CONFIG_MODES][2][KEY_FUNC_COUNT]; /* 0x000 [mode][port][func]: a g_002E2CF8 index */
    u8               unk_140[0x50];             /* 0x140 ? only copied / saved (the size of one more mode's keys) */
    WheelCalibration calib[2][2];               /* 0x190 [wheel - 2][port] (wheel modes 2, 3) */
    u8               unk_210[0x40];             /* 0x210 ? only copied / saved (the size of one more wheel mode's calibrations) */
};
EGCS_SIZE(ControllerConfig, 0x250);

#endif /* GAME_T426_CONTROLLER_CONFIG_H */
