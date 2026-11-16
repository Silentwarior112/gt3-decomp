# Second pass, calls named only in comments

Paths are relative to `core/`. `tools/callcheck.py` checks that each
function's source names every function the machine code calls (jal and
tail-call targets). Until now a name in a comment counted, so a call the
source left out but described in a comment passed. `--code` counts code only:

```sh
python tools/callcheck.py <u> --code --verbose     # the callees the code does not name
python tools/callcheck.py <u> --verbose            # the old count (comments count)
```

The difference, about 2,100 callees over the 143 units, is what this pass
reviews. The goal: `--code` finds as many calls as the old count, every
difference explained.

## For each callee `--code` reports and the old count finds

Read the call in the listing (`ref/units/<u>/listing.txt`, the ground truth)
and the function's source, then:

1. **The source does make the call, under another spelling** the checker does
   not know (an operator, a renamed method, a virtual call that resolves to
   it, a constructor or destructor of a member, a call through an inline
   accessor or macro whose body calls it, a compiler-generated copy or
   temporary): add an `implicit:` comment at the place of the call naming the
   callee as callcheck spells it (its FUNC marker name, its last `::`
   component, or `F_00XXXXXX`), with the reason:
   `/* implicit: ~Vector3 (temporary) */`, `/* implicit: memcpy (struct copy) */`.
   One comment may name several callees, comma-separated.
2. **The source leaves the call out** (the comment describes it but the code
   does not do it, or does something else): fix the code so it makes the call
   as the listing does, with the right arguments; that is a real fidelity fix.
   Declare the callee if needed (canonical header of its translation unit,
   `ref/pass2/class_headers.json` / `canonical.csv` say which).
3. **The callee is reached by the code but callcheck cannot see it** (a
   function pointer stored and called later, a jump table): `implicit:` as in 1.

Do not add `implicit:` comments for calls the code does not make. Do not
reword the existing comments beyond what the fix needs.

## Rules

`ref/PASS2_SWEEP.md`, "How to make the changes" and "Other partitions work at
the same time", apply: other agents edit `src/` concurrently, so make every
edit as a fresh read-modify-write and compile the unit right after.

## Checks (mandatory)

```sh
sh tools/cc.sh --unit <u>                  # every unit you touched: OK
python tools/callcheck.py <u> --code       # >= the old count of the unit (taken before you start)
python tools/callcheck.py <u>              # not fewer than before
python tools/check_coverage.py             # 0 missing / duplicated / unknown (other agents may be
                                           # mid-change on functions outside your units)
```

## Second round: the stricter checker

After the first round code-only became the default (`--comments` gives the
old count), and the checker stopped accepting a member function's class name
as its spelling: only constructors and destructors match by class name. A
class's `operator new` / `operator delete` match `new` / `delete`, and an
`operator delete` called from a destructor's own block counts as found (a
deleting destructor). What it still reports (`python tools/callcheck.py <u>
--verbose`) is reviewed with the same three rules, including the callees no
count found before (u119, u131, u075, u114, ...). An `implicit:` comment must
name the callee as the checker spells it (`CarEquipments::operator=` works,
the class name alone no longer does).

## Notes

`ref/pass2/calls/<partition>/notes.md`: per unit the two counts before and
after, the fidelity fixes of rule 2 (function, what was missing, the fix), and
the callees left unexplained with the reason. Scratch files in
`build/scratch/<partition>/`.
