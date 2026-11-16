/* T534_sprintf.cpp - sprintf (a translation unit of its own, no EH regions).
 * Declaration: include/engine/pdistd/T534_sprintf.h. */
#include "engine/pdistd/pdistd_internal.h"
#include "engine/pdistd/T534_sprintf.h"

// FUNC 0x0026C718 sprintf
// Varargs shim: spills $a2..$a7 as 8-byte slots at sp+0x50..0x78 (the va_list
// area) and returns vsprintf(buf, format, sp+0x50).  136 callers.
extern "C" int sprintf(char *buf, const char *format, ...)
{
    va_list args;
    int n;

    va_start(args, format);
    n = vsprintf(buf, format, args);
    va_end(args);                               /* no code in the original */
    return n;
}
