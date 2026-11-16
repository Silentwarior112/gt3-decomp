/* T462 - TirePacket: the geometry of a tyre, built once into strip
 * packets.  0x0021FB00..0x002212D8.
 *
 * virtual_1 builds, with a TStripBuilder on the stack, three packets from
 * a table of nseg + 1 points of the unit circle (x = sin, y = cos):
 *   packetOut     shader 2, PRIM 0x3C (textured strips), lit: the tread
 *                 (from x0 to the width, radius R), the shoulder bevel (45
 *                 degrees, if bevel), the sidewall (rim radius to R -
 *                 bevel); F1 tyres get both shoulders and sidewalls.
 *   packetIn      shader 0, PRIM 0x2C: the face disc (zig-zag over the
 *                 circle) and, if `inside', the inner faces drawn in
 *                 colour 0x80000000 instead of with normals.
 *   packetShadow  shader 0, PRIM 0x6C (alpha): a strip under the tyre, its
 *                 alpha a parabola along the contact.
 * All coordinates except the shadow's are divided by the rim radius (the
 * tyre is drawn scaled by it; x0 is 0.001 for the menu tyres, u101).  The
 * texture: s runs around the tyre (texHeight repeats, from a random offset,
 * backwards on one side unless the wheel is mirrored there), t across it
 * (the tread 0 .. 0.5, the shoulder .. 0.625, the sidewall .. 1).  The
 * packets are G001 VifModels; their shader numbers go to the part-texture
 * callback (virtual_2), which sets up the tread's texture wrap, blend and
 * material. */
#include "game/car/model/car_model_internal.h"
#include "game/enhanced.h"                  /* ENH_RENDER_TYRE_SHOULDER */

/* The packet tire_packet_pass_callback forwards to (T462 .sdata: a global
 * without an initialiser, zeros at the end of the file's data). */
TirePacket *g_00352AC8;                     /* 0x00352AC8 */

// FUNC 0x0021FB00 tire_packet_pass_callback
int tire_packet_pass_callback(int pass)
{
    return g_00352AC8->virtual_2(pass);
}

// FUNC 0x0021FB30 TirePacket::TirePacket
TirePacket::TirePacket()
{
    this->packetOut = NULL;
    this->packetIn = NULL;
    this->packetShadow = NULL;
    this->shadow = 0;
    this->nseg = 30;
    this->bevel = 1;
    this->texOffset = (f32)rand() * 4.6566128730773926e-10f;   /* rand() / 2^31: 0 .. 1 */
}

// FUNC 0x0021FBA0 TirePacket::~TirePacket
TirePacket::~TirePacket()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr; the deleting form (flags & 1) calls operator delete(this) */
}

// FUNC 0x0021FBD0 TirePacket::setup
void TirePacket::setup(TireModelBase *model, int nseg, int bevel, int inside, int shadow,
                       f32 x0)
{
    this->model = model;
    this->nseg = nseg;
    this->bevel = bevel;
    this->shadow = shadow;
    this->inside = inside;
    this->x0 = x0;
}

/* Draws the tyre: the tread texture made resident and sent, the two
 * packets, then the wheel when the outer
 * face is seen. */
// FUNC 0x0021FBF0 TirePacket::draw
void TirePacket::draw()
{
    TireModelBase *m = this->model;
    bindPassCallback();
    tire_file_load_texture(m->tireFile);
    tire_file_send_texture(m->tireFile);
    model_draw(this->packetOut);
    model_draw(this->packetIn);
    if (m->faceVisible)
        m->drawWheel();
}

/* The shadow strip: alpha blended over the ground, depth tested, without
 * depth writes. */
// FUNC 0x0021FC58 TirePacket::drawShadow
void TirePacket::drawShadow()
{
    if (this->packetShadow == NULL)
        return;
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_depth_mask(0);
    gfx_blend_func(PGL_BLEND_ALPHA, 1.0f);
    model_draw(this->packetShadow);
    gfx_depth_mask(1);
}

/* The tyre material (T462 .data): ambient 0.3, diffuse 0.15, specular 0.9,
 * emission (0, 0, 0, 1), shininess 16 (virtual_2 replaces it with the tyre
 * file's power). */
