/* T480 - IEEE1394Port: the port of one i.LINK node.  0x0022E470..0x0022E760.
 *
 * In a link battle each node's local SteeringPSPort (pad + wheel) is saved
 * into the node's outgoing frame (send), and every node's frame is loaded
 * into the PSPort / USBSteeringPort pair of its IEEE1394Port (receive),
 * through the link session (T473: F_0022CC50 the session runs, F_0022CC58
 * / F_0022CC78 the frames, F_0022D0F0 the exchange, F_0022D810 a node is
 * there, F_0022D830 the own node).  A Control attached to slot 12 + node
 * then gets that node's input through translateSync; the normal translate
 * does nothing.  Actuator requests are passed to the local port when the
 * port is this console's own node. */
#include "engine/input/control_internal.h"
#include "engine/input/T480_ieee1394_port.h"
#include "engine/system/T473_link_session.h"         /* F_0022CC50 .. F_0022D830 */

/* .rodata: the vtable (0x00341970), then "12IEEE1394Port": only other
 * files' code (the PadManager's constructor) references the vtable. */
// DATA_START 0x00341970

/* The steering init (SteeringPSPort::init, linkonce) is inline here; the
 * node's pad and wheel are pad / device 0 (only their link frames are
 * used). */
// FUNC 0x0022E470 IEEE1394Port::init
void IEEE1394Port::init(int node, SteeringPSPort *local)
{
    this->node = node;
    psPort.init(0);
    usbPort.init(0);
    this->local = local;
    steering.init(psPort, usbPort);
}

/* Saves the local port into this node's frame and runs the exchange;
 * 0 if the session or the node is not there (early returns: retail has no
 * result variable). */
// FUNC 0x0022E4E0 IEEE1394Port::send
int IEEE1394Port::send()
{
    if (F_0022CC50() == 0)
        return 0;
    if (F_0022D810(node) == 0)
        return 0;
    local->save(F_0022CC58());
    return F_0022D0F0();
}

/* Loads the frame received from the node into the node's ports. */
// FUNC 0x0022E538 IEEE1394Port::receive
void IEEE1394Port::receive()
{
    if (F_0022CC50() != 0 && F_0022D810(node) != 0) {
        u8 *frame = F_0022CC78(node);
        if (frame != NULL)
            steering.load(frame);
    }
}

/* Slot 1: empty.  index.txt: PortBase::enableSpring(bool) (lupo2m, a match
 * of the bare `jr $ra' only). */
// FUNC 0x0022E590 IEEE1394Port::translate
void IEEE1394Port::translate(Control &c)
{
}

/* Index: IEEE1394Port::virtual_2.  SteeringPSPort::translate called
 * directly. */
// FUNC 0x0022E598 IEEE1394Port::translateSync
void IEEE1394Port::translateSync(Control &c)
{
    if (F_0022D810(node) != 0)
        steering.translate(c);
}

/* Index: IEEE1394Port::virtual_3. */
// FUNC 0x0022E5E0 IEEE1394Port::setActuator
void IEEE1394Port::setActuator(int ch, int value)
{
    if (node == F_0022D830())
        local->setActuator(ch, value);
}

/* Index: IEEE1394Port::virtual_4. */
// FUNC 0x0022E648 IEEE1394Port::stopActuators
void IEEE1394Port::stopActuators()
{
    if (node == F_0022D830())
        local->stopActuators();
}

/* Index: IEEE1394Port::virtual_5. */
// FUNC 0x0022E690 IEEE1394Port::isReady
int IEEE1394Port::isReady()
{
    return F_0022D810(node) != 0;
}

#ifndef BOOT_OLD_GCC
/* __si_type_info __ti12IEEE1394Port (0x004B2A00), name "12IEEE1394Port"
 * (0x003419A8), base PortBase (__ti8PortBase, gp-0x4570).  Only in the
 * vtable. */
// FUNC 0x0022E6B0 __tf12IEEE1394Port
extern "C" void *__tf12IEEE1394Port(void)
{
    if (__ti12IEEE1394Port[0] == 0) {
        __tf8PortBase();
        __rtti_si(__ti12IEEE1394Port, "12IEEE1394Port", (const type_info *)__ti8PortBase);
    }
    return __ti12IEEE1394Port;
}
#endif

// FUNC 0x0022E700 IEEE1394Port::~IEEE1394Port
/* implicit: ~PortBase, ~PSPort; Index: IEEE1394Port::virtual_0.  Implicit (engine/input/T480_ieee1394_port.h
 * declares none); egcs emits this copy for the vtable: the members from the
 * last, steering and usbPort by PortBase::~PortBase(2) (their implicit
 * destructors reduce to it), psPort by PSPort::~PSPort(2), then the base
 * PortBase::~PortBase with the caller's flags. */
