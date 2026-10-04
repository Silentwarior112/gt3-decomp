/* eekernel.h - Sony EE kernel API (libkernl, PsIIlibkernl2240) as linked into
 * the GT3 US bootstrap (BOOTSTRAP_SCUS-97102).
 *
 * Sony SDK 2.2-era names.  Every syscall stub of the libkernl stub table
 * (16-byte `addiu $v1,$0,N; syscall; jr $ra; nop`, owned by the kernl_sys unit
 * in sdk/libkernl/syscalls.S) is listed in the table below with its address and
 * syscall number as read from the binary.  Stubs whose Sony name/arguments are
 * unknown (RFU, unimplemented event flags) are listed in the table only and get
 * no C prototype - nothing in the binary calls them.
 *
 * SIF syscalls (sceSifSetDma, sceSifDmaStat, ...) are declared in sif.h.
 *
 * Types: Sony's u_long is 64-bit on ee-gcc; the modern toolchain uses n32
 * (long = 32 bit), so 64-bit quantities are spelled u64 / s64 here.
 */
#ifndef BOOT_EEKERNEL_H
#define BOOT_EEKERNEL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* Structures                                                               */
/* ======================================================================== */

/* Thread parameter / status block (CreateThread, ReferThreadStatus).
 * Layout confirmed in the binary: libcdvd's param block @0x010482C8 writes
 * +0x04 entry, +0x08 stack, +0x0C stackSize, +0x10 gpReg, +0x14 initPriority;
 * io's yield_low_priority reads +0x18 currentPriority after ReferThreadStatus. */
struct ThreadParam {
    int           status;           /* +0x00 THS_*                              */
    void        (*entry)(void *);   /* +0x04                                    */
    void         *stack;            /* +0x08                                    */
    int           stackSize;        /* +0x0C                                    */
    void         *gpReg;            /* +0x10 (always &_gp = 0x01049CF0 here)    */
    int           initPriority;     /* +0x14                                    */
    int           currentPriority;  /* +0x18                                    */
    unsigned int  attr;             /* +0x1C                                    */
    unsigned int  option;           /* +0x20                                    */
    int           waitType;         /* +0x24 TSW_*                              */
    int           waitId;           /* +0x28                                    */
    int           wakeupCount;      /* +0x2C                                    */
};                                  /* 0x30 */

/* Semaphore parameter / status block (CreateSema, ReferSemaStatus).
 * Binary: CreateSema callers fill +0x04 maxCount, +0x08 initCount, +0x14 option. */
struct SemaParam {
    int           currentCount;     /* +0x00 */
    int           maxCount;         /* +0x04 */
    int           initCount;        /* +0x08 */
    int           numWaitThreads;   /* +0x0C */
    unsigned int  attr;             /* +0x10 */
    unsigned int  option;           /* +0x14 */
};                                  /* 0x18 */

#ifdef __cplusplus
static_assert(sizeof(struct ThreadParam) == 0x30, "ThreadParam");
static_assert(sizeof(struct SemaParam) == 0x18, "SemaParam");
#else
_Static_assert(sizeof(struct ThreadParam) == 0x30, "ThreadParam");
_Static_assert(sizeof(struct SemaParam) == 0x18, "SemaParam");
#endif

/* ThreadParam.status */
#define THS_RUN          0x01
#define THS_READY        0x02
#define THS_WAIT         0x04
#define THS_SUSPEND      0x08
#define THS_WAITSUSPEND  0x0C
#define THS_DORMANT      0x10

/* ThreadParam.waitType */
#define TSW_SLEEP        1
#define TSW_SEMA         2

/* INTC interrupt causes (AddIntcHandler / EnableIntc).  This binary installs
 * handlers for INTC_GS (0), INTC_VBLANK_S (2) and INTC_VIF1 (5).            */
#define INTC_GS          0
#define INTC_SBUS        1
#define INTC_VBLANK_S    2
#define INTC_VBLANK_E    3
#define INTC_VIF0        4
#define INTC_VIF1        5
#define INTC_VU0         6
#define INTC_VU1         7
#define INTC_IPU         8
#define INTC_TIM0        9
#define INTC_TIM1        10
#define INTC_TIM2        11
#define INTC_TIM3        12   /* used by the kernel alarm service */
#define INTC_SFIFO       13
#define INTC_VU0WD       14

