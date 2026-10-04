/* frame.c - GCC 2.9x-ee libgcc DWARF2 unwind-info registry, FDE lookup and
 * CFA interpreter (gcc/frame.c), as linked into the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102).
 *
 * Address range 0x01018830..0x01019860, 18 functions.  This is the stock
 * gcc-2.95.x frame.c (no gthreads, so no object mutex) with these variant
 * details confirmed from the machine code:
 *   - start_fde_sort (inlined into frame_init) mallocs both the linear and
 *     the erratic array unconditionally, with no NULL checks anywhere
 *     (fde_insert / end_fde_sort have no "array != NULL" guards);
 *   - fde_split is the gcc-2.95 "previous_max[count]" VLA version; because of
 *     the VLA it could not be inlined and was emitted out of line at the end
 *     of the file (0x010196E8), after __frame_state_for;
 *   - frame_heapsort, start_fde_sort, fde_insert, fde_compare, next_fde,
 *     get_cie and the read_Nbyte helpers are all inlined (no functions of
 *     their own in the binary) - they appear below as `static inline` helpers
 *     without FUNC markers;
 *   - the CFA interpreter knows DW_CFA_GNU_window_save (0x2D) and
 *     DW_CFA_GNU_args_size (0x2E) but not GNU_negative_offset_extended (0x2F):
 *     the jump table @0x01040110 has 0x2F entries.
 *
 * Data owned by this unit:
 *   objects        @0x01049F68 (.bss)    head of the registered-object list
 *   ""             @0x01040100 (.rodata) augmentation compare strings
 *   "eh"           @0x01040108 (.rodata)
 *   jump table     @0x01040110 (.rodata) 47 words, execute_cfa_insn's switch
 *
 * Users: crtbegin's reg_frame (0x010000C8) registers the program's
 * .eh_frame with __register_frame_info_table(0x01026C00, &object@0x01042048);
 * its dereg_frame (0x01000108) calls __deregister_frame_info(0x01026C00).  The EH
 * runtime (next_stack_level 0x010178E0, __throw 0x01017CA8, __rethrow
 * 0x01017E88) calls __frame_state_for.  __register_frame,
 * __register_frame_table and __deregister_frame have no callers.
 */
#include "types.h"
#include "libc.h"
#include "cxxrt.h"

/* ------------------------------------------------------------------------ */
/* DWARF 2 call-frame opcodes                                               */
/* ------------------------------------------------------------------------ */

#define DW_CFA_advance_loc          0x40  /* high 2 bits 01, low 6 = delta     */
#define DW_CFA_offset               0x80  /* high 2 bits 10, low 6 = register  */
#define DW_CFA_restore              0xC0  /* high 2 bits 11, low 6 = register  */
#define DW_CFA_nop                  0x00
#define DW_CFA_set_loc              0x01
#define DW_CFA_advance_loc1         0x02
#define DW_CFA_advance_loc2         0x03
#define DW_CFA_advance_loc4         0x04
#define DW_CFA_offset_extended      0x05
#define DW_CFA_restore_extended     0x06
#define DW_CFA_undefined            0x07
#define DW_CFA_same_value           0x08
#define DW_CFA_register             0x09
#define DW_CFA_remember_state       0x0A
#define DW_CFA_restore_state        0x0B
#define DW_CFA_def_cfa              0x0C
#define DW_CFA_def_cfa_register     0x0D
#define DW_CFA_def_cfa_offset       0x0E
#define DW_CFA_GNU_window_save      0x2D
#define DW_CFA_GNU_args_size        0x2E

/* frame_state.saved[] values (gcc frame.h) */
#define REG_UNSAVED         0
#define REG_SAVED_OFFSET    1
#define REG_SAVED_REG       2

/* 79 DWARF register columns on the EE target; arrays hold columns 0..79. */
#define DWARF_FRAME_REGISTERS 79

/* ------------------------------------------------------------------------ */
/* Structures                                                               */
/* ------------------------------------------------------------------------ */

