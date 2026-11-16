/* T478_steering_ps_port.h - SteeringPSPort, a pad and a wheel as one port (translation unit T478, unit u105).
 *
 * translate gives the pad's input when no wheel is there, the wheel's when
 * no pad is, else both merged: types and GT3 buttons or-ed, raw buttons [0]
 * pad / [1] wheel, channels 0-15 the pad's, 16-18 the wheel's (wheel and
 * the two pedals).  Actuator requests go to the wheel when it is connected,
 * else to the pad.  PadManager (T475) has two ({ps[0], usb[0]}, {ps[4],
 * usb[1]}), each IEEE1394Port (T480) one over its node's ports.  The
 * constructor and init are linkonce in Lupo 2 (0x002D2BC0, 0x002D2BF8).
 */
#ifndef GAME_T478_STEERING_PS_PORT_H
#define GAME_T478_STEERING_PS_PORT_H

#include "game/game.h"
#include "engine/input/T476_port_base.h"            /* base PortBase */

class Control;
class PSPort;
class USBSteeringPort;

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tf14SteeringPSPort(void);            /* 0x00230318 */
#endif

/* vtable 0x00341A58 (6 slots).  typeinfo __tf14SteeringPSPort 0x00230318
 * (__si_type_info at 0x004B2A20, base PortBase).  Size 0x10
 * (PadManager::steering[2] from +0x2CC; IEEE1394Port::steering +0xBC to
 * +0xCC). */
class SteeringPSPort : public PortBase {
public:
    PSPort          *ps;                    /* 0x08 */
    USBSteeringPort *usb;                   /* 0x0C */

    /* Inline (no callers; PadManager's and IEEE1394Port's constructors
     * expand it), linkonce in Lupo 2.  egcs emits the copies of the inline
     * members with the vtable, the last-declared first. */
    SteeringPSPort() { ps = NULL; usb = NULL; }         /* 0x002302D8 */
    /* [0] +0x0C 0x00230368 ~SteeringPSPort: implicit (no vptr store; it
     * only calls ~PortBase, and IEEE1394Port's destructor expands it) */
    virtual void translate(Control &c);                 /* [1] +0x14 0x0022FFE8 also IEEE1394Port::translateSync */
    virtual void translateSync(Control &c);             /* [2] +0x1C 0x002301A0 (empty) */
    virtual void setActuator(int ch, int value);        /* [3] +0x24 0x002301A8 the wheel if connected, else the pad */
    virtual void stopActuators();                       /* [4] +0x2C 0x00230200 */
    virtual int  isReady();                             /* [5] +0x34 0x00230250 1 */

    u8  *save(u8 *buf);                     /* 0x00230258 into a link frame: the wheel (0x3C bytes), then the pad (0x28) */
    const u8 *load(const u8 *buf);          /* 0x00230290 from a link frame (as save) */
    /* Inline (no callers; PadManager::init and IEEE1394Port::init expand
     * it), linkonce in Lupo 2.  Written ps first: egcs then stores usb
     * first, as retail. */
    void init(PSPort &p, USBSteeringPort &u) { ps = &p; usb = &u; }   /* 0x002302C8 */
};
EGCS_SIZE(SteeringPSPort, 0x10);

#endif /* GAME_T478_STEERING_PS_PORT_H */
