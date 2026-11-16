/* T002 - the render thread and GameObject.  0x001012E0..0x00101A80.
 *
 * The render thread (priority 7, its own 0x8000-byte stack) draws one
 * GameObject per frame, g_00350B10, which it copies from g_00350B14 (set by
 * render_set_object) at the top of every frame.  It alternates the two
 * packet buffers of g_0028F550 and the two frame buffers of
 * g_002F4230.frame.  The main thread asks it to stop with
 * render_thread_exit (g_00350B1C = its own thread id, then it sleeps until
 * the render thread wakes it and exits). */
#include "game/app/app_internal.h"
#include "eekernel.h"                       /* struct ThreadParam */
#include "cxxrt.h"                          /* type_info, __rtti_user (GCC 14's typeinfo function) */
/* _gp with a size, declared before G000's unsized declaration: egcs then
 * counts it as small data (-G8) and loads &_gp with one `la' (GAS: lui /
 * addiu) that the scheduler keeps in source order, as retail's
 * render_thread_start has it; unsized, it is split into %hi / %lo insns
 * scheduled with the other addresses. */
extern "C" char _gp[4];                     /* 0x003579F0 */
#include "runtime/startup/G000_collect2_ctors.h"       /* _gp (the render thread's gpReg) */
#include "engine/pgl/G001_flare.h"                /* gfx_* / gs_* / pgl_* (the GS driver), PGL_* enums */
#include "engine/media/G002_mpeg_frameq.h"          /* create_mutex_sema, signal_sema_helper */
#include "runtime/newlib/G007_stdio.h"                /* the thread / semaphore syscalls */
#include "game/app/T001_packet_buffers.h"       /* g_0028F550 */
#include "game/app/T002_render_thread.h"
#include "engine/input/T475_pad_manager.h"          /* PadManager g_002EA7F8 */
#include "engine/io/T501_memorycard_manager.h"   /* mc_manager_tick */
#include "engine/gfx/T512_render_context.h"       /* RenderContext g_002F4230, GSBufferPair */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */

enum { RENDER_THREAD_STACK = 0x8000, RENDER_THREAD_PRIO = 7, RENDER_THREAD_PRIO_VSYNC = 1,
       MAIN_THREAD_PRIO = 9 };
/* The image the bootstrap leaves on screen ("hot" start): its VRAM word
 * address, and the frames of boot_screen_fade (15 in, 120 full, 15 out). */
enum { BOOT_IMAGE_VRAM = 0x50000, BOOT_FADE_FRAMES = 150, BOOT_FADE_RAMP = 15 };

/* .sdata (-G8), in this order. */
GameObject *g_00350B10 = NULL;              /* 0x00350B10 gp-0x6EE0 the object being drawn */
GameObject *g_00350B14 = NULL;              /* 0x00350B14 gp-0x6EDC the object render_set_object requested */
int g_00350B18 = -1;                        /* 0x00350B18 gp-0x6ED8 the render thread (-1: none) */
int g_00350B1C = -1;                        /* 0x00350B1C gp-0x6ED4 the thread waiting for its exit (-1) */
int g_00350B20 = -1;                        /* 0x00350B20 gp-0x6ED0 the render semaphore */
int g_00350B24 = -1;                        /* 0x00350B24 gp-0x6ECC the boot-fade semaphore (-1: no fade) */
/* .bss: the render thread's stack (file-local). */
static u8 g_00493700[RENDER_THREAD_STACK];  /* 0x00493700 */

