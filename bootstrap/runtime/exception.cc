/* exception.cc - egcs (GCC 2.9x-ee) cp/exception.cc of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), 0x010180D8..0x01018830.
 *
 * terminate/unexpected and their setters, the C++ thrown-object record
 * (cp_eh_info) management used by compiler-generated throw/catch code,
 * exception-specification checking, the bad_cast/bad_typeid throw helpers,
 * uncaught_exception, and the std::exception / std::bad_exception classes
 * (their #pragma implementation copies: ctors, dtors, what(), typeinfo
 * functions).
 *
 * This is the egcs-1.1-era version of the file: __cplus_type_matcher and
 * __check_eh_spec use the 3-argument __throw_type_match_rtti (0x0101A298,
 * returns the adjusted object pointer or 0), not gcc 2.95's
 * __throw_type_match_rtti_2.  (The shared header cxxrt.h spells it
 * __throw_type_match_rtti_2 with the 3-argument signature; that name is used
 * here so the units agree.)
 *
 * egcs ignores `namespace std` (-fno-honor-std): the classes are mangled
 * "9exception" / "13bad_exception" and the functions "terminate__Fv" etc.
 * The classes are therefore declared at global scope here (which also keeps
 * them apart from the modern <typeinfo>'s std::exception); the functions
 * keep their std:: names from the symbol map.
 *
 * How egcs lowered the C++ constructs in this file (old-model EH):
 *   throw X(...);  ->  construct a temporary X at sp+0; obj = __eh_alloc(sizeof X);
 *                      copy-construct *obj; destroy the temporary;
 *                      __cp_push_exception(obj, <typeinfo fn from obj's vtable slot 0>(),
 *                      X::~X); __throw();
 *   throw;         ->  __uncatch_exception(); __throw();
 *   catch (...)    ->  landing pad: __exception_info = __start_cp_handler();
 *                      leaving the handler by an exception runs
 *                      __cp_pop_exception(__exception_info) and resumes via __throw.
 * The exception table of this object is at .gcc_except_table 0x01041BA0
 * (old model: {start, end, handler} triples, -1 terminated; it is the eh_ptr
 * of exception.o's CIE @0x0103C490):
 *   [0x01018468,0x01018470) -> 0x01018478   __check_eh_spec: try { unexpected(); }
 *   [0x01018478,0x010185A4) -> 0x010185A4   __check_eh_spec: catch clause -> terminate()
 *   [0x01018478,0x01018580) -> 0x0101859C   __check_eh_spec: catch clause -> resume (__throw)
 *   [0x01018484,0x01018578) -> 0x0101858C   __check_eh_spec: catch body -> __cp_pop_exception, resume
 *   [0x01018504,0x0101851C) -> 0x01018580   __check_eh_spec: bad_exception temp -> destroy, resume
 *   [0x010185B0,0x010185E0) -> 0x01018624   __throw_bad_cast: temp -> destroy, resume
 *   [0x01018630,0x01018660) -> 0x010186A4   __throw_bad_typeid: temp -> destroy, resume
 *
 * Data: __unexpected_func @0x0103C48C (.data); vtables in .rodata:
 *   _vt$13bad_exception @0x010400B0 {__tf13bad_exception, ~bad_exception, exception::what}
 *   _vt$9exception      @0x010400C8 {__tf9exception, ~exception, exception::what}
 * (8-byte slots {s16 delta, s16 0, pfn}); type names "13bad_exception"
 * @0x010400E0, "9exception" @0x010400F0; type_info objects __ti9exception
 * @0x01041EB8 (.sbss) and __ti13bad_exception @0x0104DDB8 (.bss).
 */
#include "types.h"
#include "libc.h"
#include "cxxrt.h"
#include "new.h"        /* class exception, __tf9exception / __ti9exception (cxxrt unit header) */
#include "tinfo.h"      /* ::type_info, bad_cast, bad_typeid (cxxrt unit header) */

/* The throw / catch / typeid constructs of this file are written out as the
 * egcs lowering listed above (explicit __eh_alloc / __cp_push_exception /
 * __throw calls, typeid through the __tf() slot-0 virtual), which is what the
 * machine code does.  That also keeps the file free of the modern C++ EH and
 * RTTI ABI, which this binary does not contain (tools/build.py compiles all
 * C++ with -fno-exceptions -fno-rtti). */

