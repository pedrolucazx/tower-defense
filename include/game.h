#ifndef GAME_H
#define GAME_H
#include "board.h"
#include <stdio.h>
#define MAX_TOWERS (BOARD_ROWS * BOARD_COLS)
#define MAX_ENEMIES 32
#define MAX_TURNS 2000
#define MAX_WAVES 5
#define INITIAL_HP 10
#define INITIAL_MONEY 120
#define REWARD 8
#define TOWER_TYPES 2
typedef enum { PLAYING, VICTORY, DEFEAT } GameStatus;
typedef struct { int cost, range, damage, period; } TowerStats;
typedef struct { int row, col, type, cooldown; } Tower;
typedef struct { int hp, step; } Enemy;
typedef struct {
    Board board;
    Tower towers[MAX_TOWERS];
    Enemy enemies[MAX_ENEMIES];
    int tower_count, hp, money, wave, spawned, wave_turn, turn;
    int kills, escaped, score, difficulty;
    GameStatus status;
} GameState;
extern const TowerStats TOWER_STATS[TOWER_TYPES];
char game_tower_letter(int type);
int game_enemy_hp(int wave, int difficulty);
int game_wave_size(int wave);
int game_enemy_total(void);
void game_init(GameState *g, int difficulty);
GameState *game_create(void);
int game_active(const GameState *g);
bool game_buy(GameState *g, int row, int col, int type);
void game_tick(GameState *g);
bool game_valid(const GameState *g);
void game_render(FILE *out, const GameState *g, bool color);
bool game_save(const GameState *g, const char *path);
bool game_load(GameState *g, const char *path);
bool ranking_record(const GameState *g, const char *path);
bool ranking_read(const char *path, int *best);
#endif
