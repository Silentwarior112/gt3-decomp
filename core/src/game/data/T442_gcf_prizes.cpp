/* T442 - .gcf section type 0xF, the prize car sets (? name),
 * 0x0021E4B8..0x0021E5F8. */
#include "game/data/gcf_internal.h"

/* Caller: gcf_relocate_prize_set.  Only the code (+0x4) is an offset. */
// FUNC 0x0021E4B8 gcf_relocate_prize_car
GcfPrizeCar *gcf_relocate_prize_car(GcfPrizeCar *car)
{
    long off;

    car->code = GCF_REL(const char *, car, off, car->code);
    return car;
}

/* Caller: gcf_relocate_prizes. */
// FUNC 0x0021E4D0 gcf_relocate_prize_set
GcfPrizeSet *gcf_relocate_prize_set(GcfPrizeSet *set)
{
    long off;
    int i = 0;

    set->name = GCF_REL(const char *, set, off, set->name);
    set->cars = GCF_REL(GcfPrizeCar **, set, off, set->cars);
    for (; i < set->count; i++)
        set->cars[i] = gcf_relocate_prize_car(GCF_REL(GcfPrizeCar *, set, off, set->cars[i]));
    return set;
}

/* Caller: ProductConfig::ProductConfig. */
// FUNC 0x0021E570 gcf_relocate_prizes
GcfPrizeSection *gcf_relocate_prizes(GcfPrizeSection *sec)
{
    long off;
    int i = 0;

    sec->list = GCF_REL(GcfPrizeSet **, sec, off, sec->list);
    for (; i < sec->count; i++)
        sec->list[i] = gcf_relocate_prize_set(GCF_REL(GcfPrizeSet *, sec, off, sec->list[i]));
    return sec;
}
