/* T435 - ProductConfig (? name), 0x0021CF48..0x0021E068: the product
 * database "gameconf/product.gcf".  The file is read whole, relocated in
 * place (T436..T445) and its sections are kept by type.  One instance,
 * built by product_config_init (gp-0x4F40), read through product_config()
 * by 62 callers: car categories and courses for the menus, the attract
 * demos, events, prize car sets, service prices, memory card demos. */
#include "game/data/gcf_internal.h"
#include "game/data/T435_product_config.h"

/* The instance (product_config_init, product_config).  Initialised data,
 * so egcs writes it here, before gcf_regional_car_code's guard: retail's
 * .sdata has it at 0x00352AB0 and the guard after it (0x00352AB4). */
ProductConfig *g_00352AB0 = 0;              /* 0x00352AB0 gp-0x4F40 */

/* Caller: product_config_init.  The constructor zeroes +0x00..+0x24 only:
 * cardDemos (+0x28) stays as operator new left it unless the file has a
 * type 0xD section.  Unknown section types are skipped. */
// FUNC 0x0021CF48 ProductConfig::ProductConfig(const char *)
ProductConfig::ProductConfig(const char *path)
{
    int i;

    buf = 0;
    hdr = 0;
    general = 0;
    cars = 0;
    courses = 0;
    demos = 0;
    events = 0;
    table1C = 0;
    prizes = 0;
    services = 0;
    buf = file_read_blocking(path, 0, 0, 1, &vol_device_object);
    hdr = gcf_relocate_header((GcfHeader *)buf);
    for (i = 0; i < hdr->count; i++) {
        void *data = hdr->sections[i].data;

        switch (hdr->sections[i].type) {
        case GCF_GENERAL:
            general = gcf_relocate_general((GcfGeneral *)data);
            break;
        case GCF_CARS:
            cars = gcf_relocate_cars((GcfCarSection *)data);
            break;
        case GCF_COURSES:
            courses = gcf_relocate_courses((GcfCourseSection *)data);
            break;
        case GCF_DEMOS:
            demos = gcf_relocate_demos((GcfDemoSection *)data);
            break;
        case GCF_EVENTS:
            events = gcf_relocate_table_04((GcfNamedTable *)data);
            break;
        case GCF_TABLE_06:
            table1C = gcf_relocate_table_06((GcfNamedTable *)data);
            break;
        case GCF_PRIZES:
            prizes = gcf_relocate_prizes((GcfPrizeSection *)data);
            break;
        case GCF_SERVICES:
            services = gcf_relocate_table_0A((GcfNamedTable *)data);
            break;
        case GCF_CARD_DEMOS:
            cardDemos = gcf_relocate_table_0D((GcfNamedTable *)data);
            break;
        }
    }
}

/* No callers (the instance is never deleted). */
// FUNC 0x0021D0B8 ProductConfig::~ProductConfig(void)
/* free(buf); the deleting form (flags bit 0) then calls operator delete. */
ProductConfig::~ProductConfig()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    free(buf);
}

/* Callers: car, ProductConfig::carCount.  The category named `category'
 * (by str_equal), NULL if none.  The count is read once. */
// FUNC 0x0021D100 GcfCarSection::find(const char *)
GcfCarCategory *GcfCarSection::find(const char *category)
{
    GcfCarCategory *found = 0;
    int n = count;
    int i;

    for (i = 0; i < n; i++) {
        if (str_equal(list[i]->name, category)) {
            found = list[i];
            break;
        }
    }
    return found;
}

/* Car i of `category' (no checks). */
// FUNC 0x0021D198 GcfCarSection::car(int, const char *)
GcfCar *GcfCarSection::car(int i, const char *category)
{
    return find(category)->cars[i];
}

/* Callers: course, courseCount, courseIndex.  The same search over the
 * course lists. */
// FUNC 0x0021D1D0 GcfCourseSection::find(const char *)
GcfCourseList *GcfCourseSection::find(const char *name)
{
    GcfCourseList *found = 0;
    int n = count;
    int i;

    for (i = 0; i < n; i++) {
        if (str_equal(list[i]->name, name)) {
            found = list[i];
            break;
        }
    }
    return found;
}

