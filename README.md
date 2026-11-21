# Tower Defense em C

> Trabalho final de Laboratório de Programação em C — equipe de três integrantes. O Spec-Kit é usado localmente e não é publicado.

## Status

Jogo completo jogável no terminal: mapa colorido **Jardim de Pedra** (13×16, gramado, árvores na borda e trilha de pedra com curvas), duas torres selecionáveis com custo/área/dano distintos, cinco ondas de inimigos cada vez maiores e mais rápidas, economia de moedas, vida da base, telas de vitória e derrota, salvamento e carregamento de partida, ranking em arquivo, suíte automatizada de testes com sanitizadores e simulador de balanceamento Monte Carlo. No terminal, o jogo roda em tempo real (os inimigos andam sozinhos, `p` pausa); com entrada redirecionada, roda por turnos para ser determinístico. A entrega e os checkpoints estão em `CONTRIBUICOES.md`; o balanceamento medido está em `tests/balance-report.md`.

## Pré-requisitos

- Linux
- GCC
- Make
- Git

Em Ubuntu/Debian, GCC e Make fazem parte de `build-essential`. Não é preciso biblioteca gráfica.

## Baixar o projeto

```bash
git clone https://github.com/pedrolucazx/tower-defense.git
cd tower-defense
```

## Fluxo de trabalho para iniciantes

Depois de clonar, configure sua identidade uma vez:

```bash
git config --global user.name "Seu Nome"
git config --global user.email "seu-email@example.com"
```

Antes de começar uma tarefa:

```bash
git switch main
git pull origin main
git switch -c feature/nome-da-issue
```

Depois de trabalhar:

```bash
git status
git add caminho/do/arquivo
git commit -m "Descrição curta da mudança"
git push -u origin feature/nome-da-issue
```

No GitHub, abra um Pull Request da sua branch para `main`, vincule a issue e peça revisão a outro integrante. Não faça commit diretamente em `main`.

Depois que o Pull Request for aprovado e integrado:

```bash
git switch main
git pull origin main
```

Se aparecer conflito, não apague o trabalho de outra pessoa: abra os arquivos marcados, escolha conscientemente o conteúdo final, execute os testes e só então faça o commit da resolução.

## Comandos do projeto

Na pasta do projeto (terminal com pelo menos 84 colunas; cores ANSI de 256 tons):

```bash
make            # compila
make run        # compila e abre o jogo
make test       # compila e executa toda a suíte automatizada
make sanitize   # suíte completa com AddressSanitizer + UBSan
make simulate N=1000 SEED=42   # 1000 partidas simuladas com seed 42
make clean      # apaga build/
```

`make test` verifica trilha, posicionamento das duas torres, compra, economia, combate, ondas, vitória, derrota, save/load, ranking, cores e entradas inválidas. `make simulate` é o simulador em lote (bot determinístico, sem terminal): repete a mesma semente e compara estratégias em CSV — taxa de vitória, intervalos de confiança, turnos, vida, moedas, eliminações e onda. O jogo e o simulador são programas diferentes: o primeiro é interativo, o segundo é evidência quantitativa de balanceamento.

Opções do jogo:

```bash
./build/tower-defense --color        # força cores se a detecção falhar
./build/tower-defense --no-color     # sem cores: terreno desenhado com símbolos
./build/tower-defense --snapshot     # imprime o mapa uma vez e sai
./build/tower-defense --play         # pula o menu e inicia a partida
./build/tower-defense --help
```

Redirecionar a saída ou definir `NO_COLOR=1` desativa cores automaticamente; `--color` pode forçá-las. Entre opções explícitas, vale a última. Em terminais com cor exata (truecolor: GNOME Terminal, Konsole, kitty, VS Code), que anunciam `COLORTERM=truecolor`, o mapa usa a paleta "Floresta funda"; nos demais, usa a paleta de 256 cores. Se o seu terminal suporta cor exata mas não anuncia, rode `COLORTERM=truecolor make run`.

## Como jogar

Menu inicial: `1` Iniciar, `2` Instrucoes, `3` Carregar, `4` Recorde, `5` Sair, `6` Continuar sessao.

Comandos dentro da partida:

