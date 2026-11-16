/* T241_link_manager.h - gt2k_menu::LinkManager (link_manager.cxx) and the
 * page record gt2k_menu::LinkPage (translation unit T241, unit u069),
 * 0x001C7220..0x001C7A20.
 *
 * The page records of a loaded .mbl ("menu link") file, PageManager+0x008:
 * a name-sorted table of LinkPage records (binary search), each with its
 * buttons (ButtonData, T244: the pages they lead to, with sub-buttons), the
 * number of its layout file ("<dir>/<country>/<name><n>.pmb"; 1 = the common
 * one) and the page imported (drawn) with it.  The file is relocated in place
 * by T242 (relocate_link_file 0x001C7B30). */
#ifndef GAME_T241_LINK_MANAGER_H
#define GAME_T241_LINK_MANAGER_H

#include "game/game.h"

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
struct LinkFile;                            /* T241 / T242: {int count; LinkPage **pages;} the .mbl
                                               header, the pages sorted by name */

/* A page record of the link file, 0x10C bytes. */
struct LinkPage {
    char         name[0x40];                /* 0x000 the binary search key */
    s16          pmb;                       /* 0x040 the number of the page's .pmb ("%d.pmb");
                                               1: the common layout */
    u16          flags;                     /* 0x042 bit 0 / 1: getFlag0 / getFlag1 (PageManager
                                               +0xA18 / +0xA1C); bit 2 fade in, bit 3 fade out,
                                               0x30 fade mode, 0xC0 decide effect, bit 8 cancel
                                               allowed */
    char         import[0x40];              /* 0x044 the page imported (drawn) with this one */
    char         back[0x40];                /* 0x084 the page "back" goes to */
    char         cancelExec[0x40];          /* 0x0C4 the ExecuteFuncData run on "back" */
    int          count;                     /* 0x104 buttons */
    ButtonData **items;                     /* 0x108 */
};
EGCS_SIZE(gt2k_menu::LinkPage, 0x10C);

/* Not polymorphic.  Size 0xCC (PageManager+0x008 .. +0x0D4). */
class LinkManager {
public:
    LinkManager();                          /* 0x001C7220 */
    int   load(const char *file);           /* 0x001C7240 reads (malloc) and relocates; 0 if the
                                               file does not exist */
    void  unload();                         /* 0x001C72C0 */
    void  reset();                          /* 0x001C7300 no current page, no import override */
    void  clear();                          /* 0x001C7318 reset() */
    int   setPage(const char *name);        /* 0x001C7338 current = the page; 0 if there is none */
    LinkPage *findPage(const char *name);   /* 0x001C7380 NULL if none */
    int   hasImportPage();                  /* 0x001C73B8 */
    const char *getImportPageName();        /* 0x001C73F0 importName, else current->import */
    void  setImportPage(const char *name);  /* 0x001C7410 importName = name if that page exists */
    int   getFlag0();                       /* 0x001C7468 current->flags & 1 */
    int   getFlag1();                       /* 0x001C7478 (current->flags >> 1) & 1 */
    void  checkDuplicateLinks();            /* 0x001C7490 */
    int   findButton(const char *link);     /* 0x001C7580 the index of the current page's button
                                               leading to link; 0 if none */
    const char *makePmbPath(int n);         /* 0x001C7618 "menu/x.mbl" -> "menu/US/x<n>.pmb" in pmbPath */
    const char *getCommonPmbPath();         /* 0x001C7778 makePmbPath(1) */
    const char *getPagePmbPath();           /* 0x001C7798 of the current page; NULL without one */
    const char *getImportPmbPath();         /* 0x001C77C8 of the imported page; NULL (and an error
                                               message) if it does not exist */
    int   isCommonPage();                   /* 0x001C7848 current->pmb == 1 */
    int   isImportCommon();                 /* 0x001C7860 */
    ButtonData *findButtonData(const char *name); /* 0x001C7950 the current page's button `name'
                                               (sub-buttons too); NULL if none */

    LinkFile *data;                         /* 0x00 the loaded file (malloc) */
    LinkPage *current;                      /* 0x04 */
    char      path[0x40];                   /* 0x08 the .mbl file name */
    char      pmbPath[0x40];                /* 0x48 makePmbPath's buffer */
    char      importName[0x40];             /* 0x88 overrides current->import ("" = none) */
    int       duplicateLinks;               /* 0xC8 two buttons of the current page lead to the same
                                               page */
};
EGCS_SIZE(gt2k_menu::LinkManager, 0xCC);

} /* namespace gt2k_menu */

#endif /* GAME_T241_LINK_MANAGER_H */