/* Course i of list `name' (no checks). */
// FUNC 0x0021D268 GcfCourseSection::course(int, const char *)
GcfCourse *GcfCourseSection::course(int i, const char *name)
{
    return find(name)->courses[i];
}

/* Caller: GtmodeControl::runRootPage (the title page idle time, in
 * seconds).  The bootstrap function with the same two loads is
 * ImageLoader::Source::GetName (0x01004B38), hence its name in index.txt. */
// FUNC 0x0021D2A0 ProductConfig::idleSeconds (boot: ImageLoader::Source::GetName)
int ProductConfig::idleSeconds()
{
    return general->idleSeconds;
}

/* Caller: run_attract_demo (T000): the demo race's
 * RaceInformation::timeLimit. */
// FUNC 0x0021D2B0 ProductConfig::F_0021D2B0
int ProductConfig::F_0021D2B0()
{
    return general->demoTimeLimit;
}

/* Callers: ArcadeControl::carCount, F_0021D360.  Cars in category `cat'. */
// FUNC 0x0021D2C0 ProductConfig::carCount
int ProductConfig::carCount(const char *cat)
{
    return cars->find(cat)->count;
}

// FUNC 0x0021D2E0 ProductConfig::carCode
const char *ProductConfig::carCode(int i, const char *cat)
{
    return cars->car(i, cat)->code;
}

/* Caller: ArcadeControl::raceCode, which runs it as a race code. */
// FUNC 0x0021D300 ProductConfig::carRace
const char *ProductConfig::carRace(int i, const char *cat)
{
    return cars->car(i, cat)->race;
}

/* The arcade level car i needs (SmartCarIndex::isValid passes it to
 * GameStatus::isArcadeLevelOpen). */
// FUNC 0x0021D320 ProductConfig::F_0021D320
int ProductConfig::F_0021D320(int i, const char *cat)
{
    return cars->car(i, cat)->level;
}

/* Caller: ArcadeControl::raceCode (added to the race code's last
 * character). */
// FUNC 0x0021D340 ProductConfig::carRaceStep
int ProductConfig::carRaceStep(int i, const char *cat)
{
    return cars->car(i, cat)->raceStep;
}

/* No callers.  The highest level of the category's cars (0 if none
 * higher). */
// FUNC 0x0021D360 ProductConfig::F_0021D360
int ProductConfig::F_0021D360(const char *cat)
{
    int i = 0;
    int n = carCount(cat);
    int highest = 0;

    for (; i < n; i++) {
        int level = F_0021D320(i, cat);
        if (highest < level)
            highest = level;
    }
    return highest;
}

/* Caller: ArcadeControl::isCarLocked.  1 if car i of the category can be
 * chosen. */
// FUNC 0x0021D3E0 ProductConfig::carSelectable
int ProductConfig::carSelectable(int i, const char *cat)
{
    return cars->car(i, cat)->selectable == 1;
}

/* No callers.  The +0x4 string (carRace's) of the car with code `code',
 * searched in every category (the section re-read each time); NULL if
 * none. */
// FUNC 0x0021D408 ProductConfig::carRaceOf
const char *ProductConfig::carRaceOf(const char *code)
{
    int c;
    int i;

    for (c = 0; c < cars->count; c++) {
        GcfCarCategory *category = cars->list[c];
        for (i = 0; i < category->count; i++) {
            GcfCar *car = category->cars[i];
            if (strcmp(car->code, code) == 0)
                return car->race;
        }
    }
    return 0;
}

/* Courses in list `list'. */
// FUNC 0x0021D4E8 ProductConfig::courseCount
int ProductConfig::courseCount(const char *list)
{
    return courses->find(list)->count;
}

// FUNC 0x0021D508 ProductConfig::courseName
const char *ProductConfig::courseName(int i, const char *list)
{
    return courses->course(i, list)->name;
}

/* No callers. */
// FUNC 0x0021D528 ProductConfig::F_0021D528
int ProductConfig::F_0021D528(int i, const char *list)
{
    return courses->course(i, list)->unk5;
}

/* The arcade level course i needs. */
// FUNC 0x0021D548 ProductConfig::courseLevel
int ProductConfig::courseLevel(int i, const char *list)
{
    return courses->course(i, list)->level;
}

