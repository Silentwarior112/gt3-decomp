#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/pdistd/allocator.cpp"
#pragma implementation "src/pdistd/allocator.cpp"
#pragma implementation "allocator.h"
#endif
/* allocator.cpp - Polyphony pdistd::Allocator (allocator.cxx) and the C heap
 * front-ends of the GT3 US bootstrap (BOOTSTRAP_SCUS-97102), unit "allocator".
 *
 *   0x0100F918..0x01010318  pdistd::Allocator methods, then the out-of-line
 *                           inline members (repValid, reset - defined in
 *                           include/pdistd/allocator.h) and __tf
 *   0x01011210..0x010114A8  heap_init_once, heap query wrappers, calloc/free/
 *                           malloc/realloc/memalign, global ctor/dtor
 *                           (linked interleaved with printf.cxx's statics)
 *
 * Every public Allocator entry point that touches the lists brackets its
 * work with intr_suspend() ... `if (was_enabled) ei` (no semaphore): the
 * heap is protected against interrupt handlers and thread switches by
 * disabling interrupts, including during the Allocator::debug printf dump
 * and during the one-time memset of the whole ~15 MB arena.
 *
 * #pragma implementation (egcs only, first lines of this file): allocator.cxx
 * paired it with #pragma interface in allocator.h, which is why the inline
 * members repValid and reset have out-of-line copies at the end of the
 * Allocator object.  egcs accepts the pragma only in the "main" file, and the
 * SCE driver compiles a temporary .ii whose name never matches the source's,
 * so a #line directive first names this file by its repo-relative path (the
 * same path the scorer looks for) and the pragma repeats that name; the
 * second pragma then adds "allocator.h".  Keep the #line number equal to the
 * line it precedes.
 */
#include "types.h"
#include "eekernel.h"            /* EI() */
#include "libc.h"                /* memcpy, memset, printf, malloc family */
#include "pdistd/allocator.h"

#ifndef BOOT_OLD_GCC
#include <new>                   /* placement new for the global object */
#endif

