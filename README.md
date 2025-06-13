# HULK Compiler Enhanced 🚀

A modern, feature-rich compiler for the HULK programming language with advanced semantic analysis and LLVM code generation capabilities.

## ✨ Features

### Enhanced Language Features
- **8 New Operators**: `//, %%, +++, &, |, !, @, @@`
- **Enhanced Built-in Functions**: `debug()`, `type()`, `assert()`
- **Debug Mode**: Comprehensive debugging information
- **Advanced String Operations**: Concatenation, repetition, and manipulation

### Compilation Modes
1. **Interpretation Mode** (Default): Execute HULK programs directly
2. **Semantic Analysis Mode**: Static type checking and error detection
3. **LLVM Code Generation**: Generate optimized LLVM IR

### Professional Architecture
- **Visitor Pattern**: Clean separation of concerns
- **Type System**: Comprehensive type inference and checking
- **Error Reporting**: Detailed error messages with location information
- **Symbol Management**: Multi-scope symbol tables
- **Runtime Library**: C runtime for enhanced operations

## 🔧 Building

### Prerequisites
- C++17 compatible compiler (GCC/Clang)
- LLVM development libraries (version 10.0 or higher) - optional for code generation
- Flex (lexical analyzer generator)
- Bison (parser generator)
- Make

### Installation on Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential flex bison
sudo apt-get install llvm-dev libllvm-ocaml-dev
```

### Installation on macOS
```bash
brew install llvm flex bison
export PATH="/usr/local/opt/llvm/bin:$PATH"
```

### Installation on Windows
Windows users can install the prerequisites using:
1. MSYS2 with MinGW
2. Windows Subsystem for Linux (WSL)
3. Direct installation using the provided scripts

For LLVM on Windows:
```powershell
# Run the installation script with administrator permissions
.\scripts\install-llvm-windows.ps1
```

### Building the Compiler
```bash
# Build the compiler
make

# Check LLVM installation
make check-llvm

# Clean build files
make clean
```

## 📂 Project Structure

The HULK Compiler Enhanced project is organized as follows:

```
HULK-Compiler-Enhanced/
├── backup/            # Backup of old or redundant files
├── build/             # Build artifacts (object files, intermediate files)
├── docs/              # Documentation files
├── examples/          # Example HULK programs
├── hulk/              # Compiled executables
├── include/           # Public header files
├── scripts/           # Installation and utility scripts
├── src/               # Source code
│   ├── AST/           # Abstract Syntax Tree classes
│   ├── CodeGen/       # LLVM code generation
│   ├── Evaluator/     # Interpreter engine
│   ├── Lexer/         # Lexical analysis
│   ├── Parser/        # Syntactic analysis
│   ├── PrintVisitor/  # AST visualization
│   ├── Runtime/       # Runtime support functions
│   ├── Scope/         # Scope and name resolution
│   ├── Semantic/      # Semantic analysis
│   ├── Value/         # Value representation
│   └── main.cpp       # Entry point
└── tests/             # Test cases and test scripts
```

## 🔄 Build System

The build system has been consolidated into a single unified Makefile with cross-platform support. It automatically detects the operating system (Windows or Unix-like) and adapts accordingly.

### Key Features

- Automatic LLVM detection and conditional compilation
- Support for both Windows and Unix-like operating systems
- Proper directory structure for build artifacts
- Test commands for different compilation modes

### Build Commands

- `make` - Build the compiler
- `make clean` - Clean build artifacts
- `make distclean` - Clean build artifacts and output files
- `make check-llvm` - Check LLVM availability
- `make run FILE=<filename>` - Run the compiler with a specific file

## 🧪 Running Tests

```bash
# Run the interpreter with a test file
make run FILE=tests/script.hulk

# Run with debugging enabled
make run FILE=script.hulk ARGS="--debug"

# Run semantic analysis only
make run FILE=script.hulk ARGS="--semantic"

# Generate LLVM IR (if LLVM is available)
make run FILE=script.hulk ARGS="--llvm" -o output.ll
```

## 📚 Language Examples

### Basic Expression
```
print("Result: " + (42 // 5));  # Integer division
```

### Enhanced Operators
```
let x = 15;
let y = 4;

print(x // y);   # Integer division: 3
print(x %% y);   # Enhanced modulo (always positive): 3
print("HULK" +++ 3);   # Triple addition/concatenation: "HULKHULKHULK"
print("Hello" @ "World");   # String concatenation: "HelloWorld"
print(true & false);   # Logical AND: false
print(true | false);   # Logical OR: true
print(!true);   # Logical NOT: false
```

## 🛠️ Maintenance

For project maintenance, use the provided cleanup script:

```powershell
# On Windows
.\scripts\cleanup-project.ps1
```

## 📖 Documentation

Additional documentation can be found in the `docs/` directory:
- Architecture documentation
- Build system information
- LLVM integration plans
- Semantic analysis enhancements

## 🤝 Contributing

When contributing to this project, please follow the established project structure:

1. Place new source files in the appropriate subdirectory under `src/`
2. Add tests for new features in the `tests/` directory
3. Document significant changes in the `docs/` directory
4. Use the unified build system (Makefile) for compilation

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.