/* Caller: GameStatus::updateArcadeProgress.  The highest level of the
 * list's courses (0 if none higher). */
// FUNC 0x0021D568 ProductConfig::F_0021D568
int ProductConfig::F_0021D568(const char *list)
{
    int i = 0;
    int n = courseCount(list);
    int highest = 0;

    for (; i < n; i++) {
        int level = courseLevel(i, list);
        if (highest < level)
            highest = level;
    }
    return highest;
}

/* Caller: ArcadeControl::createCourseInfo: the course length. */
// FUNC 0x0021D5E8 ProductConfig::courseLength
int ProductConfig::courseLength(int i, const char *list)
{
    return courses->course(i, list)->length;
}

/* Caller: ArcadeControl::isCourseLocked.  1 if course i of the list is
 * available. */
// FUNC 0x0021D608 ProductConfig::courseAvailable
int ProductConfig::courseAvailable(int i, const char *list)
{
    return courses->course(i, list)->available == 1;
}

/* Caller: FreeRunRecords::table.  The index of course `name' in list
 * `list', -1 if absent. */
// FUNC 0x0021D630 ProductConfig::courseIndex
int ProductConfig::courseIndex(const char *name, const char *list)
{
    int i = 0;
    GcfCourseList *courseList = courses->find(list);

    for (; i < courseList->count; i++) {
        if (strcmp(courseList->courses[i]->name, name) == 0)
            return i;
    }
    return -1;
}

/* Caller of the next six: run_attract_demo (T000). */
// FUNC 0x0021D6C8 ProductConfig::demoCount
int ProductConfig::demoCount()
{
    return demos->count;
}

/* Demo i's race code (run_attract_demo: RaceInformation::setRaceCode). */
// FUNC 0x0021D6D8 ProductConfig::demoCar
const char *ProductConfig::demoCar(int i)
{
    return demos->list[i]->race;
}

/* Demo i's course code. */
// FUNC 0x0021D6F8 ProductConfig::F_0021D6F8
const char *ProductConfig::F_0021D6F8(int i)
{
    return demos->list[i]->course;
}

/* Demo i's shop car code, NULL for none. */
// FUNC 0x0021D718 ProductConfig::F_0021D718
const char *ProductConfig::F_0021D718(int i)
{
    return demos->list[i]->shopCar;
}

/* Demo i's shop car grid position. */
// FUNC 0x0021D738 ProductConfig::F_0021D738
int ProductConfig::F_0021D738(int i)
{
    return demos->list[i]->shopCarPos;
}

/* Demo i's requirement (GameStatus::isLicenseSPassed() must reach it). */
// FUNC 0x0021D758 ProductConfig::F_0021D758
int ProductConfig::F_0021D758(int i)
{
    return demos->list[i]->requirement;
}

/* Callers: eventIndex, eventValue.  The list slot of event `name', NULL if
 * absent (binary search). */
// FUNC 0x0021D778 ProductConfig::findEvent
GcfNamed **ProductConfig::findEvent(const char *name)
{
    return gcf_find_event(events->list, events->count, name);
}

/* Caller: EventStatusObject::EventStatusObject. */
// FUNC 0x0021D7A0 ProductConfig::hasEvent
int ProductConfig::hasEvent(const char *name)
{
    return gcf_find_event(events->list, events->count, name) != 0;
}

// FUNC 0x0021D7D0 ProductConfig::eventCount
int ProductConfig::eventCount()
{
    return events->count;
}

/* The index of event `name' (garbage if absent: (NULL - list) / 4). */
// FUNC 0x0021D7E0 ProductConfig::eventIndex
u32 ProductConfig::eventIndex(const char *name)
{
    GcfNamed **list = events->list;

    return (u32)((char *)findEvent(name) - (char *)list) >> 2;
}

/* The event's requirement (event_unlocked, T036: open when
 * GameStatus::isLicenseSPassed() reaches it).  No NULL check. */
// FUNC 0x0021D810 ProductConfig::eventValue
int ProductConfig::eventValue(const char *name)
{
    return (*findEvent(name))->value;
}

