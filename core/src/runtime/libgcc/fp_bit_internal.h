/* fp_bit_internal.h - GCC 2.9x-ee libgcc runtime, no-FDE stretch G006, part
 * 0x0027A400..0x0027C010 of the GT3 US main executable:
 *   fp_bit.c  fp-bit.o (config/fp-bit.c compiled for FLOAT), 0x0027A400..
 *   frame.c   frame.o  (gcc/frame.c, DWARF2 unwind-info registry),  0x0027AFE0..
 *
 * The same machine code as the bootstrap's runtime/libgcc_math.c (fp-bit part)
 * and runtime/frame.c.  The shared declarations are in the canonical headers:
 *   runtime/libgcc/G006_fp_bit.h          this translation unit's entry points used elsewhere
 *                               (fptoui, __make_fp, fptodp, __register_frame_info_table,
 *                               __deregister_frame_info, __frame_state_for)
 *   runtime/libgcc/G005_dp_bit.h          fp_class_type, __make_dp (0x00279C20); through it
 *   sdk/libcdvd/G004_libcdvd.h         the libgcc2.c types (SItype, USItype, UDItype, SFtype, ...)
 *   runtime/libgcc/T542_eh.h              struct frame_state, DWARF_FRAME_REGISTERS, REG_*
 *   runtime/startup/G000_collect2_ctors.h  struct object (the registration record)
 * This header keeps what only G006 uses: fp-bit's unpacked float, frame.c's
 * CIE / FDE / CFA-interpreter types, comments on the two .bss statics (defined
 * in the files) and the prototypes of the entry points nothing in the game calls.
 */
#ifndef U135_H
#define U135_H

#include "types.h"
#include "libc.h"       /* malloc 0x0026C3A8, free 0x0026C380, abort 0x0027D7A0, strcmp 0x0027EA40,
                           strlen (core 0x00269A68, the MMI strlen), memset 0x0027DB38 */
#include "runtime/libgcc/G006_fp_bit.h"           /* own TU; includes G005 (fp_class_type, __make_dp) and G004 */
#include "runtime/libgcc/T542_eh.h"               /* struct frame_state, DWARF_FRAME_REGISTERS, REG_* */
#include "runtime/startup/G000_collect2_ctors.h"   /* struct object */

/* ======================================================================== */
/* fp-bit.c (FLOAT) common definitions                                      */
/* ======================================================================== */

/* (The predicates isnan / isinf / iszero / flip_sign and nan() are fp-bit's
 * inline functions, in fp_bit.c.) */

/* Preserve the sticky-bit when shifting fractions to the right. */
#define LSHIFT(a) { a = (a & 1) | (a >> 1); }

/* Bit offset between the MSB of a float and a double mantissa, (52+8) - (23+7). */
#define F_D_BITOFF 30

#define NGARDS_F        7
#define GARDROUND_F     0x3f
#define GARDMASK_F      0x7f
#define GARDMSB_F       0x40
#define EXPBITS_F       8
#define EXPBIAS_F       127
#define FRACBITS_F      23
#define EXPMAX_F        0xff
#define QUIET_NAN_F     0x100000u       /* fp-bit's value (IEEE quiet bit would be 0x400000) */
#define FRAC_NBITS_F    32
#define NORMAL_EXPMIN_F (-(EXPBIAS_F) + 1)                           /* -126 */
#define IMPLICIT_1_F    ((USItype)1 << (FRACBITS_F + NGARDS_F))      /* 0x40000000 */
#define IMPLICIT_2_F    ((USItype)1 << (FRACBITS_F + 1 + NGARDS_F))  /* 0x80000000 */
#define FRACHIGH_F      0x80000000u

#define BITS_PER_SI     32
#define MAX_SI_INT      ((SItype)0x7fffffff)
#define MAX_USI_INT     ((USItype)~0)

/* Unpacked float (fp-bit fp_number_type, FLOAT flavour).  The member keeps
 * fp-bit's name `class' (this is C). */
typedef struct {
    fp_class_type class;        /* +0x00 */
    unsigned int  sign;         /* +0x04 */
    int           normal_exp;   /* +0x08 unbiased */
    union {
        USItype ll;             /* +0x0C 1.23 mantissa << 7 (implicit bit 30) */
        UHItype l[2];
    } fraction;
} fp_number_type_f;             /* 0x10 */
_Static_assert(sizeof(fp_number_type_f) == 0x10, "fp-bit fp_number_type size");

/* FLO_union_type.  The MIPS t-file defines FLOAT_BIT_ORDER_MISMATCH for
 * little-endian, so pack/unpack use the bitfield view (retail: each field
 * store masks and merges into the word). */
