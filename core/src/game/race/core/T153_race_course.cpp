/* T153_race_course.cpp - RaceCourse and the course arena UnitArena<64>
 * (0x0018F688..0x00190198); the classes are declared in
 * include/game/race/core/T153_race_course.h.
 *
 * RaceCourse is the course of a race organization: the loaded course data
 * (CourseData, one 0xD00000-byte unit of the course arena g_002BB5B8, a
 * UnitArena<64> over a 0xD00080-byte malloc), and the course's smoke,
 * rigid body and particle managers.  Most methods are thin wrappers that
 * forward to the course's road (Runway, T457: the lap distance, the plane
 * and the contacts under a point, by a sector hint), its replay cameras
 * (CourseCamera, T172) and the managers.  The unit also loads the two
 * global camera files "cameras/start.cam" and "cameras/attach.cam"
 * (0x80-byte CourseCameraRecords).
 *
 * Data (u050's data pass): .rodata (0x0032DD10..0x0032DD99) "bad_alloc"
 * (<new>), loadCameras' two names, the vtables of RaceCourse and
 * UnitArena<64>, their type names; .data the course arena g_002BB5B8, then
 * the frame info; .sdata (0x00351F94..0x00351F9C) the two camera files'
 * pointers.
 */
// DATA_START 0x0032DD10
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include "libc.h"                           /* malloc, free, memalign */
#include <new>                              /* "bad_alloc" (retail's T153 has it first) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si, __rtti_user (GCC 14's __tf functions) */
#endif
#define GAME_T153_EMITS_TYPEINFO            /* egcs generates __tft9UnitArena1Ui64 here */
#include "game/race/core/T153_race_course.h"          /* own; + T471 _UnitArenaBase, T457 the runway queries */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/effects/T165_particle_manager.h"     /* ParticleManager::emitDebris, ParticleSource */
#include "game/car/physics/T167_rigid_body.h"           /* RigidBodyManager::add, GadgetDesc */
#include "game/camera/T172_course_camera.h"        /* CourseCamera (relocate, loopCount, collect), CourseCameraRecord */
#include "game/effects/T173_race_smoke.h"           /* RaceSmoke::emit (RaceCourse::smoke) */
#include "game/course/T179_course_data.h"          /* CourseData, course_data_relocate / F_001A9178 / course_read */
#include "game/effects/T334_tyre_smoke.h"           /* SmokeSource */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "game/race/core/race_championship_internal.h"                           /* the unit's constants */

/* The start / attach camera files (loadCameras): uninitialised globals, so
 * egcs writes them as .sdata zeros at the end of the unit, in the order of
 * their first declarations (T153_race_course.h). */
CourseCamera *g_00351F94;                   /* 0x00351F94 gp-0x5A5C "cameras/start.cam" */
CourseCamera *g_00351F98;                   /* 0x00351F98 gp-0x5A58 "cameras/attach.cam" */

// FUNC 0x0018F688 RaceCourse::initArena
//   The course arena: a 0xD00080-byte buffer (malloc) of 0xD00000-byte
//   units, one course (_UnitArenaBase::init).  Caller: race_begin (T007).
void RaceCourse::initArena()
{
    void *buf = malloc(COURSE_ARENA_SIZE);
    g_002BB5B8.init(buf, COURSE_ARENA_SIZE, COURSE_UNIT_SIZE);
}

// FUNC 0x0018F6C8 RaceCourse::freeArena
//   The arena forgets its buffer (_UnitArenaBase::reset), which is freed.
//   Caller: race_end (T007).
void RaceCourse::freeArena()
{
    free(g_002BB5B8.reset());
}

// FUNC 0x0018F6F0 RaceCourse::allocBlock
//   No callers.  A unit of the course arena (_UnitArenaBase::alloc).
void *RaceCourse::allocBlock()
{
    return g_002BB5B8.alloc(0);
}

// FUNC 0x0018F718 RaceCourse::freeBlock
//   No callers.  The unit back to the arena (_UnitArenaBase::release).
void RaceCourse::freeBlock(void *block)
{
    g_002BB5B8.release(block);
}

