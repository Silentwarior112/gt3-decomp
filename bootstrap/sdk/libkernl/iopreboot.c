/* iopreboot.c - Sony libkernl 2.2.4 (PsIIlibkernl2240) IOP reset / reboot
 * (0x010228B8 .. 0x01022B68).
 *
 * main (F_01000880) reboots the IOP with the disc's replacement module set:
 *     while (!sceSifRebootIop("cdrom0:\\IOPRP224.IMG;1")) ;
 *     while (!sceSifSyncIop()) ;
 * sceSifRebootIop sends "rom0:UDNL cdrom0:\IOPRP224.IMG;1" to the IOP as a
 * SIF_CMDC_RESET_CMD packet; UDNL then reboots the IOP kernel from the image.
 */
#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "sif.h"
#include "kernl_rest_internal.h"

/* The reset packet (DMA'd to the IOP's SIF command buffer).                 */
static sceSifCmdResetData _sif_reset_pkt __attribute__((aligned(64)));   /* 0x0104D880 .bss */

// FUNC 0x010228B8 sceSifResetIop
//   Stop SIF DMA, fetch the IOP command-buffer address (system reg
//   0x80000000), build the RESET_CMD packet {psize 0x68, dsize 0, dest 0,
//   fcode 0x80000003, size = strlen(arg), flag = mode, arg} and DMA it
//   (attr 0x44 = ERT|INT_O).  Returns 0 if the DMA could not be queued;
//   otherwise sets SMFLAG SIFINIT and CMDINIT (acknowledging/clearing them),
//   zeroes the RPC-init and IOP-cmd-buffer system registers and returns 1.
//   Quirk: only strlen(arg) bytes are copied (no terminating NUL) and size
//   excludes the NUL; the rest of arg[] keeps whatever the static packet held
//   (zero on the first reset, since the packet lives in .bss).
int sceSifResetIop(const char *arg, int mode)
{
    sceSifDmaData dma;
    unsigned int iop_cmdbuf;
    int n;

    sceSifStopDma();
    iop_cmdbuf = sceSifGetReg(SIF_SYSREG_SUBADDR);

    _sif_reset_pkt.flag = mode;
    n = 0;
    if (arg[0] != 0) {
        do {
            _sif_reset_pkt.arg[n] = arg[n];
            n++;
        } while (arg[n] != 0);
    }
    _sif_reset_pkt.chdr.dest  = 0;
    _sif_reset_pkt.size       = n;
    _sif_reset_pkt.chdr.fcode = SIF_CMDC_RESET_CMD;
    _sif_reset_pkt.chdr.dsize = 0;      /* 64-bit ld/and 0xFFFFFFFF000000FF/sd */
    _sif_reset_pkt.chdr.psize = 0x68;   /* sb */

    dma.data = (unsigned int)&_sif_reset_pkt;
    dma.addr = iop_cmdbuf;
    dma.size = 0x68;
    dma.mode = SIF_DMA_ERT | SIF_DMA_INT_O;         /* 0x44 */
    sceSifWriteBackDCache(&_sif_reset_pkt, 0x68);
    if (sceSifSetDma(&dma, 1) != 0) {
        sceSifSetReg(SIF_REG_SMFLAG, SIF_STAT_SIFINIT);
        sceSifSetReg(SIF_REG_SMFLAG, SIF_STAT_CMDINIT);
        sceSifSetReg(SIF_SYSREG_RPCINIT, 0);
        sceSifSetReg(SIF_SYSREG_SUBADDR, 0);
        return 1;
    }
    return 0;
}

// FUNC 0x010229E8 sceSifIsAliveIop
//   (SMFLAG & SIFINIT) != 0.  No callers in this binary.
//   (symbols.csv: sceSifIopIsAlive; sif.h uses Sony's sceSifIsAliveIop.)
int sceSifIsAliveIop(void)
{
    if (sceSifGetReg(SIF_REG_SMFLAG) & SIF_STAT_SIFINIT)
        return 1;
    return 0;
}

// FUNC 0x01022A10 sceSifSyncIop
//   Poll for the end of an IOP reboot: if SMFLAG has BOOTEND, acknowledge it
//   (write the bit back), re-open the DECI2 TTY (the IOP reboot also reset
//   the DECI2 manager) and return 1; else 0.  main spins on it.
int sceSifSyncIop(void)
{
    if (sceSifGetReg(SIF_REG_SMFLAG) & SIF_STAT_BOOTEND) {
        sceSifSetReg(SIF_REG_SMFLAG, SIF_STAT_BOOTEND);
        tty_reinit_after_iop_reset();
        return 1;
    }
    return 0;
}

// FUNC 0x01022A58 sceSifRebootIop
//   Build "rom0:UDNL " + img in an 0x50-byte stack buffer; if
//   strlen(img) + 11 > 0x50 print "too long parameter '%s'\n" with the
//   C-library printf (resolved to Polyphony's printf F_01011678) and return 0.
//   Else shut down the RPC layer (sceSifInitRpc(0) to make sure it is set up,
//   then sceSifExitRpc) and return sceSifResetIop(buf, 0).
//   The strlen/strcpy/strcat are inlined builtins in the binary (no calls);
//   they are written out as loops here.
int sceSifRebootIop(const char *img)
{
    char param[0x50];
    const char *prefix = "rom0:UDNL ";          /* 0x01040720 */
    const char *p;
    char *d;

    for (p = img; *p != 0; p++)
        ;
    /* strlen(img) + strlen(prefix) + NUL, compared unsigned (size_t) */
    if (p + 11 - img > sizeof(param)) {
        printf("too long parameter '%s'\n", img);   /* 0x01040730 */
        return 0;
    }

    sceSifInitRpc(0);
    sceSifExitRpc();

    d = param;
    while (*prefix != 0)
        *d++ = *prefix++;
    while (*img != 0)
        *d++ = *img++;
    *d = 0;
    return sceSifResetIop(param, 0);
}
