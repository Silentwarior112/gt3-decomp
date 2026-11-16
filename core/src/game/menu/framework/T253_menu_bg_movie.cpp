/* T253 - MenuBgMovie, the menu background movie (g_002CDC10).
 * 0x001CCBD0..0x001CD6A0.
 *
 * Wraps an mpegif stream (T529).  update() (from the render functions, once
 * per frame) takes the next decoded frame and narrows the stream's
 * presentation window to that frame; upload() copies the frame (16 x 16
 * blocks of 32-bit pixels, 0x400 bytes each, in columns of blocks) into a GS
 * buffer, either whole or, for interlaced movies, the field the display is
 * showing; MpegImage (T249) then draws it as a texture.  (? class name.)
 *
 * Data: no .rodata in the original (MenuBgMovie is not polymorphic, mpegif's
 * and GSBuffer's key functions are elsewhere); .data = g_002CDC10, then the
 * frame info. */
#include "game/menu/framework/T253_menu_bg_movie.h"
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "engine/media/T529_mpegif.h"               /* mpegif, MpegifParam, MPEGIF_CMD_*, MPEGIF_* states */
#include "engine/pdi/T522_pdimpeg.h"              /* F_0025CFC0 (the presentation window) */
#include "game/app/T000_main.h"                 /* MovieParam */
#include "engine/pgl/G001_flare.h"                /* gs_get_field_mode, gs_upload_image, gfx_* */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend / intr_resume */
#include "libc.h"                           /* strcmp */
#include "game/menu/framework/render_base_internal.h"

// FUNC 0x001CCBD0 MenuBgMovie::MenuBgMovie
MenuBgMovie::MenuBgMovie()
{
    /* implicit: GSBuffer (member constructor) */
    /* buf's GSBuffer() first */
    stream = NULL;
}

// FUNC 0x001CCC00 MenuBgMovie::~MenuBgMovie
MenuBgMovie::~MenuBgMovie()
{
    close();
    /* then buf's ~GSBuffer (flags 2); operator delete with flags & 1 */
    /* implicit: ~GSBuffer (member destructor), operator delete (deleting destructor) */
}

/* Creates a w x h stream (thread priorities: control 1, read 4, feed 10,
 * decode 11); interlaced by default. */
// FUNC 0x001CCC50 MenuBgMovie::open
void MenuBgMovie::open(int w, int h)
{
    uploaded  = 0;
    lastFrame = NULL;
    frame     = NULL;
    MpegifParam prio;
    prio.prioHigh   = 1;
    prio.prioRead   = 4;
    prio.prioFeed   = 10;
    prio.prioDecode = 11;
    stream = new mpegif(&prio, w, h);       /* operator new(0x154); the landing pad: operator delete */ /* implicit: operator new, operator delete (new-expression) */
    width  = w;
    height = h;
    setSize(w, h);
    field     = 1;
    frameNo   = -1;
    suspended = 0;
}

/* Deletes the stream with interrupts suspended. */
// FUNC 0x001CCD38 MenuBgMovie::close
void MenuBgMovie::close()
{
    lastFrame = NULL;
    frame = NULL;
    if (stream != NULL) {
        int wasEnabled = intr_suspend();
        delete stream;                      /* virtual deleting destructor (flags 3) */
        stream = NULL;
        intr_resume(wasEnabled);
    }
}

// FUNC 0x001CCDA8 MenuBgMovie::preload
void MenuBgMovie::preload(const char *file)
{
    if (stream != NULL)
        stream->preload(file, MOVIE_PRELOAD_SIZE);
}

// FUNC 0x001CCDD0 MenuBgMovie::unload
void MenuBgMovie::unload()
{
    if (stream != NULL)
        stream->freePreload();
}

/* The texture sizes: powers of 2 (at most 0x400) holding the width and the
 * height of a field (half the height when interlaced). */
// FUNC 0x001CCDF8 MenuBgMovie::setSize
void MenuBgMovie::setSize(int w, int h)
{
    width  = w;
    height = h;
    texW   = 1;
    while (texW < 0x400 && texW < width)
        texW = texW * 2;

    f32 fieldH;
    if (field == 1)
        fieldH = (f32)(h / 2);
    else
        fieldH = (f32)h;
    texH = 1;
    while (texH < 0x400 && !(fieldH <= (f32)texH))
        texH = texH * 2;
}

/* Interlaced: uploads the rows of the field being shown into a buffer of
 * half the height: the lower half of the frame while the display shows
 * field 0 (gs_get_field_mode() == PGL_FIELD_HALF_EVEN), else the upper
 * half.  Bilinear filtering off. */
