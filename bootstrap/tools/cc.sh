#!/bin/sh
# Compile-check decompiled sources with the EE toolchain (GCC 14.2).
# Usage: sh tools/cc.sh <file> [<file> ...]      (run from bootstrap/)
#        sh tools/cc.sh --all
# Objects go to build/ (discardable). Exit status is non-zero if any file fails.
# The toolchain: <repo>/sdk/ps2sdk (the GT3 Decomp Builder downloads it), else
# ps2sdk-main next to the repository, else $PS2SDK_HOME or $PS2DEV.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
for c in "$ROOT/../sdk/ps2sdk" "$ROOT/../../ps2sdk-main" "$PS2SDK_HOME" "$PS2DEV"; do
    if [ -n "$c" ] && [ -x "$c/ee/bin/mips64r5900el-ps2-elf-gcc" -o -f "$c/ee/bin/mips64r5900el-ps2-elf-gcc.exe" ]; then
        TC="$c/ee/bin"; break
    fi
done
[ -n "$TC" ] || { echo "no EE toolchain: see the README (Downloads)"; exit 1; }
export PATH="$TC:$PATH"
# -fno-builtin-printf: keep printf("...\n") a call of printf (Polyphony's printf.cxx,
#   0x01011678) instead of GCC's puts() rewrite - this binary has no puts.
# -fno-sized-deallocation (C++): deleting destructors call operator delete(void*)
#   (0x01019910) as in the binary, not the C++14 sized form.
CFLAGS="-O2 -G0 -I$ROOT/include -Wall -Wno-unused -Wno-unused-parameter -Wno-attributes -fno-builtin-printf"
CXXFLAGS_EXTRA="-fno-sized-deallocation"
mkdir -p "$ROOT/build"

if [ "$1" = "--all" ]; then
    set -- $(cd "$ROOT" && find src sdk runtime -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.S' \) 2>/dev/null | sort)
fi

rc=0
for f in "$@"; do
    case "$f" in /*|?:*) p="$f" ;; *) p="$ROOT/$f" ;; esac
    o="$ROOT/build/$(echo "$f" | tr '/\\:' '___').o"
    case "$f" in
        *.cpp|*.cc) mips64r5900el-ps2-elf-g++ $CFLAGS $CXXFLAGS_EXTRA -std=gnu++14 -fpermissive -c "$p" -o "$o" ;;
        *.c)        mips64r5900el-ps2-elf-gcc $CFLAGS -std=gnu99 -c "$p" -o "$o" ;;
        *.S|*.s)    mips64r5900el-ps2-elf-gcc -I"$ROOT/include" -c "$p" -o "$o" ;;
        *) echo "skip $f"; continue ;;
    esac
    if [ $? -eq 0 ]; then echo "OK   $f"; else echo "FAIL $f"; rc=1; fi
done
exit $rc
