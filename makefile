# HULK Compiler - Makefile Multiplataforma
# Cumple con los requisitos específicos:
# 1. Clonar repositorio
# 2. Copiar script.hulk en directorio base
# 3. make compile -> genera directorio hulk/ con artifacts
# 4. make execute -> ejecuta el programa (con dependencia automática en compile)

# ==================== DETECCIÓN DE ENTORNO ====================

# Detectar sistema operativo y entorno
ifeq ($(OS),Windows_NT)
    # Windows - Detectar si estamos en MSYS2/MinGW
    ifdef MSYSTEM
        # Estamos en MSYS2/MinGW
        DETECTED_OS := Windows-MSYS2
        EXE_EXT := .exe
        SHELL_TYPE := bash
        PATH_SEP := /
        LLVM_CONFIG := llvm-config
        LLVM_DIR := $(shell llvm-config --prefix 2>/dev/null || echo "/mingw64")
    else
        # Windows nativo (CMD/PowerShell)
        DETECTED_OS := Windows-Native
        EXE_EXT := .exe
        SHELL_TYPE := cmd
        PATH_SEP := \\
        LLVM_DIR := C:\msys64\mingw64
        LLVM_CONFIG := $(LLVM_DIR)\bin\llvm-config.exe
    endif
else
    # Unix-like (Linux, macOS, etc.)
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        DETECTED_OS := Linux
    else ifeq ($(UNAME_S),Darwin)
        DETECTED_OS := macOS
    else
        DETECTED_OS := Unix
    endif
    EXE_EXT := 
    SHELL_TYPE := bash
    PATH_SEP := /
    LLVM_CONFIG := llvm-config
    LLVM_DIR := $(shell llvm-config --prefix 2>/dev/null || echo "/usr")
endif

# ==================== DETECCIÓN DE LLVM ====================

# Verificar disponibilidad de LLVM
ifeq ($(DETECTED_OS),Windows-Native)
    # Windows nativo - verificar si existe llvm-config.exe
    HAS_LLVM := $(shell if exist "$(LLVM_CONFIG)" (echo 1) else (echo 0))
else
    # Unix-like y MSYS2 - usar which/command
    HAS_LLVM := $(shell command -v $(LLVM_CONFIG) >/dev/null 2>&1 && echo 1 || echo 0)
endif

# ==================== CONFIGURACIÓN DE COMPILADORES ====================

ifeq ($(HAS_LLVM),1)
    # LLVM disponible - usar Clang con soporte completo
    CXX := clang++
    CC := clang
    ENABLE_LLVM := 1
    
    # Obtener flags de LLVM
    LLVM_CXXFLAGS_RAW := $(shell $(LLVM_CONFIG) --cxxflags 2>/dev/null)
    # Filtrar flags problemáticos y agregar excepciones
    LLVM_CXXFLAGS := $(filter-out -fno-exceptions,$(LLVM_CXXFLAGS_RAW)) -fexceptions
    LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --libs core 2>/dev/null)
    
    CXXFLAGS = -std=c++17 -Wall -Wextra -I src -DENABLE_LLVM=1 -fexceptions $(LLVM_CXXFLAGS)
    LDFLAGS = $(LLVM_LDFLAGS)
    LLVM_STATUS := Habilitado ($(shell $(LLVM_CONFIG) --version 2>/dev/null))
else
    # Fallback sin LLVM - usar GCC
    CXX := g++
    CC := gcc
    ENABLE_LLVM := 0
    
    CXXFLAGS = -std=c++17 -Wall -Wextra -I src -DENABLE_LLVM=0
    LDFLAGS = -lm
    LLVM_STATUS := No disponible - usando fallback
endif

# ==================== HERRAMIENTAS DE BUILD ====================

FLEX := flex
BISON := bison

# ==================== ARCHIVOS FUENTE ====================

# Archivos principales
LEXER_SRC = src/Lexer/lexer.l
PARSER_SRC = src/Parser/parser.y
MAIN_SRC = src/main.cpp
RUNTIME_SRC = src/Runtime/hulk_runtime.c

# Archivos generados
LEXER_GEN = src/Lexer/lex.yy.cpp
PARSER_GEN_CPP = src/Parser/parser.tab.cpp
PARSER_GEN_HPP = src/Parser/parser.tab.hpp

