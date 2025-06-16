#!/bin/bash
# HULK Compiler - Configuración del entorno MSYS2 (Actualizado)

echo "🔧 Configurando entorno LLVM en MSYS2..."

# Configurar PATH para MinGW64
export PATH="/mingw64/bin:/mingw64/lib:/usr/bin:$PATH"

# Variables para LLVM
export LLVM_DIR="/mingw64"
export LLVM_CONFIG="llvm-config"

# Variables para compiladores
export CC=clang
export CXX=clang++

# Verificar configuración
echo "🔍 Verificando configuración..."
echo "PATH: $PATH"
echo "LLVM_DIR: $LLVM_DIR"

# Verificar herramientas
if command -v llvm-config >/dev/null 2>&1; then
    echo "✅ llvm-config: $(which llvm-config)"
    echo "📍 Versión LLVM: $(llvm-config --version)"
else
    echo "❌ llvm-config no encontrado en PATH"
fi

if command -v clang >/dev/null 2>&1; then
    echo "✅ clang: $(which clang)"
    echo "📍 Versión Clang: $(clang --version | head -1)"
else
    echo "❌ clang no encontrado en PATH"
fi

if command -v clang++ >/dev/null 2>&1; then
    echo "✅ clang++: $(which clang++)"
else
    echo "❌ clang++ no encontrado en PATH"
fi

echo "🎯 Entorno configurado. Puedes compilar con: make clean && make"
