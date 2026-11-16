/* T446 - CarModel: the drawn body of a car.  0x0021E7D8..0x0021F3F0.
 *
 * The car list: the directory "cars/day" of the volume's TOC holds one file
 * per car model; car_list_load turns it into (code, name) pairs, code =
 * nameToCode(name), sorted by code for car_list_find (a binary search).
 * The model files are "cars/day/<name>", "cars/eve/", "cars/night/" and
 * "menu/cars/" (kinds 0, 3, 1, 2).
 *
 * CarModel holds a ModelSet (T451) and a ModelSet::Callback that T451 asks
 * for the model's switches (selectSwitch: the lamps, and the lamp
 * brightnesses as morph positions) and the LOD for a projected size
 * (selectLOD).  The parts drawn: draw (part 0, the body, colour variation
 * `color'), drawShadow (part 1 squashed onto the ground plane along
 * g_0049D140), and the parts 4..7, some with the ground-pass matrices. */
#include "game/car/model/car_model_internal.h"
#include "game/enhanced.h"                  /* ENH_RENDER_CAR_ANIMS */

/* ------------------------------------------------------------------------ */
/* The car list                                                             */
/* ------------------------------------------------------------------------ */

/* qsort comparison of two entries by their 64-bit code (unsigned). */
// FUNC 0x0021E7D8 car_list_compare
int car_list_compare(const void *a, const void *b)
{
    u64 codeA = *(const u64 *)a;
    u64 codeB = *(const u64 *)b;
    if (codeA < codeB)
        return -1;
    return codeB < codeA;
}

/* The car list (T446 .sdata, zero until car_list_load). */
int           g_00352AB8 = 0;               /* 0x00352AB8 the number of entries */
CarListEntry *g_00352ABC = NULL;            /* 0x00352ABC the entries (realloc'd) */

/* A word of a TOC image (retail adds the record offset to the image last:
 * image + (off + 4), image + (off + i * 4 + 0xC); as u059's course table,
 * the twin of this list). */
static inline u32 toc_word(const u8 *image, u32 off)
{
    return *(const u32 *)(image + off);
}

/* Reads the directory "cars/day" of the volume: its record holds the entry
 * count + 1 at +4 and the entries' record offsets from +0xC; an entry
 * record starts with its name offset (low 24 bits). */
// FUNC 0x0021E800 car_list_load
void car_list_load(void)
{
    VolDir dir;                             /* sp+0x00 */
    VolDevice *dev = &vol_device_object;    /* retail adds the toc offset to the
                                               address in a register */
    vol_toc_lookup(&dir, &dev->toc, "cars/day");
    u8 *image = dir.toc->data;
    int count = toc_word(image, dir.off + 4) - 1;
    g_00352AB8 = count;
    g_00352ABC = (CarListEntry *)realloc(g_00352ABC, count * sizeof(CarListEntry));
    for (int i = 0; i < g_00352AB8; i++) {
        u8 *data = dir.toc->data;
        CarListEntry *entry = &g_00352ABC[i];
        VolDir child;                       /* sp+0x10: the entry's VolDir, built and never used */
        child.toc = dir.toc;
        child.off = toc_word(data, dir.off + i * 4 + 0xC);
        const char *name = (const char *)(data + (toc_word(data, child.off) & 0xFFFFFF));
        entry->name = name;
        entry->code = nameToCode(name);
    }
    qsort(g_00352ABC, g_00352AB8, sizeof(CarListEntry), car_list_compare);
}

/* The name of a list file, "cars/.list": retail's .rodata has the string
 * (0x00340BD8) between car_list_load's and car_model_path's, and no code uses
 * it - an inline nothing calls, whose string egcs still writes. */
static inline const char *car_list_file(void)
{
    return "cars/.list";
}

/* The index of `code' in the list; 0 when it is not there. */
// FUNC 0x0021E910 car_list_find
int car_list_find(u64 code)
{
    int lo = 0;
    int hi = g_00352AB8;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        CarListEntry *e = &g_00352ABC[mid];
        if (code == e->code)
            return mid;
        if (code < e->code)
            hi = mid;
        else
            lo = mid + 1;
    }
    return 0;
}

