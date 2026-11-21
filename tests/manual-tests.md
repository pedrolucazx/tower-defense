# Testes Manuais — Tower Defense em C

Os fluxos abaixo foram executados contra `./build/tower-defense` em 2026-11-10 e repetidos em 2026-11-21 com o código integrado na `main`. Os fluxos cobertos pela suíte automatizada citam o teste que os reproduz.

Ambiente: Linux, GCC, Make. Terminal de pelo menos 84 colunas para o mapa.

## Gate de compilação e sanitizadores

| comando | data | resultado |
|---|---|---|
| `make clean && make` | 2026-11-21 | compila com `-std=c11 -Wall -Wextra -Wpedantic -Werror` sem warnings |
| `make test` | 2026-11-21 | OK: board, game, persistência, UI, render, CLI e simulador |
| `make sanitize` | 2026-11-21 | OK: mesma suíte com AddressSanitizer + UBSan |

## Testes do roteiro mínimo

### 1. Menu e instruções

Entrada: `2` (Instrucoes), depois `5` (Sair). Esperado: tela de instruções com objetivo, torres, comandos, economia e condições finais. Obtido: "Defenda a base (##) por cinco ondas…", "Torre A: $40, dano 2, area 3x3, atira todo passo." e "Torre B: $60, dano 5, area 5x5, atira a cada 2 passos.". Evidência: capturada em 2026-11-10; também coberto por `tests/test_cli.sh` (procura "Defenda a base").

### 2. Opção inexistente não altera estado

Entrada: `9` no menu. Esperado: "Comando invalido." e estado preservado. Obtido: "Comando invalido." — o menu permaneceu exibindo as mesmas 6 opções. Evidência: capturada em 2026-11-10.

### 3. Posições inválidas de torre

Entrada em `--play`: `a -1 0` (fora do mapa), `a 0 0` (árvore), `a 5 0` (entrada), `b 5 5` (pedra), `a 4 4 9` (argumento a mais). Esperado: rejeição com o motivo, sem gastar moedas. Obtido: "Coordenada fora do mapa: linha 00-12, coluna 00-15." para `-1 0`; "Casa ocupada: torre so em grama livre (nao em pedra ou arvore)." para árvore, entrada e pedra; mensagem de uso do comando para `a 4 4 9`; o placar seguiu em `Moedas 120`. Evidência: capturada em 2026-11-10; coberto também por `tests/test_ui.c` e `tests/test_cli.sh`.

### 4. Compra das duas torres e economia

Entrada em `--play`: `a 4 4`, `b 2 2`, `a 1 1`. Esperado: A custa 40, B custa 60, compra sem saldo recusada. Obtido: "Torre A comprada." (120→80), "Torre B comprada." (80→20) e "Moedas insuficientes: a torre A custa $40." com o saldo mantido em 20. Evidência: capturada em 2026-11-10.

### 5. Vitória na quinta onda

Entrada: `a 4 10`, `a 6 10`, `b 8 12`, `a 4 12`, `b 6 12`, `a 8 8`, `b 4 7`, `a 8 9`, `b 10 10`, `a 3 10` repetidos entre passos (`n`), comprando conforme as moedas chegam. Esperado: VITÓRIA após a onda 5. Obtido: tela "VITORIA! O jardim resistiu as 5 ondas." com "Pontos 830 | Eliminados 73 | Vidas restantes 3/10". Evidência: capturada em 2026-11-10; reproduzido em `tests/test_cli.sh` (procura "VITORIA").

### 6. Derrota por vida zero

Entrada: 400× `n` sem defesa. Esperado: DERROTA quando a vida chega a 0. Obtido: tela "DERROTA! A base caiu na onda 2/5." com "Pontos 0 | Eliminados 0 | Chegaram a base 10". Comandos após o fim (`n`) respondem "Partida encerrada. Use r, m ou q." sem alterar estado. Evidência: capturada em 2026-11-10; reproduzido em `tests/test_cli.sh`.

### 7. Salvar no meio da partida e carregar

