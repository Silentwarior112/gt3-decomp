/* T368 - the course's enemy lines (CourseData +0x7C): preparing the loaded
 * file, ground queries along a line, the point of a line at a course
 * distance.  The object is CourseLines (DynamicsConductor +0xBBA8). */
/* Retail's .rodata of this file: <new>'s "bad_alloc" and UnitArena<64>'s
 * type name, nothing references them (u086's data pass found the seven
 * blocks, ref/data/u087/pending.md). */
// DATA_START 0x0033DD78
#include <new>
#define GAME_T153_EMITS_TYPEINFO           /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"
#include "game/race/ai/T368_enemy_lines.h"
#include "game/race/ai/enemy_lines_internal.h"

/* The order lines_prepare works through the lines (the racing line first:
 * line 4 needs its sectors); retail's .sdata (-G8). */
const u8 g_003529C0[8] = { 6, 0, 1, 2, 5, 3, 4, 7 };   /* 0x003529C0 */

/* An int stored in a float field of the loaded file, as a float. */
static inline f32 fixed_value(const f32 *field)
{
    union { f32 f; s32 i; } raw;
    raw.f = *field;
    return (f32)raw.i;
}

/* The runway sector under (x, y), searched from `sector'; -1 off the road.
 * Callers: lines_trace_segment, lines_prepare. */
// FUNC 0x001F8BA0 lines_find_sector
int lines_find_sector(RaceCourse *course, int sector, f32 x, f32 y)
{
    RunwayContact contact;          /* sp+0x0 (declared first: the lower slot) */
    f32 point[3];                   /* sp+0x40 (stored, only copied) */

    point[0] = x;
    point[1] = y;
    point[2] = U087_PROBE_HEIGHT;
    contact.x = point[0];
    contact.y = point[2];
    contact.z = -point[1];
    contact.sector = sector;
    course->getContact(&contact);
    if (contact.offRoad != 0)
        return -1;
    return contact.sector;
}

/* The sector at the end of segment n-1 -> n of a line, followed along it:
 * a straight in steps of about 10 m, an arc in steps of about 10 degrees.
 * For an arc it sets nodes[n-1].kind: 1 (radius > 0) or 2 (radius < 0;
 * line 5: 1 for both).  Caller: lines_prepare. */
// FUNC 0x001F8BF8 lines_trace_segment
int lines_trace_segment(RaceCourse *course, LineNode *nodes, int n, int sector, int kind)
{
    int prev = n - 1;
    LineNode *from = &nodes[prev];
    f32 fromX = from->x;
    f32 fromY = from->y;
    LineNode *to = &nodes[n];
    f32 toX = to->x;
    f32 toY = to->y;
    int steps, step;

    if (from->kind == 0) {
        f32 dx = toX - fromX;
        f32 dy = toY - fromY;
        /* (the volatile sqrt: retail loads 10.0 and the 1s after it) */
        steps = (int)(boot_sqrtf_v(dx * dx + dy * dy) / 10.0f);
        if (steps == 0)
            steps = 1;
        for (step = 1; step <= steps; step++) {
            f32 t = (f32)step / (f32)steps;
            sector = lines_find_sector(course, sector, fromX + t * dx, fromY + t * dy);
        }
        return sector;
    }

    f32 heading = from->angle;
    f32 negSin = -sinf(heading);
    f32 cosH = cosf(heading);
    f32 radius = from->radius;
    f32 centreX = fromX - cosH * radius;    /* the centre of the arc */
    f32 centreY = fromY + negSin * radius;
    f32 startAngle = atan2f(centreX - fromX, fromY - centreY);
    f32 endAngle = atan2f(centreX - toX, toY - centreY);
    /* (the step count in each arm: their cvt / mfc1 / movz tails are
     * cross-jumped, the constant 1 stays in each arm as in retail) */
    if (0.0f < radius) {
        from->kind = 1;
        if (endAngle < startAngle)
            endAngle += U087_TWO_PI;
        steps = (int)((endAngle - startAngle) / U087_TEN_DEG);
        if (steps == 0)
            steps = 1;
    } else {
        from->kind = 2;
        radius = -radius;
        if (startAngle < endAngle)
            startAngle += U087_TWO_PI;
        steps = (int)((startAngle - endAngle) / U087_TEN_DEG);
        if (steps == 0)
            steps = 1;
    }
    if (kind == LINE_ARCS)
        nodes[prev].kind = 1;
    for (step = 1; step <= steps; step++) {
        f32 phi = startAngle + ((f32)step / (f32)steps) * (endAngle - startAngle);
        f32 px = centreX - sinf(phi) * radius;
        sector = lines_find_sector(course, sector, px, centreY + cosf(phi) * radius);
    }
    return sector;
}

