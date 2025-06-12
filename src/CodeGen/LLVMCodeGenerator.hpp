#pragma once
#include "../AST/ast.hpp"
#include "CodeGenContext.hpp"
#include "llvm/IR/Value.h"

/**
 * @brief LLVM IR code generator using the visitor pattern
 * 
 * This class traverses the AST and generates LLVM IR code.
 * It implements the ExprVisitor interface to handle different node types.
 */
class LLVMCodeGenerator : public ExprVisitor {
private:
    CodeGenContext& context_;
    
    // Helper methods for built-in operations
    llvm::Value* generateBinaryOperation(const std::string& op, 
                                        llvm::Value* left, llvm::Value* right,
                                        const std::string& left_type, const std::string& right_type);
    llvm::Value* generateUnaryOperation(const std::string& op, 
                                       llvm::Value* operand, const std::string& operand_type);
    llvm::Value* generateBuiltinCall(const std::string& name, 
                                    const std::vector<llvm::Value*>& args);
    
    // Type conversion helpers
    llvm::Value* convertToString(llvm::Value* value, const std::string& from_type);
    llvm::Value* convertToNumber(llvm::Value* value, const std::string& from_type);
    llvm::Value* convertToBoolean(llvm::Value* value, const std::string& from_type);
    
public:
    explicit LLVMCodeGenerator(CodeGenContext& context) : context_(context) {}
    
    // Visitor methods for AST nodes
    void visit(Program* prog) override;
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
};