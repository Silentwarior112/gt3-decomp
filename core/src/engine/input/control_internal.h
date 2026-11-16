/* control_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u105,
 * 0x0022DA98..0x00231190: the controller input layer.
 *
 * Translation units (link order):
 *   T474  Control: the input state of one player (buttons, the previous
 *         frame's buttons, 32 analog channels, key repeat), bound to a
 *         PortBase it is translated from every frame.
 *   T480  IEEE1394Port: the port of one i.LINK node in a link battle: a
 *         PSPort + a USBSteeringPort (+ a SteeringPSPort over both) filled
 *         from the node's frame; actuator requests go to the local port.
 *   T475  PadManager (? name; no RTTI): the 8 PS pad ports, 2 USB wheels,
 *         2 SteeringPSPorts (pad + wheel of ports 1 and 2), 8 IEEE1394Ports,
 *         the list of attached Controls and a semaphore; the global
 *         instance g_002EA7F8 and a spare Control g_002EB170.
 *   T476  PortBase: the abstract port (status + 5 pure virtuals).
 *   T477  PSPort and the pad thread: libpad 2200 driven from a thread that
 *         sets the analog mode (locked), the pressure mode and the
 *         actuators of ports 1 and 2 (slot 0 only).
 *   T478  SteeringPSPort: a PSPort and a USBSteeringPort merged into one.
 *   T479  USBSteeringPort: a USB wheel (Logitech 0x46D: GT Force 0xC293 /
 *         0xC294; 0x6F8 product 4), its auto-centring sequence and force
 *         feedback, its calibration from the game options.
 *
 * Canonical headers (include/game/):
 *   T474_control.h ... T480_ieee1394_port.h  own: the classes, the pad thread
 *                            functions, the T479 button helpers
 *   T473_link_session.h      the i.LINK session (frames, nodes): included by
 *                            T475 and T480 only
 *   T426_controller_config.h ControllerConfig (the wheel calibration) and
 *   T429_game_status.h       getGameStatus, GameStatus::options.config:
 *                            included by T479 only, after its own header
 *   T087_logger_windows.h    g_003416F8 (the analog stick curve): T477 only
 *   (Retail's .rodata shows the split: T474..T478 and T480 hold only their
 *   vtables and type names, T479 the garage lists' type names after its
 *   own vtable; u105 matching.)
 *   G002_mpeg_frameq.h       intr_suspend, semaphore / thread helpers,
 *                            pjoy_* / ff_* (the USB wheel driver)
 *   G004_libcdvd.h           libpad (core addresses), pad_rpc_cmd_0x14
 * Local to the unit (below): the pad thread's state (PadState, PadThread,
 * its stack and DMA areas), the PS type table, the 0xC294 button table, the
 * typeinfo storage of T476..T480, the PjoyState / PjoyInfo / FfCmd layouts
 * and the unit's named constants.
 *
 * Offsets in comments are the ORIGINAL (egcs) ones.  egcs puts the vptr
 * after the data members of the first polymorphic class: Control +0xC0,
 * PortBase +0x4 (after `status').  A modern g++ puts it at +0, which moves
 * Control's members by 4 (PortBase stays 8 bytes either way, so the members
 * of the derived ports keep their offsets).
 */
#ifndef U105_H
#define U105_H

#include "types.h"
#include "libc.h"                           /* memset, memcmp */
#include "cxxrt.h"                          /* __rtti_si, __rtti_user, type_info */
#include "eekernel.h"                       /* WaitSema, DeleteSema, ExitDeleteThread, EI */
#include "libpad.h"                         /* SCE_PAD* button bits, scePadState*, InfoMode* */

#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend, create_mutex_sema, signal_sema_helper,
                                               thread_start_retry, pjoy_read_state / pjoy_get_info,
                                               ff_send_download_play / ff_build_constant */
