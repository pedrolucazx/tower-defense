/* render.c - desenha o jogo "Jardim de Pedra" no terminal:
 * painel, mapa, legenda e telas de vitória/derrota.
 * Só LÊ o GameState; nenhuma regra de jogo fica aqui. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESET "\x1b[0m"

typedef struct{
    unsigned rgb;
    int idx;
}Tone;

typedef struct{
    Tone bg;
    int fg;
    bool bold;
    char txt[8];
    char plain[4];
}Tile;

/* Paleta "Floresta funda": dois tons de cada, alternados em xadrez. */
static const Tone TONE_BORDER[2]={{ 0x163d22, 22 }, { 0x1b4628, 28 }};
static const Tone TONE_GRASS[2]={{ 0x7aa64a, 64 }, { 0x82ae51, 70 }};
static const Tone TONE_STONE[2]={{ 0, 248 }, { 0, 250 }};
static const Tone TONE_ENEMY={ 0, 52 };

/* Cores de texto (256 cores) de cada elemento. */
enum{FG_BLACK=16, FG_TOWER_Y=25, FG_TOWER_Z=127, FG_ENTRY=220, FG_BASE=25, FG_ENEMY_OK=15, FG_ENEMY_HURT=216 };

static const char *const COMMANDS[][2]={
    { "y LIN COL", "comprar Y" }, { "z LIN COL", "comprar Z" },
    { "n", "avancar" }, { "p", "pausar" }, { "s", "salvar" },
    { "l", "carregar" }, { "r", "reiniciar" }, { "h", "ajuda" },
    { "m", "menu" }, { "q", "sair" },
};

/* ---- util ---- */

static bool use_truecolor(void){
    const char *e = getenv("COLORTERM");
    return e && (strstr(e, "truecolor") || strstr(e, "24bit"));
}

static bool grass_at(const GameState *g, int r, int c){
    return board_contains(r, c) && g->board.cells[r][c] == GRASS;
}

/* ---- tiles: mapa e legenda usam as mesmas funções, logo as mesmas cores ---- */

static Tile make_tile(Tone bg, int fg, bool bold, const char *txt, const char *plain){
    Tile t={ bg, fg, bold, "", "" };
    snprintf(t.txt, sizeof t.txt, "%s", txt);
    snprintf(t.plain, sizeof t.plain, "%s", plain);
    return t;
}

static Tile tile_border(int p){
    return make_tile(TONE_BORDER[p], -1, false, "  ", "^^");
}

static Tile tile_grass(int p){
    return make_tile(TONE_GRASS[p], -1, false, "  ", ". ");
}

static Tile tile_stone(int p, int line){
    /* linha fina preta só onde a pedra encosta na grama */
    const char *txt=line == 1 ? "\xe2\x96\x81\xe2\x96\x81"   /* ▁▁ */
                    : line == 2 ? " \xe2\x96\x95" : "  ";      /*  ▕ */
    return make_tile(TONE_STONE[p], line ? FG_BLACK : -1, false, txt, "::");
}

/* Entrada dos inimigos: ">>". */
static Tile tile_entry(int p){
    return make_tile(TONE_STONE[p], FG_ENTRY, true, ">>", ">>");
}

static Tile tile_base(int p){
    return make_tile(TONE_STONE[p], FG_BASE, true, "##", "##");
}

static Tile tile_tower(int p, char letter){
    char s[3]={ letter, ' ', '\0' };
    return make_tile(TONE_GRASS[p], letter == 'Z' ? FG_TOWER_Z : FG_TOWER_Y, true, s, s);
}

static Tile tile_enemy(bool hurt){
    const char *s =hurt ? "o " : "O ";
    return make_tile(TONE_ENEMY, hurt ? FG_ENEMY_HURT : FG_ENEMY_OK, true, s, s);
}

static void put_tile(FILE *out, const Tile *t, bool color, bool tc){
    if(!color){
        fputs(t->plain, out);
        return;
    }
    char sgr[96]; size_t n=0;
    n +=(size_t)snprintf(sgr+n, sizeof sgr-n, "\x1b[");
    if(t->bold){
        n +=(size_t)snprintf(sgr + n, sizeof sgr - n, "1;");
    }
    if(t->fg >= 0){
        n +=(size_t)snprintf(sgr + n, sizeof sgr - n, "38;5;%d;", t->fg);
    }
    if(tc && t->bg.rgb){
        snprintf(sgr + n, sizeof sgr - n, "48;2;%u;%u;%um",
        (t->bg.rgb >> 16) & 255u, (t->bg.rgb >> 8) & 255u, t->bg.rgb & 255u);
    }
    else{
        snprintf(sgr+n, sizeof sgr-n, "48;5;%dm", t->bg.idx);
    }
    fputs(sgr, out); fputs(t->txt, out); fputs(RESET, out);
}

