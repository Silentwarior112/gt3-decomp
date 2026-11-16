/* T483 - CarInformation: a CarEquipments plus the parts owned (one bit per
 * part id) and the car's condition (CarSaveInformation, 0x2C bytes at
 * +0x1E8): prices, ownership, loading a car, the 0x1FC-byte save block.
 * 0x00237860..0x00238390.
 *
 * Names: no symbols; the names describe what the listing does. */
#include "game/data/race_database_internal.h"
#include "game/data/T483_car_information.h"

/* Callers: make_goods_car_price, GarageCar::GarageCar (0x0023AEA0).  Emitted:
 * CarEquipments::CarEquipments, CarCondition::CarCondition (the condition),
 * the vptr, then clear(); on an exception ~CarEquipments. */
// FUNC 0x00237860 CarInformation::CarInformation
CarInformation::CarInformation()
{
    /* implicit: CarCondition, CarEquipments, ~CarEquipments (compiler-generated: bases, members,
     * deleting form) */
    /* CarEquipments(), CarCondition() of condition, the vptr (0x00342388) */
    clear();
}

/* The same, then the car database `db' instead of g_002EBD88.  No caller. */
// FUNC 0x002378D0 CarInformation::CarInformation(const CarDataBase &)
CarInformation::CarInformation(const CarDataBase &db)
{
    /* implicit: CarCondition, CarEquipments, ~CarEquipments (compiler-generated: bases, members) */
    /* CarEquipments(), CarCondition() of condition, the vptr */
    clear();
    copy(db);
}

/* (index.txt: CarInformation::virtual_0.)  The vptr, then
 * CarEquipments::~CarEquipments with the caller's flags (the deleting form
 * is there).  17 callers. */
// FUNC 0x00237958 CarInformation::~CarInformation
CarInformation::~CarInformation()
{
    /* implicit: ~CarEquipments (compiler-generated: bases, members, deleting form) */
    /* ~CarEquipments (egcs: called with the flags) */
}

/* The car database g_002EBD88, nothing owned, the condition zeroed.
 * Callers: the constructors, GarageCar::clear. */
// FUNC 0x00237980 CarInformation::clear
void CarInformation::clear()
{
    copy(g_002EBD88);
    memset((void *)static_cast<CarSaveInformation *>(this), 0, sizeof(CarSaveInformation));
}

/* CarEquipments' assignment, then the 0x2C bytes of CarSaveInformation.
 * The memcpy calls of this file go through `void *' pointers: egcs expands
 * a constant-size memcpy only when it knows both pointers' alignment (a
 * `void *' has none), retail calls memcpy for the 0x2C bytes.  Caller:
 * GarageCar::assign. */
// FUNC 0x002379C0 CarInformation::operator=
CarInformation &CarInformation::operator=(const CarInformation &src)
{
    void *dst;

    CarEquipments::operator=(src);
    dst = static_cast<CarSaveInformation *>(this);
    memcpy(dst, static_cast<const CarSaveInformation *>(&src), sizeof(CarSaveInformation));
    return *this;
}

/* Marks part id `part' (< 0x57) owned; a front tyre of kinds 0..9 also owns
 * the rear tyre of the same kind (recursively; the dirt tyres 0x42 do not).
 * Callers: action_buy_parts, action_buy_all_tires, action_buy_fit_parts,
 * loadCar, addFittedParts. */
// FUNC 0x00237A08 CarInformation::addPart
void CarInformation::addPart(int part)
{
    if (part < PART_COUNT) {
        partsOwned[part >> 3] |= 1 << (part & 7);
        if (part == PART_FRONT_TIRE + 0)
            addPart(PART_REAR_TIRE + 0);
        else if (part == PART_FRONT_TIRE + 1)
            addPart(PART_REAR_TIRE + 1);
        else if (part == PART_FRONT_TIRE + 2)
            addPart(PART_REAR_TIRE + 2);
        else if (part == PART_FRONT_TIRE + 3)
            addPart(PART_REAR_TIRE + 3);
        else if (part == PART_FRONT_TIRE + 4)
            addPart(PART_REAR_TIRE + 4);
        else if (part == PART_FRONT_TIRE + 5)
            addPart(PART_REAR_TIRE + 5);
        else if (part == PART_FRONT_TIRE + 6)
            addPart(PART_REAR_TIRE + 6);
        else if (part == PART_FRONT_TIRE + 7)
            addPart(PART_REAR_TIRE + 7);
        else if (part == PART_FRONT_TIRE + 8)
            addPart(PART_REAR_TIRE + 8);
        else if (part == PART_FRONT_TIRE + 9)
            addPart(PART_REAR_TIRE + 9);
    }
}

