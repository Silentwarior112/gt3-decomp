/* T110_memorycard_dir_cache.h - gt2k_menu::MemorycardDirCache, the card
 * directory listing of a file manager (translation unit T110, unit u028),
 * 0x0014E8A8..0x0014ED40.
 *
 * The listing of a sceMcGetDir pattern ("/BASCUS-97102R*", ...) is read
 * page by page (pageSize entries, T501 mc_get_dir 0x0023F9C0) when an entry
 * of the page is asked for; the page goes to a free page, else to the one
 * least recently used (the largest stamp - page.stamp).  A member of
 * MemorycardFileManager (T109, +0xFA08: 0x200 entries, pages of 0x20).
 *
 * gt2k_menu::McDirEntry (u025 / u028) is the 0x40-byte directory entry of
 * T501 that u024 / u110 declare at global scope: one record, ::McDirEntry of
 * T108_memorycard_file.h (T501 uses that name).  There is no gt2k_menu
 * typedef of it: egcs makes a name declared both at global scope and in
 * gt2k_menu ambiguous wherever `using namespace gt2k_menu' is in effect;
 * code inside the namespace finds the global one unqualified (T108's
 * gt2k_menu::McFileEntry is the namespace's other name for it). */
#ifndef GAME_T110_MEMORYCARD_DIR_CACHE_H
#define GAME_T110_MEMORYCARD_DIR_CACHE_H

#include "game/game.h"

struct McDirEntry;                          /* T501 (pointers only): {create, modify, size, attr,
                                               unk_16, pdaAplNo, name[0x24] +0x1C}, 0x40 bytes */

namespace gt2k_menu {

/* Not polymorphic.  Size 0x5C (MemorycardFileManager::dirCache, T109). */
class MemorycardDirCache {
public:
    /* A page of the listing (pageCount of them, operator new[]). */
    struct Page {
        int         index;                  /* 0x0 page number; -1: free */
        int         count;                  /* 0x4 entries read */
        int         stamp;                  /* 0x8 the cache's stamp when last used */
        McDirEntry *entries;                /* 0xC pageSize entries (operator new[]) */
    };

    /* pageCount = maxEntries / pageSize rounded up, each page with its
     * entries; port 0, no pattern. */
    MemorycardDirCache(int maxEntries, int pageSize);   /* 0x0014E8A8 */
    ~MemorycardDirCache();                  /* 0x0014E968 the entries, then the pages (delete[]) */
    void        clear();                    /* 0x0014EA10 forgets the pages read; stamp 0 */
    void        setPattern(const char *pattern);    /* 0x0014EA58 copies it, then clear() */
    Page       *findPage(int index);        /* 0x0014EA88 the page holding page number index, NULL
                                               if it is not read */
    Page       *getFreePage();              /* 0x0014EAC8 a free page, else the least recently used
                                               one; emptied, stamp - 1 */
    Page       *readPage(Page *page, int index);    /* 0x0014EB50 lists page number index into page;
                                               NULL when nothing was found or on an error */
    McDirEntry *getEntry(int i);            /* 0x0014EBD8 entry i (its page read if needed); NULL
                                               past the end or when the page cannot be read */
    int         takeError();                /* 0x0014EC90 returns error and clears it */
    int         countEntries();             /* 0x0014ECA0 reads page after page; -1 on an error */
    /* inline: MemorycardFileManager::setPort 0x00147B40 expands it (its
     * `this' = manager + 0xFA08 in a register, `sw 0x10($a0)'; a direct
     * dirCache.port store gives `sw 0x7A18(manager + 0x8000)'); the name as
     * Lupo 2's MemorycardFileList::setPort, no copy in this build */
    void        setPort(int p) { port = p; }

    Page *pages;                            /* 0x00 */
    int   pageCount;                        /* 0x04 */
    int   pageSize;                         /* 0x08 entries per page */
    int   stamp;                            /* 0x0C use counter */
    int   port;                             /* 0x10 T501 card id (port << 2), set by the manager */
    char  pattern[0x44];                    /* 0x14 the sceMcGetDir pattern */
    int   error;                            /* 0x58 mc_get_dir's status of the last listing */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::MemorycardDirCache::Page, 0x10);
EGCS_SIZE(gt2k_menu::MemorycardDirCache, 0x5C);

#endif /* GAME_T110_MEMORYCARD_DIR_CACHE_H */
