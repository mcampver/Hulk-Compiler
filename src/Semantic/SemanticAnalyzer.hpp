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
    std::vector<SemanticError> errors_;
    TypeInfo current_type_;
    
public:
    SemanticAnalyzer() : current_type_(TypeInfo::Kind::Unknown) {}
    
    /**
     * @brief Analyze a program (entry point)
     */
    void analyze(Program* program) {
        if (!program) {
            reportError("Null program provided for analysis", 0);
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
        return errors_;
    }
    
    /**
     * @brief Check if there are any errors
     */
    bool hasErrors() const {
        return !errors_.empty();
    }
    
    /**
     * @brief Print all errors to stderr
     */
    void printErrors() const {
        for (const auto& error : errors_) {
            std::cerr << error.format() << std::endl;
        }
    }
    
    /**
     * @brief Get current inferred type
     */
    const TypeInfo& getCurrentType() const {
        return current_type_;
    }
    
    // Visitor pattern implementation
    void visit(Program* prog) override {
        for (auto& stmt : prog->stmts) {
            stmt->accept(this);
        }
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
    }
    
    void visit(NumberExpr* expr) override {
        current_type_ = TypeInfo(TypeInfo::Kind::Number);
    }
    
    void visit(StringExpr* expr) override {
        current_type_ = TypeInfo(TypeInfo::Kind::String);
    }
    
    void visit(BooleanExpr* expr) override {
        current_type_ = TypeInfo(TypeInfo::Kind::Boolean);
    }
      void visit(UnaryExpr* expr) override {
        expr->operand->accept(this);
        TypeInfo operand_type = current_type_;
        
        std::string op;
        switch (expr->op) {
            case UnaryExpr::Op::OP_NEG: op = "-"; break;
            case UnaryExpr::Op::OP_NOT: op = "!"; break;
            default:
                reportError("Unknown unary operator", 0);
                current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
                return;
        }
        
        current_type_ = TypeInfo::inferUnaryOp(op, operand_type);
        
        if (current_type_.isUnknown()) {
            reportError("Invalid unary operation: " + op + " applied to " + operand_type.toString(), 0);
        }
    }
    
    void visit(BinaryExpr* expr) override {
        expr->left->accept(this);
        TypeInfo left_type = current_type_;
        
        expr->right->accept(this);
        TypeInfo right_type = current_type_;
        
        std::string op = binaryOpToString(expr->op);
        current_type_ = TypeInfo::inferBinaryOp(op, left_type, right_type);
        
        if (current_type_.isUnknown()) {
            reportError("Invalid binary operation: " + left_type.toString() + " " + op + " " + right_type.toString(), 0);
        }
        
        // Special validation for your enhanced operators
        validateEnhancedOperators(expr->op, left_type, right_type);
    }
      void visit(CallExpr* expr) override {
        // Check if function exists
        auto func_symbol = symbol_table_.lookupFunction(expr->callee);
        if (!func_symbol) {
            reportError("Undefined function '" + expr->callee + "'", 0);
            current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
            return;
        }
        
        // Check argument count
        if (expr->args.size() != func_symbol->parameter_types.size()) {
            reportError("Function '" + expr->callee + "' expects " + 
                       std::to_string(func_symbol->parameter_types.size()) + 
                       " arguments, got " + std::to_string(expr->args.size()), 0);
        }
        
        // Check argument types
        for (size_t i = 0; i < expr->args.size() && i < func_symbol->parameter_types.size(); ++i) {
            expr->args[i]->accept(this);
            if (!current_type_.isCompatibleWith(func_symbol->parameter_types[i])) {
                reportError("Argument " + std::to_string(i + 1) + " of function '" + expr->callee +
                           "' expects " + func_symbol->parameter_types[i].toString() + 
                           ", got " + current_type_.toString(), 0);
            }
        }

        current_type_ = func_symbol->return_type;
    }
    
    void visit(VariableExpr* expr) override {
        auto symbol = symbol_table_.lookupVariable(expr->name);
        if (!symbol) {
            reportError("Undefined variable '" + expr->name + "'", 0);
            current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
            return;
        }
        
        current_type_ = symbol->type;
    }    void visit(LetExpr* expr) override {
        symbol_table_.pushScope();
        
        // Process variable declaration
        expr->initializer->accept(this);
        TypeInfo init_type = current_type_;
        
        if (!symbol_table_.declareVariable(expr->name, init_type, true, 0)) {
            reportError("Variable '" + expr->name + "' is already declared in this scope", 0);
        }
        
        // Process body
        expr->body->accept(this);
        
        symbol_table_.popScope();
    }
    
    void visit(AssignExpr* expr) override {
        auto symbol = symbol_table_.lookupVariable(expr->name);
        if (!symbol) {
            reportError("Undefined variable '" + expr->name + "'", 0);
            current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
            return;
        }
        
        if (!symbol->is_mutable) {
            reportError("Cannot assign to immutable variable '" + expr->name + "'", 0);
        }
        
        expr->value->accept(this);
        if (!current_type_.isCompatibleWith(symbol->type)) {
            reportError("Cannot assign " + current_type_.toString() + 
                       " to variable '" + expr->name + "' of type " + symbol->type.toString(), 0);
        }
    }
      void visit(IfExpr* expr) override {
        expr->condition->accept(this);
        if (!current_type_.isBoolean()) {
            reportError("If condition must be boolean, got " + current_type_.toString(), 0);
        }
        
        expr->thenBranch->accept(this);
        TypeInfo then_type = current_type_;
        
        if (expr->elseBranch) {
            expr->elseBranch->accept(this);
            if (!then_type.isCompatibleWith(current_type_)) {
                reportError("If-else branches have incompatible types: " + 
                           then_type.toString() + " and " + current_type_.toString(), 0);
            }
        }
    }
      void visit(ExprBlock* block) override {
        TypeInfo last_type(TypeInfo::Kind::Unknown);
        for (auto& stmt : block->stmts) {
            stmt->accept(this);
            // Note: stmt->accept calls StmtVisitor, not ExprVisitor
            // So we can't directly get the type from statements
        }
        
        current_type_ = last_type;
    }
      void visit(WhileExpr* expr) override {
        expr->condition->accept(this);
        if (!current_type_.isBoolean()) {
            reportError("While condition must be boolean, got " + current_type_.toString(), 0);
        }
        
        expr->body->accept(this);
        // While expressions typically don't have a meaningful return type
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
    }
    
    // StmtVisitor methods
    void visit(ExprStmt* stmt) override {
        stmt->expr->accept(this);
    }
      void visit(FunctionDecl* func) override {
        // Function declarations are handled in collectFunctions
        // This visit is called during the second pass for validation
        symbol_table_.pushScope();
        
        // Add parameters to current scope
        for (const auto& param : func->params) {
            symbol_table_.declareVariable(param, TypeInfo(TypeInfo::Kind::Unknown), false, 0);
        }
        
        // Visit function body
        if (func->body) {
            func->body->accept(this);
        }
        
        symbol_table_.popScope();
    }
    
private:
    /**
     * @brief First pass: collect all function declarations
     */
    void collectFunctions(Program* program) {
        for (auto& stmt : program->stmts) {
            if (auto func_decl = dynamic_cast<FunctionDecl*>(stmt.get())) {
                std::vector<TypeInfo> param_types;
                for (const auto& param : func_decl->params) {
                    param_types.push_back(TypeInfo(TypeInfo::Kind::Unknown)); // For now, assume unknown types
                }
                
                if (!symbol_table_.declareFunction(func_decl->name, param_types, 
                                                  TypeInfo(TypeInfo::Kind::Unknown), 0)) {
                    reportError("Function '" + func_decl->name + "' is already declared", 0);
                }
            }
        }
    }
    
    /**
     * @brief Report a semantic error
     */
    void reportError(const std::string& message, int line, int column = 0) {
        errors_.emplace_back(message, line, column);
    }
    
    /**
     * @brief Convert binary operator enum to string
     */
    std::string binaryOpToString(BinaryExpr::Op op) {
        switch (op) {
            case BinaryExpr::Op::OP_ADD: return "+";
            case BinaryExpr::Op::OP_SUB: return "-";
            case BinaryExpr::Op::OP_MUL: return "*";
            case BinaryExpr::Op::OP_DIV: return "/";
            case BinaryExpr::Op::OP_MOD: return "%";
            case BinaryExpr::Op::OP_POW: return "^";
            case BinaryExpr::Op::OP_EQ: return "==";
            case BinaryExpr::Op::OP_NEQ: return "!=";            case BinaryExpr::Op::OP_LT: return "<";
            case BinaryExpr::Op::OP_GT: return ">";
            case BinaryExpr::Op::OP_LE: return "<=";
            case BinaryExpr::Op::OP_GE: return ">=";
            case BinaryExpr::Op::OP_AND: return "&&";            case BinaryExpr::Op::OP_OR: return "||";
            // Your enhanced operators
            case BinaryExpr::Op::OP_ENHANCED_MOD: return "%%";
            case BinaryExpr::Op::OP_TRIPLE_PLUS: return "+++";
            case BinaryExpr::Op::OP_AND_SIMPLE: return "&";
            case BinaryExpr::Op::OP_OR_SIMPLE: return "|";
            case BinaryExpr::Op::OP_CONCAT: return "@";
            case BinaryExpr::Op::OP_CONCAT_SPACE: return "@@";
            default: return "unknown";
        }
    }
    
    /**
     * @brief Validate your enhanced operators
     */    void validateEnhancedOperators(BinaryExpr::Op op, const TypeInfo& left, const TypeInfo& right) {
        switch (op) {
            case BinaryExpr::Op::OP_ENHANCED_MOD:
                if (!left.isNumeric() || !right.isNumeric()) {
                    reportError("Enhanced modulo requires numeric operands", 0);
                }
                break;
                
            case BinaryExpr::Op::OP_TRIPLE_PLUS:
                if (!left.isNumeric() && !left.isString()) {
                    reportError("Triple plus requires numeric or string operands", 0);
                }
                break;
                
            case BinaryExpr::Op::OP_CONCAT:
            case BinaryExpr::Op::OP_CONCAT_SPACE:
                if (!left.isString() && !right.isString()) {
                    reportError("Concatenation operators require at least one string operand", 0);
                }
                break;
                
            case BinaryExpr::Op::OP_AND_SIMPLE:
            case BinaryExpr::Op::OP_OR_SIMPLE:
                // These can work with any types, similar to && and ||
                break;
                
            default:                break;
        }
    }

    void visit(TypeDecl* decl) override {
        // Add type to symbol table
        // For now, just report that we found a type declaration
        std::cout << "Semantic analysis: Type declaration " << decl->name << std::endl;
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
    }

    void visit(NewExpr* expr) override {
        // Check if type exists and evaluate arguments
        for (auto& arg : expr->args) {
            arg->accept(this);
        }
        // For now, assume object creation succeeds
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown); // Should be the type being created
    }

    void visit(MemberExpr* expr) override {
        expr->object->accept(this);
        // Check if member exists for the object type
        // For now, assume member access succeeds
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
    }

    void visit(SelfExpr* expr) override {
        // Should check if we're inside a type method
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown); // Should be the current type
    }    void visit(BaseExpr* expr) override {
        // Should check if we're inside an inherited type method
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown); // Should be the parent type
    }    void visit(MemberAssignExpr* expr) override {
        expr->object->accept(this);
        expr->value->accept(this);
        // Check if assignment is valid
        current_type_ = current_type_; // Return the assigned value type
    }

    void visit(MethodCallExpr* expr) override {
        // Evaluate object
        expr->object->accept(this);
        
        // Evaluate arguments
        for (auto& arg : expr->args) {
            arg->accept(this);
        }
        
        // For now, assume method call succeeds
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown); // Should be the method return type
    }
};