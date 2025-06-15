%{
#include <iostream>
#include <cstdlib>
#include <cmath>
#include "../AST/ast.hpp"
#include "../PrintVisitor/print_visitor.hpp"
#include "../Value/value.hpp"
#include "../Evaluator/evaluator.hpp"

using ExprPtr = std::unique_ptr<Expr>;
using ProgramPtr = std::unique_ptr<Program>;

using namespace std;
extern int yylex();
extern int yylineno;
void yyerror(const char* s);

Program* rootAST = nullptr;

// Variables globales temporales para el parser
TypeDecl* currentTypeDecl = nullptr;
%}


%union {
  char* str;
  Expr* expr;
  Stmt* stmt;
  Program* prog;
  std::vector<StmtPtr>* stmts;
  std::vector<ExprPtr>* expr_list;
  std::vector<std::string>* str_list;
  std::pair<std::string, Expr*>* binding;
  std::vector<std::pair<std::string, Expr*>>* bindings;
  TypeDecl* type_decl;
  std::pair<std::string, Expr*>* type_attr;
}

%start input

%type  <stmt> stmt
%type  <expr> expr 
%type  <prog> input
%type  <prog> program
%type <stmts> stmt_list
%type <binding> binding
%type <str_list> ident_list
%type <bindings> binding_list 
%type <expr> if_expr elif_list
%type <expr_list> argument_list
%type <type_decl> type_declaration type_body
%type <type_attr> type_attr


%token LET IN 
%token WHILE FOR
%token <str> IDENT
%token IF ELSE ELIF
%token FUNCTION ARROW
%token ASSIGN ASSIGN_DESTRUCT
%token <expr> TRUE FALSE NUMBER STRING
%token PLUS MINUS MULT DIV MOD POW CONCAT
%token ENHANCED_MOD TRIPLE_PLUS
%token DEBUG TYPE ASSERT NEW SELF INHERITS BASE
%token AND_SIMPLE OR_SIMPLE NOT CONCAT_SPACE
%token LE GE EQ NEQ LESS_THAN GREATER_THAN OR AND
%token LPAREN RPAREN LBRACE RBRACE COMMA SEMICOLON DOT

%left OR OR_SIMPLE
%left AND AND_SIMPLE
%left EQ NEQ
%left LESS_THAN GREATER_THAN LE GE
%left PLUS MINUS TRIPLE_PLUS
%left MULT DIV MOD ENHANCED_MOD
%left CONCAT CONCAT_SPACE
%right POW
%right NOT
%left UMINUS
%left DOT

%%

input:
    program { rootAST = $1; }
  

program:
    /* vacío */      { $$ = new Program(); }
  | program stmt SEMICOLON
    {
      $1->stmts.emplace_back( StmtPtr($2) );
      $$ = $1;
    }
;    

binding_list:
    binding {
          $$ = new std::vector<std::pair<std::string, Expr*>>();
          $$->push_back(*$1);
          delete $1;
      }
    | binding_list COMMA binding {
          $1->push_back(*$3);
          delete $3;
          $$ = $1;
      }
;

binding:
      IDENT ASSIGN expr {
          $$ = new std::pair<std::string, Expr*>(std::string($1), $3);
          free($1);
      }
;

stmt:
    
   expr
    {
      $$ = new ExprStmt( ExprPtr($1) );
    }

  | FUNCTION IDENT LPAREN ident_list RPAREN LBRACE stmt_list RBRACE {
          std::vector<std::string> args = std::move(*$4);
          delete $4;

          auto block = std::make_unique<Program>();
          block->stmts = std::move(*$7);
          delete $7;

          $$ = new FunctionDecl(std::string($2), std::move(args), std::move(block));
          free($2);
      }
  | FUNCTION IDENT LPAREN ident_list RPAREN ARROW expr  {
          std::vector<std::string> args = std::move(*$4);
          delete $4;          $$ = new FunctionDecl(std::string($2), std::move(args), StmtPtr(new ExprStmt(ExprPtr($7))));
          free($2);
      }
  | type_declaration {
          $$ = $1;
      }
;



stmt_list:
      /* vacío */ { $$ = new std::vector<StmtPtr>(); }
    | stmt_list stmt SEMICOLON {
        $1->emplace_back(StmtPtr($2));
        $$ = $1;
    }
;

ident_list:
      /* vacío */ { $$ = new std::vector<std::string>(); }
    | IDENT { $$ = new std::vector<std::string>(); $$->push_back($1); free($1); }
    | ident_list COMMA IDENT { $1->push_back($3); free($3); $$ = $1; }
