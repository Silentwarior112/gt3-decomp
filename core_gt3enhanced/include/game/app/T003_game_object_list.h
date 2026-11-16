/* T003_game_object_list.h - GameObjectList and GameObjectController
 * (translation unit T003, unit u001), 0x00101A80..0x00101F78.
 *
 * A GameObjectController keeps a doubly linked list of GameObjectList nodes
 * (head) sorted by descending priority; its update / render walk the list,
 * and each node forwards them to the GameObject it wraps.  The list is
 * edited with interrupts disabled.  A node made with no controller uses the
 * default one (g_00350B28, gp-0x6EC8, set by setDefaultController; 0 at
 * start; only T003's code uses it).  Nothing in the binary constructs
 * either class outside T003. */
#ifndef GAME_T003_GAME_OBJECT_LIST_H
#define GAME_T003_GAME_OBJECT_LIST_H

#include "game/game.h"
#include "game/app/T002_render_thread.h"        /* base GameObject */

class GSBuffer;                             /* T514 (references only) */
class GameObjectController;

/* The default controller of new nodes (T003 .sdata). */
extern GameObjectController *g_00350B28;    /* 0x00350B28 gp-0x6EC8 */

/* vtable 0x003087F0 (6 slots: [3] GameObject's 0x00101A38).  typeinfo
 * __tf14GameObjectList 0x00101F28 (si, base GameObject; __si_type_info
 * 0x004B13C0).  vptr +0x00 (GameObject's).  Size 0x1C (the controller's
 * head follows at +0x1C). */
class GameObjectList : public GameObject {
public:
    /* controller NULL: the default controller.  Not linked, priority 0. */
    GameObjectList(GameObject *object, GameObjectController *controller); /* 0x00101A80 */
    virtual ~GameObjectList();              /* [0] +0x0C 0x00101AF0 unlink() (direct call) */
    virtual void update();                  /* [1] +0x14 0x00101B38 object->update() */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x00101B70 object->render(buf) */
    /* Inserts this node before the first node of its controller whose
     * priority is not greater (nothing if linked or without controller). */
    virtual void link();                    /* [4] +0x2C 0x00101BA8 */
    virtual void unlink();                  /* [5] +0x34 0x00101C58 */
    void setPriority(int priority);         /* 0x00101CD0 unlink, priority, link again if it was linked */
    /* Inline: no caller in the binary; 0x00101F20 is the copy egcs emits
     * with the vtable in T003, after GameObjectController's typeinfo
     * function (the frame info's order). */
    static void setDefaultController(GameObjectController *c) { g_00350B28 = c; }  /* 0x00101F20 */

    int                   linked;           /* 0x04 */
    int                   priority;         /* 0x08 higher first */
    GameObjectController *controller;       /* 0x0C */
    GameObjectList       *prev;             /* 0x10 */
    GameObjectList       *next;             /* 0x14 */
    GameObject           *object;           /* 0x18 may be NULL */
};
EGCS_SIZE(GameObjectList, 0x1C);

/* vtable 0x003087B8 (6 slots: [3] GameObject's, [4] / [5] GameObjectList's).
 * typeinfo __tf20GameObjectController 0x00101ED0 (si, base GameObjectList;
 * __si_type_info 0x004B13D0).  Size 0x20.  Itself a node of no controller
 * (GameObjectList(NULL, NULL)). */
class GameObjectController : public GameObjectList {
public:
    GameObjectController();                 /* 0x00101D58 */
    virtual ~GameObjectController();        /* [0] +0x0C 0x00101D98 every node unlinked, its controller NULL */
    virtual void update();                  /* [1] +0x14 0x00101E18 every node's update() */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x00101E68 every node's render(buf) */
    GameObjectList *getHead();              /* 0x00101EC0 */
    void setHead(GameObjectList *head);     /* 0x00101EC8 */

    GameObjectList *head;                   /* 0x1C */
};
EGCS_SIZE(GameObjectController, 0x20);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tf20GameObjectController(void);     /* 0x00101ED0 */
void *__tf14GameObjectList(void);           /* 0x00101F28 */
extern u32 __ti14GameObjectList[3];         /* 0x004B13C0 (__si_type_info) */
extern u32 __ti20GameObjectController[3];   /* 0x004B13D0 */
}
#endif

#endif /* GAME_T003_GAME_OBJECT_LIST_H */
