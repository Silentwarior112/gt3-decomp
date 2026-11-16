/* T449_race_tire_model.cpp - RaceTireModel, RaceTirePacket and the brake
 * file "race/brake.bin" (unit u101, 0x00221E08..0x00224090).
 *
 * A race car has four RaceTireModels (RaceCarModel, T180).  Each holds
 * three RaceTirePackets, one per level of detail, built once into the car's
 * work space (build) and drawn by draw(): level < 0 picks it from the
 * tyre's projected size (selectLOD), level 3 is a plain scaled cylinder.
 * The brake disc / caliper quads get their positions every frame (they turn
 * with the wheel: written straight to VU1 memory, the packets carry no
 * positions); the motion blur renders the wheel face (and the tread)
 * several times turned into a small GS buffer that then replaces the
 * texture.
 *
 * Data (retail, all EXACT at these places): .rodata 0x00340ED0..0x003410C0
 * (the hexagon, the vtables, the type names, then the file's static tables),
 * .lit4 0x0035098C..0x003509B4, .sdata 0x00352ACC..0x00352AD4, .bss
 * 0x0049D160 (the brake textures), .data 0x002E6744..0x002E70AC (the frame
 * info only).  The original saw no other class with only inline virtual
 * functions: no other type names (and no <new>: no "bad_alloc").
 */
#include "game/car/model/tire_model_internal.h"
#include "game/car/model/T449_race_tire_model.h"

/* The brake file and whether its textures are linked: uninitialised C++
 * globals, which egcs writes as zeros at the end of the file's output (here
 * .sdata, -G8) in the order of their first declaration (tire_model_internal.h's). */
BrakeFile *g_00352ACC;                      /* 0x00352ACC */
int        g_00352AD0;                      /* 0x00352AD0 */

/* The brake textures: a file static with a constructor, so .bss (retail's
 * 0x0049D160; a public object without an initialiser would be .data, at its
 * definition).  egcs generates its static initialiser and destructor, named
 * after the unit's first global (relocate__9BrakeFile), and emits them after
 * the unit's functions, the destructor first (retail: 0x00223E00 / 0x00223E28,
 * before the out-of-line inline members).  (The markers name the two
 * functions: egcs gives the initialiser this definition's line, the
 * destructor the end of the file.  A hand-written placement new tested the
 * address for NULL first.) */
// FUNC 0x00223E28 T449_static_ctor (_GLOBAL_$I$relocate__9BrakeFile)
//   The .ctors entry (0x0028F174): TexSet() of g_0049D160.
static TexSet g_0049D160;                   /* 0x0049D160 */

/* File statics (internal linkage): egcs writes them at the end of .rodata,
 * after the vtables and type names, in definition order (retail
 * 0x00340FD0..0x003410C0).  The levels of detail of RaceTireModel::build
 * (index = level; 8-aligned, so each table is followed by a zero word). */
static const int g_00340FD0[RACE_TIRE_LEVELS] = { 18, 12, 8 };    /* 0x00340FD0 nseg */
static const int g_00340FE0[RACE_TIRE_LEVELS] = { 1, 1, 0 };      /* 0x00340FE0 bevel */
static const int g_00340FF0[RACE_TIRE_LEVELS] = { 1, 0, 0 };      /* 0x00340FF0 shadow (also draw) */
static const int g_00341000[RACE_TIRE_LEVELS] = { 1, 1, 0 };      /* 0x00341000 inside (and withBrake) */
static const int g_00341010[RACE_TIRE_LEVELS] = { 0, 1, 2 };      /* 0x00341010 the wheel level */
static const f32 g_00341020[RACE_TIRE_LEVELS] = { -0.004f, -0.004f, -0.004f };  /* 0x00341020 x0 */
/* The hexagon's material (virtual_2's shader 0xD): white ambient and
 * diffuse, the rest 0. */
static const GsMaterial g_00341030 = {      /* 0x00341030 */
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
};
/* A square in the y-z plane (+-1): the corners of the disc, caliper and
 * glow strips (buildDisc, buildCaliper, buildGlow). */
static const f32 g_00341080[4][3] = {       /* 0x00341080 */
    { 0.0f,  1.0f,  1.0f },
    { 0.0f,  1.0f, -1.0f },
    { 0.0f, -1.0f,  1.0f },
    { 0.0f, -1.0f, -1.0f },
};
/* The normal of the hexagon and the disc / caliper strips. */
static const f32 g_003410B0[4] = { -1.0f, 0.0f, 0.0f };          /* 0x003410B0 */

/* The disc and caliper quads (and their texture) are mirrored on the odd
 * side, or on the even side when the wheel file mirrors the odd one
 * (wheel_file_mirrored).  The side taken before the call and converted to
 * bool after it (retail: andi in the jal's delay slot, then sltu; a call
 * ends the block, so combine cannot fold the two), both arms of the ?:
 * converting again (xori / sltu): exact in all four users. */
static inline bool brake_flipped(RaceTireModel *m)
{
    int pos = m->position;
    int odd = pos & 1;
    int mirrored = wheel_file_mirrored(m->wheelFile, pos);
    bool side = odd;
    return mirrored ? side ^ 1 : side;
}

// FUNC 0x00221E08 BrakeFile::relocate
//   Turns the offsets of the loaded file into pointers: the difference
//   between the file's address and the one it was built at (base), which
//   then becomes the file's address.  The texture set is relocated too
//   (tex1_relocate).  Caller: brake_file_load.
//   The call is a statement macro (do { } while (0)): retail's alignment
//   nop before the label after it, and the store of tex before the
//   argument copy (the loop notes keep sched1 from moving it).
#define BRAKE_TEX_RELOCATE(t) do { tex1_relocate(t); } while (0)
void BrakeFile::relocate()
{
    BrakeFile *savedAt = base;
    base = this;
    int delta = (u8 *)this - (u8 *)savedAt;
    if (tex != NULL) {
        tex = (Tex1Image *)((u8 *)tex + delta);
        BRAKE_TEX_RELOCATE(tex);
    }
    if (discs != NULL)
        discs = (BrakeDisc *)((u8 *)discs + delta);
    if (calipers != NULL)
        calipers = (BrakeCaliper *)((u8 *)calipers + delta);
}

// FUNC 0x00221E78 BrakeFile::loadTexture
//   Makes the brake textures resident in VRAM (VramAllocator::loadTex1 of
//   g_002F4250) when they are not linked in g_0049D160.  Caller:
//   RaceTirePacket::draw.
void BrakeFile::loadTexture()
{
    g_002F4250.loadTex1(tex);
}