/* The road height at (x, y) probed from z; *miss: off the road (then 0 and
 * *sector unchanged).  Callers: lines_ground_at,
 * lines_find_sector_on_racing_line, lines_get_point. */
// FUNC 0x001F8EC8 lines_ground_height
f32 lines_ground_height(RaceCourse *course, f32 x, f32 y, f32 z, int *sector, int *miss)
{
    RunwayContact contact;          /* sp+0x0 */
    f32 point[3];                   /* sp+0x40 (stored, only copied) */

    point[0] = x;
    point[1] = y;
    point[2] = z;
    contact.x = point[0];
    contact.y = point[2];
    contact.z = -point[1];
    contact.sector = *sector;
    course->getContact(&contact);
    *miss = contact.offRoad;
    if (*miss == 0) {
        *sector = contact.sector;
        return contact.height;
    }
    return 0.0f;
}

/* The road under a node at (x, y) heading `angle' (forwards is (-sin, cos)
 * of it, as everywhere in the lines): the height, the surface, the
 * contact's +0x32 byte (always 0: getContact clears it), +0x33 (attr2) bit
 * 0, and the pitch / roll from four points 0.5 m ahead / behind / across
 * (-+ (cos, sin)).  0: off the road, or both points of a pair off the road.
 * Caller: lines_prepare. */
// FUNC 0x001F8F40 lines_ground_at
int lines_ground_at(RaceCourse *course, f32 *height, f32 *pitch, f32 *roll, int *surface,
                    int *attr, int *flag, int sector, f32 x, f32 y, f32 angle)
{
    RunwayContact contact;          /* sp+0x0 */
    f32 point[3];                   /* sp+0x40 (stored, only copied) */
    int hint;                       /* sp+0x50 */
    int missAhead, missBehind, missSideMinus, missSidePlus;
    f32 centre, halfSin, halfCos, ahead, behind, sideMinus, sidePlus, rise, run;

    hint = sector;
    point[0] = x;
    point[1] = y;
    point[2] = U087_PROBE_HEIGHT;
    contact.x = point[0];
    contact.y = point[2];
    contact.z = -point[1];
    contact.sector = sector;
    course->getContact(&contact);
    /* one assignment: the 0.0 in a register ($f0, swc1), which CSE then
     * also uses for *height = 0 below (two plain stores give sw $zero) */
    *pitch = *roll = 0.0f;
    hint = contact.sector;
    if (contact.offRoad != 0) {
        *flag = 0;
        *surface = 0;
        *height = 0.0f;
        *attr = 0;
        return 0;
    }
    centre = contact.height;
    *height = centre;
    *flag = contact.attr2 & 1;
    *surface = ground_surface_type(&contact);
    *attr = contact.unk_32;

    halfSin = sinf(angle) * 0.5f;
    halfCos = cosf(angle) * 0.5f;
    ahead     = lines_ground_height(course, x - halfSin, y + halfCos, U087_PROBE_HEIGHT,
                                    &hint, &missAhead);
    behind    = lines_ground_height(course, x + halfSin, y - halfCos, U087_PROBE_HEIGHT,
                                    &hint, &missBehind);
    sideMinus = lines_ground_height(course, x - halfCos, y - halfSin, U087_PROBE_HEIGHT,
                                    &hint, &missSideMinus);
    sidePlus  = lines_ground_height(course, x + halfCos, y + halfSin, U087_PROBE_HEIGHT,
                                    &hint, &missSidePlus);
    if (missAhead != 0 && missBehind != 0)
        return 0;
    if (missSideMinus != 0 && missSidePlus != 0)
        return 0;

    /* each slope over the pair, or over the half that is on the road */
    if (missAhead != 0) {
        run = 0.5f;
        rise = centre - behind;
    } else if (missBehind != 0) {
        run = 0.5f;
        rise = ahead - centre;
    } else {
        rise = ahead - behind;
        run = 1.0f;
    }
    *pitch = atan2f(rise, run);

    if (missSideMinus != 0) {
        run = 0.5f;
        rise = centre - sidePlus;
    } else if (missSidePlus != 0) {
        run = 0.5f;
        rise = sideMinus - centre;
    } else {
        rise = sideMinus - sidePlus;
        run = 1.0f;
    }
    *roll = atan2f(rise, run);
    return 1;
}

