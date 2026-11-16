/* u138 / G007: newlib mallocr.o, i.e. mallocr.c compiled with DEFINE_MALLOC
 * (Doug Lea's malloc 2.6.x, the REENT build): the allocator's data,
 * malloc_extend_top and _malloc_r; then mlock.o (__malloc_lock /
 * __malloc_unlock) and reent/sbrkr.o (_sbrk_r), which have no data.  The
 * common macros are mallocr.h; free / malloc_trim are freer.o,
 * freer.c. */
#include "mallocr.h"

/* sbrk's error number (G007_stdio.h): a C common at 0x00353520 among the
 * typeinfo nodes; no other file defines it and only _sbrk_r reads it. */
int sys_errno;                              /* 0x00353520 */

static void malloc_extend_top(struct _reent *reent_ptr, u32 nb);

/* ------------------------------------------------------------------------ */
/* mallocr.o's .data (0x003075A8..0x00307A00), in definition order.  Under   */
/* INTERNAL_NEWLIB the variables are globals with the __malloc_ link names   */
/* (newlib_part2_internal.h / runtime/newlib/G007_stdio.h declare them).  The anchor: freer.o's code,   */
/* earlier in the link, references the same variables.                      */
/* ------------------------------------------------------------------------ */
// DATA_START 0x003075A8

/* The bins: each bin header's fd / bk point at the header itself (an empty
 * list), bin_at(0) being the initial top; av_[0] / av_[1] are bin 0's
 * prev_size / size (size = the binblocks bit vector). */
#define IAV(i)  bin_at(i), bin_at(i)

mbinptr __malloc_av_[NAV * 2 + 2] = {   /* 0x003075A8 av_ */
    0, 0,
    IAV(0),    IAV(1),    IAV(2),    IAV(3),    IAV(4),    IAV(5),    IAV(6),    IAV(7),
    IAV(8),    IAV(9),    IAV(10),   IAV(11),   IAV(12),   IAV(13),   IAV(14),   IAV(15),
    IAV(16),   IAV(17),   IAV(18),   IAV(19),   IAV(20),   IAV(21),   IAV(22),   IAV(23),
    IAV(24),   IAV(25),   IAV(26),   IAV(27),   IAV(28),   IAV(29),   IAV(30),   IAV(31),
    IAV(32),   IAV(33),   IAV(34),   IAV(35),   IAV(36),   IAV(37),   IAV(38),   IAV(39),
    IAV(40),   IAV(41),   IAV(42),   IAV(43),   IAV(44),   IAV(45),   IAV(46),   IAV(47),
    IAV(48),   IAV(49),   IAV(50),   IAV(51),   IAV(52),   IAV(53),   IAV(54),   IAV(55),
    IAV(56),   IAV(57),   IAV(58),   IAV(59),   IAV(60),   IAV(61),   IAV(62),   IAV(63),
    IAV(64),   IAV(65),   IAV(66),   IAV(67),   IAV(68),   IAV(69),   IAV(70),   IAV(71),
    IAV(72),   IAV(73),   IAV(74),   IAV(75),   IAV(76),   IAV(77),   IAV(78),   IAV(79),
    IAV(80),   IAV(81),   IAV(82),   IAV(83),   IAV(84),   IAV(85),   IAV(86),   IAV(87),
    IAV(88),   IAV(89),   IAV(90),   IAV(91),   IAV(92),   IAV(93),   IAV(94),   IAV(95),
    IAV(96),   IAV(97),   IAV(98),   IAV(99),   IAV(100),  IAV(101),  IAV(102),  IAV(103),
    IAV(104),  IAV(105),  IAV(106),  IAV(107),  IAV(108),  IAV(109),  IAV(110),  IAV(111),
    IAV(112),  IAV(113),  IAV(114),  IAV(115),  IAV(116),  IAV(117),  IAV(118),  IAV(119),
    IAV(120),  IAV(121),  IAV(122),  IAV(123),  IAV(124),  IAV(125),  IAV(126),  IAV(127)
};

