/* T436 - .gcf file header relocation (? name), 0x0021E068..0x0021E0B8. */
#include "game/data/gcf_internal.h"

/* Caller: ProductConfig::ProductConfig.  The section table and each
 * section's data become pointers (offsets from the file start). */
// FUNC 0x0021E068 gcf_relocate_header
GcfHeader *gcf_relocate_header(GcfHeader *hdr)
{
    long off;

    hdr->sections = GCF_REL(GcfSectionRef *, hdr, off, hdr->sections);
    for (int i = 0; i < hdr->count; i++)
        hdr->sections[i].data = GCF_REL(void *, hdr, off, hdr->sections[i].data);
    return hdr;
}
