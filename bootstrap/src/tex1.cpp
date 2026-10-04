/* tex1.cpp - 'Tex1' texture-set library of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), unit "startup", .text 0x0100B2E8..0x0100BE0F.
 *
 * A plain-C translation unit (no .eh_frame data, only leaf helpers and GS
 * driver calls), so everything here has C linkage.  Structures: see
 * include/startup_screen.h.
 *
 * Live in this binary (called from startup_screen_start, 0x01000A50):
 *   tex1_relocate       - turn the file's offsets into pointers
 *   tex1_set_vram_base  - link the set for VRAM block 0x2800 (word 0xA0000)
 *   tex1_upload_to_vram - IMAGE transfers over GIF DMA (channel 2)
 * The CLUT animation part is dead: clut_anim_apply_all, clut_anim_step,
 * clut_anim_release and clut_anim_reset_all have no references at all, and
 * clut_anim_table_relocate (-> clut_anim_channel_reset) only runs when
 * Tex1Image.clutAnim != NULL, which is 0 in startupa.img.
 */
#include "types.h"
#include "eekernel.h"
#include "gs_driver.h"
#include "startup_screen.h"

/* EE COP1 abs.s / max.s (clut_anim_step).
 *
 * ee_absf: abs.s through a one-instruction inline asm under the matching
 * compiler.  egcs does emit abs.s for __builtin_fabsf, but the code around
 * it shows the original used an asm helper: egcs's jump pass
 * (duplicate_loop_exit_test) refuses to copy a loop exit test that contains
 * an asm, and the retail forward loop of clut_anim_step keeps its
 * "b <test>" entry jump with |keys[cur].time| <= t at the bottom.  Spelled
 * with __builtin_fabsf, a ?: macro, a statement-expression macro or a C
 * inline function, the test is duplicated in front of the loop instead
 * (all tried).  The retail `t -= |time|` also reuses the test's abs.s
 * result, which identical non-volatile asms allow (CSE).  The C fallback
 * computes the same value (abs.s only clears the sign bit).
 *
 * EE_MAXF: max.s.  The C++ front end of egcs has the GNU maximum operator
 * `>?` (MAX_EXPR, the maxsf3 pattern = max.s); a > b ? a : b on floats is
 * not folded to MAX_EXPR without -ffast-math and gives c.lt.s/bc1f/mov.s.
 * GCC 14 no longer has the operator, hence the ?: fallback (same value for
 * the finite, non-NaN EE floats). */
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

/* Adds delta to the 14-bit TBP-style field at bit `shift` of a GS register
 * (field & 0x3FFF, 32-bit add, & 0x3FFF, merged back - the carry is lost). */
static inline u64 gs_rebase_tbp_field(u64 reg, int shift, s32 delta)
{
    u32 f = (u32)(reg >> shift) & 0x3FFF;

    return (reg & ~(0x3FFFull << shift)) | ((u64)((f + (u32)delta) & 0x3FFF) << shift);
}

/* Pointer + byte delta (the relocation arithmetic of tex1_relocate).
 * RELOC is a do { } while (0) statement macro and TEX1_RELOC its NULL-checked
 * form.  The wrappers are visible in the retail code: egcs keeps the loop
 * notes of a do-while(0), which are scheduling barriers (the store of the
 * rebased pointer stays ahead of the next load) and align the first label
 * inside to 8 bytes (the padding nop after a TEX1_RELOC's store). */
#define REBASE(p, delta)  ((__typeof__(p))((u8 *)(p) + (delta)))
#define RELOC(p, d)      do { (p) = REBASE(p, d); } while (0)
#define TEX1_RELOC(p, d) do { if ((p) != NULL) (p) = REBASE(p, d); } while (0)


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
// FUNC 0x0100B2E8 tex1_set_vram_base
// Re-links the texture set for a new VRAM base: base = vramWordAddr >> 6
// (64-word blocks), delta = base - img->vramBase; nothing to do (returns img)
// when delta == 0.  Otherwise stores the new base and, for every GsTexRegs,
// adds delta to TEX0.TBP0 (bits 0-13) and TEX0.CBP (37-50), and if TEX1.MXL
// > 0 to MIPTBP1.TBP1/2/3 (bits 0, 20, 40), plus MIPTBP2.TBP4/5/6 when MXL >=
// 4; each field wraps at 14 bits.  Then adds delta to the 16-bit DBP of every
// transfer record (wraps at 16 bits, no 14-bit mask).  The tex pointer is not
// NULL-checked (only nTex); the xfer loop checks both pointer and count.
// Caller: startup_screen_start with 0xA0000 (block 0x2800).
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

