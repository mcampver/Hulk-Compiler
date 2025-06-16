# HULK Compiler - Configuración del entorno MSYS2
# Agregar al PATH de Windows para usar desde cualquier terminal

# Rutas de MSYS2 para LLVM
export LLVM_DIR="C:\msys64/mingw64"
export PATH="C:\msys64/mingw64/bin:C:\msys64/usr/bin:$PATH"

# Variables para el compilador
export CC=clang
export CXX=clang++
export LLVM_CONFIG=llvm-config

# Verificar instalación
echo "🔍 Verificando configuración LLVM..."
echo "LLVM_DIR: $LLVM_DIR"
echo "llvm-config: $(which llvm-config)"
echo "Versión LLVM: $(llvm-config --version)"