/* DMAC channels (AddDmacHandler / EnableDmac).  The GS driver hooks 0/1/2.   */
#define DMAC_VIF0        0
#define DMAC_VIF1        1
#define DMAC_GIF         2
#define DMAC_FROM_IPU    3
#define DMAC_TO_IPU      4
#define DMAC_SIF0        5
#define DMAC_SIF1        6
#define DMAC_SIF2        7
#define DMAC_FROM_SPR    8
#define DMAC_TO_SPR      9
#define DMAC_CIS         13
#define DMAC_MEIS        14
#define DMAC_BEIS        15

/* FlushCache modes.  crt0 and the GS/libmc code use 0 (write back D-cache);
 * 2 invalidates the I-cache.                                                */
#define WRITEBACK_DCACHE   0
#define INVALIDATE_DCACHE  1
#define INVALIDATE_ICACHE  2
#define INVALIDATE_CACHE   3   /* ? D+I; not used by this binary */

/* COP0 Status.EIE (bit 16): master interrupt enable tested by DIntr/EIntr.   */
#define EE_COP0_STATUS_EIE 0x10000

/* ======================================================================== */
/* Interrupt enable helpers (inline, as emitted by Sony's macros)           */
/* ======================================================================== */

/* DI(): R5900 "di" errata loop - `1: di; sync.p; mfc0 $x,$12; and EIE;
 * bnez 1b`.  Found inlined in sbrk (0x01022E50) and Polyphony's intr_suspend
 * (0x01012F40); the out-of-line, value-returning form is DIntr().            */
static __inline__ void DI(void)
{
    u32 status;
    do {
        __asm__ __volatile__("di" : : : "memory");
        __asm__ __volatile__("sync.p" : : : "memory");
        __asm__ __volatile__("mfc0 %0, $12" : "=r"(status) : : "memory");
    } while (status & EE_COP0_STATUS_EIE);
}

/* EI(): bare `ei`. */
static __inline__ void EI(void)
{
    __asm__ __volatile__("ei" : : : "memory");
}

/* Epilogue of every INTC/DMAC handler in this binary: `sync.l; ei`
 * (e.g. 0x0100E744, 0x01016158, 0x0101D984).                                */
#define ExitHandler() __asm__ __volatile__("sync.l\n\tei" : : : "memory")

/* Read COP0 Status ($12). */
static __inline__ u32 _ee_get_status(void)
{
    u32 status;
    __asm__ __volatile__("mfc0 %0, $12" : "=r"(status));
    return status;
}

