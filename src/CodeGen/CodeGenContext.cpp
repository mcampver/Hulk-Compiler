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
    , current_function_(nullptr)
    , current_self_(nullptr)
    , current_type_("") {
    
    // Initialize variable scope
    pushScope();
    
    // Create built-in functions
    createBuiltinFunctions();
}

void CodeGenContext::generateCode(Program* program) {
    LLVMCodeGenerator generator(*this);
    
    // First: Process type and function declarations (not in main)
    // Clear any current function context
    setCurrentFunction(nullptr);
    
    // Generate code for type and function declarations
    program->accept(&generator);
    
    // Create object creation functions for all types
    generator.createObjectCreationFunctions(program);
    
    // Create main function for main expressions
    llvm::FunctionType* main_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_), false);
    llvm::Function* main_func = llvm::Function::Create(
        main_type, llvm::Function::ExternalLinkage, "main", *module_);
    
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context_, "entry", main_func);
    builder_->SetInsertPoint(entry);
    setCurrentFunction(main_func);
    
    // Process main expressions in the main function context
    generator.generateMainContent(program);
    
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
    return builder_->CreateGlobalString(str, "str");
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
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_));
    } else if (type_name == "Boolean" || type_name == "boolean") {
        return llvm::Type::getInt1Ty(context_);
    } else {
        return llvm::Type::getVoidTy(context_); // Default for unknown types
    }
}

void CodeGenContext::createBuiltinFunctions() {
    // printf function for print implementation
    std::vector<llvm::Type*> printf_args;
    printf_args.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_)));
    llvm::FunctionType* printf_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_), printf_args, true);
    llvm::Function* printf_func = llvm::Function::Create(
        printf_type, llvm::Function::ExternalLinkage, "printf", *module_);
    declareFunction("printf", printf_func);
    
    // puts function for simple string output
    std::vector<llvm::Type*> puts_args;
    puts_args.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_)));
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
    auto string_type = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_));
    
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
    
    // Type conversion functions
    // Double to string conversion
    llvm::FunctionType* double_to_str_type = llvm::FunctionType::get(
        string_type, {double_type}, false);
    llvm::Function* double_to_str_func = llvm::Function::Create(
        double_to_str_type, llvm::Function::ExternalLinkage, "hulk_double_to_str", *module_);
    declareFunction("hulk_double_to_str", double_to_str_func);
    
    // Boolean to string conversion
    llvm::FunctionType* bool_to_str_type = llvm::FunctionType::get(
        string_type, {llvm::Type::getInt1Ty(context_)}, false);
    llvm::Function* bool_to_str_func = llvm::Function::Create(
        bool_to_str_type, llvm::Function::ExternalLinkage, "hulk_bool_to_str", *module_);
    declareFunction("hulk_bool_to_str", bool_to_str_func);
}

// Type management methods
void CodeGenContext::declareType(const std::string& name, llvm::StructType* type, const std::vector<std::string>& field_names) {
    custom_types_[name] = type;
    type_fields_[name] = field_names;
}

llvm::StructType* CodeGenContext::lookupType(const std::string& name) {
    auto it = custom_types_.find(name);
    return (it != custom_types_.end()) ? it->second : nullptr;
}

std::vector<std::string> CodeGenContext::getTypeFields(const std::string& name) {
    auto it = type_fields_.find(name);
    return (it != type_fields_.end()) ? it->second : std::vector<std::string>();
}

int CodeGenContext::getFieldIndex(const std::string& type_name, const std::string& field_name) {
    auto fields = getTypeFields(type_name);
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i] == field_name) {
            return static_cast<int>(i);
        }
    }
    return -1; // Field not found
}

// Type management helper methods
llvm::Type* CodeGenContext::getFieldType(const std::string& type_name, const std::string& field_name) {
    int field_index = getFieldIndex(type_name, field_name);
    if (field_index >= 0) {
        return getFieldType(type_name, field_index);
    }
    return nullptr;
}

