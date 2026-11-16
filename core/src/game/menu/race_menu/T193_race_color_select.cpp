/* T193: the random colours of the cars of a race.
 *
 * race_pick_random_colors() (RaceInformation::assignCars, T124, with the
 * generator g_00351DE0) gives every entry whose colour code is -1 one of its
 * car's colours.
 * color_table_build() lists the candidate colours of every entry (a fixed
 * colour is one candidate of weight 1; otherwise every colour of the car,
 * weights 1, 63/64, (63/64)^2 .. normalised), then divides each candidate's
 * weight by 1 + its similarity to every other candidate of the race (the
 * similarity to a fixed colour counts as s^64 * 10000), sorts the
 * candidates by weight (ascending) and normalises.  The pick walks the
 * list: candidate j is taken when a fresh random number 0..1 is below its
 * weight (the last one otherwise).
 */
/* Retail's .rodata (0x00327328..0x00327348): "bad_alloc" (<new>'s inline
 * what(), written when the header is read) and UnitArena<64>'s type name,
 * nothing else; no code references it: the anchor. */
// DATA_START 0x00327328
#include <new>
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/menu/race_menu/T193_race_color_select.h"
#include "sdk/libcdvd/G004_libcdvd.h"              /* sqrt (newlib libm) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (info) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries) */
#include "game/data/T482_car_equipments.h"       /* CarEquipments (colorCode, getModelCode, setColorCode) */
#include "game/data/T485_car_color.h"            /* CarColorDataBase g_00352D98, CarColorRecord */
#include "engine/pdistd/T531_random.h"               /* Random::next01 */
#include "game/menu/records/results_internal.h"

static const f32 SQRT_3 = 1.7320508f;       /* the longest RGB distance (gp-0x7D28) */

// FUNC 0x001767E0 color_similarity
//   1 for identical colours; else the dot product of the RGB directions
//   scaled by (sqrt 3 - the distance of the RGB vectors) / sqrt 3, clamped
//   to 0..1.  The distance is a double sqrt (the casts are the soft-float
//   calls fptodp / dptofp).
f32 color_similarity(const ColorCandidate *a, const ColorCandidate *b)
{
    /* every field is loaded before the first compare */
    f32 ar = a->r, ag = a->g, ab = a->b, al = a->len;
    f32 br = b->r, bg = b->g, bb = b->b, bl = b->len;
    if (ar == br && ag == bg && ab == bb && al == bl)
        return 1.0f;

    f32 dot = ar * br + ag * bg + ab * bb;
    /* the RGB vectors (direction * length) */
    f32 x1 = ar * al, y1 = ag * al, z1 = ab * al;
    f32 x2 = br * bl, y2 = bg * bl, z2 = bb * bl;
    f32 dr = x1 - x2;
    f32 dg = y1 - y2;
    f32 db = z1 - z2;
    f32 dist = (f32)sqrt((double)(dr * dr + dg * dg + db * db));
    /* implicit: fptodp, dptofp (the double casts, soft-float) */
    f32 similarity = ((SQRT_3 - dist) / SQRT_3) * dot;
    if (similarity < 0.0f)
        return 0.0f;
    if (1.0f < similarity)
        return 1.0f;
    return similarity;
}

// FUNC 0x00176900 color_candidate_set
//   Looks the colour up in the colour database (no NULL check) and stores
//   its RGB (0x00BBGGRR, 0..1 a channel) as a unit direction and a length
//   (a black colour gets length 1; the length is a double sqrt, through
//   fptodp / dptofp).  The record's rgb is read twice: once for red, once
//   for green and blue.
/* Matching (u041): 94.8%, open.  Retail reads rgb again after red's
 * conversion (`lw` at the join and in the `b` delay slot); here GCSE's PRE
 * replaces that second load with the first one's value.  Exact only when
 * the first read is not a GCSE expression (a volatile read of it, not kept:
 * no original would have written that).  Tried: inline accessors / channel
 * helpers (value, pointer, reference arguments), a second pointer or a
 * field pointer for either read, the assignment inside the expression, a
 * local database pointer, a bitfield view; the reference compiler
 * 991111-01 gives the same. */
