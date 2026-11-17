#define _POSIX_C_SOURCE 200809L
#include "ui.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

static long tick_ns(int wave) { return (700L - 100L * (wave - 1)) * 1000000L; }

static void arm_tick(struct timespec *next, int wave) {
    clock_gettime(CLOCK_MONOTONIC, next);
    next->tv_nsec += tick_ns(wave);
    if (next->tv_nsec >= 1000000000L) { next->tv_sec++; next->tv_nsec -= 1000000000L; }
}

static bool tick_due(const struct timespec *next) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec > next->tv_sec ||
           (now.tv_sec == next->tv_sec && now.tv_nsec >= next->tv_nsec);
}

static const char *line_handle(GameState *g, char *line, bool *quit, bool *menu,
                               bool in_menu, char *buffer, size_t size, bool realtime) {
    if (!in_menu) {
        GameStatus previous = g->status;
        const char *message = ui_command(g, line, quit, menu, realtime);
        if (previous == PLAYING && g->status != PLAYING)
            message = ranking_record(g, "ranking.dat") ? "Partida encerrada."
                                                       : "Partida encerrada; erro ao gravar recorde.";
        return message;
    }
    if (!strcmp(line, "1")) { game_init(g, 0); *menu = false; return "Partida iniciada."; }
    if (!strcmp(line, "2")) return ui_instructions();
    if (!strcmp(line, "3")) {
        if (game_load(g, "savegame.dat")) { *menu = false; return "Partida carregada."; }
        return "Save ausente ou invalido; estado preservado.";
    }
    if (!strcmp(line, "4")) {
        int best;
        if (ranking_read("ranking.dat", &best)) {
            snprintf(buffer, size, "Recorde: %d pontos", best);
            return buffer;
        }
        return "Recorde ausente ou invalido.";
    }
    if (!strcmp(line, "5") || !strcmp(line, "q")) { *quit = true; return NULL; }
    if (!strcmp(line, "6")) { *menu = false; return "Sessao retomada."; }
    return "Comando invalido.";
}

