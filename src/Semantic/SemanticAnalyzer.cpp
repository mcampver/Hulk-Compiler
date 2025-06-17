#include "SemanticAnalyzer.hpp"
#include <algorithm>
#include <sstream>
#include <set>

void SemanticAnalyzer::registerBuiltinFunctions() {
    // Register built-in functions with their signatures    // Math functions
    symbol_table_.declareFunction("sqrt", {"x"});
    symbol_table_.declareFunction("sin", {"x"});
    symbol_table_.declareFunction("cos", {"x"});
    symbol_table_.declareFunction("exp", {"x"});
    symbol_table_.declareFunction("log", {"x"});
    symbol_table_.declareFunction("pow", {"base", "exponent"});  // Agregar pow
    symbol_table_.declareFunction("rand", {});
    symbol_table_.declareFunction("floor", {"x"});
    symbol_table_.declareFunction("ceil", {"x"});
    
    // I/O functions
    symbol_table_.declareFunction("print", {"x"});
    symbol_table_.declareFunction("println", {"x"});
    
    // String functions
    symbol_table_.declareFunction("parse", {"s"});
    symbol_table_.declareFunction("str", {"x"});  // Agregar str para conversión
}

void SemanticAnalyzer::collectFunctions(Program* program) {
    for (auto& stmt : program->stmts) {
        if (auto funcDecl = dynamic_cast<FunctionDecl*>(stmt.get())) {
            if (symbol_table_.isFunctionDeclared(funcDecl->name)) {
                error_manager_.reportError(ErrorType::REDEFINED_FUNCTION,
                    "Función '" + funcDecl->name + "' ya está definida",
                    funcDecl->line_number, funcDecl->column_number, 
                    "declaración de función", "SemanticAnalyzer");
            } else {
                symbol_table_.declareFunction(funcDecl->name, funcDecl->params);
            }
        } else if (auto typeDecl = dynamic_cast<TypeDecl*>(stmt.get())) {
            if (symbol_table_.isTypeDeclared(typeDecl->name)) {
                error_manager_.reportError(ErrorType::REDEFINED_TYPE,
                    "Tipo '" + typeDecl->name + "' ya está definido",
                    typeDecl->line_number, typeDecl->column_number,
                    "declaración de tipo", "SemanticAnalyzer");
            } else {
                symbol_table_.declareType(typeDecl->name);
            }
        }
    }
}

void SemanticAnalyzer::reportError(ErrorType type, const std::string& message, 
                                  Expr* expr, const std::string& context) {
    int line = expr ? expr->line_number : 0;
    int col = expr ? expr->column_number : 0;
    error_manager_.reportError(type, message, line, col, context, "SemanticAnalyzer");
}

void SemanticAnalyzer::reportError(ErrorType type, const std::string& message, 
                                  Stmt* stmt, const std::string& context) {
    int line = stmt ? stmt->line_number : 0;
    int col = stmt ? stmt->column_number : 0;
    error_manager_.reportError(type, message, line, col, context, "SemanticAnalyzer");
}

void SemanticAnalyzer::visit(Program* prog) {
    for (auto& stmt : prog->stmts) {
        stmt->accept(this);
    }
    current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
}

void SemanticAnalyzer::visit(NumberExpr* expr) {
    current_type_ = TypeInfo(TypeInfo::Kind::Number);
}

void SemanticAnalyzer::visit(StringExpr* expr) {
    current_type_ = TypeInfo(TypeInfo::Kind::String);
}

void SemanticAnalyzer::visit(BooleanExpr* expr) {
    current_type_ = TypeInfo(TypeInfo::Kind::Boolean);
}

void SemanticAnalyzer::visit(UnaryExpr* expr) {
    expr->operand->accept(this);
    TypeInfo operand_type = current_type_;
    
    std::string op_str;
    switch (expr->op) {
        case UnaryExpr::OP_NEG:
            op_str = "negación (-)";
            if (operand_type.getKind() != TypeInfo::Kind::Number) {
                reportError(ErrorType::TYPE_MISMATCH,
                    "El operador de negación requiere un operando numérico, se encontró " + 
                    operand_type.toString(), expr, "expresión unaria");
                current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
                return;
            }
            current_type_ = TypeInfo(TypeInfo::Kind::Number);
            break;
            
        case UnaryExpr::OP_NOT:
            op_str = "negación lógica (not)";
            if (operand_type.getKind() != TypeInfo::Kind::Boolean) {
                reportError(ErrorType::TYPE_MISMATCH,
                    "El operador 'not' requiere un operando booleano, se encontró " + 
                    operand_type.toString(), expr, "expresión unaria");
                current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
                return;
            }
            current_type_ = TypeInfo(TypeInfo::Kind::Boolean);
            break;
            
        default:
            reportError(ErrorType::INVALID_OPERATION,
                "Operador unario desconocido", expr, "expresión unaria");
            current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
            break;
    }
}

