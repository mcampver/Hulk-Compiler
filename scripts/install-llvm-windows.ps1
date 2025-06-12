# HULK Compiler Enhanced - LLVM Installation Script for Windows
# Run this script in PowerShell as Administrator

param(
    [string]$Method = "binary",  # Options: binary, chocolatey, vcpkg
    [string]$Version = "17.0.6"
)

Write-Host "=== HULK Compiler Enhanced - LLVM Installation ===" -ForegroundColor Green
Write-Host "Installing LLVM using method: $Method" -ForegroundColor Yellow

function Test-Administrator {
    $user = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($user)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Install-LLVM-Binary {
    Write-Host "Installing LLVM from pre-compiled binaries..." -ForegroundColor Cyan
      $downloadUrl = "https://releases.llvm.org/$Version/LLVM-$Version-win64.exe"
    # URL alternativa si la anterior no funciona
    # $downloadUrl = "https://sourceforge.net/projects/llvm-windows/files/LLVM-$Version-win64.exe"
    $installerPath = "$env:TEMP\LLVM-$Version-win64.exe"
    
    Write-Host "Downloading LLVM installer..." -ForegroundColor Yellow
    try {
        Invoke-WebRequest -Uri $downloadUrl -OutFile $installerPath -UseBasicParsing
        Write-Host "Download completed!" -ForegroundColor Green
        
        Write-Host "Running installer (please follow the GUI)..." -ForegroundColor Yellow
        Write-Host "IMPORTANT: Make sure to check 'Add LLVM to the system PATH'" -ForegroundColor Red
        
        Start-Process -FilePath $installerPath -Wait
        
        # Clean up
        Remove-Item $installerPath -Force
        
        Write-Host "Installation completed!" -ForegroundColor Green
    }
    catch {
        Write-Host "Error downloading LLVM: $_" -ForegroundColor Red
        return $false
    }
    
    return $true
}

function Install-LLVM-Chocolatey {
    Write-Host "Installing LLVM using Chocolatey..." -ForegroundColor Cyan
    
    # Check if Chocolatey is installed
    if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
        Write-Host "Chocolatey not found. Installing Chocolatey first..." -ForegroundColor Yellow
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    }
    
    Write-Host "Installing LLVM via Chocolatey..." -ForegroundColor Yellow
    try {
        choco install llvm -y
        Write-Host "LLVM installed successfully!" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "Error installing LLVM via Chocolatey: $_" -ForegroundColor Red
        return $false
    }
}

function Install-LLVM-Vcpkg {
    Write-Host "Installing LLVM using vcpkg..." -ForegroundColor Cyan
    
    $vcpkgPath = "C:\vcpkg"
      if (-not (Test-Path $vcpkgPath)) {
        Write-Host "Cloning vcpkg..." -ForegroundColor Yellow
        git clone https://learn.microsoft.com/en-us/vcpkg/get-started vcpkg.git $vcpkgPath
        
        Write-Host "Bootstrapping vcpkg..." -ForegroundColor Yellow
        & "$vcpkgPath\bootstrap-vcpkg.bat"
    }
    
    Write-Host "Installing LLVM via vcpkg (this may take a while)..." -ForegroundColor Yellow
    try {
        & "$vcpkgPath\vcpkg.exe" install llvm:x64-windows
        & "$vcpkgPath\vcpkg.exe" integrate install
        
        Write-Host "LLVM installed successfully!" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "Error installing LLVM via vcpkg: $_" -ForegroundColor Red
        return $false
    }
}

function Test-LLVM-Installation {
    Write-Host "Testing LLVM installation..." -ForegroundColor Cyan
    
    # Refresh environment variables
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    
    try {
        $llvmVersion = & llvm-config --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "LLVM successfully installed! Version: $llvmVersion" -ForegroundColor Green
            
            $cxxflags = & llvm-config --cxxflags 2>&1
            $ldflags = & llvm-config --ldflags 2>&1
            $libs = & llvm-config --libs core 2>&1
            
            Write-Host "LLVM Configuration:" -ForegroundColor Yellow
            Write-Host "  CXX Flags: $cxxflags" -ForegroundColor White
            Write-Host "  LD Flags: $ldflags" -ForegroundColor White
            Write-Host "  Libraries: $libs" -ForegroundColor White
            
            return $true
        }
        else {
            Write-Host "LLVM installation may have issues. Please restart PowerShell and try again." -ForegroundColor Yellow
            return $false
        }
    }
    catch {
        Write-Host "LLVM not found in PATH. Please restart PowerShell and try again." -ForegroundColor Yellow
        return $false
    }
}

function Update-HULK-Project {
    Write-Host "Updating HULK project configuration..." -ForegroundColor Cyan
    
    $projectPath = Split-Path -Parent $PSScriptRoot
    $makefilePath = Join-Path $projectPath "Makefile"
    
    if (Test-Path $makefilePath) {
        # Backup original Makefile
        Copy-Item $makefilePath "$makefilePath.backup"
        
        # Use Windows-specific Makefile
        $windowsMakefile = Join-Path $projectPath "Makefile.windows"
        if (Test-Path $windowsMakefile) {
            Copy-Item $windowsMakefile $makefilePath -Force
            Write-Host "Updated Makefile for Windows LLVM support" -ForegroundColor Green
        }
    }
}

# Main installation logic
if (-not (Test-Administrator)) {
    Write-Host "Please run this script as Administrator!" -ForegroundColor Red
    exit 1
}

$success = $false

switch ($Method.ToLower()) {
    "binary" { $success = Install-LLVM-Binary }
    "chocolatey" { $success = Install-LLVM-Chocolatey }
    "vcpkg" { $success = Install-LLVM-Vcpkg }
    default {
        Write-Host "Unknown installation method: $Method" -ForegroundColor Red
        Write-Host "Available methods: binary, chocolatey, vcpkg" -ForegroundColor Yellow
        exit 1
    }
}

if ($success) {
    Write-Host "Waiting for system to update PATH..." -ForegroundColor Yellow
    Start-Sleep -Seconds 5
    
    if (Test-LLVM-Installation) {
        Update-HULK-Project
        
        Write-Host "`n=== Installation Complete! ===" -ForegroundColor Green
        Write-Host "You can now build HULK Compiler Enhanced with LLVM support:" -ForegroundColor White
        Write-Host "  1. Restart PowerShell" -ForegroundColor Yellow
        Write-Host "  2. Navigate to your project directory" -ForegroundColor Yellow
        Write-Host "  3. Run: make compile" -ForegroundColor Yellow
        Write-Host "  4. Test LLVM: make test-llvm" -ForegroundColor Yellow
    }
    else {
        Write-Host "Installation completed but verification failed." -ForegroundColor Yellow
        Write-Host "Please restart PowerShell and test manually with: llvm-config --version" -ForegroundColor Yellow
    }
}
else {
    Write-Host "Installation failed. Please try a different method or install manually." -ForegroundColor Red
}

Write-Host "`nPress any key to exit..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
