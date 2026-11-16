/* T153_race_course.h - RaceCourse, the course arena UnitArena<64> and the road
 * query records (translation unit T153, unit u050), 0x0018F688..0x00190198.
 *
 * RaceCourse is the course of a race organization (RaceOrganization's
 * course, DynamicsConductor +0xB7A8, RaceCarState +0x08; u077 CourseRef): the
 * loaded course data (CourseData, T179: one 0xD00000-byte unit of the course
 * arena g_002BB5B8) and the course's smoke, rigid body and particle managers.
 * Most methods forward to the course's road ("runway", T457: the lap
 * distance, the plane and the contacts under a point, by a sector hint), its
 * replay cameras (CourseCamera, T172) and the managers.  The static members
 * keep the course arena and the two global camera files "cameras/start.cam"
 * and "cameras/attach.cam" (0x80-byte CourseCameraRecords).
 *
 * RunwayPos / RunwayQuery / RunwayContact are the records of the road
 * queries: RaceCourse fills them on the stack or for its callers, T457 (u103)
 * writes them (its names are used).  T330's GroundQuery (u077 / u081) is a
 * typedef of RunwayContact; RaceCarState (T302) holds four at +0x14BC.
 *
 * Names: u050 (the owner) and the Lupo 2 symbols (getRigid .. getRunway,
 * getEnemyLine, getArena); the wrappers of T457 / T172 / T173 / T165 are
 * named after the functions they call (see ref/pass2/groups/h26/notes.md). */
#ifndef GAME_T153_RACE_COURSE_H
#define GAME_T153_RACE_COURSE_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */
#include "engine/pdistd/T471_unit_arena.h"           /* base _UnitArenaBase */
#include "game/course/T457_runway.h"               /* RunwayTriFunc, RunwayPointFunc (the area walks) */

class CourseData;                           /* T179_course_data.h (pointers only) */
struct CourseObjTable;                      /* T179_course_data.h */
class CourseEnv;                            /* T178_course_env.h */
struct CourseDriveParam;                    /* T361_drive_params.h */
struct EnemyLines;                          /* T368_enemy_lines.h */
struct Runway;                              /* T457_runway.h */
class CourseCamera;                         /* T172_course_camera.h */
class CourseCameraRecord;                   /* T172_course_camera.h */
class RaceSmoke;                            /* T173 (no header) */
class RigidBodyManager;                     /* T167_rigid_body.h */
class ParticleManager;                      /* T165_particle_manager.h */
class SmokeSource;                          /* T334_tyre_smoke.h */
struct ParticleSource;                      /* T165 (? name, u056) */
class type_info;

/* The typeinfo functions (vtable entry 0), declared before the classes
 * (egcs rejects an extern "C" declaration after the class; an earlier one is
 * the declaration it uses, and it then generates nothing: u045).
 * RaceCourse's is egcs's own (its key function is in T153, which writes the
 * name): GCC 14 only.  UnitArena<64>'s stays declared for egcs too: the
 * template has no key function, and egcs would write its type name into
 * every object that includes this header, in front of the objects' own
 * names (retail has it after them: the original's include order; u041 /
 * u056's typeinfo names moved by 0x18).  GCC 14 has no __tf functions: the
 * build defines them in T153 (a weak copy of the template's in T180). */
#ifndef BOOT_OLD_GCC
extern "C" const type_info &__tf10RaceCourse(void);     /* 0x00190108 __user_type_info 0x00353390 */
#endif
#if !defined(BOOT_OLD_GCC) || !defined(GAME_T153_EMITS_TYPEINFO)
/* (T153 itself defines GAME_T153_EMITS_TYPEINFO: egcs generates the template's
 * there, its copy 0x00190148.) */
extern "C" const type_info &__tft9UnitArena1Ui64(void); /* 0x00190148 __si_type_info 0x004B12C0, base
                                                           __ti14_UnitArenaBase 0x00353470 */
#endif

/* ======================================================================== */
/* The road query records                                                   */
/* ======================================================================== */

/* A point of the road and its sector hint (RaceCourse::getLapDistance). */
struct RunwayPos {
    f32 x;                                  /* 0x00 the point (runway axes: y is up) */
    f32 y;                                  /* 0x04 */
    f32 z;                                  /* 0x08 */
    int sector;                             /* 0x0C the sector hint (in / out; -1: off the road) */
};
EGCS_SIZE(RunwayPos, 0x10);

/* The query record of runway_lap_distance / runway_query (T457; the callers'
 * stack objects: RaceCourse's frames hold 0x40 bytes, runway_query writes up
 * to +0x3F).  u050: height, f10, f14, f1E, f20, f24 for lapDist .. v0. */
