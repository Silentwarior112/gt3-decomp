/* startup_screen.cpp - startup / warning screen of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), unit "startup", .text 0x01000A50..0x01000ED7.
 *
 * These six functions form their own C++ object file, linked right after
 * main.cxx and the IOP/pad object: one .eh_frame CIE of their own at .data
 * 0x01026F1C (no "eh" augmentation) with one FDE each (pc_begin words
 * 0x01026F38, 0x01026F60, 0x01026FA4, 0x01026FE8, 0x01026FF8, 0x01027018;
 * zero terminator 0x01027030; no landing pads), hence C++ linkage.
 * (Integration correction: earlier drafts called this the tail of main.cxx;
 * main.cxx's own CIE is 0x01026CD8 and ends at 0x01026E40.)
 *
 * Boot flow (main, 0x01000538):
 *   gfx_init_default(2) -> startup_screen_start() -> IOP/IRX setup ->
 *   ImageLoader::load (core.gt3) -> startup_screen_wait_thread() ->
 *   startup_screen_stop() -> IOP shutdown -> gfx_shutdown() -> ExecPS2.
 * main blocks in startup_screen_wait_thread until the render thread has shown
 * the whole image sequence, so the warning screen always runs its full length
 * (one image, 151 frames paced by gfx_vsync(2)); the stop request that
 * startup_screen_stop sends arrives after the thread has already finished.
 *
 * Frame buffers: two 640x448 PSMCT24 buffers at GS word addresses 0 and
 * 0x50000; the inflated 'Tex1' texture set is linked to word 0xA0000.
 *
 * Nothing here touches core.gt3, hashing or signatures.
 */
#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "libgraph.h"
#include "gs_driver.h"
#include "gzip.h"
#include "startup_screen.h"

/* Linker symbol, 0x01049CF0; only its address is used.  A complete small
 * type (not char[]): egcs then leaves `&_gp` to the assembler's `la`, which
 * gives the retail's late, adjacent lui/addiu pair in startup_screen_start. */
extern "C" int _gp;

/* GS local-memory word addresses used by this module. */
#define STARTUP_FB0         0x00000u                /* frame buffer 0            */
#define STARTUP_FB1         0x50000u                /* frame buffer 1            */
#define STARTUP_TEX_VRAM    0xA0000u                /* Tex1 set (block 0x2800)   */

#define STARTUP_SCREEN_W    640
#define STARTUP_SCREEN_H    448

/* Per-image timeline, in frames (each frame waits gfx_vsync(2)). */
#define STARTUP_FADE_FRAMES 15                      /* fade in 0..14, fade out 135..150 */
#define STARTUP_HOLD_END    0x87                    /* alpha 1.0 for frames 15..134     */
#define STARTUP_LAST_FRAME  0x96                    /* 150: alpha (150 - f) / 15        */
#define STARTUP_IMAGE_LEN   0x97                    /* 151 frames per image             */

/* ------------------------------------------------------------------------ */
/* Globals                                                                  */
/* ------------------------------------------------------------------------ */

static int  s_startupDoneSema;                      // @0x01041E80 (.sbss) completion semaphore
/* Quit flag, written by the main thread, read by the render thread.  The
 * retail flag is a plain int: egcs put its store into the delay slot of
 * startup_screen_request_stop's `jr`, which reorg never does for a volatile
 * access, and it is re-read after the calls in the render loop anyway.  The
 * GCC 14 build keeps it volatile so that compiler cannot cache it.          */
#ifdef BOOT_OLD_GCC
static int s_startupStopRequested;                  // @0x01041E84 (.sbss)
#else
static volatile int s_startupStopRequested;         // @0x01041E84 (.sbss)
#endif
/* static: the retail compiler puts an uninitialised C++ global into .sdata
 * as zeros, but this one is in .sbss right after the two above, which is
 * where egcs puts a file-local static.  Only this file uses it.            */
static Tex1Image *g_startupTex;                     // @0x01041E88 (.sbss) inflated startupa.img

/* Image sequence: indices into g_startupTex->tex[], negative-terminated.
 * The mapping pass called this a duration table; the thread passes each entry
 * to startup_screen_draw_frame as the image index.  Only texture 0 (the
 * 640x448 warning screen) is shown. */
static int  s_startupImageSeq[2] = { 0, -1 };       // @0x01041DD0 (.sdata)

static u8   s_startupThreadStack[0x4000] __attribute__((aligned(16)));  // @0x01042180 (.bss)
static u8   s_startupPacketBuf[0x2000]   __attribute__((aligned(16)));  // @0x01046180 (.bss)

/* ------------------------------------------------------------------------ */

