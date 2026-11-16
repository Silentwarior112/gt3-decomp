/* mallocr_part2.c - newlib mallocr.c (Doug Lea malloc 2.6.x, reentrant
 * build): rEALLOc = _realloc_r (0x002851F8) and cALLOc = _calloc_r
 * (0x00285768).  _malloc_r (0x00281FE0), _free_r (0x00281248) and the
 * empty __malloc_lock / __malloc_unlock (0x00282878 / 0x00282880) are in
 * unit u138.  This is newlib's own allocator, used by its stdio and the
 * mprec code; the game's malloc/free are Polyphony's (see libc.h).
 *
 * Configuration visible in the code: SIZE_SZ 4, MALLOC_ALIGNMENT 16
 * (request2size: (req + 0x13) & ~0xF, minimum 16 = MINSIZE), no mmap,
 * MORECORE_CLEARS 0 (calloc clears the whole chunk without the lock), the
 * size comparisons are UNSIGNED (sltu) - this version has no (long) casts.
 * MALLOC_COPY / MALLOC_ZERO are the unrolled forms for <= 36 bytes.
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part3_internal.h"

#define SIZE_SZ             4
#define MALLOC_ALIGN_MASK   0xF
#define MINSIZE             16
#define PREV_INUSE          0x1
#define SIZE_BITS           0x3         /* PREV_INUSE | IS_MMAPPED */

#define mem2chunk(mem)              ((mchunkptr)((char *)(mem) - 2 * SIZE_SZ))
#define chunk2mem(p)                ((void *)((char *)(p) + 2 * SIZE_SZ))
#define chunksize(p)                ((p)->size & ~SIZE_BITS)
#define chunk_at_offset(p, s)       ((mchunkptr)(((char *)(p)) + (s)))
#define next_chunk(p)               ((mchunkptr)(((char *)(p)) + ((p)->size & ~PREV_INUSE)))
#define prev_chunk(p)               ((mchunkptr)(((char *)(p)) - ((p)->prev_size)))
#define inuse(p)                    ((((mchunkptr)(((char *)(p)) + ((p)->size & ~PREV_INUSE)))->size) & PREV_INUSE)
#define prev_inuse(p)               ((p)->size & PREV_INUSE)
#define set_head_size(p, s)         ((p)->size = (((p)->size & PREV_INUSE) | (s)))
#define set_head(p, s)              ((p)->size = (s))
#define set_inuse_bit_at_offset(p, s) (((mchunkptr)(((char *)(p)) + (s)))->size |= PREV_INUSE)
#define request2size(req) \
    (((u32)((req) + (SIZE_SZ + MALLOC_ALIGN_MASK)) < (u32)(MINSIZE + MALLOC_ALIGN_MASK)) ? MINSIZE : \
     (((req) + (SIZE_SZ + MALLOC_ALIGN_MASK)) & ~(MALLOC_ALIGN_MASK)))

/* top = bin_at(0)->fd = av_[2]. */
#define top                         (__malloc_av_[2])

/* A plain block, as in Doug Lea's malloc 2.6: a do { } while (0) would put
 * loop notes (scheduling barriers) around the stores. */
#define unlink(P, BK, FD)  \
    {                      \
        BK = (P)->bk;      \
        FD = (P)->fd;      \
        FD->bk = BK;       \
        BK->fd = FD;       \
    }

/* MALLOC_COPY / MALLOC_ZERO: inline word copies up to 36 bytes, else a
 * call of memcpy / memset. */