;

expr:
      NUMBER            { $$ = $1; }
    | STRING            { $$ = $1; }
    | TRUE              { $$ = $1; }
    | FALSE             { $$ = $1; }

    | LBRACE stmt_list RBRACE {
          $$ = new ExprBlock(std::move(*$2));
          delete $2;
      }    | IDENT LPAREN argument_list RPAREN {
          $$ = new CallExpr(std::string($1), std::move(*$3));
          delete $3;
          free($1);
      }

    | DEBUG LPAREN expr RPAREN {
          std::vector<ExprPtr> args;
          args.push_back(ExprPtr($3));
          $$ = new CallExpr("debug", std::move(args));
      }    | TYPE LPAREN expr RPAREN {
          std::vector<ExprPtr> args;
          args.push_back(ExprPtr($3));
          $$ = new CallExpr("type", std::move(args));
      }

    | ASSERT LPAREN expr COMMA expr RPAREN {
          std::vector<ExprPtr> args;
          args.push_back(ExprPtr($3));
          args.push_back(ExprPtr($5));
          $$ = new CallExpr("assert", std::move(args));
      }    | IDENT {
          $$ = new VariableExpr(std::string($1));
          free($1);
      }
    
    | NEW IDENT LPAREN argument_list RPAREN {
          $$ = new NewExpr(std::string($2), std::move(*$4));
          delete $4;
          free($2);
      }
      
    | NEW IDENT LPAREN RPAREN {
          std::vector<ExprPtr> empty_args;
          $$ = new NewExpr(std::string($2), std::move(empty_args));
          free($2);
      }
        | expr DOT IDENT {
          $$ = new MemberExpr(ExprPtr($1), std::string($3));
          free($3);
      }
      
    | expr DOT IDENT LPAREN argument_list RPAREN {
          $$ = new MethodCallExpr(ExprPtr($1), std::string($3), std::move(*$5));
          delete $5;
          free($3);
      }
      
    | expr DOT IDENT LPAREN RPAREN {
          std::vector<ExprPtr> empty_args;
          $$ = new MethodCallExpr(ExprPtr($1), std::string($3), std::move(empty_args));
          free($3);
      }
      
    | SELF {
          $$ = new SelfExpr();
      }
      
    | BASE {
          $$ = new BaseExpr();
      }
      
    | BASE LPAREN RPAREN {
          $$ = new BaseExpr();
      }| MINUS expr %prec UMINUS {
          $$ = new UnaryExpr(UnaryExpr::OP_NEG, ExprPtr($2));
      }

    | NOT expr %prec NOT {
          $$ = new UnaryExpr(UnaryExpr::OP_NOT, ExprPtr($2));
      }

    | expr POW expr {
          $$ = new BinaryExpr(BinaryExpr::OP_POW, ExprPtr($1), ExprPtr($3));
      }

    | expr MULT expr {
          $$ = new BinaryExpr(BinaryExpr::OP_MUL, ExprPtr($1), ExprPtr($3));
      }

    | expr DIV expr {
          $$ = new BinaryExpr(BinaryExpr::OP_DIV, ExprPtr($1), ExprPtr($3));
      }    | expr MOD expr %prec MULT {
          $$ = new BinaryExpr(BinaryExpr::OP_MOD, ExprPtr($1), ExprPtr($3));
      }

    | expr ENHANCED_MOD expr %prec MULT {
          $$ = new BinaryExpr(BinaryExpr::OP_ENHANCED_MOD, ExprPtr($1), ExprPtr($3));
      }

    | expr TRIPLE_PLUS expr %prec PLUS {
          $$ = new BinaryExpr(BinaryExpr::OP_TRIPLE_PLUS, ExprPtr($1), ExprPtr($3));
      }


    | expr PLUS expr {
          $$ = new BinaryExpr(BinaryExpr::OP_ADD, ExprPtr($1), ExprPtr($3));
      }

    | expr MINUS expr {
          $$ = new BinaryExpr(BinaryExpr::OP_SUB, ExprPtr($1), ExprPtr($3));
      }

    | expr LESS_THAN expr {
          $$ = new BinaryExpr(BinaryExpr::OP_LT, ExprPtr($1), ExprPtr($3));
      }

    | expr GREATER_THAN expr {
          $$ = new BinaryExpr(BinaryExpr::OP_GT, ExprPtr($1), ExprPtr($3));
      }

    | expr LE expr {
          $$ = new BinaryExpr(BinaryExpr::OP_LE, ExprPtr($1), ExprPtr($3));
      }

    | expr GE expr {
          $$ = new BinaryExpr(BinaryExpr::OP_GE, ExprPtr($1), ExprPtr($3));
      }

    | expr EQ expr {
          $$ = new BinaryExpr(BinaryExpr::OP_EQ, ExprPtr($1), ExprPtr($3));
      }

    | expr NEQ expr {
          $$ = new BinaryExpr(BinaryExpr::OP_NEQ, ExprPtr($1), ExprPtr($3));
      }    | expr OR expr {
          $$ = new BinaryExpr(BinaryExpr::OP_OR, ExprPtr($1), ExprPtr($3));
      }

    | expr OR_SIMPLE expr {
          $$ = new BinaryExpr(BinaryExpr::OP_OR_SIMPLE, ExprPtr($1), ExprPtr($3));
      }

    | expr AND expr {
          $$ = new BinaryExpr(BinaryExpr::OP_AND, ExprPtr($1), ExprPtr($3));
      }

    | expr AND_SIMPLE expr {
          $$ = new BinaryExpr(BinaryExpr::OP_AND_SIMPLE, ExprPtr($1), ExprPtr($3));
      }    | expr CONCAT expr {
        // Creamos un BinaryExpr con OP_CONCAT
        $$ = new BinaryExpr(BinaryExpr::OP_CONCAT, ExprPtr($1), ExprPtr($3));
    }  

    | expr CONCAT_SPACE expr {
        // Creamos un BinaryExpr con OP_CONCAT_SPACE
        $$ = new BinaryExpr(BinaryExpr::OP_CONCAT_SPACE, ExprPtr($1), ExprPtr($3));
    }

    | LPAREN expr RPAREN {
          $$ = $2;
      }

    | LET binding_list IN expr {
          Expr* result = $4;
          auto& list = *$2;

          for (auto it = list.rbegin(); it != list.rend(); ++it) {
              result = new LetExpr(it->first, ExprPtr(it->second), std::make_unique<ExprStmt>(ExprPtr(result)));
          }

          delete $2;
          $$ = result;      
        }    | IDENT ASSIGN_DESTRUCT expr {
          $$ = new AssignExpr(std::string($1), ExprPtr($3));
          free($1);
      }
      
    | expr DOT IDENT ASSIGN_DESTRUCT expr {
          $$ = new MemberAssignExpr(ExprPtr($1), std::string($3), ExprPtr($5));
          free($3);
      }
    | WHILE LPAREN expr RPAREN expr {
      $$ = new WhileExpr(ExprPtr($3), ExprPtr($5));
    }

    | if_expr  
    | FOR LPAREN IDENT IN expr RPAREN expr {
        auto argsNext = std::vector<ExprPtr>();
        argsNext.push_back(std::make_unique<VariableExpr>("__iter"));
        ExprPtr callNext = std::make_unique<CallExpr>("next", std::move(argsNext));

        auto argsCurrent = std::vector<ExprPtr>();
        argsCurrent.push_back(std::make_unique<VariableExpr>("__iter"));
        ExprPtr callCurrent = std::make_unique<CallExpr>("current", std::move(argsCurrent));

        ExprPtr bodyFor = ExprPtr($7);

        Expr* innerLetRaw = new LetExpr(
            std::string($3),                      // nombre de la variable (x)
            std::move(callCurrent),               // initializer = current(__iter)
            StmtPtr(new ExprStmt(std::move(bodyFor))) // cuerpo = ExprStmt(bodyFor)
        );

        Expr* whileRaw = new WhileExpr(
            std::move(callNext),
            ExprPtr(innerLetRaw)
        );

        auto argsIter = std::vector<ExprPtr>();
        argsIter.push_back( ExprPtr($5) );
        ExprPtr callIter = std::make_unique<CallExpr>("iter", std::move(argsIter));

        Expr* outerLetRaw = new LetExpr(
            "__iter",
            std::move(callIter),
            StmtPtr(new ExprStmt( ExprPtr(whileRaw) ))
        );

        free($3);  // liberar el IDENT
        $$ = outerLetRaw;
    }

