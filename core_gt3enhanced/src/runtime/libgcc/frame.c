/* frame.c - libgcc frame.o: gcc-2.95 gcc/frame.c (DWARF2 unwind-info
 * registry, FDE lookup and CFA interpreter), 0x0027AFE0..0x0027C010 of the GT3
 * US main executable.  18 functions.
 *
 * The same machine code as the bootstrap's runtime/frame.c (0x01018830..),
 * in a different order: there frame.o comes before libgcc_math, here it follows
 * fp-bit.o; also frame.o of the toolchain's own libgcc.a.  Stock frame.c
 * built with gthr-single.h (empty mutex calls, object_mutex in .data), with
 * these details from the code:
 *   - start_fde_sort (inlined into frame_init) mallocs both arrays
 *     unconditionally; no NULL checks anywhere;
 *   - fde_split is the "previous_max[count]" VLA version and so is emitted out
 *     of line at the end of the file (0x0027BE98);
 *   - frame_heapsort, start_fde_sort, fde_insert, fde_compare, next_fde,
 *     get_cie and the read_Nbyte helpers are inlined (no functions of their own);
 *   - the CFA interpreter knows DW_CFA_GNU_window_save (0x2D) and
 *     DW_CFA_GNU_args_size (0x2E), not 0x2F: the jump table @0x00348300 has
 *     0x2F entries.
 *
 * Users: crtbegin's reg_frame (0x001000C8) calls __register_frame_info_table,
 * dereg_frame (0x00100108) __deregister_frame_info; the EH runtime
 * (next_stack_level 0x00277EA0, __throw 0x00278268, __rethrow 0x00278448)
 * calls __frame_state_for.
 */
#include "runtime/libgcc/fp_bit_internal.h"
#include "runtime/libgcc/G006_fp_bit.h"

// DATA_START 0x00306848

static void fde_split(fde_vector *linear, fde_vector *erratic); /* 0x0027BE98 */

/* gthr.h without threads (gthr-single.h): the mutex calls are empty inline
 * functions.  They still shape the code: find_fde re-tests `ob' after the
 * object loop (the unlock sits between the loop and `if (ob == 0)'). */
typedef int __gthread_mutex_t;
#define __GTHREAD_MUTEX_INIT 0

static inline int
__gthread_mutex_lock(__gthread_mutex_t *mutex __attribute__((__unused__)))
{
    return 0;
}

static inline int
__gthread_mutex_unlock(__gthread_mutex_t *mutex __attribute__((__unused__)))
{
    return 0;
}

/* frame.o's only .data word (libgcc.a: local `object_mutex'); no code reads
 * it (the gthr-single mutex calls are empty), so the anchor above gives the
 * file its .data range. */
static __gthread_mutex_t object_mutex = __GTHREAD_MUTEX_INIT;   /* 0x00306848 */

/* Objects to be searched for frame unwind info: frame.o's whole .bss (a file
 * static, `b objects' in the toolchain's frame.o), zero until crtbegin's
 * reg_frame registers the first object. */
static struct object *objects;                                  /* 0x004AD250 */

#define init_object_mutex_once()

/* The LEB128 shifts: sllv takes the count mod 32, and egcs emits it for the
 * plain shift frame.c writes.  In ISO C a count >= 32 (an over-long number)
 * is undefined, so the GCC 14 build masks it: the same result. */
#ifdef BOOT_OLD_GCC
#define LEB_SH(n) (n)
#else
#define LEB_SH(n) ((n) & 31)
#endif

/* ------------------------------------------------------------------------ */
/* Inlined helpers (no functions of their own in the binary)                */
/* ------------------------------------------------------------------------ */

/* Unaligned reads from the instruction stream (lwl/lwr, or two lbu's). */
union unaligned {
    void *p;
    u16   b2;
    u32   b4;
} __attribute__((packed));

static inline void *read_pointer(void *p) { union unaligned *up = p; return up->p; }
static inline unsigned read_1byte(void *p) { return *(unsigned char *)p; }
static inline unsigned read_2byte(void *p) { union unaligned *up = p; return up->b2; }
static inline unsigned read_4byte(void *p) { union unaligned *up = p; return up->b4; }

