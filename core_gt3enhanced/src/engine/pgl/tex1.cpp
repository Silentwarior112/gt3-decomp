/* tex1.cpp - the 'Tex1' texture set library of the graphics
 * library (G001), 0x002487B8..0x00249B28 (this unit's part).
 *
 * The same code as the bootstrap's src/tex1.cpp (0x0100B2E8..0x0100BE0F,
 * BOOT= functions, reused from there), with more of the library around it
 * in this binary: the texture override switches (gs_tex_override_enable /
 * _disable), the VifModel relocation, the CLUT / picture variation select,
 * the variation count, the upload as a DMA chain (tex1_upload_packet) and
 * the ring of gt2k_menu::CircleGauge.  Plain C with C linkage, compiled as
 * C++ like the bootstrap's file (EE_MAXF needs the `>?` operator of the
 * egcs C++ front end).  Tex1Image / GsTexRegs / VifModel: engine/pgl/G001_flare.h;
 * The CLUT animation records: pgl_effects_internal.h (Tex1Xfer: G001_flare.h).
 */
#include "engine/pgl/G001_flare.h"   /* this translation unit (G001) */
#include "engine/pgl/pgl_effects_internal.h"

/* EE COP1 abs.s / max.s (clut_anim_step) - see the bootstrap's
 * src/tex1.cpp: ee_absf is an asm helper under the matching compiler (the
 * retail forward loop keeps its entry jump, which egcs only does when the
 * exit test contains an asm), EE_MAXF the GNU maximum operator (max.s). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline f32 ee_absf(f32 a)
{
    f32 r;

    __asm__("abs.s %0, %1" : "=f"(r) : "f"(a));
    return r;
}
#else
static inline f32 ee_absf(f32 a) { return __builtin_fabsf(a); }
#endif
#ifdef BOOT_OLD_GCC
#define EE_MAXF(a, b) ((a) >? (b))
#else
#define EE_MAXF(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Pointer + byte delta (the relocation arithmetic).  RELOC is a do { }
 * while (0) statement macro and TEX1_RELOC its NULL-checked form (the
 * wrappers are visible in the retail scheduling, see the bootstrap). */
#define REBASE(p, delta)  ((__typeof__(p))((u8 *)(p) + (delta)))
#define RELOC(p, d)      do { (p) = REBASE(p, d); } while (0)
#define TEX1_RELOC(p, d) do { if ((p) != NULL) (p) = REBASE(p, d); } while (0)

/* GsTexRegs as bit fields (tex1_set_vram_base). */
typedef struct Tex1GsTex0 {
    u64 TBP0:14, TBW:6, PSM:6, TW:4, TH:4, TCC:1, TFX:2, CBP:14, CPSM:4, CSM:1, CSA:5, CLD:3;
} Tex1GsTex0;
typedef struct Tex1GsTex1 {
    u64 LCM:1, pad01:1, MXL:3, MMAG:1, MMIN:3, MTBA:1, pad10:9, L:2, pad21:11, K:12, pad44:20;
} Tex1GsTex1;
typedef struct Tex1GsMiptbp {
    u64 TBP1:14, TBW1:6, TBP2:14, TBW2:6, TBP3:14, TBW3:6, pad60:4;
} Tex1GsMiptbp;
typedef struct Tex1TexRegBits {
    Tex1GsTex0 tex0; Tex1GsTex1 tex1; Tex1GsMiptbp miptbp1; Tex1GsMiptbp miptbp2; u64 clamp;
} Tex1TexRegBits;

