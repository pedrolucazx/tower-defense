# Tower Defense em C

> Trabalho final de Laboratório de Programação em C — equipe de três integrantes.

## Status

O módulo do mapa (issue #1) já está na `main`: tabuleiro 13×16, rota dos inimigos e um teste automatizado. O resto do jogo está dividido nas issues #2 a #8 do GitHub. O Spec-Kit é usado localmente para planejamento e não é publicado neste repositório.

## Pré-requisitos

- Linux
- GCC
- Git
- Make (só a partir da issue #6)

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

## Rodar o que já existe na main

Por enquanto não há `Makefile` nem executável do jogo. O que dá para compilar e rodar é o teste do mapa, direto com o `gcc`, na pasta do projeto:

```bash
gcc -Iinclude -std=c11 -Wall -Wextra -Wpedantic -Werror tests/test_board.c src/board.c -o test-board
./test-board
```

A saída esperada é:

```
OK: trilha, cruzamento, limites, torres, percurso e reinicio.
```

Se aparecer `Assertion ... failed`, alguma regra do mapa quebrou: a mensagem mostra o arquivo e a linha do teste que falhou. O binário `test-board` gerado já está no `.gitignore` e não entra no commit.

Cada issue aberta traz o comando `gcc` para testar o próprio módulo da mesma forma, sem depender dos outros.

## Comandos do projeto (depois da issue #6)

Quando a issue #6 colocar o `Makefile` na `main`, os comandos passam a ser estes:

```bash
make
make run
make test
make simulate N=1000 SEED=42
```

`make` compila o jogo, `make run` abre o jogo e `make test` executa todos os testes automatizados. `make simulate` executa partidas em lote para avaliar o balanceamento, sem alterar o modo normal do jogo.

## Como pegar uma issue

Siga a ordem #2, #3, #4 e #5, para quem pegar a próxima já partir do código da anterior. Antes de começar, atribua a issue a você no GitHub e crie a branch seguindo o fluxo acima. Leia a issue inteira: ela lista os arquivos que podem ser alterados e o comando para testar o módulo. O prazo é 20/11, no máximo 23/11, para sobrar tempo de revisão antes da entrega em 25/11.

## Organização da equipe

- Integrante A: mapa, interface e menus.
- Integrante B: entidades, torres, inimigos, ondas e combate.
- Integrante C: testes automatizados, CLI, balanceamento Monte Carlo, CI e integração.

Todos devem revisar código de colega e compreender o funcionamento geral para a defesa.