# Directorios de código fuente
AST_SOURCES = $(wildcard src/AST/*.cpp)
EVALUATOR_SOURCES = $(wildcard src/Evaluator/*.cpp)
PRINTVISITOR_SOURCES = $(wildcard src/PrintVisitor/*.cpp)
VALUE_SOURCES = $(wildcard src/Value/*.cpp)
SCOPE_SOURCES = $(wildcard src/Scope/*.cpp)
SEMANTIC_SOURCES = $(wildcard src/Semantic/*.cpp)
CODEGEN_SOURCES = $(wildcard src/CodeGen/*.cpp)

# ==================== ARCHIVOS OBJETO ====================

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

# CodeGen solo si LLVM está disponible
ifeq ($(ENABLE_LLVM),1)
    CODEGEN_OBJS = $(CODEGEN_SOURCES:.cpp=.o)
else
    CODEGEN_OBJS = 
endif

ALL_OBJS = $(PARSER_OBJ) $(LEXER_OBJ) $(MAIN_OBJ) $(RUNTIME_OBJ) \
           $(AST_OBJS) $(EVALUATOR_OBJS) $(PRINTVISITOR_OBJS) \
           $(VALUE_OBJS) $(SCOPE_OBJS) $(SEMANTIC_OBJS) $(CODEGEN_OBJS)

# ==================== DIRECTORIOS Y EJECUTABLE ====================

BIN_DIR = hulk
EXECUTABLE = $(BIN_DIR)/hulk_compiler$(EXE_EXT)
SCRIPT_FILE = script.hulk

# ==================== COLORES PARA SALIDA ====================

RED := \033[31m
GREEN := \033[32m
YELLOW := \033[33m
BLUE := \033[34m
CYAN := \033[36m
MAGENTA := \033[35m
RESET := \033[0m

# ==================== OBJETIVOS PRINCIPALES ====================

# Objetivo por defecto - mostrar ayuda
.PHONY: all help info clean compile execute

all: help

# Mostrar ayuda de uso
help:
	@echo "$(CYAN)🚀 HULK Compiler - Makefile Multiplataforma$(RESET)"
	@echo ""
	@echo "$(YELLOW)📋 Uso según especificación:$(RESET)"
	@echo "  1. Clonar el repositorio"
	@echo "  2. Copiar tu código en $(MAGENTA)script.hulk$(RESET) en la raíz"
	@echo "  3. $(GREEN)make compile$(RESET)  - Compilar y generar artifacts en hulk/"
	@echo "  4. $(GREEN)make execute$(RESET)  - Ejecutar el programa HULK"
	@echo ""
	@echo "$(YELLOW)🔧 Objetivos principales:$(RESET)"
	@echo "  $(BLUE)make compile$(RESET)  - Compilar el proyecto y generar hulk_compiler.exe"
	@echo "  $(BLUE)make execute$(RESET)  - Ejecutar script.hulk (incluye compilación automática)"
	@echo "  $(BLUE)make info$(RESET)     - Mostrar información del sistema y LLVM"
	@echo "  $(BLUE)make clean$(RESET)    - Limpiar archivos generados"
	@echo "  $(BLUE)make help$(RESET)     - Mostrar esta ayuda"
	@echo ""
	@echo "$(YELLOW)⚡ Opciones avanzadas del compilador:$(RESET)"
	@echo "  $(MAGENTA)make execute-llvm$(RESET)   - Ejecutar con generación LLVM IR optimizado"
	@echo "  $(MAGENTA)make execute-debug$(RESET)  - Ejecutar con información detallada de depuración"
	@echo "  $(MAGENTA)make show-ir$(RESET)        - Mostrar solo el código LLVM IR generado"
	@echo ""
	@echo "$(YELLOW)🎛️ Uso con argumentos personalizados:$(RESET)"
	@echo "  $(MAGENTA)make execute ARGS=\"--llvm\"$(RESET)     - Generar código LLVM IR optimizado"
	@echo "  $(MAGENTA)make execute ARGS=\"--debug\"$(RESET)    - Mostrar información de depuración detallada"
	@echo "  $(MAGENTA)make execute ARGS=\"--show-ir\"$(RESET)  - Mostrar solo el código LLVM IR"
	@echo "  $(MAGENTA)make execute ARGS=\"--llvm --debug\"$(RESET) - Combinar múltiples opciones"
	@echo ""
	@echo "$(YELLOW)💡 Ejecución directa (alternativa al makefile):$(RESET)"
	@echo "  $(CYAN)./hulk/hulk_compiler.exe script.hulk$(RESET)         - Modo interpretación normal"
	@echo "  $(CYAN)./hulk/hulk_compiler.exe script.hulk --llvm$(RESET)   - Con generación LLVM IR"
	@echo "  $(CYAN)./hulk/hulk_compiler.exe script.hulk --debug$(RESET)  - Con información de depuración"
	@echo "  $(CYAN)./hulk/hulk_compiler.exe script.hulk --show-ir$(RESET) - Solo mostrar IR generado"
	@echo ""
	@echo "$(YELLOW)📝 Ejemplos de scripts incluidos:$(RESET)"
	@echo "  $(GREEN)cp examples/advanced_demo.hulk script.hulk$(RESET) - Script de demostración completa"
	@echo "  $(GREEN)cp tests/test_final.hulk script.hulk$(RESET)       - Test de herencia múltiple"

# Mostrar información del sistema y configuración
info:
	@echo "$(CYAN)🔍 HULK Compiler - Información del Sistema$(RESET)"
	@echo "$(BLUE)🖥️  Sistema Operativo: $(DETECTED_OS)$(RESET)"
	@echo "$(BLUE)🔧 Compilador C++: $(CXX)$(RESET)"
	@echo "$(BLUE)⚡ LLVM: $(LLVM_STATUS)$(RESET)"
ifeq ($(HAS_LLVM),1)
	@echo "$(GREEN)📂 LLVM_DIR: $(LLVM_DIR)$(RESET)"
	@echo "$(GREEN)🔗 LLVM Libs: $(shell echo '$(LLVM_LDFLAGS)' | head -c 50)...$(RESET)"
else
	@echo "$(YELLOW)⚠️  Compilando sin LLVM - funcionalidad limitada$(RESET)"
endif
	@echo "$(BLUE)🎯 Ejecutable: $(EXECUTABLE)$(RESET)"
	@echo "$(BLUE)📄 Script: $(SCRIPT_FILE)$(RESET)"

# ==================== OBJETIVO COMPILE ====================

# Compilar el proyecto y generar directorio hulk/ con artifacts
compile: $(EXECUTABLE)
	@echo "$(GREEN)✅ Compilación completada$(RESET)"
	@echo "$(BLUE)📦 Artifacts generados en: $(BIN_DIR)/$(RESET)"
	@ls -la $(BIN_DIR)/ 2>/dev/null || dir $(BIN_DIR) 2>nul || echo "Contenido del directorio hulk/ generado"

# ==================== OBJETIVO EXECUTE ====================

# Ejecutar el programa HULK (con dependencia automática en compile)
execute: compile
	@echo "$(CYAN)🚀 Ejecutando HULK Compiler...$(RESET)"
	@if [ -f "$(SCRIPT_FILE)" ]; then \
		if [ -n "$(ARGS)" ]; then \
			echo "$(GREEN)📄 Ejecutando: $(SCRIPT_FILE) $(ARGS)$(RESET)"; \
			./$(EXECUTABLE) $(SCRIPT_FILE) $(ARGS); \
		else \
			echo "$(GREEN)📄 Ejecutando: $(SCRIPT_FILE)$(RESET)"; \
			./$(EXECUTABLE) $(SCRIPT_FILE); \
		fi \
	else \
		echo "$(RED)❌ Error: No se encuentra $(SCRIPT_FILE)$(RESET)"; \
		echo "$(YELLOW)💡 Copie su código HULK en el archivo $(SCRIPT_FILE) en la raíz del proyecto$(RESET)"; \
		exit 1; \
	fi

# ==================== OBJETIVOS AVANZADOS ====================

# Ejecutar con generación LLVM IR optimizado
# Opción --llvm: Genera código intermedio LLVM, muestra IR y ejecuta optimizado
execute-llvm: compile
	@echo "$(CYAN)🚀 Ejecutando HULK Compiler con LLVM IR optimizado...$(RESET)"
	@if [ -f "$(SCRIPT_FILE)" ]; then \
		echo "$(GREEN)📄 Ejecutando: $(SCRIPT_FILE) --llvm$(RESET)"; \
		echo "$(BLUE)💡 Generando código LLVM IR optimizado y ejecutando...$(RESET)"; \
		./$(EXECUTABLE) $(SCRIPT_FILE) --llvm; \
	else \
		echo "$(RED)❌ Error: No se encuentra $(SCRIPT_FILE)$(RESET)"; \
		exit 1; \
	fi

# Ejecutar con información detallada de depuración  
# Opción --debug: Muestra análisis sintáctico, semántico, resolución de tipos y herencia
execute-debug: compile
	@echo "$(CYAN)🚀 Ejecutando HULK Compiler con información de depuración...$(RESET)"
	@if [ -f "$(SCRIPT_FILE)" ]; then \
		echo "$(GREEN)📄 Ejecutando: $(SCRIPT_FILE) --debug$(RESET)"; \
		echo "$(BLUE)🐛 Mostrando información detallada de compilación...$(RESET)"; \
		./$(EXECUTABLE) $(SCRIPT_FILE) --debug; \
	else \
		echo "$(RED)❌ Error: No se encuentra $(SCRIPT_FILE)$(RESET)"; \
		exit 1; \
	fi

# Mostrar solo el código LLVM IR generado (sin ejecutar)
# Opción --show-ir: Solo genera y muestra el código intermedio LLVM IR
show-ir: compile
	@echo "$(CYAN)🚀 Generando código LLVM IR para análisis...$(RESET)"
	@if [ -f "$(SCRIPT_FILE)" ]; then \
		echo "$(GREEN)📄 Generando IR para: $(SCRIPT_FILE)$(RESET)"; \
		echo "$(BLUE)📊 Solo mostrar código LLVM IR (sin ejecutar)...$(RESET)"; \
		./$(EXECUTABLE) $(SCRIPT_FILE) --show-ir; \
	else \
		echo "$(RED)❌ Error: No se encuentra $(SCRIPT_FILE)$(RESET)"; \
		exit 1; \
	fi

# ==================== CONSTRUCCIÓN DEL EJECUTABLE ====================

$(EXECUTABLE): $(ALL_OBJS) | $(BIN_DIR)
	@echo "$(BLUE)🔗 Enlazando ejecutable...$(RESET)"
	$(CXX) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(BIN_DIR):
	@echo "$(BLUE)📁 Creando directorio $(BIN_DIR)/$(RESET)"
	@mkdir -p $(BIN_DIR) 2>/dev/null || mkdir $(BIN_DIR) 2>nul || true

# ==================== GENERACIÓN DE PARSER Y LEXER ====================

$(PARSER_GEN_CPP) $(PARSER_GEN_HPP): $(PARSER_SRC)
	@echo "$(BLUE)🔧 Generando parser con Bison...$(RESET)"
	$(BISON) -d -o $(PARSER_GEN_CPP) $(PARSER_SRC)

$(LEXER_GEN): $(LEXER_SRC) $(PARSER_GEN_HPP)
	@echo "$(BLUE)🔧 Generando lexer con Flex...$(RESET)"
	$(FLEX) -o $(LEXER_GEN) $(LEXER_SRC)

# ==================== REGLAS DE COMPILACIÓN ====================

# Compilar archivos C++
%.o: %.cpp
	@echo "$(BLUE)🔨 Compilando $<...$(RESET)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compilar archivos C
%.o: %.c
	@echo "$(BLUE)🔨 Compilando $<...$(RESET)"
	$(CC) $(CFLAGS) -c $< -o $@

# ==================== LIMPIEZA ====================

clean:
	@echo "$(YELLOW)🧹 Limpiando archivos generados...$(RESET)"
	@rm -f $(LEXER_GEN) $(PARSER_GEN_CPP) $(PARSER_GEN_HPP) 2>/dev/null || del /Q $(LEXER_GEN) $(PARSER_GEN_CPP) $(PARSER_GEN_HPP) 2>nul || true
	@rm -f $(ALL_OBJS) 2>/dev/null || del /Q $(ALL_OBJS) 2>nul || true
	@rm -f $(EXECUTABLE) 2>/dev/null || del /Q $(EXECUTABLE) 2>nul || true
	@rm -rf $(BIN_DIR) 2>/dev/null || rmdir /S /Q $(BIN_DIR) 2>nul || true
	@echo "$(GREEN)✅ Limpieza completada$(RESET)"

# ==================== DEPENDENCIAS ====================

# Dependencias del parser
$(PARSER_OBJ): $(PARSER_GEN_CPP) $(PARSER_GEN_HPP)

# Dependencias del lexer
$(LEXER_OBJ): $(LEXER_GEN) $(PARSER_GEN_HPP)

# Dependencias principales
$(MAIN_OBJ): $(MAIN_SRC) $(PARSER_GEN_HPP)

# El archivo objeto del runtime
$(RUNTIME_OBJ): $(RUNTIME_SRC)

# Marcar objetivos que no son archivos
.PHONY: all help info clean compile execute execute-llvm execute-debug show-ir