void SemanticAnalyzer::visit(BinaryExpr* expr) {
    expr->left->accept(this);
    TypeInfo left_type = current_type_;
    
    expr->right->accept(this);
    TypeInfo right_type = current_type_;
    
    std::string op_str = getBinaryOpString(expr->op);
    
    switch (expr->op) {
        case BinaryExpr::OP_ADD:
        case BinaryExpr::OP_SUB:
        case BinaryExpr::OP_MUL:
        case BinaryExpr::OP_DIV:
        case BinaryExpr::OP_POW:
        case BinaryExpr::OP_MOD:        case BinaryExpr::OP_ENHANCED_MOD:
            // Allow operations with unknown types (assume they're numbers in mathematical context)
            if ((left_type.getKind() != TypeInfo::Kind::Number && left_type.getKind() != TypeInfo::Kind::Unknown) ||
                (right_type.getKind() != TypeInfo::Kind::Number && right_type.getKind() != TypeInfo::Kind::Unknown)) {
                reportError(ErrorType::TYPE_MISMATCH,
                    "El operador '" + op_str + "' requiere operandos numéricos, se encontró " +
                    left_type.toString() + " y " + right_type.toString(), 
                    expr, "expresión binaria aritmética");
                current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
                return;
            }
            current_type_ = TypeInfo(TypeInfo::Kind::Number);
            break;
              case BinaryExpr::OP_CONCAT:        case BinaryExpr::OP_CONCAT_SPACE:
            // En HULK, la concatenación puede manejar string + cualquier tipo (conversión automática)
            if (left_type.getKind() != TypeInfo::Kind::String && right_type.getKind() != TypeInfo::Kind::String) {
                reportError(ErrorType::TYPE_MISMATCH,
                    "El operador de concatenación '" + op_str + "' requiere al menos un operando de cadena, se encontró " +
                    left_type.toString() + " y " + right_type.toString(),
                    expr, "expresión binaria concatenación");
                current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
                return;
            }
            current_type_ = TypeInfo(TypeInfo::Kind::String);
            break;
            
        case BinaryExpr::OP_LT:
        case BinaryExpr::OP_GT:
        case BinaryExpr::OP_LE:        case BinaryExpr::OP_GE:
            if (left_type.getKind() != TypeInfo::Kind::Number || right_type.getKind() != TypeInfo::Kind::Number) {
                reportError(ErrorType::TYPE_MISMATCH,
                    "Los operadores de comparación numérica requieren operandos numéricos, se encontró " +
                    left_type.toString() + " y " + right_type.toString(),
                    expr, "expresión binaria comparación");
                current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
                return;
            }
            current_type_ = TypeInfo(TypeInfo::Kind::Boolean);
            break;
            
        case BinaryExpr::OP_EQ:
        case BinaryExpr::OP_NEQ:
            // Equality can compare any types, but they should be the same
            if (!areTypesCompatible(left_type, right_type)) {
                reportError(ErrorType::TYPE_MISMATCH,
                    "Los operadores de igualdad requieren tipos compatibles, se encontró " +
                    left_type.toString() + " y " + right_type.toString(),
                    expr, "expresión binaria igualdad");
            }
            current_type_ = TypeInfo(TypeInfo::Kind::Boolean);
            break;
            
        case BinaryExpr::OP_AND:
        case BinaryExpr::OP_OR:
        case BinaryExpr::OP_AND_SIMPLE:        case BinaryExpr::OP_OR_SIMPLE:
            if (left_type.getKind() != TypeInfo::Kind::Boolean || right_type.getKind() != TypeInfo::Kind::Boolean) {
                reportError(ErrorType::TYPE_MISMATCH,
                    "Los operadores lógicos requieren operandos booleanos, se encontró " +
                    left_type.toString() + " y " + right_type.toString(),
                    expr, "expresión binaria lógica");
                current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
                return;
            }
            current_type_ = TypeInfo(TypeInfo::Kind::Boolean);
            break;
            
        default:
            reportError(ErrorType::INVALID_OPERATION,
                "Operador binario desconocido", expr, "expresión binaria");
            current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
            break;
    }
}

