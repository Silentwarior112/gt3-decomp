/* collect2_ctors.c - the constructor / destructor / EH-frame table file
 * that GCC 2.9x's collect2 generated at link time (write_c_file_glob) and
 * linked right after crt0.o: .text 0x001000C8..0x0010013F (unit u000, G000).
 *
 * The same code as the bootstrap's src/crtbegin.c (0x010000C8..0x0100013F);
 * that file's header gives the evidence that this is collect2's output and
 * not crtstuff: count-headed lists, frame (de)registration as the last list
 * entry, __register_frame_info_table on a table of per-object .eh_frame
 * pointers, unoptimised ($fp frame) code, and __main_reference.
 *
 * The rest of collect2's file is data, which this tree does not define
 * (frame_table and ob are externs in runtime/startup/G000_collect2_ctors.h).  In this
 * binary:
 *   frame_table       0x0028E780  551 pointers, one per C++ translation unit's
 *                                 .eh_frame block (0x0028F2AC .. 0x00306850),
 *                                 then 0 at 0x0028F01C
 *   __CTOR_LIST__     0x0028F020  count 110 = 109 static constructors + reg_frame;
 *                                 first entry 0x001012C0, reg_frame last
 *                                 (0x0028F1D8), 0 at 0x0028F1DC
 *   __DTOR_LIST__     0x0028F1E0  count 49 = 48 static destructors + dereg_frame;
 *                                 first entry 0x001022D0, dereg_frame last
 *                                 (0x0028F2A4), 0 at 0x0028F2A8
 *   __main_reference  0x00350B00  (.sdata, gp-0x6EF0) = &__main (0x00276FB0)
 *   ob                0x003536C8  (.bss, 0x18 bytes, reg_frame_ob) right after
 *                                 crt0's _args
 *
 * __do_global_ctors (0x00276F00, tail-called by __main, which main calls
 * first) walks __CTOR_LIST__ backwards, so reg_frame is the FIRST static
 * constructor to run: the unwind info is registered before any other
 * constructor could throw.  The only reader of __DTOR_LIST__ is libgcc2's
 * __do_global_dtors (0x00276EA8, through its `static func_ptr *p' at
 * 0x00305EB8 = __DTOR_LIST__ + 1), which has no callers and no data
 * references, and __do_global_ctors does not hand __do_global_dtors to
 * atexit (0x0027D7F8) either: dereg_frame never runs.
 *
 * reg_frame / dereg_frame keep collect2's `static' (runtime/startup/G000_collect2_ctors.h
 * documents them, but does not declare them); they are marked used because
 * the lists that reference them are not written in this tree.
 */
#include "runtime/startup/G000_collect2_ctors.h"
#include "runtime/startup/startup_internal.h"

/* collect2's `reg_frame': register the program's unwind info with frame.c,
 * __register_frame_info_table(frame_table 0x0028E780, &ob 0x003536C8)
 * (0x0027BBD0).  Last entry of __CTOR_LIST__.  Unoptimised code ($fp frame). */
// FUNC 0x001000C8 reg_frame
static BOOT_USED void reg_frame(void)
{
    /* collect2: `static struct object ob;' here; the extern reg_frame_ob in
     * runtime/startup/G000_collect2_ctors.h stands for it */
    __register_frame_info_table(frame_table, &reg_frame_ob);
}

/* collect2's `dereg_frame': __deregister_frame_info(frame_table) (0x0027BC28);
 * the returned object pointer is ignored.  Last entry of __DTOR_LIST__; dead
 * in this binary (see above).  Unoptimised code ($fp frame). */
// FUNC 0x00100108 dereg_frame
static BOOT_USED void dereg_frame(void)
{
    __deregister_frame_info(frame_table);
}