// FUNC 0x002487B8 tex1_set_vram_base
// (bootstrap 0x0100B2E8.)  Re-links the texture set for a new VRAM base:
// base = vramWordAddr >> 6 (64-word blocks), delta = base - img->vramBase;
// nothing to do (returns img) when delta == 0.  Otherwise stores the new
// base and, for every GsTexRegs, adds delta to TEX0.TBP0 and TEX0.CBP, and
// if TEX1.MXL > 0 to MIPTBP1.TBP1/2/3, plus MIPTBP2.TBP4/5/6 when MXL >= 4;
// each field wraps at 14 bits.  Then adds delta to the 16-bit DBP of every
// transfer record.  The tex pointer is not NULL-checked (only nTex); the
// xfer loop checks both pointer and count.
Tex1Image *tex1_set_vram_base(Tex1Image *img, u32 vramWordAddr)
{
    u32 base = vramWordAddr >> 6;
    s32 delta = (s32)(base - img->vramBase);
    int i;

    if (delta == 0)
        return img;

    img->vramBase = (u16)base;
    for (i = 0; i < img->nTex; i++) {
        int mxl;

        ((Tex1TexRegBits *)img->tex)[i].tex0.TBP0 = (int)((Tex1TexRegBits *)img->tex)[i].tex0.TBP0 + delta;
        ((Tex1TexRegBits *)img->tex)[i].tex0.CBP = (int)((Tex1TexRegBits *)img->tex)[i].tex0.CBP + delta;
        mxl = ((Tex1TexRegBits *)img->tex)[i].tex1.MXL;
        if (mxl > 0) {
            ((Tex1TexRegBits *)img->tex)[i].miptbp1.TBP1 = (int)((Tex1TexRegBits *)img->tex)[i].miptbp1.TBP1 + delta;
            ((Tex1TexRegBits *)img->tex)[i].miptbp1.TBP2 = (int)((Tex1TexRegBits *)img->tex)[i].miptbp1.TBP2 + delta;
            ((Tex1TexRegBits *)img->tex)[i].miptbp1.TBP3 = (int)((Tex1TexRegBits *)img->tex)[i].miptbp1.TBP3 + delta;
            if (mxl >= 4) {
                ((Tex1TexRegBits *)img->tex)[i].miptbp2.TBP1 = (int)((Tex1TexRegBits *)img->tex)[i].miptbp2.TBP1 + delta;
                ((Tex1TexRegBits *)img->tex)[i].miptbp2.TBP2 = (int)((Tex1TexRegBits *)img->tex)[i].miptbp2.TBP2 + delta;
                ((Tex1TexRegBits *)img->tex)[i].miptbp2.TBP3 = (int)((Tex1TexRegBits *)img->tex)[i].miptbp2.TBP3 + delta;
            }
        }
    }

    if (img->xfer != NULL) {
        Tex1Xfer *x = img->xfer;

        for (i = 0; i < img->nXfer; i++, x++)
            x->dbp = (u16)(x->dbp + delta);
    }
    return img;
}

// FUNC 0x00248A30 gs_tex_override_enable
/* GL-style enable of a switch of the library state's head (GS_OVR_*):
 * 0x1000 the override itself (its masks apply), 0x1001 the material scale,
 * 0x1100..0x1104 the PRIM bits IIP / TME / FGE / ABE / AA1 that prebuilt
 * models take from GsContext.primAttr, 0x1105..0x1109 the texture register
 * fields kept from the context (TEX0.TFX, TEX1.MMAG, TEX1.MMIN, CLAMP u,
 * CLAMP v).  Other values: nothing.  Caller: CarModel::draw (0x1001).
 * 93.8%: retail aligns the case labels to 8 bytes (alignment nops before
 * the ABE, MMAG and CLAMP_U cases and the clamp tails' join); egcs aligns
 * a label only after a loop note.  Each assignment in a do { } while (0)
 * aligns the first cases but not the 64-bit ones (95.6%), so the shape is
 * not that; left as plain statements. */
void gs_tex_override_enable(u32 what)
{
    GsLibState *st = &g_002F72C0;

    switch (what) {
    case GS_OVR_ENABLE:
        st->enable = 1;
        return;
    case GS_OVR_MAT_SCALE:
        st->matScaleOn = 1;
        return;
    case GS_OVR_PRIM_IIP:
        st->primMask = st->primMask | 0x08;
        return;
    case GS_OVR_PRIM_TME:
        st->primMask = st->primMask | 0x10;
        return;
    case GS_OVR_PRIM_FGE:
        st->primMask = st->primMask | 0x20;
        return;
    case GS_OVR_PRIM_ABE:
        st->primMask = st->primMask | 0x40;
        return;
    case GS_OVR_PRIM_AA1:
        st->primMask = st->primMask | 0x80;
        return;
    case GS_OVR_TEX0_TFX:
        st->tex0Mask = st->tex0Mask | 0x0000001800000000ull;   /* bits 35-36 (ori 0xC000; dsll 21) */
        return;
    case GS_OVR_TEX1_MMAG:
        st->tex1Mask = st->tex1Mask | 0x20;                     /* bit 5 */
        return;
    case GS_OVR_TEX1_MMIN:
        st->tex1Mask = st->tex1Mask | 0x1C0;                    /* bits 6-8 */
        return;
    case GS_OVR_CLAMP_U:
        st->clampMask = st->clampMask | 0x00FFFFF3;             /* WMS, MINU, MAXU */
        return;
    case GS_OVR_CLAMP_V:
        st->clampMask = st->clampMask | 0x00000FFFFF00000Cull;  /* WMT, MINV, MAXV */
        return;
    }
}

