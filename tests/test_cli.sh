#!/bin/sh
set -eu
bin=$(realpath "${1:-./build/tower-defense}")
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cd "$tmp"
"$bin" --snapshot > "$tmp/plain"
esc=$(printf '\033')
if grep -q "$esc" "$tmp/plain"; then echo 'FAIL: ANSI em redirecionamento'; exit 1; fi
grep -q 'JARDIM DE PEDRA' "$tmp/plain"
grep -q '##' "$tmp/plain"
"$bin" --color --snapshot > "$tmp/color"
grep -q "$esc" "$tmp/color"
"$bin" --color --no-color --snapshot > "$tmp/override"
cmp "$tmp/plain" "$tmp/override"
NO_COLOR=1 "$bin" --snapshot > "$tmp/no-color"
cmp "$tmp/plain" "$tmp/no-color"
if "$bin" --invalido > "$tmp/out" 2>&1; then echo 'FAIL: opcao invalida aceita'; exit 1; fi
printf 'a 4 4\nb 4 4\nb 5 5\nb 2 2\na 1 1\na -1 0\na 999999999999999999999999 0\nn extra\nn\nr\nq\n' | "$bin" --play --no-color > "$tmp/commands"
grep -q 'Torre A comprada' "$tmp/commands"
grep -q 'Torre B comprada' "$tmp/commands"
grep -q 'Casa ocupada' "$tmp/commands"
grep -q 'Moedas insuficientes' "$tmp/commands"
grep -q 'Coordenada fora do mapa' "$tmp/commands"
grep -q 'Comando invalido' "$tmp/commands"
grep -q '1/8 entraram' "$tmp/commands"
grep -q 'Partida reiniciada' "$tmp/commands"
grep -q 'Ate logo' "$tmp/commands"
awk 'BEGIN {for (i=0;i<300;i++) printf "a"; printf "\nq\n"}' | "$bin" --play > "$tmp/long"
grep -q 'Comando muito longo' "$tmp/long"
grep -q 'Ate logo' "$tmp/long"
"$bin" < /dev/null > "$tmp/eof"
printf q | "$bin" --play > "$tmp/no-newline"
grep -q 'Ate logo' "$tmp/no-newline"
printf 'n\000lixo\nq\n' | "$bin" --play > "$tmp/nul"
grep -q 'byte invalido' "$tmp/nul"
grep -q 'Ate logo' "$tmp/nul"
if grep -q '1/8 entraram' "$tmp/nul"; then echo 'FAIL: NUL avancou estado'; exit 1; fi
printf 'n\000lixo' | "$bin" --play > "$tmp/nul-eof"
grep -q 'byte invalido' "$tmp/nul-eof"
if grep -q '1/8 entraram' "$tmp/nul-eof"; then echo 'FAIL: NUL EOF avancou estado'; exit 1; fi
if command -v script >/dev/null 2>&1; then
    env -u NO_COLOR TERM=xterm-256color script -q -e -c "$bin --snapshot" /dev/null > "$tmp/tty"
    grep -q "$esc" "$tmp/tty"
    NO_COLOR=1 TERM=xterm-256color script -q -e -c "$bin --snapshot" /dev/null > "$tmp/tty-plain"
    if grep -q "$esc" "$tmp/tty-plain"; then echo 'FAIL: NO_COLOR em TTY'; exit 1; fi
else
    echo 'SKIP: teste PTY requer script (util-linux).'
fi
printf '2\n1\ns\nn\nl\nm\n3\nq\n' | "$bin" > "$tmp/menu"
grep -q 'Defenda a base' "$tmp/menu"
grep -q 'Partida salva' "$tmp/menu"
grep -q 'Partida carregada' "$tmp/menu"
awk 'BEGIN {for (i=0;i<400;i++) print "n"; print "q"}' | "$bin" --play > "$tmp/defeat"
grep -q 'DERROTA' "$tmp/defeat"
printf '4\n5\n' | "$bin" > "$tmp/rank"
grep -q 'Recorde: 0 pontos' "$tmp/rank"
awk 'BEGIN {
    split("a 4 10,a 6 10,b 8 12,a 4 12,b 6 12,a 8 8,b 4 7,a 8 9,b 10 10,a 3 10", p, ",");
    for (r = 0; r < 400; r++) { for (i = 1; i <= 10; i++) print p[i]; print "n"; }
    print "q";
}' | "$bin" --play > "$tmp/win"
grep -q 'VITORIA' "$tmp/win"
echo 'OK: CLI, menu, cores, save/load, recorde, vitoria, derrota, invalidos e EOF.'
