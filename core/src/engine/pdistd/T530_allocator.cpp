#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/engine/pdistd/T530_allocator.cpp"
#pragma implementation "src/engine/pdistd/T530_allocator.cpp"
#pragma implementation "allocator.h"
#endif
/* T530_allocator.cpp - pdistd::Allocator (Polyphony allocator.cxx).
 *
 * #pragma implementation (egcs only, first lines): allocator.cxx paired it
 * with the #pragma interface of pdistd/allocator.h, which is why the vtable
 * and the out-of-line copies of the inline members repValid and reset are in
 * this object (as in the bootstrap's allocator.cpp; the #line names this file
 * because the SCE driver compiles a temporary .ii).
 *
 * Same machine code as the bootstrap's src/pdistd/allocator.cpp
 * (0x0100F918..0x01010318); the source below is that file's, with the core
 * addresses.  Class layout: include/pdistd/allocator.h (core addresses in
 * include/engine/pdistd/T530_allocator.h).
 *
 * The arena is carved into blocks that start with a 16-byte Rep header
 * {prev, next, prevFree, nextFree}; all Reps form an address-ordered chain
 * (a block's size is next - rep), free ones are also on a free list, allocated
 * ones carry prevFree == nextFree == 0xFFFFFFFF.  Every public entry point
 * brackets its work with intr_suspend() ... `if (was) ei`.
 *
 * vtable _vt$Q26pdistd9Allocator @0x00346098:
 *   [0] +0xC  ~Allocator  0x0026A828
 * RTTI name "Q26pdistd9Allocator" @0x003460A8, __user_type_info @0x003534F0.
 */
#include "engine/pdistd/pdistd_internal.h"
#include "engine/pdistd/T530_allocator.h"

namespace pdistd {

/* All size arithmetic is on the 32-bit addresses (sltu = unsigned compares). */
#define REP_ADDR(r) ((u32)(r))

// FUNC 0x0026A7C8 pdistd::Allocator::Allocator(int)
// Empty allocator: stores the fit-mode flag (+0x10), sets the vptr (+0x14) to
// 0x00346098 and zeroes first/end/head/freeList.  Callers: the static
// constructor global_ctor_allocator (firstFit = 0) and four game functions
// (0x0015B9D8, 0x00179B50, 0x001A6138, 0x001B3A98) that build their own.
Allocator::Allocator(int firstFit)
    : m_firstFit(firstFit)
{
    m_first    = 0;
    m_end      = 0;
    m_head     = 0;
    m_freeList = 0;
}

// FUNC 0x0026A7F0 pdistd::Allocator::Allocator(void*, unsigned int, int)
// Stores the flag and vptr, then assign(base, size).  first/end/head/freeList
// are not zeroed first (assign reads the stale m_first for its unused return
// value).  Unreferenced.
Allocator::Allocator(void *base, u32 size, int firstFit)
    : m_firstFit(firstFit)
{
    assign(base, size);
}

// FUNC 0x0026A828 pdistd::Allocator::~Allocator(void)
// Restores vptr 0x00346098, zeroes first/end/head/freeList (in that order),
// and if (flags & 1) calls operator delete(this) (0x0027C0C0).  Vtable slot 0
// (+0xC).  global_dtor_allocator passes flags = 2 (no delete).
Allocator::~Allocator()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    m_first    = 0;
    m_end      = 0;
    m_head     = 0;
    m_freeList = 0;
}

// FUNC 0x0026A868 pdistd::Allocator::alignRepStart
// Moves the start of a (just unlinked from the free list) block so that its
// payload (rep + 0x10) is aligned to `align` (a power of two >= 0x10).
//  - already aligned: returns rep.
//  - gap < 0x20: slides the 16-byte header up by `gap` bytes (only the
//    address-chain links are copied); the gap is absorbed by the previous
//    block, or - if rep was the chain head - m_head is moved up and the gap
//    in front of it is left dangling (rebuildHeadRep reclaims it on free).
//  - gap >= 0x20: splits off the gap as its own block and puts it on the
//    free list.
// Returns the (possibly new) Rep.  Only caller: allocate.
Allocator::Rep *Allocator::alignRepStart(Rep *rep, u32 align)
{
    u32 mask = align - 1;
    u32 payload = REP_ADDR(rep) + sizeof(Rep);
    u32 aligned = (payload + mask) & ~mask;
    Rep *moved;

    if (payload == aligned)
        return rep;

    u32 gap = aligned - payload;
    moved = (Rep *)(aligned - sizeof(Rep));
    if (gap < 0x20) {
        Rep *prev = rep->prev;
        Rep *next = rep->next;
        moved->prev = prev;
        moved->next = next;
        if (prev != 0)
            prev->next = moved;
        else
            m_head = moved;
        if (next < m_end)
            next->prev = moved;
        return moved;
    }

    moved = insertRepAt(rep, gap);
    pushFreeRep(rep);
    return moved;
}