// FUNC 0x0018F740 RaceCourse::loadCameras
//   Loads (once) the start and attach camera files (file_read_blocking
//   from the gt3.vol device, unpack 1) into 0x4000-byte 64-byte aligned
//   buffers (memalign) and relocates them (CourseCamera::relocate).
//   Caller: race_begin (T007).
void RaceCourse::loadCameras()
{
    if (g_00351F94 == NULL) {
        void *buf = memalign(CAMERA_FILE_ALIGN, CAMERA_FILE_SIZE);
        g_00351F94 = (CourseCamera *)file_read_blocking("cameras/start.cam", buf,
                                                        CAMERA_FILE_SIZE, 1, &vol_device_object);
        g_00351F94->relocate();
    }
    if (g_00351F98 == NULL) {
        void *buf = memalign(CAMERA_FILE_ALIGN, CAMERA_FILE_SIZE);
        g_00351F98 = (CourseCamera *)file_read_blocking("cameras/attach.cam", buf,
                                                        CAMERA_FILE_SIZE, 1, &vol_device_object);
        g_00351F98->relocate();
    }
}

// FUNC 0x0018F7D8 RaceCourse::unloadCameras
//   Frees the two camera files.  Caller: race_end (T007).
void RaceCourse::unloadCameras()
{
    if (g_00351F94 != NULL) {
        free(g_00351F94);
        g_00351F94 = NULL;
    }
    if (g_00351F98 != NULL) {
        free(g_00351F98);
        g_00351F98 = NULL;
    }
}

// FUNC 0x0018F818 RaceCourse::operator new
//   The race pool, 16-byte aligned (pdistd::Allocator::allocate).  Caller:
//   race_organization_create (T123).
void *RaceCourse::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x0018F840 RaceCourse::operator delete
//   The size (0x14) is ignored (pdistd::Allocator::free).
void RaceCourse::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x0018F868 RaceCourse::RaceCourse
//   Everything NULL, the vptr.  Caller: race_organization_create (T123).
RaceCourse::RaceCourse()
{
    data = NULL;
    smoke = NULL;
    rigid = NULL;
    particle = NULL;
}

// FUNC 0x0018F890 RaceCourse::~RaceCourse
//   (index: virtual_0.)  The vptr, unload(); deleting form: operator
//   delete(this, 0x14).
RaceCourse::~RaceCourse()
{
    unload();
}

// FUNC 0x0018F8E0 RaceCourse::update
//   An empty function (index: RaceMeterBase::render_2nd, a name matched by
//   identical code).  Caller: RaceBase::virtual_29 (T129: update(1/60)).
void RaceCourse::update(float dt)
{
}

// FUNC 0x0018F8E8 RaceCourse::unload
//   (u043: "unloads the course data".)  F_001A9178 (T179, empty) and the
//   unit back to the arena (_UnitArenaBase::release); data = NULL either
//   way.  Callers: RaceOrganization::unloadCourseData, RaceOrganization::
//   clearCourseModel, the destructor.
void RaceCourse::unload()
{
    if (data != NULL) {
        F_001A9178(data);
        g_002BB5B8.release(data);
    }
    data = NULL;
}

// FUNC 0x0018F930 RaceCourse::load
//   (u043: "loads the course data".)  If none is loaded: course `code' into
//   a unit of the arena (_UnitArenaBase::alloc, course_read), relocated
//   (course_data_relocate).  Callers: RaceBase::virtual_8, load(const char *).
void RaceCourse::load(u64 code)
{
    if (data == NULL) {
        data = course_read(code, g_002BB5B8.alloc(0));
        course_data_relocate(data);
    }
}

// FUNC 0x0018F990 RaceCourse::load
//   No callers.  By course name (nameToCode).
void RaceCourse::load(const char *name)
{
    load(nameToCode(name));
}

// FUNC 0x0018F9C8 RaceCourse::getGridAngle
//   The angle of start grid record i (Runway::grid), plus pi.  Caller:
//   DynamicsConductor::initialize (T305).
f32 RaceCourse::getGridAngle(int i)
{
    return data->runway->grid[i].angle + 3.1415927f;
}

// FUNC 0x0018F9F0 RaceCourse::getGridPosition
//   Start grid record i itself (one lq into $v0: x, y, z, angle).  Caller:
//   DynamicsConductor::initialize (T305).
u128 RaceCourse::getGridPosition(int i)
{
    return *(u128 *)&data->runway->grid[i];
}

// FUNC 0x0018FA10 RaceCourse::getLapLength
//   The runway's lap length (Runway::lapLength).  18 callers.
f32 RaceCourse::getLapLength()
{
    return data->runway->lapLength;
}

