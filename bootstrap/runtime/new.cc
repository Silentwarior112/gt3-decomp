/* new.cc - C++ dynamic-memory runtime of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102): five GCC 2.9x-ee libgcc.a members, each with its
 * own .eh_frame CIE
 *
 *   0x01019860..0x0101990F  new.cc: set_new_handler, placement new/new[], and
 *                           the out-of-line copies of bad_alloc's inline
 *                           members (#pragma implementation "new")
 *   0x01019910..0x01019987  operator delete(void *)      (egcs new2.cc)
 *   0x01019988..0x01019AAF  operator new(size_t)         (egcs new1.cc)
 *   0x01019AB0..0x01019B27  operator delete[](void *)    (egcs new2.cc)
 *   0x01019B28..0x01019B9F  operator new[](size_t)       (egcs new1.cc)
 *
 * The nothrow forms were not linked.
 *
 * malloc/free are Polyphony's pdistd allocator front-ends (0x01011390 /
 * 0x01011368, see include/libc.h).
 *
 * Data:
 *   __new_handler            @0x0103C824 (.data, 0; new.o's .data, placed just
 *                            before new.o's .eh_frame CIE at 0x0103C828)
 *   "bad_alloc"              @0x010401D0 (.rodata, returned by what())
 *   nothrow                  @0x010401E0 (.rodata, 1 zero byte + 7 padding to the
 *                            8-aligned vtable; unreferenced) [?]
 *   _vt$9bad_alloc           @0x010401E8 {__tf9bad_alloc, ~bad_alloc, what}
 *   "9bad_alloc"             @0x01040200
 *   "bad_alloc" x5           @0x01040210..0x01040250 (unreferenced copies of the
 *                            inline what() literal, one per later member that
 *                            includes <new>)
 *   __ti9bad_alloc           @0x0104DDC8 (.bss, 0xC)
 *
 * EH: operator delete/delete[] are throw() and operator new/new[] are
 * throw(bad_alloc); egcs wraps each body in a try region whose landing pads
 * (gcc_except_table 0x01041BFC..0x01041CDC) do
 *     p = __start_cp_handler();                         0x01018180
 *     __check_eh_spec(n, spec);   (n = 0, or 1 with spec[0] = &__tf9bad_alloc())
 *                                                       0x010183F8
 * with cleanup pads __cp_pop_exception(p) (0x01018300) + __throw (0x01017CA8,
 * resume unwinding) and std::terminate (0x010180D8) if a cleanup throws.  That
 * is exactly what the exception specifications below mean; it is not written
 * out separately.
 */
#include "types.h"
#include "libc.h"
#include "cxxrt.h"      /* __eh_alloc, __cp_push_exception, __throw */
#include "new.h"
#include "tinfo.h"

// @0x010401E0 (.rodata) /* ? */ identification from layout only, unreferenced.
/* egcs new.cc's `const nothrow_t nothrow = { };` (std:: is ignored,
 * -fhonor-std off).  Assigned from the 8 zero bytes
 * between new.o's "bad_alloc" literal (0x010401D0) and _vt$9bad_alloc
 * (0x010401E8): an empty struct is 1 byte, and MIPS DATA_ALIGNMENT raises
 * records to BITS_PER_WORD (64 on the EE), so 8-aligned.  Nothing references it. */
const nothrow_t nothrow = {};

/* @0x0103C824 (.data).  No initializer in egcs new.cc; g++ 2.9x emitted it as a
 * zero word in .data. */
new_handler __new_handler;

// FUNC 0x01019860 set_new_handler
//   Swap __new_handler (0x0103C824) and return the previous handler.  No callers
//   in this binary (only its FDE references it).
new_handler set_new_handler(new_handler handler)
{
    new_handler prev_handler = __new_handler;
    __new_handler = handler;
    return prev_handler;
}

// FUNC 0x01019870 operator new(unsigned int, void *)
//   Default placement new: returns place.  Inline in egcs <new>, emitted here by
//   #pragma implementation.  First of the two in header order, so the scalar
//   form (the array twin 0x01019878 is byte-identical).  No callers.
//   /* ? */ scalar vs array assignment of 0x01019870/0x01019878 is a guess from
//   declaration order; the bodies and the (FDE-only) references are identical.
void *operator new(size_t, void *place) throw()
{
    return place;
}

// FUNC 0x01019878 operator new[](unsigned int, void *)
//   Default placement array new: returns place.  No callers.
void *operator new[](size_t, void *place) throw()
{
    return place;
}

// FUNC 0x01019880 bad_alloc::what() const
//   Vtable _vt$9bad_alloc entry 2 (@0x010401FC).  Returns "bad_alloc" (.rodata
//   0x010401D0).
const char *bad_alloc::what() const throw()
{
    return "bad_alloc";
}

/* Typeinfo object of bad_alloc (__si_type_info, 0xC bytes), built by
 * __tf9bad_alloc.  @0x0104DDC8 (.bss). */
void *__ti9bad_alloc[3];

