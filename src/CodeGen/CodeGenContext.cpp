#include "CodeGenContext.hpp"
#include "../AST/ast.hpp"
#include "LLVMCodeGenerator.hpp"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include <iostream>
#include <stdexcept>

CodeGenContext::CodeGenContext() 
    : context_()
    , module_(std::make_unique<llvm::Module>("hulk_enhanced_module", context_))
    , builder_(std::make_unique<llvm::IRBuilder<>>(context_))
    , current_function_(nullptr) {
    
    // Initialize variable scope
    pushScope();
    
    // Create built-in functions
    createBuiltinFunctions();
}

void CodeGenContext::generateCode(Program* program) {
    LLVMCodeGenerator generator(*this);
    
    // Create main function
    llvm::FunctionType* main_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_), false);
    llvm::Function* main_func = llvm::Function::Create(
        main_type, llvm::Function::ExternalLinkage, "main", *module_);
    
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context_, "entry", main_func);
    builder_->SetInsertPoint(entry);
    
    setCurrentFunction(main_func);
    
    // Generate code for each top-level statement
    program->accept(&generator);
    
    // Return 0 from main
    builder_->CreateRet(llvm::ConstantInt::get(context_, llvm::APInt(32, 0)));
    
    // Verify the module
    std::string error_str;
    llvm::raw_string_ostream error_stream(error_str);
    if (llvm::verifyModule(*module_, &error_stream)) {
        throw std::runtime_error("Module verification failed: " + error_str);
    }
}

llvm::Value* CodeGenContext::popValue() {
    if (value_stack_.empty()) {
        throw std::runtime_error("Attempted to pop from empty value stack");
    }
    llvm::Value* value = value_stack_.top();
    value_stack_.pop();
    return value;
}

llvm::Value* CodeGenContext::peekValue() const {
    if (value_stack_.empty()) {
        return nullptr;
    }
    return value_stack_.top();
}

void CodeGenContext::pushScope() {
    variable_stack_.push(std::map<std::string, llvm::Value*>());
}

void CodeGenContext::popScope() {
    if (!variable_stack_.empty()) {
        variable_stack_.pop();
    }
}

void CodeGenContext::declareVariable(const std::string& name, llvm::Value* value) {
    if (!variable_stack_.empty()) {
        variable_stack_.top()[name] = value;
    }
}

llvm::Value* CodeGenContext::lookupVariable(const std::string& name) {
    // Search from current scope to global scope
    std::stack<std::map<std::string, llvm::Value*>> temp_stack;
    llvm::Value* result = nullptr;
    
    while (!variable_stack_.empty()) {
        auto& current_scope = variable_stack_.top();
        auto it = current_scope.find(name);
        if (it != current_scope.end()) {
            result = it->second;
            break;
        }
        temp_stack.push(current_scope);
        variable_stack_.pop();
    }
    
    // Restore the stack
    while (!temp_stack.empty()) {
        variable_stack_.push(temp_stack.top());
        temp_stack.pop();
    }
    
    return result;
}

void CodeGenContext::declareFunction(const std::string& name, llvm::Function* function) {
    functions_[name] = function;
}

llvm::Function* CodeGenContext::lookupFunction(const std::string& name) {
    auto it = functions_.find(name);
    return (it != functions_.end()) ? it->second : nullptr;
}

void CodeGenContext::dumpIR(const std::string& filename) {
    if (filename.empty()) {
        module_->print(llvm::outs(), nullptr);
    } else {
        std::error_code error_code;
        llvm::raw_fd_ostream file(filename, error_code, llvm::sys::fs::OF_None);
        if (error_code) {
            throw std::runtime_error("Failed to open file for IR output: " + error_code.message());
        }
        module_->print(file, nullptr);
    }
}

llvm::Value* CodeGenContext::createStringConstant(const std::string& str) {
    return builder_->CreateGlobalStringPtr(str);
}

llvm::Value* CodeGenContext::createNumberConstant(double value) {
    return llvm::ConstantFP::get(context_, llvm::APFloat(value));
}

