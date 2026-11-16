/* T211 - gt2k_menu::PageManager ("../../page_manager.cxx"),
 * 0x001B6250..0x001B8330.
 *
 * The page set of a menu screen.  loadFile reads the .mbl (LinkManager) and
 * its common layout; setPage selects a page record and its layout (the
 * common one or the page's own file, plus an imported page from the other
 * one); startPage builds the page: a Transitive layer per popup level with
 * the layout's items and a ButtonList (createButtonList, or a MenuFuncData
 * factory) whose buttons call back onButton.  A button jumps to its link,
 * opens it as a popup over the page, decides the screen or goes back
 * (BUTTON_*); ValidFuncData can turn a button into a popup, ExecuteFuncData
 * runs on every button with an exec name, FocusFuncData / the previous page
 * place the cursor.  update runs a frame of input.
 *
 * The four FuncData searches (0x001B7E50..0x001B8018) are identical binary
 * searches over tables sorted by name (? copies of one inline template);
 * they are file-local (T211_page_manager.h lists them). */
#define U064_T211                           /* page_manager_internal.h: this unit's declarations */
#include "game/menu/framework/page_manager_internal.h"
#include "game/menu/framework/T211_page_manager.h"

using namespace gt2k_menu;

static const ValidFuncData *findValidFuncData(const ValidFuncData *table, int count,
                                              const char *key);
static const ExecuteFuncData *findExecuteFuncData(const ExecuteFuncData *table, int count,
                                                  const char *key);
static const FocusFuncData *findFocusFuncData(const FocusFuncData *table, int count,
                                              const char *key);
static const SelectableFuncData *findSelectableFuncData(const SelectableFuncData *table,
                                                        int count, const char *key);

