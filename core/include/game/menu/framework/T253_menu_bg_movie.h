/* T253_menu_bg_movie.h - MenuBgMovie (? name), the menu background movie
 * g_002CDC10 (translation unit T253, unit u071), 0x001CCBD0..0x001CD6A0.
 *
 * Wraps an mpegif stream (T529).  update() (once a frame, from the render
 * functions) takes the next decoded frame and narrows the stream's
 * presentation window to it; upload() copies the frame (16 x 16 blocks of
 * 32-bit pixels, in columns of blocks) into a GS buffer, the whole frame or,
 * interlaced, the field the display shows; MpegImage (T249) then draws it as
 * a texture.  The first pass also calls the class MoviePlayer (u003) and
 * MenuMovie (u009, u013; T036_garage.h). */
#ifndef GAME_T253_MENU_BG_MOVIE_H
#define GAME_T253_MENU_BG_MOVIE_H

#include "game/game.h"
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer (buf) */

class mpegif;                               /* T529 (pointers only) */
struct MovieParam;                          /* T000: {file, width, height, field, ...} 0x18 bytes */

/* Not polymorphic.  Size 0x64.  The one instance is g_002CDC10 (static
 * constructor T253_static_init, destructor T253_static_destroy). */
class MenuBgMovie {
public:
    MenuBgMovie();                          /* 0x001CCBD0 */
    ~MenuBgMovie();                         /* 0x001CCC00 close() */
    void open(int w, int h);                /* 0x001CCC50 a new w x h stream (interlaced) */
    void close();                           /* 0x001CCD38 deletes the stream (interrupts suspended) */
    void preload(const char *file);         /* 0x001CCDA8 into the stream's 0x500000-byte buffer */
    void unload();                          /* 0x001CCDD0 drops the preloaded file */
    void setSize(int w, int h);             /* 0x001CCDF8 texW / texH: powers of 2 */
    void uploadField();                     /* 0x001CCEC0 the field shown, into a half-height buffer */
    void uploadFrame();                     /* 0x001CD048 the whole frame */
    void upload();                          /* 0x001CD168 uploadField or uploadFrame */
    void release();                         /* 0x001CD1A8 frees the buffer's VRAM after drawing */
    void update();                          /* 0x001CD1E0 the next decoded frame, the presentation
                                               window */
    /* mpegif::start(file, withSound, oneShot, MPEGIF_CMD_PLAY), or from the
     * preloaded copy if it is that file (then withSound 0); nonzero: not
     * taken yet, retry. */
    int  play(const char *file, int withSound, int oneShot);       /* 0x001CD348 */
    /* The same through MPEGIF_CMD_SWITCH, never from the preloaded copy (no
     * callers; mpegif::switchTo's counterpart). */
    int  switchTo(const char *file, int withSound, int oneShot);   /* 0x001CD408 (F_001CD408) */
    void start(const MovieParam *m);        /* 0x001CD458 switches to m, waits until it is taken */
    void stop();                            /* 0x001CD500 mpegif::stopFrames */
    void suspend(int on);                   /* 0x001CD530 on: stop(), update does nothing */
    void stopAndWait();                     /* 0x001CD558 (waitEnd) stops the stream (MPEGIF_CMD_STOP) and
                                               waits (vsync by vsync) until the feeder thread has ended */

    GSBuffer buf;                           /* 0x00 the uploaded frame */
    int      uploaded;                      /* 0x38 */
    void    *frame;                         /* 0x3C the frame got from the stream this vsync */
    void    *lastFrame;                     /* 0x40 the last frame got (MpegImage draws it) */
    int      width;                         /* 0x44 */
    int      height;                        /* 0x48 */
    int      texW;                          /* 0x4C power of 2 >= width (<= 0x400) */
    int      texH;                          /* 0x50 power of 2 >= the field height */
    int      field;                         /* 0x54 1: interlaced fields */
    int      frameNo;                       /* 0x58 frames shown (-1 before the first) */
    int      suspended;                     /* 0x5C nonzero: update does nothing */
    mpegif  *stream;                        /* 0x60 NULL: none */
};
EGCS_SIZE(MenuBgMovie, 0x64);

extern MenuBgMovie g_002CDC10;              /* 0x002CDC10 (T253 .data, defined there) the menu background movie */

/* 0x001CD658 (.dtors 0x0028F234) / 0x001CD680 (.ctors 0x0028F158): egcs's
 * static destructor and initialiser of g_002CDC10 (T253 defines it):
 * _GLOBAL_$D$__11MenuBgMovie (~MenuBgMovie(2)) and _GLOBAL_$I$__11MenuBgMovie
 * (MenuBgMovie()). */

#endif /* GAME_T253_MENU_BG_MOVIE_H */