// FUNC 0x0021E968 car_list_name
const char *car_list_name(u64 code)
{
    return g_00352ABC[car_list_find(code)].name;
}

/* kind 0: cars/day/, 1: cars/night/, 2: menu/cars/, 3: cars/eve/ */
// FUNC 0x0021E998 car_model_path
char *car_model_path(u64 code, char *path, int kind)
{
    const char *dir = "cars/day/";
    switch (kind) {
    case 3:
        dir = "cars/eve/";
        break;
    case 1:
        dir = "cars/night/";
        break;
    case 2:
        dir = "menu/cars/";
        break;
    }
    strcpy(path, dir);
    strcat(path, car_list_name(code));
    return path;
}

/* Reads the day (or menu) model file of `code' into buf (0xA8000 bytes at
 * most, unpacked); file_read_blocking's result. */
// FUNC 0x0021EA38 car_model_load
void *car_model_load(u64 code, void *buf, int menu)
{
    char path[64];                          /* sp+0x00 */
    car_model_path(code, path, menu ? 2 : 0);
    return file_read_blocking(path, buf, 0xA8000, 1, &vol_device_object);
}

/* ------------------------------------------------------------------------ */
/* CarModel                                                                 */
/* ------------------------------------------------------------------------ */

/* Takes the model (relocating it first if asked); once relocated (its base
 * is itself) and if it has images, colorCount = the largest number of
 * colour variations of its images. */
// FUNC 0x0021EA90 CarModel::setModel
void CarModel::setModel(ModelSet *m, int relocate)
{
    this->model = m;
    if (relocate)
        modelset_relocate(m);
    ModelSet *set = this->model;
    if (set != set->base)
        return;
    if (set->tex == NULL)
        return;
    if (set->tex[0] == NULL)
        return;
    int i = 0;
    int most = 0;
    /* `i < set->nTex' gives retail's allocation (most in s0, i in s1);
     * `set->nTex != 0' swaps the two, and a for / while loop schedules the
     * body differently */
    if (i < set->nTex) {
        do {
            int n = tex1_variation_count(this->model->tex[i]);  /* compared signed */
            if (most < n)
                most = n;
            i++;
        } while (i < this->model->nTex);
    }
    this->colorCount = most;
}

// FUNC 0x0021EB48 CarModel::clearModel
void CarModel::clearModel()
{
    this->model = NULL;
}

// FUNC 0x0021EB50 CarModel::CarModel
CarModel::CarModel()
{
    /* implicit: Callback (compiler-generated: bases, members, deleting form) */
    /* callback: Callback() (0x0021EC08); destroyed again (~Callback, flags
     * 0) if this throws */
    this->model = NULL;
    this->color = 0;
    this->colorCount = 0;
    this->lamps.fill(0);
    /* The enhanced engine: Lupo 2's render inputs with Lupo 2's defaults
     * (its Callback constructor, L2 0x0022D900: +0x24 0, +0x28 / +0x2C 0.5,
     * +0x30 .. +0x38 0, +0x3C -1). */
    this->wing = 0.0f;
    this->podLean = 0.5f;
    this->podBob = 0.5f;
    this->podColor = 0;
    this->podColorBlend = 0.0f;
    this->podState = 0;
    this->animSelect = -1;
}

// FUNC 0x0021EBC0 CarModel::~CarModel
CarModel::~CarModel()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* callback.~Callback() (flags 2); the deleting form (flags & 1) then
     * calls operator delete(this) */
}

// FUNC 0x0021EC08 CarModel::Callback::Callback
CarModel::Callback::Callback()
{
    this->lod = -1;
    this->lodDist = 220.0f;
    this->brakeLamps = 0;
    for (int i = 3; i >= 0; i--)
        this->wheelShake[i] = 0.0f;
    this->steer = 0.5f;
}

