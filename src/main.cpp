#include <cstdio>
#include <iostream>
#include <cstring>

#include "AST/ast.hpp"
#include "Evaluator/evaluator.hpp"
#include "PrintVisitor/print_visitor.hpp"
#include "Value/value.hpp"
#include "Scope/scope.hpp"
#include "Scope/name_resolver.hpp"
#include "Semantic/SemanticAnalyzer.hpp"

// LLVM includes - conditional compilation
#ifndef ENABLE_LLVM
#define ENABLE_LLVM 0
#endif

#if ENABLE_LLVM
#include "CodeGen/LLVMCodeGenerator.hpp"
// #include <llvm/Support/raw_ostream.h>
// #include <llvm/IR/Verifier.h>
#endif

extern FILE *yyin;
extern int yyparse();
extern int yylineno;
extern Program *rootAST;

enum CompilationMode {
    MODE_INTERPRET,  // Default: interpret mode
    MODE_SEMANTIC,   // Semantic analysis only
    MODE_LLVM       // LLVM code generation
};

int main(int argc, char *argv[])
{
    bool debugMode = false;
    bool showIR = false;
    const char* filename = nullptr;
    const char* outputFile = nullptr;
    CompilationMode mode = MODE_INTERPRET;
      // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debugMode = true;
        } else if (strcmp(argv[i], "--semantic") == 0) {
            mode = MODE_SEMANTIC;
        } else if (strcmp(argv[i], "--show-ir") == 0) {
            showIR = true;
        } else if (strcmp(argv[i], "--llvm") == 0) {
#if ENABLE_LLVM
            mode = MODE_LLVM;
#else
            std::cerr << "Error: LLVM support not available. Recompile with LLVM installed.\n";
            return 1;
#endif
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (filename == nullptr) {
            filename = argv[i];
        }
    }
    
    if (filename == nullptr)
    {
        std::cerr << "Uso: " << argv[0] << " [opciones] <archivo.hulk>" << std::endl;        std::cerr << "Opciones:" << std::endl;
        std::cerr << "  --debug     Activar modo de depuración" << std::endl;
        std::cerr << "  --semantic  Solo análisis semántico" << std::endl;
        std::cerr << "  --llvm      Generar código LLVM IR" << std::endl;
        std::cerr << "  --show-ir   Mostrar código LLVM IR generado" << std::endl;
        std::cerr << "  -o <file>   Archivo de salida (solo para --llvm)" << std::endl;
        return 1;
    }

    FILE *file = fopen(filename, "r");
    if (!file)
    {
        std::cerr << "Error: No se pudo abrir el archivo: " << filename << std::endl;
        return 1;
    }    if (debugMode) {
        std::cout << "=== HULK Compiler Enhanced ===\n";
        std::cout << "Archivo: " << filename << "\n";
        std::cout << "Modo: ";
        switch(mode) {
            case MODE_INTERPRET: std::cout << "Interpretación"; break;
            case MODE_SEMANTIC: std::cout << "Análisis semántico"; break;
            case MODE_LLVM: std::cout << "Generación LLVM IR"; break;
        }
        std::cout << "\n\n";
    }

    yylineno = 1;
    yyin = file;

    if (yyparse() != 0 || rootAST == nullptr)
    {
        std::cerr << "Error al parsear.\n";
        fclose(file);
        return 1;
    }

    // 1) Always perform basic name resolution
    try
    {
        if (debugMode) std::cout << "=== Resolviendo nombres ===\n";
        NameResolver resolver;
        rootAST->accept(&resolver);
        if (debugMode) std::cout << "=== Resolución de nombres OK ===\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error en resolución de nombres: " << e.what() << "\n";
        fclose(file);
        return 2;
    }    // 2) Enhanced semantic analysis (for semantic and LLVM modes)
    if (mode == MODE_SEMANTIC || mode == MODE_LLVM) {
        if (debugMode) std::cout << "=== Iniciando análisis semántico avanzado ===\n";
        
        try {
            SemanticAnalyzer analyzer;
            analyzer.analyze(rootAST);
              if (analyzer.hasErrors()) {
                std::cerr << "Errores semánticos encontrados:\n";
                for (const auto& error : analyzer.getErrors()) {
                    std::cerr << "Línea " << error.line << ": " << error.message << "\n";
                }
                fclose(file);
                return 2;
            }
            
            if (debugMode) std::cout << "=== Análisis semántico completado exitosamente ===\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Error durante análisis semántico: " << e.what() << "\n";
            fclose(file);
            return 2;
        }
        
        if (mode == MODE_SEMANTIC) {
            std::cout << "Análisis semántico completado exitosamente.\n";
            fclose(file);
            return 0;
        }    }    // 3) LLVM code generation
#if ENABLE_LLVM
    if (mode == MODE_LLVM || showIR) {
        if (debugMode) std::cout << "=== Iniciando generación de código LLVM ===\n";
        
        try {
            LLVMCodeGenerator codegen("hulk_module");
            rootAST->accept(&codegen);
            
            if (mode == MODE_LLVM || showIR) {
                std::cout << "\n=== Código LLVM IR Generado ===\n";
                codegen.printModule();
                std::cout << "=== Fin del código LLVM IR ===\n\n";
            }
            
            if (debugMode) std::cout << "=== Generación de código LLVM completada ===\n";
            
            // Si solo queremos mostrar IR, continuamos con la ejecución normal
            if (mode == MODE_LLVM) {
                fclose(file);
                return 0;
            }
        }
        catch (const std::exception &e) {
            std::cerr << "Error en generación de código LLVM: " << e.what() << "\n";
            if (mode == MODE_LLVM) {
                fclose(file);
                return 4;
            }
            // Si es solo --show-ir, continuamos con la ejecución
        }
    }
#endif

    // 4) Pretty-print del AST completo (solo en modo debug para interpretación)
    if (debugMode && mode == MODE_INTERPRET) {
        std::cout << "\n=== AST (Árbol de Sintaxis Abstracta) ===\n";
        PrintVisitor printer;
        rootAST->accept(&printer);
    }

    // 5) Exit early for semantic-only mode
    if (mode == MODE_SEMANTIC) {
        std::cout << "Análisis semántico completado exitosamente.\n";
        fclose(file);
        return 0;
    }

    // 6) Execution (only for interpret mode)
    if (mode == MODE_INTERPRET) {
        std::cout << "\n=== Ejecución ===\n";
        try {
            EvaluatorVisitor evaluator;
            rootAST->accept(&evaluator);
            if (debugMode) {
                std::cout << "\n=== Programa terminado exitosamente ===\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error en ejecución: " << e.what() << "\n";
            fclose(file);
            return 3;
        }
    }

    fclose(file);
    return 0;
}