GsMaterial g_002E5E90 = {                   /* 0x002E5E90 */
    { 0.3f, 0.3f, 0.3f, 0.0f },
    { 0.15f, 0.15f, 0.15f, 0.0f },
    { 0.9f, 0.9f, 0.9f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    16.0f,
};

/* The part-texture callback: shader 1, nothing; shader 2 (the tread side):
 * the texture repeats around the tyre and is clamped across it, additive
 * by destination alpha, the tyre material with the tyre file's power.
 * 1 if the shader is known. */
// FUNC 0x0021FCC8 TirePacket::virtual_2
int TirePacket::virtual_2(int pass)
{
    switch (pass) {
    case TIRE_SHADER_1:
        return 1;
    case TIRE_SHADER_TREAD:
        gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REPEAT);
        gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_CLAMP);
        gfx_blend_func(PGL_BLEND_ADD_DEST_A, 1.0f);
        g_002E5E90.shininess = this->model->tireFile->power;
        material_load(&g_002E5E90);
        return 1;
    }
    return 0;
}

// FUNC 0x0021FD48 TirePacket::bindPassCallback
void TirePacket::bindPassCallback()
{
    g_00352AC8 = this;
    /* G001 calls it as a void (*)(s32) and drops the result: the same
     * call on the EE (h03). */
    pgl_set_part_tex_func((PglPartTexFunc)tire_packet_pass_callback);
}

/* Builds the packets into buf; returns the end of what was written. */
// FUNC 0x0021FD70 TirePacket::virtual_1
void *TirePacket::virtual_1(void *buf)
{
    TireCircle *circle = (TireCircle *)__builtin_alloca((this->nseg + 1) * sizeof(TireCircle));
    makeCircle(circle);
    TStripBuilder sb(buf);                  /* fp+0x000; ~TStripBuilder at the end (and on a throw) */
    virtual_3(&sb, circle);
    this->packetOut = sb.finish(1, 0);      /* lit */
    virtual_4(&sb, circle);
    this->packetIn = sb.finish(0, 0);
    if (this->shadow) {
        virtual_5(&sb, circle);
        this->packetShadow = sb.finish(0, 0);
    }
    this->built = 1;
    return sb.cur;                          /* getCurrentAddress, inline */
}

/* The tread side: the tread, the shoulder and the sidewall (F1 tyres: both
 * shoulders and sidewalls), textured and lit. */
// FUNC 0x0021FEB8 TirePacket::virtual_3
void TirePacket::virtual_3(TStripBuilder *sb, TireCircle *c)
{
    sb->setShader(TIRE_SHADER_TREAD);
    sb->setPrim(PRIM_TRISTRIP | PRIM_IIP | PRIM_TME | PRIM_FGE);
    if (this->model->tireFile->isF1()) {
        buildTread(sb, c, 1, 0, 0);
        buildShoulder(sb, c, 1, 0, 0, 0);
        buildSidewall(sb, c, 1, 0, 0, 0);
        buildShoulder(sb, c, 1, 0, 0, 1);
        buildSidewall(sb, c, 1, 0, 0, 1);
    } else {
        buildTread(sb, c, 1, 0, 0);
        buildShoulder(sb, c, 1, 0, 0, 0);
        buildSidewall(sb, c, 1, 0, 0, 0);
    }
}

/* The face disc and, if `inside', the inside (the other face, the tread
 * reversed; F1 tyres also the far shoulder and sidewall), in colour.
 * The enhanced engine (ENH_RENDER_TYRE_SHOULDER, rendering.md 9): an
 * ordinary tyre's inside also gets its shoulder, as in Lupo 2 (L2
 * 0x00238870). */
// FUNC 0x00220020 TirePacket::virtual_4
void TirePacket::virtual_4(TStripBuilder *sb, TireCircle *c)
{
    sb->setShader(TIRE_SHADER_NONE);
    sb->setPrim(PRIM_TRISTRIP | PRIM_IIP | PRIM_FGE);
    buildFace(sb, c, 0);
    if (this->inside == 0)
        return;
    if (this->model->tireFile->isF1()) {
        buildFace(sb, c, 1);
        buildTread(sb, c, 0, 1, 1);
        buildShoulder(sb, c, 0, 1, 1, 1);
        buildSidewall(sb, c, 0, 1, 1, 1);
    } else {
        buildFace(sb, c, 1);
        buildTread(sb, c, 0, 1, 1);
#if ENH_RENDER_TYRE_SHOULDER
        /* Lupo 2 (L2 0x00238994..0x002389AC, rendering.md 9): the ordinary
         * tyre's inside gets its shoulder strip too */
        buildShoulder(sb, c, 0, 1, 1, 0);
#endif
    }
}