/* ------------------------------------------------------------------------ */
/* Types                                                                    */
/* ------------------------------------------------------------------------ */

/* (`noexcept` only to agree with modern g++'s built-in idea of std::terminate;
 * egcs had no exception specifications on these.) */
namespace std {
typedef void (*terminate_handler)();
typedef void (*unexpected_handler)();

void terminate() noexcept __attribute__((__noreturn__));
void unexpected() __attribute__((__noreturn__));
terminate_handler set_terminate(terminate_handler func) noexcept;
unexpected_handler set_unexpected(unexpected_handler func) noexcept;
bool uncaught_exception() noexcept;
}

/* eh-common.h */
typedef void *(*__eh_matcher)(void *info, void *match_info, void *exception_table);

struct __eh_info {
    __eh_matcher match_function;        /* +0x0 */
    short language;                     /* +0x4 */
    short version;                      /* +0x6 */
};

typedef struct exception_lang_info {
    short language;
    short version;
} exception_lang_info;

typedef struct exception_descriptor {
    void *runtime_id_field;             /* +0x0 */
    exception_lang_info lang;           /* +0x4 */
    /* +0x8 region table */
} exception_descriptor;

#define CATCH_ALL_TYPE ((void *)-1)
#define EH_LANG_C_plus_plus 4           /* DW_LANG_C_plus_plus */

/* Cleanup of a thrown object: old-ABI destructor (object, in-charge flags);
 * 2 = destroy, do not delete. */
typedef void (*cleanup_fn)(void *, int);

/* C++-specific information about an exception in flight (0x30 bytes,
 * allocated by __cp_push_exception; list head is eh_context.info). */
struct cp_eh_info {
    __eh_info eh_info;                  /* +0x00 match_function = __cplus_type_matcher, language 4, version 1 */
    void *value;                        /* +0x08 object pointer (adjusted by a catch match) */
    void *type;                         /* +0x0C type_info of the thrown type */
    cleanup_fn cleanup;                 /* +0x10 destructor or NULL */
    int caught;                         /* +0x14 egcs `bool` (4 bytes: lw/sw) */
    cp_eh_info *next;                   /* +0x18 next exception in flight */
    long long handlers;                 /* +0x20 `long` (64-bit on the EE: ld/sd) - active handlers */
    void *original_value;               /* +0x28 object pointer as allocated */
};
static_assert(offsetof(cp_eh_info, handlers) == 0x20, "cp_eh_info.handlers");
static_assert(offsetof(cp_eh_info, original_value) == 0x28, "cp_eh_info.original_value");
static_assert(sizeof(cp_eh_info) == 0x30, "cp_eh_info size");

/* Language-specific EH info pointer (libgcc2, eh.c). */
#define CP_EH_INFO ((cp_eh_info *)*__get_eh_info())

/* Defined in libgcc2 (eh.c) - the terminate handler, initially
 * __default_terminate.  Declared noreturn here, so std::terminate has no
 * epilogue (unlike libgcc2's own __terminate). */
extern "C" void (*__terminate_func)(void) __attribute__((__noreturn__));   /* @0x0103BE94 */

/* ---- std::exception family (egcs <exception>) ---------------------------- */

/* class exception - 4 bytes (vptr only): declared in runtime/new.h (cxxrt
 * unit header), defined below.
 * vtable _vt$9exception @0x010400C8: slot0 __tf9exception (0x010187F0),
 * slot1 ~exception (0x010187A8), slot2 what (0x010186E8). */

/* class bad_exception : exception - 4 bytes.
 * vtable _vt$13bad_exception @0x010400B0: slot0 __tf13bad_exception
 * (0x01018758), slot1 ~bad_exception (0x01018710), slot2 exception::what. */
class bad_exception : public exception {
public:
    bad_exception();
    virtual const type_info &__tf() const;      /* vtable entry 0 = __tf13bad_exception (new.h) */
    virtual ~bad_exception();
};

/* bad_cast (_vt$8bad_cast @0x010402C0 {tf 0x0101A1A0, dtor 0x0101A158,
 * exception::what}) and bad_typeid (_vt$10bad_typeid @0x010402A8 {tf
 * 0x0101A108, dtor 0x0101A0C0, exception::what}) are the cxxrt unit's egcs
 * <typeinfo> classes, declared in runtime/tinfo.h and defined in
 * runtime/tinfo.cc (ctors 0x0101A188 / 0x0101A0F0 out of line there; in the
 * binary the throw helpers below inline them: they only set the vptr).
 * `type_info` below is that header's GCC 2.9x ::type_info, not
 * std::type_info. */

