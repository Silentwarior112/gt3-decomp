/* syscalls.S - Sony libkernl (PsIIlibkernl2240) EE kernel syscall
 * stub table, 0x00285840..0x002860CF (137 stubs, #0x00 .. #0x7F).
 *
 * Every stub is the same 16-byte sequence
 *
 *     addiu $v1, $zero, N     # EE kernel call number (negative = interrupt-context form)
 *     syscall
 *     jr    $ra
 *     nop
 *
 * Arguments pass through untouched in $a0..$t3; the kernel's result comes
 * back in $v0.  C prototypes: include/eekernel.h (SIF ones in sif.h); the
 * stubs other units call are also in runtime/newlib/G007_stdio.h, with core addresses.
 *
 * The same table as the bootstrap's (decomp/bootstrap/sdk/libkernl/syscalls.S
 * and runtime/libc_syscall_stubs.S for #0..#3), reused verbatim; the notes
 * under each marker are the bootstrap's, so addresses 0x010xxxxx in them are
 * bootstrap addresses.  Differences in this binary:
 *   - funcs.json cuts PollSema .. GetCop0 (0x00285CB0..0x00285ED0) as 35
 *     separate functions here (the bootstrap merges them into one entry), so
 *     every stub is a STUB.
 *   - index.txt names both #0x10 stubs AddIntcHandler2 and both #0x12 stubs
 *     AddDmacHandler2; the first of each pair keeps the bootstrap's name
 *     (AddIntcHandler / AddDmacHandler) so the symbols stay unique.
 *   - #0..#3 (RFU000_FullReset, ResetEE, SetGsCrt, RFU003) have no names in
 *     index.txt; they take the bootstrap's.
 * Numbering notes (from the bootstrap, checked against this listing):
 *   0xFC/0xFD/-0xFE/-0xFF are the "new alarm" services that InitAlarm
 *   installs; 0x49 is encoded positive; there is no stub for 0x65 or -0x69.
 */
#include "runtime/newlib/newlib_part3_internal.h"

/* libkernl's version object, "PsIIlibkernl2240" (16 bytes, no NUL), the head
 * of libkernl's .data: 0x00307A00, after newlib mallocr's statics and before
 * initsys's s_tty_opened (0x00307A10), i.e. the .data of this object, the
 * first libkernl object in link order.  The bootstrap has it the same way:
 * 0x0103D278, right before alarm.o's patch (cache.o and intr.o follow the
 * stub table there and have no .data).  fileio.o (u140) and loadfile.o
 * (u141) compare the IOP modules' versions with its "2240" (+0x0C); the
 * extern is in runtime/newlib/G007_stdio.h.  Only their code references it (its
 * +0x0C), from later in link order, so the range tools need the anchor. */
// DATA_START 0x00307A00
	.data
	.globl	_libkernl_version
	.type	_libkernl_version, @object
	.size	_libkernl_version, 16
_libkernl_version:
	.ascii	"PsIIlibkernl2240"

	.text
	.set	noreorder
	.set	noat

#define STUB(name, num)			\
	.globl	name;			\
	.type	name, @function;	\
	.ent	name;			\
name:					\
	addiu	$3, $0, num;		\
	syscall;			\
	jr	$31;			\
	nop;				\
	.end	name;			\
	.size	name, . - name