/* First fields of a CIE.  CIE_id is 0 for a CIE (distinguishes it from FDEs). */
struct dwarf_cie {
    u32  length;            /* +0x0 bytes following this word                 */
    s32  CIE_id;            /* +0x4 0                                         */
    u8   version;           /* +0x8                                           */
    char augmentation[];    /* +0x9 NUL-terminated: "", "eh" or "z..."        */
} __attribute__((packed, aligned(4)));

/* First fields of an FDE. */
typedef struct dwarf_fde {
    u32   length;           /* +0x0 bytes following this word; 0 ends a chain */
    s32   CIE_delta;        /* +0x4 &CIE_delta - CIE; 0 means "this is a CIE" */
    void *pc_begin;         /* +0x8 first pc covered (0 = discarded linkonce) */
    u32   pc_range;         /* +0xC bytes covered                             */
} __attribute__((packed, aligned(4))) fde;

_Static_assert(sizeof(fde) == 0x10, "fde header is 16 bytes (f + 1 == instructions)");

/* Registration record; __register_frame mallocs 0x18 bytes for one.  (Opaque
 * `struct object` in cxxrt.h.) */
struct object {
    void  *pc_begin;        /* +0x00 lowest pc covered; 0 = frame_init not run */
    void  *pc_end;          /* +0x04 one past the highest pc covered          */
    fde   *fde_begin;       /* +0x08 the `begin` passed at registration       */
    fde  **fde_array;       /* +0x0C table form: NULL-terminated list of FDE
                                     chains; after frame_init: sorted fde*[] */
    size_t count;           /* +0x10 number of FDEs in fde_array              */
    struct object *next;    /* +0x14                                          */
};
_Static_assert(sizeof(struct object) == 0x18, "malloc(0x18) in __register_frame");

/* What extract_cie_info pulls out of a CIE (0x14 bytes, on the stack of
 * __frame_state_for at sp+0). */
struct cie_info {
    char    *augmentation;  /* +0x00 */
    void    *eh_ptr;        /* +0x04 pointer following an "eh" augmentation  */
    int      code_align;    /* +0x08 uleb */
    int      data_align;    /* +0x0C sleb */
    unsigned ra_regno;      /* +0x10 one byte in the CIE */
};

/* Unwind state handed back to the EH runtime (gcc frame.h `frame_state`).
 * `long` is 64-bit in the EE gcc 2.9x ABI, hence the s64 fields (stored with
 * sd).  Shared layout with the libgcc_eh unit (__throw / next_stack_level). */
struct frame_state {
    void *cfa;                                          /* +0x000 */
    void *eh_ptr;                                       /* +0x004 */
    s64   cfa_offset;                                   /* +0x008 */
    s64   args_size;                                    /* +0x010 */
    s64   reg_or_offset[DWARF_FRAME_REGISTERS + 1];     /* +0x018 80 x 8      */
    u16   cfa_reg;                                      /* +0x298 */
    u16   retaddr_column;                               /* +0x29A */
    char  saved[DWARF_FRAME_REGISTERS + 1];             /* +0x29C REG_* kinds */
};                                                      /* 0x2EC -> pad 0x2F0 */
_Static_assert(offsetof(struct frame_state, reg_or_offset) == 0x018, "");
_Static_assert(offsetof(struct frame_state, cfa_reg) == 0x298, "");
_Static_assert(offsetof(struct frame_state, saved) == 0x29C, "");
_Static_assert(sizeof(struct frame_state) == 0x2F0, "0x2F0-byte copy in __frame_state_for");

/* Current state plus the DW_CFA_remember_state stack link. */
struct frame_state_internal {
    struct frame_state s;                       /* +0x000 */
    struct frame_state_internal *saved_state;   /* +0x2F0 */
};
_Static_assert(sizeof(struct frame_state_internal) == 0x2F8, "malloc/memset 0x2F8");