/* GCC 2.9x type_info objects built by the __tf functions below.  Layouts
 * (cxxrt unit, runtime/tinfo.h): __user_type_info = {const char *_name @0;
 * vptr @4} (8 bytes), __si_type_info = {_name @0; vptr @4; const
 * __user_type_info &base @8} (0xC bytes).  Storage is declared as raw words,
 * as the cxxrt unit does (runtime/new.h declares `extern void
 * *__ti9exception[2]` and expects this file to define it): the object is
 * placement-constructed on first use, and word 0 (_name) == 0 means "not
 * built yet".
 *
 * The typeinfo builders the compiler-generated __tf functions call are the
 * cxxrt unit's extern "C" placement builders (runtime/tinfo.cc, declared in
 * cxxrt.h): __rtti_si(addr, name, base_ti) @0x01019C38 and
 * __rtti_user(addr, name) @0x01019C58 (null-checked placement new of
 * __si_type_info / __user_type_info; no return value). */

extern "C" {
void *__ti9exception[2];                    /* @0x01041EB8 (.sbss) __user_type_info (declared in new.h) */
void *__ti13bad_exception[3];               /* @0x0104DDB8 (.bss)  __si_type_info  */
const type_info &__tf13bad_exception(void);     /* 0x01018758 (__tf9exception: new.h) */
}

/* ------------------------------------------------------------------------ */
/* terminate / unexpected                                                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x010180D8 std::terminate(void)
/* Call the terminate handler (__terminate_func, 0x0103BE94); noreturn, so
 * there is no epilogue.  32 callers: 23 in game code (the "terminate" landing
 * pads of nothrow regions, e.g. a destructor throwing during unwinding), 5 in
 * this file and 4 in cxxrt's operator new/delete/new[]/delete[]. */
void std::terminate() noexcept
{
    __terminate_func();
}

// FUNC 0x010180F0 __default_unexpected
/* Default unexpected handler: std::terminate() (noreturn). */
void __default_unexpected()
{
    std::terminate();
}

static std::unexpected_handler __unexpected_func __attribute__((__noreturn__))
    = __default_unexpected;             /* @0x0103C48C (.data) */

// FUNC 0x01018100 std::set_terminate(void (*)())
/* Install a new terminate handler, return the old one.  No callers. */
std::terminate_handler std::set_terminate(std::terminate_handler func) noexcept
{
    std::terminate_handler old = __terminate_func;

    __terminate_func = func;
    return old;
}

// FUNC 0x01018110 std::set_unexpected(void (*)())
/* Install a new unexpected handler, return the old one.  No callers. */
std::unexpected_handler std::set_unexpected(std::unexpected_handler func) noexcept
{
    std::unexpected_handler old = __unexpected_func;

    __unexpected_func = func;
    return old;
}

// FUNC 0x01018120 std::unexpected(void)
/* Call the unexpected handler; noreturn (no epilogue).  Called only by
 * __check_eh_spec. */
void std::unexpected()
{
    __unexpected_func();
}

/* ------------------------------------------------------------------------ */
/* cp_eh_info management                                                    */
/* ------------------------------------------------------------------------ */

// FUNC 0x01018138 __cp_exception_info
/* Old compatibility entry: &CP_EH_INFO->value (current record + 8).
 * No callers. */
extern "C" void *__cp_exception_info(void)
{
    return &((*(cp_eh_info **)__get_eh_info())->value);
}

// FUNC 0x01018160 __cp_eh_info
/* Old compatibility entry: the current C++ exception record.  No callers. */
extern "C" cp_eh_info *__cp_eh_info(void)
{
    cp_eh_info *p = CP_EH_INFO;
    return p;
}

// FUNC 0x01018180 __start_cp_handler
/* Called at the entry of every catch clause: mark the current exception
 * caught and count one more active handler (64-bit ld/daddiu/sd).
 * Returns the record (the catch clause's hidden __exception_info).
 * Callers: __check_eh_spec and the throw()-spec wrappers of operator new /
 * delete / new[] / delete[]. */
