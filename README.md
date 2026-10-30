# Tower Defense em C

> Trabalho final de Laboratório de Programação em C — equipe de três integrantes.

## Status

O repositório está na fase de planejamento e divisão das issues. O Spec-Kit é usado localmente para planejamento e não é publicado neste repositório.

## Pré-requisitos

- Linux
- GCC
- Make
- Git

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

Quando a implementação estiver disponível:

```bash
make
make run
make test
make simulate N=1000 SEED=42
```

`make test` executa os testes automatizados. `make simulate` executa partidas em lote para avaliar o balanceamento, sem alterar o modo normal do jogo.

## Organização da equipe

- Integrante A: mapa, interface e menus.
- Integrante B: entidades, torres, inimigos, ondas e combate.
- Integrante C: testes automatizados, CLI, balanceamento Monte Carlo, CI e integração.

Todos devem revisar código de colega e compreender o funcionamento geral para a defesa.