// FUNC 0x001CCEC0 MenuBgMovie::uploadField
void MenuBgMovie::uploadField()
{
    if (stream == NULL)
        return;

    buf.psm    = PSM_CT32;
    buf.width  = width;
    buf.height = height / 2;
    buf.allocVram();
    const u8 *block = (const u8 *)lastFrame;
    uploaded = 1;

    int top, bottom;
    if (gs_get_field_mode() != PGL_FIELD_HALF_EVEN) {
        top = 0;
        bottom = height / 2;
    } else {
        top = height / 2;
        bottom = height;
    }
    for (int bx = 0; bx < width; bx += PIXEL_BLOCK) {
        for (int by = 0; by < height; by += PIXEL_BLOCK) {
            if (by >= top && by < bottom)
                gs_upload_image(buf.addr, buf.psm, gs_buffer_width(buf.width), bx,
                                by - top, PIXEL_BLOCK, PIXEL_BLOCK, block);
            block += PIXEL_BLOCK_SIZE;
        }
    }
    gfx_texflush();
    buf.setTexture();
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
}

/* Progressive: uploads the whole frame (bilinear). */
// FUNC 0x001CD048 MenuBgMovie::uploadFrame
void MenuBgMovie::uploadFrame()
{
    if (stream == NULL)
        return;

    buf.psm    = PSM_CT32;
    buf.width  = width;
    buf.height = height;
    buf.allocVram();
    uploaded = 1;
    const u8 *block = (const u8 *)lastFrame;

    for (int bx = 0; bx < width; bx += PIXEL_BLOCK) {
        for (int by = 0; by < height; by += PIXEL_BLOCK) {
            gs_upload_image(buf.addr, buf.psm, gs_buffer_width(buf.width), bx, by,
                            PIXEL_BLOCK, PIXEL_BLOCK, block);
            block += PIXEL_BLOCK_SIZE;
        }
    }
    gfx_texflush();
    buf.setTexture();
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
}

// FUNC 0x001CD168 MenuBgMovie::upload
void MenuBgMovie::upload()
{
    if (field == 1)
        uploadField();
    else
        uploadFrame();
}

/* Frees the buffer's VRAM after drawing. */
// FUNC 0x001CD1A8 MenuBgMovie::release
void MenuBgMovie::release()
{
    if (stream != NULL) {
        buf.freeVram();
        uploaded = 0;
    }
}

/* Once a frame, on field 1 (display state PGL_FIELD_HALF_ODD) only: gives
 * back the previous frame and takes the next one.  Frame n (from the second
 * on) sets the presentation window to [t(n) - half a frame, t(n + 1) + half
 * a frame], t(n) = n * 2000000000 / 59933 microseconds (the low end left at
 * t(n) while that is below half a frame); with no frame left (after the
 * first, and the feeder thread not ended) the window is opened again. */
// FUNC 0x001CD1E0 MenuBgMovie::update
void MenuBgMovie::update()
{
    if (stream == NULL)
        return;
    if (suspended != 0)
        return;

    if (gs_get_field_mode() == PGL_FIELD_HALF_ODD) {
        if (frame != NULL)
            stream->releaseFrame();
        frame = stream->nextFrame();
        if (frame != NULL) {
            s64 n = frameNo++;                  /* 64-bit: retail multiplies by shifts, not mult */
            if (n >= 0) {
                s64 start = n * MOVIE_TIME_SCALE / MOVIE_FIELD_RATE;          /* __divdi3 */ /* implicit: __divdi3 (64-bit division) */
                s64 end   = (n + 1) * MOVIE_TIME_SCALE / MOVIE_FIELD_RATE;
                F_0025CFC0(start < MOVIE_HALF_FRAME ? start : start - MOVIE_HALF_FRAME,
                           end + MOVIE_HALF_FRAME);
            }
        } else if (frameNo > 0) {
            /* the test as a value (retail: xori / beql; a direct != compare
             * reuses the field mode's register for the 4) */
            int running = stream->control(MPEGIF_CMD_GET_STATE, NULL) != MPEGIF_ENDED;
            if (running)
                F_0025CFC0(0, MOVIE_TIME_MAX);
        }
    }
    if (frame != NULL)
        lastFrame = frame;
}

/* Starts file (from the preloaded copy when it is that file: then without
 * sound, a quirk); the result is nonzero while the stream cannot start yet
 * (retry). */
// FUNC 0x001CD348 MenuBgMovie::play
int MenuBgMovie::play(const char *file, int withSound, int oneShot)
{
    mpegif *s = stream;
    if (s == NULL)
        return 0;
    int busy;
    if (s->preloadBuf != NULL && strcmp(file, s->preloadName) == 0) {
        s->usePreload = 1;
        busy = s->start(NULL, 0, oneShot, MPEGIF_CMD_PLAY);
    } else {
        s->usePreload = 0;
        busy = s->start(file, withSound, oneShot, MPEGIF_CMD_PLAY);
    }
    lastFrame = NULL;
    frame = NULL;
    return busy;
}