#define MALLOC_COPY(dest, src, nbytes)                                  \
    do {                                                                \
        u32 mcsz = (nbytes);                                            \
        if (mcsz <= 9 * sizeof(mcsz)) {                                 \
            u32 *mcsrc = (u32 *)(src);                                  \
            u32 *mcdst = (u32 *)(dest);                                 \
            if (mcsz >= 5 * sizeof(mcsz)) {                             \
                *mcdst++ = *mcsrc++;                                    \
                *mcdst++ = *mcsrc++;                                    \
                if (mcsz >= 7 * sizeof(mcsz)) {                         \
                    *mcdst++ = *mcsrc++;                                \
                    *mcdst++ = *mcsrc++;                                \
                    if (mcsz >= 9 * sizeof(mcsz)) {                     \
                        *mcdst++ = *mcsrc++;                            \
                        *mcdst++ = *mcsrc++;                            \
                    }                                                   \
                }                                                       \
            }                                                           \
            *mcdst++ = *mcsrc++;                                        \
            *mcdst++ = *mcsrc++;                                        \
            *mcdst = *mcsrc;                                            \
        } else                                                          \
            memcpy(dest, src, mcsz);                                    \
    } while (0)

#define MALLOC_ZERO(charp, nbytes)                                      \
    do {                                                                \
        u32 mzsz = (nbytes);                                            \
        if (mzsz <= 9 * sizeof(mzsz)) {                                 \
            u32 *mz = (u32 *)(charp);                                   \
            if (mzsz >= 5 * sizeof(mzsz)) {                             \
                *mz++ = 0;                                              \
                *mz++ = 0;                                              \
                if (mzsz >= 7 * sizeof(mzsz)) {                         \
                    *mz++ = 0;                                          \
                    *mz++ = 0;                                          \
                    if (mzsz >= 9 * sizeof(mzsz)) {                     \
                        *mz++ = 0;                                      \
                        *mz++ = 0;                                      \
                    }                                                   \
                }                                                       \
            }                                                           \
            *mz++ = 0;                                                  \
            *mz++ = 0;                                                  \
            *mz = 0;                                                    \
        } else                                                          \
            memset((charp), 0, mzsz);                                   \
    } while (0)

