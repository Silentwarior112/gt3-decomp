/* T457 - Runway (? name): the road surface of the course.
 *
 * The file (CourseData+0x70) is loaded somewhere and relocated in place
 * (runway_relocate).  The road is cut into sectors of triangles; two xz
 * cell maps give, per cell, the triangles (triMap) or the points (pointMap)
 * of each sector touching it.  A sector also lists its neighbours and the
 * lap lines crossing it (the distance along the lap).  Queries take a
 * sector hint: only that sector and its neighbours are searched when the
 * hint is valid.
 */
/* .rodata (0x00341380..0x003413B9, referenced by nothing): "bad_alloc"
 * (<new>, read first), then the type names in the reverse order of the
 * classes' definitions: RaceCameraBase, UnitArena<64> (T153 with
 * GAME_T153_EMITS_TYPEINFO).  Retail's T458 has no .rodata; T457 is first
 * in link order after u102's T455 (ref/data/u103/notes.md). */
// DATA_START 0x00341380
#include <new>                              /* first: "bad_alloc" */
#include "game/menu/gtmode/prize_model_internal.h"
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"          /* RunwayQuery, RunwayContact; UnitArena<64>'s name */
#include "game/camera/T383_race_camera_base.h"     /* (no use: RaceCameraBase's name) */
#include "game/car/physics/T167_rigid_body.h"           /* WallSeg (Runway::walls) */
#include "game/course/T457_runway.h"

/* runway_find_interval: the value is below the first / at or above the last
 * bound. */
#define RUNWAY_BELOW ((int)0x80000001)
#define RUNWAY_ABOVE 0x7FFFFFFE

// FUNC 0x00227378 runway_map_relocate
//   Relocates the cell table and the (nx - 1) * (nz - 1) cell pointers.
void runway_map_relocate(RunwayCellMap *m, int delta)
{
    u16 **p;
    int i, j;
    int nz, nx;

    nz = m->nz;
    nx = m->nx;
    u103_reloc(m->cells, delta);
    p = m->cells;
    for (j = 1; j < nz; j++) {
        for (i = 1; i < nx; i++, p++)
            U103_RELOC(*p, delta);
    }
}

// FUNC 0x002273F0 runway_find_interval
//   Binary search of v in n ascending bounds: the interval i with
//   bounds[i] <= v < bounds[i + 1], or RUNWAY_BELOW / RUNWAY_ABOVE.
int runway_find_interval(const f32 *bounds, int n, f32 v)
{
    int hi = n - 1;
    int lo = 0;

    if (v < bounds[0])
        return RUNWAY_BELOW;
    if (bounds[hi] <= v)
        return RUNWAY_ABOVE;
    if (hi > 0) {
        do {
            int mid = (lo + hi) >> 1;
            if (v < bounds[mid])
                hi = mid;
            else
                lo = mid + 1;
        } while (lo < hi);
    }
    return lo - 1;
}

// FUNC 0x00227478 runway_map_cell
u16 *runway_map_cell(const RunwayCellMap *m, int i, int j)
{
    return m->cells[(m->nx - 1) * j + i];
}

// FUNC 0x002274A0 runway_map_find
//   The cell under (x, z); NULL outside the map.
u16 *runway_map_find(const RunwayCellMap *m, f32 x, f32 z)
{
    int i = runway_find_interval(m->bounds, m->nx, x);
    int j = runway_find_interval(&m->bounds[m->nx], m->nz, z);

    if (i == RUNWAY_BELOW || i == RUNWAY_ABOVE || j == RUNWAY_BELOW || j == RUNWAY_ABOVE)
        return NULL;
    return m->cells[(m->nx - 1) * j + i];
}

// FUNC 0x00227558 runway_map_clamp_x
//   i clamped to the cells 0..nx-2.
int runway_map_clamp_x(const RunwayCellMap *m, int i)
{
    int max = m->nx - 2;

    if (i < 0)
        return 0;
    return (max < i) ? max : i;
}

// FUNC 0x00227578 runway_map_clamp_z
int runway_map_clamp_z(const RunwayCellMap *m, int j)
{
    int max = m->nz - 2;

    if (j < 0)
        return 0;
    return (max < j) ? max : j;
}