/* For the first node of line 4: the sector (and height) at (x, y), found
 * from the sector hints of the racing line's nodes, last node first.  0:
 * none.  Caller: lines_prepare. */
// FUNC 0x001F91E8 lines_find_sector_on_racing_line
int lines_find_sector_on_racing_line(CourseLines *cl, int *sector, f32 *height, f32 x, f32 y)
{
    int racing = lines_select(cl, LINE_RACING);
    int n;

    if (racing == -1)
        return 0;
    Line *line = cl->data->line[racing];
    LineNode *nodes = line->node;           /* (retail keeps line + 4) */
    for (n = line->count - 1; n >= 0; n--) {
        int hint = nodes[n].sector;         /* sp+0x0 */
        int miss;                           /* sp+0x4 */
        f32 h = lines_ground_height(cl->course, x, y, U087_PROBE_HEIGHT, &hint, &miss);
        if (miss == 0) {
            *height = h;
            *sector = hint;
            return 1;
        }
    }
    return 0;
}

/* Prepares the loaded lines once (state > 0 the first time): relocates the
 * line offsets (an empty line becomes NULL), converts the fixed point
 * values, then per node finds the runway sector (unless some node of some
 * line already has one), the course distance (line 4: + a lap from the node
 * on where it passes the start), the road height / pitch / roll / surface
 * (not for line 5).  Returns 1.  Caller: lines_init. */
