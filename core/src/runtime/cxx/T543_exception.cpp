#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/runtime/cxx/T543_exception.cpp"
#pragma implementation "src/runtime/cxx/T543_exception.cpp"
#pragma implementation "exception"
#endif
/* T543_exception.cpp - egcs (GCC 2.9x-ee) cp/exception.cc, translation unit
 * T543, 0x00279CA8..0x0027A400 of the GT3 US main executable.
 *
 * terminate/unexpected and their setters, the C++ thrown-object record
 * (cp_eh_info) management used by compiler-generated throw/catch code,
 * exception-specification checking, the bad_cast/bad_typeid throw helpers,
 * uncaught_exception, and the exception / bad_exception classes (their
 * #pragma implementation copies: ctors, dtors, what(), typeinfo functions).
 *
 * The same machine code as the bootstrap's runtime/exception.cc (all 25
 * functions BOOT=).  egcs-1.1-era version: __cplus_type_matcher and
 * __check_eh_spec use the 3-argument __throw_type_match_rtti (0x0027CA48,
 * T549; returns the adjusted object pointer or 0), spelled
 * __throw_type_match_rtti_2 as the canonical header does.
 *
 * Two builds:
 *   - egcs (BOOT_OLD_GCC): the original constructs (`throw;`, `throw X()`,
 *     try / catch (...), typeid).  egcs supplies the classes' inline members
 *     (<exception>, emitted here by #pragma implementation), their vtables,
 *     typeinfo functions and nodes.  #pragma implementation (egcs only, first
 *     lines) as the original's: the first one names this file and the #line
 *     makes that name the current one, because the SCE driver compiles a
 *     temporary .ii and egcs rejects the pragma ("can only appear at
 *     top-level") when the names differ.
 *     Checked with ee-gcc 2.9-ee-991111b -O2 -G0: the 25 functions come out
 *     in the listing's order, all 25 the listing's machine code with
 *     retail's data references (score.py: 25/25 exact with data).  The
 *     bad_cast / bad_typeid / what() part needs egcs's own <typeinfo>
 *     (inline ctors, dtors and name(), #pragma interface), which
 *     T548_tinfo.h includes under BOOT_OLD_GCC (ref/matching/u134/notes.md).
 *   - GCC 14: the same constructs written out as their egcs lowering, which
 *     is what the machine code does and keeps the file free of the modern
 *     C++ EH / RTTI ABI:
 *       throw X(...);  ->  construct a temporary X at sp+0; obj = __eh_alloc(sizeof X);
 *                          copy-construct *obj; destroy the temporary;
 *                          __cp_push_exception(obj, <typeinfo fn from obj's vtable entry 0>(),
 *                          X::~X); __throw();
 *       throw;         ->  __uncatch_exception(); __throw();
 *       catch (...)    ->  landing pad: __exception_info = __start_cp_handler();
 *                          leaving the handler by an exception runs
 *                          __cp_pop_exception(__exception_info) and resumes via __throw.
 *       typeid(*obj)   ->  obj->__tf(), the stand-in for egcs vtable entry 0.
 *
 * The exception-table words of this object are at 0x0034F8A4..0x0034F8F4
 * (the `; EH ref` lines of the listing):
 *   0x0027A038..         __check_eh_spec: try { unexpected(); } -> catch(...) 0x0027A048
 *   0x0027A048..0x0027A174 __check_eh_spec: catch clause landing pads
 *                        (-> terminate 0x0027A174, resume 0x0027A16C/0x0027A148,
 *                        __cp_pop_exception 0x0027A15C, bad_exception temp 0x0027A150)
 *   0x0027A1B0 -> 0x0027A1F4  __throw_bad_cast: temp -> destroy, resume
 *   0x0027A230 -> 0x0027A274  __throw_bad_typeid: temp -> destroy, resume
 *
 * Data: __unexpected_func @0x003064B4 (.data); vtables in .rodata:
 *   _vt$13bad_exception @0x00348298, _vt$9exception @0x003482B0; type names
 *   "13bad_exception" @0x003482C8, "9exception" @0x003482D8; type_info
 *   objects __ti9exception @0x00353510 (.sbss), __ti13bad_exception
 *   @0x004B2B30 (.bss).
 */
#include "runtime/cxx/exception_internal.h"
#include "runtime/cxx/T543_exception.h"

