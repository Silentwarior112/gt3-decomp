/* T477_ps_port.h - PSPort and the pad thread (translation unit T477, unit u105).
 *
 * libpad 2200 driven from a thread (priority 4, stack 0x0049F9C0, 0x1000
 * bytes) that PadManager::update wakes once a frame: for pads 0 and 4 (port
 * 1 / port 2, slot 0) it reads the pad and sets the DualShock mode (locked),
 * the pressure mode and the actuators.  The main thread reads its own copy
 * of the data (padReadAll) and each PSPort translates it into a Control:
 * the mode id gives the PS type (4 digital, 7 DualShock, 8 DualShock 2),
 * the button bits GT3's buttons, the bytes from [4] on the analog channels.
 * The pad functions' names are the first pass's (? names); the thread state
 * (struct PadState / PadThread, 0x004A09C0) stays in the unit.
 */
#ifndef GAME_T477_PS_PORT_H
#define GAME_T477_PS_PORT_H

#include "game/game.h"
#include "engine/input/T476_port_base.h"            /* base PortBase */
#include "engine/input/T474_control.h"              /* ControlPadType */

class Control;
struct PadState;                            /* the pad thread's state of one pad (0x5D bytes, u105) */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tf6PSPort(void);                     /* 0x0022FF98 */
#endif

/* One scePadRead buffer: [0] 0 = ok, [1] mode id, [2..3] buttons (active
 * low), [4..7] right x / y, left x / y, [8..19] pressures. */
struct PadData {
    u8 b[0x20];                             /* 0x00 */
};
EGCS_SIZE(PadData, 0x20);

/* The pad types by mode id (.rodata 0x00341A38): {0x41, 4}, {0x73, 7},
 * {0x79, 8}, {0, 0}.  The type is libpad's controller type id, the mode
 * id's high nibble (ps2sdk PAD_TYPE_DIGITAL 4, PAD_TYPE_DUALSHOCK 7), with
 * GT3's own 8 for the DualShock 2's 0x79; PSPort::translate also knows 2
 * (PAD_TYPE_NEJICON, the neGcon) and 5 (PAD_TYPE_ANALOG), which the table
 * no longer lists. */
struct PadTypeEntry {
    u8  id;                                 /* 0x0 0 ends the table */
    u8  unk_1[3];                           /* 0x1 */
    ControlPadType type;                    /* 0x4 */
};
EGCS_SIZE(PadTypeEntry, 0x8);

/* What PSPort::save / load copy (PSPort +0x0C..+0x33, 0x28 bytes of a link
 * frame). */
struct PSPortState {
    u8      padId;                          /* 0x00 (+0x0C) mode id of the pad, 0 = none */
    u8      unk_01[3];                      /* 0x01 */
    ControlPadType psType;                  /* 0x04 (+0x10) getPSType: padFindType(padId)->type.  The
                                               enum (T474; u105 matching): its own alias set lets
                                               PSPort::translate's switch reuse the loaded type past
                                               the Control's channel stores (u32), as retail does */
    PadData data;                           /* 0x08 (+0x14) getData */
};
EGCS_SIZE(PSPortState, 0x28);

/* vtable 0x00341A00 (6 slots).  typeinfo __tf6PSPort 0x0022FF98
 * (__si_type_info at 0x004B2A10, name 0x00352CE8, base PortBase).  Size 0x3C
 * (PadManager::ps[8] from +0x004, stride 0x3C; IEEE1394Port::psPort +0x0C
 * to +0x48). */
class PSPort : public PortBase {
public:
    s8          port;                       /* 0x08 pad index (port = i >> 2, slot = i & 3) */
    u8          unk_09[3];                  /* 0x09 */
    PSPortState st;                         /* 0x0C */
    int         actEnable;                  /* 0x34 actuatorEnable */
    u8          act[3];                     /* 0x38 the motors of this frame */
    u8          unk_3B;                     /* 0x3B */

    PSPort();                               /* 0x0022FB00 */
    virtual ~PSPort();                      /* [0] +0x0C 0x0022FB38 */
    virtual void translate(Control &c);                 /* [1] +0x14 0x0022FC30 */
    virtual void translateSync(Control &c);             /* [2] +0x1C 0x0022FE58 (empty) */
    virtual void setActuator(int ch, int value);        /* [3] +0x24 0x0022FE60 no range check */
    virtual void stopActuators();                       /* [4] +0x2C 0x0022FE70 */
    virtual int  isReady();                             /* [5] +0x34 0x0022FEA0 */

    /* init / setPadId take an int (u105 matching): the callers pass the
     * pad index / mode id without the s8 / u8 conversion. */
    void init(int port);                    /* 0x0022FB60 status 0, no pad id, actuators enabled */
    void update();                          /* 0x0022FB78 once a frame (PadManager::update) */
    void setPadId(int id);                  /* 0x0022FBB0 0: no pad; then this frame's motors go out */
    u8  *save(u8 *buf);                     /* 0x0022FEA8 into a link frame (0x28 bytes) */
    const u8 *load(const u8 *buf);          /* 0x0022FF00 from a link frame */
    /* Inline (no callers): egcs emits the copies with the vtable, the
     * last-declared first. */
    int  getPSType() const { return st.psType; }        /* 0x0022FF90 */
    PadData *getData() { return &st.data; }             /* 0x0022FF88 */
    void actuatorEnable(bool on) { actEnable = on; }    /* 0x0022FF80 */
};
EGCS_SIZE(PSPort, 0x3C);

/* The pad thread (? names). */
void padResetState(PadState *p);                        /* 0x0022F2A8 restarts the state machines of a pad */
void padThreadMain(void *arg);                          /* 0x0022F328 arg = the PadThread 0x004A09C0 */
void padStart(void);                                    /* 0x0022F810 libpad up, ports 1 and 2 opened, thread started */
void padStop(void);                                     /* 0x0022F8D8 */
void padReadAll(void);                                  /* 0x0022F938 the main thread's read of pads 0 and 4 */
void padWakeThread(void);                               /* 0x0022F9B0 */
u8   padGetData(int idx, PadData *out);                 /* 0x0022F9E8 the mode id once the pad was seen stable, else 0 */
void padSetActuators(int idx, const u8 *act);           /* 0x0022FA58 3 motor values for the pad thread */
const PadTypeEntry *padFindType(u8 id);                 /* 0x0022FAC0 the entry of mode id `id', or {0, 0} */

#endif /* GAME_T477_PS_PORT_H */
