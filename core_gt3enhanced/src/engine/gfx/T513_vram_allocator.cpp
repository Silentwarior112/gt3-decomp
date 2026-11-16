/* T513 - VramAllocator (? name): the GS VRAM allocator g_002F4250.
 *
 * Blocks are on one of three lists: free (merged with free neighbours),
 * used (handed out by alloc / reclaim) and released (freed, but the image
 * stays in VRAM until the space is needed: a later slotFor / reclaim with
 * the same tag gets it back without an upload).  Released blocks go back to
 * the free list oldest first, when an allocation does not fit, or all of
 * them at unlock (flushReleased).  Sizes are rounded up to 0x800 words (a
 * GS page).
 *
 * On the packet path (async) the draws already queued on the VIF1 list may
 * still read a released block, and the uploads go on the GIF list, which
 * runs ahead of it.  So release puts a SIGNAL marker (the block's fence) on
 * the VIF1 list, and before VRAM given back is reused the GIF list WAITs for
 * VIF1 to pass the last of them; the earlier fences are zeroed so that the
 * one WAIT matches one SIGNAL.  Uploads are followed by the opposite pair:
 * the VIF1 list WAITs for the GIF list to pass the end of the upload. */
#include "engine/gfx/vram_internal.h"
#include "engine/gfx/T513_vram_allocator.h"

/* The list operations (inlined everywhere in the binary: the list's address
 * stays in a register, as an inline function's argument). */
static inline void vram_list_clear(VramList *list)
{
    list->head = NULL;
    list->tail = NULL;
}

/* list holds blk alone (blk->list is set by the caller). */
static inline void vram_list_set(VramList *list, VramBlock *blk)
{
    list->head = blk;
    list->tail = blk;
}

static inline void vram_unlink(VramBlock *blk)
{
    VramList *list = blk->list;

    if (list != NULL) {
        if (blk->prev != NULL)
            blk->prev->next = blk->next;
        else
            list->head = blk->next;
        if (blk->next != NULL)
            blk->next->prev = blk->prev;
        else
            list->tail = blk->prev;
        blk->list = NULL;
    }
}

static inline void vram_push_back(VramList *list, VramBlock *blk)
{
    blk->next = NULL;
    blk->prev = list->tail;
    if (list->tail != NULL)
        list->tail->next = blk;
    else
        list->head = blk;
    list->tail = blk;
    blk->list = list;
}

/* blk->prev is set by the callers. */
static inline void vram_push_front(VramList *list, VramBlock *blk)
{
    blk->next = list->head;
    if (list->head != NULL)
        list->head->prev = blk;
    else
        list->tail = blk;
    list->head = blk;
    blk->list = list;
}

// FUNC 0x00243850 VramAllocator::VramAllocator
//   Trivial (init does the work).  Caller: the render context's constructor.
VramAllocator::VramAllocator()
{
}

// FUNC 0x00243858 VramAllocator::~VramAllocator
//   Nothing (term deletes the semaphore); egcs's deleting form calls
//   operator delete when (flags & 1).
VramAllocator::~VramAllocator()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x00243880 VramAllocator::init
//   One free block of the whole VRAM (0x100000 words at 0), the other 63
//   records in the spare pool, an unlocked binary semaphore.
void VramAllocator::init()
{
    SemaParam semaParam;                    /* only maxCount / initCount set */
    int i;

    for (i = 1; i < 63; i++)
        blocks[i].next = &blocks[i + 1];
    blocks[0].list = &freeList;
    spare = &blocks[1];
    blocks[63].next = NULL;
    blocks[0].next = NULL;
    blocks[0].prev = NULL;
    blocks[0].lower = NULL;
    blocks[0].upper = NULL;
    current = NULL;
    vram_list_set(&freeList, &blocks[0]);
    vram_list_clear(&usedList);
    vram_list_clear(&releasedList);
    blocks[0].addr = 0;
    blocks[0].size = VRAM_WORDS;
    maxFree = VRAM_WORDS;
    semaParam.initCount = 1;
    semaParam.maxCount = 1;
    sema = CreateSema(&semaParam);
    locked = 0;
}

