# HULK Compiler Enhanced - Unified Makefile
# Cross-platform support with automatic LLVM detection

# Detect OS
ifeq ($(OS),Windows_NT)
    detected_OS := Windows
    # Executable extension for Windows
    EXE_EXT := .exe
    # Default LLVM location for Windows
    LLVM_DIR ?= C:/Program Files/LLVM
    # Shell command for Windows (using PowerShell)
    RM_CMD := powershell -Command "Remove-Item -Force -Recurse"
    MKDIR_CMD := powershell -Command "if (-Not (Test-Path"
    MKDIR_END := ")) { New-Item -ItemType Directory -Path"
    DIR_END := "}"
    LLVM_CHECK := if exist "$(LLVM_DIR)\bin\llvm-config.exe" (echo 1) else (echo 0)
else
    detected_OS := $(shell uname -s)
    # No extension for Unix executables
    EXE_EXT := 
    # Default Unix commands
    RM_CMD := rm -rf
    MKDIR_CMD := mkdir -p
    MKDIR_END :=
    DIR_END := 
    # Default LLVM location for Unix-like OSes
    LLVM_CONFIG ?= llvm-config
    LLVM_CHECK := which $(LLVM_CONFIG) 2>/dev/null
endif

# Compiler and tools
CXX := g++
CC := gcc
FLEX := flex
BISON := bison

# Basic compilation flags
CXXFLAGS = -std=c++17 -Wall -Wextra -I src -DENABLE_LLVM=0
CFLAGS = -std=c99 -Wall -Wextra

# Source files
LEXER_SRC = src/Lexer/lexer.l
PARSER_SRC = src/Parser/parser.y
MAIN_SRC = src/main.cpp
RUNTIME_SRC = src/Runtime/hulk_runtime.c

# Generated files
LEXER_GEN = src/Lexer/lex.yy.cpp
PARSER_GEN_CPP = src/Parser/parser.tab.cpp
PARSER_GEN_HPP = src/Parser/parser.tab.hpp

