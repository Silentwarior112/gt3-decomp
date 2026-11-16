/* u138 / G007: the common part of newlib's mallocr.c (Doug Lea's malloc
 * 2.6.x, the REENT build with MALLOC_ALIGNMENT 16).  newlib compiles the one
 * source into an object per entry point (DEFINE_FREE: freer.o with free and
 * malloc_trim; DEFINE_MALLOC: mallocr.o with malloc, malloc_extend_top and
 * the allocator's data; DEFINE_REALLOC, DEFINE_CALLOC, ...), and the link has
 * them apart: freer.o at 0x00281248 (before fvwrite.o / fwalk.o / locale.o /
 * makebuf.o), mallocr.o at 0x00281D88 (its .data at 0x003075A8, after
 * locale.o's).  freer.c and mallocr_part1.c are those two
 * objects; both include this header (u139's realloc / calloc have their own
 * copy of what they use).
 *
 * INTERNAL_SIZE_T is size_t (u32); `long' / `unsigned long' are 64-bit, so
 * remainder sizes go through newlib's long_sub_size_t() (negative when
 * size < nb, instead of a huge zero-extended difference), and the binblock
 * bit vector arithmetic is done in 64 bits and truncated into the 32-bit
 * size field of bin_at(0).  The debug check_* macros are empty. */
#ifndef G007_MALLOCR_U138_H
#define G007_MALLOCR_U138_H

#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part2_internal.h"

typedef struct malloc_chunk *mchunkptr;
typedef struct malloc_chunk *mbinptr;

#define SIZE_SZ             4
#define MALLOC_ALIGNMENT    16
#define MALLOC_ALIGN_MASK   (MALLOC_ALIGNMENT - 1)
#define MINSIZE             16
#define PREV_INUSE          0x1
#define SIZE_BITS           0x3
#define malloc_getpagesize  4096
#define MORECORE_FAILURE    ((char *)-1)
#define DEFAULT_TRIM_THRESHOLD  (128 * 1024)
#define DEFAULT_TOP_PAD         (0)
/* newlib's MORECORE(n) = _sbrk_r(reent_ptr, n) and MALLOC_LOCK / MALLOC_UNLOCK
 * = __malloc_lock / __malloc_unlock(reent_ptr) are written out in the code. */

/* INTERNAL_NEWLIB: the statics under their link names (defined in
 * mallocr_part1.c, mallocr.o's .data) */
#define trim_threshold      __malloc_trim_threshold
#define top_pad             __malloc_top_pad
#define sbrk_base           __malloc_sbrk_base
#define max_sbrked_mem      __malloc_max_sbrked_mem
#define max_total_mem       __malloc_max_total_mem
#define current_mallinfo    __malloc_current_mallinfo
#define sbrked_mem          (current_mallinfo.arena)

#define long_sub_size_t(x, y) \
    (sizeof(s64) > sizeof(u32) && (x) < (y) ? -(s64)((y) - (x)) : (s64)((x) - (y)))

/* pad request bytes into a usable size */
#define request2size(req) \
    (((s64)(u32)((req) + (SIZE_SZ + MALLOC_ALIGN_MASK)) < \
      (s64)(MINSIZE + MALLOC_ALIGN_MASK)) ? MINSIZE : \
     (((req) + (SIZE_SZ + MALLOC_ALIGN_MASK)) & ~(MALLOC_ALIGN_MASK)))

#define chunk2mem(p)    ((void *)((char *)(p) + 2 * SIZE_SZ))
#define mem2chunk(mem)  ((mchunkptr)((char *)(mem) - 2 * SIZE_SZ))
#define chunksize(p)    ((p)->size & ~(SIZE_BITS))
#define chunk_at_offset(p, s) ((mchunkptr)(((char *)(p)) + (s)))
#define inuse_bit_at_offset(p, s) \
    (((mchunkptr)(((char *)(p)) + (s)))->size & PREV_INUSE)
#define set_inuse_bit_at_offset(p, s) \
    (((mchunkptr)(((char *)(p)) + (s)))->size |= PREV_INUSE)
