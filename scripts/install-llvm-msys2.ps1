# Instalación de LLVM usando MSYS2 para HULK Compiler
# Este script automatiza la instalación completa de LLVM en MSYS2

Write-Host "=== HULK Compiler - Instalación LLVM con MSYS2 ===" -ForegroundColor Cyan

# Función para verificar si MSYS2 está instalado
function Test-MSYS2 {
    $msys2Paths = @(
        "C:\msys64",
        "C:\msys2",
        "$env:USERPROFILE\msys64",
        "$env:USERPROFILE\msys2"
    )
    
    foreach ($path in $msys2Paths) {
        if (Test-Path "$path\usr\bin\bash.exe") {
            return $path
        }
    }
    return $null
}

# Verificar MSYS2
$msys2Path = Test-MSYS2
if (-not $msys2Path) {
    Write-Host "❌ MSYS2 no encontrado. Descargando e instalando..." -ForegroundColor Yellow
    
    # Descargar MSYS2
    $msys2Url = "https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe"
    $installerPath = "$env:TEMP\msys2-installer.exe"
    
    Write-Host "📥 Descargando MSYS2..." -ForegroundColor Blue
    Invoke-WebRequest -Uri $msys2Url -OutFile $installerPath
    
    Write-Host "🚀 Ejecutando instalador MSYS2..." -ForegroundColor Blue
    Start-Process -FilePath $installerPath -ArgumentList "install", "--confirm-command", "--accept-messages", "--root", "C:\msys64" -Wait
    
    $msys2Path = "C:\msys64"
}

Write-Host "✅ MSYS2 encontrado en: $msys2Path" -ForegroundColor Green

# Función para ejecutar comandos en MSYS2
function Invoke-MSYS2 {
    param([string]$Command)
    
    $bashPath = "$msys2Path\usr\bin\bash.exe"
    & $bashPath -lc $Command
}

Write-Host "🔄 Actualizando base de datos de paquetes MSYS2..." -ForegroundColor Blue
Invoke-MSYS2 "pacman -Syu --noconfirm"

Write-Host "📦 Instalando herramientas de desarrollo..." -ForegroundColor Blue
Invoke-MSYS2 "pacman -S --noconfirm mingw-w64-x86_64-toolchain"

Write-Host "🔧 Instalando LLVM y herramientas asociadas..." -ForegroundColor Blue
$llvmPackages = @(
    "mingw-w64-x86_64-llvm",
    "mingw-w64-x86_64-clang", 
    "mingw-w64-x86_64-clang-tools-extra",
    "mingw-w64-x86_64-lld",
    "mingw-w64-x86_64-lldb",
    "mingw-w64-x86_64-cmake",
    "mingw-w64-x86_64-ninja"
)

foreach ($package in $llvmPackages) {
    Write-Host "  📦 Instalando $package..." -ForegroundColor Cyan
    Invoke-MSYS2 "pacman -S --noconfirm $package"
}

Write-Host "🔍 Verificando instalación..." -ForegroundColor Blue

# Verificar llvm-config
$llvmConfigCheck = Invoke-MSYS2 "which llvm-config && llvm-config --version"
if ($llvmConfigCheck) {
    Write-Host "✅ llvm-config instalado correctamente" -ForegroundColor Green
    Write-Host "📍 Versión: $llvmConfigCheck" -ForegroundColor White
} else {
    Write-Host "❌ Error: llvm-config no encontrado" -ForegroundColor Red
}

# Verificar clang
$clangCheck = Invoke-MSYS2 "which clang && clang --version | head -1"
if ($clangCheck) {
    Write-Host "✅ Clang instalado correctamente" -ForegroundColor Green
} else {
    Write-Host "❌ Error: Clang no encontrado" -ForegroundColor Red
}

Write-Host "`n=== Configuración del entorno ===" -ForegroundColor Cyan

# Crear script de configuración del entorno
$envScript = @"
# HULK Compiler - Configuración del entorno MSYS2
# Agregar al PATH de Windows para usar desde cualquier terminal

# Rutas de MSYS2 para LLVM
export LLVM_DIR="$msys2Path/mingw64"
export PATH="$msys2Path/mingw64/bin:$msys2Path/usr/bin:`$PATH"

# Variables para el compilador
export CC=clang
export CXX=clang++
export LLVM_CONFIG=llvm-config

# Verificar instalación
echo "🔍 Verificando configuración LLVM..."
echo "LLVM_DIR: `$LLVM_DIR"
echo "llvm-config: `$(which llvm-config)"
echo "Versión LLVM: `$(llvm-config --version)"
"@

$envScriptPath = "$PWD\scripts\msys2-env.sh"
$envScript | Out-File -FilePath $envScriptPath -Encoding UTF8
Write-Host "📝 Script de entorno creado: $envScriptPath" -ForegroundColor Green

Write-Host "`n=== Próximos pasos ===" -ForegroundColor Yellow
Write-Host "1. Para usar LLVM desde MSYS2 terminal:" -ForegroundColor White
Write-Host "   - Abre MSYS2 MinGW 64-bit" -ForegroundColor Gray
Write-Host "   - Navega al proyecto: cd /d/Escuela/compilacion/new\ comments/Hulk-Compiler" -ForegroundColor Gray
Write-Host "   - Ejecuta: source scripts/msys2-env.sh" -ForegroundColor Gray

Write-Host "2. Para compilar el proyecto:" -ForegroundColor White
Write-Host "   - make clean && make" -ForegroundColor Gray

Write-Host "3. Para usar desde PowerShell (opcional):" -ForegroundColor White
Write-Host "   - Agregar al PATH: $msys2Path\mingw64\bin" -ForegroundColor Gray

Write-Host "`n✅ Instalación completada. LLVM está disponible en MSYS2!" -ForegroundColor Green