extern "C" cp_eh_info *__start_cp_handler(void)
{
    cp_eh_info *p = CP_EH_INFO;
    p->caught = 1;
    p->handlers++;
    return p;
}

// FUNC 0x010181B8 __eh_alloc
/* Allocate memory for a thrown object or record: malloc (0x01011390);
 * std::terminate() if that fails. */
extern "C" void *__eh_alloc(size_t size)
{
    void *p = malloc(size);
    if (p == 0)
        std::terminate();
    return p;
}

// FUNC 0x010181E0 __eh_free
/* free (0x01011368). */
extern "C" void __eh_free(void *p)
{
    free(p);
}

// FUNC 0x01018200 __cplus_type_matcher
/* eh_info.match_function of every C++ exception: does catch type match_info
 * accept the exception info?
 *   - a table of another language (exception_table && language != 4) -> 0;
 *   - CATCH_ALL_TYPE (-1) -> info->value;
 *   - else match_info is a typeinfo function: call it, then
 *     r = __throw_type_match_rtti(type, info->type, info->original_value);
 *     on a match store r (the object adjusted to the catch type) into
 *     info->value.  Returns r (0 = no match). */
extern "C" void *__cplus_type_matcher(__eh_info *info_, void *match_info, void *exception_table_)
{
    cp_eh_info *info = (cp_eh_info *)info_;
    exception_descriptor *exception_table = (exception_descriptor *)exception_table_;
    void *ret;

    /* No exception table implies the old style mechanism, so don't check. */
    if (exception_table != NULL
        && exception_table->lang.language != EH_LANG_C_plus_plus)
        return NULL;

    if (match_info == CATCH_ALL_TYPE)
        return info->value;

    /* we don't worry about version info yet, there is only one version! */

    void *match_type = ((void *(*)())match_info)();
    ret = __throw_type_match_rtti_2(match_type, info->type, info->original_value);
    /* change value of exception */
    if (ret)
        info->value = ret;
    return ret;
}

// FUNC 0x01018270 __cp_push_exception
/* Compiler routine for `throw`: allocate (0x30 via __eh_alloc) and fill a
 * cp_eh_info for object `value` of type `type` with destructor `cleanup`,
 * and push it on the eh_context's exception list (the __throw call follows
 * in the caller).  Callers: __check_eh_spec, __throw_bad_cast,
 * __throw_bad_typeid, operator new (bad_alloc). */
extern "C" void __cp_push_exception(void *value, void *type, cleanup_fn cleanup)
{
    cp_eh_info *p = (cp_eh_info *)__eh_alloc(sizeof(cp_eh_info));

    p->value = value;
    p->type = type;
    p->cleanup = cleanup;
    p->handlers = 0;
    p->caught = false;
    p->original_value = value;

    p->eh_info.match_function = (__eh_matcher)__cplus_type_matcher;   /* 0x01018200 */
    p->eh_info.language = EH_LANG_C_plus_plus;
    p->eh_info.version = 1;

    cp_eh_info **q = (cp_eh_info **)__get_eh_info();

    p->next = *q;
    *q = p;
}

// FUNC 0x01018300 __cp_pop_exception
/* Compiler routine run when a catch clause is left: drop one handler of p;
 * if none remain and p is not being rethrown (current and uncaught), unlink
 * p from the exception list (std::terminate() if it is not on it), run its
 * cleanup(value, 2), free the object unless the thrown type is a pointer
 * (__is_pointer 0x0101A760), and free the record.
 * NOTE: the destructor gets p->value (possibly adjusted to a base by the
 * catch match) while the memory freed is p->original_value. */
extern "C" void __cp_pop_exception(cp_eh_info *p)
{
    cp_eh_info **q = (cp_eh_info **)__get_eh_info();

    --p->handlers;

    /* Don't really pop if there are still active handlers for our exception,
       or if our exception is being rethrown (i.e. if the active exception is
       our exception and it is uncaught). */
    if (p->handlers != 0
        || (p == *q && !p->caught))
        return;

    for (; *q; q = &((*q)->next))
        if (*q == p)
            break;

    if (!*q)
        std::terminate();

    *q = p->next;

    if (p->cleanup)
        /* 2 is a magic value for destructors; see build_delete(). */
        p->cleanup(p->value, 2);

    if (!__is_pointer(p->type))
        __eh_free(p->original_value);   /* value may have been coerced */

    __eh_free(p);
}