namespace gt2k_menu {

// FUNC 0x001B6250 gt2k_menu::PageManager::PageManager
//   The layouts get the control as their owner.  Not set here: the page
//   names, the layers / lists (+0xAE8..+0xAF4) and lastCursor (initialize
//   does the names and the layers / lists).  The member constructors:
//   LinkManager(), PageLayout(control) twice, PageHistory(), MenuSound().
PageManager::PageManager(ControlBase *ctl)
    : exited(0), decided(0),
      link(), /* implicit: LinkManager, PageLayout, PageHistory, MenuSound (member constructors) */
      common(ctl), own(ctl),
      current(0), imported(0),
      history(),
      control(ctl), controlType(0), loop(0),
      root(0), popupLevel(0),
      sound(),
      soundOff(0),
      selectableFuncs(0), selectableCount(0), selectableCtx(0),
      menuFuncs(0), menuCount(0), menuCtx(0),
      executeFuncs(0), executeCount(0), executeCtx(0),
      validFuncs(0), validCount(0), validCtx(0),
      focusFuncs(0), focusCount(0), focusCtx(0),
      memory(0), selectBuffer(0), freeze(0), noCancel(0), exitRequest(0),
      fadeTimer(0)
{
}

// FUNC 0x001B6338 gt2k_menu::PageManager::initialize
//   (Name not the original's: no symbol; it calls the layouts' initialize.)
//   Everything back to "no page" for the screen's root and cursor memory.
//   NB: noCancel (+0xB48) and lastCursor are not reset.
void PageManager::initialize(RootObject *rootObject, CursorMemory *cursorMemory)
{
    exited = 0;
    decided = 0;
    link.reset();
    own.initialize();
    common.initialize();
    root = rootObject;
    current = 0;
    imported = 0;
    controlType = 0;
    loop = 0;
    currentPage[0] = '\0';
    returnPage[0] = '\0';
    fileName[0] = '\0';
    popupLevel = 0;
    soundOff = 0;
    selectableFuncs = 0;
    selectableCount = 0;
    selectableCtx = 0;
    menuFuncs = 0;
    menuCount = 0;
    menuCtx = 0;
    executeFuncs = 0;
    executeCount = 0;
    executeCtx = 0;
    validFuncs = 0;
    validCount = 0;
    validCtx = 0;
    focusFuncs = 0;
    focusCount = 0;
    focusCtx = 0;
    selectBuffer = 0;
    for (int level = 0; level < 2; level++) {
        layers[level] = 0;
        buttonLists[level] = 0;
    }
    memory = cursorMemory;
    freeze = 0;
    exitRequest = 0;
    fadeTimer = 0;
}

// FUNC 0x001B6438 gt2k_menu::PageManager::finalize
//   (Name not the original's: no symbol; it calls the layouts' finalize.)
//   Kills the layers and lists of both levels, deletes the root's tree,
//   clears the tables and the names, finalizes the layouts and the links.
void PageManager::finalize()
{
    for (int level = 0; level < 2; level++) {
        if (layers[level] != 0) {
            layers[level]->kill();
            layers[level] = 0;
        }
        if (buttonLists[level] != 0) {
            buttonLists[level]->kill();
            buttonLists[level] = 0;
        }
    }
    root->deleteChildren();
    selectBuffer = 0;
    current = 0;
    imported = 0;
    controlType = 0;
    loop = 0;
    currentPage[0] = '\0';
    returnPage[0] = '\0';
    fileName[0] = '\0';
    selectableFuncs = 0;
    selectableCount = 0;
    selectableCtx = 0;
    menuFuncs = 0;
    menuCount = 0;
    menuCtx = 0;
    executeFuncs = 0;
    executeCount = 0;
    executeCtx = 0;
    validFuncs = 0;
    validCount = 0;
    validCtx = 0;
    focusFuncs = 0;
    focusCount = 0;
    focusCtx = 0;
    exited = 0;
    popupLevel = 0;
    common.finalize();
    own.finalize();
    link.clear();
    root->updateDisable = 0;
}

// FUNC 0x001B6570 gt2k_menu::PageManager::loadFile
//   Loads the .mbl `file' and its common layout file; always 1 (the errors
//   are only reported).
int PageManager::loadFile(const char *file)
{
    if (link.load(file) == 0)
        menu_message("no such file [%s]", file);
    strcpy(fileName, link.getCommonPmbPath());
    if (common.readFile(fileName) == 0)
        menu_message("no such file [%s]", fileName);
    return 1;
}

// FUNC 0x001B6608 gt2k_menu::PageManager::unloadFile
void PageManager::unloadFile()
{
    own.unloadFile();
    common.unloadFile();
    link.unload();
}

// FUNC 0x001B6640 gt2k_menu::PageManager::setSelectBuffer
//   (Name not the original's: no symbol.)  update copies the focused
//   button's record into buf every frame; buf is zeroed first.
void PageManager::setSelectBuffer(ButtonData *buf)
{
    if (buf != 0)
        zero_memory(buf, sizeof(ButtonData));
    selectBuffer = buf;
}

// FUNC 0x001B6680 gt2k_menu::PageManager::lockRoot
//   (Name not the original's: no symbol.)  While the base page is closed
//   (closePage on level 0, closeAllPages): the root's update and the menu
//   rendering (g_00352748, T246) stop.
void PageManager::lockRoot()
{
    root->updateDisable = 1;
    gfx_vsync(4);
    g_00352748 = 1;
}

// FUNC 0x001B66B8 gt2k_menu::PageManager::unlockRoot
//   (Name not the original's: no symbol.)  startPage / closeAndExit.
void PageManager::unlockRoot()
{
    root->updateDisable = 0;
    gfx_vsync(1);
    g_00352748 = 0;
}

// FUNC 0x001B66E0 gt2k_menu::PageManager::setPage
//   Selects page `page': the common layout or the page's own layout file,
//   and on the base level the imported page from the other layout
//   (importPage, if given, replaces the record's).  0 if the LinkManager
//   has no such page, else 1 (the other errors are only reported).
int PageManager::setPage(const char *page, const char *importPage)
{
    current = 0;
    imported = 0;
    strcpy(currentPage, page);
    if (link.setPage(page) == 0) {
        menu_message("cannot set page [%s]", page);
        return 0;
    }
    if (importPage != 0)
        link.setImportPage(importPage);
    if (link.isCommonPage() != 0) {
        current = &common;
    } else {
        current = &own;
        const char *pmb = link.getPagePmbPath();
        if (own.loadFile(pmb) == 0)
            menu_message("cannot load [%s]", pmb);
    }
    controlType = link.getFlag0();
    loop = link.getFlag1();
    if (popupLevel == 0 && link.hasImportPage() != 0) {
        /* "the imported page must have the other attribute (common or not)
         * than the base page." (EUC-JP) */
        if (link.isImportCommon() != 0) {
            if (current == &common)
                menu_message("\xa5\xa4\xa5\xf3\xa5\xdd\xa1\xbc\xa5\xc8\xa4\xb9\xa4\xeb\xa5\xda\xa1\xbc"
                             "\xa5\xb8\xa4\xcf\n\xa5\xd9\xa1\xbc\xa5\xb9\xa4\xce\xa5\xda\xa1\xbc\xa5\xb8"
                             "\xa4\xc8\xb0\xdb\xa4\xca\xa4\xeb\xc2\xb0\xc0\xad(common or not)\xa4\xc7\xa4"
                             "\xa2\xa4\xeb\xa4\xb3\xa4\xc8\xa1\xa3\n%s:%s:%d",
                             link.getImportPageName(), "../../page_manager.cxx", 0x152);
            else
                imported = &common;
        } else {
            if (current == &own)
                menu_message("\xa5\xa4\xa5\xf3\xa5\xdd\xa1\xbc\xa5\xc8\xa4\xb9\xa4\xeb\xa5\xda\xa1\xbc"
                             "\xa5\xb8\xa4\xcf\n\xa5\xd9\xa1\xbc\xa5\xb9\xa4\xce\xa5\xda\xa1\xbc\xa5\xb8"
                             "\xa4\xc8\xb0\xdb\xa4\xca\xa4\xeb\xc2\xb0\xc0\xad(common or not)\xa4\xc7\xa4"
                             "\xa2\xa4\xeb\xa4\xb3\xa4\xc8\xa1\xa3\n%s:%s:%d",
                             link.getImportPageName(), "../../page_manager.cxx", 0x15E);
            else
                imported = &own;
        }
        /* NB: after the error above `imported' is still NULL and is used
         * anyway (a NULL `this' for loadFile / setPage). */
        const char *pmb = link.getImportPmbPath();
        if (imported->loadFile(pmb) == 0)
            menu_message("cannot load [%s]", pmb);
        if (imported->setPage(link.getImportPageName(), popupLevel) == 0)
            menu_message("no such import page [%s]", link.getImportPageName());
    }
    if (current->setPage(page, popupLevel) == 0)
        menu_message("no such page [%s]", page);
    return 1;
}

// FUNC 0x001B6918 gt2k_menu::PageManager::getReturnPage
//   (Name not the original's: no symbol.)  The page under the popups (the
//   history's oldest entry), or the current page when there is no popup.
void PageManager::getReturnPage(char *out)
{
    const char *basePage = history.getFirst();
    if (basePage == 0)
        strcpy(out, currentPage);
    else
        strcpy(out, basePage);
}

// FUNC 0x001B6970 gt2k_menu::PageManager::playDecideEffect
//   (Name not the original's: no symbol.)  Waits (a vsync at a time) until
//   the list has stopped scrolling, then the decide sound and the focused
//   button's effect chosen by the page flags (PAGE_EFFECT_MASK).
void PageManager::playDecideEffect()
{
    LinkPage *page = link.current;
    if (page == 0)
        return;
    ButtonList *list = buttonLists[popupLevel];
    if (list == 0)
        return;
    Button *button = list->focusButton();
    if (button == 0)
        return;
    while (list->interpolateDiff > 1.0f)
        gfx_vsync(1);
    switch (page->flags & PAGE_EFFECT_MASK) {
    case 0x00:
        menu_play_se(MENU_SE_CONFIRM);
        break;
    case 0x40:
        menu_play_se(MENU_SE_DECIDE);
        button->virtual_15();
        break;
    case 0x80:
        menu_play_se(MENU_SE_DECIDE);
        button->virtual_16();
        break;
    case 0xC0:
        menu_play_se(MENU_SE_DECIDE);
        button->virtual_17();
        break;
    }
}

// FUNC 0x001B6AD0 gt2k_menu::PageManager::buttonCallback
//   The ButtonList decide callback (ctx = the PageManager); the list is not
//   used.  The code returns onButton's value (the original's return type is
//   int); ButtonDecideFunc (T244) ignores it, so the canonical type is void.
void PageManager::buttonCallback(void *ctx, ButtonData *rec, int index, ButtonList *list,
                                 int effect)
{
    ((PageManager *)ctx)->onButton(rec, index, effect);
}

// FUNC 0x001B6AF0 gt2k_menu::PageManager::pushPopup
//   (Name not the original's: no symbol.)  Keeps the current page under the
//   next one (a popup; the caller sets the new page): history, the layout's
//   popup button, the list made inactive, one level up.  On a popup level
//   the popup is closed instead (and the level kept: popups do not nest).
void PageManager::pushPopup(ButtonData *rec)
{
    if (popupLevel > 0) {
        closePage();
        return;
    }
    history.push(currentPage);
    int level = popupLevel;
    current->popup = rec;
    if (buttonLists[level] != 0)
        buttonLists[level]->active = 0;
    popupLevel = level + 1;
}

// FUNC 0x001B6B68 gt2k_menu::PageManager::onButton
//   A button of the list was decided (index: its position; effect: play the
//   decide effect).  On the base level a ValidFuncData may name a page to
//   open as a popup instead (its ExecuteFuncData runs first when it was
//   found by the link).  Otherwise the cursor goes to the cursor memory (only
//   when two buttons lead to the same page), the button's ExecuteFuncData
//   runs and its action (BUTTON_*) is done.  0 for a jump / popup without a
//   link, else 1.
int PageManager::onButton(ButtonData *rec, int index, int effect)
{
    if (popupLevel == 0) {
        int byLink = 0;
        const char *popupPage = callValidFunc(rec, &byLink);
        if (popupPage != 0) {
            if (byLink != 0)
                callExecuteFunc(rec, index);
            /* pushPopup expanded */
            history.push(currentPage);
            int level = popupLevel;
            current->popup = rec;
            if (buttonLists[level] != 0)
                buttonLists[level]->active = 0;
            popupLevel = level + 1;
            setPage(popupPage, 0);
            startPage(0);
            return 1;
        }
    }
    if (memory != 0 && link.duplicateLinks != 0)
        memory->set(currentPage, index);
    callExecuteFunc(rec, index);
    switch (rec->flags & BUTTON_ACTION_MASK) {
    case BUTTON_JUMP:
        if (rec->link[0] == '\0')
            return 0;
        if (effect != 0)
            playDecideEffect();
        getReturnPage(returnPage);
        closeAllPages();
        current->decided = rec;
        setPage(rec->link, 0);
        startPage(returnPage);
        break;
    case BUTTON_POPUP:
        if (rec->link[0] == '\0')
            return 0;
        if (effect != 0)
            playDecideEffect();
        pushPopup(rec);
        setPage(rec->link, 0);
        startPage(0);
        break;
    case BUTTON_DECIDE:
        if (effect != 0)
            playDecideEffect();
        exited = 1;
        decided = 1;
        break;
    case BUTTON_BACK:
        back();
        break;
    case BUTTON_SUB_LIST:
        break;
    }
    return 1;
}

// FUNC 0x001B6D58 gt2k_menu::PageManager::callValidFunc
//   The ValidFuncData of the button's link (then *byLink = 1, set before
//   the call), else of its exec name: what it returns, NULL if none.
const char *PageManager::callValidFunc(ButtonData *rec, int *byLink)
{
    if (validFuncs == 0)
        return 0;
    if (rec->link[0] != '\0') {
        const ValidFuncData *entry = findValidFuncData(validFuncs, validCount, rec->link);
        if (entry != 0) {
            *byLink = 1;
            return entry->func(validCtx, rec);
        }
    }
    if (rec->exec[0] != '\0') {
        const ValidFuncData *entry = findValidFuncData(validFuncs, validCount, rec->exec);
        if (entry != 0)
            return entry->func(validCtx, rec);
    }
    return 0;
}

// FUNC 0x001B6E10 gt2k_menu::PageManager::popPopup
//   (Name not the original's: no symbol.)  Closes the popup and goes back
//   to the page under it: the history's newest name (used after the pop:
//   the ring keeps the string), the level down, the root's update on, the
//   list under it active again.  The caller starts the page.
//   Matching (94.7%): v0 / v1 swapped for the level and its index (local-
//   alloc order); --popupLevel, popupLevel--, a ButtonList local, the root
//   store first or between and getButtonList() do not change it (or are
//   worse), nor does the reference compiler.
void PageManager::popPopup()
{
    const char *page = history.getTop();
    history.pop();
    if (page == 0)
        return;
    current->popup = 0;
    closePage();
    int level = popupLevel - 1;
    popupLevel = level;
    root->updateDisable = 0;
    if (buttonLists[level] != 0)
        buttonLists[level]->active = 1;
    setPage(page, 0);
}

// FUNC 0x001B6EA8 gt2k_menu::PageManager::callExecuteFunc
//   The ExecuteFuncData of the button's exec name, if any.
void PageManager::callExecuteFunc(ButtonData *rec, int index)
{
    if (executeFuncs == 0)
        return;
    if (rec->exec[0] == '\0')
        return;
    const ExecuteFuncData *entry = findExecuteFuncData(executeFuncs, executeCount, rec->exec);
    if (entry != 0)
        entry->func(executeCtx, rec, index, current);
}

// FUNC 0x001B6F20 gt2k_menu::PageManager::callCancelFunc
//   The ExecuteFuncData named by the page's cancelExec (rec NULL, index 0).
//   NB: executeFuncs is not tested (the search returns NULL for count 0).
void PageManager::callCancelFunc()
{
    LinkPage *page = link.current;
    if (page->cancelExec[0] == '\0')
        return;
    const ExecuteFuncData *entry = findExecuteFuncData(executeFuncs, executeCount,
                                                       page->cancelExec);
    if (entry != 0)
        entry->func(executeCtx, 0, 0, current);
}

// FUNC 0x001B6F78 gt2k_menu::PageManager::back
//   (Name not the original's: no symbol.)  "Back": closes the popup; on the
//   base level goes to the page's back page ("return": the page we came
//   from), or without one runs the cancel hook and exits the screen
//   undecided.  The sound: the decide effect when a button did it (the last
//   key is decide) and no doExit is pending, else se 9 (1 when going to the
//   back page).  Always 1.
int PageManager::back()
{
    if (popupLevel > 0) {
        if (control->control1.lastKey == MENU_KEY_DECIDE) {
            if (exitRequest == 0)
                playDecideEffect();
        } else {
            menu_play_se(MENU_SE_CLOSE);
        }
        popPopup();
    } else {
        const char *backPage = link.current->back;
        if (str_equal(backPage, "return") != 0)
            backPage = returnPage;
        if (backPage[0] != '\0') {
            if (control->control1.lastKey == MENU_KEY_DECIDE) {
                if (exitRequest == 0)
                    playDecideEffect();
            } else {
                menu_play_se(MENU_SE_CANCEL);
            }
            char prevPage[0x40];
            strcpy(prevPage, currentPage);
            closePage();
            callCancelFunc();
            setPage(backPage, 0);
            startPage(prevPage);
        } else {
            if (control->control1.lastKey == MENU_KEY_DECIDE) {
                if (exitRequest == 0)
                    playDecideEffect();
            } else {
                menu_play_se(MENU_SE_CLOSE);
            }
            callCancelFunc();
            closeAndExit();
            decided = 0;
        }
    }
    return 1;
}

// FUNC 0x001B70E0 gt2k_menu::PageManager::clearTimerDone
//   The function of the PAGE_CLEAR_LATER Timer (arg = the PageManager): the
//   per-frame screen clear goes on (black).
void PageManager::clearTimerDone(Timer *timer, void *pm)
{
    ((PageManager *)pm)->fadeTimer = 0;
    g_00352750 = 1;
    g_0035274C = 0;
}

// FUNC 0x001B70F8 gt2k_menu::PageManager::createButtonList
//   Slot 0.  The list of the page's buttons: a MenuFuncData factory's if
//   one matches, else (with buttons) a ButtonList centred in the layout
//   object "menu" (y measured up from the bottom of the 448-line screen),
//   spacing 4; without "menu" one at (0, 0), size 0, not relative.  `new'
//   is Object::operator new (the landing pad gives the block back with
//   Object::operator delete if the constructor throws).
//   Matching: retail moves relative / index (t2 / t3) after the floats:
//   exact since ButtonList's 13-argument constructor (T244, changed by u070
//   with every caller) takes its floats right after layout -- (controlType,
//   loop, layout, x, y, w, h, spacing, items, count, relative, index,
//   rects); EABI gives the same registers (92% before; see
//   ref/matching/u064/notes.md).
ButtonList *PageManager::createButtonList(LinkPage *page, int index)
{
    ButtonList *list = callMenuFunc(index);
    if (list == 0) {
        MenuLayoutItem *rect = current->findObject("menu", NULL);   /* the current layer */
        int relative = 1;
        f32 x, y, w, h;
        if (rect != 0) {
            y = rect->y;
            x = rect->x;
            w = rect->w;
            h = rect->h;
            y = 448.0f - (y + h * 0.5f);
            x = x + w * 0.5f;
        } else {
            w = h = x = y = 0.0f;
            relative = 0;
        }
        if (page->count > 0)
            list = new ButtonList(controlType, loop, current, x, y, w, h, 4.0f, page->items, /* implicit: operator new, operator delete (new-expression) */
                                  page->count, relative, index, 0);
    }
    return list;
}

// FUNC 0x001B7268 gt2k_menu::PageManager::startPage
//   (Name not the original's: no symbol.)  Builds the page setPage chose on
//   the current popup level: the screen clear (PAGE_CLEAR_*), the fade in
//   (PAGE_FADE_IN), a new Transitive layer, the button list with its cursor
//   (FocusFuncData, else the button linking to `prevPage'), its availability
//   checks (SelectableFuncData), the layouts' items, then the page's music on
//   the base level.  0 without a layout or page.  The Timer is built by its
//   inline constructor (Object(), then func / frames / arg) in a block of
//   Object::operator new; the landing pads free with Object::operator delete.
int PageManager::startPage(const char *prevPage)
{
    unlockRoot();
    exited = 0;
    if (current == 0)
        return 0;
    LinkPage *page = getLinkManager()->current;
    if (page == 0) {
        menu_message("no page error");
        return 0;
    }
    switch (page->flags & PAGE_CLEAR_MASK) {
    case PAGE_CLEAR_LATER:
        g_00352750 = 0;
        g_0035274C = 0;
        if (fadeTimer != 0)
            fadeTimer->kill();
        fadeTimer = new Timer(clearTimerDone, this, 30); /* implicit: operator new, operator delete (new-expression), Object (Timer's inline constructor) */
        fadeTimer->attachTo(root);
        break;
    case PAGE_CLEAR_NOW:
        g_0035274C = 0;
        g_00352750 = 1;
        break;
    case PAGE_CLEAR_NONE:
        g_00352750 = 0;
        g_0035274C = 0;
        break;
    }
    if (page->flags & PAGE_FADE_IN)
        fadeIn();

    layers[popupLevel] = new Transitive();
    buttonLists[popupLevel] = 0;
    int cursor = -1;
    if (focusFuncs != 0) {
        const FocusFuncData *entry = findFocusFuncData(focusFuncs, focusCount, currentPage);
        if (entry != 0)
            cursor = entry->func(focusCtx, this);
    }
    if (cursor < 0) {
        cursor = getLinkManager()->findButton(prevPage);
        /* NB: dead: findButton returns 0 when no button leads there, never
         * < 0, so the cursor memory is not read here. */
        if (cursor < 0 && memory != 0)
            cursor = memory->get(currentPage);
    }
    ButtonList *list = createButtonList(page, cursor > -1 ? cursor : 0);
    buttonLists[popupLevel] = list;
    if (list != 0) {
        list->setDecideFunc(this, buttonCallback);
        if (selectableFuncs != 0) {
            const SelectableFuncData *entry =
                findSelectableFuncData(selectableFuncs, selectableCount, currentPage);
            if (entry != 0)
                buttonLists[popupLevel]->setButtonChecks(entry, selectableCtx);
        }
        buttonLists[popupLevel]->refresh();
        buttonLists[popupLevel]->attachTo(layers[popupLevel]);
        if (buttonLists[popupLevel] != 0)
            buttonLists[popupLevel]->hideButtons(freeze);
    }
    current->build(layers[popupLevel]);
    if (imported != 0)
        imported->build(layers[popupLevel]);
    layers[popupLevel]->attachTo(root);
    if (soundOff == 0 && popupLevel == 0)
        sound.selectMusic(currentPage);
    gfx_vsync(10);
    return 1;
}

// FUNC 0x001B7620 gt2k_menu::PageManager::callMenuFunc
//   The list made by the first MenuFuncData whose layout object is in the
//   page's layout (table order); NULL if none or the page has no buttons.
ButtonList *PageManager::callMenuFunc(int index)
{
    if (menuFuncs == 0)
        return 0;
    LinkPage *page = link.current;
    if (page->count <= 0)
        return 0;
    for (int i = 0; i < menuCount; i++) {
        const MenuFuncData *entry = &menuFuncs[i];
        MenuLayoutItem *rect = current->findObject(entry->name, NULL);
        if (rect != 0)
            return entry->create(menuCtx, page, rect, layers[popupLevel], controlType, loop,
                                 current, index);
    }
    return 0;
}

// FUNC 0x001B7708 gt2k_menu::PageManager::closePage
//   (Name not the original's: no symbol.)  Closes the current popup level's
//   page: the clear Timer, the fade out (PAGE_FADE_OUT, waited for), the
//   list (its cursor kept in lastCursor; it dies with the layer), the
//   layouts' input mode, the layer (a popup's runs its close script, the base
//   one is killed at once).
void PageManager::closePage()
{
    LinkPage *page = link.current;
    if (page != 0) {
        if (fadeTimer != 0) {
            fadeTimer->kill();
            fadeTimer = 0;
        }
        u16 flags = page->flags;
        if ((flags & PAGE_CLEAR_MASK) == PAGE_CLEAR_LATER) {
            g_00352750 = 0;
            g_0035274C = 0;
        }
        if (flags & PAGE_FADE_OUT)
            fadeOut();
    }
    ButtonList *list = buttonLists[popupLevel];
    if (list != 0) {
        lastCursor = list->getCursor();
        buttonLists[popupLevel] = 0;
    }
    if (popupLevel == 0)
        lockRoot();
    current->clearInputMode();
    if (imported != 0)
        imported->clearInputMode();
    Transitive *layer = layers[popupLevel];
    if (layer != 0) {
        if (popupLevel > 0)
            layer->disappearAll(0);
        else
            layer->kill();
        layers[popupLevel] = 0;
    }
    gfx_vsync(2);
}

// FUNC 0x001B7848 gt2k_menu::PageManager::closeAllPages
//   (Name not the original's: no symbol.)  closePage for every level down
//   to the base one (the history popped for the popups); popupLevel = 0.
//   The root is locked first.
void PageManager::closeAllPages()
{
    LinkPage *page = link.current;
    if (page != 0) {
        if (fadeTimer != 0) {
            fadeTimer->kill();
            fadeTimer = 0;
        }
        u16 flags = page->flags;
        if ((flags & PAGE_CLEAR_MASK) == PAGE_CLEAR_LATER) {
            g_00352750 = 0;
            g_0035274C = 0;
        }
        if (flags & PAGE_FADE_OUT)
            fadeOut();
    }
    lockRoot();
    current->clearInputMode();
    if (imported != 0)
        imported->clearInputMode();
    ButtonList *list = buttonLists[popupLevel];
    if (list != 0)
        lastCursor = list->getCursor();
    while (popupLevel >= 0) {
        buttonLists[popupLevel] = 0;
        if (popupLevel > 0)
            history.pop();
        Transitive *layer = layers[popupLevel];
        if (layer != 0) {
            if (popupLevel > 0)
                layer->disappearAll(0);
            else
                layer->kill();
            layers[popupLevel] = 0;
        }
        buttonLists[popupLevel] = 0;
        popupLevel--;
    }
    popupLevel = 0;
    gfx_vsync(2);
}

// FUNC 0x001B79B8 gt2k_menu::PageManager::closeAndExit
void PageManager::closeAndExit()
{
    closeAllPages();
    exited = 1;
    unlockRoot();
}

// FUNC 0x001B79F0 gt2k_menu::PageManager::setCursor
void PageManager::setCursor(int index)
{
    ButtonList *list = buttonLists[popupLevel];
    if (list != 0)
        list->setCursor(index);
}

// FUNC 0x001B7A28 gt2k_menu::PageManager::getLastCursor
int PageManager::getLastCursor()
{
    return lastCursor;
}

// FUNC 0x001B7A30 gt2k_menu::PageManager::fadeDrawHook
//   The fade rectangle's draw setting: alpha blending, Cs * As + Cd * (1 - As).
void PageManager::fadeDrawHook()
{
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 1.0f);
}

