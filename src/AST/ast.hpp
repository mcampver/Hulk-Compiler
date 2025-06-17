// ast.hpp
#pragma once

#ifndef AST_HPP
#define AST_HPP

#include <cmath>
#include <memory>
#include <string>
#include <vector>

struct Program;
struct NumberExpr;
struct StringExpr;
struct BooleanExpr;
struct UnaryExpr;
struct BinaryExpr;
struct CallExpr;
struct VariableExpr;
struct LetExpr;
struct Stmt;
struct ExprStmt;
struct AssignExpr;
struct FunctionDecl;
struct IfExpr;
struct ExprBlock;
struct WhileExpr;
struct TypeDecl;
struct NewExpr;
struct MemberExpr;
struct SelfExpr;
struct BaseExpr;
struct MemberAssignExpr;
struct MethodCallExpr;

struct ExprVisitor
{
    virtual void visit(Program *prog) = 0;
    virtual void visit(NumberExpr *expr) = 0;
    virtual void visit(StringExpr *expr) = 0;
    virtual void visit(BooleanExpr *) = 0;
    virtual void visit(UnaryExpr *expr) = 0;
    virtual void visit(BinaryExpr *expr) = 0;
    virtual void visit(CallExpr *expr) = 0;
    virtual void visit(VariableExpr *expr) = 0;
    virtual void visit(LetExpr *expr) = 0;
    virtual void visit(AssignExpr *expr) = 0;
    virtual void visit(IfExpr *) = 0;
    virtual void visit(ExprBlock *) = 0;
    virtual void visit(WhileExpr *) = 0;
    virtual void visit(NewExpr *) = 0;
    virtual void visit(MemberExpr *) = 0;
    virtual void visit(SelfExpr *) = 0;
    virtual void visit(BaseExpr *) = 0;
    virtual void visit(MemberAssignExpr *) = 0;
    virtual void visit(MethodCallExpr *) = 0;
    virtual ~ExprVisitor() = default;
};

struct StmtVisitor
{
    virtual void visit(Program *) = 0;
    virtual void visit(ExprStmt *) = 0;
    virtual void visit(FunctionDecl *) = 0;
    virtual void visit(TypeDecl *) = 0;
    virtual ~StmtVisitor() = default;
};

// Base class for all expression nodes
struct Expr
{
    int line_number = 0;  // Line number for error reporting
    int column_number = 0; // Column number for error reporting
    
    Expr(int line = 0, int col = 0) : line_number(line), column_number(col) {}
    virtual void accept(ExprVisitor *v) = 0;
    virtual ~Expr() = default;
};

using ExprPtr = std::unique_ptr<Expr>;

// base class for all statement nodes.
struct Stmt
{
    int line_number = 0;  // Line number for error reporting
    int column_number = 0; // Column number for error reporting
    
    Stmt(int line = 0, int col = 0) : line_number(line), column_number(col) {}
    virtual void accept(StmtVisitor *) = 0;
    virtual ~Stmt() = default;
};

using StmtPtr = std::unique_ptr<Stmt>;

// program: father of all the statements
struct Program : Stmt
{
    std::vector<StmtPtr> stmts;
    void
    accept(StmtVisitor *v) override
    {
        v->visit(this);
    }
};

// evaluates an expression
struct ExprStmt : Stmt
{
    ExprPtr expr;
    ExprStmt(ExprPtr e, int line = 0, int col = 0) : Stmt(line, col), expr(std::move(e)) {}
    void
    accept(StmtVisitor *v) override
    {
        v->visit(this);
    }
};

// Literal: numeric
struct NumberExpr : Expr
{
    double value;
    NumberExpr(double v, int line = 0, int col = 0) : Expr(line, col), value(v) {}
    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Literal: string
struct StringExpr : Expr
{
    std::string value;
    StringExpr(const std::string &s, int line = 0, int col = 0) : Expr(line, col), value(s) {}
    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Literal: bool
struct BooleanExpr : Expr
{
    bool value;
    BooleanExpr(bool v, int line = 0, int col = 0) : Expr(line, col), value(v) {}
    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Unary operation: e.g., negation
struct UnaryExpr : Expr
{    enum Op
    {
        OP_NEG,
        OP_NOT
    } op;
    ExprPtr operand;
    UnaryExpr(Op o, ExprPtr expr, int line = 0, int col = 0) : Expr(line, col), op(o), operand(std::move(expr)) {}
    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Binary operation: +, -, *, /, ^, comparisons, mod
struct BinaryExpr : Expr
{    enum Op
    {
        OP_ADD,
        OP_SUB,
        OP_MUL,
        OP_DIV,
        OP_POW,
        OP_MOD,
        OP_LT,
        OP_GT,
        OP_LE,
        OP_GE,
        OP_EQ,
        OP_NEQ,        OP_OR,
        OP_AND,        OP_CONCAT,
        // Nuevos operadores
        OP_ENHANCED_MOD, // %%
        OP_TRIPLE_PLUS,  // +++
        OP_AND_SIMPLE,   // &
        OP_OR_SIMPLE,    // |
        OP_CONCAT_SPACE  // @@
    } op;
    ExprPtr left;
    ExprPtr right;
    BinaryExpr(Op o, ExprPtr l, ExprPtr r, int line = 0, int col = 0) : Expr(line, col), op(o), left(std::move(l)), right(std::move(r)) {}
    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Function call: sqrt, sin, cos, exp, log, rand
struct CallExpr : Expr
{
    std::string callee;
    std::vector<ExprPtr> args;
    