typedef union {
    SFtype  value;
    USItype value_raw;
    struct {
        USItype      fraction:FRACBITS_F __attribute__((packed));  /* bits 0..22  */
        unsigned int exp:EXPBITS_F __attribute__((packed));        /* bits 23..30 */
        unsigned int sign:1 __attribute__((packed));               /* bit 31      */
    } bits;
} FLO_union_type_f;
_Static_assert(sizeof(FLO_union_type_f) == 4, "fp-bit FLO_union_type size");

/* fp-bit nan()'s `static fp_number_type thenan;' (zero-initialised .bss,
 * i.e. {CLASS_SNAN, sign 0, exp 0, fraction 0}; packs to +qNaN) is the
 * function-local static of nan_f() in fp_bit.c: fp-bit.o's whole .bss,
 * .bss 0x004AD240, 0x10 bytes, 8-aligned (the toolchain's fp-bit.o: `b thenan.3'). */

/* fp-bit.o entry points nothing in the game calls (the EE FPU does single
 * precision in hardware); the used ones are in runtime/libgcc/G006_fp_bit.h. */
SFtype fpadd(SFtype arg_a, SFtype arg_b);   /* 0x0027A7D8 [__addsf3] */
SFtype fpsub(SFtype arg_a, SFtype arg_b);   /* 0x0027A830 [__subsf3] */
SFtype fpmul(SFtype arg_a, SFtype arg_b);   /* 0x0027A898 [__mulsf3] */
SFtype fpdiv(SFtype arg_a, SFtype arg_b);   /* 0x0027AA90 [__divsf3] */
int    fpcmp(SFtype arg_a, SFtype arg_b);   /* 0x0027AD08 [__cmpsf2] */
SFtype sitofp(SItype arg_a);                /* 0x0027AD58 [__floatsisf] */
SItype fptosi(SFtype arg_a);                /* 0x0027AE10 [__fixsfsi] */
SFtype __negsf2(SFtype arg_a);              /* 0x0027AF38 */

/* ======================================================================== */
/* frame.c                                                                  */
/* ======================================================================== */

/* DWARF 2 call-frame opcodes */
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

/* Low 6 bits of the three high-bit opcodes: the operand (delta / register). */
#define DW_CFA_OPERAND_MASK         0x3f

/* First fields of a CIE.  CIE_id is 0 for a CIE (distinguishes it from FDEs). */
struct dwarf_cie {
    u32  length;            /* +0x0 bytes following this word                 */
    s32  CIE_id;            /* +0x4 0                                         */
    u8   version;           /* +0x8                                           */
    /* (GNU [0]: egcs C has no C99 flexible array member [].) */
    char augmentation[0];   /* +0x9 NUL-terminated: "", "eh" or "z..."        */
} __attribute__((packed, aligned(4)));

/* First fields of an FDE (struct object holds `struct dwarf_fde *'). */
typedef struct dwarf_fde {
    u32   length;           /* +0x0 bytes following this word; 0 ends a chain */
    s32   CIE_delta;        /* +0x4 &CIE_delta - CIE; 0 means "this is a CIE" */
    void *pc_begin;         /* +0x8 first pc covered (0 = discarded linkonce) */
    u32   pc_range;         /* +0xC bytes covered                             */
} __attribute__((packed, aligned(4))) fde;
_Static_assert(sizeof(fde) == 0x10, "fde header is 16 bytes (f + 1 == instructions)");

/* What extract_cie_info pulls out of a CIE (on __frame_state_for's stack at sp+0). */
struct cie_info {
    char    *augmentation;  /* +0x00 */
    void    *eh_ptr;        /* +0x04 pointer following an "eh" augmentation  */
    int      code_align;    /* +0x08 uleb */
    int      data_align;    /* +0x0C sleb */
    unsigned ra_regno;      /* +0x10 one byte in the CIE */
};

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

/* Head of the registered-object list: frame.c's `static struct object
 * *objects;', a file static of frame.c (frame.o's whole .bss, 0x004AD250). */

/* frame.o entry points nothing in the game calls (crtbegin uses the table
 * forms, declared in runtime/libgcc/G006_fp_bit.h). */
void __register_frame_info(void *begin, struct object *ob);    /* 0x0027BB78 */
void __register_frame(void *begin);                            /* 0x0027BBA0 */
void __register_frame_table(void *begin);                      /* 0x0027BBF8 */
void __deregister_frame(void *begin);                          /* 0x0027BCA0 */

/* frame.c's .rodata: "" @0x003482F0, "eh" @0x003482F8, the 47-word jump table
 * of execute_cfa_insn @0x00348300 (written inline / as a switch). */

#endif /* U135_H */
