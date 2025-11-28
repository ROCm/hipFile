#!/usr/bin/env bash
# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

# Used by CI to test hipFile IO using aiscp

# Usage:
# ci-aiscp-test.sh WORKDIR AISCP
#   WORKDIR: A directory on an AIS supported filesystem backed by an AIS supported device
#   AISCP: Path to the aiscp binary

set -e

trap 'if [ -e "$TMPDIR" ]; then rm -fr "$TMPDIR"; fi' EXIT

TESTDIR=$1
AISCP=$2

echo "Test directory: $TESTDIR"
echo "aiscp: $AISCP"

TMPDIR=$(mktemp --directory --tmpdir="$TESTDIR")
SRC="$TMPDIR/rand.dat"
DST="$TMPDIR/rand.dat.cp"
echo "Source: $SRC"
echo "Dest:   $DST"

head --bytes 1G /dev/urandom > "$SRC"

"$AISCP" "$SRC" "$DST"

SRC_MD5=$(md5sum "$SRC" | awk '{ print $1 }')
DST_MD5=$(md5sum "$DST" | awk '{ print $1 }')

echo "Source file md5:      $SRC_MD5"
echo "Destination file md5: $DST_MD5"

if [ "$SRC_MD5" != "$DST_MD5" ]; then
    echo "Files differ! Test failed. MD5 checksums do not match."
    exit 1
fi