    CallExpr(const std::string &name, std::vector<ExprPtr> &&arguments, int line = 0, int col = 0)
        : Expr(line, col), callee(name), args(std::move(arguments))
    {
    }
    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// **VariableExpr**: para referirse a un identificador
struct VariableExpr : Expr
{
    std::string name;
    VariableExpr(const std::string &n, int line = 0, int col = 0) : Expr(line, col), name(n) {}
    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// **LetExpr**: let <name> = <init> in <body>
struct LetExpr : Expr
{
    std::string name;    // nombre de la variable
    ExprPtr initializer; // expresión inicializadora
    StmtPtr body;        // cuerpo donde la variable está en alcance
    
    LetExpr(const std::string &n, ExprPtr init, StmtPtr b, int line = 0, int col = 0)
        : Expr(line, col), name(n), initializer(std::move(init)), body(std::move(b))
    {
    }
    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// a := b  destructive assignment
struct AssignExpr : Expr
{
    std::string name;
    ExprPtr value;

    AssignExpr(const std::string &n, ExprPtr v, int line = 0, int col = 0) : Expr(line, col), name(n), value(std::move(v)) {}

    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// for function's declaration
struct FunctionDecl : Stmt
{
    std::string name;
    std::vector<std::string> params;
    StmtPtr body;
    
    FunctionDecl(const std::string &n, std::vector<std::string> &&p, StmtPtr b, int line = 0, int col = 0)
        : Stmt(line, col), name(n), params(std::move(p)), body(std::move(b))
    {
    }

    void
    accept(StmtVisitor *v) override
    {
        v->visit(this);
    }
};

// if-else expressions:
struct IfExpr : Expr
{
    ExprPtr condition;
    ExprPtr thenBranch;
    ExprPtr elseBranch;
    
    IfExpr(ExprPtr cond, ExprPtr thenB, ExprPtr elseB, int line = 0, int col = 0)
        : Expr(line, col), condition(std::move(cond)), thenBranch(std::move(thenB)), elseBranch(std::move(elseB))
    {
    }

    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// bloques de expresiones
struct ExprBlock : Expr
{
    std::vector<StmtPtr> stmts;
    ExprBlock(std::vector<StmtPtr> &&s, int line = 0, int col = 0) : Expr(line, col), stmts(std::move(s)) {}
    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// patra ciclos while
struct WhileExpr : Expr
{
    ExprPtr condition;
    ExprPtr body;

    WhileExpr(ExprPtr cond, ExprPtr b, int line = 0, int col = 0) : Expr(line, col), condition(std::move(cond)), body(std::move(b)) {}

    void
    accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Type declaration
struct TypeDecl : Stmt
{
    std::string name;
    std::vector<std::string> params;
    std::string parentType;
    std::vector<ExprPtr> parentArgs;
    std::vector<std::pair<std::string, Expr*>> attributes;
    std::vector<std::pair<std::string, std::vector<std::string>>> methods;
    std::vector<ExprPtr> methodBodies;

    TypeDecl(const std::string& n, int line = 0, int col = 0) : Stmt(line, col), name(n) {}

    // Helper to add a method with its body
    void addMethod(const std::string& name, const std::vector<std::string>& params, Expr* body) {
        methods.push_back(std::make_pair(name, params));
        methodBodies.push_back(ExprPtr(body));
    }

    void accept(StmtVisitor *v) override
    {
        v->visit(this);
    }
};

// New expression for object instantiation
struct NewExpr : Expr
{
    std::string typeName;
    std::vector<ExprPtr> args;

    NewExpr(const std::string& type, std::vector<ExprPtr>&& arguments, int line = 0, int col = 0) 
        : Expr(line, col), typeName(type), args(std::move(arguments)) {}

    void accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Member access expression (obj.member)
struct MemberExpr : Expr
{
    ExprPtr object;
    std::string member;

    MemberExpr(ExprPtr obj, const std::string& mem, int line = 0, int col = 0) 
        : Expr(line, col), object(std::move(obj)), member(mem) {}

    void accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Self expression
struct SelfExpr : Expr
{
    SelfExpr(int line = 0, int col = 0) : Expr(line, col) {}
    void accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Base expression
struct BaseExpr : Expr
{
    BaseExpr(int line = 0, int col = 0) : Expr(line, col) {}
    void accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Member assignment expression (obj.member := value)
struct MemberAssignExpr : Expr
{
    ExprPtr object;
    std::string member;
    ExprPtr value;

    MemberAssignExpr(ExprPtr obj, const std::string& mem, ExprPtr val, int line = 0, int col = 0) 
        : Expr(line, col), object(std::move(obj)), member(mem), value(std::move(val)) {}

    void accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

// Method call expression (obj.method(args))
struct MethodCallExpr : Expr
{
    ExprPtr object;
    std::string method;
    std::vector<ExprPtr> args;

    MethodCallExpr(ExprPtr obj, const std::string& meth, std::vector<ExprPtr>&& arguments, int line = 0, int col = 0) 
        : Expr(line, col), object(std::move(obj)), method(meth), args(std::move(arguments)) {}

    void accept(ExprVisitor *v) override
    {
        v->visit(this);
    }
};

#endif // AST_HPP