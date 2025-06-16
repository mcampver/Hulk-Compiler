#pragma once
#include <memory>
#include <stdexcept>
#include "scope.hpp"   // tu Scope<SymbolInfo> :contentReference[oaicite:1]{index=1}
#include "AST/ast.hpp" // nodos y visitor interfaces :contentReference[oaicite:2]{index=2}

class NameResolver : public StmtVisitor, public ExprVisitor
{
    using SymScope = Scope<SymbolInfo>;
    SymScope::Ptr currentScope_;

public:    NameResolver()
        : currentScope_(std::make_shared<SymScope>(nullptr)) // scope global
    {        // Pre-declarar las funciones nativas:
        for (auto &fn : {
                 "print", "sqrt", "log", "sin", "cos", "pow",
                 "rand", "range", "iter", "next", "current",
                 "PI", "E", "function", "if", "else",
                 "debug", "type", "assert", "str"})
        {
            currentScope_->declare(fn, SymbolInfo{SymbolInfo::FUNCTION});
        }
    }

    // ---------------- StmtVisitor ----------------
    void visit(Program *p) override
    {
        for (auto &s : p->stmts)
            s->accept(this);
    }

    void visit(ExprStmt *s) override
    {
        // Una expresión suelta → resolver dentro de ella
        s->expr->accept(this);
    }

    void visit(FunctionDecl *f) override
    {
        // 1) Declarar la función
        if (currentScope_->existsInCurrent(f->name))
            throw std::runtime_error("Redeclaración de función: " + f->name);
        currentScope_->declare(f->name, {SymbolInfo::FUNCTION});
        // 2) Nuevo scope para parámetros + cuerpo
        auto parent = currentScope_;
        currentScope_ = std::make_shared<SymScope>(parent);
        for (auto &param : f->params)
        {
            if (currentScope_->existsInCurrent(param))
                throw std::runtime_error("Redeclaración de parámetro: " + param);
            currentScope_->declare(param, {SymbolInfo::VARIABLE});
        }
        f->body->accept(this);
        // 3) Cerrar scope
        currentScope_ = parent;
    }

    // ---------------- ExprVisitor ----------------
    // 1) Literales no necesitan resolución
    void visit(NumberExpr *) override {}
    void visit(StringExpr *) override {}
    void visit(BooleanExpr *) override {}

    // 2) Operadores
    void visit(UnaryExpr *expr) override
    {
        expr->operand->accept(this);
    }
    void visit(BinaryExpr *expr) override
    {
        expr->left->accept(this);
        expr->right->accept(this);
    }

    // 3) Llamadas y variables
    void visit(CallExpr *expr) override
    {
        // Verifica que la función exista en el scope
        currentScope_->lookup(expr->callee);
        for (auto &arg : expr->args)
            arg->accept(this);
    }
    void visit(VariableExpr *expr) override
    {
        currentScope_->lookup(expr->name);
    }

    // 4) Let / Assign
    void visit(LetExpr *expr) override
    {
        expr->initializer->accept(this);
        auto parent = currentScope_;
        currentScope_ = std::make_shared<SymScope>(parent);
        if (currentScope_->existsInCurrent(expr->name))
            throw std::runtime_error("Redeclaración de variable: " + expr->name);
        currentScope_->declare(expr->name, {SymbolInfo::VARIABLE});
        expr->body->accept(this);
        currentScope_ = parent;
    }
    void visit(AssignExpr *expr) override
    {
        // Verificar variable ya declarada
        currentScope_->lookup(expr->name);
        expr->value->accept(this);
    }

    // 5) Control de flujo
    void visit(IfExpr *expr) override
    {
        expr->condition->accept(this);
        expr->thenBranch->accept(this);
        if (expr->elseBranch)
            expr->elseBranch->accept(this);
    }
    void visit(ExprBlock *expr) override
    {
        auto parent = currentScope_;
        currentScope_ = std::make_shared<SymScope>(parent);
        for (auto &stmt : expr->stmts)
            stmt->accept(this);
        currentScope_ = parent;
    }    void visit(WhileExpr *expr) override
    {
        expr->condition->accept(this);
        auto parent = currentScope_;
        currentScope_ = std::make_shared<SymScope>(parent);
        expr->body->accept(this);
        currentScope_ = parent;
    }

    // Nuevos métodos para soportar tipos
    void visit(TypeDecl *) override
    {
        // Implementación básica para declaraciones de tipo
    }

    void visit(NewExpr *) override
    {
        // Implementación básica para expresiones new
    }

    void visit(MemberExpr *) override
    {
        // Implementación básica para acceso a miembros
    }

    void visit(SelfExpr *) override
    {
        // Implementación básica para self
    }    void visit(BaseExpr *) override
    {
        // Implementación básica para base
    }    void visit(MemberAssignExpr *) override
    {
        // Implementación básica para asignación a miembros
    }

    void visit(MethodCallExpr *) override
    {
        // Implementación básica para llamadas a métodos
    }
};