// FUNC 0x001012E0 render_thread_main
void render_thread_main(void *arg)
{
    if (g_00350B24 >= 0)
        boot_screen_fade();
    WaitSema(g_00350B20);
    gfx_vsync(1);
    g_002F4230.setDisplayMode(1, 2, 1);
    GSBufferPair *shown = &g_002F4230.frame;
    g_002F4230.setScreenSize(SCREEN_W, SCREEN_H);
    shown->buf[shown->cur].setDisplay();

    while (1) {                             /* not for (;;): egcs lays that one out rotated */
        /* Per frame (loop.c hoists them; retail copies the frame pair's
         * address of the code above into a register of its own). */
        GSBufferPair *frame = &g_002F4230.frame;
        VramAllocator *vram = &g_002F4230.vram;
        GSBufferPair *other = &g_002F4230.unkAC4;
        mc_manager_tick();
        g_00350B10 = g_00350B14;
        int doUpdate = g_002EA7F8.update();
        g_0028F550.cur = 1 - g_0028F550.cur;
        gfx_set_packet_buffer(g_0028F550.buf[g_0028F550.cur], g_0028F550.size);
        if (g_00350B10 != NULL) {
            if (doUpdate != 0)
                g_00350B10->update();
            gfx_begin_packet();
            vram->lock();
            g_00350B10->render(frame->buf[frame->cur]);
            gfx_dma_sync_frame();
            g_00350B10->virtual_3();
            vram->flushReleased();
            vram->unlock();
            gfx_end_packet();
            frame->buf[frame->cur].visible = 1;
        }
        /* Up for the vsync wait, back to 7 for the next frame. */
        ChangeThreadPriority(g_00350B18, RENDER_THREAD_PRIO_VSYNC);
        gfx_sync_pipeline();
        gfx_vsync(-1);
        frame->cur = 1 - frame->cur;
        frame->buf[frame->cur].setDisplay();
        other->cur = 1 - other->cur;
        if (g_00350B1C != -1)
            break;
        if (g_00350B10 != NULL)
            gfx_kick_frame_lists(NULL);
        ChangeThreadPriority(g_00350B18, RENDER_THREAD_PRIO);
    }

    WakeupThread(g_00350B1C);
    g_00350B1C = -1;
    g_00350B18 = -1;
    ExitDeleteThread();
}

/* Run first by the render thread when the bootstrap started us ("hot"): the
 * image the bootstrap left at VRAM 0x50000 is drawn over the screen for 150
 * frames, fading in over 15, full for 120, fading out over 15; then both
 * frame buffers are cleared and g_00350B24 is signalled. */
// FUNC 0x00101528 boot_screen_fade
void boot_screen_fade(void)
{
    GSBuffer screen;                        /* sp+0x00 */
    screen.setAddr(0);
    screen.width   = SCREEN_W;
    screen.height  = SCREEN_H;
    screen.psm     = 1;                     /* PSMCT24 */
    screen.visible = 1;
    GSBuffer image;                         /* sp+0x40 */
    GSBuffer *img = &image;                 /* retail sets it up through $s1 = &image */
    img->setAddr(BOOT_IMAGE_VRAM);
    img->width  = SCREEN_W;
    img->height = SCREEN_H;
    img->psm    = 1;

    gfx_vsync(1);
    if (gs_get_field_mode() == PGL_FIELD_HALF_ODD)
        gfx_vsync(1);

    for (int frameNo = 0; frameNo < BOOT_FADE_FRAMES; frameNo++) {
        float level;
        if (frameNo < BOOT_FADE_RAMP)
            level = (float)frameNo / (float)BOOT_FADE_RAMP;
        else if (frameNo < BOOT_FADE_FRAMES - BOOT_FADE_RAMP)
            level = 1.0f;
        else
            level = (float)(BOOT_FADE_FRAMES - frameNo) / (float)BOOT_FADE_RAMP;
        gfx_set_packet_buffer(g_0028F550.buf[g_0028F550.cur], g_0028F550.size);
        gfx_begin_packet();
        screen.setDrawBuffer();
        image.setTexture();
        gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
        gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
        pgl_color3f(level, level, level);
        gfx_disable(PGL_CAP_DEPTH_TEST);
        /* The origin as a variable: with 0.0f literals loop.c keeps 0.0 in
         * a saved FPU register; retail loads it in the loop. */
        float x0 = 0.0f;
        float x1 = (float)screen.width;
        float y1 = (float)screen.displayHeight();
        gs_draw_textured_sprite_f(x0, x0, x1, y1, x0, x0, (float)image.width, (float)image.height);
        gfx_end_packet();
        gfx_vsync(2);
        screen.setDisplay();
        gfx_kick_frame_lists(NULL);
        gfx_sync_pipeline();
    }

    gfx_set_packet_buffer(g_0028F550.buf[g_0028F550.cur], g_0028F550.size);
    gfx_begin_packet();
    gs_set_clear_color(0);
    GSBufferPair *frame = &g_002F4230.frame;
    frame->buf[frame->cur].setDrawBuffer();
    gs_clear_buffers(PGL_CLEAR_COLOR);
    frame->buf[1 - frame->cur].setDrawBuffer();
    gs_clear_buffers(PGL_CLEAR_COLOR);
    gfx_end_packet();
    gfx_vsync(2);
    gfx_kick_frame_lists(NULL);
    frame->buf[frame->cur].setDisplay();
    signal_sema_helper(g_00350B24);
}

