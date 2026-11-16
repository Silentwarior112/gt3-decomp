/* T022 - the one-player arcade class select page ("ar/select_class").
 * 0x00108870..0x001095A8.
 *
 * Over a "cross_bar" Cross (classBar): the class list ("mini_classes",
 * entries "<class>_mini", mark "x"), the class logo ("<class>_logo"), an
 * "arrow", the class text ("<class>_doc", or why the class is disabled), a
 * disabled / undisabled mark, the arrows of T020 (createArrows), the class's
 * still picture from courseIpic in mpegClass and, 6 frames after the class
 * changed, the class's background movie.  In an i.LINK battle
 * (ARCADE_I_LINK) decide / cancel go to the other consoles and the class
 * comes back from the first of them that sent one.
 *
 * The includes give the original's type names (.rodata 0x0030A410..): egcs
 * writes them in reverse definition order, so the original saw the
 * Transitive headers (OperatorHolder, List / Cell<OperatorHolder>,
 * OperatorList), then pdi1394 (its type name: the original's destructor was
 * inline, T541_pdi1394.h), then the GameStatus's garage lists, and
 * SmartIndexT<int> last (ArcadeControl's members; T251_smart_index.h
 * instantiates it where it is read, so T010_arcade_control.h comes last).
 * No menu car header: T022's .rodata has no SimpleCameraBase string. */
#include "libc.h"                           /* strcpy */
#include "game/menu/arcade/T020_arcade_helpers.h"       /* arcade_fade_out */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item; T216 OperatorHolder, OperatorList */
#include "game/menu/framework/T024_mini_cursor.h"          /* gt2k_menu::MiniCursor */
#include "game/menu/framework/T255_cross.h"                /* gt2k_menu::Cross (classBar) */
#include "game/menu/framework/T249_mpeg_image.h"           /* gt2k_menu::MpegImage (mpegClass) */
#include "engine/pdi/T541_pdi1394.h"              /* pdi1394 (its type name: the inline destructor) */
#include "engine/system/T473_link_session.h"         /* F_0022D3B8 (the i.LINK driver) */
#include "game/save/T429_game_status.h"          /* getGameStatus, MenuStatus; the garage lists */
#include "game/app/T004_game_status_holder.h"   /* getGameStatusHolder (the guest garage) */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "game/menu/arcade/T029_arcade_movies.h"        /* MovieParam g_00292930[8] */
#include "engine/io/T502_file_resolve.h"         /* file_resolve, FileLoc */
#include "engine/io/T503_vol_thread.h"           /* vol_device_object */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl (last: SmartIndexT<int>) */
#include "game/menu/arcade/arcade_car_select_2p_internal.h"

