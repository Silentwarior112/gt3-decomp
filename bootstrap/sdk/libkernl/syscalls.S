/* syscalls.S - Sony libkernl (PsIIlibkernl2240) EE kernel syscall stub table,
 * as linked into the GT3 US bootstrap (BOOTSTRAP_SCUS-97102).
 *
 * Unit kernl_sys, range 0x0101B800..0x0101C04F.
 *
 * Every stub is the same 16-byte sequence
 *
 *     addiu $v1, $zero, N     # EE kernel call number (negative = interrupt-context form)
 *     syscall
 *     jr    $ra
 *     nop
 *
 * Arguments are passed through untouched in $a0..$t3; the kernel's result comes
 * back in $v0.  C prototypes are in include/eekernel.h (SIF ones in sif.h).
 *
 * Stubs 0x00..0x03 (0x0101B7C0..0x0101B7FF: RFU000, ResetEE, SetGsCrt, RFU003)
 * physically precede this table but fall inside the "libc" unit's address
 * range, so they are claimed (and defined) there, not here.
 *
 * Private duplicate stubs that live inside other libkernl objects are NOT in
 * this file: alarm.o's SetSyscall/Copy/GetEntryAddress (0x0101C4F0,
 * 0x0101C500, 0x0101C548) are in kernel.c; the inline `_iGetThreadId`
 * (-0x2F) syscall is inside iWakeupThread/iSuspendThread (kernel.c).
 *
 * funcs.json merges 35 stubs into one "function" 0x0101BC30..0x0101BE5F
 * (PollSema .. GetCop0); only its first address carries a FUNC marker, the
 * others are listed with their addresses in plain comments.  To match that
 * entry, PollSema is the one function symbol (.type/.ent/.end/.size) spanning
 * all 35 stubs, and the 34 stubs after it are plain global labels (ASTUB):
 * same code, same addresses and names, but not separate STT_FUNC symbols, so
 * tools/score.py (which pairs a .S file's function symbols with its FUNC
 * markers in order) maps every marker to its own stub.
 *
 * Numbering notes (checked against the binary):
 *  - 0xFC/0xFD/-0xFE/-0xFF are the "new alarm" services that InitAlarm installs
 *    from the kernel patch blob.  The blob's own service table (blob+0x710,
 *    see kernel.c) maps 0xFC and 0xFE to the same set-alarm handler and 0xFD
 *    and 0xFF to the same release handler, so in THIS build +0xFD = ReleaseAlarm
 *    and -0xFE = iSetAlarm (ps2sdk's later numbering, ReleaseAlarm=0xFE /
 *    iSetAlarm=-0xFD, does not apply to this blob).
 *  - 0x49 is encoded positive (ps2sdk: iDeleteSema = -0x49).
 *  - 0x50..-0x5A are the unimplemented event-flag calls (the +/- pattern
 *    matches Create/Delete/Set/iSet/Clear/iClear/Wait/Poll/iPoll/Refer/iRefer);
 *    note InitAlarm re-points slot 0x5A to its private kernel Copy service.
 *  - there is no stub for 0x65, -0x69.
 */

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

/* A stub inside the merged PollSema entry: global label only (see above). */
#define ASTUB(name, num)		\
	.globl	name;			\
name:					\
	addiu	$3, $0, num;		\
	syscall;			\
	jr	$31;			\
	nop

