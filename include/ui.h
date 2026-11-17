#ifndef UI_H
#define UI_H
#include "game.h"
int ui_read(char *line, size_t capacity);
const char *ui_command(GameState *g, char *line, bool *quit, bool *menu, bool realtime);
const char *ui_instructions(void);
#endif
