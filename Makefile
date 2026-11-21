CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -g
BUILD = build
CORE = src/board.c src/game.c src/persistence.c
SOURCES = src/main.c src/ui.c src/render.c $(CORE)
HEADERS = $(wildcard include/*.h)

.PHONY: all run test sanitize simulate clean
N ?= 1000
SEED ?= 42
all: $(BUILD)/tower-defense

$(BUILD):
	mkdir -p $@

$(BUILD)/tower-defense: $(SOURCES) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $@

$(BUILD)/test-board: tests/test_board.c src/board.c include/board.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_board.c src/board.c $(LDFLAGS) -o $@

run: all
	./$(BUILD)/tower-defense

$(BUILD)/test-game: tests/test_game.c $(CORE) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_game.c $(CORE) $(LDFLAGS) -o $@

$(BUILD)/test-persistence: tests/test_persistence.c $(CORE) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_persistence.c $(CORE) $(LDFLAGS) -o $@

$(BUILD)/test-ui: tests/test_ui.c tests/stub_game_ui.c src/ui.c src/board.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_ui.c tests/stub_game_ui.c src/ui.c src/board.c $(LDFLAGS) -o $@

$(BUILD)/test-render: tests/test_render.c tests/stub_game_render.c src/render.c src/board.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_render.c tests/stub_game_render.c src/render.c src/board.c $(LDFLAGS) -o $@

$(BUILD)/simulate: tools/simulate.c $(CORE) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/simulate.c $(CORE) $(LDFLAGS) -lm -o $@

simulate: $(BUILD)/simulate
	./$(BUILD)/simulate $(N) $(SEED)

test: all $(BUILD)/test-board $(BUILD)/test-game $(BUILD)/test-persistence $(BUILD)/test-ui $(BUILD)/test-render $(BUILD)/simulate
	./$(BUILD)/test-board
	cd $(BUILD) && ./test-game
	cd $(BUILD) && ./test-persistence
	./$(BUILD)/test-ui
	./$(BUILD)/test-render
	sh tests/test_cli.sh ./$(BUILD)/tower-defense
	sh tests/test_simulate.sh ./$(BUILD)/simulate

sanitize:
	$(MAKE) BUILD=build/sanitize CFLAGS='$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' LDFLAGS='-fsanitize=address,undefined -no-pie' test

clean:
	$(RM) -r build
