#!/bin/sh
# Build Diggers from your own copy of the game:
#     ./build.sh "Diggers (1994)(Millenium)(Disk 1 of 3).adf" "...(Disk 2 of 3).adf" "...(Disk 3 of 3).adf"
# then: cd recomp && ./diggers
# The steps are recipe.json's "build", run by SuperSpider's tools/weave.py.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
SUPERSPIDER="${SUPERSPIDER:-$HOME/stuff/superspider}"
[ $# -gt 0 ] || { echo "usage: $0 DISK1.adf DISK2.adf DISK3.adf" >&2; exit 2; }
(cd "$SUPERSPIDER" && dune build)
WEAVE_RECIPES="$HERE" exec python3 -I "$SUPERSPIDER/tools/weave.py" build "$@"
