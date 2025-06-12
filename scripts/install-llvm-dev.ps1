# Script para instalar LLVM con headers de desarrollo
# Ejecutar como administrador

Write-Host "=== Instalador LLVM Completo para Desarrollo ===" -ForegroundColor Green

# Verificar si somos administrador
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "Este script requiere permisos de administrador." -ForegroundColor Red
    Write-Host "Ejecuta PowerShell como administrador y vuelve a ejecutar este script." -ForegroundColor Yellow
    exit 1
}

# Descargar LLVM completo desde la página oficial
$llvmVersion = "20.1.4"
$downloadUrl = "https://releases.llvm.org/$llvmVersion/LLVM-$llvmVersion-win64.exe"
$installerPath = "$env:TEMP\LLVM-$llvmVersion-win64.exe"

Write-Host "Descargando LLVM $llvmVersion completo..." -ForegroundColor Yellow
try {
    Invoke-WebRequest -Uri $downloadUrl -OutFile $installerPath -UseBasicParsing
    Write-Host "Descarga completada." -ForegroundColor Green
} catch {
    Write-Host "Error descargando LLVM: $_" -ForegroundColor Red
    Write-Host "Intenta descargar manualmente desde: $downloadUrl" -ForegroundColor Yellow
    exit 1
}

# Desinstalar versión anterior si existe
Write-Host "Desinstalando versión anterior de LLVM..." -ForegroundColor Yellow
if (Test-Path "C:\Program Files\LLVM\Uninstall.exe") {
    Start-Process "C:\Program Files\LLVM\Uninstall.exe" -ArgumentList "/S" -Wait
}

# Instalar nueva versión
Write-Host "Instalando LLVM completo..." -ForegroundColor Yellow
Start-Process $installerPath -ArgumentList "/S", "/NCRC" -Wait

# Verificar instalación
Write-Host "Verificando instalación..." -ForegroundColor Yellow
if (Test-Path "C:\Program Files\LLVM\include\llvm\IR\Module.h") {
    Write-Host "✅ LLVM instalado correctamente con headers de desarrollo" -ForegroundColor Green
} else {
    Write-Host "❌ La instalación no incluye headers de desarrollo" -ForegroundColor Red
    Write-Host "Intenta con vcpkg o compilar desde fuentes" -ForegroundColor Yellow
}

# Agregar al PATH si no está
$currentPath = [Environment]::GetEnvironmentVariable("PATH", "Machine")
$llvmPath = "C:\Program Files\LLVM\bin"
if ($currentPath -notlike "*$llvmPath*") {
    Write-Host "Agregando LLVM al PATH del sistema..." -ForegroundColor Yellow
    [Environment]::SetEnvironmentVariable("PATH", "$currentPath;$llvmPath", "Machine")
}

Write-Host "=== Instalación completada ===" -ForegroundColor Green
Write-Host "Reinicia tu terminal para que los cambios surtan efecto." -ForegroundColor Yellow

# Limpiar archivo temporal
Remove-Item $installerPath -ErrorAction SilentlyContinue
