/* T507_async_loader.h - the background loader object (? name) (translation unit
 * T507, unit u111), 0x00242298..0x00242650.
 *
 * One AsyncLoader (0x00352FA8, gp-0x4A48; created / deleted around the menus)
 * loads one named file at a time into its buffer through the T508 loader
 * thread.  The caller polls it every frame with the name it wants: the buffer
 * once that file is in, else NULL; asking for another name cancels the load
 * in progress.  A request waits 3 polls before it goes to the thread. */
#ifndef GAME_T507_ASYNC_LOADER_H
#define GAME_T507_ASYNC_LOADER_H

#include "game/game.h"
#include "engine/io/T504_file_queue.h"               /* FileDoneFn */

/* Not polymorphic.  Size 0x58 (operator new in async_loader_create). */
class AsyncLoader {
public:
    AsyncLoader(int bufSize);                   /* 0x00242358 buf = memalign(64, bufSize) unless 0 */
    ~AsyncLoader();                             /* 0x002423C8 frees buf */
    void *poll(const char *path, FileDoneFn done, void *ctx);              /* 0x00242418 into its own buffer */
    /* One step; dst once path is loaded (and on every later poll of the same
     * name; done(ctx, dst) when it completes), else NULL.  The file is read
     * into dst (NULL: the loader thread allocates); the member buf is not
     * used.  state 0 idle -> 1 delay -> 2 loading (loader_request) -> 0, or
     * -> 3 cancelling -> 0. */
    void *poll(const char *path, void *dst, FileDoneFn done, void *ctx);   /* 0x00242440 */
    void  forget();                             /* 0x002425F8 name = "": the file is asked for again */
    void  reset();                              /* 0x00242600 cancels a load, clears name / loaded / state / delay */
    void  setLocked(int locked);                /* 0x00242648 */

    int   bufSize;                              /* 0x00 */
    void *buf;                                  /* 0x04 NULL if bufSize is 0 */
    int   loaded;                               /* 0x08 1: name is in the buffer */
    int   state;                                /* 0x0C 0 idle, 1 waiting, 2 loading, 3 cancelling */
    int   locked;                               /* 0x10 no new load while set */
    int   delay;                                /* 0x14 polls before the request */
    char  name[0x40];                           /* 0x18 the file loaded / loading */
};
EGCS_SIZE(AsyncLoader, 0x58);

void async_loader_create(int bufSize);          /* 0x00242298 the loader = new AsyncLoader(bufSize) */
void async_loader_destroy(void);                /* 0x00242300 waits for the loader thread to be idle, deletes it */
AsyncLoader *async_loader_get(void);            /* 0x00242350 the loader (NULL: none) */

#endif /* GAME_T507_ASYNC_LOADER_H */