/* ------------------------------------------------------------------------ */
/* terminate / unexpected                                                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x00279CA8 std::terminate(void)
/* Call the terminate handler (__terminate_func, 0x00305EBC); noreturn, so
 * there is no epilogue.  575 callers: mostly the "terminate" landing pads of
 * nothrow regions in game code, 5 in this file, and the exception
 * specifications of operator new/delete/new[]/delete[] (T544..T547). */
void std::terminate() noexcept
{
    __terminate_func();
}

// FUNC 0x00279CC0 __default_unexpected
/* Default unexpected handler (initial value of __unexpected_func):
 * std::terminate() (noreturn). */
void __default_unexpected()
{
    std::terminate();
}

/* exception.cc's static: the unexpected handler, swapped by
 * std::set_unexpected.  Defined here as in exception.cc, after the default
 * handler: the first word of this object's .data, just before its frame
 * info (0x003064B8). */
static std::unexpected_handler __unexpected_func __attribute__((__noreturn__)) = __default_unexpected;  /* 0x003064B4 */

// FUNC 0x00279CD0 std::set_terminate(void (*)())
/* Install a new terminate handler, return the old one.  No callers. */
std::terminate_handler std::set_terminate(std::terminate_handler func) noexcept
{
    std::terminate_handler old = __terminate_func;

    __terminate_func = func;
    return old;
}

// FUNC 0x00279CE0 std::set_unexpected(void (*)())
/* Install a new unexpected handler, return the old one.  No callers. */
std::unexpected_handler std::set_unexpected(std::unexpected_handler func) noexcept
{
    std::unexpected_handler old = __unexpected_func;

    __unexpected_func = func;
    return old;
}

// FUNC 0x00279CF0 std::unexpected(void)
/* Call the unexpected handler; noreturn (no epilogue).  Called only by
 * __check_eh_spec. */
void std::unexpected()
{
    __unexpected_func();
}

/* ------------------------------------------------------------------------ */
/* cp_eh_info management                                                    */
/* ------------------------------------------------------------------------ */

// FUNC 0x00279D08 __cp_exception_info
/* Old compatibility entry (for code that expects the exception info in a
 * global): &CP_EH_INFO->value, the current record + 8.  No callers. */
extern "C" void *__cp_exception_info(void)
{
    return &CP_EH_INFO->value;
}

// FUNC 0x00279D30 __cp_eh_info
/* Old compatibility entry: the current C++ exception record.  No callers. */
extern "C" cp_eh_info *__cp_eh_info(void)
{
    cp_eh_info *p = CP_EH_INFO;
    return p;
}

// FUNC 0x00279D50 __start_cp_handler
/* Called at the entry of every catch clause: mark the current exception
 * caught and count one more active handler (64-bit ld/daddiu/sd).
 * Returns the record (the catch clause's hidden __exception_info).
 * Callers: __check_eh_spec and the exception specifications of operator new /
 * delete / new[] / delete[] (T544..T547). */
extern "C" cp_eh_info *__start_cp_handler(void)
{
    cp_eh_info *p = CP_EH_INFO;
    p->caught = true;
    p->handlers++;
    return p;
}

// FUNC 0x00279D88 __eh_alloc
/* Allocate memory for a thrown object or record: malloc (0x0026C3A8);
 * std::terminate() if that fails. */
extern "C" void *__eh_alloc(size_t size)
{
    void *p = malloc(size);
    if (p == 0)
        std::terminate();
    return p;
}

// FUNC 0x00279DB0 __eh_free
/* free (0x0026C380). */
extern "C" void __eh_free(void *p)
{
    free(p);
}

// FUNC 0x00279DD0 __cplus_type_matcher
/* eh_info.match_function of every C++ exception: does catch type match_info
 * accept the exception info_?
 *   - a table of another language (exception_table && language != 4) -> 0;
 *   - CATCH_ALL_TYPE (-1) -> info->value;
 *   - else match_info is a typeinfo function: call it, then
 *     ret = __throw_type_match_rtti(match type, info->type, info->original_value);
 *     on a match store ret (the object adjusted to the catch type) into
 *     info->value.  Returns ret (0 = no match).
 * Reached only through cp_eh_info.eh_info.match_function (from T542's
 * handler search). */
