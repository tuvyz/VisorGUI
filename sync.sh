#!/bin/bash
# Разнос библиотеки из этой папки в другие её копии: клон tuvyz/VisorGUI и Libs/VisorGUI других проектов.
# Пример: Libs/VisorGUI/sync.sh ../../VisorGUI ../../OtherProgram/Libs/VisorGUI
# Папки назначения заменяются целиком (кроме их .git), потом в каждом репозитории - коммит и пуш
set -e
SRC=$(cd "$(dirname "$0")" && pwd)
for DST in "$@"; do
    mkdir -p "$DST"
    find "$DST" -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf {} +
    tar -C "$SRC" --exclude=.git -cf - . | tar -C "$DST" -xf -
    echo "VisorGUI -> $DST"
done