/* Ordering function for FDEs: SIGNED difference of the start addresses
 * (subu + blez/bgez/bltz), computed in u32 for the same wrap-around. */
static inline int fde_compare(fde *x, fde *y)
{
    return (int)((u32)x->pc_begin - (u32)y->pc_begin);
}

/* Address of the FDE/CIE after P. */
static inline fde *next_fde(fde *p)
{
    return (fde *)((char *)p + p->length + sizeof(p->length));
}

/* CIE of an FDE: &f->CIE_delta - f->CIE_delta. */
static inline struct dwarf_cie *get_cie(fde *f)
{
    return (struct dwarf_cie *)((char *)&f->CIE_delta - f->CIE_delta);
}

/* Inlined into frame_init: both arrays malloc'd (count * 4 bytes each). */
static inline void start_fde_sort(fde_accumulator *accu, size_t count)
{
    accu->linear.array  = (fde **)malloc(sizeof(fde *) * count);
    accu->erratic.array = (fde **)malloc(sizeof(fde *) * count);
    accu->linear.count  = 0;
    accu->erratic.count = 0;
}

/* Inlined into add_fdes (no NULL check on the array). */
static inline void fde_insert(fde_accumulator *accu, fde *this_fde)
{
    accu->linear.array[accu->linear.count++] = this_fde;
}

/* Heap sort by pc_begin, inlined into end_fde_sort where it runs on
 * accu->erratic. */
static inline void frame_heapsort(fde_vector *erratic)
{
    fde **a = erratic->array;
#define SWAP(x, y) do { fde *tmp = x; x = y; y = tmp; } while (0)
    size_t n = erratic->count;
    size_t m = n;
    size_t i;

    while (m > 0) {
        /* Invariant: a[m..n-1] is a heap. */
        m--;
        for (i = m; 2 * i + 1 < n;) {
            if (2 * i + 2 < n
                && fde_compare(a[2 * i + 2], a[2 * i + 1]) > 0
                && fde_compare(a[2 * i + 2], a[i]) > 0) {
                SWAP(a[i], a[2 * i + 2]);
                i = 2 * i + 2;
            } else if (fde_compare(a[2 * i + 1], a[i]) > 0) {
                SWAP(a[i], a[2 * i + 1]);
                i = 2 * i + 1;
            } else
                break;
        }
    }
    while (n > 1) {
        /* Invariant: a[0..n-1] is a heap. */
        n--;
        SWAP(a[0], a[n]);
        for (i = 0; 2 * i + 1 < n;) {
            if (2 * i + 2 < n
                && fde_compare(a[2 * i + 2], a[2 * i + 1]) > 0
                && fde_compare(a[2 * i + 2], a[i]) > 0) {
                SWAP(a[i], a[2 * i + 2]);
                i = 2 * i + 2;
            } else if (fde_compare(a[2 * i + 1], a[i]) > 0) {
                SWAP(a[i], a[2 * i + 1]);
                i = 2 * i + 1;
            } else
                break;
        }
    }
#undef SWAP
}

/* ------------------------------------------------------------------------ */
/* Functions, in address order                                              */
/* ------------------------------------------------------------------------ */

// FUNC 0x0027AFE0 decode_uleb128
//   Decode the unsigned LEB128 number at BUF into *R; return the byte after it.
//   The shift is done with sllv, so shift counts >= 32 wrap mod 32.
static unsigned char *decode_uleb128(unsigned char *buf, unsigned *r)
{
    unsigned shift = 0;
    unsigned result = 0;

    while (1) {
        unsigned byte = *buf++;
        result |= (byte & 0x7f) << LEB_SH(shift);
        if ((byte & 0x80) == 0)
            break;
        shift += 7;
    }
    *r = result;
    return buf;
}