struct RunwayQuery {
    f32 x;                                  /* 0x00 the point */
    f32 y;                                  /* 0x04 out (runway_query): the road height */
    f32 z;                                  /* 0x08 */
    f32 lapDist;                            /* 0x0C out (runway_lap_distance): the distance along the lap */
    f32 u;                                  /* 0x10 out (runway_query): the vertex values at the point (0..1);
                                               getContact sets both to 1 first (u050 f10, f14) */
    f32 v;                                  /* 0x14 */
    int sector;                             /* 0x18 in: the sector hint (-1: none); out (-1: off the road) */
    u16 attr;                               /* 0x1C out: the triangle's attr (getContact clears it first) */
    u16 attr2;                              /* 0x1E out: the triangle's attr2 */
    int u0;                                 /* 0x20 out: vertex 0's raw u (sw by T457; RaceCourse reads the
                                               low halfword) */
    int v0;                                 /* 0x24 out: vertex 0's raw v */
    f32 plane[4];                           /* 0x28 out: the road plane: unit normal x, y, z, d */
    const RunwayTri    *tri;                /* 0x38 out: the triangle */
    const RunwayVertex *verts;              /* 0x3C out: its sector's vertices */
};
EGCS_SIZE(RunwayQuery, 0x40);

/* The road under a point (RaceCourse::getContact, one; runway_query_contacts,
 * four in a row: the wheels, RaceCarState +0x14BC).  4-aligned.  T330's
 * GroundQuery is a typedef of it. */
struct RunwayContact : RunwayPos {
    f32 height;                             /* 0x10 the road height (the query's y) */
    f32 u;                                  /* 0x14 the vertex values at the point (0..1) (u050 / u078 / u082 /
                                               u087 f14; u078: the smoke's intensity) */
    f32 v;                                  /* 0x18 (u050 / u078 / u082 / u087 f18) */
    f32 normal[3];                          /* 0x1C the road plane's unit normal (y up) */
    f32 d;                                  /* 0x28 the plane's d */
    u16 u0;                                 /* 0x2C vertex 0's raw u (u050 / u078 / u087 f2C) */
    u16 v0;                                 /* 0x2E vertex 0's raw v (f2E) */
    u8  offRoad;                            /* 0x30 no road under the point */
    u8  attr;                               /* 0x31 the triangle's attr (low byte) */
    u8  unk_32;                             /* 0x32 cleared (u050 / u078 / u087 / u103 f32_) */
    u8  attr2;                              /* 0x33 the triangle's attr2 (low byte; u050 / u078 / u087 f33,
                                               u087: bit 0 read) */
};
EGCS_SIZE(RunwayContact, 0x34);

/* ======================================================================== */
/* UnitArena<N>                                                             */
/* ======================================================================== */

/* The arena of N-byte aligned units.  UnitArena<64>: vtable 0x0032DD60
 * (2 slots) with an identical copy 0x00330B68 (T180), typeinfo
 * __tft9UnitArena1Ui64 0x00190148 (si, base _UnitArenaBase).  No data of its
 * own: 0x1C bytes.  Its members are template code, emitted in every unit
 * that uses UnitArena<64>: T153 (0x0018FF10, 0x0018FE98, the course arena)
 * and T180 (0x001AB5E8, 0x001AB570, the car models' arena). */
template <unsigned int N>
class UnitArena : public _UnitArenaBase {
public:
    UnitArena() {}                          /* (inline: _UnitArenaBase(), then the vptr) */
    virtual ~UnitArena() {}                 /* [0] +0x0C <64>: 0x0018FF10 (the vptr, then ~_UnitArenaBase) */
    /* The free list: units of unitSize rounded up to N bytes from the
     * N-byte aligned start of the buffer, as many as end within buffer +
     * size (unsigned compares); each unit's first word links the one
     * before, freeList = the last one.  used = 0, nfree = the units. */
    virtual void buildFreeList();           /* [1] +0x14 <64>: 0x0018FE98 */
};

template <unsigned int N>
void UnitArena<N>::buildFreeList()
{
    u8 *p = (u8 *)(((int)buffer + (int)(N - 1)) & -(int)N);
    u32 unit = (unitSize + (int)(N - 1)) & -(int)N;
    u8 *end = (u8 *)buffer + size;
    u8 *next = p + unit;
    void **head = 0;

    used = 0;
    nfree = 0;
    if (!(end < next)) {
        int n = 0;
        do {
            *(void ***)p = head;
            n++;
            head = (void **)p;
            p = next;
            next += unit;
        } while (!(end < next));
        nfree = n;
    }
    freeList = head;
}

EGCS_SIZE(UnitArena<64>, 0x1C);

/* ======================================================================== */
/* RaceCourse                                                               */
/* ======================================================================== */

