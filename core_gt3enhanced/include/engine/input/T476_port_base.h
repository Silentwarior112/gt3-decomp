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
 * vptr +0x4 (egcs).  Size 8 (PSPort's first member is at +0x8).
 * The enhanced engine: 9 slots, as Lupo 2's PortBase (backend.md B2, B3;
 * the names of slots 7 / 8 are its linkonce symbols enableSpring__8PortBaseb
 * / setDamper__8PortBaseii); the data does not change. */
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
    /* The enhanced engine (Lupo 2's slots 6..8).  [6] the level of the
     * wheel's turn for the menus' key repeat, 1..8 (0: none, the defaults;
     * Control::getRepeat, backend.md B3; L2 0x0023DFE8).  [7] a wheel's own
     * centring spring on / off (RaceInput::setMode: off while racing,
     * backend.md B2; L2 0x002D2A90).  [8] a damper force of strength k (no
     * caller, as in Lupo 2; L2 0x002D2A98).  The defaults do nothing:
     * PSPort and IEEE1394Port keep them. */
    virtual int  getSteerLevel();                       /* [6] +0x3C */
    virtual void enableSpring(bool on);                 /* [7] +0x44 */
    virtual void setDamper(int k, int unused);          /* [8] +0x4C */

    /* Inline (no callers; Lupo 2's linkonce): egcs emits the copies with
     * the vtable (T476), the last-declared first. */
    ControlStatus getStatus() const { return status; }  /* 0x0022F260 */
    void setStatus(ControlStatus s) { status = s; }     /* 0x0022F258 */
};
EGCS_SIZE(PortBase, 0x8);

#endif /* GAME_T476_PORT_BASE_H */
