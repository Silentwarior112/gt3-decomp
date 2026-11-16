/* T445 - .gcf section type 0x4, the events (sorted by name) (? name),
 * 0x0021E738..0x0021E7D8. */
#include "game/data/gcf_internal.h"

/* Caller: gcf_relocate_table_04. */
// FUNC 0x0021E738 gcf_relocate_named_04
GcfNamed *gcf_relocate_named_04(GcfNamed *entry)
{
    long off;

    entry->name = GCF_REL(const char *, entry, off, entry->name);
    return entry;
}

/* Caller: ProductConfig::ProductConfig. */
// FUNC 0x0021E750 gcf_relocate_table_04
GcfNamedTable *gcf_relocate_table_04(GcfNamedTable *table)
{
    long off;
    int i = 0;

    table->list = GCF_REL(GcfNamed **, table, off, table->list);
    for (; i < table->count; i++)
        table->list[i] = gcf_relocate_named_04(GCF_REL(GcfNamed *, table, off, table->list[i]));
    return table;
}