void SemanticAnalyzer::visit(CallExpr* expr) {
    if (!symbol_table_.isFunctionDeclared(expr->callee)) {
        reportError(ErrorType::UNDEFINED_FUNCTION,
            "Función '" + expr->callee + "' no está definida",
            expr, "llamada a función");
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
        return;
    }
    
    auto func_params = symbol_table_.getFunctionParams(expr->callee);
    if (expr->args.size() != func_params.size()) {
        std::ostringstream oss;
        oss << "La función '" << expr->callee << "' espera " << func_params.size() 
            << " argumentos, pero se proporcionaron " << expr->args.size();
        reportError(ErrorType::INVALID_ARGUMENT_COUNT, oss.str(), expr, "llamada a función");
    }
    
    // Analyze argument types
    for (auto& arg : expr->args) {
        arg->accept(this);
        // Could add more specific type checking here
    }
    
    // For mathematical functions, assume they return numbers
    // This is a simplification for recursive mathematical functions
    if (expr->callee == "fact" || expr->callee == "fib" || 
        expr->callee == "sum" || expr->callee == "power" ||
        expr->callee == "sqrt" || expr->callee == "sin" || expr->callee == "cos" ||
        expr->callee == "exp" || expr->callee == "log" || expr->callee == "pow") {
        current_type_ = TypeInfo(TypeInfo::Kind::Number);
    } else if (expr->callee == "print" || expr->callee == "println") {
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown); // print doesn't return meaningful value
    } else {
        // For user-defined functions, assume they return numbers for now
        current_type_ = TypeInfo(TypeInfo::Kind::Number);
    }
}

void SemanticAnalyzer::visit(VariableExpr* expr) {
    if (!symbol_table_.isVariableDeclared(expr->name)) {
        reportError(ErrorType::UNDEFINED_VARIABLE,
            "Variable '" + expr->name + "' no está definida",
            expr, "acceso a variable");
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
        return;
    }
    
    // Get variable type from symbol table
    current_type_ = symbol_table_.getVariableType(expr->name);
}

void SemanticAnalyzer::visit(LetExpr* expr) {
    // Analyze initializer
    expr->initializer->accept(this);
    TypeInfo init_type = current_type_;
    
    // Enter new scope for the let expression
    symbol_table_.enterScope();
    
    // Declare the variable with the type from the initializer
    if (symbol_table_.isVariableDeclared(expr->name)) {
        reportError(ErrorType::REDEFINED_VARIABLE,
            "Variable '" + expr->name + "' ya está definida en este ámbito",
            expr, "expresión let");
    } else {
        symbol_table_.declareVariable(expr->name, init_type);
    }
    
    // Analyze body
    expr->body->accept(this);
    
    // Exit scope
    symbol_table_.exitScope();
}

void SemanticAnalyzer::visit(AssignExpr* expr) {
    if (!symbol_table_.isVariableDeclared(expr->name)) {
        reportError(ErrorType::UNDEFINED_VARIABLE,
            "Variable '" + expr->name + "' no está definida",
            expr, "asignación");
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
        return;
    }
    
    // Analyze value
    expr->value->accept(this);
    TypeInfo value_type = current_type_;
    
    // Get variable type and check compatibility
    TypeInfo var_type = symbol_table_.getVariableType(expr->name);
    if (!areTypesCompatible(var_type, value_type)) {
        reportError(ErrorType::TYPE_MISMATCH,
            "No se puede asignar valor de tipo " + value_type.toString() + 
            " a variable de tipo " + var_type.toString(),
            expr, "asignación");
    }
    
    current_type_ = value_type;
}

