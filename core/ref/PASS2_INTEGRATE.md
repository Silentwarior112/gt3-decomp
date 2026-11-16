# Second pass, integration: the canonical headers as one whole

Paths are relative to `core/`. Phase 1 (ref/PASS2_HEADERS.md) wrote the
canonical headers in `include/game/`, group by group. Groups often had to
include headers of later groups that were still stubs, so they wrote in their
notes (`ref/pass2/groups/<group>/notes.md`) what they need from those headers:
layouts, sizes, the names and signatures their overrides use, types that belong
elsewhere, duplicates. All headers are now written. Integration settles these
requests before phase 2 (ref/PASS2_UNITS.md) moves the units onto the headers.

## Your partition

Your task lists the headers you own. You may change them freely, renames
included: phase 2 has not started, and the canonical name table
(ref/pass2/canonical.csv) is regenerated after integration.

Your task also lists every note item, from any group, that mentions your
translation units or classes, and the items of `ref/pass2/integration_todo.md`
that do. For each item decide:

1. **It asks for a change in one of your headers:** make it when the listing
   (`ref/units/<unit>/listing.txt`) or the code agrees; otherwise leave the
   header and say why in your notes.
2. **It asks for a change in another partition's header:** leave it. That
   partition gets the item too when it names its translation units or
   classes; when it names only yours, write it in your notes under "For other
   partitions", with the header and the change.
3. **It concerns unit sources only** (a definition that must change, a cast, a
   call to rename): nothing to do now; phase 2 reads the notes.
4. **It is already done:** nothing to do.

## Typical changes

- **Duplicates.** Two classes that are the same object: keep the definition
  in the owner's header (the translation unit whose code defines its methods,
  or whose data it is), make the other name a `typedef` there, and delete the
  other definition.
- **Misplaced declarations.** A class, function or global that belongs to one
  of your translation units but is declared in another header: add it to your
  header, then delete it from the other header (include your header there if
  it uses the type by value). That deletion is the only edit you may make
  outside your partition.
- **Virtual names and signatures.** An override must have the same name and
  parameter types as its base's declaration of that slot, or it becomes a new
  virtual. When a base of yours was renamed, your overrides follow; when your
  base renamed a slot, `python tools/vtcheck.py` shows the mismatch.
- **Destructors.** egcs stores the vptr in a user-declared destructor (even an
  empty inline one) but not in a compiler-generated one. A class whose
  destructor in the listing stores no vptr declares no destructor.
- **Inline or not.** A member is inline in the header only when the original
  is inline (an out-of-line copy listed as a duplicate, a body expanded at its
  call sites). Phase 2 keeps the FUNC markers of such copies.
- **Ambiguous names.** Do not declare a namespace typedef of a global type of
  the same name (egcs calls it ambiguous under `using namespace`):
  `tools/hdrall.py` probes for these.

Everything else in ref/PASS2_HEADERS.md still holds (comment conventions,
offsets, address comments, egcs-compatible C++).

## Checks (mandatory)

```sh
python tools/hdrcheck.py include/game/<header>.h   # every header you edited: OK
python tools/vtcheck.py                            # no problem in your classes
python tools/hdrall.py --no-egcs                   # while you work (faster)
python tools/hdrall.py                             # at the end: no error in your headers
```

Other partitions edit at the same time: an error in a header that is not
yours may be someone's work in progress; rerun before reporting it.

## Notes

Write `ref/pass2/integration/<partition>/notes.md`:

- every change: header, what, why (one line each);
- **renames** phase 2 must follow, as `old -> new (header)` lines, naming the
  units whose code uses the old name when you know them (`u041`);
- items you did not apply, and why;
- "For other partitions": changes you need in headers that are not yours.

Write nothing in `src/`. Scratch files go in `build/scratch/<partition>/`.
