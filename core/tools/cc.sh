#!/bin/sh
# Compile-check decompiled sources with the EE toolchain (GCC 14.2).
# Usage (run from core/):  sh tools/cc.sh <file> [<file> ...]
#                          sh tools/cc.sh --unit u017      every source file of work unit u017 (tools/layout.py)
#                          sh tools/cc.sh --all
# Objects go to build/cc/ (discardable). Exit status is non-zero if any file fails.
# The toolchain: <repo>/sdk/ps2sdk, else ps2sdk-main next to the repository, else
# $PS2SDK_HOME or $PS2DEV.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
for c in "$ROOT/../sdk/ps2sdk" "$ROOT/../../ps2sdk-main" "$PS2SDK_HOME" "$PS2DEV"; do
    if [ -n "$c" ] && [ -x "$c/ee/bin/mips64r5900el-ps2-elf-gcc" -o -f "$c/ee/bin/mips64r5900el-ps2-elf-gcc.exe" ]; then
        TC="$c/ee/bin"; break
    fi
done
[ -n "$TC" ] || { echo "no EE toolchain: see the README (Downloads)"; exit 1; }
export PATH="$TC:$PATH"
# -fno-builtin-printf: keep printf("...\n") a call of printf instead of GCC's puts().
# -fno-sized-deallocation (C++): operator delete(void*) as egcs calls it.
CFLAGS="-O2 -G0 -I$ROOT/include -I$ROOT/src -Wall -Wno-unused -Wno-unused-parameter -Wno-attributes -Wno-unknown-pragmas -fno-builtin-printf"
CXXFLAGS_EXTRA="-fno-sized-deallocation -Wno-pmf-conversions"
mkdir -p "$ROOT/build/cc"

if [ "$1" = "--all" ]; then
    set -- $(cd "$ROOT" && find src -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.S' -o -name '*.s' \) 2>/dev/null | sort)
elif [ "$1" = "--unit" ]; then
    set -- $(cd "$ROOT" && python tools/layout.py --files "$2")
    [ $# -gt 0 ] || { echo "no source files for work unit $2"; exit 1; }
fi

rc=0
for f in "$@"; do
    case "$f" in /*|?:*) p="$f" ;; *) p="$ROOT/$f" ;; esac
    o="$ROOT/build/cc/$(echo "$f" | tr '/\:' '___').o"
    case "$f" in
        *.cpp|*.cc) mips64r5900el-ps2-elf-g++ $CFLAGS $CXXFLAGS_EXTRA -std=gnu++14 -fpermissive -c "$p" -o "$o" ;;
        *.c)        mips64r5900el-ps2-elf-gcc $CFLAGS -std=gnu99 -c "$p" -o "$o" ;;
        *.S|*.s)    mips64r5900el-ps2-elf-gcc -I"$ROOT/include" -I"$ROOT/src" -c "$p" -o "$o" ;;
        *) echo "skip $f"; continue ;;
    esac
    if [ $? -eq 0 ]; then echo "OK   $f"; else echo "FAIL $f"; rc=1; fi
done
exit $rc
