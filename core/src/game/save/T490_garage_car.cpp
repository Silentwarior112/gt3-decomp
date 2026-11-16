/* T490 - GarageCar: a garage slot holding a CarInformation (the class is
 * T036's, game/menu/gtmode/T036_garage.h; its out-of-line methods are here), plus T490's
 * copies of the inline Cell<GarageCar> / GarageCar destructors and typeinfo
 * functions.  0x0023AEA0..0x0023B348.
 *
 * Data: .rodata 0x00342818..0x003428EA: GarageCar's two vtables and
 * Cell<GarageCar>'s (written first, as T427's: the constructor's cleanup
 * expands the inline destructors; nothing references them in the final link:
 * DATA_START), then the type names GarageCarList, List<GarageCar>,
 * GarageCar (+ its base list g_003428A8), Cell<GarageCar> and
 * RegurationResult: T490 saw RegurationResult first and the garage car
 * list, not the wheels (T045_reguration_result.h, then
 * T429_garage_car_list.h).  .data: the frame info only. */
// DATA_START 0x00342818
#include "game/menu/gtmode/T045_reguration_result.h"        /* (seen, not used: its type name) */
#include "game/save/T429_garage_car_list.h"          /* GarageCar (T036_garage_car.h); GarageCarList (seen) */
#include "libc.h"                               /* memcpy */
#include "game/data/T481_car_data_base.h"            /* nameToCode */
#include "game/data/T483_car_information.h"          /* CarInformation */
#include "game/data/T485_car_color.h"                /* CarColorDataBase g_00352D98 */
#include "game/car/model/T446_car_model.h"                /* car_list_name */
#include "game/car/model/T278_show_room_car.h"            /* ShowRoomCar (getPower's stack record) */
#include "game/data/car_database_internal.h"
#ifndef BOOT_OLD_GCC
#include "game/menu/gtmode/T036_garage.h"                   /* __ti9GarageCar, Cell<GarageCar>'s __tf / __ti */
#endif

// FUNC 0x0023AEA0 GarageCar::GarageCar
GarageCar::GarageCar()
{
    /* implicit: CarInformation, ~pCell (compiler-generated: bases, members, deleting form) */
    /* The bases: Cell<GarageCar> (inline: prev, next = 0, vptr 0x00311C50)
     * and CarInformation (CarInformation::CarInformation on +0x10); then the
     * vptrs +0x1EC = 0x00311C20 and +0x8 = 0x00311C30.  If CarInformation's
     * constructor throws, the landing pad runs the Cell part's destructor
     * (vptr 0x00311C50, pCell::~pCell(this, 0)). */
    index = -1;
    valid = 0;
}

/* Empties the slot. */
// FUNC 0x0023AF40 GarageCar::clear
void GarageCar::clear()
{
    CarInformation::clear();
    valid = 0;
    index = -1;
}

/* Becomes car `code' (by name); valid = 1 whatever loadCar returns. */
// FUNC 0x0023AF78 GarageCar::setCar
void GarageCar::setCar(const char *code)
{
    loadCar(nameToCode(code));
    valid = 1;
}

/* The car's code name (CarEquipments::getShortName), NULL if the slot is
 * empty. */
// FUNC 0x0023AFC0 GarageCar::getName
const char *GarageCar::getName()
{
    if (!valid)
        return 0;
    return getShortName();
}

/* The car's colour: the RGB of colour colorIndex of its model in the colour
 * database (0x00BBGGRR) | orMask (the callers' alpha). */
// FUNC 0x0023AFE8 GarageCar::getColor
u32 GarageCar::getColor(u32 orMask)
{
    u64 model = getModelCode();

    return orMask | g_00352D98.findColor(g_00352D98.getColor(model, colorIndex))->rgb;
}

/* The name of the car's model file (car_list_name of the model code). */
// FUNC 0x0023B040 GarageCar::getModelName
const char *GarageCar::getModelName()
{
    return car_list_name(getModelCode());
}

/* The wheel to show: the model's own for the default wheel, else the fitted
 * wheel's name. */
// FUNC 0x0023B068 GarageCar::getWheelModelName
const char *GarageCar::getWheelModelName()
{
    if (isDefaultWheel() != 0)
        return getModelName();
    return getWheelName();
}