/* ======================================================================== */
/* Syscall stub table (kernl_sys unit, sdk/libkernl/syscalls.S)             */
/* ======================================================================== */
/*
 *  address     #      name (C prototype below unless marked "-")
 *  0x0101B7C0  0x00   - RFU000_FullReset        (reserved; no prototype)
 *  0x0101B7D0  0x01   ResetEE
 *  0x0101B7E0  0x02   SetGsCrt                  (tail-called by sceGsResetGraph)
 *  0x0101B7F0  0x03   - RFU003
 *  0x0101B800  0x04   Exit                      (crt0 tail-jumps here with main's result)
 *  0x0101B810  0x05   - RFU005                  (ps2sdk: "ResumeIntrDispatch", arbitrary)
 *  0x0101B820  0x06   LoadExecPS2
 *  0x0101B830  0x07   ExecPS2                   (tail-called by exec_ps2_with_sbus_cleanup)
 *  0x0101B840  0x08   - RFU008                  (ps2sdk: "ResumeT3IntrDispatch", arbitrary)
 *  0x0101B850  0x09   - RFU009
 *  0x0101B860  0x0A   AddSbusIntcHandler
 *  0x0101B870  0x0B   RemoveSbusIntcHandler
 *  0x0101B880  0x0C   Interrupt2Iop
 *  0x0101B890  0x0D   SetVTLBRefillHandler
 *  0x0101B8A0  0x0E   SetVCommonHandler
 *  0x0101B8B0  0x0F   SetVInterruptHandler
 *  0x0101B8C0  0x10   AddIntcHandler
 *  0x0101B8D0  0x10   AddIntcHandler2           (same number, 2nd stub)
 *  0x0101B8E0  0x11   RemoveIntcHandler
 *  0x0101B8F0  0x12   AddDmacHandler
 *  0x0101B900  0x12   AddDmacHandler2           (same number, 2nd stub)
 *  0x0101B910  0x13   RemoveDmacHandler
 *  0x0101B920  0x14   _EnableIntc
 *  0x0101B930  0x15   _DisableIntc
 *  0x0101B940  0x16   _EnableDmac
 *  0x0101B950  0x17   _DisableDmac
 *  0x0101B960  0xFC   SetAlarm                  (new-alarm service installed by InitAlarm)
 *  0x0101B970  0xFD   ReleaseAlarm              (? this build: +0xFD; ps2sdk uses 0xFE)
 *  0x0101B980 -0x1A   _iEnableIntc
 *  0x0101B990 -0x1B   _iDisableIntc
 *  0x0101B9A0 -0x1C   _iEnableDmac
 *  0x0101B9B0 -0x1D   _iDisableDmac
 *  0x0101B9C0 -0xFE   iSetAlarm                 (? ps2sdk uses -0xFD)
 *  0x0101B9D0 -0xFF   iReleaseAlarm
 *  0x0101B9E0  0x20   CreateThread
 *  0x0101B9F0  0x21   DeleteThread
 *  0x0101BA00  0x22   StartThread
 *  0x0101BA10  0x23   ExitThread
 *  0x0101BA20  0x24   ExitDeleteThread
 *  0x0101BA30  0x25   TerminateThread
 *  0x0101BA40 -0x26   iTerminateThread
 *  0x0101BA50  0x27   DisableDispatchThread
 *  0x0101BA60  0x28   EnableDispatchThread
 *  0x0101BA70  0x29   ChangeThreadPriority
 *  0x0101BA80 -0x2A   iChangeThreadPriority
 *  0x0101BA90  0x2B   RotateThreadReadyQueue
 *  0x0101BAA0 -0x2C   _iRotateThreadReadyQueue  (wrapped by iRotateThreadReadyQueue)
 *  0x0101BAB0  0x2D   ReleaseWaitThread
 *  0x0101BAC0 -0x2E   iReleaseWaitThread
 *  0x0101BAD0  0x2F   GetThreadId
 *  0x0101BAE0  0x30   ReferThreadStatus
 *  0x0101BAF0 -0x31   iReferThreadStatus
 *  0x0101BB00  0x32   SleepThread
 *  0x0101BB10  0x33   WakeupThread
 *  0x0101BB20 -0x34   _iWakeupThread            (wrapped by iWakeupThread)
 *  0x0101BB30  0x35   CancelWakeupThread
 *  0x0101BB40 -0x36   iCancelWakeupThread
 *  0x0101BB50  0x37   SuspendThread
 *  0x0101BB60 -0x38   _iSuspendThread           (wrapped by iSuspendThread)
 *  0x0101BB70  0x39   ResumeThread
 *  0x0101BB80 -0x3A   iResumeThread
 *  0x0101BB90  0x3B   - RFU059
 *  0x0101BBA0  0x3C   InitMainThread            (ps2sdk "SetupThread"; crt0 issues its own inline syscall)
 *  0x0101BBB0  0x3D   InitHeap                  (ps2sdk "SetupHeap";   crt0 issues its own inline syscall)
 *  0x0101BBC0  0x3E   EndOfHeap
 *  0x0101BBD0  0x3F   - RFU063
 *  0x0101BBE0  0x40   CreateSema
 *  0x0101BBF0  0x41   DeleteSema
 *  0x0101BC00  0x42   SignalSema
 *  0x0101BC10 -0x43   iSignalSema
 *  0x0101BC20  0x44   WaitSema
 *  0x0101BC30  0x45   PollSema                  (funcs.json merges 0x0101BC30..0x0101BE5F: 35 stubs)
 *  0x0101BC40 -0x46   iPollSema
 *  0x0101BC50  0x47   ReferSemaStatus
 *  0x0101BC60 -0x48   iReferSemaStatus
 *  0x0101BC70  0x49   - RFU073                  (? POSITIVE 0x49 here; ps2sdk iDeleteSema is -0x49)
 *  0x0101BC80  0x4A   SetOsdConfigParam
 *  0x0101BC90  0x4B   GetOsdConfigParam
 *  0x0101BCA0  0x4C   GetGsHParam
 *  0x0101BCB0  0x4D   GetGsVParam
 *  0x0101BCC0  0x4E   SetGsHParam
 *  0x0101BCD0  0x4F   SetGsVParam
 *  0x0101BCE0  0x50   - CreateEventFlag         (event-flag syscalls are RFU/unimplemented in the
 *  0x0101BCF0  0x51   - DeleteEventFlag          EE kernel; names inferred from the +/- pattern
 *  0x0101BD00  0x52   - SetEventFlag             0x50,0x51,0x52,-0x53,0x54,-0x55,0x56,0x57,-0x58,
 *  0x0101BD10 -0x53   - iSetEventFlag            0x59,-0x5A which matches Create/Delete/Set/iSet/
 *  0x0101BD20  0x54   - ClearEventFlag           Clear/iClear/Wait/Poll/iPoll/Refer/iRefer exactly;
 *  0x0101BD30 -0x55   - iClearEventFlag          ps2sdk instead names 0x54..0x5A xlaunch/TLB/Copy)
 *  0x0101BD40  0x56   - WaitEventFlag
 *  0x0101BD50  0x57   - PollEventFlag
 *  0x0101BD60 -0x58   - iPollEventFlag
 *  0x0101BD70  0x59   - ReferEventFlagStatus
 *  0x0101BD80 -0x5A   - iReferEventFlagStatus
 *  0x0101BD90  0x5B   GetEntryAddress
 *  0x0101BDA0  0x5C   EnableIntcHandler
 *  0x0101BDB0 -0x5C   iEnableIntcHandler
 *  0x0101BDC0  0x5D   DisableIntcHandler
 *  0x0101BDD0 -0x5D   iDisableIntcHandler
 *  0x0101BDE0  0x5E   EnableDmacHandler
 *  0x0101BDF0 -0x5E   iEnableDmacHandler
 *  0x0101BE00  0x5F   DisableDmacHandler
 *  0x0101BE10 -0x5F   iDisableDmacHandler
 *  0x0101BE20  0x60   KSeg0
 *  0x0101BE30  0x61   EnableCache
 *  0x0101BE40  0x62   DisableCache
 *  0x0101BE50  0x63   GetCop0
 *  0x0101BE60  0x64   FlushCache                (crt0 calls FlushCache(0))
 *  0x0101BE70  0x66   CpuConfig                 (no stub for 0x65)
 *  0x0101BE80 -0x67   iGetCop0
 *  0x0101BE90 -0x68   iFlushCache
 *  0x0101BEA0 -0x6A   iCpuConfig
 *  0x0101BEB0  0x6B   sceSifStopDma             (sif.h)
 *  0x0101BEC0  0x6C   SetCPUTimerHandler
 *  0x0101BED0  0x6D   SetCPUTimer
 *  0x0101BEE0  0x6E   SetOsdConfigParam2
 *  0x0101BEF0  0x6F   GetOsdConfigParam2
 *  0x0101BF00  0x70   GsGetIMR
 *  0x0101BF10 -0x70   iGsGetIMR
 *  0x0101BF20  0x71   GsPutIMR                  (sceGsResetGraph: GsPutIMR(0xFF00))
 *  0x0101BF30 -0x71   iGsPutIMR
 *  0x0101BF40  0x72   SetPgifHandler
 *  0x0101BF50  0x73   SetVSyncFlag
 *  0x0101BF60  0x74   SetSyscall
 *  0x0101BF70  0x75   _print
 *  0x0101BF80  0x76   sceSifDmaStat             (sif.h)
 *  0x0101BF90 -0x76   isceSifDmaStat            (sif.h)
 *  0x0101BFA0  0x77   sceSifSetDma              (sif.h)
 *  0x0101BFB0 -0x77   isceSifSetDma             (sif.h)
 *  0x0101BFC0  0x78   sceSifSetDChain           (sif.h)
 *  0x0101BFD0 -0x78   isceSifSetDChain          (sif.h)
 *  0x0101BFE0  0x79   sceSifSetReg              (sif.h)
 *  0x0101BFF0  0x7A   sceSifGetReg              (sif.h)
 *  0x0101C000  0x7B   ExecOSD
 *  0x0101C010  0x7C   Deci2Call
 *  0x0101C020  0x7D   PSMode
 *  0x0101C030  0x7E   MachineType
 *  0x0101C040  0x7F   GetMemorySize
 *
 *  Private duplicate stubs (must be static / differently named in C/asm):
 *  0x010000C0  0x23   crt0's ExitThread stub (thread root passed to InitMainThread; no jr)
 *  0x0101C4F0  0x74   alarm.o private SetSyscall
 *  0x0101C500  0x5A   alarm.o private Copy(dst, src, nbytes) (kernel-memory copy)
 *  0x0101C548  0x5B   alarm.o private GetEntryAddress
 *  0x01022BC8  0x74   initsys private SetSyscall (used by _InitSys's syscall-clear loop)
 *  inline     -0x2F   `_iGetThreadId` syscall inside iWakeupThread / iSuspendThread (0x0101C7EC, 0x0101C904)
 *  inline      0x3C/0x3D  crt0 InitMainThread / InitHeap (0x01000060 / 0x0100007C)
 */

