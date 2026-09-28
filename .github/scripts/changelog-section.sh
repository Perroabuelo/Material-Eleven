#!/usr/bin/env bash
#
# Prints the body of the "## [X.Y.Z]" section of CHANGELOG.md, up to the next
# "## [" heading, without the heading itself or surrounding blank lines.
#
#     changelog-section.sh 3.0.0
#
# Exits non-zero when the section is missing or empty.

set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if [ "$#" -ne 1 ]; then
    echo "usage: changelog-section.sh X.Y.Z" >&2
    exit 2
fi

version="$1"

section="$(awk -v heading="## [$version]" '
    { sub(/\r$/, "") }
    index($0, "## [") == 1 {
        if (found) exit
        if (index($0, heading) == 1) { found = 1; next }
    }
    found { print }
' "$root/CHANGELOG.md" | sed -e '/./,$!d')"

if [ -z "$(printf '%s' "$section" | tr -d '[:space:]')" ]; then
    echo "changelog-section.sh: CHANGELOG.md has no section for $version, or it is empty" >&2
    exit 1
fi

printf '%s\n' "$section"
