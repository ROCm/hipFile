#!/usr/bin/env bash
#
# Recursively format all C & C++ sources and header files

COMMAND="clang-format"

DIRS=(
    "hipfile"
    "rocfile"
)

if [ $# -eq 1 ]; then
    COMMAND="$COMMAND-$1"
fi

echo ""
echo "bin/format_source <version>"
echo ""
echo "Format the C and C++ source using clang-format. The <version>"
echo "parameter is optional and can be used to force a specific"
echo "installed version of clang-format to be used."
echo ""

find "${DIRS[@]}" -iname "*.h" -or -iname "*.c" -or -iname "*.cpp" \
    | xargs -P0 -n1 "${COMMAND}" -style=file -i -fallback-style=none

exit 0