// FUNC 0x0026A928 pdistd::Allocator::rebuildHeadRep
// Re-creates the chain head at the arena start: m_head = m_first,
// m_first->prev = 0, m_first->next = rep->next (+ back link if inside the
// arena).  Called by free() when the freed block has no predecessor.
void Allocator::rebuildHeadRep(Rep *rep)
{
    Rep *head = m_first;
    Rep *next = rep->next;

    m_head = head;
    head->prev = 0;
    head->next = next;
    if (next < m_end)
        next->prev = head;
}

// FUNC 0x0026A958 pdistd::Allocator::insertRepAt
// Split primitive: creates a Rep header at rep + offset and links it into
// the address chain between rep and rep->next (fixing next->prev only if
// next is inside the arena).  Free-list fields of the new Rep are left
// untouched.  Returns the new Rep.
Allocator::Rep *Allocator::insertRepAt(Rep *rep, u32 offset)
{
    Rep *next = rep->next;
    Rep *ins  = (Rep *)((u8 *)rep + offset);

    if (next < m_end)
        next->prev = ins;
    ins->next = next;
    rep->next = ins;
    ins->prev = rep;
    return ins;
}

// FUNC 0x0026A988 pdistd::Allocator::unlinkRep
// Coalescing primitive: removes `next` (rep's successor) from the address
// chain so that rep absorbs its bytes.
void Allocator::unlinkRep(Rep *rep, Rep *next)
{
    Rep *after = next->next;

    rep->next = after;
    if (after < m_end)
        after->prev = rep;
}

// FUNC 0x0026A9A8 pdistd::Allocator::pushFreeRep
// Head-inserts rep on the free list (prevFree = 0, nextFree = old head).
void Allocator::pushFreeRep(Rep *rep)
{
    Rep *head = m_freeList;

    rep->prevFree = 0;
    rep->nextFree = head;
    if (head != 0)
        head->prevFree = rep;
    m_freeList = rep;
}

// FUNC 0x0026A9C8 pdistd::Allocator::removeFreeRep
// Unlinks rep from the free list; rep's own prevFree/nextFree are left as
// they were.
void Allocator::removeFreeRep(Rep *rep)
{
    Rep *prev = rep->prevFree;
    Rep *next = rep->nextFree;

    if (prev != 0)
        prev->nextFree = next;
    else
        m_freeList = next;
    if (next != 0)
        next->prevFree = prev;
}

// FUNC 0x0026A9F0 pdistd::Allocator::assign
// Hands the allocator an arena: base is rounded up to 16, the size is
// reduced by the rounding and rounded down to 16.  first = head = freeList =
// the single spanning free Rep {prev 0, next = end, prevFree 0, nextFree 0}.
// Returns the previous m_first (no known caller uses it).  Callers:
// Allocator(void*,u32,int), heap_init_once and four game functions.
void *Allocator::assign(void *base, u32 size)
{
    /* signed arithmetic: the mask is the immediate -16 (li), unlike the
     * unsigned 0xFFFFFFF0 (lui/ori) below */
    Rep  *first = (Rep *)(((int)base + 0xF) & ~0xF);
    void *old;

    size = (size - (REP_ADDR(first) - REP_ADDR(base))) & 0xFFFFFFF0;
    old = m_first;
    m_first    = first;
    m_head     = first;
    m_end      = (Rep *)((u8 *)first + size);
    m_freeList = first;
    first->prev = 0;
    /* the binary re-reads m_first / m_end here (the store above may alias) */
    m_first->next     = m_end;
    m_first->prevFree = 0;
    m_first->nextFree = 0;
    return old;
}

// FUNC 0x0026AA50 pdistd::Allocator::unassign
// Detaches the arena: returns the previous m_first and zeroes
// freeList/first/end/head (inlined reset(), same store order).
void *Allocator::unassign(void)
{
    void *old = m_first;

    reset();
    return old;
}