/* Clears the bit of part id `part' (< 0x57).  No caller. */
// FUNC 0x00237B68 CarInformation::removePart
void CarInformation::removePart(int part)
{
    if (part < PART_COUNT)
        partsOwned[part >> 3] &= ~(1 << (part & 7));
}

/* The bit of part id `part' (no bounds check; 127 callers). */
// FUNC 0x00237BA0 CarInformation::hasPart
int CarInformation::hasPart(int part)
{
    return (partsOwned[part >> 3] >> (part & 7)) & 1;
}

/* The car's price: its record's price (x 100 in region 0).  The record is
 * not checked for NULL. */
// FUNC 0x00237BC0 CarInformation::getPrice
int CarInformation::getPrice()
{
    int price;

    price = ((CarRecord *)getRecord(CARDB_CAR, carCode))->price;
    if (isJapan() != 0)
        price *= 100;
    return price;
}

/* The price of part id `part' for this car, -1 if the car has no such part.
 * 58 callers. */
// FUNC 0x00237C18 CarInformation::getPartPrice
int CarInformation::getPartPrice(int part)
{
    int category;
    u64 code;

    category = getPartCategory(part);
    code     = findPartCode(part);
    if (code == 0)
        return -1;
    return getPartPrice(category, code);
}

/* The price of record `code' of part table `table' (each record type has
 * its price field); -1 if there is no record.  The tables without a price
 * (chassis, engine, tyre size / force vol, rear tyres and above) keep -1,
 * and that is still x 100 in region 0 (-100, as in the original).  Each
 * case looks the record up again (one switch body per price offset in the
 * binary). */
// FUNC 0x00237C80 CarInformation::getPartPrice(int, u64)
int CarInformation::getPartPrice(int table, u64 code)
{
    int price;

    price = -1;
    if (getRecord(table, code) == NULL)
        return -1;
    switch ((u32)table) {
    case CARDB_STEER:
        price = ((SteerRecord *)getRecord(table, code))->price;
        break;
    case CARDB_TURBINE:
        price = ((TurbineKitRecord *)getRecord(table, code))->price;
        break;
    case CARDB_DRIVETRAIN:
        price = ((DrivetrainRecord *)getRecord(table, code))->price;
        break;
    case CARDB_SUSPENSION:
        price = ((SuspensionRecord *)getRecord(table, code))->price;
        break;
    case CARDB_BRAKE_CONTROLLER:
        price = ((BrakeControllerRecord *)getRecord(table, code))->price;
        break;
    case CARDB_ENGINE_BALANCE:
    case CARDB_NA_TUNE:
        price = ((EngineTuneRecord *)getRecord(table, code))->price;
        break;
    case CARDB_FLYWHEEL:
    case CARDB_CLUTCH:
        price = ((InertiaRecord *)getRecord(table, code))->price;
        break;
    case CARDB_PROPELLER_SHAFT:
        price = ((PropellerShaftRecord *)getRecord(table, code))->price;
        break;
    case CARDB_TCSC:
        price = ((TcsRecord *)getRecord(table, code))->price;
        break;
    case CARDB_GEAR:
        price = ((TransmissionRecord *)getRecord(table, code))->price;
        break;
    case CARDB_LSD:
        price = ((LsdRecord *)getRecord(table, code))->price;
        break;
    case CARDB_ASCC:
        price = ((AsccRecord *)getRecord(table, code))->price;
        break;
    case CARDB_WHEEL:
        price = ((WheelRecord *)getRecord(table, code))->price;
        break;
    case CARDB_BRAKE:
        price = ((BrakeRecord *)getRecord(table, code))->price;
        break;
    case CARDB_LIGHTWEIGHT:
        price = ((LightweightRecord *)getRecord(table, code))->price;
        break;
    case CARDB_RACING_MODIFY:
        price = ((RacingModifyRecord *)getRecord(table, code))->price;
        break;
    case CARDB_PORT_POLISH:
    case CARDB_DISPLACEMENT:
    case CARDB_COMPUTER:
    case CARDB_INTERCOOLER:
    case CARDB_MUFFLER:
        price = ((PowerRecord *)getRecord(table, code))->price;
        break;
    case CARDB_TIRE_COMPOUND:
        /* +0x14 of a TireCompoundRecord is inside curve0: no part has this
         * category, so it is never asked */
        price = *(int *)((u8 *)getRecord(table, code) + 0x14);
        break;
    case CARDB_FRONT_TIRE:
        price = ((TiresRecord *)getRecord(table, code))->price;
        break;
    }
    if (isJapan() != 0)
        price *= 100;
    return price;
}

