#include "game.h"
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef TOP_CELLS
#define TOP_CELLS 30
#endif
typedef struct { Position cells[BOARD_ROWS * BOARD_COLS]; int count; } Candidates;
typedef struct { Position cell; int coverage; } Scored;
static uint32_t random_next(uint32_t *state) {
    *state ^= *state << 13; *state ^= *state >> 17; *state ^= *state << 5;
    return *state;
}

static bool number(const char *s, long maximum, long *out) {
    char *end;
    errno = 0;
    long n = strtol(s, &end, 10);
    if (errno || end == s || *end || n < 1 || n > maximum) return false;
    *out = n; return true;
}

static int by_coverage(const void *a, const void *b) {
    const Scored *x = a, *y = b;
    if (x->coverage != y->coverage) return y->coverage - x->coverage;
    if (x->cell.row != y->cell.row) return x->cell.row - y->cell.row;
    return x->cell.col - y->cell.col;
}

static void candidates(const Board *b, Candidates *list, int range) {
    Scored scored[BOARD_ROWS * BOARD_COLS];
    int count = 0;
    for (int r = 0; r < BOARD_ROWS; ++r) for (int c = 0; c < BOARD_COLS; ++c) {
        if (b->cells[r][c] != GRASS) continue;
        int coverage = 0;
        for (size_t i = 0; i < b->path_length; ++i)
            coverage += abs(b->path[i].row - r) <= range && abs(b->path[i].col - c) <= range;
        if (coverage) scored[count++] = (Scored){{r, c}, coverage};
    }
    qsort(scored, (size_t)count, sizeof *scored, by_coverage);
    list->count = count < TOP_CELLS ? count : TOP_CELLS;
    for (int i = 0; i < list->count; ++i) list->cells[i] = scored[i].cell;
}

static bool buy(GameState *g, const Candidates *list, int type, uint32_t *rng) {
    if (g->money < TOWER_STATS[type].cost || !list->count) return false;
    int start = (int)(random_next(rng) % (uint32_t)list->count);
    for (int offset = 0; offset < list->count; ++offset) {
        Position p = list->cells[(start + offset) % list->count];
        if (game_buy(g, p.row, p.col, type)) return true;
    }
    return false;
}

int main(int argc, char **argv) {
    setlocale(LC_NUMERIC, "C");
    long n = 1000, seed = 42;
    if (argc > 3 || (argc > 1 && !number(argv[1], 100000, &n)) ||
        (argc > 2 && !number(argv[2], 2147483647, &seed))) {
        fputs("Uso: simulate [N:1..100000] [SEED:1..2147483647]\n", stderr); return 1;
    }
    GameState *g = game_create();
    if (!g) return 1;
    Candidates list[TOWER_TYPES];
    for (int t = 0; t < TOWER_TYPES; ++t) candidates(&g->board, &list[t], TOWER_STATS[t].range);
    const char *names[] = {"sem_defesa", "so_torre_a", "so_torre_b", "misto"};
    puts("config,strategy,n,seed,wins,losses,win_rate,ci95_low,ci95_high,mean_turns,min_turns,max_turns,mean_hp,mean_money,mean_kills,mean_wave");
    for (int difficulty = 0; difficulty < 2; ++difficulty) for (int policy = 0; policy < 4; ++policy) {
        int wins = 0, min_turn = MAX_TURNS, max_turn = 0;
        double turns = 0, hp = 0, money = 0, kills = 0, wave = 0;
        for (long run = 0; run < n; ++run) {
            game_init(g, difficulty);
            uint32_t rng = (uint32_t)seed ^ ((uint32_t)(run + 1) * UINT32_C(2654435761));
            if (!rng) rng = 1;
            while (g->status == PLAYING && g->turn < MAX_TURNS) {
                int type = policy == 1 ? 0 : policy == 2 ? 1 : g->tower_count % 2;
                if (policy && !buy(g, &list[type], type, &rng) && policy == 3)
                    buy(g, &list[1 - type], 1 - type, &rng);
                game_tick(g);
                if (!game_valid(g)) { fputs("Estado invalido durante simulacao\n", stderr); free(g); return 1; }
            }
            if (g->status == PLAYING) { fputs("Partida nao terminou\n", stderr); free(g); return 1; }
            wins += g->status == VICTORY;
            turns += g->turn; hp += g->hp; money += g->money; kills += g->kills; wave += g->wave;
            if (g->turn < min_turn) min_turn = g->turn;
            if (g->turn > max_turn) max_turn = g->turn;
        }
        double p = (double)wins / n, z2 = 1.96 * 1.96, denominator = 1 + z2 / n;
        double center = (p + z2 / (2.0 * n)) / denominator;
        double margin = 1.96 * sqrt(p * (1 - p) / n + z2 / (4.0 * n * n)) / denominator;
        printf("%s,%s,%ld,%ld,%d,%ld,%.5f,%.5f,%.5f,%.2f,%d,%d,%.2f,%.2f,%.2f,%.2f\n",
            difficulty ? "pressao" : "normal", names[policy], n, seed, wins, n-wins, p,
            center-margin, center+margin, turns/n, min_turn, max_turn, hp/n, money/n, kills/n, wave/n);
    }
    free(g);
    return 0;
}
