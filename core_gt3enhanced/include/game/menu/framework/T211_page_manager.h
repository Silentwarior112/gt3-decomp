/* T211_page_manager.h - gt2k_menu::PageManager and the FuncData tables of the
 * screens (translation unit T211, "../../page_manager.cxx", unit u064),
 * 0x001B6250..0x001B8330.
 *
 * The page set of a menu screen.  loadFile reads the .mbl (LinkManager,
 * T241) and its common layout (PageLayout, T229); setPage selects a page
 * record and its layout (the common one or the page's own file, plus an
 * imported page from the other one); startPage builds the page: a Transitive
 * layer per popup level with the layout's items and a ButtonList
 * (createButtonList, or a MenuFuncData factory) whose buttons call back
 * onButton.  A button jumps to its link (flags 0), opens it as a popup over
 * the page (1), decides the screen (2) or goes back (3); ValidFuncData can
 * turn a button into a popup, ExecuteFuncData runs on every button with an
 * exec name, FocusFuncData / the cursor memory / the previous page place the
 * cursor.  update runs a frame of input.  The screens' page controls
 * (T247 ControlBase and the LoggerPage of T081, a derived class) hold one.
 *
 * The table searches findValidFuncData 0x001B7E50, findExecuteFuncData
 * 0x001B7EE8, findFocusFuncData 0x001B7F80 and findSelectableFuncData
 * 0x001B8018 (identical binary searches by name, signed characters; ? one
 * inline template) are file-local (static) and not declared here; so are
 * the fade scripts g_00333A40 (in) / g_00333AD0 (out), T211 .rodata.
 */
#ifndef GAME_T211_PAGE_MANAGER_H
#define GAME_T211_PAGE_MANAGER_H

#include "game/game.h"
#include "game/menu/framework/T241_link_manager.h"         /* LinkManager (link), LinkPage */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout (common, own), MenuLayoutItem, MenuHook, ... */
#include "game/menu/framework/T240_page_history.h"         /* PageHistory (history) */
#include "game/menu/framework/T233_menu_sound.h"           /* MenuSound (sound) */
#include "game/menu/framework/T247_control_base.h"         /* ControlBase (control: the inline setDecideButtons) */

class CursorMemory;                         /* T423 (pointers only) */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class RootObject;                           /* T220 */
class Transitive;                           /* T216 */
class ButtonList;                           /* T244 */
class ControlBase;                          /* T247 */
class Timer;                                /* T252 */
struct ButtonData;                          /* T244 */
struct OperatorCode;                        /* T217 */
class PageManager;

/* The FuncData tables a screen gives its PageManager (set*FuncData; the
 * types are in the Lupo 2 mangled names): sorted by name for a binary search,
 * except MenuFuncData (searched in table order). */

/* Key: the page.  The initial cursor of the page (< 0: none, then the
 * button linking to the previous page; startPage's further fallback to the
 * cursor memory is dead code: LinkManager::findButton returns 0, never < 0,
 * when no button leads there). */
struct FocusFuncData {
    const char *page;                       /* 0x0 */
    int (*func)(void *ctx, PageManager *pm);    /* 0x4 */
};
EGCS_SIZE(gt2k_menu::FocusFuncData, 0x8);

/* Key: the button's link (callValidFunc: first), then its exec name.  The
 * page to open as a popup instead, NULL: none. */
struct ValidFuncData {
    const char *name;                       /* 0x0 */
    const char *(*func)(void *ctx, ButtonData *rec);    /* 0x4 */
};
EGCS_SIZE(gt2k_menu::ValidFuncData, 0x8);

/* Key: the button's exec name; also the page's cancelExec on "back" (rec
 * NULL, index 0). */
struct ExecuteFuncData {
    const char *name;                       /* 0x0 */
    void (*func)(void *ctx, ButtonData *rec, int index, PageLayout *layout);    /* 0x4 */
};
EGCS_SIZE(gt2k_menu::ExecuteFuncData, 0x8);

/* Key: a layout object of the page (PageLayout::findObject): the first
 * entry whose element exists makes the page's button list (rect: that
 * element, parent: the level's layer). */
