"""GT3 Decomp Builder: double-click to open the window (README.md)."""
import os
import sys
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "toolchain"))

try:
    from builder import gui
    sys.exit(gui.main())
except SystemExit:
    raise
except Exception:
    # pythonw has no console: write the error down and show it
    text = traceback.format_exc()
    with open(os.path.join(HERE, "GT3Builder-error.log"), "w", encoding="utf-8") as f:
        f.write(text)
    try:
        import tkinter
        from tkinter import messagebox
        tkinter.Tk().withdraw()
        messagebox.showerror("GT3 Decomp Builder", "The builder could not start:\n\n" + text[-1500:])
    except Exception:
        pass
    sys.exit(1)
