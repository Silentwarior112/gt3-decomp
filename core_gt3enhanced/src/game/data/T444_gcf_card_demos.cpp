/* T444 - .gcf section type 0xD, the memory card demos (? name),
 * 0x0021E698..0x0021E738. */
#include "game/data/gcf_internal.h"

/* Caller: gcf_relocate_table_0D. */
// FUNC 0x0021E698 gcf_relocate_named_0D
GcfNamed *gcf_relocate_named_0D(GcfNamed *entry)
{
    long off;

    entry->name = GCF_REL(const char *, entry, off, entry->name);
    return entry;
}

/* Caller: ProductConfig::ProductConfig. */
// FUNC 0x0021E6B0 gcf_relocate_table_0D
GcfNamedTable *gcf_relocate_table_0D(GcfNamedTable *table)
{
    long off;
    int i = 0;

    table->list = GCF_REL(GcfNamed **, table, off, table->list);
    for (; i < table->count; i++)
        table->list[i] = gcf_relocate_named_0D(GCF_REL(GcfNamed *, table, off, table->list[i]));
    return table;
}