/* FDE sorting work areas (frame_init's stack, sp+0..0xF). */
typedef struct fde_vector {
    fde  **array;           /* +0x0 */
    size_t count;           /* +0x4 */
} fde_vector;

typedef struct fde_accumulator {
    fde_vector linear;      /* +0x0 */
    fde_vector erratic;     /* +0x8 */
} fde_accumulator;

/* ------------------------------------------------------------------------ */
/* Globals                                                                  */
/* ------------------------------------------------------------------------ */

/* Objects to be searched for frame unwind info. */
static struct object *objects;              /* @0x01049F68 (.bss) */

static void fde_split(fde_vector *linear, fde_vector *erratic); /* 0x010196E8 */

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
 * (subu + blez/bgez/bltz), i.e. gcc's `(saddr)x->pc_begin - (saddr)y->pc_begin`.
 * Computed in u32 here to get the same wrap-around without C signed overflow. */
static inline int fde_compare(fde *x, fde *y)
{
    return (int)((u32)x->pc_begin - (u32)y->pc_begin);
}

/* Address of the FDE/CIE after P. */
static inline fde *next_fde(fde *p)
{
    return (fde *)((char *)p + p->length + sizeof(p->length));
}

/* CIE of an FDE: &f->CIE_delta - f->CIE_delta  (augmentation = f - delta + 0xD). */
static inline struct dwarf_cie *get_cie(fde *f)
{
    return (struct dwarf_cie *)((char *)&f->CIE_delta - f->CIE_delta);
}

/* Inlined into frame_init.  No NULL checks in this version: both arrays are
 * always malloc'd (count * 4 bytes each; malloc(0) when there are no FDEs). */
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

/* Heap sort by pc_begin (Harbison & Steele), inlined into end_fde_sort where
 * it runs on accu->erratic.  O(n log n). */
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

// FUNC 0x01018830 decode_uleb128
//   Decode the unsigned LEB128 number at BUF into *R; return the byte after it.
//   Callers: extract_cie_info, execute_cfa_insn, __frame_state_for.  The shift
//   is done with sllv, so shift counts >= 32 wrap mod 32 (numbers > 5 bytes).
static unsigned char *decode_uleb128(unsigned char *buf, unsigned *r)
{
    unsigned shift = 0;
    unsigned result = 0;

    while (1) {
        unsigned byte = *buf++;
        result |= (byte & 0x7f) << (shift & 31);    /* sllv: count taken mod 32 */
        if ((byte & 0x80) == 0)
            break;
        shift += 7;
    }
    *r = result;
    return buf;
}

// FUNC 0x01018878 decode_sleb128
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
        result |= (byte & 0x7f) << (shift & 31);    /* sllv: count taken mod 32 */
        shift += 7;
        if ((byte & 0x80) == 0)
            break;
    }
    if (shift < (sizeof(*r) * 8) && (byte & 0x40) != 0)
        result |= ~0u << shift;             /* gcc source: -(1 << shift); binary: -1 sllv shift */

    *r = result;
    return buf;
}

// FUNC 0x010188C8 fde_merge
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

// FUNC 0x010189C8 end_fde_sort
//   Finish sorting the FDE pointers collected by add_fdes: split into the
//   in-order ("linear") run and the out-of-order ("erratic") ones, heap-sort
//   the erratic ones (frame_heapsort inlined), merge, free the erratic array
//   and return the sorted linear array.  abort()s (0x0101AFF0) if the counts
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

// FUNC 0x01018C30 count_fdes
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

// FUNC 0x01018C78 add_fdes
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

// FUNC 0x01018D00 frame_init
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

    start_fde_sort(&accu, count);
    pc_begin = (void *)(u32)-1;
    pc_end = 0;

    if (ob->fde_array) {
        fde **p = ob->fde_array;
        for (; *p; ++p)
            add_fdes(*p, &accu, &pc_begin, &pc_end);
    } else
        add_fdes(ob->fde_begin, &accu, &pc_begin, &pc_end);

    /* No "if (array)" guard in this version; stores in binary order. */
    ob->fde_array = end_fde_sort(&accu, count);
    ob->pc_end = pc_end;
    ob->pc_begin = pc_begin;
}

