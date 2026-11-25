# Relatório de Balanceamento — Monte Carlo

Data: 2026-11-21. Execução: `make simulate N=1000 SEED=42` (comando idêntico repetido, CSV idêntico). O alvo `simulate` vem do `Makefile` da issue #6 (PR #16); antes desse merge, o mesmo resultado sai com:

```bash
gcc -Iinclude -std=c11 -Wall -Wextra -Wpedantic -Werror tools/simulate.c src/board.c src/game.c -lm -o simulate
./simulate 1000 42
sh tests/test_simulate.sh ./simulate
```

Jogo com 5 ondas (8, 12, 16, 20 e 24 inimigos, com 10, 22, 34, 46 e 58 de vida desde 25/11; antes 10, 26, 42, 58 e 74), torre Y (40 moedas, dano 2, área 3×3, todo passo) e torre Z (60 moedas, dano 5, área 5×5, a cada 2 passos).

## Método

O simulador (`tools/simulate.c`) joga 1.000 partidas por estratégia e configuração com um bot determinístico: compra no máximo uma torre por turno, quando tem moedas, numa posição sorteada entre as 30 células de grama cuja área de tiro cobre mais posições da trilha para aquele tipo de torre. Y e Z sorteiam entre o mesmo número de células, cada uma ranqueada pela própria área. Se todas as células do tipo da vez estiverem ocupadas, a política `misto` compra o outro tipo. O RNG é xorshift semeado por `SEED` e pelo índice da partida, de modo que cada configuração recebe a mesma sequência de sorteios (sementes pareadas). A cada turno o simulador valida o estado com `game_valid` e aborta se qualquer partida travar ou ficar inválida.

Políticas: `sem_defesa` (nenhuma compra), `so_torre_y`, `so_torre_z` e `misto` (alterna Y/Z a cada compra). Configurações: `normal` (vida dos inimigos padrão) e `pressao` (vida dobrada, somente simulação). Intervalos de confiança de 95% pelo método de Wilson.

## Recalibração de 25/11/2026 (vida `12 × onda − 2`)

No primeiro teste em tempo real, um jogador que ainda não conhecia o mapa perdeu todas as partidas, a maioria na onda 4. O bot padrão não mostrava isso porque só sorteia entre as 30 casas que mais cobrem a trilha, ou seja, joga como quem já conhece o mapa. Medimos também um jogador casual, que sorteia entre todas as casas de grama perto da trilha (`-DTOP_CELLS=300`):

```bash
gcc -Iinclude -std=c11 -Wall -Wextra -Wpedantic -Werror -DTOP_CELLS=300 tools/simulate.c src/board.c src/game.c src/persistence.c -lm -o simulate_casual
./simulate_casual 1000 42
```

| jogador (`normal`, N=1000, seed 42) | vida | só Y | só Z | mista |
|---|---|---|---|---|
| casual (qualquer casa) | `16 × onda − 6` (10 a 74) | 26,1% | 3,8% | 14,5% |
| casual (qualquer casa) | `12 × onda − 2` (10 a 58) | 77,6% | 65,6% | 70,1% |
| experiente (30 melhores, `make simulate`) | `12 × onda − 2` (10 a 58) | 100,0% | 100,0% | 100,0% |

Com a curva nova a mista do jogador casual fica em 70,1% (IC95 67,2 a 72,9), dentro da meta de 40% a 95%, e a diferença Y − Z cai para 12 pp. Quem já conhece as boas casas vence sempre. A onda 1 não mudou (10 de vida), então uma torre sozinha ainda segura o começo. Sem defesa continua perdendo 100%, e `pressao` continua sem vitórias.

Também testamos o efeito do tempo real com uma cópia do bot que leva de 2 a 6 segundos para cada compra enquanto o relógio corre (700 ms a 1.400 ms por passo). A velocidade mudou a taxa de vitória em no máximo 3 pontos; o que separava vitória de derrota era a escolha das casas. Por isso a velocidade do jogo não foi alterada.

A sequência de vitória do roteiro (`tests/test_cli.sh`) passou de 830 para 900 pontos: agora elimina os 80 inimigos sem perder vida.

## Resultados anteriores (vida `16 × onda − 6`, 21/11/2026)

| config | estratégia | vitórias | taxa | IC 95% | turnos méd. | min–max | vida méd. | moedas méd. | kills méd. | onda méd. |
|---|---|---|---|---|---|---|---|---|---|---|
| normal | sem_defesa | 0/1000 | 0,0% | 0,0–0,4% | 114,00 | 114–114 | 0,00 | 120,00 | 0,00 | 2,00 |
| normal | so_torre_y | 988/1000 | 98,8% | 97,9–99,3% | 347,97 | 105–412 | 9,83 | 39,56 | 79,14 | 4,97 |
| normal | so_torre_z | 838/1000 | 83,8% | 81,4–86,0% | 346,59 | 168–432 | 7,96 | 36,55 | 71,74 | 4,77 |
| normal | misto | 981/1000 | 98,1% | 97,1–98,8% | 354,91 | 183–426 | 9,67 | 20,38 | 78,89 | 4,97 |
| pressao | sem_defesa | 0/1000 | 0,0% | 0,0–0,4% | 114,00 | 114–114 | 0,00 | 120,00 | 0,00 | 2,00 |
| pressao | so_torre_y | 0/1000 | 0,0% | 0,0–0,4% | 125,37 | 107–138 | 0,00 | 22,66 | 7,51 | 2,00 |
| pressao | so_torre_z | 0/1000 | 0,0% | 0,0–0,4% | 137,77 | 112–191 | 0,00 | 22,97 | 9,91 | 2,18 |
| pressao | misto | 0/1000 | 0,0% | 0,0–0,4% | 130,62 | 112–185 | 0,00 | 18,00 | 7,75 | 2,01 |

