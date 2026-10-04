/* loadfile.c - Sony libkernl 2.2.4 (PsIIlibkernl2240) loadfile.o: EE client
 * of the IOP LOADFILE RPC server 0x80000006 (0x01021F20 .. 0x010228B8).
 *
 * The only entry point the bootstrap uses is sceSifLoadModule (main's IRX
 * loader F_01000790: sio2man/mtapman/mcman/mcserv/padman from cdrom0:).
 * ELF loading (sceSifLoadElf*) is linked but unused - core.gt3 is loaded by
 * the game's own ImageLoader, not by LOADFILE.
 *
 * All calls share one 0x200-byte transfer buffer (send AND receive) @0x0104D640;
 * wire layouts match ps2sdk's _lf_*_arg structures.
 */
#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "sif.h"

/* Sony loadfile error codes (returned negated).                             */
#define SCE_EBINDMISS     0x10000   /* LOADFILE server bind failed          */
#define SCE_ECALLMISS     0x10001   /* sceSifCallRpc failed                 */
#define SCE_EBADWIDTH     0x10002   /* ? Sony name unknown: peek/poke width not 0/1/2 */
#define SCE_ELOADMISS     0x10003   /* ELF load returned epc 0              */
#define SCE_EVERSIONMISS  0x10004   /* IOP loadfile version mismatch        */

#define LF_PATH_MAX 0xFC
#define LF_ARG_MAX  0xFC

/* LOADFILE transfer buffer @0x0104D640 (.bss, 0x200 bytes).                 */
typedef union {
    struct {                        /* fno 0 MOD_LOAD (4 = MG variant)      */
        int  arglen;                /* +0x000 in: arglen / out: module id   */
        int  modres;                /* +0x004 out: module start() result    */
        char path[LF_PATH_MAX];     /* +0x008                               */
        char args[LF_ARG_MAX];      /* +0x104                               */
    } mod;
    struct {                        /* fno 6 MOD_BUF_LOAD                   */
        const void *iop_addr;       /* +0x000 in: IOP image / out: module id */
        int  arglen;                /* +0x004 in: arglen / out: start result */
        char unused[LF_PATH_MAX];   /* +0x008                               */
        char args[LF_ARG_MAX];      /* +0x104                               */
    } buf;
    struct {                        /* fno 1 ELF_LOAD (5 = MG variant)      */
        unsigned int epc;           /* +0x000 out                           */
        unsigned int gp;            /* +0x004 out                           */
        char path[LF_PATH_MAX];     /* +0x008                               */
        char secname[LF_ARG_MAX];   /* +0x104                               */
    } elf;
    struct {                        /* fno 3 GET_ADDR / fno 2 SET_ADDR      */
        unsigned int iop_addr;      /* +0x000 in: address / out: value read */
        int  width;                 /* +0x004 0 byte, 1 half, 2 word        */
        union { u8 b; u16 h; u32 w; } val;  /* +0x008 value to write        */
    } val;
    u8 raw[0x200];
} _lf_xfer_t;

_Static_assert(sizeof(_lf_xfer_t) == 0x200, "loadfile transfer buffer");
_Static_assert(offsetof(_lf_xfer_t, mod.args) == 0x104, "mod.args");
_Static_assert(offsetof(_lf_xfer_t, elf.secname) == 0x104, "elf.secname");
_Static_assert(offsetof(_lf_xfer_t, val.val) == 0x008, "val.val");

typedef struct { char c[4]; } _lf_ver_t;   /* copied with lwl/lwr + swl/swr */

/* Body of an empty busy-wait loop.  egcs keeps an empty loop as written (an
 * asm statement in it would change the loop's shape); GCC 14 would delete
 * it, so there it gets an empty volatile asm to keep the delay.            */
#ifdef BOOT_OLD_GCC
#define LF_SPIN_BODY()
#else
#define LF_SPIN_BODY()  __asm__ __volatile__("")
#endif

/* "2240": the version digits at the tail of libkernl's version object
 * "PsIIlibkernl2240" @0x0103D278 (the digits are @0x0103D284).
 * Defined in sdk/libkernl/kernel.c (same extern as fileio.c uses).          */
