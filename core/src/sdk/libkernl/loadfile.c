/* loadfile.c - Sony libkernl 2240 loadfile.o: EE client of the
 * IOP LOADFILE RPC server 0x80000006 (0x0028CA58..0x0028D3F0).  Same code as
 * the bootstrap's sdk/libkernl/loadfile.c except _sceSifLoadModuleBuffer
 * (0x0028CC20, not byte-matched but the same logic).
 * All calls share one 0x200-byte transfer buffer (send AND receive),
 * _lf_xfer @0x004B0F00.  The game (T524, IOPLoadModuleBase) uses
 * sceSifLoadModule in a `while (sceSifLoadModule(...) < 0)` retry loop
 * (IOPLoadModuleBase::cmdLoadModule) and sceSifLoadFileReset + sceFsReset
 * after an IOP reboot (IOPLoadModuleBase::cmdReset).
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part2_internal.h"

/* loadfile.o's data, all file statics (as the bootstrap's loadfile.c).
 * .data: the bound mark and the version wildcard, whose "...." opens the
 * .rodata (0x00349360, before sceSifLoadElf's "all"; only this pointer
 * refers to it, hence the anchor).  .bss (0x004B0F00..0x004B112C, between
 * iopheap.o's and iopreboot.o's statics): the 64-byte-aligned transfer
 * buffer, the client, the IOP's version reply.                             */
// DATA_START 0x00349360
static int              _lf_bind_state = -1;           /* 0x00308268 .data, -1 = not bound */
static const char      *_lf_version_wildcard = "....";  /* 0x0030826C .data -> 0x00349360 .rodata */
static _lf_xfer_t       _lf_xfer __attribute__((aligned(64)));  /* 0x004B0F00 .bss */
static sceSifClientData _lf_client;                    /* 0x004B1100 .bss */
static char             _lf_iop_version[4];            /* 0x004B1128 .bss, reply of LF_F_GET_VERSION */

// FUNC 0x0028CA58 _sceSifLoadFileInit
//   Lazy bind: returns 0 at once if already bound.  Otherwise loops
//   sceSifBindRpc(0x80000006) until the server answers (a ~1M-iteration busy
//   delay between tries), marks the client bound, then asks the server for
//   its 4-char version (LF_F_GET_VERSION, 4-byte reply into the transfer
//   buffer) and caches it.  Returns -1 if a bind call fails, -SCE_ECALLMISS
//   if the version call fails.
static int _sceSifLoadFileInit(void)
{
    int spin;

    if (_lf_bind_state < 0) {
        while (1) {
            if (sceSifBindRpc(&_lf_client, LF_SERVER_ID, 0) < 0)
                return -1;
            if (_lf_client.serve != NULL) {
                _lf_bind_state = 0;
                if (sceSifCallRpc(&_lf_client, LF_F_GET_VERSION, 0, NULL, 0, &_lf_xfer, 4, NULL, NULL) < 0)
                    return -SCE_ECALLMISS;
                *(_lf_ver_t *)_lf_iop_version = *(_lf_ver_t *)_lf_xfer.version;
                return 0;
            }
            /* busy-wait before the next try (see SPIN_BODY) */
            spin = BIND_RETRY_SPIN;
            while (spin--)
                SPIN_BODY();
        }
    }
    return 0;
}

// FUNC 0x0028CB58 _sceSifCheckLoadFileVersion
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

// FUNC 0x0028CBE8 sceSifLoadFileReset
//   Forget the binding and the cached IOP version (used after an IOP reboot,
//   by IOPLoadModuleBase::cmdReset).
int sceSifLoadFileReset(void)
{
    _lf_bind_state = -1;
    memset(_lf_iop_version, 0, 4);
    return 0;
}

