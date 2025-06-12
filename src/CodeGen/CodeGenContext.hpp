#pragma once
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <stack>

// Forward declaration
class ASTNode;

/**
 * @brief Context for LLVM code generation
 * 
 * This class manages the LLVM IR generation state including:
 * - LLVM context, module, and builder
 * - Symbol tables for variables and functions
 * - Value stack for expression evaluation
 * - Scope management
 */
class CodeGenContext {
private:
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    
    // Symbol tables
    std::stack<std::map<std::string, llvm::Value*>> variable_stack_;
    std::map<std::string, llvm::Function*> functions_;
    
    // Value stack for expressions
    std::stack<llvm::Value*> value_stack_;
    
    // Current function being generated
    llvm::Function* current_function_;
    
    // Helper methods
    void createBuiltinFunctions();
    
public:
    CodeGenContext();
    ~CodeGenContext() = default;
    
    // Context access
    llvm::LLVMContext& getLLVMContext() { return context_; }
    llvm::Module& getModule() { return *module_; }
    llvm::IRBuilder<>& getBuilder() { return *builder_; }
    
    // Code generation
    void generateCode(class Program* program);
    
    // Value stack management
    void pushValue(llvm::Value* value) { value_stack_.push(value); }
    llvm::Value* popValue();
    llvm::Value* peekValue() const;
    
    // Variable management
    void pushScope();
    void popScope();
    void declareVariable(const std::string& name, llvm::Value* value);
    llvm::Value* lookupVariable(const std::string& name);
    
    // Function management
    void declareFunction(const std::string& name, llvm::Function* function);
    llvm::Function* lookupFunction(const std::string& name);
    
    // Current function
    void setCurrentFunction(llvm::Function* function) { current_function_ = function; }
    llvm::Function* getCurrentFunction() const { return current_function_; }
    
    // Output
    void dumpIR(const std::string& filename = "");
    void writeObjectFile(const std::string& filename);
    
    // Utility
    llvm::Value* createStringConstant(const std::string& str);
    llvm::Value* createNumberConstant(double value);
    llvm::Value* createBooleanConstant(bool value);
    llvm::Type* getLLVMType(const std::string& type_name);
};