As partidas de `so_torre_y` variam de verdade: o bot não chega a ocupar as 30 células, as moedas finais mudam entre partidas e 12 delas terminam em derrota.

### Sensibilidade ao número de células sorteadas (`normal`, N=1000, seed 42)

| regra de células | Y | Z | misto | Y − Z |
|---|---|---|---|---|
| cobertura ≥ 6 (rodada anterior: 13 células para A, 39 para B) | 100,0% | 48,2% | 92,3% | 51,8 pp |
| 13 melhores por tipo | 100,0% | 86,9% | 100,0% | 13,1 pp |
| 20 melhores por tipo | 100,0% | 94,9% | 100,0% | 5,1 pp |
| **30 melhores por tipo (padrão)** | **98,8%** | **83,8%** | **98,1%** | **15,0 pp** |
| 39 melhores por tipo | 89,9% | 47,6% | 83,8% | 42,3 pp |

Linhas alternativas: `gcc ... -DTOP_CELLS=<k> tools/simulate.c ...`. A diferença entre Y e Z depende muito de quantas células fracas o bot aceita, então ela mede tanto o bot quanto as torres.

## Critérios obrigatórios (definidos antes da simulação)

| critério | resultado |
|---|---|
| Zero partidas presas em 2000 turnos | OK — máximo observado foi 432 passos (`max_turns`, verificado em `tests/test_simulate.sh`) |
| Estado válido em todas as rodadas | OK — nenhum aborto do simulador |
| Sem defesa perde 100% | OK — 0/1000 nas duas configurações |
| Uma sequência conhecida de posições vence | OK — a sequência do roteiro de demonstração vence em `tests/test_cli.sh` (PR #16, procura "VITORIA"); eram 830 pontos com a vida antiga e são 900 desde 25/11 |
| Semente repetida produz CSV idêntico | OK — execuções idênticas |

## Indicadores exploratórios (metas registradas antes dos resultados)

1. **Mista entre 40% e 95%**: `normal` ficou em **98,1% — falha de meta registrada** (acima da faixa). `pressao` ficou em **0,0% — falha de meta registrada**, sem ajuste de faixa.
2. **Dominância entre Y e Z**: diferença de taxa de vitória acima de 20 pontos percentuais indica dominância. Em `normal`, Y = 98,8% e Z = 83,8% (**diferença de 15,0 pp — sinal ausente**).
3. **Desvio do protocolo**: `tests/balance-baseline.md` definia as células úteis como "cobre ao menos 6 posições da trilha". Esse corte favorecia A (13 células nobres) contra B (39, muitas na borda do alcance), então foi trocado pelas 30 melhores células de cada tipo depois da rodada anterior. A baseline foi mantida como registrada; a tabela de sensibilidade mostra as duas regras.

## Conclusões

- **Histórico da calibração (10/11/2026).** Depois das mudanças pedidas no playtest (inimigo anda 1 casa por passo, sem limite de torres, ondas maiores), todas as políticas com torres venciam 100% em `normal` — fora da meta da mista. Reduzir a recompensa por eliminação (8 → 4) não resolveu (`so_torre_y` seguiu em 100%). A vida dos inimigos foi o ajuste que moveu o resultado. A primeira tentativa (`8 + 10 × onda`, 18 a 58) deixou a mista em 81,7%, mas no playtest uma torre sozinha não eliminava ninguém na onda 1 e o jogador concluiu que as torres não atacavam. A curva final começa em 10 e cresce mais rápido (`16 × onda − 6`: 10, 26, 42, 58 e 74): uma torre Y elimina a onda 1 e, com o bot antigo, a mista ficava em 92,3%. O inimigo passou a aparecer como `o` quando ferido, para o dano ficar visível. A torre Z passou de 65 para 60 moedas e de 3 para 2 passos de cadência para ser uma escolha viável (numa rodada exploratória de 300 partidas com os valores antigos, `so_torre_z` vencia 42% contra 100% da Y).
- **A não domina B com o bot corrigido** (15,0 pp). A dominância de 51,8 pp da rodada anterior vinha do filtro de células, não das torres. A ainda entrega 2/40 = 0,050 de dano por moeda por passo, contra (5/2)/60 ≈ 0,042 de B, e B desperdiça tiros quando o inimigo sai da área entre um disparo e outro.
- **A mista ficou fácil demais em `normal`** (98,1%, acima da meta de 95%). Com posições razoáveis, misturar torres quase sempre vence. Qualquer ajuste novo deve ser avaliado com a mesma simulação pareada.
- **`pressao` não é vencível pelo bot.** Com vida dos inimigos dobrada, nenhuma política vence. A configuração não é acessível no jogo; serve de régua para decidir mudanças futuras.
- **O simulador joga por turnos.** No terminal o jogo acelera a cada onda (700 ms → 300 ms por passo) e o jogador precisa de tempo para digitar; essa pressão não aparece aqui. O bot compra no máximo uma torre por turno, na primeira posição livre sorteada entre as melhores. É o piso de habilidade, não o teto.
- **Limitações**: Monte Carlo amostra decisões do bot, não de humanos; repetir uma estratégia fixa não substitui teste humano. Os números são evidência quantitativa de balanceamento, não prova de equilíbrio nem critério de aprovação da entrega acadêmica.
