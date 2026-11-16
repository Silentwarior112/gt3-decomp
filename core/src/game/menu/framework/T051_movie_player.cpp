/* T051 - play_movie: plays a movie full screen with gt2k_menu::MovieControl
 * and gt2k_menu::MovieRender.  0x001287A8..0x00128D98.
 *
 * Data: .rodata 0x00313538..0x003137D6: <new>'s "bad_alloc" (unreferenced:
 * DATA_START), the two vtables, then the type names (reverse definition
 * order): MovieControl, MovieRender, OperatorList, List<OperatorHolder>,
 * OperatorHolder, Cell<OperatorHolder>, UnitArena<64>, RegurationResult,
 * RecordsMenuArg, the garage's eight, Operator; so <new>, the GameStatus,
 * RecordsMenuArg (T089), RegurationResult, T153's UnitArena<64>, the
 * operator classes (T216), the two classes last; no SmartIndexT<int> (no
 * T251: no PageManager).  .lit4: 128 / 255 (0x0034FAE8). */
// DATA_START 0x00313538
#include <new>                              /* (seen: "bad_alloc", T051's first .rodata) */
#include "game/save/T429_game_status.h"          /* (seen: the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* (seen: RecordsMenuArg) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#define GAME_T153_EMITS_TYPEINFO            /* (seen: UnitArena<64>'s type name) */
#include "game/race/core/T153_race_course.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen: the operator classes) */
#include "game/app/T000_main.h"                 /* MovieParam */
#include "game/app/T002_render_thread.h"        /* render_set_object */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push / pop */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset */
#include "engine/input/T474_control.h"              /* CONTROL_START */
#include "engine/media/T529_mpegif.h"               /* mpegif::control, MPEGIF_CMD_GET_STATE, MPEGIF_ENDED */
#include "engine/pdistd/T532_heap.h"                 /* memalign, free */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si (GCC 14's __tf) */
#include "game/menu/framework/T051_movie_player.h"         /* MovieRender, MovieControl: the last classes defined */
#include "game/menu/gtmode/gtmode_main_internal.h"

// FUNC 0x001287A8 play_movie
/* Plays p (MovieParam, T000: the file, its size and flags) through the menu
 * background movie player g_002CDC10 (T253) with a 1 MB menu heap, until the
 * movie ends (the stream's state MPEGIF_ENDED, or no stream) or pad 1's
 * start button is pressed (control1.decideButtons: CONTROL_START, T474, for
 * the frame's decide), then fades it out to black in 30 frames.  Inline (see
 * game/menu/framework/T051_movie_player.h): ControlBase() and render's inline constructor
 * (the member) come first; waitFrame() is ControlBase's (slot 1), called
 * directly.  The fade goes through a pointer to render (retail: a second
 * register holding &render for the fade loop). */
inline gt2k_menu::MovieControl::MovieControl(MovieParam *p)
{
    menu_draw_reset();
    void *heap = memalign(MENU_HEAP_ALIGN, MOVIE_HEAP_SIZE);
    menu_heap_push(heap, MOVIE_HEAP_SIZE);
    attach();
    gfx_vsync(2);
    g_002CDC10.open(640, 448);
    g_002CDC10.start(p);
    MovieRender *r = &render;
    render_set_object(r);
    control1.decideButtons = CONTROL_START;     /* start skips the movie */
    while (control1.decide == 0) {
        waitFrame();
        int playing;
        if (g_002CDC10.stream != NULL)
            playing = g_002CDC10.stream->control(MPEGIF_CMD_GET_STATE, NULL) != MPEGIF_ENDED;
        else
            playing = 0;
        if (!playing)
            break;
    }
    r->fade = 30;
    do {
        waitFrame();
    } while (r->fade > 0);
    g_002CDC10.stopAndWait();
    g_002CDC10.close();
    render_set_object(NULL);
    gfx_vsync(2);
    menu_heap_pop();
    free(heap);
    detach();
}

