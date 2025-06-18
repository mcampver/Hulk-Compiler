#pragma once
#include "../AST/ast.hpp"
#include "CodeGenContext.hpp"
#include "llvm/IR/Value.h"

// Forward declaration to avoid circular includes
class SemanticAnalyzer;

/**
 * @brief LLVM IR code generator using the visitor pattern
 * 
 * This class traverses the AST and generates LLVM IR code.
 * It implements both ExprVisitor and StmtVisitor interfaces to handle different node types.
 */
class LLVMCodeGenerator : public ExprVisitor, public StmtVisitor {
private:
    // Internal context for standalone use (must be declared before context_)
    std::unique_ptr<CodeGenContext> owned_context_;
    CodeGenContext& context_;
    
    // Reference to semantic analyzer for type information
    class SemanticAnalyzer* semantic_analyzer_;
    
    // Helper methods for built-in operations
    llvm::Value* generateBinaryOperation(const std::string& op, 
                                        llvm::Value* left, llvm::Value* right,
                                        const std::string& left_type, const std::string& right_type);
    llvm::Value* generateUnaryOperation(const std::string& op, 
                                       llvm::Value* operand, const std::string& operand_type);
    llvm::Value* generateBuiltinCall(const std::string& name, 
                                    const std::vector<llvm::Value*>& args);
      // Type conversion helpers
    llvm::Value* convertToString(llvm::Value* value, const std::string& from_type);    llvm::Value* convertToNumber(llvm::Value* value, const std::string& from_type);
    llvm::Value* convertToBoolean(llvm::Value* value, const std::string& from_type);
    
    // Helper to ensure a value is converted to string type for concatenation
    llvm::Value* ensureStringType(llvm::Value* value, const std::string& type_hint);
    
    // Helper to create struct types for custom types
    void createStructForType(TypeDecl* type);
    
    // Helper to generate init method body with field assignments
    void generateInitMethodBody(TypeDecl* type, const std::vector<std::string>& method_params, llvm::Function* llvm_func);
    
    // Helper to create inherited init methods automatically
    void createInheritedInitIfNeeded(TypeDecl* type);
      // Helper to infer field type from default value
    llvm::Type* inferFieldType(Expr* default_value);
    
    // Helper to determine return type based on method name and content
    llvm::Type* getMethodReturnType(const std::string& method_name, const ExprPtr& method_body);
    
    // Helper to check if an expression contains string operations
    bool containsStringOperations(Expr* expr);
    
public:
    // Helper methods for creating object creation functions
    void createObjectCreationFunctions(Program* prog);
    void createObjectCreationFunction(const std::string& type_name, const std::vector<std::string>& params);
    
    // Helper to generate main function content properly
    void generateMainContent(Program* prog);
      // Constructor with external context
    explicit LLVMCodeGenerator(CodeGenContext& context, SemanticAnalyzer* analyzer = nullptr) 
        : owned_context_(nullptr), context_(context), semantic_analyzer_(analyzer) {}
    
    // Constructor with module name (creates internal context)
    explicit LLVMCodeGenerator(const std::string& module_name, SemanticAnalyzer* analyzer = nullptr);
    
    // Method to print the generated LLVM module
    void printModule();
    
    // StmtVisitor methods
    void visit(Program* prog) override;
    void visit(ExprStmt* stmt) override;
    void visit(FunctionDecl* func) override;
    void visit(TypeDecl* type) override;
    
    // Additional method for main expressions
    void processMainExpressions(Program* prog);
    
    // ExprVisitor methods
    void visit(NumberExpr* expr) override;
    void visit(StringExpr* expr) override;
    void visit(BooleanExpr* expr) override;
    void visit(UnaryExpr* expr) override;
    void visit(BinaryExpr* expr) override;
    void visit(CallExpr* expr) override;
    void visit(VariableExpr* expr) override;
    void visit(LetExpr* expr) override;
    void visit(AssignExpr* expr) override;
    void visit(IfExpr* expr) override;
    void visit(ExprBlock* block) override;
    void visit(WhileExpr* expr) override;
    
    // Missing methods that need to be implemented
    void visit(NewExpr* expr) override;
    void visit(MemberExpr* expr) override;
    void visit(SelfExpr* expr) override;
    void visit(BaseExpr* expr) override;
    void visit(MemberAssignExpr* expr) override;
    void visit(MethodCallExpr* expr) override;
    
private:
    // Helper methods
    llvm::Type* inferParameterType(const std::string& method_name, const std::string& param_name);
};