// FUNC 0x00221EA0 RaceTireModel::RaceTireModel
//   TireModelBase(), the vptr 0x00340F18, the three RaceTirePacket() (with
//   the EH cleanup of the constructed ones and ~TireModelBase).  Caller:
//   RaceCarModel::RaceCarModel.
RaceTireModel::RaceTireModel()
{
    /* implicit: TireModelBase, ~TireModelBase, RaceTirePacket (compiler-generated: bases, members,
     * deleting form) */
}

// FUNC 0x00221F98 RaceTireModel::~RaceTireModel
//   (slot 0) The vptr, the packets 2, 1, 0 through their virtual
//   destructor (flags 0), then ~TireModelBase with the flags (the deleting
//   form's operator delete is TireModelBase's).
RaceTireModel::~RaceTireModel()
{
    /* implicit: ~TireModelBase (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x00222028 brake_file_load
//   Loads the brake file once (race_begin: "race/brake.bin", packed: unpack
//   1, a buffer of its own) and relocates it.
void brake_file_load(const char *path)
{
    if (g_00352ACC == NULL) {
        BrakeFile *file = (BrakeFile *)file_read_blocking(path, NULL, 0, 1, &vol_device_object);
        g_00352ACC = file;
        file->relocate();
    }
}

// FUNC 0x00222068 brake_file_free
//   Caller: race_end.
void brake_file_free(void)
{
    if (g_00352ACC != NULL)
        free(g_00352ACC);
    g_00352ACC = NULL;
}

// FUNC 0x00222090 RaceTireModel::setBrake
//   From the car's axle data (RaceCarModel::setupTires): the brake disc
//   and caliper numbers (1-based / 2-based in the data: a caliper number
//   below 2 is none), the disc's radius and offset, the caliper's angle in
//   degrees (kept in radians).
void RaceTireModel::setBrake(f32 radius, f32 offset, f32 angleDeg, int discNo, int caliperNo)
{
    disc = discNo - 1;
    caliper = caliperNo - 2;
    discRadius = radius;
    discOffset = offset;
    caliperAngle = angleDeg * 0.017453292f;     /* pi / 180 */
}

// FUNC 0x002220B8 RaceTireModel::build
//   Sets up the three levels from the tables (segments, bevel, shadow,
//   inside / brake, wheel level, x0) and builds each into the work space
//   (virtual_1, a virtual call); returns its rest.  Caller:
//   RaceCarModel::setupTires.
void *RaceTireModel::build(void *work)
{
    /* packets[level] at each use (one pointer giv, the counter kept
     * counting up: retail's slti 3; through a local packet pointer the loop
     * counts down and the table registers rotate). */
    for (int level = 0; level < RACE_TIRE_LEVELS; level++) {
        packets[level].setup(this, g_00340FD0[level], g_00340FE0[level], g_00340FF0[level],
                             g_00341000[level], g_00341010[level], g_00341020[level]);
        work = packets[level].virtual_1(work);
    }
    return work;
}

// FUNC 0x002221C8 RaceTireModel::screenSize
//   The projected size of the bounding sphere: the projection matrix's x
//   scale (vf28.x while pgl_matrix_mode(PGL_MTX_PROJECTION)) * bound / the
//   depth of the tyre (-vf31.z, the translation of the model view matrix).
f32 RaceTireModel::screenSize()
{
    Vec4 translation;                       /* sp+0x00 */
    Vec4 xColumn;                           /* sp+0x10 */
    vu0_get_vf31(&translation);
    f32 depth = -translation.z;
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_get_vf28(&xColumn);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    return (xColumn.x * bound) / depth;
}

// FUNC 0x00222258 RaceTireModel::selectLOD
//   0 above 0.09, 1 above 0.03, 2 above 0.015, else 3 (the cylinder).
int RaceTireModel::selectLOD()
{
    f32 size = screenSize();
    if (0.09f < size)
        return 0;
    if (0.03f < size)
        return 1;
    if (0.015f < size)
        return 2;
    return RACE_TIRE_CYLINDER;
}

// FUNC 0x002222B8 RaceTireModel::draw
//   Draws the tyre at level lod (< 0: selectLOD; >= 3: drawCylinder) in
//   the current matrix: rotated by steer (+ 180 on the odd side, bit 0 of
//   position) about y and camber about z (degrees), then spin about x
//   (negated on the even side).  An explicit lod 0 skips the view test
//   (setVisible sets clipped and faceVisible); otherwise nothing is drawn
//   when the virtual test (virtual_1, TireModelBase's) fails.  The level's
//   RaceTirePacket::draw between the render states; then, for a level with
//   a shadow strip (level 0) seen from the face side and not an F1 tyre,
//   the shadow (TirePacket::drawShadow) without the spin, colour only.
//   Callers: RaceCarModel::drawTires / drawMenuTires.
void RaceTireModel::draw(int lod, f32 steer, f32 camber, f32 spin, f32 glow,
                         int blurPasses, f32 blur)
{
    int forced = lod == 0;
    if (lod < 0)
        lod = selectLOD();
    int detailed = lod < RACE_TIRE_CYLINDER;
    if (!detailed) {
        drawCylinder(steer, camber, spin);
        return;
    }
    if (packets[lod].built == 0)
        return;
    pgl_push_matrix();
    f32 spinRad = spin * 0.017453292f;      /* pi / 180 */
    int side = position & 1;
    vu0_rotate_y(steer + (side ? 180.0f : 0.0f));
    vu0_rotate_z(camber);
    if (!forced) {
        if (virtual_1() == 0) {
            pgl_pop_matrix();
            return;
        }
    } else {
        setVisible();
    }
    if (side == 0)
        spin = -spin;
    vu0_rotate_x(spin);
    if (clipped == 0)
        gfx_disable(PGL_CAP_VU1_PROG_33E);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_set_texture_function(PGL_TFX_MODULATE);
    gfx_depth_mask(1);
    gfx_set_frame_write_mask(GS_WRITE_RGBA);
    RaceTirePacket *packet = &packets[lod];
    packet->draw(blurPasses, spinRad, glow, blur);
    gfx_enable(PGL_CAP_VU1_PROG_33E);
    gfx_depth_mask(1);
    gfx_set_frame_write_mask(GS_WRITE_RGBA);
    pgl_pop_matrix();
    /* `detailed' is tested again (always true here). */
    if (!tireFile->isF1() && detailed) {
        if (g_00340FF0[lod] != 0 && faceVisible != 0) {
            pgl_push_matrix();
            vu0_rotate_y(side ? 180.0f : 0.0f);
            vu0_rotate_z(camber);
            gfx_set_frame_write_mask(GS_WRITE_RGB);
            packet->drawShadow();
            gfx_set_frame_write_mask(GS_WRITE_RGBA);
            pgl_pop_matrix();
        }
    }
}

