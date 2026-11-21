# Relatório de Balanceamento — Monte Carlo

Data: 2026-11-10. Execução: `make simulate N=1000 SEED=42` (comando idêntico repetido, CSV idêntico). Jogo com 5 ondas (8, 12, 16, 20 e 24 inimigos, com 10, 26, 42, 58 e 74 de vida), torre A (40 moedas, dano 2, área 3×3, todo passo) e torre B (60 moedas, dano 5, área 5×5, a cada 2 passos).

## Método

O simulador (`tools/simulate.c`) joga 1.000 partidas por estratégia e configuração com um bot determinístico: enquanto tiver moedas, compra torres do tipo da política em posições aleatórias sorteadas entre as células de grama cuja área de tiro cobre ao menos 6 posições da trilha. O RNG é xorshift semeado por `SEED` e pelo índice da partida, de modo que cada configuração recebe a mesma sequência de sorteios (sementes pareadas). A cada turno o simulador valida o estado com `game_valid` e aborta se qualquer partida travar ou ficar inválida.

Políticas: `sem_defesa` (nenhuma compra), `so_torre_a`, `so_torre_b` e `misto` (alterna A/B a cada compra). Configurações: `normal` (vida dos inimigos padrão) e `pressao` (vida dobrada, somente simulação). Intervalos de confiança de 95% pelo método de Wilson.

## Resultados

| config | estratégia | vitórias | taxa | IC 95% | turnos méd. | min–max | vida méd. | moedas méd. | kills méd. | onda méd. |
|---|---|---|---|---|---|---|---|---|---|---|
| normal | sem_defesa | 0/1000 | 0,0% | 0,0–0,4% | 114,00 | 114–114 | 0,00 | 120,00 | 0,00 | 2,00 |
| normal | so_torre_a | 1000/1000 | 100,0% | 99,6–100,0% | 350,23 | 318–370 | 10,00 | 240,00 | 80,00 | 5,00 |
| normal | so_torre_b | 482/1000 | 48,2% | 45,1–51,3% | 300,19 | 127–439 | 4,39 | 32,81 | 51,94 | 4,16 |
| normal | misto | 923/1000 | 92,3% | 90,5–93,8% | 357,19 | 183–433 | 8,93 | 21,77 | 75,75 | 4,88 |
| pressao | sem_defesa | 0/1000 | 0,0% | 0,0–0,4% | 114,00 | 114–114 | 0,00 | 120,00 | 0,00 | 2,00 |
| pressao | so_torre_a | 0/1000 | 0,0% | 0,0–0,4% | 122,50 | 107–136 | 0,00 | 26,64 | 8,33 | 2,00 |
| pressao | so_torre_b | 0/1000 | 0,0% | 0,0–0,4% | 133,65 | 112–191 | 0,00 | 26,65 | 8,69 | 2,08 |
| pressao | misto | 0/1000 | 0,0% | 0,0–0,4% | 127,32 | 110–140 | 0,00 | 42,38 | 7,43 | 2,00 |

## Critérios obrigatórios (definidos antes da simulação)

| critério | resultado |
|---|---|
| Zero partidas presas em 2000 turnos | OK — máximo observado foi 441 passos |
| Estado válido em todas as rodadas | OK — nenhum aborto do simulador |
| Sem defesa perde 100% | OK — 0/1000 nas duas configurações |
| Uma sequência conhecida de posições vence | OK — a sequência do roteiro de demonstração vence com 830 pontos e a política `so_torre_a` vence 100% |
| Semente repetida produz CSV idêntico | OK — execuções idênticas |

## Indicadores exploratórios (metas registradas antes dos resultados)

1. **Mista entre 40% e 95%**: `normal` ficou em **92,3% — dentro da meta**. `pressao` ficou em **0,0% — falha de meta registrada**, sem ajuste de faixa.
2. **Dominância entre A e B**: diferença de taxa de vitória acima de 20 pontos percentuais indica dominância. Em `normal`, A = 100,0% e B = 48,2% (**diferença de 51,8 pp — sinal presente**, menor que os 77,9 pp da rodada anterior).

## Conclusões

- **Histórico da calibração (10/11/2026).** Depois das mudanças pedidas no playtest (inimigo anda 1 casa por passo, sem limite de torres, ondas maiores), todas as políticas com torres venciam 100% em `normal` — fora da meta da mista. Reduzir a recompensa por eliminação (8 → 4) não resolveu (`so_torre_a` seguiu em 100%). A vida dos inimigos foi o ajuste que moveu o resultado. A primeira tentativa (`8 + 10 × onda`, 18 a 58) deixou a mista em 81,7%, mas no playtest uma torre sozinha não eliminava ninguém na onda 1 e o jogador concluiu que as torres não atacavam. A curva final começa em 10 e cresce mais rápido (`16 × onda − 6`: 10, 26, 42, 58 e 74): uma torre A elimina a onda 1 e a mista fica em 92,3%, dentro da meta. O inimigo passou a aparecer como `o` quando ferido, para o dano ficar visível. A torre B passou de 65 para 60 moedas e de 3 para 2 passos de cadência para ser uma escolha viável (numa rodada exploratória de 300 partidas com os valores antigos, `so_torre_b` vencia 42% contra 100% da A).
- **A ainda domina B** (51,8 pp). A entrega 2/40 = 0,050 de dano por moeda por passo; B entrega (5/2)/60 ≈ 0,042, mas desperdiça tiros quando o inimigo sai da área entre um disparo e outro. A diferença caiu bastante; novas mudanças devem ser avaliadas com a mesma simulação pareada.
- **`pressao` não é vencível pelo bot.** Com vida dos inimigos dobrada, nenhuma política vence. A configuração não é acessível no jogo; serve de régua para decidir mudanças futuras.
- **O simulador joga por turnos.** No terminal o jogo acelera a cada onda (700 ms → 300 ms por passo) e o jogador precisa de tempo para digitar; essa pressão não aparece aqui. O bot compra na primeira posição útil sorteada — é o piso de habilidade, não o teto.
- **Limitações**: Monte Carlo amostra decisões do bot, não de humanos; repetir uma estratégia fixa não substitui teste humano. Os números são evidência quantitativa de balanceamento, não prova de equilíbrio nem critério de aprovação da entrega acadêmica.