struct MenuFuncData {
    const char *name;                       /* 0x0 */
    ButtonList *(*create)(void *ctx, LinkPage *page, MenuLayoutItem *rect, Object *parent,
                          int controlType, int loop, PageLayout *layout, int index);  /* 0x4 */
};
EGCS_SIZE(gt2k_menu::MenuFuncData, 0x8);

/* Key: the page.  The availability checks of its buttons (ButtonList::
 * setButtonChecks: check i for button i, the last one repeats). */
struct SelectableFuncData {
    const char *page;                       /* 0x0 */
    int       (**checks)(void *ctx, ButtonData *data);  /* 0x4 */
    int         count;                      /* 0x8 */
};
EGCS_SIZE(gt2k_menu::SelectableFuncData, 0xC);

/* vtable 0x003338C8 (1 slot: createButtonList; no virtual destructor).
 * typeinfo __tfQ29gt2k_menu11PageManager 0x001B82F0 (__user_type_info
 * 0x003533E8 gp-0x4608).  vptr +0xB58 (egcs: after the data); size 0xB5C. */
class PageManager {
public:
    /* The layouts get control as their owner.  The page names, the layers /
     * lists and lastCursor are not set here (initialize does the names and
     * the layers / lists); fadeTimer and the tables are zeroed. */
    PageManager(ControlBase *control);      /* 0x001B6250 */
    void initialize(RootObject *root, CursorMemory *memory); /* 0x001B6338 */
    void finalize();                        /* 0x001B6438 releases the objects of both levels and the
                                               root's children; clears the tables and the layouts */
    int  loadFile(const char *file);        /* 0x001B6570 the .mbl and its common layout file; 1 */
    void unloadFile();                      /* 0x001B6608 */
    void setSelectBuffer(ButtonData *buf);  /* 0x001B6640 update copies the focused record there */
    void lockRoot();                        /* 0x001B6680 root->updateDisable and g_00352748 = 1 (while
                                               a page changes) */
    void unlockRoot();                      /* 0x001B66B8 both 0 */
    /* The page's record and layout (common or its own file; on the base
     * level the imported page of the other layout); 0 if no such page.
     * importPage (NULL: none) goes to LinkManager::setImportPage. */
    int  setPage(const char *page, const char *importPage);    /* 0x001B66E0 */
    void getReturnPage(char *out);          /* 0x001B6918 the page under the popups */
    void playDecideEffect();                /* 0x001B6970 the decide sound and the focused button's
                                               effect (page flags 0xC0) */
    /* The ButtonList decide callback (ctx = the PageManager): onButton(rec,
     * index, effect).  The definition returns onButton's value; ButtonList
     * (T244 ButtonDecideFunc) ignores it, so the canonical type is void. */
    static void buttonCallback(void *ctx, ButtonData *rec, int index, ButtonList *list,
                               int effect); /* 0x001B6AD0 */
    void pushPopup(ButtonData *rec);        /* 0x001B6AF0 keeps the page under the next one */
    /* A button was decided: ValidFuncData, the cursor memory (only when
     * link.duplicateLinks), ExecuteFuncData, then its action (flags & 0xF).
     * 0 for a jump / popup without a link. */
    int  onButton(ButtonData *rec, int index, int effect);   /* 0x001B6B68 */
    const char *callValidFunc(ButtonData *rec, int *byLink); /* 0x001B6D58 */
    void popPopup();                        /* 0x001B6E10 */
    void callExecuteFunc(ButtonData *rec, int index);        /* 0x001B6EA8 */
    void callCancelFunc();                  /* 0x001B6F20 the page's cancelExec */
    int  back();                            /* 0x001B6F78 closes the popup / goes to the back page /
                                               exits undecided; 1 */
    static void clearTimerDone(Timer *t, void *pm); /* 0x001B70E0 the fade Timer's function (page flags
                                               0x30 == 0x20, after 30 frames): the per-frame screen clear
                                               on, g_00352750 = 1, colour g_0035274C = 0 (T261) */
    /* The list of the page's buttons: callMenuFunc's, else a ButtonList
     * (controlType, loop) centred in the layout object "menu", spacing 4. */
    virtual ButtonList *createButtonList(LinkPage *page, int index);  /* [0] +0x0C 0x001B70F8 */
    /* Builds the page setPage chose on the current popup level (fade, a new
     * layer, the list and its cursor, the layouts' items, the music);
     * 0 without a page. */
    int  startPage(const char *prev);       /* 0x001B7268 */
    ButtonList *callMenuFunc(int index);    /* 0x001B7620 NULL if no MenuFuncData element is there */
    void closePage();                       /* 0x001B7708 the current popup level's page */
    void closeAllPages();                   /* 0x001B7848 */
    void closeAndExit();                    /* 0x001B79B8 */
    void setCursor(int index);              /* 0x001B79F0 */
    int  getLastCursor();                   /* 0x001B7A28 */
    static void fadeDrawHook();             /* 0x001B7A30 the fade rectangle's draw setting */
    void startFade(const OperatorCode *script); /* 0x001B7A60 a full-screen rectangle running script */
    void fadeIn();                          /* 0x001B7B58 */
    void fadeOut();                         /* 0x001B7B78 */
    void setFreeze(int freeze);             /* 0x001B7BD0 */
    int  update();                          /* 0x001B7C08 a frame of input */