/* Arguments/return types of stubs nothing in the binary calls follow the
 * ps2sdk protocol reference where Sony's are not known.                     */

/* ---- reset / exec --------------------------------------------------- */
void  ResetEE(int init_bitfield);                                        /* 0x01 */
void  SetGsCrt(short interlace, short omode, short ffmode);             /* 0x02 */
void  Exit(int status);                                                  /* 0x04 (kill() has code after the call, so not declared noreturn) */
void  LoadExecPS2(const char *filename, int argc, char **argv);         /* 0x06 */
int   ExecPS2(void *entry, void *gp, int argc, char **argv);            /* 0x07 */
void  ExecOSD(int argc, char **argv);                                    /* 0x7B */

/* ---- SBUS / exception vectors -------------------------------------- */
int   AddSbusIntcHandler(int cause, void (*handler)(int cause));         /* 0x0A */
int   RemoveSbusIntcHandler(int cause);                                  /* 0x0B */
int   Interrupt2Iop(int cause);                                          /* 0x0C */
void  SetVTLBRefillHandler(int cause, void *handler);                    /* 0x0D */
void  SetVCommonHandler(int cause, void *handler);                       /* 0x0E */
void  SetVInterruptHandler(int cause, void *handler);                    /* 0x0F */

/* ---- INTC / DMAC handlers ------------------------------------------ */
/* Handlers return non-zero to stop the chain; `next` 0 = add at head, -1 = tail.
 * Handlers end with ExitHandler().                                          */
