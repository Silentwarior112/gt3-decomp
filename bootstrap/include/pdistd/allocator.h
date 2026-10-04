/* pdistd/allocator.h - Polyphony pdistd::Allocator (allocator.cxx) and the
 * small non-EH pdistd helpers that sit after GZIP in .text.
 *
 * Unit "allocator" of the GT3 US bootstrap (BOOTSTRAP_SCUS-97102):
 *   src/pdistd/allocator.cpp    0x0100F918..0x01010318  pdistd::Allocator methods
 *                               0x01011210..0x010114A8  heap init, malloc family,
 *                                                       heap query wrappers,
 *                                                       global ctor/dtor
 *   src/pdistd/pdistd_misc.cpp  0x01012F38..0x01012FC8  intr_suspend, global lock,
 *                                                       break stubs
 *
 * The heap is a single arena [align16(_end) .. 0x01F00000) managed by one
 * global pdistd::Allocator object at 0x01048240 (.bss).  The arena is carved
 * into blocks, each starting with a 16-byte Rep header; the payload follows
 * the header.  All Reps form an address-ordered doubly-linked chain
 * (prev/next; a block's size is implicit: next - this).  Free blocks are also
 * on a separate doubly-linked free list (prevFree/nextFree, head at
 * Allocator+0x0C); allocated blocks carry prevFree == nextFree == 0xFFFFFFFF.
 */
#ifndef PDISTD_ALLOCATOR_H
#define PDISTD_ALLOCATOR_H

#include "types.h"
#include <stddef.h>

#ifdef __cplusplus

#ifdef BOOT_OLD_GCC
/* allocator.cxx was built with #pragma interface (here) / #pragma
 * implementation (src/pdistd/allocator.cpp): egcs then emits the out-of-line
 * copies of the inline members repValid and reset, and the vtable, only in
 * the implementation file.  The modern compiler does not need (or get) it. */
#pragma interface
#endif

namespace pdistd {

/* pdistd::Allocator - RTTI name "Q26pdistd9Allocator" @0x0103E690.
 *
 * Original (GCC 2.9x-ee) object layout, 0x18 bytes:
 *   +0x00 Rep *m_first     arena start (16-aligned base passed to assign)
 *   +0x04 Rep *m_end       arena end; every Rep lies in [m_first, m_end), the
 *                          last Rep's `next` equals m_end
 *   +0x08 Rep *m_head      first Rep of the address chain.  Normally ==
 *                          m_first; alignRepStart can move the head header up
 *                          by 0x10 (then m_head = m_first + 0x10) and
 *                          rebuildHeadRep moves it back.
 *   +0x0C Rep *m_freeList  head of the free list
 *   +0x10 int  m_firstFit  ctor argument: 0 = best fit (smallest fitting free
 *                          block, ties -> the one later in the free list),
 *                          != 0 = first fit
 *   +0x14 vptr             GCC 2.9x puts the vptr AFTER the data members (the
 *                          class has no polymorphic base).  NOTE: a modern g++
 *                          build puts it at +0x00 instead, shifting the data
 *                          members by 4; the offsets above are the original
 *                          ones and are what the disassembly uses.
 *
 * vtable _vt$Q26pdistd9Allocator @0x0103E680 (.rodata), entries
 * {s16 delta, s16 index, pfn}:
 *   slot 0 (+0x00): {0, 0, __tfQ26pdistd9Allocator 0x010102D8}
 *   slot 1 (+0x08): {0, 0, pdistd::Allocator::~Allocator 0x0100F978}
 *
 * repValid() and reset() are inline members; GCC 2.9x emitted their
 * out-of-line copies next to the vtable at the end of allocator.cxx
 * (0x01010278, 0x010102C0), in the same {repValid, reset, __tf} cluster as the
 * Lupo 2 symbol dump.  repValid is also inlined into free/reallocate.
 */
class Allocator {
public:
    /* Block header, 0x10 bytes; the payload starts right after it. */
    struct Rep {
        Rep *prev;      /* +0x00 previous Rep in address order (0 = chain head)        */
        Rep *next;      /* +0x04 next Rep in address order == end of this block        */
        Rep *prevFree;  /* +0x08 free-list links; both 0xFFFFFFFF while allocated,     */
        Rep *nextFree;  /* +0x0C both 0 terminated (prevFree 0 = free-list head)       */
    };

    Allocator(int firstFit);                            /* 0x0100F918 */
    Allocator(void *base, u32 size, int firstFit);      /* 0x0100F940 */
    virtual ~Allocator();                               /* 0x0100F978 (vtable slot 1) */

    void *assign(void *base, u32 size);                 /* 0x0100FB40 returns previous m_first */
    void *unassign(void);                               /* 0x0100FBA0 returns previous m_first */
    void *allocate(u32 size, u32 align);                /* 0x0100FBB8 */
    void *reallocate(void *ptr, u32 size, u32 align);   /* 0x0100FD10 */
    void  free(void *ptr);                              /* 0x0100FED8 */
    u32   getMaxFreeSize(void);                         /* 0x01010010 */
    u32   getTotalFreeSize(void);                       /* 0x01010078 */
    void  debug(void);                                  /* 0x010100E0 */
    int   countFreeReps(void);                          /* 0x010101D8 */