;

if_expr:
    IF LPAREN expr RPAREN expr elif_list {
        $$ = new IfExpr(ExprPtr($3), ExprPtr($5), ExprPtr($6));
    }
;

elif_list:
    ELSE expr {
        $$ = $2;
    }
    | ELIF LPAREN expr RPAREN expr elif_list {
        $$ = new IfExpr(ExprPtr($3), ExprPtr($5), ExprPtr($6));
    }
;

argument_list:
      /* vacío */ { $$ = new std::vector<ExprPtr>(); }
    | expr {
          $$ = new std::vector<ExprPtr>();
          $$->emplace_back(ExprPtr($1));
      }
    | argument_list COMMA expr {
          $1->emplace_back(ExprPtr($3));
          $$ = $1;
      }
; 

// Declaración de tipos
type_declaration:
    TYPE IDENT LBRACE type_body RBRACE {
        $$ = $4; // $4 es el TypeDecl construido por type_body
        $$->name = std::string($2);
        free($2);
    }
    | TYPE IDENT LPAREN ident_list RPAREN LBRACE type_body RBRACE {
        $$ = $7; // $7 es el TypeDecl construido por type_body
        $$->name = std::string($2);
        $$->params = std::move(*$4);
        delete $4;
        free($2);
    }
    | TYPE IDENT INHERITS IDENT LBRACE type_body RBRACE {
        $$ = $6; // $6 es el TypeDecl construido por type_body
        $$->name = std::string($2);
        $$->parentType = std::string($4);
        free($2);
        free($4);
    }
    | TYPE IDENT LPAREN ident_list RPAREN INHERITS IDENT LBRACE type_body RBRACE {
        $$ = $9; // $9 es el TypeDecl construido por type_body
        $$->name = std::string($2);
        $$->params = std::move(*$4);
        $$->parentType = std::string($7);
        delete $4;
        free($2);
        free($7);
    }
    | TYPE IDENT INHERITS IDENT LPAREN argument_list RPAREN LBRACE type_body RBRACE {
        $$ = $9; // $9 es el TypeDecl construido por type_body
        $$->name = std::string($2);
        $$->parentType = std::string($4);
        $$->parentArgs = std::move(*$6);
        delete $6;
        free($2);
        free($4);
    }
    | TYPE IDENT LPAREN ident_list RPAREN INHERITS IDENT LPAREN argument_list RPAREN LBRACE type_body RBRACE {
        $$ = $12; // $12 es el TypeDecl construido por type_body
        $$->name = std::string($2);
        $$->params = std::move(*$4);
        $$->parentType = std::string($7);
        $$->parentArgs = std::move(*$9);
        delete $4;
        delete $9;
        free($2);
        free($7);
    }