- `a 4 10`: comprar a torre A (azul) na linha 4, coluna 10.
- `b 8 12`: comprar a torre B (violeta) na linha 8, coluna 12.
- `4 10` ou `04x10`: atalho que compra a torre A.
- `n`: avançar o jogo um passo manualmente (opcional — no tempo real o jogo anda sozinho).
- `p`: pausar / retomar o tempo (só no terminal).
- `s` / `l`: salvar / carregar a partida (`savegame.dat`).
- `r`: reiniciar a partida do zero (descartando a sessão atual).
- `m`: voltar ao menu. `h`: ajuda. `q`: sair (Ctrl+C ou Ctrl+D também encerram e restauram o terminal).

Linhas vão de `00` a `12` e colunas de `00` a `15`; o canto superior esquerdo é `0 0`. Torres só podem ocupar grama livre; pedra, entrada `>>`, base `##`, árvores e casas com torre são recusadas com o motivo. No modo colorido, pedra (cinza) e árvores (verde) são identificadas pela cor; com `--no-color` aparecem como `::` e `^^`.

| torre | custo | dano | área de tiro | cadência |
|---|---|---|---|---|
| A | 40 | 2 | 3×3 em volta da torre | todo passo |
| B | 60 | 5 | 5×5 em volta da torre | a cada 2 passos |

Cada torre mira o inimigo mais avançado da trilha dentro da sua área. O inimigo aparece como `O` com vida cheia e como `o` depois de ferido. Não há limite de torres: compre enquanto tiver moedas. Você começa com 120 moedas e 10 vidas; cada eliminação rende 8 moedas e 10 pontos; vitória rende 100. São cinco ondas de 8, 12, 16, 20 e 24 inimigos, com 10, 26, 42, 58 e 74 de vida. Vence ao sobreviver às cinco ondas; perde quando a vida chega a zero (cada inimigo que chega à base custa 1). No tempo real, o jogo dá um passo a cada 700 ms na onda 1 e acelera 100 ms por onda (300 ms na onda 5); um inimigo entra a cada 3 passos e anda 1 casa por passo. Comprar torre não adianta o tempo; erros de comando não alteram nada.

## Arquitetura

- `include/board.h`, `src/board.c` — tabuleiro fixo 13×16, trilha ordenada, posicionamento de torres.
- `include/game.h`, `src/game.c` — estado do jogo, economia, ondas, combate, condições finais e auditoria de invariantes (`game_valid`).
- `src/render.c` — desenho colorido do mapa (e modo texto).
- `src/ui.c`, `src/main.c` — menu, comandos, validação de entrada e opções de linha de comando.
- `src/persistence.c` — save textual versionado com escrita atômica e ranking.
- `tests/` — testes automatizados (`test_board.c`, `test_game.c`, `test_persistence.c`, `test_ui.c`, `test_render.c`, `test_cli.sh`, `test_simulate.sh`), roteiro manual com evidência e relatório de balanceamento.
- `tools/simulate.c` — simulador Monte Carlo em lote.
- `.github/workflows/ci.yml` — CI: compila e roda `make test` em todo Pull Request e push na `main`.

## Roteiro de demonstração (20 min + 5 de explicação)

1. `make` e `make run`: abrir o menu e mostrar as instruções (`2`).
2. Partida com vitória: `p` para pausar, comprar `a 4 10`, `a 6 10` e `b 8 12`; retomar com `p` e, conforme as moedas chegam, completar com `a 4 12`, `b 6 12`, `a 8 8`, `b 4 7`, `a 8 9`, `b 10 10`, `a 3 10` até a tela de VITÓRIA (830 pontos) — depois mostrar o recorde (`4`).
3. No meio de uma nova partida, `p` pausar, `s` salvar, `m` menu, `3` carregar: mostrar continuidade exata do estado.
4. Partida sem defesa: deixar o tempo correr e mostrar a tela de DERROTA por vida zero.
5. `make test` e `make simulate N=1000 SEED=42`: mostrar a suíte passando e a tabela de balanceamento.
6. Explicação técnica: trilha ordenada como vetor de posições, área quadrada e escolha de alvo, ordem do passo, tempo real com modo por turnos determinístico para testes, save atômico validado e simulação pareada.

## Organização da equipe

- Integrante A: mapa, interface e menus.
- Integrante B: entidades, torres, inimigos, ondas e combate.
- Integrante C: testes automatizados, CLI, balanceamento Monte Carlo, CI e integração.

Todos devem revisar código de colega e compreender o funcionamento geral para a defesa.
