/* T002_render_thread.h - GameObject and the render thread (translation unit
 * T002, unit u001), 0x001012E0..0x00101A80.
 *
 * The render thread (priority 7, its own 0x8000-byte stack) draws one
 * GameObject per frame: at the top of each frame it takes the object
 * render_set_object requested, flips the packet buffers (T001) and, with an
 * object, calls its update() (when PadManager::update of g_002EA7F8, T475,
 * says so), render(the current frame buffer of g_002F4230, T512) and
 * virtual_3(), then waits for the vsync and flips the frame
 * buffers.  render_thread_exit stops it.  With the bootstrap's boot screen
 * still shown ("hot" start) it first fades that image out (boot_screen_fade).
 *
 * File-local data: g_00350B10 (gp-0x6EE0) the object being drawn,
 * g_00350B14 (gp-0x6EDC) the one requested, g_00350B18 the thread id (-1),
 * g_00350B1C the thread waiting for the exit (-1), g_00350B20 the render
 * semaphore, g_00350B24 the boot-fade semaphore (-1: no fade), g_00493700
 * the 0x8000-byte stack. */
#ifndef GAME_T002_RENDER_THREAD_H
#define GAME_T002_RENDER_THREAD_H

#include "game/game.h"

class GSBuffer;                             /* T514 (references only) */

/* vtable 0x00308780 (4 slots).  typeinfo __tf10GameObject 0x00101A40
 * (__user_type_info 0x00353318).  No data: vptr +0x00, size 4.  The base of
 * the menu renderers (RenderBase, T246), the races (RaceBase, T129), the
 * transitions (T005) and GameObjectList (T003).  Slot names: Lupo 2's
 * update / render(GSBuffer &); slot 3 is unnamed. */
class GameObject {
public:
    GameObject();                           /* 0x001019B8 */
    virtual ~GameObject();                  /* [0] +0x0C 0x001019D0 */
    virtual void update();                  /* [1] +0x14 0x00101A00 empty */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x00101A08 clears buf (draw target, clear 1) */
    virtual void virtual_3();               /* [3] +0x24 0x00101A38 empty; called after render, inside
                                               the frame's packet */
    /*  vptr                                   0x00 */
};
EGCS_SIZE(GameObject, 4);

/* The thread's entry (arg unused): the boot fade if any, then the frame
 * loop until render_thread_exit; ExitDeleteThread. */
void render_thread_main(void *arg);         /* 0x001012E0 */
/* The image the bootstrap left at VRAM 0x50000 over the screen for 150
 * frames (fade in 15, 120 full, fade out 15), both frame buffers cleared,
 * then the boot-fade semaphore signalled. */
void boot_screen_fade(void);                /* 0x00101528 */
/* The render semaphore (and with bootFade the boot-fade one), the thread
 * (priority 7, gp _gp) started; the caller drops to priority 9. */
void render_thread_start(int bootFade);     /* 0x00101820 */
/* Starts the thread if needed, waits for the boot fade, then lets the
 * thread run (signals the render semaphore). */
void render_thread_release(void);           /* 0x001018C8 */
/* Asks the thread to exit and sleeps until it has. */
void render_thread_exit(void);              /* 0x00101920 */
/* Takes the render semaphore, then waits 2 vsyncs. */
void render_thread_lock(void);              /* 0x00101950 */
/* The object to draw from the next frame on; NULL signals the render
 * semaphore, anything else polls it. */
void render_set_object(GameObject *obj);    /* 0x00101978 */
GameObject *render_get_object(void);        /* 0x001019B0 the object being drawn */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tf10GameObject(void);               /* 0x00101A40 */
extern u32 __ti10GameObject[2];             /* 0x00353318 gp-0x46D8 (__user_type_info) */
}
#endif

#endif /* GAME_T002_RENDER_THREAD_H */