// FUNC 0x00227598 runway_tri_contains
//   1 if (x, z) is inside the triangle seen from above (either winding,
//   the edges included).
int runway_tri_contains(const RunwayTri *t, const RunwayVertex *verts, f32 x, f32 z)
{
    const RunwayVertex *a = &verts[t->v[0]];
    const RunwayVertex *b = &verts[t->v[1]];
    const RunwayVertex *c = &verts[t->v[2]];
    f32 ax = a->x - x;
    f32 az = a->z - z;
    f32 bx = b->x - x;
    f32 bz = b->z - z;
    f32 cx = c->x - x;
    f32 cz = c->z - z;
    f32 eab = ax * bz - bx * az;
    f32 ebc = bx * cz - cx * bz;
    f32 eca = cx * az - ax * cz;

    return (eab <= 0.0f && ebc <= 0.0f && eca <= 0.0f) ||
           (0.0f <= eab && 0.0f <= ebc && 0.0f <= eca);
}

// FUNC 0x00227670 runway_tri_height
//   The height of the triangle's plane at (x, z), clamped to the vertex
//   heights (the lowest vertex for a degenerate triangle).
//   Matching (45.3%): retail loads all nine coordinates (z, x, then y) before
//   the four min.s / max.s (volatile asm, as in runway_tri_values) and
//   computes everything else after them, so the original probably read the
//   coordinates into locals first; with locals the order of the loads and
//   the register numbers still differ (62% at best).  Tried: plain, volatile
//   and "0"-tied asm helpers, >? / <?, 216 declaration orders of the
//   coordinate locals, min/max as nested calls or four statements.
f32 runway_tri_height(const RunwayTri *t, const RunwayVertex *verts, f32 x, f32 z)
{
    const RunwayVertex *c = &verts[t->v[2]];
    const RunwayVertex *a = &verts[t->v[0]];
    const RunwayVertex *b = &verts[t->v[1]];
    f32 lo = u103_min(c->y, u103_min(a->y, b->y));
    f32 hi = u103_max(c->y, u103_max(a->y, b->y));
    f32 bx = b->x - a->x;
    f32 bz = b->z - a->z;
    f32 cx = c->x - a->x;
    f32 cz = c->z - a->z;
    f32 px = a->x - x;
    f32 pz = a->z - z;
    f32 det = bx * cz - cx * bz;
    f32 dyb = b->y - a->y;
    f32 h;

    if (det == 0.0f) {
        h = lo;
    } else {
        f32 dyc = c->y - a->y;
        h = ((a->y * det + dyb * (cx * pz - px * cz)) + dyc * (px * bz - bx * pz)) / det;
    }
    h = u103_max(h, lo);
    return u103_min(h, hi);
}

// FUNC 0x00227758 runway_list_contains
//   list = {n, item[n]}.
int runway_list_contains(const u16 *list, int item)
{
    int n = list[0];
    const u16 *p = list + 1;

    while (n > 0) {
        if (*p++ == item)
            return 1;
        n--;
    }
    return 0;
}

