# Script para eliminar referencias a GitHub
# Este script limpia cualquier referencia a GitHub del proyecto

param(
    [switch]$DryRun,
    [switch]$Verbose
)

# Directorio del proyecto
$ProjectDir = Split-Path -Parent $PSScriptRoot

# Mostrar mensaje de inicio
Write-Host "=== Limpieza de referencias a GitHub ===" -ForegroundColor Green
if ($DryRun) {
    Write-Host "Modo de prueba: no se realizarán cambios reales" -ForegroundColor Yellow
}

# Función para mostrar mensajes verbosos
function Write-Verbose-Msg {
    param([string]$Message)
    
    if ($Verbose) {
        Write-Host "[VERBOSE] $Message" -ForegroundColor Gray
    }
}

# 1. Verificar si existe un directorio .git y eliminarlo
$GitDir = Join-Path -Path $ProjectDir -ChildPath ".git"
if (Test-Path -Path $GitDir) {
    Write-Host "Encontrado directorio .git" -ForegroundColor Yellow
    
    if (-not $DryRun) {
        Write-Host "Eliminando directorio .git..." -ForegroundColor Cyan
        Remove-Item -Recurse -Force $GitDir
        Write-Host "✅ Directorio .git eliminado" -ForegroundColor Green
    } else {
        Write-Host "[DRY RUN] Se eliminaría: $GitDir" -ForegroundColor Cyan
    }
}

# 2. Buscar y reemplazar referencias a GitHub en archivos
$FileExtensions = @("*.md", "*.html", "*.txt", "*.ps1", "*.sh", "*.cpp", "*.hpp", "*.c", "*.h", "*.yml", "*.yaml")
$SearchStrings = @("github.com", "GitHub", "github", "git clone")

foreach ($Ext in $FileExtensions) {
    $Files = Get-ChildItem -Path $ProjectDir -Filter $Ext -Recurse
    
    foreach ($File in $Files) {
        $Content = Get-Content -Path $File.FullName -Raw
        $Modified = $false
        $NewContent = $Content
        
        foreach ($SearchString in $SearchStrings) {
            if ($Content -match $SearchString) {
                Write-Host "Encontrada referencia a '$SearchString' en: $($File.FullName)" -ForegroundColor Yellow
                $Modified = $true
                
                # Reemplazos específicos
                switch ($SearchString) {
                    "github.com" { 
                        $NewContent = $NewContent -replace "github\.com/[^/\s]+/[^/\s]+", "ejemplo.com/repositorio" 
                        $NewContent = $NewContent -replace "github\.com", "ejemplo.com"
                    }
                    "GitHub" { $NewContent = $NewContent -replace "GitHub", "Control de Versiones" }
                    "github" { $NewContent = $NewContent -replace "github", "repo" }
                    "git clone" { $NewContent = $NewContent -replace "git clone\s+https://github\.com[^\s]+", "# Comando para clonar repositorio removido" }
                }
            }
        }
        
        if ($Modified -and (-not $DryRun)) {
            Write-Verbose-Msg "Actualizando archivo: $($File.FullName)"
            Set-Content -Path $File.FullName -Value $NewContent
            Write-Host "✅ Archivo actualizado: $($File.FullName)" -ForegroundColor Green
        } elseif ($Modified) {
            Write-Host "[DRY RUN] Se actualizaría: $($File.FullName)" -ForegroundColor Cyan
        }
    }
}

# 3. Buscar y eliminar archivos específicos de GitHub
$GitHubFiles = @(
    ".github",
    ".gitattributes",
    ".gitmodules"
)

foreach ($GitHubFile in $GitHubFiles) {
    $FilePath = Join-Path -Path $ProjectDir -ChildPath $GitHubFile
    
    if (Test-Path -Path $FilePath) {
        Write-Host "Encontrado archivo/directorio de GitHub: $FilePath" -ForegroundColor Yellow
        
        if (-not $DryRun) {
            Write-Host "Eliminando $GitHubFile..." -ForegroundColor Cyan
            Remove-Item -Recurse -Force $FilePath
            Write-Host "✅ $GitHubFile eliminado" -ForegroundColor Green
        } else {
            Write-Host "[DRY RUN] Se eliminaría: $FilePath" -ForegroundColor Cyan
        }
    }
}

# 4. Crear o actualizar archivo que indique que el proyecto es independiente
$IndependentFile = Join-Path -Path $ProjectDir -ChildPath "PROYECTO_INDEPENDIENTE.md"
$IndependentContent = @"
# Proyecto Independiente

Este proyecto es completamente independiente y autónomo. No está vinculado a ningún repositorio externo ni a servicios de control de versiones en línea.

Fecha de desvinculación: $(Get-Date -Format "yyyy-MM-dd")

## Notas

- El proyecto puede ser utilizado, modificado y distribuido de acuerdo con los términos de la licencia incluida.
- Para más información sobre el uso y la implementación, consulte la documentación en la carpeta 'docs'.
"@

if (-not $DryRun) {
    Write-Host "Creando archivo PROYECTO_INDEPENDIENTE.md..." -ForegroundColor Cyan
    Set-Content -Path $IndependentFile -Value $IndependentContent
    Write-Host "✅ Archivo PROYECTO_INDEPENDIENTE.md creado/actualizado" -ForegroundColor Green
} else {
    Write-Host "[DRY RUN] Se crearía: $IndependentFile" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "=== Limpieza de referencias a GitHub completada ===" -ForegroundColor Green
if ($DryRun) {
    Write-Host "Ejecute el script sin -DryRun para aplicar los cambios" -ForegroundColor Yellow
} else {
    Write-Host "Todas las referencias a GitHub han sido eliminadas del proyecto" -ForegroundColor Green
}
