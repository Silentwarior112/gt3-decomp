/* T491 - GarageWheel: a garage slot holding a wheel name, and T491's copies
 * of Cell<GarageWheel>'s destructor and typeinfo.  0x0023B350..0x0023B564.
 *
 * Data: .rodata 0x003428F0..0x003429A8: GarageWheel's vtable, the type
 * names GarageWheelList, List<GarageWheel>, GarageWheel, Cell<GarageWheel>
 * (T491 saw the garage wheel list: T429_garage_wheel_list.h), then
 * Cell<GarageWheel>'s vtable.  .data: the frame info only. */
#include "game/save/T429_garage_wheel_list.h"        /* GarageWheel (T491_garage_wheel.h); GarageWheelList
                                                   (seen) */
#include "libc.h"                               /* memcpy, strcpy */
#include "game/data/T481_car_data_base.h"            /* nameToCode */
#include "game/data/car_database_internal.h"

/* Cell<GarageWheel> (inline: prev, next = 0), vptr 0x003428F0, valid = 0,
 * name = "".  Caller: GameStatus's constructor (its 200 wheel slots). */
// FUNC 0x0023B350 GarageWheel::GarageWheel
GarageWheel::GarageWheel()
{
    valid = 0;
    name[0] = 0;
}

/* Empties the slot. */
// FUNC 0x0023B378 GarageWheel::clear
void GarageWheel::clear()
{
    valid = 0;
    name[0] = 0;
}

/* name = s ("" for NULL), valid = 1; returns 1. */
// FUNC 0x0023B388 GarageWheel::setName
int GarageWheel::setName(const char *s)
{
    /* one value for the flag and the result: retail stores and returns the
     * same $v0 (separate constants give a second li) */
    int ret = 1;
    if (s != 0)
        strcpy(name, s);
    else
        name[0] = 0;
    valid = ret;
    return ret;
}

/* The name, NULL if empty. */
// FUNC 0x0023B3C8 GarageWheel::getName
const char *GarageWheel::getName()
{
    if (name[0] == 0)
        return 0;
    return name;
}

/* The 64-bit code of the name. */
// FUNC 0x0023B3E8 GarageWheel::getCode
s64 GarageWheel::getCode()
{
    return nameToCode(name);
}

/* Size of a saved slot: valid + name. */
// FUNC 0x0023B408 GarageWheel::saveSize
int GarageWheel::saveSize()
{
    return 0x44;
}

/* Loads valid + name (0x44 bytes); returns the end of what it read. */
// FUNC 0x0023B410 GarageWheel::load
const u8 *GarageWheel::load(const u8 *src)
{
    u32 size = 0x44;                            /* a variable: retail calls memcpy */
    memcpy(&valid, src, size);
    return src + size;
}

/* Saves them; returns the end of what it wrote. */
// FUNC 0x0023B440 GarageWheel::save
u8 *GarageWheel::save(u8 *dst)
{
    u32 size = 0x44;                            /* a variable: retail calls memcpy */
    memcpy(dst, &valid, size);
    return dst + size;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base Cell<GarageWheel>). */
// FUNC 0x0023B478 __tf11GarageWheel
extern "C" void *__tf11GarageWheel(void)
{
    if (__ti11GarageWheel[0] == 0) {
        __tfQ29gt2k_menut4Cell1Z11GarageWheel();
        __rtti_si(__ti11GarageWheel, "11GarageWheel",
                  (const type_info *)__tiQ29gt2k_menut4Cell1Z11GarageWheel);
    }
    return __ti11GarageWheel;
}
#endif

// FUNC 0x0023B4C8 GarageWheel::~GarageWheel (_$_11GarageWheel)
/* implicit: ~pCell (GarageWheel's implicit destructor, game/save/T491_garage_wheel.h;
 * egcs emits this copy with the vtable, T491 having the constructor, right
 * after __tf11GarageWheel: the vptr (+0x8) = Cell<GarageWheel>'s 0x00342988
 * (its inline destructor), then pCell::~pCell(this, flags)) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function of the template instance (si, base
 * pCell). */
// FUNC 0x0023B4F0 __tfQ29gt2k_menut4Cell1Z11GarageWheel
extern "C" void *__tfQ29gt2k_menut4Cell1Z11GarageWheel(void)
{
    if (__tiQ29gt2k_menut4Cell1Z11GarageWheel[0] == 0) {
        __tfQ29gt2k_menu5pCell();
        __rtti_si(__tiQ29gt2k_menut4Cell1Z11GarageWheel, "Q29gt2k_menut4Cell1Z11GarageWheel",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1Z11GarageWheel;
}
#endif

// FUNC 0x0023B540 gt2k_menu::Cell<GarageWheel>::~Cell
/* implicit: inline in game/menu/framework/menu_list.h; egcs emits this copy, the one Cell<GarageWheel>'s
 * vtable 0x00342988 uses (index.txt: virtual_0): the vptr (+0x8) =
 * 0x00342988, then pCell::~pCell(this, flags) */
