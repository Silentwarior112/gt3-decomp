/* T272 - the class registry entries of the 16 Raw* window classes.
 * 0x001D2FD0..0x001D3E40.  Per class (in alphabetical order), from one
 * macro of the original: first its record (.sdata: the name pointer slot 8
 * returns, the RawWindowClass {name, create}, the registrar), then the class
 * name (slot 8), the layout factory (eight words packed into a
 * RawWindowArgs), the registry's create function (operator new + the
 * constructor, if canCreate(args)), canCreate (1, the arguments unused).
 * egcs builds / destroys the 16 registrars itself (T272_static_init /
 * _dtor at the end).  Under -G8 the 8-byte name "RawList" is itself in
 * .sdata (0x00352790), before its pointer; the other names open .rodata in
 * this order.
 * Data: the code reaches its .rodata only through the .sdata records (the
 * name pointers), so the first name, "RawCheckBox" at 0x00337E58, anchors
 * the file's .rodata (without it T275's range ran over it). */
// DATA_START 0x00337E58
#include "game/menu/framework/menu_draw_state_internal.h"

/* ------------------------------------------------------------------------ */
/* RawCheckBox                                                              */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00352760 = "RawCheckBox";                     /* 0x00352760 */
gt2k_menu::RawWindowClass g_00352768 =                      /* 0x00352768 */
    { "RawCheckBox", gt2k_menu::RawCheckBox::create };
gt2k_menu::RawWindowRegister g_00352770(&g_00352768);       /* 0x00352770 */

