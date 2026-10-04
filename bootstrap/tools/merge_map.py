"""Merge bootstrap-map workflow results into ref/symbols.csv + ref/map_notes.md.

Reads the workflow journal (each 'result' entry is one chunk agent's structured
output), joins against ref/funcs.json, and writes:
  ref/symbols.csv   addr,end,size,name,module,kind,confidence,summary,sig
  ref/map_notes.md  per-chunk class layouts, module boundaries, open questions,
                    boundary fixes
"""
import json, os, sys, csv

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, "..", "ref")
if len(sys.argv) < 2:
    sys.exit("usage: python tools/merge_map.py <workflow journal.jsonl>")
JOURNAL = sys.argv[1]

funcs = {f["start"]: f for f in json.load(open(os.path.join(REF, "funcs.json")))}

key2label = {}
results = {}   # label -> result object (last write wins)
for line in open(JOURNAL, encoding="utf-8"):
    j = json.loads(line)
    if j.get("type") == "started":
        key2label[j["key"]] = j.get("label", "")
    elif j.get("type") == "result" and j.get("result"):
        label = key2label.get(j.get("key"), "")
        results[label] = j["result"]

print("chunk results:", sorted(results))

rows = {}
notes = []
for label in sorted(results):
    r = results[label]
    for fn in r.get("functions", []):
        try:
            a = int(fn["addr"], 16)
        except (ValueError, KeyError):
            continue
        base = funcs.get(a)
        rows[a] = {
            "addr": "0x%08X" % a,
            "end": "0x%08X" % base["end"] if base else "",
            "size": base["size"] if base else "",
            "name": fn.get("name", ""),
            "module": fn.get("module", ""),
            "kind": fn.get("kind", ""),
            "confidence": fn.get("confidence", ""),
            "summary": (fn.get("summary") or "").replace("\n", " "),
            "sig": (fn.get("sig") or "").replace("\n", " "),
        }
    notes.append("## %s\n" % label)
    if r.get("module_evidence"):
        notes.append("### module boundaries\n%s\n" % r["module_evidence"])
    if r.get("class_notes"):
        notes.append("### class notes\n%s\n" % r["class_notes"])
    for bf in r.get("boundary_fixes") or []:
        notes.append("- boundary fix %s: %s" % (bf.get("addr"), bf.get("fix")))
    if r.get("open_questions"):
        notes.append("### open questions\n%s\n" % r["open_questions"])
    notes.append("")

fixnote = {}
_extra = os.path.join(REF, "extra_starts.txt")
if os.path.exists(_extra):
    for line in open(_extra, encoding="utf-8"):
        tok, _, rest = line.partition("#")
        if tok.strip():
            fixnote[int(tok.strip(), 16)] = rest.split("|", 1)[-1].strip()

missing = [a for a in funcs if a not in rows]
extra = [a for a in rows if a not in funcs]
print("functions: %d  mapped: %d  missing: %d  extra: %d" %
      (len(funcs), len(funcs) - len(missing), len(missing), len(extra)))
prev_mod, last = {}, ""
for a in sorted(funcs):
    if a in rows:
        last = rows[a]["module"]
    prev_mod[a] = last
for a in sorted(missing):
    note = fixnote.get(a)
    if not note:
        print("UNMAPPED 0x%08X size=%d" % (a, funcs[a]["size"]))
    rows[a] = {"addr": "0x%08X" % a, "end": "0x%08X" % funcs[a]["end"],
               "size": funcs[a]["size"], "name": "F_%08X" % a,
               "module": prev_mod.get(a, ""),
               "kind": "unknown", "confidence": "low",
               "summary": ("SPLIT-OFF (boundary fix): " + note) if note else "UNMAPPED",
               "sig": ""}
for a in sorted(extra):
    print("EXTRA    %s (not a detected function start)" % rows[a]["addr"])

with open(os.path.join(REF, "symbols.csv"), "w", newline="", encoding="utf-8") as fh:
    w = csv.DictWriter(fh, ["addr", "end", "size", "name", "module", "kind",
                            "confidence", "summary", "sig"])
    w.writeheader()
    for a in sorted(rows):
        w.writerow(rows[a])

open(os.path.join(REF, "map_notes.md"), "w", encoding="utf-8").write("\n".join(notes))
print("wrote ref/symbols.csv and ref/map_notes.md")