#include "sdk/libcdvd/G004_libcdvd.h"              /* scePad* (core addresses), pad_rpc_cmd_0x14 */
#include "engine/input/T474_control.h"              /* own: Control, ControlButton, g_00352CE0 */
#include "engine/input/T475_pad_manager.h"          /* own: PadManager, g_002EA7F8, g_002EB170 */
#include "engine/input/T476_port_base.h"            /* own: PortBase, ControlStatus */
#include "engine/input/T477_ps_port.h"              /* own: PSPort, PadData, PadTypeEntry, pad* */
#include "engine/input/T478_steering_ps_port.h"     /* own: SteeringPSPort */
#include "engine/input/T479_usb_steering_port.h"    /* own: USBSteeringPort, USBPortState, usb* */
#include "engine/input/T480_ieee1394_port.h"        /* own: IEEE1394Port */

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */

/* PadManager attach slots (attach, getPortStatus) and the array sizes. */
enum {
    SLOT_PS       = 0,                      /* 0-7   ps[]: PS pad i */
    SLOT_USB      = 10,                     /* 10-11 usb[]: USB device 0 / 1 */
    SLOT_LINK     = 12,                     /* 12-19 link[]: i.LINK node 0..7 */
    SLOT_STEERING = 20,                     /* 20-21 steering[]: pad + wheel of port 1 / 2 */
    PS_PORTS       = 8,
    USB_PORTS      = 2,
    LINK_PORTS     = 8,
    STEERING_PORTS = 2
};

/* ControlStatus values PORT_NONE / PORT_CONNECTED: T476.  The PS pad
 * types (Control::type bits 0-4, PS_PAD_*), the USB wheel types (bits 5-7,
 * USB_WHEEL_*, USB_TYPE_SHIFT), the raw pad bits (PADRAW_*) and the
 * channels (CHAN_*): T474. */

/* Pad mode ids (PadData::b[1]) */
#define PADID_DIGITAL    0x41
#define PADID_ANALOG     0x53
#define PADID_DUALSHOCK  0x73
#define PADID_DUALSHOCK2 0x79
/* scePadSetMainMode lock argument (ps2sdk PAD_MMODE_LOCK) */
#define PAD_MMODE_LOCK   3

/* USB ids (pjoy_get_info).  0xC293 is the GT Force; 0xC294 and 0xC295 (?
 * the Driving Force and the MOMO Force: Linux calls them "Wheel" and "MOMO";
 * 0xC295 is treated as a GT Force). */
#define USB_VENDOR_LOGITECH     0x046D
#define USB_VENDOR_6F8          0x06F8      /* (? Guillemot) */
#define USB_PRODUCT_GTFORCE     0xC293
#define USB_PRODUCT_C294        0xC294
#define USB_PRODUCT_C295        0xC295
#define USB_PRODUCT_6F8_WHEEL   0x0004
#define USB_DEVICE_6F8_WHEEL    0x000406F8  /* USBPortState::id as a u32: vendor | product << 16 */

/* pjoy_read_state results */
#define PJOY_STATE_OK       0
#define PJOY_STATE_REMOVED  3                   /* no device (G002 PJOY_NO_DEVICE) */
#define PJOY_STATE_BUSY     5                   /* the driver is busy (G002 PJOY_BUSY): next frame */

/* The hat in a wheel's raw button word (usbC294Buttons, usb6F8Buttons). */
enum {
    WHEEL_HAT_UP    = 0x040,
    WHEEL_HAT_DOWN  = 0x080,
    WHEEL_HAT_LEFT  = 0x100,
    WHEEL_HAT_RIGHT = 0x200
};

/* USBSteeringPort force feedback: 0x7F is the centre (no force), 0 / 0xFF
 * full one way / the other (autoCentre). */
#define FF_NONE   0x7F
#define FF_LEFT   0x00                      /* (? side) */
#define FF_RIGHT  0xFF

/* The analog value of a pressed button of a pad without pressures. */
#define PRESSURE_DOWN 0x80
/* The stick curve's centre and the distance from it that counts as a
 * direction (PSPort::translate). */
#define STICK_CENTRE    0x80
#define STICK_THRESHOLD 0x41

