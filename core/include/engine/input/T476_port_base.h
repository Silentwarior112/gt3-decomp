/* T476_port_base.h - PortBase, the abstract input port (translation unit T476, unit u105).
 *
 * A port fills a Control (T474) from its device every frame (translate) or
 * from the i.LINK frames of a link battle (translateSync), and takes the
 * Control's actuator requests.  Derived: PSPort (T477), SteeringPSPort
 * (T478), USBSteeringPort (T479), IEEE1394Port (T480); PadManager (T475)
 * owns all the ports.  Names: Lupo 2's linkonce symbols (PortBase::setStatus,
 * getStatus); the slot names are descriptive.
 */
#ifndef GAME_T476_PORT_BASE_H
#define GAME_T476_PORT_BASE_H

#include "game/game.h"

class Control;

/* Lupo 2: PortBase::setStatus(ControlStatus), Control::setStatus(ControlStatus)
 * (mangled 13ControlStatus: a type of its own).  An enum (u105 matching):
 * its own alias set lets Control::assign load src.status above its store
 * of type, as retail does.  0 = nothing there, 1 = a device is connected
 * (? values). */
enum ControlStatus {
    PORT_NONE      = 0,                     /* nothing connected */
    PORT_CONNECTED = 1
};

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tf8PortBase(void);                   /* 0x0022F268 */
#endif

/* vtable 0x003419B8 (6 slots; 1-5 are __pure_virtual 0x00278FA8).  typeinfo
 * __tf8PortBase 0x0022F268 (__user_type_info at 0x00353480, gp-0x4570).
 * vptr +0x4 (egcs).  Size 8 (PSPort's first member is at +0x8). */
class PortBase {
public:
    ControlStatus status;                   /* 0x0 */
    /*  vptr                                   0x4 (egcs) */

    PortBase();                             /* 0x0022F210 */
    virtual ~PortBase();                    /* [0] +0x0C 0x0022F228 */
    virtual void translate(Control &c) = 0;             /* [1] +0x14 fills c from the device */
    virtual void translateSync(Control &c) = 0;         /* [2] +0x1C the same, from the link frames */
    virtual void setActuator(int ch, int value) = 0;    /* [3] +0x24 */
    virtual void stopActuators() = 0;                   /* [4] +0x2C */
    virtual int  isReady() = 0;                         /* [5] +0x34 */

    /* Inline (no callers; Lupo 2's linkonce): egcs emits the copies with
     * the vtable (T476), the last-declared first. */
    ControlStatus getStatus() const { return status; }  /* 0x0022F260 */
    void setStatus(ControlStatus s) { status = s; }     /* 0x0022F258 */
};
EGCS_SIZE(PortBase, 0x8);

#endif /* GAME_T476_PORT_BASE_H */
