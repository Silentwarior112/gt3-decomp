/* T524_iop_load_module_base.h - IOPLoadModuleBase (translation unit T524,
 * unit u119). */
#ifndef GAME_T524_IOP_LOAD_MODULE_BASE_H
#define GAME_T524_IOP_LOAD_MODULE_BASE_H

#include "game/game.h"

class IOPLoadModuleBase;

/* One command of an IOP set-up script: a member pointer and its argument
 * word (egcs member pointer: {s16 delta, s16 index, pfn / vptr offset}).
 * A null member pointer ends the script.  The commands take the word as an
 * int; the ones that need a string or a sub-script cast it back. */
typedef void (IOPLoadModuleBase::*IOPLoadCmd)(int arg);
struct IOPLoadCommand {
    IOPLoadCmd func;            /* 0x0 */
    int        arg;             /* 0x8 */
};
EGCS_SIZE(IOPLoadCommand, 0xC);

/* Runs an IOP set-up script (IOPLoadCommand table; run is itself a command,
 * its argument a sub-table).  Module paths are prefix + name (upper-cased
 * when `upcase') + suffix; each module is loaded once (`names').  The derived
 * RaceIOPLoadModules (T009) runs the table 0x00308B00 from load().
 *
 * vtable 0x003459B0 (2 slots), typeinfo __tf17IOPLoadModuleBase 0x0025DDE0.
 * vptr +0x4020.  Size 0x4024 (RaceIOPLoadModules adds nothing). */
class IOPLoadModuleBase {
public:
    IOPLoadModuleBase();                                    /* 0x0025DA00 */
    virtual ~IOPLoadModuleBase();                           /* [0] +0x0C 0x0025DA30 */
    virtual void load() = 0;                                /* [1] +0x14 __pure_virtual */

    char *makePath(char *out, const char *name);            /* 0x0025DA60 */
    int   checkLoaded(const char *path);                    /* 0x0025DAF8 1 = loaded before, else appended to names */
    void  run(int script);                                  /* 0x0025DB98 script = const IOPLoadCommand * */
    void  setMediaMode(int mode);                           /* 0x0025DC70 */
    /* script commands */
    void  cmdInit(int arg);                                 /* 0x0025DC78 sceSifInitRpc, sceCdInit, sceCdMmode */
    void  cmdReset(int arg);                                /* 0x0025DCB8 sceSifLoadFileReset, sceFsReset */
    void  cmdInitIopHeap(int arg);                          /* 0x0025DCE0 */
    void  cmdSetPrefix(int arg);                            /* 0x0025DD00 */
    void  cmdSetUpcase(int arg);                            /* 0x0025DD08 */
    void  cmdSetSuffix(int arg);                            /* 0x0025DD10 */
    void  cmdSetArgc(int arg);                              /* 0x0025DD18 */
    void  cmdSetArgs(int arg);                              /* 0x0025DD20 */
    void  cmdClearArgs(int arg);                            /* 0x0025DD28 */
    void  cmdReboot(int arg);                               /* 0x0025DD38 reboots the IOP with the image arg */
    void  cmdLoadModule(int arg);                           /* 0x0025DD78 loads the module arg once */

    int         mediaMode;      /* 0x0000 sceCdMmode argument */
    int         unk_0004;       /* 0x0004 (not initialised) */
    const char *prefix;         /* 0x0008 e.g. "cdrom0:\\" */
    int         upcase;         /* 0x000C nonzero: module name upper-cased */
    const char *suffix;         /* 0x0010 e.g. ";1" */
    int         argc;           /* 0x0014 sceSifLoadModule args length */
    const char *args;           /* 0x0018 sceSifLoadModule args */
    int         count;          /* 0x001C number of names in `names' */
    char        names[0x4000];  /* 0x0020 paths of the loaded modules, NUL-separated */
    /*  vptr                       0x4020 */
};
EGCS_SIZE(IOPLoadModuleBase, 0x4024);

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tf17IOPLoadModuleBase(void);            /* 0x0025DDE0 */
extern u32 __ti17IOPLoadModuleBase[2];          /* 0x003534E0 gp-0x4510 (__user_type_info) */
}
#endif

#endif /* GAME_T524_IOP_LOAD_MODULE_BASE_H */
