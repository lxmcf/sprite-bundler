# ----------------------------------------------------------------------------------
#  CONFIG
# ----------------------------------------------------------------------------------
EXE ?= game
BUILD_DIR ?= .BUILD/

BUILD ?=

PLATFORM_OS := Linux
CC ?= clang

SRC_FILES := $(wildcard src/*.c) $(wildcard src/*/*.c)
INCLUDES :=
DEFINES :=
EXE_EXT :=

# ----------------------------------------------------------------------------------
#  CONFIG SETUP
# ----------------------------------------------------------------------------------
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

ifeq ($(PLATFORM_OS), WINDOWS)
	DEFINES += -DPLATFORM_WINDOWS
endif

ifeq ($(PLATFORM_OS), LINUX)
	DEFINES += -DPLATFORM_LINUX
endif

# ----------------------------------------------------------------------------------
#  COMPILER AND LINKER ARGUMENTS
# ----------------------------------------------------------------------------------
CFLAGS := -Wall -pedantic
LD_FLAGS :=
INCLUDES += -Iinclude

ifeq ($(PLATFORM_OS), WINDOWS)
	LD_FLAGS += -lopengl32 -lgdi32 -lwinmm -lraylib -mwindows
endif

ifeq ($(PLATFORM_OS), LINUX)
	LD_FLAGS += -lGL -lm -lpthread -ldl -lrt -lX11 -lraylib
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
	./$(BUILD_DIR)$(EXE)$(EXE_EXT)

clean:
	rm -rf $(BUILD_DIR)

package:
	zip -r $(BUILD_DIR)$(EXE)$(EXE_EXT) $(ASSET_DIR)