/* Loads car `code' (setCar), owns its fitted parts, and for a car of type 2
 * (car record +0xF1) the dirt tyres too; 0 if there is no such car.
 * Callers: make_goods_car_price, run_license_test, loadCar(const char *),
 * GarageCar::setCar. */
// FUNC 0x00237DB8 CarInformation::loadCar
int CarInformation::loadCar(u64 code)
{
    if (setCar(code) == 0)
        return 0;
    addFittedParts();
    if (carType == 2) {
        addPart(PART_FRONT_TIRE_DIRT);
        addPart(PART_REAR_TIRE_DIRT);
    }
    return 1;
}

/* Owns the part ids of the fitted parts.  The brake is added twice (as in
 * the original). */
// FUNC 0x00237E18 CarInformation::addFittedParts
void CarInformation::addFittedParts()
{
    addPart(getBrakePart());
    addPart(getBrakePart());
    addPart(getBrakecontrollerPart());
    addPart(getGearPart());
    addPart(getSuspensionPart());
    addPart(getFronttirePart());
    addPart(getReartirePart());
    addPart(getDrivetrainPart());
    addPart(getLSDPart());
    addPart(getRacingmodifyPart());
    addPart(getASCCPart());
    addPart(getTCSCPart());
    addPart(getLightweightPart());
    addPart(getPortpolishPart());
    addPart(getEnginebalancePart());
    addPart(getDisplacementPart());
    addPart(getComputerPart());
    addPart(getNATunePart());
    addPart(getTurbinePart());
    addPart(getFlywheelPart());
    addPart(getClutchPart());
    addPart(getPropellershaftPart());
    addPart(getIntercoolerPart());
    addPart(getMufflerPart());
}

/* 1 if part id `part' exists for this car (findPartCode). */
// FUNC 0x00238018 CarInformation::partExists
int CarInformation::partExists(int part)
{
    if (findPartCode(part) == 0)
        return 0;
    return 1;
}

/* The size of the save block: CarSaveEquipments (0x1D0) + CarSaveInformation
 * (0x2C).  Static (GarageCar::saveSize calls it with its own `this'). */
// FUNC 0x00238040 CarInformation::dataSize
int CarInformation::dataSize()
{
    return 0x1FC;
}

/* Reads the save block (CarSaveEquipments, then CarSaveInformation);
 * returns data + 0x1FC.  Caller: GarageCar::load. */
// FUNC 0x00238048 CarInformation::deserialize
const u8 *CarInformation::deserialize(const void *data)
{
    memcpy(static_cast<CarSaveEquipments *>(this), data, sizeof(CarSaveEquipments));
    data = (const u8 *)data + sizeof(CarSaveEquipments);
    memcpy(static_cast<CarSaveInformation *>(this), data, sizeof(CarSaveInformation));
    return (const u8 *)data + sizeof(CarSaveInformation);
}

/* Writes the save block; returns buf + 0x1FC.  Caller: GarageCar::save. */
// FUNC 0x00238098 CarInformation::serialize
u8 *CarInformation::serialize(void *buf)
{
    memcpy(buf, static_cast<CarSaveEquipments *>(this), sizeof(CarSaveEquipments));
    buf = (u8 *)buf + sizeof(CarSaveEquipments);
    memcpy(buf, static_cast<CarSaveInformation *>(this), sizeof(CarSaveInformation));
    return (u8 *)buf + sizeof(CarSaveInformation);
}

