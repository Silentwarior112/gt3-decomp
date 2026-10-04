/* crtbegin.c - constructor / destructor / EH-frame tables of the GT3 US
 * bootstrap (BOOTSTRAP_SCUS-97102), .text 0x010000C8..0x0100013F,
 * .data 0x01026C00..0x01026CC7, .sdata 0x01041D80, .bss 0x01042048.
 *
 * What this object really is
 * --------------------------
 * The mapping pass took the two functions here for crtstuff's frame_dummy
 * and its deregistration counterpart.  They are not: this is the C file that
 * GCC 2.9x's collect2 GENERATES at link time (write_c_file_glob in
 * collect2.c) on targets without an .init section, then compiles and links
 * in right after the first object (crt0.o).  Evidence, all from the binary:
 *
 *   - __CTOR_LIST__ / __DTOR_LIST__ carry an explicit count (3), not -1.
 *     collect2 writes "constructors.number + frames" there; crtstuff-style
 *     lists use -1.  (The Lupo 2 game ELF, linked with a .ctors section, has
 *     -1 heads.)
 *   - the last entry of each list is the frame (de)registration function,
 *     exactly collect2's `reg_frame` / `dereg_frame`;
 *   - registration uses __register_frame_info_table on a NULL-terminated
 *     table of pointers to the per-object .eh_frame blocks (one
 *     `_GLOBAL_$F$<obj>` label per C++ object; GCC 2.9x emitted .eh_frame into
 *     .data because the target had no EH_FRAME_SECTION).  That is collect2's
 *     `frame_table`; the table order is nm's alphabetical symbol order, not
 *     link order;
 *   - both functions are unoptimised code ($fp frame: collect2 compiles its
 *     file without -O);
 *   - the .sdata word 0x01041D80 = &__main is collect2's `__main_reference`;
 *   - placement: .text right after crt0, .data at the very start of .data,
 *     `static struct object ob` (0x18 bytes) in .bss right after crt0's
 *     _args block (0x01041F00 + 0x144, aligned to 8 = 0x01042048).
 *
 * __do_global_ctors (libgcc2, 0x01017030, reached from __main at the top of
 * main) walks __CTOR_LIST__ backwards: reg_frame first, then the allocator's
 * and filename.cxx's static constructors.  Nothing ever runs the dtor list
 * (the bootstrap leaves through ExecPS2).
 *
 * Link note: the ctor/dtor list entries are all defined in the tree
 * (global_ctor/dtor_filename in src/filename.cpp, global_ctor/dtor_allocator
 * in src/pdistd/allocator.cpp, reg_frame/dereg_frame here; the lists are
 * declared for libgcc2's __do_global_ctors/__do_global_dtors in cxxrt.h).
 * The 36 eh_frame_XXXXXXXX symbols of frame_table, however, stand for the
 * per-object `_GLOBAL_$F$` .eh_frame labels that GCC 2.9x emitted into .data;
 * no C source can define them (a modern toolchain generates and registers
 * .eh_frame itself), so they stay undefined externs: this table documents
 * the original link and is compile-checked only.
 */
#include "types.h"
#include "cxxrt.h"              /* __CTOR_LIST__ / __DTOR_LIST__, __main, frame.c registry */
#include "filename.h"           /* global_ctor_filename / global_dtor_filename */
#include "pdistd/allocator.h"   /* global_ctor_allocator / global_dtor_allocator */

typedef void entry_pt(void);

/* ------------------------------------------------------------------------ */
/* frame_table @0x01026C00 (.data): the per-object .eh_frame blocks          */
/* ------------------------------------------------------------------------ */
/* Each name stands for one object's `_GLOBAL_$F$...` label (the CIE at the
 * start of its .eh_frame).  Comment: text range covered by that CIE's FDEs.
 * "eh" = the CIE has the "eh" augmentation (the object has an exception
 * table in .gcc_except_table).                                             */