// FUNC 0x00222550 RaceTireModel::drawCylinder
//   The farthest level: the coarse unit tyre (F_00221260) scaled to width x
//   outer x outer and moved by -width / 2, in the same rotations as draw.
//   Callers: draw, RaceCarModel::drawTireCylinders / drawMenuTires.
void RaceTireModel::drawCylinder(f32 steer, f32 camber, f32 spin)
{
    pgl_push_matrix();
    int side = position & 1;
    vu0_rotate_y(steer + (side ? 180.0f : 0.0f));
    vu0_rotate_z(camber);
    if (side == 0)
        spin = -spin;
    vu0_rotate_x(spin);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    pgl_translate(-width * 0.5f, 0.0f, 0.0f);
    vu0_scale(width, outer, outer);
    F_00221260();
    pgl_pop_matrix();
}

// FUNC 0x00222638 brake_textures_link
//   Links the brake textures into g_0049D160 for the race (TexSet::load);
//   while they are linked, RaceTirePacket::draw does not make them
//   resident itself.  Caller: RacePS2Base::drawCars.
void brake_textures_link(void)
{
    g_0049D160.load(g_00352ACC->tex);
    g_00352AD0 = 1;
}

// FUNC 0x00222668 brake_textures_unlink
//   TexSet::clear.  Caller: RacePS2Base::drawCars.
void brake_textures_unlink(void)
{
    g_0049D160.clear();
    g_00352AD0 = 0;
}

// FUNC 0x00222690 RaceTirePacket::RaceTirePacket
//   TirePacket(), the vptr 0x00340F30, the Callback (its constructor out of
//   line: Callback::Callback), then the fields (callback.lod written again
//   here; with the EH cleanup ~TirePacket).  Caller: RaceTireModel().
RaceTirePacket::RaceTirePacket()
{
    /* implicit: TirePacket, ~TirePacket, Callback (compiler-generated: bases, members, deleting form) */
    hexagon = 0;
    withBrake = 1;
    callback.lod = 0;
    caliperPacket = NULL;
    discPacket = NULL;
    owner = NULL;
}

// FUNC 0x00222718 RaceTirePacket::~RaceTirePacket
//   (slot 0) The vptr, ~Callback of the callback (a direct call, flags 2),
//   then ~TirePacket with the flags (the deleting form's operator delete
//   is TirePacket's).
RaceTirePacket::~RaceTirePacket()
{
    /* implicit: ~TirePacket (compiler-generated: bases, members, deleting form), ~Callback (the member callback) */
}

// FUNC 0x00222768 RaceTirePacket::setup
//   TirePacket::setup (inside and shadow in its order), then owner,
//   withBrake = inside, the wheel level and hexagon = (level 2).  Caller:
//   RaceTireModel::build.
void RaceTirePacket::setup(RaceTireModel *m, int nseg, int bevel, int shadow, int inside, int lod,
                           f32 x0)
{
    TirePacket::setup(m, nseg, bevel, inside, shadow, x0);
    owner = m;
    withBrake = inside;
    callback.lod = lod;
    if (lod == RACE_TIRE_HEXAGON)
        hexagon = 1;
    else
        hexagon = 0;
}

// FUNC 0x002227E0 RaceTirePacket::draw
//   In the current matrix (turned by RaceTireModel::draw): moved by
//   -width / 2 along x, scaled by the rim radius (pgl_set_scale), mirrored
//   in z (and the other cull face: the winding flips) when the wheel file
//   asks for it; this packet receives the shader passes (bindPassCallback).  With the face
//   seen, the wheel face (blurred if asked: drawWheelTexture into faceBuf /
//   faceTex); the tread blur into treadBuf; the tread side (packetOut,
//   colour only).  With the brake and the face seen: the disc (its texture
//   from the brake file, made resident unless linked: g_00352AD0; alpha
//   test reference 0x40) and the caliper (reference 0x7F, alpha blended;
//   the failing pixels still write the frame buffer).  Then the faces
//   (packetIn) with the additive FIX blend of glow (the heat glow strip,
//   shader 0xC, is in it) and, unless the hexagon stands for it, the wheel
//   model with this packet's callback (TireModelBase::drawWheel).  The
//   cull face found is put back.  GSBuffer() / TexSet() and ~TexSet /
//   ~GSBuffer of the locals (flags 2), with their EH cleanups.  Caller:
//   RaceTireModel::draw.
void RaceTirePacket::draw(int blurPasses, f32 spinRad, f32 glow, f32 blur)
{
    RaceTireModel *m = owner;
    pgl_push_matrix();
    gfx_enable(PGL_CAP_FLAG24);
    gfx_enable(PGL_CAP_SCALE);
    pgl_translate(-m->width * 0.5f, 0.0f, 0.0f);
    int mode = gfx_get_cull_face();
    int mirror = wheel_file_mirrored(m->wheelFile, m->position);
    pgl_set_scale(m->rim, m->rim, m->rim);         /* F_00257F40 */
    if (mirror != 0) {
        vu0_scale(1.0f, 1.0f, -1.0f);
        gfx_cull_face(gfx_other_cull_mode(mode));
    }
    bindPassCallback();
    GSBuffer faceBuf;                       /* sp+0x00 */
    TexSet faceTex;                         /* sp+0x40 */
    if (m->faceVisible != 0)
        drawWheelTexture(&faceBuf, &faceTex, blurPasses, blur);
    {
        GSBuffer treadBuf;                  /* sp+0x80 */
        drawTreadBlur(&treadBuf, blurPasses, blur);
        gfx_set_frame_write_mask(GS_WRITE_RGB);
        model_draw(packetOut);
    }
    if (withBrake != 0 && m->faceVisible != 0) {
        if (g_00352AD0 == 0)
            g_00352ACC->loadTexture();
        setDiscVertices(spinRad);
        gfx_set_texture_regs(&g_00352ACC->tex->tex[g_00352ACC->discTex + m->disc]);
        gfx_enable(PGL_CAP_ALPHA_TEST);
        gfx_alpha_func_raw(PGL_ATEST_GREATER, 0x40);
        model_draw(discPacket);
        if (caliperPacket != NULL && m->caliper >= 0) {
            setCaliperVertices(spinRad);
            gfx_set_texture_regs(&g_00352ACC->tex->tex[g_00352ACC->caliperTex + m->caliper]);
            gfx_alpha_func_raw(PGL_ATEST_GREATER, 0x7F);
            gfx_alpha_fail(PGL_AFAIL_FB_ONLY);
            gfx_blend_func(PGL_BLEND_ALPHA, 1.0f);
            model_draw(caliperPacket);
            gfx_alpha_fail(PGL_AFAIL_KEEP);
        }
    }
    gfx_blend_func(PGL_BLEND_ADD_FIX, glow);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    model_draw(packetIn);
    gfx_depth_mask(1);
    gfx_alpha_func_raw(PGL_ATEST_GREATER, 0x20);
    if (m->faceVisible != 0 && hexagon == 0) {
        gfx_set_frame_write_mask(GS_WRITE_RGBA);
        modelset_set_callback(&callback);
        m->drawWheel();
        modelset_set_callback(NULL);
    }
    gfx_cull_face(mode);
    gfx_disable(PGL_CAP_SCALE);
    gfx_disable(PGL_CAP_FLAG24);
    pgl_pop_matrix();
}