// FUNC 0x01000A50 startup_screen_start
// Called by main right after gfx_init_default(2).  Allocates a 0x40000-byte,
// 64-byte-aligned buffer (memalign), inflates the embedded gzip 'startupa.img'
// into it with a stack GZIP::InflateBase (the (out, in) constructor inflates
// the whole file; destroyed at once with the GCC 2.9x flag 2, no delete), then
// relocates the resulting Tex1 set, links it to VRAM word 0xA0000 and uploads
// it.  It then creates the completion semaphore {max 1, init 1} and takes it
// at once (the render thread signals it when it ends), creates the render
// thread (startup_screen_thread; stack 0x4000 @0x01042180, priority 2, gp =
// _gp), clears the stop flag, lowers itself to priority 3 and starts the
// thread (argument NULL).
// Stack: the 0x3060-byte frame holds the InflateBase object at sp+0; the
// SemaParam (sp+0) and ThreadParam (sp+0x20) reuse that storage afterwards,
// and only the fields written below are set - the rest hold inflater leftovers
// (SemaParam attr/option, ThreadParam status/currentPriority/attr/...).
// No NULL check on memalign's result, and none on the inflate (it has no
// error reporting).
// The declarations of sema / tp / tid come after the inflate block: egcs
// frees a block's stack slots at its end, which is how they share the
// inflater's storage at sp+0 in the retail frame.
void startup_screen_start(void)
{
    g_startupTex = (Tex1Image *)memalign(0x40, 0x40000);
    {
        GZIP::InflateBase inflater(g_startupTex, g_startupaImgGz);
    }                                               /* ~InflateBase(2) */
    tex1_relocate(g_startupTex);
    tex1_set_vram_base(g_startupTex, STARTUP_TEX_VRAM);
    tex1_upload_to_vram(g_startupTex);

    struct SemaParam sema;                          /* sp+0x00 */
    struct ThreadParam tp;                          /* sp+0x20 */
    int tid;

    sema.initCount = 1;                             /* sp+0x08 */
    sema.maxCount = 1;                              /* sp+0x04 */
    s_startupDoneSema = CreateSema(&sema);
    WaitSema(s_startupDoneSema);

    tp.entry = startup_screen_thread;               /* sp+0x24 */
    tp.stack = s_startupThreadStack;                /* sp+0x28 */
    tp.stackSize = sizeof(s_startupThreadStack);    /* sp+0x2C */
    tp.initPriority = 2;                            /* sp+0x34 */
    tp.gpReg = &_gp;                                /* sp+0x30 */
    tp.option = 0;                                  /* sp+0x40 */
    tid = CreateThread(&tp);
    s_startupStopRequested = 0;
    ChangeThreadPriority(GetThreadId(), 3);
    StartThread(tid, NULL);
}

// FUNC 0x01000B40 startup_screen_draw_frame
// Draws one complete frame into the frame buffer at GS word fbWordAddr and
// waits for it: packet buffer 0x2000 bytes @0x01046180, 640x448 PSMCT24 draw
// buffer, clear colour 0 (black) and clear the colour buffer, then
// gfx_disable 5 (Z test off: ZTST ALWAYS, ZMSK), 4 (alpha test off) and
// 6 (alpha blend off).  If image >= 0 the texture g_startupTex->tex[image] is
// drawn at its own size (CLAMP.MAXU + 1 x CLAMP.MAXV + 1), centred on the
// 640x448 screen (integer halves, arithmetic shift), modulated by the grey
// level `alpha` (gs_set_bg_color_rgb(a, a, a) - a brightness fade, not
// blending).  Then end packet, kick both chains and gfx_sync_pipeline.
// The image index is not range-checked.  Caller: startup_screen_thread.
void startup_screen_draw_frame(u32 fbWordAddr, int image, float alpha)
{
    gfx_set_packet_buffer(s_startupPacketBuf, sizeof(s_startupPacketBuf));
    gfx_begin_packet();                             /* result unused */
    gfx_set_draw_buffer(fbWordAddr, SCE_GS_PSMCT24, STARTUP_SCREEN_W, STARTUP_SCREEN_H);
    gs_set_clear_color(0);
    gs_clear_buffers(1);                            /* colour only */
    gfx_disable(5);                                 /* depth test  */
    gfx_disable(4);                                 /* alpha test  */
    gfx_disable(6);                                 /* alpha blend */

    if (image >= 0) {
        const GsTexRegs *t = &g_startupTex->tex[image];
        u64 clamp = t->clamp;
        int w = (int)(((clamp >> 14) & 0x3FF) + 1); /* CLAMP.MAXU + 1 (64-bit add) */
        int h = (int)(((clamp >> 34) & 0x3FF) + 1); /* CLAMP.MAXV + 1 */
        int x = (STARTUP_SCREEN_W - w) >> 1;
        int y = (STARTUP_SCREEN_H - h) >> 1;

        gfx_set_texture_regs(t);
        gs_set_bg_color_rgb(alpha, alpha, alpha);
        gs_draw_textured_sprite(x, y, x + w, y + h, 0, 0, w, h);
    }

    gfx_end_packet();
    gfx_kick_frame_lists(NULL);
    gfx_sync_pipeline();
}

