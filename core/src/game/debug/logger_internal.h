/* logger_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u018,
 * 0x00134630..0x00137840, second pass.
 *
 * Translation units (address order):
 *   T103  gt2k_menu::ShortmenuLicenseControl / ShortmenuLicenseRender: the
 *         licence-test variant of the short menu (u015's ShortmenuControl,
 *         T111 / T112), run by run_short_menu_license (T102) over the
 *         RaceLicense of run_license_test (T053).  The race behind the menu
 *         is restarted in replay mode whenever it signals its end, and drawn
 *         in a sub-window of the screen.
 *   T055  the data logger ("menu/logger.mbl"): gt2k_menu::LoggerControl,
 *         LoggerRender, LoggerPage, LoggerInput, a recording thread that runs
 *         the race (a replay) at full speed and samples both cars every
 *         `interval' metres of the lap distance (speed, throttle, brake,
 *         gear, x / z position) into LoggerData, the queries the logger's
 *         windows (T086 / T087) make on those samples, the replay controls
 *         (sector seek, play, speed, camera change) and the entry points:
 *         logger_run (over a race, from the race modes) and logger_main (from
 *         main: replays loaded from the memory card).
 *   T104  the non-virtual methods of gt2k_menu::MiniPrizeControlBase (T056,
 *         "menu/arcade_mini.mbl", page "arcade_mini"), MiniPrizeInfo's
 *         methods (the 0xC-byte record at its +0xE84) and the static
 *         initialiser of its item factory table ("rank_disp",
 *         "render_model").
 *
 * Every class is defined in its canonical header (include/game/T055,
 * T103, T104 and the headers of the classes they use); this header adds
 * what only this unit uses: a few constants and the typeinfo storage of the
 * unit's own classes.
 *
 * Each source includes the canonical headers it needs itself, in the order
 * that gives retail's type names (each file's comment lists them; u018's
 * data pass, ref/data/u018/notes.md), and this header last.
 */
#ifndef U018_H
#define U018_H

#include "types.h"
#include "libc.h"                           /* strcpy, strcat, strcmp, memalign, free */
#include <stddef.h>

#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* RaceBase::takeSignal's value when the race thread signalled the race's
 * end (u044's name). */
#define RACE_SIGNAL_END         1

/* The size of the recording thread's stack and of LoggerControl::run's work
 * area. */
#define LOGGER_THREAD_STACK     0x4000
#define LOGGER_WORK_SIZE        0x32000

/* The BGM group of the arcade trophy page (menu_bgm_play_if_changed). */
#define BGM_GROUP_ARCADE_MINI   0x17

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
/* The __si_type_info storage {name, vptr, base} the __tf functions fill
 * (GCC 14 only: egcs generates the typeinfo functions and their storage
 * itself). */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __tiQ29gt2k_menu23ShortmenuLicenseControl[3]; /* 0x004B1740 */
extern u32 __tiQ29gt2k_menu22ShortmenuLicenseRender[3];  /* 0x004B1770 */
extern u32 __tiQ29gt2k_menu10LoggerPage[3];              /* 0x004B1780 */
extern u32 __tiQ29gt2k_menu12LoggerRender[3];            /* 0x004B1790 */
extern u32 __tiQ29gt2k_menu13LoggerControl[3];           /* 0x004B17A0 */
extern u32 __tiQ29gt2k_menu11LoggerInput[3];             /* 0x004B17B0 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U018_H */
