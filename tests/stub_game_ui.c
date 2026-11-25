/* Versões falsas do núcleo/persistência usadas por src/ui.c (issue #2).
 * Só o necessário para testar ui_command sem src/game.c nem src/persistence.c. */
#include "game.h"
#include <string.h>

const TowerStats TOWER_STATS[TOWER_TYPES] = {{40, 1, 2, 1}, {60, 2, 5, 2}};

int stub_ticks, stub_saves, stub_loads;
bool stub_save_ok = true, stub_load_ok = true;

void game_init(GameState *g, int difficulty) {
    memset(g, 0, sizeof *g);
    board_init(&g->board);
    g->hp = INITIAL_HP;
    g->money = INITIAL_MONEY;
    g->wave = 1;
    g->difficulty = difficulty;
}

bool game_buy(GameState *g, int row, int col, int type) {
    if (g->status != PLAYING || type < 0 || type >= TOWER_TYPES ||
        g->money < TOWER_STATS[type].cost ||
        !board_place(&g->board, row, col, type ? 'Z' : 'Y'))
        return false;
    g->money -= TOWER_STATS[type].cost;
    g->towers[g->tower_count++] = (Tower){row, col, type, 0};
    return true;
}

void game_tick(GameState *g) { ++stub_ticks; ++g->turn; }

bool game_save(const GameState *g, const char *path) {
    (void)g; (void)path;
    ++stub_saves;
    return stub_save_ok;
}

bool game_load(GameState *g, const char *path) {
    (void)path;
    ++stub_loads;
    if (stub_load_ok) g->turn = 42;
    return stub_load_ok;
}
