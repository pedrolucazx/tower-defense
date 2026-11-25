#include "ui.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int ui_read(char *line, size_t capacity) {
    size_t length = 0;
    bool invalid = false;
    int ch;
    while ((ch = getchar()) != EOF && ch != '\n') {
        if (ch == 0 || length + 1 >= capacity) invalid = true;
        else line[length++] = (char)ch;
    }
    line[length] = '\0';
    if (invalid) return -1;
    return ch == EOF && !length ? 0 : 1;
}

static bool column(const char *text, int *value) {
    int c = toupper((unsigned char)text[0]) - 'A';
    if (!text[0] || text[1] || c < 0 || c >= BOARD_COLS) return false;
    *value = c;
    return true;
}

static bool coordinate(const char *text, int limit, int *value) {
    char *end;
    errno = 0;
    long n = strtol(text, &end, 10);
    if (errno || end == text || *end || n < 0 || n >= limit) return false;
    *value = (int)n;
    return true;
}

const char *ui_instructions(void) {
    return "Defenda a base (##) por cinco ondas de inimigos que andam pela trilha de pedra.\n"
           "Vence ao sobreviver as 5 ondas; perde quando a vida chega a zero (cada fuga custa 1).\n"
           "Torre Y: $40, dano 2, area 3x3, atira todo passo.\n"
           "Torre Z: $60, dano 5, area 5x5, atira a cada 2 passos.\n"
           "Cada torre atira no inimigo mais avancado dentro da sua area. Sem limite de torres.\n"
           "Comprar: y LIN COL (torre Y) ou z LIN COL (torre Z), ex: y 4 K. LIN COL ou LINxCOL compra Y.\n"
           "Linhas 00-12, colunas A-P. Torres so em grama livre.\n"
           "Comeca com 120 moedas e 10 vidas. Cada eliminacao rende 8 moedas e 10 pontos.\n"
           "Ondas de 8, 12, 16, 20 e 24 inimigos; cada onda anda mais rapido.\n"
           "p pausa | n avanca | s salva | l carrega | r reinicia | m menu | q sai.";
}

const char *ui_command(GameState *g, char *line, bool *quit, bool *menu, bool realtime) {
    char *op = strtok(line, " \t\r\n"), *a = strtok(NULL, " \t\r\n"),
         *b = strtok(NULL, " \t\r\n"), *extra = strtok(NULL, " \t\r\n");
    if (!op) return "Comando invalido.";
    char *row_text = NULL, *col_text = NULL, *x;
    int type = 0;
    if (strlen(op) == 1 && (*op == 'y' || *op == 'z') && a && b && !extra) {
        type = *op == 'z';
        row_text = a; col_text = b;
    } else if (a && !b && isdigit((unsigned char)*op)) {
        row_text = op; col_text = a;
    } else if (!a && isdigit((unsigned char)*op) && (x = strpbrk(op, "xX"))) {
        *x = '\0';
        row_text = op; col_text = x + 1;
    }
    if (row_text) {
        int r, c;
        if (!coordinate(row_text, BOARD_ROWS, &r) || !column(col_text, &c))
            return "Coordenada fora do mapa: linha 00-12, coluna A-P.";
        if (g->status != PLAYING) return "Partida encerrada. Use r, m ou q.";
        if (g->money < TOWER_STATS[type].cost)
            return type ? "Moedas insuficientes: a torre Z custa $60." : "Moedas insuficientes: a torre Y custa $40.";
        if (g->board.cells[r][c] != GRASS || g->board.towers[r][c])
            return "Casa ocupada: torre so em grama livre (nao em pedra ou arvore).";
        if (!game_buy(g, r, c, type)) return "Compra recusada.";
        if (!realtime) game_tick(g);
        return type ? "Torre Z comprada." : "Torre Y comprada.";
    }
    if ((*op == 'y' || *op == 'z') && !op[1])
        return "Uso: y LIN COL (torre Y) ou z LIN COL (torre Z), ex: z 03 H.";
    if ((*op == 'a' || *op == 'b') && !op[1])
        return "Torres agora sao y e z: y LIN COL ou z LIN COL, ex: z 03 H.";
    if (a) return "Comando invalido: argumentos inesperados.";
    if (!strcmp(op, "q")) { *quit = true; return "Ate logo!"; }
    if (!strcmp(op, "m")) { *menu = true; return "Menu."; }
    if (!strcmp(op, "r")) { game_init(g, 0); return "Partida reiniciada."; }
    if (!strcmp(op, "s")) return game_save(g, "savegame.dat") ? "Partida salva." : "Erro ao salvar; estado preservado.";
    if (!strcmp(op, "l")) return game_load(g, "savegame.dat") ? "Partida carregada." : "Save ausente ou invalido; estado preservado.";
    if (!strcmp(op, "h")) return ui_instructions();
    if (!strcmp(op, "n")) {
        if (g->status != PLAYING) return "Partida encerrada. Use r, m ou q.";
        game_tick(g); return "Avancou.";
    }
    return "Comando invalido.";
}