/* vtable 0x0032DD50 (1 slot), typeinfo __tf10RaceCourse 0x00190108 (user,
 * no bases).  0x10 bytes of data, vptr +0x10; 0x14 bytes, from the race
 * pool (operator new; RaceOrganization's construction, F_001798A0). */
class RaceCourse {
public:
    /* The race object pool g_002B3BF0 (pdistd::Allocator), 16-byte aligned. */
    static void *operator new(size_t size);             /* 0x0018F818 */
    static void  operator delete(void *p, size_t size); /* 0x0018F840 (the size is ignored) */
    RaceCourse();                                       /* 0x0018F868 all NULL */
    virtual ~RaceCourse();                              /* [0] +0x0C 0x0018F890 unload() */

    /* The course arena: a 0xD00080-byte malloc of 0xD00000-byte units (one
     * course).  From the game's start-up / shut-down (u001). */
    static void  initArena();                           /* 0x0018F688 */
    static void  freeArena();                           /* 0x0018F6C8 */
    static void *allocBlock();                          /* 0x0018F6F0 no callers */
    static void  freeBlock(void *block);                /* 0x0018F718 no callers */
    /* Loads (once) / frees "cameras/start.cam" and "cameras/attach.cam"
     * (0x4000-byte 64-byte aligned buffers, relocated). */
    static void  loadCameras();                         /* 0x0018F740 */
    static void  unloadCameras();                       /* 0x0018F7D8 */

    void  update(float dt);                             /* 0x0018F8E0 empty (RaceBase::virtual_29: 1/60) */
    /* The course data back to the arena (F_001A9178 first). */
    void  unload();                                     /* 0x0018F8E8 */
    /* When none is loaded: course `code' into a unit of the arena
     * (course_read), then course_data_relocate. */
    void  load(u64 code);                               /* 0x0018F930 */
    void  load(const char *name);                       /* 0x0018F990 by name (F_00231190) */
    f32   getGridAngle(int i);                          /* 0x0018F9C8 the start grid's record i: angle + pi */
    u128  getGridPosition(int i);                       /* 0x0018F9F0 the record itself (x, y, z; lq) */
    f32   getLapLength();                               /* 0x0018FA10 the runway's lapLength (u050 F_0018FA10) */
    /* runway_lap_distance at pos (pos->sector the hint, updated): the
     * distance along the lap, 0 when not found (u050 getHeight). */
    f32   getLapDistance(RunwayPos *pos);               /* 0x0018FA20 */
    /* runway_query at c (u = v = 1, attr = attr2 = 0 first); the result
     * copied into c: offRoad (the query returned 0), the plane, the sector,
     * the height, the attribute bytes and the vertex values. */
    void  getContact(RunwayContact *c);                 /* 0x0018FA80 */
    /* runway_query_contacts: four contacts in a row (the wheels; c[0].sector
     * the hint) (u050 F_0018FB58). */
    void  getContacts(RunwayContact *c);                /* 0x0018FB58 */
    /* The road plane under (x, y, z) from the sector hint; off the road the
     * horizontal plane (0, 1, 0, 0).  No callers. */
    void  getPlane(int sector, f32 *plane, f32 x, f32 y, f32 z);           /* 0x0018FB78 */
    void  getPlane(const f32 *pos, int *sector, f32 *plane);               /* 0x0018FBF8 */
    /* The road plane under pos (*sector the hint, updated) and the attr;
     * off the road *sector = -1, *attr = 0 and the horizontal plane. */
    void  getPlane(const f32 *pos, int *sector, int *attr, f32 *plane);    /* 0x0018FC18 */
    /* runway_for_each_tri / runway_for_each_point over (x0, z0)..(x1, z1)
     * (u050 F_0018FCE0 / F_0018FD00; u057 / u088: the walls in the box). */
    void  forEachTri(RunwayTriFunc cb, void *ctx, int sector,
                     f32 x0, f32 z0, f32 x1, f32 z1);                       /* 0x0018FCE0 no callers */
    void  forEachPoint(RunwayPointFunc cb, void *ctx, int sector,
                       f32 x0, f32 z0, f32 x1, f32 z1);                     /* 0x0018FD00 */
    /* The replay cameras' records covering course position pos, at most
     * max (CourseCamera::collect; u050 F_0018FD20). */
    int   collectReplayCameras(CourseCameraRecord **out, int max, f32 pos); /* 0x0018FD20 */
    /* CourseCamera::loopCount of the replay cameras (u050 F_0018FD40). */
    int   getReplayLoopCount();                         /* 0x0018FD40 */
    /* When the course has them (data +0x90) and there is a rigid body
     * manager: each 0x20-byte object of the runway (+0x54, +0x50 of them)
     * is added to it. */
    void  setupRigidBodies();                           /* 0x0018FD60 */
    void  emitSmoke(const SmokeSource *src);            /* 0x0018FDE0 RaceSmoke::emit, if any (u050 F_0018FDE0) */
    void  emitDebris(const ParticleSource *src);        /* 0x0018FE08 ParticleManager::emitDebris, if any;
                                                           no callers (u050 F_0018FE08) */
    /* The accessors from here on are inline, defined in T153 itself (u050's
     * data pass: retail emits them at the end of T153, after UnitArena<64>'s
     * members); no other unit calls them. */
    static void *releaseArena();                        /* 0x0018FF38 forgets the arena's buffer, returns it */
    /* The arena over buf (0xD00000-byte units); returns the old buffer. */
    static void *setArena(void *buf, u32 size);         /* 0x0018FF58 */
    static CourseCameraRecord *getAttachCamera(int i);  /* 0x0018FF88 record i of "cameras/attach.cam" */
    static int   getAttachCameraCount();                /* 0x0018FFA0 */
    static CourseCamera *getAttachCameras();            /* 0x0018FFB0 */
    static CourseCameraRecord *getStartCamera(int i);   /* 0x0018FFB8 record i of "cameras/start.cam" */
    static int   getStartCameraCount();                 /* 0x0018FFD0 */
    static CourseCamera *getStartCameras();             /* 0x0018FFE0 */
    static UnitArena<64> *getArena();                   /* 0x0018FFE8 &g_002BB5B8 */
    int   isNeighbourSector(int sector, int other);     /* 0x0018FFF8 runway_is_neighbour (u050 F_0018FFF8) */
    f32   F_00190018();                                 /* 0x00190018 the runway's +0x1C */
    f32   getMarkerValue(int n, int key);               /* 0x00190028 runway_marker_value (u050 F_00190028) */
    int   getMarkerCount(int key);                      /* 0x00190048 runway_marker_count (u050 F_00190048) */
    CourseObjTable *F_00190068();                       /* 0x00190068 the course data's objs (+0x90,
                                                           setupRigidBodies' condition) */
    RigidBodyManager *getRigid();                       /* 0x00190078 */
    void  setRigid(RigidBodyManager *r);                /* 0x00190080 */
    ParticleManager *getParticle();                     /* 0x00190088 */
    void  setParticle(ParticleManager *p);              /* 0x00190090 */
    RaceSmoke *getSmoke();                              /* 0x00190098 */
    void  setSmoke(RaceSmoke *s);                       /* 0x001900A0 */
    CourseEnv *F_001900A8();                            /* 0x001900A8 the course data's env (+0x80) */
    CourseDriveParam *F_001900B8(int i);                /* 0x001900B8 the course data's driveParam[i] (+0x94) */
    EnemyLines *getEnemyLine();                         /* 0x001900D0 the course data's enemyLine (+0x7C) */
    CourseData *getCourseData();                        /* 0x001900E0 */
    CourseCamera *getReplay();                          /* 0x001900E8 the course data's replay cameras (+0x88) */
    Runway *getRunway();                                /* 0x001900F8 the course data's +0x70 */