extern "C" void *__cplus_type_matcher(__eh_info *info_, void *match_info,
                                      exception_descriptor *exception_table)
{
    cp_eh_info *info = (cp_eh_info *)info_;

    /* No exception table implies the old style mechanism, so don't check. */
    if (exception_table != NULL
        && exception_table->lang.language != EH_LANG_C_plus_plus)
        return NULL;

    if (match_info == CATCH_ALL_TYPE)
        return info->value;

    /* we don't worry about version info yet, there is only one version! */

    void *match_type = ((void *(*)())match_info)();
    void *ret = __throw_type_match_rtti_2(match_type, info->type, info->original_value);
    /* change value of exception */
    if (ret)
        info->value = ret;
    return ret;
}

// FUNC 0x00279E40 __cp_push_exception
/* Compiler routine for `throw`: allocate (0x30 via __eh_alloc) and fill a
 * cp_eh_info for object `value` of type `type` with destructor `cleanup`,
 * and push it on the eh_context's exception list (the __throw call follows
 * in the caller).  Callers: __check_eh_spec, __throw_bad_cast,
 * __throw_bad_typeid, operator new (bad_alloc, T545). */
extern "C" void __cp_push_exception(void *value, void *type, cleanup_fn cleanup)
{
    cp_eh_info *p = (cp_eh_info *)__eh_alloc(sizeof(cp_eh_info));

    p->value = value;
    p->type = type;
    p->cleanup = cleanup;
    p->handlers = 0;
    p->caught = false;
    p->original_value = value;

    p->eh_info.match_function = (__eh_matcher)__cplus_type_matcher;   /* 0x00279DD0 */
    p->eh_info.language = EH_LANG_C_plus_plus;
    p->eh_info.version = CP_EH_VERSION;

    cp_eh_info **q = (cp_eh_info **)__get_eh_info();

    p->next = *q;
    *q = p;
}

// FUNC 0x00279ED0 __cp_pop_exception
/* Compiler routine run when a catch clause is left: drop one handler of p;
 * if none remain and p is not being rethrown (current and uncaught), unlink
 * p from the exception list (std::terminate() if it is not on it), run its
 * cleanup(value, 2), free the object unless the thrown type is a pointer
 * (__is_pointer 0x0027CF10), and free the record.
 * NOTE: the destructor gets p->value (possibly adjusted to a base by the
 * catch match) while the memory freed is p->original_value.
 * Callers: __check_eh_spec, the exception specifications of T544..T547. */
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
        p->cleanup(p->value, CLEANUP_IN_CHARGE);

    if (!__is_pointer(p->type))
        __eh_free(p->original_value);   /* value may have been coerced */

    __eh_free(p);
}

// FUNC 0x00279F98 __uncatch_exception
/* First half of `throw;`: the current exception is no longer caught.
 * std::terminate() if there is none. */
extern "C" void __uncatch_exception(void)
{
    cp_eh_info *p = CP_EH_INFO;
    if (p == 0)
        std::terminate();
    p->caught = false;
}

#ifndef BOOT_OLD_GCC
/* Cleanups handed to __cp_push_exception (GCC 14 build).  In the binary they
 * are the classes' egcs destructors themselves (bad_exception::~bad_exception
 * 0x0027A2E0, bad_cast::~bad_cast 0x0027C908, bad_typeid::~bad_typeid
 * 0x0027C870), called as (obj, CLEANUP_IN_CHARGE) = destroy without delete.
 * C++ cannot take a destructor's address, so these adaptors (no code of
 * their own in the binary) run them; the flags never ask for a delete. */
static void bad_exception_cleanup(void *obj, int) { static_cast<bad_exception *>(obj)->bad_exception::~bad_exception(); }
static void bad_cast_cleanup(void *obj, int)      { static_cast<bad_cast *>(obj)->bad_cast::~bad_cast(); }
static void bad_typeid_cleanup(void *obj, int)    { static_cast<bad_typeid *>(obj)->bad_typeid::~bad_typeid(); }
#endif

// FUNC 0x00279FC8 __check_eh_spec
/* Enforce an exception specification ([except.unexpected]): the compiler
 * wraps a function with a throw(spec...) list in a catch-all handler that
 * calls this with the n allowed typeinfo pointers (callers: operator
 * new/delete/new[]/delete[], T544..T547), so there is an active exception to
 * rethrow.  If it matches an allowed type it is rethrown.  Else
 * std::unexpected() is called; if that throws, the new exception is checked
 * the same way (rethrown if allowed); else if bad_exception is allowed a
 * bad_exception is thrown; else std::terminate().  Never returns.
 * Registers: n s3, spec s5, p s2 (replaced by the catch record s4 when that
 * differs), i s0/s1. */
