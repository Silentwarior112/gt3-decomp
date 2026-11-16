/* T510 - VolDevice: a mounted volume (gt3.vol), in the global list
 * g_00352F58.  0x0023DCA8..0x0023E0F0. */
#include "engine/io/vol_internal.h"
#include "engine/io/T510_vol_device.h"

/* The list of the volumes (head): T510's .sdata, 0x00352F58 (gp-0x4A98;
 * the 4 zero bytes after it are the 8-alignment of T501's .sdata). */
VolDevice *g_00352F58 = NULL;                   /* 0x00352F58 */

// FUNC 0x0023DCA8 vol_find_device
//   The volume named `name' (strcmp with VolDevice::name), NULL if none.
//   No callers.
VolDevice *vol_find_device(const char *name)
{
    int intrOn = intr_suspend();
    VolDevice *dev = g_00352F58;
    while (dev != NULL && strcmp(dev->name, name) != 0)
        dev = dev->next;
    if (intrOn)
        __asm__ volatile("ei");
    return dev;
}

// FUNC 0x0023DD18 VolDevice::VolDevice
//   Links the device in at the head of the list (interrupts off).  (Its EH
//   landing pad, a bare __throw, is the cleanup of the VolToc member: VolToc
//   has a user destructor.)
//   Caller: the static constructor of vol_device_object (T503_static_init).
VolDevice::VolDevice() : toc()
{
    int intrOn = intr_suspend();
    prev = NULL;
    next = g_00352F58;
    g_00352F58 = this;
    if (intrOn)
        __asm__ volatile("ei");
    toc_buf = NULL;
}

// FUNC 0x0023DD80 VolDevice::~VolDevice
//   Frees the TOC buffer and unlinks the device (interrupts off).
//   Caller: T503's static destructor (0x00241BA8).
VolDevice::~VolDevice()
{
    freeToc();
    int intrOn = intr_suspend();
    if (prev != NULL)
        prev->next = next;
    else
        g_00352F58 = next;
    if (next != NULL)
        next->prev = prev;
    if (intrOn)
        __asm__ volatile("ei");
}

// FUNC 0x0023DE10 vol_mount
//   Mounts the volume file `name': type 0 = on the disc (found with
//   iso9660_find, retried until it is), type 2 = a host file.  The TOC is
//   `nsect' sectors long; nsect 0 = read the first 0x40 bytes and take
//   hdr[3] + hdr[4] (header + body size) rounded up to sectors.  The TOC is
//   read into a memalign(0x40) buffer, decoded if needed and its current
//   directory set to the root.  Other types: only type and name are stored.
//   Caller: vol_init (type 0, nsect 0).
//   Not exact (97.7%): the record's fields are read with T526's
//   iso_read_u32 (retail's split lwl / lwr bases for the size, as u111's
//   file_resolve), but rec gets $a1 where retail has $a2, so the lsn read
//   goes to $v0 and is copied to $a1 (one insn more).  global.c's
//   expand_preferences gives rec the preferences of the lsn (the call's $a1)
//   because rec dies in that read; with rec kept live past it (a dummy asm
//   use, experiment only) rec gets $a2 and the reads are exact.  Tried: the
//   lsn as a plain packed read, the loop as while / for forms, a const or
//   byte pointer, helpers taking the record (or the extent), the lsn read
//   first, size2 from extent.size.
/* vol_mount's helper: the TOC's body size (word 2), 0 without a TOC.  An
 * inline helper: its test comes out as retail's (the call taken when data !=
 * NULL).  (After the marker, so that callcheck finds the call in vol_mount.) */
static inline u32 vol_toc_body_size(const VolToc *toc)
{
    if (toc->data == NULL)
        return 0;
    return vol_toc_word(toc, 8);
}

void vol_mount(VolDevice *dev, int type, const char *name, u32 nsect)
{
    u8 buf[0x40 + 0x3F];                        /* sp+0x00: room for a 64-byte aligned header */
    u32 *hdr = (u32 *)(((s32)buf + 0x3F) & ~0x3F);

    dev->type = (VolDeviceType)type;
    dev->name = name;
    switch (type) {
    case VOL_DISC_VOLUME: {
        IsoDirRecord *rec;
        do
            rec = iso9660_find(name);
        while (rec == NULL);
        dev->extent.mounted = 1;
        dev->extent.size = iso_read_u32(&rec->size);
        dev->extent.size2 = iso_read_u32(&rec->size);
        dev->extent.lsn = iso_read_u32(&rec->lsn);
        if (nsect == 0) {
            pdicdvd_read(hdr, dev->extent.lsn, 0x40, 1);
            nsect = (hdr[3] + hdr[4] + 0x7FF) >> VOL_SECTOR_SHIFT;
        }
        u32 bytes = nsect << VOL_SECTOR_SHIFT;
        dev->toc_buf = (u8 *)memalign(0x40, bytes);
        pdicdvd_read(dev->toc_buf, dev->extent.lsn, bytes, 1);
        dev->toc.data = dev->toc_buf;
        if (!vol_toc_is_plain(&dev->toc))
            vol_toc_decrypt(&dev->toc);
        vol_toc_set_root(&dev->toc);
        break;
    }
    case VOL_HOST_VOLUME: {
        dev->extent.mounted = 1;
        u32 size = host_file_size(name);
        dev->extent.lsn = 0;
        dev->extent.size = size;
        dev->extent.size2 = size;
        if (nsect == 0) {
            host_read_file(dev->name, hdr, 0, 0x40);
            nsect = (hdr[3] + hdr[4] + 0x7FF) >> VOL_SECTOR_SHIFT;
        }
        u32 bytes = nsect << VOL_SECTOR_SHIFT;
        dev->toc_buf = (u8 *)memalign(0x40, bytes);
        host_read_file(dev->name, dev->toc_buf, 0, bytes);
        /* Quirk: the size check reads word 2 of the PREVIOUS toc.data (it is
         * only pointed at the new buffer below); 0 while it is NULL. */
        if (vol_toc_body_size(&dev->toc) >= bytes)
            printf("  FSO buffer overflow. (%d >= %d)\n", vol_toc_body_size(&dev->toc), bytes);
        dev->toc.data = dev->toc_buf;
        if (!vol_toc_is_plain(&dev->toc))
            vol_toc_decrypt(&dev->toc);
        vol_toc_set_root(&dev->toc);
        break;
    }
    }
}

// FUNC 0x0023E048 VolDevice::freeToc
//   Callers: ~VolDevice, vol_shutdown.
void VolDevice::freeToc()
{
    if (toc_buf != NULL) {
        free(toc_buf);
        toc_buf = NULL;
    }
}

// FUNC 0x0023E080 vol_toc_word
//   The TOC word at byte offset `off'.
u32 vol_toc_word(const VolToc *toc, int off)
{
    return *(u32 *)(toc->data + off);
}

// FUNC 0x0023E090 vol_toc_set_root
//   Current directory := the root record (0x14).
void vol_toc_set_root(VolToc *toc)
{
    toc->cwd = VOL_ROOT_DIR;
}

// FUNC 0x0023E0A0 vol_toc_is_plain
//   1 when the TOC starts with the magic "RoFS" (0 without data).
int vol_toc_is_plain(const VolToc *toc)
{
    const u8 *data = toc->data;
    if (data == NULL)
        return 0;
    int plain = 0;
    if (data[0] == 'R' && data[1] == 'o' && data[2] == 'F')
        plain = data[3] == 'S';
    return plain;
}