    CourseData       *data;                 /* 0x00 the loaded course, NULL: none */
    RaceSmoke        *smoke;                /* 0x04 (u044 obj04, T173) set by RacePS2Base::virtual_8 */
    RigidBodyManager *rigid;                /* 0x08 (u044 rigidBodies) */
    ParticleManager  *particle;             /* 0x0C (u044 particles) */
    /*  vptr                                   0x10 */
};
EGCS_SIZE(RaceCourse, 0x14);

/* ======================================================================== */
/* T153 data and functions                                                  */
/* ======================================================================== */

extern UnitArena<64> g_002BB5B8;            /* 0x002BB5B8 the course arena (one 0xD00000-byte unit; u043:
                                               the pool the course data are allocated from) */
extern CourseCamera *g_00351F94;            /* 0x00351F94 gp-0x5A5C "cameras/start.cam" */
extern CourseCamera *g_00351F98;            /* 0x00351F98 gp-0x5A58 "cameras/attach.cam" */

/* The .ctors / .dtors entries of the course arena are egcs's
 * _GLOBAL_$D$initArena__10RaceCourse 0x0018FE30 (.dtors entry 0x0028F218:
 * ~UnitArena<64>) and _GLOBAL_$I$initArena__10RaceCourse 0x0018FE60 (.ctors
 * entry 0x0028F12C: UnitArena<64>()), generated for g_002BB5B8, which T153
 * defines (u050's race_course_static_dtor / _ctor). */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
extern u32 __ti10RaceCourse[2];             /* 0x00353390 gp-0x4660 the __user_type_info */
extern u32 __tit9UnitArena1Ui64[3];         /* 0x004B12C0 the __si_type_info (u050 / u060 g_004B12C0; the plan also put it in T151) */
}
#endif

#endif /* GAME_T153_RACE_COURSE_H */