// FUNC 0x0027B028 decode_sleb128
//   Decode the signed LEB128 number at BUF into *R; sign-extends from bit 6 of
//   the last byte when fewer than 32 bits were consumed (sltiu shift, 0x20).
//   Only caller: extract_cie_info (data_align).
static unsigned char *decode_sleb128(unsigned char *buf, int *r)
{
    unsigned shift = 0;
    unsigned result = 0;
    unsigned byte;

    while (1) {
        byte = *buf++;
        result |= (byte & 0x7f) << LEB_SH(shift);
        shift += 7;
        if ((byte & 0x80) == 0)
            break;
    }
    if (shift < (sizeof(*r) * 8) && (byte & 0x40) != 0)
        result |= - (1 << shift);

    *r = result;
    return buf;
}

// FUNC 0x0027B078 fde_merge
//   Merge V2 (the heap-sorted erratic FDEs) into V1 (the linear FDEs, whose
//   array has room for both), both sorted by pc_begin, back to front, in place.
//   V1->count += V2->count.  Only caller: end_fde_sort.
static void fde_merge(fde_vector *v1, const fde_vector *v2)
{
    size_t i1, i2;
    fde *fde2;

    i2 = v2->count;
    if (i2 > 0) {
        i1 = v1->count;
        do {
            i2--;
            fde2 = v2->array[i2];
            while (i1 > 0 && fde_compare(v1->array[i1 - 1], fde2) > 0) {
                v1->array[i1 + i2] = v1->array[i1 - 1];
                i1--;
            }
            v1->array[i1 + i2] = fde2;
        } while (i2 > 0);
        v1->count += v2->count;
    }
}

// FUNC 0x0027B178 end_fde_sort
//   Finish sorting the FDE pointers collected by add_fdes: split into the
//   in-order ("linear") run and the out-of-order ("erratic") ones, heap-sort
//   the erratic ones (frame_heapsort inlined), merge, free the erratic array
//   and return the sorted linear array.  abort()s (0x0027D7A0) if the counts
//   do not add up.  Only caller: frame_init.
static fde **end_fde_sort(fde_accumulator *accu, size_t count)
{
    if (accu->linear.count != count)
        abort();
    fde_split(&accu->linear, &accu->erratic);
    if (accu->linear.count + accu->erratic.count != count)
        abort();
    frame_heapsort(&accu->erratic);
    fde_merge(&accu->linear, &accu->erratic);
    free(accu->erratic.array);
    return accu->linear.array;
}

// FUNC 0x0027B3E0 count_fdes
//   Count the real FDEs in one CIE/FDE chain (terminated by length == 0),
//   skipping CIEs (CIE_delta == 0) and discarded link-once FDEs
//   (pc_begin == 0).  Only caller: frame_init.
static size_t count_fdes(fde *this_fde)
{
    size_t count;

    for (count = 0; this_fde->length != 0; this_fde = next_fde(this_fde)) {
        /* Skip CIEs and linked once FDE entries. */
        if (this_fde->CIE_delta == 0 || this_fde->pc_begin == 0)
            continue;

        ++count;
    }
    return count;
}

// FUNC 0x0027B428 add_fdes
//   Append every real FDE of one chain to accu->linear (fde_insert inlined,
//   no NULL check) and widen [*beg_ptr, *end_ptr) to cover it (unsigned
//   compares).  Only caller: frame_init.
static void add_fdes(fde *this_fde, fde_accumulator *accu, void **beg_ptr, void **end_ptr)
{
    void *pc_begin = *beg_ptr;
    void *pc_end = *end_ptr;

    for (; this_fde->length != 0; this_fde = next_fde(this_fde)) {
        /* Skip CIEs and linked once FDE entries. */
        if (this_fde->CIE_delta == 0 || this_fde->pc_begin == 0)
            continue;

        fde_insert(accu, this_fde);

        if (this_fde->pc_begin < pc_begin)
            pc_begin = this_fde->pc_begin;
        if ((char *)this_fde->pc_begin + this_fde->pc_range > (char *)pc_end)
            pc_end = (char *)this_fde->pc_begin + this_fde->pc_range;
    }

    *beg_ptr = pc_begin;
    *end_ptr = pc_end;
}