namespace pdistd {

/* All size arithmetic is on the 32-bit addresses (sltu = unsigned compares). */
#define REP_ADDR(r) ((u32)(r))

// FUNC 0x0100F918 pdistd::Allocator::Allocator(int)
// Empty allocator: stores the fit-mode flag (+0x10), sets the vptr (+0x14) to
// 0x0103E680 and zeroes first/end/head/freeList.  Only caller: the static
// constructor global_ctor_allocator (0x01011480) with firstFit = 0.
Allocator::Allocator(int firstFit)
    : m_firstFit(firstFit)
{
    m_first    = 0;
    m_end      = 0;
    m_head     = 0;
    m_freeList = 0;
}

// FUNC 0x0100F940 pdistd::Allocator::Allocator(void*, unsigned int, int)
// Stores the flag and vptr, then assign(base, size).  first/end/head/freeList
// are not zeroed first (assign reads the stale m_first for its unused return
// value).  Unreferenced in this binary.
Allocator::Allocator(void *base, u32 size, int firstFit)
    : m_firstFit(firstFit)
{
    assign(base, size);
}

// FUNC 0x0100F978 pdistd::Allocator::~Allocator(void)
// GCC 2.9x destructor with hidden `int flags`: restores vptr 0x0103E680,
// zeroes first/end/head/freeList (in that order), and if (flags & 1) calls
// operator delete(this) (0x01019910).  Vtable slot 1 (0x0103E68C).  The only
// direct caller, global_dtor_allocator, passes flags = 2 (no delete).
Allocator::~Allocator()
{
    m_first    = 0;
    m_end      = 0;
    m_head     = 0;
    m_freeList = 0;
}

// FUNC 0x0100F9B8 pdistd::Allocator::alignRepStart
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
    u32 mask = align - 1;       /* a variable: ~mask stays a nor, not -align */
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

// FUNC 0x0100FA78 pdistd::Allocator::rebuildHeadRep
// Re-creates the chain head at the arena start: m_head = m_first,
// m_first->prev = 0, m_first->next = rep->next (+ back link if inside the
// arena).  Called by free() when the freed block has no predecessor; this
// reclaims the <0x20-byte hole alignRepStart may have left in front of the
// head.  Returns nothing (free keeps using `rep`, see the quirk there).
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

// FUNC 0x0100FAA8 pdistd::Allocator::insertRepAt
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

// FUNC 0x0100FAD8 pdistd::Allocator::unlinkRep
// Coalescing primitive: removes `next` (rep's successor) from the address
// chain so that rep absorbs its bytes: rep->next = next->next, and the new
// successor's prev = rep if it is inside the arena.
void Allocator::unlinkRep(Rep *rep, Rep *next)
{
    Rep *after = next->next;

    rep->next = after;
    if (after < m_end)
        after->prev = rep;
}

// FUNC 0x0100FAF8 pdistd::Allocator::pushFreeRep
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

// FUNC 0x0100FB18 pdistd::Allocator::removeFreeRep
// Unlinks rep from the free list.  rep's own prevFree/nextFree are left as
// they were (allocate overwrites them with the allocated marker; the
// coalescing paths in free/reallocate leave them stale).
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

// FUNC 0x0100FB40 pdistd::Allocator::assign
// Hands the allocator an arena: base is rounded up to 16, the size is
// reduced by the rounding and rounded down to 16.  first = head = freeList =
// the single spanning free Rep {prev 0, next = end, prevFree 0, nextFree 0}.
// Returns the previous m_first (v0 is loaded from this+0 before the stores;
// no caller uses it).  Callers: Allocator(void*,u32,int), heap_init_once.
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

// FUNC 0x0100FBA0 pdistd::Allocator::unassign
// Detaches the arena: returns the previous m_first and zeroes
// freeList/first/end/head (inlined reset(), same store order).  Unreferenced
// in this binary.  (The mapping pass read the leading `lw $v0, 0($a0)` as a
// dead load; it is the return value.)
void *Allocator::unassign(void)
{
    void *old = m_first;

    reset();
    return old;
}

// FUNC 0x0100FBB8 pdistd::Allocator::allocate
// align = (align > 15) ? align : 16; need = roundup(size, align) + 0x10.
// Under intr_suspend, scans the free list for a block whose size covers
// need plus the alignment slack of its payload.  Best fit (m_firstFit == 0):
// keeps the smallest such block, a later block of equal size replacing the
// earlier one; first fit (m_firstFit != 0): stops at the first match.  The
// chosen block is taken off the free list, aligned (alignRepStart), and if
// at least need + 0x20 bytes remain, the tail is split off at +need and
// pushed back on the free list.  The block is marked allocated
// (prevFree = nextFree = 0xFFFFFFFF).  Returns rep + 0x10, or NULL.
// Callers: calloc, malloc, memalign front-ends and reallocate.
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

    firstFit = m_firstFit;      /* read once, before the scan (lw before the loop) */
    bestSize = 0;
    for (rep = m_freeList; rep != 0; rep = rep->nextFree) {
        u32 payload   = REP_ADDR(rep) + sizeof(Rep);
        u32 blockSize = REP_ADDR(rep->next) - REP_ADDR(rep);
        u32 m         = align - 1;  /* recomputed here (not `mask`): the
                                       hoisted copies decide the registers */
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
        bestSize = REP_ADDR(best->next) - REP_ADDR(best);   /* reused */
        if (!(bestSize < need + 0x20))
            pushFreeRep(insertRepAt(best, need));
        best->prevFree = PDISTD_REP_ALLOCATED;
        best->nextFree = PDISTD_REP_ALLOCATED;
    }

    if (intr)
        EI();

    return (best != 0) ? (void *)(best + 1) : 0;
}

// FUNC 0x0100FD10 pdistd::Allocator::reallocate
// ptr == NULL -> allocate(size, align).  An invalid ptr (repValid fails)
// returns NULL.  The in-place test always rounds size to 16 (align is only
// used for a fresh allocation): need = roundup16(size) + 0x10.
//  - block already >= need: returns ptr unchanged (no shrink/split).
//  - else, if the following block is inside the arena and free, it is
//    absorbed (removeFreeRep + unlinkRep); if the merged block is
//    >= roundup16(size) + 0x30 (= need + 0x20), the tail is split off at
//    +need, freed, and ptr is returned.
//  - otherwise (interrupts restored first): new = allocate(need, align),
//    memcpy(new, ptr, rep->next - rep), free(ptr), return new.
// Quirks, reproduced: the fresh allocation asks for `need` (0x10 more than
// size); the copy length is the whole old block INCLUDING its 16-byte header,
// so it reads 16 bytes past the old payload, and when a free neighbour was
// absorbed without a split (merged size need + 0x10, all block sizes being
// multiples of 16) the copy can write 0x10 bytes past the new block's payload
// (new payload >= need; exactly need when allocate() split its block), i.e.
// over the Rep header of the block that follows it - after a split that is
// the free remainder just pushed on the free list; the result of allocate()
// is not checked for NULL.  Only caller: realloc (0x010113C8).
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