void SemanticAnalyzer::visit(IfExpr* expr) {
    // Analyze condition
    expr->condition->accept(this);
    TypeInfo cond_type = current_type_;    // Be more lenient with condition types - comparisons should result in boolean
    if (cond_type.getKind() != TypeInfo::Kind::Boolean && cond_type.getKind() != TypeInfo::Kind::Unknown) {
        reportError(ErrorType::TYPE_MISMATCH,
            "La condición del 'if' debe ser booleana, se encontró " + cond_type.toString(),
            expr, "expresión if");
    }
    
    // Analyze then branch
    expr->thenBranch->accept(this);
    TypeInfo then_type = current_type_;
    
    // Analyze else branch if present
    TypeInfo else_type(TypeInfo::Kind::Unknown);
    if (expr->elseBranch) {
        expr->elseBranch->accept(this);
        else_type = current_type_;
        
        // Check type compatibility between branches
        if (!areTypesCompatible(then_type, else_type)) {
            reportError(ErrorType::TYPE_MISMATCH,
                "Las ramas 'then' y 'else' deben tener tipos compatibles, se encontró " +
                then_type.toString() + " y " + else_type.toString(),
                expr, "expresión if");
            current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
            return;
        }
    }
    
    current_type_ = then_type;
}

void SemanticAnalyzer::visit(ExprBlock* expr) {
    symbol_table_.enterScope();
    
    TypeInfo last_type(TypeInfo::Kind::Unknown);
    for (auto& stmt : expr->stmts) {
        stmt->accept(this);
        // The type of a block is the type of its last statement
        if (auto exprStmt = dynamic_cast<ExprStmt*>(stmt.get())) {
            last_type = current_type_;
        }
    }
    
    symbol_table_.exitScope();
    current_type_ = last_type;
}

void SemanticAnalyzer::visit(WhileExpr* expr) {
    // Analyze condition
    expr->condition->accept(this);
    TypeInfo cond_type = current_type_;
      if (cond_type.getKind() != TypeInfo::Kind::Boolean) {
        reportError(ErrorType::TYPE_MISMATCH,
            "La condición del 'while' debe ser booleana, se encontró " + cond_type.toString(),
            expr, "expresión while");
    }
    
    // Analyze body
    expr->body->accept(this);
    
    // While expressions typically don't have a meaningful return type
    current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
}

void SemanticAnalyzer::visit(NewExpr* expr) {
    if (!symbol_table_.isTypeDeclared(expr->typeName)) {
        reportError(ErrorType::UNDEFINED_TYPE,
            "Tipo '" + expr->typeName + "' no está definido",
            expr, "expresión new");
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
        return;
    }
    
    // Analyze constructor arguments
    for (auto& arg : expr->args) {
        arg->accept(this);
    }
    
    current_type_ = TypeInfo(TypeInfo::Kind::Object, expr->typeName);
}

void SemanticAnalyzer::visit(MemberExpr* expr) {
    expr->object->accept(this);
    TypeInfo obj_type = current_type_;
      if (obj_type.getKind() != TypeInfo::Kind::Object) {
        reportError(ErrorType::INVALID_MEMBER_ACCESS,
            "Solo se puede acceder a miembros de objetos, se encontró " + obj_type.toString(),
            expr, "acceso a miembro");
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
        return;
    }
    
    // In a complete implementation, we'd check if the member exists
    // For now, assume it returns an unknown type
    current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
}

void SemanticAnalyzer::visit(SelfExpr* expr) {
    // Self is only valid inside type methods
    // For now, assume it's valid and has object type
    current_type_ = TypeInfo(TypeInfo::Kind::Object, "self");
}

void SemanticAnalyzer::visit(BaseExpr* expr) {
    // Base is only valid inside type methods with inheritance
    // For now, assume it's valid and has object type
    current_type_ = TypeInfo(TypeInfo::Kind::Object, "base");
}

void SemanticAnalyzer::visit(MemberAssignExpr* expr) {
    expr->object->accept(this);
    TypeInfo obj_type = current_type_;
      if (obj_type.getKind() != TypeInfo::Kind::Object) {
        reportError(ErrorType::INVALID_MEMBER_ACCESS,
            "Solo se puede asignar a miembros de objetos, se encontró " + obj_type.toString(),
            expr, "asignación a miembro");
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
        return;
    }
    
    expr->value->accept(this);
    TypeInfo value_type = current_type_;
    
    // In a complete implementation, we'd check member type compatibility
    current_type_ = value_type;
}

