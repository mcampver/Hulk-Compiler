# HULK Compiler - Simple LLVM Installation Script for Windows
param(
    [string]$Method = "winget"
)

Write-Host "=== HULK Compiler - LLVM Installation ===" -ForegroundColor Green
Write-Host "Installation method: $Method" -ForegroundColor Yellow

function Test-Administrator {
    $user = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($user)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Install-LLVM-Winget {
    Write-Host "Installing LLVM using Winget..." -ForegroundColor Cyan
    
    try {
        winget install LLVM.LLVM
        
        # Add to PATH if not already there
        $llvmPath = "C:\Program Files\LLVM\bin"
        $currentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
        if ($currentPath -notlike "*$llvmPath*") {
            Write-Host "Adding LLVM to system PATH..." -ForegroundColor Yellow
            [Environment]::SetEnvironmentVariable("Path", "$currentPath;$llvmPath", "Machine")
        }
        
        Write-Host "LLVM installed successfully!" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "Error installing LLVM via Winget: $_" -ForegroundColor Red
        return $false
    }
}

function Install-LLVM-Chocolatey {
    Write-Host "Installing LLVM using Chocolatey..." -ForegroundColor Cyan
    
    if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
        Write-Host "Installing Chocolatey first..." -ForegroundColor Yellow
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    }
    
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

function Show-ManualInstructions {
    Write-Host "Manual LLVM installation instructions:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "1. Go to: https://releases.llvm.org/" -ForegroundColor Yellow
    Write-Host "2. Download the latest Windows release (LLVM-X.X.X-win64.exe)" -ForegroundColor Yellow
    Write-Host "3. Run the installer as Administrator" -ForegroundColor Yellow
    Write-Host "4. During installation, CHECK 'Add LLVM to the system PATH'" -ForegroundColor Red
    Write-Host "5. Restart your terminal/PowerShell after installation" -ForegroundColor Yellow
    Write-Host ""
    return $true
}

function Test-LLVM-Installation {
    Write-Host "Testing LLVM installation..." -ForegroundColor Cyan
    
    # Refresh environment variables
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    
    $foundTools = @()
    
    try {
        $version = llvm-config --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            $foundTools += "llvm-config"
            Write-Host "✓ llvm-config found: $version" -ForegroundColor Green
        }
    }
    catch {
        Write-Host "✗ llvm-config not found" -ForegroundColor Yellow
    }
    
    try {
        $version = clang --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            $foundTools += "clang"
            Write-Host "✓ clang found" -ForegroundColor Green
        }
    }
    catch {
        Write-Host "✗ clang not found" -ForegroundColor Yellow
    }
    
    if ($foundTools.Count -gt 0) {
        Write-Host ""
        Write-Host "LLVM tools detected: $($foundTools -join ', ')" -ForegroundColor Green
        return $true
    }
    else {
        Write-Host "No LLVM tools found." -ForegroundColor Red
        return $false
    }
}

# Main logic
$success = $false

Write-Host "Detected OS: Windows" -ForegroundColor White
Write-Host ""

switch ($Method.ToLower()) {
    "winget" { 
        if (Test-Administrator) {
            $success = Install-LLVM-Winget 
        } else {
            Write-Host "Winget installation requires Administrator privileges." -ForegroundColor Red
            Write-Host "Please run PowerShell as Administrator." -ForegroundColor Yellow
        }
    }
    "chocolatey" { 
        if (Test-Administrator) {
            $success = Install-LLVM-Chocolatey 
        } else {
            Write-Host "Chocolatey installation requires Administrator privileges." -ForegroundColor Red
            Write-Host "Please run PowerShell as Administrator." -ForegroundColor Yellow
        }
    }
    "manual" { 
        $success = Show-ManualInstructions 
    }
    default {
        Write-Host "Unknown installation method: $Method" -ForegroundColor Red
        Write-Host "Available methods: winget, chocolatey, manual" -ForegroundColor Yellow
        exit 1
    }
}

if ($success -and $Method -ne "manual") {
    Write-Host ""
    Write-Host "Waiting for system to update environment..." -ForegroundColor Yellow
    Start-Sleep -Seconds 3
    
    if (Test-LLVM-Installation) {
        Write-Host ""
        Write-Host "=== Installation Complete! ===" -ForegroundColor Green
        Write-Host "You can now:" -ForegroundColor White
        Write-Host "  1. Restart PowerShell" -ForegroundColor Yellow
        Write-Host "  2. Run: make check-llvm" -ForegroundColor Yellow
        Write-Host "  3. Run: make compile" -ForegroundColor Yellow
    }
    else {
        Write-Host ""
        Write-Host "Installation may have completed, but LLVM tools are not detected." -ForegroundColor Yellow
        Write-Host "Please restart PowerShell and test manually." -ForegroundColor Yellow
    }
}
elseif ($Method -eq "manual") {
    Write-Host ""
    Write-Host "After manual installation, please:" -ForegroundColor White
    Write-Host "  1. Restart PowerShell" -ForegroundColor Yellow
    Write-Host "  2. Test with: llvm-config --version" -ForegroundColor Yellow
    Write-Host "  3. Run: make check-llvm" -ForegroundColor Yellow
}
else {
    Write-Host ""
    Write-Host "Installation failed or was cancelled." -ForegroundColor Red
    Write-Host "You can still use the compiler without LLVM." -ForegroundColor Yellow
}
