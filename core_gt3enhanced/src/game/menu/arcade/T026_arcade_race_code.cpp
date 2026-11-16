/* T026 - the race of the arcade selection (gt2k_menu::ArcadeControl::
 * raceCode, declared with the class in T010_arcade_control.h).
 * 0x0010F228..0x0010F5C0.
 *
 * Arcade race codes are "A" + the level letter (E, N, H, P) + the class
 * (C, B, A, S) + "T0001" ("A?AD0001" for dirt), from g_002924B0[level][column
 * of the class].  The home garage car gets the class of its weight / power
 * ratio; a car of the product database may name its own race or add a step
 * to the code's last digit.
 *
 * Includes: retail's .rodata type names (0x0030B890) are SmartIndexT<int>,
 * then the garage lists (GarageWheelList .. Cell<GarageCar>), no Operator:
 * the GameStatus first, then ArcadeControl (SmartIndexT<int>, T251). */
#include "game/save/T429_game_status.h"          /* getGameStatus; T427 MenuStatus (raceCar), GarageCar */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl, ARCADE_* modes and classes */
#include "game/data/T435_product_config.h"       /* product_config: carRace, carRaceStep */
#include "game/data/T483_car_information.h"      /* CarInformation (the garage car's) */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* menu_car_spec, MenuCarSpec::setCar */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar::power (MenuCarSpec::car) */
#include "engine/media/G002_mpeg_frameq.h"          /* strlen */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "game/menu/arcade/arcade_course_select_internal.h"

/* T026's data, in the original's order: the class table and g_0030B810 in
 * .rodata with the race code strings between them (written out when
 * g_002924B0, in .data, is), then the function's strings and its jump
 * table. */
const CarClassRange g_0030B660[] = {        /* 0x0030B660 */
    { 0.0f, 2.1f,       'S', 0 },
    { 2.1f, 3.0f,       'S', 1 },
    { 3.0f, 3.5f,       'A', 0 },
    { 3.5f, 5.0f,       'A', 1 },
    { 5.0f, 5.8f,       'B', 0 },
    { 5.8f, 6.3f,       'B', 1 },
    { 6.3f, 9.0f,       'C', 0 },
    { 9.0f, 1000000.0f, 'C', 1 },
    { 0.0f, 0.0f,       '-', 0 },
};

const char *g_002924B0[4][5] = {            /* 0x002924B0 */
    { "AECT0001", "AEBT0001", "AEAT0001", "AEST0001", "AEAD0001" },
    { "ANCT0001", "ANBT0001", "ANAT0001", "ANST0001", "ANAD0001" },
    { "AHCT0001", "AHBT0001", "AHAT0001", "AHST0001", "AHAD0001" },
    { "APCT0001", "APBT0001", "APAT0001", "APST0001", "APAD0001" },
};

const int g_0030B810[ARCADE_CLASS_PREVIEW + 1] = { /* 0x0030B810 */
    0, 1, 2, 3, 4,                          /* C, B, A, S, dirt */
};

/* raceCode's two result buffers: T026's .bss (file statics; right after
 * T025's two buffers 0x0049BB50 / 0x0049BB90). */
static char g_0049BBD0[0x10];               /* 0x0049BBD0 the home garage car's race code */
static char g_0049BBE0[0x10];               /* 0x0049BBE0 a code with the car's step added */

/* A GarageCar's CarInformation base (+0x10) without the null test of a
 * pointer conversion (arcade_car_select_internal.h has the same helper): the listing adds 0x10 to
 * the car directly.  egcs tests a static_cast of the dereferenced car for
 * null (movz), a reference bound to it not; as an inline call the
 * conversion is done after menu_car_spec(), as retail. */
static inline CarInformation *garageCarInfo(GarageCar *car)
{
    CarInformation &info = *car;

    return &info;
}

/* dst = base with the class letter (dst[2]) and the step added to the last
 * digit (dst[7]) of the race car (MenuStatus::raceCar[0], the home garage
 * car): its spec put into the MenuCarSpec (through its CarInformation, no
 * null test), ratio = the spec's weight / its computed power
 * (ShowRoomCar::power of MenuCarSpec::car, catalogue 0; rpm
 * unused), looked up in g_0030B660: S < 2.1 <= S+1 < 3.0 <= A < 3.5 <= A+1
 * < 5.0 <= B < 5.8 <= B+1 < 6.3 <= C < 9.0 <= C+1, the '-' row (ending the
 * table) when no row matches. */
