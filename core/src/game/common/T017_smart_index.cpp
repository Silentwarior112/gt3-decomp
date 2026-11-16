/* T017 - gt2k_menu::SmartSkipIndex and its car / course variants.
 * 0x00103218..0x001036D0.
 *
 * A SmartIndexT<int> (T251) over [begin, end) whose positions can be
 * invalid (isValid, slot 2): get / size count only the valid ones and move
 * steps over the invalid ones.  SmartCarIndex accepts the cars of the
 * player's class the arcade progress opened, SmartCourseIndex the courses
 * the arcade level allows.  The get / size / move bodies are shared by the
 * three vtables, so they are SmartSkipIndex's although index.txt files them
 * under SmartCarIndex. */
/* In the original's class order (egcs writes the type names in reverse):
 * the GameStatus's garage, RegurationResult, then SmartIndexT<int>. */
#include "cxxrt.h"                          /* type_info, __rtti_si / __rtti_user (GCC 14's typeinfo
                                               functions) */
#include "game/save/T429_game_status.h"          /* getGameStatus (isArcadeLevelOpen, arcadeLevel) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: a class the original saw) */
#include "game/common/T017_smart_index.h"
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl::getModeName / getPlayerClassName */
#include "game/data/T435_product_config.h"       /* ProductConfig, product_config */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T489_car_database.h"         /* db_car_is_locked */

namespace gt2k_menu {

/* index.txt: SmartCarIndex::virtual_3.  SmartIndexT<int>::move until it
 * lands on a valid index; if the base stops moving (0) on an invalid one,
 * the old value back (set).  1 if the value changed. */
// FUNC 0x00103218 gt2k_menu::SmartSkipIndex::move
int SmartSkipIndex::move(int delta)
{
    int old = value;
    for (;;) {
        int moved = SmartIndexT<int>::move(delta);
        int valid = isValid(value);
        if (moved == 0) {
            if (valid == 0)
                set(old);
            if (old != value)
                return 1;
            return 0;
        }
        if (valid != 0)
            return 1;
    }
}

/* index.txt: SmartCarIndex::virtual_1.  The valid indices in
 * [begin, end). */
// FUNC 0x001032C8 gt2k_menu::SmartSkipIndex::size
int SmartSkipIndex::size()
{
    int count = 0;
    for (int i = begin; i < end; i++)
        if (isValid(i) != 0)
            count++;
    return count;
}

/* index.txt: SmartCarIndex::virtual_0.  The valid indices in [0, value):
 * the position of value among the valid ones. */
// FUNC 0x00103348 gt2k_menu::SmartSkipIndex::get
int SmartSkipIndex::get()
{
    int count = 0;
    for (int i = 0; i < value; i++)
        if (isValid(i) != 0)
            count++;
    return count;
}

/* index.txt: SmartCarIndex::virtual_2.  Every index while the owner has no
 * class for this player (getPlayerClassName NULL: the garages); else car i
 * of the class must have its arcade level open (ProductConfig F_0021D320,
 * GameStatus::isArcadeLevelOpen) and not be locked (db_car_is_locked of its
 * code). */
// FUNC 0x001033C0 gt2k_menu::SmartCarIndex::isValid
int SmartCarIndex::isValid(int i)
{
    int player = (player2 == 0) ? 1 : 2;
    if (owner->getPlayerClassName(player) == NULL)
        return 1;
    const char *cls = owner->getPlayerClassName(player);
    int level = product_config()->F_0021D320(i, cls);
    const char *code = product_config()->carCode(i, cls);
    int valid = 0;
    if (getGameStatus()->isArcadeLevelOpen(level) != 0) {
        if (db_car_is_locked(nameToCode(code)) == 0)
            valid = 1;
    }
    return valid;
}

/* index.txt: SmartCourseIndex::virtual_2.  The course's arcade level (in
 * the course list of the owner's mode) <= GameStatus::arcadeLevel. */
// FUNC 0x001034A0 gt2k_menu::SmartCourseIndex::isValid
int SmartCourseIndex::isValid(int i)
{
    ProductConfig *config = product_config();
    int need = config->courseLevel(i, owner->getModeName());
    return getGameStatus()->arcadeLevel >= need;
}

// FUNC 0x00103508 gt2k_menu::SmartCarIndex::SmartCarIndex
/* inline in game/common/T017_smart_index.h; egcs emits this copy (SmartIndexT<int>()
 * and SmartSkipIndex(owner) expanded: value 0, begin 0, end 1, clamp 0,
 * wrapped 0) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * SmartSkipIndex). */
// FUNC 0x00103540 __tfQ29gt2k_menu13SmartCarIndex
extern "C" void *__tfQ29gt2k_menu13SmartCarIndex(void)
{
    if (__tiQ29gt2k_menu13SmartCarIndex[0] == 0) {
        __tfQ29gt2k_menu14SmartSkipIndex();
        __rtti_si(__tiQ29gt2k_menu13SmartCarIndex, "Q29gt2k_menu13SmartCarIndex",
                  (const type_info *)__tiQ29gt2k_menu14SmartSkipIndex);
    }
    return __tiQ29gt2k_menu13SmartCarIndex;
}
#endif

namespace gt2k_menu {

// FUNC 0x00103590 gt2k_menu::SmartCourseIndex::SmartCourseIndex
/* inline in game/common/T017_smart_index.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * SmartSkipIndex). */
// FUNC 0x001035C0 __tfQ29gt2k_menu16SmartCourseIndex
extern "C" void *__tfQ29gt2k_menu16SmartCourseIndex(void)
{
    if (__tiQ29gt2k_menu16SmartCourseIndex[0] == 0) {
        __tfQ29gt2k_menu14SmartSkipIndex();
        __rtti_si(__tiQ29gt2k_menu16SmartCourseIndex, "Q29gt2k_menu16SmartCourseIndex",
                  (const type_info *)__tiQ29gt2k_menu14SmartSkipIndex);
    }
    return __tiQ29gt2k_menu16SmartCourseIndex;
}
#endif

namespace gt2k_menu {

// FUNC 0x00103610 gt2k_menu::SmartSkipIndex::SmartSkipIndex
/* inline in game/common/T017_smart_index.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * SmartIndexT<int>). */
// FUNC 0x00103640 __tfQ29gt2k_menu14SmartSkipIndex
extern "C" void *__tfQ29gt2k_menu14SmartSkipIndex(void)
{
    if (__tiQ29gt2k_menu14SmartSkipIndex[0] == 0) {
        __tfQ29gt2k_menut11SmartIndexT1Zi();
        __rtti_si(__tiQ29gt2k_menu14SmartSkipIndex, "Q29gt2k_menu14SmartSkipIndex",
                  (const type_info *)__tiQ29gt2k_menut11SmartIndexT1Zi);
    }
    return __tiQ29gt2k_menu14SmartSkipIndex;
}

/* Compiler-generated typeinfo function of the template instance
 * SmartIndexT<int> (__user_type_info; T017's copy is the called one, also
 * by __tfQ29gt2k_menu21SmartIndexDoubleRange 0x001B2B90). */
// FUNC 0x00103690 __tfQ29gt2k_menut11SmartIndexT1Zi
extern "C" void *__tfQ29gt2k_menut11SmartIndexT1Zi(void)
{
    if (__tiQ29gt2k_menut11SmartIndexT1Zi[0] == 0)
        __rtti_user(__tiQ29gt2k_menut11SmartIndexT1Zi, "Q29gt2k_menut11SmartIndexT1Zi");
    return __tiQ29gt2k_menut11SmartIndexT1Zi;
}
#endif