// FUNC 0x0101B800 Exit
//   #0x04.  crt0 tail-jumps here with main()'s return value; kill() calls it too.
STUB(Exit, 0x04)
// FUNC 0x0101B810 RFU005
//   #0x05, reserved (ps2sdk: "ResumeIntrDispatch", arbitrary name).  No callers.
STUB(RFU005, 0x05)
// FUNC 0x0101B820 LoadExecPS2
//   #0x06.  No callers.
STUB(LoadExecPS2, 0x06)
// FUNC 0x0101B830 ExecPS2
//   #0x07.  Tail-called by exec_ps2_with_sbus_cleanup (0x01016F70) to start core.gt3.
STUB(ExecPS2, 0x07)
// FUNC 0x0101B840 RFU008
//   #0x08.  Slot 8 is re-pointed by InitAlarm to the alarm blob's
//   "return from alarm callback" service (blob+0x6C0); the trampoline copied to
//   0x00082000 issues it as `syscall -8`.  No callers of this stub.
STUB(RFU008, 0x08)
// FUNC 0x0101B850 RFU009
//   #0x09, reserved.  No callers.
STUB(RFU009, 0x09)
// FUNC 0x0101B860 AddSbusIntcHandler
//   #0x0A.  No callers.
STUB(AddSbusIntcHandler, 0x0A)
// FUNC 0x0101B870 RemoveSbusIntcHandler
//   #0x0B.  Called for causes 0..15 by exec_ps2_with_sbus_cleanup (0x01016F70).
STUB(RemoveSbusIntcHandler, 0x0B)
// FUNC 0x0101B880 Interrupt2Iop
//   #0x0C.
STUB(Interrupt2Iop, 0x0C)
// FUNC 0x0101B890 SetVTLBRefillHandler
//   #0x0D.
STUB(SetVTLBRefillHandler, 0x0D)
// FUNC 0x0101B8A0 SetVCommonHandler
//   #0x0E.
STUB(SetVCommonHandler, 0x0E)
// FUNC 0x0101B8B0 SetVInterruptHandler
//   #0x0F.
STUB(SetVInterruptHandler, 0x0F)
// FUNC 0x0101B8C0 AddIntcHandler
//   #0x10.  Used by the game's VBlank/GS handler setup (0x0100C490).
STUB(AddIntcHandler, 0x10)
// FUNC 0x0101B8D0 AddIntcHandler2
//   #0x10 again (second stub, extra-argument variant).  No callers.
STUB(AddIntcHandler2, 0x10)
// FUNC 0x0101B8E0 RemoveIntcHandler
//   #0x11.  Game code and sceGsResetGraph.
STUB(RemoveIntcHandler, 0x11)
// FUNC 0x0101B8F0 AddDmacHandler
//   #0x12.  sceSifInitCmd registers the SIF0 (ch5) handler through it.
STUB(AddDmacHandler, 0x12)
// FUNC 0x0101B900 AddDmacHandler2
//   #0x12 again (second stub).  No callers.
STUB(AddDmacHandler2, 0x12)
// FUNC 0x0101B910 RemoveDmacHandler
//   #0x13.  sceSifExitCmd and game code.
STUB(RemoveDmacHandler, 0x13)
// FUNC 0x0101B920 _EnableIntc
//   #0x14.  Raw call, wrapped by EnableIntc (0x0101C338).
STUB(_EnableIntc, 0x14)
// FUNC 0x0101B930 _DisableIntc
//   #0x15.  Wrapped by DisableIntc (0x0101C2D0).
STUB(_DisableIntc, 0x15)
// FUNC 0x0101B940 _EnableDmac
//   #0x16.  Wrapped by EnableDmac (0x0101C408).
STUB(_EnableDmac, 0x16)
// FUNC 0x0101B950 _DisableDmac
//   #0x17.  Wrapped by DisableDmac (0x0101C3A0).
STUB(_DisableDmac, 0x17)
// FUNC 0x0101B960 SetAlarm
//   #0xFC: new-alarm service installed by InitAlarm (blob handler +0x440).
//   Used by libcdvd (0x01013000) and libmc (0x01016170) delays.
STUB(SetAlarm, 0xFC)
// FUNC 0x0101B970 ReleaseAlarm
//   #0xFD: new-alarm release (blob handler +0x2A0, shared with 0xFF).  No callers.
STUB(ReleaseAlarm, 0xFD)
// FUNC 0x0101B980 _iEnableIntc
//   #-0x1A.  Wrapped by iEnableIntc (0x0101C470).
STUB(_iEnableIntc, -0x1A)
// FUNC 0x0101B990 _iDisableIntc
//   #-0x1B.  Wrapped by iDisableIntc (0x0101C490).
STUB(_iDisableIntc, -0x1B)
// FUNC 0x0101B9A0 _iEnableDmac
//   #-0x1C.  Wrapped by iEnableDmac (0x0101C4B0).
STUB(_iEnableDmac, -0x1C)
// FUNC 0x0101B9B0 _iDisableDmac
//   #-0x1D.  Wrapped by iDisableDmac (0x0101C4D0).
STUB(_iDisableDmac, -0x1D)
// FUNC 0x0101B9C0 iSetAlarm
//   #-0xFE: interrupt-context set-alarm (blob handler +0x440, shared with 0xFC).  No callers.
STUB(iSetAlarm, -0xFE)
// FUNC 0x0101B9D0 iReleaseAlarm
//   #-0xFF: interrupt-context release (blob handler +0x2A0).  No callers.
STUB(iReleaseAlarm, -0xFF)
// FUNC 0x0101B9E0 CreateThread
//   #0x20.
STUB(CreateThread, 0x20)
// FUNC 0x0101B9F0 DeleteThread
//   #0x21.
STUB(DeleteThread, 0x21)
// FUNC 0x0101BA00 StartThread
//   #0x22.
STUB(StartThread, 0x22)
// FUNC 0x0101BA10 ExitThread
//   #0x23.  (crt0 has its own private copy at 0x010000C0.)
STUB(ExitThread, 0x23)
// FUNC 0x0101BA20 ExitDeleteThread
//   #0x24.
STUB(ExitDeleteThread, 0x24)
// FUNC 0x0101BA30 TerminateThread
//   #0x25.
STUB(TerminateThread, 0x25)
// FUNC 0x0101BA40 iTerminateThread
//   #-0x26.
STUB(iTerminateThread, -0x26)
// FUNC 0x0101BA50 DisableDispatchThread
//   #0x27.
STUB(DisableDispatchThread, 0x27)
// FUNC 0x0101BA60 EnableDispatchThread
//   #0x28.
STUB(EnableDispatchThread, 0x28)
// FUNC 0x0101BA70 ChangeThreadPriority
//   #0x29.
STUB(ChangeThreadPriority, 0x29)
// FUNC 0x0101BA80 iChangeThreadPriority
//   #-0x2A.
STUB(iChangeThreadPriority, -0x2A)
// FUNC 0x0101BA90 RotateThreadReadyQueue
//   #0x2B.  Also executed by the thread server for queued request 1.
STUB(RotateThreadReadyQueue, 0x2B)
// FUNC 0x0101BAA0 _iRotateThreadReadyQueue
//   #-0x2C.  Not used even by iRotateThreadReadyQueue (which always queues).
STUB(_iRotateThreadReadyQueue, -0x2C)
// FUNC 0x0101BAB0 ReleaseWaitThread
//   #0x2D.
STUB(ReleaseWaitThread, 0x2D)
// FUNC 0x0101BAC0 iReleaseWaitThread
//   #-0x2E.
STUB(iReleaseWaitThread, -0x2E)
// FUNC 0x0101BAD0 GetThreadId
//   #0x2F.
STUB(GetThreadId, 0x2F)
// FUNC 0x0101BAE0 ReferThreadStatus
//   #0x30.
STUB(ReferThreadStatus, 0x30)
// FUNC 0x0101BAF0 iReferThreadStatus
//   #-0x31.
STUB(iReferThreadStatus, -0x31)
// FUNC 0x0101BB00 SleepThread
//   #0x32.
STUB(SleepThread, 0x32)
// FUNC 0x0101BB10 WakeupThread
//   #0x33.  Executed by the thread server for queued request 0.
STUB(WakeupThread, 0x33)
// FUNC 0x0101BB20 _iWakeupThread
//   #-0x34.  Raw call used by iWakeupThread (0x0101C7E0) when tid != current.
STUB(_iWakeupThread, -0x34)
// FUNC 0x0101BB30 CancelWakeupThread
//   #0x35.
STUB(CancelWakeupThread, 0x35)
// FUNC 0x0101BB40 iCancelWakeupThread
//   #-0x36.
STUB(iCancelWakeupThread, -0x36)
// FUNC 0x0101BB50 SuspendThread
//   #0x37.  Executed by the thread server for queued request 2.
STUB(SuspendThread, 0x37)
// FUNC 0x0101BB60 _iSuspendThread
//   #-0x38.  Raw call used by iSuspendThread (0x0101C8F8) when tid != current.
STUB(_iSuspendThread, -0x38)
// FUNC 0x0101BB70 ResumeThread
//   #0x39.
STUB(ResumeThread, 0x39)
// FUNC 0x0101BB80 iResumeThread
//   #-0x3A.
STUB(iResumeThread, -0x3A)
// FUNC 0x0101BB90 RFU059
//   #0x3B, reserved (ps2sdk: "JoinThread").  No callers.
STUB(RFU059, 0x3B)
// FUNC 0x0101BBA0 InitMainThread
//   #0x3C (ps2sdk "SetupThread").  crt0 issues its own inline syscall; no callers.
STUB(InitMainThread, 0x3C)
// FUNC 0x0101BBB0 InitHeap
//   #0x3D (ps2sdk "SetupHeap").  crt0 issues its own inline syscall; no callers.
STUB(InitHeap, 0x3D)
// FUNC 0x0101BBC0 EndOfHeap
//   #0x3E.  Used by sbrk (0x01022E28).
STUB(EndOfHeap, 0x3E)
// FUNC 0x0101BBD0 RFU063
//   #0x3F, reserved.  No callers.
STUB(RFU063, 0x3F)
// FUNC 0x0101BBE0 CreateSema
//   #0x40.
STUB(CreateSema, 0x40)
// FUNC 0x0101BBF0 DeleteSema
//   #0x41.
STUB(DeleteSema, 0x41)
// FUNC 0x0101BC00 SignalSema
//   #0x42.
STUB(SignalSema, 0x42)
// FUNC 0x0101BC10 iSignalSema
//   #-0x43.  Interrupt context: _request_end, the i-thread wrappers, libcdvd/libmc callbacks.
STUB(iSignalSema, -0x43)
// FUNC 0x0101BC20 WaitSema
//   #0x44.
STUB(WaitSema, 0x44)
// FUNC 0x0101BC30 PollSema
//   #0x45.  funcs.json merges this stub and the following 34 (0x0101BC30..
//   0x0101BE5F) into one entry, so the PollSema symbol spans all 35 and the
//   34 after it are global labels inside it (ASTUB, see the file header).
	.globl	PollSema
	.type	PollSema, @function
	.ent	PollSema
