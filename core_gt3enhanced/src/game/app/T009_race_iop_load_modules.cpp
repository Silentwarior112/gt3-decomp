/* T009 - RaceIOPLoadModules.  0x001029E0..0x00102B58.
 *
 * load() runs the script g_00308B00 (IOPLoadCommand entries, T524) through
 * IOPLoadModuleBase::run: cmdSetMediaMode first, then sub-scripts that
 * reboot the IOP with ioprp224.img, load sio2man / mtapman / mcman / mcserv
 * / padman / libsd / usbd / ilink, memclear, and (between cmdInitClients and
 * cmdBindClients) libpdi / pdicdvd / pdispu2 / pdistr / pdi1394 / pdiusb,
 * from "cdrom0:\" with the ";1" suffix. */
#include "cxxrt.h"                          /* type_info, __rtti_si (GCC 14's typeinfo function) */
#include "engine/media/G002_mpeg_frameq.h"          /* usbd_init, usbjoy_start */
#include "engine/pdi/G003_pdi1394_rpc.h"          /* pdi1394_init, pdi1394_bind */
#include "game/app/T008_build_switches.h"       /* F_00102E58 (the media mode) */
#include "game/app/T009_race_iop_load_modules.h"
#include "engine/pdi/T519_pdicdvd.h"              /* pdicdvd_init */
#include "engine/pdi/T520_pdispu2.h"              /* pdispu2_init */
#include "engine/pdi/T521_pdibgm.h"               /* pdibgm_init */
#include "engine/pdi/T522_pdimpeg.h"              /* pdimpeg_init */
#include "engine/pdi/T523_pdipipe.h"              /* pdipipe_init */

/* The disc's product code and the region prefix of the memory card's
 * directory names ("BA" + "SCUS-97102", T108's mcGetNamePrefix, their only
 * reader).  Retail has them right before this unit's script: "SCUS-97102"
 * at 0x00308AF0, after T005's type names (the end of T005's .rodata: egcs
 * writes a file's globals where it defines them, before its vtables and
 * type names, so not T005's), and "BA" in the .sdata at 0x00350B38, before
 * the script's ";1"; T006 has no data of its own.  (T006 compiled -G8
 * defining both would give the same bytes.) */
// DATA_START 0x00308AF0
// DATA_START 0x00350B38
const char g_00308AF0[] = "SCUS-97102";    /* 0x00308AF0 */
char g_00350B38[] = "BA";                   /* 0x00350B38 gp-0x6EB8 */

/* The IOP set-up script (.rodata 0x00308B00..0x00308E48, T524's
 * IOPLoadCommand entries; a null member pointer ends each one): the script
 * load() runs, then its sub-scripts, each after the strings it is the first
 * to use (";1", 3 bytes, goes to the .sdata: -G8).  Each table has external
 * linkage: egcs writes a namespace-scope const object of internal linkage
 * at the end of the unit, after the vtables (the sub-scripts would follow
 * the type name), one of external linkage where it is defined. */
extern const IOPLoadCommand g_00308B88[];   /* 0x00308B88 reboot the IOP */
extern const IOPLoadCommand g_00308C50[];   /* 0x00308C50 the system modules */
extern const IOPLoadCommand g_00308CD0[];   /* 0x00308CD0 memclear */
extern const IOPLoadCommand g_00308D48[];   /* 0x00308D48 the pdi modules */
extern const IOPLoadCommand g_00308DC8[];   /* 0x00308DC8 the disc's paths */
extern const IOPLoadCommand g_00308E08[];   /* 0x00308E08 the same again */

const IOPLoadCommand g_00308B00[] = {
    { (IOPLoadCmd)&RaceIOPLoadModules::cmdSetMediaMode, 0 },
    { &IOPLoadModuleBase::run, (int)g_00308DC8 },
    { &IOPLoadModuleBase::run, (int)g_00308B88 },
    { &IOPLoadModuleBase::run, (int)g_00308E08 },
    { &IOPLoadModuleBase::run, (int)g_00308CD0 },
    { &IOPLoadModuleBase::run, (int)g_00308DC8 },
    { &IOPLoadModuleBase::run, (int)g_00308C50 },
    { &IOPLoadModuleBase::run, (int)g_00308E08 },
    { &IOPLoadModuleBase::run, (int)g_00308D48 },
    { 0, 0 }
};

