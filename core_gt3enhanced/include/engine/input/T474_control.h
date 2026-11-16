/* T474_control.h - Control, the input state of one player (translation unit T474, unit u105).
 *
 * Buttons (this frame and the previous one), 32 analog channels and key
 * repeat, translated every frame from the PortBase (T476) it is bound to.
 * PadManager (T475) keeps the list of attached Controls (linked / prev /
 * next) and binds them to its ports by slot: 0-7 PS pads, 10-11 USB wheels,
 * 12-19 i.LINK nodes, 20-21 pad + wheel.  Bases of RaceInput (T413),
 * gt2k_menu::MenuControl (T214) and others; the link session (T473) and
 * PadManager hold one each (0x002E9820, 0x002EB170).
 */
#ifndef GAME_T474_CONTROL_H
#define GAME_T474_CONTROL_H

#include "game/game.h"
#include "engine/input/T476_port_base.h"            /* ControlStatus */

class PortBase;

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function and its storage itself (and rejects
 * hand declarations). */
extern "C" {
void *__tf7Control(void);                   /* 0x0022E430 */
extern u32 __ti7Control[2];                 /* 0x00353478 gp-0x4578 __user_type_info */
}
#endif

/* The buttons of one frame, copied as one 16-byte block (latch()). */
struct ControlButtons {
    u32 mask;                               /* 0x0 the buttons that count (latch: all; invalidButtons: none) */
    u32 buttons;                            /* 0x4 GT3 buttons: 1 up, 2 down, 4 left, 8 right, 0x10 start,
                                               0x20 select, 0x40 circle / cross, 0x80 triangle / square */
    u32 raw[2];                             /* 0x8 device buttons ([0] the PS pad / USB wheel bits,
                                               [1] the wheel of a SteeringPSPort) */
};
EGCS_SIZE(ControlButtons, 0x10);

/* GT3's buttons (ControlButtons::buttons; getButtons / getButtonsOn /
 * getButtonsOff / getRepeat masks), what every port translates its device
 * into.  MenuControl (T214) calls 0x40 decide and 0x80 cancel. */
enum ControlButton {
    CONTROL_UP     = 0x01,                  /* d-pad / left stick / hat up */
    CONTROL_DOWN   = 0x02,
    CONTROL_LEFT   = 0x04,                  /* also a quick turn of a wheel */
    CONTROL_RIGHT  = 0x08,
    CONTROL_START  = 0x10,
    CONTROL_SELECT = 0x20,
    CONTROL_DECIDE = 0x40,                  /* circle, cross; a wheel's first pedal under the pedal map */
    CONTROL_CANCEL = 0x80                   /* triangle, square; the second pedal */
};

/* Control::type bits 0-4 (rawPSType): the PS pad type.  These are libpad's
 * controller type ids (scePadInfoMode; ps2sdk PAD_TYPE_NEJICON 2,
 * PAD_TYPE_DIGITAL 4, PAD_TYPE_ANALOG 5, PAD_TYPE_DUALSHOCK 7), the high
 * nibble of the pad's mode id, plus GT3's own 8 for the DualShock 2's mode
 * id 0x79.  PSPort's type table (g_00341A38) only lists 4, 7 and 8;
 * PSPort::translate still reads types 2 and 5, which only a link frame could
 * carry. */
enum ControlPadType {
    PS_PAD_NONE       = 0,
    PS_PAD_NEGCON     = 2,                  /* mode id 0x23 (not in the table) */
    PS_PAD_DIGITAL    = 4,                  /* mode id 0x41 */
    PS_PAD_ANALOG     = 5,                  /* mode id 0x53 (not in the table; RaceInput reads it as analog) */
    PS_PAD_DUALSHOCK  = 7,                  /* mode id 0x73 */
    PS_PAD_DUALSHOCK2 = 8,                  /* mode id 0x79: pressure-sensitive buttons */
    PS_PAD_TYPE_MASK  = 0x1F
};

/* Control::type bits 5-7 (rawUSBType): the USB wheel type
 * (USBSteeringPort, T479; Control::setUSBType). */