    /* 0x01010278 - inline (definition below). */
    bool  repValid(Rep *rep);
    /* 0x010102C0 - inline (definition below). */
    void  reset(void);

private:
    Rep  *alignRepStart(Rep *rep, u32 align);           /* 0x0100F9B8 */
    void  rebuildHeadRep(Rep *rep);                     /* 0x0100FA78 */
    Rep  *insertRepAt(Rep *rep, u32 offset);            /* 0x0100FAA8 */
    void  unlinkRep(Rep *rep, Rep *next);               /* 0x0100FAD8 */
    void  pushFreeRep(Rep *rep);                        /* 0x0100FAF8 */
    void  removeFreeRep(Rep *rep);                      /* 0x0100FB18 */

    Rep  *m_first;      /* +0x00 */
    Rep  *m_end;        /* +0x04 */
    Rep  *m_head;       /* +0x08 */
    Rep  *m_freeList;   /* +0x0C */
    int   m_firstFit;   /* +0x10 */
    /*    vptr             +0x14 (GCC 2.9x) */
};

/* Marker stored in Rep::prevFree / Rep::nextFree of an allocated block. */
#define PDISTD_REP_ALLOCATED ((pdistd::Allocator::Rep *)0xFFFFFFFF)

/* pdistd::Allocator::repValid (out-of-line copy 0x01010278, FUNC marker in
 * src/pdistd/allocator.cpp)
 * True if rep lies inside [m_first, m_end) (unsigned compares) and is marked
 * allocated (prevFree == nextFree == 0xFFFFFFFF).  Out-of-line copy is
 * unreferenced; free() and reallocate() inline it to validate the caller's
 * pointer (ptr - 0x10). */
inline bool Allocator::repValid(Rep *rep)
{
    if (rep < m_first)
        return false;
    if (!(rep < m_end))
        return false;
    if (rep->prevFree != PDISTD_REP_ALLOCATED)
        return false;
    return rep->nextFree == PDISTD_REP_ALLOCATED;   /* nor + sltiu 1 */
}

/* pdistd::Allocator::reset (out-of-line copy 0x010102C0, FUNC marker in
 * src/pdistd/allocator.cpp)
 * Forgets the arena: zeroes first, end, head and the free-list head (the
 * binary's store order is freeList, first, end, head: egcs issues the last of
 * these independent stores first).  The out-of-line copy is unreferenced;
 * unassign() inlines it. */
inline void Allocator::reset(void)
{
    m_first    = 0;
    m_end      = 0;
    m_head     = 0;
    m_freeList = 0;
}

} /* namespace pdistd */

static_assert(sizeof(pdistd::Allocator::Rep) == 0x10, "Allocator::Rep");
static_assert(sizeof(pdistd::Allocator) == 0x18, "pdistd::Allocator");

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* allocator.cpp - C front-ends over the global Allocator @0x01048240       */
/* ------------------------------------------------------------------------ */
/* malloc / free / realloc / calloc are declared in libc.h; memalign too.    */

/* Unreferenced in this binary (only their .data metadata records exist).    */
unsigned int heap_max_free_size(void);      /* 0x010112A0 Allocator::getMaxFreeSize   */
unsigned int heap_total_free_size(void);    /* 0x010112C0 Allocator::getTotalFreeSize */
void         heap_dump(void);               /* 0x010112E0 Allocator::debug            */
int          heap_free_block_count(void);   /* 0x01011300 Allocator::countFreeReps    */

/* Static constructor / destructor of the global Allocator object.  Listed in
 * the crt ctor table at 0x01026C98 {3, 0x01003CC0, 0x01011480, 0x010000C8}
 * and dtor table at 0x01026CB0 {3, 0x01003C90, 0x01011458, 0x01000108};
 * __do_global_ctors (0x01017030) calls the ctor table from the last entry
 * backwards, so this ctor runs before the filename.cxx statics that may
 * already malloc.  Compiler-generated in the original; modern build: written
 * out in src/pdistd/allocator.cpp; egcs build: assembler aliases of the
 * generated _GLOBAL_$I$__Q26pdistd9Allocatori / _GLOBAL_$D$__Q26pdistd9Allocatori. */
void global_ctor_allocator(void);           /* 0x01011480 _GLOBAL_$I$ */
void global_dtor_allocator(void);           /* 0x01011458 _GLOBAL_$D$ */

/* ------------------------------------------------------------------------ */
/* pdistd_misc.cpp                                                          */
/* ------------------------------------------------------------------------ */

/* Disable interrupts (R5900 di-errata loop) and return the previous
 * Status.EIE bit (1 = interrupts were enabled).  Callers re-enable with a
 * bare `ei` only when it returned non-zero.                                 */
int  intr_suspend(void);                    /* 0x01012F38 */

/* Lazily created binary semaphore (.sdata 0x01041E60, init -1) serialising
 * the "host:" fileio wrappers (hostfs_io unit, host_open..host_close).
 * global_lock_acquire returns WaitSema's result (no caller uses it).       */
int  global_lock_acquire(void);             /* 0x01012F68 */
void global_lock_release(void);             /* 0x01012FA0 */

/* `break; return 0` trap stubs.  Pointers to them sit in .sdata at
 * 0x01041E48 / 0x01041E4C (g_break_hook_0 / g_break_hook_1).  break_stub_1
 * is also what abort() (0x0101AFF0) calls with 6 = SIGABRT, i.e. it plays the
 * part of newlib's raise(); it is declared in libc.h too.                   */
int  break_stub_0(void);                    /* 0x01012FA8 */
int  break_stub_1(int code);                /* 0x01012FB8 */

extern int (*g_break_hook_0)(void);         /* 0x01041E48 (.sdata) = break_stub_0 */
extern int (*g_break_hook_1)(int);          /* 0x01041E4C (.sdata) = break_stub_1 */

#ifdef __cplusplus
}
#endif

#endif /* PDISTD_ALLOCATOR_H */