    /* The accessors are inline in the original: no callers, and
     * 0x001B80B8..0x001B8290 are the out-of-line copies egcs emits before
     * the typeinfo function; the screens expand setHooks, set*FuncData,
     * isExit and setDecideButtons (u007, u019, u020, u026).  Declared in
     * the reverse of their addresses: egcs writes a class's in-class inline
     * members in reverse declaration order (T211's frame info 0x002C6E8C:
     * endFadeTimer's FDE first, setHooks' last, then the __tf's). */
    /* The same table to both layouts (PageLayout::setMultiFuncs /
     * setSliderFuncs / setHooks). */
    void setHooks(const MenuHook *table, int count, void *ctx) /* 0x001B8290 */
    {
        common.setHooks(table, count, ctx);
        own.setHooks(table, count, ctx);
    }
    void setSliderFuncs(const MenuSliderFunc *table, int count, void *ctx) /* 0x001B8230 */
    {
        common.setSliderFuncs(table, count, ctx);
        own.setSliderFuncs(table, count, ctx);
    }
    void setMultiFuncs(const MenuMultiFunc *table, int count, void *ctx) /* 0x001B81D0 */
    {
        common.setMultiFuncs(table, count, ctx);
        own.setMultiFuncs(table, count, ctx);
    }
    void setSelectableFuncData(const SelectableFuncData *table, int count, void *ctx) /* 0x001B81C0 */
    {
        selectableFuncs = table;
        selectableCount = count;
        selectableCtx = ctx;
    }
    void setMenuFuncData(const MenuFuncData *table, int count, void *ctx) /* 0x001B81B0 */
    {
        menuFuncs = table;
        menuCount = count;
        menuCtx = ctx;
    }
    void setExecuteFuncData(const ExecuteFuncData *table, int count, void *ctx) /* 0x001B81A0 */
    {
        executeFuncs = table;
        executeCount = count;
        executeCtx = ctx;
    }
    void setValidFuncData(const ValidFuncData *table, int count, void *ctx) /* 0x001B8190 */
    {
        validFuncs = table;
        validCount = count;
        validCtx = ctx;
    }
    void setFocusFuncData(const FocusFuncData *table, int count, void *ctx) /* 0x001B8180 */
    {
        focusFuncs = table;
        focusCount = count;
        focusCtx = ctx;
    }
    LinkManager *getLinkManager() const { return (LinkManager *)&link; } /* 0x001B8178 */
    LinkPage *findPage(const char *name)            /* 0x001B8158 link.findPage(name) */
    {
        return link.findPage(name);
    }
    PageLayout *getFragmentManager() const { return current; } /* 0x001B8150 current */
    Transitive *getLayer() const                    /* 0x001B8138 of the current popup level */
    {
        return layers[popupLevel];
    }
    RootObject *getMostRootObject() { return root; } /* 0x001B8130 */
    void disableSound(bool on) { soundOff = on; }   /* 0x001B8128 */
    int getPopupLevel() const { return popupLevel; } /* 0x001B8120 */
    ButtonList *getButtonList() const               /* 0x001B8108 of the current popup level */
    {
        return buttonLists[popupLevel];
    }
    ControlBase *getControl() const { return control; } /* 0x001B8100 */
    char *getCurrentPage() const { return (char *)currentPage; } /* 0x001B80F8 */
    void ignoreCancel(bool on) { noCancel = on; }   /* 0x001B80F0 */
    void doExit() { exitRequest = 1; }              /* 0x001B80E0 the next update goes back */
    int isExit() const { return exited; }           /* 0x001B80D8 */
    int isDecided() const { return decided; }       /* 0x001B80D0 */
    void setDecideButtons(unsigned int buttons)     /* 0x001B80C0 control->decideButtons */
    {
        control->control1.decideButtons = buttons;
    }
    void endFadeTimer() { fadeTimer = 0; }          /* 0x001B80B8 */

