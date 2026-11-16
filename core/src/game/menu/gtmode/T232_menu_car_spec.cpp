/* T232 - MenuCarSpec: the car on show in the menus.  0x001C4210..0x001C4550.
 *
 * One instance (menu_car_spec_instance, made by menu_car_spec_create with
 * the global operator new).  setCar sets up the ShowRoomCar `car' (T278
 * SetupShowRoomCar) from a car code (its own CarEquipments, a fresh
 * CarCondition) or from a garage car (CarInformation: equipments copied, the
 * car's own condition); the getters read the equipments in use (current)
 * and the colour database (T485).
 *
 * Data: retail's T232 has no .rodata (no type name: it saw no class with
 * only inline virtual functions, so not the GameStatus); .sdata 0x00352644:
 * the instance pointer below. */
#include "game/menu/framework/page_layout_internal.h"
#include "game/menu/gtmode/T232_menu_car_spec.h"
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar (SetupShowRoomCar) */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments */
#include "game/data/T483_car_information.h"      /* CarInformation (its condition) */
#include "game/data/T485_car_color.h"            /* CarColorDataBase g_00352D98, CarColorRecord */

MenuCarSpec *menu_car_spec_instance = 0;    /* 0x00352644 */

/* The ShowRoomCar has no constructor; equip's CarEquipments() runs. */
// FUNC 0x001C4210 MenuCarSpec::MenuCarSpec
MenuCarSpec::MenuCarSpec()
{
    /* implicit: CarEquipments (member constructor) */
    current = 0;
}

// FUNC 0x001C4240 MenuCarSpec::clear
void MenuCarSpec::clear()
{
    current = 0;
}

/* Nothing changes if the car `name' does not load (equip may be changed). */
// FUNC 0x001C4248 MenuCarSpec::setCar
void MenuCarSpec::setCar(const char *name)
{
    if (equip.setCar(nameToCode(name)) != 0) {
        CarCondition cond;                  /* a new car: everything 0 */
        car.SetupShowRoomCar(&equip, &cond);
        current = &equip;
    }
}

/* The spec is built from car itself, with its own condition (+0x1F4). */
// FUNC 0x001C42B0 MenuCarSpec::setCar
void MenuCarSpec::setCar(CarInformation *info)
{
    equip = *info;                          /* CarEquipments::operator= (the 0x1E8 bytes) */ /* implicit: CarEquipments::operator= (assignment) */
    current = &equip;
    car.SetupShowRoomCar(info, &info->condition);
}

// FUNC 0x001C4308 MenuCarSpec::getName
const char *MenuCarSpec::getName()
{
    return current->getShortName();
}

// FUNC 0x001C4328 MenuCarSpec::getCode
s64 MenuCarSpec::getCode()
{
    return current->getModelCode();
}

/* The maker's name (CarEquipments::getMakerName). */
// FUNC 0x001C4348 MenuCarSpec::getMakerName
const char *MenuCarSpec::getMakerName()
{
    return current->getMakerName();
}

/* The brand's name (CarEquipments::getBrandName). */
// FUNC 0x001C4368 MenuCarSpec::getBrandName
const char *MenuCarSpec::getBrandName()
{
    return current->getBrandName();
}

/* The colours of the model in use (getCode: the model's code). */
// FUNC 0x001C4388 MenuCarSpec::getColorCount
int MenuCarSpec::getColorCount()
{
    return g_00352D98.getNumColors(getCode());
}

/* out[i] = the RGB of colour i (0x00BBGGRR) | orMask (the caller's alpha). */
// FUNC 0x001C43B0 MenuCarSpec::getColors
void MenuCarSpec::getColors(u32 *out, u32 orMask)
{
    int count = getColorCount();
    for (int i = 0; i < count; i++) {
        int color = g_00352D98.getColor(getCode(), i);
        *out++ = g_00352D98.findColor(color)->rgb | orMask;
    }
}

/* NULL out of range. */
// FUNC 0x001C4450 MenuCarSpec::getColorName
const char *MenuCarSpec::getColorName(int i)
{
    if (i < 0 || i >= getColorCount())
        return 0;
    int color = g_00352D98.getColor(getCode(), i);
    return g_00352D98.getColorName(color);
}

/* 1 if the car has the dirt tyres on (the front tyre part). */
// FUNC 0x001C44C0 MenuCarSpec::hasDirtTires
int MenuCarSpec::hasDirtTires()
{
    return current->getFronttirePart() == PART_ID_FRONT_TIRE_DIRT;
}

// FUNC 0x001C44E8 MenuCarSpec::getEquipments
CarEquipments *MenuCarSpec::getEquipments()
{
    return current;
}

/* Called once at start-up (game_init, T007). */
// FUNC 0x001C44F0 menu_car_spec_create
void menu_car_spec_create(void)
{
    menu_car_spec_instance = new MenuCarSpec(); /* implicit: operator new, operator delete (new-expression) */
}

// FUNC 0x001C4548 menu_car_spec
MenuCarSpec *menu_car_spec(void)
{
    return menu_car_spec_instance;
}
