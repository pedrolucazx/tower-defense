#include "ui.h"
#include <assert.h>
#include <string.h>

extern int stub_ticks, stub_saves, stub_loads;
extern bool stub_save_ok, stub_load_ok;

static bool quit, menu;

/* ui_command altera a linha (strtok), então cada chamada usa uma cópia. */
static const char *run(GameState *g, const char *text, bool realtime) {
    char line[128];
    strcpy(line, text);
    quit = menu = false;
    return ui_command(g, line, &quit, &menu, realtime);
}

static void expect(GameState *g, const char *text, bool realtime, const char *message) {
    const char *got = run(g, text, realtime);
    if (strcmp(got, message)) {
        fprintf(stderr, "FAIL: \"%s\" -> \"%s\" (esperado \"%s\")\n", text, got, message);
        assert(0);
    }
}

/* Comando recusado não pode alterar nada no estado. */
static void unchanged(GameState *g, const char *text, const char *message) {
    GameState before = *g;
    int ticks = stub_ticks;
    expect(g, text, false, message);
    assert(!memcmp(&before, g, sizeof before));
    assert(stub_ticks == ticks);
}

static const char *OUT = "Coordenada fora do mapa: linha 00-12, coluna A-P.";
static const char *ENDED = "Partida encerrada. Use r, m ou q.";
static const char *BUSY = "Casa ocupada: torre so em grama livre (nao em pedra ou arvore).";
static const char *USAGE = "Uso: yLINxCOL (torre Y) ou zLINxCOL (torre Z), ex: y10xN.";

static void test_buy(GameState *g) {
    game_init(g, 0);
    stub_ticks = 0;
    expect(g, "y 4 E", false, "Torre Y comprada.");
    assert(g->board.towers[4][4] == 'Y' && g->money == 80 && stub_ticks == 1);
    expect(g, "z 03 H", false, "Torre Z comprada.");
    assert(g->board.towers[3][7] == 'Z' && g->money == 20 && stub_ticks == 2);

    game_init(g, 0);
    expect(g, "4 K", false, "Torre Y comprada.");
    expect(g, "6xk", false, "Torre Y comprada.");
    expect(g, "8XM", false, "Torre Y comprada.");
    assert(g->board.towers[4][10] == 'Y' && g->board.towers[6][10] == 'Y' && g->board.towers[8][12] == 'Y');

    /* Formato compacto: torre + linha + x + coluna. */
    game_init(g, 0);
    expect(g, "y10xN", false, "Torre Y comprada.");
    expect(g, "z4Xk", false, "Torre Z comprada.");
    assert(g->board.towers[10][13] == 'Y' && g->board.towers[4][10] == 'Z' && g->money == 20);

    /* Tempo real: compra não adianta o passo. */
    game_init(g, 0);
    stub_ticks = 0;
    expect(g, "y 4 E", true, "Torre Y comprada.");
    assert(stub_ticks == 0);
}

static void test_buy_refused(GameState *g) {
    game_init(g, 0);
    const char *out_of_map[] = {"y13xA", "y0xQ", "y10x", "y10xNN", "y 13 A", "y 0 Q", "y -1 A", "12 Q", "y x D", "3 abc", "y 4 4", "y 4 KK",
                                "y 999999999999999999999999 A", "3x", "y 4.5 E"};
    for (size_t i = 0; i < sizeof out_of_map / sizeof *out_of_map; ++i) unchanged(g, out_of_map[i], OUT);
    unchanged(g, "y 0 A", BUSY);   /* árvore */
    unchanged(g, "y 5 D", BUSY);   /* trilha */
    unchanged(g, "z 7 D", BUSY);   /* pedra decorativa */
    expect(g, "y 4 E", false, "Torre Y comprada.");
    unchanged(g, "z 4 E", BUSY);   /* torre já existe */

    g->money = 59;
    unchanged(g, "z 4 F", "Moedas insuficientes: a torre Z custa $60.");
    g->money = 39;
    unchanged(g, "y 4 F", "Moedas insuficientes: a torre Y custa $40.");
    /* Ordem: dinheiro é verificado antes de casa ocupada; mapa antes de tudo. */
    unchanged(g, "y 0 A", "Moedas insuficientes: a torre Y custa $40.");
    g->status = DEFEAT;
    unchanged(g, "y 4 F", ENDED);
    unchanged(g, "y 99 A", OUT);
    unchanged(g, "n", ENDED);
}