/* variables holding tunable values */
u64 __malloc_trim_threshold = DEFAULT_TRIM_THRESHOLD;  /* 0x003079B0 trim_threshold */
u64 __malloc_top_pad = DEFAULT_TOP_PAD;                /* 0x003079B8 top_pad */

/* The first value returned from sbrk */
char *__malloc_sbrk_base = (char *)(-1);               /* 0x003079C0 sbrk_base */

/* The maximum memory obtained from system via sbrk */
u64 __malloc_max_sbrked_mem = 0;                       /* 0x003079C8 max_sbrked_mem */

/* The maximum via either sbrk or mmap (no mmap here) */
u64 __malloc_max_total_mem = 0;                        /* 0x003079D0 max_total_mem */

/* internal working copy of mallinfo */
struct mallinfo __malloc_current_mallinfo = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };   /* 0x003079D8 current_mallinfo */

/* ------------------------------------------------------------------------ */
/* malloc_extend_top (this version has no correction_failed handling: a     */
/* failing second sbrk just returns)                                        */
/* ------------------------------------------------------------------------ */

// FUNC 0x00281D88 malloc_extend_top
static void malloc_extend_top(struct _reent *reent_ptr, u32 nb)
{
    char *brk;          /* return value from sbrk */
    u32 front_misalign; /* unusable bytes at front of sbrked space */
    u32 correction;     /* bytes for 2nd sbrk call */
    char *new_brk;      /* return of 2nd sbrk call */
    u32 top_size;       /* new size of top chunk */

    mchunkptr old_top = top;                    /* Record state of old top */
    u32 old_top_size = chunksize(old_top);
    char *old_end = (char *)(chunk_at_offset(old_top, old_top_size));

    /* Pad request with top_pad plus minimal overhead */
    u32 sbrk_size = nb + top_pad + MINSIZE;
    u64 pagesz = malloc_getpagesize;

    /* If not the first time through, round to preserve page boundary.
     * Otherwise, we need to correct to a page size below anyway. */
    if (sbrk_base != (char *)(-1))
        sbrk_size = (sbrk_size + (pagesz - 1)) & ~(pagesz - 1);

    brk = (char *)(_sbrk_r(reent_ptr, (s32)sbrk_size));

    /* Fail if sbrk failed or if a foreign sbrk call killed our space */
    if (brk == MORECORE_FAILURE || (brk < old_end && old_top != initial_top))
        return;

    sbrked_mem += sbrk_size;

    if (brk == old_end) {   /* can just add bytes to current top */
        top_size = sbrk_size + old_top_size;
        set_head(top, top_size | PREV_INUSE);
    } else {
        if (sbrk_base == (char *)(-1))  /* First time through. Record base */
            sbrk_base = brk;
        else    /* Someone else called sbrk().  Count those bytes as sbrked_mem. */
            sbrked_mem += brk - (char *)old_end;

        /* Guarantee alignment of first new chunk made from this space */
        front_misalign = (u32)chunk2mem(brk) & MALLOC_ALIGN_MASK;
        if (front_misalign > 0) {
            correction = (MALLOC_ALIGNMENT) - front_misalign;
            brk += correction;
        } else
            correction = 0;

        /* Guarantee the next brk will be at a page boundary */
        correction += pagesz - ((u32)(brk + sbrk_size) & (pagesz - 1));

        /* Allocate correction */
        new_brk = (char *)(_sbrk_r(reent_ptr, (s32)correction));
        if (new_brk == MORECORE_FAILURE)
            return;

        sbrked_mem += correction;

        top = (mchunkptr)brk;
        top_size = new_brk - brk + correction;
        set_head(top, top_size | PREV_INUSE);

        if (old_top != initial_top) {
            /* There must have been an intervening foreign sbrk call.
             * A double fencepost is necessary to prevent consolidation. */

            /* If not enough space to do this, then user did something very wrong */
            if (old_top_size < MINSIZE) {
                set_head(top, PREV_INUSE);  /* will force null return from malloc */
                return;
            }

            /* Also keep size a multiple of MALLOC_ALIGNMENT */
            old_top_size = (old_top_size - 3 * SIZE_SZ) & ~MALLOC_ALIGN_MASK;
            set_head_size(old_top, old_top_size);
            chunk_at_offset(old_top, old_top_size)->size = SIZE_SZ | PREV_INUSE;
            chunk_at_offset(old_top, old_top_size + SIZE_SZ)->size = SIZE_SZ | PREV_INUSE;
            /* If possible, release the rest. */
            if (old_top_size >= MINSIZE)
                _free_r(reent_ptr, chunk2mem(old_top));
        }
    }

    /* (int sbrked_mem sign-extended to unsigned long) */
    if ((u64)(s64)sbrked_mem > (u64)max_sbrked_mem)
        max_sbrked_mem = (s64)sbrked_mem;
    if ((u64)(s64)sbrked_mem > (u64)max_total_mem)    /* mmapped_mem + sbrked_mem, no mmap */
        max_total_mem = (s64)sbrked_mem;
}

