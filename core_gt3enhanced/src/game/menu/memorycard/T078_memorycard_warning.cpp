/* T078 - gt2k_menu::MemorycardWarningControl: the memory card warning at
 * boot (game/menu/memorycard/T078_memorycard_warning.h).
 *
 * Data: .rodata 0x00320330..0x003206B6: <new>'s "bad_alloc" (unreferenced:
 * DATA_START), the functions' strings, the vtable, then the type names
 * (reverse definition order): MemorycardWarningControl, List / Cell of
 * MemorycardFile, OperatorList, List<OperatorHolder>, OperatorHolder,
 * Cell<OperatorHolder>, UnitArena<64>, RegurationResult, RecordsMenuArg, the
 * garage's eight, SmartIndexT<int>, Operator.  So <new>, SmartIndexT<int>
 * (T251), the GameStatus, RecordsMenuArg (T089), RegurationResult, T153's
 * UnitArena<64>, the operator classes (T216), the memory card files (T108),
 * MemorycardWarningControl last.  .sdata: "warning". */
// DATA_START 0x00320330
#include <new>                              /* (seen: "bad_alloc", T078's first .rodata) */
#include "game/common/T251_smart_index.h"          /* (seen: SmartIndexT<int>) */
#include "game/save/T429_game_status.h"          /* (seen: the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* (seen: RecordsMenuArg) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#define GAME_T153_EMITS_TYPEINFO            /* (seen: UnitArena<64>'s type name) */
#include "game/race/core/T153_race_course.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen: the operator classes) */
#include "game/menu/memorycard/T108_memorycard_file.h"      /* MemorycardGameFile (warning_check_card) */
#include "game/menu/memorycard/T078_memorycard_warning.h"   /* MemorycardWarningControl: the last class defined */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, MenuTexture */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object::attachTo */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuText */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "engine/io/T501_memorycard_manager.h"   /* mc_slot_* */
#include "engine/io/T502_file_resolve.h"         /* file_size_retry (a file's size) */
#include "engine/io/T503_vol_thread.h"           /* vol_device_object */
#include "engine/io/T509_serialize.h"            /* Serialize::Base, serialize_create / _destroy */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend */
#include "libc.h"                           /* memalign, free */
#include "game/menu/memorycard/memorycard_icon_internal.h"

namespace gt2k_menu {

// FUNC 0x0015BA98 gt2k_menu::MemorycardWarningControl::load
void MemorycardWarningControl::load()
{
    pages.loadFile("menu/memorycard.mbl");
}

// FUNC 0x0015BAC0 gt2k_menu::MemorycardWarningControl::unload
void MemorycardWarningControl::unload()
{
    pages.unloadFile();
}

// FUNC 0x0015BAE0 gt2k_menu::MemorycardWarningControl::begin
void MemorycardWarningControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    pages.initialize(&render.root, NULL);
    g_00352750 = 0;                     /* no per-frame screen clear */
    g_0035274C = 0;
}

// FUNC 0x0015BB38 gt2k_menu::MemorycardWarningControl::end
void MemorycardWarningControl::end()
{
    pages.finalize();
    gfx_vsync(2);
    render.detach();
    detach();
}

} /* namespace gt2k_menu */

/* The serialised size of the game status (a game save's data). */
// FUNC 0x0015BB78 game_save_size
int game_save_size(void)
{
    Serialize::Base *status = serialize_create("GameStatus");
    int size = status->saveSize();

    serialize_destroy(status);
    return size;
}

/* The card `port' (a T501 card id: MC_CARD_SLOT1 / MC_CARD_SLOT2):
 * WARNING_CARD_OK if it is a PS2 card with needKB + 2 KB free, an unformatted
 * one (the save formats it) or one that already has the game data;
 * WARNING_CARD_NONE without a PS2 card or on an error;
 * WARNING_CARD_NO_ROOM for a PS2 card without the room and without the game
 * data.  The card type and space are read with the interrupts off. */
// FUNC 0x0015BBC8 warning_check_card
int warning_check_card(int port, int needKB)
{
    int isPS2 = 0;
    int hasRoom = 0;
    int intrOn = intr_suspend();
    int found;

    if (mc_slot_type(port) == sceMcTypePS2) {
        isPS2 = 1;
        hasRoom = 1;
        if (mc_slot_formatted(port) != 0 && mc_slot_free(port) - 2 < needKB)
            hasRoom = 0;
    }
    if (intrOn != 0)
        __asm__ volatile("ei");
    if (isPS2 == 0)
        return WARNING_CARD_NONE;
    if (hasRoom != 0)
        return WARNING_CARD_OK;
    {
        /* ~MemorycardGameFile (inline: MemorycardGT3File's destructor) on
         * every return. */
        /* implicit: ~MemorycardGT3File (save's implicit destructor) */
        gt2k_menu::MemorycardGameFile save;

        found = save.exists(port);
        if (found == 1)
            return WARNING_CARD_OK;
        if (found < 0)
            return WARNING_CARD_NONE;
        return WARNING_CARD_NO_ROOM;
    }
}