// FUNC 0x0018FA20 RaceCourse::getLapDistance
//   (u050 getHeight.)  runway_lap_distance on a query at pos (sector hint
//   pos->sector, updated; lapDist 0 first, so 0 when it finds none; its
//   result is not tested): the distance along the lap.  Callers:
//   DynamicsConductor::noseDistance / courseDistance (T305),
//   lines_prepare (T368).
f32 RaceCourse::getLapDistance(RunwayPos *pos)
{
    RunwayQuery query;                      /* sp+0x00 */
    query.x = pos->x;
    query.y = pos->y;
    query.z = pos->z;
    query.sector = pos->sector;
    query.lapDist = 0.0f;
    runway_lap_distance(data->runway, &query);
    pos->sector = query.sector;
    return query.lapDist;
}

// FUNC 0x0018FA80 RaceCourse::getContact
//   runway_query on a query at c (u = v = 1, the sector hint c->sector,
//   attr = attr2 = 0), and the result copied into c: offRoad (the call
//   returned 0), the plane's d, the sector, the height (the query's y), the
//   low bytes of attr / attr2, the vertex values, the low halfwords of the
//   raw vertex values (lhu), the normal; unk_32 cleared.  Callers:
//   DynCar_place (T330), lines_find_sector / lines_ground_height /
//   lines_ground_at (T368).
void RaceCourse::getContact(RunwayContact *c)
{
    RunwayQuery query;                      /* sp+0x00 */
    query.x = c->x;
    query.y = c->y;
    query.z = c->z;
    query.u = 1.0f;
    query.v = 1.0f;
    query.sector = c->sector;
    query.attr = 0;
    query.attr2 = 0;
    int onRoad = runway_query(data->runway, &query);
    c->offRoad = (onRoad == 0);
    c->sector = query.sector;
    c->height = query.y;
    c->attr = (u8)query.attr;
    c->attr2 = (u8)query.attr2;
    c->u = query.u;
    c->v = query.v;
    c->u0 = (u16)query.u0;
    c->v0 = (u16)query.v0;
    c->normal[0] = query.plane[0];
    c->normal[1] = query.plane[1];
    c->normal[2] = query.plane[2];
    c->unk_32 = 0;
    c->d = query.plane[3];
}

// FUNC 0x0018FB58 RaceCourse::getContacts
//   (u050 F_0018FB58.)  runway_query_contacts: the four wheel contacts in a
//   row.  Caller: car_ground_contact (T298).
void RaceCourse::getContacts(RunwayContact *c)
{
    runway_query_contacts(data->runway, c);
}

// FUNC 0x0018FB78 RaceCourse::getPlane
//   No callers.  The road plane under (x, y, z) from the sector hint
//   `sector' (runway_query); the horizontal plane (0, 1, 0, 0) off the
//   road.
void RaceCourse::getPlane(int sector, f32 *plane, f32 x, f32 y, f32 z)
{
    RunwayQuery query;                      /* sp+0x00 */
    query.sector = sector;
    query.x = x;
    query.y = y;
    query.z = z;
    if (runway_query(data->runway, &query) != 0) {
        plane[0] = query.plane[0];
        plane[1] = query.plane[1];
        plane[2] = query.plane[2];
        plane[3] = query.plane[3];
    } else {
        plane[0] = 0.0f;
        plane[1] = 1.0f;
        plane[2] = 0.0f;
        plane[3] = 0.0f;
    }
}

// FUNC 0x0018FBF8 RaceCourse::getPlane
//   The 4-pointer form with the attribute into a local.  Callers:
//   OnBoardCamera::isViewBlocked (T378), AutomaticCamera::placeAhead /
//   placeBeside (T401), MatrixCamera::update, ZoomupCamera::update.
void RaceCourse::getPlane(const f32 *pos, int *sector, f32 *plane)
{
    int attr;                               /* sp+0x00 */
    getPlane(pos, sector, &attr, plane);
}