;

// Cuerpo del tipo - construye gradualmente el TypeDecl
type_body:
    /* vacío */ { 
        $$ = new TypeDecl("");
    }    | type_body type_attr { 
        $1->attributes.push_back(*$2);
        delete $2;
        $$ = $1;
    }
    | type_body IDENT LPAREN ident_list RPAREN ARROW expr SEMICOLON {
        // Agregar método con parámetros
        $1->addMethod(std::string($2), *$4, $7);
        delete $4;
        free($2);
        $$ = $1;
    }
    | type_body IDENT LPAREN RPAREN ARROW expr SEMICOLON {
        // Agregar método sin parámetros
        std::vector<std::string> emptyParams;
        $1->addMethod(std::string($2), emptyParams, $6);
        free($2);
        $$ = $1;
    }    | type_body IDENT LPAREN ident_list RPAREN LBRACE stmt_list RBRACE {
        // Agregar método con parámetros y bloque de código
        std::vector<StmtPtr> stmts = std::move(*$7);
        ExprBlock* block = new ExprBlock(std::move(stmts));
        $1->addMethod(std::string($2), *$4, block);
        delete $4;
        delete $7;
        free($2);
        $$ = $1;
    }
    | type_body IDENT LPAREN RPAREN LBRACE stmt_list RBRACE {
        // Agregar método sin parámetros y bloque de código
        std::vector<std::string> emptyParams;
        std::vector<StmtPtr> stmts = std::move(*$6);
        ExprBlock* block = new ExprBlock(std::move(stmts));
        $1->addMethod(std::string($2), emptyParams, block);
        delete $6;
        free($2);
        $$ = $1;
    }
;

// Atributo del tipo
type_attr:
    IDENT ASSIGN expr SEMICOLON { 
        $$ = new std::pair<std::string, Expr*>(std::string($1), $3);
        free($1);
    }
;

%%

void yyerror(const char* s) {
    std::cerr << "Parse error: " << s << " at line " << yylineno << std::endl;
}