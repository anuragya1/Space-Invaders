# Convenience wrapper around the CMake build.
#
# CMake is the official build path. This file keeps the old `make` though planning to completely move on cmake
# memory working without maintaining a second build graph.

CMAKE ?= cmake
BUILD_DIR ?= build
CMAKE_FLAGS ?=
BUILD_FLAGS ?= --parallel
CTEST_FLAGS ?= --output-on-failure

.PHONY: all configure sdl3 configure-sdl3 tests run-tests clean

all: configure
	$(CMAKE) --build $(BUILD_DIR) --target si_pro $(BUILD_FLAGS)

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

sdl3: configure-sdl3
	$(CMAKE) --build $(BUILD_DIR) --target si_pro_sdl3 $(BUILD_FLAGS)

configure-sdl3:
	$(CMAKE) -S . -B $(BUILD_DIR) -DSI_BUILD_SDL3=ON $(CMAKE_FLAGS)

tests: configure
	$(CMAKE) --build $(BUILD_DIR) $(BUILD_FLAGS)

run-tests: tests
	ctest --test-dir $(BUILD_DIR) $(CTEST_FLAGS)

clean:
	$(CMAKE) -E rm -rf $(BUILD_DIR)