/* play() through MPEGIF_CMD_SWITCH and without the preloaded copy (no
 * callers; the frames are only dropped once the switch is taken).  $a1..$a3
 * go to mpegif::start untouched. */
// FUNC 0x001CD408 MenuBgMovie::switchTo
int MenuBgMovie::switchTo(const char *file, int withSound, int oneShot)
{
    mpegif *s = stream;
    if (s == NULL)
        return 0;
    s->usePreload = 0;
    int busy = s->start(file, withSound, oneShot, MPEGIF_CMD_SWITCH);
    if (busy == 0) {
        lastFrame = NULL;
        frame = NULL;
    }
    return busy;
}

/* Switches to movie m and waits (vsync by vsync) until it is taken.
 * (Matching, 77.4%: retail loads one 1 into $v1, copies it to both flags
 * ($s1 / $s2) and clears them with movz, `this' in $s3.  This form gets
 * conditional moves, but the withSound one as movn on oneShot's register,
 * and `this' in $s1.  Tried: ?: 1 : 0 / != 0 / !! (sltu), if / else, flags
 * set at the top, in the middle or chained (x = y = 1), from a common
 * variable, bool flags, do { } while (0) wraps, &= / && forms, a `self'
 * copy of this, for (;;) with break.  The flags are read between the
 * frameNo and field stores (u071's data pass, 76.8% -> 77.4%): retail
 * loads them before the field store, which an int store after them in the
 * source would keep behind it.) */
// FUNC 0x001CD458 MenuBgMovie::start
void MenuBgMovie::start(const MovieParam *m)
{
    frameNo = -1;
    int withSound = 1;
    int oneShot = 1;
    if (m->withSound == 0)
        withSound = 0;
    if (m->oneShot == 0)
        oneShot = 0;
    field = m->field;
    setSize(m->width, m->height);
    while (play(m->file, withSound, oneShot) != 0)
        gfx_vsync(1);
}

// FUNC 0x001CD500 MenuBgMovie::stop
void MenuBgMovie::stop()
{
    lastFrame = NULL;
    frame = NULL;
    if (stream != NULL)
        stream->stopFrames();
}

// FUNC 0x001CD530 MenuBgMovie::suspend
void MenuBgMovie::suspend(int on)
{
    suspended = on;
    if (on != 0)
        stop();
}

/* (? names) The three wait conditions of stopAndWait, each false without a
 * stream: the stop command not taken yet; the stream neither switched nor
 * ended; the feeder thread not ended. */
static inline int movie_control(MenuBgMovie *m, int cmd)
{
    if (m->stream != NULL)
        return m->stream->control(cmd, NULL);
    else
        return 0;
}

static inline int movie_switching(MenuBgMovie *m)
{
    if (m->stream != NULL) {
        int state = m->stream->control(MPEGIF_CMD_GET_STATE, NULL);
        return state != MPEGIF_SWITCHED && state != MPEGIF_ENDED;
    } else
        return 0;
}

static inline int movie_running(MenuBgMovie *m)
{
    if (m->stream != NULL)
        return m->stream->control(MPEGIF_CMD_GET_STATE, NULL) != MPEGIF_ENDED;
    else
        return 0;
}

/* Stops the movie and waits for its end, vsync by vsync: the presentation
 * window opened (so the decoder is not held back), MPEGIF_CMD_STOP until it
 * is taken, then the state until it is MPEGIF_SWITCHED or MPEGIF_ENDED, then
 * until it is MPEGIF_ENDED.  Retail computes each condition as a value (the
 * helpers above, inlined) and tests it at the bottom of a rotated while. */
// FUNC 0x001CD558 MenuBgMovie::stopAndWait
void MenuBgMovie::stopAndWait()
{
    /* implicit: control (mpegif::control, in the inlined helpers) */
    F_0025CFC0(0, MOVIE_TIME_MAX);
    while (movie_control(this, MPEGIF_CMD_STOP))
        gfx_vsync(1);
    while (movie_switching(this))
        gfx_vsync(1);
    while (movie_running(this))
        gfx_vsync(1);
}

/* The object (.data 0x002CDC10: egcs puts a public object that has a
 * constructor in .data, zero-filled, not in .bss). */
MenuBgMovie g_002CDC10;

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002CDC10 (.dtors entry 0x0028F234, .ctors entry 0x0028F158), emitted at
 * the end of the unit, the destructor first: ~MenuBgMovie(2) / MenuBgMovie()
 * on it. */
// FUNC 0x001CD658 T253_static_destroy (_GLOBAL_$D$__11MenuBgMovie)
/* implicit: ~MenuBgMovie (the static destructor of g_002CDC10) */

// FUNC 0x001CD680 T253_static_init (_GLOBAL_$I$__11MenuBgMovie)
/* implicit: MenuBgMovie (the static initialiser of g_002CDC10) */