PollSema:
	addiu	$3, $0, 0x45
	syscall
	jr	$31
	nop
//   0x0101BC40  -0x46  iPollSema
ASTUB(iPollSema, -0x46)
//   0x0101BC50   0x47  ReferSemaStatus
ASTUB(ReferSemaStatus, 0x47)
//   0x0101BC60  -0x48  iReferSemaStatus
ASTUB(iReferSemaStatus, -0x48)
//   0x0101BC70   0x49  RFU073 (positive 0x49; ps2sdk's iDeleteSema is -0x49)  /* ? name */
ASTUB(RFU073, 0x49)
//   0x0101BC80   0x4A  SetOsdConfigParam
ASTUB(SetOsdConfigParam, 0x4A)
//   0x0101BC90   0x4B  GetOsdConfigParam
ASTUB(GetOsdConfigParam, 0x4B)
//   0x0101BCA0   0x4C  GetGsHParam
ASTUB(GetGsHParam, 0x4C)
//   0x0101BCB0   0x4D  GetGsVParam
ASTUB(GetGsVParam, 0x4D)
//   0x0101BCC0   0x4E  SetGsHParam
ASTUB(SetGsHParam, 0x4E)
//   0x0101BCD0   0x4F  SetGsVParam
ASTUB(SetGsVParam, 0x4F)
//   0x0101BCE0   0x50  CreateEventFlag        (event-flag names inferred from the +/- pattern)  /* ? */
ASTUB(CreateEventFlag, 0x50)
//   0x0101BCF0   0x51  DeleteEventFlag
ASTUB(DeleteEventFlag, 0x51)
//   0x0101BD00   0x52  SetEventFlag
ASTUB(SetEventFlag, 0x52)
//   0x0101BD10  -0x53  iSetEventFlag
ASTUB(iSetEventFlag, -0x53)
//   0x0101BD20   0x54  ClearEventFlag
ASTUB(ClearEventFlag, 0x54)
//   0x0101BD30  -0x55  iClearEventFlag
ASTUB(iClearEventFlag, -0x55)
//   0x0101BD40   0x56  WaitEventFlag
ASTUB(WaitEventFlag, 0x56)
//   0x0101BD50   0x57  PollEventFlag
ASTUB(PollEventFlag, 0x57)
//   0x0101BD60  -0x58  iPollEventFlag
ASTUB(iPollEventFlag, -0x58)
//   0x0101BD70   0x59  ReferEventFlagStatus
ASTUB(ReferEventFlagStatus, 0x59)
//   0x0101BD80  -0x5A  iReferEventFlagStatus
ASTUB(iReferEventFlagStatus, -0x5A)
//   0x0101BD90   0x5B  GetEntryAddress
ASTUB(GetEntryAddress, 0x5B)
//   0x0101BDA0   0x5C  EnableIntcHandler
ASTUB(EnableIntcHandler, 0x5C)
//   0x0101BDB0  -0x5C  iEnableIntcHandler
ASTUB(iEnableIntcHandler, -0x5C)
//   0x0101BDC0   0x5D  DisableIntcHandler
ASTUB(DisableIntcHandler, 0x5D)
//   0x0101BDD0  -0x5D  iDisableIntcHandler
ASTUB(iDisableIntcHandler, -0x5D)
//   0x0101BDE0   0x5E  EnableDmacHandler
ASTUB(EnableDmacHandler, 0x5E)
//   0x0101BDF0  -0x5E  iEnableDmacHandler
ASTUB(iEnableDmacHandler, -0x5E)
//   0x0101BE00   0x5F  DisableDmacHandler
ASTUB(DisableDmacHandler, 0x5F)
//   0x0101BE10  -0x5F  iDisableDmacHandler
ASTUB(iDisableDmacHandler, -0x5F)
//   0x0101BE20   0x60  KSeg0
ASTUB(KSeg0, 0x60)
//   0x0101BE30   0x61  EnableCache
ASTUB(EnableCache, 0x61)
//   0x0101BE40   0x62  DisableCache
ASTUB(DisableCache, 0x62)
//   0x0101BE50   0x63  GetCop0
ASTUB(GetCop0, 0x63)
	.end	PollSema
	.size	PollSema, . - PollSema
