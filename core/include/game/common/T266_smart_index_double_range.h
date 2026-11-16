/* T266_smart_index_double_range.h - gt2k_menu::SmartIndexDoubleRange
 * (translation unit T266, unit u062), 0x001B29A8..0x001B2C20.
 *
 * A SmartIndexT<int> (T251) whose value also stays inside an inner range
 * [lo, hi) of its range, wrapping around it or clamping to it (clamp): the
 * value of a gt2k_menu::Slider (T238, held by value at +0x48C; the inner
 * range is SliderDesc's limitMin / limitMax).
 *
 * T266 also has a copy of the template instance's typeinfo function
 * __tfQ29gt2k_menut11SmartIndexT1Zi (0x001B2BE0) that nothing references:
 * the linker kept T017's 0x00103690 (see T251_smart_index.h). */
#ifndef GAME_T266_SMART_INDEX_DOUBLE_RANGE_H
#define GAME_T266_SMART_INDEX_DOUBLE_RANGE_H

#include "game/game.h"
#include "game/common/T251_smart_index.h"              /* base gt2k_menu::SmartIndexT<int> */

namespace gt2k_menu {

/* vtable 0x00332230 (4 slots: [0] get 0x001CC990, [1] size 0x001CC980,
 * [2] isValid 0x001CC998 are SmartIndexT<int>'s).  typeinfo
 * __tfQ29gt2k_menu21SmartIndexDoubleRange 0x001B2B90 (si, base
 * SmartIndexT<int>; __si_type_info 0x004B2440).  SmartIndexT<int>'s data
 * and vptr (+0x14), then the inner range.  Size 0x20 (Slider: index +0x48C,
 * initial +0x4AC). */
class SmartIndexDoubleRange : public SmartIndexT<int> {
public:
    SmartIndexDoubleRange();                    /* 0x001B29A8 SmartIndexT<int>() (value 0, range
                                                   [0, 1), wraps), inner range [0, 1) */
    virtual int move(int delta);                /* [3] +0x24 0x001B2AA8 value + delta inside [lo, hi):
                                                   wrapped (wrapped = 1 when it went out) or
                                                   clamped; nonzero if the value changed */
    void setRange(int b, int e);                /* 0x001B29E0 SmartIndexT<int>::setRange(b, e), and
                                                   the inner range [b, e) */
    void setInnerRange(int l, int h);           /* 0x001B2A28 [l, h) (ignored when l == h), the value
                                                   clamped into it */
    void set(int v);                            /* 0x001B2A68 value = v clamped into [lo, hi); hi when
                                                   the inner range is empty */

    int lo;                                     /* 0x18 the inner range [lo, hi) */
    int hi;                                     /* 0x1C exclusive */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::SmartIndexDoubleRange, 0x20);

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu21SmartIndexDoubleRange(void); /* 0x001B2B90 */
#endif

#endif /* GAME_T266_SMART_INDEX_DOUBLE_RANGE_H */
