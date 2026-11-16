/* T241 - gt2k_menu::LinkManager, link_manager.cxx (0x001C7220..0x001C7A20).
 *
 * The page records of a loaded .mbl ("menu link") file, PageManager+0x008:
 * a name-sorted table of LinkPage records (binary search), each with its
 * buttons (ButtonData, which lead to other pages, with sub lists), the
 * number of its layout file ("<dir>/<country>/<name><n>.pmb"; 1 = the
 * common one) and the page imported (drawn) with it.
 *
 * Data (retail .rodata 0x00336B60..0x00336BAF, .sdata 0x003526C8): the
 * literals of getImportPmbPath and makePmbPath only, no type name.
 */
#include "game/menu/framework/T241_link_manager.h"
#include "game/menu/framework/T244_button_data.h"          /* gt2k_menu::ButtonData */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_message */
#include "game/common/T494_text_utils.h"           /* str_equal */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "game/data/T516_country.h"              /* Country, g_00352FD8 */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/framework/menu_links_internal.h"

namespace gt2k_menu {

enum { PMB_COMMON = 1 };                    /* LinkPage::pmb of the common layout */

/* Caller: PageManager::PageManager (0x001B6250). */
// FUNC 0x001C7220 gt2k_menu::LinkManager::LinkManager
LinkManager::LinkManager()
{
    data = 0;
    current = 0;
    duplicateLinks = 0;
    path[0] = 0;
    pmbPath[0] = 0;
    importName[0] = 0;
}

/* Reads the whole file (unpacked, into a new malloc block) and turns its
 * offsets into pointers.  Caller: 0x001B6570 (T211). */
// FUNC 0x001C7240 gt2k_menu::LinkManager::load
int LinkManager::load(const char *file)
{
    LinkFile *loaded = (LinkFile *)file_read_blocking(file, 0, 0, 1, &vol_device_object);
    if (loaded == 0)
        return 0;
    strcpy(path, file);
    data = loaded;
    relocate_link_file(loaded, loaded);
    return 1;
}

/* Caller: 0x001B6608 (T211). */
// FUNC 0x001C72C0 gt2k_menu::LinkManager::unload
void LinkManager::unload()
{
    if (data != 0) {
        free(data);
        data = 0;
        path[0] = 0;
    }
}

/* Callers: 0x001B6338 (T211), clear. */
// FUNC 0x001C7300 gt2k_menu::LinkManager::reset
void LinkManager::reset()
{
    current = 0;
    duplicateLinks = 0;
    pmbPath[0] = 0;
    importName[0] = 0;
}

/* Caller: 0x001B6438 (T211). */
// FUNC 0x001C7318 gt2k_menu::LinkManager::clear
void LinkManager::clear()
{
    reset();
}

/* Caller: PageManager::setPage (0x001B66E0). */
// FUNC 0x001C7338 gt2k_menu::LinkManager::setPage
int LinkManager::setPage(const char *name)
{
    importName[0] = 0;
    LinkPage *page = findPage(name);
    if (page != 0) {
        current = page;
        checkDuplicateLinks();
        return 1;
    }
    current = 0;
    return 0;
}

// FUNC 0x001C7380 gt2k_menu::LinkManager::findPage
LinkPage *LinkManager::findPage(const char *name)
{
    LinkPage **entry = bsearch_page(data->pages, data->count, name);
    if (entry == 0)
        return 0;
    return *entry;
}

// FUNC 0x001C73B8 gt2k_menu::LinkManager::hasImportPage
int LinkManager::hasImportPage()
{
    if (current != 0 && (current->import[0] != 0 || importName[0] != 0))
        return 1;
    return 0;
}

// FUNC 0x001C73F0 gt2k_menu::LinkManager::getImportPageName
const char *LinkManager::getImportPageName()
{
    if (importName[0] == 0)
        return current->import;
    return importName;
}

/* importName = name if that page exists, else "".  Not exact (84.5%): this
 * and name swap $s0 / $s1 (global-alloc: name has the shorter life, so the
 * higher priority; retail gives this $s0).  Tried: the tests as || / nested
 * ifs / a page local / early returns, &importName[0], a dst local; the
 * reference compiler gives the same. */
// FUNC 0x001C7410 gt2k_menu::LinkManager::setImportPage
void LinkManager::setImportPage(const char *name)
{
    if (name != 0 && findPage(name) != 0)
        strcpy(importName, name);
    else
        importName[0] = 0;
}

// FUNC 0x001C7468 gt2k_menu::LinkManager::getFlag0
int LinkManager::getFlag0()
{
    return current->flags & 1;
}

// FUNC 0x001C7478 gt2k_menu::LinkManager::getFlag1
int LinkManager::getFlag1()
{
    return (current->flags >> 1) & 1;
}

/* duplicateLinks = 1 if two buttons of the current page lead to the same
 * page (current is read again after every compare). */
// FUNC 0x001C7490 gt2k_menu::LinkManager::checkDuplicateLinks
void LinkManager::checkDuplicateLinks()
{
    if (current != 0) {
        for (int i = 0; i < current->count; i++) {
            const char *link = current->items[i]->link;
            for (int j = i + 1; j < current->count; j++) {
                if (str_equal(link, current->items[j]->link) != 0) {
                    duplicateLinks = 1;
                    return;
                }
            }
        }
    }
    duplicateLinks = 0;
}

/* Index of the button of the current page that leads to `link`; 0 when
 * there is none (as for the first button).  Caller 0x001B7268 (T211). */
// FUNC 0x001C7580 gt2k_menu::LinkManager::findButton
int LinkManager::findButton(const char *link)
{
    if (current != 0 && link != 0) {
        for (int i = 0; i < current->count; i++) {
            if (str_equal(current->items[i]->link, link) != 0)
                return i;
        }
    }
    return 0;
}

/* The .pmb file name of layout n: the .mbl name with the country directory
 * inserted after every '/', its extension replaced by "<n>.pmb"
 * ("menu/x.mbl" -> "menu/US/x1.pmb").  Quirk: without a '.' the number is
 * written from pmbPath[-1]. */
// FUNC 0x001C7618 gt2k_menu::LinkManager::makePmbPath
const char *LinkManager::makePmbPath(int n)
{
    const char *country = g_00352FD8.code();
    int src = 0;
    int dst = 0;

    if (path[0] != 0) {
        do {
            char c = path[src];
            pmbPath[dst] = c;
            dst++;
            src++;
            if (c == '/') {
                strcpy(&pmbPath[dst], country);
                dst += strlen(country);
                pmbPath[dst] = c;
                dst++;
            }
        } while (path[src] != 0);
    }
    pmbPath[dst] = 0;

    /* dst again for the '.' (retail keeps both in one register) */
    for (dst = strlen(pmbPath) - 1; dst >= 0 && pmbPath[dst] != '.'; dst--)
        ;
    sprintf(&pmbPath[dst], "%d.pmb", n);
    return pmbPath;
}

/* Caller: 0x001B6570 (T211). */
// FUNC 0x001C7778 gt2k_menu::LinkManager::getCommonPmbPath
const char *LinkManager::getCommonPmbPath()
{
    return makePmbPath(PMB_COMMON);
}

// FUNC 0x001C7798 gt2k_menu::LinkManager::getPagePmbPath
const char *LinkManager::getPagePmbPath()
{
    if (current == 0)
        return 0;
    return makePmbPath(current->pmb);
}

// FUNC 0x001C77C8 gt2k_menu::LinkManager::getImportPmbPath
const char *LinkManager::getImportPmbPath()
{
    if (current == 0)
        return 0;
    const char *name = getImportPageName();
    LinkPage *page = findPage(name);
    if (page == 0) {
        /* EUC-JP "インポートページの記述が間違っています" (the import page
         * entry is wrong), then the page, the file and the line */
        menu_message("\xA5\xA4\xA5\xF3\xA5\xDD\xA1\xBC\xA5\xC8\xA5\xDA\xA1\xBC\xA5\xB8"
                     "\xA4\xCE\xB5\xAD\xBD\xD2\xA4\xAC\xB4\xD6\xB0\xE3\xA4\xC3\xA4\xC6"
                     "\xA4\xA4\xA4\xDE\xA4\xB9\n[%s]:%s:%d",
                     name, "../../link_manager.cxx", 0x151);
        return 0;
    }
    return makePmbPath(page->pmb);
}

// FUNC 0x001C7848 gt2k_menu::LinkManager::isCommonPage
int LinkManager::isCommonPage()
{
    return current->pmb == PMB_COMMON;
}

// FUNC 0x001C7860 gt2k_menu::LinkManager::isImportCommon
int LinkManager::isImportCommon()
{
    if (hasImportPage() == 0)
        return 0;
    LinkPage *page = findPage(getImportPageName());
    if (page == 0)
        return 0;
    return page->pmb == PMB_COMMON;
}

/* Depth-first search of the button `name` in list and its sub lists. */
// FUNC 0x001C78B8 gt2k_menu::find_button
ButtonData *find_button(ButtonData **list, int count, const char *name)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(name, list[i]->name) == 0)
            return list[i];
        ButtonData *found = find_button(list[i]->subItems, list[i]->subCount, name);
        if (found != 0)
            return found;
    }
    return 0;
}

