/* T143 - RaceFreeRunDisplay (u048), 0x00189998..0x00189AE0.
 *
 * The race display of the free run: RaceDisplay with the two session
 * messages ("timing starts at the next control line", "START ends the
 * session") at the start and no finish display.
 *
 * Includes (u048's data pass): the classes retail's T143 saw, in its order.
 * Its .rodata (0x0032BDC8..0x0032BF69) is "bad_alloc" (<new>), the vtable
 * (referenced only by T142's expanded destructor), then the type names:
 * RaceFreeRunDisplay and T154_race_display.h's (RaceMiniMap ..
 * RaceMeterBase, RaceCameraBase, UnitArena<64>, RaceValueDisplayBase,
 * RaceDisplayBase).  Its .sdata (0x00351EC8) is the "" of T157's inline
 * RaceMessageDisplay::clear: T143 was compiled -G8 (ref/compile_flags.json;
 * under -G0 the "" lands in .rodata before the vtable, where retail has
 * none).
 */
// DATA_START 0x0032BDC8
// DATA_START 0x00351EC8
#include "types.h"
#include <new>                              /* "bad_alloc" (retail's T143 has it first) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf function) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/race/display/T154_race_display.h"         /* base RaceDisplay, RaceEventDisplay::addEvent (first:
                                               retail's type-name order) */
#include "game/race/display/T143_race_free_run_display.h"    /* own: RaceFreeRunDisplay */
#include "game/data/messages/T186_msg_race_display.h"     /* g_002C43E8, the RACEDISPLAY texts */

/* The colour of the event board's messages (RaceEventDisplay's default,
 * ABGR). */
#define FREE_RUN_MESSAGE_COLOR 0x802BCC49
/* The enhanced engine (hud.md C23): RaceDisplay::replay of a live session
 * (T154's RD_LIVE). */
#include "game/enhanced.h"                  /* ENH_HUD_FREE_RUN_LIVE_MESSAGES */
#define FREE_RUN_LIVE 0

// FUNC 0x00189998 RaceFreeRunDisplay::RaceFreeRunDisplay
//   RaceDisplay (0x001909E0), then its vptr.
RaceFreeRunDisplay::RaceFreeRunDisplay()
{
    /* implicit: RaceDisplay (the base's / members' constructors) */
}

// FUNC 0x001899D0 RaceFreeRunDisplay::put_start_display
//   (index.txt: virtual_18) RaceDisplay's ("@start"), then the texts
//   FreeRunReady and FreeRunHelp on the event board, each for 3 s: the
//   first 2 s after the last event's start, the second when it ends.
//   The enhanced engine (ENH_HUD_FREE_RUN_LIVE_MESSAGES, hud.md C23; Lupo 2
//   L2 0x00187218): the two texts only in a live session (GT3 also put them
//   up in its replays), in "sw_16" set here.
void RaceFreeRunDisplay::put_start_display()
{
    RaceDisplay::put_start_display();
    RaceEventDisplay *board = &events;
#if ENH_HUD_FREE_RUN_LIVE_MESSAGES
    if (replay != FREE_RUN_LIVE)
        return;
    board->color = FREE_RUN_MESSAGE_COLOR;
    board->setFont("sw_16");
    board->addEvent(g_002C43E8[RACEDISPLAY_FREE_RUN_READY], 3.0f, 2.0f);
    board->addEvent(g_002C43E8[RACEDISPLAY_FREE_RUN_HELP], 3.0f, 0.0f);
#else
    board->color = FREE_RUN_MESSAGE_COLOR;
    board->addEvent(g_002C43E8[RACEDISPLAY_FREE_RUN_READY], 3.0f, 2.0f);
    board->color = FREE_RUN_MESSAGE_COLOR;
    board->addEvent(g_002C43E8[RACEDISPLAY_FREE_RUN_HELP], 3.0f, 0.0f);
#endif
}

// FUNC 0x00189A60 RaceFreeRunDisplay::put_finish_display
//   Nothing: a free run has no finish.
void RaceFreeRunDisplay::put_finish_display()
{
}

// FUNC 0x00189A68 RaceFreeRunDisplay::~RaceFreeRunDisplay
/* inline in game/race/display/T143_race_free_run_display.h; egcs emits this copy: its
 * vptr, then ~RaceDisplay with the caller's flags (which deletes).
 * RaceFreeRun's destructor has it expanded. */
/* implicit: ~RaceDisplay (the base / member destructors) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00189A90 __tf18RaceFreeRunDisplay
//   Builds the __si_type_info __ti18RaceFreeRunDisplay (base RaceDisplay,
//   __ti11RaceDisplay) on the first call.
extern "C" const type_info &__tf18RaceFreeRunDisplay(void)
{
    if (__ti18RaceFreeRunDisplay[0] == 0) {
        __tf11RaceDisplay();
        __rtti_si(__ti18RaceFreeRunDisplay, "18RaceFreeRunDisplay", (const type_info *)__ti11RaceDisplay);
    }
    return *(const type_info *)__ti18RaceFreeRunDisplay;
}
#endif