enum ControlUsbType {
    USB_WHEEL_NONE    = 0,
    USB_WHEEL_GTFORCE = 1,                  /* Logitech 0xC293 (and 0xC295): ControllerConfig mode 2 */
    USB_WHEEL_6F8     = 2,                  /* vendor 0x6F8, product 4: fixed buttons */
    USB_WHEEL_C294    = 3,                  /* Logitech 0xC294: ControllerConfig mode 3 (the "jaguar_*" pictures) */
    USB_WHEEL_DFP     = 4,                  /* the enhanced engine: Logitech 0xC298, the Driving Force Pro
                                               (backend.md B1; only Lupo 2's IOP module reports it) */
    USB_WHEEL_OTHER   = 7,                  /* any other device */
    USB_TYPE_SHIFT    = 5
};

/* The raw PS pad bits (ControlButtons::raw[0] of a PS pad port, T477):
 * libpad's button word with its two bytes swapped (the scePadRead bytes [2]
 * << 8 | [3], made active high; ps2sdk libpad.h PAD_L2 0x0100 .. PAD_LEFT
 * 0x0080 -> 0x0001 .. 0x8000).  MenuControl (T214) and RaceInput (T413)
 * read them. */
enum ControlRawButton {
    PADRAW_L2       = 0x0001,
    PADRAW_R2       = 0x0002,
    PADRAW_L1       = 0x0004,
    PADRAW_R1       = 0x0008,
    PADRAW_TRIANGLE = 0x0010,
    PADRAW_CIRCLE   = 0x0020,
    PADRAW_CROSS    = 0x0040,
    PADRAW_SQUARE   = 0x0080,
    PADRAW_SELECT   = 0x0100,
    PADRAW_L3       = 0x0200,
    PADRAW_R3       = 0x0400,
    PADRAW_START    = 0x0800,
    PADRAW_UP       = 0x1000,
    PADRAW_RIGHT    = 0x2000,
    PADRAW_DOWN     = 0x4000,
    PADRAW_LEFT     = 0x8000
};

/* Control::channels.  A PS pad: PadData bytes 4.. (the sticks, then the
 * DualShock 2's pressures in its report order; 0..255).  A USB wheel: the
 * wheel and the two pedals.  A SteeringPSPort: the pad's first 16, then the
 * wheel's 3 (from CHAN_PAD_COUNT). */
enum ControlChannel {
    CHAN_RSTICK_X       = 0,
    CHAN_RSTICK_Y       = 1,                /* MenuControl::analogRVertical (its low byte) */
    CHAN_LSTICK_X       = 2,
    CHAN_LSTICK_Y       = 3,
    CHAN_PRESS_RIGHT    = 4,
    CHAN_PRESS_LEFT     = 5,
    CHAN_PRESS_UP       = 6,
    CHAN_PRESS_DOWN     = 7,
    CHAN_PRESS_TRIANGLE = 8,
    CHAN_PRESS_CIRCLE   = 9,
    CHAN_PRESS_CROSS    = 10,
    CHAN_PRESS_SQUARE   = 11,
    CHAN_PRESS_L1       = 12,
    CHAN_PRESS_R1       = 13,
    CHAN_PRESS_L2       = 14,
    CHAN_PRESS_R2       = 15,
    CHAN_PAD_COUNT      = 16,               /* what SteeringPSPort takes from the pad; its wheel's first channel */
    CHAN_WHEEL          = 0,
    CHAN_PEDAL1         = 1,
    CHAN_PEDAL2         = 2,
    CHAN_WHEEL_COUNT    = 3,
    CHAN_COUNT          = 32
};

/* The pedal map flag of isPedalMapEnable / setPedalMapEnable (the first word
 * of T474's .sdata, initially 1).  USBSteeringPort (T479) reads it;
 * CalibrationItem (T100) clears it while one of its items exists. */
extern int g_00352CE0;                      /* 0x00352CE0 gp-0x4D10 */

