/* test_render.c - testes de game_render: conferem o texto e as cores gerados
 * com e sem cor, em 256 cores e em truecolor, e as telas de vitória/derrota. */
#define _POSIX_C_SOURCE 200809L   /* necessário para open_memstream, setenv e unsetenv */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails=0;   /* quantidade de CHECKs que falharam */

/* CHECK(cond): se a condição for falsa, imprime o texto dela e a linha do teste e conta a falha.
 * O do/while(0) faz a macro se comportar como uma instrução só. */
#define CHECK(c)                                              \
    do{                                                       \
        if(!(c)){                                             \
            printf("FALHOU: %s (linha %d)\n", #c, __LINE__);  \
            fails++;                                          \
        }                                                     \
    }while(0)

/* Roda game_render escrevendo numa memória (em vez da tela) e devolve o texto gerado.
 * Quem chama deve liberar o resultado com free(). */
static char *capture(const GameState *g, bool color){
    char *buf=NULL; size_t len=0;
    FILE *f=open_memstream(&buf, &len);   /* "arquivo" que grava em buf */
    game_render(f, g, color);
    fclose(f);                            /* fechar finaliza buf e len */
    return buf;
}

/* Largura visível da maior linha: ignora ANSI e conta pontos de código UTF-8. */
static int max_width(const char *s){
    int best=0, w=0;   /* best = maior largura até agora; w = largura da linha atual */
    for(; *s; s++){
        if(*s == '\x1b'){            /* início de código ANSI: pula até o 'm' final */
            while(*s && *s != 'm'){
                s++;
             } 
            if(!*s){
                break;
            }
            continue;
        }
        if(*s == '\n'){              /* fim de linha: guarda a maior largura e recomeça */
            if(w>best){
                best=w;
            }
            w=0;
            continue;
        }
        /* bytes 10xxxxxx são continuação de UTF-8 (ex.: ▁): só conta o primeiro byte de cada caractere */
        if(((unsigned char)*s & 0xC0) != 0x80) w++;
    }
    return best>w ? best : w;
}

/* Monta um estado de jogo padrão: duas torres, uma onda em andamento e dois inimigos.
 * O estado é static para não ocupar a pilha (GameState é grande). */
static GameState make_state(void){
    static GameState g;
    memset(&g, 0, sizeof g);
    board_init(&g.board);
    board_place(&g.board, 1, 4, 'A');
    board_place(&g.board, 6, 4, 'B');
    g.hp=7; g.money=50; g.score=120; g.wave=2;
    g.spawned=2; g.kills=4; g.escaped=1; g.status=PLAYING;
    g.enemies[0]=(Enemy){.hp = game_enemy_hp(2, 0), .step = 3 };/* cheio */
    g.enemies[1]=(Enemy){.hp = game_enemy_hp(2, 0) / 2, .step = 20 };/* ferido */
    return g;
}

/* só a parte do mapa (antes da legenda) */
static void cut_legend(char *s){
    char *p = strstr(s, "Legenda:"); if (p) *p = '\0';   /* a legenda tem os dois tipos de inimigo; cortá-la isola o mapa */
}

int main(void){
    GameState g=make_state();
    unsetenv("COLORTERM");   /* começa sem truecolor, para testar o modo de 256 cores */

    /* ---- sem cor ---- */
    char *plain=capture(&g, false);
    CHECK(strchr(plain, '\033') ==NULL);   /* sem cor: nenhum código ANSI (ESC) */
    CHECK(strstr(plain, "^^") && strstr(plain, ". ") && strstr(plain, "::"));   /* árvore, grama e pedra */
    CHECK(strstr(plain, "##") && strstr(plain, ">>"));   /* base e entrada */
    CHECK(strstr(plain, "000102030405060708091011121314" "15") != NULL);/* 00..15 */
    CHECK(strstr(plain, "\n05 ") != NULL && strstr(plain, "\n12 ") != NULL);/* linhas */
    CHECK(strstr(plain, "A ") && strstr(plain, "B "));   /* as duas torres */
    CHECK(strstr(plain, "JARDIM DE PEDRA | EM JOGO"));
    CHECK(strstr(plain, "Onda 3/5 | Vida 7/10 | Moedas 50 | Pontos 120"));   /* wave 2 (base 0) aparece como 3 */
    char want[128];
    snprintf(want, sizeof want, "Inimigos da onda: 2/%d entraram | Na trilha 2 | Eliminados 4 | Escaparam 1",
             game_wave_size(2));   /* o total da onda vem do núcleo, então é calculado aqui também */
    CHECK(strstr(plain, want));
    CHECK(strstr(plain, "Legenda:"));
    CHECK(max_width(plain) <= 84);   /* nenhuma linha passa de 84 colunas */
    CHECK(strstr(plain, "\xe2\x96") == NULL);/* sem ▁ ▕ sem cor */
    free(plain);

    /* ---- com cor, 256 cores ---- */
    char *col=capture(&g, true);
    CHECK(strchr(col, '\033') !=NULL);   /* com cor: há códigos ANSI */
    CHECK(strstr(col, "##") && strstr(col, ">>"));
    CHECK(strstr(col, "48;5;64") && strstr(col, "48;5;70"));/* grama xadrez */
    CHECK(strstr(col, "48;5;22") && strstr(col, "48;5;28"));/* borda (árvores) */
    CHECK(strstr(col, "48;5;248") && strstr(col, "48;5;250"));/* pedra */
    CHECK(strstr(col, "38;5;220") && strstr(col, "38;5;25") && strstr(col, "38;5;127"));   /* entrada, torre A/base, torre B */
    CHECK(strstr(col, "\xe2\x96\x81\xe2\x96\x81") != NULL);/* ▁▁ embaixo */
    CHECK(strstr(col, "\xe2\x96\x95") !=NULL);/* ▕ à direita */
    CHECK(strstr(col, "48;2;")==NULL);   /* sem COLORTERM não pode haver truecolor */
    CHECK(max_width(col)<=84);   /* mede com a legenda ainda presente */
    cut_legend(col);             /* daqui em diante só o mapa */
    CHECK(strstr(col, "48;5;52"));
    CHECK(strstr(col, "38;5;15"));/* inimigo com vida cheia: O branco */
    CHECK(strstr(col, "38;5;216"));/* inimigo ferido: o salmão */
    free(col);

    /* ferido vira 'o' / cheio fica 'O' (isolando cada caso) */
    GameState one = make_state();
    one.enemies[0].hp=0;/* só o ferido */
    char *h=capture(&one, true); cut_legend(h);
    CHECK(strstr(h, "38;5;216") && !strstr(h, "38;5;15;"));   /* o ";" final evita casar com outros códigos, como 38;5;150 */
    free(h);
    one=make_state(); one.enemies[1].hp=0;/* só o cheio */
    h=capture(&one, true); cut_legend(h);
    CHECK(strstr(h, "38;5;15") && !strstr(h, "38;5;216"));
    free(h);

    /* ---- truecolor ---- */
    setenv("COLORTERM", "truecolor", 1);
    char *tc=capture(&g, true);
    CHECK(strstr(tc, "48;2;122;166;74") && strstr(tc, "48;2;130;174;81"));  /* grama */
    CHECK(strstr(tc, "48;2;22;61;34") && strstr(tc, "48;2;27;70;40"));      /* borda */
    CHECK(strstr(tc, "48;5;64") == NULL);   /* a grama não pode cair no modo 256 */
    CHECK(max_width(tc) <=84);
    free(tc);
    setenv("COLORTERM", "24bit", 1);   /* o outro valor aceito também ativa truecolor */
    tc=capture(&g, true);
    CHECK(strstr(tc, "48;2;122;166;74"));
    free(tc);
    unsetenv("COLORTERM");

    /* ---- fim de partida ---- */
    GameState w=make_state(); w.status=VICTORY; w.wave=MAX_WAVES; w.hp=4;
    char *ws=capture(&w, false);
    CHECK(strstr(ws, "JARDIM DE PEDRA | VITORIA"));
    CHECK(strstr(ws, "VITORIA! O jardim resistiu as 5 ondas."));
    CHECK(strstr(ws, "Pontos 120") && strstr(ws, "Eliminados 4") && strstr(ws, "Vidas restantes 4/10"));
    CHECK(strstr(ws, "r: jogar de novo | m: menu | q: sair"));
    CHECK(!strstr(ws, "Legenda") && !strstr(ws, "DERROTA!"));   /* a legenda dá lugar ao painel final */
    CHECK(max_width(ws)<=84);
    free(ws);
    char *wc=capture(&w, true);
    CHECK(strstr(wc, "\x1b[1;32mVITORIA!"));     /* verde */
    CHECK(strstr(wc, "\x1b[1;33mr"));            /* tecla amarela */
    free(wc);

    GameState d=make_state(); d.status=DEFEAT; d.hp=0;
    char *ds=capture(&d, false);
    CHECK(strstr(ds, "JARDIM DE PEDRA | DERROTA"));
    CHECK(strstr(ds, "DERROTA! A base caiu na onda 3/5."));
    CHECK(strstr(ds, "Chegaram a base 1"));
    CHECK(strstr(ds, "r: jogar de novo | m: menu | q: sair"));
    CHECK(!strstr(ds, "Legenda"));
    free(ds);
    char *dc=capture(&d, true);
    CHECK(strstr(dc, "\x1b[1;31mDERROTA!"));/* vermelho */
    free(dc);

    /* entradas inválidas não podem derrubar o programa */
    game_render(NULL, &g, false);
    FILE *f=tmpfile(); game_render(f, NULL, true); fclose(f);

    puts(fails ? "TESTES COM FALHA" : "OK: todos os testes passaram");
    return fails !=0;   /* 0 = sucesso; 1 = houve falha */
}