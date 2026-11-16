/* T479_usb_steering_port.h - USBSteeringPort, a USB steering wheel (translation unit T479, unit u105).
 *
 * Devices: Logitech (vendor 0x46D) GT Force 0xC293 (0xC295 taken as 0xC293)
 * and 0xC294, vendor 0x6F8 product 4.  A new Logitech wheel first runs an
 * auto-centring sequence with its force feedback (autoCentre) before it
 * counts as connected.  update reads the driver's report every frame (G002
 * pjoy_*, the report being G002's PjoyState; with the GT Force's calibration
 * from the game options while the pedal map is on), sends the force values
 * and resets them; translate maps the wheel's buttons, its hat and quick
 * steering movements to GT3's buttons and gives channels 0-2 = wheel, pedal
 * 1, pedal 2.  Control USB types: 1 GT Force, 3 0xC294, 2 0x6F8 wheel, 7
 * anything else.  The accessors are Lupo 2's linkonce getX .. getReport
 * (0x002D2CC0..0x002D2D08, emitted in the reverse order).
 */
#ifndef GAME_T479_USB_STEERING_PORT_H
#define GAME_T479_USB_STEERING_PORT_H

#include "game/game.h"
#include "engine/input/T476_port_base.h"            /* base PortBase */

class Control;
class ControllerConfig;                     /* T426: the wheel calibration of the game options */
struct PjoyState;                           /* G002 (u123): the driver's 0x38-byte report (pjoy_read_state);
                                               u105's first pass called it USBReport */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tf15USBSteeringPort(void);               /* 0x00231120 */
#endif

/* The device of a USBPortState (G002 pjoy_get_info: id[0] vendor, id[1]
 * product).  A plain member, not in a union with a u32 (u105 matching): a
 * union member access gets alias set 0, but USBSteeringPort::translate
 * loads the vendor above its store of the Control's type; the 0x6F8 wheel's
 * single compare reads the pair as a u32 (USB_DEVICE_6F8_WHEEL). */
struct USBDeviceId {
    u16 vendor;                             /* 0x0 */
    u16 product;                            /* 0x2 */
};
EGCS_SIZE(USBDeviceId, 0x4);

/* What USBSteeringPort::save / load copy (USBSteeringPort +0x20..+0x5B, 0x3C
 * bytes of a link frame): the connection and the last report's values. */
struct USBPortState {
    int connected;                          /* 0x00 (+0x20) */
    int x;                                  /* 0x04 (+0x24) the wheel (10 bits) */
    int y;                                  /* 0x08 */
    int z;                                  /* 0x0C */
    int rx;                                 /* 0x10 */
    int ry;                                 /* 0x14 */
    int rz;                                 /* 0x18 */
    int slider[2];                          /* 0x1C (+0x3C) the pedals (8 bits) */
    int pov[1];                             /* 0x24 (+0x44) hat 0..7 = N, NE, E, ... */
    u8  buttons[16];                        /* 0x28 (+0x48) */
    USBDeviceId id;                         /* 0x38 (+0x58) vendor +0x58, product +0x5A */
};
EGCS_SIZE(USBPortState, 0x3C);

/* vtable 0x00341B38 (6 slots).  typeinfo __tf15USBSteeringPort 0x00231120
 * (__si_type_info at 0x004B2A30, name 0x00341C58, base PortBase).  Size 0x74
 * (PadManager::usb[2] from +0x1E4; IEEE1394Port::usbPort +0x48 to +0xBC).
 * No out-of-line constructor (inline in PadManager / IEEE1394Port). */
class USBSteeringPort : public PortBase {
public:
    int          usbPort;                   /* 0x08 the USB device index */
    int          centred;                   /* 0x0C 1 when the centring sequence is done (0x46D) or not needed */
    int          centreState;               /* 0x10 0..4 (autoCentre) */
    int          centreLeft;                /* 0x14 x at the end of state 1 (? the left stop) */
    int          centreRight;               /* 0x18 x at the end of state 3 (? the right stop) */
    int          centreCount;               /* 0x1C frames in the state */
    USBPortState st;                        /* 0x20 */
    int          report;                    /* 0x5C getReport */
    int          prevX;                     /* 0x60 the previous frame's x */
    s8           actCount;                  /* 0x64 4 */
    u8           act[4];                    /* 0x65 force-feedback values (0x7F = none) */
    u8           unk_69[0xB];               /* 0x69 */

