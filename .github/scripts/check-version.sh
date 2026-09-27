#!/usr/bin/env bash
#
# Checks that a release tag matches VITA_VERSION in CMakeLists.txt.
#
#     check-version.sh v3.0.0
#
# The tag must be vX.Y.Z with X in 0..99 and Y, Z in 0..9, because the console
# shows the version as XX.YZ (v3.0.0 is 03.00). Exits non-zero with a message
# when the tag is malformed or does not match.

set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if [ "$#" -ne 1 ]; then
    echo "usage: check-version.sh vX.Y.Z" >&2
    exit 2
fi

tag="$1"

if [[ ! "$tag" =~ ^v(0|[1-9][0-9]?)\.([0-9])\.([0-9])$ ]]; then
    echo "check-version.sh: '$tag' is not vX.Y.Z with X <= 99 and Y, Z <= 9" >&2
    exit 1
fi

expected="$(printf '%02d.%s%s' "${BASH_REMATCH[1]}" "${BASH_REMATCH[2]}" "${BASH_REMATCH[3]}")"

actual="$(sed -n 's/^[[:space:]]*set(VITA_VERSION[[:space:]]*"\([^"]*\)").*/\1/p' "$root/CMakeLists.txt" | tr -d '\r')"

if [ -z "$actual" ]; then
    echo "check-version.sh: no VITA_VERSION found in CMakeLists.txt" >&2
    exit 1
fi

if [ "$actual" != "$expected" ]; then
    echo "check-version.sh: tag $tag needs VITA_VERSION $expected, but CMakeLists.txt has $actual" >&2
    exit 1
fi

echo "$tag matches VITA_VERSION $actual"
