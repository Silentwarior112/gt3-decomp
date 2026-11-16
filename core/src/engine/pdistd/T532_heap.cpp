/* T532_heap.cpp - the global heap: one pdistd::Allocator (heap_object,
 * 0x004A9EC0) over [align16(&_end) .. 0x01F00000), and the C malloc family
 * over it.  Declarations: include/engine/pdistd/T532_heap.h (calloc in libc.h, the
 * heap_* query wrappers in pdistd/allocator.h).
 *
 * Same machine code as the second half of the bootstrap's
 * src/pdistd/allocator.cpp (0x01011210..0x010114A8); the source below is that
 * file's, with the core addresses.  The global object is defined here, so
 * egcs generates its _GLOBAL_$I$ / _GLOBAL_$D$ functions as in the original.
 *
 * The file's data: .sdata 0x003531C0..0x003531D8 (the two break hooks, then
 * the heap words heap_base 0x003531C8 (0 until heap_init_once), heap_size
 * 0x003531CC, heap_start 0x003531D0 = &_end = 0x004B2C0C, heap_limit
 * 0x003531D4 = 0x01F00000), .bss heap_object 0x004A9EC0.
 */
// DATA_START 0x003531C0
#include "engine/pdistd/pdistd_internal.h"
#include "engine/pdistd/T532_heap.h"

extern "C" u8 _end[];   /* linker symbol: the end of .bss, 0x004B2C0C */

/* The two .sdata pointers to the `break' stubs of pdistd_misc.c (u127),
 * the bootstrap's g_break_hook_0 / g_break_hook_1 (declared in
 * pdistd/allocator.h).  No code reads them.  They are this file's: in every
 * build they sit right in front of the heap words (US core 0x003531C0, EU
 * core 0x00354B60, US / EU bootstraps 0x01041E48 / 0x010417C8, Lupo 2
 * 0x0035E9D8, GT Concept 0x00337100), while PrintFormat's "(null)", which
 * precedes them here and in the bootstraps, is elsewhere in Lupo 2 and GT
 * Concept (after pdistd_misc's "%s"). */
int (*g_break_hook_0)(void) = break_stub_0;     /* 0x003531C0 */
int (*g_break_hook_1)(int)  = break_stub_1;     /* 0x003531C4 */

/* The heap words (.sdata, $gp-relative; file statics as in the bootstrap's
 * allocator.cpp: only this file uses them). */
#define HEAP_TOP ((void *)0x01F00000)           /* the top of the arena (31 MB) */
static void *heap_base  = 0;                    /* 0x003531C8 0 until heap_init_once */
static u32   heap_size  = 0;                    /* 0x003531CC */
static void *heap_start = _end;                 /* 0x003531D0 = &_end = 0x004B2C0C */
static void *heap_limit = HEAP_TOP;             /* 0x003531D4 */

/* The global Allocator (best fit), an object with a constructor and a
 * destructor: egcs builds and destroys it with the functions at the end of
 * the unit.  A file static (the bootstrap's s_allocator): retail has it in
 * .bss (0x004A9EC0), where egcs puts a static object; a global one would be
 * .data at its definition. */
static pdistd::Allocator heap_object(0);        /* 0x004A9EC0 */

// FUNC 0x0026C228 heap_init_once
// Under intr_suspend: on the first call (base == 0) computes
// base = align16(&_end), size = (0x01F00000 - base) & ~0xF, zero-fills the
// whole arena with memset, hands it to the global Allocator
// (assign(base, size)) and records base/size.  Later calls do nothing.
// Called at the top of calloc, malloc, realloc and memalign (not free).
static void heap_init_once(void)
{
    int intr = intr_suspend();

    if (heap_base == 0) {
        /* signed int arithmetic: both masks are the immediate -16 */
        u8 *base = (u8 *)(((int)heap_start + 0xF) & ~0xF);
        u32 size = ((int)heap_limit - (int)base) & ~0xF;

        memset(base, 0, size);
        heap_object.assign(base, size);
        heap_base = base;
        heap_size = size;
    }

    if (intr)
        EI();
}

// FUNC 0x0026C2B8 heap_max_free_size
// global Allocator .getMaxFreeSize().  No heap_init_once.  Unreferenced.
extern "C" unsigned int heap_max_free_size(void)
{
    return heap_object.getMaxFreeSize();
}

// FUNC 0x0026C2D8 heap_total_free_size
// global Allocator .getTotalFreeSize().  No heap_init_once.  Unreferenced.
extern "C" unsigned int heap_total_free_size(void)
{
    return heap_object.getTotalFreeSize();
}

// FUNC 0x0026C2F8 heap_dump
// global Allocator .debug().  No heap_init_once.  Unreferenced.
extern "C" void heap_dump(void)
{
    heap_object.debug();
}

// FUNC 0x0026C318 heap_free_block_count
// global Allocator .countFreeReps().  No heap_init_once.  Unreferenced.
extern "C" int heap_free_block_count(void)
{
    return heap_object.countFreeReps();
}

// FUNC 0x0026C338 calloc
// heap_init_once, then allocate(nmemb * size, 0x10) (low 32 bits of a
// signed `mult`; no overflow check).  Quirk: the block is NOT zeroed - only
// memory never handed out before is zero (the arena was memset once by
// heap_init_once).  Unreferenced.
extern "C" void *calloc(size_t nmemb, size_t size)
{
    heap_init_once();
    return heap_object.allocate((u32)(nmemb * size), 0x10);
}

// FUNC 0x0026C380 free
// if (ptr != NULL) global Allocator .free(ptr).  No heap_init_once.
// 72 callers (operator delete / delete[], the EH runtime, game code).
extern "C" void free(void *ptr)
{
    if (ptr != 0)
        heap_object.free(ptr);
}

// FUNC 0x0026C3A8 malloc
// heap_init_once, then global Allocator .allocate(size, 0x10).  22 callers,
// including operator new (0x0027C138).
extern "C" void *malloc(size_t size)
{
    heap_init_once();
    return heap_object.allocate(size, 0x10);
}

// FUNC 0x0026C3E0 realloc
// heap_init_once, then global Allocator .reallocate(ptr, size, 0x10).
// 2 callers.
extern "C" void *realloc(void *ptr, size_t size)
{
    heap_init_once();
    return heap_object.reallocate(ptr, size, 0x10);
}

// FUNC 0x0026C428 memalign
// heap_init_once, then global Allocator .allocate(size, align) (align < 16
// is raised to 16 by allocate).  36 callers.
extern "C" void *memalign(size_t align, size_t size)
{
    heap_init_once();
    return heap_object.allocate(size, align);
}

/* Compiler-generated: egcs's static destructor and constructor of
 * heap_object (named after the unit's first public object, g_break_hook_0),
 * emitted at the end of the unit, the destructor first (the bootstrap's
 * names for them: global_dtor_allocator / global_ctor_allocator). */
// FUNC 0x0026C470 global_dtor_allocator (_GLOBAL_$D$g_break_hook_0)
//   (.dtors) ~Allocator(&heap_object, 2) (flags 2 = do not delete), a
//   direct call of 0x0026A828.
/* implicit: ~Allocator */
// FUNC 0x0026C498 global_ctor_allocator (_GLOBAL_$I$g_break_hook_0)
//   (.ctors) Allocator(&heap_object, 0) (best fit).  Must run before the
//   first malloc (heap_init_once would not run again after this wiped the
//   state).
/* implicit: Allocator(0) */