/* ------------------------------------------------------------------------ */
/* malloc                                                                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x00281FE0 _malloc_r
void *_malloc_r(struct _reent *reent_ptr, size_t bytes)
{
    mchunkptr victim;       /* inspected/selected chunk */
    u32 victim_size;        /* its size */
    int idx;                /* index for bin traversal */
    mbinptr bin;            /* associated bin */
    mchunkptr remainder;    /* remainder from a split */
    s64 remainder_size;     /* its size (long) */
    int remainder_index;    /* its bin index */
    u64 block;              /* block traverser bit (unsigned long) */
    int startidx;           /* first bin of a traversed block */
    mchunkptr fwd;          /* misc temp for linking */
    mchunkptr bck;          /* misc temp for linking */
    mbinptr q;              /* misc temp */

    u32 nb = request2size(bytes);   /* padded request size; */

    __malloc_lock(reent_ptr);

    /* Check for exact match in a bin */

    if (is_small_request(nb)) {     /* Faster version for small requests */
        idx = smallbin_index(nb);

        /* No traversal or size check necessary for small bins.  */
        q = bin_at(idx);
        victim = last(q);

        /* (MALLOC_ALIGN == 16: no scan of the next bin) */
        if (victim != q) {
            victim_size = chunksize(victim);
            unlink(victim, bck, fwd);
            set_inuse_bit_at_offset(victim, victim_size);
            __malloc_unlock(reent_ptr);
            return chunk2mem(victim);
        }

        idx += 2;   /* Set for bin scan below. We've already scanned 2 bins. */
    } else {
        idx = bin_index(nb);
        bin = bin_at(idx);

        for (victim = last(bin); victim != bin; victim = victim->bk) {
            victim_size = chunksize(victim);
            remainder_size = long_sub_size_t(victim_size, nb);

            if (remainder_size >= (s64)MINSIZE) {   /* too big */
                --idx;  /* adjust to rescan below after checking last remainder */
                break;
            } else if (remainder_size >= 0) {       /* exact fit */
                unlink(victim, bck, fwd);
                set_inuse_bit_at_offset(victim, victim_size);
                __malloc_unlock(reent_ptr);
                return chunk2mem(victim);
            }
        }

        ++idx;
    }

    /* Try to use the last split-off remainder */

    if ((victim = last_remainder->fd) != last_remainder) {
        victim_size = chunksize(victim);
        remainder_size = long_sub_size_t(victim_size, nb);

        if (remainder_size >= (s64)MINSIZE) {   /* re-split */
            remainder = chunk_at_offset(victim, nb);
            set_head(victim, nb | PREV_INUSE);
            link_last_remainder(remainder);
            set_head(remainder, remainder_size | PREV_INUSE);
            set_foot(remainder, remainder_size);
            __malloc_unlock(reent_ptr);
            return chunk2mem(victim);
        }

        clear_last_remainder;

        if (remainder_size >= 0) {  /* exhaust */
            set_inuse_bit_at_offset(victim, victim_size);
            __malloc_unlock(reent_ptr);
            return chunk2mem(victim);
        }

        /* Else place in bin */
        frontlink(victim, victim_size, remainder_index, bck, fwd);
    }

    /* If there are any possibly nonempty big-enough blocks, search for best
     * fitting chunk by scanning bins in blockwidth units. */

    if ((block = idx2binblock(idx)) <= (u64)binblocks) {
        /* Get to the first marked block */
        if ((block & binblocks) == 0) {
            /* force to an even block boundary */
            idx = (idx & ~(BINBLOCKWIDTH - 1)) + BINBLOCKWIDTH;
            block <<= 1;
            while ((block & binblocks) == 0) {
                idx += BINBLOCKWIDTH;
                block <<= 1;
            }
        }

        /* For each possibly nonempty block ... */
        for (;;) {
            startidx = idx;     /* (track incomplete blocks) */
            q = bin = bin_at(idx);

            /* For each bin in this block ... */
            do {
                /* Find and use first big enough chunk ... */
                for (victim = last(bin); victim != bin; victim = victim->bk) {
                    victim_size = chunksize(victim);
                    remainder_size = long_sub_size_t(victim_size, nb);

                    if (remainder_size >= (s64)MINSIZE) {   /* split */
                        remainder = chunk_at_offset(victim, nb);
                        set_head(victim, nb | PREV_INUSE);
                        unlink(victim, bck, fwd);
                        link_last_remainder(remainder);
                        set_head(remainder, remainder_size | PREV_INUSE);
                        set_foot(remainder, remainder_size);
                        __malloc_unlock(reent_ptr);
                        return chunk2mem(victim);
                    } else if (remainder_size >= 0) {       /* take */
                        set_inuse_bit_at_offset(victim, victim_size);
                        unlink(victim, bck, fwd);
                        __malloc_unlock(reent_ptr);
                        return chunk2mem(victim);
                    }
                }

                bin = next_bin(bin);

                /* MALLOC_ALIGN == 16: small bins come in pairs */
                if (idx < MAX_SMALLBIN) {
                    bin = next_bin(bin);
                    ++idx;
                }
            } while ((++idx & (BINBLOCKWIDTH - 1)) != 0);

            /* Clear out the block bit. */
            do {    /* Possibly backtrack to try to clear a partial block */
                if ((startidx & (BINBLOCKWIDTH - 1)) == 0) {
                    binblocks &= ~block;
                    break;
                }
                --startidx;
                q = prev_bin(q);
            } while (first(q) == q);

            /* Get to the next possibly nonempty block */
            if ((block <<= 1) <= (u64)binblocks && (block != 0)) {
                while ((block & binblocks) == 0) {
                    idx += BINBLOCKWIDTH;
                    block <<= 1;
                }
            } else
                break;
        }
    }

    /* Try to use top chunk */

    /* Require that there be a remainder, ensuring top always exists  */
    remainder_size = long_sub_size_t(chunksize(top), nb);
    if (chunksize(top) < nb || remainder_size < (s64)MINSIZE) {
        /* Try to extend */
        malloc_extend_top(reent_ptr, nb);
        remainder_size = long_sub_size_t(chunksize(top), nb);
        if (chunksize(top) < nb || remainder_size < (s64)MINSIZE) {
            __malloc_unlock(reent_ptr);
            return NULL;    /* propagate failure */
        }
    }

    victim = top;
    set_head(victim, nb | PREV_INUSE);
    top = chunk_at_offset(victim, nb);
    set_head(top, remainder_size | PREV_INUSE);
    __malloc_unlock(reent_ptr);
    return chunk2mem(victim);
}

/* ------------------------------------------------------------------------ */
/* mlock.c: no threads, the hooks are empty                                 */
/* ------------------------------------------------------------------------ */

// FUNC 0x00282878 __malloc_lock
void __malloc_lock(struct _reent *ptr)
{
}

// FUNC 0x00282880 __malloc_unlock
void __malloc_unlock(struct _reent *ptr)
{
}

/* ------------------------------------------------------------------------ */
/* reent/sbrkr.c                                                            */
/* ------------------------------------------------------------------------ */

// FUNC 0x00282A20 _sbrk_r
void *_sbrk_r(struct _reent *ptr, int incr)
{
    char *ret;

    sys_errno = 0;
    if ((ret = (char *)(sbrk(incr))) == (void *)-1 && sys_errno != 0)
        ptr->_errno = sys_errno;
    return ret;
}