// FUNC 0x00248BB8 gs_tex_override_disable
/* The matching disable: clears what gs_tex_override_enable sets.  93.4%: the
 * same label alignment as gs_tex_override_enable. */
void gs_tex_override_disable(u32 what)
{
    GsLibState *st = &g_002F72C0;

    switch (what) {
    case GS_OVR_ENABLE:
        st->enable = 0;
        return;
    case GS_OVR_MAT_SCALE:
        st->matScaleOn = 0;
        return;
    case GS_OVR_PRIM_IIP:
        st->primMask = st->primMask & 0xFFF7;
        return;
    case GS_OVR_PRIM_TME:
        st->primMask = st->primMask & 0xFFEF;
        return;
    case GS_OVR_PRIM_FGE:
        st->primMask = st->primMask & 0xFFDF;
        return;
    case GS_OVR_PRIM_ABE:
        st->primMask = st->primMask & 0xFFBF;
        return;
    case GS_OVR_PRIM_AA1:
        st->primMask = st->primMask & 0xFF7F;
        return;
    case GS_OVR_TEX0_TFX:
        st->tex0Mask = st->tex0Mask & 0xFFFFFFE7FFFFFFFFull;
        return;
    case GS_OVR_TEX1_MMAG:
        st->tex1Mask = st->tex1Mask & (u64)(s64)-0x21;
        return;
    case GS_OVR_TEX1_MMIN:
        st->tex1Mask = st->tex1Mask & (u64)(s64)-0x1C1;
        return;
    case GS_OVR_CLAMP_U:
        st->clampMask = st->clampMask & 0xFFFFFFFFFF00000Cull;
        return;
    case GS_OVR_CLAMP_V:
        st->clampMask = st->clampMask & 0xFFFFF00000FFFFF3ull;
        return;
    }
}

// FUNC 0x00248D48 vif_model_relocate
/* Offsets -> pointers of a VifModel (race/flare.bin, model sets): delta =
 * m - m->self, self = m; ptr04 rebased when non-NULL, every part's data
 * pointer unconditionally.  Returns m. */
VifModel *vif_model_relocate(VifModel *m)
{
    VifModel *old = m->self;
    s32 delta;
    int n;
    int i;

    m->self = m;
    delta = (s32)((u8 *)m - (u8 *)old);
    TEX1_RELOC(m->ptr04, delta);
    n = m->nParts;
    for (i = 0; i < n; i++)
        RELOC(m->part[i].data, delta);
    return m;
}

// FUNC 0x00248DA8 tex1_relocate
// (bootstrap 0x0100B560.)  Converts the set's internal offsets to pointers
// for its current address: delta = img - img->self; returns at once when 0.
// Else self = img and ptr08 / tex / xfer / ptrTable / clutAnim are rebased
// when non-NULL; every transfer's data pointer is rebased unconditionally
// (count read once); ptrTable = {count, ptr[count]} gets its non-NULL
// entries rebased; the CLUT animation table goes through
// clut_anim_table_relocate.  Returns img.
Tex1Image *tex1_relocate(Tex1Image *img)
{
    s32 delta = (s32)((u8 *)img - (u8 *)img->self);

    if (delta == 0)
        return img;

    img->self = img;
    TEX1_RELOC(img->ptr08, delta);
    TEX1_RELOC(img->tex, delta);
    if (img->xfer != NULL) {
        int n;
        int i;

        RELOC(img->xfer, delta);
        n = img->nXfer;
        for (i = 0; i < n; i++)
            RELOC(img->xfer[i].data, delta);                /* no NULL check */
    }
    if (img->ptrTable != NULL) {
        u32 *p;
        int n;

        RELOC(img->ptrTable, delta);
        p = img->ptrTable;
        n = (int)*p++;
        while (n--) {                                       /* n words, as a count */
            if (*p != 0)
                *p += (u32)delta;
            p++;
        }
    }
    if (img->clutAnim != NULL) {
        RELOC(img->clutAnim, delta);
        clut_anim_table_relocate(img->clutAnim, delta);
    }
    return img;
}

/* TEX0 as GS bit fields (a local view of GsTexRegs.tex0, the layout of
 * Sony's sceGsTex0).  Retail writes CPSM, CBP and CSA as three separate
 * read-modify-writes and re-reads img->tex after each (a union access
 * aliases everything), and adds the CLUT base in 64 bits (the u64 field). */