/* The MovieControl lives at sp+0x000: its constructor (above), which plays
 * the movie, and its implicit destructor are expanded here, with the
 * constructor's landing pads.  Callers: main (T000), arcade_menu (T015),
 * F_0013F508 (T061). */
void play_movie(MovieParam *p)
{
    /* implicit: ControlBase, RenderBase, MpegImage (the inline MovieControl / MovieRender constructors), ~MpegImage, ~RootObject, ~GameObject, ~ControlBase (the implicit destructor and the exception cleanups) */
    gt2k_menu::MovieControl ctl(p);         /* sp+0x000 */
}

#ifndef BOOT_OLD_GCC
/* Builds the __si_type_info of MovieControl (base ControlBase, whose node
 * is __tiQ29gt2k_menu11ControlBase gp-0x45F8) on the first call. */
// FUNC 0x00128B30 __tfQ29gt2k_menu12MovieControl
extern "C" void *__tfQ29gt2k_menu12MovieControl(void)
{
    if (__tiQ29gt2k_menu12MovieControl[0] == 0) {       /* _name not set yet */
        __tfQ29gt2k_menu11ControlBase();                /* the base's node first */
        __rtti_si(__tiQ29gt2k_menu12MovieControl, "Q29gt2k_menu12MovieControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu12MovieControl;
}
#endif

// FUNC 0x00128B80 gt2k_menu::MovieControl::~MovieControl
/* implicit in game/menu/framework/T051_movie_player.h; egcs emits this copy for the
 * vtable: no vptr store; render's inline destructor (movie's
 * gt2k_menu::MpegImage::~MpegImage, render.root's RootObject::~RootObject,
 * render's GameObject::~GameObject, all flags 2), then
 * gt2k_menu::ControlBase::~ControlBase(this, flags), which does the
 * delete. */
/* implicit: ~MpegImage, ~RootObject, ~GameObject (member destructors of render), ~ControlBase (base destructor) */

// FUNC 0x00128BE8 gt2k_menu::MovieRender::render
/* Slot 2: inline in game/menu/framework/T051_movie_player.h (MovieRender has no key
 * function); egcs emits this copy for the vtable, at the end of the unit
 * after MovieControl's group, before update (reverse declaration order). */
/* implicit: update, beginRender, virtual_2, gfx_enable, gfx_blend_func, pgl_color4f, gs_draw_flat_sprite, endRender (the inline body in game/menu/framework/T051_movie_player.h) */

// FUNC 0x00128CC0 gt2k_menu::MovieRender::update
/* Slot 1: inline in game/menu/framework/T051_movie_player.h; egcs emits this copy for the
 * vtable. */
/* implicit: virtual_1 (the inline body in game/menu/framework/T051_movie_player.h: movie's update) */

#ifndef BOOT_OLD_GCC
/* Builds the __si_type_info of MovieRender (base RenderBase) on the first
 * call. */
// FUNC 0x00128CF8 __tfQ29gt2k_menu11MovieRender
extern "C" void *__tfQ29gt2k_menu11MovieRender(void)
{
    if (__tiQ29gt2k_menu11MovieRender[0] == 0) {        /* _name not set yet */
        __tfQ29gt2k_menu10RenderBase();                 /* the base's node first */
        __rtti_si(__tiQ29gt2k_menu11MovieRender, "Q29gt2k_menu11MovieRender",
                  (const type_info *)__tiQ29gt2k_menu10RenderBase);
    }
    return __tiQ29gt2k_menu11MovieRender;
}
#endif

// FUNC 0x00128D48 gt2k_menu::MovieRender::~MovieRender
/* implicit in game/menu/framework/T051_movie_player.h; egcs emits this copy for the
 * vtable: no vptr store; movie's gt2k_menu::MpegImage::~MpegImage (flags 2),
 * then RenderBase's inline destructor: root's RootObject::~RootObject (flags
 * 2), GameObject::~GameObject(this, flags), which does the delete. */
/* implicit: ~MpegImage (member destructor), ~RootObject, ~GameObject (the inline ~RenderBase) */