// FUNC 0x00227790 runway_tri_values
//   The vertex values u, v at the query point (barycentric, clamped to the
//   vertex values) into q->u / q->v, and vertex 0's raw values.  lapLength
//   is passed (in $f12) but not used.
//   Matching (78.6%): the schedule is retail's (statement order found by a
//   720-order search; the u / v loads stay after the q->u store through
//   RunwayVertex's unions), the register numbers are not: a permutation of
//   the FPU temporaries and of $a0 / $a2.  Separate val / lo / hi variables
//   per channel give retail's ties in the v half but move the first half
//   (70.6%).
void runway_tri_values(const RunwaySector *s, RunwayQuery *q, const RunwayTri *t, f32 lapLength)
{
    const RunwayVertex *verts = s->verts;
    const RunwayVertex *a = &verts[t->v[0]];
    const RunwayVertex *b = &verts[t->v[1]];
    const RunwayVertex *c = &verts[t->v[2]];
    f32 px = q->x;
    f32 pz = q->z;
    f32 ab = a->x * b->z - b->x * a->z;
    f32 det = (((ab + b->x * c->z) - c->x * b->z) + c->x * a->z) - a->x * c->z;
    f32 wa = ((b->x * c->z - c->x * b->z) + px * (b->z - c->z)) + pz * (c->x - b->x);
    f32 wb = ((c->x * a->z - a->x * c->z) + px * (c->z - a->z)) + pz * (a->x - c->x);
    f32 wc = (ab + px * (a->z - b->z)) + pz * (b->x - a->x);
    f32 au, bu, cu, av, bv, cv, val, lo, hi;

    wa = wa / det;
    wb = wb / det;
    au = (f32)a->u / 65535.0f;
    bu = (f32)b->u / 65535.0f;
    cu = (f32)c->u / 65535.0f;
    wc = wc / det;
    val = (wa * au + wb * bu) + wc * cu;
    lo = u103_min(u103_min(au, bu), cu);
    hi = u103_max(u103_max(au, bu), cu);
    q->u = u103_min(u103_max(val, lo), hi);

    bv = (f32)b->v / 65535.0f;
    av = (f32)a->v / 65535.0f;
    cv = (f32)c->v / 65535.0f;
    val = (wa * av + wb * bv) + wc * cv;
    lo = u103_min(u103_min(av, bv), cv);
    hi = u103_max(u103_max(av, bv), cv);
    q->u0 = a->u;
    q->v0 = a->v;
    q->v = u103_min(u103_max(val, lo), hi);
}

// FUNC 0x00227980 runway_relocate
//   Relocates a loaded file to its address (self is set first, nothing
//   else when it is already there).  Called by course_data_relocate (T179).
void runway_relocate(Runway *r)
{
    int delta = (u8 *)r - (u8 *)r->self;
    u32 i;

    r->self = r;
    if (delta == 0)
        return;
    u103_reloc(r->markers, delta);
    u103_reloc(r->grid, delta);
    u103_reloc(r->sectors, delta);
    u103_reloc(r->walls, delta);
    u103_reloc(r->lines, delta);
    u103_reloc(r->triMap, delta);
    u103_reloc(r->pointMap, delta);
    if (r->objectCount != 0)
        u103_reloc(r->objects, delta);
    if (r->lightCount != 0)
        u103_reloc(r->lights, delta);
    runway_map_relocate(r->triMap, delta);
    runway_map_relocate(r->pointMap, delta);
    /* written out on r->sectors[i] (retail: i * 0x40 + sectors per pass,
     * no pointer; the inline helper's reference would be strength-reduced) */
    for (i = 0; i < r->sectorCount; i++) {
        r->sectors[i].verts = (RunwayVertex *)((u8 *)r->sectors[i].verts + delta);
        r->sectors[i].tris = (RunwayTri *)((u8 *)r->sectors[i].tris + delta);
        r->sectors[i].lines = (u16 *)((u8 *)r->sectors[i].lines + delta);
    }
}

// FUNC 0x00227AA0 runway_lap_distance
//   The distance along the lap at (q->x, q->z) into q->lapDist (0 when not
//   found).  Searches the hint sector q->sector, then its neighbours; all
//   sectors when the hint is not a sector (unsigned compare: -1 too).
//   q->sector is not changed.  1: found.
int runway_lap_distance(Runway *r, RunwayQuery *q)
{
    u32 sector = (u32)q->sector;
    int i;

    q->lapDist = 0.0f;
    if (sector >= r->sectorCount) {
        for (i = 0; i < (int)r->sectorCount; i++) {
            if (runway_sector_lap_distance(r, q, i) != 0)
                return 1;
        }
        return 0;
    }
    if (runway_sector_lap_distance(r, q, sector) != 0)
        return 1;
    for (i = 0; i < r->sectors[sector].neighbourCount; i++) {
        if (runway_sector_lap_distance(r, q, r->sectors[sector].neighbours[i]) != 0)
            return 1;
    }
    return 0;
}

