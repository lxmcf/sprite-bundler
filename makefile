# ----------------------------------------------------------------------------------
#  CONFIG
# ----------------------------------------------------------------------------------
EXE ?= game
ASSET_DIR ?= assets/
LIB_DIR ?= lib/
DATA_DIR ?= data/
BUILD_DIR ?= .BUILD/

BUILD ?=
PLATFORM ?= PLATFORM_DESKTOP

PLATFORM_OS := Linux
RUNNER :=
CC ?= gcc

SRC_FILES := $(wildcard src/*.c) $(wildcard src/*/*.c)
INCLUDES :=
DEFINES :=
EXE_EXT :=

# ----------------------------------------------------------------------------------
#  CONFIG SETUP
# ----------------------------------------------------------------------------------
ifeq ($(PLATFORM), PLATFORM_WEB)
	PLATFORM_OS = WEB
	BUILD_DIR := $(BUILD_DIR)Web/
	LIB_DIR := $(LIB_DIR)Web/

	EXE_EXT = .html
else
	ifeq ($(OS), Windows_NT)
		PLATFORM_OS = WINDOWS
		BUILD_DIR := $(BUILD_DIR)Windows/
		LIB_DIR := $(LIB_DIR)Windows/

		EXE_EXT = .exe
	else
		UNAME_OS = $(shell uname)

		ifeq ($(UNAME_OS), Linux)
			PLATFORM_OS = LINUX
			BUILD_DIR := $(BUILD_DIR)Linux/
			LIB_DIR := $(LIB_DIR)Linux/
		endif
	endif
endif


ifeq ($(BUILD), BUILD_DEBUG)
	COMPILER_FLAGS += -g
	DEFINES += -DBUILD_DEBUG
endif

ifeq ($(BUILD), BUILD_RELEASE)
	COMPILER_FLAGS += -O3
	DEFINES += -DBUILD_RELEASE
endif

# ----------------------------------------------------------------------------------
#  PLATFORM DEFINITIONS
# ----------------------------------------------------------------------------------
ifeq ($(PLATFORM), PLATFORM_DESKTOP)
	DEFINES += -DPLATFORM_DESKTOP
	CC = gcc

	ifeq ($(PLATFORM_OS), WINDOWS)
		DEFINES += -DPLATFORM_WINDOWS
	endif

	ifeq ($(PLATFORM_OS), LINUX)
		DEFINES += -DPLATFORM_LINUX
	endif
endif

ifeq ($(PLATFORM), PLATFORM_WEB)
	DEFINES += -DPLATFORM_WEB

	CC = emcc
endif

# ----------------------------------------------------------------------------------
#  COMPILER AND LINKER ARGUMENTS
# ----------------------------------------------------------------------------------
CFLAGS := -Wall -pedantic
LD_FLAGS :=
INCLUDES += -Iinclude

ifeq ($(PLATFORM), PLATFORM_DESKTOP)
	ifeq ($(PLATFORM_OS), WINDOWS)
		LD_FLAGS += -lopengl32 -lgdi32 -lwinmm -lraylib -mwindows
	endif

	ifeq ($(PLATFORM_OS), LINUX)
		LD_FLAGS += -lGL -lm -lpthread -ldl -lrt -lX11 -lraylib
	endif
endif

ifeq ($(PLATFORM), PLATFORM_WEB)
	EXE_EXT = .html
	RUNNER = emrun

	CC = emcc

	LD_FLAGS += -s USE_GLFW=3 -s ASYNCIFY --preload-file $(ASSET_DIR) --shell-file $(DATA_DIR)shell.html
	SRC_FILES := $(SRC_FILES) $(LIB_DIR)libraylib.a
endif

# ----------------------------------------------------------------------------------
#  TARGETS
# ----------------------------------------------------------------------------------
.PHONY: build run clean package

build: $(EXE)$(EXE_EXT)

$(EXE)$(EXE_EXT): $(SRC_FILES)
	mkdir -p $(BUILD_DIR)
	$(CC) $(DEFINES) $(CFLAGS) $(INCLUDES) $^ $(COMPILER_FLAGS) $(LD_FLAGS) -o $(BUILD_DIR)$@

run: build
ifeq ($(PLATFORM), PLATFORM_DESKTOP)
	./$(BUILD_DIR)$(EXE)$(EXE_EXT)
else
	$(RUNNER) $(BUILD_DIR)$(EXE)$(EXE_EXT)
endif

clean:
	rm -rf $(BUILD_DIR)

package:
ifeq ($(PLATFORM), PLATFORM_DESKTOP)
	zip -r $(BUILD_DIR)$(EXE)$(EXE_EXT) $(ASSET_DIR)
endif

ifeq ($(PLATFORM), PLATFORM_WEB)
	zip -rj $(EXE).zip $(BUILD_DIR)
endif