extern void *eh_frame_01026CD8;   /* eh  0x01000140..0x0100078C main.cxx (select_boot_source, main) */
extern void *eh_frame_01026E44;   /*     0x01000790..0x01000A4C IOP boot / pad (src/main_iop.cpp)   */
extern void *eh_frame_01026F1C;   /*     0x01000A50..0x01000ED4 startup screen                      */
extern void *eh_frame_01027034;   /*     0x01000ED8..0x010010EC buffer.cxx                          */
extern void *eh_frame_01027170;   /*     0x010010F0..0x01003570 SHA-512                             */
extern void *eh_frame_01027308;   /*     0x01003570..0x010041BC filename.cxx                        */
extern void *eh_frame_01027978;   /* eh  0x010041C0..0x01004BF4 imageloader.cxx                     */
extern void *eh_frame_01027BDC;   /* eh  0x01004BF8..0x01004EEC hostsource                          */
extern void *eh_frame_01027CE0;   /* eh  0x01004EF0..0x010053C4 disksource                          */
extern void *eh_frame_01027DF8;   /* eh  0x010053C8..0x0100596C mcardsource                         */
extern void *eh_frame_01027F20;   /*     no FDEs (an object without functions)                      */
extern void *eh_frame_01033C94;   /*     0x01005970..0x01005BC0 host: file I/O                      */
extern void *eh_frame_01033E50;   /*     0x01007148..0x01007784 BigNumRep                           */
extern void *eh_frame_0103445C;   /* eh  0x01008FC0..0x0100B2E8 Montgomery / modpow                 */
extern void *eh_frame_01033D80;   /* eh  0x01005BC0..0x01007148 BigNum                              */
extern void *eh_frame_0103401C;   /* eh  0x01007788..0x01007988 BigNumRep::addMag                   */
extern void *eh_frame_010340B0;   /* eh  0x01007988..0x01007B6C BigNumRep::subMag                   */
extern void *eh_frame_01034144;   /* eh  0x01007B70..0x010081B8 BigNumRep::mulMagTrunc              */
extern void *eh_frame_0103426C;   /* eh  0x010081B8..0x01008B98 BigNumRep::divmod                   */
extern void *eh_frame_01034390;   /*     0x01008B98..0x01008D7C shr1                                */
extern void *eh_frame_010343E8;   /*     0x01008D80..0x01008FC0 shl1                                */
extern void *eh_frame_01039940;   /*     0x01011210..0x010114A4 heap front-ends (allocator unit)    */
extern void *eh_frame_0103A184;   /*     0x01011700..0x01012F34 GZIP inflate                        */
extern void *eh_frame_01039540;   /*     0x0100F918..0x01010314 pdistd::Allocator                   */
extern void *eh_frame_01039B24;   /* eh  0x010114A8..0x010116FC printf.cxx PrintTarget              */
extern void *eh_frame_01039860;   /*     0x01010318..0x0101120C pdistd::PrintFormat                 */
extern void *eh_frame_01039848;   /*     no FDEs (an object without functions)                      */
extern void *eh_frame_0103BE9C;   /*     0x01017100..0x0101806C libgcc2 (L_eh)                      */
extern void *eh_frame_0103C490;   /* eh  0x010180D8..0x01018830 exception.cc                        */
extern void *eh_frame_0103C8C8;   /* eh  0x01019910..0x01019988 operator delete                     */
extern void *eh_frame_0103C910;   /* eh  0x01019988..0x01019AB0 operator new                        */
extern void *eh_frame_0103C964;   /* eh  0x01019AB0..0x01019B28 operator delete[]                   */
extern void *eh_frame_0103C9AC;   /* eh  0x01019B28..0x01019B9C operator new[]                      */
extern void *eh_frame_0103C9F4;   /*     0x01019BA0..0x0101A270 tinfo.cc                            */
extern void *eh_frame_0103CE00;   /*     0x0101A270..0x0101AFEC tinfo2.cc                           */
extern void *eh_frame_0103C828;   /*     0x01019860..0x01019910 new.cc (set_new_handler)            */