/* Copies the CarInformation (CarInformation::operator=), index and valid;
 * returns *this. */
// FUNC 0x0023B0C0 GarageCar::assign
GarageCar &GarageCar::assign(const GarageCar &o)
{
    /* the members in declaration order (an implicit operator= would copy
     * them so); valid is a bool for egcs, so the index load passes the
     * valid store as in retail */
    CarInformation::operator=(o);
    valid = o.valid;
    index = o.index;
    return *this;
}

/* The car's power ('ps') and its rpm, from a showroom record built on the
 * stack with the car's own condition. */
// FUNC 0x0023B110 GarageCar::getPower
void GarageCar::getPower(int *power, int *rpm)
{
    ShowRoomCar showroom;

    showroom.SetupShowRoomCar(this, &condition);
    *power = showroom.power(rpm, POWER_UNIT_PS, 0);
}

/* Size of a saved slot: the CarInformation's, then valid and index. */
// FUNC 0x0023B168 GarageCar::saveSize
int GarageCar::saveSize()
{
    return CarInformation::dataSize() + 8;
}

/* Loads a slot; returns the end of what it read. */
// FUNC 0x0023B188 GarageCar::load
const u8 *GarageCar::load(const u8 *src)
{
    const u8 *p = deserialize(src);
    u32 size = 8;               /* valid, index; a variable: retail calls memcpy */

    memcpy(&valid, p, size);
    return p + size;
}

/* Saves a slot; returns the end of what it wrote. */
// FUNC 0x0023B1D0 GarageCar::save
u8 *GarageCar::save(u8 *dst)
{
    u8 *p = serialize(dst);
    u32 size = 8;               /* valid, index; a variable: retail calls memcpy */

    memcpy(p, &valid, size);
    return p + size;
}

// FUNC 0x0023B218 gt2k_menu::Cell<GarageCar>::~Cell
/* implicit: inline in game/menu/framework/menu_list.h; egcs emits this copy (the vptr +0x8 =
 * 0x00311C50, then pCell::~pCell with the caller's flags; unreferenced:
 * T036's is 0x00125A10) */

#ifndef BOOT_OLD_GCC
/* T490's copy of __tf9GarageCar (unreferenced): CarInformation's typeinfo
 * (__tf14CarInformation 0x002382F0) and Cell<GarageCar>'s (it calls T036's
 * 0x00125B00, not T490's copy), then the __class_type_info with T490's copy
 * of the base list g_003428A8.  Weak: T036's definition wins. */
// FUNC 0x0023B240 __tf9GarageCar
extern "C" __attribute__((weak)) void *__tf9GarageCar(void)
{
    if (__ti9GarageCar[0] == 0) {
        __tf14CarInformation();
        __tfQ29gt2k_menut4Cell1Z9GarageCar();
        __rtti_class(__ti9GarageCar, "9GarageCar", g_003428A8, 2);
    }
    return __ti9GarageCar;
}
#endif

// FUNC 0x0023B2A0 GarageCar::~GarageCar
/* implicit: GarageCar's implicit destructor (game/menu/gtmode/T036_garage.h); egcs
 * emits this copy (both vptrs, 0x00311C20 at +0x1EC and 0x00311C30 at
 * +0x8, CarInformation::~CarInformation on +0x10 with flags 0, then a call
 * of Cell<GarageCar>'s destructor with the caller's flags: synthesized
 * before that one is compiled here, so not inlined; the call reaches T036's
 * copy 0x00125A10, the first one linked; unreferenced: T036's is
 * 0x00125A98) */

#ifndef BOOT_OLD_GCC
/* T490's copy of __tfQ29gt2k_menut4Cell1Z9GarageCar (unreferenced).  Weak:
 * T036's definition wins. */
// FUNC 0x0023B300 __tfQ29gt2k_menut4Cell1Z9GarageCar
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut4Cell1Z9GarageCar(void)
{
    if (__tiQ29gt2k_menut4Cell1Z9GarageCar[0] == 0) {
        __tfQ29gt2k_menu5pCell();
        __rtti_si(__tiQ29gt2k_menut4Cell1Z9GarageCar, "Q29gt2k_menut4Cell1Z9GarageCar",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1Z9GarageCar;
}
#endif
