#!/bin/sh
# usage: tools/bn/run_dti.sh CLASS [CLASS...]   (BN must be open on the game DB: bnctl open ...unpacked.exe)
D="$(cd "$(dirname "$0")" && pwd)"
T="$(mktemp)"
{ printf 'ARGS = ['; for a in "$@"; do printf '"%s",' "$a"; done; echo ']'; cat "$D/dti.py"; } > "$T"
PYTHONIOENCODING=utf-8 python /c/Projects/tools/binja/bnctl.py exec -f "$T"
rm -f "$T"