// FUNC 0x00227BA8 runway_sector_lap_distance
//   For each lap line l of the sector: the point is past l (a*x + b*z + c
//   <= 0) and not past the next line m (the first one after the last on a
//   looped course), and between the lines' two edge segments: the distance
//   is interpolated from l->dist to m->dist (+ the lap length when m is
//   behind), wrapped at the lap length.
int runway_sector_lap_distance(Runway *r, RunwayQuery *q, u32 sector)
{
    int i;

    /* r->sectors[sector], r->lines and q->x / q->z read in place: retail
     * keeps sector * 0x40 and r->sectors in reaching registers and reloads
     * lineCount every pass; loop.c hoists the rest. */
    for (i = 0; i < r->sectors[sector].lineCount; i++) {
        u32 lineIndex = r->sectors[sector].lines[i];
        RunwayLine *line = &r->lines[lineIndex];
        RunwayLine *next;
        f32 sideLine, sideNext, distLine, distNext, dist;
        int nextIndex;

        sideLine = (line->a * q->x + line->b * q->z) + line->c;
        if (0.0f < sideLine)
            continue;
        nextIndex = lineIndex + 1;
        if (nextIndex == r->lineCount) {
            if (r->looped == 0)
                continue;
            nextIndex = 0;
        }
        next = &r->lines[nextIndex];
        sideNext = (next->a * q->x + next->b * q->z) + next->c;
        if (sideNext < 0.0f)
            continue;
        /* between the segments joining the lines' first ends and their
         * second ends */
        if ((next->x0 - line->x0) * (q->z - line->z0) - (next->z0 - line->z0) * (q->x - line->x0) < 0.0f)
            continue;
        if (0.0f < (next->x1 - line->x1) * (q->z - line->z1) - (next->z1 - line->z1) * (q->x - line->x1))
            continue;
        distLine = line->dist;
        distNext = next->dist;
        if (distNext < distLine)
            distNext = distNext + r->lapLength;
        dist = distLine + ((distNext - distLine) * sideLine) / (sideLine - sideNext);
        q->lapDist = dist;
        if (r->lapLength <= dist)
            q->lapDist = dist - r->lapLength;
        return 1;
    }
    return 0;
}

// FUNC 0x00227D78 runway_query
//   The road under (q->x, q->z): of the triangles of the cell containing
//   the point (hint sector and neighbours only, when q->sector is a sector)
//   the highest.  Fills y (the height), sector, attr / attr2, the vertex
//   values (runway_tri_values), the triangle and the unit plane.  0: no
//   road there (q->sector is then -1, the rest unchanged).
//   Matching (97.6%): only the block before the runway_tri_values call
//   differs: retail sets up $a0 (&r->sectors[bestSector]) and $f12 before
//   the attr stores.  Tried: all orders of the four stores, the arguments in
//   locals (180 orders), q's attr fields as unions, other bestSector types.
int runway_query(Runway *r, RunwayQuery *q)
{
    f32 x = q->x;
    f32 z = q->z;
    u16 *cell = runway_map_find(r->triMap, x, z);
    const u16 *near;
    int hint;
    const RunwayTri *bestTri;
    const RunwayVertex *bestVerts;
    int bestSector;
    f32 best;
    u32 n;

    if (cell == NULL)
        return 0;
    hint = q->sector;
    q->sector = -1;
    near = NULL;
    if (hint >= 0)
        near = &r->sectors[hint].neighbourCount;
    bestTri = NULL;
    bestVerts = NULL;
    bestSector = -1;
    best = -3.4028235e38f;
    while ((n = cell[0]) != 0) {
        int sector = cell[1];
        const u16 *idx = cell + 2;

        if (sector == hint || near == NULL || runway_list_contains(near, sector) != 0) {
            RunwaySector *s = &r->sectors[sector];
            const RunwayTri *found = NULL;
            const RunwayVertex *foundVerts = NULL;
            f32 top = -3.4028235e38f;
            int left = cell[0] - 1;

            if (left != -1) {
                do {
                    const RunwayTri *t = &s->tris[*idx];
                    if (runway_tri_contains(t, s->verts, x, z) != 0) {
                        f32 h = runway_tri_height(t, s->verts, x, z);
                        if (top < h) {
                            top = h;
                            found = t;
                            foundVerts = s->verts;
                        }
                    }
                    left--;
                    idx++;
                } while (left != -1);
            }
            if (found != NULL && !(top < best)) {
                best = top;
                bestSector = sector;
                bestTri = found;
                bestVerts = foundVerts;
            }
        }
        cell += cell[0] + 2;
    }
    if (bestTri == NULL)
        return 0;

    q->y = best;
    q->sector = bestSector;
    q->attr = bestTri->attr;
    q->attr2 = bestTri->attr2;
    runway_tri_values(&r->sectors[bestSector], q, bestTri, r->lapLength);
    {
        const RunwayVertex *c = &bestVerts[bestTri->v[2]];
        const RunwayVertex *a = &bestVerts[bestTri->v[0]];
        const RunwayVertex *b = &bestVerts[bestTri->v[1]];
        f32 ax = a->x, ay = a->y, az = a->z;
        f32 bx = b->x - ax, by = b->y - ay, bz = b->z - az;
        f32 cx = c->x - ax, cy = c->y - ay, cz = c->z - az;
        f32 nx, ny, nz, inv;

        q->tri = bestTri;
        q->verts = bestVerts;
        nx = by * cz - bz * cy;
        ny = bz * cx - bx * cz;
        nz = bx * cy - by * cx;
        inv = u103_rsqrt(1.0f, (nx * nx + ny * ny) + nz * nz);
        nx = nx * inv;
        ny = ny * inv;
        nz = nz * inv;
        q->plane[0] = nx;
        q->plane[1] = ny;
        q->plane[2] = nz;
        q->plane[3] = -((nx * ax + ny * ay) + nz * az);
    }
    return 1;
}

