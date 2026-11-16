/* T542_eh.h - the GCC 2.9x-ee exception-handling runtime (libgcc2.c, L_eh block;
 * translation unit T542, unit u132, written in C).
 *
 * Same machine code as the bootstrap's runtime/eh.c.  The structures are
 * gcc/eh-common.h's and gcc/frame.h's, in the EE layout.  Statics of the
 * unit: eh_context_initialize 0x002777D8 (the initial value of the
 * get_eh_context pointer, 0x00305EC0; it replaces itself with
 * eh_context_static 0x00277800), new_eh_context 0x00277730, throw_helper
 * 0x00277F38 and the table walkers.
 */
#ifndef GAME_T542_EH_H
#define GAME_T542_EH_H

#include "game/game.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Language matcher of a thrown exception (exception.cc: __cplus_type_matcher). */
typedef void *(*__eh_matcher)(void *info, void *match_info, void *exception_table);

/* Language-independent head of every thrown-exception record
 * (cp_eh_info, T543, starts with it). */
typedef struct __eh_info {
    __eh_matcher match_function;                /* 0x0 */
    short        language;                      /* 0x4 DWARF DW_LANG_*, 4 = C++ */
    short        version;                       /* 0x6 */
} __eh_info;
EGCS_SIZE(__eh_info, 0x8);

typedef struct exception_lang_info {
    short language;                             /* 0x0 */
    short version;                              /* 0x2 */
} exception_lang_info;
EGCS_SIZE(exception_lang_info, 0x4);

/* New-model exception table entry; a start_region of -1 ends the table. */
typedef struct exception_table {
    void *start_region;                         /* 0x0 */
    void *end_region;                           /* 0x4 */
    void *exception_handler;                    /* 0x8 landing pad */
    void *match_info;                           /* 0xC type matched; NULL = cleanup */
} exception_table;
EGCS_SIZE(exception_table, 0x10);

/* Head of a new-model exception table (frame_state.eh_ptr points at one). */
typedef struct exception_descriptor {
    void               *runtime_id_field;       /* 0x0 NEW_EH_RUNTIME */
    exception_lang_info lang;                   /* 0x4 */
    exception_table     table[1];               /* 0x8 */
} exception_descriptor;
EGCS_SIZE(exception_descriptor, 0x18);

#define NEW_EH_RUNTIME ((void *)-2)

/* 79 DWARF register columns on the EE; the arrays hold columns 0..79. */
#define DWARF_FRAME_REGISTERS 79

/* frame_state.saved[] kinds (gcc/frame.h). */
#define REG_UNSAVED      0
#define REG_SAVED_OFFSET 1
#define REG_SAVED_REG    2

/* frame.c's unwind state (gcc/frame.h), filled by __frame_state_for (G006).
 * The word_type members are DImode (ld / sd).  __throw keeps two of them at
 * fp+0 and fp+0x2F0; the inline struct copies move 0x2F0 bytes. */
typedef struct frame_state {
    void          *cfa;                         /* 0x000 */
    void          *eh_ptr;                      /* 0x004 exception table of this frame */
    s64            cfa_offset;                  /* 0x008 */
    s64            args_size;                   /* 0x010 */
    s64            reg_or_offset[DWARF_FRAME_REGISTERS + 1];   /* 0x018 */
    unsigned short cfa_reg;                     /* 0x298 */
    unsigned short retaddr_column;              /* 0x29A */
    char           saved[DWARF_FRAME_REGISTERS + 1];           /* 0x29C REG_UNSAVED / REG_SAVED_* */
} frame_state;
EGCS_SIZE(frame_state, 0x2F0);

/* abort(): the initial value of __terminate_func (0x00305EBC). */
void   __default_terminate(void);               /* 0x002776C0 */
/* (*__terminate_func)(); returns (full epilogue).  __pure_virtual (G005)
 * tail-calls it. */
void   __terminate(void);                       /* 0x002776D0 */
/* &eh_context.info of the current thread (the cp_eh_info list, T543). */
void **__get_eh_info(void);                     /* 0x002777B0 */
/* Unwinds to the handler of the exception in flight; also the resume call at
 * the end of every cleanup landing pad (compiler-emitted). */
void   __throw(void);                           /* 0x00278268 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_T542_EH_H */