union U113Tex0 {
    u64 raw;
    struct {
        u64 tbp0 : 14;
        u64 tbw : 6;
        u64 psm : 6;
        u64 tw : 4;
        u64 th : 4;
        u64 tcc : 1;
        u64 tfx : 2;
        u64 cbp : 14;
        u64 cpsm : 4;
        u64 csm : 1;
        u64 csa : 5;
        u64 cld : 3;
    } f;
};
#define U113_TEX0(p) ((union U113Tex0 *)(p))

// FUNC 0x00248EC8 tex1_select_variation
/* Applies variation `index` of the set (ptrTable = {count, rec[count]};
 * nothing for no table or an index outside 0..count-1): rec = {n (low 16
 * bits), entry[n]}, each entry (texture << 23 | CPSM << 19 | CBP offset << 5
 * | CSA) rewrites TEX0.CPSM, TEX0.CBP (= offset + vramBase, 14 bits) and
 * TEX0.CSA of that texture: a picture / colour variation through the CLUT. */
void tex1_select_variation(Tex1Image *img, int index)
{
    u32 *p = img->ptrTable;
    u64 vb;
    int n;

    if (p == NULL)
        return;
    if (index < 0)
        return;
    n = *p++;                               /* the table's count */
    if (!(index < n))
        return;
    vb = img->vramBase;
    p = (u32 *)p[index];
    n = *p++;                               /* the record's count (low 16 bits) */
    n &= 0xFFFF;
    while (n--) {
        u32 e = *p++;
        u32 t = e >> 23;

        U113_TEX0(&img->tex[t].tex0)->f.cpsm = e >> 19;
        U113_TEX0(&img->tex[t].tex0)->f.cbp = ((e >> 5) & 0x3FFF) + vb;
        U113_TEX0(&img->tex[t].tex0)->f.csa = e;
    }
}

// FUNC 0x00248FF8 clut_anim_channel_reset
// (bootstrap 0x0100B680.)  Rewinds a channel to its first key: time 0,
// dir +1, lerp 1.0, cur 0, next = (nKeys != 0), hold = 1 if keys[0].time <
// 0 (compared against the just-stored time, i.e. 0.0f) else 0.  keys[0] is
// read even when nKeys == 0.  Does not touch dirty.
void clut_anim_channel_reset(ClutAnimChannel *ch)
{
    ch->time = 0.0f;                            /* sw $zero */
    ch->dir = 1;
    ch->lerp = 1.0f;
    ch->cur = 0;
    ch->hold = 0;
    ch->next = (ch->nKeys != 0);
    if (ch->keys[0].time < ch->time)
        ch->hold = 1;
}

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
/* EE MMI, as in the binary (see the bootstrap): CLUT_PEXTLW broadcasts a
 * weight, CLUT_BLEND blends 4 RGBA bytes: pextlb + pextlh widen, pmulth /
 * pmaddh multiply-accumulate in LO/HI, psrlw 12, ppach + ppacb pack. */
#define CLUT_PEXTLW(d, s) __asm__ __volatile__("pextlw %0, %1, %1" : "=r"(d) : "r"(s))
#define CLUT_BLEND(a, b, w4, wn4)                                           \
    __asm__ __volatile__("pextlb %0, $0, %0\n\tpextlh %0, $0, %0\n\t"     \
                         "pextlb %1, $0, %1\n\tpextlh %1, $0, %1\n\t"     \
                         "pmulth %0, %0, %2\n\tpmaddh %0, %1, %3\n\t"     \
                         "psrlw %0, %0, 12\n\tppach %0, $0, %0\n\t"       \
                         "ppacb %0, $0, %0"                               \
                         : "+r"(a), "+r"(b) : "r"(w4), "r"(wn4) : "hi", "lo")
#endif

