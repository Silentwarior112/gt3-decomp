/* T506_image_pack.h - image packs (.imgs files, ? name) (translation unit T506,
 * unit u111), 0x00242058..0x00242298.
 *
 * A pack is loaded whole (file_read_blocking, unpacked, buffer allocated:
 * free() releases it), then relocated in place: the offset of the first image
 * becomes a pointer, and each entry's image index a pointer to that image
 * (walking the Tex1 images by their size word +0x0C), relocated by
 * tex1_relocate.  Entries are sorted by name (binary search, signed char
 * compare); "00no_texture" stands in for a missing name.  C++ only. */
#ifndef GAME_T506_IMAGE_PACK_H
#define GAME_T506_IMAGE_PACK_H

#include "game/game.h"

#ifdef __cplusplus

struct Tex1Image;                               /* G001 (pointers only) */

/* One named image of a pack. */
struct ImagePackEntry {
    char       name[0x3C];                      /* 0x00 */
    Tex1Image *image;                           /* 0x3C in the file: the image's index in the pack */
};
EGCS_SIZE(ImagePackEntry, 0x40);                /* the stride of ImagePack::entries */

/* An .imgs file as loaded (variable size). */
struct ImagePack {
    u8             unk_00[8];                   /* 0x00 */
    int            count;                       /* 0x08 entries */
    u8            *images;                      /* 0x0C in the file: the offset of the first image */
    ImagePackEntry entries[1];                  /* 0x10 [count], sorted by name */
};

ImagePack *image_pack_load(const char *path);   /* 0x00242058 */
/* The image named name, else the one named "00no_texture", else NULL. */
Tex1Image *image_pack_find(ImagePack *pack, const char *name);  /* 0x00242198 */

#endif /* __cplusplus */

#endif /* GAME_T506_IMAGE_PACK_H */