// FUNC 0x0027B4B0 frame_init
//   Lazily build an object's sorted FDE table the first time find_fde meets it
//   (ob->pc_begin == 0).  Handles both registration forms: a single chain at
//   ob->fde_begin, or (table form) a NULL-terminated array of chains at
//   ob->fde_array.  Stores count (+0x10), the sorted array (+0xC, replacing
//   the table pointer) and the covered pc range (+0/+4).  With no FDEs at all
//   pc_begin stays 0xFFFFFFFF (still "initialized") and pc_end 0.
static void frame_init(struct object *ob)
{
    size_t count;
    fde_accumulator accu;           /* sp+0x00 */
    void *pc_begin, *pc_end;        /* sp+0x10, sp+0x14 */

    if (ob->fde_array) {
        fde **p = ob->fde_array;
        for (count = 0; *p; ++p)
            count += count_fdes(*p);
    } else
        count = count_fdes(ob->fde_begin);

    ob->count = count;

    start_fde_sort(&accu, count);   /* implicit: malloc; inlined: two malloc(count * 4) */
    pc_begin = (void *)(u32)-1;
    pc_end = 0;

    if (ob->fde_array) {
        fde **p = ob->fde_array;
        for (; *p; ++p)
            add_fdes(*p, &accu, &pc_begin, &pc_end);
    } else
        add_fdes(ob->fde_begin, &accu, &pc_begin, &pc_end);

    /* No "if (array)" guard in this version. */
    ob->fde_array = end_fde_sort(&accu, count);
    ob->pc_begin = pc_begin;
    ob->pc_end = pc_end;
}

// FUNC 0x0027B5F0 find_fde
//   Return the FDE covering PC, or 0.  Walks the registered objects
//   (initializing each on first use) until one's [pc_begin, pc_end) contains
//   PC, then binary-searches its sorted fde_array.  All pc compares unsigned.
//   Only caller: __frame_state_for.
static fde *find_fde(void *pc)
{
    struct object *ob;
    size_t lo, hi;

    init_object_mutex_once();
    __gthread_mutex_lock(&object_mutex);

    for (ob = objects; ob; ob = ob->next) {
        if (ob->pc_begin == 0)
            frame_init(ob);
        if (pc >= ob->pc_begin && pc < ob->pc_end)
            break;
    }

    __gthread_mutex_unlock(&object_mutex);

    if (ob == 0)
        return 0;

    /* Standard binary search algorithm. */
    for (lo = 0, hi = ob->count; lo < hi;) {
        size_t i = (lo + hi) / 2;
        fde *f = ob->fde_array[i];

        if (pc < f->pc_begin)
            hi = i;
        else if ((char *)pc >= (char *)f->pc_begin + f->pc_range)
            lo = i + 1;
        else
            return f;
    }

    return 0;
}

// FUNC 0x0027B6D8 extract_cie_info
//   Parse the CIE of F into *C and return a pointer to the CIE's initial
//   instructions, or 0 for an unknown augmentation (anything other than "",
//   "eh" or a string starting with 'z').  "eh" is followed by an unaligned
//   pointer (lwl/lwr) stored as eh_ptr; 'z' augmentations carry a uleb length
//   of augmentation data that is skipped.  Strings "" @0x003482F0 and "eh"
//   @0x003482F8; strcmp 0x0027EA40, strlen 0x00269A68.
static void *extract_cie_info(fde *f, struct cie_info *c)
{
    unsigned char *p;
    int aug_data_len;               /* sp+0 */

    c->augmentation = get_cie(f)->augmentation;

    if (strcmp(c->augmentation, "") != 0
        && strcmp(c->augmentation, "eh") != 0
        && c->augmentation[0] != 'z')
        return 0;

    p = (unsigned char *)c->augmentation + strlen(c->augmentation) + 1;

    if (strcmp(c->augmentation, "eh") == 0) {
        c->eh_ptr = read_pointer(p);
        p += sizeof(void *);
    } else
        c->eh_ptr = 0;

    p = decode_uleb128(p, (unsigned *)&c->code_align);
    p = decode_sleb128(p, &c->data_align);
    c->ra_regno = *p++;

    /* If the augmentation starts with 'z', we now see the length of the
       augmentation fields. */
    if (c->augmentation[0] == 'z') {
        p = decode_uleb128(p, (unsigned *)&aug_data_len);
        p += aug_data_len;
    }

    return p;
}

