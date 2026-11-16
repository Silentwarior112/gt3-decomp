/* T475 - PadManager (? name; the class has no RTTI): all the input ports and
 * the list of attached Controls.  0x0022E760..0x0022F210.
 *
 * init (from game_init 0x00102C00) starts the pad thread (T477) and binds
 * the ports: ps[i] = pad i (only 0 and 4 are polled: port 1 and port 2,
 * slot 0), usb[i] = USB wheel i, steering[0] = ps[0] + usb[0], steering[1]
 * = ps[4] + usb[1], link[n] = i.LINK node n with the local steering[0].
 * update (once a frame, render_thread_main 0x001012E0) reads the pads,
 * wakes the pad thread, reads the wheels and updates every Control under
 * the semaphore; updateLink (from RaceLinkBattle) exchanges the link frames
 * and updates the Controls from them (without the semaphore). */
#include "engine/input/control_internal.h"
#include "engine/input/T475_pad_manager.h"
#include "engine/system/T473_link_session.h"         /* F_0022CC50 .. F_0022D830 */

/* IEEE1394Port's constructor, inline here (declared in
 * engine/input/T480_ieee1394_port.h): only the implicit PadManager constructor below
 * uses it, and retail has no copy of it in T480.  Empty: PortBase(), the
 * vptr, then psPort, usbPort and steering (their constructors inline but
 * PSPort's); node and local are not set.  The implicit constructor egcs
 * would synthesize is not expanded into PadManager's once SteeringPSPort's
 * inline constructor is part of it (u105 matching). */
inline IEEE1394Port::IEEE1394Port()
{
}

// FUNC 0x0022E760 PadManager::lock
void PadManager::lock()
{
    WaitSema(sema);
}

// FUNC 0x0022E780 PadManager::unlock
void PadManager::unlock()
{
    signal_sema_helper(sema);
}

// FUNC 0x0022E7A0 PadManager::init
void PadManager::init()
{
    sema = create_mutex_sema(1);
    padStart();
    for (int i = 0; i < PS_PORTS; i++)
        ps[i].init(i);
    for (int i = 0; i < USB_PORTS; i++)
        usb[i].init(i);
    /* SteeringPSPort::init (linkonce) inline */
    steering[0].init(ps[0], usb[0]);
    steering[1].init(ps[4], usb[1]);
    for (int node = 0; node < LINK_PORTS; node++)
        link[node].init(node, &steering[0]);
    head = NULL;
    g_002EB170.clear();
}

/* No caller. */
// FUNC 0x0022E8C0 PadManager::term
void PadManager::term()
{
    padStop();
    DeleteSema(sema);
}

/* Once a frame.  The steering ports have nothing to read: their loop is
 * left empty (an inline that compiles to nothing, ? what it was).  One
 * counter for the three loops (retail keeps it in s0 for the empty one). */
// FUNC 0x0022E8F0 PadManager::update
int PadManager::update()
{
    int i;
    padReadAll();
    for (i = 0; i < PS_PORTS; i++)
        ps[i].update();
    padWakeThread();
    for (i = 0; i < USB_PORTS; i++)
        usb[i].update();
    for (i = 0; i < STEERING_PORTS; i++) {
        /* steering[i]: nothing */
    }
    lock();
    for (Control *c = head; c != NULL; c = c->next)
        c->update();
    unlock();
    return 1;
}

/* The link battle's frame: sends this node's input; if that worked, loads
 * every node's frame and updates the Controls from them. */
// FUNC 0x0022E9C8 PadManager::updateLink
int PadManager::updateLink()
{
    int exchanged = 0;
    if (F_0022D830() >= 0)
        exchanged = link[F_0022D830()].send();
    if (exchanged == 0)
        return 0;
    for (int node = 0; node < LINK_PORTS; node++)
        link[node].receive();
    for (Control *c = head; c != NULL; c = c->next)
        c->updateSync();
    return 1;
}