// FUNC 0x0026AA68 pdistd::Allocator::allocate (heap_alloc_core)
// align = (align > 15) ? align : 16; need = roundup(size, align) + 0x10.
// Under intr_suspend, scans the free list for a block whose size covers
// need plus the alignment slack of its payload.  Best fit (m_firstFit == 0):
// keeps the smallest such block, a later block of equal size replacing the
// earlier one; first fit (m_firstFit != 0): stops at the first match.  The
// chosen block is taken off the free list, aligned (alignRepStart), and if
// at least need + 0x20 bytes remain, the tail is split off at +need and
// pushed back on the free list.  The block is marked allocated
// (prevFree = nextFree = 0xFFFFFFFF).  Returns rep + 0x10, or NULL.
// 48 callers: the malloc family and the game's own Allocators.
void *Allocator::allocate(u32 size, u32 align)
{
    Rep *best = 0;
    Rep *rep;
    u32  bestSize;
    u32  mask;
    u32  need;
    int  intr;
    int  firstFit;

    if (!(0xF < align))
        align = 0x10;
    mask = align - 1;
    need = ((size + mask) & ~mask) + sizeof(Rep);

    intr = intr_suspend();

    firstFit = m_firstFit;      /* read once, before the scan */
    bestSize = 0;
    for (rep = m_freeList; rep != 0; rep = rep->nextFree) {
        u32 payload   = REP_ADDR(rep) + sizeof(Rep);
        u32 blockSize = REP_ADDR(rep->next) - REP_ADDR(rep);
        u32 m         = align - 1;
        u32 slack     = ((payload + m) & ~m) - payload;

        if (blockSize < need + slack)
            continue;
        if (best != 0 && bestSize < blockSize)
            continue;
        bestSize = blockSize;
        best     = rep;
        if (firstFit != 0)
            break;
    }

    if (best != 0) {
        removeFreeRep(best);
        best = alignRepStart(best, align);
        bestSize = REP_ADDR(best->next) - REP_ADDR(best);
        if (!(bestSize < need + 0x20))
            pushFreeRep(insertRepAt(best, need));
        best->prevFree = PDISTD_REP_ALLOCATED;
        best->nextFree = PDISTD_REP_ALLOCATED;
    }

    if (intr)
        EI();

    return (best != 0) ? (void *)(best + 1) : 0;
}

// FUNC 0x0026ABC0 pdistd::Allocator::reallocate
// ptr == NULL -> allocate(size, align).  An invalid ptr (repValid fails)
// returns NULL.  The in-place test always rounds size to 16:
// need = roundup16(size) + 0x10.
//  - block already >= need: returns ptr unchanged (no shrink/split).
//  - else, if the following block is inside the arena and free, it is
//    absorbed; if the merged block is >= roundup16(size) + 0x30, the tail is
//    split off at +need, freed, and ptr is returned.
//  - otherwise (interrupts restored first): new = allocate(need, align),
//    memcpy(new, ptr, rep->next - rep), free(ptr), return new.
// Quirks, reproduced: the fresh allocation asks for `need` (0x10 more than
// size); the copy length is the whole old block INCLUDING its 16-byte header
// (reads 16 bytes past the old payload, can write 16 bytes past the new one);
// the result of allocate() is not checked for NULL.  Only caller: realloc.
void *Allocator::reallocate(void *ptr, u32 size, u32 align)
{
    Rep  *rep;
    Rep  *next;
    void *result;
    u32   alignedSize;
    u32   need;
    int   intr;

    if (ptr == 0)
        return allocate(size, align);

    rep = (Rep *)ptr - 1;
    if (!repValid(rep))
        return 0;

    result = ptr;
    intr = intr_suspend();
    next = rep->next;
    alignedSize = ((int)size + 0xF) & ~0xF;
    need = alignedSize + sizeof(Rep);
    if (REP_ADDR(next) - REP_ADDR(rep) < need) {
        result = 0;
        if (next < m_end && next->prevFree != PDISTD_REP_ALLOCATED) {
            removeFreeRep(next);
            unlinkRep(rep, next);
            if (!(REP_ADDR(rep->next) - REP_ADDR(rep) < alignedSize + 0x30)) {
                result = ptr;
                pushFreeRep(insertRepAt(rep, need));
            }
        }
    }

    if (intr)
        EI();

    if (result == 0) {
        result = allocate(need, align);
        memcpy(result, ptr, REP_ADDR(rep->next) - REP_ADDR(rep));
        free(ptr);
    }
    return result;
}

// FUNC 0x0026AD88 pdistd::Allocator::free (heap_free_core)
// Validates ptr - 0x10 with the inlined repValid() and silently ignores
// invalid pointers.  Under intr_suspend: if the previous block is free it is
// taken off the free list and absorbs this one (rep = prev); if there is no
// previous block, rebuildHeadRep(rep) re-creates the head at m_first.  Then
// a free following block is absorbed, and the result is pushed on the free
// list.
// QUIRK (reproduced): after rebuildHeadRep the code keeps working on the old
// `rep` header - wrong if alignRepStart had slid the head header up (latent
// heap corruption, see the bootstrap notes).  47 callers.
void Allocator::free(void *ptr)
{
    Rep *rep = (Rep *)ptr - 1;
    Rep *prev;
    Rep *next;
    int  intr;

    if (!repValid(rep))
        return;

    intr = intr_suspend();

    prev = rep->prev;
    if (prev != 0) {
        if (prev->prevFree != PDISTD_REP_ALLOCATED) {
            removeFreeRep(prev);
            unlinkRep(prev, rep);
            rep = prev;
        }
    } else {
        rebuildHeadRep(rep);
    }

    next = rep->next;
    if (next < m_end && next->prevFree != PDISTD_REP_ALLOCATED) {
        removeFreeRep(next);
        unlinkRep(rep, next);
    }
    pushFreeRep(rep);

    if (intr)
        EI();
}

