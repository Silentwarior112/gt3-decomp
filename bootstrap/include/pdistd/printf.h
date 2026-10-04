/* pdistd/printf.h - Polyphony pdistd printf engine of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102).  Unit "printf", three translation units of the
 * original, in link order:
 *   src/pdistd/printf_tables.cpp  the number tables (data only)
 *   src/pdistd/printformat.cpp    PrintFormatTargetBase + PrintFormat
 *   src/pdistd/printf.cpp         printf.cxx: PrintTarget, vprintf, printf
 * (the allocator's malloc/free front-ends sit between the last two).
 *
 * printf() / vprintf() themselves (C linkage; they override libc's and are also
 * called by Sony libpad / libkernl) are declared in libc.h.
 *
 * Output path:  printf -> vprintf -> pdistd::PrintFormat(&PrintTarget, ...)
 *               -> PrintTarget::putc (256-byte line buffer)
 *               -> PrintTarget::flush -> tty_puts -> scePrintf("%s", buf)
 *               (libkernl kernel/TTY printf, eekernel.h).
 */
#ifndef BOOT_PDISTD_PRINTF_H
#define BOOT_PDISTD_PRINTF_H

#include "types.h"
#include <stdarg.h>

#ifdef __cplusplus
namespace pdistd {

/* Abstract-ish character sink the format engine writes to.  sizeof == 4.
 *   +0x00 vptr = _vt$Q26pdistd21PrintFormatTargetBase @0x0103FC60 (.rodata)
 * GCC 2.9x vtable, 8-byte entries {s16 delta, s16 index, pfn}:
 *   [0] +0x00/+0x04  __tfQ26pdistd21PrintFormatTargetBase   0x010111D0
 *   [1] +0x08/+0x0C  ~PrintFormatTargetBase (int flags)      0x01010330
 *   [2] +0x10/+0x14  putc(int)  (default: return 0)          0x01010360
 * RTTI name "Q26pdistd21PrintFormatTargetBase" @0x0103FC78; its
 * __user_type_info object lives at 0x01041EA8 (.sbss).
 * The engine calls slot [2] as pfn(this + delta, ch) and treats a return of
 * -1 as a sink failure (PrintFormat then returns -1). */
class PrintFormatTargetBase {
public:
    PrintFormatTargetBase();                    /* 0x01010318 */
    virtual ~PrintFormatTargetBase();           /* 0x01010330 */
    virtual int putc(int c);                    /* 0x01010360 */
};

/* The %-format interpreter (0x01010368).  `args` is the EE va_list: a pointer
 * to consecutive 8-byte argument slots.  Returns the number of characters
 * produced (see the quirks in printf.cpp: %e over-counts by one) or -1 if the
 * target's putc returned -1.  Sole caller: vprintf. */
int PrintFormat(PrintFormatTargetBase *target, const char *format, va_list args);

} /* namespace pdistd */

/* PrintFormat's number tables (src/pdistd/printf_tables.cpp, .rodata,
 * referenced by absolute address only from PrintFormat). */
extern const u64  s_pow10_double[635];  /* @0x0103E6A8: bits of ~10^(i-324), + 2 zero words */
extern const u64  s_pow10_u64[20];      /* @0x0103FA80: 10^0..10^18, + a zero word */
extern const char s_digits[32];         /* @0x0103FB20: "0123456789abcdef0123456789ABCDEF" */
#endif /* __cplusplus */

/* PrintTarget flush backend (0x01012FC8): scePrintf("%s", s).  Lives in the
 * non-EH pdistd helper object after GZIP in .text, C linkage assumed. */
EXTERN_C void tty_puts(const char *s);

#endif /* BOOT_PDISTD_PRINTF_H */