// FUNC 0x01000C88 startup_screen_thread
// Render thread (priority 2, started by startup_screen_start; arg unused).
// Display setup: gfx_vsync(1), interlaced NTSC field mode
// (gfx_change_display_mode(1, 2, 0)), 640x448 screen and display area, and
// one more vsync when gs_get_display_state() == 4 (frame mode, odd field).
// Then, unless the stop flag is already set or the sequence is empty, for each
// image index of s_startupImageSeq (just {0}) it runs 151 frames: alpha f/15
// for f = 0..14, 1.0 for f = 15..134, (150 - f)/15 for f = 135..150, drawn
// into alternating buffers (0 first, then 0x50000), gfx_vsync(2), then that
// buffer is displayed.  The stop flag is checked once per frame.
// Exit: a black frame (image -1, alpha 0) into buffer 0, gfx_vsync(1),
// display buffer 0, then image 2 (the 640x57 texture) at full brightness
// into buffer 0x50000 - which nothing in the bootstrap ever displays -
// SignalSema(completion) and ExitDeleteThread (0x0101BA20, syscall 0x24; the
// mapping pass said ExitThread).
// gfx_set_display_fb is called with a 4th argument 448 that it ignores.
void startup_screen_thread(void *arg)
{
    int flip = 0;                                   /* s4: 0 -> buffer 0, 1 -> 0x50000 */
    int seq = 0;                                    /* s3: index into s_startupImageSeq */
    int frame = 0;                                  /* s1: frame within the image       */

    gfx_vsync(1);
    gfx_change_display_mode(1, 2, 0);
    gfx_set_screen_size(STARTUP_SCREEN_W, STARTUP_SCREEN_H);
    gfx_set_display_area(0, 0, STARTUP_SCREEN_W, STARTUP_SCREEN_H);
    if (gs_get_display_state() == 4)
        gfx_vsync(1);

    /* A plain while loop: egcs inverts it into the retail shape (first test
     * with seq == 0 folded, then the body, then the test again at the
     * bottom, whose seq * 4 the next iteration reuses). */
    while (s_startupStopRequested == 0 && s_startupImageSeq[seq] >= 0) {
        float alpha;
        u32 fb;

        if (frame < STARTUP_FADE_FRAMES)
            alpha = (float)frame / 15.0f;
        else if (frame < STARTUP_HOLD_END)
            alpha = 1.0f;
        else
            alpha = (float)(STARTUP_LAST_FRAME - frame) / 15.0f;
        fb = flip ? STARTUP_FB1 : STARTUP_FB0;       /* movz */

        startup_screen_draw_frame(fb, s_startupImageSeq[seq], alpha);
        frame++;
        gfx_vsync(2);
        gfx_set_display_fb(fb, SCE_GS_PSMCT24, STARTUP_SCREEN_W, STARTUP_SCREEN_H);
        flip = 1 - flip;
        if (frame >= STARTUP_IMAGE_LEN) {
            seq++;
            frame = 0;
        }
    }

    startup_screen_draw_frame(STARTUP_FB0, -1, 0.0f);
    gfx_vsync(1);
    gfx_set_display_fb(STARTUP_FB0, SCE_GS_PSMCT24, STARTUP_SCREEN_W, STARTUP_SCREEN_H);
    startup_screen_draw_frame(STARTUP_FB1, 2, 1.0f);
    SignalSema(s_startupDoneSema);
    ExitDeleteThread();
}

// FUNC 0x01000E70 startup_screen_request_stop
// Sets the render thread's stop flag.  ($v0 is left = 1 by the store
// sequence; no caller reads it.)  Caller: startup_screen_stop.
void startup_screen_request_stop(void)
{
    s_startupStopRequested = 1;
}

// FUNC 0x01000E80 startup_screen_wait_thread
// Blocks until the render thread has signalled completion, then re-posts the
// semaphore so it can be waited on again.  Callers: main (directly, after
// ImageLoader::load) and startup_screen_stop.
void startup_screen_wait_thread(void)
{
    WaitSema(s_startupDoneSema);
    SignalSema(s_startupDoneSema);
}

// FUNC 0x01000EA8 startup_screen_stop
// Request stop, wait for the thread to finish, delete the completion
// semaphore.  Caller: main, after startup_screen_wait_thread (so the thread
// has already ended and the stop request has no effect there).
void startup_screen_stop(void)
{
    startup_screen_request_stop();
    startup_screen_wait_thread();
    DeleteSema(s_startupDoneSema);
}