/* Procura um inimigo vivo cuja posição na trilha (step) caia na casa (r, c). */
static const Enemy *enemy_at(const GameState *g, int r, int c){
    for(int i=0; i<MAX_ENEMIES; i++){
        const Enemy *e=&g->enemies[i];
        if (e->hp>0 && e->step >=0 && (size_t)e->step <g->board.path_length
        && g->board.path[e->step].row ==r && g->board.path[e->step].col ==c){
            return e;
        }
    }
    return NULL;
}

/* Decide o que desenhar em (r, c). Prioridade: inimigo > torre > terreno. */
static Tile tile_at(const GameState *g, int r, int c){
    int p=(r+c)%2;   /* xadrez: alterna o tom a cada casa */
    const Enemy *e=enemy_at(g, r, c);
    if(e){
        /* ferido = vida em 2/3 ou menos da vida máxima da onda */
        int mx=game_enemy_hp(g->wave, g->difficulty);
        return tile_enemy(e->hp * 3 <= mx * 2);
    }
    if(g->board.towers[r][c]){
        return tile_tower(p, g->board.towers[r][c]);
    }
    switch(g->board.cells[r][c]){
    case TREE:     return tile_border(p);
    case ENTRANCE: return tile_entry(p);
    case BASE:     return tile_base(p);
    case PATH:
        /* linha fina só onde a pedra encosta na grama: embaixo tem prioridade sobre à direita */
        return tile_stone(p, grass_at(g, r + 1, c) ? 1 : grass_at(g, r, c + 1) ? 2 : 0);
    default:       return tile_grass(p);
    }
}

/* ---- seções ---- */

/* Painel de 3 linhas: título e estado, recursos da partida e andamento da onda. */
static void render_panel(FILE *out, const GameState *g, bool color){
    const char *st=g->status ==VICTORY ? "VITORIA" : g->status ==DEFEAT ? "DERROTA" : "EM JOGO";
    int alive=0;   /* inimigos vivos na trilha */
    for(int i=0; i<MAX_ENEMIES; i++){
        if(g->enemies[i].hp > 0){
            alive++;
        }
    }
    fprintf(out, "%sJARDIM DE PEDRA | %s%s\n", color ? "\x1b[1m" : "", st, color ? RESET : "");
    fprintf(out, "Onda %d/%d | Vida %d/%d | Moedas %d | Pontos %d\n",
            g->wave, MAX_WAVES, g->hp, INITIAL_HP, g->money, g->score);
    fprintf(out, "Inimigos da onda: %d/%d entraram | Na trilha %d | Eliminados %d | Escaparam %d\n",
            g->spawned, game_wave_size(g->wave), alive, g->kills, g->escaped);
}

/* Mapa: cabeçalho de colunas A..P e, em cada linha, o número dela seguido das casas. */
static void render_map(FILE *out, const GameState *g, bool color, bool tc){
    fputs("   ", out);  /* largura do rótulo de linha */
    for(int c=0; c<BOARD_COLS; c++){
        fprintf(out, "%c ", 'A' + c);
    }
    fputc('\n', out);
    for(int r=0; r<BOARD_ROWS; r++){
        fprintf(out, "%02d ", r);
        for (int c=0; c<BOARD_COLS; c++) {
            Tile t=tile_at(g, r, c);
            put_tile(out, &t, color, tc);
        }
        fputc('\n', out);
    }
}

/* Item da legenda: o símbolo (com a cor do mapa) seguido do rótulo. */
static void item(FILE *out, const Tile *t, const char *label, bool color, bool tc){
    put_tile(out, t, color, tc);
    fprintf(out, " %s", label);
}

/* Escreve uma tecla de comando em amarelo (só com cor). */
static void key(FILE *out, const char *k, bool color){
    fprintf(out, "%s%s%s", color ? "\x1b[1;33m" : "", k, color ? RESET : "");
}

