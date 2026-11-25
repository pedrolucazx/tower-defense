#include "board.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    Board *b = malloc(sizeof *b);
    assert(b != NULL);
    board_init(b);
    assert(b->path_length == 46);
    assert(b->path[0].row == 5 && b->path[0].col == 0);
    Position last = b->path[b->path_length - 1];
    assert(last.row == BOARD_ROWS - 1 && last.col == 8);
    for (size_t i = 0; i < b->path_length; ++i) {
        Position p = b->path[i];
        assert(board_contains(p.row, p.col));
        assert(b->cells[p.row][p.col] != GRASS);
        assert(b->cells[p.row][p.col] != TREE);
        assert(!board_place(b, p.row, p.col, 'Y'));
        if (i) {
            Position previous = b->path[i - 1];
            assert(abs(p.row - previous.row) + abs(p.col - previous.col) == 1);
        }
    }
    assert(board_place(b, 2, 3, 'Y'));
    assert(board_place(b, 1, 7, 'Y'));
    assert(board_place(b, 8, 4, 'Y'));
    assert(board_place(b, 8, 9, 'Z'));
    assert(b->towers[8][9] == 'Z');
    assert(!board_contains(-1, 0) && !board_contains(0, BOARD_COLS));
    assert(!board_place(b, BOARD_ROWS, 0, 'Y'));
    assert(!board_place(b, 0, 0, 'Y'));
    assert(!board_place(b, 5, 5, 'Z'));
    assert(!board_place(b, 4, 4, 'A'));
    assert(board_place(b, 3, 5, 'Y'));
    assert(!board_place(b, 3, 5, 'Y'));
    assert(board_place(b, 3, 7, 'Y'));
    for (size_t i = 1; i < b->path_length; ++i) {
        assert(board_advance(b));
        assert(b->enemy_step == i);
    }
    assert(!board_advance(b));
    assert(b->enemy_step == b->path_length - 1);
    board_init(b);
    assert(b->enemy_step == 0 && b->towers[5][5] == 0);
    Board other;
    board_init(&other);
    assert(memcmp(b, &other, sizeof other) == 0);
    free(b);
    puts("OK: trilha, cruzamento, limites, torres, percurso e reinicio.");
    return 0;
}
