/* T091_records_items.h - the page handlers of the records screen and their
 * tables (translation unit T091, unit u040), 0x001731B0..0x001735A0;
 * gt2k_menu::MenuItemArray.
 *
 * RecordsControl::begin (T090) gives its PageManager the MenuFuncData table
 * g_00351C88 {"menu_records", create_menu_records}, the ExecuteFuncData
 * table g_00351C98 {"delete", records_action_delete} and both layouts the
 * hooks g_002B1450: "entry_code1..3", CallbackText items (T239,
 * create_callback_text) showing the three 10-character parts of the ranking
 * code of the place the panel was opened on as "XXXX-XX-XXXX".
 * T091_static_init fills the three tables.
 *
 * gt2k_menu::MenuItemArray (u040 / u063's name) is T225's ::MenuItemArray
 * (the same constructor 0x001C1520 and setEntry 0x001C15F0, the first
 * pass's F_001C15F0); the users name it without the namespace. */
#ifndef GAME_T091_RECORDS_ITEMS_H
#define GAME_T091_RECORDS_ITEMS_H

#include "game/game.h"
#include "game/menu/framework/T225_menu_item_array.h"      /* ::MenuItemArray (create_menu_records' entries) */
#include "game/menu/framework/T211_page_manager.h"         /* MenuFuncData, ExecuteFuncData; T229's MenuHook (the tables) */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class ButtonList;                           /* T244 */
class ControlBase;                          /* T247 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct LinkPage;                            /* T241 */
struct ButtonData;                          /* T244 */

/* The per-entry data of a list built at run time: T225's (see above). */
/* (No typedef here: ::MenuItemArray is found from inside gt2k_menu, and a
 * gt2k_menu::MenuItemArray would be ambiguous under `using namespace` in egcs.) */
}

/* "menu_records" (MenuFuncData, ctx: the RecordsControl): the list of
 * places of ctl->arg, one entry per place (a MenuItemArray of
 * arg->getCount() copies of page->items[0] on the layout's first item,
 * labelled ""), a RecordsButtonList (T092) built centred in rect with
 * controlType / loop, a FreeAgent holding the entries' block hung under it.
 * NULL if the layout has no item.  parent is not used.  (u040: Object *,
 * RecordsControl *ctl, MenuElement *elem = the LinkPage.) */
gt2k_menu::ButtonList *create_menu_records(void *ctx, gt2k_menu::LinkPage *page,
        gt2k_menu::MenuLayoutItem *rect, gt2k_menu::Object *parent, int controlType, int loop,
        gt2k_menu::PageLayout *layout, int index);     /* 0x001731B0 */
/* "delete" (ExecuteFuncData, ctx: the RecordsControl): arg->deleteRecord(
 * index), the place the panel was opened on.  Only ctx is read (u040:
 * RecordsControl *ctl, const char *arg). */
void records_action_delete(void *ctx, gt2k_menu::ButtonData *rec, int index,
                           gt2k_menu::PageLayout *layout);    /* 0x00173378 */
/* A 10-character code part as "XXXX-XX-XXXX" (13 bytes with the NUL). */
void records_format_code(char *buf, const char *code);       /* 0x001733A8 */
/* The CallbackText callbacks (T239 CallbackTextFn; owner: the
 * RecordsControl): characters 0..9 / 10..19 / 20..29 of the ranking code of
 * place index; size is not used. */
void records_text_code1(gt2k_menu::ControlBase *owner, char *buf, int size);  /* 0x001733F0 */
void records_text_code2(gt2k_menu::ControlBase *owner, char *buf, int size);  /* 0x00173438 */
void records_text_code3(gt2k_menu::ControlBase *owner, char *buf, int size);  /* 0x00173480 */
/* .ctors entry 0x0028F0BC: the tables' initialisers (names, functions; the
 * hooks' p8 = the callback, the other words 0). */
void T091_static_init(void);                /* 0x001734C8 */

/* T091's data (counts in .sdata next to the tables).  The two tables are
 * declared without a size, as the original's header had them: although they
 * sit in .sdata, retail's T090 and T091 address them with lui/addiu (egcs
 * marks a symbol small data when its first declaration is seen, by the size
 * known then); the counts are $gp-relative. */
extern gt2k_menu::MenuFuncData    g_00351C88[];     /* 0x00351C88 gp-0x5D68 {"menu_records", create_menu_records} */
extern int                        g_00351C90;       /* 0x00351C90 gp-0x5D60 1, its count */
extern gt2k_menu::ExecuteFuncData g_00351C98[];     /* 0x00351C98 gp-0x5D58 {"delete", records_action_delete} */
extern int                        g_00351CA0;       /* 0x00351CA0 gp-0x5D50 1, its count */
extern int                        g_00351CA4;       /* 0x00351CA4 gp-0x5D4C 3, the count of g_002B1450 */
extern gt2k_menu::MenuHook        g_002B1450[3];    /* 0x002B1450 "entry_code1..3" (create_callback_text,
                                                       p8 records_text_code1..3) */

#endif /* GAME_T091_RECORDS_ITEMS_H */