namespace gt2k_menu {

/* "disabled" / "disabled_2p" over a class that cannot be chosen, else
 * "undisabled". */
// FUNC 0x00108870 gt2k_menu::ArcadeControl::newDisabledMark
Item *ArcadeControl::newDisabledMark(int disabled, int player)
{
    PageLayout *layout = pages.current;
    const char *name;
    if (disabled != 0)
        name = (player != 2) ? "disabled" : "disabled_2p";
    else
        name = "undisabled";
    /* Object::operator new (the menu heap) */
    /* implicit: operator new, operator delete (new-expression) */
    return new Item(name, layout);
}

/* Pointer to member 0x00350C60 (the class lists' "refuse decide" test, a
 * CheckFn): the road classes on a dirt course, the dirt class on a road
 * course, a garage without cars (carCount < 1: carCount counts the player's
 * current class, which is `cls' in every call: the lists pass the value of
 * the player's class index).  The jump table also has the cases 7 and 8
 * (compulsory, preview: 0). */
// FUNC 0x00108910 gt2k_menu::ArcadeControl::isClassDisabled
int ArcadeControl::isClassDisabled(int cls, int player)
{
    switch (cls) {
    case ARCADE_CLASS_C:
    case ARCADE_CLASS_B:
    case ARCADE_CLASS_A:
    case ARCADE_CLASS_S:
        return isDirtCourse();
    case ARCADE_CLASS_DIRT:
        return !isDirtCourse();         /* xori 1: isDirtCourse returns a bool (0 / 1) */
    case ARCADE_CLASS_HOME_GARAGE:
    case ARCADE_CLASS_GUEST_GARAGE:
        return carCount(player) < 1;
    case ARCADE_CLASS_COMPULSORY:
    case ARCADE_CLASS_PREVIEW:
        break;
    }
    return 0;
}

/* Stops the movie (stopAndWait) and shows player 1's class's still picture
 * (the picture named after the class label in courseIpic, through a stack
 * IpicArchive) in mpegClass. */
// FUNC 0x00108988 gt2k_menu::ArcadeControl::showClassPicture
void ArcadeControl::showClassPicture()
{
    g_002CDC10.stopAndWait();
    const char *label = getClassLabel(class1.value);
    IpicArchive archive(courseIpic);
    archive.find(&ipic, label);
    mpegClass->setIpic(&ipic);
}

/* "<class>_logo" of the player's class. */
// FUNC 0x00108A38 gt2k_menu::ArcadeControl::newClassLogo
Item *ArcadeControl::newClassLogo(int player)
{
    SmartIndexT<int> *cls = classOf(this, player);
    PageLayout *layout = pages.current;
    char name[64];

    const char *label = getClassLabel(cls->value);
    sprintf(name, "%s_logo", label);
    /* Object::operator new (the menu heap) */
    /* implicit: operator new, operator delete (new-expression) */
    return new Item(name, layout);
}

/* The class text: "<class>_doc", or for a disabled class (tested for
 * player 1: the argument of isClassDisabled is 0) disable_road_doc /
 * disable_dirt_doc / disable_no_car_doc.  Quirk: a disabled class past the
 * guest garage (not possible: isClassDisabled is 0 there) would leave the
 * name unset. */
// FUNC 0x00108AD0 gt2k_menu::ArcadeControl::newClassDoc
Item *ArcadeControl::newClassDoc(int player)
{
    SmartIndexT<int> *cls = classOf(this, player);
    PageLayout *layout = pages.current;
    char name[64];

    if (isClassDisabled(cls->value, 0) == 0) {
        const char *label = getClassLabel(cls->value);
        sprintf(name, "%s_doc", label);
    } else {
        /* the strcpys are inlined as 8-byte copies */
        switch (cls->value) {
        case ARCADE_CLASS_C:
        case ARCADE_CLASS_B:
        case ARCADE_CLASS_A:
        case ARCADE_CLASS_S:
            strcpy(name, "disable_road_doc");
            break;
        case ARCADE_CLASS_DIRT:
            strcpy(name, "disable_dirt_doc");
            break;
        case ARCADE_CLASS_HOME_GARAGE:
        case ARCADE_CLASS_GUEST_GARAGE:
            strcpy(name, "disable_no_car_doc");
            break;
        }
    }
    /* Object::operator new (the menu heap) */
    /* implicit: operator new, operator delete (new-expression) */
    return new Item(name, layout);
}

/* The entry names of the class lists (a NameFn: pointers to member
 * 0x00350C40 and 0x00350CF0); `player' is not used. */
// FUNC 0x00108C10 gt2k_menu::ArcadeControl::formatClassMini
void ArcadeControl::formatClassMini(char *buf, int cls, int player)
{
    const char *label = getClassLabel(cls);
    sprintf(buf, "%s_mini", label);
}

/* Builds the page.  The bar's targets start at its own position, then it
 * jumps to the class's page (jumpX); the items placed with attachToBar are
 * relative to it. */
// FUNC 0x00108C50 gt2k_menu::ArcadeControl::openClassSelect
void ArcadeControl::openClassSelect()
{
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];

    /* Object::operator new (the menu heap) */
    /* implicit: operator new, operator delete (new-expression) */
    classBar = new Cross(0);
    classBar->setResource("cross_bar", layout);
    classBar->targetX = classBar->x.value;
    classBar->targetY = classBar->y.value;
    classBar->jumpX(class1.value);
    classBar->attachTo(layer);

    miniClasses = createMiniCursor("mini_classes", &class1, &ArcadeControl::formatClassMini, 0);
    miniClasses->setMark("x", layout);
    if (isDirtCourse()) {
        miniClasses->setMarked(ARCADE_CLASS_C, 1);
        miniClasses->setMarked(ARCADE_CLASS_B, 1);
        miniClasses->setMarked(ARCADE_CLASS_A, 1);
        miniClasses->setMarked(ARCADE_CLASS_S, 1);
    } else {
        miniClasses->setMarked(ARCADE_CLASS_DIRT, 1);
    }
    if (getGameStatus()->getCarNum() <= 0)
        miniClasses->setMarked(ARCADE_CLASS_HOME_GARAGE, 1);
    if (getGameStatusHolder()->get()->getCarNum() <= 0)
        miniClasses->setMarked(ARCADE_CLASS_GUEST_GARAGE, 1);
    miniClasses->attachTo(layer);

