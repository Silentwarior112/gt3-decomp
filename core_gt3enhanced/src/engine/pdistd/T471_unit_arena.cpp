/* T471 - _UnitArenaBase: a pool of units with a free list (the free list
 * is built by the derived UnitArena<N>, virtual slot 1 buildFreeList).
 * 0x0022C960..0x0022CB58. */
#include "game/sound/gtsound_internal.h"
#include "engine/pdistd/T471_unit_arena.h"

// FUNC 0x0022C960 _UnitArenaBase::~_UnitArenaBase
_UnitArenaBase::~_UnitArenaBase()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    reset();
    /* the deleting form: operator delete (0x0027C0C0) for flags & 1 */
}

// FUNC 0x0022C9B0 _UnitArenaBase::_UnitArenaBase
_UnitArenaBase::_UnitArenaBase()
    : buffer(NULL), size(0), unitSize(0), freeList(NULL), used(0), nfree(0)
{
}

// FUNC 0x0022C9E0 _UnitArenaBase::_UnitArenaBase(void *, int, int)
_UnitArenaBase::_UnitArenaBase(void *buf, int bytes, int unit)
    : buffer(buf), size(bytes), unitSize(unit), freeList(NULL), used(0), nfree(0)
{
}

/* Forgets the buffer; returns it.  (used / nfree are left as they are.) */
// FUNC 0x0022CA10 _UnitArenaBase::reset
void *_UnitArenaBase::reset()
{
    void *old = buffer;

    buffer = NULL;
    size = 0;
    unitSize = 0;
    freeList = NULL;
    return old;
}

/* Takes a new buffer and has the free list built; returns the old buffer. */
// FUNC 0x0022CA28 _UnitArenaBase::init
void *_UnitArenaBase::init(void *buf, int bytes, int unit)
{
    void *old = reset();

    buffer = buf;
    size = bytes;
    unitSize = unit;
    freeList = NULL;
    buildFreeList();
    return old;
}

/* Takes the first free unit (no check for an empty list). */
// FUNC 0x0022CAA8 _UnitArenaBase::alloc
void *_UnitArenaBase::alloc(int unused)
{
    void **unit = freeList;

    freeList = (void **)*unit;
    nfree--;
    used++;
    return unit;
}

/* A free unit's first word links to the next free unit (a void **, as
 * freeList: retail keeps that store before the counters'). */
// FUNC 0x0022CAD8 _UnitArenaBase::release
void _UnitArenaBase::release(void *unit)
{
    *(void ***)unit = freeList;
    nfree++;
    used--;
    freeList = (void **)unit;
}

// FUNC 0x0022CB00 _UnitArenaBase::getUnitSize
int _UnitArenaBase::getUnitSize()
{
    return unitSize;
}

// FUNC 0x0022CB08 _UnitArenaBase::getUsed
int _UnitArenaBase::getUsed()
{
    return used;
}

// FUNC 0x0022CB10 _UnitArenaBase::getFree
int _UnitArenaBase::getFree()
{
    return nfree;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (egcs emits it with the vtable). */
// FUNC 0x0022CB18 __tf14_UnitArenaBase
extern "C" void *__tf14_UnitArenaBase(void)
{
    if (__ti14_UnitArenaBase[0] == 0)
        __rtti_user(__ti14_UnitArenaBase, "14_UnitArenaBase");
    return __ti14_UnitArenaBase;
}
#endif