// FUNC 0x00222B68 RaceTirePacket::setDiscVertices
//   The four corners of the disc quad, turned by -spinRad (sincosf): x on
//   the disc plane ((discOffset + width / 2) / rim), the corners (+-1, +-1)
//   of the y-z square of half size discRadius / rim (sin and cos scaled),
//   sin negated when flipped (brake_flipped: wheel_file_mirrored and the
//   side).  They go to VU1 address 0
//   (vu1_unpack_xyz), where the disc packet (built without positions) takes
//   them.
void RaceTirePacket::setDiscVertices(f32 spinRad)
{
    RaceTireModel *m = owner;
    int flip = brake_flipped(m);                 /* implicit: wheel_file_mirrored (inline) */
    f32 s, c;                               /* sp+0x00, sp+0x04 */
    sincosf(-spinRad, &s, &c);
    f32 invRim = 1.0f / m->rim;
    f32 scale = invRim * m->discRadius;
    f32 x = (m->discOffset + m->width * 0.5f) * invRim;
    s = s * scale;
    c = c * scale;
    if (flip != 0)
        s = -s;
    f32 *xyz = (f32 *)vu1_unpack_xyz(4);
    /* corner (a, b): y = a sin + b cos, z = a cos - b sin.  (s and c live
     * in memory, sincosf has their addresses: each f32 store may alias them,
     * so they are reloaded for every component, as in retail.) */
    xyz[0] = x;                             /* ( 1,  1) */
    xyz[1] = s + c;
    xyz[2] = c - s;
    xyz[3] = x;                             /* (-1,  1) */
    xyz[4] = -s + c;
    xyz[5] = -c - s;
    xyz[6] = x;                             /* ( 1, -1) */
    xyz[7] = s - c;
    xyz[8] = c + s;
    xyz[9] = x;                             /* (-1, -1) */
    xyz[10] = -s - c;
    xyz[11] = -c + s;
}

// FUNC 0x00222CE0 RaceTirePacket::setCaliperVertices
//   As setDiscVertices for the caliper quad of the brake file (its centre
//   and half size on the disc plane, centreU negated with the sin when
//   flipped: brake_flipped, wheel_file_mirrored), turned by -(spinRad + caliperAngle), 0.005 nearer than the
//   disc.  (Without a caliper it would read through NULL: draw checks it
//   first.)
void RaceTirePacket::setCaliperVertices(f32 spinRad)
{
    RaceTireModel *m = owner;
    int flip = brake_flipped(m);                 /* implicit: wheel_file_mirrored (inline) */
    f32 s, c;                               /* sp+0x00, sp+0x04 */
    sincosf(-(spinRad + m->caliperAngle), &s, &c);
    BrakeCaliper *cal;
    if (m->caliper < 0)
        cal = NULL;
    else
        cal = &g_00352ACC->calipers[m->caliper];
    f32 invRim = 1.0f / m->rim;
    f32 scale = invRim * m->discRadius;
    f32 halfU = cal->halfU;
    f32 halfV = cal->halfV;
    f32 x = ((m->discOffset + m->width * 0.5f) - 0.005f) * invRim;
    s = s * scale;
    c = c * scale;
    f32 centerV = cal->centerV;
    f32 centerU = cal->centerU;
    if (flip != 0) {
        centerU = -centerU;
        s = -s;
    }
    f32 *xyz = (f32 *)vu1_unpack_xyz(4);
    /* corner (a, b): y = a sin + b cos, z = a cos - b sin */
    xyz[0] = x;
    f32 b = centerV + halfV;
    f32 a = centerU + halfU;
    xyz[1] = a * s + b * c;
    xyz[2] = a * c - b * s;
    xyz[3] = x;
    a = centerU - halfU;
    xyz[4] = a * s + b * c;
    xyz[5] = a * c - b * s;
    xyz[6] = x;
    b = centerV - halfV;
    a = centerU + halfU;
    xyz[7] = a * s + b * c;
    xyz[8] = a * c - b * s;
    xyz[9] = x;
    a = centerU - halfU;
    xyz[10] = a * s + b * c;
    xyz[11] = a * c - b * s;
}