    createArrows(&class1, layout, layer);

    logo = newClassLogo(0);
    attachToBar(logo, classBar);
    logo->attachTo(layer);

    arrow = new Item("arrow", layout);
    attachToBar(arrow, classBar);
    arrow->attachTo(layer);

    doc = newClassDoc(0);
    attachToBar(doc, classBar);
    doc->attachTo(layer);

    disabledMark = newDisabledMark(isClassDisabled(class1.value, 0), 0);
    attachToBar(disabledMark, classBar);
    disabledMark->attachTo(layer);

    showClassPicture();
}

/* After a move: the new picture, the bar towards the class's page
 * (wrapTargetX), the arrows, and a new logo, arrow, text and mark (the old
 * ones through arcade_fade_out). */
// FUNC 0x00108F38 gt2k_menu::ArcadeControl::updateClassSelect
void ArcadeControl::updateClassSelect()
{
    PageManager *pm = &pages;
    SmartIndexT<int> *cls = &class1;
    Transitive *layer = pm->layers[pm->popupLevel];

    showClassPicture();
    classBar->wrapTargetX(cls->value);
    updateArrows(cls);

    arcade_fade_out(logo);
    logo = newClassLogo(0);
    attachToBar(logo, classBar);
    logo->attachTo(layer);

    arcade_fade_out(arrow);
    /* Object::operator new (the menu heap) */
    /* implicit: operator new, operator delete (new-expression) */
    arrow = new Item("arrow", pm->current);
    attachToBar(arrow, classBar);
    arrow->attachTo(layer);

    arcade_fade_out(doc);
    doc = newClassDoc(0);
    attachToBar(doc, classBar);
    doc->attachTo(layer);

    arcade_fade_out(disabledMark);
    disabledMark = newDisabledMark(isClassDisabled(class1.value, 0), 0);
    attachToBar(disabledMark, classBar);
    disabledMark->attachTo(layer);
}

/* Forgets the page's objects (the page manager releases them). */
// FUNC 0x001090C8 gt2k_menu::ArcadeControl::clearClassSelect
void ArcadeControl::clearClassSelect()
{
    classBar = NULL;
    miniClasses = NULL;
    clearArrows();
    /* (sched2 puts the last store, the one where `this' dies, first) */
    arrow = NULL;
    logo = NULL;
    doc = NULL;
    disabledMark = NULL;
}

/* class1.move(delta) through an inline call (? an inline member of
 * SmartIndexT in the original): the inline's pointer argument is a fresh
 * pseudo in each branch, which loop.c hoists into one copy of `cls' in front
 * of classSelect's input loop, as retail has it ($s3); with cls->move the
 * loop uses cls's register. */
static inline int moveIndex(SmartIndexT<int> *idx, int delta)
{
    return idx->move(delta);
}

/* The page (runSingleRace, runTimeAttack and runILink, T010).  `forward' is
 * the caller's last result: in a single race (ARCADE_SINGLE_RACE) a
 * forward entry slides the course's history banner in (showHistoryArea), and
 * a return of 0 (back) slides it out again.  Returns 1 = class chosen, 0 =
 * back.  The class starts as the one chosen last (MenuStatus +0x5E8, moved
 * off the classes the course does not allow) and is stored back there; a
 * different class clears MenuStatus::lastArcadeCar.  After a move the movie
 * of the new class (courseMovieName of its label) starts 6 frames later, if
 * the file exists.
 *
 * In an i.LINK battle the consoles first sync on group 1 (the playing
 * nodes; failing: back).  decide sends the class to the group (a data packet
 * 0x10 of one byte) and then a zero byte (sendZero: sendByte(0)), cancel
 * only the zero byte; the page ends on every console once a byte arrives
 * (pollByte: recvByte, the byte dropped).  The class is then the one of
 * the first slot that sent a data packet (waitPackets, polled once), and the
 * page returns 0 if none did. */
