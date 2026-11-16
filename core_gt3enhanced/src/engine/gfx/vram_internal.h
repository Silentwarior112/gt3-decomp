/* vram_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u112,
 * 0x00243850..0x00245C10: translation units T513 .. T518.
 *
 *   T513  VramAllocator (? name): the GS VRAM allocator g_002F4250 (a member
 *         of the render context g_002F4230, +0x20).  1M words of VRAM cut
 *         into page-aligned (0x800-word) blocks; freed blocks keep their image
 *         on a "released" list (LRU) so that a texture asked for again by the
 *         same tag is found without an upload; they are given back to the
 *         free list (merged with their neighbours) only when space runs out
 *         or on unlock.  A semaphore guards it.
 *   T514  GSBuffer: a GS frame / texture buffer descriptor (address, PSM,
 *         size, field mode, scan mask, draw rectangle), VRAM from g_002F4250.
 *   T515  TexSet (? name): a Tex1 texture set (or one raw image) placed in
 *         VRAM from g_002F4250.
 *   T516  Country (? name): the country / language setting g_00352FD8.
 *   T517  GT2KRealTimeText: named tables of key -> text pointers, filled from
 *         text/realtime.dat for the current language.
 *   T518  UnitSettings: the speed / power / torque units and conversions.
 *
 * Canonical headers (include/game/):
 *   T513_vram_allocator.h .. T518_unit_settings.h   the six translation units
 *   T512_render_context.h  g_002F4230 and g_002F4250 (its .vram member)
 *   G001_flare.h           GsTexRegs, Tex1Image and the graphics library
 *                          (pgl_, gfx_, gs_, texregs_, tex1_ functions)
 *   T503_vol_thread.h      file_read_blocking, vol_device_object
 *   G007_stdio.h           strcmp, strcpy, strcat, memcpy, the semaphore calls
 *   T532_heap.h            malloc, free
 *   G002_mpeg_frameq.h     strlen (the game's MMI strlen, 0x00269A68)
 * eekernel.h gives struct SemaParam, cxxrt.h __rtti_user.
 *
 * Conventions
 *   - Offsets in comments are the ORIGINAL ones (egcs layout: the vptr is
 *     after the data members).  A modern g++ puts the vptr at +0.
 *   - VRAM addresses and sizes are in GS words (32-bit), as GS TBP0 * 64.
 *   - Names marked "?" are guesses.
 * Everything below is local to the unit.
 */
#ifndef U112_H
#define U112_H

#include "types.h"
#include "eekernel.h"                   /* struct SemaParam */
#include "cxxrt.h"                      /* __rtti_user */
#include <stddef.h>
/* No <new>: its inline bad_alloc::what() puts "bad_alloc" into every
 * object's .rodata, and none of the six originals has it (no file uses
 * placement new). */

#include "runtime/newlib/G007_stdio.h"            /* strcmp, strcpy, strcat, memcpy, CreateSema .. WaitSema */
#include "engine/pdistd/T532_heap.h"             /* malloc, free */
#include "engine/media/G002_mpeg_frameq.h"      /* strlen 0x00269A68 */
#include "engine/pgl/G001_flare.h"            /* GsTexRegs, Tex1Image, the graphics library */
#include "engine/io/T503_vol_thread.h"       /* file_read_blocking, vol_device_object */
#include "engine/gfx/T512_render_context.h"   /* g_002F4230, g_002F4250 */
#include "engine/gfx/T513_vram_allocator.h"   /* own: VramAllocator, VramBlock, VramList */
#include "engine/gfx/T514_gs_buffer.h"        /* own: GSBuffer */
#include "engine/gfx/T515_tex_set.h"          /* own: TexSet */
#include "game/data/T516_country.h"          /* own: Country, g_00352FD8 */
#include "engine/text/T517_realtime_text.h"    /* own: GT2KRealTimeText */
#include "game/data/T518_unit_settings.h"    /* own: UnitSettings, g_002F59D8 */

/* ======================================================================== */
/* Graphics library constants                                              */
/* ======================================================================== */

/* The values G001's functions take (PGL_LIST_*, PGL_MARK_*, PGL_CAP_*, the
 * gfx_tex_param / texregs_set_mode pnames PGL_TEXPARAM_* and values,
 * PGL_REGION_*, PGL_ATTR_*, the gfx_set_field_mode modes PGL_FIELD_* (what
 * GSBuffer::field holds; gs_get_field_mode returns FRAME, HALF_EVEN,
 * HALF_ODD or FRAME_SHIFT), the blend modes): G001_flare.h. */

/* What GSBuffer::blit / blitField save (0x887). */
#define PGL_ATTRIB_BLIT (PGL_ATTR_SCISSOR | PGL_ATTR_VIEWPORT | PGL_ATTR_TARGET_B | \
                         PGL_ATTR_DEPTH | PGL_ATTR_DRAW_BUFFER)

/* pgl_color: white, GS alpha 0x80 (1.0). */
#define GS_RGBA_WHITE 0x80FFFFFF

/* ======================================================================== */
/* T513                                                                     */
/* ======================================================================== */

#define VRAM_WORDS      0x100000        /* 4 MB of GS local memory */
#define VRAM_PAGE_WORDS 0x800           /* a GS page (8 KB): the allocation unit */
#define VRAM_BLOCK_SHIFT 6              /* 64-word blocks (TBP0, Tex1Image.vramSize) -> words */

/* ======================================================================== */
/* T517                                                                     */
/* ======================================================================== */

/* An entry of text/realtime.dat's index (sorted by name). */
struct RealTimeEntry {
    const char *name;                       /* 0x0 "<table>_<key>" (an offset in the file) */
    int         text;                       /* 0x4 offset of the text in a language block */
};
/* text/realtime.dat, as loaded. */
struct RealTimeData {
    u8            pad_00[8];
    char         *texts;                    /* 0x08 the language blocks (an offset in the file) */
    u32           count;                    /* 0x0C entries */
    int           langSize;                 /* 0x10 bytes per language block */
    RealTimeEntry entries[1];               /* 0x14 [count] */
};
/* T517's .sdata (defined there). */
extern RealTimeData     *g_realtimeData;    /* 0x00353018 gp-0x49D8 the loaded file, NULL before realtime_load */
extern GT2KRealTimeText *g_realtimeTables;  /* 0x00353024 gp-0x49CC the list of tables (newest first) */

RealTimeEntry *realtime_find(const char *name); /* 0x002454E0 */

#endif /* U112_H */