// FUNC 0x002280C8 runway_query_contacts
//   The road under four points at once (the wheels; c[0].sector is the
//   hint).  Points in the same cell are tested together.  A point keeps the
//   highest triangle strictly containing it: height, sector, the vertex
//   values, the plane y = gx*x + gz*z + c, vertex 0's values, the low bytes
//   of attr / attr2.  Then for all four: y = height, offRoad = (sector < 0),
//   the plane normalised to a unit normal (y up) and d.  (A point off the
//   road keeps the plane of the previous call, normalised again.)
void runway_query_contacts(Runway *r, RunwayContact *c)
{
    u16 *cells[4];                              /* sp+0x00 */
    RunwayContact *group[4];                    /* sp+0x10 the contacts in the cell */
    const u16 *near = NULL;
    int hint = c[0].sector;
    int i, k;                                   /* k: a contact, then the triangles left */

    if (hint >= 0)
        near = &r->sectors[hint].neighbourCount;
    for (k = 0; k < 4; k++) {
        c[k].height = -3.4028235e38f;
        c[k].sector = -1;
        cells[k] = runway_map_find(r->triMap, c[k].x, c[k].z);
    }

    for (i = 0; i < 4; i++) {
        u16 *cell = cells[i];
        u32 n;
        int groupCount;

        if (cell == NULL)
            continue;
        groupCount = 0;
        for (k = 0; k < 4; k++) {
            if (cells[k] == cell) {
                group[groupCount] = &c[k];
                groupCount++;
                cells[k] = NULL;
            }
        }
        while ((n = cell[0]) != 0) {
            int sector = cell[1];

            if (sector == hint || near == NULL || runway_list_contains(near, sector) != 0) {
                RunwaySector *s = &r->sectors[sector];
                const u16 *idx = cell + 2;

                k = cell[0];
                while (k-- != 0) {
                    const RunwayTri *t = &s->tris[*idx];
                    const RunwayVertex *verts = s->verts;
                    const RunwayVertex *va = &verts[t->v[0]];
                    const RunwayVertex *vb = &verts[t->v[1]];
                    const RunwayVertex *vc = &verts[t->v[2]];
                    f32 ax = va->x;
                    f32 az = va->z;
                    f32 bx = vb->x - ax;
                    f32 bz = vb->z - az;
                    f32 cx = vc->x - ax;
                    f32 cz = vc->z - az;
                    f32 inv = 1.0f / (bx * cz - cx * bz);
                    int g;

                    for (g = 0; g < groupCount; g++) {
                        RunwayContact *q = group[g];
                        f32 px = q->x - ax;
                        f32 pz = q->z - az;
                        f32 wb = (px * cz - cx * pz) * inv;
                        f32 wc = (bx * pz - px * bz) * inv;
                        f32 h, ay, dyb, dyc, gx, gz, d;

                        if (!(0.0f < wb))
                            continue;
                        if (!(0.0f < wc))
                            continue;
                        if (!(wb + wc < 1.0f))
                            continue;
                        h = ((vb->y - va->y) * wb + (vc->y - va->y) * wc) + va->y;
                        if (!(q->height < h))
                            continue;
                        q->height = h;
                        q->sector = sector;
                        {
                            f32 a0 = (f32)va->u / 65535.0f;
                            f32 b0 = (f32)vb->u / 65535.0f;
                            f32 c0 = (f32)vc->u / 65535.0f;
                            q->u = ((b0 - a0) * wb + (c0 - a0) * wc) + a0;
                        }
                        {
                            f32 a0 = (f32)va->v / 65535.0f;
                            f32 b0 = (f32)vb->v / 65535.0f;
                            f32 c0 = (f32)vc->v / 65535.0f;
                            q->v = ((b0 - a0) * wb + (c0 - a0) * wc) + a0;
                        }
                        ay = va->y;
                        dyb = vb->y - ay;
                        dyc = vc->y - ay;
                        gx = (cz * dyb - bz * dyc) * inv;
                        gz = (bx * dyc - cx * dyb) * inv;
                        d = -((gx * va->x + gz * va->z) - ay);
                        q->normal[2] = gz;
                        q->normal[0] = gx;
                        q->d = d;
                        q->u0 = va->u;
                        q->v0 = va->v;
                        q->attr = (u8)t->attr;
                        q->attr2 = (u8)t->attr2;
                    }
                    idx++;
                }
            }
            cell += cell[0] + 2;
        }
    }

    for (k = 0; k < 4; k++) {
        f32 gx = c[k].normal[0];
        f32 gz = c[k].normal[2];
        f32 scale;

        c[k].unk_32 = 0;
        c[k].y = c[k].height;
        c[k].offRoad = (u32)c[k].sector >> 31;
        scale = u103_rsqrt(-1.0f, (gx * gx + 1.0f) + gz * gz);
        c[k].normal[0] = gx * scale;
        c[k].normal[2] = gz * scale;
        c[k].d = c[k].d * scale;
        c[k].normal[1] = -scale;
    }
}