// FUNC 0x0021D838 ProductConfig::eventName
const char *ProductConfig::eventName(int i)
{
    return events->list[i]->name;
}

/* Caller: index1C.  The same search in the type 0x6 table. */
// FUNC 0x0021D858 ProductConfig::find1C
GcfNamed **ProductConfig::find1C(const char *name)
{
    return gcf_find_1C(table1C->list, table1C->count, name);
}

/* No callers. */
// FUNC 0x0021D880 ProductConfig::count1C
int ProductConfig::count1C()
{
    return table1C->count;
}

/* Caller: CourseBestTimes::find (its index into the 44 best times). */
// FUNC 0x0021D890 ProductConfig::index1C
u32 ProductConfig::index1C(const char *name)
{
    GcfNamed **list = table1C->list;

    return (u32)((char *)find1C(name) - (char *)list) >> 2;
}

/* Caller: prizeCarCode.  The car code to use in the current country
 * (g_00352FD8): Opel -> Vauxhall in Britain, the Oreca Viper -> Dodge and
 * S2000 -> S2000 "a" in the US, S2000 -> S2000 "e" in Europe (Britain,
 * France, Germany, Italy, Spain).  *replaced (if given) = 1 when the code
 * was changed. */
// FUNC 0x0021D8C0 gcf_regional_car_code
const char *gcf_regional_car_code(const char *code, int *replaced)
{
    enum {
        US     = 1 << COUNTRY_US,                           /* 0x02 */
        UK     = 1 << COUNTRY_BR,                           /* 0x04 */
        EUROPE = (1 << COUNTRY_BR) | (1 << COUNTRY_FR) | (1 << COUNTRY_GR) |
                 (1 << COUNTRY_IT) | (1 << COUNTRY_SP)      /* 0x7C */
    };

    if (replaced != 0)
        *replaced = 0;
    if (code == 0)
        return 0;
#ifdef BOOT_OLD_GCC
#define GCF_ALIAS(from, to, countries) GcfCarAlias(from, to, countries)
#else
#define GCF_ALIAS(from, to, countries) { from, to, countries }
#endif
    /* egcs builds it at the first call (GcfCarAlias's constructor; retail:
     * the table 0x0049D0E0 in .bss, its guard 0x00352AB4 in .sdata, set
     * first, the stores through a cursor); GCC 14 makes it constant data. */
    static GcfCarAlias aliases[] = {
        GCF_ALIAS("opel_astra_race_00",   "vauxhall_astra_race_00",    UK),
        GCF_ALIAS("opel_speedster_00",    "vauxhall_vx220_00",         UK),
        GCF_ALIAS("opel_calibra_race_95", "vauxhall_calibra_race_95",  UK),
        GCF_ALIAS("viper_gtsr_oreca_00",  "dodge_viper_gtsr_oreca_00", US),
        GCF_ALIAS("s2000_99",             "s2000_a_99",                US),
        GCF_ALIAS("s2000_99_ext",         "s2000_a_99_ext",            US),
        GCF_ALIAS("s2000_99",             "s2000_e_99",                EUROPE),
        GCF_ALIAS("s2000_99_ext",         "s2000_e_99_ext",            EUROPE)
    };
#undef GCF_ALIAS

    /* an unsigned compare: loop.c's pointer test is sltu */
    for (u32 i = 0; i < sizeof(aliases) / sizeof(aliases[0]); i++) {
        int country = g_00352FD8.value;
        const char *to;

        if ((aliases[i].countries >> country) & 1 && strcmp(aliases[i].from, code) == 0)
            to = aliases[i].to;
        else
            to = 0;
        if (to != 0) {
            if (replaced != 0)
                *replaced = 1;
            return to;
        }
    }
    return code;
}

/* Caller: PrizeSetFlags::ratio.  The number of prize sets with exactly
 * one car. */
// FUNC 0x0021DA90 ProductConfig::singleCarPrizeSetCount
int ProductConfig::singleCarPrizeSetCount()
{
    int singles = 0;

    for (int i = 0; i < prizes->count; i++) {
        if (prizes->list[i]->count == 1)
            singles++;
    }
    return singles;
}

