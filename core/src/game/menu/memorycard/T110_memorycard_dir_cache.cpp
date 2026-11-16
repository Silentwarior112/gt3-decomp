/* T110 - gt2k_menu::MemorycardDirCache, the card directory listing of a
 * MemorycardFileManager (T109).  0x0014E8A8..0x0014ED40.
 *
 * The listing of the pattern is read page by page (pageSize 0x40-byte
 * ::McDirEntry records, T501 mc_get_dir) when an entry of the page is asked
 * for; a page goes to a free page, else to the least recently used one (the
 * largest stamp - page.stamp).
 *
 * Includes (data pass): retail's T110 has no .rodata (its .data is the frame
 * info alone): no header that defines a class with only inline virtual
 * functions (those put their type names into every object that sees them). */
#include "game/menu/memorycard/T110_memorycard_dir_cache.h"
#include "engine/io/T501_memorycard_manager.h"   /* ::McDirEntry, mc_get_dir, mc_slot_dir_count */
#include "game/menu/memorycard/memorycard_menu_internal.h"                           /* libc.h: strcpy */

namespace gt2k_menu {

/* pageCount = maxEntries / pageSize, rounded up; every page gets its
 * entries. */
// FUNC 0x0014E8A8 gt2k_menu::MemorycardDirCache::MemorycardDirCache
MemorycardDirCache::MemorycardDirCache(int maxEntries, int pageSize)
{
    int count = (maxEntries + pageSize - 1) / pageSize;
    int i;

    /* The pages and the entries come from operator new[]; neither type has
     * a constructor, so there is no array cookie. */
    port = 0;
    pattern[0] = '\0';
    this->pageSize = pageSize;
    pageCount = count;
    pages = new Page[count];                /* implicit: operator new[] */
    stamp = 0;
    for (i = 0; i < pageCount; i++) {
        /* pages[i] indexed in each statement: retail computes i * 16 + pages
         * (a local Page pointer gives pages + i * 16) */
        pages[i].index = -1;
        pages[i].stamp = 0;
        pages[i].count = 0;
        pages[i].entries = new McDirEntry[this->pageSize]; /* implicit: operator new[] */
    }
}

/* Called with the egcs flags (bit 0: operator delete afterwards) by
 * ~MemorycardFileManager and its constructor's cleanup. */
// FUNC 0x0014E968 gt2k_menu::MemorycardDirCache::~MemorycardDirCache
MemorycardDirCache::~MemorycardDirCache()
{
    int i;

    /* operator delete[] of each page's entries, then of the pages; operator
     * delete(this) when the flags ask for it. */
    for (i = 0; i < pageCount; i++)
        delete[] pages[i].entries;          /* implicit: operator delete[] */
    delete[] pages;                         /* implicit: operator delete[] */
    /* implicit: operator delete (the deleting destructor, flags & 1) */
}

/* Forgets the pages read. */
// FUNC 0x0014EA10 gt2k_menu::MemorycardDirCache::clear
void MemorycardDirCache::clear()
{
    int i;

    stamp = 0;
    for (i = 0; i < pageCount; i++) {
        pages[i].index = -1;
        pages[i].stamp = 0;                 /* the constructor's order (count first gives
                                               the two stores swapped) */
        pages[i].count = 0;
    }
}

// FUNC 0x0014EA58 gt2k_menu::MemorycardDirCache::setPattern
void MemorycardDirCache::setPattern(const char *pattern)
{
    strcpy(this->pattern, pattern);
    clear();
}

/* The page holding page number index, NULL if it is not read. */
// FUNC 0x0014EA88 gt2k_menu::MemorycardDirCache::findPage
MemorycardDirCache::Page *MemorycardDirCache::findPage(int index)
{
    int i;

    for (i = 0; i < pageCount; i++) {
        if (pages[i].index == index)
            return &pages[i];
    }
    return NULL;
}

/* The first free page, else the one least recently used; emptied, its
 * stamp one older than the current one.  (With no page at all the stores
 * go through NULL, as in the original.) */
// FUNC 0x0014EAC8 gt2k_menu::MemorycardDirCache::getFreePage
MemorycardDirCache::Page *MemorycardDirCache::getFreePage()
{
    int oldest = -1;
    Page *victim = NULL;
    int i;

    for (i = 0; i < pageCount; i++) {
        Page *page = &pages[i];
        int age;

        if (page->index < 0) {
            victim = page;
            break;
        }
        age = stamp - page->stamp;
        if (oldest < age) {
            oldest = age;
            victim = page;
        }
    }
    victim->count = 0;
    victim->index = -1;
    victim->stamp = stamp - 1;
    return victim;
}

/* Lists page number index into page; NULL when nothing was found or the
 * listing failed (error keeps mc_get_dir's status). */
// FUNC 0x0014EB50 gt2k_menu::MemorycardDirCache::readPage
MemorycardDirCache::Page *MemorycardDirCache::readPage(Page *page, int index)
{
    int found;

    error = mc_get_dir(port, pattern, page->entries, index * pageSize, pageSize);
    found = mc_slot_dir_count(port);
    if (found == 0 || error != 0)
        return NULL;
    page->count = found;
    page->index = index;
    return page;
}

/* Entry i of the listing (its page read if needed), NULL past the end or
 * when the page cannot be read. */
// FUNC 0x0014EBD8 gt2k_menu::MemorycardDirCache::getEntry
McDirEntry *MemorycardDirCache::getEntry(int i)
{
    int index = i / pageSize;
    int offset = i % pageSize;
    Page *page = findPage(index);

    if (page == NULL) {
        page = readPage(getFreePage(), index);
        if (page == NULL)
            return NULL;
    }
    if (offset >= page->count)
        return NULL;
    if (page->stamp != stamp)
        page->stamp = ++stamp;
    return &page->entries[offset];
}

/* Returns the status of the last listing and clears it. */
// FUNC 0x0014EC90 gt2k_menu::MemorycardDirCache::takeError
int MemorycardDirCache::takeError()
{
    int status = error;

    error = 0;
    return status;
}

/* The number of entries: reads page after page until one is missing (the
 * first entry of each); -1 on an error. */
// FUNC 0x0014ECA0 gt2k_menu::MemorycardDirCache::countEntries
int MemorycardDirCache::countEntries()
{
    int first = 0;
    Page *page;

    while (getEntry(first) != NULL)
        first += pageSize;
    if (error != 0)
        return -1;
    if (first == 0)
        return 0;
    first -= pageSize;                      /* the last page found */
    page = findPage(first / pageSize);
    if (page == NULL)
        return -1;
    return first + page->count;
}

} /* namespace gt2k_menu */