// FUNC 0x001B7A60 gt2k_menu::PageManager::startFade
//   (Name not the original's: no symbol.)  A full-screen (640 x 448, centred
//   at 320, 224) Rectangle in front of everything (z 10000) running
//   `script' (it releases itself at the end).  Rectangle() is inline:
//   Transitive(), frameType / frameSize 0 (Object::operator new; the landing
//   pad: Object::operator delete).
void PageManager::startFade(const OperatorCode *script)
{
    Rectangle *rect = new Rectangle(); /* implicit: operator new, operator delete (new-expression), Transitive (Rectangle's inline constructor) */
    rect->x.set(320.0f);
    rect->y.set(224.0f);
    rect->w = 640.0f;
    rect->h = 448.0f;
    rect->z = 10000.0f;
    rect->start(script);
    rect->drawSettingFunc = fadeDrawHook;
    rect->attachTo(root);
}

} /* namespace gt2k_menu */

/* One instruction of the fade scripts: code (T217's Operator::op* index),
 * arg, the operands f10..f20, result (1 ends the frame); arg2 and the
 * unknown bytes are 0.  An opCall function goes in as (int)fn: the union's
 * first member is the int (as u002's U002_OP). */
#define U064_OP(code, arg, f10, f14, f18, f1C, f20, result) \
    { (code), (arg), 0, { 0, 0, 0, 0 }, (f10), (f14), (f18), (f1C), (f20), \
      { 0, 0, 0, 0, 0, 0, 0, 0 }, (result) }

