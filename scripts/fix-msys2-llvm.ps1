# Script para solucionar problemas de lock en MSYS2 y reinstalar LLVM
# Este script limpia locks y realiza la instalación paso a paso

Write-Host "=== HULK Compiler - Solucionando problemas de MSYS2 ===" -ForegroundColor Cyan

# Función para ejecutar comandos en MSYS2
function Invoke-MSYS2 {
    param([string]$Command, [string]$Description = "")
    
    if ($Description) {
        Write-Host "🔧 $Description" -ForegroundColor Blue
    }
    
    $bashPath = "C:\msys64\usr\bin\bash.exe"
    & $bashPath -lc $Command
}

Write-Host "🔍 Verificando procesos de pacman..." -ForegroundColor Yellow

# Terminar procesos de pacman que puedan estar colgados
Invoke-MSYS2 "pkill -f pacman || echo 'No hay procesos pacman ejecutándose'"

Write-Host "🧹 Limpiando locks de pacman..." -ForegroundColor Yellow

# Limpiar locks de pacman
Invoke-MSYS2 "sudo rm -f /var/lib/pacman/db.lck" "Eliminando lock de base de datos"
Invoke-MSYS2 "sudo rm -f /var/cache/pacman/pkg/*.part" "Limpiando descargas parciales"

Write-Host "🔄 Actualizando MSYS2..." -ForegroundColor Blue

# Actualizar MSYS2 en pasos separados
Invoke-MSYS2 "pacman -Sy --noconfirm" "Sincronizando base de datos"

Write-Host "📦 Instalando herramientas básicas..." -ForegroundColor Blue

# Instalar herramientas básicas primero
Invoke-MSYS2 "pacman -S --noconfirm --needed base-devel" "Instalando herramientas de desarrollo base"

Write-Host "🔧 Instalando toolchain MinGW..." -ForegroundColor Blue

# Instalar toolchain
Invoke-MSYS2 "pacman -S --noconfirm mingw-w64-x86_64-toolchain" "Instalando compiladores GCC"

Write-Host "⚡ Instalando LLVM (paso 1/4)..." -ForegroundColor Green

# Instalar LLVM en pasos separados para evitar problemas
Invoke-MSYS2 "pacman -S --noconfirm mingw-w64-x86_64-llvm" "Instalando LLVM core"

Write-Host "⚡ Instalando Clang (paso 2/4)..." -ForegroundColor Green

Invoke-MSYS2 "pacman -S --noconfirm mingw-w64-x86_64-clang" "Instalando Clang"

Write-Host "⚡ Instalando herramientas adicionales (paso 3/4)..." -ForegroundColor Green

Invoke-MSYS2 "pacman -S --noconfirm mingw-w64-x86_64-clang-tools-extra" "Instalando herramientas extra de Clang"

Write-Host "⚡ Instalando linkers y debuggers (paso 4/4)..." -ForegroundColor Green

Invoke-MSYS2 "pacman -S --noconfirm mingw-w64-x86_64-lld mingw-w64-x86_64-lldb" "Instalando LLD y LLDB"

Write-Host "🛠️ Instalando herramientas de build..." -ForegroundColor Blue

Invoke-MSYS2 "pacman -S --noconfirm mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja" "Instalando CMake y Ninja"

Write-Host "🔍 Verificando instalación..." -ForegroundColor Cyan

# Verificar desde el terminal MinGW64 correcto
$verification = Invoke-MSYS2 "export PATH='/mingw64/bin:$PATH' && which llvm-config && llvm-config --version"

if ($verification) {
    Write-Host "✅ LLVM instalado correctamente!" -ForegroundColor Green
    Write-Host "📍 Versión: $verification" -ForegroundColor White
} else {
    Write-Host "⚠️ Verificando instalación manualmente..." -ForegroundColor Yellow
    
    # Verificar archivos directamente
    if (Test-Path "C:\msys64\mingw64\bin\llvm-config.exe") {
        Write-Host "✅ llvm-config.exe encontrado en C:\msys64\mingw64\bin\" -ForegroundColor Green
    } else {
        Write-Host "❌ llvm-config.exe no encontrado" -ForegroundColor Red
    }
    
    if (Test-Path "C:\msys64\mingw64\bin\clang.exe") {
        Write-Host "✅ clang.exe encontrado en C:\msys64\mingw64\bin\" -ForegroundColor Green
    } else {
        Write-Host "❌ clang.exe no encontrado" -ForegroundColor Red
    }
}

Write-Host "`n=== Configuración del entorno ===" -ForegroundColor Cyan

# Crear script de configuración actualizado
$envScript = @"
#!/bin/bash
# HULK Compiler - Configuración del entorno MSYS2 (Actualizado)

echo "🔧 Configurando entorno LLVM en MSYS2..."

# Configurar PATH para MinGW64
export PATH="/mingw64/bin:/mingw64/lib:/usr/bin:`$PATH"

# Variables para LLVM
export LLVM_DIR="/mingw64"
export LLVM_CONFIG="llvm-config"

# Variables para compiladores
export CC=clang
export CXX=clang++

# Verificar configuración
echo "🔍 Verificando configuración..."
echo "PATH: `$PATH"
echo "LLVM_DIR: `$LLVM_DIR"

# Verificar herramientas
if command -v llvm-config >/dev/null 2>&1; then
    echo "✅ llvm-config: `$(which llvm-config)"
    echo "📍 Versión LLVM: `$(llvm-config --version)"
else
    echo "❌ llvm-config no encontrado en PATH"
fi

if command -v clang >/dev/null 2>&1; then
    echo "✅ clang: `$(which clang)"
    echo "📍 Versión Clang: `$(clang --version | head -1)"
else
    echo "❌ clang no encontrado en PATH"
fi

if command -v clang++ >/dev/null 2>&1; then
    echo "✅ clang++: `$(which clang++)"
else
    echo "❌ clang++ no encontrado en PATH"
fi

echo "🎯 Entorno configurado. Puedes compilar con: make clean && make"
"@

$envScriptPath = "$PWD\scripts\msys2-env-fixed.sh"
$envScript | Out-File -FilePath $envScriptPath -Encoding UTF8

Write-Host "📝 Script de entorno actualizado: $envScriptPath" -ForegroundColor Green

Write-Host "`n=== Instrucciones de uso ===" -ForegroundColor Yellow
Write-Host "1. Abre 'MSYS2 MinGW 64-bit' (IMPORTANTE: no MSYS2 MSYS)" -ForegroundColor White
Write-Host "2. Navega al proyecto:" -ForegroundColor White
Write-Host "   cd /d/Escuela/compilacion/new\ comments/Hulk-Compiler" -ForegroundColor Gray
Write-Host "3. Configura el entorno:" -ForegroundColor White
Write-Host "   source scripts/msys2-env-fixed.sh" -ForegroundColor Gray
Write-Host "4. Compila el proyecto:" -ForegroundColor White
Write-Host "   make clean && make" -ForegroundColor Gray

Write-Host "`n✅ Proceso completado. Sigue las instrucciones para continuar." -ForegroundColor Green