const IOPLoadCommand g_00308B88[] = {
    { &IOPLoadModuleBase::cmdInit,        0 },
    { &IOPLoadModuleBase::cmdReboot,      (int)"ioprp224.img" },
    { &IOPLoadModuleBase::cmdInit,        0 },
    { &IOPLoadModuleBase::cmdReset,       0 },
    { &IOPLoadModuleBase::cmdInitIopHeap, 0 },
    { 0, 0 }
};
const IOPLoadCommand g_00308C50[] = {
    { &IOPLoadModuleBase::cmdLoadModule, (int)"sio2man.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"mtapman.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"mcman.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"mcserv.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"padman.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"libsd.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"usbd.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"ilink.irx" },
    { 0, 0 }
};
const IOPLoadCommand g_00308CD0[] = {
    { &IOPLoadModuleBase::cmdLoadModule, (int)"memclear.irx" },
    { 0, 0 }
};
const IOPLoadCommand g_00308D48[] = {
    { (IOPLoadCmd)&RaceIOPLoadModules::cmdInitClients, 0 },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"libpdi.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"pdicdvd.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"pdispu2.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"pdistr.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"pdi1394.irx" },
    { &IOPLoadModuleBase::cmdLoadModule, (int)"pdiusb.irx" },
    { (IOPLoadCmd)&RaceIOPLoadModules::cmdBindClients, 0 },
    { 0, 0 }
};
/* The disc's paths: prefix "cdrom0:\", names upper-cased, suffix ";1", no
 * arguments.  Two identical scripts (before the reboot image and before the
 * modules). */
const IOPLoadCommand g_00308DC8[] = {
    { &IOPLoadModuleBase::cmdSetPrefix, (int)"cdrom0:\\" },
    { &IOPLoadModuleBase::cmdSetUpcase, 1 },
    { &IOPLoadModuleBase::cmdSetSuffix, (int)";1" },
    { &IOPLoadModuleBase::cmdClearArgs, 0 },
    { 0, 0 }
};
const IOPLoadCommand g_00308E08[] = {
    { &IOPLoadModuleBase::cmdSetPrefix, (int)"cdrom0:\\" },
    { &IOPLoadModuleBase::cmdSetUpcase, 1 },
    { &IOPLoadModuleBase::cmdSetSuffix, (int)";1" },
    { &IOPLoadModuleBase::cmdClearArgs, 0 },
    { 0, 0 }
};

// FUNC 0x001029E0 RaceIOPLoadModules::RaceIOPLoadModules
RaceIOPLoadModules::RaceIOPLoadModules()
{
    /* IOPLoadModuleBase::IOPLoadModuleBase, then the vptr (+0x4020). */
    /* implicit: IOPLoadModuleBase (base constructor) */
}

// FUNC 0x00102A18 RaceIOPLoadModules::~RaceIOPLoadModules
RaceIOPLoadModules::~RaceIOPLoadModules()
{
    /* The vptr (+0x4020) = 0x00308E48, then
     * IOPLoadModuleBase::~IOPLoadModuleBase with the flags. */
    /* implicit: ~IOPLoadModuleBase (base destructor) */
}

// FUNC 0x00102A40 RaceIOPLoadModules::load
void RaceIOPLoadModules::load()
{
    run((int)g_00308B00);
}

/* Script command (entry 0x00308B00): the media mode of the build switch
 * F_00102E58 (2). */
// FUNC 0x00102A60 RaceIOPLoadModules::cmdSetMediaMode
void RaceIOPLoadModules::cmdSetMediaMode(int arg)
{
    setMediaMode(F_00102E58());
}

/* Script command (entry 0x00308D48), before the pdi modules are loaded. */
// FUNC 0x00102A90 RaceIOPLoadModules::cmdInitClients
void RaceIOPLoadModules::cmdInitClients(int arg)
{
    usbd_init(1);
    pdi1394_init(1);
}

/* Script command (entry 0x00308D9C), after the pdi modules are loaded. */
// FUNC 0x00102AB8 RaceIOPLoadModules::cmdBindClients
void RaceIOPLoadModules::cmdBindClients(int arg)
{
    usbjoy_start();
    pdi1394_bind();
    pdicdvd_init();
    pdispu2_init();
    pdibgm_init();
    pdimpeg_init();
    pdipipe_init();
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * IOPLoadModuleBase). */
// FUNC 0x00102B08 __tf18RaceIOPLoadModules
extern "C" void *__tf18RaceIOPLoadModules(void)
{
    if (__ti18RaceIOPLoadModules[0] == 0) {
        __tf17IOPLoadModuleBase();
        __rtti_si(__ti18RaceIOPLoadModules, "18RaceIOPLoadModules",
                  (const type_info *)__ti17IOPLoadModuleBase);
    }
    return __ti18RaceIOPLoadModules;
}
#endif