# Source directories
AST_SOURCES = $(wildcard src/AST/*.cpp)
EVALUATOR_SOURCES = $(wildcard src/Evaluator/*.cpp)
PRINTVISITOR_SOURCES = $(wildcard src/PrintVisitor/*.cpp)
VALUE_SOURCES = $(wildcard src/Value/*.cpp)
SCOPE_SOURCES = $(wildcard src/Scope/*.cpp)
SEMANTIC_SOURCES = $(wildcard src/Semantic/*.cpp)

# Object files
PARSER_OBJ = $(PARSER_GEN_CPP:.cpp=.o)
LEXER_OBJ = $(LEXER_GEN:.cpp=.o)
MAIN_OBJ = $(MAIN_SRC:.cpp=.o)
RUNTIME_OBJ = $(RUNTIME_SRC:.c=.o)

AST_OBJS = $(AST_SOURCES:.cpp=.o)
EVALUATOR_OBJS = $(EVALUATOR_SOURCES:.cpp=.o)
PRINTVISITOR_OBJS = $(PRINTVISITOR_SOURCES:.cpp=.o)
VALUE_OBJS = $(VALUE_SOURCES:.cpp=.o)
SCOPE_OBJS = $(SCOPE_SOURCES:.cpp=.o)
SEMANTIC_OBJS = $(SEMANTIC_SOURCES:.cpp=.o)

ALL_OBJS = $(PARSER_OBJ) $(LEXER_OBJ) $(MAIN_OBJ) $(RUNTIME_OBJ) \
           $(AST_OBJS) $(EVALUATOR_OBJS) $(PRINTVISITOR_OBJS) \
           $(VALUE_OBJS) $(SCOPE_OBJS) $(SEMANTIC_OBJS)
#           $(SEMANTIC_OBJS)  # Temporarily disabled

# Output
BIN_DIR = hulk
EXECUTABLE = $(BIN_DIR)/hulk_compiler.exe

# Default target
all: compile

# Check LLVM installation
check-llvm:
	@echo "Checking LLVM installation..."
	@if exist "$(LLVM_BIN)\clang.exe" ( \
		echo LLVM found at: $(LLVM_DIR) && \
		echo LLVM support could be enabled \
	) else ( \
		echo LLVM not found at: $(LLVM_DIR) && \
		echo Building without LLVM support \
	)

# Generate parser
$(PARSER_GEN_CPP) $(PARSER_GEN_HPP): $(PARSER_SRC)
	bison -d -o $(PARSER_GEN_CPP) $(PARSER_SRC)

# Generate lexer
$(LEXER_GEN): $(LEXER_SRC) $(PARSER_GEN_HPP)
	flex -o $(LEXER_GEN) $(LEXER_SRC)

# Compile C++ files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create directory
$(BIN_DIR):
	mkdir $(BIN_DIR)

# Main compilation
compile: $(EXECUTABLE)

$(EXECUTABLE): $(PARSER_GEN_CPP) $(LEXER_GEN) $(ALL_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(ALL_OBJS) -o $@
	@echo "Build successful! Executable: $(EXECUTABLE)"
	@echo "Available modes: --interpret (default), --semantic"

# Test targets
test-interpret: compile
	./$(EXECUTABLE) --debug test_enhanced.hulk

test-semantic: compile
	./$(EXECUTABLE) --debug --semantic test_enhanced.hulk

# Run targets
# Default file to run if none is specified
FILE ?= hulk.hulk
ARGS ?=

run: compile
	./$(EXECUTABLE) $(ARGS) $(FILE)

run-debug: compile
	./$(EXECUTABLE) --debug $(ARGS) $(FILE)

run-semantic: compile
	./$(EXECUTABLE) --semantic $(ARGS) $(FILE)

# Execute script.hulk
execute: compile
	./$(EXECUTABLE) script.hulk

# Build info
info:
	@echo "=== HULK Compiler Enhanced Build Information ==="
	@echo "CXX: $(CXX)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "Source files found:"
	@echo "  AST: $(words $(AST_SOURCES)) files"
	@echo "  Evaluator: $(words $(EVALUATOR_SOURCES)) files"
	@echo "  Print Visitor: $(words $(PRINTVISITOR_SOURCES)) files"
	@echo "  Value: $(words $(VALUE_SOURCES)) files"
	@echo "  Scope: $(words $(SCOPE_SOURCES)) files"
	@echo "  Semantic: $(words $(SEMANTIC_SOURCES)) files"

# Clean
clean:
	del /Q /F $(subst /,\,$(ALL_OBJS)) 2>nul || exit 0
	del /Q /F $(subst /,\,$(LEXER_GEN)) $(subst /,\,$(PARSER_GEN_CPP)) $(subst /,\,$(PARSER_GEN_HPP)) 2>nul || exit 0
	del /Q /F $(subst /,\,$(EXECUTABLE)) 2>nul || exit 0

distclean: clean
	rmdir /S /Q $(BIN_DIR) 2>nul || exit 0

# Help
help:
	@echo "=== HULK Compiler Enhanced Build System ==="
	@echo ""	@echo "Main targets:"
	@echo "  compile     - Build the compiler"	@echo "  execute     - Execute script.hulk from the root directory"
	@echo "  test-interpret  - Test interpretation mode"
	@echo "  test-semantic   - Test semantic analysis mode"
	@echo "  run         - Run in interpretation mode (FILE=filename.hulk to specify file)"
	@echo "  run-debug   - Run with debug output (FILE=filename.hulk to specify file)"
	@echo "  run-semantic - Run semantic analysis only (FILE=filename.hulk to specify file)"
	@echo "  check-llvm  - Check LLVM installation"
	@echo "  info        - Show build information"
	@echo "  clean       - Clean build artifacts"
	@echo "  help        - Show this help"

.PHONY: all compile execute test-interpret test-semantic run run-debug run-semantic check-llvm info clean distclean help