// FUNC 0x01018E40 find_fde
//   Return the FDE covering PC, or 0.  Walks the registered objects
//   (initializing each on first use) until one's [pc_begin, pc_end) contains
//   PC, then binary-searches its sorted fde_array.  All pc compares unsigned.
//   Only caller: __frame_state_for.
static fde *find_fde(void *pc)
{
    struct object *ob;
    size_t lo, hi;

    for (ob = objects; ob; ob = ob->next) {
        if (ob->pc_begin == 0)
            frame_init(ob);
        if (pc >= ob->pc_begin && pc < ob->pc_end)
            break;
    }

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

// FUNC 0x01018F28 extract_cie_info
//   Parse the CIE of F into *C and return a pointer to the CIE's initial
//   instructions, or 0 for an unknown augmentation (anything other than "",
//   "eh" or a string starting with 'z').  "eh" is followed by an unaligned
//   pointer (read with lwl/lwr) stored as eh_ptr; 'z' augmentations carry a
//   uleb length of augmentation data that is skipped.  Uses strcmp
//   (0x0101B380) and the game's strlen (0x0100F538).
static void *extract_cie_info(fde *f, struct cie_info *c)
{
    unsigned char *p;
    int i;                          /* sp+0 */

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
        p = decode_uleb128(p, (unsigned *)&i);
        p += i;
    }

    return p;
}

// FUNC 0x01019030 execute_cfa_insn
//   Execute one DWARF2 call-frame instruction at P against STATE; return the
//   pointer to the next instruction.  PC is the location counter (NULL while
//   running the CIE's initial instructions - a location opcode there would
//   write through NULL).  The low-opcode switch is the 47-entry jump table at
//   0x01040110 (0x0F..0x2C and >= 0x2F -> abort 0x0101AFF0).
//   QUIRK (stock gcc 2.95): the high-bit tests are `insn & 0x40`, then
//   `insn & 0x80`, then `insn & 0xC0`, so DW_CFA_restore (0xC0|reg) is taken
//   as DW_CFA_advance_loc (0x40 bit set) and the restore branch below is
//   unreachable dead code (still present in the binary).
static void *execute_cfa_insn(void *p_, struct frame_state_internal *state,
                              struct cie_info *info, void **pc)
{
    unsigned char *p = p_;
    unsigned insn = *p++;
    unsigned reg;                   /* sp+4 */
    int offset;                     /* sp+0 */

