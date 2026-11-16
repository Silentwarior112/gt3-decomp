/* T069_memorycard_items.h - the items of the memory card screens
 * ("menu/memorycard.mbl", run by MemorycardControl, T068): gt2k_menu::
 * SlotTitle, TitleView, ProgressBar, SingleIconView, IconView, and the hook
 * table of the page (translation unit T069, unit u027),
 * 0x0014B610..0x0014E880.
 *
 * T069's static initialiser fills the 51-entry hook table g_002A4E20 that
 * MemorycardControl::begin hands to the page layouts:
 *   - the texts of the file information pages (create_callback_text
 *     0x001C6E70 of T239, p8 = a text callback (MemorycardControl *ctl, char
 *     *buf, int size)): size, dates, comment, title lines, copy protection of
 *     any file ("o_pt_*"); money / days / cars / licence / achievement of a
 *     game save ("g_pt_*"); time, laps, car, course, power of a replay
 *     ("r_pt_*"), of a best lap / ghost ("b_pt_*", plus the mass) and of a
 *     setting file ("s_pt_*"); the free space of both cards; "text_box";
 *   - the factories of the five item classes owned here:
 *       SlotTitle       "title_bar_slot1/2"   dims with its slot
 *       TitleView       "title_base"          the screen title of the mode
 *       ProgressBar     "progress_bar"        the save / load progress
 *       SingleIconView  "op_icon_face", "confirm_icon_face",
 *                       "working_icon_face"   the selected file's icon,
 *                       flying between three places of the screen
 *       IconView        "icon_view_slot1/2"   the grid of a card's icons
 *
 * All five: vtables of 8 slots, slots 3..7 Item's (Object::virtual_3
 * 0x001BFF00, Item::virtual_4 0x001BA898, Image::virtual_5 0x001CF358,
 * Transitive::virtual_6 / _7 0x001BB9F8 / 0x001BBA00); vptr +0x1C
 * (Object's).  Slots 1 / 2 keep T220's names virtual_1 (update) / virtual_2
 * (draw); the other build names these overrides update(void) /
 * render(void).  Built by the factories with Object's operator new (the
 * menu heap); the constructors are inline in the factories, except
 * TitleView's.
 *
 * The five classes are defined in T069's source, not here (u027's data
 * pass, 2026-10-17): no other original saw them (their type names are only
 * in T069's .rodata), and the original defined each class with its member
 * bodies in the class, followed by its factory.  egcs emits such inline
 * members at the end of the unit per class (the members in reverse order,
 * the typeinfo function, the implicit destructor), classes in reverse
 * definition order: T069's frame info and .text order show it.  Their
 * vtables have 8 slots, slots 3..7 Item's (see above); vptr +0x1C.
 *
 * The first pass's types: u027's MenuElement / MenuImage / MenuFile are
 * T229's MenuLayoutItem / T213's MenuTexture / T229's PageLayout, its
 * gt2k_menu::MotionValue is ::MotionValue (T260; no gt2k_menu typedef: egcs
 * makes the name ambiguous under `using namespace gt2k_menu'), its
 * gt2k_menu::Color4f is ::Color4f (T218), its MemcardHook is T229's MenuHook. */
#ifndef GAME_T069_MEMORYCARD_ITEMS_H
#define GAME_T069_MEMORYCARD_ITEMS_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item, MenuTexture */
#include "game/menu/framework/T218_color.h"                /* Color4f (baseColor) */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue (IconView::scroll) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (g_002A4E20), PageLayout */
#include "game/menu/memorycard/T072_memorycard_icon_sprites.h"  /* gt2k_menu::MemorycardIconSprites (sprites) */

namespace gt2k_menu {
class PageManager;                          /* T211 (pointers only) */
class MemorycardControl;                    /* T068 */
struct MemorycardSlot;                      /* T071 */
class MemorycardFile;                       /* T108 */
struct MemorycardIconRenderInfo;            /* T108 */
struct MenuLayoutItem;                      /* T229 */

/* u027 / u063's name of the layout file (0x84 bytes, passed by value to
 * MemorycardIconSprites). */
typedef PageLayout MenuFile;

} /* namespace gt2k_menu */

/* The five item classes (IconView, SingleIconView, ProgressBar, TitleView,
 * SlotTitle) are defined in T069's source, each followed by its factory:
 * see the top of this header. */

/* The hook table of the memory card page (the names sorted; its 51 entries
 * counted by g_00351808): zero in the file, filled by T069_static_init. */
extern gt2k_menu::MenuHook g_002A4E20[51];  /* 0x002A4E20..0x002A52E8 (.data) */
/* The entries of g_002A4E20 (= 51), read by MemorycardControl::begin (T068,
 * 0x00149AF8).  T069's .sdata: after its strings 0x003517A0..0x00351800,
 * before T070's g_00351810, as T070's own count g_00351818 follows its
 * table. */
extern int g_00351808;                      /* 0x00351808 gp-0x61E8 */

void T069_static_init(void);                /* 0x0014C548 (.ctors 0x0028F094) fills g_002A4E20 */

/* "r_pt_course": the course name of the selected replay's thumbnail
 * (RaceDataBase on the stack); buf emptied without one. */
void memcard_text_replay_course(gt2k_menu::MemorycardControl *ctl, char *buf,
                                int size);  /* 0x0014BD10 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu9SlotTitle(void);         /* 0x0014DB98 */
void *__tfQ29gt2k_menu9TitleView(void);         /* 0x0014DC50 */
void *__tfQ29gt2k_menu11ProgressBar(void);      /* 0x0014DDA0 */
void *__tfQ29gt2k_menu14SingleIconView(void);   /* 0x0014E108 */
void *__tfQ29gt2k_menu8IconView(void);          /* 0x0014E590 */
}
#endif

#endif /* GAME_T069_MEMORYCARD_ITEMS_H */
