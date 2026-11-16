/* T454_sprite_model.cpp - SpriteModel (? name; unit u102).
 *
 * A model of camera-facing quads (? trees, spectators): each part is a list
 * of entries, either a sprite (a quad of the sprite's width and height
 * standing at x, y, z, turned to face the camera about the vertical) or,
 * with bit 15 of the index, a prebuilt packet.  One TEX1 texture.
 */
#include "engine/gfx/model_set_internal.h"
#include "vu0asm.h"
#include "engine/gfx/T454_sprite_model.h"

/* SpriteEntry.idx bit 15: a packet number, not a sprite. */
enum { SPRITE_ENTRY_PACKET = 0x8000 };

/* .bss (a file static): the camera's right axis in model space, xz only,
 * normalised (the quads of spriteset_draw_part are built along it). */
static GsVec4 g_0049D1A0;                   /* 0x0049D1A0 */

/* As modelset_relocate: statement macros (U102_RELOC_STMT, and
 * U102_RELOC_NZ for unk_08 and the material offsets), one index per loop,
 * the material offsets through a pointer view (not the union). */
// FUNC 0x00226AA0 spriteset_relocate
void spriteset_relocate(SpriteModel *m)
{
    int delta;

    if (m == NULL)
        return;
    delta = (u8 *)m - (u8 *)m->base;
    m->base = m;
    U102_RELOC_NZ(m->unk_08, delta);
    if (m->parts != NULL) {
        int i;
        U102_RELOC_STMT(m->parts, delta);
        for (i = 0; i < m->nParts; i++) {
            if (m->parts[i] != NULL)
                U102_RELOC_STMT(m->parts[i], delta);
        }
    }
    if (m->tex != NULL) {
        U102_RELOC_STMT(m->tex, delta);
        tex1_relocate(m->tex);
    }
    if (m->materials != NULL) {
        int i;
        U102_RELOC_STMT(m->materials, delta);
        for (i = 0; i < m->nMaterials; i++)
            U102_RELOC_NZ(*(void **)&m->materials[i].x48.offset, delta);
    }
    if (m->sprites != NULL) {
        int i;
        U102_RELOC_STMT(m->sprites, delta);
        for (i = 0; i < m->nSprites; i++) {
            if (m->sprites[i].uvs != NULL)
                U102_RELOC_STMT(m->sprites[i].uvs, delta);
            if (m->sprites[i].colors != NULL)
                U102_RELOC_STMT(m->sprites[i].colors, delta);
        }
    }
    if (m->packets != NULL) {
        int i;
        U102_RELOC_STMT(m->packets, delta);
        for (i = 0; i < m->nPackets; i++) {
            if (m->packets[i] != NULL) {
                U102_RELOC_STMT(m->packets[i], delta);
                vif_model_relocate(m->packets[i]);
            }
        }
    }
}

// FUNC 0x00226C80 spriteset_reset_clut_anims
void spriteset_reset_clut_anims(SpriteModel *m)
{
    if (m != NULL && m->tex != NULL)
        clut_anim_reset_all(m->tex);
}

// FUNC 0x00226CB0 spriteset_step_clut_anims
void spriteset_step_clut_anims(SpriteModel *m, f32 dt, int id)
{
    if (m != NULL && m->tex != NULL)
        clut_anim_step(m->tex, dt, id);
}

// FUNC 0x00226CE0 spriteset_apply_clut_anims
void spriteset_apply_clut_anims(SpriteModel *m)
{
    if (m != NULL && m->tex != NULL)
        clut_anim_apply_all(m->tex);
}

// FUNC 0x00226D10 spriteset_release_clut_anims
void spriteset_release_clut_anims(SpriteModel *m, int id)
{
    if (m != NULL && m->tex != NULL)
        clut_anim_release(m->tex, id);
}

/* The render states (texture, fog, depth test, blending; alpha test EQUAL
 * 0x80, failing pixels written to the colour buffer only, not to Z), then
 * the billboard axis: with R = the
 * rotation of the modelview matrix (vu0_load_modelview: vf20..vf22) and t its
 * translation (vf23), vf23 = R^T * -t (? the eye in model space: unused
 * here), vf5 = R^T * (0, 0, 1), g_0049D1A0 = (0, 1, 0) x vf5, then
 * normalised in x and z (rsqrt.s).  Last, the texture is made resident and
 * the packets get m's texture and material tables with texmat_select as
 * their texture hook.
 * Under egcs the transpose is one asm whose five 128-bit temporaries are
 * outputs (the compiler picks retail's registers in this operand order),
 * the store of vf7 is an asm of its own (its address is formed after the
 * VU0 code), the rsqrt.s a volatile asm; the two table calls sit in a
 * statement block (retail aligns the return label after it). */
