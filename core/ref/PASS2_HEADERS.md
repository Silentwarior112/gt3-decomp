# Second pass, phase 1: canonical headers

Paths are relative to `core/`. The first pass (ref/DECOMP_RULES.md) gave every
unit its own partial copy of the classes it uses. Phase 1 writes **one
canonical definition** of every class shared by two or more units (or known from
RTTI), of every function another unit calls, and of every global two or more
units use, in `include/game/<TU>_<name>.h`: one header per original translation
unit. Phase 2 then switches the units over to these headers.

## Inputs

- Your group's task lists your headers and what goes in each.
- `ref/pass2/classes/<Class>.txt`, the class dossier: owner, RTTI vtables
  with each slot's method, the methods defined (FUNC markers with their
  definition line), **the merged field table** (offset → every unit's name
  and type), then every unit's definition in full.
- `ref/pass2/class_headers.json`: the canonical header of every shared class,
  for your `#include`s. Every header already exists as a stub, so includes
  resolve.
- The owner unit's source (`src/<unit>/`) and listing
  (`ref/units/<unit>/listing.txt`): the ground truth when units disagree.
- `ref/classes.txt` (vtables), `ref/symbols.csv`, `ref/pass2/globals.txt`.

## What a canonical header contains

```cpp
/* T206_button.h - gt2k_menu::Button (translation unit T206, unit u063). */
#ifndef GAME_T206_BUTTON_H
#define GAME_T206_BUTTON_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

namespace gt2k_menu {
class ButtonList;                           /* pointers only: forward declarations */

/* vtable 0x00332B60 (18 slots), secondary 0x00332B40 (Cell<Button>, this-delta -0x100).
 * typeinfo __tfQ29gt2k_menu6Button 0x001B4850.  Size 0x160 (operator new in T244). */
class Button : public Item, public Cell<Button> {
public:
    Button(ButtonList *owner, ...);         /* 0x001B3C50 */
    virtual ~Button();                      /* [0] +0x0C 0x001B3E60 */
    virtual void update();                  /* [1] +0x14 0x001B4008 */
    ...
    void setFocus(int focus);               /* 0x001B3F58 */

    int controlType;                        /* 0x10C */
    PageLayout *layout;                     /* 0x110 */
    ...
    u8 unk_14C[0xC];                        /* 0x14C  ? */
};
EGCS_SIZE(Button, 0x160);
}

extern int g_menuButtonCount;               /* 0x00350B30 */
void menu_button_reset(int mode);           /* 0x001B3BE8 */

#endif
```

- **Names.** Choose the canonical names of the classes' fields, methods and
  parameters, and of the globals and functions you declare. Prefer the owner
  unit's name when it says what the thing is, else the best name any unit
  found (the merged table lists them all), else keep `unk_<offset>` (fields),
  `F_00XXXXXX` (functions) or `g_00XXXXXX` (globals). Do not invent a meaning
  nobody found evidence for; when you do find it (a string, a caller, the
  listing), use it.
- **Fields.** Every data member, in offset order, each with its original
  (egcs) offset in a comment on its line: `int count; /* 0x10 */`. Fill
  unknown space with `u8 unk_<off>[n];`. Types: the most specific type the
  evidence supports (a class pointer over `void *`, `u16` over `int` when the
  code uses halfword access). Where units disagree on an offset, a size or a
  type, the owner's listing decides.
- **The vptr.** egcs places it after the data of the first class in the
  hierarchy that declares virtuals; declare data members so that egcs
  reproduces the documented offsets (hdrcheck verifies it).
- **Virtual methods.** A class declares the virtuals it introduces, in slot
  order, and the ones it overrides. Each with `/* [slot] +off 0xADDR */`.
  The destructor is a slot like any other.
- **Methods.** Declare every method the owner translation unit defines (the
  dossier lists them), with the signature of its definition (look the
  definition up in the owner's source when the dossier line is cut), and its
  address in a comment.
- **Functions and globals.** Declare each with its address in a comment.
  Functions use the signature of their definition.
- **Sizes.** `EGCS_SIZE(Class, 0xN);` at namespace scope when the size is
  known (operator new, an embedding, an array stride, a stack frame).
- **C translation units** (`G###` without classes): a C header, wrapped in
  `#ifdef __cplusplus extern "C" { ... } #endif`.
- **Includes.** `game/game.h`, then the headers of base classes and of types
  held by value (from `class_headers.json`). Types used through pointers or
  references only: forward declarations. Never include a unit's `src/` header.
- **Language.** C++ both compilers accept (see game.h): no `override`,
  `= default`, `= delete`, in-class member initialisers, `enum class`,
  `constexpr`, `nullptr`; data members `public:`.
- **Templates** stay templates (`template <int N> class RaceBattleT`).
- **Only declarations.** Inline member bodies only when the original is
  inline (an out-of-line copy listed as a duplicate, a tiny accessor the first
  pass wrote inline and that has no FUNC marker of its own).

## Check (mandatory)

```sh
python tools/hdrcheck.py include/game/<header>.h     # every header of your group: OK
```

It compiles the header alone with GCC 14 and makes egcs compute every
documented offset and EGCS_SIZE. A wrong offset is a wrong type or a missing
member before it: fix the declaration, not the comment, unless the listing
shows the comment was wrong. If a header you include belongs to a group that
has not written it yet (still a stub), and a forward declaration cannot
replace it, the check fails for that reason only: say so in your notes.

## Notes

Write your findings to `ref/pass2/groups/<group>/notes.md`: names you chose
where units disagreed, layout conflicts and how the listing settled them,
classes whose owner looks wrong, headers you needed that were still stubs.
Write nothing outside your headers and that file.

## Scratch files

Other agents work at the same time. Keep any scratch files (test copies of
`include/`, throwaway sources) in a folder of your own named after your group,
e.g. `build/scratch/<group>/`, never in a shared folder, and never in `include/` or `src/`.
