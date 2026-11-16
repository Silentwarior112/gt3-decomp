/* T533_printf.cpp - printf.cxx of Polyphony's pdistd: the anonymous-namespace
 * PrintTarget (a 256-byte line buffer), vprintf and printf.  Declarations:
 * include/engine/pdistd/T533_printf.h (vprintf in libc.h).
 *
 * Same machine code as the bootstrap's src/pdistd/printf.cpp
 * (0x010114A8..0x01011700); the source below is that file's, with the core
 * addresses.  Output path: printf -> vprintf -> pdistd::PrintFormat(&target)
 * -> PrintTarget::putc -> (full / end) PrintTarget::flush -> printf_to_deci2
 * (0x0026E3D0, the bootstrap's tty_puts: scePrintf("%s", buf)).
 *
 * The anonymous namespace is "_GLOBAL_$N$.._.._printf.cxxQakP7f" (RTTI name
 * "Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget" @0x003476F8,
 * __si_type_info node ti_printf_PrintTarget @0x004B2B10).  The class is
 * local to this file, so it is declared here; its layout is documented in
 * T533_printf.h.
 */
#include "engine/pdistd/pdistd_internal.h"
#include "engine/pdistd/T533_printf.h"

/* The anonymous namespace's name (as the bootstrap's src/pdistd/printf.cpp,
 * whose comment has the details): egcs names `namespace {` after the first
 * public symbol already assembled in the file, else after the input file
 * name plus six random characters.  The original got
 * "_GLOBAL_$N$.._.._printf.cxxQakP7f" (RTTI name @0x003476F8, every
 * PrintTarget symbol).  For the retail compiler only, a zero-size public
 * object with exactly that assembler name comes first, so egcs derives the
 * original's name deterministically; it emits no bytes (a label in front of
 * the vtable, which is 8-byte aligned anyway).  GCC 14 does not need it. */
#ifdef BOOT_OLD_GCC
extern const char printf_cxx_namespace_anchor[] __asm__(".._.._printf.cxxQakP7f") = {};
#endif

namespace {

/* Line-buffered console target.  sizeof == 0x10.
 *   +0x00 vptr = _vt$Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget @0x003476E0
 *         [0] +0xC   ~PrintTarget  0x0026C520
 *         [1] +0x14  putc          0x0026C548 */
class PrintTarget : public pdistd::PrintFormatTargetBase {
public:
    PrintTarget(char *buffer, int size);
    virtual ~PrintTarget();
    virtual int putc(int c);
    void flush();

private:
    char *buffer_;      /* +0x04 */
    int   capacity_;    /* +0x08: size - 1 (one byte kept for the NUL) */
    int   length_;      /* +0x0C */
};

static_assert(sizeof(PrintTarget) == 0x10, "PrintTarget layout");

// FUNC 0x0026C4C0 PrintTarget::PrintTarget (anonymous namespace of printf.cxx: __Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTargetPci; abi PrintTarget_ctor)
// Base ctor (0x0026B1C8), vptr = 0x003476E0, buffer, capacity = size - 1,
// length = 0; returns this.  Only caller: vprintf (size 0x100).
PrintTarget::PrintTarget(char *buffer, int size)
    : buffer_(buffer), capacity_(size - 1), length_(0)
{
    /* implicit: PrintFormatTargetBase (the base constructor) */
}

// FUNC 0x0026C520 PrintTarget::~PrintTarget (anonymous namespace of printf.cxx: _$_Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget; abi PrintTarget_dtor)
// Vtable slot 0: re-installs vptr 0x003476E0 and calls the base destructor
// (0x0026B1E0) with the caller's hidden flags unchanged (the base does the
// operator delete when bit 0 is set).  vprintf passes flags = 2.
PrintTarget::~PrintTarget()
{
    /* implicit: ~PrintFormatTargetBase (the base destructor) */
}

// FUNC 0x0026C548 PrintTarget::putc (anonymous namespace of printf.cxx: putc__Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTargeti)
// Vtable slot 1 - the sink PrintFormat writes through.  Flushes first when the
// buffer is full (length == capacity), then stores the low byte of c.  Never
// fails: always returns 0.
int PrintTarget::putc(int c)
{
    if (length_ == capacity_)
        flush();
    buffer_[length_++] = (char)c;
    return 0;
}

// FUNC 0x0026C5A8 PrintTarget::flush (anonymous namespace of printf.cxx: flush__Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget; abi PrintTarget_flush)
// If anything is buffered: NUL-terminate, hand the text to printf_to_deci2
// (0x0026E3D0), reset the length.
void PrintTarget::flush()
{
    if (length_ != 0) {
        buffer_[length_] = '\0';
        printf_to_deci2(buffer_);
        length_ = 0;
    }
}

} /* anonymous namespace */

// FUNC 0x0026C5F0 vprintf
// Formats into a stack PrintTarget over a 256-byte buffer (sp+0x000; the
// target object is at sp+0x100), flushes the tail, destroys the target
// (~PrintTarget(2)) and returns PrintFormat's result (-1 on sink failure).
// The EH landing pads only run ~PrintTarget(2) and resume with __throw
// (0x00278268); a throw from that destructor ends in std::terminate
// (0x00279CA8).  Caller: printf.
extern "C" int vprintf(const char *format, va_list args)
{
    char buffer[256];
    PrintTarget target(buffer, sizeof(buffer));
    int n;

    n = pdistd::PrintFormat(&target, format, args);
    target.flush();
    return n;
}

// FUNC 0x0026C690 printf
// Varargs shim: spills $a1..$a7 as 8-byte slots at sp+0x58..0x88 (the va_list
// area) and calls vprintf(format, sp+0x58).  16 callers (debug output of the
// game, Allocator::debug, Sony libpad / libmc / libkernl).
extern "C" int printf(const char *format, ...)
{
    va_list args;
    int n;

    va_start(args, format);
    n = vprintf(format, args);
    va_end(args);                               /* no code in the original */
    return n;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (the symbol name has '$' and '.', so
 * it is written under a C name). */
// FUNC 0x0026C6C8 __tfQ233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget
// Builds the __si_type_info node @0x004B2B10 on the first call: the base's
// typeinfo function first (its result unused; the base node is re-addressed
// from $gp, 0x003534F8), then __rtti_si.  Vtable entry 0x003476E4.
extern "C" void *tf_printf_PrintTarget(void)
{
    if (ti_printf_PrintTarget[0] == 0) {
        __tfQ26pdistd21PrintFormatTargetBase();
        __rtti_si(ti_printf_PrintTarget, "Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget",
                  (const type_info *)__tiQ26pdistd21PrintFormatTargetBase);
    }
    return ti_printf_PrintTarget;
}
#endif