// FUNC 0x0028CC20 _sceSifLoadModuleBuffer
//   LF_F_MOD_BUF_LOAD: link+start an IRX image already in IOP memory.  The
//   image address is stored before argp is tested; args are copied only if
//   argp != NULL, truncated to 0xFC bytes (args >= 0xFD: an inlined 0xFC-byte
//   block copy, 7 x 32 bytes + 28, with an aligned and an unaligned ldl/ldr
//   path).  Returns the module id (reply word 0) and stores the module's
//   start() result (reply word 1) into *result.
//   Not byte-matched to the bootstrap's 0x010220E8 but the same logic
//   (listing checked).
static int _sceSifLoadModuleBuffer(const void *iop_addr, int args, const char *argp,
                                   int *result)
{
    int id;

    if (_sceSifLoadFileInit() < 0)
        return -SCE_EBINDMISS;
    if (_sceSifCheckLoadFileVersion() != 0)
        return -SCE_EVERSIONMISS;

    _lf_xfer.buf.p.ptr = iop_addr;
    if (argp != NULL) {
        if (args > LF_ARG_MAX) {
            memcpy(_lf_xfer.buf.args, argp, LF_ARG_MAX);    /* inlined in the binary */
            _lf_xfer.buf.q.arg_len = LF_ARG_MAX;
        } else {
            memcpy(_lf_xfer.buf.args, argp, args);
            _lf_xfer.buf.q.arg_len = args;
        }
    } else {
        _lf_xfer.buf.q.arg_len = 0;
    }
    if (sceSifCallRpc(&_lf_client, LF_F_MOD_BUF_LOAD, 0, &_lf_xfer, sizeof(_lf_xfer), &_lf_xfer, 8, NULL, NULL) < 0)
        return -SCE_ECALLMISS;
    id = _lf_xfer.buf.p.result;                     /* reply +0: module id      */
    *result = _lf_xfer.buf.q.modres;                /* reply +4: start() result */
    return id;
}

// FUNC 0x0028CE28 sceSifLoadModuleBuffer
//   _sceSifLoadModuleBuffer with a discarded result slot on the stack.
int sceSifLoadModuleBuffer(const void *iop_addr, int args, const char *argp)
{
    int modres;

    return _sceSifLoadModuleBuffer(iop_addr, args, argp, &modres);
}

// FUNC 0x0028CE48 sceSifLoadStartModuleBuffer
//   _sceSifLoadModuleBuffer forwarding the caller's start()-result pointer.
int sceSifLoadStartModuleBuffer(const void *iop_addr, int args, const char *argp,
                                int *result)
{
    return _sceSifLoadModuleBuffer(iop_addr, args, argp, result);
}

// FUNC 0x0028CE68 _sceSifLoadModule
//   LOADFILE by path: fno (LF_F_MOD_LOAD) chosen by the wrappers.  Path is
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
        if (args > LF_ARG_MAX) {
            memcpy(_lf_xfer.mod.args, argp, LF_ARG_MAX);    /* inlined in the binary */
            _lf_xfer.mod.p.arg_len = LF_ARG_MAX;
        } else {
            memcpy(_lf_xfer.mod.args, argp, args);
            _lf_xfer.mod.p.arg_len = args;
        }
    } else {
        _lf_xfer.mod.args[0] = 0;
        _lf_xfer.mod.p.arg_len = 0;
    }
    if (sceSifCallRpc(&_lf_client, fno, 0, &_lf_xfer, sizeof(_lf_xfer), &_lf_xfer, 8, NULL, NULL) < 0)
        return -SCE_ECALLMISS;
    id = _lf_xfer.mod.p.result;                     /* reply +0: module id      */
    *result = _lf_xfer.mod.modres;                  /* reply +4: start() result */
    return id;
}

// FUNC 0x0028D090 sceSifLoadModule
//   Load + start an IRX by path (LF_F_MOD_LOAD), start() result discarded.
//   Called by the game's IOPLoadModuleBase::cmdLoadModule (T524) in a
//   `while (sceSifLoadModule(path, this->argc, this->args) < 0) yield;`
//   retry loop.
int sceSifLoadModule(const char *filename, int args, const char *argp)
{
    int modres;

    return _sceSifLoadModule(filename, args, argp, &modres, LF_F_MOD_LOAD);
}

// FUNC 0x0028D0B0 sceSifLoadStartModule
//   As sceSifLoadModule, returning the module's start() result via *result.
//   Unused in this binary.
int sceSifLoadStartModule(const char *filename, int args, const char *argp, int *result)
{
    return _sceSifLoadModule(filename, args, argp, result, LF_F_MOD_LOAD);
}

