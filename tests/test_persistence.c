#define _POSIX_C_SOURCE 200809L
#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "w");
    assert(f);
    fputs(text, f);
    fclose(f);
}

static GameState *playing_state(void) {
    GameState *g = game_create();
    assert(g);
    game_init(g, 0);
    assert(game_buy(g, 4, 10, 0));
    assert(game_buy(g, 8, 12, 1));
    for (int i = 0; i < 40; ++i) game_tick(g);
    assert(g->status == PLAYING && g->spawned > 0);
    return g;
}

static void rejected_without_change(GameState *g, const char *text) {
    GameState before = *g;
    write_file("bad.dat", text);
    assert(!game_load(g, "bad.dat"));
    assert(memcmp(&before, g, sizeof before) == 0);
}

int main(void) {
    GameState *g = playing_state();

    assert(game_save(g, "save.dat"));
    GameState *loaded = game_create();
    assert(loaded && game_load(loaded, "save.dat"));
    assert(memcmp(g, loaded, sizeof *g) == 0);
    for (int i = 0; i < 50; ++i) {
        game_tick(g);
        game_tick(loaded);
        assert(memcmp(g, loaded, sizeof *g) == 0);
    }

    assert(!game_load(g, "nao-existe.dat"));
    rejected_without_change(g, "");
    rejected_without_change(g, "TD1\n0 10 120 1 0 0 0 0 0 0 0 0\n");
    rejected_without_change(g, "TD2\n0 999999999999 120 1 0 0 0 0 0 0 0 0\n");
    rejected_without_change(g, "TD2\n0 10 120 1 0 0 0 0 0 0 0 0\n0 0\n");

    FILE *f = fopen("save.dat", "a");
    assert(f);
    fputs("lixo\n", f);
    fclose(f);
    GameState before = *g;
    assert(!game_load(g, "save.dat"));
    assert(memcmp(&before, g, sizeof before) == 0);

    remove("rank.dat");
    assert(!ranking_record(g, "rank.dat"));
    game_init(g, 0);
    assert(game_buy(g, 4, 10, 0));
    for (int i = 0; i < MAX_TURNS && g->status == PLAYING; ++i) game_tick(g);
    assert(g->status == DEFEAT && g->score > 0);
    assert(ranking_record(g, "rank.dat"));
    int best = -1;
    assert(ranking_read("rank.dat", &best) && best == g->score);
    write_file("rank.dat", "TDR1 850\n");
    assert(ranking_record(g, "rank.dat"));
    assert(ranking_read("rank.dat", &best) && best == 850);
    write_file("rank.dat", "TDR1 999999\n");
    assert(!ranking_read("rank.dat", &best) && best == 850);

    remove("save.dat");
    remove("bad.dat");
    remove("rank.dat");
    free(g);
    free(loaded);
    puts("OK: save/load identico, saves invalidos recusados sem alterar a partida e recorde.");
    return 0;
}