void color_candidate_set(ColorCandidate *c, int color)
{
    const CarColorRecord *rec = g_00352D98.findColor(color);
    f32 r = (f32)(rec->rgb & 0xFF) / 255.0f;
    u32 rgb = rec->rgb;
    f32 g = (f32)((rgb >> 8) & 0xFF) / 255.0f;
    f32 b = (f32)((rgb >> 16) & 0xFF) / 255.0f;
    f32 len = (f32)sqrt((double)(r * r + g * g + b * b));
    /* implicit: fptodp, dptofp (the double casts, soft-float) */
    if (len == 0.0f)
        len = 1.0f;
    r /= len;
    g /= len;
    b /= len;
    c->color = color;
    c->len = len;
    c->r = r;
    c->g = g;
    c->b = b;
}

// FUNC 0x00176AA8 color_candidates_sort
//   Insertion sort by weight, ascending (whole 0x18-byte records are
//   swapped; the count is read again every pass).
void color_candidates_sort(ColorCandidates *list)
{
    for (int i = 0; i < list->count; i++) {
        ColorCandidate held = list->c[i];
        for (int j = 0; j < i; j++) {
            ColorCandidate *p = &list->c[j];
            if (held.weight < p->weight) {
                ColorCandidate swap = *p;
                *p = held;
                held = swap;
            }
        }
        list->c[i] = held;
    }
}

// FUNC 0x00176C28 color_table_build
//   See the top of the file.  A car's colours come from the colour database
//   by its model code (CarEquipments::getModelCode).
void color_table_build(RaceOrganization &org, ColorTable *tab)
{
    /* one set of indices for the whole function (retail reuses the
     * registers: j of the first part is j of the second) */
    int i, j, k, m;
    CarColorDataBase *db = &g_00352D98;     /* kept in $fp */
    int entries = org.entries->count;
    tab->count = entries;

    for (i = 0; i < entries; i++) {
        CarEquipments *eq = &org.entries->cars[i]->info;
        ColorCandidates *list = &tab->car[i];
        u64 model = eq->getModelCode();
        u32 color = eq->colorCode;          /* read once: before the list->count store */
        if (color == (u32)-1) {
            int count = db->getNumColors(model);
            f32 decay = 0.984375f;          /* 63/64 */
            f32 weight = 1.0f;
            list->count = count;
            f32 sum = 0.0f;
            for (j = 0; j < count; j++) {
                sum += weight;
                ColorCandidate *c = &list->c[j];
                color_candidate_set(c, db->getColor(model, j));
                c->weight = weight;
                weight *= decay;
            }
            for (j = 0; j < count; j++)
                list->c[j].weight /= sum;
        } else {
            list->count = 1;
            color_candidate_set(&list->c[0], color);
            list->c[0].weight = 1.0f;
        }
    }

    for (i = 0; i < entries; i++) {
        ColorCandidates *list = &tab->car[i];
        f32 total = 0.0f;
        for (j = 0; j < list->count; j++) {
            ColorCandidate *cand = &list->c[j];
            f32 crowd = 1.0f;
            for (k = 0; k < entries; k++) {
                ColorCandidates *other = &tab->car[k];
                for (m = 0; m < other->count; m++) {
                    if (i == k && j == m)
                        continue;
                    f32 s = color_similarity(cand, &other->c[m]);
                    if (other->count == 1) {
                        /* a fixed colour: s^64 * 10000 */
                        for (int p = 5; p >= 0; p--)
                            s *= s;
                        s *= 10000.0f;
                    }
                    crowd += s;
                }
            }
            cand->weight = cand->weight * (1.0f / crowd);
            total += cand->weight;
        }
        color_candidates_sort(list);
        int count = list->count;
        for (j = 0; j < count; j++)
            list->c[j].weight /= total;
    }
}

// FUNC 0x00176FD0 race_pick_random_colors
//   The whole candidate table is rebuilt for every entry that gets a colour:
//   CarEquipments::setColorCode stores the colour, so the colours picked so
//   far count as fixed colours (the strong s^64 * 10000 penalty) for the
//   later entries.  The entry count is read once.
void race_pick_random_colors(RaceOrganization &org, Random *rng)
{
    ColorTable tab;

    int entries = org.entries->count;
    for (int i = 0; i < entries; i++) {
        CarEquipments *eq = &org.entries->cars[i]->info;
        if (eq->colorCode != (u32)-1)
            continue;
        color_table_build(org, &tab);
        ColorCandidates *list = &tab.car[i];
        int last = list->count - 1;
        int j = 0;
        while (j < last && !(rng->next01() < list->c[j].weight))
            j++;
        eq->setColorCode(list->c[j].color);
    }
}
