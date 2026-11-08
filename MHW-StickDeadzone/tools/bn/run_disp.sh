#!/bin/sh
# usage: tools/bn/run_disp.sh OFFSET [OFFSET...]   (BN must be open on the game DB: bnctl open ...unpacked.exe)
D="$(cd "$(dirname "$0")" && pwd)"
T="$(mktemp)"
{ echo "ARGS = [$(echo "$@" | sed 's/ /,/g')]"; cat "$D/disp.py"; } > "$T"
PYTHONIOENCODING=utf-8 python /c/Projects/tools/binja/bnctl.py exec -f "$T"
rm -f "$T"