    if (insn & DW_CFA_advance_loc)
        *pc = (char *)*pc + (insn & 0x3f) * info->code_align;     /* mult3 */
    else if (insn & DW_CFA_offset) {
        reg = (insn & 0x3f);
        p = decode_uleb128(p, (unsigned *)&offset);
        offset *= info->data_align;
        state->s.saved[reg] = REG_SAVED_OFFSET;
        state->s.reg_or_offset[reg] = offset;                       /* sign-extended */
    } else if (insn & DW_CFA_restore) {
        /* unreachable - see QUIRK above */
        reg = (insn & 0x3f);
        state->s.saved[reg] = REG_UNSAVED;
    } else switch (insn) {
    case DW_CFA_set_loc:                                /* 0x010190CC */
        *pc = read_pointer(p);
        p += sizeof(void *);
        break;
    case DW_CFA_advance_loc1:                           /* 0x010190E0 */
        *pc = (char *)*pc + read_1byte(p);
        p += 1;
        break;
    case DW_CFA_advance_loc2:                           /* 0x010190F0 */
        *pc = (char *)*pc + read_2byte(p);
        p += 2;
        break;
    case DW_CFA_advance_loc4:                           /* 0x01019118 */
        *pc = (char *)*pc + read_4byte(p);
        p += 4;
        break;

    case DW_CFA_offset_extended:                        /* 0x01019134 */
        p = decode_uleb128(p, &reg);
        p = decode_uleb128(p, (unsigned *)&offset);
        offset *= info->data_align;
        state->s.saved[reg] = REG_SAVED_OFFSET;
        state->s.reg_or_offset[reg] = offset;
        break;
    case DW_CFA_restore_extended:                       /* 0x01019184 */
        p = decode_uleb128(p, &reg);
        state->s.saved[reg] = REG_UNSAVED;
        break;

    case DW_CFA_undefined:                              /* 0x010193AC */
    case DW_CFA_same_value:
    case DW_CFA_nop:
        break;

    case DW_CFA_register: {                             /* 0x010191A4 */
        unsigned reg2;              /* sp+8 */
        p = decode_uleb128(p, &reg);
        p = decode_uleb128(p, &reg2);
        state->s.saved[reg] = REG_SAVED_REG;
        state->s.reg_or_offset[reg] = reg2;                         /* zero-extended */
        break;
    }

    case DW_CFA_def_cfa:                                /* 0x010191EC */
        p = decode_uleb128(p, &reg);
        p = decode_uleb128(p, (unsigned *)&offset);
        state->s.cfa_reg = reg;
        state->s.cfa_offset = offset;                               /* sign-extended */
        break;
    case DW_CFA_def_cfa_register:                       /* 0x0101921C */
        p = decode_uleb128(p, &reg);
        state->s.cfa_reg = reg;
        break;
    case DW_CFA_def_cfa_offset:                         /* 0x01019238 */
        p = decode_uleb128(p, (unsigned *)&offset);
        state->s.cfa_offset = offset;
        break;

    case DW_CFA_remember_state: {                       /* 0x01019254 */
        /* malloc(0x2F8), no NULL check; memcpy inlined as an ld/sd loop. */
        struct frame_state_internal *save =
            (struct frame_state_internal *)malloc(sizeof(struct frame_state_internal));
        memcpy(save, state, sizeof(struct frame_state_internal));
        state->saved_state = save;
        break;
    }
    case DW_CFA_restore_state: {                        /* 0x010192BC */
        /* memcpy inlined; the copy also restores state->saved_state. */
        struct frame_state_internal *save = state->saved_state;
        memcpy(state, save, sizeof(struct frame_state_internal));
        free(save);
        break;
    }

    /* FIXME in gcc: hardcoded for the SPARC register-window layout. */
    case DW_CFA_GNU_window_save:                        /* 0x01019324 */
        for (reg = 16; reg < 32; ++reg) {
            state->s.saved[reg] = REG_SAVED_OFFSET;
            state->s.reg_or_offset[reg] = (reg - 16) * sizeof(void *);  /* zero-extended */
        }
        break;

    case DW_CFA_GNU_args_size:                          /* 0x01019388 */
        p = decode_uleb128(p, (unsigned *)&offset);
        state->s.args_size = offset;
        break;

    default:                                            /* 0x010193A4 */
        abort();
    }
    return p;
}

// FUNC 0x010193C8 __register_frame_info
//   Register the single FDE chain at BEGIN using caller-provided storage OB
//   (pushed on the front of `objects`; initialized lazily by find_fde).
//   No direct callers: only reached by __register_frame's tail-jump.  (The
//   mapping pass missed this start - it sits right after execute_cfa_insn's
//   epilogue.)
void __register_frame_info(void *begin, struct object *ob)
{
    ob->fde_begin = begin;
    ob->next = objects;
    objects = ob;
    ob->pc_end = 0;
    ob->pc_begin = 0;
    ob->fde_array = 0;
    ob->count = 0;
}

// FUNC 0x010193F0 __register_frame
//   malloc(0x18) a struct object (no NULL check) and tail-call
//   __register_frame_info.  No callers in the binary.
void __register_frame(void *begin)
{
    struct object *ob = (struct object *)malloc(sizeof(struct object));
    __register_frame_info(begin, ob);
}

