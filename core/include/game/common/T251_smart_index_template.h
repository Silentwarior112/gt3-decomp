/* T251_smart_index_template.h - the class template gt2k_menu::SmartIndexT<T>
 * (translation unit T251, unit u071; the out-of-line members of
 * SmartIndexT<int> are T251's, declared in T251_smart_index.h).
 *
 * An index `value' in [begin, end) that wraps around (clamp 0; `wrapped'
 * notes that the last move did) or stops at the ends (clamp != 0): the
 * cursor of the menu lists (ButtonList, ColorBox), the PageHistory ring, the
 * T017 indices (SmartSkipIndex) and SmartIndexDoubleRange (T266).
 *
 * Split from T251_smart_index.h in u071's data pass (2026-10-17).  Reading
 * this header does not instantiate SmartIndexT<int> under egcs: the first
 * member or base of that type does.  egcs writes the type names of the
 * classes a file sees in the reverse order of their definitions (template
 * instances at their point of instantiation), and retail's files differ:
 * - T042, T046, T047, T092, T206, T208..T210, T219, T224, T227, T230, T238,
 *   T267..T269 have List<Button> defined before SmartIndexT<int>: they saw
 *   SmartIndexT<int> only through ButtonList, whose index member
 *   instantiates it after its List<Button> base.  T244_button_list.h reads
 *   this header.
 * - T036, T037, T038, T044, T045, T055, T071, T072, T078, T080, T085, T086,
 *   T097, T101, T207, T229 (and the headers T010, T017, T043, T101, T231,
 *   T235, T240, T266) have it where they read T251_smart_index.h, whose
 *   explicit specialization declarations and size check instantiate it there.
 * Under GCC 14 the explicit specialization declarations are here too: they
 * must come before any use of the members (the order of type names is an
 * egcs matter only). */
#ifndef GAME_T251_SMART_INDEX_TEMPLATE_H
#define GAME_T251_SMART_INDEX_TEMPLATE_H

#include "game/game.h"

namespace gt2k_menu {

/* vtable: 4 slots (no virtual destructor).  egcs layout: value +0x00, begin
 * +0x04, end +0x08, clamp +0x0C, wrapped +0x10, vptr +0x14.  Size 0x18
 * (ColorBox+0x10C, PageHistory+0x800 / +0x818, ButtonList+0xD0). */
template <class T>
class SmartIndexT {
public:
    SmartIndexT() : value(0), begin(0), end(1), clamp(0), wrapped(0) {}
    virtual T   get();                      /* [0] +0x0C 0x001CC990 value */
    virtual T   size();                     /* [1] +0x14 0x001CC980 end - begin */
    virtual int isValid(T i);               /* [2] +0x1C 0x001CC998 1: every index (SmartSkipIndex
                                               overrides it) */
    virtual int move(T delta);              /* [3] +0x24 0x001CC9A0 by delta, wrapping or clamping;
                                               nonzero if the value changed */
    void normalize();                       /* 0x001CC8F8 value back into [begin, end) (end if the
                                               range is empty) */
    void setRange(T b, T e);                /* 0x001CC938 begin = b, end = e, then normalize() */
    void set(T v);                          /* 0x001CC960 value = v, then normalize() */
    /* Inline (? name).  GtmodeControl's constructor (u009) sets clamp
     * through it right after constructing infoMode: the store goes through
     * the same `this' register as the constructor's clamp(0), so flow
     * deletes that first store as dead, and retail has the one clamp store
     * behind the oilCylinder store. */
    void setClamp(bool on) { clamp = on; }
    /* Inline (? names, u063).  A clamped index that cannot move further
     * forwards / backwards: T210's Arrow::virtual_1 reads clamp / end /
     * begin through `this' (&list->index in a register) and the result is
     * egcs's store-flag shape (x = 0; if (clamp == 1) { x = clamp; if (...)
     * x = 0; }). */
    bool atEnd() { return clamp == true && value + 1 >= end; }
    bool atBegin() { return clamp == true && value <= begin; }
    /* Inline (? name, u066).  i outside [begin, end): GarageButtonList's
     * constructor (T219, 0x001BD858) tests it through `this' (&index in a
     * register) as a flag (v = 0; if (i < begin || !(i < end)) v = 1). */
    bool outOfRange(T i) { return i < begin || i >= end; }

    T   value;                              /* 0x00 */
    T   begin;                              /* 0x04 */
    T   end;                                /* 0x08 exclusive */
    /* bool (4 bytes in egcs, lw / sw): its own alias set.  ArcadeControl::
     * updateArrows (u002, 0x001050C8) reads idx->clamp twice around an
     * int store (arrowL->visible) and GCSE keeps the first load (the
     * `daddu $t1, $a3' copy), which an int field forbids; SelectBox::
     * virtual_1 (u004) the same around its atTop store.  Scored on every
     * source file: 6 functions of other units improve, none gets worse
     * (ref/matching/u002/notes.md). */
    bool clamp;                             /* 0x0C 0: wraps around, else stops at the ends */
    /* bool too (u069): the constructors' wrapped(0) store is not ordered
     * with int accesses.  ColorBox() (u069 0x001C5228) and KeyConfigItem()
     * (u014 0x0012AB90) are exact only so (retail loads the int member
     * after it, the store stays late), PageHistory::getTop's copy loads it
     * ahead of the int fields; GtmodeControl() (u009 0x00119778) 100 ->
     * 98.6% (ref/matching/u009/pending.md).  Scored on every file that
     * includes this header (u069 notes). */
    bool wrapped;                           /* 0x10 the last move wrapped around */
    /*  vptr                                   0x14 */
};

#ifndef BOOT_OLD_GCC
/* The members of SmartIndexT<int>, defined in T251 (egcs: declared in
 * T251_smart_index.h, see above). */
template <> int  SmartIndexT<int>::get();
template <> int  SmartIndexT<int>::size();
template <> int  SmartIndexT<int>::isValid(int i);
template <> int  SmartIndexT<int>::move(int delta);
template <> void SmartIndexT<int>::normalize();
template <> void SmartIndexT<int>::setRange(int b, int e);
template <> void SmartIndexT<int>::set(int v);
#endif

} /* namespace gt2k_menu */

#endif /* GAME_T251_SMART_INDEX_TEMPLATE_H */
