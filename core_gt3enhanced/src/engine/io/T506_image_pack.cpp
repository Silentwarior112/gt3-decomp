/* T506 - image packs (.imgs files, ? name) (u111).  0x00242058..0x00242298.
 *
 * A pack is loaded whole (file_read_blocking, unpacked), then relocated in
 * place: the offset of the first image becomes a pointer, and each entry's
 * image index becomes a pointer to that image (walking the Tex1 images by
 * their size field), relocated by tex1_relocate.  Entries are sorted by
 * name and found by binary search; "00no_texture" stands in for a missing
 * name.
 */
#include "engine/io/file_loader_internal.h"
#include "engine/io/T506_image_pack.h"

// FUNC 0x00242058 image_pack_load
//   ? name.  Loads path (buffer allocated) and relocates it.  Caller:
//   0x00191830.
ImagePack *image_pack_load(const char *path)
{
    ImagePack *pack =
        (ImagePack *)file_read_blocking(path, NULL, 0, 1, &vol_device_object);

    image_pack_relocate(pack);
    return pack;
}

// FUNC 0x002420A0 image_pack_free
//   ? name.  free(pack).  No callers.
void image_pack_free(ImagePack *pack)
{
    free(pack);
}

// FUNC 0x002420C0 image_pack_nth
//   ? name.  The index-th image (index <= 0: the first).
Tex1Image *image_pack_nth(ImagePack *pack, int index)
{
    Tex1Image *image = (Tex1Image *)pack->images;

    for (int i = 0; i < index; i++)
        image = (Tex1Image *)((u8 *)image + image->size);
    return image;
}

// FUNC 0x002420F0 image_pack_entry_relocate
//   ? name.  The entry's index -> its image, then tex1_relocate.
void image_pack_entry_relocate(ImagePackEntry *entry, ImagePack *pack)
{
    entry->image = image_pack_nth(pack, (int)entry->image);
    tex1_relocate(entry->image);
}

// FUNC 0x00242128 image_pack_relocate
//   ? name.  images += pack, then every entry.
void image_pack_relocate(ImagePack *pack)
{
    int i;

    pack->images += (u32)pack;
    for (i = 0; i < pack->count; i++)
        image_pack_entry_relocate(&pack->entries[i], pack);
}

// FUNC 0x00242198 image_pack_find
//   The image named name, else the one named "00no_texture", else NULL.
//   Callers: 0x00195208, 0x001965A8, 0x0019D030.
Tex1Image *image_pack_find(ImagePack *pack, const char *name)
{
    ImagePackEntry *entry = image_pack_bsearch(pack->entries, pack->count, name);

    if (entry != NULL)
        return entry->image;
    entry = image_pack_bsearch(pack->entries, pack->count, "00no_texture");
    if (entry == NULL)
        return NULL;
    return entry->image;
}

// FUNC 0x00242200 image_pack_bsearch
//   ? name.  Binary search of count entries sorted by name (signed char
//   compare, as strcmp on lb).  Tail-recursive: egcs turns the self-calls
//   into jumps.  The shape of the other name searches (u064's
//   findValidFuncData, ref/matching/u064/notes.md): the new count in a
//   variable tested before the call, the character loop left by returns
//   and a break, `return entry' after it.
ImagePackEntry *image_pack_bsearch(ImagePackEntry *entries, int count, const char *key)
{
    if (count <= 0)
        return 0;
    ImagePackEntry *entry = &entries[count / 2];
    const char *name = entry->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return image_pack_bsearch(entries, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return image_pack_bsearch(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}