// FUNC 0x0018FC18 RaceCourse::getPlane
//   The road plane under pos (runway_query, sector hint *sector, updated)
//   and the triangle's attr (lhu); off the road *sector = -1, attr 0 and
//   the horizontal plane.  Callers: getPlane, F_001A0200 (T167),
//   RaceCarModel::emitSparks (T180).
void RaceCourse::getPlane(const f32 *pos, int *sector, int *attr, f32 *plane)
{
    RunwayQuery query;                      /* sp+0x00 */
    query.x = pos[0];
    query.y = pos[1];
    query.z = pos[2];
    query.sector = *sector;
    if (runway_query(data->runway, &query) != 0) {
        *sector = query.sector;
        plane[0] = query.plane[0];
        plane[1] = query.plane[1];
        plane[2] = query.plane[2];
        plane[3] = query.plane[3];
        *attr = query.attr;
    } else {
        plane[0] = 0.0f;
        plane[1] = 1.0f;
        plane[2] = 0.0f;
        plane[3] = 0.0f;
        *sector = -1;
        *attr = 0;
    }
}

// FUNC 0x0018FCE0 RaceCourse::forEachTri
//   (u050 F_0018FCE0.)  No callers.  runway_for_each_tri: all arguments
//   passed through ($a1..$a3, $f12..$f15).
void RaceCourse::forEachTri(RunwayTriFunc cb, void *ctx, int sector,
                            f32 x0, f32 z0, f32 x1, f32 z1)
{
    runway_for_each_tri(data->runway, cb, ctx, sector, x0, z0, x1, z1);
}

// FUNC 0x0018FD00 RaceCourse::forEachPoint
//   (u050 F_0018FD00.)  runway_for_each_point: all arguments passed
//   through.  Callers: F_001A0258 (T167), car_box_sweep_walls /
//   car_box_push_out_walls (T376).
void RaceCourse::forEachPoint(RunwayPointFunc cb, void *ctx, int sector,
                              f32 x0, f32 z0, f32 x1, f32 z1)
{
    runway_for_each_point(data->runway, cb, ctx, sector, x0, z0, x1, z1);
}

// FUNC 0x0018FD20 RaceCourse::collectReplayCameras
//   (u050 F_0018FD20.)  CourseCamera::collect on the course's replay
//   cameras: the records covering course position pos, at most max.
//   Callers: camera_find_record / camera_find_next_record /
//   camera_find_next_record_for (T382).
int RaceCourse::collectReplayCameras(CourseCameraRecord **out, int max, f32 pos)
{
    return data->replay->collect(out, max, pos);
}

// FUNC 0x0018FD40 RaceCourse::getReplayLoopCount
//   (u050 F_0018FD40.)  CourseCamera::loopCount of the course's replay
//   cameras.  7 callers (T377, T382, T398, T408).
int RaceCourse::getReplayLoopCount()
{
    return data->replay->loopCount();
}

// FUNC 0x0018FD60 RaceCourse::setupRigidBodies
//   When the course has gadget shapes (CourseData::objs) and there is a
//   rigid body manager: each 0x20-byte object of the runway's list (Runway
//   objects / objectCount, unsigned count) is added to it
//   (RigidBodyManager::add; its result is not tested).  Caller:
//   RacePS2Base::virtual_8.
void RaceCourse::setupRigidBodies()
{
    CourseData *course = data;
    if (course->objs != NULL) {
        RigidBodyManager *bodies = rigid;
        Runway *runway = course->runway;
        if (bodies != NULL) {
            for (u32 i = 0; i < runway->objectCount; i++)
                bodies->add(&runway->objects[i]);
        }
    }
}

// FUNC 0x0018FDE0 RaceCourse::emitSmoke
//   (u050 F_0018FDE0.)  RaceSmoke::emit on the smoke manager, if any.
//   Caller: tyre_smoke_emit (T334).
void RaceCourse::emitSmoke(const SmokeSource *src)
{
    if (smoke != NULL)
        smoke->emit(src);
}

// FUNC 0x0018FE08 RaceCourse::emitDebris
//   (u050 F_0018FE08.)  No callers.  ParticleManager::emitDebris on the
//   particle manager, if any.
void RaceCourse::emitDebris(const ParticleSource *src)
{
    if (particle != NULL)
        particle->emitDebris(src);
}

/* The course arena (.data 0x002BB5B8: egcs puts a public object that has a
 * constructor in .data, zero-filled). */
