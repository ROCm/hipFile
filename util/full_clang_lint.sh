#!/usr/bin/env bash
# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

# CI script that will highlight any issues clang-format complains about
# as well as the diff of what needs to change.
# Requires a .clang-format file in the directory selected to lint.

if [[ $# -ne 1 ]];
then
        echo "Requires one argument of the directory to lint."
        exit 1
fi

WORKING_DIRECTORY=$1
ERROR_FOUND=0

for file_to_lint in $(find ${WORKING_DIRECTORY} -type f \( -name "*.c" -or -name "*.h" \));
do
        clang-format --Werror --dry-run --style=file ${file_to_lint}
        if [[ $? -ne 0 ]];
        then
                ERROR_FOUND=1
                clang-format --Werror --style=file ${file_to_lint} | diff -u -p --color ${file_to_lint} -
                echo -e "\n"
        fi
done

exit ${ERROR_FOUND}
