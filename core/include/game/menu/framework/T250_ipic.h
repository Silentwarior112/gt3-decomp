/* T250_ipic.h - gt2k_menu::Ipic and gt2k_menu::IpicArchive (translation
 * unit T250, unit u071), 0x001CC5F8..0x001CC8F8.
 *
 * An IpicArchive is a loaded file: {?, count, count x {offset, name[0x1C]}}
 * followed by IPU pictures.  find / get point an Ipic at one picture's
 * (unaligned) header; Ipic::decode expands it with the IPU decoder
 * (ipu_decode_image 0x00264E58, G002) into width * height * 4 bytes of 16 x 16 pixel blocks
 * (MpegImage::setIpic, T249). */
#ifndef GAME_T250_IPIC_H
#define GAME_T250_IPIC_H

#include "game/game.h"

/* A picture's header, unaligned (packed): G002's IpuImageHeader (+0x04 u32
 * bitstream size, +0x08 s16 width, +0x0A s16 height, +0x10 u8 picture
 * flags; u071's IpicHeader before the sweep, the same 17 bytes). */
struct IpuImageHeader;                      /* G002_mpeg_frameq.h (pointers only) */

namespace gt2k_menu {
/* The archive's records (T250 only; pointers here). */
struct IpicEntry;                           /* 0x20 bytes: {u32 offset (from the archive start);
                                               char name[0x1C];} */
struct IpicArchiveData;                     /* the file: {u32 unk0; int count; IpicEntry entries[];} */

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T250_ipic.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* vtable 0x003370C8 (1 slot).  typeinfo __tfQ29gt2k_menu4Ipic 0x001CC8B8
 * (__user_type_info 0x00353400 gp-0x45F0, no bases).  0x18 bytes of data,
 * vptr +0x18; size 0x1C. */
class Ipic {
public:
    Ipic();                                 /* 0x001CC5F8 */
    virtual ~Ipic();                        /* [0] +0x0C 0x001CC610 */
    void set(const IpuImageHeader *hdr);    /* 0x001CC640 the picture at hdr */
    void decode(void *dst);                 /* 0x001CC6B8 IPU-decodes it into dst (size bytes) */
    int  getHeight();                       /* 0x001CC8A0 */
    int  getWidth();                        /* 0x001CC8A8 */
    int  getSize();                         /* 0x001CC8B0 */

    const IpuImageHeader *data;             /* 0x00 */
    u32               bitstreamSize;        /* 0x04 the header's +0x04 (unk04 before the sweep) */
    int               size;                 /* 0x08 width * height * 4 */
    int               width;                /* 0x0C */
    int               height;               /* 0x10 */
    int               flags;                /* 0x14 the header's byte +0x10: the picture flags (unk14) */
    /*                vptr                     0x18 */
};

/* The inline accessors 0x001CC8A0..0x001CC8B0: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
inline int Ipic::getSize() { return size; }  /* 0x001CC8B0 */
inline int Ipic::getWidth() { return width; }  /* 0x001CC8A8 */
inline int Ipic::getHeight() { return height; }  /* 0x001CC8A0 */
EGCS_SIZE(gt2k_menu::Ipic, 0x1C);

/* vtable 0x003370B8 (1 slot).  typeinfo __tfQ29gt2k_menu11IpicArchive
 * 0x001CC860 (__user_type_info 0x00353408 gp-0x45E8, no bases).  0xC bytes
 * of data, vptr +0x0C; size 0x10. */
class IpicArchive {
public:
    IpicArchive(const IpicArchiveData *d);  /* 0x001CC6E0 */
    virtual ~IpicArchive();                 /* [0] +0x0C 0x001CC708 */
    int find(Ipic *pic, const char *name);  /* 0x001CC738 pic at the picture `name': 1, 0 if none */
    int get(Ipic *pic, int index);          /* 0x001CC7E8 pic at picture index: 1, 0 past the end */
    const char *getName(int index);         /* 0x001CC838 NULL past the end */

    const IpicArchiveData *base;            /* 0x00 the file */
    int                    count;           /* 0x04 */
    const IpicEntry       *entries;         /* 0x08 */
    /*                     vptr                0x0C */
};
EGCS_SIZE(gt2k_menu::IpicArchive, 0x10);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu11IpicArchive(void);  /* 0x001CC860 */
void *__tfQ29gt2k_menu4Ipic(void);          /* 0x001CC8B8 */
}
#endif

#endif /* GAME_T250_IPIC_H */