namespace gt2k_menu {

// FUNC 0x001D2FD0 gt2k_menu::RawCheckBox::virtual_8
const char *RawCheckBox::virtual_8()
{
    return g_00352760;      /* "RawCheckBox" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D2FD8 gt2k_menu::RawCheckBox::factory
RawWindow *RawCheckBox::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00352768.create). */
// FUNC 0x001D3018 gt2k_menu::RawCheckBox::create
RawWindow *RawCheckBox::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawCheckBox(*args); /* implicit: Object::operator new(0x21C); operator delete if
                                          the constructor throws */
    return NULL;
}

// FUNC 0x001D3090 gt2k_menu::RawCheckBox::canCreate
int RawCheckBox::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawLabel                                                                 */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00352778 = "RawLabel";                        /* 0x00352778 */
gt2k_menu::RawWindowClass g_00352780 =                      /* 0x00352780 */
    { "RawLabel", gt2k_menu::RawLabel::create };
gt2k_menu::RawWindowRegister g_00352788(&g_00352780);       /* 0x00352788 */

namespace gt2k_menu {

// FUNC 0x001D3098 gt2k_menu::RawLabel::virtual_8
const char *RawLabel::virtual_8()
{
    return g_00352778;      /* "RawLabel" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D30A0 gt2k_menu::RawLabel::factory
RawWindow *RawLabel::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                             int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00352780.create). */
// FUNC 0x001D30E0 gt2k_menu::RawLabel::create
RawWindow *RawLabel::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawLabel(*args); /* implicit: Object::operator new(0x204); operator delete if
                                       the constructor throws */
    return NULL;
}

// FUNC 0x001D3158 gt2k_menu::RawLabel::canCreate
int RawLabel::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawList                                                                  */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser); the
 * name string itself is .sdata 0x00352790 (8 bytes, -G8). */
const char *g_00352798 = "RawList";                         /* 0x00352798 */
gt2k_menu::RawWindowClass g_003527A0 =                      /* 0x003527A0 */
    { "RawList", gt2k_menu::RawList::create };
gt2k_menu::RawWindowRegister g_003527A8(&g_003527A0);       /* 0x003527A8 */

namespace gt2k_menu {

// FUNC 0x001D3160 gt2k_menu::RawList::virtual_8
const char *RawList::virtual_8()
{
    return g_00352798;      /* "RawList" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3168 gt2k_menu::RawList::factory
RawWindow *RawList::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                            int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_003527A0.create). */
// FUNC 0x001D31A8 gt2k_menu::RawList::create
RawWindow *RawList::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawList(*args); /* implicit: Object::operator new(0x248); operator delete if
                                      the constructor throws */
    return NULL;
}

// FUNC 0x001D3220 gt2k_menu::RawList::canCreate
int RawList::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListFixed                                                             */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_003527B0 = "RawListFixed";                    /* 0x003527B0 */
gt2k_menu::RawWindowClass g_003527B8 =                      /* 0x003527B8 */
    { "RawListFixed", gt2k_menu::RawListFixed::create };
gt2k_menu::RawWindowRegister g_003527C0(&g_003527B8);       /* 0x003527C0 */

namespace gt2k_menu {

// FUNC 0x001D3228 gt2k_menu::RawListFixed::virtual_8
const char *RawListFixed::virtual_8()
{
    return g_003527B0;      /* "RawListFixed" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3230 gt2k_menu::RawListFixed::factory
RawWindow *RawListFixed::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                 int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_003527B8.create). */
// FUNC 0x001D3270 gt2k_menu::RawListFixed::create
RawWindow *RawListFixed::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListFixed(*args); /* implicit: Object::operator new(0x248); operator delete if
                                           the constructor throws */
    return NULL;
}

// FUNC 0x001D32E8 gt2k_menu::RawListFixed::canCreate
int RawListFixed::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListH                                                                 */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_003527C8 = "RawListH";                        /* 0x003527C8 */
gt2k_menu::RawWindowClass g_003527D0 =                      /* 0x003527D0 */
    { "RawListH", gt2k_menu::RawListH::create };
gt2k_menu::RawWindowRegister g_003527D8(&g_003527D0);       /* 0x003527D8 */

namespace gt2k_menu {

// FUNC 0x001D32F0 gt2k_menu::RawListH::virtual_8
const char *RawListH::virtual_8()
{
    return g_003527C8;      /* "RawListH" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D32F8 gt2k_menu::RawListH::factory
RawWindow *RawListH::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                             int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_003527D0.create). */
// FUNC 0x001D3338 gt2k_menu::RawListH::create
RawWindow *RawListH::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListH(*args); /* implicit: Object::operator new(0x248); operator delete if
                                       the constructor throws */
    return NULL;
}

// FUNC 0x001D33B0 gt2k_menu::RawListH::canCreate
int RawListH::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListHFixed                                                            */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_003527E0 = "RawListHFixed";                   /* 0x003527E0 */
gt2k_menu::RawWindowClass g_003527E8 =                      /* 0x003527E8 */
    { "RawListHFixed", gt2k_menu::RawListHFixed::create };
gt2k_menu::RawWindowRegister g_003527F0(&g_003527E8);       /* 0x003527F0 */

namespace gt2k_menu {

// FUNC 0x001D33B8 gt2k_menu::RawListHFixed::virtual_8
const char *RawListHFixed::virtual_8()
{
    return g_003527E0;      /* "RawListHFixed" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D33C0 gt2k_menu::RawListHFixed::factory
RawWindow *RawListHFixed::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                  int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_003527E8.create). */
// FUNC 0x001D3400 gt2k_menu::RawListHFixed::create
RawWindow *RawListHFixed::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListHFixed(*args); /* implicit: Object::operator new(0x248); operator delete if
                                            the constructor throws */
    return NULL;
}

// FUNC 0x001D3478 gt2k_menu::RawListHFixed::canCreate
int RawListHFixed::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListHPos                                                              */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_003527F8 = "RawListHPos";                     /* 0x003527F8 */
gt2k_menu::RawWindowClass g_00352800 =                      /* 0x00352800 */
    { "RawListHPos", gt2k_menu::RawListHPos::create };
gt2k_menu::RawWindowRegister g_00352808(&g_00352800);       /* 0x00352808 */

namespace gt2k_menu {

// FUNC 0x001D3480 gt2k_menu::RawListHPos::virtual_8
const char *RawListHPos::virtual_8()
{
    return g_003527F8;      /* "RawListHPos" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3488 gt2k_menu::RawListHPos::factory
RawWindow *RawListHPos::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00352800.create). */
// FUNC 0x001D34C8 gt2k_menu::RawListHPos::create
RawWindow *RawListHPos::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListHPos(*args); /* implicit: Object::operator new(0x248); operator delete if
                                          the constructor throws */
    return NULL;
}

// FUNC 0x001D3540 gt2k_menu::RawListHPos::canCreate
int RawListHPos::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListHSize                                                             */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00352810 = "RawListHSize";                    /* 0x00352810 */
gt2k_menu::RawWindowClass g_00352818 =                      /* 0x00352818 */
    { "RawListHSize", gt2k_menu::RawListHSize::create };
gt2k_menu::RawWindowRegister g_00352820(&g_00352818);       /* 0x00352820 */

namespace gt2k_menu {

// FUNC 0x001D3548 gt2k_menu::RawListHSize::virtual_8
const char *RawListHSize::virtual_8()
{
    return g_00352810;      /* "RawListHSize" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3550 gt2k_menu::RawListHSize::factory
RawWindow *RawListHSize::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                 int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00352818.create). */
// FUNC 0x001D3590 gt2k_menu::RawListHSize::create
RawWindow *RawListHSize::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListHSize(*args); /* implicit: Object::operator new(0x248); operator delete if
                                           the constructor throws */
    return NULL;
}

// FUNC 0x001D3608 gt2k_menu::RawListHSize::canCreate
int RawListHSize::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListItem                                                              */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00352828 = "RawListItem";                     /* 0x00352828 */
gt2k_menu::RawWindowClass g_00352830 =                      /* 0x00352830 */
    { "RawListItem", gt2k_menu::RawListItem::create };
gt2k_menu::RawWindowRegister g_00352838(&g_00352830);       /* 0x00352838 */

namespace gt2k_menu {

// FUNC 0x001D3610 gt2k_menu::RawListItem::virtual_8
const char *RawListItem::virtual_8()
{
    return g_00352828;      /* "RawListItem" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3618 gt2k_menu::RawListItem::factory
RawWindow *RawListItem::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00352830.create). */
// FUNC 0x001D3658 gt2k_menu::RawListItem::create
RawWindow *RawListItem::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListItem(*args); /* implicit: Object::operator new(0x204); operator delete if
                                          the constructor throws */
    return NULL;
}

// FUNC 0x001D36D0 gt2k_menu::RawListItem::canCreate
int RawListItem::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListItemFixed                                                         */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00352840 = "RawListItemFixed";                /* 0x00352840 */
gt2k_menu::RawWindowClass g_00352848 =                      /* 0x00352848 */
    { "RawListItemFixed", gt2k_menu::RawListItemFixed::create };
gt2k_menu::RawWindowRegister g_00352850(&g_00352848);       /* 0x00352850 */

namespace gt2k_menu {

// FUNC 0x001D36D8 gt2k_menu::RawListItemFixed::virtual_8
const char *RawListItemFixed::virtual_8()
{
    return g_00352840;      /* "RawListItemFixed" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D36E0 gt2k_menu::RawListItemFixed::factory
RawWindow *RawListItemFixed::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                     int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00352848.create). */
// FUNC 0x001D3720 gt2k_menu::RawListItemFixed::create
RawWindow *RawListItemFixed::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListItemFixed(*args); /* implicit: Object::operator new(0x204); operator delete if
                                               the constructor throws */
    return NULL;
}

// FUNC 0x001D3798 gt2k_menu::RawListItemFixed::canCreate
int RawListItemFixed::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListV                                                                 */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00352858 = "RawListV";                        /* 0x00352858 */
gt2k_menu::RawWindowClass g_00352860 =                      /* 0x00352860 */
    { "RawListV", gt2k_menu::RawListV::create };
gt2k_menu::RawWindowRegister g_00352868(&g_00352860);       /* 0x00352868 */

namespace gt2k_menu {

// FUNC 0x001D37A0 gt2k_menu::RawListV::virtual_8
const char *RawListV::virtual_8()
{
    return g_00352858;      /* "RawListV" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D37A8 gt2k_menu::RawListV::factory
RawWindow *RawListV::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                             int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00352860.create). */
// FUNC 0x001D37E8 gt2k_menu::RawListV::create
RawWindow *RawListV::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListV(*args); /* implicit: Object::operator new(0x248); operator delete if
                                       the constructor throws */
    return NULL;
}

// FUNC 0x001D3860 gt2k_menu::RawListV::canCreate
int RawListV::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListVFixed                                                            */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00352870 = "RawListVFixed";                   /* 0x00352870 */
gt2k_menu::RawWindowClass g_00352878 =                      /* 0x00352878 */
    { "RawListVFixed", gt2k_menu::RawListVFixed::create };
gt2k_menu::RawWindowRegister g_00352880(&g_00352878);       /* 0x00352880 */

namespace gt2k_menu {

// FUNC 0x001D3868 gt2k_menu::RawListVFixed::virtual_8
const char *RawListVFixed::virtual_8()
{
    return g_00352870;      /* "RawListVFixed" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3870 gt2k_menu::RawListVFixed::factory
RawWindow *RawListVFixed::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                  int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00352878.create). */
// FUNC 0x001D38B0 gt2k_menu::RawListVFixed::create
RawWindow *RawListVFixed::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListVFixed(*args); /* implicit: Object::operator new(0x248); operator delete if
                                            the constructor throws */
    return NULL;
}

// FUNC 0x001D3928 gt2k_menu::RawListVFixed::canCreate
int RawListVFixed::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListVPos                                                              */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00352888 = "RawListVPos";                     /* 0x00352888 */
gt2k_menu::RawWindowClass g_00352890 =                      /* 0x00352890 */
    { "RawListVPos", gt2k_menu::RawListVPos::create };
gt2k_menu::RawWindowRegister g_00352898(&g_00352890);       /* 0x00352898 */

namespace gt2k_menu {

// FUNC 0x001D3930 gt2k_menu::RawListVPos::virtual_8
const char *RawListVPos::virtual_8()
{
    return g_00352888;      /* "RawListVPos" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3938 gt2k_menu::RawListVPos::factory
RawWindow *RawListVPos::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00352890.create). */
// FUNC 0x001D3978 gt2k_menu::RawListVPos::create
RawWindow *RawListVPos::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListVPos(*args); /* implicit: Object::operator new(0x248); operator delete if
                                          the constructor throws */
    return NULL;
}

// FUNC 0x001D39F0 gt2k_menu::RawListVPos::canCreate
int RawListVPos::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawListVSize                                                             */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_003528A0 = "RawListVSize";                    /* 0x003528A0 */
gt2k_menu::RawWindowClass g_003528A8 =                      /* 0x003528A8 */
    { "RawListVSize", gt2k_menu::RawListVSize::create };
gt2k_menu::RawWindowRegister g_003528B0(&g_003528A8);       /* 0x003528B0 */

namespace gt2k_menu {

// FUNC 0x001D39F8 gt2k_menu::RawListVSize::virtual_8
const char *RawListVSize::virtual_8()
{
    return g_003528A0;      /* "RawListVSize" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3A00 gt2k_menu::RawListVSize::factory
RawWindow *RawListVSize::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                 int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_003528A8.create). */
// FUNC 0x001D3A40 gt2k_menu::RawListVSize::create
RawWindow *RawListVSize::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawListVSize(*args); /* implicit: Object::operator new(0x248); operator delete if
                                           the constructor throws */
    return NULL;
}

// FUNC 0x001D3AB8 gt2k_menu::RawListVSize::canCreate
int RawListVSize::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawScrollBar                                                             */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_003528B8 = "RawScrollBar";                    /* 0x003528B8 */
gt2k_menu::RawWindowClass g_003528C0 =                      /* 0x003528C0 */
    { "RawScrollBar", gt2k_menu::RawScrollBar::create };
gt2k_menu::RawWindowRegister g_003528C8(&g_003528C0);       /* 0x003528C8 */

namespace gt2k_menu {

// FUNC 0x001D3AC0 gt2k_menu::RawScrollBar::virtual_8
const char *RawScrollBar::virtual_8()
{
    return g_003528B8;      /* "RawScrollBar" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3AC8 gt2k_menu::RawScrollBar::factory
RawWindow *RawScrollBar::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                 int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_003528C0.create). */
// FUNC 0x001D3B08 gt2k_menu::RawScrollBar::create
RawWindow *RawScrollBar::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawScrollBar(*args); /* implicit: Object::operator new(0x204); operator delete if
                                           the constructor throws */
    return NULL;
}

// FUNC 0x001D3B80 gt2k_menu::RawScrollBar::canCreate
int RawScrollBar::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* RawWindow                                                                */
/* ------------------------------------------------------------------------ */
/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_003528D0 = "RawWindow";                       /* 0x003528D0 */
gt2k_menu::RawWindowClass g_003528D8 =                      /* 0x003528D8 */
    { "RawWindow", gt2k_menu::RawWindow::create };
gt2k_menu::RawWindowRegister g_003528E0(&g_003528D8);       /* 0x003528E0 */

namespace gt2k_menu {

// FUNC 0x001D3B88 gt2k_menu::RawWindow::virtual_8
const char *RawWindow::virtual_8()
{
    return g_003528D0;      /* "RawWindow" */
}

/* No callers in the image: the layout factory (a MenuHookFn, T229). */
// FUNC 0x001D3B90 gt2k_menu::RawWindow::factory
RawWindow *RawWindow::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_003528D8.create). */
// FUNC 0x001D3BD0 gt2k_menu::RawWindow::create
RawWindow *RawWindow::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new RawWindow(*args); /* implicit: Object::operator new(0x1E4); operator delete if
                                        the constructor throws */
    return NULL;
}

// FUNC 0x001D3C48 gt2k_menu::RawWindow::canCreate
int RawWindow::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

using namespace gt2k_menu;

/* Compiler-generated: egcs's static destructor and initialiser of the 16
 * registrars above (named after the unit's first global), emitted here,
 * after the last function. */
// FUNC 0x001D3C50 T272_static_dtor (_GLOBAL_$D$g_00352760)
//   (.dtors entry 0x0028F23C) ~RawWindowRegister (flags 2) of the
//   registrars, in reverse order.
/* implicit: ~RawWindowRegister */
// FUNC 0x001D3D28 T272_static_init (_GLOBAL_$I$g_00352760)
//   (.ctors entry 0x0028F160) RawWindowRegister(&class) of the 16.
/* implicit: RawWindowRegister */