int main(int argc, char **argv) {
    const char *term = getenv("TERM");
    bool interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    bool color = isatty(STDOUT_FILENO) && term && strcmp(term, "dumb") && !getenv("NO_COLOR");
    bool snapshot = false, menu = true;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--color")) color = true;
        else if (!strcmp(argv[i], "--no-color")) color = false;
        else if (!strcmp(argv[i], "--snapshot")) snapshot = true;
        else if (!strcmp(argv[i], "--play")) menu = false;
        else if (!strcmp(argv[i], "--help")) {
            puts("Uso: tower-defense [--color|--no-color] [--snapshot] [--play]\n"
                 "Terminal de 84 colunas. No terminal o tempo corre sozinho (p pausa).\n"
                 "Ultima opcao de cor prevalece.");
            return EXIT_SUCCESS;
        } else { fprintf(stderr, "Opcao desconhecida: %s\n", argv[i]); return EXIT_FAILURE; }
    }
    GameState *g = game_create();
    if (!g) { fputs("Sem memoria.\n", stderr); return EXIT_FAILURE; }
    if (snapshot) { game_render(stdout, g, color); free(g); return EXIT_SUCCESS; }

    char line[128], buffer[64];
    const char *message = "Bem-vindo!";
    bool quit = false;

    if (!interactive) {
        while (!quit) {
            if (menu) puts("JARDIM DE PEDRA\n1 Iniciar\n2 Instrucoes\n3 Carregar\n4 Recorde\n5 Sair\n6 Continuar sessao");
            else game_render(stdout, g, color);
            printf("\n%s\n> ", message); fflush(stdout);
            int read_status = ui_read(line, sizeof line);
            if (!read_status) break;
            if (read_status < 0) { message = "Comando muito longo ou com byte invalido."; continue; }
            const char *next = line_handle(g, line, &quit, &menu, menu, buffer, sizeof buffer, false);
            if (next) message = next;
        }
        puts("Ate logo!");
        free(g);
        return EXIT_SUCCESS;
    }

    struct termios saved;
    tcgetattr(STDIN_FILENO, &saved);
    struct termios raw = saved;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    fputs("\033[H\033[2J", stdout);
    puts("JARDIM DE PEDRA\n1 Iniciar\n2 Instrucoes\n3 Carregar\n4 Recorde\n5 Sair\n6 Continuar sessao");
    printf("\n%s\n> ", message);
    fflush(stdout);

    size_t length = 0;
    int esc = 0;
    bool paused = false, have_tick = false;
    struct timespec next_tick = {0, 0};
    while (!quit) {
        bool dirty = false;
        if (menu || g->status != PLAYING) have_tick = false;
        else if (!have_tick) { arm_tick(&next_tick, g->wave); have_tick = true; }
        if (have_tick && paused) arm_tick(&next_tick, g->wave);
        if (have_tick && !paused && tick_due(&next_tick)) {
            game_tick(g);
            dirty = true;
            if (g->status != PLAYING) {
                have_tick = false;
                message = ranking_record(g, "ranking.dat") ? "Partida encerrada."
                                                           : "Partida encerrada; erro ao gravar recorde.";
            } else {
                next_tick.tv_sec += tick_ns(g->wave) / 1000000000L;
                next_tick.tv_nsec += tick_ns(g->wave) % 1000000000L;
                if (next_tick.tv_nsec >= 1000000000L) { next_tick.tv_sec++; next_tick.tv_nsec -= 1000000000L; }
            }
        }
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        struct timeval timeout = {0, 250000};
        int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout);
        if (ready < 0 && errno != EINTR) break;
        if (ready > 0) {
            unsigned char ch;
            ssize_t n;
            while ((n = read(STDIN_FILENO, &ch, 1)) > 0) {
                if (esc) {
                    if (esc == 1 && (ch == '[' || ch == 'O')) esc = 2;
                    else if (esc == 1 || (ch >= 0x40 && ch <= 0x7E)) esc = 0;
                    continue;
                }
                if (ch == 27) { esc = 1; continue; }
                if (ch == 3 || ch == 4) { quit = true; break; }
                if (ch == '\n' || ch == '\r') {
                    line[length] = '\0';
                    if (!menu && !strcmp(line, "p")) {
                        paused = !paused;
                        arm_tick(&next_tick, g->wave);
                        message = paused ? "Pausado." : "Retomado.";
                    } else if (length) {
                        const char *next = line_handle(g, line, &quit, &menu, menu, buffer, sizeof buffer, true);
                        if (next) message = next;
                    }
                    length = 0;
                    line[0] = '\0';
                    dirty = true;
                } else if (ch == 127 || ch == 8) {
                    if (length) {
                        --length;
                        line[length] = '\0';
                        fputs("\b \b", stdout);
                        dirty = true;
                    }
                } else if (ch == 'p' && !length && !menu) {
                    paused = !paused;
                    arm_tick(&next_tick, g->wave);
                    message = paused ? "Pausado." : "Retomado.";
                    dirty = true;
                } else if (ch >= 32 && length + 1 < sizeof line) {
                    line[length++] = (char)ch;
                    fputc(ch, stdout);
                }
            }
            if (n < 0 && errno != EINTR) break;
            if (quit) break;
            fflush(stdout);
        }
        if (dirty) {
            fputs("\033[H\033[2J", stdout);
            if (menu) puts("JARDIM DE PEDRA\n1 Iniciar\n2 Instrucoes\n3 Carregar\n4 Recorde\n5 Sair\n6 Continuar sessao");
            else game_render(stdout, g, color);
            printf("\n%s\n> %s", message, line);
            fflush(stdout);
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &saved);
    puts("\nAte logo!");
    free(g);
    return EXIT_SUCCESS;
}