// FUNC 0x0100FED8 free__Q26pdistd9AllocatorPv pdistd::Allocator::free
// Validates ptr - 0x10 with the inlined repValid() and silently ignores
// invalid pointers.  Under intr_suspend: if the previous block is free it is
// taken off the free list and absorbs this one (rep = prev); if there is no
// previous block, rebuildHeadRep(rep) re-creates the head at m_first.  Then
// a free following block is absorbed, and the result is pushed on the free
// list.
// QUIRK (reproduced): after rebuildHeadRep the code keeps working on the old
// `rep` header.  That is fine when rep == m_first (the normal case), but if
// the head block had been slid up by alignRepStart (memalign with
// align >= 0x20 on the arena's first block, gap 0x10), the stale header at
// m_first + 0x10 is what gets merged with its successor and pushed on the
// free list, while the header now in the chain (m_first) is never put on the
// free list - a latent heap-corruption bug.  Callers: free() front-end
// (0x01011368) and reallocate.
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

// FUNC 0x01010010 pdistd::Allocator::getMaxFreeSize
// Under intr_suspend: largest (next - rep) over the free list, minus the
// 0x10 header, i.e. the largest payload a 16-aligned allocation could get.
// Quirk: with an empty free list it returns 0 - 0x10 = 0xFFFFFFF0.
// Wrapped by heap_max_free_size (0x010112A0).
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

// FUNC 0x01010078 pdistd::Allocator::getTotalFreeSize
// Under intr_suspend: sum over the free list of (next - rep - 0x10), i.e.
// total free payload bytes.  Wrapped by heap_total_free_size (0x010112C0).
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

// FUNC 0x010100E0 pdistd::Allocator::debug
// Heap dump through Polyphony printf (0x01011678), with interrupts disabled
// for the whole dump:
//   "Allocator::debug - %#010x %#010x\n"            head, freeList   @0x0103E5F8
//   "  %#010x: --- start\n"                          first            @0x0103E620
//   "  %#010x: %#010x %#010x %#010x %#010x %#010x\n" rep, prev, next,
//                                   prevFree, nextFree, next - rep   @0x0103E638
//     for each Rep of the address chain from m_head while first <= rep < end
//   "  %#010x: --- end\n"                            end              @0x0103E668
// Wrapped by heap_dump (0x010112E0), itself unreferenced.
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

// FUNC 0x010101D8 pdistd::Allocator::countFreeReps
// Under intr_suspend: walks the address chain from m_head while
// first <= rep < end and counts the Reps with prevFree != 0xFFFFFFFF AND
// nextFree != 0xFFFFFFFF, i.e. the number of FREE blocks (fragment count).
// (The mapping pass called it countAllocatedReps - the test is the other way
// round: `beq prevFree, -1, skip` then `movn` on ~nextFree != 0.)
// Wrapped by heap_free_block_count (0x01011300).
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

/* The inline members repValid and reset (bodies in include/pdistd/allocator.h)
 * get their out-of-line copies here through #pragma implementation; egcs
 * emits them at the end of the object, as in the binary. */

// FUNC 0x01010278 repValid__Q26pdistd9AllocatorPQ36pdistd9Allocator3Rep pdistd::Allocator::repValid (out-of-line copy of the inline member)
//   True if rep lies inside [m_first, m_end) (unsigned compares) and is marked
//   allocated (prevFree == nextFree == 0xFFFFFFFF).  Unreferenced; free() and
//   reallocate() inline it to validate the caller's pointer (ptr - 0x10).

// FUNC 0x010102C0 reset__Q26pdistd9Allocator pdistd::Allocator::reset (out-of-line copy of the inline member)
//   Zeroes the free-list head, first, end and head (in that store order).
//   Unreferenced; unassign() inlines it.