/* Creates the render semaphore (and, with bootFade, the boot-fade one),
 * creates and starts the render thread (priority 7) and drops the calling
 * thread to priority 9. */
// FUNC 0x00101820 render_thread_start
void render_thread_start(int bootFade)
{
    struct ThreadParam param;               /* sp+0x00 */

    g_00350B1C = -1;
    g_00350B20 = create_mutex_sema(0);
    if (bootFade != 0)
        g_00350B24 = create_mutex_sema(0);
    param.entry        = render_thread_main;
    param.stack        = g_00493700;
    param.stackSize    = RENDER_THREAD_STACK;
    param.initPriority = RENDER_THREAD_PRIO;
    param.gpReg        = _gp;
    param.option       = 0;
    g_00350B18 = CreateThread(&param);
    StartThread(g_00350B18, NULL);
    ChangeThreadPriority(GetThreadId(), MAIN_THREAD_PRIO);
}

/* Starts the render thread if needed, waits for the boot fade and deletes
 * its semaphore, then lets the render thread run. */
// FUNC 0x001018C8 render_thread_release
void render_thread_release(void)
{
    if (g_00350B18 == -1)
        render_thread_start(0);
    if (g_00350B24 >= 0) {
        WaitSema(g_00350B24);
        DeleteSema(g_00350B24);
        g_00350B24 = -1;
    }
    signal_sema_helper(g_00350B20);
}

// FUNC 0x00101920 render_thread_exit
void render_thread_exit(void)
{
    if (g_00350B18 != -1) {
        g_00350B1C = GetThreadId();
        SleepThread();
    }
}

// FUNC 0x00101950 render_thread_lock
void render_thread_lock(void)
{
    WaitSema(g_00350B20);
    gfx_vsync(2);
}

/* NULL releases the render semaphore, anything else polls (takes) it. */
// FUNC 0x00101978 render_set_object
void render_set_object(GameObject *obj)
{
    g_00350B14 = obj;
    if (obj == NULL)
        signal_sema_helper(g_00350B20);
    else
        PollSema(g_00350B20);
}

// FUNC 0x001019B0 render_get_object
GameObject *render_get_object(void)
{
    return g_00350B10;
}

// FUNC 0x001019B8 GameObject::GameObject
GameObject::GameObject()
{
}

// FUNC 0x001019D0 GameObject::~GameObject
GameObject::~GameObject()
{
    /* The vptr, then operator delete with flags & 1. */
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x00101A00 GameObject::update
void GameObject::update()
{
}

/* The default render: clears the buffer. */
// FUNC 0x00101A08 GameObject::render
void GameObject::render(GSBuffer &buf)
{
    buf.setDrawBuffer();
    gs_set_clear_color(0);
    gs_clear_buffers(PGL_CLEAR_COLOR);
}

// FUNC 0x00101A38 GameObject::virtual_3
void GameObject::virtual_3()
{
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__user_type_info, no base). */
// FUNC 0x00101A40 __tf10GameObject
extern "C" void *__tf10GameObject(void)
{
    if (__ti10GameObject[0] == 0)
        __rtti_user(__ti10GameObject, "10GameObject");
    return __ti10GameObject;
}
#endif