/* vtable 0x00341908 (10 slots).  typeinfo __tf7Control 0x0022E430
 * (__user_type_info __ti7Control 0x00353478, gp-0x4578).  vptr +0xC0
 * (egcs).  Size 0xC4 (the static objects 0x002E9820 / 0x002EB170 and the
 * derived classes' first members at +0xC4).  Names: Lupo 2's linkonce
 * accessors (0x002D2760..0x002D2D08) and the slots' behaviour. */
class Control {
public:
    int             linked;                 /* 0x00 1 while in the PadManager's list */
    Control        *prev;                   /* 0x04 */
    Control        *next;                   /* 0x08 getNext */
    PortBase       *port;                   /* 0x0C setPort */
    ControlStatus   status;                 /* 0x10 the port's status (rawStatus) */
    int             type;                   /* 0x14 bits 0-4 the PS pad type (ControlPadType), bits 5-7 the USB
                                               wheel type (ControlUsbType).  int: RaceInput (T413) shifts it
                                               with sra (u095) */
    ControlButtons  prevButtons;            /* 0x18 the previous frame */
    ControlButtons  curButtons;             /* 0x28 this frame */
    u32             channels[CHAN_COUNT];   /* 0x38 analog values (ControlChannel: PS sticks, pressures; USB wheel, pedals) */
    int             repeating;              /* 0xB8 isRepeating: the first repeat delay is over */
    int             repeatCount;            /* 0xBC frames since the last repeat */
    /*  vptr                                   0xC0 (egcs) */

    Control();                              /* 0x0022DA98 */
    virtual ~Control();                     /* [0] +0x0C 0x0022DB48 */
    virtual void update();                  /* [1] +0x14 0x0022DED8 latch, port->translate, emulatePressure */
    virtual void updateSync();              /* [2] +0x1C 0x0022DF40 the same with port->translateSync */
    virtual int  isPortReady();             /* [3] +0x24 0x0022E010 port->isReady() */
    virtual u32  getButtons() const;        /* [4] +0x2C 0x0022DFA8 the buttons held */
    virtual u32  getButtonsOn() const;      /* [5] +0x34 0x0022DFB0 pressed this frame */
    virtual u32  getButtonsOff() const;     /* [6] +0x3C 0x0022DFD8 released this frame */
    virtual u32  getAnalog(int ch) const;   /* [7] +0x44 0x0022E000 channels[ch] */
    virtual void latch();                   /* [8] +0x4C 0x0022DE10 prevButtons = curButtons */
    virtual void emulatePressure();         /* [9] +0x54 0x0022DCE8 pressure channels of pads without them */

