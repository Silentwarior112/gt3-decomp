/* T220 - gt2k_menu::Object and gt2k_menu::RootObject.  0x001BF390..0x001C0198.
 *
 * The menu object tree.  Every link change runs with interrupts suspended
 * (intr_suspend / ei).  Objects die in steps: kill() sets condition 1, the
 * root's update walk steps it to 2 and 3 on the next frames, and the frame
 * after that unlinks it and deletes it with its children. */
/* Retail's .rodata (0x003348A8..0x00334940): the two vtables, then the type
 * names RootObject, Object: the original saw no other polymorphic class. */
#include "pdistd/allocator.h"               /* intr_suspend */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_resume, yield_low_priority */
#include "runtime/newlib/G007_stdio.h"                /* qsort */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current */
#include "game/menu/framework/T220_object.h"
#include "game/menu/framework/menu_widgets_internal.h"

namespace gt2k_menu {

/* The object conditions: Object::CONDITION_* (T220). */

static void delete_tree(Object *first);
static void destroy_object(Object *obj);
static void delete_list(Object *first);
static int  compare_z(const void *a, const void *b);

// FUNC 0x001BF390 gt2k_menu::Object::Object
Object::Object()
{
    prev = 0;
    sibling = 0;
    child = 0;
    lastChild = 0;
    parent = 0;
    condition = CONDITION_ALIVE;
    z = 0;          /* sw $zero */
}

/* The class allocator: the current menu heap, 16-aligned; yields and tries
 * again until it gets the block.  The loop flag lives in $s1 and is never
 * cleared.
 * An exact form, not necessarily the original: retail keeps the constant 1
 * in $s1 and tests it on every pass (`beqz s1').  With `while (retry)'
 * egcs's last GCSE pass propagates the constant into the jump and drops the
 * test (-fno-gcse keeps it); a comparison with a constant other than 0 is a
 * store-flag insn, which that pass cannot fold, and combine later makes it
 * the plain test of $s1.  retry is 1 throughout, so `retry < 2' is always
 * true, as `retry' was: the same loop.  (The original perhaps compared a
 * retry counter with a limit and never counted.)  Same with the reference
 * compiler 991111-01. */
// FUNC 0x001BF3C0 gt2k_menu::Object::operator new
void *Object::operator new(size_t size)
{
    void *block = 0;
    int retry = 1;
    while (retry < 2) {
        block = menu_heap_current()->allocate(size, 0x10);
        if (block != 0)
            break;
        yield_low_priority();
    }
    return block;
}

/* No callers: forwards to operator new. */
// FUNC 0x001BF438 gt2k_menu::Object::operator new[]
void *Object::operator new[](size_t size)
{
    return Object::operator new(size);
}

// FUNC 0x001BF458 gt2k_menu::Object::operator delete
void Object::operator delete(void *p)
{
    menu_heap_current()->free(p);
}

/* No callers: forwards to operator delete. */
// FUNC 0x001BF488 gt2k_menu::Object::operator delete[]
void Object::operator delete[](void *p)
{
    Object::operator delete(p);
}

/* Inserts this as the sibling after `after', under after's parent. */
// FUNC 0x001BF4A8 gt2k_menu::Object::linkAfter
void Object::linkAfter(Object *after)
{
    int intr = intr_suspend();
    prev = after;
    sibling = after->sibling;
    if (after->sibling != 0)
        after->sibling->prev = this;
    Object *p = after->parent;
    after->sibling = this;
    parent = p;
    if (p->lastChild == prev)
        p->lastChild = this;
    intr_resume(intr);
}

/* Appends this as the last child of p. */
// FUNC 0x001BF520 gt2k_menu::Object::attachTo
void Object::attachTo(Object *p)
{
    int intr = intr_suspend();
    Object *last = p->lastChild;
    if (last != 0) {
        prev = last;
        sibling = 0;
        last->sibling = this;
    } else {
        p->child = this;
    }
    p->lastChild = this;
    parent = p;
    intr_resume(intr);
}

/* Inserts this among p's children before the first one with a greater z
 * (after the equal ones). */
// FUNC 0x001BF578 gt2k_menu::Object::attachSorted
void Object::attachSorted(Object *p)
{
    int intr = intr_suspend();
    Object *next = p->child;
    if (next != 0) {
        while (next != 0 && next->z <= z)
            next = next->sibling;
        if (next == 0) {                    /* after the last child */
            next = p->lastChild;
            sibling = 0;
            prev = next;
            next->sibling = this;
            p->lastChild = this;
        } else {                            /* before next */
            prev = next->prev;
            sibling = next;
            next->prev = this;
            if (prev != 0)
                prev->sibling = this;
            else
                p->child = this;
        }
    } else {
        p->child = this;
        p->lastChild = this;
    }
    parent = p;
    intr_resume(intr);
}

/* Unlinks this from its parent and siblings.  Quirk: prev is cleared before
 * the next sibling's back link is fixed, so next->prev always becomes 0. */
// FUNC 0x001BF638 gt2k_menu::Object::detach
void Object::detach()
{
    int intr = intr_suspend();
    if (parent != 0) {
        if (parent->child == this)
            parent->child = sibling;
        if (parent->lastChild == this)
            parent->lastChild = prev;
        parent = 0;
    }
    if (prev != 0) {
        if (prev->sibling == this)
            prev->sibling = sibling;
        prev = 0;
    }
    if (sibling != 0) {
        if (sibling->prev == this)
            sibling->prev = prev;   /* already 0 (see above) */
        sibling = 0;
    }
    intr_resume(intr);
}

// FUNC 0x001BF6F0 gt2k_menu::Object::disappearAll
void Object::disappearAll(const OperatorCode *script)
{
    int intr = intr_suspend();
    disappearTree(script);
    intr_resume(intr);
}

/* virtual_4(script) on this, then on every child's subtree. */
// FUNC 0x001BF748 gt2k_menu::Object::disappearTree
void Object::disappearTree(const OperatorCode *script)
{
    virtual_4(script);
    if (child != 0) {           /* retail tests child once more before the loop */
        for (Object *node = child; node != 0; node = node->sibling)
            node->disappearTree(script);
    }
}

// FUNC 0x001BF7B0 gt2k_menu::Object::forEach
void Object::forEach(ObjectFunc fn, void *a, void *b, void *c)
{
    forEachTree(fn, a, b, c);
}

/* fn(obj, a, b, c) on this, then on every child's subtree. */
// FUNC 0x001BF7D0 gt2k_menu::Object::forEachTree
void Object::forEachTree(ObjectFunc fn, void *a, void *b, void *c)
{
    fn(this, a, b, c);
    if (child != 0) {           /* as disappearTree */
        for (Object *node = child; node != 0; node = node->sibling)
            node->forEachTree(fn, a, b, c);
    }
}

// FUNC 0x001BF860 gt2k_menu::Object::kill
void Object::kill()
{
    if (condition == CONDITION_ALIVE)
        condition = CONDITION_KILLED;
}

// FUNC 0x001BF878 gt2k_menu::RootObject::RootObject
RootObject::RootObject()
    : Object()
{
    updateDisable = 0;
}

/* The RootObject vptr, deleteChildren(), ~Object (flags 0), then if
 * (flags & 1) the class operator delete. */
// FUNC 0x001BF8B0 gt2k_menu::RootObject::~RootObject
RootObject::~RootObject()
{
    deleteChildren();
    /* implicit: ~Object (base destructor), operator delete (deleting destructor) */
}

/* Deletes first, its following siblings and all their children (depth
 * first), clearing the links as it goes. */
// FUNC 0x001BF910 delete_tree
static void delete_tree(Object *first)
{
    if (first->sibling != 0) {
        delete_tree(first->sibling);
        first->sibling = 0;
    }
    if (first->child != 0) {
        delete_tree(first->child);
        first->child = 0;
    }
    delete first;   /* virtual destructor, flags 3 */
}

// FUNC 0x001BF980 gt2k_menu::RootObject::deleteChildren
void RootObject::deleteChildren()
{
    if (child != 0) {
        int intr = intr_suspend();
        delete_tree(child);
        child = 0;
        intr_resume(intr);
    }
}

// FUNC 0x001BF9D8 gt2k_menu::RootObject::virtual_1
void RootObject::virtual_1()
{
    if (updateDisable == 0) {
        int intr = intr_suspend();
        if (child != 0)
            updateList(this, child);
        intr_resume(intr);
    }
}

/* Update walk: an alive object is updated, then its children; a dying one
 * steps 1 -> 2 -> 3, and at 3 it is unlinked and deleted.  `parent' is
 * only passed down. */
// FUNC 0x001BFA38 gt2k_menu::RootObject::updateList
void RootObject::updateList(Object *parent, Object *first)
{
    Object *node = first;
    do {
        int cond = node->condition;
        Object *next = node->sibling;
        if (cond == CONDITION_ALIVE) {
            node->virtual_1();
            if (node->child != 0)
                updateList(node, node->child);
        } else if (cond == CONDITION_KILLED) {
            node->condition = CONDITION_DYING;
        } else if (cond == CONDITION_DYING) {
            node->condition = CONDITION_DEAD;
        } else if (cond == CONDITION_DEAD) {
            destroy_object(node);
        }
        node = next;
    } while (node != 0);
}

// FUNC 0x001BFB10 gt2k_menu::RootObject::virtual_2
void RootObject::virtual_2()
{
    if (child != 0)
        drawList(child);
}

/* Draw walk in tree order: alive objects, then their children. */
// FUNC 0x001BFB38 gt2k_menu::RootObject::drawList
void RootObject::drawList(Object *first)
{
    Object *node = first;
    do {
        if (node->condition == CONDITION_ALIVE) {
            node->virtual_2();
            if (node->child != 0)
                drawList(node->child);
        }
        node = node->sibling;
    } while (node != 0);
}

/* Stores every alive object whose virtual_3() is non-zero (and the same of
 * its children) at out; returns the end. */
// FUNC 0x001BFBB0 gt2k_menu::RootObject::collectDrawable
Object **RootObject::collectDrawable(Object **out, Object *first)
{
    for (Object *node = first; node != 0; node = node->sibling) {
        if (node->condition == CONDITION_ALIVE && node->virtual_3() != 0) {
            *out++ = node;
            if (node->child != 0)
                out = collectDrawable(out, node->child);
        }
    }
    return out;
}

/* qsort comparator: by z, ascending. */
// FUNC 0x001BFC48 compare_z
static int compare_z(const void *a, const void *b)
{
    f32 za = (*(Object *const *)a)->z;
    f32 zb = (*(Object *const *)b)->z;
    if (za < zb)
        return -1;
    if (zb < za)
        return 1;
    return 0;
}

/* Draws the tree sorted by z: collects the drawable objects into an alloca
 * array (sized by countAlive()), sorts it when there are two or more, and
 * draws them in order. */
// FUNC 0x001BFC88 gt2k_menu::RootObject::drawSorted
void RootObject::drawSorted()
{
    if (child != 0) {
        Object **list = (Object **)__builtin_alloca(countAlive() * 4);
        int count = collectDrawable(list, child) - list;
        if (count >= 2)
            qsort(list, count, 4, compare_z);
        for (int i = 0; i < count; i++)
            list[i]->virtual_2();
    }
}

/* Unlinks obj (without clearing its own links), deletes its children and
 * then obj itself; interrupts suspended. */
// FUNC 0x001BFD50 destroy_object
static void destroy_object(Object *obj)
{
    if (obj != 0) {
        int intr = intr_suspend();
        if (obj->parent != 0) {
            if (obj->parent->child == obj)
                obj->parent->child = obj->sibling;
            if (obj->parent->lastChild == obj)
                obj->parent->lastChild = obj->prev;
        }
        if (obj->prev != 0 && obj->prev->sibling == obj)
            obj->prev->sibling = obj->sibling;
        if (obj->sibling != 0 && obj->sibling->prev == obj)
            obj->sibling->prev = obj->prev;
        if (obj->child != 0)
            delete_list(obj->child);
        delete obj;     /* virtual destructor, flags 3 */
        intr_resume(intr);
    }
}

/* Deletes first, its following siblings and all their children (links left
 * as they are). */
// FUNC 0x001BFE38 delete_list
static void delete_list(Object *first)
{
    if (first->sibling != 0)
        delete_list(first->sibling);
    if (first->child != 0)
        delete_list(first->child);
    delete first;   /* virtual destructor, flags 3 */
}

/* No callers; deleteChildren() again (? an inline member emitted out of
 * line, its name unknown). */
// FUNC 0x001BFEA0 gt2k_menu::RootObject::F_001BFEA0
void RootObject::F_001BFEA0()
{
    deleteChildren();
}

/* Stores the Object vptr; the deleting form calls operator delete. */
// FUNC 0x001BFEC0 gt2k_menu::Object::~Object
Object::~Object()
{
}

// FUNC 0x001BFEF0 gt2k_menu::Object::virtual_1
void Object::virtual_1()
{
}

// FUNC 0x001BFEF8 gt2k_menu::Object::virtual_2
void Object::virtual_2()
{
}

/* index.txt: gt2k_menu::RootObject::virtual_3; it is slot 3 of Object's own
 * vtable too, so it is Object's (inherited by RootObject). */
// FUNC 0x001BFF00 gt2k_menu::Object::virtual_3
int Object::virtual_3()
{
    return 1;
}

/* index.txt: gt2k_menu::RootObject::virtual_4 (also slot 4 of Object's
 * vtable, as above).  The script is not used. */
// FUNC 0x001BFF08 gt2k_menu::Object::virtual_4
void Object::virtual_4(const OperatorCode *script)
{
    kill();
}

/* Number of alive objects under this (dying ones and their children are
 * not counted). */
// FUNC 0x001BFF28 gt2k_menu::RootObject::countAlive
int RootObject::countAlive()
{
    int count = 0;
    if (child != 0) {
        int intr = intr_suspend();
        count = countAliveList(child);
        intr_resume(intr);
    }
    return count;
}

// FUNC 0x001BFF80 gt2k_menu::RootObject::countAliveList
int RootObject::countAliveList(Object *first)
{
    int count = 0;
    for (Object *node = first; node != 0; node = node->sibling) {
        if (node->condition == CONDITION_ALIVE) {
            count++;
            if (node->child != 0)
                count += countAliveList(node->child);
        }
    }
    return count;
}

/* Number of objects under this. */
// FUNC 0x001BFFF0 gt2k_menu::RootObject::countAll
int RootObject::countAll()
{
    int count = 0;
    if (child != 0) {
        int intr = intr_suspend();
        count = countAllList(child);
        intr_resume(intr);
    }
    return count;
}

// FUNC 0x001C0048 gt2k_menu::RootObject::countAllList
int RootObject::countAllList(Object *first)
{
    int count = 0;
    Object *node = first;
    do {
        count++;
        if (node->child != 0)
            count += countAllList(node->child);
        node = node->sibling;
    } while (node != 0);
    return count;
}

// FUNC 0x001C00B0 gt2k_menu::RootObject::setUpdateDisable(bool)
/* inline in game/menu/framework/T220_object.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Object). */
// FUNC 0x001C00B8 __tfQ29gt2k_menu10RootObject
extern "C" void *__tfQ29gt2k_menu10RootObject(void)
{
    if (__tiQ29gt2k_menu10RootObject[0] == 0) {
        __tfQ29gt2k_menu6Object();
        __rtti_si(__tiQ29gt2k_menu10RootObject, "Q29gt2k_menu10RootObject",
                  (const type_info *)__tiQ29gt2k_menu6Object);
    }
    return __tiQ29gt2k_menu10RootObject;
}
#endif

namespace gt2k_menu {

// FUNC 0x001C0108 gt2k_menu::Object::isAlive(void) const
/* inline in game/menu/framework/T220_object.h; egcs emits this copy */

// FUNC 0x001C0118 gt2k_menu::Object::Condition(void) const
/* inline in game/menu/framework/T220_object.h; egcs emits this copy */

// FUNC 0x001C0120 gt2k_menu::Object::getZ(void) const
/* inline in game/menu/framework/T220_object.h; egcs emits this copy */

// FUNC 0x001C0128 gt2k_menu::Object::incZ(float)
/* inline in game/menu/framework/T220_object.h; egcs emits this copy */

// FUNC 0x001C0138 gt2k_menu::Object::setZ(float)
/* inline in game/menu/framework/T220_object.h; egcs emits this copy */

// FUNC 0x001C0140 gt2k_menu::Object::getSibling(void)
/* inline in game/menu/framework/T220_object.h; egcs emits this copy */

// FUNC 0x001C0148 gt2k_menu::Object::getChild(void)
/* inline in game/menu/framework/T220_object.h; egcs emits this copy */

// FUNC 0x001C0150 gt2k_menu::Object::getParent(void)
/* inline in game/menu/framework/T220_object.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__user_type_info: no bases). */
// FUNC 0x001C0158 __tfQ29gt2k_menu6Object
extern "C" void *__tfQ29gt2k_menu6Object(void)
{
    if (__tiQ29gt2k_menu6Object[0] == 0)
        __rtti_user(__tiQ29gt2k_menu6Object, "Q29gt2k_menu6Object");
    return __tiQ29gt2k_menu6Object;
}
#endif
