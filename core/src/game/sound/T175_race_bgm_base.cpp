#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/sound/T175_race_bgm_base.cpp"
#pragma implementation "src/game/sound/T175_race_bgm_base.cpp"
#pragma implementation "T175_race_bgm_base.h"
#endif
/* T175_race_bgm_base.cpp - RaceBGMBase, the race BGM interface
 * (0x001A6318..0x001A6488): every method empty (RaceBGMPS2, T130, is the
 * real one), the class's copies of RaceBase's race-pool operator new /
 * delete, the static dummy instance g_003521D0 (RaceBase's bgm until
 * RacePS2Base installs its RaceBGMPS2) with its static constructor /
 * destructor, then the out-of-line copies of the two inline accessors and
 * the typeinfo function.
 *
 * #pragma implementation (egcs only, first lines; the #line names this
 * file, as T130 / T216 do): the pair of T175_race_bgm_base.h's #pragma
 * interface, so the inline accessors, the vtable and __tf11RaceBGMBase are
 * emitted here, after the static functions.
 *
 * Data layout (retail .rodata 0x00330880: "bad_alloc", the vtable, the type
 * name; .sdata 0x003521D0: the object, FLT_MAX): <new> first, then only the
 * headers the code needs, this unit's last.  The "bad_alloc" is
 * unreferenced: the anchor below. */
// DATA_START 0x00330880
#include <new>
#include "types.h"
#include "runtime/cxx/T548_tinfo.h"                /* type_info, __rtti_user (the GCC 14 __tf function) */

/* The dummy BGM declared while RaceBGMBase is still incomplete (as T125 and
 * T129, u043, do): egcs then leaves the 8-byte object out of small data, and the
 * static constructor / destructor build its address with lui / addiu as
 * retail does (not $gp-relative). */
class RaceBGMBase;
extern RaceBGMBase g_003521D0;              /* 0x003521D0 */

#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/sound/T175_race_bgm_base.h"

/* 0x003521D0 (.sdata): defined before getMarkerTime, whose FLT_MAX follows
 * it.  Its _GLOBAL_$I$ / $D$ functions are the two markers below. */
RaceBGMBase g_003521D0;

// FUNC 0x001A6318 RaceBGMBase::operator new
//   From the race pool (Allocator::allocate), 16-aligned.  (No callers.)
void *RaceBGMBase::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x001A6340 RaceBGMBase::operator delete
//   Back to the race pool (Allocator::free).  Caller: the deleting
//   destructor (size 8).
void RaceBGMBase::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x001A6368 RaceBGMBase::RaceBGMBase
//   Only the vptr (+4 = 0x00330890); org is not set.  Callers: RaceBGMPS2's
//   constructor (T130), T175_static_init.
RaceBGMBase::RaceBGMBase()
{
}

// FUNC 0x001A6380 RaceBGMBase::~RaceBGMBase
//   (index: RaceBGMBase::virtual_0)  The vptr, then with egcs flags bit 0
//   operator delete(this, 8).
RaceBGMBase::~RaceBGMBase()
{
}

// FUNC 0x001A63B0 RaceBGMBase::update
//   The update: nothing.
void RaceBGMBase::update()
{
}

// FUNC 0x001A63B8 RaceBGMBase::getMarkerTime
//   The time to channel ch's next marker: FLT_MAX (the word 0x003521D8).
f32 RaceBGMBase::getMarkerTime(int ch)
{
    return 3.4028235e38f;
}

// FUNC 0x001A63C8 RaceBGMBase::getMarkerHit
//   Channel ch passed a marker this frame: never.
int RaceBGMBase::getMarkerHit(int ch)
{
    return 0;
}

// FUNC 0x001A63D0 RaceBGMBase::play
//   Play: nothing.
void RaceBGMBase::play(int code, int arg)
{
}

// FUNC 0x001A63D8 RaceBGMBase::pause
//   Pause: nothing.
void RaceBGMBase::pause()
{
}

// FUNC 0x001A63E0 RaceBGMBase::resume
//   Resume: nothing.
void RaceBGMBase::resume()
{
}

// FUNC 0x001A63E8 RaceBGMBase::stop
//   Stop: nothing.
void RaceBGMBase::stop()
{
}

// FUNC 0x001A63F0 T175_static_dtor (_GLOBAL_$D$g_003521D0)
/* the static destructor (the .dtors entry 0x0028F21C): g_003521D0's
 * destructor (RaceBGMBase::virtual_0) with egcs flags 2; egcs emits it at
 * the end of the unit */
/* implicit: ~RaceBGMBase (the static object's destructor) */

// FUNC 0x001A6418 T175_static_init (_GLOBAL_$I$g_003521D0)
/* the static constructor (the .ctors entry 0x0028F13C): RaceBGMBase::
 * RaceBGMBase on g_003521D0 */
/* implicit: RaceBGMBase (the static object's constructor) */

// FUNC 0x001A6438 RaceBGMBase::F_001A6438 (F_001A6438__11RaceBGMBase)
/* inline in game/sound/T175_race_bgm_base.h (return org); egcs emits this copy
 * (#pragma implementation) */

// FUNC 0x001A6440 RaceBGMBase::F_001A6440 (F_001A6440__11RaceBGMBaseP16RaceOrganization)
/* inline in game/sound/T175_race_bgm_base.h (org = o); egcs emits this copy
 * (#pragma implementation) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001A6448 __tf11RaceBGMBase
//   The __user_type_info __ti11RaceBGMBase (gp-0x4628).  Caller:
//   __tf10RaceBGMPS2 (T130).
extern "C" const type_info &__tf11RaceBGMBase(void)
{
    if (__ti11RaceBGMBase[0] == 0)
        __rtti_user(__ti11RaceBGMBase, "11RaceBGMBase");
    return *(const type_info *)__ti11RaceBGMBase;
}
#endif
