/* T220_object.h - gt2k_menu::Object, gt2k_menu::RootObject (translation unit
 * T220, unit u067).
 *
 * The menu object tree.  Every node is an Object: links to its siblings, its
 * children and its parent, a draw order (z) and a condition.  Objects die in
 * steps: kill() sets condition 1, the root's update walk steps it to 2 and 3
 * on the next frames, and the frame after that unlinks it and deletes it with
 * its children (so nothing drawn in the meantime refers to freed memory).
 * Every link change runs with interrupts suspended.  Object has its own
 * allocator: the current menu heap (T204), 16-aligned, retried (with a yield)
 * until it succeeds.
 *
 * Virtual slots of the whole Object hierarchy:
 *   [0] the destructor,
 *   [1] the per-frame update,
 *   [2] draw,
 *   [3] "takes part in the sorted draw" (RootObject::drawSorted): Object's
 *       returns 1; RawWindow (T264) and its 33 subclasses override it,
 *   [4] disappear: Object's kills the object, Transitive's (T216) runs the
 *       close script given (NULL: its default one).
 * The original names are known from the symbols of overrides: [1]
 * update(void) (TumbleCar, ProgressBar, IconView, ...), [2] render(void)
 * (TrophyItem, TitleView, ...), [4] disappear(void *) (DecayItem
 * 0x001258E8).  The slots keep the names virtual_1 / virtual_2 / virtual_4
 * (and slot 4 the parameter type const OperatorCode *, the close script)
 * because every unit and the derived classes' canonical headers (T216, T213,
 * T206, T244, T256, T257, T259) override them under these names: a rename
 * has to change all of them together. */
#ifndef GAME_T220_OBJECT_H
#define GAME_T220_OBJECT_H

#include "game/game.h"
#include <stddef.h>

namespace gt2k_menu {
struct OperatorCode;                        /* T217 operator scripts (pointers only) */

class Object;

/* The function forEach / forEachTree call on every node. */
typedef void (*ObjectFunc)(Object *obj, void *a, void *b, void *c);

/* vtable 0x003348D8 (5 slots).  typeinfo __tfQ29gt2k_menu6Object 0x001C0158
 * (__user_type_info 0x003533F0, no bases).  0x1C bytes of data, vptr +0x1C;
 * size 0x20 (RootObject's member at +0x20). */
class Object {
public:
    /* condition (u067's names; u072's OBJECT_CONDITION_DEAD). */
    enum {
        CONDITION_ALIVE  = 0,               /* updated and drawn */
        CONDITION_KILLED = 1,               /* kill(): skipped from now on */
        CONDITION_DYING  = 2,               /* the next frame */
        CONDITION_DEAD   = 3                /* the frame after: unlinked and deleted at the end of
                                               the update walk; its position no longer counts for
                                               the objects anchored to it (T259) */
    };

    Object();                               /* 0x001BF390 */
    static void *operator new(size_t size); /* 0x001BF3C0 the current menu heap (T204), 16-aligned;
                                               yields and retries until it gets the block */
    static void *operator new[](size_t size);  /* 0x001BF438 forwards to operator new (no callers) */
    static void  operator delete(void *p);  /* 0x001BF458 the current menu heap's free */
    static void  operator delete[](void *p);   /* 0x001BF488 forwards to operator delete (no callers) */

    void linkAfter(Object *after);          /* 0x001BF4A8 inserts this as the sibling after `after' */
    void attachTo(Object *p);               /* 0x001BF520 appends this to p's children */
    void attachSorted(Object *p);           /* 0x001BF578 inserts this among p's children by z */
    void detach();                          /* 0x001BF638 unlinks this from its parent and siblings */
    void disappearAll(const OperatorCode *script);  /* 0x001BF6F0 disappearTree, interrupts suspended */
    void disappearTree(const OperatorCode *script); /* 0x001BF748 virtual_4(script) on this and every
                                               node below */
    void forEach(ObjectFunc fn, void *a, void *b, void *c);     /* 0x001BF7B0 forEachTree */
    void forEachTree(ObjectFunc fn, void *a, void *b, void *c); /* 0x001BF7D0 fn on this and every node below */
    void kill();                            /* 0x001BF860 condition 0 -> 1 (the deferred delete) */