// FUNC 0x010183C8 __uncatch_exception
/* First half of `throw;`: the current exception is no longer caught.
 * std::terminate() if there is none. */
extern "C" void __uncatch_exception(void)
{
    cp_eh_info *p = CP_EH_INFO;
    if (p == 0)
        std::terminate();
    p->caught = false;
}

// FUNC 0x010183F8 __check_eh_spec
/* Enforce an exception specification: the compiler wraps a function with a
 * throw(spec...) list in a catch-all handler that calls this with the n
 * allowed typeinfo pointers (callers: operator new/delete/new[]/delete[]).
 * If the current exception matches an allowed type it is rethrown.  Else
 * std::unexpected() is called; if that throws, the new exception is checked
 * the same way (rethrown if allowed); else if bad_exception is allowed a
 * bad_exception is thrown; else std::terminate().  Never returns.
 * Registers: n s3, spec s5, p s2, catch __exception_info s4, loop i s0/s1.
 * EH regions: see the table in the file header. */
/* Cleanups handed to __cp_push_exception.  In the binary they are the
 * classes' old-ABI destructors themselves (bad_exception::~bad_exception
 * 0x01018710, bad_cast::~bad_cast 0x0101A158, bad_typeid::~bad_typeid
 * 0x0101A0C0), called as (obj, 2) = destroy without delete.  C++ cannot take
 * a destructor's address, so these adaptors run them; flag 2 never deletes. */
static void bad_exception_cleanup(void *obj, int) { static_cast<bad_exception *>(obj)->~bad_exception(); }
static void bad_cast_cleanup(void *obj, int)      { static_cast<bad_cast *>(obj)->~bad_cast(); }
static void bad_typeid_cleanup(void *obj, int)    { static_cast<bad_typeid *>(obj)->~bad_typeid(); }

/* __check_eh_spec's catch(...) clause (0x01018478..0x010185A4).  In the binary
 * it is entered only through this object's exception table (0x01041BA0), when
 * the unexpected handler throws; the rebuilt image carries no egcs exception
 * tables, so there __throw finds no handler for it and terminates instead.
 * The clause is kept here, complete, as a function of its own. */
__attribute__((used, noreturn))
static void check_eh_spec_catch_all(int n, const void **spec, cp_eh_info *p)
{
    /* egcs catch(...) entry (0x01018478): the hidden __exception_info. */
    cp_eh_info *exception_info = __start_cp_handler();

    if (p != exception_info) {
        p = exception_info;
        for (int i = 0; i < n; ++i) {
            if (__throw_type_match_rtti_2(spec[i], p->type, p->value)) {
                __uncatch_exception();      /* throw; */
                __throw();
            }
        }
    }

    const type_info &bad_exc = __tf13bad_exception();   /* typeid (bad_exception) */
    for (int i = 0; i < n; ++i) {
        if (__throw_type_match_rtti_2(spec[i], &bad_exc, p->value)) {
            /* throw bad_exception(); temp @sp+0; __eh_alloc(4); vptr 0x010400B0 */
            bad_exception *obj = new (__eh_alloc(sizeof(bad_exception))) bad_exception;
            __cp_push_exception(obj, (void *)&obj->__tf(), bad_exception_cleanup);
            __throw();
        }
    }

    std::terminate();
}

extern "C" void __check_eh_spec(int n, const void **spec)
{
    cp_eh_info *p = CP_EH_INFO;

    for (int i = 0; i < n; ++i) {
        if (__throw_type_match_rtti_2(spec[i], p->type, p->value)) {
            __uncatch_exception();          /* throw; */
            __throw();
        }
    }

    /* try { */
    std::unexpected();                      /* 0x01018468; noreturn */
    /* } catch (...) { check_eh_spec_catch_all(n, spec, p); } */
}

// FUNC 0x010185B0 __throw_bad_cast
/* throw bad_cast(): temp @sp+0 (vptr _vt$8bad_cast 0x010402C0), obj =
 * __eh_alloc(4), obj->vptr = _vt$8bad_cast, temp destroyed (inlined dtor chain
 * leaves vptr _vt$9exception), __cp_push_exception(obj, __tf8bad_cast() [from
 * the vtable], bad_cast::~bad_cast 0x0101A158), __throw().  Would serve
 * dynamic_cast<T&> failures; no callers in this binary. */
