/* T108 - gt2k_menu's memory card files, 0x00142790..0x00147870: one object
 * in retail, one file here (units u023 0x00142790..0x001447D8 and u024
 * 0x001447D8..0x00147870 share it).
 *
 * In address order:
 *   - the text helpers: card titles are Shift-JIS, the menu font is EUC-JP,
 *     so readInfo converts titles to EUC-JP (sjis_to_euc) and writeIconSys
 *     converts GT3's title back (euc_to_sjis);
 *   - MemorycardDir's date / id helpers, and the operations on a save
 *     directory "/<name>" every MemorycardFile shares (file list, size in
 *     KB, delete, copy to the other card with a progress callback);
 *   - the PS1 / PS2 / other-PS2 / damaged / dummy / deleted file classes
 *     and MemorycardGT3File's constructors;
 *   - (u024 from here) the GT3 save names (mcInitIconSys .. mcCountFiles):
 *     the prefix "BASCUS-97102", the type letter, 7 base-64 digits of a
 *     42-bit code;
 *   - MemorycardGT3File: the thumbnail file, the size of a save, the save
 *     sequence (saveFiles: format, directory, icons, icon.sys, data,
 *     thumbnail, with a progress callback), load, the comment change;
 *   - the four GT3 kinds (game data 'G', replay 'R', best lap / licence
 *     replay 'B', setting 'S'): constructors, search patterns, icon, title,
 *     filter and name code;
 *   - MemorycardFileList: the pool of file cells, lookups, eviction;
 *   - what egcs emits at the end of the unit: the copies of the template
 *     destructors, clones, implicit destructors, small virtual functions,
 *     inline accessors and typeinfo functions of the hierarchy (every
 *     class's key method is in this file, so its vtable, typeinfo function
 *     and inline members come out of this object).
 *
 * Every path is built the same way: '/' + name, then "/" + file.  The
 * operations of T501 (mc_*) return a status, MC_STATUS_OK on success; their
 * `port' is T501's card id (port * 4 + slot).
 *
 * The includes give retail's .rodata (0x0031B570..0x0031C376): <new>'s
 * "bad_alloc" first, then the literals, the vtables and the type names in
 * reverse class-definition order: T108's own (0x0031BFB8..0x0031C211), the
 * menu classes of T072's header (OperatorList, List<OperatorHolder>,
 * OperatorHolder, Cell<OperatorHolder>), RegurationResult, the garage car
 * list (GarageCarList, List<GarageCar>, GarageCar, Cell<GarageCar>) and
 * Operator (a non-polymorphic base: last).  So the garage car list and
 * RegurationResult are read first (retail's T108 saw them without using
 * them; not the whole GameStatus: no GarageWheel names), then the menu
 * headers, game/menu/memorycard/T108_memorycard_file.h last. */
// DATA_START 0x0031B570
#include "types.h"
#include <stddef.h>
#include <new>                              /* placement new (egcs checks its result for NULL); its
                                               "bad_alloc" opens T108's .rodata (0x0031B570) */
#include "libmc.h"                          /* sceMcFileAttrDupProhibit, sceMcFileAttrPS1, sceMcFileInfoAttr */
#include "libgraph.h"                       /* SCE_GS_PSMT4, SCE_GS_PSMCT16 */

/* seen by retail's T108, not used (their type names only) */
#include "game/save/T429_garage_car_list.h"      /* GarageCarList, List / Cell<GarageCar>, GarageCar */
#include "game/menu/gtmode/T045_reguration_result.h"    /* RegurationResult */

/* outside the unit */
#include "engine/pgl/G001_flare.h"                /* Tex1Image, GsTexRegs, gfx_alloc_tail, gfx_set_texture_regs,
                                               gfx_disable, pgl_push / pop_matrix, pgl_translate,
                                               vu0_scale, pgl_color4f, pgl_draw_strip, vu0_rand,
                                               gfx_vsync */
#include "runtime/newlib/G007_stdio.h"                /* memcpy, strcat, strchr, strcmp, strcpy, strncpy */
#include "game/menu/memorycard/T072_memorycard_icon_sprites.h" /* gt2k_menu::MemorycardIconSprites::draw, setPosition,
                                               setSize, setColor */
#include "game/menu/memorycard/T077_memorycard_icon.h"      /* McIconModel, McIconLight, icon_* */
#include "game/data/messages/T095_msg_memorycard.h"       /* g_002B1C68, the MEMORYCARDTEXT texts */
#include "game/sound/T465_clock.h"                /* clock_get */
#include "game/data/T489_car_database.h"         /* db_car_index, db_course_index */
#include "engine/io/T501_memorycard_manager.h"   /* mc_* */
#include "engine/io/T502_file_resolve.h"         /* file_size_retry */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "engine/gfx/T512_render_context.h"       /* g_002F4250 (VramAllocator::loadTex1) */
#include "engine/pdi/T519_pdicdvd.h"              /* struct CdClock (not T092_records_button_list.h: its
                                               Button lists would put three type names, Cell<Button>,
                                               List<Button>, SmartIndex<int>, in front of T108's) */
#include "engine/pdistd/T532_heap.h"                 /* malloc, memalign, free */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */

/* own translation unit, after the menu headers: its classes are defined
 * last, so their type names come first */
#include "game/menu/memorycard/T108_memorycard_file.h"      /* the file classes, MemorycardDir, McDirEntry,
                                               MemorycardIconSys, gt2k_game::GameThumbnail, the text
                                               helpers, the save name functions */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* type_info, __rtti_si (the __tf functions) */
#endif
#include "game/menu/memorycard/memorycard_file_internal.h"                           /* the unit's constants and data declarations */

/* "BASCUS-97102" once mcGetNamePrefix has built it: T108's only .data
 * variable, zeros in front of its frame info (egcs writes an uninitialised
 * C++ global there, whatever its place in the file). */
char g_002A2160[16];                        /* 0x002A2160 */

/* GT3's icon.sys, the template of new saves (menu/icon/icon.sys, read by
 * mcInitIconSys): a file static, T108's .bss (0x0049BEF0..0x0049C0B4). */
static gt2k_menu::MemorycardIconSys g_0049BEF0;    /* 0x0049BEF0 */