llvm::Type* CodeGenContext::getFieldType(const std::string& type_name, int field_index) {
    llvm::StructType* struct_type = lookupType(type_name);
    if (!struct_type || field_index < 0 || static_cast<size_t>(field_index) >= struct_type->getNumElements()) {
        return nullptr;
    }
    return struct_type->getElementType(static_cast<unsigned>(field_index));
}

// Virtual table management methods
void CodeGenContext::createVTable(const std::string& type_name, const std::vector<std::string>& method_names) {
    // Create array type for vtable (array of function pointers)
    llvm::Type* func_ptr_type = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_));
    llvm::ArrayType* vtable_type = llvm::ArrayType::get(func_ptr_type, method_names.size());
    
    // Create vtable as global variable
    std::string vtable_name = type_name + "_vtable";
    llvm::GlobalVariable* vtable = new llvm::GlobalVariable(
        *module_, vtable_type, true, // isConstant = true
        llvm::GlobalValue::ExternalLinkage, nullptr, vtable_name);
    
    vtables_[type_name] = vtable;
    virtual_methods_[type_name] = method_names;
}

llvm::GlobalVariable* CodeGenContext::getVTable(const std::string& type_name) {
    auto it = vtables_.find(type_name);
    return (it != vtables_.end()) ? it->second : nullptr;
}

// Memory management helper methods
llvm::Value* CodeGenContext::createObjectAllocation(const std::string& type_name) {
    llvm::StructType* struct_type = lookupType(type_name);
    if (!struct_type) {
        return nullptr;
    }
      // Calculate size needed for this type
    const llvm::DataLayout& data_layout = module_->getDataLayout();
    uint64_t type_size = data_layout.getTypeAllocSize(struct_type);
    
    // Create malloc call
    llvm::Function* malloc_func = lookupFunction("malloc");
    if (!malloc_func) {
        // Declare malloc if not already declared
        llvm::FunctionType* malloc_type = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_)),
            {llvm::Type::getInt64Ty(context_)}, false);
        malloc_func = llvm::Function::Create(
            malloc_type, llvm::Function::ExternalLinkage, "malloc", *module_);
        declareFunction("malloc", malloc_func);
    }
    
    // Create malloc call with proper size
    llvm::Value* size_val = llvm::ConstantInt::get(context_, llvm::APInt(64, type_size));
    llvm::Value* allocated_ptr = builder_->CreateCall(malloc_func, {size_val});
    
    // Cast to proper type
    return builder_->CreateBitCast(allocated_ptr, llvm::PointerType::getUnqual(struct_type));
}

llvm::Constant* CodeGenContext::getTypeSize(const std::string& type_name) {    llvm::StructType* struct_type = lookupType(type_name);
    if (!struct_type) {
        return llvm::ConstantInt::get(context_, llvm::APInt(64, 0));
    }
    
    const llvm::DataLayout& data_layout = module_->getDataLayout();
    uint64_t type_size = data_layout.getTypeAllocSize(struct_type);
    return llvm::ConstantInt::get(context_, llvm::APInt(64, type_size));
}

// Dynamic type management methods
void CodeGenContext::declareVariableType(const std::string& name, const std::string& type) {
    variable_types_[name] = type;
}

std::string CodeGenContext::getVariableType(const std::string& name) const {
    auto it = variable_types_.find(name);
    return (it != variable_types_.end()) ? it->second : "";
}

void CodeGenContext::addLetVariable(const std::string& name, const std::string& type) {
    let_variables_.push_back({name, type});
}

const std::vector<std::pair<std::string, std::string>>& CodeGenContext::getLetVariables() const {
    return let_variables_;
}

// Inheritance management methods
void CodeGenContext::declareInheritance(const std::string& child, const std::string& parent) {
    inheritance_map_[child] = parent;
}

std::string CodeGenContext::getParentType(const std::string& child) const {
    auto it = inheritance_map_.find(child);
    return (it != inheritance_map_.end()) ? it->second : "";
}