// FUNC 0x00243940 VramAllocator::term
void VramAllocator::term()
{
    DeleteSema(sema);
}

// FUNC 0x00243960 VramAllocator::lock
//   Also samples whether a packet is open (async: the packet path).
void VramAllocator::lock()
{
    WaitSema(sema);
    locked = 1;
    async = gfx_packet_is_open() != 0;
}

// FUNC 0x002439A0 VramAllocator::unlock
//   Gives every released block back first.
void VramAllocator::unlock()
{
    flushReleased();
    locked = 0;
    SignalSema(sema);
}

// FUNC 0x002439D0 VramAllocator::alloc
//   A block of size words (rounded up to 0x800) on the used list, tagged.
//   Returns its address, -1 if size is 0 or nothing fits.  The block of the
//   last slotFor is released first.
int VramAllocator::alloc(int size, const void *tag)
{
    int wasLocked;
    VramBlock *blk;

    if (size == 0)
        return -1;
    wasLocked = locked;
    if (!wasLocked)
        lock();
    if (current != NULL) {
        release(current);
        current = NULL;
    }
    blk = take(size);
    if (blk != NULL) {
        blk->prev = NULL;
        blk->tag = tag;
        vram_push_front(&usedList, blk);
    }
    if (!wasLocked)
        unlock();
    if (blk != NULL)
        return blk->addr;
    return -1;
}

// FUNC 0x00243AB0 VramAllocator::dealloc
//   Moves the used block at addr (searched from the oldest) to the released
//   list.  addr < 0: nothing.  Off the packet path the released blocks are
//   given back at once.
void VramAllocator::dealloc(int addr)
{
    int wasLocked;
    VramBlock *blk;

    if (addr < 0)
        return;
    wasLocked = locked;
    if (!wasLocked)
        lock();
    for (blk = usedList.tail; blk != NULL; blk = blk->prev) {
        if (blk->addr == addr) {
            vram_unlink(blk);
            release(blk);
            break;
        }
    }
    if (async == 0)
        flushReleased();
    if (!wasLocked)
        unlock();
}

// FUNC 0x00243B80 VramAllocator::reclaim
//   Takes the released block tagged tag back onto the used list (its image
//   is still there).  Returns its address or -1.  No callers.
int VramAllocator::reclaim(const void *tag)
{
    int wasLocked;
    VramBlock *blk;

    wasLocked = locked;
    if (!wasLocked)
        lock();
    blk = findReleased(tag);
    if (blk != NULL) {
        blk->prev = NULL;
        vram_push_front(&usedList, blk);
    }
    if (!wasLocked)
        unlock();
    if (blk != NULL)
        return blk->addr;
    return -1;
}

// FUNC 0x00243C28 VramAllocator::flushReleased
//   Only while locked: releases the slotFor block, then gives every
//   released block back to the free list.  The fence of each one but the
//   last is zeroed as the next is taken; on the packet path the GIF list
//   then waits for VIF1 to pass the last fence.
void VramAllocator::flushReleased()
{
    u32 *lastFence;
    int prevList;

    if (locked == 0)
        return;
    if (current != NULL) {
        release(current);
        current = NULL;
    }
    if (releasedList.head != NULL) {
        lastFence = NULL;
        do {
            if (lastFence != NULL)
                *lastFence = 0;             /* a later fence covers it */
            lastFence = releasedList.head->fence;
            freeOldestReleased();
        } while (releasedList.head != NULL);
        if (async != 0) {
            prevList = gfx_select_list(PGL_LIST_GIF);
            gfx_put_irq_marker(PGL_MARK_WAIT);
            gfx_select_list(prevList);
        }
    }
}