// FUNC 0x0101BE60 FlushCache
//   #0x64.  crt0, InitAlarm (modes 0 and 2), sceSifInitCmd (0), game GS code.
STUB(FlushCache, 0x64)
// FUNC 0x0101BE70 CpuConfig
//   #0x66 (there is no stub for 0x65).
STUB(CpuConfig, 0x66)
// FUNC 0x0101BE80 iGetCop0
//   #-0x67.
STUB(iGetCop0, -0x67)
// FUNC 0x0101BE90 iFlushCache
//   #-0x68.
STUB(iFlushCache, -0x68)
// FUNC 0x0101BEA0 iCpuConfig
//   #-0x6A (no stub for -0x69).
STUB(iCpuConfig, -0x6A)
// FUNC 0x0101BEB0 sceSifStopDma
//   #0x6B.  Used by sceSifResetIop (0x010228B8).
STUB(sceSifStopDma, 0x6B)
// FUNC 0x0101BEC0 SetCPUTimerHandler
//   #0x6C.
STUB(SetCPUTimerHandler, 0x6C)
// FUNC 0x0101BED0 SetCPUTimer
//   #0x6D.
STUB(SetCPUTimer, 0x6D)
// FUNC 0x0101BEE0 SetOsdConfigParam2
//   #0x6E.
STUB(SetOsdConfigParam2, 0x6E)
// FUNC 0x0101BEF0 GetOsdConfigParam2
//   #0x6F.
STUB(GetOsdConfigParam2, 0x6F)
// FUNC 0x0101BF00 GsGetIMR
//   #0x70.
STUB(GsGetIMR, 0x70)
// FUNC 0x0101BF10 iGsGetIMR
//   #-0x70.
STUB(iGsGetIMR, -0x70)
// FUNC 0x0101BF20 GsPutIMR
//   #0x71.  sceGsResetGraph: GsPutIMR(0xFF00).
STUB(GsPutIMR, 0x71)
// FUNC 0x0101BF30 iGsPutIMR
//   #-0x71.
STUB(iGsPutIMR, -0x71)
// FUNC 0x0101BF40 SetPgifHandler
//   #0x72.
STUB(SetPgifHandler, 0x72)
// FUNC 0x0101BF50 SetVSyncFlag
//   #0x73.  Used by 0x01022C90.
STUB(SetVSyncFlag, 0x73)
// FUNC 0x0101BF60 SetSyscall
//   #0x74.  (alarm.o and initsys.o carry private copies at 0x0101C4F0 / 0x01022BC8.)
STUB(SetSyscall, 0x74)
// FUNC 0x0101BF70 _print
//   #0x75: kernel-ROM printf.  No callers.
STUB(_print, 0x75)
// FUNC 0x0101BF80 sceSifDmaStat
//   #0x76.  libpad DMA wait (0x01014410).
STUB(sceSifDmaStat, 0x76)
// FUNC 0x0101BF90 isceSifDmaStat
//   #-0x76.
STUB(isceSifDmaStat, -0x76)
// FUNC 0x0101BFA0 sceSifSetDma
//   #0x77.  Thread-context send path of _sif_cmd_send, libpad, sceSifResetIop, RPC server replies.
STUB(sceSifSetDma, 0x77)
// FUNC 0x0101BFB0 isceSifSetDma
//   #-0x77.  Interrupt-context send path of _sif_cmd_send (isceSifSendCmd).
STUB(isceSifSetDma, -0x77)
// FUNC 0x0101BFC0 sceSifSetDChain
//   #0x78.  Arms the SIF0 receive chain (sceSifInitCmd).
STUB(sceSifSetDChain, 0x78)
// FUNC 0x0101BFD0 isceSifSetDChain
//   #-0x78.  Re-arms SIF0 inside the command interrupt handler (0x0101D878).
STUB(isceSifSetDChain, -0x78)
// FUNC 0x0101BFE0 sceSifSetReg
//   #0x79.
STUB(sceSifSetReg, 0x79)
// FUNC 0x0101BFF0 sceSifGetReg
//   #0x7A.
STUB(sceSifGetReg, 0x7A)
// FUNC 0x0101C000 ExecOSD
//   #0x7B.  No callers.
STUB(ExecOSD, 0x7B)
// FUNC 0x0101C010 Deci2Call
//   #0x7C.  Backs the sceDeci2* wrappers and kPuts (0x01022F78..0x01023130).
STUB(Deci2Call, 0x7C)
// FUNC 0x0101C020 PSMode
//   #0x7D.
STUB(PSMode, 0x7D)
// FUNC 0x0101C030 MachineType
//   #0x7E.
STUB(MachineType, 0x7E)
// FUNC 0x0101C040 GetMemorySize
//   #0x7F.  Last stub of the table (cache.o follows at 0x0101C050).
STUB(GetMemorySize, 0x7F)
