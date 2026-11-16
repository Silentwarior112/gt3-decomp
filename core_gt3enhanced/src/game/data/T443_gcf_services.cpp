/* T443 - .gcf section type 0xA, the service prices (sorted by name) (? name),
 * 0x0021E5F8..0x0021E698. */
#include "game/data/gcf_internal.h"

/* Caller: gcf_relocate_table_0A. */
// FUNC 0x0021E5F8 gcf_relocate_named_0A
GcfNamed *gcf_relocate_named_0A(GcfNamed *entry)
{
    long off;

    entry->name = GCF_REL(const char *, entry, off, entry->name);
    return entry;
}

/* Caller: ProductConfig::ProductConfig. */
// FUNC 0x0021E610 gcf_relocate_table_0A
GcfNamedTable *gcf_relocate_table_0A(GcfNamedTable *table)
{
    long off;
    int i = 0;

    table->list = GCF_REL(GcfNamed **, table, off, table->list);
    for (; i < table->count; i++)
        table->list[i] = gcf_relocate_named_0A(GCF_REL(GcfNamed *, table, off, table->list[i]));
    return table;
}