#ifdef BOOT_OLD_GCC
extern "C" void __check_eh_spec(int n, const void **spec)
{
    cp_eh_info *p = CP_EH_INFO;

    for (int i = 0; i < n; ++i) {
        if (__throw_type_match_rtti_2(spec[i], p->type, p->value))
            throw;
    }

    try {
        std::unexpected();                  /* 0x0027A038 */
    } catch (...) {
        /* __exception_info is the artificial variable egcs pushes into each
         * catch block (__start_cp_handler's result, 0x0027A048). */
        if (p != __exception_info) {
            p = __exception_info;
            for (int i = 0; i < n; ++i) {
                if (__throw_type_match_rtti_2(spec[i], p->type, p->value))
                    throw;
            }
        }

        const type_info &bad_exc = typeid(bad_exception);   /* __tf13bad_exception() */
        for (int i = 0; i < n; ++i) {
            if (__throw_type_match_rtti_2(spec[i], &bad_exc, p->value))
                throw bad_exception();
        }

        std::terminate();
    }
}
#else
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
    std::unexpected();                      /* 0x0027A038; noreturn */
    /* } catch (...) { check_eh_spec_catch_all(n, spec, p); } */
}

/* __check_eh_spec's catch(...) clause (0x0027A048..0x0027A17C), GCC 14
 * build.  In the binary it is part of __check_eh_spec, entered only through
 * this object's exception table (0x0034F8A4..) when the unexpected handler
 * throws; it is kept here, complete, as a function of its own (no address of
 * its own). */
__attribute__((used, noreturn))
static void check_eh_spec_catch_all(int n, const void **spec, cp_eh_info *p)
{
    /* egcs catch(...) entry (0x0027A048): the hidden __exception_info. */
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
            /* throw bad_exception(); temp @sp+0 (vptr _vt$13bad_exception
             * 0x00348298), __eh_alloc(4), obj->vptr = 0x00348298, temp
             * destroyed (vptr _vt$9exception 0x003482B0), typeinfo from
             * obj's vtable entry 0. */
            bad_exception *obj = new (__eh_alloc(sizeof(bad_exception))) bad_exception;
            __cp_push_exception(obj, (void *)&obj->__tf(), bad_exception_cleanup);
            __throw();
        }
    }

    std::terminate();
    /* Landing pads of the clause (0x0027A148..0x0027A178): leaving it by an
     * exception runs __cp_pop_exception(exception_info) and resumes with
     * __throw; an exception escaping the pop -> std::terminate(). */
}
#endif

// FUNC 0x0027A180 __throw_bad_cast
/* throw bad_cast(): temp @sp+0 (vptr _vt$8bad_cast 0x003484B0), obj =
 * __eh_alloc(4), obj->vptr = _vt$8bad_cast, temp destroyed (inlined dtor
 * chain leaves vptr _vt$9exception 0x003482B0), __cp_push_exception(obj,
 * __tf8bad_cast() [from the vtable], bad_cast::~bad_cast 0x0027C908),
 * __throw().  Would serve dynamic_cast<T&> failures; no callers (its address
 * is in the exception table, 0x0034F8E0). */
extern "C" void __throw_bad_cast(void)
{
#ifdef BOOT_OLD_GCC
    throw bad_cast();
#else
    bad_cast *obj = new (__eh_alloc(sizeof(bad_cast))) bad_cast;
    __cp_push_exception(obj, (void *)&obj->__tf(), bad_cast_cleanup);
    __throw();
#endif
}

// FUNC 0x0027A200 __throw_bad_typeid
/* throw bad_typeid(): same shape with _vt$10bad_typeid (0x00348498) and
 * cleanup bad_typeid::~bad_typeid (0x0027C870).  Would serve typeid(*null);
 * no callers. */
extern "C" void __throw_bad_typeid(void)
{
#ifdef BOOT_OLD_GCC
    throw bad_typeid();
#else
    bad_typeid *obj = new (__eh_alloc(sizeof(bad_typeid))) bad_typeid;
    __cp_push_exception(obj, (void *)&obj->__tf(), bad_typeid_cleanup);
    __throw();
#endif
}

// FUNC 0x0027A280 std::uncaught_exception(void)
/* True while an exception is in flight but not yet caught.  No callers. */
bool std::uncaught_exception() noexcept
{
    cp_eh_info *p = CP_EH_INFO;
    return p && !p->caught;
}

