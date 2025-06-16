# Test HULK Compiler without LLVM
# This script tests the compiler functionality when LLVM is not available

Write-Host "=== Testing HULK Compiler (No LLVM) ===" -ForegroundColor Green

# Check if project directory exists
if (-not (Test-Path "makefile")) {
    Write-Host "Error: No makefile found. Run this script from the project root directory." -ForegroundColor Red
    exit 1
}

Write-Host "1. Checking LLVM status..." -ForegroundColor Yellow
make check-llvm

Write-Host "`n2. Cleaning previous build..." -ForegroundColor Yellow
make clean

Write-Host "`n3. Building compiler..." -ForegroundColor Yellow
$buildResult = make compile
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "`n4. Testing help output..." -ForegroundColor Yellow
.\hulk\hulk_compiler.exe

Write-Host "`n5. Testing with a simple script..." -ForegroundColor Yellow

# Create a simple test file
$testScript = @"
// Simple HULK test
let x = 42;
let message = "Hello, HULK!";
print(message);
print("The answer is: " @ string(x));
"@

$testScript | Out-File -FilePath "test_no_llvm.hulk" -Encoding UTF8

Write-Host "Created test file: test_no_llvm.hulk" -ForegroundColor Cyan
Write-Host "Content:" -ForegroundColor Cyan
Get-Content "test_no_llvm.hulk"

Write-Host "`n6. Running interpretation mode..." -ForegroundColor Yellow
.\hulk\hulk_compiler.exe --debug test_no_llvm.hulk

Write-Host "`n7. Running semantic analysis mode..." -ForegroundColor Yellow
.\hulk\hulk_compiler.exe --debug --semantic test_no_llvm.hulk

Write-Host "`n8. Testing LLVM mode (should fail gracefully)..." -ForegroundColor Yellow
Write-Host "Expected: Error message about LLVM not being available" -ForegroundColor Cyan
.\hulk\hulk_compiler.exe --llvm test_no_llvm.hulk

# Cleanup
Remove-Item "test_no_llvm.hulk" -Force

Write-Host "`n=== Test Complete ===" -ForegroundColor Green
Write-Host "The compiler should work in interpretation and semantic modes." -ForegroundColor White
Write-Host "LLVM mode should show an appropriate error message." -ForegroundColor White
