/* Linker script for the rebuilt GT3 US bootstrap (SCUS_971.02).
 *
 * Same memory map as the retail file: one PT_LOAD at 0x01000000, entry
 * _start (crt0.o first, so _start lands at 0x01000008 like the original),
 * .sbss/.bss cleared by crt0 from _fbss to _end, heap from _end up to the
 * 1 MB main-thread stack at 0x01F00000..0x02000000.
 *
 * The retail image loads core.gt3 to 0x00100000..~0x004B2C0C while this code
 * runs, so everything here must stay above that - 0x01000000 does.
 */
ENTRY(_start)

PHDRS
{
    boot PT_LOAD FLAGS(7);
}

SECTIONS
{
    . = 0x01000000;

    .text : {
        KEEP(*crt0.o(.text))
        KEEP(*crt0.S.o(.text))
        *(.text .text.* .stub .gnu.linkonce.t.*)
    } :boot

    . = ALIGN(128);
    .rodata : {
        *(.rodata .rodata.* .rodata1 .rdata .gnu.linkonce.r.*)
    } :boot

    /* egcs's old-model EH: per-function exception tables, referenced from
     * .data (retail: .gcc_except_table at 0x01041000).  GCC 14 objects are
     * built without exceptions and contribute nothing. */
    .gcc_except_table : {
        *(.gcc_except_table .gcc_except_table.*)
    } :boot

    . = ALIGN(128);
    .data : {
        *(.data .data.* .data1 .gnu.linkonce.d.*)
    } :boot

    . = ALIGN(16);
    .sdata : {
        _fsdata = .;
        *(.lit8 .lit4 .sdata .sdata.* .gnu.linkonce.s.*)
    } :boot
    /* No -G0 code is gp-relative; _gp only has to be the conventional value
     * handed to the kernel (InitMainThread / thread params). */
    _gp = _fsdata + 0x7FF0;

    . = ALIGN(16);
    _fbss = .;
    .sbss (NOLOAD) : { *(.sbss .sbss.* .scommon .gnu.linkonce.sb.*) } :boot
    .bss (NOLOAD) : { *(.bss .bss.* COMMON .gnu.linkonce.b.*) } :boot
    . = ALIGN(16);
    _end = .;

    /* crt0 parameters: the main-thread stack is the top 1 MB of RAM, and the
     * heap runs from _end to it (InitHeap(_end, -1)). */
    _stack      = 0x01F00000;
    _stack_size = 0x00100000;
    _heap_size  = -1;

    /* Double-precision soft float.  The EE's libgcc was built with GOFAST
     * names, and the original compiler called dpmul/dpadd/... directly; GCC 14
     * emits the standard libcall names instead, so they are pointed at the
     * decompiled fp-bit routines (runtime/libgcc_math.c).  Only comparisons
     * whose NaN result agrees with fp-bit's compare() (NaN -> 1) are mapped:
     * __gtdf2/__gedf2 expect NaN -> -1 and are deliberately absent. */
    PROVIDE(__adddf3      = dpadd);
    PROVIDE(__subdf3      = dpsub);
    PROVIDE(__muldf3      = dpmul);
    PROVIDE(__divdf3      = dpdiv);
    PROVIDE(__cmpdf2      = dpcmp);
    PROVIDE(__eqdf2       = dpcmp);
    PROVIDE(__nedf2       = dpcmp);
    PROVIDE(__ltdf2       = dpcmp);
    PROVIDE(__ledf2       = dpcmp);
    PROVIDE(__floatsidf   = litodp);
    PROVIDE(__fixdfsi     = dptoli);
    PROVIDE(__fixunsdfsi  = dptoul);
    PROVIDE(__extendsfdf2 = fptodp);
    PROVIDE(__truncdfsf2  = dptofp);

    /* Nothing here is loaded by the BIOS or used at run time. Constructor
     * lists are explicit in src/crtbegin.c; tools/build.py refuses objects
     * that carry .ctors/.init_array so none can be dropped silently here. */
    /DISCARD/ : {
        *(.MIPS.abiflags) *(.MIPS.options) *(.reginfo) *(.pdr)
        *(.comment) *(.gnu.attributes) *(.note .note.*)
        *(.eh_frame .eh_frame_hdr)
        *(.mdebug .mdebug.*)
    }
}
