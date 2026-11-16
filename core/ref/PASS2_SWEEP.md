# Second pass, sweep: cross-unit cleanups and renames

Paths are relative to `core/`. Phase 2 (ref/PASS2_UNITS.md) moved every unit
onto the canonical headers in `include/game/`. Units could not rename what a
canonical header already declared, nor move a constant that several units
define, so their notes (`ref/pass2/units/<unit>/notes.md`) end with proposals,
and `ref/pass2/after_phase2.md` lists the cross-unit cleanups. The sweep
applies them, partition by partition. Behaviour must not change: the listing
(`ref/units/<unit>/listing.txt`) stays the ground truth.

## Your partition

Your task lists the canonical headers you own and every item (cleanup or
rename proposal) that mentions their translation units, classes or functions.
For each item decide:

1. **It changes a declaration in one of your headers** (a rename, a type, a
   missing member or declaration, a constant set that belongs there): do it
   when the evidence agrees (the notes give it; check the listing when in
   doubt), and update every use in `src/` and `include/`, in any unit.
2. **It changes a declaration in another partition's header:** leave it; that
   partition gets the item too when it names its headers. When it names only
   yours, write it in your notes under "For other partitions".
3. **It is for the matching pass** (function or vtable order, egcs-only
   typeinfo guards, the shared bootstrap headers' copy order), the tools, or
   `ref/funcs.json` splits: skip it, say so in your notes.
4. **It is already done:** skip it.

## How to make the changes

- **Renames.** Change the declaration (and the FUNC marker names and the
  definitions in the owner unit), then compile every unit that includes the
  header: the compiler finds every use left. Before you finish, grep for the
  old name: no use may silently bind to another declaration of that name (a
  base member, an overload, a local). Keep the address comments.
- **Shared constants / enums.** One definition in the right header (the names
  the notes agree on; where units disagree, choose and rename the uses), then
  delete the local copies in the units, in the same step, so nothing clashes.
- **Inline accessors** (out-of-line copies with no callers, expanded by the
  callers): define them in the class, delete the out-of-line definition, keep
  its FUNC marker with a comment (ref/PASS2_UNITS.md rule 9); callers that
  wrote the body out with a TODO(pass2) now call the accessor.
- **Missing declarations or argument types:** add or correct them; remove the
  callers' casts and TODO(pass2) workarounds they made unnecessary.
- **TODO(pass2) comments** your changes resolve: delete them.

## Other partitions work at the same time

They edit `src/` files too. Make every source edit as a fresh
read-modify-write (read the file just before the edit, or use a small script
that rereads it), never from an old copy, and compile the unit right after.
If a unit fails to compile on something that is not yours, rerun once later
before reporting it.

## Checks (mandatory)

```sh
sh tools/cc.sh --unit <u>              # every unit you touched or whose headers you changed: OK
python tools/check_coverage.py         # still 11388 claimed, 0 missing / duplicated / unknown
python tools/callcheck.py <u>          # every unit you touched: not fewer calls than before
python tools/hdrcheck.py <header>      # every header you changed: OK
python tools/vtcheck.py                # 0 problems
python tools/hdrall.py --no-egcs       # at the end: no error in your headers
```

Run `python tools/callcheck.py <u>` on a unit before you first edit it, to
know its count.

## Notes

Write `ref/pass2/sweep/<partition>/notes.md`: every change (old -> new, the
header, the units updated), items skipped and why, items for other
partitions. Scratch files go in `build/scratch/<partition>/`.
