/* T051_movie_player.h - play_movie, gt2k_menu::MovieControl and
 * gt2k_menu::MovieRender (translation unit T051, unit u013),
 * 0x001287A8..0x00128D98.
 *
 * play_movie plays a movie full screen through the menu background movie
 * player (MenuBgMovie g_002CDC10, T253) with a 1 MB menu heap, until it
 * ends or pad 1's start button is pressed (control1.decideButtons =
 * CONTROL_START 0x10, T474: the frame's decide), then fades it out to black
 * in 30 frames.  The MovieControl lives on its stack
 * (sp+0x000); its constructor (which does the playing) and destructor are
 * expanded there. */
#ifndef GAME_T051_MOVIE_PLAYER_H
#define GAME_T051_MOVIE_PLAYER_H

#include "game/game.h"
#include "game/menu/framework/T246_render_base.h"          /* base gt2k_menu::RenderBase */
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T249_mpeg_image.h"           /* gt2k_menu::MpegImage (MovieRender::movie) */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 (MovieRender::render) */
#include "engine/pgl/G001_flare.h"                /* gfx_enable, gfx_blend_func, pgl_color4f,
                                               gs_draw_flat_sprite (MovieRender::render) */

class GSBuffer;                             /* T514 (references only) */
struct MovieParam;                          /* T000: file, size, flags (pointers only) */

namespace gt2k_menu {

/* vtable 0x00313560 (4 slots: [3] GameObject's 0x00101A38).  typeinfo
 * __tfQ29gt2k_menu11MovieRender 0x00128CF8 (si, base RenderBase; storage
 * 0x004B1630).  Size 0x17C (MovieControl +0x28C .. +0x408).  The
 * destructor is implicit in the original: 0x00128D48 does not store the
 * vptr (movie's ~MpegImage with flags 2, then the inline ~RenderBase).
 *
 * No key function: update and render are inline (u013's data pass).  T051's
 * frame info (0x0029B440) has render 0x00128BE8, update 0x00128CC0,
 * __tf 0x00128CF8, ~MovieRender 0x00128D48 at the end of the unit (egcs's
 * group of a key-less class: its inline members in reverse declaration
 * order, __tf, the implicit members) and no out-of-line constructor, which
 * egcs would write if update were the key function.  Only T051's original
 * has the class's type name: the callers of play_movie include
 * T051_movie_entry.h. */
class MovieRender : public RenderBase {
public:
    /* Inline (play_movie expands it): RenderBase(), the vptr, fade -1, the
     * MpegImage without a texture, centred on the 640 x 448 screen, its
     * four corners grey 0x80 (128 / 255).  Through movie's inline
     * Drawable::setRect / setColor(u32): retail stores w, h and the colours
     * through &movie in a register (the inline's `this'), and folds all four
     * channels of the colour to 128 / 255. */
    MovieRender() : fade(-1), movie(NULL)
    {
        movie.Drawable::setRect(320.0f, 224.0f, 640.0f, 448.0f);
        movie.Drawable::setColor(0x80808080u);
    }
    /*  [0] +0x0C 0x00128D48 ~MovieRender: implicit (see above) */

    /* [1] +0x14 0x00128CC0: the movie's update (MpegImage's, Item's), and
     * the fade-out countdown (down to 0).  (Its FUNC marker is in
     * T051_movie_player.cpp.) */
    virtual void update()
    {
        movie.virtual_1();
        if (fade > 0)
            fade--;
    }

    /* [2] +0x1C 0x00128BE8: the movie frame (MenuBgMovie::update) and the
     * movie instead of the menu tree: the projection (beginRender), the
     * movie (MpegImage's draw), while fade >= 0 a black 642 x 450 rectangle
     * over it, alpha 1 - fade / 30, blended as (Cs - Cd) * As + Cd;
     * endRender.  (Its FUNC marker is in T051_movie_player.cpp.) */
    virtual void render(GSBuffer &buf)
    {
        g_002CDC10.update();
        beginRender(buf);
        movie.virtual_2();                  /* MpegImage's draw */
        if (fade >= 0) {
            f32 frames = (f32)fade;
            gfx_enable(PGL_CAP_BLEND);
            gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 1.0f);
            pgl_color4f(0.0f, 0.0f, 0.0f, 1.0f - frames / 30.0f);
            gs_draw_flat_sprite(0, 0, 642, 450);
        }
        endRender(buf);
    }

    int       fade;                         /* 0x58 frames of the fade-out left; -1: none */
    MpegImage movie;                        /* 0x5C */
};
EGCS_SIZE(gt2k_menu::MovieRender, 0x17C);

/* vtable 0x00313548 (2 slots: [1] ControlBase's waitFrame 0x001CBD30).
 * typeinfo __tfQ29gt2k_menu12MovieControl 0x00128B30 (si, base
 * ControlBase; storage 0x004B1640).  ControlBase's data +0x000 .. +0x288,
 * vptr +0x288.  Size 0x408 (play_movie's frame).  The destructor is
 * implicit in the original: 0x00128B80 does not store the vptr (render's
 * inline destructor, then ~ControlBase with the flags). */
class MovieControl : public ControlBase {
public:
    /* Plays p: the whole of play_movie's work is this inline constructor
     * (T051 defines it; play_movie expands it).  Retail keeps `this' in a
     * register through the movie, calls ControlBase's waitFrame directly
     * (egcs resolves a virtual call on `this' in a constructor), and its
     * landing pads destroy render's parts and the base with flags 0: the
     * cleanups of a constructor body. */
    inline MovieControl(MovieParam *p);
    /*  [0] +0x0C 0x00128B80 ~MovieControl: implicit (see above) */

    MovieRender render;                     /* 0x28C */
};
EGCS_SIZE(gt2k_menu::MovieControl, 0x408);

} /* namespace gt2k_menu */

/* play_movie: in its own header (its callers did not see the classes). */
#include "game/menu/framework/T051_movie_entry.h"

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu12MovieControl(void); /* 0x00128B30 */
void *__tfQ29gt2k_menu11MovieRender(void);  /* 0x00128CF8 */
}
#endif

#endif /* GAME_T051_MOVIE_PLAYER_H */