// FUNC 0x002851F8 _realloc_r
//   realloc(NULL, n) = _malloc_r.  Otherwise, under the (empty) malloc lock:
//   if the chunk is too small try, in order, growing into a free next
//   chunk or into top (top only if a MINSIZE remainder is left), then
//   next + previous free chunks, then the previous chunk alone (data moved
//   down with MALLOC_COPY), else _malloc_r + copy + _free_r (no copy when
//   the new chunk is the one right after the old).  Any surplus of at least
//   MINSIZE is split off and handed to _free_r.  Only caller: __submore.
void *_realloc_r(struct _reent *reent_ptr, void *oldmem, size_t bytes)
{
    u32 nb;                     /* padded request size */

    mchunkptr oldp;             /* chunk corresponding to oldmem */
    u32 oldsize;                /* its size */

    mchunkptr newp;             /* chunk to return */
    u32 newsize;                /* its size */
    void *newmem;               /* corresponding user mem */

    mchunkptr next;             /* next contiguous chunk after oldp */
    u32 nextsize;               /* its size */

    mchunkptr prev;             /* previous contiguous chunk before oldp */
    u32 prevsize;               /* its size */

    mchunkptr remainder;        /* holds split off extra space from newp */
    u32 remainder_size;         /* its size */

    mchunkptr bck;              /* misc temp for linking */
    mchunkptr fwd;              /* misc temp for linking */

    /* realloc of null is supposed to be same as malloc */
    if (oldmem == 0)
        return _malloc_r(reent_ptr, bytes);

    __malloc_lock(reent_ptr);                               /* MALLOC_LOCK */

    newp = oldp = mem2chunk(oldmem);
    newsize = oldsize = chunksize(oldp);

    nb = request2size(bytes);

    if (oldsize < nb) {
        /* Try expanding forward */
        next = chunk_at_offset(oldp, oldsize);
        if (next == top || !inuse(next)) {
            nextsize = chunksize(next);

            /* Forward into top only if a remainder */
            if (next == top) {
                if (nextsize + newsize >= nb + MINSIZE) {
                    newsize += nextsize;
                    top = chunk_at_offset(oldp, nb);
                    set_head(top, (newsize - nb) | PREV_INUSE);
                    set_head_size(oldp, nb);
                    __malloc_unlock(reent_ptr);             /* MALLOC_UNLOCK */
                    return chunk2mem(oldp);
                }
            }
            /* Forward into next chunk */
            else if (nextsize + newsize >= nb) {
                unlink(next, bck, fwd);
                newsize += nextsize;
                goto split;
            }
        } else {
            next = 0;
            nextsize = 0;
        }

        /* Try shifting backwards. */
        if (!prev_inuse(oldp)) {
            prev = prev_chunk(oldp);
            prevsize = chunksize(prev);

            /* try forward + backward first to save a later consolidation */
            if (next != 0) {
                /* into top */
                if (next == top) {
                    if (nextsize + prevsize + newsize >= nb + MINSIZE) {
                        unlink(prev, bck, fwd);
                        newp = prev;
                        newsize += prevsize + nextsize;
                        newmem = chunk2mem(newp);
                        MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);    /* implicit: memcpy if > 36 bytes */
                        top = chunk_at_offset(newp, nb);
                        set_head(top, (newsize - nb) | PREV_INUSE);
                        set_head_size(newp, nb);
                        __malloc_unlock(reent_ptr);         /* MALLOC_UNLOCK */
                        return newmem;
                    }
                }
                /* into next chunk */
                else if (nextsize + prevsize + newsize >= nb) {
                    unlink(next, bck, fwd);
                    unlink(prev, bck, fwd);
                    newp = prev;
                    newsize += nextsize + prevsize;
                    newmem = chunk2mem(newp);
                    MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);    /* implicit: memcpy if > 36 bytes */
                    goto split;
                }
            }

            /* backward only */
            if (prev != 0 && prevsize + newsize >= nb) {
                unlink(prev, bck, fwd);
                newp = prev;
                newsize += prevsize;
                newmem = chunk2mem(newp);
                MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);    /* implicit: memcpy if > 36 bytes */
                goto split;
            }
        }

        /* Must allocate */
        newmem = _malloc_r(reent_ptr, bytes);

        if (newmem == 0) {      /* propagate failure */
            __malloc_unlock(reent_ptr);                     /* MALLOC_UNLOCK */
            return 0;
        }

        /* Avoid copy if newp is next chunk after oldp. */
        /* (This can only happen when new chunk is sbrk'ed.) */
        if ((newp = mem2chunk(newmem)) == next_chunk(oldp)) {
            newsize += chunksize(newp);
            newp = oldp;
            goto split;
        }

        /* Otherwise copy, free, and exit */
        MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);    /* implicit: memcpy if > 36 bytes */
        _free_r(reent_ptr, oldmem);
        __malloc_unlock(reent_ptr);                         /* MALLOC_UNLOCK */
        return newmem;
    }

split: /* split off extra room in old or expanded chunk */

    remainder_size = newsize - nb;

    if (remainder_size >= MINSIZE) {    /* split off remainder */
        remainder = chunk_at_offset(newp, nb);
        set_head_size(newp, nb);
        set_head(remainder, remainder_size | PREV_INUSE);
        set_inuse_bit_at_offset(remainder, remainder_size);
        _free_r(reent_ptr, chunk2mem(remainder));  /* let free() deal with it */
    } else {
        set_head_size(newp, newsize);
        set_inuse_bit_at_offset(newp, newsize);
    }

    __malloc_unlock(reent_ptr);                             /* MALLOC_UNLOCK */
    return chunk2mem(newp);
}

// FUNC 0x00285768 _calloc_r
//   _malloc_r(n * elem_size) (32-bit product, no overflow check) and clear
//   the whole chunk (chunksize - SIZE_SZ bytes) with MALLOC_ZERO.  Only
//   caller: _Balloc.
void *_calloc_r(struct _reent *reent_ptr, size_t n, size_t elem_size)
{
    mchunkptr p;
    u32 csz;
    u32 sz = n * elem_size;
    void *mem;

    mem = _malloc_r(reent_ptr, sz);

    if (mem == 0)
        return 0;
    else {
        p = mem2chunk(mem);
        csz = chunksize(p);
        MALLOC_ZERO(mem, csz - SIZE_SZ);         /* implicit: memset if > 36 bytes */
        return mem;
    }
}