/* Caller: PrizeCarList::PrizeCarList. */
// FUNC 0x0021DAE0 ProductConfig::prizeSetCount
int ProductConfig::prizeSetCount()
{
    return prizes->count;
}

/* Caller: PrizeCarList::PrizeCarList. */
// FUNC 0x0021DAF0 ProductConfig::prizeSetName
const char *ProductConfig::prizeSetName(int i)
{
    return prizes->list[i]->name;
}

/* The prize set `set' (binary search; no NULL check). */
// FUNC 0x0021DB10 ProductConfig::findPrizeSet
GcfPrizeSet *ProductConfig::findPrizeSet(const char *set)
{
    return *gcf_find_prize_set(prizes->list, prizes->count, set);
}

/* Callers: car_get_prizes, PrizeCarList::PrizeCarList. */
// FUNC 0x0021DB40 ProductConfig::prizeCarCount
int ProductConfig::prizeCarCount(const char *set)
{
    return findPrizeSet(set)->count;
}

/* Callers: car_get_prizes, PrizeSetFlags::carCode.  The regional code of
 * car i of the set, NULL out of range. */
// FUNC 0x0021DB60 ProductConfig::prizeCarCode
const char *ProductConfig::prizeCarCode(const char *set, int i, int *replaced)
{
    GcfPrizeSet *prizeSet = findPrizeSet(set);

    if (replaced != 0)
        *replaced = 0;
    if (i < 0)
        return 0;
    if (i < prizeSet->count)
        return gcf_regional_car_code(prizeSet->cars[i]->code, replaced);
    return 0;
}

/* Caller: car_get_prizes (its u32 colour).  (u32)-1 out of range (retail
 * loads the unsigned constant unextended: lui / ori 0xFFFF). */
// FUNC 0x0021DBD8 ProductConfig::prizeCarColor
u32 ProductConfig::prizeCarColor(const char *set, int i)
{
    GcfPrizeSet *prizeSet = findPrizeSet(set);

    if (i >= 0 && i < prizeSet->count)
        return prizeSet->cars[i]->color;
    return (u32)-1;
}

/* Callers: PrizeSetFlags::set, PrizeSetFlags::isSet.  The index of the set
 * (garbage if absent). */
// FUNC 0x0021DC38 ProductConfig::prizeSetIndex
u32 ProductConfig::prizeSetIndex(const char *set)
{
    GcfPrizeSet **list = prizes->list;

    return (u32)((char *)gcf_find_prize_set(list, prizes->count, set) -
                 (char *)list) >> 2;
}

/* Caller: car_get_prizes (no range check). */
// FUNC 0x0021DC78 ProductConfig::prizeCarWeight
int ProductConfig::prizeCarWeight(const char *set, int i)
{
    return findPrizeSet(set)->cars[i]->weight;
}

/* Caller: servicePrice (binary search; no NULL check). */
// FUNC 0x0021DCB0 ProductConfig::findService
GcfNamed *ProductConfig::findService(const char *name)
{
    return *gcf_find_service(services->list, services->count, name);
}

/* 11 callers ("car_wash", "oil_change", "initial", "wheel", ...).  The
 * price, x100 in Japan (yen). */
// FUNC 0x0021DCE0 ProductConfig::servicePrice
int ProductConfig::servicePrice(const char *name)
{
    int price = findService(name)->value;

    if (g_00352FD8.value == COUNTRY_JP)
        price *= 100;
    return price;
}

/* Callers: MemorycardDemoFileManager::countDemos / getDemoName (T079). */
// FUNC 0x0021DD18 ProductConfig::cardDemoCount
int ProductConfig::cardDemoCount()
{
    return cardDemos->count;
}

// FUNC 0x0021DD28 ProductConfig::cardDemoName
const char *ProductConfig::cardDemoName(int i)
{
    return cardDemos->list[i]->name;
}

/* The level demo i needs (GameStatus::isLicenseSPassed() must reach it). */
// FUNC 0x0021DD48 ProductConfig::cardDemoLevel
int ProductConfig::cardDemoLevel(int i)
{
    return cardDemos->list[i]->value;
}

