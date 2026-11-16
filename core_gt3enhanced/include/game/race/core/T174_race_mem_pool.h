/* T174_race_mem_pool.h - RaceMemPool, the replay memory of a race
 * (translation unit T174, unit u058), 0x001A6138..0x001A6318.
 *
 * A pdistd::Allocator (best fit) over a block the race gives it, and the
 * buffers it takes from it, 16-aligned: per car of the pair a control
 * record buffer of g_003529B4 bytes (T318's size, declared there) and 40
 * SectorRecords (0x43800 bytes, T313), a 0x20000-byte 64-aligned buffer (the logger reads
 * its course map file into it: u018 / u038 / u039), and the largest block
 * left (the logger's menu heap is pushed over it).  The static instance is
 * g_0049C378 (RaceBase::virtual_4, T129); RaceSolitaire holds one (T139,
 * +0x217AC; virtual_57 assigns it 0x1F8000 bytes).
 */
#ifndef GAME_T174_RACE_MEM_POOL_H
#define GAME_T174_RACE_MEM_POOL_H

#include "game/game.h"
#include "engine/pdistd/T530_allocator.h"            /* pdistd::Allocator (held by value) */

class OneLapGhostInfo;                         /* T313 (pointers only: forward declarations) */
class LoggerCourseMap;                      /* T088 */

/* The buffers of one car of the pair, 8 bytes (u058's anonymous struct;
 * u038 cars[2] {unk0, sectors}). */
struct RaceMemPoolBufs {
    void         *small;                    /* 0x0 g_003529B4 bytes */
    OneLapGhostInfo *sectors;                  /* 0x4 40 x 0x1B00 bytes (u058 big) */
};
EGCS_SIZE(RaceMemPoolBufs, 0x8);

/* u047's name.  Not polymorphic, 0x38 bytes. */
class RaceMemPool {
public:
    /* Allocator(0) (best fit), the buffer pointers and lastSector cleared.
     * Callers: T129 (g_0049C378's static constructor), T139. */
    RaceMemPool();                          /* 0x001A6138 */
    /* ~Allocator (egcs flags 2), then operator delete with flags bit 0. */
    ~RaceMemPool();                         /* 0x001A61A8 */
    /* Assigns [base, base + size) and takes the buffers (see above): heap
     * is the largest free block, heapSize its size.  Caller: RaceSolitaire::
     * virtual_57. */
    void F_001A61F0(void *base, u32 size);  /* 0x001A61F0 */
    /* The word +0x08 (OneLapGhostInfo::length) of the 40 records of both cars
     * cleared, the last first.  Caller: RaceSolitaire::virtual_56. */
    void F_001A62B0();                      /* 0x001A62B0 */

    pdistd::Allocator allocator;            /* 0x00 (u047 u8[0x18]) */
    RaceMemPoolBufs   bufs[2];              /* 0x18 */
    int               lastSector;           /* 0x28 (? u038: the sector count - 1; u058 f28) */
    LoggerCourseMap  *courseMap;            /* 0x2C 0x20000 bytes, 64-aligned: the logger's course map file
                                               (logger_course_map, T087; u018 work; u058 f2C) */
    void             *heap;                 /* 0x30 the rest (u018 / u039; u058 f30) */
    u32               heapSize;             /* 0x34 its size (u018 / u039 int heapSize; u058 f34) */
};
EGCS_SIZE(RaceMemPool, 0x38);

/* g_003529B4, the size of one control record buffer (two of it are taken
 * per car), is T318's data and declared in T318_control_record.h. */

#endif /* GAME_T174_RACE_MEM_POOL_H */
