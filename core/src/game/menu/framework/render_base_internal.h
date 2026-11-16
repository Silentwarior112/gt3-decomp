/* render_base_internal.h - unit u071 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001CB3F0..0x001CE5D8, second pass.
 *
 * Translation units (link order):
 *   T246  gt2k_menu::RenderBase: the GameObject a menu controller hands to the
 *         render thread (root object tree, magnify spring, mask, transition)
 *   T247  gt2k_menu::ControlBase: the two MenuControls of a menu controller,
 *         idle-frame counter / timeout, debug pause
 *   T248  a menu-wide "signal" flag (set by an operator script, polled by the
 *         PageManager)
 *   T249  gt2k_menu::MpegImage: an Item that shows the background movie frame
 *         or a decoded IPU picture (Ipic)
 *   T250  gt2k_menu::Ipic / IpicArchive: IPU pictures and their archive
 *   T251  gt2k_menu::SmartIndexT<int>: the out-of-line members of the template
 *   T252  gt2k_menu::Timer: an Object that calls a function after n frames
 *   T253  MenuBgMovie, the menu background movie (g_002CDC10, an mpegif stream
 *         uploaded to a GSBuffer every frame)
 *   T254  gt2k_menu::Blur2D: a Transitive that blurs the screen buffer
 *   T255  gt2k_menu::Cross: a cross-hair / window mask Item on springs
 *
 * Every class is defined in its canonical header (include/game/T246..T255).
 * Each source includes the canonical headers it needs itself, in the order
 * that gives the original's type names (data pass: the type names in an
 * object's .rodata are those of the classes the file saw, in reverse
 * definition order; each source's comment lists retail's).  This header
 * adds only what no class header has and defines no class egcs writes a
 * type name for: named constants, the T250 archive records and (GCC 14
 * only) the type_info storage of the unit's own classes.
 */
#ifndef U071_H
#define U071_H

#include "types.h"
#include "game/game.h"
#include <stddef.h>

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */
/* The menus' virtual screen: T259's MENU_SCREEN_W / MENU_SCREEN_H. */
static const int MENU_FRAMES_PER_SECOND = 60;

/* T247: the screen saver comes on after 30 idle minutes. */
static const int SCREEN_SAVER_FRAMES = 30 * 60 * MENU_FRAMES_PER_SECOND;   /* 108000 */
/* T247: the PadManager slots of the two menu pads: steering[0] / [1] (pad 1
 * or wheel 1, pad 5 or wheel 2: T475). */
static const u32 PAD_SLOT_MENU1 = 20;
static const u32 PAD_SLOT_MENU2 = 21;

/* T249 / T253: the decoded pictures and movie frames are 16 x 16 blocks of
 * 32-bit pixels (0x400 bytes each), uploaded block by block into a GSBuffer
 * of PSMCT32 (libgraph SCE_GS_PSMCT32) whose buffer width is the width
 * rounded up to 64 pixels. */
static const int PSM_CT32         = 0;
static const int PIXEL_BLOCK      = 16;
static const int PIXEL_BLOCK_SIZE = PIXEL_BLOCK * PIXEL_BLOCK * 4;
static inline int gs_buffer_width(int w) { return (w + 0x3F) & ~0x3F; }

/* T253: the presentation times of the menu movies, in microseconds: frame n
 * of a 29.97 Hz movie (59.933 fields a second) starts at n * 2000000000 /
 * 59933; MOVIE_HALF_FRAME (16685 us) is half a frame. */
static const s64 MOVIE_TIME_SCALE = 2000000000LL;
static const s64 MOVIE_FIELD_RATE = 59933;
static const s64 MOVIE_HALF_FRAME = 0x412D;
static const s64 MOVIE_TIME_MAX   = 0x7FFFFFFFFFFFFFFFLL;  /* (0, MAX): no window */
/* The size of the stream's preload buffer (mpegif::preload). */
static const int MOVIE_PRELOAD_SIZE = 0x500000;

/* T255: the screen position wraps modulo the screen plus a 20-pixel margin
 * on each side (680 x 488); the springs pull with 600 per pixel of distance
 * and are damped by 30. */
static const f32 CROSS_MARGIN  = 20.0f;
static const f32 CROSS_WRAP_W  = 680.0f;
static const f32 CROSS_WRAP_H  = 488.0f;
static const f32 CROSS_SPRING  = 600.0f;
static const f32 CROSS_DAMPING = 30.0f;

/* (The file-local data is defined in its file: T246's .sdata g_00352700 /
 * g_00352704, T248's .sbss g_0035324C, T254's .rodata g_003372B0.) */

/* ======================================================================== */
/* T250 - the archive records (forward-declared in T250_ipic.h)              */
/* ======================================================================== */
namespace gt2k_menu {

/* A picture's header: G002's IpuImageHeader (the sweep deleted the local
 * IpicHeader, the same 17 packed bytes). */

/* One entry of an IpicArchive, 0x20 bytes. */
struct IpicEntry {
    u32  offset;                            /* 0x00 of the IpuImageHeader, from the archive start */
    char name[0x1C];                        /* 0x04 */
};

/* The archive file. */
struct IpicArchiveData {
    u32       unk0;                         /* 0x00 (not read) */
    int       count;                        /* 0x04 */
    IpicEntry entries[1];                   /* 0x08 count of them, then the pictures */
};

} /* namespace gt2k_menu */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si, __rtti_user */

/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The type_info storage of the classes only this unit builds
 * (__user_type_info = 2 words, __si_type_info = 3).  (RenderBase's and
 * ControlBase's are in T246 / T247, Item's, Transitive's, Object's and
 * GameObject's in T213 / T216 / T220 / T002.) */
extern "C" {
extern u32 __tiQ29gt2k_menu9MpegImage[3];   /* 0x004B2670 */
extern u32 __tiQ29gt2k_menu11IpicArchive[2]; /* 0x00353408 gp-0x45E8 */
extern u32 __tiQ29gt2k_menu4Ipic[2];        /* 0x00353400 gp-0x45F0 */
extern u32 __tiQ29gt2k_menu5Timer[3];       /* 0x004B2680 */
extern u32 __tiQ29gt2k_menu6Blur2D[3];      /* 0x004B2690 */
extern u32 __tiQ29gt2k_menu5Cross[3];       /* 0x004B26A0 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U071_H */