// FUNC 0x0100B560 tex1_relocate
// Converts the set's internal offsets to pointers for its current address:
// delta = img - img->self; returns at once when 0.  Else self = img and
// ptr08 / tex / xfer / ptrTable / clutAnim are rebased when non-NULL; every
// transfer's data pointer is rebased unconditionally (count read once);
// ptrTable = {count, ptr[count]} gets its non-NULL entries rebased; the CLUT
// animation table goes through clut_anim_table_relocate.  Returns img.
// Caller: startup_screen_start (result ignored).
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

// FUNC 0x0100B680 clut_anim_channel_reset
// Rewinds a channel to its first key: time 0, dir +1, lerp 1.0, cur 0,
// next = (nKeys != 0), hold = 1 if keys[0].time < 0 (compared against the
// just-stored time, i.e. 0.0f) else 0.  keys[0] is read even when nKeys == 0.
// Does not touch dirty.  Callers: clut_anim_reset_all, clut_anim_table_relocate.
// (Renamed from clut_fade_channel_reset.)
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
/* EE MMI, as in the binary (the matching compiler keeps a u128 in one 128-bit
 * GPR).  CLUT_PEXTLW: pextlw d, s, s.  CLUT_BLEND: a = per byte
 * (a * w + b * wn) >> 12 with w4 / wn4 the weights broadcast to 4 words:
 * pextlb + pextlh widen the RGBA bytes to words, pmulth / pmaddh multiply the
 * even halfwords (signed 16 x 16) and accumulate in LO/HI (rd gets
 * LO.w0 HI.w0 LO.w2 HI.w2 = the 4 channel sums), psrlw 12, ppach + ppacb keep
 * the low byte of each word.  One asm statement, so nothing is scheduled
 * between pmulth and pmaddh (LO/HI), which it declares clobbered.  GCC 14
 * cannot hold a u128 in one register; it (and -DNON_MATCHING) gets the C
 * model below, which computes the same bytes. */
#define CLUT_PEXTLW(d, s) __asm__ __volatile__("pextlw %0, %1, %1" : "=r"(d) : "r"(s))
#define CLUT_BLEND(a, b, w4, wn4)                                           \
    __asm__ __volatile__("pextlb %0, $0, %0\n\tpextlh %0, $0, %0\n\t"     \
                         "pextlb %1, $0, %1\n\tpextlh %1, $0, %1\n\t"     \
                         "pmulth %0, %0, %2\n\tpmaddh %0, %1, %3\n\t"     \
                         "psrlw %0, %0, 12\n\tppach %0, $0, %0\n\t"       \
                         "ppacb %0, $0, %0"                               \
                         : "+r"(a), "+r"(b) : "r"(w4), "r"(wn4) : "hi", "lo")
#endif

// FUNC 0x0100B6D0 clut_anim_channel_apply
// If dirty: w = (int)(lerp * 4096.0f) (EE cvt.w.s truncates), wn = 0x1000 - w;
// for each of nColors colours dest[i] = per byte channel
// (cur[i].c * w + next[i].c * wn) >> 12, packed back to bytes, where cur/next
// are keys[cur].palette / keys[next].palette; then dirty = 0.  The binary
// does this with EE MMI: pextlw x2 broadcast w / wn to 4 words (as halfword
// pairs {w, 0}), per colour pextlb+pextlh widen the RGBA bytes to words,
// pmulth (signed 16x16, even halfwords) + pmaddh accumulate, psrlw 12,
// ppach + ppacb keep the low byte of each word.  Written in C here; the s16
// casts reproduce the signed halfword multiply.  Callers: clut_anim_apply_all,
// clut_anim_reset_all (both dead).  (Renamed from clut_fade_apply.)
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

/* The CLUT animation entry points below share one shape: the exported
 * function checks img->clutAnim and hands it to a table-level inline helper
 * (which checks the channel array and loops over the channels), and the
 * per-channel work of clut_anim_step is a further inline helper.  None of
 * the helpers has code of its own.  The split is visible in the retail
 * code: the helper's ClutAnimTable * is a fresh pseudo (an inline argument
 * copy), so egcs's GCSE does not reuse the tbl->ch value loaded by the NULL
 * check for the loop (the loops reload tbl->ch on entry and after every
 * call), and the per-channel helper's argument &tbl->ch[i] is expanded as
 * an address sum ((i << 5) + tbl->ch, the operand order of the retail
 * addu).  Written as one function, the same statements give the check's
 * value carried into the loop and a different register assignment. */

static inline void clut_anim_table_apply(ClutAnimTable *tbl)
{
    int i;

    if (tbl->ch == NULL)
        return;
    for (i = 0; i < tbl->count; i++)
        clut_anim_channel_apply(&tbl->ch[i]);
}

// FUNC 0x0100B798 clut_anim_apply_all
// Recomputes every dirty channel's palette: for each channel of
// img->clutAnim (NULL table, NULL channel array or count 0 -> nothing)
// clut_anim_channel_apply.  Dead code (no references).  (Renamed from
// clut_fade_update_all; 0x0100B808 and 0x0100BB28 were split off this block.)
void clut_anim_apply_all(Tex1Image *img)
{
    if (img->clutAnim == NULL)
        return;
    clut_anim_table_apply(img->clutAnim);
}

