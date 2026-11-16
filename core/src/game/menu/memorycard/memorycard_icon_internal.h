/* memorycard_icon_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u032,
 * 0x00159EE8..0x0015D448, second pass.
 *
 * Translation units (link order):
 *   T077  the 3D icons of the memory card screens (game/menu/memorycard/T077_memorycard_icon.h):
 *         a PS2 save icon (.ico) is checked, unpacked into a McIconModel in
 *         one of two pdistd::Allocator heaps or fixed slots, and drawn.
 *   T078  gt2k_menu::MemorycardWarningControl, the memory card warning at boot
 *         (game/menu/memorycard/T078_memorycard_warning.h).
 *   T079  gt2k_menu::MemorycardDemoFileManager, the demo replays of the disc as
 *         a file list (game/app/T079_demo_file_manager.h), plus T079's unreferenced
 *         copies of MemorycardFileManagerBase's inline members (T109).
 *   T080  gt2k_menu::SettingControl and setting_menu, the car setting menu
 *         (game/menu/car_settings/T080_setting_control.h), plus T080's unreferenced copies of
 *         GarageCar's / Cell<GarageCar>'s inline destructors and typeinfo
 *         functions (T036).
 *
 * Every shared class is defined in a canonical header; this header adds what
 * only this unit uses: T077's file-local functions (the unpacking, the
 * lighting of a vertex, the animation weights) and data (the two heaps, the
 * two slots, the quads), T078's game_save_size, and the typeinfo storage of
 * the unit's own classes.
 *
 * Each source includes the canonical headers it needs itself, in the order
 * that gives the original's type names (u032's data pass: egcs writes the
 * type names of the classes a file sees in the reverse order of their
 * definitions; each file's comment lists the original's), then this header.
 * It includes only headers that make a file see no class (no type names, no
 * literals).
 */
#ifndef U032_H
#define U032_H

#include "types.h"
#include <stddef.h>
#include "pdistd/allocator.h"               /* pdistd::Allocator (the icon heaps) */
#include "game/menu/memorycard/T077_memorycard_icon.h"      /* McIconModel, McIconLight, McIconAnim, icon_* */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_user, __rtti_class, base_info */
#endif

/* ======================================================================== */
/* T077: file-local functions and data                                      */
/* ======================================================================== */

/* The size of a fixed icon slot (icon_slots_alloc takes it from each heap;
 * icon_slot_create refuses a bigger model). */
enum { ICON_SLOT_SIZE = 0x25800 };

/* A vertex of the morphed mesh in the scratchpad buffer (? name): what
 * tri_draw16 reads (12-bit fixed point x, y, z, then a word the copy brings
 * along). */
struct McIconVertex {
    s16 x, y, z, w;
};

/* The bytes icon_model_build needs for an icon file, -1 if it is not a
 * usable icon. */
int          icon_model_size(const u32 *ico);   /* 0x00159EE8 */
/* Unpacks an icon file into mem (icon_model_size bytes, 16-aligned; size is
 * not used); NULL if it is not an icon file. */
McIconModel *icon_model_build(void *mem, int size, const u32 *ico,
                              const McIconLight *light); /* 0x0015A0E0 */
/* The RLE texture of an icon file into dst (at most 0x8000 bytes).  The
 * texels are s16 (retail ORs in 0x8000 as the short -0x8000 and sign-extends
 * the run texel). */
void         icon_texture_unpack(s16 *dst, const s16 *src, int size); /* 0x0015A558 */
/* The lit colour of a vertex with normal n. */
u32          icon_light_vertex(const McIconLight *light, const f32 *n, u32 rgba); /* 0x0015ABB0 */
f32          icon_anim_key_value(McIconAnim *anim, int frame, f32 t); /* 0x0015B6E8 */
f32          icon_anim_time(McIconAnim *anim, f32 time); /* 0x0015B7C0 */
void         icon_anim_weights(McIconAnim *anim, s32 *out, f32 time); /* 0x0015B810 */