extern char _libkernl_version[16];                           /* 0x0103D278 */
#define LIBKERNL_VERSION_DIGITS (&_libkernl_version[12])     /* 0x0103D284 */

static int              _lf_bind_state = -1;           /* 0x0103DAD4 .data, -1 = not bound */
static const char      *_lf_version_wildcard = "....";  /* 0x0103DAD8 .data -> 0x01040710 .rodata */
static _lf_xfer_t       _lf_xfer __attribute__((aligned(64)));  /* 0x0104D640 .bss */
static sceSifClientData _lf_client;                    /* 0x0104D840 .bss */
static char             _lf_iop_version[4];            /* 0x0104D868 .bss, reply of fno 0xFF */

// FUNC 0x01021F20 _sceSifLoadFileInit
//   Lazy bind: returns 0 at once if already bound.  Otherwise loops
//   sceSifBindRpc(0x80000006) until the server answers (a ~1M-iteration busy
//   delay between tries), marks the client bound, then asks the server for
//   its 4-char version (fno 0xFF, 4-byte reply into the transfer buffer) and
//   caches it.  Returns -1 if a bind call fails, -SCE_ECALLMISS if the
//   version call fails.
static int _sceSifLoadFileInit(void)
{
    int i;

    if (_lf_bind_state < 0) {
        while (1) {
            if (sceSifBindRpc(&_lf_client, 0x80000006, 0) < 0)
                return -1;
            if (_lf_client.serve != NULL) {
                _lf_bind_state = 0;
                if (sceSifCallRpc(&_lf_client, 0xFF, 0, NULL, 0, &_lf_xfer, 4, NULL, NULL) < 0)
                    return -SCE_ECALLMISS;
                *(_lf_ver_t *)_lf_iop_version = *(_lf_ver_t *)&_lf_xfer;
                return 0;
            }
            /* busy-wait before the next try: 0x100000 passes (the four nops
             * in the binary are the R5900 short-loop erratum padding) */
            i = 0x100000;
            while (i--)
                LF_SPIN_BODY();
        }
    }
    return 0;
}

// FUNC 0x01022020 _sceSifCheckLoadFileVersion
//   Returns 1 (mismatch) unless the IOP loadfile version equals this
//   library's "2240", or the IOP reports the wildcard "....", or the library
//   side is itself the wildcard (never true here).  Callers turn 1 into
//   -SCE_EVERSIONMISS.
static int _sceSifCheckLoadFileVersion(void)
{
    const char *lib = LIBKERNL_VERSION_DIGITS;
    const char *iop = _lf_iop_version;

    return memcmp(iop, lib, 4) != 0 && memcmp(iop, _lf_version_wildcard, 4) != 0
        && memcmp(lib, _lf_version_wildcard, 4) != 0;
}

// FUNC 0x010220B0 sceSifLoadFileReset
//   Forget the binding and the cached IOP version (used after an IOP reboot).
int sceSifLoadFileReset(void)
{
    _lf_bind_state = -1;
    memset(_lf_iop_version, 0, 4);
    return 0;
}

// FUNC 0x010220E8 _sceSifLoadModuleBuffer
//   LOADFILE fno 6 (MOD_BUF_LOAD): link+start an IRX image already in IOP
//   memory.  args are copied only if argp != NULL, truncated to 0xFC bytes
//   (the >= 0xFD case is an inlined 0xFC-byte block copy).  Returns the module
//   id (reply word 0) and stores the module's start() result (reply word 1)
//   into *result.
static int _sceSifLoadModuleBuffer(const void *iop_addr, int args, const char *argp,
                                   int *result)
{
    int id;

    if (_sceSifLoadFileInit() < 0)
        return -SCE_EBINDMISS;
    if (_sceSifCheckLoadFileVersion() != 0)
        return -SCE_EVERSIONMISS;

    _lf_xfer.buf.iop_addr = iop_addr;
    if (argp != NULL) {
        if (args > 0xFC) {
            memcpy(_lf_xfer.buf.args, argp, LF_ARG_MAX);    /* inlined in the binary */
            _lf_xfer.buf.arglen = LF_ARG_MAX;
        } else {
            memcpy(_lf_xfer.buf.args, argp, args);
            _lf_xfer.buf.arglen = args;
        }
    } else {
        _lf_xfer.buf.arglen = 0;
    }
    if (sceSifCallRpc(&_lf_client, 6, 0, &_lf_xfer, 0x200, &_lf_xfer, 8, NULL, NULL) < 0)
        return -SCE_ECALLMISS;
    id = (int)_lf_xfer.buf.iop_addr;                /* reply +0: module id      */
    *result = _lf_xfer.buf.arglen;                  /* reply +4: start() result */
    return id;
}

