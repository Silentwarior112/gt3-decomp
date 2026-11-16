/* T172_course_camera.cpp - CourseCamera, the camera file of a course
 * (0x001A3EA8..0x001A45A8): a piecewise linear table lookup, the relocation
 * of a loaded file and the lookups of its camera records (sorted by their
 * start on the course; the course position wraps by whole laps) and zones.
 * Callers: the replay cameras (DesignerCamera, PasslineCamera,
 * AutomountCamera) and T153's wrappers (RaceCourse::loadCameras /
 * collectReplayCameras / getReplayLoopCount). */
#include "game/race/pause/replay_pause_internal.h"
#include "game/camera/T172_course_camera.h"

// FUNC 0x001A3EA8 interpolate_table
//   The y of x on the table: a binary search for the first point with
//   x < pts[i].x, then the segment (i - 1, i), clamped to the last segment
//   at the top.  Not clamped at the bottom: x < pts[0].x uses the segment
//   (-1, 0), i.e. the word before the table and the count as a point (as
//   the original; the callers keep x in range).
f32 interpolate_table(const InterpTable *t, f32 x)
{
    int lo, hi;
    int seg;
    const InterpPoint *a, *b;

    lo = 0;
    hi = t->n;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        if (x < t->pts[mid].x)
            hi = mid;
        else
            lo = mid + 1;
    }
    /* (retail keeps the count GCSE copied from t->n in $t0 and hi in the
     * loaded register; here CSE2 makes the copy the loaded one, so the
     * argument registers are permuted: 86%) */
    seg = lo - 1;
    if (!(seg < t->n - 1))
        seg = t->n - 2;
    a = &t->pts[seg];
    b = &t->pts[seg + 1];
    return (b->y - a->y) * (x - a->x) / (b->x - a->x) + a->y;
}

/* t moved by delta bytes. */
static inline InterpTable *relocated(InterpTable *t, int delta)
{
    return (InterpTable *)((u8 *)t + delta);
}

// FUNC 0x001A3F58 CourseCameraRecord::relocate
//   Adds delta to the 13 table pointers +0x30..+0x60 (the original loads
//   and stores them all, in an unrolled order).
void CourseCameraRecord::relocate(int delta)
{
    table30 = relocated(table30, delta);
    zoom = relocated(zoom, delta);
    table38[0] = relocated(table38[0], delta);
    table38[1] = relocated(table38[1], delta);
    pos[0] = relocated(pos[0], delta);
    pos[1] = relocated(pos[1], delta);
    pos[2] = relocated(pos[2], delta);
    table4C[0] = relocated(table4C[0], delta);
    table4C[1] = relocated(table4C[1], delta);
    table4C[2] = relocated(table4C[2], delta);
    target[0] = relocated(target[0], delta);
    target[1] = relocated(target[1], delta);
    target[2] = relocated(target[2], delta);
}

// FUNC 0x001A4008 CourseCamera::relocate
//   delta = this - base: the three tables and the pointers of every record
//   of both record tables are moved by it; base = this.  Callers:
//   RaceCourse::loadCameras (F_0018F740, after loading the file), the
//   course data's relocation course_data_relocate.
void CourseCamera::relocate()
{
    int delta = (int)((u8 *)this - (u8 *)base);
    u32 i;

    records = (CourseCameraRecord *)((u8 *)records + delta);
    records2 = (CourseCameraRecord *)((u8 *)records2 + delta);
    zones = (CourseCameraZone *)((u8 *)zones + delta);
    base = this;
    for (i = 0; i < (u32)count; i++)
        records[i].relocate(delta);
    for (i = 0; i < (u32)count2; i++)
        records2[i].relocate(delta);
}

// FUNC 0x001A40D8 CourseCamera::loopLength
//   The length of the whole laps the records cover:
//   length * ((int)(end / length) - 1 - (int)(records[0].start / length)).
f32 CourseCamera::loopLength()
{
    f32 len = length;
    int first = (int)(records[0].start / len);
    int last = (int)(end / len) - 1;

    return len * (f32)(last - first);
}

// FUNC 0x001A4128 CourseCamera::loopCount
//   The same number of laps, at least 1.  Caller: RaceCourse::
//   getReplayLoopCount (F_0018FD40).
int CourseCamera::loopCount()
{
    f32 len = length;
    int first = (int)(records[0].start / len);
    int last = (int)(end / len) - 1;
    int laps = last - first;

    return (laps > 0) ? laps : 1;
}