/* Binds c to the port of `slot' (unknown slots keep its port), translates
 * it once and puts it in the list (under the semaphore).  setPort (inline):
 * each branch adds `this' itself, as retail. */
// FUNC 0x0022EA88 PadManager::attach
void PadManager::attach(Control *c, u32 slot)
{
    if (slot - SLOT_PS < PS_PORTS)
        c->setPort(ps[slot - SLOT_PS]);
    else if (slot - SLOT_USB < USB_PORTS)
        c->setPort(usb[slot - SLOT_USB]);
    else if (slot - SLOT_LINK < LINK_PORTS)
        c->setPort(link[slot - SLOT_LINK]);
    else if (slot - SLOT_STEERING < STEERING_PORTS)
        c->setPort(steering[slot - SLOT_STEERING]);
    lock();
    if (c->port != NULL)
        c->port->translate(*c);
    c->link(&head);
    unlock();
}

// FUNC 0x0022EB68 PadManager::detach
void PadManager::detach(Control *c)
{
    lock();
    c->unlink(&head);
    unlock();
}

/* (? an inline clearing the first word, the list head; no callers) */
// FUNC 0x0022EBB0 PadManager::F_0022EBB0
void PadManager::F_0022EBB0()
{
    head = NULL;
}

/* The status of the port of `slot' (PortBase::getStatus inline: the
 * element's address is folded with the field offset, as retail); 0 for the
 * steering slots and unknown ones.  Caller: option_check_calib_steer
 * (0x0013FF98). */
// FUNC 0x0022EBB8 PadManager::getPortStatus
ControlStatus PadManager::getPortStatus(u32 slot)
{
    if (slot - SLOT_PS < PS_PORTS)
        return ps[slot - SLOT_PS].getStatus();
    if (slot - SLOT_USB < USB_PORTS)
        return usb[slot - SLOT_USB].getStatus();
    if (slot - SLOT_LINK < LINK_PORTS)
        return link[slot - SLOT_LINK].getStatus();
    return PORT_NONE;
}

/* The objects (egcs puts a public object that has a constructor in .data,
 * zero-filled; T475's static constructor builds them in this order, its
 * static destructor destroys them in the reverse one). */
PadManager g_002EA7F8;                      /* 0x002EA7F8 */
Control    g_002EB170;                      /* 0x002EB170 */

/* Compiler-generated, emitted after the unit's last function in this order:
 * the implicit destructor (needed by the static destructor), the static
 * destructor, the implicit constructor (needed by the static constructor),
 * the static constructor. */
// FUNC 0x0022EC18 PadManager::~PadManager (_$_10PadManager)
/* implicit: operator delete (the member arrays from the last element down,
 * virtual destructor calls with flags 0: link (~IEEE1394Port), steering
 * (~SteeringPSPort), usb (~USBSteeringPort), ps (~PSPort); then operator
 * delete(this) when flags & 1) */

// FUNC 0x0022ED78 T475_static_dtor (_GLOBAL_$D$lock__10PadManager)
/* implicit: ~Control(g_002EB170, 2), then ~PadManager(g_002EA7F8, 2)
 * (.dtors entry 0x0028F254) */

// FUNC 0x0022EDB0 PadManager::PadManager (__10PadManager)
/* implicit: the member arrays: PSPort::PSPort() (out of line) for ps[]; the
 * inline USBSteeringPort / SteeringPSPort constructors (PortBase::PortBase(),
 * the vptr; the steering pointers NULL) and the inline IEEE1394Port
 * constructor (PortBase::PortBase(), the vptr, then psPort by
 * PSPort::PSPort(), usbPort, steering; node and local are not set).  The
 * rest of the 0x42C bytes are the exception cleanups of the partly built
 * arrays (PortBase::~PortBase / PSPort::~PSPort, flags 0) */

// FUNC 0x0022F1E0 T475_static_init (_GLOBAL_$I$lock__10PadManager)
/* implicit: PadManager() on g_002EA7F8, then Control() on g_002EB170
 * (.ctors entry 0x0028F180) */
