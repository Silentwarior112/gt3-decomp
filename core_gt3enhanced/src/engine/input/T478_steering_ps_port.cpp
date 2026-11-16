/* T478 - SteeringPSPort: a PSPort and a USBSteeringPort as one port.
 * 0x0022FFE8..0x00230388.
 *
 * translate gives the pad's input when no wheel is there, the wheel's when
 * no pad is, else both merged: types and GT3 buttons or-ed, raw buttons
 * [0] pad / [1] wheel, channels 0-15 the pad's, 16-18 the wheel's (wheel
 * and the two pedals).  Actuator requests go to the wheel when it is
 * connected, else to the pad.  The constructor and init are linkonce in
 * Lupo 2 (0x002D2BC0, 0x002D2BF8); PadManager inlines them. */
#include "engine/input/control_internal.h"
#include "engine/input/T478_steering_ps_port.h"

/* Index: SteeringPSPort::virtual_1.  Also called directly by
 * IEEE1394Port::translateSync.  Two local Controls (constructed with
 * Control::Control, destroyed with Control::~Control(2)) take the two
 * ports' translations; the virtual translate calls of the pad and the
 * wheel, Control::assign, the non-virtual getButtons. */
// FUNC 0x0022FFE8 SteeringPSPort::translate
void SteeringPSPort::translate(Control &c)
{
    Control pad;
    Control wheel;
    ps->translate(pad);
    usb->translate(wheel);
    if (pad.status == PORT_NONE) {
        c.assign(wheel);
    } else if (wheel.status == PORT_NONE) {
        c.assign(pad);
    } else {
        c.status = PORT_CONNECTED;
        c.type   = pad.type | wheel.type;
        c.curButtons.raw[0] = pad.curButtons.raw[0];
        c.curButtons.raw[1] = wheel.curButtons.raw[0];
        c.curButtons.buttons = pad.getButtons() | wheel.getButtons();  /* direct calls: local objects */
        for (int ch = 0; ch < CHAN_PAD_COUNT; ch++)
            c.channels[ch] = pad.channels[ch];
        for (int ch = 0; ch < CHAN_WHEEL_COUNT; ch++)
            c.channels[CHAN_PAD_COUNT + ch] = wheel.channels[ch];
    }
}

/* Index: SteeringPSPort::virtual_2. */
// FUNC 0x002301A0 SteeringPSPort::translateSync
void SteeringPSPort::translateSync(Control &c)
{
}

/* Index: SteeringPSPort::virtual_3. */
// FUNC 0x002301A8 SteeringPSPort::setActuator
void SteeringPSPort::setActuator(int ch, int value)
{
    if (usb->status == PORT_CONNECTED)
        usb->setActuator(ch, value);
    else
        ps->setActuator(ch, value);
}

/* Index: SteeringPSPort::virtual_4. */
// FUNC 0x00230200 SteeringPSPort::stopActuators
void SteeringPSPort::stopActuators()
{
    ps->stopActuators();
    usb->stopActuators();
}

/* Index: SteeringPSPort::virtual_5. */
// FUNC 0x00230250 SteeringPSPort::isReady
int SteeringPSPort::isReady()
{
    return 1;
}

/* The enhanced engine (backend.md B2, B3): Lupo 2's slots 6..8 forwarded to
 * the wheel (L2 0x0023EF98 / 0x0023F028 / 0x0023F058). */
int SteeringPSPort::getSteerLevel()
{
    return usb->getSteerLevel();
}

void SteeringPSPort::enableSpring(bool on)
{
    usb->enableSpring(on);
}

void SteeringPSPort::setDamper(int k, int unused)
{
    usb->setDamper(k, unused);
}

/* (? name) Into a link frame: the wheel (0x3C bytes), then the pad (0x28).
 * Caller: IEEE1394Port::send. */
// FUNC 0x00230258 SteeringPSPort::save
u8 *SteeringPSPort::save(u8 *buf)
{
    return ps->save(usb->save(buf));
}

/* (? name) From a link frame (as save).  Caller: IEEE1394Port::receive. */
// FUNC 0x00230290 SteeringPSPort::load
const u8 *SteeringPSPort::load(const u8 *buf)
{
    return ps->load(usb->load(buf));
}

/* init and the constructor are inline in engine/input/T478_steering_ps_port.h (no
 * callers: PadManager and IEEE1394Port expand them); egcs emits these copies
 * with the vtable, the last-declared first. */
// FUNC 0x002302C8 SteeringPSPort::init
/* inline: ps = &p; usb = &u (stored usb first) */

// FUNC 0x002302D8 SteeringPSPort::SteeringPSPort
/* implicit: PortBase (inline: PortBase::PortBase, the vptr, both pointers
 * NULL) */

#ifndef BOOT_OLD_GCC
/* __si_type_info __ti14SteeringPSPort (0x004B2A20), name "14SteeringPSPort"
 * (0x00341A90), base PortBase.  Only in the vtable. */
// FUNC 0x00230318 __tf14SteeringPSPort
extern "C" void *__tf14SteeringPSPort(void)
{
    if (__ti14SteeringPSPort[0] == 0) {
        __tf8PortBase();
        __rtti_si(__ti14SteeringPSPort, "14SteeringPSPort", (const type_info *)__ti8PortBase);
    }
    return __ti14SteeringPSPort;
}
#endif

// FUNC 0x00230368 SteeringPSPort::~SteeringPSPort
/* implicit: ~PortBase; Index: SteeringPSPort::virtual_0.  Implicit (engine/input/T478_steering_ps_port.h
 * declares none); egcs emits this copy for the vtable: only
 * PortBase::~PortBase with the caller's flags (no vptr store of its own). */