/* ======================================================================== */
/* The pad thread (T477)                                                    */
/* ======================================================================== */

/* The 7 bytes of scePadSetActDirect. */
struct PadActDirect {
    u8 b[7];
};

/* The pad thread's state of one pad (0x5D bytes, byte aligned). */
struct PadState {
    PadData      data;                      /* 0x00 read by the main thread (padReadAll) */
    PadData      rdata;                     /* 0x20 read by the pad thread */
    u8           act[3];                    /* 0x40 the requested motors (padSetActuators) */
    u8           actAlign[6];               /* 0x43 scePadSetActAlign data */
    PadActDirect actDirect;                 /* 0x49 scePadSetActDirect data */
    PadActDirect actSent;                   /* 0x50 the last one sent */
    u8           stable;                    /* 0x57 1 once the pad was seen FindCTP1 / Stable */
    u8           modeState;                 /* 0x58 PAD_MAINMODE_*: the main mode state machine */
    u8           pressState;                /* 0x59 PAD_PRESS_*: the pressure mode state machine */
    u8           actState;                  /* 0x5A PAD_ACT_*: the actuator state machine */
    u8           actCount;                  /* 0x5B scePadInfoAct(-1): number of actuators */
    u8           unk_5C;
};
static_assert(sizeof(PadState) == 0x5D, "PadState");

/* PadState::modeState */
enum { PAD_MAINMODE_DONE = 0, PAD_MAINMODE_WAIT = 1, PAD_MAINMODE_SET = 2 };
/* PadState::pressState */
enum { PAD_PRESS_DONE = 0, PAD_PRESS_WAIT = 1, PAD_PRESS_ENTER = 2, PAD_PRESS_CHECK = 3 };
/* PadState::actState */
enum { PAD_ACT_NONE = 0, PAD_ACT_SEND = 1, PAD_ACT_ALIGN = 2 };

#define PAD_COUNT        8                  /* pad i = port i >> 2, slot i & 3 */
#define PAD_STEP         4                  /* only slot 0 of ports 1 and 2 is polled */
#define PAD_THREAD_PRIO  4
#define PAD_STACK_SIZE   0x1000

/* At 0x004A09C0, after the pad thread's stack. */
struct PadThread {
    int      quitSema;                      /* 0x0 -1 while running; padStop puts a semaphore here */
    int      sema;                          /* 0x4 signalled once a frame (padWakeThread) */
    int      pending;                       /* 0x8 1 from the signal until the thread's pass */
    PadState pads[PAD_COUNT];               /* 0xC */
};
/* T477's file statics (its .bss, defined there): the scePadPortOpen DMA
 * areas g_0049F1C0, the pad thread's stack g_0049F9C0 and its state
 * g_004A09C0; and its PS type table g_00341A38 (.rodata, internal
 * linkage). */

/* ======================================================================== */
/* The USB wheel driver's records (G002, u123)                              */
/* ======================================================================== */

/* PjoyState, PjoyInfo, FfCmd: engine/media/G002_mpeg_frameq.h. */

/* ======================================================================== */
/* Other unit-private data                                                  */
/* ======================================================================== */

/* The 0xC294 wheel's 12 buttons as raw bits: defined in T479 (the first
 * word of its .rodata). */
extern const u32 g_00341AB0[12];            /* 0x00341AB0 */

#ifndef BOOT_OLD_GCC
/* The typeinfo storage of T476..T480 (__ti7Control is T474's header's). */
extern "C" {
extern u32 __ti8PortBase[2];                /* 0x00353480 gp-0x4570 __user_type_info */
extern u32 __ti12IEEE1394Port[4];           /* 0x004B2A00 __si_type_info */
extern u32 __ti6PSPort[4];                  /* 0x004B2A10 __si_type_info */
extern u32 __ti14SteeringPSPort[4];         /* 0x004B2A20 __si_type_info */
extern u32 __ti15USBSteeringPort[4];        /* 0x004B2A30 __si_type_info */
}
#endif

#endif /* U105_H */