// FUNC 0x0010F228 garage_car_race_code
void garage_car_race_code(char *dst, const char *base)
{
    int row = 0;
    GarageCar *car = getGameStatus()->getMenuStatus()->raceCar[0];
    menu_car_spec()->setCar(garageCarInfo(car));
    int rpm;
    int power = menu_car_spec()->car.power(&rpm, UNIT_TAG_PS, 0);
    f32 ratio = (f32)menu_car_spec()->car.spec.weight / (f32)power;
    for (row = 0; g_0030B660[row].letter != '-'; row++)
        if (g_0030B660[row].lo <= ratio && ratio < g_0030B660[row].hi)
            break;
    strcpy(dst, base);
    dst[2] = g_0030B660[row].letter;
    dst[7] += g_0030B660[row].step;
}

namespace gt2k_menu {

/* The race to run for the current selection; NULL for modes 4 (goodies), 5
 * (load guest garage), 7 and any other.  Single race: the car's own race
 * (ProductConfig::carRace of player 1's car in its class list) if it has
 * one, else from the table by level and class (the garages: the home garage
 * car's class, or the dirt race on a dirt course), plus the car's step
 * (ProductConfig::carRaceStep) on the last character for the data base
 * classes C .. dirt.  Caller: finish (T010). */
// FUNC 0x0010F398 gt2k_menu::ArcadeControl::raceCode
const char *ArcadeControl::raceCode()
{
    const char *code = NULL;
    switch (mode) {
    case ARCADE_SINGLE_RACE: {
        int cls = class1.value;
        int column = g_0030B810[cls];
        const char *carRace = NULL;
        const char *classList = getPlayerClassName(0);
        if (classList != NULL)
            carRace = product_config()->carRace(car1.value, classList);
        if (carRace != NULL && carRace[0] != '\0') {
            code = carRace;
        } else {
            if (cls == ARCADE_CLASS_HOME_GARAGE || cls == ARCADE_CLASS_GUEST_GARAGE) {
                if (isDirtCourse() != 0) {
                    code = g_002924B0[level][ARCADE_CLASS_DIRT];
                } else {
                    garage_car_race_code(g_0049BBD0, g_002924B0[level][0]);
                    code = g_0049BBD0;
                }
            } else {
                code = g_002924B0[level][column];
            }
            if (cls < ARCADE_CLASS_HOME_GARAGE) {
                ProductConfig *config = product_config();
                int index = car1.value;
                int step = config->carRaceStep(index, getPlayerClassName(0));
                if (step > 0) {
                    strcpy(g_0049BBE0, code);
                    g_0049BBE0[strlen(g_0049BBE0) - 1] += step;
                    code = g_0049BBE0;
                }
            }
        }
        break;
    }
    case ARCADE_COMPULSORY:
        code = product_config()->carRace(car1.value, "CLASS_COMPULSORY");
        break;
    case ARCADE_TIME_ATTACK:
        code = "ATAF0001";
        break;
    case ARCADE_TWO_PLAYER:
        code = "A2P0001";
        break;
    case ARCADE_I_LINK:
        code = "ALNK0001";
        break;
    case ARCADE_POD_RACE:
        /* the enhanced engine (Lupo 2 L2 0x001140E0..0x0011411C; menus_a.md
         * 14): 1P the car's race of the Pod class (Lupo 2: always the first
         * car's), the first car's when it has none; 2P APO20001 */
        if (podPlayers == 2) {
            code = ARCADE_POD_2P_RACE;
        } else {
            code = product_config()->carRace(car1.value, ARCADE_POD_CLASS);
            if (code == NULL || code[0] == '\0')
                code = product_config()->carRace(0, ARCADE_POD_CLASS);
        }
        break;
    case ARCADE_GOODIES:
    case ARCADE_LOAD_GUEST_GARAGE:
    case ARCADE_NONE:
        break;
    }
    return code;
}

} /* namespace gt2k_menu */