// FUNC 0x00226D40 spriteset_begin_draw
void spriteset_begin_draw(SpriteModel *m)
{
    f32 axisX, axisZ, invLen;

    if (m == NULL)
        return;
    gfx_enable(PGL_CAP_VU1_PROG_33E);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_enable(PGL_CAP_TEXTURE);
    gfx_enable(PGL_CAP_FOG);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_enable(PGL_CAP_BLEND);
    gfx_depth_func(PGL_DEPTHFUNC_LEQUAL);
    gfx_alpha_func(PGL_ATEST_EQUAL, 1.0f);        /* AREF 0x80 */
    gfx_alpha_fail(PGL_AFAIL_FB_ONLY);
    gfx_blend_func(PGL_BLEND_ALPHA, 0.0f);
    __asm__ volatile(
        "vnop\n\t"
        "vmr32.xyzw   $vf5, $vf0\n\t");         /* vf5 = (0, 0, 1, 0) */
    vu0_load_modelview();
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    {
        u128 r0, r1, r2, t0, t1;

        __asm__ volatile(
            "vmr32.xyzw   $vf6, $vf5\n\t"       /* vf6 = (0, 1, 0, 0) */
            "vsub.xyz     $vf23, $vf0, $vf23");
        /* transpose the 3x3 of vf20..vf22 (w = 0): r0..r2 the rows */
        __asm__ volatile(
            "qmfc2.ni     %0, $vf20\n\t"
            "qmfc2.ni     %4, $vf21\n\t"
            "qmfc2.ni     %2, $vf22\n\t"
            "pextlw       %3, %4, %0\n\t"
            "pextuw       %1, %4, %0\n\t"
            "pextlw       %4, $0, %2\n\t"
            "pextuw       %2, $0, %2\n\t"
            "pcpyld       %0, %4, %3\n\t"
            "pcpyud       %4, %3, %4\n\t"
            "pcpyld       %2, %2, %1\n\t"
            "qmtc2.ni     %0, $vf20\n\t"
            "qmtc2.ni     %4, $vf21\n\t"
            "qmtc2.ni     %2, $vf22\n\t"
            : "=&r"(r0), "=&r"(t1), "=&r"(r2), "=&r"(t0), "=&r"(r1));
    }
    __asm__ volatile(
        "vmulax.xyz   " VU0_ACC ", $vf20, $vf23x\n\t"
        "vmadday.xyz  " VU0_ACC ", $vf21, $vf23y\n\t"
        "vmaddz.xyz   $vf23, $vf22, $vf23z\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf5x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf5y\n\t"
        "vmaddz.xyzw  $vf5, $vf22, $vf5z\n\t"
        "vopmula.xyz  " VU0_ACC ", $vf6, $vf5\n\t"
        "vopmsub.xyz  $vf7, $vf5, $vf6\n\t");
    __asm__ volatile("sqc2         $vf7, %0" : "=m"(g_0049D1A0));
    axisX = g_0049D1A0.x;
    axisZ = g_0049D1A0.z;
    {
        f32 sq = axisX * axisX + axisZ * axisZ;
        __asm__ volatile("rsqrt.s %0, %2, %1" : "=f"(invLen) : "f"(sq), "f"(1.0f));
    }
    g_0049D1A0.x = axisX * invLen;
    g_0049D1A0.z = axisZ * invLen;
#else
    __asm__ volatile(
        "vmr32.xyzw   $vf6, $vf5\n\t"           /* vf6 = (0, 1, 0, 0) */
        "vsub.xyz     $vf23, $vf0, $vf23\n\t"
        /* transpose the 3x3 of vf20..vf22 (w = 0) */
        "qmfc2.ni     $6, $vf20\n\t"
        "qmfc2.ni     $2, $vf21\n\t"
        "qmfc2.ni     $4, $vf22\n\t"
        "pextlw       $3, $2, $6\n\t"
        "pextuw       $5, $2, $6\n\t"
        "pextlw       $2, $0, $4\n\t"
        "pextuw       $4, $0, $4\n\t"
        "pcpyld       $6, $2, $3\n\t"
        "pcpyud       $2, $3, $2\n\t"
        "pcpyld       $4, $4, $5\n\t"
        "qmtc2.ni     $6, $vf20\n\t"
        "qmtc2.ni     $2, $vf21\n\t"
        "qmtc2.ni     $4, $vf22\n\t"
        "vmulax.xyz   " VU0_ACC ", $vf20, $vf23x\n\t"
        "vmadday.xyz  " VU0_ACC ", $vf21, $vf23y\n\t"
        "vmaddz.xyz   $vf23, $vf22, $vf23z\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf5x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf5y\n\t"
        "vmaddz.xyzw  $vf5, $vf22, $vf5z\n\t"
        "vopmula.xyz  " VU0_ACC ", $vf6, $vf5\n\t"
        "vopmsub.xyz  $vf7, $vf5, $vf6\n\t"
        "sqc2         $vf7, 0(%0)\n\t"
        : : "r"(&g_0049D1A0) : "memory", "$2", "$3", "$4", "$5", "$6");
    axisX = g_0049D1A0.x;
    axisZ = g_0049D1A0.z;
    invLen = u102_rsqrt(1.0f, axisX * axisX + axisZ * axisZ);
    g_0049D1A0.x = axisX * invLen;
    g_0049D1A0.z = axisZ * invLen;
#endif
    g_002F4250.loadTex1(m->tex);
    pgl_set_part_tex_func(texmat_select);
    do {
        texsel_set_table(m->tex->tex);
        matsel_set_table(m->materials);
    } while (0);
}