// FUNC 0x00249048 clut_anim_channel_apply
// (bootstrap 0x0100B6D0.)  If dirty: w = (int)(lerp * 4096.0f), wn = 0x1000
// - w; for each of nColors colours dest[i] = per byte channel
// (cur[i].c * w + next[i].c * wn) >> 12, packed back to bytes, where
// cur/next are keys[cur].palette / keys[next].palette; then dirty = 0.  The
// binary does this with EE MMI (pextlw / pextlb / pextlh / pmulth / pmaddh /
// psrlw / ppach / ppacb); the C model's s16 casts reproduce the signed
// halfword multiply.
// 75.5%, the bootstrap's open function (bootstrap/MATCHING.md, Open
// functions: the MMI asm helpers and the loop shape); the same code here.
void clut_anim_channel_apply(ClutAnimChannel *ch)
{
    s32 w, wn;
    const u32 *src0, *src1;
    u32 *dst;
    u32 n;

    if (ch->dirty == 0)
        return;

    w  = (s32)(ch->lerp * 4096.0f);
    wn = 0x1000 - w;
    src0 = ch->keys[ch->cur].palette;
    src1 = ch->keys[ch->next].palette;
    dst  = ch->dest;
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    {
        u128 w4, wn4;

        CLUT_PEXTLW(w4, w);
        CLUT_PEXTLW(wn4, wn);
        CLUT_PEXTLW(w4, w4);
        CLUT_PEXTLW(wn4, wn4);
        for (n = 0; n < ch->nColors; n++) {
            u32 a = *src0++;
            u32 b = *src1++;

            CLUT_BLEND(a, b, w4, wn4);
            *dst++ = a;
        }
    }
#else
    for (n = ch->nColors; n != 0; n--) {
        u32 a = *src0++;
        u32 b = *src1++;
        u32 out = 0;
        int k;

        for (k = 0; k < 4; k++) {
            s32 ca = (s32)((a >> (8 * k)) & 0xFF);
            s32 cb = (s32)((b >> (8 * k)) & 0xFF);
            u32 sum = (u32)(ca * (s16)w) + (u32)(cb * (s16)wn);   /* pmulth + pmaddh */
            out |= ((sum >> 12) & 0xFF) << (8 * k);                /* psrlw, ppach, ppacb */
        }
        *dst++ = out;
    }
#endif
    ch->dirty = 0;
}

/* The CLUT animation entry points share one shape: the exported function
 * checks img->clutAnim and hands it to a table-level inline helper (which
 * checks the channel array and loops over the channels); the per-channel
 * work of clut_anim_step is a further inline helper.  None of the helpers
 * has code of its own (see the bootstrap for why the split is visible). */

static inline void clut_anim_table_apply(ClutAnimTable *tbl)
{
    int i;

    if (tbl->ch == NULL)
        return;
    for (i = 0; i < tbl->count; i++)
        clut_anim_channel_apply(&tbl->ch[i]);
}

// FUNC 0x00249110 clut_anim_apply_all
// (bootstrap 0x0100B798.)  Recomputes every dirty channel's palette: for
// each channel of img->clutAnim (NULL table, NULL channel array or count 0
// -> nothing) clut_anim_channel_apply.
void clut_anim_apply_all(Tex1Image *img)
{
    if (img->clutAnim == NULL)
        return;
    clut_anim_table_apply(img->clutAnim);          /* implicit: clut_anim_channel_apply (inline) */
}

/* One channel of clut_anim_step (see there).  k mirrors ch->cur. */
static inline void clut_anim_channel_step(ClutAnimChannel *ch, float dt)
{
    int k;              /* current key index (== ch->cur throughout) */
    f32 t;
    f32 prev;

    if (ch->hold > 0)
        return;
    prev = ch->time;
    t = prev + dt;
    k = ch->cur;

    /* ---- forward ---- */
    while (ee_absf(ch->keys[ch->cur].time) <= t) {
        t -= ee_absf(ch->keys[ch->cur].time);
        ch->cur = ch->next;
        switch (ch->mode) {
        case 0:                                         /* loop */
            k = ch->cur;
            if (k != (int)ch->nKeys - 1)
                ch->next = (u16)(k + 1);
            else
                ch->next = 0;
            break;
        case 1:                                         /* ping-pong */
            if (ch->cur == 0) {
                ch->dir = 1;
                k = ch->cur;
            } else {
                k = ch->cur;
                if (k == (int)ch->nKeys - 1)
                    ch->dir = -1;
            }
            ch->next = (u16)(ch->cur + ch->dir);
            break;
        case 2:                                         /* once */
            k = ch->cur;
            if (k == (int)ch->nKeys - 1)
                goto done;                              /* last key: stop here */
            ch->next = (u16)(k + 1);
            break;
        default:
            k = ch->cur;
            break;
        }
        if (ch->keys[ch->cur].time < 0.0f)
            ch->hold++;
        if (ch->hold > 0) {
            t = 0.0f;
            goto done;
        }
    }

    /* ---- backward (negative dt) ---- */
    while (t < 0.0f) {
        if (ch->keys[ch->cur].time < 0.0f && 0.0f < prev)
            ch->hold++;
        if (ch->hold > 0) {
            t = 0.0f;
            goto done;
        }
        ch->next = ch->cur;
        switch (ch->mode) {
        case 0:                                         /* loop */
            ch->cur = (ch->cur != 0) ? ch->cur - 1 : ch->nKeys - 1;
            k = ch->cur;
            break;
        case 1:                                         /* ping-pong */
            if (ch->cur == 0)
                ch->dir = -1;
            else if (k == (int)ch->nKeys - 1)
                ch->dir = 1;                            /* binary stores the mode value (1) */
            ch->cur = ch->cur - ch->dir;
            k = ch->cur;
            break;
        case 2:                                         /* once */
            if (ch->cur == 0) {
                ch->next = 1;
                goto done;
            }
            ch->cur = ch->cur - 1;
            k = ch->cur;
            break;
        default:
            break;
        }
        t += ee_absf(ch->keys[ch->cur].time);
        prev = t;
    }

done:
    ch->time = t;
    t = EE_MAXF(t, 0.0f);
    ch->lerp = 1.0f - t / ee_absf(ch->keys[ch->cur].time);
    ch->dirty = 1;
}