// FUNC 0x010222F0 sceSifLoadModuleBuffer
//   _sceSifLoadModuleBuffer with a discarded result slot on the stack.
int sceSifLoadModuleBuffer(const void *iop_addr, int args, const char *argp)
{
    int dummy;

    return _sceSifLoadModuleBuffer(iop_addr, args, argp, &dummy);
}

// FUNC 0x01022310 sceSifLoadStartModuleBuffer
//   _sceSifLoadModuleBuffer forwarding the caller's start()-result pointer.
int sceSifLoadStartModuleBuffer(const void *iop_addr, int args, const char *argp,
                                int *result)
{
    return _sceSifLoadModuleBuffer(iop_addr, args, argp, result);
}

// FUNC 0x01022330 _sceSifLoadModule
//   LOADFILE by path: fno (0 = MOD_LOAD) chosen by the wrappers.  Path is
//   strncpy'd (0xFC, forced NUL at [0xFB]); args as in the buffer variant but
//   with the length word at +0 and args[0] cleared when argp == NULL.
//   Returns the module id and stores the start() result into *result.
static int _sceSifLoadModule(const char *filename, int args, const char *argp,
                             int *result, int fno)
{
    int id;

    if (_sceSifLoadFileInit() < 0)
        return -SCE_EBINDMISS;
    if (_sceSifCheckLoadFileVersion() != 0)
        return -SCE_EVERSIONMISS;

    strncpy(_lf_xfer.mod.path, filename, LF_PATH_MAX);
    _lf_xfer.mod.path[LF_PATH_MAX - 1] = 0;
    if (argp != NULL) {
        if (args > 0xFC) {
            memcpy(_lf_xfer.mod.args, argp, LF_ARG_MAX);    /* inlined in the binary */
            _lf_xfer.mod.arglen = LF_ARG_MAX;
        } else {
            memcpy(_lf_xfer.mod.args, argp, args);
            _lf_xfer.mod.arglen = args;
        }
    } else {
        _lf_xfer.mod.args[0] = 0;
        _lf_xfer.mod.arglen = 0;
    }
    if (sceSifCallRpc(&_lf_client, fno, 0, &_lf_xfer, 0x200, &_lf_xfer, 8, NULL, NULL) < 0)
        return -SCE_ECALLMISS;
    id = _lf_xfer.mod.arglen;                       /* reply +0: module id */
    *result = _lf_xfer.mod.modres;
    return id;
}

// FUNC 0x01022558 sceSifLoadModule
//   Load + start an IRX by path (fno 0), start() result discarded.  Called by
//   main's IRX loader (F_01000790) in a `while (sceSifLoadModule(p,0,NULL) < 0)`
//   retry loop.
int sceSifLoadModule(const char *filename, int args, const char *argp)
{
    int dummy;

    return _sceSifLoadModule(filename, args, argp, &dummy, 0);
}

// FUNC 0x01022578 sceSifLoadStartModule
//   As sceSifLoadModule, returning the module's start() result via *result.
//   Unused in this binary.
int sceSifLoadStartModule(const char *filename, int args, const char *argp, int *result)
{
    return _sceSifLoadModule(filename, args, argp, result, 0);
}

