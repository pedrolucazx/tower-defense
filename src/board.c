#include "board.h"
#include <assert.h>
#include <string.h>

bool board_contains(int row, int col) {
    return row >= 0 && row < BOARD_ROWS && col >= 0 && col < BOARD_COLS;
}

void board_init(Board *board) {
    static const Position corners[] = {
        {5, 0}, {5, 9}, {2, 9}, {2, 6}, {7, 6}, {7, 11},
        {3, 11}, {3, 13}, {9, 13}, {9, 8}, {12, 8}
    };
    memset(board, 0, sizeof *board);
    for (int r = 0; r < BOARD_ROWS; ++r) {
        for (int c = 0; c < BOARD_COLS; ++c) {
            bool edge = r == 0 || r == BOARD_ROWS - 1 || c == 0 || c == BOARD_COLS - 1;
            bool corner = false;
            board->cells[r][c] = edge || corner ? TREE : GRASS;
        }
    }
    Position p = corners[0];
    board->path[board->path_length++] = p;
    board->cells[p.row][p.col] = PATH;
    for (size_t i = 1; i < sizeof corners / sizeof corners[0]; ++i) {
        Position target = corners[i];
        assert(p.row == target.row || p.col == target.col);
        while (p.row != target.row || p.col != target.col) {
            p.row += (target.row > p.row) - (target.row < p.row);
            p.col += (target.col > p.col) - (target.col < p.col);
            assert(board_contains(p.row, p.col));
            assert(board->path_length < PATH_CAPACITY);
            board->path[board->path_length++] = p;
            board->cells[p.row][p.col] = PATH;
        }
    }
    board->cells[5][0] = ENTRANCE;
    board->cells[p.row][p.col] = BASE;

    static const Position stones[] = {
        {7, 3}, {7, 4}, {7, 5}, {8, 3}, {8, 6}, {9, 3}, {9, 6}, {10, 3}, {10, 4}, {10, 5}, {10, 6}
    };
    for (size_t i = 0; i < sizeof stones / sizeof stones[0]; ++i)
        board->cells[stones[i].row][stones[i].col] = PATH;
}

bool board_place(Board *board, int row, int col, char tower) {
    if (!board_contains(row, col) || (tower != 'Y' && tower != 'Z')) return false;
    if (board->cells[row][col] != GRASS || board->towers[row][col]) return false;
    board->towers[row][col] = tower;
    return true;
}

bool board_advance(Board *board) {
    if (board->enemy_step + 1 >= board->path_length) return false;
    ++board->enemy_step;
    return true;
}