    /* [0] +0x0C 0x00231170 ~USBSteeringPort: implicit (no vptr store; it
     * only calls ~PortBase) */
    virtual void translate(Control &c);                 /* [1] +0x14 0x00230D10 */
    virtual void translateSync(Control &c);             /* [2] +0x1C 0x00230F50 (empty) */
    virtual void setActuator(int ch, int value);        /* [3] +0x24 0x00230F58 only channel 2 (the force) */
    virtual void stopActuators();                       /* [4] +0x2C 0x00230F70 0x7F = no force */
    virtual int  isReady();                             /* [5] +0x34 0x00230FA8 */

    void init(int usbPort);                 /* 0x002307F0 */
    void onConnect(u16 vendor, u16 product);/* 0x00230830 */
    void onDisconnect();                    /* 0x00230858 */
    void clearReport();                     /* 0x00230878 */
    void resetCentring();                   /* 0x00230928 */
    void autoCentre(const PjoyState *r);    /* 0x00230938 one step a frame of the centring sequence */
    void update();                          /* 0x00230A90 once a frame (PadManager::update) */
    u8  *save(u8 *buf);                     /* 0x00230FB0 into a link frame (0x3C bytes) */
    const u8 *load(const u8 *buf);          /* 0x00231038 from a link frame */
    /* The accessors are inline (none of the copies has a caller; Lupo 2's
     * linkonce getX .. getReport).  egcs emits the copies with the vtable,
     * the last-declared first: declared in the reverse order of their
     * addresses.  getX returns +0x24, the wheel the report's first word
     * goes to (index.txt's names for 0x002310D8..0x00231110 are shifted by
     * one). */
    int  getX() const { return st.x; }                  /* 0x00231118 */
    int  getY() const { return st.y; }                  /* 0x00231110 */
    int  getZ() const { return st.z; }                  /* 0x00231108 */
    int  getRX() const { return st.rx; }                /* 0x00231100 */
    int  getRY() const { return st.ry; }                /* 0x002310F8 */
    int  getRZ() const { return st.rz; }                /* 0x002310F0 */
    int *getSlider() { return st.slider; }              /* 0x002310E8 */
    int *getPov() { return st.pov; }                    /* 0x002310E0 */
    u8  *getButtons() { return st.buttons; }            /* 0x002310D8 */
    int  getReport() const { return report; }           /* 0x002310D0 */
};
EGCS_SIZE(USBSteeringPort, 0x74);

/* The calibration of (port, wheel) applied to a report: the wheel to
 * 0..0x3FF around the stored centre, the pedals to 0..0xFF (? name).
 * Only caller: update (wheel mode 2, the GT Force's). */
void usbApplyCalibration(ControllerConfig *cfg, int port, int wheel, PjoyState *r);   /* 0x00230388 */
/* GT3 buttons of the wheels (? names). */
u32  usbGTForceButtons(const u8 *b);                    /* 0x00230538 the GT Force's 6 buttons */
u32  usbC294Buttons(const u8 *b, const int *pov);       /* 0x00230580 12 buttons (table 0x00341AB0) and the hat */
u32  usb46DSteerButtons(const int *pov, int x, int prevX); /* 0x00230628 left / right from the wheel (pov unused) */
u32  usb6F8Buttons(const u8 *b, const int *pov);        /* 0x00230698 the 0x6F8 wheel's 10 buttons and hat */
u32  usb6F8SteerButtons(int x, int prevX);              /* 0x00230778 as usb46DSteerButtons with its limits */

#endif /* GAME_T479_USB_STEERING_PORT_H */