/* One channel of clut_anim_step (see there).  k mirrors ch->cur (it is
 * reloaded from ch->cur whenever cur changes); the key times are read as
 * ch->keys[ch->cur], the hold counter as ch->hold.  In the retail code GCSE
 * keeps ch->keys, ch->cur and ch->hold in registers across the loops (the
 * hold copy in $t1, keys in $t3 / $t2), and k << 3 is recomputed for every
 * key access, which a `keys[k]` spelling does not give. */
static inline void clut_anim_channel_step(ClutAnimChannel *ch, float dt)
{
    int k;              /* $a3: current key index (== ch->cur throughout) */
    f32 t;              /* $f3 */
    f32 prev;           /* $f2 */

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

// FUNC 0x0100B808 clut_anim_step
// Advances the CLUT animation of channel `id` (every channel if id < 0) by dt.
// Channels with hold > 0 are frozen.  t = time + dt; forward: while
// |keys[cur].time| <= t, t -= that, cur = next and next is chosen by mode
// (0 loop, 1 ping-pong via dir, 2 once - stops on the last key, other: next
// unchanged); entering a key with negative time increments hold, and once
// hold > 0 the channel stops with t = 0.  If t ends up negative (dt < 0) it
// walks backwards (next = cur, cur moves back by mode; mode 2 on key 0 sets
// next = 1 and stops) adding |key time| until t >= 0; there a negative-time
// key only counts towards hold on the first step and only if the old time was
// > 0.  Finally time = t, lerp = 1 - max(t, 0) / |keys[cur].time|, dirty = 1.
// Dead code (no references; split off the F_0100B798 block by the mapping).
// Register notes: the channel index lives in t4, t0 doubles as the constant 1.
void clut_anim_step(Tex1Image *img, int id, float dt)
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

// FUNC 0x0100BB28 clut_anim_release
// Decrements hold (16-bit wrap) of channel `id`, or of every channel when
// id < 0 - the counterpart of the negative-time hold keys in clut_anim_step.
// Dead code (no references; split off the F_0100B798 block by the mapping).
void clut_anim_release(Tex1Image *img, int id)
{
    if (img->clutAnim == NULL)
        return;
    clut_anim_table_release(img->clutAnim, id);
}

static inline void clut_anim_table_reset(ClutAnimTable *tbl)
{
    int i;

    if (tbl->ch == NULL)
        return;
    for (i = 0; i < tbl->count; i++) {
        clut_anim_channel_reset(&tbl->ch[i]);
        tbl->ch[i].dirty = 1;
        clut_anim_channel_apply(&tbl->ch[i]);
    }
}

// FUNC 0x0100BBA0 clut_anim_reset_all
// For every channel: clut_anim_channel_reset, dirty = 1, clut_anim_channel_apply
// - i.e. rewinds all animations and writes keys[0]'s palette to the CLUT.
// Dead code (no references).  (Renamed from clut_fade_finish_all.)
void clut_anim_reset_all(Tex1Image *img)
{
    if (img->clutAnim == NULL)
        return;
    clut_anim_table_reset(img->clutAnim);
}

/* Per-channel part of clut_anim_table_relocate (always inlined, no code of
 * its own): dest and keys rebased when non-NULL, each key's non-NULL palette
 * pointer rebased (only when keys != NULL).  An inline function, not a block
 * in the loop: egcs expands an inline call's argument &tbl->ch[i] as an
 * address sum, (i << 5) + tbl->ch, which is the operand order of the
 * retail addu; a local pointer gets tbl->ch + (i << 5). */
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

// FUNC 0x0100BC40 clut_anim_table_relocate
// Relocation of the animation table (tbl itself already rebased by the
// caller): nothing if ch == NULL; else ch += delta and per channel dest and
// keys are rebased when non-NULL, each key's non-NULL palette pointer is
// rebased (only when keys != NULL), then the channel is reset
// (clut_anim_channel_reset) and dirty cleared.  Caller: tex1_relocate.
// (Renamed from clut_fade_table_relocate.)
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

// FUNC 0x0100BD38 tex1_upload_to_vram
// gfx_sync_pipeline, then per transfer record: build the 6-qword BITBLTBUF /
// TRXPOS (0,0) / TRXREG / TRXDIR + IMAGE GIFtag header in a stack buffer
// (gs_build_image_transfer_packet, returns the image qwc), FlushCache(0),
// lock GIF DMA, send the header (6 qwc) then the pixel data, unlock; finally
// gfx_dma_sync(2).  (gfx_dma_send itself waits for the previous transfer, so
// the stack buffer is not rewritten under a running DMA.)
// Caller: startup_screen_start.
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