// FUNC 0x001A4178 CourseCamera::wrap
//   pos < 0 or pos < end: pos; else pos - L * (int)(L / pos), L =
//   loopLength().  (sic: L / pos, not pos / L: the division is
//   div.s f1, f0 (L), f20 (pos).)
f32 CourseCamera::wrap(f32 pos)
{
    f32 len;

    if (pos < 0.0f || pos < end)
        return pos;
    len = loopLength();
    return pos - len * (f32)(int)(len / pos);
}

// FUNC 0x001A41F0 CourseCamera::find
//   The last record whose start is <= pos (binary search), -1 if pos is
//   before the first.
int CourseCamera::find(f32 pos)
{
    int lo, hi;

    if (pos < records[0].start)
        return -1;
    lo = 0;
    hi = count;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        if (pos < records[mid].start)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo - 1;
}

// FUNC 0x001A4258 CourseCamera::collect
//   The records around the course position pos (wrapped): from the last
//   one starting at or before it, every record that has started (start <=
//   pos) and not ended (pos < end), going on past the last record into the
//   next lap (pos - loopLength(), while that still decreases) - at most max
//   of them into out.  Returns how many.  Caller: RaceCourse::
//   collectReplayCameras (F_0018FD20, the replay cameras).
int CourseCamera::collect(CourseCameraRecord **out, int max, f32 pos)
{
    f32 at = wrap(pos);
    f32 loop = loopLength();
    int index = find(at);
    int found = 0;

    /* count is hoisted out of the loop; records is reloaded after each
     * store into out (same pointer type) */
    if (index >= 0) {
        while (records[index].start <= at && found < max) {
            if (at < records[index].end)
                out[found++] = &records[index];
            index++;
            if (!(index < count)) {
                /* past the last record: on in the next lap */
                f32 prev = at;
                at -= loop;
                index -= count;
                if (!(at < prev))
                    break;
            }
        }
    }
    return found;
}

// FUNC 0x001A4378 CourseCamera::findZone
//   pos < 0: NULL.  pos is taken modulo the lap length, then the first zone
//   with a <= pos, pos <= b, a <= pos + length or pos + length <= b (as
//   the original: almost any zone) is returned; NULL if there is none.
//   (No callers.)
CourseCameraZone *CourseCamera::findZone(f32 pos)
{
    f32 len;
    f32 nextLap;
    CourseCameraZone *zone;
    u32 i;

    if (pos < 0.0f)
        return NULL;
    len = length;
    if (len < pos)
        pos -= (f32)(int)(pos / len) * len;
    nextLap = pos + len;
    zone = zones;
    for (i = 0; i < zoneCount; i++, zone++) {
        if (zone->a <= pos || pos <= zone->b || zone->a <= nextLap || nextLap <= zone->b)
            return zone;
    }
    return NULL;
}

// FUNC 0x001A4438 CourseCameraZone::overlaps
//   [a, b] (either order) meets [lo, hi] and its mask has a bit of m.
int CourseCameraZone::overlaps(f32 lo, f32 hi, u32 m)
{
    if (a < lo && b < lo)
        return 0;
    if (hi < a && hi < b)
        return 0;
    return (mask & m) != 0;
}

// FUNC 0x001A4490 CourseCamera::zoneInRange
//   1 if a zone with a bit of mask meets [lo, hi]; a range running past
//   the lap length is also tried as [0, hi - length].  lo < 0 or hi < lo:
//   0.  Caller: AutomountCamera::update (u090).
int CourseCamera::zoneInRange(f32 lo, f32 hi, u32 mask)
{
    CourseCameraZone *zone;
    u32 i;

    if (lo < 0.0f)
        return 0;
    if (hi < lo)
        return 0;
    zone = zones;
    for (i = 0; i < zoneCount; i++, zone++) {
        if (zone->overlaps(lo, hi, mask) != 0)
            return 1;
    }
    if (length < hi) {
        f32 rest = hi - length;

        zone = zones;
        for (i = 0; i < zoneCount; i++, zone++) {
            if (zone->overlaps(0.0f, rest, mask) != 0)
                return 1;
        }
    }
    return 0;
}