/* The fade scripts of the full-screen rectangle (startFade).  File-static:
 * egcs writes internal-linkage const tables at the very end of .rodata,
 * after the vtable and the type names, in definition order (retail's are
 * there, after Operator's name).  In: opaque black, alpha -2 / s for 0.5 s,
 * opRelease.  Out: transparent black, alpha +4 / s for 0.25 s, opCall
 * menu_signal_set (fadeOut waits for it), opWait 0.1 s, opRelease. */
static const gt2k_menu::OperatorCode g_00333A40[3] = {  /* 0x00333A40 fade in */
    U064_OP(18 /* opSetColors */,  0,                     0.0f, 0.0f, 0.0f,  1.0f, 0.0f,  0),
    U064_OP(20 /* opFadeColors */, 0,                     0.0f, 0.0f, 0.0f, -2.0f, 0.5f,  1),
    U064_OP( 6 /* opRelease */,    0,                     0.0f, 0.0f, 0.0f,  0.0f, 0.0f,  1)
};
static const gt2k_menu::OperatorCode g_00333AD0[5] = {  /* 0x00333AD0 fade out */
    U064_OP(18 /* opSetColors */,  0,                     0.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0),
    U064_OP(20 /* opFadeColors */, 0,                     0.0f, 0.0f, 0.0f,  4.0f, 0.25f, 1),
    U064_OP(30 /* opCall */,       (int)menu_signal_set,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0),
    U064_OP( 5 /* opWait */,       0,                     0.1f, 0.0f, 0.0f,  0.0f, 0.0f,  1),
    U064_OP( 6 /* opRelease */,    0,                     0.0f, 0.0f, 0.0f,  0.0f, 0.0f,  1)
};