// FUNC 0x00109108 gt2k_menu::ArcadeControl::classSelect
int ArcadeControl::classSelect(int forward)
{
    PageManager *pm = &pages;
    SmartIndexT<int> *cls = &class1;
    FileLoc loc;

    int link = (mode == ARCADE_I_LINK);
    pdi1394 *ilink = F_0022D3B8();
    g_002CDC10.start(&g_00292930[2]);
    pm->setPage("ar/select_class", NULL);
    pm->startPage(NULL);
    cls->set(getGameStatus()->getMenuStatus()->arcadeClass);
    if (isDirtCourse()) {
        if (class1.value < ARCADE_CLASS_DIRT)
            cls->set(ARCADE_CLASS_DIRT);
    } else if (class1.value == ARCADE_CLASS_DIRT) {
        cls->set(ARCADE_CLASS_C);
    }

    int movieDelay = 1;
    openClassSelect();
    const char *movie = courseMovieName(getClassLabel(class1.value));
    g_002CDC10.suspend(1);
    if (link && ilink->setGroupAndSync(PDI1394_STATE_PLAY) == 0) {
        forward = 0;
        goto out;
    }
    transitionIn();
    if (forward != 0 && mode == ARCADE_SINGLE_RACE)
        showHistoryArea();
    gfx_vsync(10);

    do {
        if (control1.getLeft() != 0 || control1.getRight() != 0) {
            int moved = 0;
            if (control1.getLeft() != 0)
                moved = moveIndex(&class1, -1);
            else if (control1.getRight() != 0)
                moved = moveIndex(&class1, 1);
            if (moved != 0) {
                menu_play_se(MENU_SE_LIST_MOVE);
                movieDelay = 6;
                updateClassSelect();
                movie = courseMovieName(getClassLabel(class1.value));
                g_002CDC10.suspend(1);
            } else {
                menu_play_se(MENU_SE_REFUSED);
            }
        } else if (control1.getDecide() != 0 || control1.getCancel() != 0) {
            /* One block: when decide reads 0 after the isClassDisabled call
             * and cancel is 0, the pollByte test below is not reached. */
            if (control1.getDecide() != 0 && isClassDisabled(class1.value, 0) != 0) {
                menu_play_se(MENU_SE_REFUSED);
            } else if (control1.getDecide() != 0) {
                if (link) {
                    ilink->beginPacket(PDI1394_CMD_DATA);
                    ilink->putByte((u8)class1.value);
                    ilink->endPacket();
                    ilink->sendPacket(PDI1394_STATE_PLAY);
                    ilink->sendZero();
                } else {
                    menu_play_se(MENU_SE_DECIDE);
                    forward = 1;
                    goto decided;
                }
            } else if (control1.getCancel() != 0) {
                if (link) {
                    ilink->sendZero();
                } else {
                    menu_play_se(MENU_SE_CANCEL);
                    forward = 0;
                    goto decided;
                }
            }
        } else if (link && ilink->pollByte() != 0) {
            goto decided;
        }

        if (movieDelay > 0) {
            if (--movieDelay <= 0 && movie != NULL &&
                file_resolve(movie, &loc, &vol_device_object) >= 0) {
                while (g_002CDC10.play(movie, 0, 0) != 0)
                    waitFrame();
                movie = NULL;
                g_002CDC10.suspend(0);
            }
        }
        waitFrame();
    } while (timedOut == 0);
    forward = 0;
    goto out;

decided:
    if (link) {
        forward = ilink->waitPackets(PDI1394_STATE_PLAY, PDI1394_WAIT_POLL);
        if (forward != 0) {
            ilink->openMessage(-1);
            int chosen = ilink->getByte();
            ilink->closeMessage();
            cls->set(chosen);
        }
    }
out:
    if (forward == 0 && mode == ARCADE_SINGLE_RACE)
        hideHistoryArea();
    transitionOut();
    g_002CDC10.suspend(0);
    clearClassSelect();
    F_0010E310();
    if (getGameStatus()->getMenuStatus()->arcadeClass != class1.value)
        getGameStatus()->getMenuStatus()->lastArcadeCar = 0;
    getGameStatus()->getMenuStatus()->arcadeClass = class1.value;
    pm->closeAndExit();
    gfx_vsync(2);
    return forward;
}

} /* namespace gt2k_menu */