// FUNC 0x00222F00 RaceTirePacket::drawTreadBlur
//   Makes the tread texture resident and sends it (tire_file_load_texture,
//   tire_file_send_texture).  When the tyre has a tread blur (flags bit 0;
//   tested for an F1 tyre or a seen face, as needTreadBlur), with
//   blurPasses > 0, blur != 0 and wheel level 0 or 1: renders it into buf
//   (128 x 64, PSMCT32; setDrawBuffer is called twice): once as is, then
//   for each doubling of the passes below blurPasses buf into itself
//   shifted along s by ((int)b & 0x7FF) / 16 texels, blended 50/50; b
//   starts at half the blur angle as 1/16 texels of the 128-texel tread
//   repeat (blur / 2 * texHeight * 1024 / pi; negated on the odd side) and
//   halves every pass.  buf stays the texture for the tread.
void RaceTirePacket::drawTreadBlur(GSBuffer *buf, int blurPasses, f32 blur)
{
    RaceTireModel *m = owner;
    tire_file_load_texture(m->tireFile);
    tire_file_send_texture(m->tireFile);
    int flags = m->tireFile->flags;
    int treadBlur = 0;
    if (((flags >> 1) & 1) != 0 || m->faceVisible != 0)     /* isF1() */
        treadBlur = flags & TIRE_TREAD_BLUR;
    if (treadBlur == 0)
        return;
    if (blurPasses <= 0)
        return;
    if (blur == 0.0f)
        return;
    if (!(callback.lod < RACE_TIRE_HEXAGON))
        return;
    pgl_push_attrib(PGL_ATTRIB_TARGET);
    buf->psm = GS_PSMCT32;
    buf->width = TREAD_BLUR_W;
    blur = blur * 0.5f;
    buf->height = TREAD_BLUR_H;
    buf->allocVram();
    buf->setDrawBuffer();
    buf->setDrawBuffer();
    gfx_disable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_FOG);
    gfx_depth_mask(0);
    /* a statement macro (do { } while (0)): its loop notes make sched1 set
     * the int argument first, as retail (the same with the macro around
     * gfx_depth_mask) */
    do {
        gfx_blend_func(PGL_BLEND_FIX, 0.5f);
    } while (0);
    f32 repeats = (f32)m->tireFile->texHeight;
    blur = blur * (repeats * 325.9493f);    /* 1024 / pi */
    if (m->position & 1)
        blur = -blur;
    int passes = 2;
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gfx_disable(PGL_CAP_BLEND);
    gs_draw_textured_sprite_f(0.0f, 0.0f, 128.0f, 64.0f, 0.0f, 0.0f, 128.0f, 64.0f);
    buf->setTexture();
    gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REPEAT);
    gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_CLAMP);
    while (passes < blurPasses) {
        blur = blur * 0.5f;
        passes <<= 1;
        f32 shift = (f32)((int)blur & 0x7FF) * 0.0625f;
        gfx_enable(PGL_CAP_BLEND);
        gs_draw_textured_sprite_f(0.0f, 0.0f, 128.0f, 64.0f, shift, 0.0f, shift + 128.0f, 64.0f);
    }
    pgl_pop_attrib();
    gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REPEAT);
    gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_CLAMP);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_depth_mask(1);
}

// FUNC 0x002231D0 F_002231D0
//   One pass of the wheel face blur: the current texture's (0, 0)..
//   (texSize, texSize) square, turned by angle about its centre, into the
//   rows size..3 size of the w x 3w blur buffer at double height (x = h +
//   the turned x, y = size + 2 (h + the turned y) + yOffset, h = size / 2).
//   drawWheelTexture draws it still (yOffset 0, every row) and turned
//   (yOffset 1 with the even rows masked), then shrinks the rows back to
//   size x size, which averages the two.  The positions are computed into
//   st and copied to xy, then st gets the texture's corners (draw_poly2d).
//   Caller: drawWheelTexture.
void F_002231D0(f32 angle, f32 size, f32 texSize, f32 yOffset)
{
    /* The four corners as one record (4-byte aligned): its copy is retail's
     * ldl / ldr + sdl / sdr block move (a memcpy of the 8-aligned arrays
     * gave ld / sd). */
    struct Quad2D { f32 v[4][2]; };
    Quad2D xy;                              /* sp+0x00 */
    Quad2D st;                              /* sp+0x20 */
    f32 s, c;                               /* sp+0x40, sp+0x44 */
    f32 half = size * 0.5f;
    sincosf(angle, &s, &c);
    s = s * half;
    c = c * half;
    f32 sum = c + s;
    f32 diff = c - s;
    /* corner (a, b) of the square turned: x = h + a c - b s, y = h + a s +
     * b c (s, c scaled by h); y at double height from row size */
    st.v[0][0] = half - diff;               /* (-1, -1) */
    st.v[0][1] = size + ((half - sum) + (half - sum)) + yOffset;
    st.v[1][0] = half + sum;                /* ( 1, -1) */
    st.v[1][1] = size + ((half - diff) + (half - diff)) + yOffset;
    st.v[2][0] = half + diff;               /* ( 1,  1) */
    st.v[2][1] = size + ((half + sum) + (half + sum)) + yOffset;
    st.v[3][0] = half - sum;                /* (-1,  1) */
    st.v[3][1] = size + ((half + diff) + (half + diff)) + yOffset;
    xy = st;
    st.v[0][0] = 0.0f;
    st.v[0][1] = 0.0f;
    st.v[1][0] = texSize;
    st.v[1][1] = 0.0f;
    st.v[2][0] = texSize;
    st.v[2][1] = texSize;
    st.v[3][0] = 0.0f;
    st.v[3][1] = texSize;
    draw_poly2d(&xy.v[0][0], &st.v[0][0], NULL, 4);
}