#define set_head_size(p, s) ((p)->size = (((p)->size & PREV_INUSE) | (s)))
#define set_head(p, s)      ((p)->size = (s))
#define set_foot(p, s)      (((mchunkptr)((char *)(p) + (s)))->prev_size = (s))

/* the bins: av_ at 0x003075A8 */
#define av_             __malloc_av_
#define bin_at(i)       ((mbinptr)((char *)&(av_[2 * (i) + 2]) - 2 * SIZE_SZ))
#define next_bin(b)     ((mbinptr)((char *)(b) + 2 * sizeof(mbinptr)))
#define prev_bin(b)     ((mbinptr)((char *)(b) - 2 * sizeof(mbinptr)))
#define top             (bin_at(0)->fd)     /* 0x003075B0 */
#define last_remainder  (bin_at(1))         /* the bin header at 0x003075B0 */
#define initial_top     ((mchunkptr)(bin_at(0)))
#define first(b)        ((b)->fd)
#define last(b)         ((b)->bk)

#define NAV                 128
#define MAX_SMALLBIN        63
#define MAX_SMALLBIN_SIZE   512
#define SMALLBIN_WIDTH      8
#define is_small_request(nb) ((nb) < MAX_SMALLBIN_SIZE - SMALLBIN_WIDTH)
#define smallbin_index(sz)  (((u32)(sz)) >> 3)

#define bin_index(sz)                                                          \
    (((((u32)(sz)) >> 9) == 0) ? (((u32)(sz)) >> 3) :                          \
     ((((u32)(sz)) >> 9) <= 4) ? 56 + (((u32)(sz)) >> 6) :                     \
     ((((u32)(sz)) >> 9) <= 20) ? 91 + (((u32)(sz)) >> 9) :                    \
     ((((u32)(sz)) >> 9) <= 84) ? 110 + (((u32)(sz)) >> 12) :                  \
     ((((u32)(sz)) >> 9) <= 340) ? 119 + (((u32)(sz)) >> 15) :                 \
     ((((u32)(sz)) >> 9) <= 1364) ? 124 + (((u32)(sz)) >> 18) :                \
     126)

#define BINBLOCKWIDTH       4
#define binblocks           (bin_at(0)->size)   /* 0x003075AC */
#define idx2binblock(ix)    ((u64)1 << ((ix) / BINBLOCKWIDTH))
#define mark_binblock(ii)   (binblocks |= idx2binblock(ii))

#define unlink(P, BK, FD) \
    {                     \
        BK = P->bk;       \
        FD = P->fd;       \
        FD->bk = BK;      \
        BK->fd = FD;      \
    }

#define frontlink(P, S, IDX, BK, FD)                                  \
    {                                                                 \
        if (S < MAX_SMALLBIN_SIZE) {                                  \
            IDX = smallbin_index(S);                                  \
            mark_binblock(IDX);                                       \
            BK = bin_at(IDX);                                         \
            FD = BK->fd;                                              \
            P->bk = BK;                                               \
            P->fd = FD;                                               \
            FD->bk = BK->fd = P;                                      \
        } else {                                                      \
            IDX = bin_index(S);                                       \
            BK = bin_at(IDX);                                         \
            FD = BK->fd;                                              \
            if (FD == BK)                                             \
                mark_binblock(IDX);                                   \
            else {                                                    \
                while (FD != BK && S < chunksize(FD))                 \
                    FD = FD->fd;                                      \
                BK = FD->bk;                                          \
            }                                                         \
            P->bk = BK;                                               \
            P->fd = FD;                                               \
            FD->bk = BK->fd = P;                                      \
        }                                                             \
    }

#define link_last_remainder(P)                                  \
    {                                                           \
        last_remainder->fd = last_remainder->bk = P;            \
        P->fd = P->bk = last_remainder;                         \
    }
#define clear_last_remainder \
    (last_remainder->fd = last_remainder->bk = last_remainder)

#endif /* G007_MALLOCR_U138_H */
