/* T517 - GT2KRealTimeText: the texts that change with the language at run
 * time.  Every table (a static GT2KRealTimeText subclass per message group,
 * e.g. GT2KMSGGTMODE::Message) links itself into g_realtimeTables;
 * text/realtime.dat holds a sorted index of "<table>_<key>" names and one
 * block of texts per language.  realtime_set_language points each table's
 * texts[] into the block of a language (empty texts keep the built-in
 * default). */
#include "engine/gfx/vram_internal.h"
#include "engine/text/T517_realtime_text.h"

/* T517's .sdata, in the original's order: g_realtimeData (initialised, so
 * written here), realtime_set_language's "_", then g_realtimeTables (a
 * global without an initialiser: zeros at the end of the file's output). */
RealTimeData     *g_realtimeData = NULL;        /* 0x00353018 gp-0x49D8 */
GT2KRealTimeText *g_realtimeTables;             /* 0x00353024 gp-0x49CC */

/* An offset in the file -> a pointer (in place).  The field's address is the
 * reference argument: retail computes &entries[i].name into a register for
 * the load and the store. */
template <class T> static inline void realtime_reloc(T *&p, const void *base)
{
    p = (T *)((u32)p + (u32)base);
}

// FUNC 0x002454E0 realtime_find
//   Binary search of the index by name (strcmp); NULL if not found or the
//   index is empty.
RealTimeEntry *realtime_find(const char *name)
{
    int lo = 0;
    int hi = (int)g_realtimeData->count;

    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        int cmp = strcmp(name, g_realtimeData->entries[mid].name);

        if (cmp == 0)
            return &g_realtimeData->entries[mid];
        if (cmp < 0)
            hi = mid;
        else
            lo = mid + 1;
    }
    return NULL;
}

// FUNC 0x00245588 realtime_load
//   Once: reads text/realtime.dat (unpacked, from vol_device_object), turns
//   its offsets into pointers and applies the current language.  Caller:
//   game_init.
void realtime_load(void)
{
    RealTimeData *data;
    u32 i;
    int lang;

    if (g_realtimeData != NULL)
        return;
    data = (RealTimeData *)file_read_blocking("text/realtime.dat", NULL, 0, 1,
                                              &vol_device_object);
    g_realtimeData = data;
    realtime_reloc(data->texts, data);
    for (i = 0; i < g_realtimeData->count; i++)
        realtime_reloc(g_realtimeData->entries[i].name, data);
    lang = g_00352FD8.value;
    realtime_set_language(lang);
}

// FUNC 0x00245628 realtime_set_language
//   For every key of every table: the text of "<name>_<key>" in language
//   lang's block, if it is in the index and not empty.  g_realtimeData is
//   not NULL-checked (Country::set before realtime_load would crash).
void realtime_set_language(int lang)
{
    char key[0x400];
    GT2KRealTimeText *table = g_realtimeTables;
    const char *langTexts = g_realtimeData->texts + g_realtimeData->langSize * lang;
    RealTimeEntry *entry;
    int i;

    for (; table != NULL; table = table->next) {
        for (i = 0; i < table->count; i++) {
            strcpy(key, table->name);
            strcat(key, "_");
            strcat(key, table->keys[i]);
            entry = realtime_find(key);
            if (entry != NULL && strlen(langTexts + entry->text) != 0)
                table->texts[i] = langTexts + entry->text;
        }
    }
}

// FUNC 0x00245728 GT2KRealTimeText::GT2KRealTimeText
//   Pushes the table on the front of g_realtimeTables.  texts[] holds the
//   defaults until a language is applied.
GT2KRealTimeText::GT2KRealTimeText(const char *tableName, const char **tableKeys,
                                   const char **tableTexts, int n)
{
    name = tableName;
    next = g_realtimeTables;
    keys = tableKeys;
    texts = tableTexts;
    count = n;
    g_realtimeTables = this;
}

// FUNC 0x00245758 GT2KRealTimeText::~GT2KRealTimeText
//   Only the vptr store: the table stays linked in g_realtimeTables (as in
//   the binary).  egcs's deleting form calls operator delete when
//   (flags & 1).
GT2KRealTimeText::~GT2KRealTimeText()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00245788 __tf16GT2KRealTimeText
extern "C" void *__tf16GT2KRealTimeText(void)
{
    if (__ti16GT2KRealTimeText[0] == 0)
        __rtti_user(__ti16GT2KRealTimeText, "16GT2KRealTimeText");
    return __ti16GT2KRealTimeText;
}
#endif
