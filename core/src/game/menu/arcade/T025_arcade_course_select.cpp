/* T025 - "ar/select_course", the arcade course select (gt2k_menu::
 * ArcadeControl methods, declared with the class in T010_arcade_control.h).
 * 0x0010DFF0..0x0010F228.
 *
 * The page: a cross bar ("cross_bar", a Cross) with the course's name,
 * logo, document, an arrow, the total length and length text, the area
 * (single race only) and the surface hung off it (attachToBar), the
 * course's still (an Ipic of "stream/course.ipic" in mpegCourse), the
 * MiniCursor strip "mini_courses" with an "x" over the locked courses, and
 * the left / right arrows (T020).  Behind it the background movie
 * g_00292930[4] runs; once the cursor has rested 6 frames on a course its
 * preview "stream/<course>_full.pss" replaces it.  In the i.LINK battle the
 * decision goes to the other consoles and the chosen course comes back over
 * the link.
 *
 * Includes: retail's .rodata (0x0030B400) has the literals, then the type
 * names pdi1394, SmartIndexT<int>, OperatorList, List<OperatorHolder>,
 * OperatorHolder, Cell<OperatorHolder>, the garage lists (GarageWheelList ..
 * Cell<GarageCar>) and Operator (reverse definition order): the GameStatus
 * first (T429 defines Operator before the garage classes), then the menu
 * items (T216's OperatorHolder ..), ArcadeControl (SmartIndexT<int>), the
 * i.LINK class (its type name: T541_pdi1394.h's inline destructor). */
#include "game/save/T429_game_status.h"        /* getGameStatus; T427 MenuStatus (arcadeMode, arcadeCourse) */
#include "game/menu/framework/T213_menu_item.h"            /* Item (the course's items) */
#include "game/menu/framework/T255_cross.h"                /* Cross (the course bar) */
#include "game/menu/framework/T249_mpeg_image.h"           /* MpegImage::setIpic */
#include "game/menu/framework/T024_mini_cursor.h"          /* MiniCursor (mini_courses) */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl; SmartIndexT<int> (T251), Ipic (T250) */
#include "engine/pdi/T541_pdi1394.h"              /* pdi1394 (the i.LINK course choice), PDI1394_* */
#include "engine/system/T473_link_session.h"         /* F_0022D3B8: the pdi1394 driver */
#include "engine/pdi/G003_pdi1394_rpc.h"          /* F_0026EB90 (the link reset on cancel) */
#include "game/menu/arcade/T020_arcade_helpers.h"       /* arcade_fade_out (the items given up on a refresh) */
#include "game/menu/arcade/T029_arcade_movies.h"        /* g_00292930 (the background movie) */
#include "game/menu/framework/T250_ipic.h"                 /* IpicArchive (the course stills) */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/data/T435_product_config.h"       /* product_config: the course lists */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getCourseKind */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 (the course length) */
#include "engine/io/T502_file_resolve.h"         /* file_resolve, FileLoc */
#include "engine/io/T503_vol_thread.h"           /* vol_device_object */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/pgl/G001_flare.h"                /* fmt_sprintf (%t), gfx_vsync */
#include "engine/media/G002_mpeg_frameq.h"          /* strlen */
#include "runtime/newlib/G007_stdio.h"                /* strcpy, strcat, _ctype_ */
#include "game/menu/arcade/arcade_course_select_internal.h"