// FUNC 0x002285B8 runway_for_each_tri
//   cb(triangle, its sector's vertices, ctx) for the triangles listed in the
//   triMap cells over the rectangle (x0, z0)..(x1, z1) widened by 8 (hint
//   sector and neighbours only when sector >= 0).  A triangle listed in
//   several cells is passed several times.
//   Matching (98.5%, as runway_for_each_point): two scheduling ties left,
//   the jal delay slot of the third clamp (retail: the copy of j) and the
//   order of the cb arguments' loads (retail: ctx before s->verts).
void runway_for_each_tri(Runway *r, RunwayTriFunc cb, void *ctx, int sector,
                         f32 x0, f32 z0, f32 x1, f32 z1)
{
    const u16 *near = NULL;
    int i0, j0, i1, j1, i, j;
    f32 xa, za, xb, zb;

    if (sector >= 0)
        near = &r->sectors[sector].neighbourCount;
    xa = x0 - 8.0f;
    za = z0 - 8.0f;
    xb = x1 + 8.0f;
    zb = z1 + 8.0f;
    i0 = runway_find_interval(r->triMap->bounds, r->triMap->nx, xa);
    j0 = runway_find_interval(&r->triMap->bounds[r->triMap->nx], r->triMap->nz, za);
    i1 = runway_find_interval(r->triMap->bounds, r->triMap->nx, xb);
    j1 = runway_find_interval(&r->triMap->bounds[r->triMap->nx], r->triMap->nz, zb);
    i0 = runway_map_clamp_x(r->triMap, i0);
    j = runway_map_clamp_z(r->triMap, j0);
    i1 = runway_map_clamp_x(r->triMap, i1);
    j1 = runway_map_clamp_z(r->triMap, j1);
    for (; j <= j1; j++) {
        for (i = i0; i <= i1; i++) {
            u16 *cell = runway_map_cell(r->triMap, i, j);
            u32 n;

            if (cell == NULL)
                continue;
            while ((n = cell[0]) != 0) {
                int sec = cell[1];

                if (sec == sector || near == NULL || runway_list_contains(near, sec) != 0) {
                    RunwaySector *s = &r->sectors[sec];
                    const u16 *idx = cell + 2;
                    int left = cell[0] - 1;

                    if (cell[0] != 0) {
                        do {
                            u32 ti = *idx;
                            left--;
                            idx++;
                            cb(&s->tris[ti], s->verts, ctx);
                        } while (left != -1);
                    }
                }
                cell += cell[0] + 2;
            }
        }
    }
}

