/* T251_smart_index.h - gt2k_menu::SmartIndexT<int> (translation unit T251,
 * unit u071), 0x001CC8F8..0x001CCA88: the out-of-line members of
 * SmartIndexT<int>.  The class template is in T251_smart_index_template.h.
 *
 * The constructor is inline (every user expands it).  Only SmartIndexT<int>
 * exists; its vtable is emitted by its users (0x0030D998, 0x0030F890,
 * 0x00314388, 0x00335C48, 0x00336478, 0x00336B18, 0x00336C60) and so is its
 * typeinfo function __tfQ29gt2k_menut11SmartIndexT1Zi (copies 0x00103690
 * T017, the one called, declared in T017_smart_index.h with the
 * __user_type_info 0x003532F8 gp-0x46F8; 0x0011A270 T043, 0x001B2BE0 T266,
 * 0x001C41D0 T231, 0x001C5D00 T235, 0x001C71E0 T240, 0x001CA510 T244, without
 * callers).  T251 defines the members of SmartIndexT<int> (declared below as
 * explicit specializations).
 *
 * Under egcs the declarations below and the size check instantiate
 * SmartIndexT<int> where this header is read: the files whose retail type
 * names have SmartIndexT<int> there read it (T251_smart_index_template.h
 * lists them); ButtonList's header reads the template only. */
#ifndef GAME_T251_SMART_INDEX_H
#define GAME_T251_SMART_INDEX_H

#include "game/game.h"
#include "game/common/T251_smart_index_template.h" /* the class template gt2k_menu::SmartIndexT<T> */

namespace gt2k_menu {

#ifdef BOOT_OLD_GCC
/* The members of SmartIndexT<int>, defined in T251 (GCC 14: declared in
 * T251_smart_index_template.h). */
template <> int  SmartIndexT<int>::get();
template <> int  SmartIndexT<int>::size();
template <> int  SmartIndexT<int>::isValid(int i);
template <> int  SmartIndexT<int>::move(int delta);
template <> void SmartIndexT<int>::normalize();
template <> void SmartIndexT<int>::setRange(int b, int e);
template <> void SmartIndexT<int>::set(int v);
#endif

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::SmartIndexT<int>, 0x18);

#endif /* GAME_T251_SMART_INDEX_H */