namespace gt2k_menu {

// FUNC 0x001B7B58 gt2k_menu::PageManager::fadeIn
//   From black to the page in 0.5 s (not waited for).
void PageManager::fadeIn()
{
    startFade(g_00333A40);
}

// FUNC 0x001B7B78 gt2k_menu::PageManager::fadeOut
//   To black in 0.25 s: waits until the script's opCall has set the menu
//   signal.
void PageManager::fadeOut()
{
    menu_signal_clear();
    startFade(g_00333AD0);
    while (menu_signal_poll() != 0)
        gfx_vsync(1);
}

// FUNC 0x001B7BD0 gt2k_menu::PageManager::setFreeze
//   (Name not the original's: no symbol.)  While set, update does nothing
//   and the list's buttons are hidden (also for the lists startPage makes).
void PageManager::setFreeze(int frozen)
{
    freeze = frozen;
    ButtonList *list = buttonLists[popupLevel];
    if (list != 0)
        list->hideButtons(frozen);
}

// FUNC 0x001B7C08 gt2k_menu::PageManager::update
//   One frame: a pending doExit goes back (1); else the layouts' slide side
//   from the input (inputMode: horizontal lists 3 left / 4 otherwise,
//   vertical 1 up / 2 otherwise), the list's input, the focused record into
//   the select buffer, cancel (page flag bit 8, unless ignoreCancel).  1 if
//   a direction, decide or cancel is down, 0 when frozen.
int PageManager::update()
{
    if (freeze != 0)
        return 0;
    if (exitRequest != 0) {
        back();
        exitRequest = 0;
        return 1;
    }
    if (controlType == 0) {
        if (control->control1.up != 0)
            current->inputMode = 1;
        else
            current->inputMode = 2;
    } else {
        if (control->control1.left != 0)
            current->inputMode = 3;
        else
            current->inputMode = 4;
    }
    ButtonList *list = buttonLists[popupLevel];
    if (list != 0) {
        list->doControl(control->control1);
        if (selectBuffer != 0) {
            ButtonData *rec = buttonLists[popupLevel]->focusData();
            if (rec != 0)
                *selectBuffer = *rec;
        }
    }
    if (noCancel == 0 && control->control1.cancel != 0 &&
        ((link.current->flags >> PAGE_CANCEL_SHIFT) & 1))
        back();
    /* MenuControl::anyKey() expanded (inline in the original: its copy
     * 0x001BB048 has no callers). */
    const MenuControl &pad = control->control1;
    return pad.up != 0 || pad.down != 0 || pad.left != 0 || pad.right != 0 ||
           pad.decide != 0 || pad.cancel != 0;
}

} /* namespace gt2k_menu */

