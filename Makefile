## Makefile - simple build for graders without cmake.
##
##   make            # build ./si_pro          (terminal version)
##   make sdl3       # build ./si_pro_sdl3     (windowed version, needs SDL3)
##   make tests      # build tests in ./build/tests/
##   make run-tests  # build + run tests
##   make clean

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc
LDFLAGS  ?=
LDLIBS   ?=

UNAME_S := $(shell uname -s 2>/dev/null)
ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(findstring CYGWIN,$(UNAME_S))$(findstring UCRT,$(UNAME_S)))
    # Windows (MSYS2 / MinGW / Cygwin / UCRT64 shells)
    PLATFORM_SRC = src/platform/platform_win32.cpp
    LDLIBS      += -lws2_32
    BIN_EXT      = .exe
else ifeq ($(UNAME_S),)
    # uname not available at all - assume bare Windows MinGW
    PLATFORM_SRC = src/platform/platform_win32.cpp
    LDLIBS      += -lws2_32
    BIN_EXT      = .exe
else
    PLATFORM_SRC = src/platform/platform_posix.cpp
    LDFLAGS     += -pthread
    BIN_EXT      =
endif

CORE_SRCS = \
    src/core/rng.cpp \
    src/core/difficulty.cpp \
    src/config/config.cpp \
    src/config/cli.cpp \
    src/debug/logger.cpp \
    src/render/rbuf.cpp \
    src/persistence/save_state.cpp \
    src/persistence/leaderboard.cpp \
    src/persistence/stats.cpp \
    src/persistence/achievements.cpp \
    src/persistence/replay_file.cpp \
    src/persistence/level_file.cpp \
    src/persistence/telemetry.cpp \
    src/net/tcp_socket.cpp \
    src/input/input_source.cpp \
    src/input/keyboard_source.cpp \
    src/input/ai_source.cpp \
    src/input/replay_source.cpp \
    src/input/coop_source.cpp \
    src/game/game.cpp \
    src/game/game_step.cpp \
    src/game/game_render.cpp \
    src/editor/level_editor.cpp \
    src/ui/banner.cpp \
    src/ui/menus.cpp \
    src/ui/tools.cpp \
    src/i18n/strings.cpp \
    $(PLATFORM_SRC)

CORE_OBJS = $(CORE_SRCS:.cpp=.o)

BIN = si_pro$(BIN_EXT)

# ---- SDL3 build (parallel target, does not affect terminal build) ----
#
# Build with:   make sdl3
#
# Requires SDL3 3.2+ headers + libraries.
#
# On POSIX with pkg-config, the flags are discovered automatically. If your
# SDL3 install isn't on the default pkg-config path, set PKG_CONFIG_PATH
# (e.g. export PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig).
#
# On Windows (MinGW) without pkg-config, override the paths:
#   make sdl3 SDL3_DIR=C:/SDL3
# That expects $SDL3_DIR/include/SDL3/SDL.h and $SDL3_DIR/lib/libSDL3.dll.a
# (or a Windows-flavored equivalent).
SDL3_DIR ?=
ifneq ($(SDL3_DIR),)
    SDL3_CFLAGS = -I$(SDL3_DIR)/include
    SDL3_LIBS   = -L$(SDL3_DIR)/lib -lSDL3
else
    # POSIX path: rely on pkg-config sdl3
    SDL3_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
    SDL3_LIBS   := $(shell pkg-config --libs   sdl3 2>/dev/null)
    ifeq ($(SDL3_LIBS),)
        SDL3_LIBS = -lSDL3
    endif
endif

BIN_SDL3 = si_pro_sdl3$(BIN_EXT)

SDL3_EXTRA_SRCS = \
    src/render/sdl3_renderer.cpp \
    src/render/sdl3_particles.cpp \
    src/render/sdl3_sprites.cpp \
    src/audio/sdl3_audio.cpp \
    src/ui/sdl3_menu.cpp \
    src/ui/sdl3_screens.cpp \
    src/input/sdl3_keyboard.cpp \
    src/director/director.cpp

SDL3_EXTRA_OBJS = $(SDL3_EXTRA_SRCS:.cpp=.o)

TESTS = test_rng test_replay test_level test_ai test_determinism
TEST_BINS = $(addprefix build/tests/, $(TESTS))

all: $(BIN)

$(BIN): src/main.o $(CORE_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

sdl3: $(BIN_SDL3)

$(BIN_SDL3): src/main_sdl3.o $(CORE_OBJS) $(SDL3_EXTRA_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(SDL3_LIBS) $(LDLIBS)

# SDL3 translation units need the include path.
src/main_sdl3.o: src/main_sdl3.cpp
	$(CXX) $(CXXFLAGS) $(SDL3_CFLAGS) -c -o $@ $<

src/render/sdl3_renderer.o: src/render/sdl3_renderer.cpp
	$(CXX) $(CXXFLAGS) $(SDL3_CFLAGS) -c -o $@ $<

src/render/sdl3_particles.o: src/render/sdl3_particles.cpp
	$(CXX) $(CXXFLAGS) $(SDL3_CFLAGS) -c -o $@ $<

src/render/sdl3_sprites.o: src/render/sdl3_sprites.cpp
	$(CXX) $(CXXFLAGS) $(SDL3_CFLAGS) -c -o $@ $<

src/audio/sdl3_audio.o: src/audio/sdl3_audio.cpp
	$(CXX) $(CXXFLAGS) $(SDL3_CFLAGS) -c -o $@ $<

src/ui/sdl3_menu.o: src/ui/sdl3_menu.cpp
	$(CXX) $(CXXFLAGS) $(SDL3_CFLAGS) -c -o $@ $<

src/ui/sdl3_screens.o: src/ui/sdl3_screens.cpp
	$(CXX) $(CXXFLAGS) $(SDL3_CFLAGS) -c -o $@ $<

src/input/sdl3_keyboard.o: src/input/sdl3_keyboard.cpp
	$(CXX) $(CXXFLAGS) $(SDL3_CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

tests: $(TEST_BINS)

build/tests/%: tests/%.cpp $(CORE_OBJS) | build/tests
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(CORE_OBJS) $(LDLIBS)

build/tests:
	@mkdir -p $@

run-tests: tests
	@for t in $(TEST_BINS); do \
	    echo "==> $$t"; \
	    $$t || exit 1; \
	done
	@echo "All tests passed."

clean:
	rm -f $(BIN) $(BIN_SDL3) src/main.o src/main_sdl3.o $(CORE_OBJS) $(SDL3_EXTRA_OBJS)
	rm -rf build

.PHONY: all sdl3 tests run-tests clean
