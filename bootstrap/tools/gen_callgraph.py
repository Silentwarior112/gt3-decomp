import json, os
WS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "ref")
funcs = json.load(open(os.path.join(WS, "funcs.json")))
with open(os.path.join(WS, "callgraph.txt"), "w") as fh:
    for f in funcs:
        line = "F_%08X size=%-6d" % (f["start"], f["size"])
        if f["callees"]:
            line += " calls: " + " ".join("F_%08X" % c for c in f["callees"])
        if f["callers"]:
            line += " | callers(%d): %s" % (len(f["callers"]), " ".join("F_%08X" % c for c in f["callers"][:12]))
            if len(f["callers"]) > 12:
                line += " ..."
        if f["data_xrefs"]:
            line += " | dataxref: " + " ".join(hex(x) for x in f["data_xrefs"][:8])
        fh.write(line + "\n")
print("ok", len(funcs))
import glob
for c in sorted(glob.glob(os.path.join(WS, "chunks", "*.txt"))):
    print(os.path.basename(c), os.path.getsize(c))
print("datamap", os.path.getsize(os.path.join(WS, "datamap.txt")))
print("full", os.path.getsize(os.path.join(WS, "full.txt")))