    int           exited;                   /* 0x000 isExit */
    int           decided;                  /* 0x004 isDecided */
    LinkManager   link;                     /* 0x008 */
    PageLayout    common;                   /* 0x0D4 the common layout */
    PageLayout    own;                      /* 0x158 the current page's own layout */
    PageLayout   *current;                  /* 0x1DC getFragmentManager: common or &own */
    PageLayout   *imported;                 /* 0x1E0 the layout of the imported page */
    PageHistory   history;                  /* 0x1E4 the pages under the popups */
    ControlBase  *control;                  /* 0xA14 */
    int           controlType;              /* 0xA18 LinkPage flags bit 0: the lists' controlType */
    int           loop;                     /* 0xA1C LinkPage flags bit 1: the lists' loop */
    char          currentPage[0x40];        /* 0xA20 */
    char          returnPage[0x40];         /* 0xA60 "return": the page we came from */
    char          fileName[0x40];           /* 0xAA0 the common layout file */
    RootObject   *root;                     /* 0xAE0 */
    int           popupLevel;               /* 0xAE4 0 or 1 */
    Transitive   *layers[2];                /* 0xAE8 per popup level */
    ButtonList   *buttonLists[2];           /* 0xAF0 */
    MenuSound     sound;                    /* 0xAF8 */
    int           soundOff;                 /* 0xAFC disableSound */
    const SelectableFuncData *selectableFuncs;  /* 0xB00 */
    int           selectableCount;          /* 0xB04 */
    void         *selectableCtx;            /* 0xB08 */
    const MenuFuncData *menuFuncs;          /* 0xB0C */
    int           menuCount;                /* 0xB10 */
    void         *menuCtx;                  /* 0xB14 */
    const ExecuteFuncData *executeFuncs;    /* 0xB18 */
    int           executeCount;             /* 0xB1C */
    void         *executeCtx;               /* 0xB20 */
    const ValidFuncData *validFuncs;        /* 0xB24 */
    int           validCount;               /* 0xB28 */
    void         *validCtx;                 /* 0xB2C */
    const FocusFuncData *focusFuncs;        /* 0xB30 */
    int           focusCount;               /* 0xB34 */
    void         *focusCtx;                 /* 0xB38 */
    CursorMemory *memory;                   /* 0xB3C */
    ButtonData   *selectBuffer;             /* 0xB40 gets the focused button's record */
    int           freeze;                   /* 0xB44 */
    int           noCancel;                 /* 0xB48 ignoreCancel */
    int           exitRequest;              /* 0xB4C doExit */
    Timer        *fadeTimer;                /* 0xB50 */
    int           lastCursor;               /* 0xB54 the cursor of the last list closed */
    /*            vptr                         0xB58 (egcs) */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::PageManager, 0xB5C);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu11PageManager(void);  /* 0x001B82F0 */
extern u32 __tiQ29gt2k_menu11PageManager[2];    /* 0x003533E8 gp-0x4608 its __user_type_info */
}
#endif

#endif /* GAME_T211_PAGE_MANAGER_H */