Entrada: `1` (Iniciar), `s`, `n`, `l`, `m`, `3` (Carregar). Esperado: "Partida salva." e "Partida carregada." nos dois caminhos de carregamento. Obtido: as três mensagens apareceram na ordem esperada. A continuidade exata do estado (tabuleiro, torres, inimigos, recurso, vida, onda e passo) é verificada byte a byte em `tests/test_persistence.c`, que salva, carrega e compara o estado a cada passo por 50 passos. Evidência: capturada em 2026-11-10.

### 8. Save ausente ou corrompido

Entrada: gravar texto arbitrário em `savegame.dat` e escolher `3` (Carregar). Esperado: erro recuperável sem fechar o programa nem alterar o estado ativo. Obtido: "Save ausente ou invalido; estado preservado." — o menu permaneceu funcional. Coberto também por `tests/test_persistence.c` (arquivo ausente ou vazio, versão inválida, número gigante, vetor truncado e lixo anexado ao fim). Evidência: capturada em 2026-11-10.

### 9. Ranking após o fim da partida

Entrada: vencer uma partida (item 5) e escolher `4` (Recorde). Esperado: recorde lido do arquivo. Obtido: "Recorde: 830 pontos". O registro é gravado automaticamente quando a partida sai de PLAYING; `tests/test_cli.sh` confirma "Recorde: 0 pontos" em arquivo vazio e `tests/test_persistence.c` confirma gravação/leitura do melhor score. Evidência: capturada em 2026-11-10.

### 10. Reiniciar partida

Entrada em `--play`: `a 4 4`, `r`. Esperado: "Partida reiniciada." e estado volta aos valores iniciais. Obtido: "Partida reiniciada." e placar de volta a `Onda 1/5 | Vida 10/10 | Moedas 120 | Pontos 0`. Evidência: capturada em 2026-11-10.

### 11. Demonstração final (a executar em 25/11/2026)

Roteiro de até 20 minutos + 5 de explicação: compilar (`make`), abrir o menu e as instruções, jogar uma vitória (sequência do item 5), mostrar save/load no meio, provocar uma derrota sem defesa, abrir o recorde e encerrar. A explicação técnica cobre: trilha ordenada, área de tiro quadrada e escolha de alvo, ordem determinística do passo, save textual versionado com escrita atômica e o simulador Monte Carlo.

### 12. Alteração individual demonstrável (a executar nos checkpoints)

Cada integrante prepara uma mudança simples com teste que comprova o efeito, em 10–15 minutos:

- **Integrante A (interface)**: alterar a mensagem de boas-vindas do menu em `src/main.c`; prova: `printf '5\n' | ./build/tower-defense --no-color` mostra a nova mensagem; `make test` continua OK.
- **Integrante B (núcleo)**: alterar `TOWER_STATS[1].damage` de 5 para 4 em `src/game.c`; prova: `make test` falha em `test-game` na asserção do fixture da torre B (espera `enemies[0].hp == 5` após o primeiro tiro) — restaurar depois.
- **Integrante C (qualidade)**: acrescentar uma asserção a `tests/test_game.c` (ex.: a torre A não atinge um inimigo a 2 casas de distância); prova: a nova asserção passa e uma versão com a área errada falha.

## Cobertura de requisitos

Os testes 1–6 cobrem os requisitos R1–R9 (compra, ataque, ondas, vitória, derrota, entradas inválidas); 7–10 cobrem persistência e integração (R6–R8); 11–12 cobrem a defesa. A suíte automatizada (`make test`) reproduz todos os fluxos de terminal sem interação humana.

## 13. Tempo real no terminal

Em um terminal (TTY), o jogo corre sozinho: um passo a cada 700 ms na onda 1, 100 ms mais rápido a cada onda; inimigos entram a cada 3 passos e andam 1 casa por passo; `p` pausa e retoma; a compra vale na hora e não adianta o tempo. Teclas especiais (setas, F1–F12, colar) são descartadas inteiras em vez de virarem texto, e o Backspace apaga o que foi digitado. Evidência em 2026-11-10 via pseudo-terminal (`script`): `b 4 10` comprou a torre B e ela apareceu no mapa; setas e F1 misturadas a `4 10` resultaram na compra em `04×10` sem caracteres extras; `4 9`, Backspace, `10` comprou em `04×10`; Ctrl+C encerrou com "Ate logo" e o terminal foi restaurado. Com entrada redirecionada (não-TTY), o modo por turnos é mantido para que testes e scripts sejam determinísticos.