// FUNC 0x01022598 _sceSifLoadElfPart
//   LOADFILE ELF load (fno 1 from the wrappers): path and section name are
//   strncpy'd (0xFC each, forced NUL), 0x10-byte reply {epc, gp}.  epc == 0
//   means the IOP could not load it (-SCE_ELOADMISS); else data->epc/gp are
//   filled and 0 is returned.
static int _sceSifLoadElfPart(const char *name, const char *secname,
                              sceExecData *data, int fno)
{
    unsigned int epc;

    if (_sceSifLoadFileInit() < 0)
        return -SCE_EBINDMISS;
    if (_sceSifCheckLoadFileVersion() != 0)
        return -SCE_EVERSIONMISS;

    strncpy(_lf_xfer.elf.path, name, LF_PATH_MAX);
    _lf_xfer.elf.path[LF_PATH_MAX - 1] = 0;
    strncpy(_lf_xfer.elf.secname, secname, LF_ARG_MAX);
    _lf_xfer.elf.secname[LF_ARG_MAX - 1] = 0;
    if (sceSifCallRpc(&_lf_client, fno, 0, &_lf_xfer, 0x200, &_lf_xfer, 0x10, NULL, NULL) < 0)
        return -SCE_ECALLMISS;
    epc = _lf_xfer.elf.epc;
    if (epc == 0)
        return -SCE_ELOADMISS;
    data->epc = epc;
    data->gp  = _lf_xfer.elf.gp;
    return 0;
}

// FUNC 0x010226A0 sceSifLoadElfPart
//   Load one section of an ELF on the IOP side (fno 1).  Unused here.
int sceSifLoadElfPart(const char *name, const char *secname, sceExecData *data)
{
    return _sceSifLoadElfPart(name, secname, data, 1);
}

// FUNC 0x010226C0 sceSifLoadElf
//   Load a whole ELF (section name "all" @0x01040718).  Unused here.
int sceSifLoadElf(const char *name, sceExecData *data)
{
    return _sceSifLoadElfPart(name, "all", data, 1);
}

// FUNC 0x010226E8 sceSifGetIopAddr
//   LOADFILE fno 3 (GET_ADDR, ps2sdk SifIopGetVal): read a byte/half/word
//   (width 0/1/2, unsigned check) of IOP memory into *out; the value comes
//   back in reply word 0.  Note: bind is checked but, unlike the load calls,
//   NOT the loadfile version.  Sony name uncertain.
int sceSifGetIopAddr(unsigned int iop_addr, void *out, int width)
{
    if (_sceSifLoadFileInit() < 0)
        return -SCE_EBINDMISS;
    if (!((unsigned int)width < 3))
        return -SCE_EBADWIDTH;

    _lf_xfer.val.iop_addr = iop_addr;
    _lf_xfer.val.width = width;
    if (sceSifCallRpc(&_lf_client, 3, 0, &_lf_xfer, 0x20, &_lf_xfer, 0x20, NULL, NULL) < 0)
        return -SCE_ECALLMISS;
    if (width == 0)
        *(u8 *)out = *(u8 *)&_lf_xfer;
    else if (width == 1)
        *(u16 *)out = *(u16 *)&_lf_xfer;
    else if (width == 2)
        *(u32 *)out = *(u32 *)&_lf_xfer;
    else
        return -SCE_EBADWIDTH;          /* unreachable after the check above */
    return 0;
}

// FUNC 0x010227D8 sceSifSetIopAddr
//   LOADFILE fno 2 (SET_ADDR, ps2sdk SifIopSetVal): write *value (byte/half/
//   word by width 0/1/2) to IOP memory.  addr/width are stored before the
//   width is validated.  No version check.  Returns 0 or -SCE_ECALLMISS.
//   Sony name uncertain.
int sceSifSetIopAddr(unsigned int iop_addr, const void *value, int width)
{
    int r;

    if (_sceSifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    _lf_xfer.val.iop_addr = iop_addr;
    _lf_xfer.val.width = width;
    if (width == 0)
        _lf_xfer.val.val.b = *(const u8 *)value;
    else if (width == 1)
        _lf_xfer.val.val.h = *(const u16 *)value;
    else if (width == 2)
        _lf_xfer.val.val.w = *(const u32 *)value;
    else
        return -SCE_EBADWIDTH;
    r = sceSifCallRpc(&_lf_client, 2, 0, &_lf_xfer, 0x20, &_lf_xfer, 0x10, NULL, NULL);
    if (r < 0)
        return -SCE_ECALLMISS;
    return 0;
}