// FUNC 0x01019420 __register_frame_info_table
//   Like __register_frame_info, but BEGIN is a NULL-terminated array of FDE
//   chain pointers: it is stored both as fde_begin (the deregistration key)
//   and as fde_array (walked by frame_init).  Called by crtbegin's
//   reg_frame (0x010000C8) with (0x01026C00, 0x01042048).
void __register_frame_info_table(void *begin, struct object *ob)
{
    ob->fde_array = begin;
    ob->next = objects;
    objects = ob;
    ob->fde_begin = begin;
    ob->pc_end = 0;
    ob->pc_begin = 0;
    ob->count = 0;
}

// FUNC 0x01019448 __register_frame_table
//   malloc(0x18) a struct object (no NULL check) and tail-call
//   __register_frame_info_table.  No callers in the binary.
void __register_frame_table(void *begin)
{
    struct object *ob = (struct object *)malloc(sizeof(struct object));
    __register_frame_info_table(begin, ob);
}

// FUNC 0x01019478 __deregister_frame_info
//   Unlink the object registered with BEGIN (matched on fde_begin) and return
//   it; if frame_init had run (pc_begin != 0) free its sorted fde_array first.
//   abort()s (tail-jump to 0x0101AFF0) when BEGIN was never registered.
//   Callers: crtbegin dtor 0x01000108, __deregister_frame.
struct object *__deregister_frame_info(void *begin)
{
    struct object **p;

    p = &objects;
    while (*p) {
        if ((*p)->fde_begin == begin) {
            struct object *ob = *p;
            *p = (*p)->next;

            /* If we've run init_frame for this object, free the FDE array. */
            if (ob->pc_begin)
                free(ob->fde_array);

            return ob;
        }
        p = &((*p)->next);
    }

    abort();
}

// FUNC 0x010194F0 __deregister_frame
//   free(__deregister_frame_info(begin)) - counterpart of __register_frame.
//   No callers in the binary.
void __deregister_frame(void *begin)
{
    free(__deregister_frame_info(begin));
}

// FUNC 0x01019510 __frame_state_for
//   Compute the unwind state for PC_TARGET into *STATE_IN and return
//   STATE_IN, or 0 when no FDE covers the pc or its CIE has an unknown
//   augmentation.  Runs the CIE's initial instructions, then the FDE's
//   instructions while the location counter is <= PC_TARGET (unsigned).
//   Frame: 0x3C0 bytes; info @sp+0, state @sp+0x20 (0x2F8), i @sp+0x320,
//   pc @sp+0x324.  memset is a real call (0x0101B1B8); the final 0x2F0-byte
//   memcpy is inlined as an ld/sd loop.  Callers: next_stack_level
//   0x010178E0, __throw 0x01017CA8, __rethrow 0x01017E88.
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
        int i;
        insn = decode_uleb128(insn, (unsigned *)&i);
        insn += i;
    }

    /* Then the insns in the FDE up to our target PC. */
    end = (unsigned char *)next_fde(f);
    pc = f->pc_begin;
    while (insn < end && pc <= pc_target)
        insn = execute_cfa_insn(insn, &state, &info, &pc);

    memcpy(state_in, &state.s, sizeof(state.s));
    return state_in;
}

// FUNC 0x010196E8 fde_split
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
        while (linear_max != (size_t)-1
               && fde_compare(linear->array[i], linear->array[linear_max]) < 0) {
            erratic->array[erratic->count++] = linear->array[linear_max];
            linear->array[linear_max] = NULL;
            linear_max = previous_max[linear_max];
        }
        previous_max[i] = linear_max;
        linear_max = i;
    }

    for (i = j = 0; i < count; i++)
        if (linear->array[i] != NULL)
            linear->array[j++] = linear->array[i];
    linear->count = j;
}