static inline void clut_anim_table_step(ClutAnimTable *tbl, int id, float dt)
{
    int i;

    if (tbl->ch == NULL)
        return;
    for (i = 0; i < tbl->count; i++) {
        if (id >= 0 && tbl->ch[i].id != id)
            continue;
        clut_anim_channel_step(&tbl->ch[i], dt);
    }
}

// FUNC 0x00249180 clut_anim_step
// (bootstrap 0x0100B808.)  Advances the CLUT animation of channel `id`
// (every channel if id < 0) by dt.  Channels with hold > 0 are frozen.
// t = time + dt; forward: while |keys[cur].time| <= t, t -= that, cur =
// next and next is chosen by mode (0 loop, 1 ping-pong via dir, 2 once -
// stops on the last key, other: next unchanged); entering a key with
// negative time increments hold, and once hold > 0 the channel stops with
// t = 0.  If t ends up negative (dt < 0) it walks backwards (next = cur,
// cur moves back by mode; mode 2 on key 0 sets next = 1 and stops) adding
// |key time| until t >= 0; there a negative-time key only counts towards
// hold on the first step and only if the old time was > 0.  Finally time =
// t, lerp = 1 - max(t, 0) / |keys[cur].time|, dirty = 1.  (dt is declared
// before id: u102's callers set $f12 before $a1; EABI registers unchanged.)
// 98.5%, the bootstrap's open function (sched2 puts add.s straight after
// its lwc1 in retail, which changes a b delay slot); the same code here.
void clut_anim_step(Tex1Image *img, float dt, int id)
{
    if (img->clutAnim == NULL)
        return;
    clut_anim_table_step(img->clutAnim, id, dt);
}

static inline void clut_anim_table_release(ClutAnimTable *tbl, int id)
{
    int i;

    if (tbl->ch == NULL)
        return;
    for (i = 0; i < tbl->count; i++) {
        if (id >= 0 && tbl->ch[i].id != id)
            continue;
        tbl->ch[i].hold--;                              /* s16: lhu, -1, sh */
    }
}

// FUNC 0x002494A0 clut_anim_release
// (bootstrap 0x0100BB28.)  Decrements hold (16-bit wrap) of channel `id`,
// or of every channel when id < 0 - the counterpart of the negative-time
// hold keys in clut_anim_step.
void clut_anim_release(Tex1Image *img, int id)
{
    if (img->clutAnim == NULL)
        return;
    clut_anim_table_release(img->clutAnim, id);
}

static inline void clut_anim_table_reset(ClutAnimTable *tbl, int id)
{
    int i;

    if (tbl->ch == NULL)
        return;
    for (i = 0; i < tbl->count; i++) {
        /* Lupo 2 (L2 0x00264720): only the channels of animation `id' when
         * id >= 0 (clut_anim_step's test) */
        if (id >= 0 && tbl->ch[i].id != id)
            continue;
        clut_anim_channel_reset(&tbl->ch[i]);
        tbl->ch[i].dirty = 1;
        /* Lupo 2 never applies here: an id reset waits for the next
         * clut_anim_apply_all (CarModel::applyAnims, every frame).  GT3's
         * "all" reset (id < 0) keeps its immediate apply: the prize models
         * never apply their animations themselves (backend.md 5). */
        if (id < 0)
            clut_anim_channel_apply(&tbl->ch[i]);
    }
}