/* Sets its vptr and calls ModelSet::Callback::~Callback with the same
 * flags (that one does the operator delete). */
// FUNC 0x0021EC68 CarModel::Callback::~Callback
CarModel::Callback::~Callback()
{
}

/* The model's switch `what' (command 0x06): 0 -> the lamps' state;
 * 1..4 -> wheelShake[what - 1] as the morph position (command 0x35), branch 0;
 * 5 -> steer as the morph position, branch 0; others branch 0.
 * The enhanced engine (rendering.md 2, Lupo 2 L2 0x0022D9E8, its 11-entry
 * jump table): ids 6..10 from the CarModel's render inputs.  The callback is
 * the CarModel's first member (+0x00), so `this' is the CarModel too (Lupo 2
 * keeps the inputs in its Callback).  GT3's car models never ask them. */
// FUNC 0x0021EC90 CarModel::Callback::selectSwitch
int CarModel::Callback::selectSwitch(int what)
{
    CarModel *owner = (CarModel *)this;

    switch (what) {
    case 0:
        return this->brakeLamps;
    case 1:
    case 2:
    case 3:
    case 4:
        modelset_set_morph(this->wheelShake[what - 1]);
        break;
    case 5:
        modelset_set_morph(this->steer);
        break;
    /* ---- Lupo 2 (L2 0x0022DA40..0x0022DA8C) */
    case 6:                                 /* the moving wing (kinds 2 / 8 / 9 / 10) */
        modelset_set_morph(owner->wing);
        break;
    case 7:                                 /* the Pod's tail: the lean while it leans, else the bob */
        if (owner->podLean != 0.5f) {
            modelset_set_morph(owner->podLean);
            break;
        }
        modelset_set_morph(owner->podBob);
        return 1;
    case 8:                                 /* the Pod's colour category, cross-faded */
        modelset_set_morph(owner->podColorBlend);
        return owner->podColor;
    case 9:                                 /* the Pod's state */
        return owner->podState;
    case 10:                                /* the Pod's face (the CLUT animation id) */
        return owner->animSelect;
    }
    return 0;
}

/* The LOD (command 0x07): the fixed one, else 0 beyond lodDist, 1 beyond
 * 0.33 lodDist, else 2. */
// FUNC 0x0021ED08 CarModel::Callback::selectLOD
int CarModel::Callback::selectLOD(f32 dist)
{
    int level = this->lod;
    if (level < 0) {
        if (this->lodDist < dist)
            level = 0;
        else if (this->lodDist * 0.33f < dist)
            level = 1;
        else
            level = 2;
    }
    return level;
}

// FUNC 0x0021ED50 CarModelLamps::fill
void CarModelLamps::fill(int on)
{
    for (int i = 0; i < 4; i++)             /* reversed by loop.c (check_dbra_loop) */
        this->v[i] = on;
}

// FUNC 0x0021ED80 CarModelLamps::push
void CarModelLamps::push(int on)
{
    for (int i = 0; i < 3; i++)
        this->v[i] = (u8)this->v[i + 1];
    this->v[3] = on;
}

// FUNC 0x0021EDB0 CarModelLamps::fraction
f32 CarModelLamps::fraction()
{
    f32 lit = 0.0f;
    for (int i = 0; i < 4; i++) {
        if (this->v[i] != 0)
            lit += 1.0f;
    }
    return lit * 0.25f;
}

// FUNC 0x0021EDF8 CarModel::pushLamps
void CarModel::pushLamps(int on)
{
    this->callback.brakeLamps = on;
    this->lamps.push(on);
}

// FUNC 0x0021EE20 CarModel::setLamps
void CarModel::setLamps(int on)
{
    this->callback.brakeLamps = on;
    this->lamps.fill(on);
}

// FUNC 0x0021EE48 CarModel::setWheelShake
void CarModel::setWheelShake(u32 wheel, f32 v)
{
    if (wheel < 4)
        this->callback.wheelShake[wheel] = v;
}

// FUNC 0x0021EE68 CarModel::setSteer
void CarModel::setSteer(f32 v)
{
    this->callback.steer = v;
}

