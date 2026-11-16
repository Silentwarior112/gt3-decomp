#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/menu/framework/T250_ipic.cpp"
#pragma implementation "src/game/menu/framework/T250_ipic.cpp"
#pragma implementation "T250_ipic.h"
#endif
/* T250 - gt2k_menu::Ipic, gt2k_menu::IpicArchive.  0x001CC5F8..0x001CC8F8.
 *
 * An IpicArchive is a loaded file: {?, count, count x {offset, name[0x1C]}}
 * followed by IPU still images.  find / get point an Ipic at one picture's
 * (unaligned) header; Ipic::decode expands it with the IPU decoder
 * (ipu_decode_image, G002) into width * height * 4 bytes of 16 x 16 pixel
 * blocks (MpegImage::setIpic).
 *
 * Data (retail .rodata 0x003370B8..0x00337110): the vtables of IpicArchive and
 * Ipic, then their type names (IpicArchive, Ipic) only.  (The
 * SmartIndexT<int> name after them, 0x00337110, is T251's.) */
#include "game/menu/framework/T250_ipic.h"
#include "engine/media/G002_mpeg_frameq.h"          /* ipu_decode_image, IpuImageHeader */
#include "libc.h"                           /* strcmp (core 0x0027EA40) */
#include "game/menu/framework/render_base_internal.h"                           /* IpicEntry, IpicArchiveData */

namespace gt2k_menu {

// FUNC 0x001CC5F8 gt2k_menu::Ipic::Ipic
Ipic::Ipic()
{
}

// FUNC 0x001CC610 gt2k_menu::Ipic::~Ipic
Ipic::~Ipic()
{
    /* the vptr; operator delete with flags & 1 */
    /* implicit: operator delete (deleting destructor) */
}

/* The width / height halfwords are read byte by byte (s16), the size word
 * with lwl / lwr: the header is not aligned. */
// FUNC 0x001CC640 gt2k_menu::Ipic::set
void Ipic::set(const IpuImageHeader *hdr)
{
    data = hdr;
    s16 w = hdr->width;
    width = w;
    s16 h = hdr->height;
    height = h;
    bitstreamSize = hdr->size;
    size = w * h * 4;
    flags = hdr->flags;
}

// FUNC 0x001CC6B8 gt2k_menu::Ipic::decode
void Ipic::decode(void *dst)
{
    ipu_decode_image((u32)dst, data);
}

// FUNC 0x001CC6E0 gt2k_menu::IpicArchive::IpicArchive
IpicArchive::IpicArchive(const IpicArchiveData *file)
{
    count   = file->count;
    entries = file->entries;
    base    = file;
}

// FUNC 0x001CC708 gt2k_menu::IpicArchive::~IpicArchive
IpicArchive::~IpicArchive()
{
    /* the vptr; operator delete with flags & 1 */
    /* implicit: operator delete (deleting destructor) */
}

/* Points pic at the picture called name: 1, or 0 if there is none. */
// FUNC 0x001CC738 gt2k_menu::IpicArchive::find
int IpicArchive::find(Ipic *pic, const char *name)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            pic->set((const IpuImageHeader *)((const u8 *)base + entries[i].offset));
            return 1;
        }
    }
    return 0;
}

/* Points pic at picture number index: 1, or 0 past the end. */
// FUNC 0x001CC7E8 gt2k_menu::IpicArchive::get
int IpicArchive::get(Ipic *pic, int index)
{
    if (index < count) {
        pic->set((const IpuImageHeader *)((const u8 *)base + entries[index].offset));
        return 1;
    }
    return 0;
}

// FUNC 0x001CC838 gt2k_menu::IpicArchive::getName
const char *IpicArchive::getName(int index)
{
    if (index < count)
        return entries[index].name;
    return NULL;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__user_type_info, no bases). */
// FUNC 0x001CC860 __tfQ29gt2k_menu11IpicArchive
extern "C" void *__tfQ29gt2k_menu11IpicArchive(void)
{
    if (__tiQ29gt2k_menu11IpicArchive[0] == 0)
        __rtti_user(__tiQ29gt2k_menu11IpicArchive, "Q29gt2k_menu11IpicArchive");
    return __tiQ29gt2k_menu11IpicArchive;
}
#endif

namespace gt2k_menu {

// FUNC 0x001CC8A0 gt2k_menu::Ipic::getHeight
/* inline in game/menu/framework/T250_ipic.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001CC8A8 gt2k_menu::Ipic::getWidth
/* inline in game/menu/framework/T250_ipic.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001CC8B0 gt2k_menu::Ipic::getSize
/* inline in game/menu/framework/T250_ipic.h; egcs emits this copy (#pragma implementation) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__user_type_info, no bases). */
// FUNC 0x001CC8B8 __tfQ29gt2k_menu4Ipic
extern "C" void *__tfQ29gt2k_menu4Ipic(void)
{
    if (__tiQ29gt2k_menu4Ipic[0] == 0)
        __rtti_user(__tiQ29gt2k_menu4Ipic, "Q29gt2k_menu4Ipic");
    return __tiQ29gt2k_menu4Ipic;
}
#endif
