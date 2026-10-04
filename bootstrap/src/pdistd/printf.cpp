/* printf.cpp - printf.cxx of Polyphony's pdistd in the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102): the anonymous-namespace PrintTarget (a 256-byte
 * line buffer), vprintf and printf.  Unit "printf".
 *
 * One translation unit of the original (tools/layout.py): .text
 * 0x010114A8..0x01011700, its own EH frame block (0x01039B24..0x01039C70,
 * with exception regions: vprintf's cleanup), .rodata 0x0103FCA0..0x0103FCEB
 * (PrintTarget's vtable and type name), .gcc_except_table 0x01041B74.. and the
 * typeinfo node in .bss (0x0104DDA8).  The file name is the original's: the
 * RTTI name "Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget" carries it.
 * The format engine is another object (src/pdistd/printformat.cpp, linked
 * before the allocator front-ends that separate the two in .text).
 *
 * Output path: printf -> vprintf -> PrintFormat(&PrintTarget) -> PrintTarget::putc
 *   -> (buffer full / end) PrintTarget::flush -> tty_puts -> scePrintf("%s", buf)
 * (tty_puts 0x01012FC8 lives in the non-EH helper object after GZIP,
 * src/pdistd/pdistd_misc.cpp).
 */
#include "types.h"
#include "libc.h"       /* printf / vprintf prototypes (C linkage) */
#include "pdistd/printf.h"

/* The anonymous namespace's name.  egcs names it, when `namespace {` is first
 * seen, after the first public symbol already assembled in the file
 * (first_global_object_name), else after the input file name plus six random
 * characters (get_file_function_name_long / append_random_chars: time and
 * pid).  The original had nothing public before it, so it got
 * "_GLOBAL_$N$.._.._printf.cxxQakP7f" (compiled as ../../printf.cxx), which
 * is spelled out in its RTTI name @0x0103FCB8 and in every PrintTarget
 * symbol.  A rebuild cannot reproduce random characters, and a different
 * random name in every compile would make the symbols (and the FUNC markers
 * that name them) unstable.  So, for the retail compiler only, a zero-size
 * public object whose assembler name is exactly that string comes first: it
 * becomes first_global_object_name and egcs then derives the original's
 * namespace name, deterministically.  It emits no bytes (a label at the start
 * of the object's .rodata, which is 8-byte aligned anyway for the vtable
 * there), so the type name string has the retail bytes and length (51, which
 * puts the next object's .rodata at 0x0103FCF0 as in retail).  It is not
 * matching-only (NON_MATCHING builds need stable names too); GCC 14 builds do
 * not need it (Itanium mangling has no such name). */
#ifdef BOOT_OLD_GCC
extern const char printf_cxx_namespace_anchor[] __asm__(".._.._printf.cxxQakP7f") = {};
#endif

namespace {

// FUNC 0x010116B0 __tfQ233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget (compiler-generated)
//   typeinfo function for (anonymous namespace)::PrintTarget (vtable slot 0,
//   pointer at 0x0103FCA4); Lupo 2 twin 0x002D7240:
//     if (*(u32 *)0x0104DDA8 == 0) {        // .bss __si_type_info object
//         __tfQ26pdistd21PrintFormatTargetBase();                     // 0x010111D0
//         __si_type_info::__si_type_info((void *)0x0104DDA8,           // 0x01019C38
//                 "Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget" /* @0x0103FCB8 */,
//                 *(const __user_type_info *)0x01041EA8);             // base's type_info
//     }
//     return (type_info *)0x0104DDA8;
//   (The base typeinfo function's return value is ignored; the base object's
//   address is re-materialised from $gp.)
//   (The typeinfo function is emitted after printf; its marker is here,
//   above the class whose closing line its debug record carries, so
//   tools/score.py maps it by line: the "." of the namespace name stops
//   the scorer's name matching.)

/* Line-buffered console target.  sizeof == 0x10.
 *   +0x00 vptr = vtable @0x0103FCA0 (.rodata), 8-byte GCC 2.9x entries:
 *         [0] +0x04 __tf...PrintTarget  0x010116B0
 *         [1] +0x0C ~PrintTarget        0x01011508
 *         [2] +0x14 putc                0x01011530
 *   RTTI name "Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget" @0x0103FCB8,
 *   __si_type_info object at 0x0104DDA8 (.bss).
 * The FUNC markers use the original's symbol names (namespace name: see the
 * anchor above). */
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

// FUNC 0x010114A8 (anonymous namespace)::PrintTarget::PrintTarget (__Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTargetPci)
// Constructor: base ctor (0x01010318), vptr = 0x0103FCA0, buffer, capacity =
// size - 1, length = 0; returns this.  Only caller: vprintf (size 0x100).
PrintTarget::PrintTarget(char *buffer, int size)
    : buffer_(buffer), capacity_(size - 1), length_(0)
{
}

// FUNC 0x01011508 (anonymous namespace)::PrintTarget::~PrintTarget (_$_Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget)
// Destructor (vtable slot 1): re-installs vptr 0x0103FCA0 and calls the base
// destructor (0x01010330) with the caller's hidden flags unchanged, so the
// base performs the operator delete when bit 0 is set.  vprintf's stack
// instance is destroyed with flags = 2 (no delete).
PrintTarget::~PrintTarget()
{
}

// FUNC 0x01011530 (anonymous namespace)::PrintTarget::putc (putc__Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTargeti)
// Vtable slot 2 - the sink PrintFormat writes through.  Flushes first when the
// buffer is full (length == capacity), then stores the low byte of c.  Never
// fails: always returns 0.
int PrintTarget::putc(int c)
{
    if (length_ == capacity_)
        flush();
    buffer_[length_++] = (char)c;
    return 0;
}

// FUNC 0x01011590 (anonymous namespace)::PrintTarget::flush (flush__Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget)
// If anything is buffered: NUL-terminate, hand the line to tty_puts
// (0x01012FC8), reset the length.
void PrintTarget::flush()
{
    if (length_ != 0) {
        buffer_[length_] = '\0';
        tty_puts(buffer_);
        length_ = 0;
    }
}

} /* anonymous namespace */

// FUNC 0x010115D8 vprintf
// Formats into a stack PrintTarget over a 256-byte buffer (sp+0x000; the
// target object is at sp+0x100), flushes the tail, destroys the target
// (~PrintTarget(2)) and returns PrintFormat's result (-1 on sink failure).
// The gcc_except_table entries 0x01041B74.. only cover the RAII cleanup: on an
// exception the landing pads run ~PrintTarget(2) and resume unwinding with
// __throw (0x01017CA8); an exception from that destructor ends in
// std::terminate (0x010180D8).  Callers: printf only.
extern "C" int vprintf(const char *format, va_list args)
{
    char buffer[256];
    PrintTarget target(buffer, sizeof(buffer));
    int n;

    n = pdistd::PrintFormat(&target, format, args);
    target.flush();
    return n;
}

// FUNC 0x01011678 printf
// Varargs shim: spills $a1..$t3 as 8-byte slots at sp+0x58..0x88 (the va_list
// area) and calls vprintf(format, sp+0x58).  Callers: pdistd::Allocator debug
// dump (0x010100E0), Sony libpad (0x01014410, 0x01014540, 0x010147A0), libmc
// sceMcInit (0x010157B8) and libkernl sceSifRebootIop (0x01022A58) - all as
// the plain C "printf".
extern "C" int printf(const char *format, ...)
{
    va_list args;
    int n;

    va_start(args, format);
    n = vprintf(format, args);
    va_end(args);                               /* no code in the original */
    return n;
}
