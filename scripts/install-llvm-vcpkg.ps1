# Script para instalar LLVM usando vcpkg (más confiable para desarrollo)
# Ejecutar en el directorio del proyecto

Write-Host "=== Instalador LLVM con vcpkg ===" -ForegroundColor Green

# Verificar si vcpkg está instalado
if (-not (Get-Command "vcpkg" -ErrorAction SilentlyContinue)) {
    Write-Host "vcpkg no está instalado. Instalando..." -ForegroundColor Yellow
      # Clonar vcpkg
    if (-not (Test-Path "vcpkg")) {
        # Usando la URL de Microsoft Learn en lugar de GitHub
        git clone https://learn.microsoft.com/en-us/vcpkg/get-started vcpkg
    }
    
    # Bootstrapear vcpkg
    Set-Location "vcpkg"
    .\bootstrap-vcpkg.bat
    
    # Integrar con Visual Studio
    .\vcpkg integrate install
    
    # Volver al directorio original
    Set-Location ".."
    
    # Agregar vcpkg al PATH temporalmente
    $env:PATH += ";$(Get-Location)\vcpkg"
}

Write-Host "Instalando LLVM con vcpkg..." -ForegroundColor Yellow
Write-Host "Esto puede tomar 30-60 minutos..." -ForegroundColor Yellow

# Instalar LLVM con todas las características necesarias
vcpkg install llvm:x64-windows

Write-Host "Verificando instalación..." -ForegroundColor Yellow

# Verificar que se instaló correctamente
$vcpkgRoot = "vcpkg\installed\x64-windows"
if (Test-Path "$vcpkgRoot\include\llvm\IR\Module.h") {
    Write-Host "✅ LLVM instalado correctamente con vcpkg" -ForegroundColor Green
    Write-Host "Headers ubicados en: $vcpkgRoot\include" -ForegroundColor Green
    Write-Host "Librerías ubicadas en: $vcpkgRoot\lib" -ForegroundColor Green
} else {
    Write-Host "❌ Error en la instalación con vcpkg" -ForegroundColor Red
}

Write-Host "=== Configuración completada ===" -ForegroundColor Green