/* A quarter of the part's price (signed division: -1 stays 0).  No caller. */
// FUNC 0x002380F0 CarInformation::getPartSellPrice
int CarInformation::getPartSellPrice(int part)
{
    return getPartPrice(part) / 4;
}

/* A quarter of the car's price.  No caller. */
// FUNC 0x00238120 CarInformation::getSellPrice
int CarInformation::getSellPrice()
{
    return getPrice() / 4;
}

/* loadCar of the code of a name.  No caller. */
// FUNC 0x00238150 CarInformation::loadCar(const char *)
int CarInformation::loadCar(const char *name)
{
    return loadCar(nameToCode(name));
}

/* ------------------------------------------------------------------------ */
/* The condition (CarCondition wrappers, T280)                              */
/* ------------------------------------------------------------------------ */

// FUNC 0x00238188 CarInformation::getEngineMileage
f32 CarInformation::getEngineMileage()
{
    return condition.engineMileage;
}

// FUNC 0x00238190 CarInformation::setEngineMileage
void CarInformation::setEngineMileage(f32 v)
{
    condition.setEngineMileage(v);
}

// FUNC 0x002381B0 CarInformation::getOdometer
u32 CarInformation::getOdometer()
{
    return condition.odometerDistance();
}

// FUNC 0x002381D0 CarInformation::setOdometer
void CarInformation::setOdometer(u32 odo)
{
    condition.setOdometer(odo);
}

// FUNC 0x002381F0 CarInformation::setDirt
void CarInformation::setDirt(f32 dirt)
{
    condition.setDirt(dirt);
}

// FUNC 0x00238210 CarInformation::getDirt
f32 CarInformation::getDirt()
{
    return condition.dirt;
}

// FUNC 0x00238218 CarInformation::changeOil
void CarInformation::changeOil()
{
    condition.changeOil();
}

// FUNC 0x00238238 CarInformation::setOilMileage
void CarInformation::setOilMileage(f32 v)
{
    condition.setOilMileage(v);
}

// FUNC 0x00238258 CarInformation::getOilDirt
f32 CarInformation::getOilDirt()
{
    return condition.oilDirtiness();
}

// FUNC 0x00238278 CarInformation::getOilMileage
f32 CarInformation::getOilMileage()
{
    return condition.oilMileage;
}

// FUNC 0x00238280 CarInformation::resetEngine
void CarInformation::resetEngine()
{
    condition.resetEngine();
}

// FUNC 0x002382A0 CarInformation::getCondition
CarCondition *CarInformation::getCondition()
{
    return &condition;
}

/* The 0x20 bytes of the condition (unaligned 64-bit copies). */
// FUNC 0x002382A8 CarInformation::setCondition
void CarInformation::setCondition(const CarCondition *c)
{
    condition = *c;
}

/* ------------------------------------------------------------------------ */
/* Typeinfo                                                                 */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated: builds the __class_type_info __ti14CarInformation
 * (two bases: CarEquipments at 0, CarSaveInformation at +0x1E8, list
 * 0x003423B0) on the first call.  Callers: __tf9GarageCar and 6 others;
 * vtable 0x00342388 entry 0. */
// FUNC 0x002382F0 __tf14CarInformation
extern "C" void *__tf14CarInformation(void)
{
    if (__ti14CarInformation[0] == 0) {
        __tf18CarSaveInformation();
        __tf13CarEquipments();
        __rtti_class(__ti14CarInformation, "14CarInformation", g_003423B0, 2);
    }
    return __ti14CarInformation;
}

/* Compiler-generated: builds the __user_type_info __ti18CarSaveInformation
 * (gp-0x4548) on the first call. */
// FUNC 0x00238350 __tf18CarSaveInformation
extern "C" void *__tf18CarSaveInformation(void)
{
    if (__ti18CarSaveInformation[0] == 0)
        __rtti_user(__ti18CarSaveInformation, "18CarSaveInformation");
    return __ti18CarSaveInformation;
}
#endif