/* Legenda (partida em andamento): terreno, torres com seus atributos, inimigos e comandos. */
static void render_legend(FILE *out, bool color, bool tc){
    Tile b=tile_border(0), gr=tile_grass(0), s=tile_stone(0, 0);
    Tile en=tile_entry(0), ba=tile_base(0);
    fputs("Legenda: ", out);
    item(out, &b, "borda", color, tc);   fputs("  ", out);
    item(out, &gr, "grama", color, tc);  fputs("  ", out);
    item(out, &s, "pedra", color, tc);   fputs("  ", out);
    item(out, &en, "entrada", color, tc); fputs("  ", out);
    item(out, &ba, "base", color, tc);   fputc('\n', out);

    /* uma linha por tipo de torre; custo, dano e cadência vêm de TOWER_STATS e a área, do alcance
     * (alcance 1 = 3x3, alcance 2 = 5x5, supondo raio quadrado) */
    for(int t=0; t<TOWER_TYPES; t++){
        Tile tt=tile_tower(0, game_tower_letter(t));
        int side=2 * TOWER_STATS[t].range + 1;
        char rate[24];
        if(TOWER_STATS[t].period <= 1){
            snprintf(rate, sizeof rate, "todo passo");
        }else{
            snprintf(rate, sizeof rate, "a cada %d passos", TOWER_STATS[t].period);
        }
        char lbl[96];
        snprintf(lbl, sizeof lbl, "$%d, dano %d, area %dx%d, atira %s",
                 TOWER_STATS[t].cost, TOWER_STATS[t].damage, side, side, rate);
        fputs(t ? "         " : "Torres:  ", out);
        item(out, &tt, lbl, color, tc);
        fputc('\n', out);
    }

    Tile ok=tile_enemy(false), hurt=tile_enemy(true);
    fputs("Inimigo: ", out);
    item(out, &ok, "vida boa", color, tc);  fputs("  ", out);
    item(out, &hurt, "ferido", color, tc);  fputc('\n', out);

    /* teclas de comando em amarelo, separadas por "|", em duas linhas */
    fputs("Comandos: ", out);
    for (size_t i=0; i<sizeof COMMANDS / sizeof COMMANDS[0]; i++){
        if(i==4){
            fputs("\n          ", out);
        }else if(i){
            fputs(" | ", out);
        }
        key(out, COMMANDS[i][0], color);
        fprintf(out, ": %s", COMMANDS[i][1]);
    }
    fputc('\n', out);
}

/* Painel final (substitui a legenda): mensagem verde de vitória ou vermelha de derrota,
 * um resumo da partida e as opções para continuar. */
static void render_end(FILE *out, const GameState *g, bool color){
    bool won=g->status == VICTORY;
    if(color){
        fputs(won ? "\x1b[1;32m" : "\x1b[1;31m", out);
    }
    if(won){
        fprintf(out, "VITORIA! O jardim resistiu as %d ondas.", MAX_WAVES);
    }else{
        fprintf(out, "DERROTA! A base caiu na onda %d/%d.", g->wave, MAX_WAVES);
    }
    if(color){
        fputs(RESET, out);
    }
    fputc('\n', out);

    /* resumo: na vitória mostra as vidas que sobraram; na derrota, quantos inimigos chegaram à base */
    if(won){
        fprintf(out, "Pontos %d | Eliminados %d | Vidas restantes %d/%d\n", g->score, g->kills, g->hp, INITIAL_HP);
    }else{
        fprintf(out, "Pontos %d | Eliminados %d | Chegaram a base %d\n", g->score, g->kills, g->escaped);
    }
    key(out, "r", color); fputs(": jogar de novo | ", out);
    key(out, "m", color); fputs(": menu | ", out);
    key(out, "q", color); fputs(": sair\n", out);
}

/* Ponto de entrada: escreve em `out` o painel, o mapa e, conforme o estado,
 * a legenda (em jogo) ou o painel final (vitória/derrota). */
void game_render(FILE *out, const GameState *g, bool color){
    if(!out || !g){
        return;
    }
    bool tc=color && use_truecolor();   /* truecolor só se houver cor e o terminal suportar */
    render_panel(out, g, color);
    render_map(out, g, color, tc);
    if(g->status == PLAYING){
        render_legend(out, color, tc);
    }else{
        render_end(out, g, color);
    }
}