UnitArena<64> g_002BB5B8;                   /* 0x002BB5B8 */

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002BB5B8, emitted at the end of the unit, the destructor first. */
// FUNC 0x0018FE30 race_course_static_dtor (_GLOBAL_$D$initArena__10RaceCourse)
//   .dtors entry (0x0028F218): UnitArena<64>::~UnitArena of the course
//   arena, inlined (the vptr 0x0032DD60, then _UnitArenaBase::
//   ~_UnitArenaBase with flags 2).
/* implicit: ~_UnitArenaBase (base destructor, inlined ~UnitArena) */

// FUNC 0x0018FE60 race_course_static_ctor (_GLOBAL_$I$initArena__10RaceCourse)
//   .ctors entry (0x0028F12C): the course arena (UnitArena<64>'s inline
//   constructor: _UnitArenaBase::_UnitArenaBase, then the vptr 0x0032DD60).
/* implicit: _UnitArenaBase (base constructor, inlined UnitArena constructor) */

// FUNC 0x0018FE98 UnitArena<64>::buildFreeList
/* template code in game/race/core/T153_race_course.h (UnitArena<N>::buildFreeList,
 * index: virtual_1); egcs emits this copy for the course arena (T180 has
 * another, 0x001AB570) */

// FUNC 0x0018FF10 UnitArena<64>::~UnitArena
/* inline in game/race/core/T153_race_course.h (index: virtual_0: the vptr, then
 * _UnitArenaBase::~_UnitArenaBase with the caller's flags); egcs emits this
 * copy (T180 has another, 0x001AB5E8) */
/* implicit: ~_UnitArenaBase (base destructor) */

/* RaceCourse's accessors (u050's data pass): inline, defined here out of
 * the class, in the reverse of retail's order.  egcs writes out-of-class
 * inline members at the end of the unit in the reverse of their
 * definition order, after UnitArena<64>'s members and before
 * __tf10RaceCourse (this unit defines the class's first virtual function,
 * so every inline member is written): retail's T153 has them there,
 * releaseArena first (the frame info).  An exact form; the original may
 * have had them inline in the class.  No caller anywhere (none expands
 * them either): only this unit sees the definitions, which the header's
 * declarations say. */

// FUNC 0x001900F8 RaceCourse::getRunway
//   The course data's road (CourseData::runway, +0x70).
inline Runway *RaceCourse::getRunway()
{
    return data->runway;
}

// FUNC 0x001900E8 RaceCourse::getReplay
//   The course data's replay cameras (CourseData::replay, +0x88).
inline CourseCamera *RaceCourse::getReplay()
{
    return data->replay;
}

// FUNC 0x001900E0 RaceCourse::getCourseData
inline CourseData *RaceCourse::getCourseData()
{
    return data;
}

// FUNC 0x001900D0 RaceCourse::getEnemyLine
//   The course data's enemy lines (CourseData::enemyLine, +0x7C).
inline EnemyLines *RaceCourse::getEnemyLine()
{
    return data->enemyLine;
}

// FUNC 0x001900B8 RaceCourse::F_001900B8
//   No callers.  The course data's drive parameters of class i
//   (CourseData::driveParam[i], +0x94).
inline CourseDriveParam *RaceCourse::F_001900B8(int i)
{
    return data->driveParam[i];
}

// FUNC 0x001900A8 RaceCourse::F_001900A8
//   No callers.  The course data's CourseEnv (CourseData::env, +0x80).
inline CourseEnv *RaceCourse::F_001900A8()
{
    return data->env;
}

// FUNC 0x001900A0 RaceCourse::setSmoke
inline void RaceCourse::setSmoke(RaceSmoke *s)
{
    smoke = s;
}

// FUNC 0x00190098 RaceCourse::getSmoke
inline RaceSmoke *RaceCourse::getSmoke()
{
    return smoke;
}

// FUNC 0x00190090 RaceCourse::setParticle
inline void RaceCourse::setParticle(ParticleManager *p)
{
    particle = p;
}

// FUNC 0x00190088 RaceCourse::getParticle
inline ParticleManager *RaceCourse::getParticle()
{
    return particle;
}

// FUNC 0x00190080 RaceCourse::setRigid
inline void RaceCourse::setRigid(RigidBodyManager *r)
{
    rigid = r;
}

// FUNC 0x00190078 RaceCourse::getRigid
inline RigidBodyManager *RaceCourse::getRigid()
{
    return rigid;
}

// FUNC 0x00190068 RaceCourse::F_00190068
//   No callers.  The course data's gadget shape table (CourseData::objs,
//   setupRigidBodies' condition).
inline CourseObjTable *RaceCourse::F_00190068()
{
    return data->objs;
}