    void clear();                                       /* 0x0022DBA8 */
    void assign(const Control &src);                    /* 0x0022DC10 */
    void attach(int slot);                              /* 0x0022DC98 g_002EA7F8.attach(this, slot) */
    void detach();                                      /* 0x0022DCC0 g_002EA7F8.detach(this) */
    void link(Control **head);                          /* 0x0022DE40 */
    void unlink(Control **head);                        /* 0x0022DEA8 */
    void setActuator(int ch, int value);                /* 0x0022E040 */
    void stopActuators();                               /* 0x0022E070 */
    /* 1 on the press of `mask', then after `first' frames every `interval'
     * frames while held (repeating / repeatCount). */
    int  getRepeat(u32 mask, int first, int interval);  /* 0x0022E0A0 */
    /* The accessors are inline (u105 matching: none of the copies
     * 0x0022E178..0x0022E420 has a caller, and the ports and assign() expand
     * them).  egcs emits the out-of-line copies with the vtable, the
     * last-declared first: they are declared here in the reverse order of
     * their addresses.  getOff / getOn / getNow (u061): ReplayOption::update
     * (T196) expands getOn, `andi 0x40; sltu' (the mask is still a parameter
     * when the store-flag is expanded; `(getButtonsOn() & 0x40) != 0' written
     * in place gives `sra 6; andi 1').  rawButtonUp / rawButtonDown (u047):
     * RaceSolitaire::virtual_28 (T139) expands rawButtonDown(0); the two
     * halves in their own variables (written as one expression, fold
     * regroups the four operands).  rawUSBType / rawPSType: KeyConfigItem
     * (T101) and RaceInput expand them (lbu +0x14, dsrl 5 / lw +0x14, andi
     * 0x1F); rawUSBType masks the shifted int (`(u8)type >> 5' gives srl,
     * this form retail's dsrl). */
    int  isRepeating() const { return repeating != 0; }                 /* 0x0022E420 */
    Control *getNext() const { return next; }                           /* 0x0022E418 */
    void setPort(PortBase &p) { port = &p; }                            /* 0x0022E410 */
    void portTranslate() { if (port != NULL) port->translate(*this); }  /* 0x0022E3D8 */
    void portTranslateSync() { if (port != NULL) port->translateSync(*this); }  /* 0x0022E3A0 */
    void setStatus(ControlStatus s) { status = s; }                     /* 0x0022E398 */
    void setType(int t) { type = t; }                                   /* 0x0022E390 */
    /* setPSType / setUSBType replace the whole word (the other type's bits
     * are cleared). */
    void setPSType(int t) { type = t & PS_PAD_TYPE_MASK; }              /* 0x0022E380 */
    void setUSBType(int t) { type = (t & 7) << USB_TYPE_SHIFT; }        /* 0x0022E370 */
    void setRawButtons(u32 b, int i) { curButtons.raw[i] = b; }         /* 0x0022E360 */
    void setButtons(u32 b) { curButtons.buttons = b; }                  /* 0x0022E358 */
    void setChannel(u32 value, int ch) { channels[ch] = value; }        /* 0x0022E348 */
    /* The word copy `while (n--) *d++ = *s++;' (assign() copies the raw
     * buttons the same way): `bne -1' loops. */
    void setChannels(const Control &src)                                /* 0x0022E310 */
    {
        u32 *d = channels;
        const u32 *s = src.channels;
        int n = CHAN_COUNT;
        while (n--)
            *d++ = *s++;
    }
    ControlStatus rawStatus() const { return status; }                  /* 0x0022E308 */
    u32  rawType() const { return type; }                               /* 0x0022E300 */
    int  rawPSType() const { return type & PS_PAD_TYPE_MASK; }          /* 0x0022E2F0 */
    int  rawUSBType() const { return (type >> USB_TYPE_SHIFT) & 7; }    /* 0x0022E2E0 */
    u32  rawButtons(int i) const { return curButtons.raw[i]; }          /* 0x0022E2D0 */
    u32  rawButtonDown(int i) const                                     /* 0x0022E2A0 */
    {
        u32 on = curButtons.raw[i] & prevButtons.mask;
        u32 off = ~prevButtons.raw[i] & curButtons.mask;
        return on & off;
    }
    u32  rawButtonUp(int i) const                                       /* 0x0022E270 */
    {
        u32 off = ~curButtons.raw[i] & prevButtons.mask;
        u32 on = prevButtons.raw[i] & curButtons.mask;
        return off & on;
    }
    u32  rawPrevButtons(int i) const { return prevButtons.raw[i]; }     /* 0x0022E260 */
    u32  rawAnalogValue(int ch) const { return channels[ch]; }          /* 0x0022E250 */
    void invalidButtons() { curButtons.mask = 0; }                      /* 0x0022E248 */
    void setPedalMapEnable(bool on) { g_00352CE0 = on; }                /* 0x0022E240 */
    int  isPedalMapEnable() const { return g_00352CE0; }                /* 0x0022E238 */
    int  getNow(int mask) const { return (getButtons() & mask) != 0; }     /* 0x0022E1F8 */
    int  getOn(int mask) const { return (getButtonsOn() & mask) != 0; }    /* 0x0022E1B8 */
    int  getOff(int mask) const { return (getButtonsOff() & mask) != 0; }  /* 0x0022E178 */
};
EGCS_SIZE(Control, 0xC4);

#endif /* GAME_T474_CONTROL_H */