// FUNC 0x00220150 TirePacket::virtual_5
void TirePacket::virtual_5(TStripBuilder *sb, TireCircle *c)
{
    sb->setShader(TIRE_SHADER_NONE);
    sb->setPrim(PRIM_TRISTRIP | PRIM_IIP | PRIM_FGE | PRIM_ABE);
    if (this->shadow)
        buildShadow(sb, c);
}

/* The bevel as a fraction of the sidewall height. */
// FUNC 0x002201B8 TirePacket::bevelRatio
f32 TirePacket::bevelRatio()
{
    f32 ratio = 0.25f;
    if (this->model->tireFile->isF1())
        ratio = 0.3f;
    return ratio;
}

/* The tread: a strip around the circle at radius R, x from x0 (+ bevel)
 * to the width (- bevel on F1 tyres); swap exchanges the two edges.  uv:
 * with texture coordinates; color: with colour 0x80000000 instead of the
 * normals.  The normals lean out at the x0 edge (45 degrees, 22.5 with a
 * bevel) and at the far edge of F1 tyres.
 * 93.3%: the loop head's schedule (retail loads c and tEdge before the
 * colour flags, the pointer flags in s3 / s2 / s1 interleaved with the
 * products).  Tried: the flags before / after the c loads, as addVertex
 * arguments, the c loads in both orders. */
// FUNC 0x002201E8 TirePacket::buildTread
void TirePacket::buildTread(TStripBuilder *sb, TireCircle *c, int uv, int color, int swap)
{
    TireModelBase *m = this->model;
    int pos = m->position;
    int mirrored = wheel_file_mirrored(m->wheelFile, pos);
    pos &= 1;
    f32 bevelH = (m->outer - m->rim) * bevelRatio();
    f32 s, ds;                              /* the texture's s around the tyre; ds: sp+0x44 */
    int odd = (pos != 0);
    if (mirrored ? (odd ^ 1) != 0 : odd != 0) {
        s = this->texOffset + 0.0f;
        ds = (f32)m->tireFile->texHeight / (f32)this->nseg;
    } else {
        s = (f32)m->tireFile->texHeight + this->texOffset;
        ds = -(f32)m->tireFile->texHeight / (f32)this->nseg;
    }
    f32 tEdge;                              /* sp+0x48 the texture's t at the x0 edge */
    if (this->bevel)
        tEdge = 0.5f;
    else
        tEdge = 1.0f - 0.375f / (1.0f - bevelRatio());
    f32 x0 = this->x0;
    f32 xEdge0;                             /* the x0 edge */
    if (this->bevel)
        xEdge0 = bevelH + x0;
    else
        xEdge0 = x0 + 0.0f;
    f32 xEdgeW;                             /* the width edge */
    if (!m->tireFile->isF1())
        xEdgeW = m->width;
    else if (this->bevel)
        xEdgeW = m->width - bevelH;
    else
        xEdgeW = m->width;
    f32 inv = 1.0f / m->rim;
    f32 radius = m->outer * inv;
    xEdgeW *= inv;
    xEdge0 *= inv;
    if (swap) {
        f32 t = xEdgeW;
        xEdgeW = xEdge0;
        xEdge0 = t;
    }
    f32 nx = -0.70710677f;                  /* the normal at the x0 edge: x, radial */
    if (this->bevel)
        nx = -0.38268343f;
    f32 nr = 0.70710677f;
    if (this->bevel)
        nr = 0.9238795f;
    sb->endStrip();
    for (int i = 0; i < this->nseg + 1; i++) {
        f32 p[3];                           /* sp+0x00 */
        f32 n[3];                           /* sp+0x10 */
        f32 st[2];                          /* sp+0x20 */
        u32 col;                            /* sp+0x30 */
        f32 cx = c[i].x;
        f32 cy = c[i].y;
        f32 *np = (color == 0) ? n : NULL;
        f32 *stp = (uv != 0) ? st : NULL;
        u32 *colp = (color != 0) ? &col : NULL;
        col = 0x80000000;
        p[0] = xEdge0;
        n[0] = nx;
        n[2] = nr * cy;
        n[1] = nr * cx;
        p[1] = radius * cx;
        p[2] = radius * cy;
        st[0] = s;
        st[1] = tEdge;
        sb->addVertex(p, np, stp, colp);
        p[1] = radius * cx;
        p[2] = radius * cy;
        p[0] = xEdgeW;
        if (m->tireFile->isF1()) {
            n[1] = nr * cx;
            n[2] = nr * cy;
            n[0] = -nx;
        } else {
            n[1] = cx;
            n[2] = cy;
            n[0] = 0.0f;
        }
        st[0] = s;
        st[1] = 0.0f;
        sb->addVertex(p, np, stp, colp);
        s += ds;
    }
    sb->endStrip();
}

