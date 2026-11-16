/* core decomp: copied from bootstrap/include/libcdvd.h. Address comments in it are the
 * bootstrap's (0x010xxxxx); the core addresses of shared code are in ref/libmatch.json. */
/* libcdvd.h - Sony EE libcdvd (PsIIlibcdvd 2240) as linked into the GT3 US
 * bootstrap (BOOTSTRAP_SCUS-97102).  Sony SDK 2.2-era names.
 *
 * Owner: libcdvd unit (sdk/libcdvd/libcdvd.c, 0x01012FD8..0x01014410).
 * The bootstrap does NOT read the disc through libcdvd: core.gt3 and the IRX
 * modules are read via fileio "cdrom0:" (sifdev.h).  main only uses
 * sceCdInit / sceCdMmode / sceCdDiskReady.  sceCdRead & co. are not linked.
 *
 * IOP RPC servers used: 0x80000592 init, 0x80000593 S-command,
 * 0x80000595 N-command, 0x80000596 power-off callback, 0x80000597 search file,
 * 0x8000059A disk ready; SIF command 0x80000012 = power-off notification.
 */
#ifndef BOOT_LIBCDVD_H
#define BOOT_LIBCDVD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- sceCdInit modes ---------------------------------------------------- */
#define SCECdINIT       0x00    /* initialise and wait for the drive (main: sceCdInit(SCECdINIT)) */
#define SCECdINoD       0x01    /* initialise, don't wait                                  */
#define SCECdEXIT       0x05    /* shut down: 'Libcdvd Exit', tear down callback thread    */

/* ---- sceCdMmode media --------------------------------------------------- */
#define SCECdCD         1
#define SCECdDVD        2       /* main: sceCdMmode(SCECdDVD) */
#define SCECdMmodeCd    SCECdCD     /* ps2sdk spellings */
#define SCECdMmodeDvd   SCECdDVD

/* ---- sceCdDiskReady ------------------------------------------------------ */
/* Sony semantics: mode 0 = wait until ready, 1 = don't wait (main calls it
 * with 1 during init and with 0 from F_010001C8).                          */
#define SCECdComplete   0x02    /* ready                                            */
#define SCECdNotReady   0x06    /* busy / not ready (also returned while an S-cmd runs) */

/* ---- callback reasons (sceCdCallback) ------------------------------------ */
#define SCECdFuncRead       1
#define SCECdFuncReadCDDA   2
#define SCECdFuncGetToc     3
#define SCECdFuncSeek       4
#define SCECdFuncStandby    5
#define SCECdFuncStop       6
#define SCECdFuncPause      7
#define SCECdFuncBreak      8

/* ---- sector sizes / spindle (sceCdRMode) --------------------------------- */
#define SCECdSecS2048   0
#define SCECdSecS2328   1
#define SCECdSecS2340   2
#define SCECdSpinMax    0
#define SCECdSpinNom    1
#define SCECdSpinStm    0

/* ---- error codes (sceCdGetError) ----------------------------------------- */
#define SCECdErFAIL     (-1)
#define SCECdErNO       0x00
#define SCECdErABRT     0x01
#define SCECdErCMD      0x10
#define SCECdErOPENS    0x11
#define SCECdErNODISC   0x12
#define SCECdErNORDY    0x13
#define SCECdErCUD      0x14
#define SCECdErIPI      0x20
#define SCECdErILI      0x21
#define SCECdErPRM      0x22
#define SCECdErREAD     0x30
#define SCECdErTRMOPN   0x31
#define SCECdErEOM      0x32

/* ---- structures ------------------------------------------------------------ */

/* sceCdSearchFile result.  In this 2.24 build the struct is 0x24 bytes:
 * sceCdSearchFile copies 0x24 bytes (4 x ldl/ldr pairs + lwl/lwr) from the
 * uncached send buffer @0x01048340 back into the caller's struct.  The send
 * buffer is {sceCdlFILE result @+0x00, char name[256] @+0x24, EE address of
 * the buffer itself @+0x124} = 0x128 bytes.  (ps2sdk's 0x20-byte sceCdlFILE
 * lacks the trailing word.)                                                  */
