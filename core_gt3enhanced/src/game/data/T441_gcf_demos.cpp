/* T441 - .gcf section type 0xC, the attract demos (? name),
 * 0x0021E3F0..0x0021E4B8. */
#include "game/data/gcf_internal.h"

/* Caller: gcf_relocate_demos.  The three strings, unconditionally: shopCar
 * cannot be NULL afterwards, so run_attract_demo's NULL test never fails
 * (? the file gives an empty string for "none"). */
// FUNC 0x0021E3F0 gcf_relocate_demo
GcfDemo *gcf_relocate_demo(GcfDemo *demo)
{
    long off;

    demo->race = GCF_REL(const char *, demo, off, demo->race);
    demo->course = GCF_REL(const char *, demo, off, demo->course);
    demo->shopCar = GCF_REL(const char *, demo, off, demo->shopCar);
    return demo;
}

/* Caller: ProductConfig::ProductConfig. */
// FUNC 0x0021E430 gcf_relocate_demos
GcfDemoSection *gcf_relocate_demos(GcfDemoSection *sec)
{
    long off;
    int i = 0;

    sec->list = GCF_REL(GcfDemo **, sec, off, sec->list);
    for (; i < sec->count; i++)
        sec->list[i] = gcf_relocate_demo(GCF_REL(GcfDemo *, sec, off, sec->list[i]));
    return sec;
}
