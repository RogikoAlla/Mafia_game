#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

to_unix() {
    local path="$1"
    if command -v cygpath >/dev/null 2>&1; then
        cygpath -u "$path"
        return
    fi
    path="${path//\\//}"
    local drive="${path:0:1}"
    echo "/${drive,,}${path:2}"
}

prepend_if_dir() {
    if [[ -d "$1" ]]; then
        PATH="$1:$PATH"
    fi
}

if [[ -n "${LOCALAPPDATA:-}" ]]; then
    local_appdata="$(to_unix "$LOCALAPPDATA")"
    prepend_if_dir "$local_appdata/Microsoft/WinGet/Links"
fi
prepend_if_dir "/c/mingw64/bin"
prepend_if_dir "/mingw64/bin"

if [[ -z "${CXX:-}" ]]; then
    if command -v g++ >/dev/null 2>&1; then
        CXX="$(command -v g++)"
    elif [[ -x /c/mingw64/bin/g++.exe ]]; then
        CXX="/c/mingw64/bin/g++.exe"
    else
        echo "g++ не найден. Ожидается C:/mingw64/bin в PATH." >&2
        exit 1
    fi
fi

# Git Bash resolves g++ without the .exe suffix. CMake on Windows requires the real file.
if [[ "$CXX" != *.exe && -x "${CXX}.exe" ]]; then
    CXX="${CXX}.exe"
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake не найден." >&2
    exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "ninja не найден." >&2
    exit 1
fi

cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER="$CXX"
cmake --build build

if [[ -f build/mafia.exe ]]; then
    ./build/mafia.exe
elif [[ -f build/mafia ]]; then
    ./build/mafia
else
    echo "Сборка прошла, но файл mafia не найден в build/." >&2
    exit 1
fi