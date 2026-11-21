CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -g
BUILD = build
N ?= 1000
SEED ?= 42
CORE = src/board.c src/game.c src/persistence.c
TESTS = test-board test-game test-persistence test-ui test-render
BINS = $(addprefix $(BUILD)/,tower-defense simulate $(TESTS))

.PHONY: all run test sanitize simulate clean
all: $(BUILD)/tower-defense

$(BUILD):
	mkdir -p $@

$(BUILD)/tower-defense: src/main.c src/ui.c src/render.c $(CORE)
$(BUILD)/simulate: tools/simulate.c $(CORE)
$(BUILD)/simulate: LDLIBS = -lm
$(BUILD)/test-board: tests/test_board.c src/board.c
$(BUILD)/test-game: tests/test_game.c $(CORE)
$(BUILD)/test-persistence: tests/test_persistence.c $(CORE)
$(BUILD)/test-ui: tests/test_ui.c tests/stub_game_ui.c src/ui.c src/board.c
$(BUILD)/test-render: tests/test_render.c tests/stub_game_render.c src/render.c src/board.c

$(BINS): $(wildcard include/*.h) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) $(LDLIBS) -o $@

run: all
	./$(BUILD)/tower-defense

simulate: $(BUILD)/simulate
	./$(BUILD)/simulate $(N) $(SEED)

test: $(BINS)
	cd $(BUILD) && for t in $(TESTS); do ./$$t || exit 1; done
	sh tests/test_cli.sh ./$(BUILD)/tower-defense
	sh tests/test_simulate.sh ./$(BUILD)/simulate

sanitize:
	$(MAKE) BUILD=build/sanitize CFLAGS='$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' LDFLAGS='-fsanitize=address,undefined -no-pie' test

clean:
	$(RM) -r build $(BUILD)
