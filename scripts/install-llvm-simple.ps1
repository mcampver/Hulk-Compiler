# HULK Compiler - Simple LLVM Installation Script for Windows
# This script provides multiple installation methods for LLVM

param(
    [string]$Method = "winget"  # Options: winget, chocolatey, manual, precompiled
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
        # Check if winget is available
        $wingetVersion = winget --version 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Winget not available. Please install it from Microsoft Store or use another method." -ForegroundColor Red
            return $false
        }
        
        Write-Host "Installing LLVM via Winget..." -ForegroundColor Yellow
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
    
    # Check if Chocolatey is installed
    if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
        Write-Host "Installing Chocolatey first..." -ForegroundColor Yellow
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    }
    
    try {
        Write-Host "Installing LLVM via Chocolatey..." -ForegroundColor Yellow
        choco install llvm -y
        Write-Host "LLVM installed successfully!" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "Error installing LLVM via Chocolatey: $_" -ForegroundColor Red
        return $false
    }
}

function Install-LLVM-Manual {
    Write-Host "Manual LLVM installation instructions:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "1. Go to: https://releases.llvm.org/" -ForegroundColor Yellow
    Write-Host "2. Download the latest Windows release (LLVM-X.X.X-win64.exe)" -ForegroundColor Yellow
    Write-Host "3. Run the installer as Administrator" -ForegroundColor Yellow
    Write-Host "4. During installation, CHECK 'Add LLVM to the system PATH'" -ForegroundColor Red
    Write-Host "5. Restart your terminal/PowerShell after installation" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Alternative: Download pre-compiled version (smaller, dev tools only)" -ForegroundColor White
    Write-Host "1. Go to: https://github.com/llvm/llvm-project/releases" -ForegroundColor Yellow
    Write-Host "2. Download: llvm-X.X.X-windows-x64.tar.xz" -ForegroundColor Yellow
    Write-Host "3. Extract to C:\LLVM" -ForegroundColor Yellow
    Write-Host "4. Add C:\LLVM\bin to your PATH manually" -ForegroundColor Yellow
    Write-Host ""
    return $true
}

function Install-LLVM-Precompiled {
    Write-Host "Installing pre-compiled LLVM (development tools only)..." -ForegroundColor Cyan
    
    $llvmVersion = "17.0.6"
    $downloadUrl = "https://github.com/llvm/llvm-project/releases/download/llvmorg-$llvmVersion/LLVM-$llvmVersion-win64.exe"
    $installerPath = "$env:TEMP\LLVM-$llvmVersion-win64.exe"
    
    try {
        Write-Host "Downloading LLVM $llvmVersion..." -ForegroundColor Yellow
        Invoke-WebRequest -Uri $downloadUrl -OutFile $installerPath -UseBasicParsing
        
        Write-Host "Running installer..." -ForegroundColor Yellow
        Write-Host "IMPORTANT: Please check 'Add LLVM to the system PATH' during installation!" -ForegroundColor Red
        
        Start-Process -FilePath $installerPath -Wait
        
        # Cleanup
        Remove-Item $installerPath -Force
        
        Write-Host "Installation completed!" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "Error downloading LLVM: $_" -ForegroundColor Red
        return $false
    }
}

function Test-LLVM-Installation {
    Write-Host "Testing LLVM installation..." -ForegroundColor Cyan
    
    # Refresh environment variables
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    
    try {
        # Test for various LLVM tools
        $tools = @("llvm-config", "clang", "clang++")
        $foundTools = @()
        
        foreach ($tool in $tools) {
            try {
                $version = & $tool --version 2>&1
                if ($LASTEXITCODE -eq 0) {
                    $foundTools += $tool
                    Write-Host "✓ $tool found: $($version.Split([Environment]::NewLine)[0])" -ForegroundColor Green
                }
            }
            catch {
                Write-Host "✗ $tool not found" -ForegroundColor Yellow
            }
        }
        
        if ($foundTools.Count -gt 0) {
            Write-Host ""
            Write-Host "LLVM tools detected: $($foundTools -join ', ')" -ForegroundColor Green
            
            # Test llvm-config specifically
            if ($foundTools -contains "llvm-config") {
                try {
                    $cxxflags = & llvm-config --cxxflags 2>&1
                    $ldflags = & llvm-config --ldflags 2>&1
                    $libs = & llvm-config --libs core 2>&1
                    
                    Write-Host ""
                    Write-Host "LLVM Configuration:" -ForegroundColor Yellow
                    Write-Host "  CXX Flags: $cxxflags" -ForegroundColor White
                    Write-Host "  LD Flags: $ldflags" -ForegroundColor White
                    Write-Host "  Libraries: $libs" -ForegroundColor White
                }
                catch {
                    Write-Host "llvm-config found but not working properly" -ForegroundColor Yellow
                }
            }
            
            return $true
        }
        else {
            Write-Host "No LLVM tools found. Installation may have failed." -ForegroundColor Red
            return $false
        }
    }
    catch {
        Write-Host "Error testing LLVM installation: $_" -ForegroundColor Red
        return $false
    }
}

# Main installation logic
$success = $false

Write-Host "Detected OS: Windows" -ForegroundColor White
Write-Host "PowerShell Version: $($PSVersionTable.PSVersion)" -ForegroundColor White
Write-Host ""

switch ($Method.ToLower()) {
    "winget" { 
        if (Test-Administrator) {
            $success = Install-LLVM-Winget 
        } else {
            Write-Host "Winget installation requires Administrator privileges." -ForegroundColor Red
            Write-Host "Please run PowerShell as Administrator or use 'manual' method." -ForegroundColor Yellow
        }
    }
    "chocolatey" { 
        if (Test-Administrator) {
            $success = Install-LLVM-Chocolatey 
        } else {
            Write-Host "Chocolatey installation requires Administrator privileges." -ForegroundColor Red
            Write-Host "Please run PowerShell as Administrator or use 'manual' method." -ForegroundColor Yellow
        }
    }
    "manual" { 
        $success = Install-LLVM-Manual 
    }
    "precompiled" { 
        $success = Install-LLVM-Precompiled 
    }
    default {
        Write-Host "Unknown installation method: $Method" -ForegroundColor Red
        Write-Host "Available methods: winget, chocolatey, manual, precompiled" -ForegroundColor Yellow
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
        Write-Host "You can now build HULK Compiler with LLVM support:" -ForegroundColor White
        Write-Host "  1. Restart PowerShell (important!)" -ForegroundColor Yellow
        Write-Host "  2. Navigate to your project directory" -ForegroundColor Yellow
        Write-Host "  3. Run: make check-llvm" -ForegroundColor Yellow
        Write-Host "  4. Run: make compile" -ForegroundColor Yellow
    }
    else {
        Write-Host ""
        Write-Host "Installation may have completed, but LLVM tools are not detected." -ForegroundColor Yellow
        Write-Host "Please restart PowerShell and test manually with: llvm-config --version" -ForegroundColor Yellow
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
    Write-Host "Installation failed. You can:" -ForegroundColor Red
    Write-Host "  1. Try a different method: .\install-llvm-simple.ps1 -Method chocolatey" -ForegroundColor Yellow
    Write-Host "  2. Install manually: .\install-llvm-simple.ps1 -Method manual" -ForegroundColor Yellow
    Write-Host "  3. Continue without LLVM (the compiler will work without it)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Press any key to exit..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
