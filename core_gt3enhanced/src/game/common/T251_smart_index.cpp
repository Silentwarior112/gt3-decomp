/* T251 - gt2k_menu::SmartIndexT<int>.  0x001CC8F8..0x001CCA88.
 *
 * The out-of-line members of the SmartIndexT<int> instance: an index in
 * [begin, end) that wraps around (or, with clamp set, stops at the ends).
 * Declared as explicit specializations in game/common/T251_smart_index.h.
 *
 * Data: retail's .rodata is SmartIndexT<int>'s type name alone (0x00337110,
 * after T250's; no code reference: the anchor below), no vtable (its users
 * write it). */
// DATA_START 0x00337110
#include "game/common/T251_smart_index.h"

namespace gt2k_menu {

/* Brings value back into [begin, end) (end itself if the range is empty). */
// FUNC 0x001CC8F8 gt2k_menu::SmartIndexT<int>::normalize
template <> void SmartIndexT<int>::normalize()
{
    if (begin == end) {
        value = end;
        return;
    }
    if (value < begin) {
        value = begin;
        return;
    }
    if (!(value < end))
        value = end - 1;
}

// FUNC 0x001CC938 gt2k_menu::SmartIndexT<int>::setRange
template <> void SmartIndexT<int>::setRange(int b, int e)
{
    begin = b;
    end = e;
    normalize();
}

// FUNC 0x001CC960 gt2k_menu::SmartIndexT<int>::set
template <> void SmartIndexT<int>::set(int v)
{
    value = v;
    normalize();
}

/* Slot 1 (index.txt: gt2k_menu::SmartIndexT<int>::virtual_1). */
// FUNC 0x001CC980 gt2k_menu::SmartIndexT<int>::size
template <> int SmartIndexT<int>::size()
{
    return end - begin;
}

/* Slot 0 (index.txt: gt2k_menu::SmartIndexT<int>::virtual_0). */
// FUNC 0x001CC990 gt2k_menu::SmartIndexT<int>::get
template <> int SmartIndexT<int>::get()
{
    return value;
}

/* Slot 2 (index.txt: gt2k_menu::SmartSkipIndex::virtual_2): SmartIndexT
 * <int>'s, which SmartSkipIndex's vtable shares (every index is valid). */
// FUNC 0x001CC998 gt2k_menu::SmartIndexT<int>::isValid
template <> int SmartIndexT<int>::isValid(int i)
{
    return 1;
}

/* Slot 3 (index.txt: gt2k_menu::SmartIndexT<int>::virtual_3).  Moves by
 * delta, wrapping (and noting it in `wrapped') or clamping; nonzero if the
 * value changed. */
// FUNC 0x001CC9A0 gt2k_menu::SmartIndexT<int>::move
template <> int SmartIndexT<int>::move(int delta)
{
    int old = value;
    value = old + delta;
    if (clamp == 0) {
        if (value < begin || !(value < end))
            wrapped = 1;
        else
            wrapped = 0;
        while (value < begin)
            value = value + (end - begin);
        while (!(value < end))
            value = value - (end - begin);
    } else {
        if (value < begin)
            value = begin;
        if (!(value < end))
            value = end - 1;
    }
    return old != value;
}

} /* namespace gt2k_menu */