// FUNC 0x00223320 RaceTirePacket::drawWheelTexture
//   Picks the wheel face of this level (the wheel model's texture table,
//   or its set wheel_file_lod * 3 + level): the blurred face when
//   blurPasses < 0 and |blur| > 3 degrees (if there is one), else the still
//   one (none: nothing).  Its variation from the wheel file
//   (wheel_file_variation, -1: none; tex1_select_variation).  Without a
//   rendered blur (blurPasses <= 0 or blur == 0): linked into set
//   (TexSet::load) and its first register block copied into the wheel
//   face slot (TEX2_WHEEL_FACE of texsel_get_table2).  With one: renders it
//   into buf (w x 3w, w = its width up to 64) still and turned by blur / 2
//   (F_002231D0; scan mask on the even rows), shrinks that to w x w at the
//   top (setTexture, the sprite), then while the passes double below
//   blurPasses the result the same way with the angle halved; buf's
//   registers (makeTexRegs, w x w: texregs_set_size) go into the slot.
//   The face is made resident (VramAllocator::loadTex1) and sent from its
//   second register block for the renders (? why the second).
void RaceTirePacket::drawWheelTexture(GSBuffer *buf, TexSet *set, int blurPasses, f32 blur)
{
    Tex1Image *face = NULL;
    RaceTireModel *m = owner;
    int texSet = callback.lod;
    texSet = texSet + wheel_file_lod(m->wheelFile, m->position) * RACE_TIRE_LEVELS;
    if (blurPasses < 0 && (0.05235988f < blur || blur < -0.05235988f)) {   /* 3 degrees */
        ModelSet *model = m->wheelFile->model;
        if (model->texSets != NULL)
            face = model->texSets[texSet][WHEEL_TEX_FACE_BLURRED];
        else
            face = model->tex[WHEEL_TEX_FACE_BLURRED];
    }
    if (face == NULL) {
        ModelSet *model = m->wheelFile->model;
        if (model->texSets != NULL)
            face = model->texSets[texSet][WHEEL_TEX_FACE];
        else
            face = model->tex[WHEEL_TEX_FACE];
        if (face == NULL)
            return;
    }
    GsTexRegs *slot = texsel_get_table2() + TEX2_WHEEL_FACE;
    int variation = wheel_file_variation(m->wheelFile, m->color, m->position);
    if (variation >= 0)
        tex1_select_variation(face, variation);
    if (blurPasses <= 0 || blur == 0.0f) {
        set->load(face);
        GsTexRegs *src = face->tex;
        slot->tex0 = src->tex0;
        slot->tex1 = src->tex1;
        slot->miptbp1 = src->miptbp1;
        slot->miptbp2 = src->miptbp2;
        slot->clamp = src->clamp;
        return;
    }
    pgl_push_attrib(PGL_ATTRIB_TARGET);
    buf->psm = GS_PSMCT32;
    f32 angle = blur * 0.5f;
    int texW = 1 << (int)((face->tex->tex0 >> 26) & 0xF);  /* TEX0.TW */
    int w = WHEEL_BLUR_MAX_W;               /* (retail: 64 preset, slti + movn) */
    if (w > texW)
        w = texW;
    buf->width = w;
    buf->height = w * 3;
    buf->allocVram();
    buf->setDrawBuffer();
    g_002F4250.loadTex1(face);
    gfx_set_texture_regs(&face->tex[1]);
    gfx_disable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_depth_mask(0);
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    if (m->position & 1)
        angle = -angle;
    int passes = 2;
    /* (f32)w / (f32)texW at each use, not in variables: retail converts
     * them where used (and w * 3 again), GCSE keeping the copies */
    F_002231D0(0.0f, (f32)w, (f32)texW, 0.0f);
    gs_set_scanmask_mode(PGL_SCANMASK_EVEN);
    F_002231D0(angle, (f32)w, (f32)texW, 1.0f);
    gs_set_scanmask_mode(PGL_SCANMASK_OFF);
    buf->setTexture();
    gs_draw_textured_sprite_f(0.0f, 0.0f, (f32)w, (f32)w, 0.0f, (f32)w + 0.5f, (f32)w, (f32)(w * 3) + 0.5f);
    gfx_texflush();
    while (passes < blurPasses) {
        F_002231D0(0.0f, (f32)w, (f32)w, 0.0f);
        angle = angle * 0.5f;
        passes <<= 1;
        gs_set_scanmask_mode(PGL_SCANMASK_EVEN);
        F_002231D0(angle, (f32)w, (f32)w, 1.0f);
        gs_set_scanmask_mode(PGL_SCANMASK_OFF);
        gs_draw_textured_sprite_f(0.0f, 0.0f, (f32)w, (f32)w, 0.0f, (f32)w + 0.5f, (f32)w, (f32)(w * 3) + 0.5f);
        gfx_texflush();
    }
    pgl_pop_attrib();
    gfx_depth_mask(1);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    buf->makeTexRegs(slot);
    texregs_set_size(&slot->tex0, w, w);
}

// FUNC 0x00223730 RaceTirePacket::virtual_2
//   (slot 2) The shader passes when drawn: TirePacket::virtual_2's first;
//   0xC (the heat glow): the glow texture of the brake file, no depth
//   writes; 0xD (the hexagon): the white material g_00341030 and the wheel
//   face slot (texsel_get_table2, filled by drawWheelTexture).  1 if known.
int RaceTirePacket::virtual_2(int pass)
{
    if (TirePacket::virtual_2(pass) != 0)
        return 1;
    switch (pass) {
    case RACE_SHADER_HEXAGON:
        material_load(&g_00341030);
        gfx_set_texture_regs(texsel_get_table2() + TEX2_WHEEL_FACE);
        return 1;
    case RACE_SHADER_GLOW:
        gfx_set_texture_regs(&g_00352ACC->tex->tex[g_00352ACC->glowTex]);
        gfx_depth_mask(0);
        return 1;
    }
    return 0;
}

// FUNC 0x002237C0 RaceTirePacket::virtual_1
//   (slot 1) Builds TirePacket::virtual_1's packets at buf, then in a
//   TStripBuilder on the stack (begin after them; ~TStripBuilder at the
//   end, with its EH cleanup), with the brake: the disc (shader 0, PRIM
//   0x3C, finish) and, with a caliper, the caliper (PRIM 0x7C).  Returns the
//   end of what was built (the builder's cur: getCurrentAddress inline).
void *RaceTirePacket::virtual_1(void *buf)
{
    RaceTireModel *m = owner;
    TStripBuilder sb(TirePacket::virtual_1(buf));      /* sp+0x00 */
    if (withBrake != 0) {
        sb.setShader(RACE_SHADER_NONE);
        sb.setPrim(PRIM_TRISTRIP | PRIM_IIP | PRIM_TME | PRIM_FGE);
        buildDisc(&sb);
        discPacket = sb.finish(STRIP_LIT, STRIP_SCALE_NONE);
        if (withBrake != 0 && m->caliper >= 0) {
            sb.setShader(RACE_SHADER_NONE);
            sb.setPrim(PRIM_TRISTRIP | PRIM_IIP | PRIM_TME | PRIM_FGE | PRIM_ABE);
            buildCaliper(&sb);
            caliperPacket = sb.finish(STRIP_LIT, STRIP_SCALE_NONE);
        }
    }
    return sb.cur;
}

// FUNC 0x002238C0 RaceTirePacket::virtual_3
//   (slot 3) TirePacket::virtual_3's tread side, then at level 2 the
//   hexagon in place of the wheel model (shader 0xD, PRIM 0x3C).
void RaceTirePacket::virtual_3(TStripBuilder *sb, TireCircle *c)
{
    TirePacket::virtual_3(sb, c);
    if (hexagon != 0) {
        sb->setShader(RACE_SHADER_HEXAGON);
        sb->setPrim(PRIM_TRISTRIP | PRIM_IIP | PRIM_TME | PRIM_FGE);
        buildHexagon(sb);
    }
}