/* The sidewall: a flat ring at x0 (mirror: at the width), from the rim
 * radius to R (- bevel).  The circle is walked backwards when mirror and
 * rev differ.
 * 93.0%: FP register pressure differs - retail keeps tTop on the stack
 * (sp+0x48) and loads its 1.0 twice, and schedules the first vertex's
 * stores differently.  Tried: the second vertex's products as locals in
 * several orders, store orders (hill climb), the circle point by pointer /
 * index / per-arm loads. */
// FUNC 0x00220600 TirePacket::buildSidewall
void TirePacket::buildSidewall(TStripBuilder *sb, TireCircle *c, int uv, int color, int rev,
                               int mirror)
{
    TireModelBase *m = this->model;
    int mirrored = wheel_file_mirrored(m->wheelFile, m->position);
    f32 bevelH = (m->outer - m->rim) * bevelRatio();
    f32 s, ds;
    if (mirrored) {
        s = this->texOffset + 0.0f;
        ds = (f32)m->tireFile->texHeight / (f32)this->nseg;
    } else {
        s = (f32)m->tireFile->texHeight + this->texOffset;
        ds = -(f32)m->tireFile->texHeight / (f32)this->nseg;
    }
    f32 tTop;                               /* sp+0x48 the texture's t at the top */
    if (this->bevel)
        tTop = 0.625f;
    else
        tTop = 1.0f - 0.375f / (1.0f - bevelRatio());
    f32 x;
    if (mirror)
        x = m->width;
    else
        x = this->x0;
    f32 rTop;
    if (this->bevel)
        rTop = m->outer - bevelH;
    else
        rTop = m->outer - 0.0f;
    f32 nxRim = -0.70710677f;               /* the normals' x at the rim and at the top */
    f32 inv = 1.0f / m->rim;
    f32 nxTop = nxRim;
    f32 rRim = m->rim * inv;
    rTop *= inv;
    x *= inv;
    if (this->bevel)
        nxTop = -0.9238795f;
    f32 nrTop = 0.70710677f;                /* the radial part at the top */
    if (this->bevel)
        nrTop = 0.38268343f;
    if (mirror) {
        nxTop = -nxTop;
        nxRim = 0.70710677f;
    }
    sb->endStrip();
    int backwards = (rev != 0);
    for (int i = 0; i < this->nseg + 1; i++) {
        f32 p[3];                           /* sp+0x00 */
        f32 n[3];                           /* sp+0x10 */
        f32 st[2];                          /* sp+0x20 */
        u32 col;                            /* sp+0x30 */
        col = 0x80000000;
        f32 *np = (color == 0) ? n : NULL;
        f32 *stp = (uv != 0) ? st : NULL;
        u32 *colp = (color != 0) ? &col : NULL;
        f32 cy, cx;
        if (mirror ? (backwards ^ 1) != 0 : backwards != 0) {
            cy = c[this->nseg - i].y;
            cx = c[this->nseg - i].x;
        } else {
            cy = c[i].y;
            cx = c[i].x;
        }
        f32 rimX = rRim * cx;
        f32 rimY = rRim * cy;
        f32 nrX = -0.70710677f * cx;
        f32 nrY = -0.70710677f * cy;
        f32 topX = rTop * cx;
        f32 topY = rTop * cy;
        f32 ntX = nrTop * cx;
        f32 ntY = nrTop * cy;
        st[0] = s;
        p[0] = x;
        n[0] = nxRim;
        p[1] = rimX;
        p[2] = rimY;
        n[1] = nrX;
        n[2] = nrY;
        st[1] = 1.0f;
        sb->addVertex(p, np, stp, colp);
        st[0] = s;
        p[1] = topX;
        p[2] = topY;
        s += ds;
        n[1] = ntX;
        n[2] = ntY;
        p[0] = x;
        n[0] = nxTop;
        st[1] = tTop;
        sb->addVertex(p, np, stp, colp);
    }
    sb->endStrip();
}