/* ------------------------------------------------------------------------ */
/* exception / bad_exception                                                */
/* ------------------------------------------------------------------------ */

// FUNC 0x0027A2B8 exception::what(void) const
/* typeid(*this).name(): calls the typeinfo function in vtable entry 0
 * (vptr+4, no this adjustment) and returns its _name (the mangled type name,
 * e.g. "9exception").  Shared by exception, bad_exception, bad_cast and
 * bad_typeid (slot 1 of all four vtables). */
const char *exception::what() const
{
#ifdef BOOT_OLD_GCC
    return typeid(*this).name();
#else
    return __tf().name();                   /* typeid(*this).name() */
#endif
}

#ifndef BOOT_OLD_GCC
/* GCC 14 stand-ins for vtable entry 0 (egcs stores __tf9exception /
 * __tf13bad_exception themselves there).  No addresses of their own. */
const type_info &exception::__tf() const
{
    return __tf9exception();
}

const type_info &bad_exception::__tf() const
{
    return __tf13bad_exception();
}
#endif

// FUNC 0x0027A2E0 bad_exception::~bad_exception(int)
/* Stores the base vtable _vt$9exception 0x003482B0 (the own-vtable store is
 * dead and was eliminated; ~exception is inlined); deletes this (operator
 * delete 0x0027C0C0) if (flags & 1).  Also the cleanup of thrown
 * bad_exceptions.  Under egcs: inline in <exception>, emitted here by
 * #pragma implementation. */
#ifndef BOOT_OLD_GCC
bad_exception::~bad_exception()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}
#endif

// FUNC 0x0027A310 bad_exception::bad_exception(void)
/* vptr = _vt$13bad_exception (0x00348298); returns this.  No callers (the
 * throw in __check_eh_spec inlines it).  Under egcs: inline in <exception>,
 * emitted here by #pragma implementation. */
#ifndef BOOT_OLD_GCC
bad_exception::bad_exception()
{
}
#endif

#ifndef BOOT_OLD_GCC
// FUNC 0x0027A328 __tf13bad_exception
/* typeinfo function for bad_exception (vtable entry 0 of 0x00348298): on
 * first call (name word of __ti13bad_exception still 0) makes sure the base
 * typeinfo exists (__tf9exception, result unused) and builds the
 * __si_type_info in place: __rtti_si(&__ti13bad_exception, "13bad_exception"
 * @0x003482C8, &__ti9exception @0x00353510) (0x0027C3E8).  Returns
 * &__ti13bad_exception.  egcs emits it itself (with the vtable). */
extern "C" const type_info &__tf13bad_exception(void)
{
    if (__ti13bad_exception[0] == 0) {  /* _name not set yet */
        __tf9exception();               /* result unused */
        __rtti_si(__ti13bad_exception, "13bad_exception",
                  (const type_info *)__ti9exception);
    }
    return *(const type_info *)__ti13bad_exception;
}
#endif

// FUNC 0x0027A378 exception::~exception(int)
/* vptr = _vt$9exception (0x003482B0); delete this (operator delete
 * 0x0027C0C0) if (flags & 1).  Under egcs: inline in <exception>, emitted
 * here by #pragma implementation. */
#ifndef BOOT_OLD_GCC
exception::~exception()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}
#endif

// FUNC 0x0027A3A8 exception::exception(void)
/* vptr = _vt$9exception (0x003482B0); returns this.  No callers.  Under
 * egcs: inline in <exception>, emitted here by #pragma implementation. */
#ifndef BOOT_OLD_GCC
exception::exception()
{
}
#endif

#ifndef BOOT_OLD_GCC
// FUNC 0x0027A3C0 __tf9exception
/* typeinfo function for exception (vtable entry 0 of 0x003482B0): on first
 * call builds the __user_type_info in place: __rtti_user(&__ti9exception,
 * "9exception" @0x003482D8) (0x0027C408).  Returns &__ti9exception.  Also
 * called by the bad_alloc, bad_typeid and bad_cast typeinfo functions
 * (0x0027C040, 0x0027C8B8, 0x0027C950) for their base.  egcs emits it itself
 * (with the vtable). */
extern "C" const type_info &__tf9exception(void)
{
    if (__ti9exception[0] == 0)         /* _name not set yet */
        __rtti_user(__ti9exception, "9exception");
    return *(const type_info *)__ti9exception;
}
#endif