// FUNC 0x00243CD0 VramAllocator::takeFree
//   First fit from the free list (size rounded up to 0x800 words); the scan
//   recomputes maxFree.  An exact fit is unlinked, otherwise a spare record
//   takes the low part of the block.  NULL when no spare record is left or
//   maxFree < size (compared before the rounding).  The found block is not
//   NULL-checked (maxFree is exact).
VramBlock *VramAllocator::takeFree(int size)
{
    VramBlock *fit;
    VramBlock *blk;
    int left;

    if (spare == NULL)
        return NULL;
    if (maxFree < size)
        return NULL;
    size = (size + VRAM_PAGE_WORDS - 1) & ~(VRAM_PAGE_WORDS - 1);
    fit = NULL;
    maxFree = 0;
    for (blk = freeList.head; blk != NULL; blk = blk->next) {
        if (fit == NULL) {
            left = blk->size - size;
            if (left >= 0) {
                fit = blk;
                if (maxFree < left)
                    maxFree = left;
                continue;
            }
        }
        if (maxFree < blk->size)
            maxFree = blk->size;
    }
    if (fit->size == size) {
        vram_unlink(fit);
        blk = fit;
    } else {
        blk = spare;                        /* the low part of fit, on no list */
        blk->addr = fit->addr;
        spare = blk->next;
        blk->size = size;
        blk->lower = fit->lower;
        blk->upper = fit;
        fit->addr += size;
        fit->size -= size;
        blk->list = NULL;
        if (fit->lower != NULL)
            fit->lower->upper = blk;
        fit->lower = blk;
    }
    blk->fence = NULL;
    blk->tag = NULL;
    blk->loaded = 0;
    return blk;
}

// FUNC 0x00243E08 VramAllocator::take
//   takeFree, giving released blocks back (oldest first, the fences handled
//   as in flushReleased) until it fits or none is left; then on the packet
//   path the GIF list waits for the last fence, if any block was given back.
VramBlock *VramAllocator::take(int size)
{
    VramBlock *blk;
    u32 *lastFence;
    int prevList;

    blk = takeFree(size);
    if (blk == NULL && releasedList.head != NULL) {
        lastFence = NULL;
        do {
            if (lastFence != NULL)
                *lastFence = 0;
            lastFence = releasedList.head->fence;
            freeOldestReleased();
            blk = takeFree(size);
        } while (blk == NULL && releasedList.head != NULL);
        if (async != 0) {
            prevList = gfx_select_list(PGL_LIST_GIF);
            gfx_put_irq_marker(PGL_MARK_WAIT);
            gfx_select_list(prevList);
        }
    }
    return blk;
}

// FUNC 0x00243EC8 VramAllocator::release
//   Appends blk to the released list.  On the packet path its fence is a
//   SIGNAL marker on the VIF1 list, else NULL.
void VramAllocator::release(VramBlock *blk)
{
    if (async != 0) {
        int prevList = gfx_select_list(PGL_LIST_VIF1);

        blk->fence = gfx_put_irq_marker(PGL_MARK_SIGNAL);
        gfx_select_list(prevList);
    } else {
        blk->fence = NULL;
    }
    vram_push_back(&releasedList, blk);
}

// FUNC 0x00243F58 VramAllocator::freeOldestReleased
//   Gives the oldest released block back: merged into a free lower and / or
//   upper neighbour (the absorbed records go to the spare pool), maxFree
//   updated, and appended to the free list unless it already is on it.
//   releasedList.head is not NULL-checked.
void VramAllocator::freeOldestReleased()
{
    VramBlock *blk = releasedList.head;
    VramBlock *lower;
    VramBlock *upper;

    vram_unlink(blk);

    lower = blk->lower;
    if (lower != NULL && lower->list == &freeList) {
        lower->upper = blk->upper;
        lower->size += blk->size;
        if (blk->upper != NULL)
            blk->upper->lower = lower;
        blk->next = spare;
        spare = blk;
        blk = lower;
    }

    upper = blk->upper;
    if (upper != NULL && upper->list == &freeList) {
        upper->size += blk->size;
        upper->lower = blk->lower;
        upper->addr = blk->addr;
        if (blk->lower != NULL)
            blk->lower->upper = upper;
        vram_unlink(blk);
        blk->next = spare;
        spare = blk;
        blk = upper;
    }

    if (maxFree < blk->size)
        maxFree = blk->size;
    if (blk->list == NULL)
        vram_push_back(&freeList, blk);
}

