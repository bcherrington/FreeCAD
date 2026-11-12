#!/usr/bin/env bash
set -eu
motion_mod_paths=
for motion_mod in "$PWD"/build/debug/Mod/*; do
    [ -d "$motion_mod" ] && motion_mod_paths="${motion_mod_paths}:$motion_mod"
done
export LD_LIBRARY_PATH="$PWD/build/debug/lib${motion_mod_paths}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$PWD/build/debug/bin/FreeCAD" "$@"
