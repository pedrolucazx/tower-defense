#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define BOARD_ROWS 13
#define BOARD_COLS 16
#define PATH_CAPACITY (BOARD_ROWS * BOARD_COLS)

typedef struct { int row, col; } Position;
typedef enum { GRASS, TREE, PATH, ENTRANCE, BASE } Terrain;
typedef struct {
    Terrain cells[BOARD_ROWS][BOARD_COLS];
    char towers[BOARD_ROWS][BOARD_COLS];
    Position path[PATH_CAPACITY];
    size_t path_length;
    size_t enemy_step; /* Index in the ordered path, including crossings. */
} Board;

void board_init(Board *board);
bool board_contains(int row, int col);
bool board_place(Board *board, int row, int col, char tower);
bool board_advance(Board *board);
void board_render(FILE *out, const Board *board, bool color);

#endif
