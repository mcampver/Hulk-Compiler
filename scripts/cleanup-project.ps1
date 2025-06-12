# Clean up and organize HULK Compiler Enhanced project

# Remove object files
Write-Host "Removing object files..." -ForegroundColor Cyan
Get-ChildItem -Path "src" -Filter "*.o" -Recurse | Remove-Item -Force

# Remove auto-generated files (if not needed immediately)
Write-Host "Removing generated files..." -ForegroundColor Cyan
if (Test-Path "src\Lexer\lex.yy.cpp") {
    Remove-Item -Path "src\Lexer\lex.yy.cpp" -Force
}
if (Test-Path "src\Parser\parser.tab.cpp") {
    Remove-Item -Path "src\Parser\parser.tab.cpp" -Force
}
if (Test-Path "src\Parser\parser.tab.hpp") {
    Remove-Item -Path "src\Parser\parser.tab.hpp" -Force
}

# Make sure necessary directories exist
Write-Host "Ensuring all directories exist..." -ForegroundColor Cyan
$directories = @(
    "build",
    "build\AST", 
    "build\CodeGen",
    "build\Evaluator",
    "build\Lexer",
    "build\Parser", 
    "build\PrintVisitor",
    "build\Runtime",
    "build\Scope",
    "build\Semantic",
    "build\Value",
    "hulk"
)

foreach ($dir in $directories) {
    if (-Not (Test-Path $dir)) {
        New-Item -Path $dir -ItemType Directory -Force
        Write-Host "Created directory: $dir" -ForegroundColor Green
    }
}

# Create .gitkeep files to preserve empty directories
Write-Host "Creating .gitkeep files..." -ForegroundColor Cyan
foreach ($dir in $directories) {
    if (-Not (Test-Path "$dir\.gitkeep")) {
        New-Item -Path "$dir\.gitkeep" -ItemType File -Force
        Write-Host "Created .gitkeep in: $dir" -ForegroundColor Green
    }
}

# Remove redundant LLVM demo files from CodeGen
if (Test-Path "src\CodeGen\LLVMCodeGenerator_DEMO.cpp") {
    Write-Host "Moving redundant LLVM demo file to backup..." -ForegroundColor Yellow
    if (-Not (Test-Path "backup\CodeGen")) {
        New-Item -Path "backup\CodeGen" -ItemType Directory -Force
    }
    Move-Item -Path "src\CodeGen\LLVMCodeGenerator_DEMO.cpp" -Destination "backup\CodeGen\" -Force
}

if (Test-Path "src\CodeGen\LLVMDemo.cpp") {
    Write-Host "Moving redundant LLVM demo file to backup..." -ForegroundColor Yellow
    if (-Not (Test-Path "backup\CodeGen")) {
        New-Item -Path "backup\CodeGen" -ItemType Directory -Force
    }
    Move-Item -Path "src\CodeGen\LLVMDemo.cpp" -Destination "backup\CodeGen\" -Force
}

if (Test-Path "src\CodeGen\LLVMCodeGenerator.cpp.new") {
    Write-Host "Moving redundant LLVM file version to backup..." -ForegroundColor Yellow
    if (-Not (Test-Path "backup\CodeGen")) {
        New-Item -Path "backup\CodeGen" -ItemType Directory -Force
    }
    Move-Item -Path "src\CodeGen\LLVMCodeGenerator.cpp.new" -Destination "backup\CodeGen\" -Force
}

if (Test-Path "src\CodeGen\LLVMGenerator.hpp") {
    Write-Host "Moving redundant LLVM header to backup..." -ForegroundColor Yellow
    if (-Not (Test-Path "backup\CodeGen")) {
        New-Item -Path "backup\CodeGen" -ItemType Directory -Force
    }
    Move-Item -Path "src\CodeGen\LLVMGenerator.hpp" -Destination "backup\CodeGen\" -Force
}

Write-Host "Clean-up completed successfully!" -ForegroundColor Green
