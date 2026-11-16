/* T535_vsprintf.cpp - vsprintf.cxx of Polyphony's pdistd: the anonymous-
 * namespace PrintTarget that writes into a caller buffer, and vsprintf.
 * Declarations: include/engine/pdistd/T535_vsprintf.h.
 *
 * The anonymous namespace is "_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r" (RTTI name
 * "Q235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTarget" @0x00347748,
 * __si_type_info node ti_vsprintf_PrintTarget @0x004B2B20).  The class is
 * local to this file, so it is declared here; its layout is documented in
 * T535_vsprintf.h.  No bounds check: like C sprintf, the caller's buffer must
 * be big enough.
 *
 * The method names are the overridden virtuals' (putc, the destructor); the
 * mangled names below follow from egcs's scheme and the printf.cxx twins
 * (the binary has no symbols for them).
 */
#include "engine/pdistd/pdistd_internal.h"
#include "engine/pdistd/T535_vsprintf.h"

/* The anonymous namespace's name, "_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r" in
 * the original (RTTI name @0x00347748): a zero-size public object with that
 * assembler name comes first, for the retail compiler only, so egcs derives
 * it instead of six random characters (see T533_printf.cpp's anchor). */
#ifdef BOOT_OLD_GCC
extern const char vsprintf_cxx_namespace_anchor[] __asm__(".._.._vsprintf.cxxUhxD9r") = {};
#endif

namespace {

/* String target.  sizeof == 8.
 *   +0x00 vptr = _vt$Q235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTarget @0x00347730
 *         [0] +0xC   ~PrintTarget  0x0026C798
 *         [1] +0x14  putc          0x0026C7C0 */
class PrintTarget : public pdistd::PrintFormatTargetBase {
public:
    PrintTarget(char *buffer);
    virtual ~PrintTarget();
    virtual int putc(int c);

private:
    char *cur_;         /* +0x04 next byte to write */
};

static_assert(sizeof(PrintTarget) == 8, "PrintTarget layout");

// FUNC 0x0026C750 PrintTarget::PrintTarget (anonymous namespace of vsprintf.cxx: __Q235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTargetPc; abi SPrintTarget_ctor)
// Base ctor (0x0026B1C8), cur = buffer, vptr = 0x00347730; returns this.
// Only caller: vsprintf.
PrintTarget::PrintTarget(char *buffer)
    : cur_(buffer)
{
    /* implicit: PrintFormatTargetBase (the base constructor) */
}

// FUNC 0x0026C798 PrintTarget::~PrintTarget (anonymous namespace of vsprintf.cxx: _$_Q235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTarget; abi SPrintTarget_dtor)
// Vtable slot 0: re-installs vptr 0x00347730 and calls the base
// destructor (0x0026B1E0) with the hidden flags unchanged.  vsprintf passes
// flags = 2.
PrintTarget::~PrintTarget()
{
    /* implicit: ~PrintFormatTargetBase (the base destructor) */
}

// FUNC 0x0026C7C0 PrintTarget::putc (anonymous namespace of vsprintf.cxx: putc__Q235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTargeti; abi SPrintTarget_put)
// Vtable slot 1: *cur++ = (char)c; returns 0 (never fails).
int PrintTarget::putc(int c)
{
    *cur_++ = (char)c;
    return 0;
}

} /* anonymous namespace */

// FUNC 0x0026C7D8 vsprintf
// Formats into buf through a stack PrintTarget (sp+0x00), then writes the
// terminating NUL with a direct (non-virtual) putc(0), destroys the target
// (flags 2) and returns PrintFormat's count (without the NUL).  The EH
// landing pads only run the destructor and resume with __throw (0x00278268),
// std::terminate (0x00279CA8) if that throws.  Callers: sprintf, 0x001D26B0.
extern "C" int vsprintf(char *buf, const char *format, va_list args)
{
    PrintTarget target(buf);
    int n;

    n = pdistd::PrintFormat(&target, format, args);
    target.putc(0);
    return n;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (the symbol name has '$' and '.', so
 * it is written under a C name). */
// FUNC 0x0026C868 __tfQ235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTarget
// Builds the __si_type_info node @0x004B2B20 on the first call (base
// typeinfo function first, its result unused).  Vtable entry 0x00347734.
extern "C" void *tf_vsprintf_PrintTarget(void)
{
    if (ti_vsprintf_PrintTarget[0] == 0) {
        __tfQ26pdistd21PrintFormatTargetBase();
        __rtti_si(ti_vsprintf_PrintTarget, "Q235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTarget",
                  (const type_info *)__tiQ26pdistd21PrintFormatTargetBase);
    }
    return ti_vsprintf_PrintTarget;
}
#endif