// FUNC 0x0026AEC0 pdistd::Allocator::getMaxFreeSize
// Under intr_suspend: largest (next - rep) over the free list, minus the
// 0x10 header.  Quirk: with an empty free list it returns 0xFFFFFFF0.
u32 Allocator::getMaxFreeSize(void)
{
    int  intr = intr_suspend();
    u32  max  = 0;
    Rep *rep;

    for (rep = m_freeList; rep != 0; rep = rep->nextFree) {
        u32 blockSize = REP_ADDR(rep->next) - REP_ADDR(rep);
        if (max < blockSize)
            max = blockSize;
    }

    if (intr)
        EI();
    return max - sizeof(Rep);
}

// FUNC 0x0026AF28 pdistd::Allocator::getTotalFreeSize
// Under intr_suspend: sum over the free list of (next - rep - 0x10).
u32 Allocator::getTotalFreeSize(void)
{
    int  intr  = intr_suspend();
    u32  total = 0;
    Rep *rep;

    for (rep = m_freeList; rep != 0; rep = rep->nextFree)
        total += REP_ADDR(rep->next) - REP_ADDR(rep) - sizeof(Rep);

    if (intr)
        EI();
    return total;
}

// FUNC 0x0026AF90 pdistd::Allocator::debug
// Heap dump through printf (0x0026C690), interrupts disabled throughout.
// Strings @0x00346010, 0x00346038, 0x00346050, 0x00346080.
void Allocator::debug(void)
{
    int  intr = intr_suspend();
    Rep *rep;

    printf("Allocator::debug - %#010x %#010x\n", REP_ADDR(m_head), REP_ADDR(m_freeList));
    printf("  %#010x: --- start\n", REP_ADDR(m_first));
    for (rep = m_head; !(rep < m_first) && rep < m_end; rep = rep->next) {
        printf("  %#010x: %#010x %#010x %#010x %#010x %#010x\n",
               REP_ADDR(rep), REP_ADDR(rep->prev), REP_ADDR(rep->next),
               REP_ADDR(rep->prevFree), REP_ADDR(rep->nextFree),
               REP_ADDR(rep->next) - REP_ADDR(rep));
    }
    printf("  %#010x: --- end\n", REP_ADDR(m_end));

    if (intr)
        EI();
}

// FUNC 0x0026B088 pdistd::Allocator::countFreeReps
// Under intr_suspend: walks the address chain from m_head while
// first <= rep < end and counts the FREE blocks (prevFree and nextFree both
// != 0xFFFFFFFF).
int Allocator::countFreeReps(void)
{
    int  intr  = intr_suspend();
    int  count = 0;
    Rep *rep;

    for (rep = m_head; !(rep < m_first) && rep < m_end; rep = rep->next) {
        if (rep->prevFree != PDISTD_REP_ALLOCATED &&
            rep->nextFree != PDISTD_REP_ALLOCATED)
            count++;
    }

    if (intr)
        EI();
    return count;
}

} /* namespace pdistd */

/* The inline members repValid and reset (bodies in include/pdistd/allocator.h)
 * get their out-of-line copies here through #pragma implementation; egcs
 * emits them at the end of the object, as in the binary. */

// FUNC 0x0026B128 repValid__Q26pdistd9AllocatorPQ36pdistd9Allocator3Rep pdistd::Allocator::repValid (out-of-line copy of the inline member)
/* inline in pdistd/allocator.h; egcs emits this copy.  True if rep lies
 * inside [m_first, m_end) (unsigned compares) and is marked allocated
 * (prevFree == nextFree == 0xFFFFFFFF).  Unreferenced (free and reallocate
 * inline it). */

// FUNC 0x0026B170 reset__Q26pdistd9Allocator pdistd::Allocator::reset (out-of-line copy of the inline member)
/* inline in pdistd/allocator.h; egcs emits this copy.  Zeroes the free-list
 * head, first, end and head (that store order).  Unreferenced (unassign
 * inlines it). */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0026B188 __tfQ26pdistd9Allocator
extern "C" void *__tfQ26pdistd9Allocator(void)
{
    if (__tiQ26pdistd9Allocator[0] == 0)
        __rtti_user(__tiQ26pdistd9Allocator, "Q26pdistd9Allocator");
    return __tiQ26pdistd9Allocator;
}
#endif
