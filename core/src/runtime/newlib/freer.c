/* u138 / G007: newlib freer.o, i.e. mallocr.c compiled with DEFINE_FREE
 * (Doug Lea's malloc 2.6.x, the REENT build): _free_r and _malloc_trim_r.
 * The allocator's common macros are mallocr.h; its data (the bins
 * __malloc_av_ and the tunables) is mallocr.o's, mallocr_part1.c.  The
 * object has no data of its own. */
#include "mallocr.h"

/* ------------------------------------------------------------------------ */
/* free                                                                     */
/* ------------------------------------------------------------------------ */

// FUNC 0x00281248 _free_r
void _free_r(struct _reent *reent_ptr, void *mem)
{
    mchunkptr p;        /* chunk corresponding to mem */
    u32 hd;             /* its head field */
    u32 sz;             /* its size */
    int idx;            /* its bin index */
    mchunkptr next;     /* next contiguous chunk */
    u32 nextsz;         /* its size */
    u32 prevsz;         /* size of previous contiguous chunk */
    mchunkptr bck;      /* misc temp for linking */
    mchunkptr fwd;      /* misc temp for linking */
    int islr;           /* track whether merging with last_remainder */

    if (mem == NULL)    /* free(0) has no effect */
        return;

    __malloc_lock(reent_ptr);

    p = mem2chunk(mem);
    hd = p->size;

    sz = hd & ~PREV_INUSE;
    next = chunk_at_offset(p, sz);
    nextsz = chunksize(next);

    if (next == top) {  /* merge with top */
        sz += nextsz;

        if (!(hd & PREV_INUSE)) {   /* consolidate backward */
            prevsz = p->prev_size;
            p = chunk_at_offset(p, -(s32)prevsz);
            sz += prevsz;
            unlink(p, bck, fwd);
        }

        set_head(p, sz | PREV_INUSE);
        top = p;
        if ((u64)sz >= (u64)trim_threshold)
            _malloc_trim_r(reent_ptr, (u32)top_pad);
        __malloc_unlock(reent_ptr);
        return;
    }

    set_head(next, nextsz);     /* clear inuse bit */

    islr = 0;

    if (!(hd & PREV_INUSE)) {   /* consolidate backward */
        prevsz = p->prev_size;
        p = chunk_at_offset(p, -(s32)prevsz);
        sz += prevsz;

        if (p->fd == last_remainder)    /* keep as last_remainder */
            islr = 1;
        else
            unlink(p, bck, fwd);
    }

    if (!(inuse_bit_at_offset(next, nextsz))) {     /* consolidate forward */
        sz += nextsz;

        if (!islr && next->fd == last_remainder) {  /* re-insert last_remainder */
            islr = 1;
            link_last_remainder(p);
        } else
            unlink(next, bck, fwd);
    }

    set_head(p, sz | PREV_INUSE);
    set_foot(p, sz);
    if (!islr)
        frontlink(p, sz, idx, bck, fwd);

    __malloc_unlock(reent_ptr);
}

/* ------------------------------------------------------------------------ */
/* malloc_trim                                                              */
/* ------------------------------------------------------------------------ */

// FUNC 0x00281568 _malloc_trim_r
int _malloc_trim_r(struct _reent *reent_ptr, size_t pad)
{
    s64 top_size;       /* Amount of top-most memory */
    s64 extra;          /* Amount to release */
    char *current_brk;  /* address returned by pre-check sbrk call */
    char *new_brk;      /* address returned by negative sbrk call */

    u64 pagesz = malloc_getpagesize;

    __malloc_lock(reent_ptr);

    top_size = chunksize(top);
    /* implicit: unsigned long arithmetic (pagesz): __udivdi3, then __muldi3 */
    extra = ((top_size - pad - MINSIZE + (pagesz - 1)) / pagesz - 1) * pagesz;

    if (extra < (s64)pagesz) {  /* Not enough memory to release */
        __malloc_unlock(reent_ptr);
        return 0;
    }

    /* Test to make sure no one else called sbrk */
    current_brk = (char *)(_sbrk_r(reent_ptr, 0));
    if (current_brk != (char *)(top) + top_size) {
        __malloc_unlock(reent_ptr);
        return 0;   /* Apparently we don't own memory; must fail */
    }

    new_brk = (char *)(_sbrk_r(reent_ptr, -(s32)extra));

    if (new_brk == MORECORE_FAILURE) {  /* sbrk failed? */
        /* Try to figure out what we have */
        current_brk = (char *)(_sbrk_r(reent_ptr, 0));
        top_size = current_brk - (char *)top;
        if (top_size >= (s64)MINSIZE) { /* if not, we are very very dead! */
            sbrked_mem = current_brk - sbrk_base;
            set_head(top, top_size | PREV_INUSE);
        }
        __malloc_unlock(reent_ptr);
        return 0;
    }

    /* Success. Adjust top accordingly. */
    set_head(top, (top_size - extra) | PREV_INUSE);
    sbrked_mem -= extra;
    __malloc_unlock(reent_ptr);
    return 1;
}