// FUNC 0x002440D0 VramAllocator::findReleased
//   The released block tagged tag (searched from the newest), unlinked, its
//   fence zeroed (its VRAM is not reused, so the GIF list need not wait).
//   NULL if tag is NULL or none.
VramBlock *VramAllocator::findReleased(const void *tag)
{
    VramBlock *blk;

    if (tag == NULL)
        return NULL;
    for (blk = releasedList.tail; blk != NULL; blk = blk->prev) {
        if (blk->tag == tag) {
            if (blk->fence != NULL)
                *blk->fence = 0;
            vram_unlink(blk);
            return blk;
        }
    }
    return NULL;
}

// FUNC 0x00244160 VramAllocator::slotFor
//   Only while locked and on the packet path (else NULL): the block for an
//   image tagged tag, kept as `current` (not on any list) until the next
//   slotFor / alloc / unlock releases it.  The current block if it has the
//   tag, else a released one with the tag (loaded stays set), else a new one
//   (loaded = 0).
VramBlock *VramAllocator::slotFor(int size, const void *tag)
{
    VramBlock *blk;

    if (locked == 0)
        return NULL;
    if (async == 0)
        return NULL;
    if (current != NULL) {
        if (tag != NULL && current->tag == tag)
            return current;
        release(current);
        current = NULL;
    }
    blk = findReleased(tag);
    if (blk != NULL) {
        current = blk;
        return blk;
    }
    blk = take(size);
    if (blk != NULL) {
        blk->tag = tag;
        current = blk;
        blk->loaded = 0;
        return blk;
    }
    return NULL;
}

// FUNC 0x00244228 VramAllocator::loadTex1
//   Makes a Tex1 texture set resident (tag = img): links it for the block
//   and queues its uploads on the GIF list the first time (VIF1 waits for
//   them).  Returns the VRAM address, -1 if img->vramSize is 0 or there is
//   no block.  17 callers.
int VramAllocator::loadTex1(Tex1Image *img)
{
    VramBlock *blk;
    int prevList;

    if (img->vramSize == 0)
        return -1;
    blk = slotFor(img->vramSize << VRAM_BLOCK_SHIFT, img);
    if (blk == NULL)
        return -1;
    if (blk->loaded == 0) {
        blk->loaded = 1;
        tex1_set_vram_base(img, blk->addr);
        prevList = gfx_select_list(PGL_LIST_GIF);
        tex1_upload_packet(img);
        gfx_put_irq_marker(PGL_MARK_SIGNAL);
        gfx_select_list(PGL_LIST_VIF1);
        gfx_put_irq_marker(PGL_MARK_WAIT);
        gfx_select_list(prevList);
        gfx_texflush();
    }
    return blk->addr;
}

// FUNC 0x002442E0 VramAllocator::loadImage
//   Makes a raw w x h image of psm resident (tag = pixels), uploading it on
//   the GIF list the first time, and selects it as the texture (level 0, the
//   width rounded up to the psm's page).  Returns the VRAM address, -1 if
//   tex_vram_size gives 0 or there is no block (tex_vram_size's -1 for an
//   unknown psm is not checked).  Callers: the car icons.
int VramAllocator::loadImage(int psm, int w, int h, const void *pixels)
{
    s32 alignedW;
    int size;
    VramBlock *blk;
    int prevList;

    size = tex_vram_size(psm, w, h, &alignedW);
    if (size == 0)
        return -1;
    blk = slotFor(size, pixels);
    if (blk == NULL)
        return -1;
    if (blk->loaded == 0) {
        blk->loaded = 1;
        prevList = gfx_select_list(PGL_LIST_GIF);
        gs_upload_image(blk->addr, psm, alignedW, 0, 0, w, h, pixels);
        gfx_put_irq_marker(PGL_MARK_SIGNAL);
        gfx_select_list(PGL_LIST_VIF1);
        gfx_put_irq_marker(PGL_MARK_WAIT);
        gfx_select_list(prevList);
        gfx_texflush();
    }
    gfx_set_texture_buffer(0, blk->addr, psm, alignedW);
    gfx_set_texture_size(w, h);
    return blk->addr;
}
