/* T174_race_mem_pool.cpp - RaceMemPool, the replay memory of a race
 * (0x001A6138..0x001A6318): a pdistd::Allocator over a block given by the
 * race, per car of the pair a control record buffer (g_003529B4 bytes) and
 * 40 SectorRecords, the logger's 0x20000-byte course map buffer (64-aligned)
 * and all the rest.  The static instance is g_0049C378 (T129). */
/* Retail's .rodata (0x00330870..0x0033087A, right after T173's tables): only
 * <new>'s "bad_alloc" (its inline what(), written when the header is read),
 * unreferenced: the anchor below. */
// DATA_START 0x00330870
#include <new>                              /* retail's .rodata: only its "bad_alloc" */
#include "game/race/pause/replay_pause_internal.h"
#include "libc.h"                           /* memset */
#include "game/race/dynamics/T318_control_record.h"       /* g_003529B4, the control record buffer size */
#include "game/race/dynamics/T313_dc_free_run.h"          /* OneLapGhostInfo (the sector records) */
#include "game/race/core/T174_race_mem_pool.h"

/* The data logger's sectors per car (OneLapGhostInfo, 0x1B00 bytes each). */
enum { RACE_MEM_POOL_SECTORS = 40 };

// FUNC 0x001A6138 RaceMemPool::RaceMemPool
//   Allocator(0) (best fit), the buffer pointers and lastSector cleared
//   (memset).  (EH: ~Allocator.)  Callers: race_mem_pool_static_ctor
//   (F_0017CDD8), RaceSolitaire::RaceSolitaire (F_00186E10).
RaceMemPool::RaceMemPool()
    : allocator(0)
{
    /* implicit: Allocator (member constructor), ~Allocator (exception cleanup) */
    memset(bufs, 0, sizeof(bufs));
    lastSector = 0;
}

// FUNC 0x001A61A8 RaceMemPool::~RaceMemPool
//   ~Allocator (egcs flags 2), then operator delete with flags bit 0.
//   Callers: race_mem_pool_static_dtor (F_0017CDB0), RaceSolitaire::
//   RaceSolitaire (F_00186E10, EH), RaceSolitaire::virtual_0.
RaceMemPool::~RaceMemPool()
{
    /* implicit: ~Allocator (member destructor), operator delete (deleting destructor) */
}

// FUNC 0x001A61F0 RaceMemPool::F_001A61F0
//   Assigns [base, base + size) (the arguments go straight through to
//   Allocator::assign) and takes (allocate), 16-aligned, the control record
//   buffer and the sector records of both cars, the course map buffer
//   (64-aligned), then the largest free block (getMaxFreeSize) as the heap.
//   Caller: RaceSolitaire::virtual_57.
void RaceMemPool::F_001A61F0(void *base, u32 size)
{
    int car;

    allocator.assign(base, size);
    for (car = 0; car < 2; car++) {
        bufs[car].small = allocator.allocate(g_003529B4, 0x10);
        bufs[car].sectors = (OneLapGhostInfo *)allocator.allocate(
            RACE_MEM_POOL_SECTORS * sizeof(OneLapGhostInfo), 0x10);
    }
    courseMap = (LoggerCourseMap *)allocator.allocate(0x20000, 0x40);
    heapSize = allocator.getMaxFreeSize();
    heap = allocator.allocate(heapSize, 0x10);
}

// FUNC 0x001A62B0 RaceMemPool::F_001A62B0
//   The length of each of the 40 sector records of both cars cleared, the
//   last first.  Caller: RaceSolitaire::virtual_56.
void RaceMemPool::F_001A62B0()
{
    int car, sector;

    for (car = 0; car < 2; car++) {
        for (sector = RACE_MEM_POOL_SECTORS - 1; sector >= 0; sector--)
            bufs[car].sectors[sector].length = 0;
    }
}
