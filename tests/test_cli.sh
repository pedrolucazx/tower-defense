#!/bin/sh
set -eu
export LC_ALL=C
bin=$(realpath "${1:-./build/tower-defense}")
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 1' HUP INT TERM
cd "$tmp"
esc=$(printf '\033')

has() {
    f=$1; shift
    for p; do grep -q -- "$p" "$f" || { echo "FAIL: '$p' ausente em $f"; exit 1; }; done
}
lacks() {
    f=$1; shift
    for p; do if grep -q -- "$p" "$f"; then echo "FAIL: '$p' presente em $f"; exit 1; fi; done
}
play() { out=$1; shift; "$bin" "$@" > "$out"; }

play plain --snapshot
lacks plain "$esc"
has plain 'JARDIM DE PEDRA' '##'
play color --color --snapshot
has color "$esc"
play override --color --no-color --snapshot
cmp plain override
NO_COLOR=1 "$bin" --snapshot > no-color
cmp plain no-color
if "$bin" --invalido > out 2>&1; then echo 'FAIL: opcao invalida aceita'; exit 1; fi

printf 'a 4 4\nb 4 4\nb 5 5\nb 2 2\na 1 1\na -1 0\na 999999999999999999999999 0\nn extra\nn\nr\nq\n' | play commands --play --no-color
has commands 'Torre A comprada' 'Torre B comprada' 'Casa ocupada' 'Moedas insuficientes' \
    'Coordenada fora do mapa' 'Comando invalido' '1/8 entraram' 'Partida reiniciada' 'Ate logo'
awk 'BEGIN {for (i=0;i<300;i++) printf "a"; printf "\nq\n"}' | play long --play
has long 'Comando muito longo' 'Ate logo'
play eof < /dev/null
has eof 'Ate logo'
printf q | play no-newline --play
has no-newline 'Ate logo'
printf 'n\000lixo\nq\n' | play nul --play
has nul 'byte invalido' 'Ate logo'
lacks nul '1/8 entraram'
printf 'n\000lixo' | play nul-eof --play
has nul-eof 'byte invalido'
lacks nul-eof '1/8 entraram'

if script --version 2>/dev/null | grep -q util-linux; then
    pty() { env "$@" BIN="$bin" TERM=xterm-256color script -q -e -c '"$BIN" --snapshot' /dev/null; }
    pty -u NO_COLOR > tty
    has tty "$esc"
    pty NO_COLOR=1 > tty-plain
    lacks tty-plain "$esc"
else
    echo 'SKIP: teste PTY requer script (util-linux).'
fi

printf '2\n1\ns\nn\nl\nm\n3\nq\n' | play menu
has menu 'Defenda a base' 'Partida salva' 'Partida carregada'
awk 'BEGIN {for (i=0;i<400;i++) print "n"; print "q"}' | play defeat --play
has defeat 'DERROTA'
printf '4\n5\n' | play rank
has rank 'Recorde: 0 pontos'
awk 'BEGIN {
    split("a 4 10,a 6 10,b 8 12,a 4 12,b 6 12,a 8 8,b 4 7,a 8 9,b 10 10,a 3 10", p, ",");
    for (r = 0; r < 400; r++) { for (i = 1; i <= 10; i++) print p[i]; print "n"; }
    print "q";
}' | play win --play
has win 'VITORIA'
echo 'OK: CLI, menu, cores, save/load, recorde, vitoria, derrota, invalidos e EOF.'