/* Caller: RawWindow::loadChild (0x001D74F0, T264). */
// FUNC 0x001C7950 gt2k_menu::LinkManager::findButtonData
ButtonData *LinkManager::findButtonData(const char *name)
{
    if (current == 0)
        return 0;
    return find_button(current->items, current->count, name);
}

/* Binary search of `name` in the name-sorted pages (the strcmp is inlined,
 * signed chars).  NULL if absent.  The name-search shape of the menu units
 * (tail recursion, each half's count in its own variable; ref/MATCHING.md). */
// FUNC 0x001C7988 gt2k_menu::bsearch_page
LinkPage **bsearch_page(LinkPage **pages, int count, const char *name)
{
    if (count <= 0)
        return 0;
    LinkPage **entry = &pages[count / 2];
    const char *key = (*entry)->name;
    for (int i = 0; ; i++) {
        if (name[i] != key[i]) {
            if (name[i] < key[i]) {
                int n = count / 2;
                if (n <= 0)
                    return 0;
                return bsearch_page(pages, n, name);
            }
            int n = count - count / 2 - 1;
            if (n <= 0)
                return 0;
            return bsearch_page(entry + 1, n, name);
        }
        if (name[i] == '\0')
            break;
    }
    return entry;
}

} /* namespace gt2k_menu */