llvm::Value* CodeGenContext::createBooleanConstant(bool value) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context_), value);
}

llvm::Type* CodeGenContext::getLLVMType(const std::string& type_name) {
    if (type_name == "Number" || type_name == "number") {
        return llvm::Type::getDoubleTy(context_);
    } else if (type_name == "String" || type_name == "string") {
        return llvm::Type::getInt8PtrTy(context_);
    } else if (type_name == "Boolean" || type_name == "boolean") {
        return llvm::Type::getInt1Ty(context_);
    } else {
        return llvm::Type::getVoidTy(context_); // Default for unknown types
    }
}

void CodeGenContext::createBuiltinFunctions() {
    // printf function for print implementation
    std::vector<llvm::Type*> printf_args;
    printf_args.push_back(llvm::Type::getInt8PtrTy(context_));
    llvm::FunctionType* printf_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_), printf_args, true);
    llvm::Function* printf_func = llvm::Function::Create(
        printf_type, llvm::Function::ExternalLinkage, "printf", *module_);
    declareFunction("printf", printf_func);
    
    // puts function for simple string output
    std::vector<llvm::Type*> puts_args;
    puts_args.push_back(llvm::Type::getInt8PtrTy(context_));
    llvm::FunctionType* puts_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_), puts_args, false);
    llvm::Function* puts_func = llvm::Function::Create(
        puts_type, llvm::Function::ExternalLinkage, "puts", *module_);
    declareFunction("puts", puts_func);
    
    // Math functions
    auto double_type = llvm::Type::getDoubleTy(context_);
    
    // sin, cos, sqrt, exp functions
    std::vector<std::string> unary_math_funcs = {"sin", "cos", "sqrt", "exp"};
    for (const auto& func_name : unary_math_funcs) {
        llvm::FunctionType* func_type = llvm::FunctionType::get(double_type, {double_type}, false);
        llvm::Function* func = llvm::Function::Create(
            func_type, llvm::Function::ExternalLinkage, func_name, *module_);
        declareFunction(func_name, func);
    }
    
    // Binary math functions (min, max, pow)
    std::vector<std::string> binary_math_funcs = {"fmin", "fmax", "pow"};
    for (const auto& func_name : binary_math_funcs) {
        llvm::FunctionType* func_type = llvm::FunctionType::get(
            double_type, {double_type, double_type}, false);
        llvm::Function* func = llvm::Function::Create(
            func_type, llvm::Function::ExternalLinkage, func_name, *module_);
        declareFunction(func_name, func);
    }
    
    // Random function
    llvm::FunctionType* rand_type = llvm::FunctionType::get(double_type, {}, false);
    llvm::Function* rand_func = llvm::Function::Create(
        rand_type, llvm::Function::ExternalLinkage, "hulk_rand", *module_);
    declareFunction("hulk_rand", rand_func);
    
    // String concatenation functions (for your enhanced operators)
    auto string_type = llvm::Type::getInt8PtrTy(context_);
    
    // String concat (@)
    llvm::FunctionType* concat_type = llvm::FunctionType::get(
        string_type, {string_type, string_type}, false);
    llvm::Function* concat_func = llvm::Function::Create(
        concat_type, llvm::Function::ExternalLinkage, "hulk_str_concat", *module_);
    declareFunction("hulk_str_concat", concat_func);
    
    // String concat with space (@@)
    llvm::Function* concat_space_func = llvm::Function::Create(
        concat_type, llvm::Function::ExternalLinkage, "hulk_str_concat_space", *module_);
    declareFunction("hulk_str_concat_space", concat_space_func);
    
    // String equality
    llvm::FunctionType* str_eq_type = llvm::FunctionType::get(
        llvm::Type::getInt1Ty(context_), {string_type, string_type}, false);
    llvm::Function* str_eq_func = llvm::Function::Create(
        str_eq_type, llvm::Function::ExternalLinkage, "hulk_str_equals", *module_);
    declareFunction("hulk_str_equals", str_eq_func);
}