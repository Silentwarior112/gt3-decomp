/* T513_vram_allocator.h - VramAllocator (? name), the GS VRAM allocator
 * (translation unit T513, unit u112), 0x00243850..0x00244400.
 *
 * The one instance is g_002F4230.vram (0x002F4250, see T512_render_context.h;
 * the other units call it "the texture manager").  1M words of VRAM cut into
 * page-aligned (0x800-word) blocks on one of three lists: free (merged with
 * free neighbours), used (handed out by alloc / reclaim) and released (freed,
 * but the image stays in VRAM: a later slotFor / reclaim with the same tag
 * gets it back without an upload).  Released blocks go back to the free list
 * oldest first when an allocation does not fit, or all of them at unlock.
 * A semaphore guards it.  VRAM addresses and sizes are in GS words (TBP0 * 64). */
#ifndef GAME_T513_VRAM_ALLOCATOR_H
#define GAME_T513_VRAM_ALLOCATOR_H

#include "game/game.h"

struct Tex1Image;                               /* G001 (pointers only) */
struct VramList;

/* A VRAM block record. */
struct VramBlock {
    VramList   *list;                           /* 0x00 the list it is on, NULL none */
    VramBlock  *prev;                           /* 0x04 */
    VramBlock  *next;                           /* 0x08 (also the link of the spare pool) */
    VramBlock  *lower;                          /* 0x0C the adjacent block below it in VRAM */
    VramBlock  *upper;                          /* 0x10 the adjacent block above it */
    int         addr;                           /* 0x14 VRAM word address */
    int         size;                           /* 0x18 words (a multiple of 0x800) */
    u32        *fence;                          /* 0x1C the VIF1 SIGNAL marker (gfx_put_irq_marker(0x20000))
                                                   put at release on the packet path: VIF1 is done with
                                                   the VRAM there.  The GIF list waits for the last one
                                                   before it uploads into freed VRAM; the others are
                                                   zeroed (a VIF NOP).  NULL: none */
    const void *tag;                            /* 0x20 who owns the image (a Tex1Image, pixels), NULL none */
    int         loaded;                         /* 0x24 the image has been uploaded */
};
EGCS_SIZE(VramBlock, 0x28);                     /* the stride of VramAllocator::blocks */

struct VramList {
    VramBlock *head;                            /* 0x0 */
    VramBlock *tail;                            /* 0x4 */
};
EGCS_SIZE(VramList, 0x8);

/* Not polymorphic.  Size 0xA30 (RenderContext: vram +0x20, frame +0xA50). */
class VramAllocator {
public:
    VramAllocator();                            /* 0x00243850 nothing (init does the work) */
    ~VramAllocator();                           /* 0x00243858 */
    void init();                                /* 0x00243880 one free block of the whole VRAM, a binary semaphore */
    void term();                                /* 0x00243940 deletes the semaphore */
    void lock();                                /* 0x00243960 */
    void unlock();                              /* 0x002439A0 gives the released blocks back first */
    int  alloc(int size, const void *tag);      /* 0x002439D0 address, -1 if size is 0 or nothing fits */
    void dealloc(int addr);                     /* 0x00243AB0 the used block at addr -> released (addr < 0: nothing) */
    int  reclaim(const void *tag);              /* 0x00243B80 the released block of tag -> used: address or -1 */
    void flushReleased();                       /* 0x00243C28 (while locked) every released block -> free */
    VramBlock *takeFree(int size);              /* 0x00243CD0 first fit from the free list */
    VramBlock *take(int size);                  /* 0x00243E08 takeFree, freeing released blocks until it fits */
    void release(VramBlock *b);                 /* 0x00243EC8 appends b to the released list */
    void freeOldestReleased();                  /* 0x00243F58 */
    VramBlock *findReleased(const void *tag);   /* 0x002440D0 unlinked, NULL if none */
    VramBlock *slotFor(int size, const void *tag);  /* 0x00244160 (locked, packet path) the block for tag's image */
    /* A Tex1 set made resident (tag = img; uploads queued the first time):
     * its VRAM address, -1 if img->vramSize is 0 or there is no block. */
    int  loadTex1(Tex1Image *img);              /* 0x00244228 */
    /* A raw w x h image of psm made resident (tag = pixels) and selected as
     * the texture: its VRAM address or -1. */
    int  loadImage(int psm, int w, int h, const void *pixels);  /* 0x002442E0 */

    VramBlock  blocks[64];                      /* 0x000 [0] the initial (whole VRAM) block, [1..63] the spare pool */
    VramBlock *spare;                           /* 0xA00 unused block records (linked by next) */
    VramList   freeList;                        /* 0xA04 */
    VramList   usedList;                        /* 0xA0C */
    VramList   releasedList;                    /* 0xA14 freed, image still in VRAM; oldest first */
    VramBlock *current;                         /* 0xA1C the block of the last slotFor (on no list) */
    int        sema;                            /* 0xA20 */
    int        maxFree;                         /* 0xA24 the largest free block */
    int        locked;                          /* 0xA28 */
    int        async;                           /* 0xA2C gfx_packet_is_open() when locked: uploads go on the
                                                   GIF packet list, synchronised with VIF1 by markers */
};
EGCS_SIZE(VramAllocator, 0xA30);

#endif /* GAME_T513_VRAM_ALLOCATOR_H */
