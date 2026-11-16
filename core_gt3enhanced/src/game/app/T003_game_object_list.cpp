/* T003 - GameObjectList / GameObjectController.  0x00101A80..0x00101F78.
 *
 * A GameObjectController keeps a doubly linked list of GameObjectList nodes
 * (head at +0x1C) sorted by descending priority; update and render walk it.
 * Each node forwards update / render to the GameObject it wraps.  The list
 * is edited with interrupts disabled (intr_suspend ... `ei'). */
#include "cxxrt.h"                          /* type_info, __rtti_si (GCC 14's typeinfo functions) */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend / intr_resume */
#include "game/app/T003_game_object_list.h"

/* .sdata (-G8): the default controller of new nodes (file-local). */
GameObjectController *g_00350B28 = NULL;    /* 0x00350B28 gp-0x6EC8 */

/* object may be NULL; controller NULL means the default controller
 * g_00350B28. */
// FUNC 0x00101A80 GameObjectList::GameObjectList
GameObjectList::GameObjectList(GameObject *object, GameObjectController *controller)
{
    this->object     = object;
    this->controller = (controller != NULL) ? controller : g_00350B28;
    linked   = 0;
    priority = 0;
    prev     = NULL;
    next     = NULL;
}

// FUNC 0x00101AF0 GameObjectList::~GameObjectList
GameObjectList::~GameObjectList()
{
    /* Bound statically in a destructor (a direct call to 0x00101C58). */
    unlink();
    /* then GameObject::~GameObject with the flags */
    /* implicit: ~GameObject (base destructor) */
}

// FUNC 0x00101B38 GameObjectList::update
void GameObjectList::update()
{
    if (object != NULL)
        object->update();
}

// FUNC 0x00101B70 GameObjectList::render
void GameObjectList::render(GSBuffer &buf)
{
    if (object != NULL)
        object->render(buf);
}

/* index.txt files it under GameObjectController (slot 4 of both vtables):
 * GameObjectList's.  Inserts this node before the first node whose
 * priority is not greater than its own. */
// FUNC 0x00101BA8 GameObjectList::link
void GameObjectList::link()
{
    if (linked != 0)
        return;
    GameObjectController *owner = controller;
    if (owner == NULL)
        return;

    int intr = intr_suspend();
    GameObjectList *before = NULL;
    GameObjectList *after  = owner->head;
    while (after != NULL && priority < after->priority) {
        before = after;
        after  = after->next;
    }
    if (before == NULL)
        owner->head = this;
    else
        before->next = this;
    if (after != NULL)
        after->prev = this;
    prev   = before;
    next   = after;
    linked = 1;
    intr_resume(intr);
}

/* GameObjectList's (slot 5 of both vtables, see link). */
// FUNC 0x00101C58 GameObjectList::unlink
void GameObjectList::unlink()
{
    if (linked == 0)
        return;
    GameObjectController *owner = controller;
    if (owner == NULL)
        return;

    int intr = intr_suspend();
    GameObjectList *before = prev;
    GameObjectList *after  = next;
    if (before == NULL)
        owner->head = after;
    else
        before->next = after;
    if (after != NULL)
        after->prev = before;
    linked = 0;
    intr_resume(intr);
}

/* Re-sorts the node: unlink (virtual), the priority, link again (virtual)
 * if it was linked; all with interrupts off. */
// FUNC 0x00101CD0 GameObjectList::setPriority
void GameObjectList::setPriority(int priority)
{
    int intr = intr_suspend();
    int wasLinked = linked;
    unlink();
    this->priority = priority;
    if (wasLinked != 0)
        link();
    intr_resume(intr);
}

// FUNC 0x00101D58 GameObjectController::GameObjectController
GameObjectController::GameObjectController()
    : GameObjectList(NULL, NULL)
{
    head = NULL;
}

/* Detaches every node (unlink, controller NULL). */
// FUNC 0x00101D98 GameObjectController::~GameObjectController
GameObjectController::~GameObjectController()
{
    for (GameObjectList *node = head; node != NULL; node = node->next) {
        node->unlink();
        node->controller = NULL;
    }
    /* then GameObjectList::~GameObjectList with the flags */
}

// FUNC 0x00101E18 GameObjectController::update
void GameObjectController::update()
{
    for (GameObjectList *node = head; node != NULL; node = node->next)
        node->update();
}

// FUNC 0x00101E68 GameObjectController::render
void GameObjectController::render(GSBuffer &buf)
{
    for (GameObjectList *node = head; node != NULL; node = node->next)
        node->render(buf);
}

// FUNC 0x00101EC0 GameObjectController::getHead
GameObjectList *GameObjectController::getHead()
{
    return head;
}

// FUNC 0x00101EC8 GameObjectController::setHead
void GameObjectController::setHead(GameObjectList *head)
{
    this->head = head;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * GameObjectList). */
// FUNC 0x00101ED0 __tf20GameObjectController
extern "C" void *__tf20GameObjectController(void)
{
    if (__ti20GameObjectController[0] == 0) {
        __tf14GameObjectList();
        __rtti_si(__ti20GameObjectController, "20GameObjectController",
                  (const type_info *)__ti14GameObjectList);
    }
    return __ti20GameObjectController;
}
#endif

// FUNC 0x00101F20 GameObjectList::setDefaultController
/* inline in game/app/T003_game_object_list.h; egcs emits this copy with the
 * vtables at the end of the unit (GameObjectController's typeinfo function,
 * then GameObjectList's members, then its typeinfo function). */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base GameObject). */
// FUNC 0x00101F28 __tf14GameObjectList
extern "C" void *__tf14GameObjectList(void)
{
    if (__ti14GameObjectList[0] == 0) {
        __tf10GameObject();
        __rtti_si(__ti14GameObjectList, "14GameObjectList",
                  (const type_info *)__ti10GameObject);
    }
    return __ti14GameObjectList;
}
#endif
