/* libmc.h - Sony EE libmc (PsIIlibmc 2240) as linked into the GT3 US
 * bootstrap (BOOTSTRAP_SCUS-97102).  Sony SDK 2.2-era names.
 *
 * Owner: libmc unit (sdk/libmc/libmc.c, 0x010157B8..0x01016F70).
 * mcserv RPC server 0x80000400 (client @0x01048940).  Every call except
 * sceMcSync / sceMcInit is asynchronous: it returns 0 when the request was
 * issued and the result is collected with sceMcSync.  Issue errors seen in
 * sceMcOpen: -200 (0xFF38) PollSema on the library semaphore @0x0103BE8C
 * failed = another request in flight, -100 client not bound (no sceMcInit),
 * -210 NULL/empty name.  The function number of the pending request is kept
 * at 0x0103BE88 (0 = idle).
 *
 * Game callers: the io unit's blocking wrappers mc_*_sync (0x0100F130..
 * 0x0100F538) use Init, GetInfo, GetDir, Open, Read, Close, Sync.
 */
#ifndef BOOT_LIBMC_H
#define BOOT_LIBMC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- function numbers (sceMcSync *cmd, and the RPC fno) ------------------ */
#define sceMcFuncNoCardInfo   1
#define sceMcFuncNoOpen       2
#define sceMcFuncNoClose      3
#define sceMcFuncNoSeek       4
#define sceMcFuncNoRead       5
#define sceMcFuncNoWrite      6
#define sceMcFuncNoFlush      0x0A
#define sceMcFuncNoMkdir      0x0B   /* sceMcMkdir = sceMcOpen(port, slot, name, 0x40), then code := 0xB */
#define sceMcFuncNoChDir      0x0C
#define sceMcFuncNoGetDir     0x0D
#define sceMcFuncNoFileInfo   0x0E
#define sceMcFuncNoDelete     0x0F
#define sceMcFuncNoFormat     0x10
#define sceMcFuncNoUnformat   0x11
#define sceMcFuncNoEntSpace   0x12
#define sceMcFuncNoRename     0x13   /* sent to mcserv as fno 0x0E (FileInfo) in this build */
#define sceMcFuncChgPrior     0x14
#define sceMcFuncSlotMax      0x15
/* 0xFE: sceMcInit's version/handshake RPC (not a sceMcSync code) */

/* ---- sceMcSync -------------------------------------------------------------- */
#define sceMcExecIdle     (-1)   /* nothing pending (code word 0) */
#define sceMcExecRun      0      /* still running (poll mode) */
#define sceMcExecFinish   1      /* finished; *result = word @0x01049F00, code word cleared */
/* mode: 0 = wait (alarm delay 0x3C between sceSifCheckStatRpc polls),
 * non-zero = poll once.  *cmd (if non-NULL) receives the pending code.     */

/* ---- results ---------------------------------------------------------------- */
#define sceMcResSucceed         0
#define sceMcResChangedCard     (-1)
#define sceMcResNoFormat        (-2)
#define sceMcResFullDevice      (-3)
#define sceMcResNoEntry         (-4)
#define sceMcResDeniedPermit    (-5)
#define sceMcResNotEmpty        (-6)
#define sceMcResUpLimitHandle   (-7)
#define sceMcResFailReplace     (-8)

/* ---- sceMcGetInfo *type ------------------------------------------------------ */
#define sceMcTypeNoCard   0
#define sceMcTypePS1      1
#define sceMcTypePS2      2
#define sceMcTypePDA      3

/* ---- file attributes (sceMcTblGetDir.AttrFile) ------------------------------- */
#define sceMcFileAttrReadable     0x0001
#define sceMcFileAttrWriteable    0x0002
#define sceMcFileAttrExecutable   0x0004
#define sceMcFileAttrDupProhibit  0x0008
#define sceMcFileAttrFile         0x0010
#define sceMcFileAttrSubdir       0x0020
#define sceMcFileCreateDir        0x0040   /* sceMcOpen mode used by sceMcMkdir */
#define sceMcFileAttrClosed       0x0080
#define sceMcFileCreateFile       0x0200
#define sceMcFileAttrPDAExec      0x0800
#define sceMcFileAttrPS1          0x1000
#define sceMcFileAttrHidden       0x2000
#define sceMcFileAttrExists       0x8000