// FUNC 0x0027B7E0 execute_cfa_insn
//   Execute one DWARF2 call-frame instruction at P against STATE; return the
//   pointer to the next instruction.  PC is the location counter (NULL while
//   running the CIE's initial instructions).  The low-opcode switch is the
//   47-entry jump table @0x00348300 (0x0F..0x2C and >= 0x2F -> abort).
//   QUIRK (stock gcc 2.95): the high-bit tests are `insn & 0x40`, then
//   `insn & 0x80`, then `insn & 0xC0`, so DW_CFA_restore (0xC0|reg) is taken
//   as DW_CFA_advance_loc (0x40 bit set) and the restore branch below is
//   unreachable dead code (still present in the binary).
static void *execute_cfa_insn(void *insn_ptr, struct frame_state_internal *state,
                              struct cie_info *info, void **pc)
{
    unsigned char *p = insn_ptr;
    unsigned insn = *p++;
    unsigned reg;                   /* sp+4 */
    int offset;                     /* sp+0 */

    if (insn & DW_CFA_advance_loc)
        *pc = (char *)*pc + (insn & DW_CFA_OPERAND_MASK) * info->code_align;  /* mult3 */
    else if (insn & DW_CFA_offset) {
        reg = (insn & DW_CFA_OPERAND_MASK);
        p = decode_uleb128(p, (unsigned *)&offset);
        offset *= info->data_align;
        state->s.saved[reg] = REG_SAVED_OFFSET;
        state->s.reg_or_offset[reg] = offset;                       /* sign-extended */
    } else if (insn & DW_CFA_restore) {
        /* unreachable - see QUIRK above */
        reg = (insn & DW_CFA_OPERAND_MASK);
        state->s.saved[reg] = REG_UNSAVED;
    } else switch (insn) {
    case DW_CFA_set_loc:
        *pc = read_pointer(p);
        p += sizeof(void *);
        break;
    case DW_CFA_advance_loc1:
        *pc = (char *)*pc + read_1byte(p);
        p += 1;
        break;
    case DW_CFA_advance_loc2:
        *pc = (char *)*pc + read_2byte(p);
        p += 2;
        break;
    case DW_CFA_advance_loc4:
        *pc = (char *)*pc + read_4byte(p);
        p += 4;
        break;

    case DW_CFA_offset_extended:
        p = decode_uleb128(p, &reg);
        p = decode_uleb128(p, (unsigned *)&offset);
        offset *= info->data_align;
        state->s.saved[reg] = REG_SAVED_OFFSET;
        state->s.reg_or_offset[reg] = offset;
        break;
    case DW_CFA_restore_extended:
        p = decode_uleb128(p, &reg);
        state->s.saved[reg] = REG_UNSAVED;
        break;

    case DW_CFA_undefined:
    case DW_CFA_same_value:
    case DW_CFA_nop:
        break;

    case DW_CFA_register: {
        unsigned saved_in_reg;      /* sp+8 the register that holds REG's value */
        p = decode_uleb128(p, &reg);
        p = decode_uleb128(p, &saved_in_reg);
        state->s.saved[reg] = REG_SAVED_REG;
        state->s.reg_or_offset[reg] = saved_in_reg;                 /* zero-extended */
        break;
    }

    case DW_CFA_def_cfa:
        p = decode_uleb128(p, &reg);
        p = decode_uleb128(p, (unsigned *)&offset);
        state->s.cfa_reg = reg;
        state->s.cfa_offset = offset;                               /* sign-extended */
        break;
    case DW_CFA_def_cfa_register:
        p = decode_uleb128(p, &reg);
        state->s.cfa_reg = reg;
        break;
    case DW_CFA_def_cfa_offset:
        p = decode_uleb128(p, (unsigned *)&offset);
        state->s.cfa_offset = offset;
        break;

    case DW_CFA_remember_state: {
        /* malloc(0x2F8), no NULL check; memcpy inlined as an ld/sd loop. */
        struct frame_state_internal *save =
            (struct frame_state_internal *)malloc(sizeof(struct frame_state_internal));
        memcpy(save, state, sizeof(struct frame_state_internal));
        state->saved_state = save;
        break;
    }
    case DW_CFA_restore_state: {
        /* memcpy inlined; the copy also restores state->saved_state. */
        struct frame_state_internal *save = state->saved_state;
        memcpy(state, save, sizeof(struct frame_state_internal));
        free(save);
        break;
    }

    /* FIXME in gcc: hardcoded for the SPARC register-window layout
     * (%l0..%i7 = columns 16..31 saved in the window at cfa + 0..0x3C);
     * meaningless on MIPS, never emitted for it. */
    case DW_CFA_GNU_window_save:
        for (reg = 16; reg < 32; ++reg) {
            state->s.saved[reg] = REG_SAVED_OFFSET;
            state->s.reg_or_offset[reg] = (reg - 16) * sizeof(void *);  /* zero-extended */
        }
        break;

    case DW_CFA_GNU_args_size:
        p = decode_uleb128(p, (unsigned *)&offset);
        state->s.args_size = offset;
        break;

    default:
        abort();
    }
    return p;
}