typedef struct {
    unsigned int  lsn;        /* +0x00 */
    unsigned int  size;       /* +0x04 */
    char          name[16];   /* +0x08 */
    unsigned char date[8];    /* +0x18 */
    unsigned int  flag;       /* +0x20 ? name; copied but never read by the bootstrap */
} sceCdlFILE;                 /* 0x24 */

typedef struct {
    unsigned char trycount;     /* +0 */
    unsigned char spindlctrl;   /* +1 SCECdSpin* */
    unsigned char datapattern;  /* +2 SCECdSecS* */
    unsigned char pad;          /* +3 */
} sceCdRMode;

typedef struct {
    unsigned char stat;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char pad;
    unsigned char day;
    unsigned char month;
    unsigned char year;
} sceCdCLOCK;

typedef struct {
    unsigned char minute;
    unsigned char second;
    unsigned char sector;
    unsigned char track;
} sceCdlLOCCD;

#ifdef __cplusplus
static_assert(sizeof(sceCdlFILE) == 0x24, "sceCdlFILE");
#else
_Static_assert(sizeof(sceCdlFILE) == 0x24, "sceCdlFILE");
#endif

typedef void (*sceCdCBFunc)(int reason);

/* ---- API present in the binary -------------------------------------------- */
int   sceCdInit(int init_mode);                                            /* 0x01013E70 */
/* Media mode S-command 0x22; returns the IOP's 4-byte result, 0 on RPC error. */
int   sceCdMmode(int media);                                               /* 0x01014348 */
int   sceCdDiskReady(int mode);                                            /* 0x01014150 */
/* N-command 0x0E: drive-ready status via the N-command server.            */
int   sceCdNCmdDiskReady(void);                                            /* 0x01013B58 ? name (ps2sdk) */
/* mode 0: block until the current N-command (and the callback) completed,
 * returns 0; mode != 0: return 1 if still busy, else 0.                    */
int   sceCdSync(int mode);                                                 /* 0x01013BF0 */
int   sceCdSearchFile(sceCdlFILE *fp, const char *name);                   /* 0x010136E8 */
/* Returns the previous callback; returns 0 without swapping while busy.   */
sceCdCBFunc sceCdCallback(sceCdCBFunc func);                               /* 0x01013068 */
/* Returns the previous power-off callback.                                */
void *sceCdPOffCallback(void (*func)(void *arg), void *arg);               /* 0x010134B0 */
/* Starts the EE callback thread; unreferenced in this binary.              */
int   sceCdInitEeCB(int priority, void *stack, int stack_size);            /* 0x01013220 */

/* ---- libcdvd internals (libcdvd unit only; listed for reference) ---------
 *   0x01012FD8 _sceCdAlarmHandler   0x01013000 _sceCdDelay(u16 hsync)
 *   0x010130C0 _sceCdCbDispatch     0x01013160 _sceCdCallbackThread
 *   0x010132F8 _sceCdAlignReadCb    0x01013398 _sceCdSemaInit
 *   0x01013430 _sceCdEeCbExit       0x01013520 _sceCdPOffCmdHandler
 *   0x01013560 _sceCdPOffCbBind     0x010139E8 _sceCdCheckNCmd(int cmd)
 *   0x01013C90 _sceCdSyncS(int mode) 0x01013D00 _sceCdCheckSCmd(int cmd)
 * (all file-static in sdk/libcdvd/libcdvd.c, as are the library globals)
 * Debug prints go through scePrintf (eekernel.h), gated by the verbosity
 * word @0x0103A3D0.                                                         */

/* ---- Sony API NOT linked into this binary (no FUNC markers) --------------- */
int   sceCdRead(unsigned int lsn, unsigned int sectors, void *buf, sceCdRMode *mode);
int   sceCdGetError(void);
int   sceCdGetDiskType(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_LIBCDVD_H */
