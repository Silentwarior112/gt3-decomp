/* T480_ieee1394_port.h - IEEE1394Port, the port of one i.LINK node (translation unit T480, unit u105).
 *
 * In a link battle each node's local SteeringPSPort (pad + wheel) is saved
 * into the node's outgoing frame (send), and every node's frame is loaded
 * into the PSPort / USBSteeringPort pair of its IEEE1394Port (receive),
 * through the link session (T473).  A Control attached to slot 12 + node
 * then gets that node's input through translateSync; translate does
 * nothing.  Actuator requests go to the local port when the port is this
 * console's own node.
 */
#ifndef GAME_T480_IEEE1394_PORT_H
#define GAME_T480_IEEE1394_PORT_H

#include "game/game.h"
#include "engine/input/T476_port_base.h"            /* base PortBase */
#include "engine/input/T477_ps_port.h"              /* PSPort (member) */
#include "engine/input/T478_steering_ps_port.h"     /* SteeringPSPort (member) */
#include "engine/input/T479_usb_steering_port.h"    /* USBSteeringPort (member) */

class Control;

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tf12IEEE1394Port(void);              /* 0x0022E6B0 */
#endif

/* vtable 0x00341970 (6 slots; index.txt names slot 1 PortBase::enableSpring
 * (bool), a lupo2m match of the bare `jr $ra' only).  typeinfo
 * __tf12IEEE1394Port 0x0022E6B0 (__si_type_info at 0x004B2A00, name
 * 0x003419A8, base PortBase).  Size 0xD0 (PadManager::link[8] from +0x2EC).
 * No out-of-line constructor (inline in PadManager's: PortBase(), vptr,
 * psPort, usbPort, steering; node and local are not set). */
class IEEE1394Port : public PortBase {
public:
    int              node;                  /* 0x08 i.LINK node number */
    PSPort           psPort;                /* 0x0C the node's pad */
    USBSteeringPort  usbPort;               /* 0x48 the node's wheel */
    SteeringPSPort   steering;              /* 0xBC over the two */
    SteeringPSPort  *local;                 /* 0xCC the local port (PadManager steering[0]) */

    /* Defined inline in T475 (src/engine/input/T475_pad_manager.cpp), whose implicit
     * PadManager constructor expands it: retail has no copy of it in T480
     * (an inline definition here would put one there), and the implicit
     * constructor egcs synthesizes is not expanded once SteeringPSPort's
     * inline constructor is part of it (u105 matching). */
    IEEE1394Port();
    /* [0] +0x0C 0x0022E700 ~IEEE1394Port: implicit (no vptr store; the
     * members' destructors, last first) */
    virtual void translate(Control &c);                 /* [1] +0x14 0x0022E590 (empty) */
    virtual void translateSync(Control &c);             /* [2] +0x1C 0x0022E598 steering.translate if the node is there */
    virtual void setActuator(int ch, int value);        /* [3] +0x24 0x0022E5E0 local->setActuator for the own node */
    virtual void stopActuators();                       /* [4] +0x2C 0x0022E648 */
    virtual int  isReady();                             /* [5] +0x34 0x0022E690 the node is there */

    void init(int node, SteeringPSPort *local);         /* 0x0022E470 */
    int  send();                                        /* 0x0022E4E0 local port into the frame, exchange; 0 if no session / node */
    void receive();                                     /* 0x0022E538 the node's frame into steering */
};
EGCS_SIZE(IEEE1394Port, 0xD0);

#endif /* GAME_T480_IEEE1394_PORT_H */