extern "C" void __throw_bad_cast(void)
{
    bad_cast *obj = new (__eh_alloc(sizeof(bad_cast))) bad_cast;
    __cp_push_exception(obj, (void *)&obj->__tf(), bad_cast_cleanup);
    __throw();
}

// FUNC 0x01018630 __throw_bad_typeid
/* throw bad_typeid(): same shape with _vt$10bad_typeid (0x010402A8) and
 * cleanup bad_typeid::~bad_typeid (0x0101A0C0).  Would serve typeid(*null);
 * no callers in this binary. */
extern "C" void __throw_bad_typeid(void)
{
    bad_typeid *obj = new (__eh_alloc(sizeof(bad_typeid))) bad_typeid;
    __cp_push_exception(obj, (void *)&obj->__tf(), bad_typeid_cleanup);
    __throw();
}

// FUNC 0x010186B0 std::uncaught_exception(void)
/* True while an exception is in flight but not yet caught. */
bool std::uncaught_exception() noexcept
{
    cp_eh_info *p = CP_EH_INFO;
    return p && !p->caught;
}

/* ------------------------------------------------------------------------ */
/* std::exception / std::bad_exception                                       */
/* ------------------------------------------------------------------------ */

// FUNC 0x010186E8 exception::what(void) const
/* typeid(*this).name(): calls the typeinfo function in vtable slot 0
 * (vptr+4) and returns its _name (the mangled type name, e.g. "9exception").
 * Shared by exception, bad_exception, bad_cast and bad_typeid. */
const char *exception::what() const
{
    return __tf().name();                   /* typeid(*this).name() */
}

/* Modern-vtable stand-ins for entry 0 (egcs stores __tf9exception /
 * __tf13bad_exception themselves there; see runtime/new.h).  No addresses of
 * their own in the binary. */
const type_info &exception::__tf() const
{
    return __tf9exception();
}

const type_info &bad_exception::__tf() const
{
    return __tf13bad_exception();
}

// FUNC 0x01018710 bad_exception::~bad_exception(int)
/* Stores the base vtable _vt$9exception (the own-vtable store is dead and
 * was eliminated; ~exception is inlined); deletes this (operator delete
 * 0x01019910) if (flags & 1).  Also the cleanup of thrown bad_exceptions. */
bad_exception::~bad_exception()
{
}

// FUNC 0x01018740 bad_exception::bad_exception(void)
/* vptr = _vt$13bad_exception (0x010400B0); returns this. */
bad_exception::bad_exception()
{
}

// FUNC 0x01018758 __tf13bad_exception
/* typeinfo function for bad_exception (vtable slot 0 of 0x010400B0): on
 * first call (name word of __ti13bad_exception still 0) makes sure the base
 * typeinfo exists (__tf9exception, result unused) and builds the
 * __si_type_info in place: __rtti_si(&__ti13bad_exception, "13bad_exception"
 * @0x010400E0, &__ti9exception @0x01041EB8) (0x01019C38).  Returns
 * &__ti13bad_exception. */
extern "C" const type_info &__tf13bad_exception(void)
{
    if (__ti13bad_exception[0] == 0) {  /* _name not set yet */
        __tf9exception();               /* result unused */
        __rtti_si(__ti13bad_exception, "13bad_exception",
                  (const type_info *)__ti9exception);
    }
    return *(const type_info *)__ti13bad_exception;
}

// FUNC 0x010187A8 exception::~exception(int)
/* vptr = _vt$9exception (0x010400C8); delete this if (flags & 1). */
exception::~exception()
{
}

// FUNC 0x010187D8 exception::exception(void)
/* vptr = _vt$9exception (0x010400C8); returns this. */
exception::exception()
{
}

// FUNC 0x010187F0 __tf9exception
/* typeinfo function for exception (vtable slot 0 of 0x010400C8): on first
 * call builds the __user_type_info in place: __rtti_user(&__ti9exception,
 * "9exception" @0x010400F0) (0x01019C58).  Returns &__ti9exception.  Also
 * called by the bad_alloc, bad_cast and bad_typeid typeinfo functions (cxxrt)
 * for their base. */
extern "C" const type_info &__tf9exception(void)
{
    if (__ti9exception[0] == 0)         /* _name not set yet */
        __rtti_user(__ti9exception, "9exception");
    return *(const type_info *)__ti9exception;
}