/* ---- sceMcSetFileInfo valid bits ---------------------------------------------- */
#define sceMcFileInfoCreate   0x01
#define sceMcFileInfoModify   0x02
#define sceMcFileInfoAttr     0x04

/* sceMcOpen mode uses the SCE_RDONLY / SCE_WRONLY / SCE_RDWR / SCE_CREAT flags
 * of sifdev.h.                                                               */

typedef struct {
    unsigned char  Resv2;   /* +0 */
    unsigned char  Sec;     /* +1 */
    unsigned char  Min;     /* +2 */
    unsigned char  Hour;    /* +3 */
    unsigned char  Day;     /* +4 */
    unsigned char  Month;   /* +5 */
    unsigned short Year;    /* +6 */
} sceMcStDateTime;          /* 8 */

/* sceMcGetDir table entry: 0x40 bytes (GetDir sends maxent << 6).          */
typedef struct {
    sceMcStDateTime _Create;        /* +0x00 */
    sceMcStDateTime _Modify;        /* +0x08 */
    unsigned int    FileSizeByte;   /* +0x10 */
    unsigned short  AttrFile;       /* +0x14 */
    unsigned short  Reserve1;       /* +0x16 */
    unsigned int    Reserve2;       /* +0x18 */
    unsigned int    PdaAplNo;       /* +0x1C */
    unsigned char   EntryName[32];  /* +0x20 */
} sceMcTblGetDir __attribute__((aligned(64)));  /* 0x40 */

#ifdef __cplusplus
static_assert(sizeof(sceMcTblGetDir) == 0x40, "sceMcTblGetDir");
#else
_Static_assert(sizeof(sceMcTblGetDir) == 0x40, "sceMcTblGetDir");
#endif

int   sceMcInit(void);                                                     /* 0x010157B8 */
int   sceMcChangeThreadPriority(int level);                                /* 0x010159A0 fno 0x14 */
int   sceMcGetSlotMax(int port);                                           /* 0x01015A58 fno 0x15 */
int   sceMcOpen(int port, int slot, const char *name, int mode);           /* 0x01015B18 */
int   sceMcMkdir(int port, int slot, const char *name);                    /* 0x01015C50 */
int   sceMcClose(int fd);                                                  /* 0x01015C88 */
int   sceMcSeek(int fd, int offset, int origin);                           /* 0x01015D40 */
int   sceMcRead(int fd, void *buf, int size);                              /* 0x01015EA8 */
int   sceMcWrite(int fd, const void *buf, int size);                       /* 0x01015FC8 */
int   sceMcSync(int mode, int *cmd, int *result);                          /* 0x010161B8 */
int   sceMcGetInfo(int port, int slot, int *type, int *free, int *format); /* 0x010162F0 */
int   sceMcGetDir(int port, int slot, const char *name, unsigned int mode,
                  int maxent, sceMcTblGetDir *table);                      /* 0x01016478 */
int   sceMcChdir(int port, int slot, const char *path, char *pwd);         /* 0x01016658 */
int   sceMcFormat(int port, int slot);                                     /* 0x010167A8 */
int   sceMcDelete(int port, int slot, const char *name);                   /* 0x01016870 */
int   sceMcFlush(int fd);                                                  /* 0x01016998 */
int   sceMcSetFileInfo(int port, int slot, const char *name,
                       const char *info, unsigned int valid);              /* 0x01016A50 */
int   sceMcRename(int port, int slot, const char *org, const char *newname); /* 0x01016C28 */
int   sceMcUnformat(int port, int slot);                                   /* 0x01016D90 */
int   sceMcGetEntSpace(int port, int slot, const char *path);              /* 0x01016E58 */

/* libmc internals (libmc unit only):
 *   0x01015970 mc_get_rpc_state (unreferenced)   0x01015E18 sceMcRead end-callback
 *   0x01016148 alarm handler (iWakeupThread)     0x01016170 alarm-based delay
 *   0x01016298 sceMcGetInfo end-callback         0x010165D0 mc_chdir_end_callback(void *end_para)
 * exec_ps2_with_sbus_cleanup (0x01016F70) is declared in eekernel.h.        */

#ifdef __cplusplus
}
#endif

#endif /* BOOT_LIBMC_H */