// FUNC 0x00249518 clut_anim_reset_all
// (bootstrap 0x0100BBA0.)  For every channel: clut_anim_channel_reset,
// dirty = 1, clut_anim_channel_apply - i.e. rewinds all animations and
// writes keys[0]'s palette to the CLUT.
// The enhanced engine (rendering.md 1, backend.md 5): Lupo 2's select id
// (L2 0x00264720).  id < 0 is GT3's reset above (every caller GT3 has passes
// -1); id >= 0 rewinds only the channels with that id and leaves them dirty
// for the next apply (CarModel::setAnimSelect, T446).
void clut_anim_reset_all(Tex1Image *img, int id)
{
    if (img->clutAnim == NULL)
        return;
    clut_anim_table_reset(img->clutAnim, id);      /* implicit: clut_anim_channel_reset, clut_anim_channel_apply (inline) */
}

/* Per-channel part of clut_anim_table_relocate (always inlined): dest and
 * keys rebased when non-NULL, each key's non-NULL palette pointer rebased
 * (only when keys != NULL). */
static inline void clut_anim_channel_relocate(ClutAnimChannel *ch, s32 delta)
{
    TEX1_RELOC(ch->dest, delta);
    if (ch->keys != NULL) {
        int n;
        int j;

        RELOC(ch->keys, delta);
        n = ch->nKeys;
        for (j = 0; j < n; j++) {
            if (ch->keys[j].palette != NULL)
                RELOC(ch->keys[j].palette, delta);
        }
    }
}

// FUNC 0x002495B8 clut_anim_table_relocate
// (bootstrap 0x0100BC40.)  Relocation of the animation table (tbl itself
// already rebased by the caller): nothing if ch == NULL; else ch += delta
// and per channel dest and keys are rebased when non-NULL, each key's
// non-NULL palette pointer is rebased (only when keys != NULL), then the
// channel is reset (clut_anim_channel_reset) and dirty cleared.
void clut_anim_table_relocate(ClutAnimTable *tbl, s32 delta)
{
    int i;

    if (tbl->ch == NULL)
        return;
    RELOC(tbl->ch, delta);
    for (i = 0; i < (int)tbl->count; i++) {
        clut_anim_channel_relocate(&tbl->ch[i], delta);
        clut_anim_channel_reset(&tbl->ch[i]);
        tbl->ch[i].dirty = 0;
    }
}

// FUNC 0x002496B0 tex1_variation_count
/* The number of variations of the set (ptrTable[0]), 0 without a table. */
u32 tex1_variation_count(Tex1Image *img)
{
    u32 *tbl = img->ptrTable;

    if (tbl == NULL)
        return 0;
    return tbl[0];
}

// FUNC 0x002496D0 tex1_upload_packet
/* The upload of every transfer record queued into the packet buffer
 * instead of sent (cf. tex1_upload_to_vram): per record a DMA cnt of 6
 * qwords (VIF FLUSH, DIRECT 6) holding the BITBLTBUF / TRXPOS / TRXREG /
 * TRXDIR + IMAGE header (gs_build_image_transfer_packet), then a DMA ref
 * of the pixel data with VIF DIRECT of its qword count.  (GSBuffer::loadTex1
 * and two more callers.)
 * 79.9%: retail keeps the loop's end (img->nXfer re-read, i++) behind
 * the last packet store and gives img / the address mask other s
 * registers; here sched1 moves the loop test up.  The loop body in a
 * do { } while (0) keeps the end in place (82.3%, registers still off),
 * not adopted.  Tried: 120 orders of the ref stores, while / do loops,
 * an xfer pointer. */
void tex1_upload_packet(Tex1Image *img)
{
    GsContext *ctx = GS_SPR_CTX;
    int i;

    for (i = 0; i < (int)img->nXfer; i++) {
        u32 *hdr = ctx->cur;
        Tex1Xfer *x = &img->xfer[i];
        u32 *ref;
        int qwc;

        ctx->cur = hdr + 0x1C;                  /* 7 qwords */
        hdr[0] = DMATAG_CNT(6);
        hdr[1] = 0;
        hdr[3] = VIF_DIRECTHL(6);
        hdr[2] = VIF_FLUSH;
        qwc = gs_build_image_transfer_packet(hdr + 4, (const u32 *)&x->dbp, 0, 0, x->w, x->h);
        ref = ctx->cur;
        ref[0] = DMATAG_REF(qwc);               /* the pixels */
        ref[1] = (u32)x->data & 0x0FFFFFFF;
        ref[3] = VIF_DIRECTHL(qwc);
        ctx->cur = ref + 4;
        ref[2] = 0;
    }
}