// FUNC 0x001F92F8 lines_prepare
int lines_prepare(CourseLines *cl)
{
    int raw = 0;                    /* sp+0x40 */
    f32 lap = cl->course->getLapLength();    /* 0x0018FA10 */
    EnemyLines *file = cl->data;
    int k, n;
    int cached;

    if (file->state > 0) {
        raw = 1;
        file->state = -file->state;
    }
    if (raw != 0) {
        for (k = 0; k < 8; k++) {
            Line **slot = &file->line[g_003529C0[k]];
            if (*slot != NULL) {
                Line *line = (Line *)((u8 *)file + (u32)*slot);
                *slot = line;
                if (line->count == 0)
                    *slot = NULL;
            }
        }
    }

    /* the sectors are cached: some node of some line has one */
    cached = 0;
    for (k = 0; k < 8; k++) {
        Line *line = file->line[g_003529C0[k]];
        if (line == NULL)
            continue;
        LineNode *node = line->node;
        for (n = 0; n < line->count; n++, node++) {
            if (node->sector != 0) {
                cached = 1;
                break;
            }
            if (cached == 1)
                break;
        }
        if (cached == 1)
            break;
    }

    for (k = 0; k < 8; k++) {
        u8 kind = g_003529C0[k];
        Line *line = cl->data->line[kind];
        if (line == NULL)
            continue;
        LineNode *nodes = line->node;
        int count = line->count;
        int pastStart = 0;          /* sp+0x44 */
        int sector = -1;            /* sp+0x20 */
        f32 startHeight;            /* sp+0x24 (not used) */

        for (n = 0; n < count; n++) {
            LineNode *node;
            f32 point[3];           /* sp+0x0 */

            if (raw != 0) {
                node = &nodes[n];
                node->x = -fixed_value(&node->x) * U087_FIXED_ONE;
                node->angle = fixed_value(&node->angle) * U087_TWO_PI * U087_FIXED_ONE + U087_PI;
                node->radius = fixed_value(&node->radius) * U087_FIXED_ONE;
                node->y = -fixed_value(&node->y) * U087_FIXED_ONE;
            }
            node = &nodes[n];
            point[0] = node->x;
            point[1] = node->y;
            point[2] = U087_PROBE_HEIGHT;
            if (cached != 0) {
                sector = node->sector;
            } else {
                if (kind != LINE_PIT) {
                    if (n != 0)
                        sector = lines_trace_segment(cl->course, nodes, n, sector, kind);
                    else
                        sector = lines_find_sector(cl->course, sector, point[0], point[1]);
                } else if (n == 0) {
                    lines_find_sector_on_racing_line(cl, &sector, &startHeight, point[0], point[1]);
                } else {
                    sector = lines_find_sector(cl->course, sector, point[0], point[1]);
                }
                nodes[n].sector = (s16)sector;
            }

            RunwayPos query;        /* sp+0x10 */
            query.x = point[0];
            query.y = point[2];
            query.z = -point[1];
            query.sector = sector;
            node = &nodes[n];
            node->dist = cl->course->getLapDistance(&query);    /* 0x0018FA20 */
            node->height = 0.0f;
            node->pitch = 0.0f;
            node->roll = 0.0f;
            node->surface = 0;
            node->attr = 0;
            if (kind != LINE_ARCS) {
                f32 groundHeight, groundPitch, groundRoll;  /* sp+0x28.. */
                int groundSurface, groundAttr, groundFlag;  /* sp+0x34.. */
                if (lines_ground_at(cl->course, &groundHeight, &groundPitch, &groundRoll,
                                    &groundSurface, &groundAttr, &groundFlag, sector,
                                    node->x, node->y, node->angle) != 0) {
                    node->height = groundHeight;
                    node->pitch = groundPitch;
                    node->roll = groundRoll;
                    node->surface = (s8)groundSurface;
                    node->attr = (u8)groundAttr;
                }
            }
            if (kind == LINE_PIT) {
                /* the lane crosses the start line: from the first node in the
                 * first half of the lap on, the distances run on past it */
                node = &nodes[n];
                f32 dist = node->dist;
                if (dist < lap * 0.5f)
                    pastStart = 1;
                if (pastStart == 1)
                    node->dist = dist + lap;
            }
        }
    }
    return 1;
}

/* On the racing line: the course distance (- a lap) of the last node at
 * which a tight arc (|radius| <= maxRadius) starts after a straight / wide
 * part, with its distance <= dist + a lap; dist if none.
 * Caller: DynamicsConductor::virtual_25 (0x001F34B8). */