// FUNC 0x00223920 RaceTirePacket::virtual_4
//   (slot 4) TirePacket::virtual_4's faces, then with the brake and a heat
//   brake disc the glow (shader 0xC, PRIM 0x5C: alpha blended, no fog).
void RaceTirePacket::virtual_4(TStripBuilder *sb, TireCircle *c)
{
    RaceTireModel *m = owner;
    TirePacket::virtual_4(sb, c);
    if (withBrake != 0 && (g_00352ACC->discs[m->disc].flags & BRAKE_DISC_HEAT)) {
        sb->setShader(RACE_SHADER_GLOW);
        sb->setPrim(PRIM_TRISTRIP | PRIM_IIP | PRIM_TME | PRIM_ABE);
        buildGlow(sb);
    }
}

/* The hexagon in the y-z plane (radius 1.1547, the corners at +-60 degrees
 * a little above 1 in z, as the original's floats): T449's first .rodata
 * item, so external linkage (tire_model_internal.h's extern): egcs writes it where it is
 * defined, before the vtables (a file static would come after the type
 * names, with the tables above).  Defined after the file's first function:
 * the static initialiser keeps its name (relocate__9BrakeFile). */
const f32 g_00340ED0[6][3] = {              /* 0x00340ED0 */
    { 0.0f, -0.577350497f,  1.00000036f },
    { 0.0f,  0.577350497f,  1.00000036f },
    { 0.0f, -1.15470099f,   0.0f },
    { 0.0f,  1.15470099f,   0.0f },
    { 0.0f, -0.577350497f, -1.00000036f },
    { 0.0f,  0.577350497f, -1.00000036f },
};

// FUNC 0x002239A8 RaceTirePacket::buildHexagon
//   One strip of the six vertices of g_00340ED0 (closing the open strip
//   first: endStrip), normal g_003410B0, uv = (z / 2 + 0.5, 0.5 - y / 2).
//   (The this pointer is not used.)
void RaceTirePacket::buildHexagon(TStripBuilder *sb)
{
    f32 uv[2];                              /* sp+0x00 */
    sb->endStrip();
    /* The position as a running pointer, the uv read by index: retail's
     * two pointers (the vertex, and a reduced giv at the y component) and
     * the counter reversed (bgezl).  With g_00340ED0[i] as the argument,
     * loop.c eliminates the counter through that giv (slt / bnel). */
    const f32 *pos = g_00340ED0[0];
    for (int i = 0; i < 6; i++) {
        uv[0] = g_00340ED0[i][2] * 0.5f + 0.5f;
        uv[1] = 0.5f - g_00340ED0[i][1] * 0.5f;
        sb->addVertex(pos, g_003410B0, uv, NULL);
        pos += 3;
    }
    sb->endStrip();
}

// FUNC 0x00223A70 RaceTirePacket::buildDisc
//   One strip of four vertices without positions (setDiscVertices sends
//   them), normal g_003410B0, uv from the square g_00341080: (z / 2 + 0.5
//   (0.5 - z / 2 when flipped: brake_flipped, wheel_file_mirrored),
//   0.5 - y / 2).
void RaceTirePacket::buildDisc(TStripBuilder *sb)
{
    RaceTireModel *m = owner;
    int flip = brake_flipped(m);                 /* implicit: wheel_file_mirrored (inline) */
    f32 uv[2];                              /* sp+0x00 */
    sb->endStrip();
    /* g_00341080[i][k] at each use: retail's one address giv at the y
     * component (y at 0, z at +4) and the counter reversed (bgez). */
    for (int i = 0; i < 4; i++) {
        if (flip != 0)
            uv[0] = 0.5f - g_00341080[i][2] * 0.5f;
        else
            uv[0] = g_00341080[i][2] * 0.5f + 0.5f;
        uv[1] = 0.5f - g_00341080[i][1] * 0.5f;
        sb->addVertex(NULL, g_003410B0, uv, NULL);
    }
    sb->endStrip();
}

// FUNC 0x00223B70 RaceTirePacket::buildCaliper
//   As buildDisc with the uv axes swapped: (0.5 - y / 2, 0.5 - z / 2 (z / 2
//   + 0.5 when flipped: brake_flipped, wheel_file_mirrored)).
void RaceTirePacket::buildCaliper(TStripBuilder *sb)
{
    RaceTireModel *m = owner;
    int flip = brake_flipped(m);                 /* implicit: wheel_file_mirrored (inline) */
    f32 uv[2];                              /* sp+0x00 */
    sb->endStrip();
    for (int i = 0; i < 4; i++) {
        if (flip != 0)
            uv[1] = g_00341080[i][2] * 0.5f + 0.5f;
        else
            uv[1] = 0.5f - g_00341080[i][2] * 0.5f;
        uv[0] = 0.5f - g_00341080[i][1] * 0.5f;
        sb->addVertex(NULL, g_003410B0, uv, NULL);
    }
    sb->endStrip();
}

// FUNC 0x00223C70 RaceTirePacket::buildGlow
//   One strip of four vertices: the square g_00341080 scaled to the disc
//   (discRadius / rim) on a plane 0.001 nearer than the disc, no normal,
//   colour 0x80808080, uv = (z / 2 + 0.5, 0.5 - y / 2).
void RaceTirePacket::buildGlow(TStripBuilder *sb)
{
    RaceTireModel *m = owner;
    f32 rim = m->rim;
    f32 scale = m->discRadius / rim;
    f32 x = ((m->discOffset + m->width * 0.5f) - 0.001f) / rim;
    f32 pos[3];                             /* sp+0x00 */
    f32 uv[2];                              /* sp+0x10 */
    u32 rgba;                               /* sp+0x20 */
    sb->endStrip();
    for (int i = 0; i < 4; i++) {
        pos[0] = x;
        pos[1] = g_00341080[i][1] * scale;
        pos[2] = g_00341080[i][2] * scale;
        uv[0] = g_00341080[i][2] * 0.5f + 0.5f;
        uv[1] = 0.5f - g_00341080[i][1] * 0.5f;
        rgba = 0x80808080;
        sb->addVertex(pos, NULL, uv, &rgba);
    }
    sb->endStrip();
}

// FUNC 0x00223DB0 RaceTirePacket::Callback::Callback
//   (out of line) The vptr 0x00340F68, lod = 0.
RaceTirePacket::Callback::Callback()
{
    lod = 0;
}

// FUNC 0x00223DC8 RaceTirePacket::Callback::~Callback
//   (slot 0) The vptr, then ModelSet::Callback::~Callback with the flags
//   (the deleting form's operator delete is the base's).
RaceTirePacket::Callback::~Callback()
{
}