/* (m is unused) */
// FUNC 0x00226EB0 spriteset_end_draw
void spriteset_end_draw(SpriteModel *m)
{
    gfx_disable(PGL_CAP_TEXTURE);
    gfx_disable(PGL_CAP_FOG);
    gfx_disable(PGL_CAP_BLEND);
    gfx_alpha_fail(PGL_AFAIL_KEEP);
}

/* The quads of a part, batched (strip_batch_begin / strip_batch_end) while
 * the texture stays the same; a packet entry closes the batch and forgets
 * the texture.  A quad: one strip of 4 vertices (12 bytes each) in 3
 * qwords of the packet's tail: (x - ax w, y + h, z - az w), (x - ax w, y,
 * z - az w), (x + ax w, y + h, z + az w), (x + ax w, y, z + az w); ax, az =
 * g_0049D1A0, with the sprite's colours and uvs.  The coordinates are
 * written straight into the buffer (vertex 3, then 1, then 2 and 0 from
 * them): the float stores may alias g_0049D1A0 and the sprite, so retail
 * reloads w, h and the axis after each store.  curTex is a u16 like the
 * entry's tex (a promoted register, spilled as a word): with an int, egcs
 * reused the compared e->tex for texmat_select on the no-batch path, where
 * retail reloads it in the call's delay slot on both. */
// FUNC 0x00226EE8 spriteset_draw_part
void spriteset_draw_part(SpriteModel *m, int part)
{
    SpritePart *list;
    SpriteEntry *e;
    u16 curTex;                             /* as SpriteEntry.tex */
    int i, batchOpen;

    if (m == NULL || part >= m->nParts)
        return;
    list = m->parts[part];
    if (list == NULL)
        return;
    e = list->e;
    curTex = 0;
    batchOpen = 0;
    for (i = 0; i < list->count; i++, e++) {
        u32 idx = e->idx;
        if (idx & SPRITE_ENTRY_PACKET) {
            if (batchOpen != 0) {
                batchOpen = 0;
                strip_batch_end();
            }
            curTex = 0;
            idx &= ~SPRITE_ENTRY_PACKET;        /* and 0xFFFF7FFF */
            model_draw(m->packets[idx]);
        } else {
            Sprite *s = &m->sprites[idx];
            f32 *v;
            if (curTex != e->tex) {
                if (batchOpen != 0) {
                    batchOpen = 0;
                    strip_batch_end();
                }
                texmat_select(e->tex);
                curTex = e->tex;
            }
            v = (f32 *)gfx_alloc_tail(3);
            v[9]  = e->x + g_0049D1A0.x * s->w;     /* 0x24 vertex 3 */
            v[10] = e->y;                           /* 0x28 */
            v[11] = e->z + g_0049D1A0.z * s->w;     /* 0x2C */
            v[3]  = e->x - g_0049D1A0.x * s->w;     /* 0x0C vertex 1 */
            v[4]  = e->y;                           /* 0x10 */
            v[5]  = e->z - g_0049D1A0.z * s->w;     /* 0x14 */
            v[6]  = v[9];                           /* 0x18 vertex 2 */
            v[7]  = v[10] + s->h;                   /* 0x1C */
            v[8]  = v[11];                          /* 0x20 */
            v[0]  = v[3];                           /* 0x00 vertex 0 */
            v[1]  = v[4] + s->h;                    /* 0x04 */
            v[2]  = v[5];                           /* 0x08 */
            if (batchOpen == 0) {
                strip_batch_begin();
                batchOpen = 1;
            }
            strip_draw(v, NULL, s->colors, s->uvs, 4);
        }
    }
    if (batchOpen != 0)
        strip_batch_end();
}

// FUNC 0x00227100 spriteset_draw_all
void spriteset_draw_all(SpriteModel *m)
{
    int part;

    if (m == NULL)
        return;
    spriteset_begin_draw(m);
    for (part = 0; part < m->nParts; part++)
        spriteset_draw_part(m, part);
    spriteset_end_draw(m);
}