// FUNC 0x001F9708 lines_corner_entry_before
f32 lines_corner_entry_before(CourseLines *cl, f32 dist, f32 maxRadius)
{
    int racing = lines_select(cl, LINE_RACING);
    int i;

    if (racing == -1)
        return dist;
    Line *line = cl->data->line[racing];
    int count = line->count;
    LineNode *nodes = line->node;
    f32 limit = cl->course->getLapLength() + dist;    /* 0x0018FA10 */

    for (i = count - 1; i >= 0; i--) {
        int prev = i - 1;
        if (!(-1 < prev))
            prev += count;
        int prevTight = nodes[prev].kind;
        int curTight = nodes[i].kind;
        if (prevTight != 0) {
            f32 r = nodes[prev].radius;
            if (maxRadius < r || r < -maxRadius)
                prevTight = 0;
        }
        if (curTight != 0) {
            f32 r = nodes[i].radius;
            if (maxRadius < r || r < -maxRadius)
                curTight = 0;
        }
        if (prevTight != 0)
            continue;
        if (curTight == 0)
            continue;
        LineNode *entry = &nodes[i];
        if (!(limit < entry->dist))
            return entry->dist - cl->course->getLapLength();
    }
    return dist;
}

/* The point of line `kind' (lines_select) at course distance dist (wrapped
 * into the lap; clamp: not before the last node - a lap): the position, the
 * road height (probed from the line's height; once more from 1 m higher
 * with no sector hint if that misses), the heading towards the point 10 m
 * on, the sector.  0: no such line.  Caller: DynamicsConductor::racingLinePoint. */
// FUNC 0x001F9880 lines_get_point
int lines_get_point(CourseLines *cl, u32 kind, int *sector, f32 *x, f32 *y, f32 *height,
                    f32 *angle, int clamp, f32 dist)
{
    f32 lap = cl->course->getLapLength();    /* 0x0018FA10 */
    int lineNo = lines_select(cl, kind);
    int node;
    /* Scalars (retail packs them at sp+0x0 / sp+0xC; arrays of 3 are
     * 16-byte aligned).  i is the node search's index and then the probe's
     * miss flag: addressed, it lives in its slot, which the loop stores to
     * (and the loop's first test is rolled to the end and copied, i = 0
     * known in the copy). */
    f32 hereX, hereY, hereZ;        /* sp+0x0 */
    f32 aheadX, aheadY, aheadZ;     /* sp+0xC */
    int hint;                       /* sp+0x18 */
    int i;                          /* sp+0x1C */

    if (lineNo == -1)
        return 0;
    Line *line = cl->data->line[lineNo];
    int count = line->count;
    LineNode *nodes = line->node;

    if (clamp != 0) {
        f32 first = nodes[count - 1].dist - lap;
        if (dist < first)
            dist = first;
    }
    while (dist < 0.0f)
        dist += lap;
    while (lap <= dist)
        dist -= lap;

    node = count - 1;
    for (i = 0; i < count; i++) {
        if (dist <= nodes[i].dist) {
            node = i;
            break;
        }
    }

    line_point_at(cl, lineNo, node, dist, &hereX, &hereY, &hereZ);
    line_point_at(cl, lineNo, node, dist + 10.0f, &aheadX, &aheadY, &aheadZ);
    hint = nodes[node].sector;
    *x = hereX;
    *y = hereY;
    *height = lines_ground_height(cl->course, hereX, hereY, hereZ, &hint, &i);
    if (i != 0) {
        int retryHint;              /* sp+0x20 */
        hereZ = hereZ + 1.0f;
        retryHint = -1;
        *height = lines_ground_height(cl->course, hereX, hereY, hereZ, &retryHint, &i);
        if (i == 0)
            hint = retryHint;
    }
    *angle = atan2f(-aheadX + hereX, aheadY - hereY);
    *sector = hint;
    return 1;
}

/* course, data = course->data->enemyLine, lines_prepare; hasPit: lines 3
 * and 4 both exist.  Caller: DynamicsConductor::initialize (slot 22). */
// FUNC 0x001F9B98 lines_init
void lines_init(CourseLines *cl, RaceCourse *course)
{
    cl->course = course;
    cl->data = course->data->enemyLine;
    lines_prepare(cl);
    cl->hasPit = 1;
    if (cl->data->line[LINE_PIT_IN] == NULL || cl->data->line[LINE_PIT] == NULL)
        cl->hasPit = 0;
}