// FUNC 0x00223DF0 RaceTirePacket::Callback::selectSwitch
//   (slot 1) Switch 0 always, as ModelSet::Callback's.
int RaceTirePacket::Callback::selectSwitch(int id)
{
    return 0;
}

// FUNC 0x00223DF8 RaceTirePacket::Callback::selectLOD
//   (slot 2) The wheel level of the packet, whatever the size.  index.txt
//   names it TStripBuilder::getCurrentAddress(void) [lupo2m]: the same two
//   instructions (lw v0, 4(a0)), but the address is slot 2 of this class's
//   vtable (data xref 0x00340F84).
int RaceTirePacket::Callback::selectLOD(f32 size)
{
    return lod;
}

/* RaceTireModel's eight members below were inline in the original: egcs
 * writes them in this file (the class's key function, ~RaceTireModel, is
 * here), unused, at the end of the file: after the static destructor /
 * initialiser, before __tf13RaceTireModel (retail 0x00223E48..0x00223F90;
 * the frame info's FDE order shows it).  They are defined out of class
 * (`inline'), where the brake file and the graphics library are declared:
 * egcs writes out-of-class inline members first in the end-of-file group,
 * in reverse definition order, so they are defined last first.  (In-class
 * bodies in reverse declaration order would give the same order: the class
 * is the last one defined.) */

// FUNC 0x00223F60 RaceTireModel::needTreadBlur
//   ? name.  No callers.  The test of drawTreadBlur: flags bit 0 of the
//   tyre file, for an F1 tyre or a seen face.
inline int RaceTireModel::needTreadBlur()
{
    int flags = tireFile->flags;
    int treadBlur = 0;
    if (((flags >> 1) & 1) != 0 || faceVisible != 0)        /* isF1() */
        treadBlur = flags & TIRE_TREAD_BLUR;
    return treadBlur;
}

// FUNC 0x00223F58 RaceTireModel::needWheelMotionBlur
//   No callers.
inline int RaceTireModel::needWheelMotionBlur()
{
    return faceVisible;
}

// FUNC 0x00223F30 RaceTireModel::getCaliper
//   ? name.  No callers.  The caliper record, NULL without one.
inline BrakeCaliper *RaceTireModel::getCaliper()
{
    if (caliper < 0)
        return NULL;
    return &g_00352ACC->calipers[caliper];
}

// FUNC 0x00223EF0 RaceTireModel::setDiscTexture
//   ? name.  No callers.  Sends the texture of this disc.
inline void RaceTireModel::setDiscTexture()
{
    gfx_set_texture_regs(&g_00352ACC->tex->tex[g_00352ACC->discTex + disc]);
}

// FUNC 0x00223EB0 RaceTireModel::setCaliperTexture
//   ? name.  No callers.  Sends the texture of this caliper.
inline void RaceTireModel::setCaliperTexture()
{
    gfx_set_texture_regs(&g_00352ACC->tex->tex[g_00352ACC->caliperTex + caliper]);
}

// FUNC 0x00223E78 RaceTireModel::setGlowTexture
//   ? name.  No callers; does not use this.  Sends the glow texture of the
//   brake file (as virtual_2's shader 0xC).
inline void RaceTireModel::setGlowTexture()
{
    gfx_set_texture_regs(&g_00352ACC->tex->tex[g_00352ACC->glowTex]);
}

// FUNC 0x00223E68 RaceTireModel::existCaliper
//   No callers.  (Compiled as ~caliper >> 31, unsigned.)
inline int RaceTireModel::existCaliper()
{
    return caliper >= 0;
}

// FUNC 0x00223E48 RaceTireModel::isHeatBrake
//   No callers.
inline int RaceTireModel::isHeatBrake()
{
    return g_00352ACC->discs[disc].flags & BRAKE_DISC_HEAT;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __si_type_info
 * __ti13RaceTireModel (0x004B29D0, name 0x00340F88) on the first call,
 * after __tf13TireModelBase (base __ti13TireModelBase, 0x00353440). */
// FUNC 0x00223F90 __tf13RaceTireModel
extern "C" void *__tf13RaceTireModel(void)
{
    if (__ti13RaceTireModel[0] == 0) {
        __tf13TireModelBase();
        __rtti_si(__ti13RaceTireModel, "13RaceTireModel", (const type_info *)__ti13TireModelBase);
    }
    return __ti13RaceTireModel;
}
#endif

// FUNC 0x00223FE0 RaceTirePacket::getWheelLOD
//   No callers.
/* inline in game/car/model/T449_race_tire_model.h; egcs emits this copy */

// FUNC 0x00223FE8 RaceTirePacket::setWheelLOD
//   No callers.
/* inline in game/car/model/T449_race_tire_model.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: __ti14RaceTirePacket (0x004B29E0,
 * name 0x00340F98), base __ti10TirePacket (0x00353448, after
 * __tf10TirePacket). */
// FUNC 0x00223FF0 __tf14RaceTirePacket
extern "C" void *__tf14RaceTirePacket(void)
{
    if (__ti14RaceTirePacket[0] == 0) {
        __tf10TirePacket();
        __rtti_si(__ti14RaceTirePacket, "14RaceTirePacket", (const type_info *)__ti10TirePacket);
    }
    return __ti14RaceTirePacket;
}

/* Compiler-generated typeinfo function: __tiQ214RaceTirePacket8Callback
 * (0x004B29C0, name 0x00340FB0), base __tiQ28ModelSet8Callback (0x00353458,
 * after __tfQ28ModelSet8Callback). */
// FUNC 0x00224040 __tfQ214RaceTirePacket8Callback
extern "C" void *__tfQ214RaceTirePacket8Callback(void)
{
    if (__tiQ214RaceTirePacket8Callback[0] == 0) {
        __tfQ28ModelSet8Callback();
        __rtti_si(__tiQ214RaceTirePacket8Callback, "Q214RaceTirePacket8Callback",
                  (const type_info *)__tiQ28ModelSet8Callback);
    }
    return __tiQ214RaceTirePacket8Callback;
}
#endif

/* Compiler-generated: egcs's static destructor of the brake textures (its
 * initialiser: see g_0049D160 above). */
// FUNC 0x00223E00 T449_static_dtor (_GLOBAL_$D$relocate__9BrakeFile)
//   The .dtors entry (0x0028F248): ~TexSet of g_0049D160 (flags 2).
/* implicit: ~TexSet (the static destructor of g_0049D160) */