// FUNC 0x010102D8 __tfQ26pdistd9Allocator (compiler-generated)
//   typeinfo function for pdistd::Allocator, vtable slot 0 (0x0103E684).
//   If the first word (name pointer) of the static __user_type_info object at
//   0x01041EA0 (.sbss, gp-0x7E50, 8 bytes {name, vptr}) is still 0, constructs
//   it in place with __user_type_info::__user_type_info(const char *)
//   (0x01019C58) and the name "Q26pdistd9Allocator" (@0x0103E690); returns
//   &0x01041EA0 (a `const type_info &`).  In a modern build the compiler
//   emits the equivalent typeinfo object for the polymorphic class itself.

} /* namespace pdistd */

/* ======================================================================== */
/* Global heap: one pdistd::Allocator over [align16(_end) .. 0x01F00000)     */
/* ======================================================================== */

extern "C" u8 _end[];   /* linker symbol, 0x0104DE94 in this binary */

#ifdef BOOT_OLD_GCC
/* The global allocator object @0x01048240 (.bss, 0x18 bytes), an ordinary
 * global in the original: egcs emits its static constructor / destructor
 * (_GLOBAL_$I$ / _GLOBAL_$D$, see the end of this file). */
static pdistd::Allocator s_allocator(0);

static inline pdistd::Allocator &theAllocator(void)
{
    return s_allocator;
}
#else
/* The global allocator object @0x01048240 (.bss, 0x18 bytes).  Constructed
 * explicitly by global_ctor_allocator (called from the crt ctor table) and
 * destroyed by global_dtor_allocator, exactly as the GCC 2.9x
 * _GLOBAL_$I$/_GLOBAL_$D$ functions do; kept as raw storage here so that the
 * modern compiler does not generate its own static init. */
alignas(8) static u8 s_allocatorStorage[sizeof(pdistd::Allocator)];

static inline pdistd::Allocator &theAllocator(void)
{
    return *reinterpret_cast<pdistd::Allocator *>(s_allocatorStorage);
}
#endif

static void *s_heapBase  = 0;                    /* @0x01041E50 (.sdata) 0 until heap_init_once */
static u32   s_heapSize  = 0;                    /* @0x01041E54 (.sdata) */
static void *s_heapStart = _end;                 /* @0x01041E58 (.sdata) = &_end = 0x0104DE94 */
static void *s_heapLimit = (void *)0x01F00000;   /* @0x01041E5C (.sdata) top of the arena (31 MB) */

// FUNC 0x01011210 heap_init_once
// Under intr_suspend: on the first call (s_heapBase == 0) computes
// base = align16(&_end), size = (0x01F00000 - base) & ~0xF, zero-fills the
// whole arena with memset, hands it to the global Allocator
// (assign(base, size)) and records base/size.  Later calls do nothing.
// Called at the top of calloc, malloc, realloc and memalign (not free).
static void heap_init_once(void)
{
    int intr = intr_suspend();

    if (s_heapBase == 0) {
        /* signed int arithmetic: both masks are the immediate -16 */
        u8 *base = (u8 *)(((int)s_heapStart + 0xF) & ~0xF);
        u32 size = ((int)s_heapLimit - (int)base) & ~0xF;

        memset(base, 0, size);
        theAllocator().assign(base, size);
        s_heapBase = base;
        s_heapSize = size;
    }

    if (intr)
        EI();
}

// FUNC 0x010112A0 heap_max_free_size
// global Allocator .getMaxFreeSize().  No heap_init_once.  Unreferenced.
extern "C" unsigned int heap_max_free_size(void)
{
    return theAllocator().getMaxFreeSize();
}

// FUNC 0x010112C0 heap_total_free_size
// global Allocator .getTotalFreeSize().  No heap_init_once.  Unreferenced.
extern "C" unsigned int heap_total_free_size(void)
{
    return theAllocator().getTotalFreeSize();
}

// FUNC 0x010112E0 heap_dump
// global Allocator .debug().  No heap_init_once.  Unreferenced.
extern "C" void heap_dump(void)
{
    theAllocator().debug();
}