namespace gt2k_menu {

/* Nothing if a card is usable (warning_check_card); else the page "warning"
 * with the text that fits and the KB needed, until a card is usable (the
 * page is then left through exitRequest) or the page is left. */
// FUNC 0x0015BD00 gt2k_menu::MemorycardWarningControl::warningLoop
void MemorycardWarningControl::warningLoop()
{
    char text[0x400];
    int gameSize, iconSize, needKB, slot1, slot2;
    PageManager *pm;
    PageLayout *layout;
    MenuLayoutItem *needItem;
    const char *needFormat;
    Item *lack1, *lack2, *lack12, *noCard, *need;
    Transitive *layer;

    gameSize = game_save_size();
    /* (T502 declares the size u32; the rounding below divides it signed, so
     * it is kept in an int) */
    iconSize = file_size_retry("menu/icon/ICON_a_spec.ico", 1, &vol_device_object);
    /* KB, rounded up (signed divisions): the data, the icon and 5 KB */
    needKB = (gameSize + 0x3FF) / 0x400 + 5 + (iconSize + 0x3FF) / 0x400;
    slot1 = warning_check_card(MC_CARD_SLOT1, needKB);
    slot2 = warning_check_card(MC_CARD_SLOT2, needKB);
    if (slot1 == WARNING_CARD_OK || slot2 == WARNING_CARD_OK)
        return;

    pm = &pages;
    pm->setPage("warning", NULL);
    pm->startPage(NULL);
    layout = pm->current;
    /* The Items come from Object::operator new (the menu heap); if a
     * constructor throws, Object::operator delete. */
    /* implicit: operator new, operator delete (Object's, the new Items and
     * their landing pads) */
    lack1  = new Item("text_lack1", layout);    /* slot 1's card lacks room, no card in slot 2 */
    lack2  = new Item("text_lack2", layout);    /* slot 2's card lacks room, no card in slot 1 */
    lack12 = new Item("text_lack12", layout);   /* both cards lack room */
    noCard = new Item("text_no_card", layout);
    needItem = layout->findHiddenItem("text_need");
    needFormat = needItem->text->text;          /* "%d" KB */
    need = new Item(needItem, layout->getTexture(needItem));
    sprintf(text, needFormat, needKB);
    need->setText(text);
    noCard->visible = 0;
    layer = pages.layers[pm->popupLevel];
    lack1->visible = 0;
    lack2->visible = 0;
    lack12->visible = 0;
    noCard->attachTo(layer);
    lack1->attachTo(layer);
    lack2->attachTo(layer);
    lack12->attachTo(layer);
    need->attachTo(layer);

    while (pages.exited == 0) {
        if (mc_slot_take_changed(MC_CARD_SLOT1) != 0)
            slot1 = warning_check_card(MC_CARD_SLOT1, needKB);
        if (mc_slot_take_changed(MC_CARD_SLOT2) != 0)
            slot2 = warning_check_card(MC_CARD_SLOT2, needKB);
        if (slot1 == WARNING_CARD_OK || slot2 == WARNING_CARD_OK) {
            pm->exitRequest = 1;                /* the next update leaves the page */
        } else if (slot1 == WARNING_CARD_NONE && slot2 == WARNING_CARD_NONE) {
            noCard->visible = 1;
            lack1->visible = 0;
            lack2->visible = 0;
            lack12->visible = 0;
        } else if (slot2 == WARNING_CARD_NONE) {
            noCard->visible = 0;
            lack1->visible = 1;
            lack2->visible = 0;
            lack12->visible = 0;
        } else if (slot1 == WARNING_CARD_NONE) {
            noCard->visible = 0;
            lack1->visible = 0;
            lack2->visible = 1;
            lack12->visible = 0;
        } else {
            noCard->visible = 0;
            lack1->visible = 0;
            lack2->visible = 0;
            lack12->visible = 1;
        }
        pm->update();
        waitFrame();
    }
    pm->closePage();
}

// FUNC 0x0015C108 gt2k_menu::MemorycardWarningControl::run
void MemorycardWarningControl::run()
{
    load();
    begin();
    warningLoop();
    end();
    unload();
}

} /* namespace gt2k_menu */

/* Called by main at boot: the warning screen over a 1 MB menu heap.
 * Always 1. */
// FUNC 0x0015C150 memorycard_warning
int memorycard_warning(void)
{
    void *heap;

    menu_draw_reset();
    heap = memalign(0x40, WARNING_HEAP_SIZE);
    menu_heap_push(heap, WARNING_HEAP_SIZE);
    {
        /* The inline constructor: ControlBase(), the vptr, RenderBase(),
         * PageManager(this); the implicit destructor at the end of the
         * scope: ~RootObject (render.root), GameObject::~GameObject (render),
         * ControlBase::~ControlBase. */
        /* implicit: ControlBase, RenderBase, PageManager (the inline
         * constructor), ~GameObject (the implicit destructor) */
        /* implicit: ~RootObject (render.root, the implicit destructor) */
        gt2k_menu::MemorycardWarningControl control;

        control.run();
        gfx_vsync(1);
    }
    menu_heap_pop();
    free(heap);
    return 1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0015C2C0 __tfQ29gt2k_menu24MemorycardWarningControl
extern "C" void *__tfQ29gt2k_menu24MemorycardWarningControl(void)
{
    if (__tiQ29gt2k_menu24MemorycardWarningControl[0] == 0) {    /* _name not set yet */
        __tfQ29gt2k_menu11ControlBase();                        /* the base's node first */
        __rtti_si(__tiQ29gt2k_menu24MemorycardWarningControl,
                  "Q29gt2k_menu24MemorycardWarningControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu24MemorycardWarningControl;
}
#endif

// FUNC 0x0015C310 gt2k_menu::MemorycardWarningControl::~MemorycardWarningControl
/* implicit (game/menu/memorycard/T078_memorycard_warning.h); egcs emits this copy, slot 0 of
 * the vtable 0x003203D8: no vptr store; render's inline ~RenderBase
 * (gt2k_menu::RootObject::~RootObject of render.root, flags 2, then
 * GameObject::~GameObject, flags 2), then gt2k_menu::ControlBase::~ControlBase
 * with the flags, which does the delete; pages has no destructor. */
/* implicit: ~RootObject, ~GameObject, ~ControlBase (compiler-generated
 * destructor) */
