/* T369 - choosing an enemy line: the first existing line of a kind's
 * fallback list (g_002DA508), the side lines of a kind. */
/* Retail's .rodata of this file: <new>'s "bad_alloc" and UnitArena<64>'s
 * type name, nothing references them (u086's data pass found the seven
 * blocks, ref/data/u087/pending.md). */
// DATA_START 0x0033DDA0
#include <new>
#define GAME_T153_EMITS_TYPEINFO           /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"
#include "game/race/ai/T369_line_select.h"
#include "game/race/ai/enemy_lines_internal.h"

/* line kind -> up to 6 lines to try, -1 ends (retail's .data: not const;
 * the last two of each row are 0). */
s8 g_002DA508[8][6] = {                     /* 0x002DA508 */
    { -1, -1, -1, -1 },
    {  1,  6,  2, -1 },
    {  2,  6,  1, -1 },
    {  3, -1, -1, -1 },
    {  4, -1, -1, -1 },
    { -1, -1, -1, -1 },
    {  6,  1,  2, -1 },
    {  7,  6, -1, -1 },
};

/* The line to use for `kind' (>= 8: the racing line's list): the first line
 * of its list that exists; -1 if none (or no line file).  Callers:
 * DynamicsConductor::linePointWorld, ai_nearest_line, ai_reset, ai_change_line,
 * lines_find_sector_on_racing_line, lines_corner_entry_before,
 * lines_get_point. */
// FUNC 0x001F9BF8 lines_select
int lines_select(CourseLines *cl, u32 kind)
{
    u32 list;
    int tried;

    /* (the if / else: kind < 8 tested, list = 6 then movn kind; a ?: or
     * `list = 6; if (kind < 8)' tests 7 < kind) */
    if (kind >= 8)
        list = LINE_RACING;
    else
        list = kind;
    if (cl->data == NULL)
        return -1;
    const s8 *entry = g_002DA508[list];
    int lineNo = *entry;
    tried = 0;
    /* The do-while under an if: the first test branches to the final
     * return.  cl->data re-read in the loop: retail's loop.c hoisted
     * &data->line and the -1; with a local `file' egcs's PRE puts
     * file + 8 on the loop's entry edge instead and loop.c skips the loop
     * as phony (-dL), so the -1 stays in it (exact under -fno-gcse). */
    if (lineNo != -1) {
        do {
            if (cl->data->line[lineNo] != NULL)
                return lineNo;
            tried++;
            if (!(tried < 6))
                break;
            entry++;
            lineNo = *entry;
        } while (lineNo != -1);
    }
    return -1;
}

/* kind 1 or 6: line 2 if it exists; else -1. */
// FUNC 0x001F9C80 lines_side_line_2
int lines_side_line_2(EnemyLines *file, int kind)
{
    /* one return -1 (an inner `return -1' is if-converted to movn) */
    if (kind == LINE_SIDE_1 || kind == LINE_RACING) {
        if (file->line[LINE_SIDE_2] != NULL)
            return LINE_SIDE_2;
    }
    return -1;
}

/* kind 2 or 6: line 1 if it exists; else -1. */
// FUNC 0x001F9CB0 lines_side_line_1
int lines_side_line_1(EnemyLines *file, int kind)
{
    if (kind == LINE_SIDE_2 || kind == LINE_RACING) {
        if (file->line[LINE_SIDE_1] != NULL)
            return LINE_SIDE_1;
    }
    return -1;
}

/* The side line of `kind' at course distance dist: lines_side_line_2's
 * choice off line 5's arcs, lines_side_line_1's on them (line 5 searched
 * from node `hint').  Caller: ai_avoid_traffic. */
// FUNC 0x001F9CE0 lines_side_line
int lines_side_line(CourseLines *cl, int kind, int hint, f32 dist)
{
    int onArc = line5_arc_at(cl, hint, dist);
    EnemyLines *file = cl->data;

    if (onArc == 0)
        return lines_side_line_2(file, kind);
    return lines_side_line_1(file, kind);
}

/* As lines_side_line, the other way round.  Caller: ai_avoid_traffic. */
// FUNC 0x001F9D38 lines_side_line_other
int lines_side_line_other(CourseLines *cl, int kind, int hint, f32 dist)
{
    int onArc = line5_arc_at(cl, hint, dist);
    EnemyLines *file = cl->data;

    if (onArc == 0)
        return lines_side_line_1(file, kind);
    return lines_side_line_2(file, kind);
}
