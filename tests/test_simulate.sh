#!/bin/sh
set -eu
export LC_ALL=C
bin=$(realpath "$1")
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
"$bin" 20 42 > "$tmp/a"
"$bin" 20 42 > "$tmp/b"
cmp "$tmp/a" "$tmp/b"
awk -F, 'NR>1 {if ($3 != 20 || $5+$6 != 20 || $12>2000) exit 1; if ($2=="sem_defesa" && $5!=0) exit 1} END {if(NR!=9) exit 1}' "$tmp/a"
if "$bin" 0 42 >/dev/null 2>&1; then exit 1; fi
if "$bin" 1 garbage >/dev/null 2>&1; then exit 1; fi
echo 'OK: simulador reproduzivel, configuracoes, limites e derrota sem defesa.'
