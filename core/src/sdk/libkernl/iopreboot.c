/* iopreboot.c - Sony libkernl 2240 iopreboot.o: IOP reset /
 * reboot (0x0028D3F0..0x0028D6A0).  Same code as the bootstrap's
 * sdk/libkernl/iopreboot.c.  The game's IOPLoadModuleBase::cmdReboot (T524)
 * does
 *     while (!sceSifRebootIop(img)) ;  while (!sceSifSyncIop()) ;
 * ps2sdk's names (iopcontrol.h): SifIopReset, SifIopIsAlive, SifIopSync,
 * SifIopReboot.
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part2_internal.h"

/* iopreboot.o's .bss (0x004B1140..0x004B11A8, between loadfile.o's and
 * deci2.o's statics): the SIF_CMDC_RESET_CMD packet DMA'd to the IOP's SIF
 * command buffer, a file static aligned to the cache line (as the
 * bootstrap's iopreboot.c).  The alignment is also what egcs's code relies
 * on: knowing the packet 8-byte aligned, it clears chdr.dsize with retail's
 * 64-bit ld / and / sd (with the type's 4-byte alignment it uses lbu / sw /
 * sb, sceSifResetIop 80.3%).                                               */
static sceSifCmdResetData _sif_reset_pkt __attribute__((aligned(64)));  /* 0x004B1140 .bss */

// FUNC 0x0028D3F0 sceSifResetIop
//   Stop SIF DMA, fetch the IOP command-buffer address (system reg
//   SIF_SYSREG_SUBADDR), build the RESET_CMD packet {psize 0x68, dsize 0,
//   dest 0, fcode SIF_CMDC_RESET_CMD, size = strlen(arg), flag = mode, arg}
//   and DMA it (attr 0x44 = ERT|INT_O).  Returns 0 if the DMA could not be
//   queued; otherwise sets SMFLAG SIFINIT and CMDINIT (acknowledging /
//   clearing them), zeroes the RPC-init and IOP-cmd-buffer system registers
//   and returns 1.
//   Quirk: only strlen(arg) bytes are copied (no terminating NUL) and size
//   excludes the NUL (sif.h's "strlen(arg)+1" is wrong for this build); the
//   rest of arg[] keeps whatever the static packet held (zero on the first
//   reset, since the packet lives in .bss).
int sceSifResetIop(const char *arg, int mode)
{
    sceSifDmaData dma;
    unsigned int iop_cmdbuf;
    int len;

    sceSifStopDma();
    iop_cmdbuf = sceSifGetReg(SIF_SYSREG_SUBADDR);

    _sif_reset_pkt.flag = mode;
    len = 0;
    if (arg[0] != 0) {
        do {
            _sif_reset_pkt.arg[len] = arg[len];
            len++;
        } while (arg[len] != 0);
    }
    _sif_reset_pkt.chdr.dest  = 0;
    _sif_reset_pkt.size       = len;
    _sif_reset_pkt.chdr.fcode = SIF_CMDC_RESET_CMD;
    _sif_reset_pkt.chdr.dsize = 0;      /* 64-bit ld/and 0xFFFFFFFF000000FF/sd */
    _sif_reset_pkt.chdr.psize = sizeof(_sif_reset_pkt);    /* 0x68, sb */

    dma.data = (unsigned int)&_sif_reset_pkt;
    dma.addr = iop_cmdbuf;
    dma.size = sizeof(_sif_reset_pkt);
    dma.mode = SIF_DMA_ERT | SIF_DMA_INT_O;         /* 0x44 */
    sceSifWriteBackDCache(&_sif_reset_pkt, sizeof(_sif_reset_pkt));
    if (sceSifSetDma(&dma, 1) != 0) {
        sceSifSetReg(SIF_REG_SMFLAG, SIF_STAT_SIFINIT);
        sceSifSetReg(SIF_REG_SMFLAG, SIF_STAT_CMDINIT);
        sceSifSetReg(SIF_SYSREG_RPCINIT, 0);
        sceSifSetReg(SIF_SYSREG_SUBADDR, 0);
        return 1;
    }
    return 0;
}

// FUNC 0x0028D520 sceSifIsAliveIop
//   (SMFLAG & SIFINIT) != 0.  No callers in this binary.  (ps2sdk
//   iopcontrol.h: SifIopIsAlive.)
int sceSifIsAliveIop(void)
{
    if (sceSifGetReg(SIF_REG_SMFLAG) & SIF_STAT_SIFINIT)
        return 1;
    return 0;
}

// FUNC 0x0028D548 sceSifSyncIop
//   Poll for the end of an IOP reboot: if SMFLAG has BOOTEND, acknowledge it
//   (write the bit back), re-open the DECI2 TTY (the IOP reboot also reset
//   the DECI2 manager) and return 1; else 0.  IOPLoadModuleBase::cmdReboot
//   (T524) spins on it.
int sceSifSyncIop(void)
{
    if (sceSifGetReg(SIF_REG_SMFLAG) & SIF_STAT_BOOTEND) {
        sceSifSetReg(SIF_REG_SMFLAG, SIF_STAT_BOOTEND);
        tty_reinit_after_iop_reset();
        return 1;
    }
    return 0;
}

// FUNC 0x0028D590 sceSifRebootIop
//   Build "rom0:UDNL " + img in an 0x50-byte stack buffer; if
//   strlen(img) + 11 > 0x50 print "too long parameter '%s'\n" with the
//   C-library printf (Polyphony's printf 0x0026C690) and return 0.
//   Else shut down the RPC layer (sceSifInitRpc(0) to make sure it is set up,
//   then sceSifExitRpc) and return sceSifResetIop(buf, 0).
//   The strlen/strcpy/strcat are inlined builtins in the binary (no calls);
//   they are written out as loops here.
int sceSifRebootIop(const char *img)
{
    char param[IOP_RESET_ARG_MAX];
    const char *prefix = IOP_REBOOT_PREFIX;     /* 0x00349370 */
    const char *end;
    char *dst;

    for (end = img; *end != 0; end++)
        ;
    /* strlen(img) + strlen(prefix) + NUL (11), compared unsigned (size_t) */
    if (end + sizeof(IOP_REBOOT_PREFIX) - img > sizeof(param)) {
        printf("too long parameter '%s'\n", img);   /* 0x00349380 */
        return 0;
    }

    sceSifInitRpc(0);
    sceSifExitRpc();

    dst = param;
    while (*prefix != 0)
        *dst++ = *prefix++;
    while (*img != 0)
        *dst++ = *img++;
    *dst = 0;
    return sceSifResetIop(param, 0);
}