// FUNC 0x01019890 __tf9bad_alloc (compiler-generated)
//   Typeinfo function of bad_alloc, vtable entry 0 (@0x010401EC).  If
//   __ti9bad_alloc (0x0104DDC8, .bss) is unbuilt (word 0 == 0): __tf9exception()
//   (0x010187F0, result unused), then __rtti_si(&__ti9bad_alloc, "9bad_alloc"
//   (.rodata 0x01040200), &__ti9exception (0x01041EB8)).  Returns &__ti9bad_alloc.
//   Called by operator new/new[]'s exception-spec landing pads and, through the
//   vtable, by the throw in operator new.
extern "C" const type_info &__tf9bad_alloc(void)
{
    return egcs_tf_si(__ti9bad_alloc, "9bad_alloc", __tf9exception, __ti9exception);
}

/* Modern-vtable stand-in for entry 0 (egcs: __tf9bad_alloc itself; see
 * runtime/new.h).  No address of its own in the binary. */
const type_info &bad_alloc::__tf() const
{
    return __tf9bad_alloc();
}

/* The cleanup operator new hands to __cp_push_exception: in the binary it is
 * bad_alloc::~bad_alloc (0x010198E0) itself, called as (obj, 2) = destroy
 * without delete.  C++ cannot take a destructor's address, so this adaptor
 * runs it; flag 2 never deletes, which a plain destructor call matches. */
static void bad_alloc_cleanup(void *obj, int /* flags: always 2 */)
{
    static_cast<bad_alloc *>(obj)->~bad_alloc();
}

// FUNC 0x010198E0 bad_alloc::~bad_alloc()
//   Implicit virtual dtor, vtable entry 1 (@0x010401F4).  (this, flags): the
//   inlined ~exception stores exception's vtable 0x010400C8 into the vptr (+0)
//   (bad_alloc's own vptr store is dead and was dropped), then
//   operator delete(this) (0x01019910) if (flags & 1).  Also passed as the
//   cleanup function of the thrown object in operator new.
bad_alloc::~bad_alloc()
{
}

// FUNC 0x01019910 operator delete(void *)
//   `if (ptr) free(ptr);` (free = 0x01011368).  throw() spec: see the file
//   header for the landing pads at 0x01019930..0x01019977.  29 callers (game
//   deleting destructors, bignum, ImageLoader, ~type_info ...).
void operator delete(void *ptr) throw()
{
    if (ptr)
        free(ptr);
}

// FUNC 0x01019988 operator new(unsigned int)
//   malloc (0x01011390) with size 0 bumped to 1 (movn); on failure call
//   __new_handler (0x0103C824, re-read every iteration) and retry, or throw
//   bad_alloc if there is no handler.  In this binary nothing installs a
//   handler, so an allocation failure throws immediately.
//   `throw bad_alloc()` is lowered as:
//       tmp.vptr = 0x010401E8;             // temporary bad_alloc at sp+0
//       obj = __eh_alloc(4);               // 0x010181B8 (terminates on NULL)
//       obj->vptr = 0x010401E8;            // copy of tmp
//       tmp.vptr = 0x010400C8;             // tmp destroyed (inlined ~exception)
//       __cp_push_exception(obj, obj->vptr[0].pfn() /* __tf9bad_alloc */,
//                           bad_alloc::~bad_alloc /* 0x010198E0 */);  // 0x01018270
//       __throw();                         // 0x01017CA8
//   (landing pad 0x01019A48 destroys tmp - vptr = 0x010400C8 - and resumes
//   unwinding if __eh_alloc / the copy throws).  throw(bad_alloc)
//   spec: pads compute spec[0] = &__tf9bad_alloc() and call __check_eh_spec(1, spec).
void *operator new(size_t sz) throw(bad_alloc)
{
    void *p;

    /* malloc (0) is unpredictable; avoid it. */
    if (sz == 0)
        sz = 1;
    p = malloc(sz);
    while (p == 0) {
        new_handler handler = __new_handler;
        if (!handler) {
            /* throw bad_alloc(); - written as the egcs lowering above (this is
             * the old-model EH of runtime/eh.c, not the modern C++ ABI). */
            bad_alloc *obj = new (__eh_alloc(sizeof(bad_alloc))) bad_alloc;
            __cp_push_exception(obj, (void *)&obj->__tf(), bad_alloc_cleanup);
            __throw();
        }
        handler();
        p = malloc(sz);
    }

    return p;
}

// FUNC 0x01019AB0 operator delete[](void *)
//   Same as operator delete: `if (ptr) free(ptr);` - calls free (0x01011368)
//   directly, not ::operator delete.  throw() spec, same landing-pad shape
//   (0x01019AD0..0x01019B17).  7 callers (FileName / DirPrefix / McBaseDir
//   string buffers, 0x01003570..0x01003B20).
void operator delete[](void *ptr) throw()
{
    if (ptr)
        free(ptr);
}

// FUNC 0x01019B28 operator new[](unsigned int)
//   Forwards to ::operator new (0x01019988).  throw(bad_alloc) spec, landing pads
//   0x01019B40..0x01019B8F.  6 callers: strdup_alloc 0x010035E0, the three
//   FileName GetPath overrides, the McBaseDir ctor, strdup_with_trailing_slash.
void *operator new[](size_t sz) throw(bad_alloc)
{
    return ::operator new(sz);
}
