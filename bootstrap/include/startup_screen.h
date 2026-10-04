/* startup_screen.h - startup / warning screen of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), unit "startup".
 *
 * Two pieces of code:
 *
 *  1. src/startup_screen.cpp, .text 0x01000A50..0x01000ED7 - its own C++
 *     object file linked right after main.cxx and the IOP/pad object (own
 *     .eh_frame CIE at .data 0x01026F1C, no "eh" augmentation, FDE list
 *     zero-terminated at 0x01027030; the main unit's src/crtbegin.c lists it
 *     as a separate frame_table entry).  At boot main calls
 *     startup_screen_start: it inflates the embedded gzip 'startupa.img' into a
 *     0x40000-byte heap buffer, turns it into a live Tex1 texture set, uploads it
 *     to GS VRAM and starts a render thread (priority 2; main drops itself to 3)
 *     that fades texture 0 in and out (151 frames at gfx_vsync(2) pacing).
 *     The images shown come from the negative-terminated index list at
 *     .sdata 0x01041DD0 = {0, -1} (the mapping pass called it a duration
 *     table).  On exit the thread shows a black frame and draws texture 2
 *     (640x57) into the back buffer (word 0x50000), which the bootstrap never
 *     displays.  main waits for the thread to end (startup_screen_wait_thread,
 *     so the screen always runs its full length), then startup_screen_stop.
 *
 *  2. src/tex1.cpp, .text 0x0100B2E8..0x0100BE0F - a small plain-C 'Tex1'
 *     texture-set library (no EH data): pointer relocation, VRAM rebasing,
 *     upload over GIF DMA, and a keyframed CLUT (palette) animation engine
 *     whose step/release/apply-all/reset-all entry points are never called in
 *     this binary (the startupa.img set has no animation table anyway).
 *     The Lupo 2 symbols name a Polyphony type "PGLUtexSet" used for texture
 *     files; the 'Tex1' set is probably that type (? name not proven here).
 *
 * Nothing in this unit touches core.gt3, hashing or signatures.
 */
#ifndef BOOT_STARTUP_SCREEN_H
#define BOOT_STARTUP_SCREEN_H

#include "types.h"
#include "gs_driver.h"      /* GsTexRegs */

/* ======================================================================== */
/* Tex1 texture set (in-memory layout of the inflated startupa.img)          */
/* ======================================================================== */

/* One keyframe of a CLUT animation channel (8 bytes). */
typedef struct ClutKey {
    f32  time;              /* +0x00 duration of the key; |time| is used for the
                             *       timeline, a negative value makes the channel
                             *       hold on this key (hold counter, see
                             *       clut_anim_step / clut_anim_release)          */
    u32 *palette;           /* +0x04 nColors RGBA32 colours (relocated)           */
} ClutKey;

/* One CLUT animation channel (0x20 bytes).  The palette written to `dest` is
 * the per-byte blend  keys[cur].palette * lerp + keys[next].palette * (1-lerp)
 * in 4.12 fixed point (clut_anim_channel_apply).                            */
typedef struct ClutAnimChannel {
    u16      id;            /* +0x00 matched against clut_anim_step/release's id  */
    u16      mode;          /* +0x02 0 loop, 1 ping-pong, 2 play once, else hold  */
    u16      nColors;       /* +0x04 colours per palette                          */
    u16      nKeys;         /* +0x06                                              */
    u32     *dest;          /* +0x08 destination CLUT colours (relocated)         */
    ClutKey *keys;          /* +0x0C nKeys keyframes (relocated)                  */
    u16      cur;           /* +0x10 current key                                  */
    u16      next;          /* +0x12 key being blended towards                    */
    f32      time;          /* +0x14 time into the current key                    */
    f32      lerp;          /* +0x18 weight of keys[cur] (1.0 = exactly cur)      */
    s16      hold;          /* +0x1C > 0: channel frozen (negative-time key)      */
    s8       dir;           /* +0x1E ping-pong direction, +1 / -1                 */
    u8       dirty;         /* +0x1F dest needs recomputing                       */
} ClutAnimChannel;

/* Tex1Image.clutAnim target. */
typedef struct ClutAnimTable {
    u16              count; /* +0x00 channels                                     */
    u16              pad02;
    ClutAnimChannel *ch;    /* +0x04 (relocated)                                  */
} ClutAnimTable;

/* One VRAM upload (0x0C bytes): a host->local IMAGE transfer of w x h pixels
 * of DPSM to (DBP, DBW) at (0, 0).  The word at +4 (dbp, dbw, dpsm) is passed
 * as-is, through a u32 pointer to &dbp, as the upper half of BITBLTBUF to
 * gs_build_image_transfer_packet.  (No anonymous struct: the egcs C++ front
 * end rejects one that declares no object.)                                  */
typedef struct Tex1Xfer {
    void *data;             /* +0x00 pixel data (relocated; file: offset)         */
    u16   dbp;              /* +0x04 DBP, 64-word blocks (rebased, 16-bit wrap);
                             *       +0x04..+0x07 = BITBLTBUF bits 63..32         */
    u8    dbw;              /* +0x06 DBW                                          */
    u8    dpsm;             /* +0x07 DPSM                                         */
    u16   w;                /* +0x08 */
    u16   h;                /* +0x0A */
} Tex1Xfer;

