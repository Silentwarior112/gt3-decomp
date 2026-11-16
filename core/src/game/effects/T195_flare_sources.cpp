/* T195_flare_sources.cpp - the flare sources of a course (0x0019AB38..
 * 0x0019AD00): the flare section of the course file is relocated in place
 * (its pointers were made for the address in +0x04) and its groups of
 * sources are handed to FlareClass::addSource.  The records are local to
 * this unit (race_display_draw_internal.h); T195 has no canonical header.
 *
 * Data (u055's data pass): the original T195 has no .rodata (no type name,
 * not the "" of T157's inline RaceMessageDisplay::clear) and no .sdata: its
 * .data is its frame info alone (0x002BECAC).  It sees FlareClass (whose key
 * function is in T159: no name here) and its own records only. */
#include "types.h"
#include "game/effects/T159_flare.h"                /* FlareClass::addSource, FlareSource */
#include "game/race/display/race_display_draw_internal.h"                           /* the records FlareSourceData, FlareGroupTable, FlareGroup */

/* Moves a pointer of the section by delta (the distance between where the
 * section is and where its pointers were made for).  An inline helper in
 * the original: retail recomputes the slot's address (index + base, then
 * + 4) and reloads it after the NULL test, as an inline function's
 * reference argument gives (FlareGroupTable::relocate). */
template <class T> static inline void reloc_ptr(T *&p, int delta)
{
    p = (T *)((u8 *)p + delta);
}

// FUNC 0x0019AB38 FlareSourceData::relocate
//   Caller: T179's course_data_relocate (the course data, for its +0x74
//   section).  No table (present == 0): table = NULL.  Else the pointers
//   move by the distance between this and the address they were made for.
void FlareSourceData::relocate()
{
    int delta;

    if (present == 0) {
        table = NULL;
        return;
    }
    delta = (int)((u8 *)this - (u8 *)base);
    base = this;
    if (delta != 0 && table != NULL) {
        reloc_ptr(table, delta);
        table->relocate(delta);
    }
}

// FUNC 0x0019AB88 FlareGroupTable::relocate
//   Moves every non-NULL group pointer by delta.
void FlareGroupTable::relocate(int delta)
{
    int i;

    for (i = 0; i < count; i++) {
        if (group[i] != NULL)
            reloc_ptr(group[i], delta);
    }
}

// FUNC 0x0019ABD8 FlareSourceData::submit
//   Caller: T179's course_submit_flares (for each of its list of group
//   numbers, or -1 for all).  group < 0: every group (the table pointer is
//   reloaded after each submission); else that one when it is in range.
void FlareSourceData::submit(FlareClass *fc, int group)
{
    int i;

    if (table == NULL)
        return;
    if (group < 0) {
        for (i = 0; i < table->count; i++) {
            if (table->group[i] != NULL)
                table->group[i]->submit(fc);
        }
    } else if (group < table->count) {
        if (table->group[group] != NULL)
            table->group[group]->submit(fc);
    }
}

// FUNC 0x0019AC90 FlareGroup::submit
//   FlareClass::addSource for each source of the group (the count
//   is reloaded every time).
void FlareGroup::submit(FlareClass *fc)
{
    int i;

    for (i = 0; i < count; i++)
        fc->addSource(&src[i]);
}