int   AddIntcHandler(int cause, int (*handler)(int cause), int next);    /* 0x10 */
int   AddIntcHandler2(int cause, int (*handler)(int cause, void *arg, void *addr), int next, void *arg); /* 0x10 */
int   RemoveIntcHandler(int cause, int handler_id);                      /* 0x11 */
int   AddDmacHandler(int channel, int (*handler)(int channel), int next); /* 0x12 */
int   AddDmacHandler2(int channel, int (*handler)(int channel, void *arg, void *addr), int next, void *arg); /* 0x12 */
int   RemoveDmacHandler(int channel, int handler_id);                    /* 0x13 */
int   _EnableIntc(int cause);                                            /* 0x14 */
int   _DisableIntc(int cause);                                           /* 0x15 */
int   _EnableDmac(int channel);                                          /* 0x16 */
int   _DisableDmac(int channel);                                         /* 0x17 */
int   _iEnableIntc(int cause);                                           /* -0x1A */
int   _iDisableIntc(int cause);                                          /* -0x1B */
int   _iEnableDmac(int channel);                                         /* -0x1C */
int   _iDisableDmac(int channel);                                        /* -0x1D */
int   EnableIntcHandler(int cause);                                      /* 0x5C */
int   iEnableIntcHandler(int cause);                                     /* -0x5C */
int   DisableIntcHandler(int cause);                                     /* 0x5D */
int   iDisableIntcHandler(int cause);                                    /* -0x5D */
int   EnableDmacHandler(int channel);                                    /* 0x5E */
int   iEnableDmacHandler(int channel);                                   /* -0x5E */
int   DisableDmacHandler(int channel);                                   /* 0x5F */
int   iDisableDmacHandler(int channel);                                  /* -0x5F */