// FUNC 0x0021EE70 CarModel::setDirt
void CarModel::setDirt(f32 v)
{
    this->dirt = v;
}

/* The environment-map colours (lighting mode 2) scaled by 1 - dirt. */
// FUNC 0x0021EE78 CarModel::beginFade
void CarModel::beginFade()
{
    pgl_set_scalar_param(PGL_PARAM_ENVMAP_SCALE, 1.0f - this->dirt);
}

// FUNC 0x0021EEA8 CarModel::endFade
void CarModel::endFade()
{
    pgl_set_scalar_param(PGL_PARAM_ENVMAP_SCALE, 1.0f);
}

/* Draws the body (part 0) in colour `color' (modulo the model's colour
 * count) with the material scales of the flagged materials on: ambient /
 * diffuse and specular 1, the emission the lamps' fraction (? the lamp
 * materials glow while the lamps are on). */
// FUNC 0x0021EED0 CarModel::draw
void CarModel::draw()
{
    ModelSet *m = this->model;
    if (m == NULL)
        return;
    int count = this->colorCount;
    int c = this->color;
    if (c >= count) {
        if (count == 0)
            c = 0;
        else
            c = c % count;
    }
    modelset_select_tex_frame(m, c);
    modelset_select_material_set(m, c);
    modelset_set_callback(&this->callback);
    gs_tex_override_enable(GS_OVR_MAT_SCALE);
    mat_scale_set_grey(PGL_MAT_AMBIENT_AND_DIFFUSE, 1.0f);
    mat_scale_set_grey(PGL_MAT_SPECULAR, 1.0f);
    mat_scale_set_grey(PGL_MAT_EMISSION, this->lamps.fraction());
    modelset_draw_part(this->model, 0);
    gs_tex_override_disable(GS_OVR_MAT_SCALE);
    modelset_set_callback(NULL);
}

// FUNC 0x0021EFC0 CarModel::F_0021EFC0
void CarModel::F_0021EFC0()
{
}

/* Part 7. */
// FUNC 0x0021EFC8 CarModel::F_0021EFC8
void CarModel::F_0021EFC8()
{
    if (this->model == NULL)
        return;
    modelset_set_callback(&this->callback);
    modelset_draw_part(this->model, 7);
    modelset_set_callback(NULL);
}

/* Part 4. */
// FUNC 0x0021F010 CarModel::F_0021F010
void CarModel::F_0021F010()
{
    if (this->model == NULL)
        return;
    modelset_set_callback(&this->callback);
    modelset_draw_part(this->model, 4);
    modelset_set_callback(NULL);
}

/* T446 .sdata after the car list: a word nothing in the program reads or
 * writes (no code or data reference to 0x00352AC0), then the shadow
 * strength (game/car/model/T446_car_model.h). */
int g_00352AC0 = 0;                         /* 0x00352AC0 */
f32 g_00352AC4 = 0.0f;                      /* 0x00352AC4 */

/* The car shadow (T446 .bss, file statics): its direction in the space of
 * the current matrix when car_shadow_set_dir ran, and its colour. */
static f32 g_0049D140[4] __attribute__((aligned(16)));     /* 0x0049D140 (x, 0, z, 0) through the matrix */
static f32 g_0049D150[4] __attribute__((aligned(16)));     /* 0x0049D150 r, g, b, a */

/* The shadow direction (x, 0, z) through the current matrix.  y is not
 * used. */
// FUNC 0x0021F058 car_shadow_set_dir
void car_shadow_set_dir(f32 x, f32 y, f32 z)
{
    f32 dir[4];                             /* sp+0x00 */
    dir[0] = x;
    dir[2] = z;
    dir[1] = 0.0f;
    dir[3] = 0.0f;
    vu0_apply_matrix(g_0049D140, dir);
}

// FUNC 0x0021F090 car_shadow_set_color
void car_shadow_set_color(f32 r, f32 g, f32 b, f32 a)
{
    g_0049D150[0] = r;
    g_0049D150[1] = g;
    g_0049D150[2] = b;
    g_0049D150[3] = a;
}