// FUNC 0x002497F0 tex1_upload_to_vram
// (bootstrap 0x0100BD38.)  gfx_sync_pipeline, then per transfer record:
// build the 6-qword BITBLTBUF / TRXPOS (0,0) / TRXREG / TRXDIR + IMAGE
// GIFtag header in a stack buffer (gs_build_image_transfer_packet, returns
// the image qwc), FlushCache(0), lock GIF DMA, send the header (6 qwc) then
// the pixel data, unlock; finally gfx_dma_sync(2).
void tex1_upload_to_vram(Tex1Image *img)
{
    u32 pkt[6 * 4] __attribute__((aligned(16)));   /* sp+0x00, 6 qwords */
    int i;

    gfx_sync_pipeline();
    for (i = 0; i < (int)img->nXfer; i++) {
        Tex1Xfer *x = &img->xfer[i];
        int qwc = gs_build_image_transfer_packet(pkt, (const u32 *)&x->dbp, 0, 0, x->w, x->h);

        FlushCache(WRITEBACK_DCACHE);
        gfx_dma_lock(DMAC_GIF);
        gfx_dma_send(DMAC_GIF, pkt, 6);
        gfx_dma_send(DMAC_GIF, x->data, (u32)qwc);
        gfx_dma_unlock(DMAC_GIF);
    }
    gfx_dma_sync(DMAC_GIF);
}

/* The ring's point (1, 0, 0) and its transform. */
const f32 g_00344AD0[4] = {             /* 0x00344AD0 */
    1.0f, 0.0f, 0.0f, 0.0f
};
static f32 g_004A6870[4];               /* 0x004A6870 */

// FUNC 0x002498C8 ring_draw
/* A ring of n vertex pairs from angle deg0 to deg1 (a full circle when
 * deg0 == deg1; then the last pair repeats the first): per pair the point
 * g_00344AD0 (1, 0, 0) rotated by -deg about z (VU0 matrix from identity,
 * vu0_rotate_z) as the outer vertex and the same scaled by 1 - width as
 * the inner one, colours colorOuter / colorInner.  Both arrays are
 * allocated from the end of the packet buffer; drawn by pgl_draw_strip with
 * 2n vertices (no normals, no texture coordinates).  The angle step is
 * span / (n - 1).  Caller: gt2k_menu::CircleGauge::virtual_2. */
void ring_draw(u32 colorInner, u32 colorOuter, int n, f32 deg0, f32 deg1,
               f32 width)
{
    f32 *xyz;
    u32 *rgba;
    f32 *pair;
    f32 span, step;
    int last, count;
    int i;

    gs_flush_dirty_regs();
    xyz = (f32 *)gfx_alloc_tail((n * 6) / 4);
    rgba = (u32 *)gfx_alloc_tail(n / 2);
    for (i = 0; i < n; i++) {
        rgba[i * 2] = colorOuter;
        rgba[i * 2 + 1] = colorInner;
    }
    last = n - 1;
    count = n * 2;
    span = 360.0f;
    if (!(deg0 == deg1))
        span = deg1 - deg0;
    step = span / (f32)last;
    for (i = 0; i < n; i++) {
        pair = &xyz[i * 6];
        if (i == last && span == 360.0f) {
            pair[0] = xyz[0];
            pair[1] = xyz[1];
            pair[2] = xyz[2];
            pair[3] = xyz[3];
            pair[4] = xyz[4];
            pair[5] = xyz[5];
        } else {
            pgl_push_matrix();
            vu0_load_identity_regs();
            vu0_rotate_z(-deg0);
            vu0_apply_matrix(g_004A6870, g_00344AD0);
            pair[0] = g_004A6870[0];
            pair[1] = g_004A6870[1];
            pair[2] = g_004A6870[2];
            vu0_scale(1.0f - width, 1.0f - width, 1.0f - width);
            vu0_apply_matrix(g_004A6870, g_00344AD0);
            pair[3] = g_004A6870[0];
            pair[4] = g_004A6870[1];
            pair[5] = g_004A6870[2];
            pgl_pop_matrix();
        }
        deg0 = deg0 + step;
    }
    pgl_draw_strip(xyz, NULL, rgba, NULL, count);
}
