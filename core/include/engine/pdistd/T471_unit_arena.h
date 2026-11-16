/* T471_unit_arena.h - _UnitArenaBase: a pool of fixed-size units with a free
 * list (translation unit T471, unit u104).
 *
 * The free list is built by the derived UnitArena<N> (slot 1): units of
 * unitSize rounded up to N bytes from the N-byte aligned start of the
 * buffer, as many as fit; each free unit's first word links the next.
 * UnitArena<64>: the course arena (T153, one 0xD00000-byte unit) and the car
 * models' arena (T180, 0xA8000-byte units).
 */
#ifndef GAME_T471_UNIT_ARENA_H
#define GAME_T471_UNIT_ARENA_H

#include "game/game.h"

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function and its storage itself (and rejects
 * hand declarations). */
extern "C" {
void *__tf14_UnitArenaBase(void);           /* 0x0022CB18 */
/* {vptr, name}; the base node of UnitArena<64>'s (T153) (u050 / u060: g_00353470) */
extern u32 __ti14_UnitArenaBase[2];         /* 0x00353470 gp-0x4580 __user_type_info */
}
#endif

/* vtable 0x003416C8 (2 slots), typeinfo __tf14_UnitArenaBase 0x0022CB18
 * (__user_type_info 0x00353470).  0x18 bytes of data, egcs vptr +0x18; 0x1C
 * bytes. */
class _UnitArenaBase {
public:
    _UnitArenaBase();                                           /* 0x0022C9B0 all 0 */
    _UnitArenaBase(void *buffer, int size, int unitSize);       /* 0x0022C9E0 no free list yet */
    /* reset() (forgets the buffer). */
    virtual ~_UnitArenaBase();                                  /* [0] +0x0C 0x0022C960 */
    /* Builds the free list over the buffer; sets used = 0 and nfree. */
    virtual void buildFreeList() = 0;                           /* [1] +0x14 __pure_virtual (virtual_1) */

    /* Forgets the buffer (used / nfree are kept); returns it. */
    void *reset();                                              /* 0x0022CA10 */
    /* Takes a new buffer and has the free list built; returns the old one. */
    void *init(void *buffer, int size, int unitSize);           /* 0x0022CA28 */
    /* The first free unit (no check for an empty list); the argument is
     * not used. */
    void *alloc(int unused);                                    /* 0x0022CAA8 */
    void  release(void *unit);                                  /* 0x0022CAD8 */
    int   getUnitSize();                                        /* 0x0022CB00 (getUnk08) */
    int   getUsed();                                            /* 0x0022CB08 */
    int   getFree();                                            /* 0x0022CB10 */

    void  *buffer;                              /* 0x00 */
    int    size;                                /* 0x04 bytes of the buffer */
    int    unitSize;                            /* 0x08 bytes of a unit (before the rounding) */
    void **freeList;                            /* 0x0C */
    int    used;                                /* 0x10 units taken */
    int    nfree;                               /* 0x14 units left */
    /*  vptr                                       0x18 */
};
EGCS_SIZE(_UnitArenaBase, 0x1C);

#endif /* GAME_T471_UNIT_ARENA_H */
