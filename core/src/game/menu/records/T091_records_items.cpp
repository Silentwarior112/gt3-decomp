/* T091 - the page handlers of the records screen and their tables
 * (game/menu/records/T091_records_items.h).  0x001731B0..0x001735A0.
 *
 * T091_static_init fills the three tables RecordsControl::begin (T090)
 * hands to its PageManager: the MenuFuncData {"menu_records",
 * create_menu_records} (g_00351C88), the ExecuteFuncData {"delete",
 * records_action_delete} (g_00351C98) and the hooks of both layouts
 * (g_002B1450): "entry_code1..3", CallbackText items (create_callback_text,
 * T239) showing the three 10-character parts of the ranking code of the
 * place the panel was opened on as "XXXX-XX-XXXX".  Every handler's context
 * is the RecordsControl. */
#include "libc.h"                           /* memcpy */
/* Retail's .rodata (0x00325390..0x00325640): the literals, then the type
 * names in the reverse order of the classes' definitions: List / Cell<Button>,
 * RecordsControl, RecordsMenuArg, SmartIndexT<int>, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, the GameStatus's garage lists,
 * Operator. */
#include "game/menu/framework/T217_operator.h"             /* (seen) */
#include "game/save/T429_game_status.h"          /* (seen, not used) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/framework/T211_page_manager.h"         /* MenuFuncData, ExecuteFuncData; SmartIndexT<int> */
#include "game/menu/records/T089_records_menu_args.h"    /* RecordsMenuArg (getCount, getCode, deleteRecord) */
#include "game/menu/records/T090_records_control.h"      /* RecordsControl (arg, index) */
#include "game/menu/framework/T206_button.h"               /* (Button, Cell<Button>) */
#include "game/menu/framework/T244_button_list.h"          /* ButtonList, ButtonData */
#include "game/menu/records/T092_records_button_list.h"  /* RecordsButtonList */
#include "game/menu/framework/T225_menu_item_array.h"      /* ::MenuItemArray */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout (firstPageItem), MenuLayoutItem, MenuHook */
#include "game/menu/framework/T236_free_agent.h"           /* FreeAgent */
#include "game/menu/framework/T239_callback_text.h"        /* create_callback_text */
#include "game/menu/framework/T241_link_manager.h"         /* LinkPage */
#include "game/menu/framework/T220_object.h"               /* Object::attachTo */
#include "game/menu/records/T091_records_items.h"

using namespace gt2k_menu;

/* "menu_records": the list of places of the records shown, one entry per
 * place (each a copy of the page's first button record on the layout's
 * first item, labelled ""), a RecordsButtonList built centred in rect; a
 * FreeAgent holding the entries' block hangs under it.  NULL if the layout
 * has no item.  `parent' is not used.  The landing pads free the list's and
 * the FreeAgent's blocks (Object::operator delete). */
// FUNC 0x001731B0 create_menu_records
ButtonList *create_menu_records(void *ctx, LinkPage *page, MenuLayoutItem *rect, Object *parent,
                                int controlType, int loop, PageLayout *layout, int index)
{
    RecordsControl *control = static_cast<RecordsControl *>(ctx);
    RecordsMenuArg *records = control->arg;
    MenuLayoutItem *firstItem = layout->firstPageItem();

    if (firstItem == NULL)
        return NULL;
    int count = records->getCount();
    ::MenuItemArray entries(count);         /* sp+0x20 */
    ButtonData *tmpl = page->items[0];
    for (int i = 0; i < count; i++)
        entries.setEntry(i, tmpl, firstItem, "");   /* "" at 0x00351C80 */
    /* (Through MenuItemArray's inline accessors: retail keeps &entries in
     * $s6 for them, set before the loop and on its bypass, as u009's
     * create_menu_wheel.) */
    RecordsButtonList *list = new RecordsButtonList(controlType, loop, layout, entries.getItems(), /* implicit: Object::operator delete (if the constructor throws) */
                                                    count, rect, entries.getRects(), index, control,
                                                    records);
    FreeAgent *agent = new FreeAgent(entries.getBuffer());  /* inline: Object(), the vptr, mem */ /* implicit: Object::operator delete (if the constructor throws) */
    agent->attachTo(list);
    return list;
}

/* "delete": deletes the place the panel was opened on (only ctx is read). */
// FUNC 0x00173378 records_action_delete
void records_action_delete(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    RecordsControl *control = static_cast<RecordsControl *>(ctx);

    control->arg->deleteRecord(control->index);
}

/* A 10-character code part as "XXXX-XX-XXXX" (13 bytes with the NUL).
 * Callers: the three entry_code texts.  Three memcpys, then the dashes:
 * egcs's MIPS block move copies the 2 bytes as lb, lb, sb, sb (retail's
 * signed loads, both ahead of the stores). */
// FUNC 0x001733A8 records_format_code
void records_format_code(char *buf, const char *code)
{
    memcpy(buf, code, 4);
    memcpy(buf + 5, code + 4, 2);
    memcpy(buf + 8, code + 6, 4);
    buf[4] = '-';
    buf[7] = '-';
    buf[12] = 0;
}

