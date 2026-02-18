# config.default.mk

CC       := gcc
CFLAGS   ?= -Wall -Wextra -std=c17 -Wno-unused-variable -O2
LDFLAGS  ?= 
INCLUDES ?= -Isrc

# Detect llvm-config (allow override via environment or command-line)
LLVM_CONFIG ?= llvm-config

ifeq ($(OS),Windows_NT)
LLVM_CONFIG ?= llvm-config.exe
else
# Non-Windows (macOS / Linux)
LLVM_CONFIG_AVAILABLE := $(shell $(LLVM_CONFIG) --version >/dev/null 2>&1 && echo yes || echo no)
ifeq ($(LLVM_CONFIG_AVAILABLE),no)
# macOS Homebrew fallback
BREW_LLVM_CONFIG := $(shell brew --prefix llvm 2>/dev/null)/bin/llvm-config
BREW_LLVM_AVAILABLE := $(shell $(BREW_LLVM_CONFIG) --version >/dev/null 2>&1 && echo yes || echo no)
ifeq ($(BREW_LLVM_AVAILABLE),yes)
LLVM_CONFIG := $(BREW_LLVM_CONFIG)
else
$(error llvm-config not found. Install LLVM or set LLVM_CONFIG=/path/to/llvm-config)
endif
endif
endif

# LLVM configuration - request all necessary components
LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs --system-libs all)

# Add LLVM flags to existing flags  
override CFLAGS += $(LLVM_CFLAGS)
override LDFLAGS += $(LLVM_LDFLAGS) $(LLVM_LIBS) -lstdc++

UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Darwin)
override LDFLAGS := $(filter-out -lstdc++,$(LDFLAGS)) -lc++
endif

SRC_DIR  = src
OBJ_DIR  = build

# Recursive function to find all .c files
define find_c_sources
$(wildcard $(1)/*.c) \
$(foreach d,$(wildcard $(1)/*),$(call find_c_sources,$(d)))
endef

# Find all source files recursively
SRC_FILES := $(call find_c_sources,$(SRC_DIR))

# Generate object file paths
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
