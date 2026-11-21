#include "game.h"
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static bool token_read(FILE *f, char *token, size_t capacity) {
    size_t length = 0;
    int ch;
    do { ch = fgetc(f); } while (ch != EOF && isspace((unsigned char)ch));
    if (ch == EOF) return false;
    bool valid = true;
    do {
        if (!ch || length + 1 >= capacity) valid = false;
        else token[length++] = (char)ch;
        ch = fgetc(f);
    } while (ch != EOF && !isspace((unsigned char)ch));
    token[length] = '\0';
    return valid && !ferror(f);
}

static bool integer(FILE *f, int *value) {
    char token[48], *end;
    if (!token_read(f, token, sizeof token)) return false;
    errno = 0;
    long n = strtol(token, &end, 10);
    if (errno || end == token || *end || n < 0 || n > MAX_TURNS) return false;
    *value = (int)n;
    return true;
}

static bool header(FILE *f, const char *expected) {
    char word[48];
    return token_read(f, word, sizeof word) && strcmp(word, expected) == 0;
}

static bool end_of_file(FILE *f) {
    int ch;
    do { ch = fgetc(f); } while (ch != EOF && isspace((unsigned char)ch));
    return ch == EOF && !ferror(f);
}

static bool temp_name(const char *path, char *temp, size_t size) {
    int n = snprintf(temp, size, "%s.tmp", path);
    return n >= 0 && (size_t)n < size;
}

bool game_save(const GameState *g, const char *path) {
    char temp[1024];
    if (!game_valid(g) || !temp_name(path, temp, sizeof temp)) return false;
    FILE *f = fopen(temp, "w");
    if (!f) return false;
    fprintf(f, "TD2\n%d %d %d %d %d %d %d %d %d %d %d %d\n",
        g->difficulty, g->hp, g->money, g->wave, g->spawned, g->wave_turn,
        g->turn, g->kills, g->escaped, g->score, (int)g->status, g->tower_count);
    for (int i = 0; i < g->tower_count; ++i) {
        Tower t = g->towers[i];
        fprintf(f, "%d %d %d %d\n", t.row, t.col, t.type, t.cooldown);
    }
    for (int i = 0; i < MAX_ENEMIES; ++i)
        fprintf(f, "%d %d\n", g->enemies[i].hp, g->enemies[i].step);
    bool ok = !ferror(f);
    if (fclose(f) != 0) ok = false;
    if (ok) ok = rename(temp, path) == 0;
    if (!ok) remove(temp);
    return ok;
}

bool game_load(GameState *g, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    GameState *copy = game_create();
    if (!copy) { fclose(f); return false; }
    int status = 0;
    int *fields[] = {&copy->difficulty, &copy->hp, &copy->money, &copy->wave,
        &copy->spawned, &copy->wave_turn, &copy->turn, &copy->kills, &copy->escaped,
        &copy->score, &status, &copy->tower_count};
    bool ok = header(f, "TD2");
    for (size_t i = 0; ok && i < sizeof fields / sizeof fields[0]; ++i) ok = integer(f, fields[i]);
    if (status > DEFEAT || copy->tower_count > MAX_TOWERS) ok = false;
    copy->status = (GameStatus)status;
    for (int i = 0; ok && i < copy->tower_count; ++i) {
        Tower *t = &copy->towers[i];
        ok = integer(f, &t->row) && integer(f, &t->col) && integer(f, &t->type) && integer(f, &t->cooldown);
        if (ok) ok = t->type >= 0 && t->type < TOWER_TYPES &&
                     board_place(&copy->board, t->row, t->col, game_tower_letter(t->type));
    }
    for (int i = 0; ok && i < MAX_ENEMIES; ++i)
        ok = integer(f, &copy->enemies[i].hp) && integer(f, &copy->enemies[i].step);
    if (ok) ok = end_of_file(f) && game_valid(copy);
    if (fclose(f) != 0) ok = false;
    if (ok) *g = *copy;
    free(copy);
    return ok;
}

bool ranking_read(const char *path, int *best) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    int value = 0;
    bool ok = header(f, "TDR1") && integer(f, &value) && value <= 10 * game_enemy_total() + 100 &&
              end_of_file(f);
    if (fclose(f) != 0) ok = false;
    if (ok) *best = value;
    return ok;
}

bool ranking_record(const GameState *g, const char *path) {
    if (g->status == PLAYING || !game_valid(g)) return false;
    int best = 0;
    FILE *existing = fopen(path, "r");
    if (existing) {
        fclose(existing);
        if (!ranking_read(path, &best)) return false;
    } else if (errno != ENOENT) return false;
    if (g->score > best) best = g->score;
    char temp[1024];
    if (!temp_name(path, temp, sizeof temp)) return false;
    FILE *f = fopen(temp, "w");
    if (!f) return false;
    bool ok = fprintf(f, "TDR1 %d\n", best) > 0;
    if (fclose(f) != 0) ok = false;
    if (ok) ok = rename(temp, path) == 0;
    if (!ok) remove(temp);
    return ok;
}