static void *frame_table[] = {                 /* @0x01026C00, 36 entries + 0 */
    &eh_frame_01026CD8, &eh_frame_01026E44, &eh_frame_01026F1C, &eh_frame_01027034,
    &eh_frame_01027170, &eh_frame_01027308, &eh_frame_01027978, &eh_frame_01027BDC,
    &eh_frame_01027CE0, &eh_frame_01027DF8, &eh_frame_01027F20, &eh_frame_01033C94,
    &eh_frame_01033E50, &eh_frame_0103445C, &eh_frame_01033D80, &eh_frame_0103401C,
    &eh_frame_010340B0, &eh_frame_01034144, &eh_frame_0103426C, &eh_frame_01034390,
    &eh_frame_010343E8, &eh_frame_01039940, &eh_frame_0103A184, &eh_frame_01039540,
    &eh_frame_01039B24, &eh_frame_01039860, &eh_frame_01039848, &eh_frame_0103BE9C,
    &eh_frame_0103C490, &eh_frame_0103C8C8, &eh_frame_0103C910, &eh_frame_0103C964,
    &eh_frame_0103C9AC, &eh_frame_0103C9F4, &eh_frame_0103CE00, &eh_frame_0103C828,
    0
};

/* The registration record, as collect2 declares it ("This must match what's
 * in frame.h"); same layout as runtime/frame.c's struct object.            */
struct object {
    void          *pc_begin;    /* +0x00 */
    void          *pc_end;      /* +0x04 */
    void          *fde_begin;   /* +0x08 */
    void          *fde_array;   /* +0x0C */
    size_t         count;       /* +0x10 */
    struct object *next;        /* +0x14 */
};
_Static_assert(sizeof(struct object) == 0x18, "struct object");

// FUNC 0x010000C8 reg_frame
//   collect2's `reg_frame` (the mapping pass's "frame_dummy"): register the
//   program's unwind info with frame.c,
//   __register_frame_info_table(frame_table 0x01026C00, &ob 0x01042048)
//   (0x01019420).  Last entry of __CTOR_LIST__, so the FIRST constructor
//   __do_global_ctors runs.  Unoptimised code ($fp frame).
static void reg_frame(void)
{
    static struct object ob;    /* @0x01042048 (.bss) */

    __register_frame_info_table(frame_table, &ob);
}

// FUNC 0x01000108 dereg_frame
//   collect2's `dereg_frame` (the mapping pass's
//   "__deregister_frame_info_dtor"): __deregister_frame_info(frame_table)
//   (0x01019478); the returned object pointer is ignored.  Last entry of
//   __DTOR_LIST__; never runs in practice (__do_global_dtors has no callers).
static void dereg_frame(void)
{
    __deregister_frame_info(frame_table);
}

/* filename.cxx's static constructor / destructor (_GLOBAL_$I$ / _GLOBAL_$D$,
 * 0x01003CC0 / 0x01003C90) are global_ctor_filename / global_dtor_filename,
 * defined in src/filename.cpp and declared in filename.h.                  */

/* Word 0 is the entry count (constructors + 1 for reg_frame); the list is
 * 0-terminated.  Declared in cxxrt.h (used by runtime/libgcc2.c).          */
entry_pt *__CTOR_LIST__[] = {           /* @0x01026C98 (.data) */
    (entry_pt *)3,
    global_ctor_filename,               /* 0x01003CC0 */
    global_ctor_allocator,              /* 0x01011480 */
    reg_frame,                          /* 0x010000C8 */
    0
};

entry_pt *__DTOR_LIST__[] = {           /* @0x01026CB0 (.data) */
    (entry_pt *)3,
    global_dtor_filename,               /* 0x01003C90 */
    global_dtor_allocator,              /* 0x01011458 */
    dereg_frame,                        /* 0x01000108 */
    0
};

/* Forces libgcc2's __main (and with it __do_global_ctors) into the link. */
entry_pt *__main_reference = __main;    /* @0x01041D80 (.sdata) */