/* The shoulder (bevel only): from (x0, R - bevel) to (x0 + bevel, R), or
 * mirrored from (width, R - bevel) to (width - bevel, R).
 * 91.9%: retail keeps m->outer in f20 and a copy (mov.s f4) for rBase, and
 * schedules the loop body's products and stores differently (as in
 * buildSidewall).  Tried: bevelH from the loads or the locals, the products
 * as locals, store orders (hill climb over products and stores). */
// FUNC 0x00220A00 TirePacket::buildShoulder
void TirePacket::buildShoulder(TStripBuilder *sb, TireCircle *c, int uv, int color, int rev,
                               int mirror)
{
    TireModelBase *m = this->model;
    if (this->bevel == 0)
        return;
    int pos = m->position;
    int mirrored = wheel_file_mirrored(m->wheelFile, pos);
    pos &= 1;
    f32 ratio = bevelRatio();
    f32 outerR = m->outer;
    f32 rim = m->rim;
    f32 bevelH = (outerR - rim) * ratio;
    f32 s, ds;
    int odd = (pos != 0);
    if (mirrored ? (odd ^ 1) != 0 : odd != 0) {
        s = this->texOffset + 0.0f;
        ds = (f32)m->tireFile->texHeight / (f32)this->nseg;
    } else {
        s = (f32)m->tireFile->texHeight + this->texOffset;
        ds = -(f32)m->tireFile->texHeight / (f32)this->nseg;
    }
    f32 xTop;                               /* the tread's edge */
    if (mirror)
        xTop = m->width - bevelH;
    else
        xTop = bevelH + this->x0;
    f32 nxTop = -0.38268343f;               /* sp+0x48 the normal at the top: x, radial */
    f32 nrTop = 0.9238795f;
    f32 xBase;                              /* the sidewall's edge */
    if (mirror)
        xBase = m->width;
    else
        xBase = this->x0;
    f32 rBase = outerR - bevelH;
    f32 inv = 1.0f / rim;
    f32 nxBase = -0.9238795f;               /* sp+0x4C the normal at the base: x, radial */
    f32 nrBase = 0.38268343f;
    f32 rTop = outerR * inv;
    rBase *= inv;
    xTop *= inv;
    xBase *= inv;
    if (mirror) {
        nxBase = 0.9238795f;
        nxTop = -nxTop;
    }
    sb->endStrip();
    int backwards = (rev != 0);
    for (int i = 0; i < this->nseg + 1; i++) {
        f32 p[3];                           /* sp+0x00 */
        f32 n[3];                           /* sp+0x10 */
        f32 st[2];                          /* sp+0x20 */
        u32 col;                            /* sp+0x30 */
        col = 0x80000000;
        f32 *np = (color == 0) ? n : NULL;
        f32 *stp = (uv != 0) ? st : NULL;
        u32 *colp = (color != 0) ? &col : NULL;
        f32 cy, cx;
        if (mirror ? (backwards ^ 1) != 0 : backwards != 0) {
            cy = c[this->nseg - i].y;
            cx = c[this->nseg - i].x;
        } else {
            cy = c[i].y;
            cx = c[i].x;
        }
        f32 baseX = rBase * cx;
        f32 baseY = rBase * cy;
        f32 nbX = nrBase * cx;
        f32 nbY = nrBase * cy;
        f32 topX = rTop * cx;
        f32 topY = rTop * cy;
        f32 ntX = nrTop * cx;
        f32 ntY = nrTop * cy;
        n[0] = nxBase;
        p[1] = baseX;
        p[2] = baseY;
        n[1] = nbX;
        n[2] = nbY;
        st[0] = s;
        p[0] = xBase;
        st[1] = 0.625f;
        sb->addVertex(p, np, stp, colp);
        st[0] = s;
        p[1] = topX;
        s += ds;
        p[2] = topY;
        n[1] = ntX;
        n[2] = ntY;
        p[0] = xTop;
        n[0] = nxTop;
        st[1] = 0.5f;
        sb->addVertex(p, np, stp, colp);
    }
    sb->endStrip();
}