// FUNC 0x00285840 RFU000_FullReset
//   #0x00: reserved kernel entry (no prototype).  The table starts on a
//   64-byte boundary, after nop padding (0x00285824..).
STUB(RFU000_FullReset, 0x00)
// FUNC 0x00285850 ResetEE
//   #0x01: void ResetEE(int init_bitfield).
STUB(ResetEE, 0x01)
// FUNC 0x00285860 SetGsCrt
//   #0x02: void SetGsCrt(short interlace, short omode, short ffmode); in the
//   bootstrap sceGsResetGraph tail-calls it (j).
STUB(SetGsCrt, 0x02)
// FUNC 0x00285870 RFU003
//   #0x03: reserved kernel entry (no prototype).
STUB(RFU003, 0x03)
// FUNC 0x00285880 Exit
//   #0x04.  crt0 tail-jumps here with main()'s return value; kill() calls it too.
STUB(Exit, 0x04)
// FUNC 0x00285890 RFU005
//   #0x05, reserved (ps2sdk: "ResumeIntrDispatch", arbitrary name).  No callers.
STUB(RFU005, 0x05)
// FUNC 0x002858A0 LoadExecPS2
//   #0x06.  No callers.
STUB(LoadExecPS2, 0x06)
// FUNC 0x002858B0 ExecPS2
//   #0x07.  Tail-called by exec_ps2_with_sbus_cleanup (0x01016F70) to start core.gt3.
STUB(ExecPS2, 0x07)
// FUNC 0x002858C0 RFU008
//   #0x08.  Slot 8 is re-pointed by InitAlarm to the alarm blob's
//   "return from alarm callback" service (blob+0x6C0); the trampoline copied to
//   0x00082000 issues it as `syscall -8`.  No callers of this stub.
STUB(RFU008, 0x08)
// FUNC 0x002858D0 RFU009
//   #0x09, reserved.  No callers.
STUB(RFU009, 0x09)
// FUNC 0x002858E0 AddSbusIntcHandler
//   #0x0A.  No callers.
STUB(AddSbusIntcHandler, 0x0A)
// FUNC 0x002858F0 RemoveSbusIntcHandler
//   #0x0B.  Called for causes 0..15 by exec_ps2_with_sbus_cleanup (0x01016F70).
STUB(RemoveSbusIntcHandler, 0x0B)
// FUNC 0x00285900 Interrupt2Iop
//   #0x0C.
STUB(Interrupt2Iop, 0x0C)
// FUNC 0x00285910 SetVTLBRefillHandler
//   #0x0D.
STUB(SetVTLBRefillHandler, 0x0D)
// FUNC 0x00285920 SetVCommonHandler
//   #0x0E.
STUB(SetVCommonHandler, 0x0E)
// FUNC 0x00285930 SetVInterruptHandler
//   #0x0F.
STUB(SetVInterruptHandler, 0x0F)
// FUNC 0x00285940 AddIntcHandler
//   #0x10.  Used by the game's VBlank/GS handler setup (0x0100C490).
STUB(AddIntcHandler, 0x10)
// FUNC 0x00285950 AddIntcHandler2
//   #0x10 again (second stub, extra-argument variant).  No callers.
STUB(AddIntcHandler2, 0x10)
// FUNC 0x00285960 RemoveIntcHandler
//   #0x11.  Game code and sceGsResetGraph.
STUB(RemoveIntcHandler, 0x11)
// FUNC 0x00285970 AddDmacHandler
//   #0x12.  sceSifInitCmd registers the SIF0 (ch5) handler through it.
STUB(AddDmacHandler, 0x12)
// FUNC 0x00285980 AddDmacHandler2
//   #0x12 again (second stub).  No callers.
STUB(AddDmacHandler2, 0x12)
// FUNC 0x00285990 RemoveDmacHandler
//   #0x13.  sceSifExitCmd and game code.
STUB(RemoveDmacHandler, 0x13)
// FUNC 0x002859A0 _EnableIntc
//   #0x14.  Raw call, wrapped by EnableIntc (0x0101C338).
STUB(_EnableIntc, 0x14)
// FUNC 0x002859B0 _DisableIntc
//   #0x15.  Wrapped by DisableIntc (0x0101C2D0).
STUB(_DisableIntc, 0x15)
// FUNC 0x002859C0 _EnableDmac
//   #0x16.  Wrapped by EnableDmac (0x0101C408).
STUB(_EnableDmac, 0x16)
// FUNC 0x002859D0 _DisableDmac
//   #0x17.  Wrapped by DisableDmac (0x0101C3A0).
STUB(_DisableDmac, 0x17)
// FUNC 0x002859E0 SetAlarm
//   #0xFC: new-alarm service installed by InitAlarm (blob handler +0x440).
//   Used by libcdvd (0x01013000) and libmc (0x01016170) delays.
STUB(SetAlarm, 0xFC)
// FUNC 0x002859F0 ReleaseAlarm
//   #0xFD: new-alarm release (blob handler +0x2A0, shared with 0xFF).  No callers.
STUB(ReleaseAlarm, 0xFD)
// FUNC 0x00285A00 _iEnableIntc
//   #-0x1A.  Wrapped by iEnableIntc (0x0101C470).
STUB(_iEnableIntc, -0x1A)
// FUNC 0x00285A10 _iDisableIntc
//   #-0x1B.  Wrapped by iDisableIntc (0x0101C490).
STUB(_iDisableIntc, -0x1B)
// FUNC 0x00285A20 _iEnableDmac
//   #-0x1C.  Wrapped by iEnableDmac (0x0101C4B0).
STUB(_iEnableDmac, -0x1C)
// FUNC 0x00285A30 _iDisableDmac
//   #-0x1D.  Wrapped by iDisableDmac (0x0101C4D0).
STUB(_iDisableDmac, -0x1D)
// FUNC 0x00285A40 iSetAlarm
//   #-0xFE: interrupt-context set-alarm (blob handler +0x440, shared with 0xFC).  No callers.
STUB(iSetAlarm, -0xFE)
// FUNC 0x00285A50 iReleaseAlarm
//   #-0xFF: interrupt-context release (blob handler +0x2A0).  No callers.
STUB(iReleaseAlarm, -0xFF)
// FUNC 0x00285A60 CreateThread
//   #0x20.
STUB(CreateThread, 0x20)
// FUNC 0x00285A70 DeleteThread
//   #0x21.
STUB(DeleteThread, 0x21)
// FUNC 0x00285A80 StartThread
//   #0x22.
STUB(StartThread, 0x22)
// FUNC 0x00285A90 ExitThread
//   #0x23.  (crt0 has its own private copy at 0x010000C0.)
STUB(ExitThread, 0x23)
// FUNC 0x00285AA0 ExitDeleteThread
//   #0x24.
STUB(ExitDeleteThread, 0x24)
// FUNC 0x00285AB0 TerminateThread
//   #0x25.
STUB(TerminateThread, 0x25)
// FUNC 0x00285AC0 iTerminateThread
//   #-0x26.
STUB(iTerminateThread, -0x26)
// FUNC 0x00285AD0 DisableDispatchThread
//   #0x27.
STUB(DisableDispatchThread, 0x27)
// FUNC 0x00285AE0 EnableDispatchThread
//   #0x28.
STUB(EnableDispatchThread, 0x28)
// FUNC 0x00285AF0 ChangeThreadPriority
//   #0x29.
STUB(ChangeThreadPriority, 0x29)
// FUNC 0x00285B00 iChangeThreadPriority
//   #-0x2A.
STUB(iChangeThreadPriority, -0x2A)
// FUNC 0x00285B10 RotateThreadReadyQueue
//   #0x2B.  Also executed by the thread server for queued request 1.
STUB(RotateThreadReadyQueue, 0x2B)
// FUNC 0x00285B20 _iRotateThreadReadyQueue
//   #-0x2C.  Not used even by iRotateThreadReadyQueue (which always queues).
STUB(_iRotateThreadReadyQueue, -0x2C)
// FUNC 0x00285B30 ReleaseWaitThread
//   #0x2D.
STUB(ReleaseWaitThread, 0x2D)
// FUNC 0x00285B40 iReleaseWaitThread
//   #-0x2E.
STUB(iReleaseWaitThread, -0x2E)
// FUNC 0x00285B50 GetThreadId
//   #0x2F.
STUB(GetThreadId, 0x2F)
// FUNC 0x00285B60 ReferThreadStatus
//   #0x30.
STUB(ReferThreadStatus, 0x30)
// FUNC 0x00285B70 iReferThreadStatus
//   #-0x31.
STUB(iReferThreadStatus, -0x31)
// FUNC 0x00285B80 SleepThread
//   #0x32.
STUB(SleepThread, 0x32)
// FUNC 0x00285B90 WakeupThread
//   #0x33.  Executed by the thread server for queued request 0.
STUB(WakeupThread, 0x33)
// FUNC 0x00285BA0 _iWakeupThread
//   #-0x34.  Raw call used by iWakeupThread (0x0101C7E0) when tid != current.
STUB(_iWakeupThread, -0x34)
// FUNC 0x00285BB0 CancelWakeupThread
//   #0x35.
STUB(CancelWakeupThread, 0x35)
// FUNC 0x00285BC0 iCancelWakeupThread
//   #-0x36.
STUB(iCancelWakeupThread, -0x36)
// FUNC 0x00285BD0 SuspendThread
//   #0x37.  Executed by the thread server for queued request 2.
STUB(SuspendThread, 0x37)
// FUNC 0x00285BE0 _iSuspendThread
//   #-0x38.  Raw call used by iSuspendThread (0x0101C8F8) when tid != current.
STUB(_iSuspendThread, -0x38)
// FUNC 0x00285BF0 ResumeThread
//   #0x39.
STUB(ResumeThread, 0x39)
// FUNC 0x00285C00 iResumeThread
//   #-0x3A.
STUB(iResumeThread, -0x3A)
// FUNC 0x00285C10 RFU059
//   #0x3B, reserved (ps2sdk: "JoinThread").  No callers.
STUB(RFU059, 0x3B)
// FUNC 0x00285C20 InitMainThread
//   #0x3C (ps2sdk "SetupThread").  crt0 issues its own inline syscall; no callers.
STUB(InitMainThread, 0x3C)
// FUNC 0x00285C30 InitHeap
//   #0x3D (ps2sdk "SetupHeap").  crt0 issues its own inline syscall; no callers.
STUB(InitHeap, 0x3D)
// FUNC 0x00285C40 EndOfHeap
//   #0x3E.  Used by sbrk (0x01022E28).
STUB(EndOfHeap, 0x3E)
// FUNC 0x00285C50 RFU063
//   #0x3F, reserved.  No callers.
STUB(RFU063, 0x3F)
// FUNC 0x00285C60 CreateSema
//   #0x40.
STUB(CreateSema, 0x40)
// FUNC 0x00285C70 DeleteSema
//   #0x41.
STUB(DeleteSema, 0x41)
// FUNC 0x00285C80 SignalSema
//   #0x42.
STUB(SignalSema, 0x42)
// FUNC 0x00285C90 iSignalSema
//   #-0x43.  Interrupt context: _request_end, the i-thread wrappers, libcdvd/libmc callbacks.
STUB(iSignalSema, -0x43)
// FUNC 0x00285CA0 WaitSema
//   #0x44.
STUB(WaitSema, 0x44)
// FUNC 0x00285CB0 PollSema
//   #0x45.
STUB(PollSema, 0x45)
// FUNC 0x00285CC0 iPollSema
//   #-0x46.
STUB(iPollSema, -0x46)
// FUNC 0x00285CD0 ReferSemaStatus
//   #0x47.
STUB(ReferSemaStatus, 0x47)
// FUNC 0x00285CE0 iReferSemaStatus
//   #-0x48.
STUB(iReferSemaStatus, -0x48)
// FUNC 0x00285CF0 RFU073
//   #0x49.
STUB(RFU073, 0x49)
// FUNC 0x00285D00 SetOsdConfigParam
//   #0x4A.
STUB(SetOsdConfigParam, 0x4A)
// FUNC 0x00285D10 GetOsdConfigParam
//   #0x4B.
STUB(GetOsdConfigParam, 0x4B)
// FUNC 0x00285D20 GetGsHParam
//   #0x4C.
STUB(GetGsHParam, 0x4C)
// FUNC 0x00285D30 GetGsVParam
//   #0x4D.
STUB(GetGsVParam, 0x4D)
// FUNC 0x00285D40 SetGsHParam
//   #0x4E.
STUB(SetGsHParam, 0x4E)
// FUNC 0x00285D50 SetGsVParam
//   #0x4F.
STUB(SetGsVParam, 0x4F)
// FUNC 0x00285D60 CreateEventFlag
//   #0x50.
STUB(CreateEventFlag, 0x50)
// FUNC 0x00285D70 DeleteEventFlag
//   #0x51.
STUB(DeleteEventFlag, 0x51)
// FUNC 0x00285D80 SetEventFlag
//   #0x52.
STUB(SetEventFlag, 0x52)
// FUNC 0x00285D90 iSetEventFlag
//   #-0x53.
STUB(iSetEventFlag, -0x53)
// FUNC 0x00285DA0 ClearEventFlag
//   #0x54.
STUB(ClearEventFlag, 0x54)
// FUNC 0x00285DB0 iClearEventFlag
//   #-0x55.
STUB(iClearEventFlag, -0x55)
// FUNC 0x00285DC0 WaitEventFlag
//   #0x56.
STUB(WaitEventFlag, 0x56)
// FUNC 0x00285DD0 PollEventFlag
//   #0x57.
STUB(PollEventFlag, 0x57)
// FUNC 0x00285DE0 iPollEventFlag
//   #-0x58.
STUB(iPollEventFlag, -0x58)
// FUNC 0x00285DF0 ReferEventFlagStatus
//   #0x59.
STUB(ReferEventFlagStatus, 0x59)
// FUNC 0x00285E00 iReferEventFlagStatus
//   #-0x5A.
STUB(iReferEventFlagStatus, -0x5A)
// FUNC 0x00285E10 GetEntryAddress
//   #0x5B.
STUB(GetEntryAddress, 0x5B)
// FUNC 0x00285E20 EnableIntcHandler
//   #0x5C.
STUB(EnableIntcHandler, 0x5C)
// FUNC 0x00285E30 iEnableIntcHandler
//   #-0x5C.
STUB(iEnableIntcHandler, -0x5C)
// FUNC 0x00285E40 DisableIntcHandler
//   #0x5D.
STUB(DisableIntcHandler, 0x5D)
// FUNC 0x00285E50 iDisableIntcHandler
//   #-0x5D.
STUB(iDisableIntcHandler, -0x5D)
// FUNC 0x00285E60 EnableDmacHandler
//   #0x5E.
STUB(EnableDmacHandler, 0x5E)
// FUNC 0x00285E70 iEnableDmacHandler
//   #-0x5E.
STUB(iEnableDmacHandler, -0x5E)
// FUNC 0x00285E80 DisableDmacHandler
//   #0x5F.
STUB(DisableDmacHandler, 0x5F)
// FUNC 0x00285E90 iDisableDmacHandler
//   #-0x5F.
STUB(iDisableDmacHandler, -0x5F)
// FUNC 0x00285EA0 KSeg0
//   #0x60.
STUB(KSeg0, 0x60)
// FUNC 0x00285EB0 EnableCache
//   #0x61.
STUB(EnableCache, 0x61)
// FUNC 0x00285EC0 DisableCache
//   #0x62.
STUB(DisableCache, 0x62)
// FUNC 0x00285ED0 GetCop0
//   #0x63.
STUB(GetCop0, 0x63)
// FUNC 0x00285EE0 FlushCache
//   #0x64.  crt0, InitAlarm (modes 0 and 2), sceSifInitCmd (0), game GS code.
STUB(FlushCache, 0x64)
// FUNC 0x00285EF0 CpuConfig
//   #0x66 (there is no stub for 0x65).
STUB(CpuConfig, 0x66)
// FUNC 0x00285F00 iGetCop0
//   #-0x67.
STUB(iGetCop0, -0x67)
// FUNC 0x00285F10 iFlushCache
//   #-0x68.
STUB(iFlushCache, -0x68)
// FUNC 0x00285F20 iCpuConfig
//   #-0x6A (no stub for -0x69).
STUB(iCpuConfig, -0x6A)
// FUNC 0x00285F30 sceSifStopDma
//   #0x6B.  Used by sceSifResetIop (0x010228B8).
STUB(sceSifStopDma, 0x6B)
// FUNC 0x00285F40 SetCPUTimerHandler
//   #0x6C.
STUB(SetCPUTimerHandler, 0x6C)
// FUNC 0x00285F50 SetCPUTimer
//   #0x6D.
STUB(SetCPUTimer, 0x6D)
// FUNC 0x00285F60 SetOsdConfigParam2
//   #0x6E.
STUB(SetOsdConfigParam2, 0x6E)
// FUNC 0x00285F70 GetOsdConfigParam2
//   #0x6F.
STUB(GetOsdConfigParam2, 0x6F)
// FUNC 0x00285F80 GsGetIMR
//   #0x70.
STUB(GsGetIMR, 0x70)
// FUNC 0x00285F90 iGsGetIMR
//   #-0x70.
STUB(iGsGetIMR, -0x70)
// FUNC 0x00285FA0 GsPutIMR
//   #0x71.  sceGsResetGraph: GsPutIMR(0xFF00).
STUB(GsPutIMR, 0x71)
// FUNC 0x00285FB0 iGsPutIMR
//   #-0x71.
STUB(iGsPutIMR, -0x71)
// FUNC 0x00285FC0 SetPgifHandler
//   #0x72.
STUB(SetPgifHandler, 0x72)
// FUNC 0x00285FD0 SetVSyncFlag
//   #0x73.  Used by 0x01022C90.
STUB(SetVSyncFlag, 0x73)
// FUNC 0x00285FE0 SetSyscall
//   #0x74.  (alarm.o and initsys.o carry private copies at 0x0101C4F0 / 0x01022BC8.)
STUB(SetSyscall, 0x74)
// FUNC 0x00285FF0 _print
//   #0x75: kernel-ROM printf.  No callers.
STUB(_print, 0x75)
// FUNC 0x00286000 sceSifDmaStat
//   #0x76.  libpad DMA wait (0x01014410).
STUB(sceSifDmaStat, 0x76)
// FUNC 0x00286010 isceSifDmaStat
//   #-0x76.
STUB(isceSifDmaStat, -0x76)
// FUNC 0x00286020 sceSifSetDma
//   #0x77.  Thread-context send path of _sif_cmd_send, libpad, sceSifResetIop, RPC server replies.
STUB(sceSifSetDma, 0x77)
// FUNC 0x00286030 isceSifSetDma
//   #-0x77.  Interrupt-context send path of _sif_cmd_send (isceSifSendCmd).
STUB(isceSifSetDma, -0x77)
// FUNC 0x00286040 sceSifSetDChain
//   #0x78.  Arms the SIF0 receive chain (sceSifInitCmd).
STUB(sceSifSetDChain, 0x78)
// FUNC 0x00286050 isceSifSetDChain
//   #-0x78.  Re-arms SIF0 inside the command interrupt handler (0x0101D878).
STUB(isceSifSetDChain, -0x78)
// FUNC 0x00286060 sceSifSetReg
//   #0x79.
STUB(sceSifSetReg, 0x79)
// FUNC 0x00286070 sceSifGetReg
//   #0x7A.
STUB(sceSifGetReg, 0x7A)
// FUNC 0x00286080 ExecOSD
//   #0x7B.  No callers.
STUB(ExecOSD, 0x7B)
// FUNC 0x00286090 Deci2Call
//   #0x7C.  Backs the sceDeci2* wrappers and kPuts (0x01022F78..0x01023130).
STUB(Deci2Call, 0x7C)
// FUNC 0x002860A0 PSMode
//   #0x7D.
STUB(PSMode, 0x7D)
// FUNC 0x002860B0 MachineType
//   #0x7E.
STUB(MachineType, 0x7E)
// FUNC 0x002860C0 GetMemorySize
//   #0x7F.  Last stub of the table (cache.o follows at 0x0101C050).
STUB(GetMemorySize, 0x7F)