// FUNC 0x01011300 heap_free_block_count
// global Allocator .countFreeReps().  No heap_init_once.  Unreferenced.
// (Mapping-pass name: heap_check.)
extern "C" int heap_free_block_count(void)
{
    return theAllocator().countFreeReps();
}

// FUNC 0x01011320 calloc
// heap_init_once, then allocate(nmemb * size, 0x10) (low 32 bits of a
// signed `mult`; no overflow check).  Quirk: the block is NOT zeroed - only
// memory never handed out before is zero (the arena was memset once by
// heap_init_once).  Unreferenced in this binary.
extern "C" void *calloc(size_t nmemb, size_t size)
{
    heap_init_once();
    return theAllocator().allocate((u32)(nmemb * size), 0x10);
}

// FUNC 0x01011368 free
// if (ptr != NULL) global Allocator .free(ptr).  No heap_init_once.
// Callers: operator delete (0x01019910), operator delete[] (0x01019AB0), the
// EH runtime, frame.c and game code (11 callers).
extern "C" void free(void *ptr)
{
    if (ptr != 0)
        theAllocator().free(ptr);
}

// FUNC 0x01011390 malloc
// heap_init_once, then global Allocator .allocate(size, 0x10).  12 callers,
// including operator new (0x01019988) and __eh_alloc.
extern "C" void *malloc(size_t size)
{
    heap_init_once();
    return theAllocator().allocate(size, 0x10);
}

// FUNC 0x010113C8 realloc
// heap_init_once, then global Allocator .reallocate(ptr, size, 0x10).
// One caller (0x01000FB8, imageloader).
extern "C" void *realloc(void *ptr, size_t size)
{
    heap_init_once();
    return theAllocator().reallocate(ptr, size, 0x10);
}

// FUNC 0x01011410 memalign
// heap_init_once, then global Allocator .allocate(size, align) (align < 16
// is raised to 16 by allocate).  Callers: 0x01000A50 (startup screen) and
// 0x01005458, both with align 0x40.
extern "C" void *memalign(size_t align, size_t size)
{
    heap_init_once();
    return theAllocator().allocate(size, align);
}

/* Static constructor / destructor of the global Allocator.  In the original
 * (and in the egcs build) they are compiler-generated from the definition of
 * s_allocator above; egcs names them after the first global symbol of the
 * file, pdistd::Allocator::Allocator(int) = __Q26pdistd9Allocatori, and puts
 * them at the end of the object, destructor first.  collect2 listed them in
 * the ctor/dtor tables; src/crtbegin.c names them global_ctor_allocator /
 * global_dtor_allocator, which the egcs build defines as assembler aliases of
 * the generated functions (no code of their own).  The modern build has no
 * collect2-style static init for this object, so there they are written out
 * with the same effect. */

// FUNC 0x01011458 _GLOBAL_$D$__Q26pdistd9Allocatori global_dtor_allocator (compiler-generated static destructor)
// Static destructor of the global Allocator: ~Allocator(&0x01048240, 2)
// (flags 2 = do not delete).  Entry of the crt dtor table at 0x01026CB8.

// FUNC 0x01011480 _GLOBAL_$I$__Q26pdistd9Allocatori global_ctor_allocator (compiler-generated static constructor)
// Static constructor of the global Allocator: Allocator(&0x01048240, 0)
// (best fit).  Entry of the crt ctor table at 0x01026CA0.  Runs before any
// malloc (see the ctor-order note in allocator.h); if it ran after
// heap_init_once it would wipe the heap state (heap_init_once would not run
// again because s_heapBase is already set).

#ifdef BOOT_OLD_GCC
__asm__("\t.globl\tglobal_dtor_allocator\n"
        "\tglobal_dtor_allocator = _GLOBAL_$D$__Q26pdistd9Allocatori\n"
        "\t.globl\tglobal_ctor_allocator\n"
        "\tglobal_ctor_allocator = _GLOBAL_$I$__Q26pdistd9Allocatori\n");
#else
extern "C" void global_dtor_allocator(void)
{
    /* qualified call = direct jal 0x0100F978, not a virtual dispatch */
    theAllocator().pdistd::Allocator::~Allocator();
}

extern "C" void global_ctor_allocator(void)
{
    new (s_allocatorStorage) pdistd::Allocator(0);
}
#endif
