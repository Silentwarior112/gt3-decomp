/* T437 - .gcf section type 0xE, the cars by category (? name),
 * 0x0021E0B8..0x0021E208. */
#include "game/data/gcf_internal.h"

/* Caller: gcf_relocate_car_category.  Offsets from the entry. */
// FUNC 0x0021E0B8 gcf_relocate_car
GcfCar *gcf_relocate_car(GcfCar *car)
{
    long off;

    car->code = GCF_REL(const char *, car, off, car->code);
    car->race = GCF_REL(const char *, car, off, car->race);
    return car;
}

/* Caller: gcf_relocate_cars.  Offsets from the category. */
// FUNC 0x0021E0E0 gcf_relocate_car_category
GcfCarCategory *gcf_relocate_car_category(GcfCarCategory *cat)
{
    long off;
    int i = 0;

    cat->name = GCF_REL(const char *, cat, off, cat->name);
    cat->cars = GCF_REL(GcfCar **, cat, off, cat->cars);
    for (; i < cat->count; i++)
        cat->cars[i] = gcf_relocate_car(GCF_REL(GcfCar *, cat, off, cat->cars[i]));
    return cat;
}

/* Caller: ProductConfig::ProductConfig.  Offsets from the section. */
// FUNC 0x0021E180 gcf_relocate_cars
GcfCarSection *gcf_relocate_cars(GcfCarSection *sec)
{
    long off;
    int i = 0;

    sec->list = GCF_REL(GcfCarCategory **, sec, off, sec->list);
    for (; i < sec->count; i++)
        sec->list[i] = gcf_relocate_car_category(GCF_REL(GcfCarCategory *, sec, off, sec->list[i]));
    return sec;
}