// FUNC 0x0027BB78 __register_frame_info
//   Register the single FDE chain at BEGIN using caller-provided storage OB
//   (pushed on the front of `objects`; initialized lazily by find_fde).
//   Only reached by __register_frame's tail-jump.
void __register_frame_info(void *begin, struct object *ob)
{
    ob->fde_begin = begin;

    ob->pc_begin = ob->pc_end = 0;
    ob->fde_array = 0;
    ob->count = 0;

    init_object_mutex_once();
    __gthread_mutex_lock(&object_mutex);

    ob->next = objects;
    objects = ob;

    __gthread_mutex_unlock(&object_mutex);
}

// FUNC 0x0027BBA0 __register_frame
//   malloc(0x18) a struct object (no NULL check) and tail-call
//   __register_frame_info.  No callers.
void __register_frame(void *begin)
{
    struct object *ob = (struct object *)malloc(sizeof(struct object));
    __register_frame_info(begin, ob);
}

// FUNC 0x0027BBD0 __register_frame_info_table
//   Like __register_frame_info, but BEGIN is a NULL-terminated array of FDE
//   chain pointers: stored both as fde_begin (the deregistration key) and as
//   fde_array (walked by frame_init).  Caller: crtbegin's reg_frame 0x001000C8.
void __register_frame_info_table(void *begin, struct object *ob)
{
    ob->fde_begin = begin;
    ob->fde_array = begin;

    ob->pc_begin = ob->pc_end = 0;
    ob->count = 0;

    init_object_mutex_once();
    __gthread_mutex_lock(&object_mutex);

    ob->next = objects;
    objects = ob;

    __gthread_mutex_unlock(&object_mutex);
}

// FUNC 0x0027BBF8 __register_frame_table
//   malloc(0x18) a struct object (no NULL check) and tail-call
//   __register_frame_info_table.  No callers.
void __register_frame_table(void *begin)
{
    struct object *ob = (struct object *)malloc(sizeof(struct object));
    __register_frame_info_table(begin, ob);
}

// FUNC 0x0027BC28 __deregister_frame_info
//   Unlink the object registered with BEGIN (matched on fde_begin) and return
//   it; if frame_init had run (pc_begin != 0) free its sorted fde_array first.
//   abort()s (tail-jump to 0x0027D7A0) when BEGIN was never registered.
//   Callers: crtbegin's dereg_frame 0x00100108, __deregister_frame.
struct object *__deregister_frame_info(void *begin)
{
    struct object **p;

    init_object_mutex_once();
    __gthread_mutex_lock(&object_mutex);