/* T077 data.  The core addresses of the Allocator methods used here:
 * allocate = heap_alloc_core 0x0026AA68, free = heap_free_core 0x0026AD88,
 * assign 0x0026A9F0, unassign 0x0026AA50, Allocator(int) 0x0026A7C8. */
extern pdistd::Allocator g_iconHeap[2];         /* 0x002A8328 (0x18 each; defined in T077) */
/* bool (4 bytes in egcs): icon_model_create's load of it is not ordered
 * after its int store *error = ICON_OK (another alias set), as in retail.
 * The three are defined in T077 without an initialiser: egcs writes them as
 * zeros at the end of T077's .sdata in the order of these declarations (the
 * first ones), as retail has them. */
extern bool         g_iconHeapReady[2];         /* 0x003518D0 gp-0x6120 the heap is assigned */
extern int          g_iconSlotUsed[2];          /* 0x003518D8 gp-0x6118 the slot holds a model */
extern McIconModel *g_iconSlot[2];              /* 0x003518E0 gp-0x6110 ICON_SLOT_SIZE blocks of the heaps */
/* The quads of icon_model_render: the sprite's (u, v), the sprite's (x, y,
 * z) and the background's (x, y, z); defined in T077 (external linkage: egcs
 * writes them where they are defined, before icon_model_draw's jump table,
 * as retail has them). */
extern const f32 g_00320290[4][2];              /* 0x00320290 (0,0) (1,0) (0,1) (1,1) */
extern const f32 g_003202B0[4][3];              /* 0x003202B0 (-1,-1,0) (1,-1,0) (-1,1,0) (1,1,0) */
extern const f32 g_003202E0[4][3];              /* 0x003202E0 (-22,-22,-50) (33,-22,-50) (-22,26.4,-50)
                                                   (33,26.4,-50) */

/* ======================================================================== */
/* T078 / T080: file-local                                                  */
/* ======================================================================== */

/* The serialised size of the game status: a game save's data (T078). */
int game_save_size(void);                       /* 0x0015BB78 */

/* The menu heaps of memorycard_warning and setting_menu: memalign(0x40, n),
 * pushed with menu_heap_push for the controller's lifetime. */
enum {
    WARNING_HEAP_SIZE = 0x100000,               /* 1 MB */
    SETTING_HEAP_SIZE = 0x40000                 /* 256 KB */
};

/* The memory card ids warning_check_card is asked about (T501: port * 4 +
 * slot): slot 1 and slot 2 of the console, first multitap slot. */
enum {
    MC_CARD_SLOT1 = 0,
    MC_CARD_SLOT2 = 4
};

/* warning_check_card's results. */
enum {
    WARNING_CARD_OK      = 0,                   /* usable: room for a game save, unformatted, or a save
                                                   already there */
    WARNING_CARD_NONE    = 1,                   /* no PS2 memory card (or an error) */
    WARNING_CARD_NO_ROOM = 2                    /* a PS2 card without the room and without a save */
};

/* ======================================================================== */
/* RTTI (modern compiler only; egcs generates these itself)                 */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
extern "C" {
/* The __si_type_info nodes of the unit's own classes, filled by their __tf
 * functions. */
extern u32 __tiQ29gt2k_menu24MemorycardWarningControl[3];  /* 0x004B1BD0 */
extern u32 __tiQ29gt2k_menu25MemorycardDemoFileManager[3]; /* 0x004B1BE0 */
extern u32 __tiQ29gt2k_menu14SettingControl[3];            /* 0x004B1610 */
}
/* T080's copy of GarageCar's base list (the same words as T036's
 * g_00311EA0): {Cell<GarageCar> 0x004B1350, offset 0, public},
 * {CarInformation 0x004B2A50, offset 0x10, public}. */
extern const __class_type_info::base_info g_00320D48[2];   /* 0x00320D48 */
#endif /* !BOOT_OLD_GCC */

#endif /* U032_H */
