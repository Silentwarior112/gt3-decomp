/* T440 - .gcf section type 0x6, a sorted name table (ProductConfig +0x1C;
 * its index is CourseBestTimes' slot) (? name), 0x0021E350..0x0021E3F0. */
#include "game/data/gcf_internal.h"

/* Caller: gcf_relocate_table_06. */
// FUNC 0x0021E350 gcf_relocate_named_06
GcfNamed *gcf_relocate_named_06(GcfNamed *entry)
{
    long off;

    entry->name = GCF_REL(const char *, entry, off, entry->name);
    return entry;
}

/* Caller: ProductConfig::ProductConfig. */
// FUNC 0x0021E368 gcf_relocate_table_06
GcfNamedTable *gcf_relocate_table_06(GcfNamedTable *table)
{
    long off;
    int i = 0;

    table->list = GCF_REL(GcfNamed **, table, off, table->list);
    for (; i < table->count; i++)
        table->list[i] = gcf_relocate_named_06(GCF_REL(GcfNamed *, table, off, table->list[i]));
    return table;
}