    p = &objects;
    while (*p) {
        if ((*p)->fde_begin == begin) {
            struct object *ob = *p;
            *p = (*p)->next;

            /* If we've run init_frame for this object, free the FDE array. */
            if (ob->pc_begin)
                free(ob->fde_array);

            __gthread_mutex_unlock(&object_mutex);
            return ob;
        }
        p = &((*p)->next);
    }

    __gthread_mutex_unlock(&object_mutex);
    abort();
}

// FUNC 0x0027BCA0 __deregister_frame
//   free(__deregister_frame_info(begin)) (tail-jump to free) - counterpart of
//   __register_frame.  No callers.
void __deregister_frame(void *begin)
{
    free(__deregister_frame_info(begin));
}

// FUNC 0x0027BCC0 __frame_state_for
//   Compute the unwind state for PC_TARGET into *STATE_IN and return
//   STATE_IN, or 0 when no FDE covers the pc or its CIE has an unknown
//   augmentation.  Runs the CIE's initial instructions, then the FDE's
//   instructions while the location counter is <= PC_TARGET (unsigned).
//   Frame: info @sp+0, state @sp+0x20 (0x2F8), aug_data_len @sp+0x320,
//   pc @sp+0x324.
//   memset is a real call (0x0027DB38); the final 0x2F0-byte memcpy is
//   inlined as an ld/sd loop.  Callers: next_stack_level 0x00277EA0,
//   __throw 0x00278268, __rethrow 0x00278448.
struct frame_state *__frame_state_for(void *pc_target, struct frame_state *state_in)
{
    fde *f;
    unsigned char *insn, *end;
    void *pc;
    struct cie_info info;
    struct frame_state_internal state;

    f = find_fde(pc_target);
    if (f == 0)
        return 0;

    insn = extract_cie_info(f, &info);
    if (insn == 0)
        return 0;

    memset(&state, 0, sizeof(state));
    state.s.retaddr_column = info.ra_regno;
    state.s.eh_ptr = info.eh_ptr;

    /* First decode all the insns in the CIE. */
    end = (unsigned char *)next_fde((fde *)get_cie(f));
    while (insn < end)
        insn = execute_cfa_insn(insn, &state, &info, 0);

    insn = (unsigned char *)(f + 1);

    if (info.augmentation[0] == 'z') {
        int aug_data_len;
        insn = decode_uleb128(insn, (unsigned *)&aug_data_len);
        insn += aug_data_len;
    }

    /* Then the insns in the FDE up to our target PC. */
    end = (unsigned char *)next_fde(f);
    pc = f->pc_begin;
    while (insn < end && pc <= pc_target)
        insn = execute_cfa_insn(insn, &state, &info, &pc);

    memcpy(state_in, &state.s, sizeof(state.s));
    return state_in;
}

// FUNC 0x0027BE98 fde_split
//   Split LINEAR into the longest run of FDEs already in pc order (kept in
//   LINEAR, compacted) and the out-of-order ones (appended to ERRATIC), O(N).
//   previous_max[] is a VLA (alloca'd, count*4 rounded up to 16, frame
//   pointer $fp), which is why gcc emitted this `static inline` function out
//   of line here at the end of the file.  NB: the compaction pass runs even
//   when nothing was moved, and linear->count is set to the number kept.
//   Only caller: end_fde_sort.
static void fde_split(fde_vector *linear, fde_vector *erratic)
{
    size_t count = linear->count;
    size_t linear_max = (size_t)-1;
    size_t previous_max[count];
    size_t i, j;

    for (i = 0; i < count; i++) {
        /* Elements before i that are greater than linear->array[i] leave
           the linear sequence. */
        for (j = linear_max;
             j != (size_t)-1
             && fde_compare(linear->array[i], linear->array[j]) < 0;
             j = previous_max[j]) {
            erratic->array[erratic->count++] = linear->array[j];
            linear->array[j] = (fde *)NULL;
        }
        previous_max[i] = j;
        linear_max = i;
    }

    for (i = 0, j = 0; i < count; i++)
        if (linear->array[i] != (fde *)NULL)
            linear->array[j++] = linear->array[i];
    linear->count = j;
}