/* ---- alarms (kernel patch installed by InitAlarm) ------------------- */
/* time in H-SYNC units; callback(alarm_id, time, arg) runs in interrupt context. */
int   SetAlarm(u16 time, void (*callback)(int alarm_id, u16 time, void *arg), void *arg);  /* 0xFC */
int   ReleaseAlarm(int alarm_id);                                        /* 0xFD */
int   iSetAlarm(u16 time, void (*callback)(int alarm_id, u16 time, void *arg), void *arg); /* -0xFE */
int   iReleaseAlarm(int alarm_id);                                       /* -0xFF */

/* ---- threads -------------------------------------------------------- */
int   CreateThread(struct ThreadParam *param);                           /* 0x20 */
int   DeleteThread(int tid);                                             /* 0x21 */
int   StartThread(int tid, void *arg);                                   /* 0x22 */
void  ExitThread(void);                                                  /* 0x23 */
void  ExitDeleteThread(void);                                            /* 0x24 */
int   TerminateThread(int tid);                                          /* 0x25 */
int   iTerminateThread(int tid);                                         /* -0x26 */
int   DisableDispatchThread(void);                                       /* 0x27 */
int   EnableDispatchThread(void);                                        /* 0x28 */
int   ChangeThreadPriority(int tid, int priority);                       /* 0x29 */
int   iChangeThreadPriority(int tid, int priority);                      /* -0x2A */
int   RotateThreadReadyQueue(int priority);                              /* 0x2B */
int   _iRotateThreadReadyQueue(int priority);                            /* -0x2C */
int   ReleaseWaitThread(int tid);                                        /* 0x2D */
int   iReleaseWaitThread(int tid);                                       /* -0x2E */
int   GetThreadId(void);                                                 /* 0x2F */
int   ReferThreadStatus(int tid, struct ThreadParam *info);              /* 0x30 */
int   iReferThreadStatus(int tid, struct ThreadParam *info);             /* -0x31 */
int   SleepThread(void);                                                 /* 0x32 */
int   WakeupThread(int tid);                                             /* 0x33 */
int   _iWakeupThread(int tid);                                           /* -0x34 */
int   CancelWakeupThread(int tid);                                       /* 0x35 */
int   iCancelWakeupThread(int tid);                                      /* -0x36 */
int   SuspendThread(int tid);                                            /* 0x37 */
int   _iSuspendThread(int tid);                                          /* -0x38 */
int   ResumeThread(int tid);                                             /* 0x39 */
int   iResumeThread(int tid);                                            /* -0x3A */
void *InitMainThread(void *gp, void *stack, int stack_size, char *args, void (*root)(void)); /* 0x3C, returns sp */
void *InitHeap(void *heap, int heap_size);                               /* 0x3D (crt0: InitHeap(_end, -1)) */
void *EndOfHeap(void);                                                   /* 0x3E */

/* ---- semaphores ----------------------------------------------------- */
int   CreateSema(struct SemaParam *param);                               /* 0x40 */
int   DeleteSema(int sid);                                               /* 0x41 */
int   SignalSema(int sid);                                               /* 0x42 */
int   iSignalSema(int sid);                                              /* -0x43 */
int   WaitSema(int sid);                                                 /* 0x44 */
int   PollSema(int sid);                                                 /* 0x45 */
int   iPollSema(int sid);                                                /* -0x46 */
int   ReferSemaStatus(int sid, struct SemaParam *info);                  /* 0x47 */
int   iReferSemaStatus(int sid, struct SemaParam *info);                 /* -0x48 */