/* "entry_code1": characters 0..9 of the ranking code of the place the panel
 * was opened on (`size' is not used). */
// FUNC 0x001733F0 records_text_code1
void records_text_code1(ControlBase *owner, char *buf, int size)
{
    RecordsControl *control = static_cast<RecordsControl *>(owner);

    records_format_code(buf, control->arg->getCode(control->index));
}

/* "entry_code2": characters 10..19. */
// FUNC 0x00173438 records_text_code2
void records_text_code2(ControlBase *owner, char *buf, int size)
{
    RecordsControl *control = static_cast<RecordsControl *>(owner);

    records_format_code(buf, control->arg->getCode(control->index) + 10);
}

/* "entry_code3": characters 20..29. */
// FUNC 0x00173480 records_text_code3
void records_text_code3(ControlBase *owner, char *buf, int size)
{
    RecordsControl *control = static_cast<RecordsControl *>(owner);

    records_format_code(buf, control->arg->getCode(control->index) + 20);
}

/* The tables and their counts (zero in .sdata / .data: T091_static_init
 * fills the tables), defined here, where the original defined them: .sdata
 * holds create_menu_records' "" (0x00351C80), then these, then the
 * initialiser's "delete" (0x00351CA8).  The header declares the tables
 * without a size (retail addresses them with lui/addiu).  T090's
 * RecordsControl::begin references the five words too (it comes first in
 * the link): the anchors give them to this file, which defines them. */
// DATA_START 0x00351C88
// DATA_START 0x00351C90
// DATA_START 0x00351C98
// DATA_START 0x00351CA0
// DATA_START 0x00351CA4
MenuFuncData    g_00351C88[1] = { { NULL, NULL } };                      /* 0x00351C88 */
int             g_00351C90 = sizeof(g_00351C88) / sizeof(g_00351C88[0]); /* 0x00351C90 */
ExecuteFuncData g_00351C98[1] = { { NULL, NULL } };                      /* 0x00351C98 */
int             g_00351CA0 = sizeof(g_00351C98) / sizeof(g_00351C98[0]); /* 0x00351CA0 */
MenuHook        g_002B1450[3] = { { NULL, NULL, 0, 0, 0, 0 } };          /* 0x002B1450 */
int             g_00351CA4 = sizeof(g_002B1450) / sizeof(g_002B1450[0]); /* 0x00351CA4 */

/* One table entry: the inline constructors of the original's MenuFuncData /
 * ExecuteFuncData / MenuHook (see T091_static_init); the fields in member
 * order. */
static inline void set_menu(MenuFuncData *e, const char *name,
                            ButtonList *(*create)(void *ctx, LinkPage *page, MenuLayoutItem *rect,
                                                  Object *parent, int controlType, int loop,
                                                  PageLayout *layout, int index))
{
    e->name   = name;
    e->create = create;
}

static inline void set_action(ExecuteFuncData *e, const char *name,
                              void (*func)(void *ctx, ButtonData *rec, int index,
                                           PageLayout *layout))
{
    e->name = name;
    e->func = func;
}

static inline void set_hook(MenuHook *e, const char *name, MenuHookFn *create,
                            int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* One element as egcs's build_vec_init expands it (u006 / u015's form): the
 * constructor on a copy of the cursor, then the cursor and the count of the
 * elements left stepped. */
#define T091_HOOK(name, create, p8, pC)                                           { MenuHook *e = hooks; set_hook(e, name, create, p8, pC, 0, 0); }             hooks++;                                                                      hooksLeft--

/* Static initialiser (.ctors entry 0x0028F0BC).  Compiler-generated in the
 * original (_GLOBAL_$I$...): the three tables were defined in this unit with
 * objects built by inline constructors, which egcs builds at run time
 * (cp/init.c build_vec_init: a walking cursor per table, in definition
 * order; .rodata gets "menu_records" before "entry_code1..3", as retail; the
 * scheduler interleaves the stores).  Written out the way build_vec_init
 * expands it (a cursor, a copy of it per element, the dead count of the
 * elements left): an exact form, not the original; it stores the same 1 + 1
 * + 3 entries.  The hooks: create_callback_text with p8 = the text callback,
 * pC = 0 (the text is made before every draw). */
// FUNC 0x001734C8 T091_static_init
void T091_static_init(void)
{
    MenuFuncData *menus = g_00351C88;
    int menusLeft = 0;                      /* build_vec_init's iterator: elements - 1 (dead) */
    { MenuFuncData *e = menus; set_menu(e, "menu_records", create_menu_records); }
    menus++;
    menusLeft--;

    ExecuteFuncData *actions = g_00351C98;
    int actionsLeft = 0;                    /* the same (dead) */
    { ExecuteFuncData *e = actions; set_action(e, "delete", records_action_delete); }
    actions++;
    actionsLeft--;

    MenuHook *hooks = g_002B1450;
    int hooksLeft = 2;                      /* the same (dead) */
    T091_HOOK("entry_code1", create_callback_text, (int)records_text_code1, 0);
    T091_HOOK("entry_code2", create_callback_text, (int)records_text_code2, 0);
    T091_HOOK("entry_code3", create_callback_text, (int)records_text_code3, 0);
}