namespace gt2k_menu {

/* 1 if course `course' of the mode's list cannot be chosen: not available
 * in the product database, or (i.LINK battle) more consoles in the play
 * state than its kind takes: kind 0 six, kind 1 four, kind 2 (dirt) two, an
 * unknown kind none (maxConsoles stays 0).  (Settled: the 0 is the
 * listing's initial value, set before the switch.)  The database lives in
 * the i.LINK block: its exception cleanup sits between the two returns. */
// FUNC 0x0010DFF0 gt2k_menu::ArcadeControl::isCourseLocked
int ArcadeControl::isCourseLocked(int course)
{
    const char *list = getModeName();
    if (product_config()->courseAvailable(course, list) == 0)
        return 1;
    if (mode == ARCADE_I_LINK) {
        RaceDataBase db;
        int maxConsoles = 0;
        ProductConfig *config = product_config();
        switch (db.getCourseKind(nameToCode(config->courseName(course, list)))) {
        case 0:
            maxConsoles = 6;
            break;
        case 1:
            maxConsoles = 4;
            break;
        case COURSE_KIND_DIRT:
            maxConsoles = 2;
            break;
        }
        if (maxConsoles < F_0022D3B8()->getGroupCount(PDI1394_STATE_PLAY))
            return 1;
    }
    return 0;
}

/* newlib's tolower() (ctype.h: a statement expression; _U = 1). */
#define CTYPE_TOLOWER(c) ({ int __x = (c); ((_ctype_ + 1)[__x] & 1) ? __x - 'A' + 'a' : __x; })

/* "stream/" + the course name in lower case + "_full.pss", in a static
 * buffer (0x0049BB50).  The buffer is this file's: egcs aligns a defined
 * char array to 8, so strcpy's "stream/" (an .sdata literal under -G8) is
 * one ld / sd (an extern array gives ldl / ldr / sdl / sdr).  Also called
 * by classSelect (T022). */
// FUNC 0x0010E130 gt2k_menu::ArcadeControl::courseMovieName
const char *ArcadeControl::courseMovieName(const char *course)
{
    static char buf[0x40];                  /* 0x0049BB50 */
    int len = strlen(course);
    strcpy(buf, "stream/");
    int n = strlen(buf);
    for (int i = 0; i < len; i++)
        buf[n++] = CTYPE_TOLOWER(course[i]);
    buf[n] = '\0';
    strcat(&buf[n], "_full.pss");
    return buf;
}

/* No callers. */
// FUNC 0x0010E200 gt2k_menu::ArcadeControl::F_0010E200
const char *ArcadeControl::F_0010E200(const char *name)
{
    static char buf[0x40];                  /* 0x0049BB90 */
    sprintf(buf, "%s1", name);
    return buf;
}

/* Stops the background movie (stopAndWait: the stream stopped, its feeder
 * thread ended), then shows the current course's still from courseIpic in
 * mpegCourse (IpicArchive on the stack; find's result is not checked). */
// FUNC 0x0010E240 gt2k_menu::ArcadeControl::loadCourseImage
void ArcadeControl::loadCourseImage()
{
    g_002CDC10.stopAndWait();
    ProductConfig *config = product_config();
    int index = course.value;
    const char *name = config->courseName(index, getModeName());
    IpicArchive archive((const IpicArchiveData *)courseIpic);
    archive.find(&ipic, name);
    mpegCourse->setIpic(&ipic);
}

/* Empty.  Also called by classSelect (T022). */
// FUNC 0x0010E310 gt2k_menu::ArcadeControl::F_0010E310
void ArcadeControl::F_0010E310()
{
}

/* "<course>_logo" under the current layer, on the bar.  The page's
 * layout and layer through PageManager's inline accessors (retail keeps
 * &pages in a register), the name in a local before sprintf (the format's
 * address is loaded after the call), as in all the T025 builders. */
// FUNC 0x0010E318 gt2k_menu::ArcadeControl::createCourseLogo
void ArcadeControl::createCourseLogo()
{
    char name[0x40];
    ProductConfig *config = product_config();
    int index = course.value;
    const char *label = config->courseName(index, getModeName());
    sprintf(name, "%s_logo", label);
    /* implicit: operator new, operator delete (new-expression) */
    logo = new Item(name, pages.getFragmentManager());   /* Object::operator new; operator delete if Item() throws */
    attachToBar(logo, courseBar);
    logo->attachTo(pages.getLayer());
}

/* "<course>_doc" and "arrow" under the current layer, on the bar. */
// FUNC 0x0010E3F8 gt2k_menu::ArcadeControl::createCourseDoc
void ArcadeControl::createCourseDoc()
{
    Transitive *layer = pages.getLayer();
    char name[0x40];
    ProductConfig *config = product_config();
    int index = course.value;
    const char *label = config->courseName(index, getModeName());
    sprintf(name, "%s_doc", label);
    /* implicit: operator new, operator delete (new-expression) */
    doc = new Item(name, pages.getFragmentManager());    /* Object::operator new; operator delete if Item() throws */
    attachToBar(doc, courseBar);
    doc->attachTo(layer);
    arrow = new Item("arrow", pages.getFragmentManager());
    attachToBar(arrow, courseBar);
    arrow->attachTo(layer);
}

/* "total_length", "length" with the course's length as its text, the area
 * "area_a".."area_f" (the arcade level the course needs, ProductConfig
 * courseLevel; single race only, else area = NULL) and "dirt" / "tarmac".
 * The length: the course length (ProductConfig courseLength) converted from
 * metric (UnitSettings::convertDistance) and printed "%4d%s" (metric, the
 * unit "m") or "%.3t%s" (imperial, fmt_sprintf's fixed point: thousandths
 * of "miles").  The switch is on 0 / 3 (system == UNIT_METRIC ? 0 : 3; ? an
 * inline UnitSettings accessor, the decimals of the short distance), so its
 * default "----" (an inlined 5-byte copy) is dead. */
// FUNC 0x0010E548 gt2k_menu::ArcadeControl::createCourseInfo
void ArcadeControl::createCourseInfo()
{
    PageLayout *layout = pages.getFragmentManager();
    Transitive *layer = pages.getLayer();
    /* implicit: operator new, operator delete (new-expression) */
    totalLength = new Item("total_length", layout); /* Object::operator new; operator delete if Item()
                                               throws (each new below the same) */
    attachToBar(totalLength, courseBar);
    totalLength->attachTo(layer);
    length = new Item("length", layout);
    {
        char text[0x40];
        ProductConfig *config = product_config();
        int index = course.value;
        int distance = g_002F59D8.convertDistance(config->courseLength(index, getModeName()),
                                                  UNIT_METRIC);
        int decimals = (g_002F59D8.system == UNIT_METRIC) ? 0 : 3;
        switch (decimals) {
        case 0:
            sprintf(text, "%4d%s", distance, g_002F59D8.shortDistanceName());
            break;
        case 3:
            fmt_sprintf(text, "%.3t%s", distance, g_002F59D8.shortDistanceName());
            break;
        default:
            strcpy(text, "----");
            break;
        }
        length->setText(text);
    }
    attachToBar(length, courseBar);
    length->attachTo(layer);
    if (mode == ARCADE_SINGLE_RACE) {
        ProductConfig *config = product_config();
        int index = course.value;
        int level = config->courseLevel(index, getModeName());
        const char *areaNames[6] = {        /* a copy of the .rodata table 0x0030B420 */
            "area_a", "area_b", "area_c", "area_d", "area_e", "area_f",
        };
        area = new Item(areaNames[level], layout);
        attachToBar(area, courseBar);
        area->attachTo(layer);
    } else {
        area = NULL;
    }
    const char *surfaceName;
    if (isDirtCourse() != 0)
        surfaceName = "dirt";
    else
        surfaceName = "tarmac";
    surface = new Item(surfaceName, layout);
    attachToBar(surface, courseBar);
    surface->attachTo(layer);
}

/* The NameFn of the "mini_courses" strip (its pointer-to-member constant
 * 0x00350E50 = {0, -1, 0x0010E898}): "<course>_mini".  arg is unused. */
// FUNC 0x0010E898 gt2k_menu::ArcadeControl::miniCourseName
void ArcadeControl::miniCourseName(char *buf, int index, int arg)
{
    ProductConfig *config = product_config();
    const char *label = config->courseName(index, getModeName());
    sprintf(buf, "%s_mini", label);
}

/* Builds the page.  The bar (Cross mode 0, "cross_bar") keeps its layout
 * position as its target and jumps to the course's screen (the valid
 * courses before it: course.get(), called directly as SmartSkipIndex::get /
 * size); the course name, logo, doc, info; the strip with an "x" over every
 * locked course (the loop marks the first size() indices, whatever their
 * validity); the arrows; the still. */
// FUNC 0x0010E910 gt2k_menu::ArcadeControl::createCourseScreen
void ArcadeControl::createCourseScreen()
{
    PageLayout *layout = pages.getFragmentManager();
    Transitive *layer = pages.getLayer();
    /* implicit: operator new, operator delete (new-expression) */
    courseBar = new Cross(0);               /* Object::operator new(0x114); operator delete if Cross() throws */
    courseBar->setResource("cross_bar", layout);
    courseBar->targetX = courseBar->x.value;
    courseBar->targetY = courseBar->y.value;
    courseBar->attachTo(layer);
    courseBar->jumpX(course.get());
    ProductConfig *config = product_config();
    int index = course.value;
    const char *name = config->courseName(index, getModeName());
    courseName = new Item(name, layout);
    attachToBar(courseName, courseBar);
    courseName->attachTo(layer);
    createCourseLogo();
    createCourseDoc();
    createCourseInfo();
    miniCourses = createMiniCursor("mini_courses", &course, &ArcadeControl::miniCourseName, 0);
    miniCourses->setMark("x", layout);
    for (int i = 0; i < course.size(); i++)
        if (isCourseLocked(i) != 0)
            miniCourses->setMarked(i, 1);
    miniCourses->attachTo(layer);
    createArrows(&course, layout, layer);
    loadCourseImage();
}

/* After a move: the new still, the bar towards the course's screen, and the
 * course's items replaced (the old ones given to arcade_fade_out; the area only
 * if there is one), the arrows updated. */
// FUNC 0x0010EB48 gt2k_menu::ArcadeControl::refreshCourseScreen
void ArcadeControl::refreshCourseScreen()
{
    PageLayout *layout = pages.getFragmentManager();
    Transitive *layer = pages.getLayer();
    loadCourseImage();
    courseBar->wrapTargetX(course.get());
    arcade_fade_out(courseName);
    ProductConfig *config = product_config();
    int index = course.value;
    const char *name = config->courseName(index, getModeName());
    /* implicit: operator new, operator delete (new-expression) */
    courseName = new Item(name, layout);    /* Object::operator new; operator delete if Item() throws */
    attachToBar(courseName, courseBar);
    courseName->attachTo(layer);
    arcade_fade_out(logo);
    createCourseLogo();
    arcade_fade_out(doc);
    arcade_fade_out(arrow);
    createCourseDoc();
    arcade_fade_out(totalLength);
    arcade_fade_out(length);
    if (area != NULL)
        arcade_fade_out(area);
    arcade_fade_out(surface);
    createCourseInfo();
    updateArrows(&course);
}

/* Forgets the page's items (the page's layer owns them). */
// FUNC 0x0010ECC8 gt2k_menu::ArcadeControl::clearCourseScreen
void ArcadeControl::clearCourseScreen()
{
    totalLength = NULL;
    length = NULL;
    area = NULL;
    surface = NULL;
    arrow = NULL;
    doc = NULL;
    clearArrows();
    miniCourses = NULL;
    courseBar = NULL;
    logo = NULL;
    courseName = NULL;
}

/* course.move(delta) through an inline call, as u003's classSelect
 * (T022): the virtual call through a pointer (? an inline member of
 * SmartIndexT in the original). */
static inline int moveIndex(SmartIndexT<int> *idx, int delta)
{
    return idx->move(delta);
}

/* The page: 1 when a course was chosen, 0 on cancel.  `first' nonzero: the
 * index is set up anew over the mode's course list (clamped), on the
 * remembered course (MenuStatus::arcadeCourse) if it was remembered for this
 * mode; the history banner of the level comes in (single race).  Quirk: when
 * the loop ends through timedOut outside the i.LINK battle, the result is
 * `first' itself.
 *
 * i.LINK battle: the consoles in the play state first sync
 * (setGroupAndSync; 0 ends the page undecided); a decide sends the course
 * (a 0x10 packet with its index, then a zero byte), a cancel resets the
 * link (F_0026EB90: no argument; the listing's $a0 = 1 there is
 * menu_play_se(1)'s argument, scheduled into the shared branch delay slot);
 * the loop ends when a byte comes in (pollByte), then a polled packet of the
 * play group (waitPackets) gives the chosen course.  Callers: T010
 * runILinkOnly, runSingleRace, runTimeAttack, runTwoPlayer, runILink.
 *
 * The shape of u003's classSelect (T022): `first' is the result, the
 * control accessors (the fields directly are merged into one ld), the
 * moves through the inline moveIndex (retail's hoisted copy of &course in
 * $s3).  `while (1)', not `for (;;)': egcs lays the for loop out rotated
 * (the head after the timeout test), the while loop as retail. */
// FUNC 0x0010ED18 gt2k_menu::ArcadeControl::selectCourse
int ArcadeControl::selectCourse(int first)
{
    PageManager *pm = &pages;
    g_002CDC10.start(&g_00292930[4]);
    int linked = (mode == ARCADE_I_LINK);
    pdi1394 *link = F_0022D3B8();
    pm->setPage("ar/select_course", NULL);
    pm->startPage(NULL);
    if (first != 0) {
        SmartIndexT<int> *index = &course;
        ProductConfig *config = product_config();
        index->setRange(0, config->courseCount(getModeName()));
        index->set(0);
        index->clamp = 1;
        if (getGameStatus()->getMenuStatus()->arcadeMode == mode)
            index->set(getGameStatus()->getMenuStatus()->arcadeCourse);
    }
    int restFrames = 1;                     /* frames until the preview movie starts */
    createCourseScreen();
    transitionIn();
    const char *movie;
    {
        ProductConfig *config = product_config();
        int index = course.value;
        movie = courseMovieName(config->courseName(index, getModeName()));
    }
    g_002CDC10.suspend(1);
    if (first != 0 && mode == ARCADE_SINGLE_RACE)
        showHistoryLevel();
    gfx_vsync(10);

    if (linked && link->setGroupAndSync(PDI1394_STATE_PLAY) == 0) {
        first = 0;
        goto out;
    }
    while (1) {
        if (control1.getLeft() != 0 || control1.getRight() != 0) {
            int moved = 0;
            if (control1.getLeft() != 0)
                moved = moveIndex(&course, -1);
            else if (control1.getRight() != 0)
                moved = moveIndex(&course, 1);
            if (moved != 0) {
                menu_play_se(MENU_SE_LIST_MOVE);
                restFrames = 6;
                refreshCourseScreen();
                ProductConfig *config = product_config();
                int value = course.value;
                movie = courseMovieName(config->courseName(value, getModeName()));
                g_002CDC10.suspend(1);
            } else {
                menu_play_se(MENU_SE_REFUSED);
            }
        } else if (control1.getDecide() != 0 || control1.getCancel() != 0) {
            if (control1.getDecide() != 0 && isCourseLocked(course.value) != 0) {
                menu_play_se(MENU_SE_REFUSED);
                waitFrame();
                continue;                   /* skips the movie count and the timeout test */
            }
            if (control1.getDecide() != 0) {
                if (linked) {
                    link->beginPacket(PDI1394_CMD_DATA);
                    link->putByte((u8)course.value);
                    link->endPacket();
                    link->sendPacket(PDI1394_STATE_PLAY);
                    link->sendZero();
                } else {
                    menu_play_se(MENU_SE_DECIDE);
                    first = 1;
                    break;
                }
            } else if (control1.getCancel() != 0) {
                if (linked) {
                    F_0026EB90();
                } else {
                    menu_play_se(MENU_SE_CANCEL);
                    first = 0;
                    break;
                }
            }
        } else if (linked && link->pollByte() != 0) {
            break;
        }
        if (restFrames > 0) {
            restFrames--;
            if (restFrames <= 0 && movie != NULL) {
                FileLoc loc;
                if (file_resolve(movie, &loc, &vol_device_object) >= 0) {
                    while (g_002CDC10.play(movie, 0, 0) != 0)
                        waitFrame();
                    movie = NULL;
                    g_002CDC10.suspend(0);
                }
            }
        }
        waitFrame();
        if (timedOut != 0)
            break;
    }
    if (linked) {
        first = link->waitPackets(PDI1394_STATE_PLAY, PDI1394_WAIT_POLL);
        if (first != 0) {
            link->openMessage(-1);
            course.set(link->getByte());
            link->closeMessage();
            menu_play_se(MENU_SE_DECIDE);
        } else {
            menu_play_se(MENU_SE_CANCEL);
        }
    }
out:
    if (first == 0 && mode == ARCADE_SINGLE_RACE)
        hideHistoryLevel();
    transitionOut();
    g_002CDC10.suspend(0);
    clearCourseScreen();
    F_0010E310();
    waitFrame();
    getGameStatus()->getMenuStatus()->arcadeCourse = course.value;
    pm->closeAndExit();
    gfx_vsync(2);
    return first;
}

} /* namespace gt2k_menu */