    /* The accessors are inline in the original: no callers, and
     * 0x001C0108..0x001C0150 are egcs's out-of-line copies before Object's
     * typeinfo function (RootObject::setUpdateDisable before RootObject's).
     * Declared last-first: egcs writes a class's inline members out in the
     * reverse order of their declaration (T220's frame info 0x002C99B0..
     * 0x002C9A20: isAlive first). */
    Object *getParent() { return parent; }          /* 0x001C0150 */
    Object *getChild() { return child; }            /* 0x001C0148 */
    Object *getSibling() { return sibling; }        /* 0x001C0140 */
    void setZ(f32 v) { z = v; }                     /* 0x001C0138 */
    void incZ(f32 d) { z = z + d; }                 /* 0x001C0128 */
    f32 getZ() const { return z; }                  /* 0x001C0120 */
    int Condition() const { return condition; }     /* 0x001C0118 */
    int isAlive() const { return condition == CONDITION_ALIVE; } /* 0x001C0108 */

    Object *prev;                           /* 0x00 previous sibling */
    Object *sibling;                        /* 0x04 next sibling */
    Object *child;                          /* 0x08 first child */
    Object *lastChild;                      /* 0x0C */
    Object *parent;                         /* 0x10 */
    int     condition;                      /* 0x14 0 alive; 1..3 dying (kill(), then the update walk) */
    f32     z;                              /* 0x18 draw order (drawSorted, attachSorted), ascending */
    /*      vptr                               0x1C */
    virtual ~Object();                      /* [0] +0x0C 0x001BFEC0 */
    virtual void virtual_1();               /* [1] +0x14 0x001BFEF0 per-frame update (empty) */
    virtual void virtual_2();               /* [2] +0x1C 0x001BFEF8 draw (empty) */
    virtual int  virtual_3();               /* [3] +0x24 0x001BFF00 drawn by drawSorted: returns 1 */
    virtual void virtual_4(const OperatorCode *script); /* [4] +0x2C 0x001BFF08 disappear: kill() */
};
EGCS_SIZE(gt2k_menu::Object, 0x20);

/* vtable 0x003348A8 (5 slots: [3] / [4] Object's).  typeinfo
 * __tfQ29gt2k_menu10RootObject 0x001C00B8 (si, base Object;
 * __si_type_info 0x004B2530).  The root of a tree: its update and draw walk
 * the tree (the dying nodes stepped and deleted by the update walk); its
 * destructor deletes the whole tree.  Size 0x24. */
class RootObject : public Object {
public:
    RootObject();                           /* 0x001BF878 */
    virtual ~RootObject();                  /* [0] +0x0C 0x001BF8B0 deleteChildren() */
    virtual void virtual_1();               /* [1] +0x14 0x001BF9D8 updateList(this, child) unless
                                               updateDisable, interrupts suspended */
    virtual void virtual_2();               /* [2] +0x1C 0x001BFB10 drawList(child) */

    void deleteChildren();                  /* 0x001BF980 deletes the tree below (interrupts suspended) */
    void updateList(Object *parent, Object *first); /* 0x001BFA38 alive: virtual_1, then the children;
                                               dying: 1 -> 2 -> 3, at 3 unlinked and deleted */
    void drawList(Object *first);           /* 0x001BFB38 alive ones: virtual_2, then the children */
    Object **collectDrawable(Object **out, Object *first); /* 0x001BFBB0 stores the alive nodes whose
                                               virtual_3() != 0 (and theirs below); returns the end */
    void drawSorted();                      /* 0x001BFC88 draws the tree in z order (qsort) */
    void F_001BFEA0();                      /* 0x001BFEA0 deleteChildren() (? an out-of-line inline) */
    int  countAlive();                      /* 0x001BFF28 alive nodes below (not under dying ones) */
    int  countAliveList(Object *first);     /* 0x001BFF80 */
    int  countAll();                        /* 0x001BFFF0 every node below */
    int  countAllList(Object *first);       /* 0x001C0048 */
    void setUpdateDisable(bool b) { updateDisable = b; } /* 0x001C00B0 inline in the original
                                               (PageManager::lockRoot expands it, u064) */

    int updateDisable;                      /* 0x20 non-zero: virtual_1 does nothing */
};
EGCS_SIZE(gt2k_menu::RootObject, 0x24);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand declarations). */
extern "C" {
void *__tfQ29gt2k_menu10RootObject(void);   /* 0x001C00B8 */
void *__tfQ29gt2k_menu6Object(void);        /* 0x001C0158 */
extern u32 __tiQ29gt2k_menu6Object[2];      /* 0x003533F0 gp-0x4600 its __user_type_info (2 words) */
}
#endif

#endif /* GAME_T220_OBJECT_H */