// FUNC 0x00228848 runway_for_each_point
//   The same over the pointMap: cb(&r->walls[item], ctx).  The cells are
//   not checked for NULL here.
//   Matching (99.0%): only the jal delay slot of the third clamp differs
//   (retail: daddu $s5, $v0, the copy of j; ours: the i1 argument).  The
//   reference compiler 991111-01 gives the same.
void runway_for_each_point(Runway *r, RunwayPointFunc cb, void *ctx, int sector,
                           f32 x0, f32 z0, f32 x1, f32 z1)
{
    const u16 *near = NULL;
    int i0, j0, i1, j1, i, j;
    f32 xa, za, xb, zb;

    if (sector >= 0)
        near = &r->sectors[sector].neighbourCount;
    xa = x0 - 8.0f;
    za = z0 - 8.0f;
    xb = x1 + 8.0f;
    zb = z1 + 8.0f;
    i0 = runway_find_interval(r->pointMap->bounds, r->pointMap->nx, xa);
    j0 = runway_find_interval(&r->pointMap->bounds[r->pointMap->nx], r->pointMap->nz, za);
    i1 = runway_find_interval(r->pointMap->bounds, r->pointMap->nx, xb);
    j1 = runway_find_interval(&r->pointMap->bounds[r->pointMap->nx], r->pointMap->nz, zb);
    i0 = runway_map_clamp_x(r->pointMap, i0);
    j = runway_map_clamp_z(r->pointMap, j0);
    i1 = runway_map_clamp_x(r->pointMap, i1);
    j1 = runway_map_clamp_z(r->pointMap, j1);
    for (; j <= j1; j++) {
        for (i = i0; i <= i1; i++) {
            u16 *cell = runway_map_cell(r->pointMap, i, j);
            u32 n;


            while ((n = cell[0]) != 0) {
                int sec = cell[1];

                if (sec == sector || near == NULL || runway_list_contains(near, sec) != 0) {
                    const u16 *idx = cell + 2;
                    int left = cell[0] - 1;

                    if (left != -1) {
                        do {
                            u32 pi = *idx;
                            left--;
                            idx++;
                            cb(&r->walls[pi], ctx);
                        } while (left != -1);
                    }
                }
                cell += cell[0] + 2;
            }
        }
    }
}

// FUNC 0x00228AB0 runway_marker_count
//   The number of markers of type 2 with this key.
int runway_marker_count(Runway *r, int key)
{
    int n = r->markerCount;
    int count = 0;
    int i;

    for (i = 0; i < n; i++) {
        RunwayMarker *mk = &r->markers[i];
        if (mk->key == key) {
            if (mk->type == 2)
                count++;
        }
    }
    return count;
}

// FUNC 0x00228B00 runway_marker_value
//   The value of the n-th (from 0) marker of type 2 with this key; 0.0 if
//   there are not that many.
f32 runway_marker_value(Runway *r, int n, int key)
{
    int count = r->markerCount;
    int seen = 0;
    int i;

    for (i = 0; i < count; i++) {
        RunwayMarker *mk = &r->markers[i];
        if (mk->key == key && mk->type == 2) {
            if (seen == n)
                return mk->value;
            seen++;
        }
    }
    return 0.0f;
}

// FUNC 0x00228B68 runway_is_neighbour
//   1 if other is in the neighbour list of sector.
int runway_is_neighbour(Runway *r, int sector, int other)
{
    int n = r->sectors[sector].neighbourCount;
    const u16 *p = r->sectors[sector].neighbours;
    int i;

    for (i = 0; i < n; i++) {
        if (*p++ == other)
            return 1;
    }
    return 0;
}