/* Both matrix stacks pushed, the depth range moved by 0.005 and the model
 * lifted by 0.02 (the parts drawn on the ground). */
// FUNC 0x0021F0B0 CarModel::beginGroundPass
void CarModel::beginGroundPass()
{
    modelset_set_callback(&this->callback);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    proj_shift_depth(0.005f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    pgl_translate(0.0f, 0.02f, 0.0f);
}

// FUNC 0x0021F108 CarModel::endGroundPass
void CarModel::endGroundPass()
{
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_pop_matrix();
    modelset_set_callback(NULL);
}

/* The shadow: the shadow direction into model space (the inverse of the
 * current matrix), then part 1 drawn through the projection onto y = 0
 * along it (the matrix takes y to (d.x, 0, d.z): x += y d.x, z += y d.z,
 * y = 0), in the shadow colour with alpha g_00352AC4 * colour.a (the
 * colour and vector of lighting mode 5: GsContext.constColor / constVector). */
// FUNC 0x0021F148 CarModel::drawShadow
void CarModel::drawShadow()
{
    if (this->model == NULL)
        return;
    f32 dir[4];                             /* sp+0x00 */
    f32 squash[4][4];                       /* sp+0x10 */
    pgl_push_matrix();
    vu0_invert_rigid();
    vu0_apply_matrix(dir, g_0049D140);
    pgl_pop_matrix();
    squash[1][0] = dir[0];
    squash[1][2] = dir[2];
    squash[0][0] = 1.0f;
    squash[2][0] = 0.0f;
    squash[3][0] = 0.0f;
    squash[0][1] = 0.0f;
    squash[1][1] = 0.0f;
    squash[2][1] = 0.0f;
    squash[3][1] = 0.0f;
    squash[0][2] = 0.0f;
    squash[2][2] = 1.0f;
    squash[3][2] = 0.0f;
    squash[0][3] = 0.0f;
    squash[1][3] = 0.0f;
    squash[2][3] = 0.0f;
    squash[3][3] = 1.0f;
    beginGroundPass();
    pgl_mult_matrix(&squash[0][0]);
    pgl_set_const_color(g_0049D150[0], g_0049D150[1], g_0049D150[2], 0.0f);    /* constColor */
    pgl_set_const_vector(1.0f, 1.0f, 1.0f, g_00352AC4 * g_0049D150[3]);         /* constVector */
    modelset_draw_part(this->model, 1);
    endGroundPass();
}

/* Part 6 on the ground in the shadow colour (its own alpha). */
// FUNC 0x0021F258 CarModel::F_0021F258
void CarModel::F_0021F258()
{
    if (this->model == NULL)
        return;
    beginGroundPass();
    pgl_set_const_color(g_0049D150[0], g_0049D150[1], g_0049D150[2], 0.0f);
    pgl_set_const_vector(1.0f, 1.0f, 1.0f, g_0049D150[3]);
    modelset_draw_part(this->model, 6);
    endGroundPass();
}

/* Part 5 on the ground. */
// FUNC 0x0021F2E0 CarModel::F_0021F2E0
void CarModel::F_0021F2E0()
{
    if (this->model == NULL)
        return;
    beginGroundPass();
    modelset_draw_part(this->model, 5);
    endGroundPass();
}

/* Part 6 in black, in the current matrices. */
// FUNC 0x0021F328 CarModel::F_0021F328
void CarModel::F_0021F328()
{
    if (this->model == NULL)
        return;
    modelset_set_callback(&this->callback);
    pgl_set_const_color(0.0f, 0.0f, 0.0f, 0.0f);
    pgl_set_const_vector(1.0f, 1.0f, 1.0f, 1.0f);
    modelset_draw_part(this->model, 6);
    modelset_set_callback(NULL);
}

/* ------------------------------------------------------------------------ */
/* The enhanced engine: Lupo 2's model animation (rendering.md 1, 2)        */
/* ------------------------------------------------------------------------ */

/* Lupo 2's step of a car model's animations: 1/60 s a frame (gp-0x6C88),
 * whatever the frame rate. */
#define CAR_MODEL_ANIM_STEP (1.0f / 60.0f)

/* The enhanced engine (new, Lupo 2 L2 0x0022DAE8; rendering.md 1).  Steps
 * the model's animations by 1/60 s: the CLUT records of animSelect (every
 * record with -1) and, with -1, the model's animation tracks
 * (modelset_step_anims).  GT3 never animates a car model; Lupo 2 does it for
 * every car and for the menu car.  With ENH_RENDER_CAR_ANIMS 0 only the
 * CLUT records step: GT3's car models have none, but la0008 has a live
 * track.  Callers: RaceCarModel::updateBodyState (T180), gt2k_menu::Car::
 * virtual_1 (T212). */
void CarModel::animate()
{
    ModelSet *m = this->model;

    if (m == NULL)
        return;
#if ENH_RENDER_CAR_ANIMS
    modelset_step_anims(m, CAR_MODEL_ANIM_STEP, this->animSelect);
#else
    modelset_step_clut_anims(m, CAR_MODEL_ANIM_STEP, this->animSelect);
#endif
}

/* The enhanced engine (new, Lupo 2 L2 0x0022DB18, its CarModel::reset;
 * rendering.md 1).  The dirty CLUT records written into the palettes
 * (modelset_apply_clut_anims).  Callers: RaceCarModel::applyAnims (T180,
 * from RacePS2Base::virtual_3, race/core), gt2k_menu::Car::virtual_1. */
void CarModel::applyAnims()
{
    ModelSet *m = this->model;

    if (m == NULL)
        return;
    modelset_apply_clut_anims(m);
}

/* The enhanced engine (new, Lupo 2 L2 0x0022DC90; rendering.md 1, 5).  A new
 * animation id rewinds that id's CLUT records (modelset_reset_anims with the
 * id: marked dirty, not applied; -1 resets every record and the tracks, and
 * applies) and is kept (SWITCH 10, the next animate).  The same id does
 * nothing.  modelset_reset_anims returns at once without a model.  Callers:
 * RaceCarModel::updateBodyState (the Pod's face), gt2k_menu::Car::
 * virtual_1. */
void CarModel::setAnimSelect(int id)
{
    if (this->animSelect != id)
        modelset_reset_anims(this->model, id);
    this->animSelect = id;
}

/* The enhanced engine (new, Lupo 2 L2 0x0022DC60): SWITCH 6's morph. */
void CarModel::setWing(f32 v)
{
    this->wing = v;
}

/* The enhanced engine (new, Lupo 2 L2 0x0022DC68): SWITCH 7's two morphs. */
void CarModel::setPodTail(f32 lean, f32 bob)
{
    this->podBob = bob;
    this->podLean = lean;
}

/* The enhanced engine (new, Lupo 2 L2 0x0022DC78): SWITCH 8's branch and
 * morph. */
void CarModel::setPodColor(int color, f32 blend)
{
    this->podColorBlend = blend;
    this->podColor = color;
}

/* The enhanced engine (new, Lupo 2 L2 0x0022DC88): SWITCH 9's branch. */
void CarModel::setPodState(int state)
{
    this->podState = state;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __si_type_info
 * __tiQ28CarModel8Callback (0x004B29B0, name 0x00340C48, base
 * __tiQ28ModelSet8Callback 0x00353458) on the first call. */
// FUNC 0x0021F3A0 __tfQ28CarModel8Callback
extern "C" void *__tfQ28CarModel8Callback(void)
{
    if (__tiQ28CarModel8Callback[0] == 0) {
        __tfQ28ModelSet8Callback();
        __rtti_si(__tiQ28CarModel8Callback, "Q28CarModel8Callback",
                  (const type_info *)__tiQ28ModelSet8Callback);
    }
    return __tiQ28CarModel8Callback;
}
#endif