/* The face disc at x = width: one strip zig-zagging over the circle
 * (points 1, n, 2, n-1, ...; rev starts from the other end), colour
 * 0x80000000, radius the rim (F1) or R. */
// FUNC 0x00220DF0 TirePacket::buildFace
void TirePacket::buildFace(TStripBuilder *sb, TireCircle *c, int rev)
{
    TireModelBase *m = this->model;
    f32 x = m->width;
    f32 radius, rim;
    if (m->tireFile->isF1()) {
        radius = m->rim;
        rim = radius;
    } else {
        radius = m->outer;
        rim = m->rim;
    }
    int hi = this->nseg;
    int lo = 1;
    f32 inv = 1.0f / rim;
    x *= inv;
    radius *= inv;
    sb->endStrip();
    for (int i = 0; i < this->nseg; i++) {
        f32 p[3];                           /* sp+0x00 */
        u32 col;                            /* sp+0x10 */
        int k;
        col = 0x80000000;
        int odd = i & 1;
        if (rev ? (odd ^ 1) != 0 : odd != 0)
            k = hi--;
        else
            k = lo++;
        p[0] = x;
        p[1] = radius * c[k].x;
        p[2] = radius * c[k].y;
        sb->addVertex(p, NULL, NULL, &col);
    }
    sb->endStrip();
}

/* The shadow: a strip across the tyre (x = -w/2 - 0.02 .. w/2 + 0.02, not
 * scaled by the rim) along the lower half circle; the alpha of the outer
 * edge is 0, of the inner 256 - (i - q)^2 * 256 / q^2 (q = nseg / 4 + 1),
 * clamped to 0..255, 0 at both ends. */
// FUNC 0x00220F68 TirePacket::buildShadow
void TirePacket::buildShadow(TStripBuilder *sb, TireCircle *c)
{
    TireModelBase *m = this->model;
    int q = this->nseg / 4 + 1;
    f32 outerR = m->outer;
    f32 w = m->width;
    f32 xNear = -w * 0.5f + 0.02f;
    f32 xFar = w * 0.5f + 0.02f;
    f32 falloff = 256.0f / (f32)(q * q);
    f32 zScale = (m->diameter * 0.5f) / outerR;
    f32 lift = 0.003f;                      /* kept in a register (f27) as in retail */
    sb->endStrip();
    for (int i = 0; i < this->nseg / 2 + 1; i++) {
        f32 p[3];                           /* sp+0x00 */
        u32 colNear;                        /* sp+0x10 */
        u32 colFar;                         /* sp+0x14 */
        f32 y = outerR * c[i].x + lift;
        f32 z = outerR * c[i].y * zScale;
        int alpha = 0;
        if (i != 0 && i != this->nseg / 2) {
            int d = i - q;
            alpha = 0x100 - (int)((f32)(d * d) * falloff);
            if (alpha >= 0x100)
                alpha = 0xFF;
            if (alpha < 0)
                alpha = 0;
        }
        colNear = 0;
        colFar = alpha << 24;
        p[0] = xNear;
        p[1] = y;
        p[2] = z;
        sb->addVertex(p, NULL, NULL, &colNear);
        p[1] = y;
        p[2] = z;
        p[0] = xFar;
        sb->addVertex(p, NULL, NULL, &colFar);
    }
    sb->endStrip();
}

/* nseg + 1 points of the unit circle by rotation: (sin, cos) of
 * i * 2pi / nseg, the last one set to (0, 1) exactly. */
// FUNC 0x002211A0 TirePacket::makeCircle
void TirePacket::makeCircle(TireCircle *c)
{
    f32 sinStep, cosStep;                   /* sp+0x00, sp+0x04 */
    sincosf(6.2831855f / (f32)this->nseg, &sinStep, &cosStep);
    f32 x = 0.0f;
    f32 y = 1.0f;
    for (int i = 0; i < this->nseg; i++) {
        c[i].x = x;
        c[i].y = y;
        f32 nextX = sinStep * y + cosStep * x;
        y = cosStep * y - sinStep * x;
        x = nextX;
    }
    c[this->nseg].y = 1.0f;
    c[this->nseg].x = 0.0f;
}