// FUNC 0x0028D0D0 _sceSifLoadElfPart
//   LOADFILE ELF load (LF_F_ELF_LOAD from the wrappers): path and section
//   name are strncpy'd (0xFC each, forced NUL), 0x10-byte reply {epc, gp}.
//   epc == 0 means the IOP could not load it (-SCE_ELOADMISS); else
//   data->epc/gp are filled and 0 is returned.
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
    if (sceSifCallRpc(&_lf_client, fno, 0, &_lf_xfer, sizeof(_lf_xfer), &_lf_xfer, 0x10, NULL, NULL) < 0)
        return -SCE_ECALLMISS;
    epc = _lf_xfer.elf.epc;
    if (epc == 0)
        return -SCE_ELOADMISS;
    data->epc = epc;
    data->gp  = _lf_xfer.elf.gp;
    return 0;
}

// FUNC 0x0028D1D8 sceSifLoadElfPart
//   Load one section of an ELF on the IOP side (LF_F_ELF_LOAD).  Unused here.
int sceSifLoadElfPart(const char *name, const char *secname, sceExecData *data)
{
    return _sceSifLoadElfPart(name, secname, data, LF_F_ELF_LOAD);
}

// FUNC 0x0028D1F8 sceSifLoadElf
//   Load a whole ELF (section name "all" @0x00349368).  Unused here.
int sceSifLoadElf(const char *name, sceExecData *data)
{
    return _sceSifLoadElfPart(name, "all", data, LF_F_ELF_LOAD);
}

// FUNC 0x0028D220 sceSifGetIopAddr
//   LF_F_GET_ADDR (ps2sdk SifIopGetVal): read a byte/half/word (LF_VAL_BYTE /
//   SHORT / LONG, unsigned check) of IOP memory into *out; the value comes
//   back in reply word 0.  Note: bind is checked but, unlike the load calls,
//   NOT the loadfile version.  Sony name uncertain.
int sceSifGetIopAddr(unsigned int iop_addr, void *out, int width)
{
    if (_sceSifLoadFileInit() < 0)
        return -SCE_EBINDMISS;
    if (!((unsigned int)width <= LF_VAL_LONG))
        return -SCE_EBADWIDTH;

    _lf_xfer.val.p.iop_addr = iop_addr;
    _lf_xfer.val.type = width;
    if (sceSifCallRpc(&_lf_client, LF_F_GET_ADDR, 0, &_lf_xfer, 0x20, &_lf_xfer, 0x20, NULL, NULL) < 0)
        return -SCE_ECALLMISS;
    if (width == LF_VAL_BYTE)
        *(u8 *)out = _lf_xfer.val.p.result.b;
    else if (width == LF_VAL_SHORT)
        *(u16 *)out = _lf_xfer.val.p.result.s;
    else if (width == LF_VAL_LONG)
        *(u32 *)out = _lf_xfer.val.p.result.l;
    else
        return -SCE_EBADWIDTH;          /* unreachable after the check above */
    return 0;
}

// FUNC 0x0028D310 sceSifSetIopAddr
//   LF_F_SET_ADDR (ps2sdk SifIopSetVal): write *value (byte/half/word by
//   width LF_VAL_*) to IOP memory.  addr/width are stored before the width
//   is validated.  No version check.  Returns 0 or -SCE_ECALLMISS.
//   Sony name uncertain.
int sceSifSetIopAddr(unsigned int iop_addr, const void *value, int width)
{
    int rpc;

    if (_sceSifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    _lf_xfer.val.p.iop_addr = iop_addr;
    _lf_xfer.val.type = width;
    if (width == LF_VAL_BYTE)
        _lf_xfer.val.val.b = *(const u8 *)value;
    else if (width == LF_VAL_SHORT)
        _lf_xfer.val.val.s = *(const u16 *)value;
    else if (width == LF_VAL_LONG)
        _lf_xfer.val.val.l = *(const u32 *)value;
    else
        return -SCE_EBADWIDTH;
    rpc = sceSifCallRpc(&_lf_client, LF_F_SET_ADDR, 0, &_lf_xfer, 0x20, &_lf_xfer, 0x10, NULL, NULL);
    if (rpc < 0)
        return -SCE_ECALLMISS;
    return 0;
}