namespace gt2k_menu {

/* A built-in icon at the rectangle and colour of info: x .. a copied into
 * the sprites (+0xE00), then MemorycardIconSprites::draw(icon).  Inline in
 * every render() of the original (MemorycardIconRenderInfo has no method
 * for it).  The setters are inline members: retail loads each group (x y,
 * w h, then a r g b) before storing it. */
static inline void drawBuiltinIcon(MemorycardIconRenderInfo &info, int icon)
{
    MemorycardIconSprites *sprites = info.sprites;

    sprites->setPosition(info.x, info.y);
    sprites->setSize(info.w, info.h);
    sprites->setColor(info.r, info.g, info.b, info.a);
    sprites->draw(icon);
}

/* progress(step / total, ctx) when there is a callback: the step reports of
 * saveFiles (each one expanded in the original). */
static inline void reportProgress(MemorycardProgressFunc progress, void *ctx, int step, int total)
{
    if (progress != 0)
        progress((f32)step / (f32)total, ctx);
}

/* A best lap file of a licence test (its category is the licence, 9..14).
 * A macro: as an inline function's value, getIconFiles's ?: compiles to
 * the opposite compare and a branch (retail: sltu 5 < cat - 9, movn). */
#define MC_IS_LICENCE_CATEGORY(category) \
    ((category) >= MC_CATEGORY_LICENCE_FIRST \
     && (category) < MC_CATEGORY_LICENCE_FIRST + MC_CATEGORY_LICENCE_COUNT)

/* ======================================================================== */
/* Text helpers                                                             */
/* ======================================================================== */

/* Shift-JIS -> EUC-JP in place: a lead byte 0x81..0x9F / 0xE0..0xFC and its
 * trail byte become the EUC-JP pair; everything else is left alone.  Stops
 * at the terminator, or at a lead byte with no trail byte.
 * Not exact (84.8%, the shape is retail's): registers only.  Retail keeps
 * the comparison in v0 (so the row's sll stays out of the delay slot) and
 * the final value in a1; tried: one variable for lead / lo / result and
 * trail / row (every combination), an inline code converter, the ?: forms,
 * the reference compiler (same). */
// FUNC 0x00142790 gt2k_menu::sjis_to_euc
void sjis_to_euc(char *str)
{
    s8 *s = (s8 *)str;

    for (;;) {
        s8 lead = s[0];
        if (lead == 0)
            return;
        if ((u32)((lead ^ -0x60) - 0x21) < 0x3C) {    /* a Shift-JIS lead byte */
            s8 trail = s[1];
            if (trail == 0)
                return;
            int code = (lead << 8) | (trail & 0xFF);
            int lo = code & 0xFF;
            int row = (code & 0x3F00) << 1;
            int euc;
            if (lo >= 0x9F)
                euc = row + 0x1F82;             /* even JIS row */
            else
                euc = row + 0x1EE0 + (lo < 0x7F);   /* odd JIS row (the trail byte 0x7F is skipped) */
            euc = (euc + lo) | 0x8080;
            s[1] = (s8)euc;
            s[0] = (s8)(euc >> 8);
            s += 2;
        } else {
            s++;
        }
    }
}

/* EUC-JP -> Shift-JIS in place (the inverse of sjis_to_euc): bytes without
 * bit 7 are left alone.  Not exact (79.2%, the shape is retail's):
 * registers only (retail: trail and lo in a1, lead and the result in v1);
 * the same variable combinations as sjis_to_euc tried. */
// FUNC 0x00142818 gt2k_menu::euc_to_sjis
void euc_to_sjis(char *str)
{
    s8 *s = (s8 *)str;

    for (;;) {
        s8 lead = s[0];
        if (lead == 0)
            return;
        if ((lead & 0x80) != 0) {
            s8 trail = s[1];
            if (trail == 0)
                return;
            int code = (lead << 8) | (trail & 0xFF);
            int lo = code & 0x7F;
            int sjis = (((code & 0x7F00) + 0x2100) >> 1) ^ 0xA000;
            if ((code & 0x100) == 0)
                sjis -= 2;                      /* even JIS row */
            else
                sjis = sjis + 0x20 - (lo < 0x60);   /* odd JIS row */
            sjis += lo;
            s[1] = (s8)sjis;
            s[0] = (s8)(sjis >> 8);
            s += 2;
        } else {
            s++;
        }
    }
}

/* Each character of src -> its full-width EUC-JP pair from the table
 * g_003029E8 (indexed with the signed char: bytes >= 0x80 read before the
 * table). */
// FUNC 0x00142890 gt2k_menu::ascii_to_fullwidth
void ascii_to_fullwidth(char *dst, const char *src)
{
    const s8 *p = (const s8 *)src;
    s8 c;

    while ((c = *p++) != 0) {
        int wide = g_003029E8[c];
        dst[1] = (char)wide;
        dst[0] = (char)(wide >> 8);
        dst += 2;
    }
    *dst = 0;
}

// FUNC 0x001428E0 gt2k_menu::bcd_to_int
int bcd_to_int(int bcd)
{
    return bcd % 16 + bcd / 16 * 10;
}

/* ======================================================================== */
/* MemorycardDir                                                            */
/* ======================================================================== */

/* Both dates of the entry := now (the clock is BCD, years from 2000). */
// FUNC 0x00142910 gt2k_menu::MemorycardDir::setTimeNow
int MemorycardDir::setTimeNow()
{
    CdClock now;                            /* sp+0x00 */

    clock_get(&now);
    entry.create.resv = 0;
    entry.create.sec = bcd_to_int(now.second);
    entry.create.min = bcd_to_int(now.minute);
    entry.create.hour = bcd_to_int(now.hour);
    entry.create.day = bcd_to_int(now.day);
    entry.create.month = bcd_to_int(now.month);
    entry.create.year = bcd_to_int(now.year) + 2000;
    entry.modify = entry.create;
    return 1;
}

/* Keeps a valid id as -0x400 - id (MemorycardFileList::markAll does this
 * to every file before a new listing gives the ids again). */
// FUNC 0x00142998 gt2k_menu::MemorycardDir::setOldId
void MemorycardDir::setOldId()
{
    if (id >= 0)
        id = -0x400 - id;
}

// FUNC 0x001429B8 gt2k_menu::MemorycardDir::getOldId
int MemorycardDir::getOldId()
{
    return -0x400 - id;
}

/* ======================================================================== */
/* MemorycardFile                                                           */
/* ======================================================================== */

/* A file that is not on the card yet (isNewFile): its info counts as read,
 * it is selectable.  (pCell's inline constructor zeroes the links; Cell's
 * vptr store is dead.) */
// FUNC 0x001429C8 gt2k_menu::MemorycardFile::MemorycardFile
MemorycardFile::MemorycardFile()
{
    dir.id = -1;
    dir.entry.name[0] = 0;
    sizeKB = -1;
    infoRead = 1;
    selectable = 1;
    readCount = 0;
}

/* A directory found on the card: no info read yet, not selectable until
 * checkSelectable says so. */
// FUNC 0x00142A08 gt2k_menu::MemorycardFile::MemorycardFile
MemorycardFile::MemorycardFile(const MemorycardDir &d)
{
    dir = d;
    char *name = dir.entry.name;
    sizeKB = -1;
    infoRead = 0;
    selectable = 0;
    readCount = 0;
    name[0x20] = 0;                         /* names have at most 32 characters */
}

/* The copy is not linked into any list: the links are zeroed by pCell's
 * inline constructor and again after the members (as in the binary).
 * Not exact (80.6%): retail loads f.sizeKB and f.infoRead before storing
 * either, then f.readCount and f.selectable, and zeroes prev before next:
 * as if the pairs were copied through temporaries (95.8% so) or the fields
 * had different alias sets.  Tried: field and statement orders, mem-
 * initializers, next = prev = 0, the reference compiler (same). */
// FUNC 0x00142A88 gt2k_menu::MemorycardFile::MemorycardFile
MemorycardFile::MemorycardFile(const MemorycardFile &f)
    : Cell<MemorycardFile>()
{
    dir = f.dir;
    sizeKB = f.sizeKB;
    infoRead = f.infoRead;
    readCount = f.readCount;
    selectable = f.selectable;
    clearLinks();
}

/* Empty: its own vptr store is dead (egcs drops it).  The subclasses'
 * destructors call this copy. */
// FUNC 0x00142B18 gt2k_menu::MemorycardFile::~MemorycardFile
MemorycardFile::~MemorycardFile()
{
    /* then the inline ~Cell<MemorycardFile> (its vptr) and
     * pCell::~pCell(this, flags), which unlinks the cell and does the
     * operator delete */
    /* implicit: ~pCell (base destructor) */
}

/* "/NAME/NAME": the main file of a save has the directory's name. */
// FUNC 0x00142B40 gt2k_menu::MemorycardFile::getMainFilePath
char *MemorycardFile::getMainFilePath(char *buf)
{
    char *name = dir.entry.name;

    buf[0] = '/';
    strcpy(buf + 1, name);
    strcat(buf + 1, "/");
    strcat(buf + 1, name);
    return buf;
}

/* The layer of the info panel shown for the file. */
/* (index.txt: MemorycardDamageFile::virtual_18; the slot is inherited from here) */
// FUNC 0x00142BB0 gt2k_menu::MemorycardFile::getInfoName
const char *MemorycardFile::getInfoName()
{
    return "info_other";
}

/* A file of no GT3 kind passes only the screen's "any file" filter (0);
 * course, car and the categories are not looked at. */
/* (index.txt: MemorycardDeletedFile::virtual_6; the slot is inherited from here) */
// FUNC 0x00142BC0 gt2k_menu::MemorycardFile::checkSelectable
void MemorycardFile::checkSelectable(int filter, u64 course, u64 car, int category,
                                     int subCategory)
{
    if (filter == 0)
        selectable = 1;
    else
        selectable = 0;
}

/* The files of the directory, without "." and "..", into list (at most 20
 * entries are asked for; the callers have room for 18).  Their count, -1 if
 * the directory cannot be listed. */
// FUNC 0x00142BD8 gt2k_menu::MemorycardFile::getFileList
int MemorycardFile::getFileList(int port, McFileEntry *list)
{
    char pattern[0x50];                     /* sp+0x000 */
    McDirEntry entries[MC_DIR_LIST_MAX];    /* sp+0x050 */

    pattern[0] = '/';
    strcpy(pattern + 1, dir.entry.name);
    strcat(pattern + 1, "/*");
    if (mc_get_dir(port, pattern, entries, 0, MC_DIR_LIST_MAX) != MC_STATUS_OK)
        return -1;
    int listed = mc_slot_dir_count(port);
    int count = 0;
    for (int i = 0; i < listed; i++) {
        McDirEntry *e = &entries[i];
        if (strcmp(".", e->name) != 0 && strcmp("..", e->name) != 0)
            list[count++] = *e;
    }
    return count;
}

// FUNC 0x00142D30 gt2k_menu::MemorycardFile::updateSize
int MemorycardFile::updateSize(int port)
{
    return calcSize(port, &sizeKB);
}

/* KB the save takes: its files rounded up to 1 KB each, plus (files + 1) / 2
 * + 1 for the directory.  0 (kb untouched) if the directory cannot be
 * listed. */
// FUNC 0x00142D50 gt2k_menu::MemorycardFile::calcSize
int MemorycardFile::calcSize(int port, int *kb)
{
    McDirEntry files[MC_SAVE_FILES_MAX];    /* sp+0x000 */

    int count = getFileList(port, files);
    if (count < 0)
        return 0;
    int total = 0;
    for (int i = 0; i < count; i++)
        total += (files[i].size + 0x3FF) >> 10;
    total += (count + 1) / 2;               /* the entries of the directory, 2 per KB */
    total += 1;
    *kb = total;
    return 1;
}

/* KB of the main file "/NAME/NAME", -1 if it is not there. */
// FUNC 0x00142DE8 gt2k_menu::MemorycardFile::getMainFileKB
int MemorycardFile::getMainFileKB(int port)
{
    char path[0x50];                        /* sp+0x00 */
    McDirEntry entry;                       /* sp+0x50 */

    getMainFilePath(path);
    if (mc_get_dir(port, path, &entry, 0, 1) != MC_STATUS_OK)
        return -1;
    if (mc_slot_dir_count(port) != 1)
        return -1;
    return (entry.size + 0x3FF) >> 10;
}

/* Deletes every file of the directory, then the directory.  Progress 0.1,
 * then the files share 0.1 .. 0.9 (the share is divided before the count
 * is checked), 1.0 before and after the directory.  0 on a card error. */
// FUNC 0x00142E50 gt2k_menu::MemorycardFile::deleteDir
int MemorycardFile::deleteDir(int port, MemorycardProgressFunc progress, void *ctx)
{
    char path[0x50];                        /* sp+0x000 */
    McDirEntry files[MC_SAVE_FILES_MAX];    /* sp+0x050 */
    f32 rate = 0.1f;

    if (progress != NULL)
        progress(rate, ctx);
    int count = getFileList(port, files);
    if (count < 0)
        return 0;
    /* 0.9f - 0.1f folds to 0x3F4CCCCC, the constant of the binary */
    f32 step = (0.9f - 0.1f) / (f32)count;
    for (int i = 0; i < count; i++) {
        rate += step;
        if (progress != NULL)
            progress(rate, ctx);
        path[0] = '/';
        strcpy(path + 1, dir.entry.name);
        strcat(path + 1, "/");
        strcat(path + 1, files[i].name);
        if (mc_delete(port, path) != MC_STATUS_OK)
            return 0;
    }
    rate = 1.0f;
    if (progress != NULL)
        progress(rate, ctx);
    path[0] = '/';
    strcpy(path + 1, dir.entry.name);
    if (mc_delete(port, path) != MC_STATUS_OK)
        return 0;
    if (progress != NULL)
        progress(rate, ctx);
    return 1;
}

/* Copies the save from card src to card dst.  format: dst is formatted
 * first, and only if it is not (a formatted card is an error); replace: the
 * save already on dst is deleted first.  Then the directory, each file in
 * 64 KB pieces (the first piece creates the file, the others write at their
 * offset) and the directory's attributes.  Progress: 0.25 (format), +0.2
 * (replace), +0.1, +0.1, then what is left up to 0.95 shared by the files
 * in proportion to their bytes (an empty file gets its share at once), 1.0
 * before and after the attributes.  The 64 KB buffer is alloca'd inside the
 * file loop: the stack grows by 64 KB per file until the function returns.
 * 0 on a card error. */
// FUNC 0x00143008 gt2k_menu::MemorycardFile::copyTo
int MemorycardFile::copyTo(int src, int dst, int replace, int format,
                           MemorycardProgressFunc progress, void *ctx)
{
    char path[0x50];                        /* fp+0x000 */
    McDirEntry files[MC_SAVE_FILES_MAX];    /* fp+0x050 */
    f32 rate = 0.0f;

    if (format) {
        rate = 0.25f;
        if (progress != NULL)
            progress(rate, ctx);
        if (mc_slot_formatted(dst) != 0)
            return 0;
        if (mc_format(dst) != MC_STATUS_OK)
            return 0;
    }
    if (replace) {
        rate += 0.2f;
        if (progress != NULL)
            progress(rate, ctx);
        if (deleteDir(dst, NULL, NULL) == 0)
            return 0;
    }
    rate += 0.1f;
    if (progress != NULL)
        progress(rate, ctx);
    int count = getFileList(src, files);
    if (count < 0)
        return 0;
    rate += 0.1f;
    if (progress != NULL)
        progress(rate, ctx);
    path[0] = '/';
    strcpy(path + 1, dir.entry.name);
    if (mc_mkdir(dst, path) != MC_STATUS_OK)
        return 0;
    f32 fileShare = ((1.0f - rate) - 0.05f) / (f32)count;
    for (int i = 0; i < count; i++) {
        path[0] = '/';
        strcpy(path + 1, dir.entry.name);
        int offset = 0;
        f32 byteShare = fileShare;
        strcat(path + 1, "/");
        McDirEntry *file = &files[i];
        strcat(path + 1, file->name);
        int remain = (int)file->size;
        u8 *buf = (u8 *)__builtin_alloca(MC_COPY_CHUNK);
        if (remain != 0)
            byteShare /= (f32)remain;
        do {
            int chunk = (remain > MC_COPY_CHUNK) ? MC_COPY_CHUNK : remain;
            if (remain != 0)
                rate += byteShare * (f32)chunk;
            else
                rate += byteShare;
            if (progress != NULL)
                progress(rate, ctx);
            if (mc_read_file(src, path, buf, offset, chunk) != MC_STATUS_OK)
                return 0;
            if (offset != 0) {
                if (mc_write_file(dst, path, buf, offset, chunk) != MC_STATUS_OK)
                    return 0;
            } else {
                if (mc_create_file(dst, path, buf, chunk) != MC_STATUS_OK)
                    return 0;
            }
            offset += MC_COPY_CHUNK;
            remain -= MC_COPY_CHUNK;
        } while (remain > 0);
    }
    rate = 1.0f;
    if (progress != NULL)
        progress(rate, ctx);
    path[0] = '/';
    strcpy(path + 1, dir.entry.name);
    if (mc_set_file_info(dst, path, &dir.entry, sceMcFileInfoAttr) != MC_STATUS_OK)
        return 0;
    if (progress != NULL)
        progress(rate, ctx);
    return 1;
}

/* Lists "/NAME": 1 if the directory is on the card, 0 if not, -1 on error. */
// FUNC 0x00143360 gt2k_menu::MemorycardFile::exists
int MemorycardFile::exists(int port)
{
    char path[0x30];                        /* sp+0x00 */
    McDirEntry entry;                       /* sp+0x30 */

    path[0] = '/';
    strcpy(path + 1, dir.entry.name);
    if (mc_get_dir(port, path, &entry, 0, 1) != MC_STATUS_OK)
        return -1;
    return mc_slot_dir_count(port);
}

/* ======================================================================== */
/* MemorycardPS1File                                                        */
/* ======================================================================== */

// FUNC 0x001433C0 gt2k_menu::MemorycardPS1File::MemorycardPS1File
MemorycardPS1File::MemorycardPS1File()
    : MemorycardFile()
{
}

// FUNC 0x001433F8 gt2k_menu::MemorycardPS1File::MemorycardPS1File
MemorycardPS1File::MemorycardPS1File(const MemorycardDir &d)
    : MemorycardFile(d)
{
    header.title[0] = 0;
}

/* Reads the save's header block (the first 0x200 bytes of "/NAME/NAME"),
 * the title to EUC-JP.  1 when read. */
// FUNC 0x00143430 gt2k_menu::MemorycardPS1File::readInfo
int MemorycardPS1File::readInfo(int port)
{
    char path[0x50];                        /* sp+0x00 */

    if (mc_read_file(port, getMainFilePath(path), &header, 0, sizeof(header)) != MC_STATUS_OK)
        return 0;
    if (mc_slot_read_size(port) < (int)sizeof(header))
        return 0;
    header.titleEnd = 0;                    /* terminates the 64-byte title */
    sjis_to_euc(getTitle());
    infoRead = 1;
    return 1;
}

/* Without info: built-in icon 6.  Else the header's 16 x 16 4-bit icon,
 * frame (int)(time / 0.17) % frames (a division trap with 0 frames), as a
 * textured quad: a 'Tex1' set built in the packet buffer (two uploads: the
 * frame at block 0, its 16-colour CLUT at block 1) over a stack copy of the
 * GS registers texRegs, made resident by the VRAM allocator; drawn
 * without depth or alpha test, at (x, y), w x h, in (r, g, b, a) x 2. */
// FUNC 0x001434C8 gt2k_menu::MemorycardPS1File::render
void MemorycardPS1File::render(MemorycardIconRenderInfo &info)
{
    /* The icon's quad, a triangle strip of 4 vertices. */
    static const f32 uv[4][2] = {           /* 0x0031B590 */
        {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}
    };
    static const f32 xyz[4][3] = {          /* 0x0031B5B0 */
        {-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}
    };
    /* TEX0: 16 x 16 PSMT4 at block 0, TBW 1; CLUT PSMCT16 at block 1, CLD
     * 1.  TEX1 0x60 (bilinear), CLAMP 5 (clamp u and v). */
    static const GsTexRegs texRegs = {      /* 0x0031B5E0 */
        0x2010002111404000ULL, 0x60, 0, 0, 5
    };

    if (infoValid() == 0) {
        /* implicit: draw (inline drawBuiltinIcon) */
        drawBuiltinIcon(info, MC_ICON_PS1); /* MemorycardIconSprites::draw */
        return;
    }
    int frame = (int)(info.time / MC_PS1_FRAME_TIME) % (header.iconFlag & 0xF);
    GsTexRegs regs = texRegs;               /* sp+0x00 */

    /* (self, ptr08 and size of the set are not set) */
    Tex1Xfer *xfer = (Tex1Xfer *)gfx_alloc_tail(2);
    xfer[0].data = header.icon[frame];      /* the frame: 16 x 16 PSMT4 at block 0 */
    xfer[0].dbp = 0;
    xfer[0].dbw = 1;
    xfer[0].dpsm = SCE_GS_PSMT4;
    xfer[0].w = 16;
    xfer[0].h = 16;
    xfer[1].data = header.clut;             /* the CLUT: 16 colours, 8 x 2 PSMCT16 at block 1 */
    xfer[1].dbp = 1;
    xfer[1].dbw = 1;
    xfer[1].dpsm = SCE_GS_PSMCT16;
    xfer[1].w = 8;
    xfer[1].h = 2;

    Tex1Image *img = (Tex1Image *)gfx_alloc_tail(3);
    img->magic = TEX1_MAGIC;
    img->vramBase = 0;
    img->vramSize = 2;                      /* 64-word blocks */
    img->nTex = 1;
    img->nXfer = 2;
    img->tex = &regs;
    img->xfer = xfer;
    img->ptrTable = NULL;
    img->clutAnim = NULL;
    g_002F4250.loadTex1(img);
    gfx_set_texture_regs(img->tex);

    pgl_push_matrix();
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    pgl_translate(info.x, info.y, 0.0f);
    vu0_scale(info.w * 0.5f, info.h * 0.5f, 1.0f);
    /* the current colour (pgl_color: glColor-like) */
    pgl_color4f(info.r + info.r, info.g + info.g, info.b + info.b, info.a + info.a);
    pgl_draw_strip(xyz, NULL, NULL, uv, 4);
    pgl_pop_matrix();
}

/* ======================================================================== */
/* MemorycardPS2File                                                        */
/* ======================================================================== */

// FUNC 0x00143738 gt2k_menu::MemorycardPS2File::MemorycardPS2File
MemorycardPS2File::MemorycardPS2File()
    : MemorycardFile()
{
}

// FUNC 0x00143770 gt2k_menu::MemorycardPS2File::MemorycardPS2File
MemorycardPS2File::MemorycardPS2File(const MemorycardDir &d)
    : MemorycardFile(d)
{
    iconSys.titleName[0] = 0;
}

/* (The 0x1C4-byte copy of iconSys is inline: ldl / ldr when either address
 * is not 8-aligned, ld / sd otherwise; unk_22C is not copied.) */
// FUNC 0x001437A8 gt2k_menu::MemorycardPS2File::MemorycardPS2File
MemorycardPS2File::MemorycardPS2File(const MemorycardPS2File &f)
    : MemorycardFile(f)
{
    iconSys = f.iconSys;
}

/* Bytes of the title's first line: icon.sys's offsLF if it is 2..0x20,
 * else 64 (the whole title). */
/* (index.txt: MemorycardOtherPS2File::virtual_8; the slot is inherited from here) */
// FUNC 0x001438A0 gt2k_menu::MemorycardPS2File::getTitleFirstLineBytes
int MemorycardPS2File::getTitleFirstLineBytes()
{
    int bytes = iconSys.offsLF;

    if (bytes < 2 || bytes > 0x20)
        bytes = 0x40;
    return bytes;
}

/* width: of a run of blanks, 1 per ASCII space and 2 per full-width one;
 * writes (width + 3) / 4 ASCII spaces and returns the end. */
// FUNC 0x001438B8 gt2k_menu::put_spaces
char *put_spaces(char *p, int width)
{
    width = (width + 3) / 4;                /* the spaces */
    for (int i = 0; i < width; i++)
        *p++ = ' ';
    return p;
}

/* Squeezes the blanks of the (EUC-JP) title in place: each run of ASCII /
 * full-width (0xA1A0, 0xA1A1) spaces becomes put_spaces(width), and offsLF
 * moves to where the first getTitleFirstLineBytes() source bytes end up
 * (-1 if the title is shorter). */
// FUNC 0x00143908 gt2k_menu::MemorycardPS2File::packTitle
void MemorycardPS2File::packTitle()
{
    int lineBreak = -1;
    u8 *title = (u8 *)getTitle();
    u8 *dst = title;
    u8 *src = title + 1;                    /* one ahead of c */
    int firstLine = getTitleFirstLineBytes();
    u32 c = title[0];
    int width = 0;                          /* of the blanks not written yet */

    if (c != 0) {
        do {
            if ((s8)c >= 0) {             /* ASCII */
                if (c == ' ') {
                    width += 1;
                    goto next;
                }
                if (width != 0) {
                    dst = (u8 *)put_spaces((char *)dst, width);
                    width = 0;
                }
                *dst++ = c;
            } else {
                u32 trail = *src++;
                if (c == 0xA1 && (u8)(trail + 0x60) < 2) {
                    width += 2;             /* a full-width space */
                    goto next;
                }
                if (width != 0) {
                    dst = (u8 *)put_spaces((char *)dst, width);
                    width = 0;
                }
                *dst++ = c;
                if (trail == 0)
                    break;
                *dst++ = trail;
            }
        next:
            c = *src;
            if (lineBreak < 0 && src - title >= firstLine)
                lineBreak = dst - title;
            src++;
        } while (c != 0);
    }
    *dst = 0;
    iconSys.offsLF = lineBreak;
}

/* Reads "/NAME/icon.sys" (0x1C4 bytes), terminates the title (64) and the
 * icon file names (32), converts the title to EUC-JP and packs it.  1 when
 * read. */
/* (index.txt: MemorycardOtherPS2File::virtual_9; the slot is inherited from here) */
// FUNC 0x00143A70 gt2k_menu::MemorycardPS2File::readInfo
int MemorycardPS2File::readInfo(int port)
{
    char path[0x50];                        /* sp+0x00 */

    path[0] = '/';
    strcpy(path + 1, dir.entry.name);
    strcat(path + 1, "/");
    strcat(path + 1, "icon.sys");
    if (mc_read_file(port, path, &iconSys, 0, sizeof(iconSys)) != MC_STATUS_OK)
        return 0;
    if (mc_slot_read_size(port) < (int)sizeof(iconSys))
        return 0;
    iconSys.titleName[0x40] = 0;
    iconSys.iconFile[0][0x20] = 0;
    iconSys.iconFile[1][0x20] = 0;
    iconSys.iconFile[2][0x20] = 0;
    sjis_to_euc(getTitle());
    packTitle();
    infoRead = 1;
    return 1;
}

/* Writes "/NAME/icon.sys": iconSys, zero filled to 0x3C4 bytes, the title
 * back in Shift-JIS.  1 on success. */
// FUNC 0x00143B50 gt2k_menu::MemorycardPS2File::writeIconSys
int MemorycardPS2File::writeIconSys(int port)
{
    char path[0x50];                        /* sp+0x000 */
    u8 file[MC_ICONSYS_FILE_SIZE];          /* sp+0x050 */

    path[0] = '/';
    strcpy(path + 1, dir.entry.name);
    strcat(path + 1, "/");
    strcat(path + 1, "icon.sys");
    /* a variable size: memcpy is called, not expanded (retail calls it) */
    u32 size = sizeof(iconSys);
    u8 *p = file;
    memcpy(p, &iconSys, size);
    for (u32 i = size; i < MC_ICONSYS_FILE_SIZE; i++)
        p[i] = 0;
    euc_to_sjis(((MemorycardIconSys *)p)->titleName);
    return mc_create_file(port, path, p, MC_ICONSYS_FILE_SIZE) == MC_STATUS_OK;
}

/* files: up to 3 volume paths (NULL ends the list early).  Copies each to
 * "/NAME/icon<i>.ico" and names it in iconSys; the remaining icon names
 * repeat the last file ("icon-1.ico" when there is none).  The read buffer
 * (the size file_size_retry gives with packed 1, + 0x80, 64-aligned) is alloca'd
 * inside the loop.  1 on success. */
// FUNC 0x00143C18 gt2k_menu::MemorycardPS2File::writeIcons
int MemorycardPS2File::writeIcons(int port, const char **files)
{
    char path[0x50];                        /* fp+0x00 */
    int count;
    int i;

    for (count = 0; count < 3 && files[count] != NULL; count++)
        ;
    for (i = 0; i < count; i++) {
        char *name = iconSys.iconFile[i];
        /* (i < count always holds here; the machine code keeps the select) */
        sprintf(name, "icon%d.ico", (i < count) ? i : count - 1);
        path[0] = '/';
        strcpy(path + 1, dir.entry.name);
        strcat(path + 1, "/");
        strcat(path + 1, name);
        u32 size = file_size_retry(files[i], 1, &vol_device_object);
        u8 *buf = (u8 *)(((int)__builtin_alloca(size + 0x80) + 0x3F) & ~0x3F);
        file_read_blocking(files[i], buf, 0, 1, &vol_device_object);
        if (mc_create_file(port, path, buf, size) != MC_STATUS_OK)
            return 0;
    }
    for (; i < 3; i++)
        sprintf(iconSys.iconFile[i], "icon%d.ico", count - 1);
    return 1;
}

/* Makes "/NAME".  1 on success. */
// FUNC 0x00143DE8 gt2k_menu::MemorycardPS2File::makeDir
int MemorycardPS2File::makeDir(int port)
{
    char path[0x30];                        /* sp+0x00 */

    path[0] = '/';
    strcpy(path + 1, dir.entry.name);
    return mc_mkdir(port, path) == MC_STATUS_OK;
}

/* The title from text; a '\n' is not copied, its index becomes offsLF. */
// FUNC 0x00143E30 gt2k_menu::MemorycardPS2File::setTitle
void MemorycardPS2File::setTitle(const char *text)
{
    char *dst = getTitle();
    const char *p = text;
    char c;

    while ((c = *p++) != 0) {
        int i = p - text - 1;               /* the index of c */
        if (c == '\n')
            iconSys.offsLF = i;
        else
            *dst++ = c;
    }
    *dst = 0;
}

/* ======================================================================== */
/* MemorycardOtherPS2File                                                   */
/* ======================================================================== */

// FUNC 0x00143EC0 gt2k_menu::MemorycardOtherPS2File::MemorycardOtherPS2File
MemorycardOtherPS2File::MemorycardOtherPS2File()
    : MemorycardPS2File()
{
    icon = NULL;
    iconPrepared = 0;
}

// FUNC 0x00143F00 gt2k_menu::MemorycardOtherPS2File::MemorycardOtherPS2File
MemorycardOtherPS2File::MemorycardOtherPS2File(const MemorycardDir &d)
    : MemorycardPS2File(d)
{
    icon = NULL;
    iconPrepared = 0;
}

/* The copy shares the icon model: one more reference. */
// FUNC 0x00143F40 gt2k_menu::MemorycardOtherPS2File::MemorycardOtherPS2File
MemorycardOtherPS2File::MemorycardOtherPS2File(const MemorycardOtherPS2File &f)
    : MemorycardPS2File(f)
{
    iconPrepared = f.iconPrepared;
    icon = f.icon;
    if (icon != NULL)
        icon->refCount++;
}

/* releaseIcon(false) is called directly (the vptr is this class's here),
 * then ~MemorycardFile(this, flags) (the implicit copy 0x00142B18;
 * ~MemorycardPS2File is implicit and empty). */
// FUNC 0x00143FA0 gt2k_menu::MemorycardOtherPS2File::~MemorycardOtherPS2File
MemorycardOtherPS2File::~MemorycardOtherPS2File()
{
    if (icon != NULL)
        MemorycardOtherPS2File::releaseIcon(false);
    /* implicit: ~MemorycardFile (base destructor) */
}

/* Loads icon file `which' of icon.sys (0 the list icon, kept in `icon';
 * 1 / 2 the copy / delete icon: into T077's fixed slot 0, which render
 * draws with useSlot0) with icon.sys's lights and background.  Without
 * info, or when the file is empty, larger than 0x195D4C bytes, does not fit
 * in the heap or is no icon, the icon counts as prepared (there is nothing
 * to show).  When the icon heap has no room (icon_model_create error 2) it
 * stays unprepared: the screen asks again later.  1 when done, 0 on a card
 * error. */
// FUNC 0x00143FF8 gt2k_menu::MemorycardOtherPS2File::readIcon
int MemorycardOtherPS2File::readIcon(int port, int which)
{
    char path[0x50];                        /* sp+0x00 */
    McDirEntry entry;                       /* sp+0x50 */
    McIconLight light;                      /* sp+0x90 (0x54; its slot is 0x60) */
    int error;                              /* sp+0xF0 */
    McIconModel *model;

    if (infoValid() == 0) {
        iconPrepared = 1;
        return 1;
    }
    if (icon != NULL) {
        if (which == 0)
            return 1;
        if (strcmp(iconSys.iconFile[which], iconSys.iconFile[0]) == 0)
            return 1;                       /* the list icon's file: already loaded */
    }
    path[0] = '/';
    strcpy(path + 1, dir.entry.name);
    strcat(path + 1, "/");
    strcat(path + 1, iconSys.iconFile[which]);
    if (mc_get_dir(port, path, &entry, 0, 1) != MC_STATUS_OK)
        return 0;
    if (mc_slot_dir_count(port) != 1)
        return 0;
    int size = (int)entry.size;
    if (size > MC_ICON_FILE_MAX) {
        iconPrepared = 1;
        return 1;
    }
    if (size <= 0) {
        iconPrepared = 1;
        return 1;
    }
    void *ico = malloc(size);
    if (ico == NULL) {
        iconPrepared = 1;
        return 1;
    }
    if (mc_read_file(port, path, ico, 0, size) != MC_STATUS_OK) {
        free(ico);
        return 0;
    }
    icon_light_init(&light, iconSys.lightDir, iconSys.lightColor, iconSys.ambient);
    if (which == 0) {
        error = 0;
        icon = icon_model_create(port, (const u32 *)ico, &light, &error);
        free(ico);
        if (icon == NULL) {
            if (error == 1)
                iconPrepared = 1;           /* not an icon */
            return 1;                       /* else no room in the icon heap: not prepared */
        }
        iconPrepared = 1;
        model = icon;
    } else {
        model = icon_slot_create(0, (const u32 *)ico, &light);
        free(ico);
        if (model == NULL)
            return 0;
    }
    if (model != NULL)                      /* (always: retail keeps the test) */
        icon_model_set_bg(model, iconSys.bgColor[0], iconSys.bgColor[1], iconSys.bgColor[2],
                          iconSys.bgColor[3], iconSys.transRate);
    return 1;
}

/* The model stops being drawn before it is released. */
// FUNC 0x00144228 gt2k_menu::MemorycardOtherPS2File::prepareRelease
void MemorycardOtherPS2File::prepareRelease()
{
    if (icon != NULL)
        icon->ready = 0;
}

/* noWait is handed to icon_model_release as is (false: two frames are
 * waited for before the model is freed). */
// FUNC 0x00144240 gt2k_menu::MemorycardOtherPS2File::releaseIcon
void MemorycardOtherPS2File::releaseIcon(bool noWait)
{
    if (icon != NULL)
        icon_model_release(icon, noWait);
    icon = NULL;
    iconPrepared = 0;
}

/* The list icon (with useSlot0: T077's slot-0 model when there is one) once
 * it is ready; built-in icon 5 when there is no model at all. */
// FUNC 0x00144280 gt2k_menu::MemorycardOtherPS2File::render
void MemorycardOtherPS2File::render(MemorycardIconRenderInfo &info)
{
    McIconModel *model = icon;

    if (info.useSlot0 > 0) {
        model = icon_slot_get(0);
        if (model == NULL)
            model = icon;
    }
    if (model != NULL) {
        if (model->ready != 0)
            icon_model_render(model, info.x, info.y, info.w, info.h, info.r, info.g, info.b,
                              info.a, info.time);
    } else {
        /* implicit: draw (inline drawBuiltinIcon) */
        drawBuiltinIcon(info, MC_ICON_OTHER);   /* MemorycardIconSprites::draw */
    }
}

/* ======================================================================== */
/* MemorycardDamageFile                                                     */
/* ======================================================================== */

// FUNC 0x00144368 gt2k_menu::MemorycardDamageFile::MemorycardDamageFile
MemorycardDamageFile::MemorycardDamageFile()
    : MemorycardFile()
{
}

/* Marked copy-prohibited, so that it is never offered for copying. */
// FUNC 0x001443A0 gt2k_menu::MemorycardDamageFile::MemorycardDamageFile
MemorycardDamageFile::MemorycardDamageFile(const MemorycardDir &d)
    : MemorycardFile(d)
{
    getDir()->entry.attr |= sceMcFileAttrDupProhibit;
}

// FUNC 0x001443E8 gt2k_menu::MemorycardDamageFile::getTitle
char *MemorycardDamageFile::getTitle()
{
    return (char *)g_002B1C68[MCTEXT_CORRUPT_FILE];
}

// FUNC 0x001443F8 gt2k_menu::MemorycardDamageFile::render
void MemorycardDamageFile::render(MemorycardIconRenderInfo &info)
{
    /* implicit: draw (inline drawBuiltinIcon) */
    drawBuiltinIcon(info, MC_ICON_DAMAGE);  /* MemorycardIconSprites::draw */
}

/* ======================================================================== */
/* MemorycardDummyFile, MemorycardDeletedFile                               */
/* ======================================================================== */

/* id -2 (isNotReading()).  The rest is dead code in the binary: a copy of
 * the entry on the stack (sp+0x00) with its dates, size and attributes
 * cleared, never used. */
// FUNC 0x00144460 gt2k_menu::MemorycardDummyFile::MemorycardDummyFile
MemorycardDummyFile::MemorycardDummyFile()
    : MemorycardFile()
{
    dir.id = -2;
    McDirEntry *e = &dir.entry;
    McDirEntry unused = *e;                 /* sp+0x00 */
    McDateTime zero = {0, 0, 0, 0, 0, 0, 0};
    unused.create = zero;
    unused.attr = 0;
    unused.size = 0;
    unused.modify = zero;
}

/* (index.txt: MemorycardDeletedFile::virtual_7; the slot is inherited from here) */
// FUNC 0x001444E0 gt2k_menu::MemorycardDummyFile::getTitle
char *MemorycardDummyFile::getTitle()
{
    return (char *)"";
}

// FUNC 0x001444F0 gt2k_menu::MemorycardDummyFile::render
void MemorycardDummyFile::render(MemorycardIconRenderInfo &info)
{
    /* implicit: draw (inline drawBuiltinIcon) */
    drawBuiltinIcon(info, MC_ICON_READING); /* MemorycardIconSprites::draw */
}

/* No info panel. */
/* (index.txt: MemorycardDeletedFile::virtual_18; the slot is inherited from here) */
// FUNC 0x00144558 gt2k_menu::MemorycardDummyFile::getInfoName
const char *MemorycardDummyFile::getInfoName()
{
    return NULL;
}

// FUNC 0x00144560 gt2k_menu::MemorycardDeletedFile::render
void MemorycardDeletedFile::render(MemorycardIconRenderInfo &info)
{
    /* implicit: draw (inline drawBuiltinIcon) */
    drawBuiltinIcon(info, MC_ICON_BLANK);   /* MemorycardIconSprites::draw */
}

/* ======================================================================== */
/* MemorycardGT3File                                                        */
/* ======================================================================== */

/* A new GT3 save: GT3's icon.sys; its thumbnail counts as valid. */
// FUNC 0x001445C8 gt2k_menu::MemorycardGT3File::MemorycardGT3File
MemorycardGT3File::MemorycardGT3File()
    : MemorycardPS2File()
{
    iconSys = g_0049BEF0;
    thumbnailValid = 1;
}

/* A GT3 save found on the card (MemorycardPS2File(d) clears the title, then
 * the whole icon.sys is overwritten); the thumbnail is read by readInfo. */
// FUNC 0x001446C0 gt2k_menu::MemorycardGT3File::MemorycardGT3File
MemorycardGT3File::MemorycardGT3File(const MemorycardDir &d)
    : MemorycardPS2File(d)
{
    iconSys = g_0049BEF0;
    thumbnailValid = 0;
}

/* Its vptr, then ~MemorycardFile(this, flags) (the implicit copy
 * 0x00142B18; ~MemorycardPS2File is implicit and empty). */
// FUNC 0x001447B0 gt2k_menu::MemorycardGT3File::~MemorycardGT3File
MemorycardGT3File::~MemorycardGT3File()
{
    /* implicit: ~MemorycardFile (base destructor) */
}

/* ======================================================================== */
/* Save names                                                               */
/* ======================================================================== */

/* (? name) mcInitIconSys's stack buffer: 0x400 bytes 64-byte aligned by
 * get() (int arithmetic: retail's mask is the signed -64), cleared byte by
 * byte by its constructor (a pointer and a count: retail steps both).  Its
 * destructor is declared and empty: retail has an exception region over the
 * function whose landing pad only rethrows (as T102's DemoBuffer). */
struct IconSysBuffer {
    IconSysBuffer()
    {
        u8 *p = buf;
        for (u32 i = 0; i < sizeof(buf); i++)
            *p++ = 0;
    }
    ~IconSysBuffer() {}
    MemorycardIconSys *get() { return (MemorycardIconSys *)(((int)buf + 0x3F) & ~0x3F); }
    u8 buf[0x440];
};

/* Loads menu/icon/icon.sys into a 64-byte aligned buffer (cleared byte by
 * byte first) and keeps its first 0x1C4 bytes as the icon.sys template of
 * new GT3 saves.  Caller: MemorycardControl::begin. */
// FUNC 0x001447D8 gt2k_menu::mcInitIconSys
void mcInitIconSys()
{
    IconSysBuffer buf;
    MemorycardIconSys *iconSys = buf.get();

    file_read_blocking("menu/icon/icon.sys", iconSys, 0, 1, &vol_device_object);
    g_0049BEF0 = *iconSys;
}

/* The common prefix of the GT3 save names, built on the first call from
 * two arrays of another unit ("BA", "SCUS-97102": real strcpy / strcat
 * calls, which a literal would let egcs expand). */
// FUNC 0x00144910 gt2k_menu::mcGetNamePrefix
char *mcGetNamePrefix()
{
    if (g_002A2160[0] == 0) {
        strcpy(g_002A2160, g_00350B38);
        strcat(g_002A2160, g_00308AF0);
    }
    return g_002A2160;
}

/* What follows the prefix in a save name: the type letter and the code.  No
 * callers. */
// FUNC 0x00144960 gt2k_menu::mcGetNameSuffix
char *mcGetNameSuffix(char *name)
{
    return name + MC_NAME_PREFIX_LEN;
}

/* 1 if name starts with the GT3 prefix.  Callers: MemorycardFileList::add,
 * MemorycardFileManager::buildSortedIndex. */
// FUNC 0x00144968 gt2k_menu::mcIsGT3Name
int mcIsGT3Name(const char *name)
{
    const char *prefix = mcGetNamePrefix();

    while (*prefix != 0) {
        if (*prefix++ != *name++)
            return 0;
    }
    return 1;
}

/* The type letter of a GT3 save name. */
// FUNC 0x001449C8 gt2k_menu::mcGetNameType
char mcGetNameType(const char *name)
{
    return name[MC_NAME_TYPE_POS];
}

/* "/BASCUS-97102<type>*" (type < 0: "/BASCUS-97102*"), the listing pattern
 * of one kind of save. */
// FUNC 0x001449D0 gt2k_menu::mcMakeSearchPattern
void mcMakeSearchPattern(char *buf, int type)
{
    buf[0] = '/';
    strcpy(buf + 1, mcGetNamePrefix());
    buf += 1 + MC_NAME_PREFIX_LEN;
    if (type >= 0)
        *buf++ = type;
    *buf++ = '*';
    *buf = 0;
}

/* The 64 digits of the name code. */
const char *g_00351754 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";  /* 0x00351754 */

/* The value 0..63 of a base-64 digit, -1 for NUL or a character outside the
 * alphabet. */
// FUNC 0x00144A30 gt2k_menu::mcDecodeChar
int mcDecodeChar(char c)
{
    const char *digit;

    if (c == 0 || (digit = strchr(g_00351754, c)) == 0)
        return -1;
    return digit - g_00351754;
}

// FUNC 0x00144A70 gt2k_menu::mcEncodeChar
char mcEncodeChar(int value)
{
    return g_00351754[value];
}

/* The 42-bit code of the 7 digits after the type letter (low digit first),
 * -1 if one is not a digit. */
// FUNC 0x00144A80 gt2k_menu::mcDecodeNameCode
s64 mcDecodeNameCode(const char *name)
{
    s64 code = 0;
    s64 scale = 1;
    const char *p = name + MC_NAME_CODE_POS;

    for (int i = 0; i < MC_NAME_CODE_DIGITS; i++) {
        int digit = mcDecodeChar(*p++);
        if (digit < 0)
            return -1;
        /* implicit: __muldi3 (64-bit multiply) */
        code += scale * digit;              /* __muldi3 */
        scale <<= MC_NAME_DIGIT_BITS;
    }
    return code;
}

/* name = prefix + type + 7 digits of code (low digit first). */
// FUNC 0x00144B00 gt2k_menu::mcMakeName
void mcMakeName(char *name, char type, s64 code)
{
    strcpy(name, mcGetNamePrefix());
    name += MC_NAME_PREFIX_LEN;
    *name++ = type;
    for (int i = 0; i < MC_NAME_CODE_DIGITS; i++) {
        *name++ = mcEncodeChar((int)(code & 0x3F));
        code >>= MC_NAME_DIGIT_BITS;
    }
    *name = 0;
}

/* Gives the directory a new name of kind `type': the file's name code plus
 * a 12-bit number, random at first and counted up until the card lists no
 * entry of that name (every number at most once).  1 = done, 0 = listing
 * error or no free name.  Caller: saveFiles. */
// FUNC 0x00144B90 gt2k_menu::MemorycardGT3File::makeUniqueName
int MemorycardGT3File::makeUniqueName(int port, char type)
{
    McDirEntry entry;                       /* sp+0x00 */
    int tries = 0;
    char *name = dir.entry.name;
    s64 fields = getNameCode() & MC_CODE_FIELDS_MASK;
    /* implicit: __fixsfdi (float to s64 conversion) */
    s64 unique = (s64)(vu0_rand() * 4095.0f);   /* __fixsfdi (0x00278630) */

    do {
        mcMakeName(name, type, fields | ((unique & MC_CODE_UNIQUE_MASK) << MC_CODE_UNIQUE_SHIFT));
        if (mc_get_dir(port, name, &entry, 0, 1) != MC_STATUS_OK)
            return 0;
        if (mc_slot_dir_count(port) == 0)
            return 1;                       /* not on the card */
        unique++;
    } while (++tries < MC_UNIQUE_TRIES);
    return 0;
}

/* The fields of the name code (-1 if the name has no valid code).  Caller
 * of all four: MemorycardFileManager::buildSortedIndex. */
// FUNC 0x00144C80 gt2k_menu::mcGetNameCourse
int mcGetNameCourse(const char *name)
{
    s64 code = mcDecodeNameCode(name);
    if (code < 0)
        return -1;
    return (int)((code >> MC_CODE_COURSE_SHIFT) & MC_CODE_COURSE_MASK);
}

// FUNC 0x00144CB8 gt2k_menu::mcGetNameCar
int mcGetNameCar(const char *name)
{
    s64 code = mcDecodeNameCode(name);
    if (code < 0)
        return -1;
    return (int)(code & MC_CODE_CAR_MASK);
}

// FUNC 0x00144CF0 gt2k_menu::mcGetNameCategory
int mcGetNameCategory(const char *name)
{
    s64 code = mcDecodeNameCode(name);
    if (code < 0)
        return -1;
    return (int)((code >> MC_CODE_CATEGORY_SHIFT) & MC_CODE_CATEGORY_MASK);
}

// FUNC 0x00144D28 gt2k_menu::mcGetNameSubCategory
int mcGetNameSubCategory(const char *name)
{
    s64 code = mcDecodeNameCode(name);
    if (code < 0)
        return -1;
    return (int)((code >> MC_CODE_SUBCAT_SHIFT) & MC_CODE_SUBCAT_MASK);
}

/* Counts the saves of kind `type' on the card (16 entries per listing), -1
 * on a listing error.  No callers.  Quirk kept: every listing starts at
 * entry 0, so with 16 or more matches it never ends. */
// FUNC 0x00144D60 gt2k_menu::mcCountFiles
int mcCountFiles(int port, char type)
{
    char pattern[0x30];                     /* sp+0x000 */
    McDirEntry entries[MC_COUNT_CHUNK];     /* sp+0x030 */
    int count;
    int listed;

    mcMakeSearchPattern(pattern, type);
    count = 0;                              /* after the call: retail's jal delay slot */
    do {
        if (mc_get_dir(port, pattern, entries, 0, MC_COUNT_CHUNK) != MC_STATUS_OK)
            return -1;
        listed = mc_slot_dir_count(port);
        count += listed;
    } while (listed >= MC_COUNT_CHUNK);
    return count;
}

/* ======================================================================== */
/* MemorycardGT3File                                                        */
/* ======================================================================== */

/* "/<name>/thumbnail"; returns buf. */
// FUNC 0x00144DE0 gt2k_menu::MemorycardGT3File::getThumbnailPath
char *MemorycardGT3File::getThumbnailPath(char *buf)
{
    char *path = buf + 1;

    buf[0] = '/';
    strcpy(path, dir.entry.name);
    strcat(path, "/thumbnail");
    return buf;
}

/* Reads the thumbnail file (all 0x60 bytes, else 0).  The slot of every GT3
 * vtable (index: MemorycardSettingFile::virtual_9). */
// FUNC 0x00144E38 gt2k_menu::MemorycardGT3File::readInfo
int MemorycardGT3File::readInfo(int port)
{
    char path[0x50];

    getThumbnailPath(path);
    if (mc_read_file(port, path, &thumbnail, 0, sizeof(gt2k_game::GameThumbnail)) == MC_STATUS_OK) {
        if (mc_slot_read_size(port) >= (int)sizeof(gt2k_game::GameThumbnail)) {
            infoRead = 1;
            thumbnailValid = 1;
            return 1;
        }
    }
    return 0;
}

/* sizeKB = the 1 KB clusters a save of dataSize bytes takes: the data
 * file, one each for icon.sys (0x3C4 bytes) and the thumbnail, the icon
 * files (their size on the volume), and the directory's entries, two per
 * cluster: (icons + 4) / 2 + 1 = the clusters of ".", "..", the three files
 * and the icons.  Caller: MemorycardControl::begin. */
// FUNC 0x00144EB0 gt2k_menu::MemorycardGT3File::calcSize
void MemorycardGT3File::calcSize(int dataSize)
{
    int clusters = (dataSize + MC_CLUSTER_SIZE - 1) / MC_CLUSTER_SIZE;    /* the data file */
    clusters += 2;                          /* icon.sys, the thumbnail */
    int icons = 0;
    const char **file = getIconFiles();

    while (*file != 0) {
        int bytes = (int)file_size_retry(*file++, 1, &vol_device_object);
        icons++;
        clusters += (bytes + MC_CLUSTER_SIZE - 1) / MC_CLUSTER_SIZE;
    }
    clusters += (icons + 4) / MC_DIR_ENTRIES_PER_CLUSTER;   /* the directory's entries */
    clusters++;
    sizeKB = clusters;
}

/* Writes the save, calling progress after each step (step / total: total 4
 * for a plain overwrite, + 2 when formatting first, + 5 for a new save or a
 * renamed one):
 *   format:              only an unformatted card (mc_slot_formatted 0) is
 *                        formatted; a formatted one makes the save fail;
 *   a new save (id -1):  (newName: a fresh unique name,) the directory, the
 *                        icon files, icon.sys;
 *   an old save:         newName: a fresh name, the old data file is
 *                        deleted and the directory renamed (the old name is
 *                        put back if no name was found);
 *   then the data file and the thumbnail.  Other ids (< -1) fail.  1 = done.
 * Callers: the save of the four GT3 kinds (newName 0 for the game data). */
// FUNC 0x00144F90 gt2k_menu::MemorycardGT3File::saveFiles
int MemorycardGT3File::saveFiles(int port, void *data, int dataSize, char type, bool format,
                                 MemorycardProgressFunc progress, void *ctx, int newName)
{
    char path[0x50];                        /* sp+0x00 */
    char oldName[0x30];                     /* sp+0x50 */
    char oldPath[0x50];                     /* sp+0x80 */
    int total = 4;
    int step = 0;

    if (format) {
        total = 6;
        step = 2;
        reportProgress(progress, ctx, step, total);     /* 0.33333334f */
        if (mc_slot_formatted(port) != 0)
            return 0;
        if (mc_format(port) != MC_STATUS_OK)
            return 0;
    }

    if (dir.id == MC_DIR_ID_NEW) {
        total += 5;
        step++;
        reportProgress(progress, ctx, step, total);
        if (newName && !makeUniqueName(port, type))
            return 0;
        step++;
        reportProgress(progress, ctx, step, total);
        if (!makeDir(port))
            return 0;
        step += 2;
        reportProgress(progress, ctx, step, total);
        if (!writeIcons(port, getIconFiles()))
            return 0;
        step++;
        reportProgress(progress, ctx, step, total);
        if (!writeIconSys(port))
            return 0;
    } else {
        if (dir.id < 0)
            return 0;
        if (newName) {
            total += 5;
            step++;
            reportProgress(progress, ctx, step, total);
            step++;
            reportProgress(progress, ctx, step, total);
            char *name = dir.entry.name;
            strcpy(oldName, name);
            step++;
            getMainFilePath(oldPath);
            reportProgress(progress, ctx, step, total);
            if (!makeUniqueName(port, type)) {
                strcpy(name, oldName);
                return 0;
            }
            step++;
            reportProgress(progress, ctx, step, total);
            if (mc_delete(port, oldPath) != MC_STATUS_OK)
                return 0;
            step++;
            reportProgress(progress, ctx, step, total);
            if (mc_rename(port, oldName, name) != MC_STATUS_OK)
                return 0;
        }
    }

    step += 3;
    reportProgress(progress, ctx, step, total);
    getMainFilePath(path);
    if (mc_create_file(port, path, data, dataSize) != MC_STATUS_OK)
        return 0;
    step++;
    reportProgress(progress, ctx, step, total);
    getThumbnailPath(path);
    if (mc_create_file(port, path, &thumbnail, sizeof(gt2k_game::GameThumbnail)) != MC_STATUS_OK)
        return 0;
    reportProgress(progress, ctx, step, total);     /* the same step again */
    return 1;
}

/* Reads the data file of the save into a new buffer (memalign 64; NULL on
 * failure).  The slot of every GT3 vtable (index:
 * MemorycardSettingFile::virtual_16).  Callers (virtual):
 * MemorycardAutoLoad::load, WorkingMenu::makeSubMenu. */
// FUNC 0x001453D8 gt2k_menu::MemorycardGT3File::load
void *MemorycardGT3File::load(int port, MemorycardProgressFunc progress, void *ctx)
{
    char path[0x50];                        /* sp+0x00 */
    McDirEntry entry;                       /* sp+0x50 */

    mc_slot_take_status(port);              /* the status of an earlier operation is dropped */
    if (dir.id < 0)
        return 0;
    if (progress != 0)
        progress(0.1f, ctx);
    getMainFilePath(path);
    if (mc_get_dir(port, path, &entry, 0, 1) != MC_STATUS_OK)
        return 0;
    if (mc_slot_dir_count(port) != 1)
        return 0;
    int size = entry.size;
    if (progress != 0)
        progress(1.0f, ctx);
    void *buf = memalign(64, size);
    if (buf == 0)
        return 0;
    if (mc_read_file(port, path, buf, 0, size) != MC_STATUS_OK) {
        free(buf);
        return 0;
    }
    if (progress != 0)
        progress(1.0f, ctx);
    return buf;
}

/* The file named like the directory, read from the disc volume instead of
 * the card: the demo replays of MemorycardDemoFileManager (T079), whose
 * names are volume paths.  (index: MemorycardSettingFile::virtual_17.) */
// FUNC 0x00145508 gt2k_menu::MemorycardGT3File::loadFromVolume
void *MemorycardGT3File::loadFromVolume(MemorycardProgressFunc progress, void *ctx)
{
    if (progress != 0)
        progress(1.0f, ctx);
    void *buf = file_read_blocking(dir.entry.name, 0, 0, 1, &vol_device_object);
    if (progress != 0)
        progress(1.0f, ctx);
    return buf;
}

/* A new comment: rewrites the thumbnail file only.  1 = done.  Caller:
 * CommentChangeWorkingMenu::makeSubMenu. */
// FUNC 0x00145590 gt2k_menu::MemorycardGT3File::changeComment
int MemorycardGT3File::changeComment(int port, const char *comment,
                                     MemorycardProgressFunc progress, void *ctx)
{
    char path[0x50];

    if (progress != 0)
        progress(1.0f, ctx);
    strcpy(thumbnail.comment, comment);
    getThumbnailPath(path);
    if (mc_create_file(port, path, &thumbnail, sizeof(gt2k_game::GameThumbnail)) != MC_STATUS_OK)
        return 0;
    if (progress != 0)
        progress(1.0f, ctx);
    return 1;
}

/* ======================================================================== */
/* MemorycardGameFile ('G')                                                 */
/* ======================================================================== */

/* The game data has a fixed name.  (If strcpy / strcat threw, the landing
 * pad would undo the base: ~MemorycardGT3File.)  Callers:
 * MemorycardControl::begin, memorycard_game_file_exists (T075, 0x00156A98),
 * MemorycardAutoLoad::load, WorkingMenu::makeSubMenu, warning_check_card. */
// FUNC 0x00145648 gt2k_menu::MemorycardGameFile::MemorycardGameFile
MemorycardGameFile::MemorycardGameFile()
    : MemorycardGT3File()
{
    char *name = dir.entry.name;

    strcpy(name, mcGetNamePrefix());
    strcat(name, "GAMEDATA");
}

/* Caller: MemorycardFileList::add. */
// FUNC 0x001456D8 gt2k_menu::MemorycardGameFile::MemorycardGameFile(const MemorycardDir&)
MemorycardGameFile::MemorycardGameFile(const MemorycardDir &d)
    : MemorycardGT3File(d)
{
}

/* No callers. */
// FUNC 0x00145710 gt2k_menu::MemorycardGameFile::makeSearchPattern
void MemorycardGameFile::makeSearchPattern(char *buf)
{
    mcMakeSearchPattern(buf, MC_TYPE_GAME);
}

// FUNC 0x00145730 gt2k_menu::MemorycardGameFile::render
void MemorycardGameFile::render(MemorycardIconRenderInfo &info)
{
    /* implicit: draw (inline helper drawBuiltinIcon) */
    drawBuiltinIcon(info, MC_ICON_GAME);    /* MemorycardIconSprites::draw */
}

// FUNC 0x00145798 gt2k_menu::MemorycardGameFile::getInfoName
const char *MemorycardGameFile::getInfoName()
{
    return "info_game";
}

const char *g_00351758[2] = { "menu/icon/ICON_a_spec.ico", NULL };    /* 0x00351758 */

// FUNC 0x001457A8 gt2k_menu::MemorycardGameFile::getIconFiles
const char **MemorycardGameFile::getIconFiles()
{
    return g_00351758;                      /* ICON_a_spec.ico */
}

/* Title "GT3 game data" + '\n' (g_002B1C90), never a new name. */
// FUNC 0x001457B8 gt2k_menu::MemorycardGameFile::save
int MemorycardGameFile::save(int port, void *data, int size, bool format,
                             MemorycardProgressFunc progress, void *ctx)
{
    char title[0x100];                      /* sp+0x10 */

    sprintf(title, "%s\n", g_002B1C68[MCTEXT_GT3_GAME_DATA]);
    setTitle(title);
    return saveFiles(port, data, size, MC_TYPE_GAME, format, progress, ctx, 0);
}

// FUNC 0x00145870 gt2k_menu::MemorycardGameFile::getNameCode
s64 MemorycardGameFile::getNameCode()
{
    return 0;
}

// FUNC 0x00145878 gt2k_menu::MemorycardGameFile::checkSelectable
void MemorycardGameFile::checkSelectable(int filter, u64 course, u64 car,
                                         int category, int subCategory)
{
    if (filter == MC_TYPE_GAME || filter == 0)
        selectable = 1;
    else
        selectable = 0;
}

/* ======================================================================== */
/* MemorycardReplayFile ('R')                                               */
/* ======================================================================== */

/* Callers: MemorycardControl::begin, MemorycardDemoFileManager::buildList. */
// FUNC 0x001458A0 gt2k_menu::MemorycardReplayFile::MemorycardReplayFile
MemorycardReplayFile::MemorycardReplayFile()
    : MemorycardGT3File()
{
}

/* Caller: MemorycardFileList::add. */
// FUNC 0x001458D8 gt2k_menu::MemorycardReplayFile::MemorycardReplayFile(const MemorycardDir&)
MemorycardReplayFile::MemorycardReplayFile(const MemorycardDir &d)
    : MemorycardGT3File(d)
{
}

/* Caller: MemorycardFileManager::setFilter. */
// FUNC 0x00145910 gt2k_menu::MemorycardReplayFile::makeSearchPattern
void MemorycardReplayFile::makeSearchPattern(char *buf)
{
    mcMakeSearchPattern(buf, MC_TYPE_REPLAY);
}

// FUNC 0x00145930 gt2k_menu::MemorycardReplayFile::render
void MemorycardReplayFile::render(MemorycardIconRenderInfo &info)
{
    /* implicit: draw (inline helper drawBuiltinIcon) */
    drawBuiltinIcon(info, MC_ICON_REPLAY);  /* MemorycardIconSprites::draw */
}

// FUNC 0x00145998 gt2k_menu::MemorycardReplayFile::getInfoName
const char *MemorycardReplayFile::getInfoName()
{
    return "info_replay";
}

const char *g_00351768[2] = { "menu/icon/ICON_full.ico", NULL };      /* 0x00351768 */

// FUNC 0x001459A8 gt2k_menu::MemorycardReplayFile::getIconFiles
const char **MemorycardReplayFile::getIconFiles()
{
    return g_00351768;                      /* ICON_full.ico */
}

/* Title "GT3 replay data" + '\n' + the first 16 characters of the comment
 * made full-width; a new name every time. */
// FUNC 0x001459B8 gt2k_menu::MemorycardReplayFile::save
int MemorycardReplayFile::save(int port, void *data, int size, bool format,
                               MemorycardProgressFunc progress, void *ctx)
{
    char title[0x50];                       /* sp+0x10 */
    char wide[0x50];                        /* sp+0x60 */
    char comment[0x30];                     /* sp+0xB0 (the frame gives it 0x21..0x30 bytes; 17 used) */

    strcpy(title, g_002B1C68[MCTEXT_GT3_REPLAY_DATA]);
    strcat(title, "\n");
    strncpy(comment, thumbnail.comment, MC_TITLE_COMMENT_LEN);
    comment[MC_TITLE_COMMENT_LEN] = 0;
    ascii_to_fullwidth(wide, comment);
    strcat(title, wide);
    setTitle(title);
    return saveFiles(port, data, size, MC_TYPE_REPLAY, format, progress, ctx, 1);
}

/* course index << 12 | car index | category << 21 | sub-category << 26 (the
 * order of the mcGetName* functions). */
// FUNC 0x00145AC0 gt2k_menu::MemorycardReplayFile::getNameCode
s64 MemorycardReplayFile::getNameCode()
{
    int course = db_course_index(thumbnail.replay.course);
    int car = db_car_index(thumbnail.replay.car);

    /* an int, sign-extended to the s64 (retail's final move) */
    return (course << MC_CODE_COURSE_SHIFT) | car | (thumbnail.category << MC_CODE_CATEGORY_SHIFT)
         | (thumbnail.subCategory << MC_CODE_SUBCAT_SHIFT);
}

// FUNC 0x00145B20 gt2k_menu::MemorycardReplayFile::checkSelectable
void MemorycardReplayFile::checkSelectable(int filter, u64 course, u64 car,
                                           int category, int subCategory)
{
    if ((filter == MC_TYPE_REPLAY
         && (course == 0 || course == thumbnail.replay.course)
         && (car == 0 || car == thumbnail.replay.car)
         && (category == 0 || category == thumbnail.category)
         && (subCategory == 0 || subCategory == thumbnail.subCategory))
        || filter == 0)
        selectable = 1;
    else
        selectable = 0;
}

/* ======================================================================== */
/* MemorycardBestlapReplayFile ('B'; categories 9..14: licence replays)     */
/* ======================================================================== */

/* Callers: MemorycardControl::begin, MemorycardDemoFileManager::buildList. */
// FUNC 0x00145B98 gt2k_menu::MemorycardBestlapReplayFile::MemorycardBestlapReplayFile
MemorycardBestlapReplayFile::MemorycardBestlapReplayFile()
    : MemorycardGT3File()
{
}

/* Caller: MemorycardFileList::add. */
// FUNC 0x00145BD0 gt2k_menu::MemorycardBestlapReplayFile::MemorycardBestlapReplayFile(const MemorycardDir&)
MemorycardBestlapReplayFile::MemorycardBestlapReplayFile(const MemorycardDir &d)
    : MemorycardGT3File(d)
{
}

/* Caller: MemorycardFileManager::setFilter. */
// FUNC 0x00145C08 gt2k_menu::MemorycardBestlapReplayFile::makeSearchPattern
void MemorycardBestlapReplayFile::makeSearchPattern(char *buf)
{
    mcMakeSearchPattern(buf, MC_TYPE_BESTLAP);
}

// FUNC 0x00145C28 gt2k_menu::MemorycardBestlapReplayFile::render
void MemorycardBestlapReplayFile::render(MemorycardIconRenderInfo &info)
{
    MemorycardIconSprites *sprites = info.sprites;

    /* the icon set up once, the two draw calls in the branches */
    sprites->setPosition(info.x, info.y);
    sprites->setSize(info.w, info.h);
    sprites->setColor(info.r, info.g, info.b, info.a);
    if (MC_IS_LICENCE_CATEGORY(thumbnail.category))
        sprites->draw(MC_ICON_LICENSE);
    else
        sprites->draw(MC_ICON_BEST_REPLAY);
}

// FUNC 0x00145CA8 gt2k_menu::MemorycardBestlapReplayFile::getInfoName
const char *MemorycardBestlapReplayFile::getInfoName()
{
    return "info_best_replay";
}

const char *g_00351778[2] = { "menu/icon/ICON_best.ico", NULL };      /* 0x00351778 */
const char *g_00351780[2] = { "menu/icon/ICON_license.ico", NULL };   /* 0x00351780 */

// FUNC 0x00145CB8 gt2k_menu::MemorycardBestlapReplayFile::getIconFiles
const char **MemorycardBestlapReplayFile::getIconFiles()
{
    return MC_IS_LICENCE_CATEGORY(thumbnail.category) ? g_00351780    /* ICON_license.ico */
                                                      : g_00351778;   /* ICON_best.ico */
}

/* Title "GT3 licence replay data" (categories 9..14) or "GT3 best lap replay
 * data", then as MemorycardReplayFile::save. */
// FUNC 0x00145CD8 gt2k_menu::MemorycardBestlapReplayFile::save
int MemorycardBestlapReplayFile::save(int port, void *data, int size, bool format,
                                      MemorycardProgressFunc progress, void *ctx)
{
    char title[0x50];                       /* sp+0x10 */
    char wide[0x50];                        /* sp+0x60 */
    char comment[0x30];                     /* sp+0xB0 (the frame gives it 0x21..0x30 bytes; 17 used) */

    if (MC_IS_LICENCE_CATEGORY(thumbnail.category))
        strcpy(title, g_002B1C68[MCTEXT_GT3_LICENSE_REPLAY_DATA]);
    else
        strcpy(title, g_002B1C68[MCTEXT_GT3_BEST_LAP_REPLAY_DATA]);
    strcat(title, "\n");
    strncpy(comment, thumbnail.comment, MC_TITLE_COMMENT_LEN);
    comment[MC_TITLE_COMMENT_LEN] = 0;
    ascii_to_fullwidth(wide, comment);
    strcat(title, wide);
    setTitle(title);
    return saveFiles(port, data, size, MC_TYPE_BESTLAP, format, progress, ctx, 1);
}

/* As MemorycardReplayFile::getNameCode. */
// FUNC 0x00145E08 gt2k_menu::MemorycardBestlapReplayFile::getNameCode
s64 MemorycardBestlapReplayFile::getNameCode()
{
    int course = db_course_index(thumbnail.bestlap.course);
    int car = db_car_index(thumbnail.bestlap.car);

    /* an int, sign-extended to the s64 (retail's final move) */
    return (course << MC_CODE_COURSE_SHIFT) | car | (thumbnail.category << MC_CODE_CATEGORY_SHIFT)
         | (thumbnail.subCategory << MC_CODE_SUBCAT_SHIFT);
}

/* Selectable by the replay filter as well. */
// FUNC 0x00145E68 gt2k_menu::MemorycardBestlapReplayFile::checkSelectable
void MemorycardBestlapReplayFile::checkSelectable(int filter, u64 course, u64 car,
                                                  int category, int subCategory)
{
    if (((filter == MC_TYPE_BESTLAP || filter == MC_TYPE_REPLAY)
         && (course == 0 || course == thumbnail.bestlap.course)
         && (car == 0 || car == thumbnail.bestlap.car)
         && (category == 0 || category == thumbnail.category)
         && (subCategory == 0 || subCategory == thumbnail.subCategory))
        || filter == 0)
        selectable = 1;
    else
        selectable = 0;
}

/* ======================================================================== */
/* MemorycardSettingFile ('S')                                              */
/* ======================================================================== */

/* Caller: MemorycardControl::begin. */
// FUNC 0x00145EE8 gt2k_menu::MemorycardSettingFile::MemorycardSettingFile
MemorycardSettingFile::MemorycardSettingFile()
    : MemorycardGT3File()
{
}

/* Caller: MemorycardFileList::add. */
// FUNC 0x00145F20 gt2k_menu::MemorycardSettingFile::MemorycardSettingFile(const MemorycardDir&)
MemorycardSettingFile::MemorycardSettingFile(const MemorycardDir &d)
    : MemorycardGT3File(d)
{
}

/* Caller: MemorycardFileManager::setFilter. */
// FUNC 0x00145F58 gt2k_menu::MemorycardSettingFile::makeSearchPattern
void MemorycardSettingFile::makeSearchPattern(char *buf)
{
    mcMakeSearchPattern(buf, MC_TYPE_SETTING);
}

// FUNC 0x00145F78 gt2k_menu::MemorycardSettingFile::render
void MemorycardSettingFile::render(MemorycardIconRenderInfo &info)
{
    /* implicit: draw (inline helper drawBuiltinIcon) */
    drawBuiltinIcon(info, MC_ICON_SETTING); /* MemorycardIconSprites::draw */
}

// FUNC 0x00145FE0 gt2k_menu::MemorycardSettingFile::getInfoName
const char *MemorycardSettingFile::getInfoName()
{
    return "info_setting";
}

const char *g_00351788[2] = { "menu/icon/ICON_setting.ico", NULL };   /* 0x00351788 */

// FUNC 0x00145FF0 gt2k_menu::MemorycardSettingFile::getIconFiles
const char **MemorycardSettingFile::getIconFiles()
{
    return g_00351788;                      /* ICON_setting.ico */
}

/* The setting's car code is at +0x08 of the thumbnail and its course code
 * at +0x10 (the replays': +0x10 / +0x18). */
// FUNC 0x00146000 gt2k_menu::MemorycardSettingFile::getNameCode
s64 MemorycardSettingFile::getNameCode()
{
    int course = db_course_index(thumbnail.setting.course);
    int car = db_car_index(thumbnail.setting.car);

    /* an int, sign-extended to the s64 (retail's final move) */
    return (course << MC_CODE_COURSE_SHIFT) | car | (thumbnail.category << MC_CODE_CATEGORY_SHIFT)
         | (thumbnail.subCategory << MC_CODE_SUBCAT_SHIFT);
}

// FUNC 0x00146060 gt2k_menu::MemorycardSettingFile::checkSelectable
void MemorycardSettingFile::checkSelectable(int filter, u64 course, u64 car,
                                            int category, int subCategory)
{
    if ((filter == MC_TYPE_SETTING
         && (course == 0 || course == thumbnail.setting.course)
         && (car == 0 || car == thumbnail.setting.car)
         && (category == 0 || category == thumbnail.category)
         && (subCategory == 0 || subCategory == thumbnail.subCategory))
        || filter == 0)
        selectable = 1;
    else
        selectable = 0;
}

/* Title "GT3 setting data" + '\n' + the comment, as the replay. */
// FUNC 0x001460D8 gt2k_menu::MemorycardSettingFile::save
int MemorycardSettingFile::save(int port, void *data, int size, bool format,
                                MemorycardProgressFunc progress, void *ctx)
{
    char title[0x50];                       /* sp+0x10 */
    char wide[0x50];                        /* sp+0x60 */
    char comment[0x30];                     /* sp+0xB0 (the frame gives it 0x21..0x30 bytes; 17 used) */

    strcpy(title, g_002B1C68[MCTEXT_GT3_SETTING_DATA]);
    strcat(title, "\n");
    strncpy(comment, thumbnail.comment, MC_TITLE_COMMENT_LEN);
    comment[MC_TITLE_COMMENT_LEN] = 0;
    ascii_to_fullwidth(wide, comment);
    strcat(title, wide);
    setTitle(title);
    return saveFiles(port, data, size, MC_TYPE_SETTING, format, progress, ctx, 1);
}

/* ======================================================================== */
/* MemorycardFileList                                                       */
/* ======================================================================== */

/* count cells of the size of the largest file class, rounded up to 16
 * bytes.  The local table is a copy of .rodata 0x0031B780 =
 * {0x270, 0x238, 0x298, 0x298, 0x298, 0x68}; the store into cellSize is
 * repeated at each new maximum.  Caller: the MemorycardFileManager
 * constructor (0x00147870, 200 cells). */
// FUNC 0x001461E0 gt2k_menu::MemorycardFileList::MemorycardFileList
MemorycardFileList::MemorycardFileList(int count)
    : List<MemorycardFile>()
{
    cellCount = count;
    cellSize = 0;
    /* (? which classes) The values fix the sizes only: the three 0x298
     * entries are three of the four GT3 kinds (or MemorycardGT3File), the
     * 0x68 one any of the 0x68 classes (add builds MemorycardDamageFile). */
    u32 sizes[6] = {
        sizeof(MemorycardPS1File),          /* 0x270 */
        sizeof(MemorycardOtherPS2File),     /* 0x238 */
        sizeof(MemorycardGameFile),         /* 0x298 */
        sizeof(MemorycardReplayFile),       /* 0x298 */
        sizeof(MemorycardSettingFile),      /* 0x298 */
        sizeof(MemorycardDamageFile),       /* 0x068 */
    };
    for (int i = 0; i < 6; i++) {
        if (cellSize < sizes[i])
            cellSize = sizes[i];
    }
    cellSize = (cellSize + MC_CELL_ALIGN - 1) & ~(u32)(MC_CELL_ALIGN - 1);
    pool = memalign(16, cellSize * count);
    buildFreeList();
    scoreFn = 0;
    scoreCtx = 0;
    /* (if this threw, the landing pad would destroy the List<MemorycardFile>
     * base: its vptr, ~pList(this, 0)) */
    /* implicit: ~pList (base destructor, landing pad) */
}

/* Every file released, the pool freed.  Callers: the landing pad of
 * 0x00147870, ~MemorycardFileManager. */
// FUNC 0x001462F8 gt2k_menu::MemorycardFileList::~MemorycardFileList
MemorycardFileList::~MemorycardFileList()
{
    releaseAll();
    free(pool);
    /* then ~List<MemorycardFile>: its vptr, ~pList(this, flags) */
    /* implicit: ~pList (base destructor) */
}

/* Links all cells of the pool into the free list, in address order. */
// FUNC 0x00146358 gt2k_menu::MemorycardFileList::buildFreeList
void MemorycardFileList::buildFreeList()
{
    freeList = pool;
    u8 *cell = (u8 *)pool;
    int last = cellCount - 1;

    for (int i = 0; i < last; i++) {
        u8 *nextCell = cell + cellSize;
        *(void **)cell = nextCell;
        cell = nextCell;
    }
    *(void **)cell = 0;
}

/* Every file: prepareRelease; two vsyncs (the icons stop being drawn);
 * then every file: releaseIcon(true) and back to the free list.  Callers:
 * ~MemorycardFileList, MemorycardFileManager::clear / setFilter. */
// FUNC 0x001463A0 gt2k_menu::MemorycardFileList::releaseAll
void MemorycardFileList::releaseAll()
{
    /* two blocks: egcs gives their iterators one stack slot (retail's
     * 0($sp) in both walks), and the first file pointer, not live across a
     * call, a temporary register */
    {
        ListIterator<MemorycardFile> it(this);
        MemorycardFile *file;
        while ((file = it.next()) != 0)
            file->prepareRelease();
    }
    gfx_vsync(2);
    {
        ListIterator<MemorycardFile> it(this);
        MemorycardFile *file;
        while ((file = it.next()) != 0) {
            file->releaseIcon(true);
            release(file);
        }
    }
}

/* Callers: MemorycardFileManager::getFile, readInfo, readSize, readIcon. */
// FUNC 0x00146470 gt2k_menu::MemorycardFileList::findById
MemorycardFile *MemorycardFileList::findById(int id)
{
    MemorycardFile *file;

    ListIterator<MemorycardFile> it(this);
    while ((file = it.next()) != 0) {
        if (file->dir.id == id)
            return file;
    }
    return 0;
}

/* Callers: MemorycardFileManager::updateFileId / updateFileIdForDelete. */
// FUNC 0x001464C0 gt2k_menu::MemorycardFileList::findByName
MemorycardFile *MemorycardFileList::findByName(const char *name)
{
    MemorycardFile *file;

    ListIterator<MemorycardFile> it(this);
    while ((file = it.next()) != 0) {
        if (strcmp(file->dir.entry.name, name) == 0)
            return file;
    }
    return 0;
}

/* Before a new listing: every file's id is marked not seen
 * (MemorycardDir::setOldId).  Callers: MemorycardFileManager::updateFileId /
 * updateFileIdForDelete. */
// FUNC 0x00146540 gt2k_menu::MemorycardFileList::markAll
void MemorycardFileList::markAll()
{
    MemorycardFile *file;

    ListIterator<MemorycardFile> it(this);
    while ((file = it.next()) != 0)
        file->dir.setOldId();
}

/* After it: the files still marked (id <= -0x400) are released.  Callers:
 * MemorycardFileManager::updateFileId / updateFileIdForDelete. */
// FUNC 0x00146598 gt2k_menu::MemorycardFileList::removeMarked
void MemorycardFileList::removeMarked()
{
    MemorycardFile *file;

    ListIterator<MemorycardFile> it(this);
    while ((file = it.next()) != 0) {
        if (file->dir.id <= MC_DIR_ID_OLD_MAX)
            release(file);
    }
}

/* A free cell (raw memory), NULL if none. */
// FUNC 0x00146610 gt2k_menu::MemorycardFileList::allocCell
void *MemorycardFileList::allocCell()
{
    void *cell = freeList;

    if (cell == 0)
        return 0;
    freeList = *(void **)cell;
    return cell;
}

/* Caller: MemorycardFileManager::readInfo. */
// FUNC 0x00146630 gt2k_menu::MemorycardFileList::hasFreeCell
int MemorycardFileList::hasFreeCell()
{
    return freeList != 0;
}

/* Unlinks the file, destroys it (its virtual destructor, flags 2: no
 * delete) and puts its cell on the free list. */
// FUNC 0x00146640 gt2k_menu::MemorycardFileList::release
void MemorycardFileList::release(MemorycardFile *file)
{
    unlink(file);
    file->~MemorycardFile();
    *(void **)file = freeList;
    freeList = file;
}

/* Caller: MemorycardFileManager::setupScoreFunc. */
// FUNC 0x00146698 gt2k_menu::MemorycardFileList::setScoreFunc
void MemorycardFileList::setScoreFunc(int (*fn)(void *ctx, MemorycardFile *file), void *ctx)
{
    scoreFn = fn;
    scoreCtx = ctx;
}

/* The file scoreFn rates highest (scores above -1 only; NULL if none): the
 * one recycled when the pool is full.  Callers: add,
 * MemorycardFileManager::readInfo. */
// FUNC 0x001466A8 gt2k_menu::MemorycardFileList::findMaxScore
MemorycardFile *MemorycardFileList::findMaxScore()
{
    MemorycardFile *best = 0;
    int bestScore = -1;
    MemorycardFile *file;

    ListIterator<MemorycardFile> it(this);
    while ((file = it.next()) != 0) {
        int score = scoreFn(scoreCtx, file);
        if (bestScore < score) {
            bestScore = score;
            best = file;
        }
    }
    return best;
}

/* The same among the files holding icon memory.  Caller:
 * MemorycardFileManager::readIcon. */
// FUNC 0x00146750 gt2k_menu::MemorycardFileList::findMaxScoreWithIcon
MemorycardFile *MemorycardFileList::findMaxScoreWithIcon()
{
    MemorycardFile *best = 0;
    int bestScore = -1;
    MemorycardFile *file;

    ListIterator<MemorycardFile> it(this);
    while ((file = it.next()) != 0) {
        if (file->isAllocIconMemory()) {
            int score = scoreFn(scoreCtx, file);
            if (bestScore < score) {
                bestScore = score;
                best = file;
            }
        }
    }
    return best;
}

/* A file object for a listed directory, appended to the list: a PS1 save
 * (attr 0x1000), a GT3 save of a known kind, a damaged file (a GT3 name of
 * another kind) or another game's PS2 save.  When the pool is full the
 * highest-scored file is released first.  NULL if no cell.  Caller:
 * MemorycardFileManager::readInfo. */
// FUNC 0x00146810 gt2k_menu::MemorycardFileList::add
MemorycardFile *MemorycardFileList::add(const MemorycardDir &d)
{
    void *cell = allocCell();

    if (cell == 0) {
        MemorycardFile *victim = findMaxScore();
        if (victim == 0)
            return 0;
        release(victim);
        cell = allocCell();
        if (cell == 0)
            return 0;
    }

    MemorycardFile *file;
    if (d.entry.attr & sceMcFileAttrPS1) {
        file = new (cell) MemorycardPS1File(d);
    } else if (mcIsGT3Name(d.entry.name)) {
        switch (mcGetNameType(d.entry.name)) {
        case MC_TYPE_GAME:
            file = new (cell) MemorycardGameFile(d);
            break;
        case MC_TYPE_REPLAY:
            file = new (cell) MemorycardReplayFile(d);
            break;
        case MC_TYPE_BESTLAP:
            file = new (cell) MemorycardBestlapReplayFile(d);
            break;
        case MC_TYPE_SETTING:
            file = new (cell) MemorycardSettingFile(d);
            break;
        default:
            file = new (cell) MemorycardDamageFile(d);
            break;
        }
    } else {
        file = new (cell) MemorycardOtherPS2File(d);
    }
    if (file == 0)                          /* egcs's check of the placement new */
        return 0;
    append(file);
    return file;
}

/* Steps through the list and does nothing with the files (a debug dump
 * compiled out, probably).  No callers. */
// FUNC 0x001469B8 gt2k_menu::MemorycardFileList::walk
void MemorycardFileList::walk()
{
    ListIterator<MemorycardFile> it(this);

    while (it.next() != 0)
        ;
}

/* Replaces file by a MemorycardDamageFile of the same directory (in a new
 * cell), unless it is one already.  0 only if no cell was free.  Callers:
 * MemorycardFileManager::readInfo, readSize, readIcon.  The directory is
 * copied before file is checked for NULL (a NULL file is released). */
// FUNC 0x001469F0 gt2k_menu::MemorycardFileList::setDamaged
int MemorycardFileList::setDamaged(MemorycardFile *file)
{
    MemorycardDir d = file->dir;            /* sp+0x00 */

    if (dynamic_cast<MemorycardDamageFile *>(file) == 0) {
        release(file);
        void *cell = allocCell();
        if (cell == 0)
            return 0;
        append(new (cell) MemorycardDamageFile(d));
    }
    return 1;
}

/* ======================================================================== */
/* The copies egcs emits at the end of the unit                            */
/* ======================================================================== */
/* The template destructors first, then class by class in reverse definition
 * order (MemorycardFileList, List<MemorycardFile>, the four GT3 kinds,
 * MemorycardGT3File, the deleted, dummy, damaged, other-PS2, PS2 and PS1
 * files, MemorycardFile, Cell<MemorycardFile>): the class's inline members
 * (game/menu/memorycard/T108_memorycard_file.h, in reverse declaration order), its typeinfo
 * function, its implicit destructor.  The markers below follow that order,
 * which is retail's. */

// FUNC 0x00146AD8 gt2k_menu::List<gt2k_menu::MemorycardFile>::~List (_$_Q29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile)
/* inline in game/menu/framework/menu_list.h; egcs emits this copy: the vptr 0x0031B7B0,
 * then ~pList with the flags */
/* implicit: ~pList (base destructor) */

// FUNC 0x00146B00 gt2k_menu::Cell<gt2k_menu::MemorycardFile>::~Cell (_$_Q29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile)
/* inline in game/menu/framework/menu_list.h; egcs emits this copy: the vptr 0x0031BF98,
 * then ~pCell with the flags */
/* implicit: ~pCell (base destructor) */

// FUNC 0x00146B28 gt2k_menu::MemorycardFileList::setUnk24
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy (no callers) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00146B30 __tfQ29gt2k_menu18MemorycardFileList
extern "C" void *__tfQ29gt2k_menu18MemorycardFileList(void)
{
    if (__tiQ29gt2k_menu18MemorycardFileList[0] == 0) {
        __tfQ29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile();
        __rtti_si(__tiQ29gt2k_menu18MemorycardFileList, "Q29gt2k_menu18MemorycardFileList",
                  (const type_info *)__tiQ29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile);
    }
    return __tiQ29gt2k_menu18MemorycardFileList;
}

// FUNC 0x00146B80 __tfQ29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile
extern "C" void *__tfQ29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile(void)
{
    if (__tiQ29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile[0] == 0) {
        __tfQ29gt2k_menu5pList();
        __rtti_si(__tiQ29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile,
                  "Q29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile",
                  (const type_info *)__tiQ29gt2k_menu5pList);
    }
    return __tiQ29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile;
}
#endif

namespace gt2k_menu {

/* The clones of the GT3 kinds (this one and the three below). */
// FUNC 0x00146BD0 gt2k_menu::MemorycardSettingFile::clone
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */
/* operator new(0x298); the implicit copy constructor: MemorycardGT3File's
 * (MemorycardPS2File's copy constructor 0x001437A8, the MemorycardGT3File
 * vptr, the thumbnail and thumbnailValid), then the kind's vptr;
 * operator delete if the copy throws */
/* implicit: operator new, MemorycardPS2File, operator delete (new-expression, copy constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00146C98 __tfQ29gt2k_menu21MemorycardSettingFile
extern "C" void *__tfQ29gt2k_menu21MemorycardSettingFile(void)
{
    if (__tiQ29gt2k_menu21MemorycardSettingFile[0] == 0) {
        __tfQ29gt2k_menu17MemorycardGT3File();
        __rtti_si(__tiQ29gt2k_menu21MemorycardSettingFile, "Q29gt2k_menu21MemorycardSettingFile",
                  (const type_info *)__tiQ29gt2k_menu17MemorycardGT3File);
    }
    return __tiQ29gt2k_menu21MemorycardSettingFile;
}
#endif

namespace gt2k_menu {

// FUNC 0x00146CE8 gt2k_menu::MemorycardSettingFile::~MemorycardSettingFile
/* implicit (game/menu/memorycard/T108_memorycard_file.h); egcs emits this copy:
 * ~MemorycardGT3File(this, flags), which does the operator delete */
/* implicit: ~MemorycardGT3File (base destructor) */

// FUNC 0x00146D08 gt2k_menu::MemorycardBestlapReplayFile::clone
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */
/* as MemorycardSettingFile::clone: operator new(0x298), MemorycardPS2File's
 * copy constructor, operator delete if the copy throws */
/* implicit: operator new, MemorycardPS2File, operator delete (new-expression, copy constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00146DD0 __tfQ29gt2k_menu27MemorycardBestlapReplayFile
extern "C" void *__tfQ29gt2k_menu27MemorycardBestlapReplayFile(void)
{
    if (__tiQ29gt2k_menu27MemorycardBestlapReplayFile[0] == 0) {
        __tfQ29gt2k_menu17MemorycardGT3File();
        __rtti_si(__tiQ29gt2k_menu27MemorycardBestlapReplayFile,
                  "Q29gt2k_menu27MemorycardBestlapReplayFile",
                  (const type_info *)__tiQ29gt2k_menu17MemorycardGT3File);
    }
    return __tiQ29gt2k_menu27MemorycardBestlapReplayFile;
}
#endif

namespace gt2k_menu {

// FUNC 0x00146E20 gt2k_menu::MemorycardBestlapReplayFile::~MemorycardBestlapReplayFile
/* implicit (game/menu/memorycard/T108_memorycard_file.h); egcs emits this copy:
 * ~MemorycardGT3File(this, flags) */
/* implicit: ~MemorycardGT3File (base destructor) */

// FUNC 0x00146E40 gt2k_menu::MemorycardReplayFile::clone
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */
/* as MemorycardSettingFile::clone: operator new(0x298), MemorycardPS2File's
 * copy constructor, operator delete if the copy throws */
/* implicit: operator new, MemorycardPS2File, operator delete (new-expression, copy constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00146F08 __tfQ29gt2k_menu20MemorycardReplayFile
extern "C" void *__tfQ29gt2k_menu20MemorycardReplayFile(void)
{
    if (__tiQ29gt2k_menu20MemorycardReplayFile[0] == 0) {
        __tfQ29gt2k_menu17MemorycardGT3File();
        __rtti_si(__tiQ29gt2k_menu20MemorycardReplayFile, "Q29gt2k_menu20MemorycardReplayFile",
                  (const type_info *)__tiQ29gt2k_menu17MemorycardGT3File);
    }
    return __tiQ29gt2k_menu20MemorycardReplayFile;
}
#endif

namespace gt2k_menu {

// FUNC 0x00146F58 gt2k_menu::MemorycardReplayFile::~MemorycardReplayFile
/* implicit (game/menu/memorycard/T108_memorycard_file.h); egcs emits this copy:
 * ~MemorycardGT3File(this, flags) */
/* implicit: ~MemorycardGT3File (base destructor) */

// FUNC 0x00146F78 gt2k_menu::MemorycardGameFile::clone
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */
/* as MemorycardSettingFile::clone: operator new(0x298), MemorycardPS2File's
 * copy constructor, operator delete if the copy throws */
/* implicit: operator new, MemorycardPS2File, operator delete (new-expression, copy constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00147040 __tfQ29gt2k_menu18MemorycardGameFile
extern "C" void *__tfQ29gt2k_menu18MemorycardGameFile(void)
{
    if (__tiQ29gt2k_menu18MemorycardGameFile[0] == 0) {
        __tfQ29gt2k_menu17MemorycardGT3File();
        __rtti_si(__tiQ29gt2k_menu18MemorycardGameFile, "Q29gt2k_menu18MemorycardGameFile",
                  (const type_info *)__tiQ29gt2k_menu17MemorycardGT3File);
    }
    return __tiQ29gt2k_menu18MemorycardGameFile;
}
#endif

namespace gt2k_menu {

// FUNC 0x00147090 gt2k_menu::MemorycardGameFile::~MemorycardGameFile
/* implicit (game/menu/memorycard/T108_memorycard_file.h); egcs emits this copy:
 * ~MemorycardGT3File(this, flags) */
/* implicit: ~MemorycardGT3File (base destructor) */

// FUNC 0x001470B0 gt2k_menu::MemorycardGT3File::setThumbnail
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy (no callers):
 * the 0x60 bytes copied 32 at a time */

// FUNC 0x001470F8 gt2k_menu::MemorycardGT3File::getThumbnail
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy (no callers) */

/* Both the information and the thumbnail have been read. */
// FUNC 0x00147100 gt2k_menu::MemorycardGT3File::infoValid
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00147120 __tfQ29gt2k_menu17MemorycardGT3File
extern "C" void *__tfQ29gt2k_menu17MemorycardGT3File(void)
{
    if (__tiQ29gt2k_menu17MemorycardGT3File[0] == 0) {
        __tfQ29gt2k_menu17MemorycardPS2File();
        __rtti_si(__tiQ29gt2k_menu17MemorycardGT3File, "Q29gt2k_menu17MemorycardGT3File",
                  (const type_info *)__tiQ29gt2k_menu17MemorycardPS2File);
    }
    return __tiQ29gt2k_menu17MemorycardGT3File;
}

// FUNC 0x00147170 __tfQ29gt2k_menu21MemorycardDeletedFile
extern "C" void *__tfQ29gt2k_menu21MemorycardDeletedFile(void)
{
    if (__tiQ29gt2k_menu21MemorycardDeletedFile[0] == 0) {
        __tfQ29gt2k_menu19MemorycardDummyFile();
        __rtti_si(__tiQ29gt2k_menu21MemorycardDeletedFile, "Q29gt2k_menu21MemorycardDeletedFile",
                  (const type_info *)__tiQ29gt2k_menu19MemorycardDummyFile);
    }
    return __tiQ29gt2k_menu21MemorycardDeletedFile;
}
#endif

namespace gt2k_menu {

// FUNC 0x001471C0 gt2k_menu::MemorycardDeletedFile::~MemorycardDeletedFile
/* implicit (game/menu/memorycard/T108_memorycard_file.h); egcs emits this copy:
 * ~MemorycardFile(this, flags) directly (the dummy file's implicit
 * destructor expanded) */
/* implicit: ~MemorycardFile (base destructor) */

// FUNC 0x001471E0 gt2k_menu::MemorycardDummyFile::readInfo
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001471E8 gt2k_menu::MemorycardDummyFile::getTitleFirstLineBytes
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

/* Also the clone of MemorycardDeletedFile (a copy of it is a dummy file). */
// FUNC 0x001471F0 gt2k_menu::MemorycardDummyFile::clone
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */
/* operator new(0x68), MemorycardFile's copy constructor (0x00142A88), the
 * dummy vptr; operator delete if the copy throws */
/* implicit: operator new, MemorycardFile, operator delete (new-expression, copy constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00147268 __tfQ29gt2k_menu19MemorycardDummyFile
extern "C" void *__tfQ29gt2k_menu19MemorycardDummyFile(void)
{
    if (__tiQ29gt2k_menu19MemorycardDummyFile[0] == 0) {
        __tfQ29gt2k_menu14MemorycardFile();
        __rtti_si(__tiQ29gt2k_menu19MemorycardDummyFile, "Q29gt2k_menu19MemorycardDummyFile",
                  (const type_info *)__tiQ29gt2k_menu14MemorycardFile);
    }
    return __tiQ29gt2k_menu19MemorycardDummyFile;
}
#endif

namespace gt2k_menu {

// FUNC 0x001472B8 gt2k_menu::MemorycardDummyFile::~MemorycardDummyFile
/* implicit (game/menu/memorycard/T108_memorycard_file.h); egcs emits this copy:
 * ~MemorycardFile(this, flags) */
/* implicit: ~MemorycardFile (base destructor) */

// FUNC 0x001472D8 gt2k_menu::MemorycardDamageFile::readInfo
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001472E8 gt2k_menu::MemorycardDamageFile::getTitleFirstLineBytes
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001472F0 gt2k_menu::MemorycardDamageFile::clone
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */
/* operator new(0x68), MemorycardFile's copy constructor, the damage vptr;
 * operator delete if the copy throws */
/* implicit: operator new, MemorycardFile, operator delete (new-expression, copy constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00147368 __tfQ29gt2k_menu20MemorycardDamageFile
extern "C" void *__tfQ29gt2k_menu20MemorycardDamageFile(void)
{
    if (__tiQ29gt2k_menu20MemorycardDamageFile[0] == 0) {
        __tfQ29gt2k_menu14MemorycardFile();
        __rtti_si(__tiQ29gt2k_menu20MemorycardDamageFile, "Q29gt2k_menu20MemorycardDamageFile",
                  (const type_info *)__tiQ29gt2k_menu14MemorycardFile);
    }
    return __tiQ29gt2k_menu20MemorycardDamageFile;
}
#endif

namespace gt2k_menu {

// FUNC 0x001473B8 gt2k_menu::MemorycardDamageFile::~MemorycardDamageFile
/* implicit (game/menu/memorycard/T108_memorycard_file.h); egcs emits this copy:
 * ~MemorycardFile(this, flags) */
/* implicit: ~MemorycardFile (base destructor) */

// FUNC 0x001473D8 gt2k_menu::MemorycardOtherPS2File::isAllocIconMemory
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001473E8 gt2k_menu::MemorycardOtherPS2File::isPrepareIcon
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001473F0 gt2k_menu::MemorycardOtherPS2File::clone
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */
/* operator new(0x238) and the out-of-line copy constructor (0x00143F40,
 * which shares the icon); operator delete if the copy throws */
/* implicit: operator new, MemorycardOtherPS2File, operator delete (new-expression, copy constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00147458 __tfQ29gt2k_menu22MemorycardOtherPS2File
extern "C" void *__tfQ29gt2k_menu22MemorycardOtherPS2File(void)
{
    if (__tiQ29gt2k_menu22MemorycardOtherPS2File[0] == 0) {
        __tfQ29gt2k_menu17MemorycardPS2File();
        __rtti_si(__tiQ29gt2k_menu22MemorycardOtherPS2File, "Q29gt2k_menu22MemorycardOtherPS2File",
                  (const type_info *)__tiQ29gt2k_menu17MemorycardPS2File);
    }
    return __tiQ29gt2k_menu22MemorycardOtherPS2File;
}
#endif

namespace gt2k_menu {

/* The icon.sys title. */
// FUNC 0x001474A8 gt2k_menu::MemorycardPS2File::getTitle
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x001474B0 __tfQ29gt2k_menu17MemorycardPS2File
extern "C" void *__tfQ29gt2k_menu17MemorycardPS2File(void)
{
    if (__tiQ29gt2k_menu17MemorycardPS2File[0] == 0) {
        __tfQ29gt2k_menu14MemorycardFile();
        __rtti_si(__tiQ29gt2k_menu17MemorycardPS2File, "Q29gt2k_menu17MemorycardPS2File",
                  (const type_info *)__tiQ29gt2k_menu14MemorycardFile);
    }
    return __tiQ29gt2k_menu17MemorycardPS2File;
}
#endif

namespace gt2k_menu {

// FUNC 0x00147500 gt2k_menu::MemorycardPS2File::~MemorycardPS2File
/* implicit (game/menu/memorycard/T108_memorycard_file.h); egcs emits this copy:
 * ~MemorycardFile(this, flags) */
/* implicit: ~MemorycardFile (base destructor) */

// FUNC 0x00147520 gt2k_menu::MemorycardPS1File::getTitleFirstLineBytes
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

/* The title in the PS1 save header (+0x74). */
// FUNC 0x00147528 gt2k_menu::MemorycardPS1File::getTitle
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147530 gt2k_menu::MemorycardPS1File::clone
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */
/* operator new(0x270), MemorycardFile's copy constructor (0x00142A88), the
 * PS1 vptr, the 0x200-byte header; operator delete if the copy throws */
/* implicit: operator new, MemorycardFile, operator delete (new-expression, copy constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00147650 __tfQ29gt2k_menu17MemorycardPS1File
extern "C" void *__tfQ29gt2k_menu17MemorycardPS1File(void)
{
    if (__tiQ29gt2k_menu17MemorycardPS1File[0] == 0) {
        __tfQ29gt2k_menu14MemorycardFile();
        __rtti_si(__tiQ29gt2k_menu17MemorycardPS1File, "Q29gt2k_menu17MemorycardPS1File",
                  (const type_info *)__tiQ29gt2k_menu14MemorycardFile);
    }
    return __tiQ29gt2k_menu17MemorycardPS1File;
}
#endif

namespace gt2k_menu {

// FUNC 0x001476A0 gt2k_menu::MemorycardPS1File::~MemorycardPS1File
/* implicit (game/menu/memorycard/T108_memorycard_file.h); egcs emits this copy:
 * ~MemorycardFile(this, flags) */
/* implicit: ~MemorycardFile (base destructor) */

/* ======================================================================== */
/* MemorycardFile: the copies of the small members                          */
/* ======================================================================== */

// FUNC 0x001476C0 gt2k_menu::MemorycardFile::retry
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy (no callers;
 * Lupo 2: allowError) */

/* (index: MemorycardDeletedFile::virtual_17; the base slot, shared by
 * every non-GT3 vtable.) */
// FUNC 0x001476D8 gt2k_menu::MemorycardFile::loadFromVolume
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

/* (index: MemorycardDeletedFile::virtual_16.) */
// FUNC 0x001476E0 gt2k_menu::MemorycardFile::load
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001476E8 gt2k_menu::MemorycardFile::save
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001476F0 gt2k_menu::MemorycardFile::render
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001476F8 gt2k_menu::MemorycardFile::releaseIcon
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147700 gt2k_menu::MemorycardFile::readIcon
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147708 gt2k_menu::MemorycardFile::isAllocIconMemory
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147710 gt2k_menu::MemorycardFile::isPrepareIcon
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147718 gt2k_menu::MemorycardFile::isSelectable
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147720 gt2k_menu::MemorycardFile::isNotReading
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147738 gt2k_menu::MemorycardFile::isNewFile
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147748 gt2k_menu::MemorycardFile::infoValid
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147750 gt2k_menu::MemorycardFile::isSizeInvalid
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147768 gt2k_menu::MemorycardFile::setSizeInvalid
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147778 gt2k_menu::MemorycardFile::isSizeError
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x00147790 gt2k_menu::MemorycardFile::setSizeError
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001477A0 gt2k_menu::MemorycardFile::invalidateSize
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001477B0 gt2k_menu::MemorycardFile::getSize
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001477B8 gt2k_menu::MemorycardFile::getDir
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001477C0 gt2k_menu::MemorycardFile::getId
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

// FUNC 0x001477C8 gt2k_menu::MemorycardFile::prepareRelease
/* inline in game/menu/memorycard/T108_memorycard_file.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x001477D0 __tfQ29gt2k_menu14MemorycardFile
extern "C" void *__tfQ29gt2k_menu14MemorycardFile(void)
{
    if (__tiQ29gt2k_menu14MemorycardFile[0] == 0) {
        __tfQ29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile();
        __rtti_si(__tiQ29gt2k_menu14MemorycardFile, "Q29gt2k_menu14MemorycardFile",
                  (const type_info *)__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile);
    }
    return __tiQ29gt2k_menu14MemorycardFile;
}

// FUNC 0x00147820 __tfQ29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile
extern "C" void *__tfQ29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile(void)
{
    if (__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile[0] == 0) {
        __tfQ29gt2k_menu5pCell();
        __rtti_si(__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile,
                  "Q29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile;
}
#endif
