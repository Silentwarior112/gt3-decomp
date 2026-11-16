/* T476 - PortBase: the abstract input port.  0x0022F210..0x0022F2A8.
 * status (+0x0) is PORT_NONE (0) while no device is there, PORT_CONNECTED
 * (1) when one is; the vptr follows it (+0x4).  getStatus / setStatus are
 * linkonce in Lupo 2 (0x002D2A80 / 0x002D2A88; here in the reverse order:
 * the header declares getStatus first, and egcs emits the last-declared
 * copy first). */
#include "engine/input/control_internal.h"
#include "engine/input/T476_port_base.h"

/* Sets the vptr only (status is left as it is).  Callers: PSPort::PSPort,
 * SteeringPSPort::SteeringPSPort, PadManager::PadManager (the inline
 * constructors of the other ports). */
// FUNC 0x0022F210 PortBase::PortBase
PortBase::PortBase()
{
}

/* Index: PortBase::~PortBase. */
// FUNC 0x0022F228 PortBase::~PortBase
PortBase::~PortBase()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* (egcs: stores PortBase's vptr; operator delete(this) when flags & 1) */
}

/* setStatus / getStatus are inline in engine/input/T476_port_base.h (no callers;
 * the ports and PadManager::getPortStatus expand them); egcs emits these
 * copies with the vtable. */
// FUNC 0x0022F258 PortBase::setStatus
/* inline: status = s */

// FUNC 0x0022F260 PortBase::getStatus
/* inline: return status */

#ifndef BOOT_OLD_GCC
/* __user_type_info __ti8PortBase (0x00353480, gp-0x4570), name "8PortBase"
 * (0x003419F0).  Callers: the four derived ports' typeinfo functions. */
// FUNC 0x0022F268 __tf8PortBase
extern "C" void *__tf8PortBase(void)
{
    if (__ti8PortBase[0] == 0)
        __rtti_user(__ti8PortBase, "8PortBase");
    return __ti8PortBase;
}
#endif
