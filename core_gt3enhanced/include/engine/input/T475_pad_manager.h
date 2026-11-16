/* T475_pad_manager.h - PadManager, all the input ports (translation unit T475, unit u105).
 *
 * PadManager (? name, as in u014; the class has no RTTI): the 8 PS pad
 * ports, 2 USB wheels, 2 SteeringPSPorts (pad + wheel of ports 1 and 2), 8
 * IEEE1394Ports, the list of attached Controls and a semaphore.  init (from
 * the game init 0x00102C00) starts the pad thread (T477) and binds the
 * ports; update (once a frame, 0x001012E0) reads the pads, wakes the pad
 * thread, reads the wheels and updates every Control under the semaphore;
 * updateLink (RaceLinkBattle) exchanges the i.LINK frames and updates the
 * Controls from them.  One global instance, g_002EA7F8.
 */
#ifndef GAME_T475_PAD_MANAGER_H
#define GAME_T475_PAD_MANAGER_H

#include "game/game.h"
#include "engine/input/T474_control.h"              /* Control (g_002EB170) */
#include "engine/input/T476_port_base.h"            /* ControlStatus */
#include "engine/input/T477_ps_port.h"              /* PSPort (member) */
#include "engine/input/T478_steering_ps_port.h"     /* SteeringPSPort (member) */
#include "engine/input/T479_usb_steering_port.h"    /* USBSteeringPort (member) */
#include "engine/input/T480_ieee1394_port.h"        /* IEEE1394Port (member) */

/* Attach slots (attach / getPortStatus): 0-7 ps[], 10-11 usb[], 12-19
 * link[], 20-21 steering[].  Size 0x974 (the static object 0x002EA7F8,
 * built by T475's static constructor).  The constructor builds the member
 * arrays (PSPort() out of line, the other ports' constructors inline). */
class PadManager {
public:
    Control         *head;                  /* 0x000 the attached Controls */
    PSPort           ps[8];                 /* 0x004 */
    USBSteeringPort  usb[2];                /* 0x1E4 */
    SteeringPSPort   steering[2];           /* 0x2CC {ps[0], usb[0]}, {ps[4], usb[1]} */
    IEEE1394Port     link[8];               /* 0x2EC i.LINK node n, with the local steering[0] */
    u8               unk_96C[4];            /* 0x96C */
    int              sema;                  /* 0x970 */

    /* PadManager() 0x0022EDB0 and ~PadManager() 0x0022EC18 are implicit
     * (T475 defines g_002EA7F8: egcs emits them for its static constructor
     * and destructor): the member arrays, the destructor from the last
     * element down. */
    void lock();                            /* 0x0022E760 WaitSema(sema) */
    void unlock();                          /* 0x0022E780 */
    void init();                            /* 0x0022E7A0 */
    void term();                            /* 0x0022E8C0 */
    int  update();                          /* 0x0022E8F0 once a frame; 1 */
    int  updateLink();                      /* 0x0022E9C8 1 if the exchange went through */
    void attach(Control *c, u32 slot);      /* 0x0022EA88 binds c to the port of `slot', puts it in the list */
    void detach(Control *c);                /* 0x0022EB68 */
    void F_0022EBB0();                      /* 0x0022EBB0 head = NULL (? an inline; no callers) */
    ControlStatus getPortStatus(u32 slot);  /* 0x0022EBB8 0 for the steering slots and unknown ones */
};
EGCS_SIZE(PadManager, 0x974);

extern PadManager g_002EA7F8;               /* 0x002EA7F8 the pad / device manager */
extern Control    g_002EB170;               /* 0x002EB170 a spare Control (? cleared by init) */

/* T475's static constructor (0x0022F1E0, .ctors entry 0x0028F180) builds
 * g_002EA7F8 and g_002EB170, its static destructor (0x0022ED78, .dtors entry
 * 0x0028F254) destroys them in the reverse order: both compiler-generated. */

#endif /* GAME_T475_PAD_MANAGER_H */
