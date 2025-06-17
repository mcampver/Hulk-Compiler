#pragma once
#include "../AST/ast.hpp"
#include "SymbolTable.hpp"
#include "SemanticError.hpp"
#include "TypeInfo.hpp"
#include <vector>
#include <iostream>

/**
 * @brief Semantic analyzer for HULK language
 * 
 * This class performs semantic analysis on the AST, including:
 * - Type checking
 * - Variable declaration and usage validation
 * - Function signature checking
 * - Scope management
 */
class SemanticAnalyzer : public ExprVisitor, public StmtVisitor {
private:
    SymbolTable symbol_table_;
    ErrorManager error_manager_;  // Use ErrorManager instead of vector<SemanticError>
    TypeInfo current_type_;
    
public:SemanticAnalyzer() : current_type_(TypeInfo::Kind::Unknown) {
        // Registrar funciones built-in
        registerBuiltinFunctions();
    }
      /**
     * @brief Analyze a program (entry point)
     */
    void analyze(Program* program) {
        if (!program) {
            error_manager_.reportError(ErrorType::GENERAL_ERROR, 
                "Programa nulo proporcionado para análisis", 0, 0, "", "SemanticAnalyzer");
            return;
        }
        
        // First pass: collect function declarations
        collectFunctions(program);
        
        // Second pass: analyze all expressions
        program->accept(this);
    }
    
    /**
     * @brief Get all semantic errors found
     */
    const std::vector<SemanticError>& getErrors() const {
        return error_manager_.getErrors();
    }
    
    /**
     * @brief Check if there are any errors
     */
    bool hasErrors() const {
        return error_manager_.hasErrors();
    }
    
    /**
     * @brief Print all errors to stderr
     */
    void printErrors() const {
        error_manager_.printErrors();
    }    
    /**
     * @brief Get current inferred type
     */
    const TypeInfo& getCurrentType() const {
        return current_type_;
    }
    
    // Visitor pattern implementation - ExprVisitor
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
    void visit(ExprBlock* expr) override;
    void visit(WhileExpr* expr) override;
    void visit(NewExpr* expr) override;
    void visit(MemberExpr* expr) override;
    void visit(SelfExpr* expr) override;
    void visit(BaseExpr* expr) override;
    void visit(MemberAssignExpr* expr) override;
    void visit(MethodCallExpr* expr) override;
    
    // Visitor pattern implementation - StmtVisitor
    void visit(ExprStmt* stmt) override;
    void visit(FunctionDecl* stmt) override;
    void visit(TypeDecl* stmt) override;

private:
    /**
     * @brief First pass: collect all function and type declarations
     */
    void collectFunctions(Program* program);
    
    /**
     * @brief Register built-in functions in the symbol table
     */
    void registerBuiltinFunctions();
    
    /**
     * @brief Report a semantic error for expressions
     */
    void reportError(ErrorType type, const std::string& message, Expr* expr, 
                    const std::string& context = "");
    
    /**
     * @brief Report a semantic error for statements
     */
    void reportError(ErrorType type, const std::string& message, Stmt* stmt, 
                    const std::string& context = "");
    
    /**
     * @brief Convert binary operator enum to string
     */
    std::string getBinaryOpString(BinaryExpr::Op op);
    
    /**
     * @brief Check if two types are compatible
     */
    bool areTypesCompatible(const TypeInfo& type1, const TypeInfo& type2);
};