static void test_usage_and_invalid(GameState *g) {
    game_init(g, 0);
    unchanged(g, "y", USAGE);
    unchanged(g, "z 3", USAGE);
    unchanged(g, "y 1 2 3", USAGE);
    unchanged(g, "", "Comando invalido.");
    unchanged(g, "   ", "Comando invalido.");
    unchanged(g, "xyz", "Comando invalido.");
    unchanged(g, "p", "Comando invalido.");  /* pausa é tratada pelo laço em tempo real */
    unchanged(g, "n extra", "Comando invalido: argumentos inesperados.");
    unchanged(g, "q agora", "Comando invalido: argumentos inesperados.");
    unchanged(g, "4 5 6", "Comando invalido: argumentos inesperados.");
    unchanged(g, "a 4 4", "Torres agora sao y e z: yLINxCOL ou zLINxCOL, ex: y10xN.");   /* comando antigo da torre A */
    unchanged(g, "b", "Torres agora sao y e z: yLINxCOL ou zLINxCOL, ex: y10xN.");
    assert(!quit && !menu);
}

static void test_commands(GameState *g) {
    game_init(g, 0);
    stub_ticks = 0;
    expect(g, "n", false, "Avancou.");
    assert(stub_ticks == 1);

    expect(g, "q", false, "Ate logo!");
    assert(quit && !menu);
    expect(g, "m", false, "Menu.");
    assert(menu && !quit);

    expect(g, "y 4 E", false, "Torre Y comprada.");
    g->status = VICTORY;
    expect(g, "r", false, "Partida reiniciada.");
    assert(g->status == PLAYING && g->money == INITIAL_MONEY && !g->board.towers[4][4]);

    stub_saves = stub_loads = 0;
    stub_save_ok = stub_load_ok = true;
    expect(g, "s", false, "Partida salva.");
    expect(g, "l", false, "Partida carregada.");
    assert(g->turn == 42);
    stub_save_ok = stub_load_ok = false;
    expect(g, "s", false, "Erro ao salvar; estado preservado.");
    expect(g, "l", false, "Save ausente ou invalido; estado preservado.");
    assert(stub_saves == 2 && stub_loads == 2);

    assert(!strcmp(run(g, "h", false), ui_instructions()));
    assert(strstr(ui_instructions(), "Defenda a base"));
}

static void test_read(void) {
    char line[8];
    FILE *in = tmpfile();
    assert(in);
    static const char input[] = "ok\n1234567890\nn\0x\nfim";
    fwrite(input, 1, sizeof input - 1, in);
    rewind(in);
    FILE *saved = stdin;
    stdin = in;
    assert(ui_read(line, sizeof line) == 1 && !strcmp(line, "ok"));
    assert(ui_read(line, sizeof line) == -1);                  /* longa demais */
    assert(ui_read(line, sizeof line) == -1);                  /* NUL no meio */
    assert(ui_read(line, sizeof line) == 1 && !strcmp(line, "fim"));  /* sem \n no fim */
    assert(ui_read(line, sizeof line) == 0);                   /* EOF */
    stdin = saved;
    fclose(in);
}

int main(void) {
    GameState g;
    test_buy(&g);
    test_buy_refused(&g);
    test_usage_and_invalid(&g);
    test_commands(&g);
    test_read();
    puts("OK: ui_command e ui_read.");
    return 0;
}