/* The FuncData searches: binary searches by name over the sorted tables
 * (signed character compares; a hit is a full match).  NULL if not found.
 * Tail-recursive: egcs turns the self-calls into jumps (the lower half's
 * to the entry after the top test, the upper half's to the top).  The same
 * code as T203's find_part_name and the searches of T229 / T241 / u073 /
 * u099 / u100 / u111 (one shape, ? a template).  Exact only in this form:
 * the new count in a variable tested before the call (a test of `count'
 * itself after `count = count / 2' is cross-jumped with the test at the
 * top), the character loop left by returns and a break with `return entry'
 * after it (with the return inside the loop, the function's return label
 * falls inside the loop and loop.c skips it: no hoisting), see
 * ref/matching/u064/notes.md. */

// FUNC 0x001B7E50 findValidFuncData
static const ValidFuncData *findValidFuncData(const ValidFuncData *table, int count,
                                              const char *key)
{
    if (count <= 0)
        return 0;
    const ValidFuncData *entry = &table[count / 2];
    const char *name = entry->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return findValidFuncData(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return findValidFuncData(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

// FUNC 0x001B7EE8 findExecuteFuncData
static const ExecuteFuncData *findExecuteFuncData(const ExecuteFuncData *table, int count,
                                                  const char *key)
{
    if (count <= 0)
        return 0;
    const ExecuteFuncData *entry = &table[count / 2];
    const char *name = entry->name;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return findExecuteFuncData(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return findExecuteFuncData(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

// FUNC 0x001B7F80 findFocusFuncData
static const FocusFuncData *findFocusFuncData(const FocusFuncData *table, int count,
                                              const char *key)
{
    if (count <= 0)
        return 0;
    const FocusFuncData *entry = &table[count / 2];
    const char *name = entry->page;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return findFocusFuncData(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return findFocusFuncData(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

// FUNC 0x001B8018 findSelectableFuncData
//   The same over the 12-byte entries.
static const SelectableFuncData *findSelectableFuncData(const SelectableFuncData *table, int count,
                                                        const char *key)
{
    if (count <= 0)
        return 0;
    const SelectableFuncData *entry = &table[count / 2];
    const char *name = entry->page;
    for (int i = 0; ; i++) {
        if ((s8)key[i] != (s8)name[i]) {
            if ((s8)key[i] < (s8)name[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return findSelectableFuncData(table, n, key);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return findSelectableFuncData(entry + 1, n, key);
        }
        if (key[i] == '\0')
            break;
    }
    return entry;
}

namespace gt2k_menu {

/* The inline accessors (out-of-line copies; Lupo 2 names where index.txt has
 * them). */

// FUNC 0x001B80B8 gt2k_menu::PageManager::endFadeTimer
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B80C0 gt2k_menu::PageManager::setDecideButtons
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B80D0 gt2k_menu::PageManager::isDecided
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B80D8 gt2k_menu::PageManager::isExit
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B80E0 gt2k_menu::PageManager::doExit
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B80F0 gt2k_menu::PageManager::ignoreCancel
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B80F8 gt2k_menu::PageManager::getCurrentPage
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8100 gt2k_menu::PageManager::getControl
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8108 gt2k_menu::PageManager::getButtonList
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8120 gt2k_menu::PageManager::getPopupLevel
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8128 gt2k_menu::PageManager::disableSound
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8130 gt2k_menu::PageManager::getMostRootObject
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8138 gt2k_menu::PageManager::getLayer
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8150 gt2k_menu::PageManager::getFragmentManager
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8158 gt2k_menu::PageManager::findPage
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy (LinkManager::findPage) */
/* implicit: findPage (the inline body) */

// FUNC 0x001B8178 gt2k_menu::PageManager::getLinkManager
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8180 gt2k_menu::PageManager::setFocusFuncData
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B8190 gt2k_menu::PageManager::setValidFuncData
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B81A0 gt2k_menu::PageManager::setExecuteFuncData
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B81B0 gt2k_menu::PageManager::setMenuFuncData
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B81C0 gt2k_menu::PageManager::setSelectableFuncData
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy */

// FUNC 0x001B81D0 gt2k_menu::PageManager::setMultiFuncs
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy (PageLayout::setMultiFuncs
 * of common and own) */
/* implicit: setMultiFuncs (the inline body) */

// FUNC 0x001B8230 gt2k_menu::PageManager::setSliderFuncs
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy (PageLayout::setSliderFuncs
 * of common and own) */
/* implicit: setSliderFuncs (the inline body) */

// FUNC 0x001B8290 gt2k_menu::PageManager::setHooks
/* inline in game/menu/framework/T211_page_manager.h; egcs emits this copy (PageLayout::setHooks of
 * common and own) */
/* implicit: setHooks (the inline body) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__user_type_info: no bases). */
// FUNC 0x001B82F0 __tfQ29gt2k_menu11PageManager
extern "C" void *__tfQ29gt2k_menu11PageManager(void)
{
    if (__tiQ29gt2k_menu11PageManager[0] == 0)
        __rtti_user(__tiQ29gt2k_menu11PageManager, "Q29gt2k_menu11PageManager");
    return __tiQ29gt2k_menu11PageManager;
}
#endif
