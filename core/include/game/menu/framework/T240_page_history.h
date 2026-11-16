/* T240_page_history.h - gt2k_menu::PageHistory (translation unit T240, unit
 * u069), 0x001C6FC0..0x001C7220.
 *
 * The page "back" stack of the PageManager (+0x1E4): a ring of 32 page
 * names between `first' (the oldest) and `top' (one past the newest), empty
 * when they are equal.  push() writes at top and advances it, dropping the
 * oldest name when the ring is full; both indices wrap (clamp 0) over
 * 0 .. 31.  T240 ends with a copy of the SmartIndexT<int> typeinfo function
 * (0x001C71E0, see T251_smart_index.h). */
#ifndef GAME_T240_PAGE_HISTORY_H
#define GAME_T240_PAGE_HISTORY_H

#include "game/game.h"
#include "game/common/T251_smart_index.h"          /* SmartIndexT<int> (first, top) */

namespace gt2k_menu {

/* Not polymorphic.  Size 0x830 (PageManager+0x1E4 .. +0xA14). */
class PageHistory {
public:
    PageHistory();                          /* 0x001C6FC0 both indices over [0, 32), wrapping */
    void        push(const char *page);     /* 0x001C7078 */
    void        pop();                      /* 0x001C70F8 drops the newest (nothing if empty) */
    const char *getTop();                   /* 0x001C7138 the newest, NULL if empty */
    const char *getFirst();                 /* 0x001C71C0 the oldest, NULL if empty */

    char             names[32][0x40];       /* 0x000 */
    SmartIndexT<int> first;                 /* 0x800 the oldest name */
    SmartIndexT<int> top;                   /* 0x818 one past the newest */
};
EGCS_SIZE(gt2k_menu::PageHistory, 0x830);

} /* namespace gt2k_menu */

#endif /* GAME_T240_PAGE_HISTORY_H */