void SemanticAnalyzer::visit(MethodCallExpr* expr) {
    expr->object->accept(this);
    TypeInfo obj_type = current_type_;
      if (obj_type.getKind() != TypeInfo::Kind::Object) {
        reportError(ErrorType::INVALID_METHOD_CALL,
            "Solo se pueden llamar métodos en objetos, se encontró " + obj_type.toString(),
            expr, "llamada a método");
        current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
        return;
    }
    
    // Analyze method arguments
    for (auto& arg : expr->args) {
        arg->accept(this);
    }
    
    // In a complete implementation, we'd check method signatures
    current_type_ = TypeInfo(TypeInfo::Kind::Unknown);
}

// Statement visitors
void SemanticAnalyzer::visit(ExprStmt* stmt) {
    stmt->expr->accept(this);
}

void SemanticAnalyzer::visit(FunctionDecl* stmt) {
    symbol_table_.enterScope();
      // Add parameters to current scope with inferred types
    // For recursive functions, assume numeric types for mathematical operations
    for (const auto& param : stmt->params) {
        // Check if parameter name is a reserved word
        if (isReservedWord(param)) {
            reportError(ErrorType::INVALID_OPERATION,
                "No se puede usar la palabra reservada '" + param + "' como nombre de parámetro",
                stmt, "declaración de función '" + stmt->name + "'");
        }
        
        if (symbol_table_.isVariableDeclared(param)) {
            reportError(ErrorType::REDEFINED_VARIABLE,
                "Parámetro '" + param + "' está duplicado",
                stmt, "declaración de función");
        } else {
            // Assume numeric types for mathematical functions
            symbol_table_.declareVariable(param, TypeInfo(TypeInfo::Kind::Number));
        }
    }
    
    // Analyze function body
    stmt->body->accept(this);
    
    symbol_table_.exitScope();
}

void SemanticAnalyzer::visit(TypeDecl* stmt) {
    // Type declarations would need more complex analysis
    // For now, just mark that we've seen this type
    symbol_table_.enterScope();
    
    // Process type parameters, inheritance, methods, etc.
    // This is a simplified implementation
    
    symbol_table_.exitScope();
}

// Helper methods
std::string SemanticAnalyzer::getBinaryOpString(BinaryExpr::Op op) {
    switch (op) {
        case BinaryExpr::OP_ADD: return "+";
        case BinaryExpr::OP_SUB: return "-";
        case BinaryExpr::OP_MUL: return "*";
        case BinaryExpr::OP_DIV: return "/";
        case BinaryExpr::OP_POW: return "^";
        case BinaryExpr::OP_MOD: return "%";
        case BinaryExpr::OP_ENHANCED_MOD: return "%%";
        case BinaryExpr::OP_LT: return "<";
        case BinaryExpr::OP_GT: return ">";
        case BinaryExpr::OP_LE: return "<=";
        case BinaryExpr::OP_GE: return ">=";
        case BinaryExpr::OP_EQ: return "==";
        case BinaryExpr::OP_NEQ: return "!=";
        case BinaryExpr::OP_AND: return "and";
        case BinaryExpr::OP_OR: return "or";
        case BinaryExpr::OP_AND_SIMPLE: return "&";
        case BinaryExpr::OP_OR_SIMPLE: return "|";
        case BinaryExpr::OP_CONCAT: return "@";
        case BinaryExpr::OP_CONCAT_SPACE: return "@@";
        default: return "?";
    }
}

bool SemanticAnalyzer::areTypesCompatible(const TypeInfo& type1, const TypeInfo& type2) {
    // Basic type compatibility check
    if (type1.getKind() == TypeInfo::Kind::Unknown || type2.getKind() == TypeInfo::Kind::Unknown) {
        return true;  // Unknown types are compatible with anything
    }
    
    if (type1.getKind() == type2.getKind()) {
        if (type1.getKind() == TypeInfo::Kind::Object) {
            return type1.getTypeName() == type2.getTypeName();
        }
        return true;
    }
    
    return false;
}

bool SemanticAnalyzer::isReservedWord(const std::string& word) {
    static const std::set<std::string> reserved_words = {
        "base", "self", "new", "type", "if", "else", "while", "for", "in",
        "function", "let", "true", "false", "null", "is", "inherits",
        "protocol", "extends", "class", "method", "attribute"
    };
    return reserved_words.find(word) != reserved_words.end();
}
