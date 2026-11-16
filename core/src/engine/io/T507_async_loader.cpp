/* T507 - the background loader object (? name) (u111).
 * 0x00242298..0x00242650.
 *
 * One AsyncLoader (g_00352FA8, created / deleted around the menus) loads
 * one named file at a time into its buffer through the T508 loader thread.
 * The caller polls it every frame with the name it wants: the buffer when
 * that file is in, else NULL; asking for another name cancels the load in
 * progress.  A request waits 3 frames (polls) before it is handed to the
 * thread.
 */
#include "engine/io/file_loader_internal.h"
#include "engine/io/T507_async_loader.h"

// FUNC 0x00242298 async_loader_create
//   g_00352FA8 = new AsyncLoader(bufSize): operator new(0x58), then the
//   constructor (the EH cleanup calls operator delete if it throws).
void async_loader_create(int bufSize)
{
    g_00352FA8 = new AsyncLoader(bufSize);     /* implicit: operator new, operator delete (landing pad) */
}

// FUNC 0x00242300 async_loader_destroy
//   Waits a vsync, then for the loader thread to be idle (one vsync per
//   poll), deletes the loader (~AsyncLoader, deleting form).
void async_loader_destroy(void)
{
    gfx_vsync(2);
    while (!loader_idle())
        gfx_vsync(1);
    if (g_00352FA8 != NULL)
        delete g_00352FA8;
    g_00352FA8 = NULL;
}

// FUNC 0x00242350 async_loader_get
AsyncLoader *async_loader_get(void)
{
    return g_00352FA8;
}

// FUNC 0x00242358 AsyncLoader::AsyncLoader
//   Everything 0; the buffer memalign(64, bufSize) unless bufSize is 0; the
//   name cleared (loop.c reverses the loop: from its last byte down).
AsyncLoader::AsyncLoader(int bufSize)
{
    int i;

    this->bufSize = bufSize;
    buf = NULL;
    loaded = 0;
    state = ASYNC_IDLE;
    locked = 0;
    delay = 0;
    if (bufSize != 0)
        buf = memalign(0x40, bufSize);
    for (i = 0; i < (int)sizeof(name); i++)
        name[i] = '\0';
}

// FUNC 0x002423C8 AsyncLoader::~AsyncLoader
//   Frees the buffer (the deleting form then operator delete).
AsyncLoader::~AsyncLoader()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    if (buf != NULL)
        free(buf);
}

// FUNC 0x00242418 AsyncLoader::poll
//   poll into its own buffer.  No callers.
void *AsyncLoader::poll(const char *path, FileDoneFn done, void *ctx)
{
    return poll(path, buf, done, ctx);
}

// FUNC 0x00242440 AsyncLoader::poll
//   One step; returns dst once path is loaded (also on every later poll of
//   the same name), else NULL.
//     idle        unless locked, forget a loaded file and, if the thread is
//                 idle, wait (the delay is 3);
//     waiting     count the delay down; then hand path to the thread
//                 (loader_request): loading, remember the name;
//     loading     the same name: once the thread is idle, loaded, idle,
//                 done(ctx, dst), dst; another name: cancel, forget,
//                 cancelling;
//     cancelling  once the thread is idle: idle (the delay is set to 3
//                 again).
//   The member buf is not used here: dst is the parameter.
void *AsyncLoader::poll(const char *path, void *dst, FileDoneFn done, void *ctx)
{
    if (loaded == 1 && strcmp(name, path) == 0)
        return dst;

    switch (state) {
    case ASYNC_IDLE:
        if (locked == 0) {
            if (loaded == 1) {
                name[0] = '\0';
                loaded = 0;
            }
            if (loader_idle()) {
                state = ASYNC_WAITING;
                delay = ASYNC_REQUEST_DELAY;
            }
        }
        return NULL;
    case ASYNC_WAITING:
        if (--delay < 0) {
            if (loader_request(path, dst) == 1) {
                state = ASYNC_LOADING;
                strcpy(name, path);
            }
        }
        return NULL;
    case ASYNC_LOADING:
        if (strcmp(name, path) == 0) {
            if (!loader_idle())
                return NULL;
            loaded = 1;
            state = ASYNC_IDLE;
            if (done != NULL)
                done(ctx, dst);
            return dst;
        }
        loader_cancel();
        name[0] = '\0';
        state = ASYNC_CANCELLING;
        return NULL;
    case ASYNC_CANCELLING:
        if (loader_idle()) {
            delay = ASYNC_REQUEST_DELAY;
            state = ASYNC_IDLE;
        }
        return NULL;
    default:
        return NULL;
    }
}

// FUNC 0x002425F8 AsyncLoader::forget
//   ? name.  Clears the name (the loaded file is asked for again).
void AsyncLoader::forget()
{
    name[0] = '\0';
}

// FUNC 0x00242600 AsyncLoader::reset
//   ? name.  Cancels a load in progress, clears name / loaded / state /
//   delay.  No callers.
void AsyncLoader::reset()
{
    if (state == ASYNC_LOADING)
        loader_cancel();
    loaded = 0;
    state = ASYNC_IDLE;
    delay = 0;
    name[0] = '\0';                             /* last: retail's store order (searched) */
}

// FUNC 0x00242648 AsyncLoader::setLocked
//   ? name.  No callers.
void AsyncLoader::setLocked(int locked)
{
    this->locked = locked;
}

/* The loader: an uninitialised global, which egcs writes as zeros at the
 * end of the file (.sdata). */
AsyncLoader *g_00352FA8;                        /* 0x00352FA8 gp-0x4A48 */
