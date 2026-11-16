/* T524 - IOPLoadModuleBase.  0x0025DA00..0x0025DE20.
 *
 * Runs an IOP set-up script: a table of {member pointer, argument} commands
 * (IOPLoadCommand), ended by a null member pointer.  The derived
 * RaceIOPLoadModules (T009) runs the table at 0x00308B00 from load().  run
 * itself is also a command (data refs 0x00308B10..0x00308B4C): its argument
 * is a sub-table.  Module paths are built as prefix + name (upper-cased when
 * `upcase') + suffix, and each module is loaded once (`names'). */
#include "engine/pdi/T524_iop_load_module_base.h"
#include "engine/pdi/pdi_internal.h"

#define IOP_PATH_MAX 0x100              /* the commands' stack path buffers */

/* Caller: RaceIOPLoadModules::RaceIOPLoadModules (0x001029E0).  unk_0004 and
 * names are not initialised. */
// FUNC 0x0025DA00 IOPLoadModuleBase::IOPLoadModuleBase
IOPLoadModuleBase::IOPLoadModuleBase()
{
    mediaMode = 0;
    prefix = NULL;
    upcase = 0;
    suffix = NULL;
    argc   = 0;
    args   = NULL;
    count  = 0;
}

/* Caller: RaceIOPLoadModules::~RaceIOPLoadModules (0x00102A18). */
// FUNC 0x0025DA30 IOPLoadModuleBase::~IOPLoadModuleBase
IOPLoadModuleBase::~IOPLoadModuleBase()
{
}

/* Callers: cmdReboot, cmdLoadModule (out = an IOP_PATH_MAX stack buffer). */
// FUNC 0x0025DA60 IOPLoadModuleBase::makePath
char *IOPLoadModuleBase::makePath(char *out, const char *name)
{
    char *namePart;

    out[0] = 0;
    if (prefix != NULL)
        strcat(out, prefix);
    namePart = out + strlen(out);       /* 0x00269A68, the MMI strlen */
    strcat(out, name);
    if (upcase)
        strupr(namePart);
    if (suffix != NULL)
        strcat(out, suffix);
    return out;
}

/* Returns 1 when path was loaded before, else appends it to `names' and
 * returns 0.  Caller: cmdLoadModule. */
// FUNC 0x0025DAF8 IOPLoadModuleBase::checkLoaded
int IOPLoadModuleBase::checkLoaded(const char *path)
{
    char *loaded = names;
    int i;

    for (i = 0; i < count; i++) {
        if (strcmp(loaded, path) == 0)
            return 1;
        loaded += strlen(loaded) + 1;
    }
    strcpy(loaded, path);
    count++;
    return 0;
}

/* Callers: RaceIOPLoadModules::load (0x00102A40), and the script itself.  A
 * virtual member pointer is resolved through the vptr (+0x4020) in egcs. */
// FUNC 0x0025DB98 IOPLoadModuleBase::run
void IOPLoadModuleBase::run(int script)
{
    const IOPLoadCommand *cmd = (const IOPLoadCommand *)script;

    /* the command copied out first (the listing's ldl / ldr to the stack,
     * the argument loaded before the end test) */
    for (;;) {
        IOPLoadCmd f = cmd->func;
        int arg = cmd->arg;
        if (f == 0)
            break;
        (this->*f)(arg);
        cmd++;
    }
}

/* Caller: RaceIOPLoadModules::cmdSetMediaMode (0x00102A60, with
 * F_00102E58()). */
// FUNC 0x0025DC70 IOPLoadModuleBase::setMediaMode
void IOPLoadModuleBase::setMediaMode(int mode)
{
    mediaMode = mode;
}

/* Script command (0x00308B8C, 0x00308BA4). */
// FUNC 0x0025DC78 IOPLoadModuleBase::cmdInit
void IOPLoadModuleBase::cmdInit(int arg)
{
    sceSifInitRpc(0);
    sceCdInit(SCECdINIT);
    sceCdMmode(mediaMode);
    count = 0;
}

/* Script command (0x00308BB0), after an IOP reboot. */
// FUNC 0x0025DCB8 IOPLoadModuleBase::cmdReset
void IOPLoadModuleBase::cmdReset(int arg)
{
    sceSifLoadFileReset();
    sceFsReset();
}

/* Script command (0x00308BBC). */
// FUNC 0x0025DCE0 IOPLoadModuleBase::cmdInitIopHeap
void IOPLoadModuleBase::cmdInitIopHeap(int arg)
{
    sceSifInitIopHeap();
}

/* Script command (0x00308DCC, 0x00308E0C). */
// FUNC 0x0025DD00 IOPLoadModuleBase::cmdSetPrefix
void IOPLoadModuleBase::cmdSetPrefix(int arg)
{
    prefix = (const char *)arg;
}

/* Script command (0x00308DD8, 0x00308E18). */
// FUNC 0x0025DD08 IOPLoadModuleBase::cmdSetUpcase
void IOPLoadModuleBase::cmdSetUpcase(int arg)
{
    upcase = arg;
}

/* Script command (0x00308DE4, 0x00308E24). */
// FUNC 0x0025DD10 IOPLoadModuleBase::cmdSetSuffix
void IOPLoadModuleBase::cmdSetSuffix(int arg)
{
    suffix = (const char *)arg;
}

/* Not in the retail script. */
// FUNC 0x0025DD18 IOPLoadModuleBase::cmdSetArgc
void IOPLoadModuleBase::cmdSetArgc(int arg)
{
    argc = arg;
}

/* Not in the retail script. */
// FUNC 0x0025DD20 IOPLoadModuleBase::cmdSetArgs
void IOPLoadModuleBase::cmdSetArgs(int arg)
{
    args = (const char *)arg;
}

/* Script command (0x00308DF0, 0x00308E30). */
// FUNC 0x0025DD28 IOPLoadModuleBase::cmdClearArgs
void IOPLoadModuleBase::cmdClearArgs(int arg)
{
    argc = 0;
    args = NULL;
}

/* Script command (0x00308B98): reboots the IOP with the image `arg'. */
// FUNC 0x0025DD38 IOPLoadModuleBase::cmdReboot
void IOPLoadModuleBase::cmdReboot(int arg)
{
    char path[IOP_PATH_MAX];

    makePath(path, (const char *)arg);
    while (!sceSifRebootIop(path))
        ;
    while (!sceSifSyncIop())
        ;
}

/* Script command (0x00308C54..0x00308C90): loads the module `arg' unless it
 * was loaded before, retrying until sceSifLoadModule succeeds. */
// FUNC 0x0025DD78 IOPLoadModuleBase::cmdLoadModule
void IOPLoadModuleBase::cmdLoadModule(int arg)
{
    char path[IOP_PATH_MAX];

    makePath(path, (const char *)arg);
    if (checkLoaded(path) != 0)
        return;
    while (sceSifLoadModule(path, argc, args) < 0)
        yield_low_priority();
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function.  Caller: __tf18RaceIOPLoadModules. */
// FUNC 0x0025DDE0 __tf17IOPLoadModuleBase
extern "C" void *__tf17IOPLoadModuleBase(void)
{
    if (__ti17IOPLoadModuleBase[0] == 0)
        __rtti_user(__ti17IOPLoadModuleBase, "17IOPLoadModuleBase");
    return __ti17IOPLoadModuleBase;
}
#endif