/* Caller: game_init.  Builds the instance once. */
// FUNC 0x0021DD68 product_config_init
void product_config_init(void)
{
    /* implicit: operator new(0x2C), then the constructor; the landing pad calls
     * operator delete on the object if the constructor throws. */
    if (g_00352AB0 == 0) {
        /* in a local: retail loads the address into $s0 before operator new */
        const char *path = "gameconf/product.gcf";
        g_00352AB0 = new ProductConfig(path);
    }
}

/* 62 callers. */
// FUNC 0x0021DDE0 product_config
ProductConfig *product_config(void)
{
    return g_00352AB0;
}

/* The four binary searches below are the same code (? one template
 * instantiated per entry type): the slot of `list' (n entries sorted by
 * name, signed char order) whose entry's name is `name', NULL if none.
 * The project's name-search shape (ref/matching/u064/notes.md): an early
 * count test and a tail call per half (egcs turns them into jumps). */

/* Callers: findEvent, hasEvent. */
// FUNC 0x0021DDE8 gcf_find_event
GcfNamed **gcf_find_event(GcfNamed **list, int n, const char *name)
{
    if (n <= 0)
        return 0;
    GcfNamed **mid = &list[n / 2];
    const char *entryName = (*mid)->name;
    for (int i = 0; ; i++) {
        if ((s8)name[i] != (s8)entryName[i]) {
            if ((s8)name[i] < (s8)entryName[i]) {
                int half = n / 2;
                if (half <= 0)
                    return 0;
                return gcf_find_event(list, half, name);
            }
            int rest = n - n / 2 - 1;
            if (rest <= 0)
                return 0;
            return gcf_find_event(mid + 1, rest, name);
        }
        if (name[i] == '\0')
            break;
    }
    return mid;
}

/* Caller: find1C. */
// FUNC 0x0021DE88 gcf_find_1C
GcfNamed **gcf_find_1C(GcfNamed **list, int n, const char *name)
{
    if (n <= 0)
        return 0;
    GcfNamed **mid = &list[n / 2];
    const char *entryName = (*mid)->name;
    for (int i = 0; ; i++) {
        if ((s8)name[i] != (s8)entryName[i]) {
            if ((s8)name[i] < (s8)entryName[i]) {
                int half = n / 2;
                if (half <= 0)
                    return 0;
                return gcf_find_1C(list, half, name);
            }
            int rest = n - n / 2 - 1;
            if (rest <= 0)
                return 0;
            return gcf_find_1C(mid + 1, rest, name);
        }
        if (name[i] == '\0')
            break;
    }
    return mid;
}

/* Callers: findPrizeSet, prizeSetIndex. */
// FUNC 0x0021DF28 gcf_find_prize_set
GcfPrizeSet **gcf_find_prize_set(GcfPrizeSet **list, int n, const char *name)
{
    if (n <= 0)
        return 0;
    GcfPrizeSet **mid = &list[n / 2];
    const char *entryName = (*mid)->name;
    for (int i = 0; ; i++) {
        if ((s8)name[i] != (s8)entryName[i]) {
            if ((s8)name[i] < (s8)entryName[i]) {
                int half = n / 2;
                if (half <= 0)
                    return 0;
                return gcf_find_prize_set(list, half, name);
            }
            int rest = n - n / 2 - 1;
            if (rest <= 0)
                return 0;
            return gcf_find_prize_set(mid + 1, rest, name);
        }
        if (name[i] == '\0')
            break;
    }
    return mid;
}

/* Caller: findService. */
// FUNC 0x0021DFC8 gcf_find_service
GcfNamed **gcf_find_service(GcfNamed **list, int n, const char *name)
{
    if (n <= 0)
        return 0;
    GcfNamed **mid = &list[n / 2];
    const char *entryName = (*mid)->name;
    for (int i = 0; ; i++) {
        if ((s8)name[i] != (s8)entryName[i]) {
            if ((s8)name[i] < (s8)entryName[i]) {
                int half = n / 2;
                if (half <= 0)
                    return 0;
                return gcf_find_service(list, half, name);
            }
            int rest = n - n / 2 - 1;
            if (rest <= 0)
                return 0;
            return gcf_find_service(mid + 1, rest, name);
        }
        if (name[i] == '\0')
            break;
    }
    return mid;
}
