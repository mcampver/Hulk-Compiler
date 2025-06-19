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
    
    // Type management for custom types (structs)
    std::map<std::string, llvm::StructType*> custom_types_;
    std::map<std::string, std::vector<std::string>> type_fields_; // field names for each type
    
    // Virtual table management for polymorphism
    std::map<std::string, llvm::GlobalVariable*> vtables_;
    std::map<std::string, std::vector<std::string>> virtual_methods_; // method names for each type
    
    // Value stack for expressions
    std::stack<llvm::Value*> value_stack_;
    
    // Current function being generated
    llvm::Function* current_function_;
    
    // Current context for method generation
    llvm::Value* current_self_;
    std::string current_type_;
    
    // Symbol table for variable types (variable_name -> type_name)
    std::map<std::string, std::string> variable_types_;
    
    // Track let expressions for proper main generation
    std::vector<std::pair<std::string, std::string>> let_variables_; // (var_name, type_name)
    
    // Track inheritance relationships (child -> parent)
    std::map<std::string, std::string> inheritance_map_;
    
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
    void generateCode(struct Program* program);
    
    // Value stack management
    void pushValue(llvm::Value* value) { value_stack_.push(value); }
    llvm::Value* popValue();
    llvm::Value* peekValue() const;
    bool hasValue() const { return !value_stack_.empty(); }
    
    // Variable management
    void pushScope();
    void popScope();
    void declareVariable(const std::string& name, llvm::Value* value);
    llvm::Value* lookupVariable(const std::string& name);
    
    // Function management
    void declareFunction(const std::string& name, llvm::Function* function);
    llvm::Function* lookupFunction(const std::string& name);
    
    // Type management
    void declareType(const std::string& name, llvm::StructType* type, const std::vector<std::string>& field_names);
    llvm::StructType* lookupType(const std::string& name);
    std::vector<std::string> getTypeFields(const std::string& name);
    int getFieldIndex(const std::string& type_name, const std::string& field_name);
    
    // Virtual table management
    void createVTable(const std::string& type_name, const std::vector<std::string>& method_names);
    llvm::GlobalVariable* getVTable(const std::string& type_name);
    
    // Memory management helpers
    llvm::Value* createObjectAllocation(const std::string& type_name);
    llvm::Constant* getTypeSize(const std::string& type_name);
    
    // Current function
    void setCurrentFunction(llvm::Function* function) { current_function_ = function; }
    llvm::Function* getCurrentFunction() const { return current_function_; }
    
    // Current context management
    void setCurrentSelf(llvm::Value* self) { current_self_ = self; }
    llvm::Value* getCurrentSelf() const { return current_self_; }
    void setCurrentType(const std::string& type) { current_type_ = type; }
    std::string getCurrentType() const { return current_type_; }
    
    // Output
    void dumpIR(const std::string& filename = "");
    void writeObjectFile(const std::string& filename);
    
    // Utility
    llvm::Value* createStringConstant(const std::string& str);
    llvm::Value* createNumberConstant(double value);
    llvm::Value* createBooleanConstant(bool value);
    llvm::Type* getLLVMType(const std::string& type_name);
    
    // Dynamic type management
    void declareVariableType(const std::string& name, const std::string& type);
    std::string getVariableType(const std::string& name) const;
    void addLetVariable(const std::string& name, const std::string& type);
    const std::vector<std::pair<std::string, std::string>>& getLetVariables() const;
    
    // Inheritance management
    void declareInheritance(const std::string& child, const std::string& parent);
    std::string getParentType(const std::string& child) const;
    
    // Type management helpers
    llvm::Type* getFieldType(const std::string& type_name, const std::string& field_name);
    llvm::Type* getFieldType(const std::string& type_name, int field_index);
    
    // Get all type names defined in the program
    std::vector<std::string> getAllTypeNames() const {
        std::vector<std::string> types;
        for (const auto& pair : custom_types_) {
            types.push_back(pair.first);
        }
        return types;
    }
};