/* ---- OSD / GS CRT parameters --------------------------------------- */
void  SetOsdConfigParam(void *config);                                   /* 0x4A */
void  GetOsdConfigParam(void *config);                                   /* 0x4B */
void  GetGsHParam(void *p0, void *p1, void *p2);                         /* 0x4C */
int   GetGsVParam(void);                                                 /* 0x4D */
void  SetGsHParam(void *p0, void *p1, void *p2, void *p3);               /* 0x4E */
void  SetGsVParam(int p0);                                               /* 0x4F */
void  SetOsdConfigParam2(void *config, int size, int offset);            /* 0x6E */
void  GetOsdConfigParam2(void *config, int size, int offset);            /* 0x6F */

/* ---- misc kernel ---------------------------------------------------- */
void *GetEntryAddress(int syscall_no);                                   /* 0x5B */
void  KSeg0(int onoff);                                                  /* 0x60 */
int   EnableCache(int cache);                                            /* 0x61 */
int   DisableCache(int cache);                                           /* 0x62 */
u32   GetCop0(int reg);                                                  /* 0x63 */
void  FlushCache(int mode);                                              /* 0x64 */
u32   CpuConfig(u32 config);                                             /* 0x66 */
u32   iGetCop0(int reg);                                                 /* -0x67 */
void  iFlushCache(int mode);                                             /* -0x68 */
u32   iCpuConfig(u32 config);                                            /* -0x6A */
void  SetCPUTimerHandler(void (*handler)(void));                         /* 0x6C */
void  SetCPUTimer(int compare);                                          /* 0x6D */
u64   GsGetIMR(void);                                                    /* 0x70 */
u64   iGsGetIMR(void);                                                   /* -0x70 */
u64   GsPutIMR(u64 imr);                                                 /* 0x71 */
u64   iGsPutIMR(u64 imr);                                                /* -0x71 */
void  SetPgifHandler(void *handler);                                     /* 0x72 */
/* flag is 32-bit (sw/lw), csr 64-bit (ld) - see wait_vblank_get_csr 0x01022C90 */
void  SetVSyncFlag(u32 *flag, u64 *csr);                                 /* 0x73 */
void  SetSyscall(int syscall_no, void *handler);                         /* 0x74 */
void  _print(const char *fmt, ...);                                      /* 0x75 kernel-ROM printf */
int   Deci2Call(int func, u32 *param);                                   /* 0x7C */
void  PSMode(void);                                                      /* 0x7D */
int   MachineType(void);                                                 /* 0x7E */
int   GetMemorySize(void);                                               /* 0x7F */

/* ======================================================================== */
/* libkernl C functions (kernl_sys / kernl_rest units)                      */
/* ======================================================================== */

/* DIntr: if Status.EIE set, DI() loop; returns 1 if interrupts were enabled. */
int   DIntr(void);                                                       /* 0x01022B68 */
/* EIntr: returns Status.EIE!=0 (before), then `ei`.                          */
int   EIntr(void);                                                       /* 0x01022BB0 */

/* Cache maintenance: start/end are rounded down to 64-byte lines and passed
 * to the line loop (_SyncDCache 0x0101C050 / _InvalidDCache 0x0101C190).
 * The non-i versions bracket it with DIntr()/EIntr() only if Status.EIE was
 * set on entry; the i-versions (interrupt context) just tail-jump the loop. */
void  SyncDCache(void *start, void *end);                                /* 0x0101C0F8 */
void  iSyncDCache(void *start, void *end);                               /* 0x0101C178 */
void  InvalidDCache(void *start, void *end);                             /* 0x0101C238 */
void  iInvalidDCache(void *start, void *end);                            /* 0x0101C2B8 */