/* 'Tex1' texture set header.  In the file every pointer is an offset from the
 * start (self = 0) and VRAM addresses are relative to block 0 (vramBase = 0).
 * startupa.img: size 0x38020, vramSize 0x37F, 4 GsTexRegs @0x30 (PSMT4 with
 * CT32 CLUTs at CBP 5, CSA 0/2/4/6: 640x448, 197x219, 640x57, 524x40 by
 * CLAMP), 6 transfers @0xD0 (64x864 + five small CT32 blocks), no pointer
 * table, no CLUT animation.                                                 */
typedef struct Tex1Image {
    u32            magic;       /* +0x00 'Tex1' (0x31786554; never checked)      */
    struct Tex1Image *self;     /* +0x04 address the pointers are relative to    */
    void          *ptr08;       /* +0x08 relocated if non-NULL (? unknown use)   */
    u32            size;        /* +0x0C total size (? not read by the code)     */
    u16            vramBase;    /* +0x10 VRAM base the set is linked for (64-word blocks) */
    u16            vramSize;    /* +0x12 VRAM footprint in blocks (? not read)   */
    u16            nTex;        /* +0x14 GsTexRegs entries                       */
    u16            nXfer;       /* +0x16 Tex1Xfer entries                        */
    GsTexRegs     *tex;         /* +0x18 per-texture GS register blocks          */
    Tex1Xfer      *xfer;        /* +0x1C upload list                             */
    u32           *ptrTable;    /* +0x20 {count, ptr[count]}: non-NULL entries
                                 *       relocated (? unknown use)               */
    ClutAnimTable *clutAnim;    /* +0x24                                         */
    u32            pad28[2];    /* +0x28 zero in startupa.img; data starts at 0x30 */
} Tex1Image;

#ifdef __cplusplus
static_assert(sizeof(ClutKey) == 0x08, "ClutKey");
static_assert(sizeof(ClutAnimChannel) == 0x20, "ClutAnimChannel");
static_assert(sizeof(ClutAnimTable) == 0x08, "ClutAnimTable");
static_assert(sizeof(Tex1Xfer) == 0x0C, "Tex1Xfer");
static_assert(sizeof(Tex1Image) == 0x30, "Tex1Image");
static_assert(sizeof(GsTexRegs) == 0x28, "GsTexRegs stride of Tex1Image.tex");
#endif

/* ======================================================================== */
/* Tex1 library (src/tex1.cpp) - plain C, C linkage                          */
/* ======================================================================== */

#ifdef __cplusplus
extern "C" {
#endif

Tex1Image *tex1_set_vram_base(Tex1Image *img, u32 vramWordAddr);   /* 0x0100B2E8 */
Tex1Image *tex1_relocate(Tex1Image *img);                           /* 0x0100B560 */
void clut_anim_channel_reset(ClutAnimChannel *ch);                  /* 0x0100B680 (was clut_fade_channel_reset) */
void clut_anim_channel_apply(ClutAnimChannel *ch);                  /* 0x0100B6D0 (was clut_fade_apply) */
void clut_anim_apply_all(Tex1Image *img);                           /* 0x0100B798 (was clut_fade_update_all; dead) */
void clut_anim_step(Tex1Image *img, int id, float dt);              /* 0x0100B808 (was F_0100B808; dead) */
void clut_anim_release(Tex1Image *img, int id);                     /* 0x0100BB28 (was F_0100BB28; dead) */
void clut_anim_reset_all(Tex1Image *img);                           /* 0x0100BBA0 (was clut_fade_finish_all; dead) */
void clut_anim_table_relocate(ClutAnimTable *tbl, s32 delta);       /* 0x0100BC40 (was clut_fade_table_relocate) */
void tex1_upload_to_vram(Tex1Image *img);                           /* 0x0100BD38 */

#ifdef __cplusplus
}
#endif

/* ======================================================================== */
/* Startup screen (src/startup_screen.cpp) - own C++ object, C++ linkage    */
/* ======================================================================== */

#ifdef __cplusplus
void startup_screen_start(void);                                    /* 0x01000A50 */
void startup_screen_draw_frame(u32 fbWordAddr, int image, float alpha); /* 0x01000B40 */
void startup_screen_thread(void *arg);                              /* 0x01000C88 */
void startup_screen_request_stop(void);                             /* 0x01000E70 */
void startup_screen_wait_thread(void);                              /* 0x01000E80 */
void startup_screen_stop(void);                                     /* 0x01000EA8 */

/* Globals (all used only by this unit).  The inflated startupa.img pointer
 * (.sbss 0x01041E88) is a static of startup_screen.cpp.                    */
extern const u8   g_startupaImgGz[];             /* @0x01027F38 .data, 0xBD5B bytes
                                                  * (data/startupa_img.gz, tools/extract_startup_img.py).
                                                  * Link it at least 4-byte aligned (8 in the ELF):
                                                  * GZIP::InflateBase reads 32-bit words and loses
                                                  * header bits when (addr & 3) == 3. */
#endif

#endif /* BOOT_STARTUP_SCREEN_H */