/* The prebuilt packet's one part (a block as TStripBuilder::flush writes it,
 * from its +0x10, the strip table after the positions): a hexagonal prism
 * x 0..1, radius 1, as two strips, the side (14 vertices) and the cap at
 * x = 0 (6), all in colour 0x80000000.  Only g_00340CE0 points at it (T462
 * .rodata, after the vtable and the type name: an internal-linkage const,
 * deferred to the end of the file's .rodata). */
struct TirePrismPart {
    u32 unpackTag;                          /* +0x000 UNPACK V4-32 1 to 0x0C0 */
    u32 tag[4];                             /* +0x004 the GIF tag */
    u32 unpackPos;                          /* +0x014 UNPACK V3-32 20 to 0x000 */
    f32 pos[20][3];                         /* +0x018 */
    u32 unpackStrips;                       /* +0x108 UNPACK S-8 3 to 0x040 */
    u8  strips[4];                          /* +0x10C the strip count, 3 * (vertices - 2) per strip */
    u32 unpackCol;                          /* +0x110 UNPACK V4-8 20 to 0x080 */
    u32 col[20];                            /* +0x114 */
    u32 pad[3];                             /* +0x164 to the quadword */
};
EGCS_SIZE(TirePrismPart, 0x170);

static const TirePrismPart g_00340D40 __attribute__((aligned(16))) = {   /* 0x00340D40 */
    0x6C01C0C0,
    { 0x00008014, 0x30164000, 0x00000412, 0x00000000 },   /* NLOOP 20, EOP; PRE, PRIM 0x2C, NREG 3; ST RGBAQ XYZF2 */
    0x6814C000,
    {
        { 0.0f, -0.5f,  0.8660254f }, { 1.0f, -0.5f,  0.8660254f },
        { 0.0f,  0.5f,  0.8660254f }, { 1.0f,  0.5f,  0.8660254f },
        { 0.0f,  1.0f,  0.0f       }, { 1.0f,  1.0f,  0.0f       },
        { 0.0f,  0.5f, -0.8660254f }, { 1.0f,  0.5f, -0.8660254f },
        { 0.0f, -0.5f, -0.8660254f }, { 1.0f, -0.5f, -0.8660254f },
        { 0.0f, -1.0f,  0.0f       }, { 1.0f, -1.0f,  0.0f       },
        { 0.0f, -0.5f,  0.8660254f }, { 1.0f, -0.5f,  0.8660254f },
        { 0.0f, -0.5f,  0.8660254f }, { 0.0f,  0.5f,  0.8660254f },
        { 0.0f, -1.0f,  0.0f       }, { 0.0f,  1.0f,  0.0f       },
        { 0.0f, -0.5f, -0.8660254f }, { 0.0f,  0.5f, -0.8660254f },
    },
    0x6203C040,
    { 2, 36, 12, 0 },
    0x6E14C080,
    {
        0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000,
        0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000,
        0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000,
        0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000,
    },
    { 0, 0, 0 },
};

/* The prebuilt packet (T462 .rodata, before the vtable): one part of 0x17
 * quadwords, 20 vertices, 16 triangles, unlit. */
const VifModel g_00340CE0 = {               /* 0x00340CE0 */
    NULL, NULL, 0, 0, 1, 20, 16,
    { { (u32 *)&g_00340D40, 0x17, 0 } },
};

/* Draws the prebuilt packet g_00340CE0 (a coarse unit tyre) with gfx state
 * 0xF (cullEnable) off. */
// FUNC 0x00221260 F_00221260
void F_00221260(void)
{
    gfx_disable(PGL_CAP_CULL_FACE);
    model_draw((VifModel *)&g_00340CE0);
    gfx_enable(PGL_CAP_CULL_FACE);
}

// FUNC 0x00221290 TirePacket::isBuilt
int TirePacket::isBuilt()
{
    return this->built;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __user_type_info
 * __ti10TirePacket (0x00353448, name 0x00340D30) on the first call. */
// FUNC 0x00221298 __tf10TirePacket
extern "C" void *__tf10TirePacket(void)
{
    if (__ti10TirePacket[0] == 0)
        __rtti_user(__ti10TirePacket, "10TirePacket");
    return __ti10TirePacket;
}
#endif
