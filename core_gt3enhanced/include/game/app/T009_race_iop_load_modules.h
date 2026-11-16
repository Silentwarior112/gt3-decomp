/* T009_race_iop_load_modules.h - RaceIOPLoadModules, the game's IOP module
 * set-up (translation unit T009, unit u001), 0x001029E0..0x00102B58.
 *
 * load() runs the file-local script g_00308B00 (IOPLoadCommand entries,
 * T524) through IOPLoadModuleBase::run: cmdSetMediaMode first, then
 * sub-scripts that reboot the IOP with ioprp224.img and load sio2man,
 * mtapman, mcman, mcserv, padman, libsd, usbd, ilink, memclear and (between
 * cmdInitClients and cmdBindClients) libpdi, pdicdvd, pdispu2, pdistr,
 * pdi1394 and pdiusb, from the cdrom0: prefix with the ";1" suffix.  game_boot (T007)
 * keeps one on its stack for game_init. */
#ifndef GAME_T009_RACE_IOP_LOAD_MODULES_H
#define GAME_T009_RACE_IOP_LOAD_MODULES_H

#include "game/game.h"
#include "engine/pdi/T524_iop_load_module_base.h" /* base IOPLoadModuleBase */

/* vtable 0x00308E48 (2 slots).  typeinfo __tf18RaceIOPLoadModules
 * 0x00102B08 (si, base IOPLoadModuleBase; __si_type_info 0x004B1430).  vptr
 * +0x4020 (IOPLoadModuleBase's).  No data of its own: size 0x4024. */
class RaceIOPLoadModules : public IOPLoadModuleBase {
public:
    RaceIOPLoadModules();                   /* 0x001029E0 */
    virtual ~RaceIOPLoadModules();          /* [0] +0x0C 0x00102A18 */
    virtual void load();                    /* [1] +0x14 0x00102A40 run(g_00308B00) */

    /* Script commands (the argument word is not used). */
    void cmdSetMediaMode(int arg);          /* 0x00102A60 setMediaMode(F_00102E58()) (T008: 2) */
    void cmdInitClients(int arg);           /* 0x00102A90 before the pdi modules: usbd_init(1),
                                               pdi1394_init(1) */
    void cmdBindClients(int arg);           /* 0x00102AB8 after them: usbjoy_start, pdi1394_bind,
                                               pdicdvd_init, pdispu2_init, pdibgm_init, pdimpeg_init,
                                               pdipipe_init */
};
EGCS_SIZE(RaceIOPLoadModules, 0x4024);

/* The script load() runs (T009 .rodata). */
extern const IOPLoadCommand g_00308B00[];   /* 0x00308B00 */
/* The disc's product code and the region prefix of the memory card's
 * directory names (T108's mcGetNamePrefix: "BA" "SCUS-97102"). */
extern const char g_00308AF0[];             /* 0x00308AF0 "SCUS-97102" */
extern char g_00350B38[];                   /* 0x00350B38 "BA" */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tf18RaceIOPLoadModules(void);       /* 0x00102B08 */
extern u32 __ti18RaceIOPLoadModules[3];     /* 0x004B1430 (__si_type_info) */
}
#endif

#endif /* GAME_T009_RACE_IOP_LOAD_MODULES_H */
