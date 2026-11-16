/* T168_pause_base.cpp - PauseBase, the base of the pause menus
 * (0x001A2590..0x001A2678): the menu that continues at once, and a frame
 * counter with an optional timeout (the frames since the last input). */
/* Its own include set (not race_pause_internal.h): retail's .rodata 0x0032FF98..0x003300A1
 * is "bad_alloc" (<new>'s inline what()), the vtable, then the type names
 * 9PauseBase, the race displays (RaceMiniMap .. RaceDisplayBase, T154) and
 * UnitArena<64> last (instantiated first: T153 with GAME_T153_EMITS_TYPEINFO
 * before T154); its .sdata 0x00352180 is the "" of T157's inline
 * RaceMessageDisplay::clear, which T154 brings.  None of them is used. */
#include "types.h"
#include <new>                              /* retail's "bad_alloc" (DATA_START) */
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"          /* UnitArena<64>'s type name (no use) */
#include "game/race/display/T154_race_display.h"         /* the race displays' type names, T157's "" (no use) */
#include "game/race/pause/T168_pause_base.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* type_info, __rtti_user (the GCC 14 __tf9PauseBase) */
#endif

// DATA_START 0x0032FF98
// DATA_START 0x00352180

#define PAUSE_FRAMES_PER_SECOND 60          /* PauseBase::setTimeout */

// FUNC 0x001A2590 PauseBase::PauseBase
PauseBase::PauseBase()
    : timer(0), timeout(0)
{
}

// FUNC 0x001A25B0 PauseBase::~PauseBase
PauseBase::~PauseBase()
{
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x001A25E0 PauseBase::setTimeout
//   timeout = seconds * 60 frames.  Caller: RaceArcadeDemo::virtual_8 (30 s).
//   /* ? */ name only (no symbol); the code is certain.
void PauseBase::setTimeout(int seconds)
{
    timeout = seconds * PAUSE_FRAMES_PER_SECOND;
}

// FUNC 0x001A25F0 PauseBase::virtual_2
//   The update of a paused frame: continue at once.
int PauseBase::virtual_2(RaceInput *in, RaceOrganization *org)
{
    return PAUSE_RESULT_CONTINUE;
}

// FUNC 0x001A25F8 PauseBase::virtual_3
//   The draw: nothing.
void PauseBase::virtual_3(GSBuffer *buf, RaceOrganization *org, VariableCameraBase *camera)
{
}

// FUNC 0x001A2600 PauseBase::tick
//   Counts a paused frame; 1 once more than `timeout' frames have passed
//   since the last resetTimer (never without a timeout: the timer then does
//   not count).  Callers: the virtual_2 of RacePause / ReplayPause /
//   RacePauseWithRetry, every frame without input.
//   /* ? */ name only (no symbol); the code is certain.
int PauseBase::tick()
{
    int r = 0;

    if (timeout != 0) {
        timer++;
        r = timeout < timer;
    }
    return r;
}

// FUNC 0x001A2628 PauseBase::resetTimer
//   The timer back to 0: on opening and on every input (the same callers).
//   /* ? */ name only (no symbol); the code is certain.
void PauseBase::resetTimer()
{
    timer = 0;
}

// FUNC 0x001A2630 PauseBase::virtual_1
//   Opened: nothing.
void PauseBase::virtual_1()
{
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001A2638 __tf9PauseBase
//   The __user_type_info __ti9PauseBase (gp-0x4630), the base node of
//   RacePause / ReplayPause / RacePauseWithRetry.
extern "C" const type_info &__tf9PauseBase(void)
{
    if (__ti9PauseBase[0] == 0)
        __rtti_user(__ti9PauseBase, "9PauseBase");
    return *(const type_info *)__ti9PauseBase;
}
#endif