// FUNC 0x00190048 RaceCourse::getMarkerCount
//   (u050 F_00190048.)  No callers.  runway_marker_count: the markers of
//   type 2 with this key.
inline int RaceCourse::getMarkerCount(int key)
{
    return runway_marker_count(data->runway, key);
}

// FUNC 0x00190028 RaceCourse::getMarkerValue
//   (u050 F_00190028.)  No callers.  runway_marker_value: the value of the
//   n-th marker of type 2 with this key.
inline f32 RaceCourse::getMarkerValue(int n, int key)
{
    return runway_marker_value(data->runway, n, key);
}

// FUNC 0x00190018 RaceCourse::F_00190018
//   No callers.  The runway's +0x1C (Runway::unk_1C).
inline f32 RaceCourse::F_00190018()
{
    return data->runway->unk_1C;
}

// FUNC 0x0018FFF8 RaceCourse::isNeighbourSector
//   (u050 F_0018FFF8.)  No callers.  runway_is_neighbour: 1 when `other'
//   is in the neighbour list of `sector'.
inline int RaceCourse::isNeighbourSector(int sector, int other)
{
    return runway_is_neighbour(data->runway, sector, other);
}

// FUNC 0x0018FFE8 RaceCourse::getArena
inline UnitArena<64> *RaceCourse::getArena()
{
    return &g_002BB5B8;
}

// FUNC 0x0018FFE0 RaceCourse::getStartCameras
//   No callers.
inline CourseCamera *RaceCourse::getStartCameras()
{
    return g_00351F94;
}

// FUNC 0x0018FFD0 RaceCourse::getStartCameraCount
//   No callers.
inline int RaceCourse::getStartCameraCount()
{
    return g_00351F94->count;
}

// FUNC 0x0018FFB8 RaceCourse::getStartCamera
//   No callers.  Record i of "cameras/start.cam".
inline CourseCameraRecord *RaceCourse::getStartCamera(int i)
{
    return &g_00351F94->records[i];
}

// FUNC 0x0018FFB0 RaceCourse::getAttachCameras
//   No callers.
inline CourseCamera *RaceCourse::getAttachCameras()
{
    return g_00351F98;
}

// FUNC 0x0018FFA0 RaceCourse::getAttachCameraCount
//   No callers.
inline int RaceCourse::getAttachCameraCount()
{
    return g_00351F98->count;
}

// FUNC 0x0018FF88 RaceCourse::getAttachCamera
//   No callers.  Record i of "cameras/attach.cam".
inline CourseCameraRecord *RaceCourse::getAttachCamera(int i)
{
    return &g_00351F98->records[i];
}

// FUNC 0x0018FF58 RaceCourse::setArena
//   No callers.  The arena over buf (0xD00000-byte units,
//   _UnitArenaBase::init); returns the previous buffer.
inline void *RaceCourse::setArena(void *buf, u32 size)
{
    return g_002BB5B8.init(buf, size, COURSE_UNIT_SIZE);
}

// FUNC 0x0018FF38 RaceCourse::releaseArena
//   No callers.  The arena forgets its buffer (_UnitArenaBase::reset) and
//   returns it.
inline void *RaceCourse::releaseArena()
{
    return g_002BB5B8.reset();
}

#ifndef BOOT_OLD_GCC
// FUNC 0x00190108 __tf10RaceCourse
//   The __user_type_info __ti10RaceCourse (gp-0x4660).
extern "C" const type_info &__tf10RaceCourse(void)
{
    if (__ti10RaceCourse[0] == 0)
        __rtti_user(__ti10RaceCourse, "10RaceCourse");
    return *(const type_info *)__ti10RaceCourse;
}

// FUNC 0x00190148 __tft9UnitArena1Ui64
//   The __si_type_info __tit9UnitArena1Ui64, base __ti14_UnitArenaBase
//   (gp-0x4580).
extern "C" const type_info &__tft9UnitArena1Ui64(void)
{
    if (__tit9UnitArena1Ui64[0] == 0) {
        __tf14_UnitArenaBase();
        __rtti_si(__tit9UnitArena1Ui64, "t9UnitArena1Ui64",
                  (const type_info *)__ti14_UnitArenaBase);
    }
    return *(const type_info *)__tit9UnitArena1Ui64;
}
#endif
