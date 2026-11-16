/* deci2_part1.c - Sony libkernl 2240 deci2.o: sceDeci2Open
 * (0x0028D778..0x0028D7C0; the rest of deci2.o follows in the next unit).
 * Same code as the bootstrap's sdk/libkernl/deci2_tty.c.
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part2_internal.h"

/* deci2.o's .bss (right after iopreboot.o's packet, the last object .bss
 * before the commons): the kernel-side status words.  A file static (as the
 * bootstrap's deci2_tty.c); only sceDeci2Open refers to it.                */
static u32 s_deci2_kstatus[4];                                  /* 0x004B11A8 .bss */

// FUNC 0x0028D778 sceDeci2Open
//   Deci2Call(DECI2_CALL_OPEN, {protocol, opt, handler, uncached &kernel
//   status}).  Returns the socket (>= 0) or a negative error.  The only
//   caller is the DECI2 TTY driver's _deci2_tty_init (u140).
int sceDeci2Open(unsigned short protocol, void *opt,
                 void (*handler)(int event, int param, void *opt))
{
    u32 param[4];

    param[0] = protocol;
    param[1] = (u32)opt;
    param[2] = (u32)handler;
    param[3] = (u32)UNCACHED(s_deci2_kstatus);
    return Deci2Call(DECI2_CALL_OPEN, param);
}