/* INTC/DMAC mask wrappers (DIntr / _xxx / EIntr) and interrupt-context forms. */
int   DisableIntc(int cause);                                            /* 0x0101C2D0 */
int   EnableIntc(int cause);                                             /* 0x0101C338 */
int   DisableDmac(int channel);                                          /* 0x0101C3A0 */
int   EnableDmac(int channel);                                           /* 0x0101C408 */
int   iEnableIntc(int cause);                                            /* 0x0101C470 */
int   iDisableIntc(int cause);                                           /* 0x0101C490 */
int   iEnableDmac(int channel);                                          /* 0x0101C4B0 */
int   iDisableDmac(int channel);                                         /* 0x0101C4D0 */

/* Interrupt-safe thread operations: work around the kernel bug via the
 * thread-server queue (InitThread / top thread).                           */
int   iWakeupThread(int tid);                                            /* 0x0101C7E0 */
int   iRotateThreadReadyQueue(int priority);                             /* 0x0101C878 */
int   iSuspendThread(int tid);                                           /* 0x0101C8F8 */

/* libkernl start-up, called by crt0 (0x01000008) before main.
 * = clear user syscalls 0x80..0xFF; InitAlarm(); tail-jump InitThread().    */
void  _InitSys(void);                                                    /* 0x01022C18 */

/* Kernel TTY printf: temporarily routes the putc hook (0x0103DA30) to the
 * DECI2 kputs path.  symbols.csv calls it "printf"; it is NOT the libc
 * printf (which is Polyphony's, see libc.h).  Used by libcdvd debug output
 * and by Polyphony's tty_puts (0x01012FC8, `scePrintf("%s", s)`).           */
int   scePrintf(const char *fmt, ...);                                   /* 0x0101D2A8 */

/* ---- DECI2 (Sony deci2.h API, implemented in libkernl via Deci2Call) ---- */
int   sceDeci2Open(unsigned short protocol, void *opt,
                   void (*handler)(int event, int param, void *opt));    /* 0x01022F78 */
int   sceDeci2Close(int s);                                              /* 0x01022FC0 */
int   sceDeci2ReqSend(int s, char dest);                                 /* 0x01022FE8 */
int   sceDeci2Poll(int s);                                               /* 0x01023018 */
int   sceDeci2ExRecv(int s, void *buf, unsigned short len);              /* 0x01023040 */
int   sceDeci2ExSend(int s, void *buf, unsigned short len);              /* 0x01023078 */
int   sceDeci2ExReqSend(int s, char dest);                               /* 0x010230B0 */
int   sceDeci2ExLock(int s);                                             /* 0x010230E0 */
int   sceDeci2ExUnLock(int s);                                           /* 0x01023108 */
void  kPuts(const char *s);                                              /* 0x01023130 Deci2Call(0x10, ...) */

/* ---- libkernl internals shared between the kernl_sys and kernl_rest units -- */
void  InitAlarm(void);                                                   /* 0x0101C558 (kernl_sys; called by _InitSys) */
/* Thread-server init (ps2sdk name "InitThread"): CreateSema{max 0xFF},
 * CreateThread{entry 0x0101C630, stack 0x01049F70, size 0x400, gp, prio 0},
 * StartThread, ChangeThreadPriority(self, 1).  _InitSys tail-jumps here.
 * Returns the server thread id, or -1 (already created / sema or thread
 * creation failed) - return type corrected from void by kernl_sys.         */
int   InitThread(void);                                                  /* 0x0101C708 (kernl_sys) */
/* Varargs shim over the kernel vprintf using the current putc hook (SIO CRLF). */
void  _printf(const char *fmt, ...);                                     /* 0x0101D270 (kernl_sys; used by DECI2 TTY handler) */

/* ======================================================================== */
/* Launch wrapper used by main                                              */
/* ======================================================================== */

/* RemoveSbusIntcHandler(0..15), then tail-call ExecPS2(entry, NULL, argc, argv).
 * Sits between libmc and libgcc in .text; owned by the libmc unit
 * (src/execps2.c or sdk/libmc - that unit decides and may rename; name is the
 * mapping pass's).  main calls it as (entry, 2, {path, "hot", NULL}).       */
void  exec_ps2_with_sbus_cleanup(void *entry, int argc, char **argv);    /* 0x01016F70 */

#ifdef __cplusplus
}
#endif

#endif /* BOOT_EEKERNEL_H */
