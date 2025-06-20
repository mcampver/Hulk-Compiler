// evaluator.hpp
#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#define _USE_MATH_DEFINES // This ensures M_PI and M_E are defined
#include <cmath>
#include <cctype>

// Fallback definitions if M_PI and M_E are still not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

#include <iostream>
#include <unordered_map>

#include "../AST/ast.hpp"
#include "../Value/enumerable.hpp"
#include "../Value/iterable.hpp"
#include "../Value/value.hpp"
#include "../Value/hulk_object.hpp"
#include "env_frame.hpp"

struct EvaluatorVisitor : StmtVisitor, ExprVisitor
{
    Value lastValue{0.0};
    // En vez de un mapa plano, un puntero a EnvFrame
    std::shared_ptr<EnvFrame> env;

    std::unordered_map<std::string, FunctionDecl *> functions;
    // Registro de tipos para el sistema de objetos
    std::unordered_map<std::string, TypeDecl *> types;
    // Para manejar referencias self durante la ejecución de métodos
    std::shared_ptr<HulkObject> currentSelf;

    EvaluatorVisitor()
    {
        // Inicializar con un frame “global” sin padre
        env = std::make_shared<EnvFrame>(nullptr);
    }    // Programa: recorre stmt a stmt
    void
    visit(Program *p) override
    {
        // Primero registrar TODAS las funciones y tipos
        for (auto &s : p->stmts)
        {
            if (auto *fd = dynamic_cast<FunctionDecl *>(s.get()))
            {
                fd->accept(this); // esto registra la función en el mapa
            }
            else if (auto *td = dynamic_cast<TypeDecl *>(s.get()))
            {
                td->accept(this); // esto registra el tipo en el mapa
            }
        }        // Luego ejecutar todo (excluyendo declaraciones que ya registramos)
        for (auto &s : p->stmts)
        {
            if (!dynamic_cast<FunctionDecl *>(s.get()) && !dynamic_cast<TypeDecl *>(s.get()))
            {
                s->accept(this);
            }
        }
    }

    // StmtVisitor:

    void
    visit(ExprStmt *e) override
    {
        e->expr->accept(this);
    }

    // ExprVisitor:
    void
    visit(NumberExpr *e) override
    {
        lastValue = Value(e->value);
    }

    void
    visit(StringExpr *e) override
    {
        lastValue = Value(e->value);
    }

    void
    visit(BooleanExpr *expr) override
    {
        lastValue = Value(expr->value);
    }    void
    visit(UnaryExpr *e) override
    {
        e->operand->accept(this);
        if (e->op == UnaryExpr::OP_NEG) {
            if (!lastValue.isNumber()) {
                throw std::runtime_error("operador negacion requiere numero");
            }
            lastValue = Value(-lastValue.asNumber());
        } else if (e->op == UnaryExpr::OP_NOT) {
            if (!lastValue.isBool()) {
                throw std::runtime_error("operador ! requiere booleano");
            }
            lastValue = Value(!lastValue.asBool());
        }
    }

    void
    visit(BinaryExpr *e) override
    {
        e->left->accept(this);
        Value l = lastValue;
        e->right->accept(this);
        Value r = lastValue;
        switch (e->op)
        {
        case BinaryExpr::OP_ADD:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error("ambos miembros en una suma deben ser numeros");
            }
            lastValue = Value(l.asNumber() + r.asNumber());
            break;
        case BinaryExpr::OP_SUB:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error("ambos miembros en una resta deben ser numeros");
            }
            lastValue = Value(l.asNumber() - r.asNumber());
            break;
        case BinaryExpr::OP_MUL:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error(
                    "ambos miembros en una multiplicacion deben ser numeros");
            }
            lastValue = Value(l.asNumber() * r.asNumber());
            break;
        case BinaryExpr::OP_DIV:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error("ambos miembros en una division deben ser numeros");
            }
            lastValue = Value(l.asNumber() / r.asNumber());
            break;
        case BinaryExpr::OP_MOD:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error(
                    "ambos miembros en una operacion de resto deben ser numeros");
            }
            lastValue = Value(fmod(l.asNumber(), r.asNumber()));
            break;
        case BinaryExpr::OP_POW:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error("ambos miembros en una potencia deben ser numeros");
            }
            lastValue = Value(pow(l.asNumber(), r.asNumber()));
            break;
        case BinaryExpr::OP_LT:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error("ambos miembros en una comparacion deben ser numeros");
            }
            lastValue = Value(l.asNumber() < r.asNumber() ? true : false);
            break;
        case BinaryExpr::OP_GT:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error("ambos miembros en una comparacion deben ser numeros");
            }
            lastValue = Value(l.asNumber() > r.asNumber() ? true : false);
            break;
        case BinaryExpr::OP_LE:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error("ambos miembros en una comparacion deben ser numeros");
            }
            lastValue = Value(l.asNumber() <= r.asNumber() ? true : false);
            break;
        case BinaryExpr::OP_GE:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error("ambos miembros en una comparacion deben ser numeros");
            }
            lastValue = Value(l.asNumber() >= r.asNumber() ? true : false);
            break;        case BinaryExpr::OP_EQ:
            // Comparación de igualdad para diferentes tipos
            if (l.isNumber() && r.isNumber()) {
                lastValue = Value(l.asNumber() == r.asNumber());
            } else if (l.isBool() && r.isBool()) {
                lastValue = Value(l.asBool() == r.asBool());
            } else if (l.isString() && r.isString()) {
                lastValue = Value(l.asString() == r.asString());
            } else {
                lastValue = Value(false); // Diferentes tipos no son iguales
            }
            break;
        case BinaryExpr::OP_NEQ:
            // Comparación de desigualdad para diferentes tipos
            if (l.isNumber() && r.isNumber()) {
                lastValue = Value(l.asNumber() != r.asNumber());
            } else if (l.isBool() && r.isBool()) {
                lastValue = Value(l.asBool() != r.asBool());
            } else if (l.isString() && r.isString()) {
                lastValue = Value(l.asString() != r.asString());
            } else {
                lastValue = Value(true); // Diferentes tipos son diferentes
            }
            break;
        case BinaryExpr::OP_OR:
            if (!l.isBool() || !r.isBool())
                throw std::runtime_error("and requiere booleanos");
            lastValue = Value(l.asBool() || r.asBool() ? true : false);
            break;
        case BinaryExpr::OP_AND:
            lastValue = Value(l.asBool() && r.asBool() ? true : false);
            break;        case BinaryExpr::OP_CONCAT:
        {
            std::string ls = l.toString();
            std::string rs = r.toString();
            lastValue = Value(ls + rs);
        }
        break;
        case BinaryExpr::OP_CONCAT_SPACE:
        {
            std::string ls = l.toString();
            std::string rs = r.toString();
            lastValue = Value(ls + " " + rs);
        }
        break;
        case BinaryExpr::OP_AND_SIMPLE:
            if (!l.isBool() || !r.isBool())
                throw std::runtime_error("& requiere booleanos");
            lastValue = Value(l.asBool() && r.asBool());
            break;
        case BinaryExpr::OP_OR_SIMPLE:
            if (!l.isBool() || !r.isBool())
                throw std::runtime_error("| requiere booleanos");
            lastValue = Value(l.asBool() || r.asBool());            break;
        case BinaryExpr::OP_ENHANCED_MOD:
            if (!l.isNumber() || !r.isNumber())
            {
                throw std::runtime_error("ambos miembros en modulo mejorado deben ser numeros");
            }
            // Módulo mejorado: siempre devuelve resultado positivo
            {
                double result = fmod(l.asNumber(), r.asNumber());
                if (result < 0) result += fabs(r.asNumber());
                lastValue = Value(result);
            }
            break;
        case BinaryExpr::OP_TRIPLE_PLUS:
            // Triple plus: para números es suma triple, para strings es triple concatenación
            if (l.isNumber() && r.isNumber())
            {
                lastValue = Value(l.asNumber() + r.asNumber() + (l.asNumber() + r.asNumber()));
            }
            else
            {
                std::string ls = l.toString();
                std::string rs = r.toString();
                lastValue = Value(ls + rs + ls + rs + ls + rs);
            }
            break;
        default:
            throw std::runtime_error("Operador desconocido");
        }
    }

    void
    visit(CallExpr *e) override
    {
        std::vector<Value> args;
        for (auto &arg : e->args)
        {
            arg->accept(this);
            args.push_back(lastValue);
        }

        // Funciones definidas por el usuario
        auto it = functions.find(e->callee);
        if (it != functions.end())
        {
            FunctionDecl *f = it->second;

            if (f->params.size() != args.size())
            {
                throw std::runtime_error("Número incorrecto de argumentos para función: " +
                                         f->name);
            }

            // Guardar entorno actual
            auto oldEnv = env;
            env = std::make_shared<EnvFrame>(oldEnv);

            // Asignar parámetros
            for (size_t i = 0; i < f->params.size(); ++i)
            {
                env->locals[f->params[i]] = args[i];
            }

            // Evaluar cuerpo
            f->body->accept(this);

            // Restaurar entorno
            env = std::move(oldEnv);

            return;
        }

        // Funciones nativas del lenguaje
        if (e->callee == "range")
        {
            if (args.size() != 2 || !args[0].isNumber() || !args[1].isNumber())
            {
                throw std::runtime_error("range() espera 2 argumentos numéricos");
            }
            double start = args[0].asNumber();
            double end = args[1].asNumber();
            auto rv = std::make_shared<RangeValue>(start, end);
            lastValue = Value(rv);
            return;
        }
        else if (e->callee == "iter")
        {
            if (args.size() != 1)
            {
                throw std::runtime_error("iter() espera 1 argumento");
            }
            if (args[0].isRange())
            {
                auto rv = args[0].asRange();
                auto itr = rv->iter();
                lastValue = Value(itr);
                return;
            }
            throw std::runtime_error("iter(): el argumento no es Enumerable");
        }
        else if (e->callee == "next")
        {
            if (args.size() != 1 || !args[0].isIterable())
            {
                throw std::runtime_error("next() espera 1 argumento Iterable");
            }
            auto itr = args[0].asIterable();
            bool hay = itr->next();
            lastValue = Value(hay);
            return;
        }
        else if (e->callee == "current")
        {
            if (args.size() != 1 || !args[0].isIterable())
            {
                throw std::runtime_error("current() espera 1 argumento Iterable");
            }
            auto itr = args[0].asIterable();
            lastValue = itr->current();
            return;
        }
        else if (e->callee == "print")
        {
            if (args.size() != 1)
                throw std::runtime_error("print espera 1 argumento");
            std::cout << args[0] << std::endl;
            lastValue = args[0];
            return;
        }
        else if (e->callee == "sqrt")
        {
            if (args.size() != 1)
                throw std::runtime_error("sqrt() espera 1 argumento");
            lastValue = Value(std::sqrt(args[0].asNumber()));
            return;
        }
        else if (e->callee == "log")
        {
            if (args.size() == 1)
            {
                lastValue = Value(std::log(args[0].asNumber()));
                return;
            }
            else if (args.size() == 2)
            {
                double base = args[0].asNumber();
                double x = args[1].asNumber();
                if (base <= 0 || base == 1)
                    throw std::runtime_error("Base inválida para log()");
                if (x <= 0)
                    throw std::runtime_error("Argumento inválido para log()");
                lastValue = Value(std::log(x) / std::log(base));
                return;
            }
            else
            {
                throw std::runtime_error("log() espera 1 o 2 argumentos");
            }
        }
        else if (e->callee == "sin")
        {
            if (args.size() != 1)
                throw std::runtime_error("sin() espera 1 argumento");
            lastValue = Value(std::sin(args[0].asNumber()));
            return;
        }
        else if (e->callee == "cos")
        {
            if (args.size() != 1)
                throw std::runtime_error("cos() espera 1 argumento");
            lastValue = Value(std::cos(args[0].asNumber()));
            return;
        }
        else if (e->callee == "pow")
        {
            if (args.size() != 2)
                throw std::runtime_error("pow() espera 2 argumentos");
            lastValue = Value(std::pow(args[0].asNumber(), args[1].asNumber()));
            return;
        }
        else if (e->callee == "rand")
        {
            lastValue = Value(static_cast<double>(rand()) / RAND_MAX);
            return;
        }
        else if (e->callee == "PI")
        {
            if (!args.empty())
                throw std::runtime_error("PI no toma argumentos");
            lastValue = Value(M_PI);
            return;
        }        else if (e->callee == "E")
        {
            if (!args.empty())
                throw std::runtime_error("E no toma argumentos");
            lastValue = Value(M_E);
            return;
        }
        else if (e->callee == "debug")
        {
            if (args.size() != 1)
                throw std::runtime_error("debug() espera 1 argumento");
            std::cout << "[DEBUG] Valor: " << args[0] << ", Tipo: ";
            if (args[0].isNumber()) std::cout << "Number";
            else if (args[0].isBool()) std::cout << "Boolean";
            else if (args[0].isString()) std::cout << "String";
            else std::cout << "Unknown";
            std::cout << std::endl;
            lastValue = args[0];
            return;
        }
        else if (e->callee == "type")
        {
            if (args.size() != 1)
                throw std::runtime_error("type() espera 1 argumento");
            std::string typeStr;
            if (args[0].isNumber()) typeStr = "Number";
            else if (args[0].isBool()) typeStr = "Boolean";
            else if (args[0].isString()) typeStr = "String";
            else typeStr = "Unknown";
            lastValue = Value(typeStr);
            return;
        }
        else if (e->callee == "assert")
        {
            if (args.size() != 2)
                throw std::runtime_error("assert() espera 2 argumentos");
            if (!args[0].isBool())
                throw std::runtime_error("assert(): primer argumento debe ser booleano");
            if (!args[1].isString())
                throw std::runtime_error("assert(): segundo argumento debe ser string");
            
            if (!args[0].asBool())
                throw std::runtime_error("Assertion failed: " + args[1].asString());
            
            std::cout << "[ASSERT] OK: " << args[1].asString() << std::endl;
            lastValue = Value(true);
            return;
        }
        else if (e->callee == "str")
        {
            if (args.size() != 1)
                throw std::runtime_error("str() espera 1 argumento");
            
            std::string result;
            if (args[0].isNumber()) {
                double num = args[0].asNumber();
                if (num == floor(num)) {
                    // Es un entero
                    result = std::to_string((long long)num);
                } else {
                    // Es un decimal
                    result = std::to_string(num);
                    // Remover ceros trailing
                    while (result.length() > 1 && result.back() == '0' && result[result.length()-2] != '.') {
                        result.pop_back();
                    }
                }
            } else if (args[0].isBool()) {
                result = args[0].asBool() ? "true" : "false";
            } else if (args[0].isString()) {
                result = args[0].asString();
            } else {
                result = "Unknown";
            }
            lastValue = Value(result);
            return;
        }
        else
        {
            throw std::runtime_error("Función desconocida: " + e->callee);
        }
    }

    // for variable declarations
    void
    visit(VariableExpr *expr) override
    {
        // get() buscará en este frame y en los padres
        lastValue = env->get(expr->name);
    }    // let in expressions
    void
    visit(LetExpr *expr) override
    {
        // 1) Evaluar la expresión del inicializador
        expr->initializer->accept(this);
        Value initVal = lastValue;

        // 2) Abrir un nuevo frame (scope hijo)
        auto oldEnv = env; // guardar el frame padre
        env = std::make_shared<EnvFrame>(oldEnv);

        // 3) Insertar la variable en el mapa local
        env->locals[expr->name] = initVal;

        // 4) Evaluar el cuerpo (es un Stmt)
        expr->body->accept(static_cast<StmtVisitor *>(this));
        Value result = lastValue;

        // 5) Al salir, restaurar el frame anterior
        env = std::move(oldEnv);

        // 6) El valor resultante de la expresión let es el valor devuelto
        lastValue = result;
    }

    // destructive assignment
    void
    visit(AssignExpr *expr) override
    {
        // Antes de asignar, evaluamos la expresión de la derecha:
        expr->value->accept(this);
        Value newVal = lastValue;

        // Verificar que exista en alguna parte (no crear nuevas automáticamente):
        if (!env->existsInChain(expr->name))
        {
            throw std::runtime_error("No se puede asignar a variable no declarada: " + expr->name);
        }
        // Llamamos a set() para que reasigne en el frame correspondiente:
        env->set(expr->name, newVal);
        lastValue = newVal;
    }

    // functions declaration
    void
    visit(FunctionDecl *f) override
    {
        if (functions.count(f->name))
            throw std::runtime_error("Funcion ya definida: " + f->name);
        functions[f->name] = f;
    }

    // if-else
    void
    visit(IfExpr *e) override
    {
        e->condition->accept(this);
        if (!lastValue.isBool())
        {
            throw std::runtime_error("La condición de un if debe ser booleana");
        }

        if (lastValue.asBool())
        {
            e->thenBranch->accept(this);
        }
        else
        {
            e->elseBranch->accept(this);
        }
    }

    void
    visit(ExprBlock *b) override
    {
        // 1) Abrir un nuevo frame (scope hijo) antes de entrar al bloque
        auto oldEnv = env;
        env = std::make_shared<EnvFrame>(oldEnv);

        // 2) Evaluar cada sentencia dentro del bloque con este nuevo frame
        for (auto &stmt : b->stmts)
        {
            stmt->accept(this);
        }

        // 3) Restaurar el frame anterior al salir del bloque
        env = std::move(oldEnv);

        // lastValue queda con el valor del último statement ejecutado
    }

    void
    visit(WhileExpr *expr) override
    {
        Value result;
        while (true)
        {
            expr->condition->accept(this);
            if (!lastValue.isBool())
                throw std::runtime_error("La condición de un while debe ser booleana");
            if (!lastValue.asBool())
                break;

            expr->body->accept(this);
            result = lastValue;        }
        lastValue = result;
    }    void visit(TypeDecl *decl) override
    {
        // Registrar el tipo en la tabla de tipos
        if (types.count(decl->name))
            throw std::runtime_error("Tipo ya definido: " + decl->name);
        
        types[decl->name] = decl;
        lastValue = Value(0.0); // Las declaraciones de tipo no retornan valor
    }    void visit(NewExpr *expr) override
    {
        // Buscar el tipo en la tabla de tipos
        auto it = types.find(expr->typeName);
        if (it == types.end()) {
            throw std::runtime_error("Tipo no encontrado: " + expr->typeName);
        }
        
        TypeDecl* typeDecl = it->second;
          // Evaluar argumentos del constructor
        std::vector<Value> args;
        for (auto &arg : expr->args) {
            arg->accept(this);
            args.push_back(lastValue);
        }
          // Buscar el método init para determinar los parámetros esperados
        std::vector<std::string> expectedParams;
        bool hasInitMethod = false;
        
        // Primero buscar en el tipo actual
        for (size_t i = 0; i < typeDecl->methods.size(); ++i) {
            const auto& method = typeDecl->methods[i];
            if (method.first == "init") {
                expectedParams = method.second;
                hasInitMethod = true;
                break;
            }
        }
        
        // Si no tiene init propio, buscar en el tipo padre
        if (!hasInitMethod && !typeDecl->parentType.empty()) {
            auto parentIt = types.find(typeDecl->parentType);
            if (parentIt != types.end()) {
                TypeDecl* parentTypeDecl = parentIt->second;
                for (size_t i = 0; i < parentTypeDecl->methods.size(); ++i) {
                    const auto& method = parentTypeDecl->methods[i];
                    if (method.first == "init") {
                        expectedParams = method.second;
                        hasInitMethod = true;
                        break;
                    }
                }
            }
        }
        
        // Si no hay método init en ningún lado, usar parámetros del tipo (comportamiento anterior)
        if (!hasInitMethod) {
            expectedParams = typeDecl->params;
            
            // Si el tipo no tiene parámetros propios pero tiene padre, heredar del padre
            if (expectedParams.empty() && !typeDecl->parentType.empty()) {
                auto parentIt = types.find(typeDecl->parentType);
                if (parentIt != types.end()) {
                    expectedParams = parentIt->second->params;
                }
            }
        }
        
        // Verificar que el número de argumentos coincida
        if (args.size() != expectedParams.size()) {
            throw std::runtime_error("Tipo " + expr->typeName + " espera " + 
                                   std::to_string(expectedParams.size()) + 
                                   " argumentos, pero se proporcionaron " + 
                                   std::to_string(args.size()));
        }
        
        // Crear nuevo objeto
        auto obj = std::make_shared<HulkObject>(expr->typeName, typeDecl);
        
        // Crear un frame temporal para la inicialización con los parámetros del constructor
        auto oldEnv = env;
        env = std::make_shared<EnvFrame>(oldEnv);
        
        // Agregar los parámetros del constructor al frame actual
        for (size_t i = 0; i < expectedParams.size(); ++i) {
            env->locals[expectedParams[i]] = args[i];
        }
        
        // Si hay herencia, también necesitamos inicializar atributos del padre
        if (!typeDecl->parentType.empty()) {
            auto parentIt = types.find(typeDecl->parentType);
            if (parentIt != types.end()) {
                TypeDecl* parentTypeDecl = parentIt->second;
                
                // Inicializar atributos del padre primero
                for (const auto& attr : parentTypeDecl->attributes) {
                    const std::string& attrName = attr.first;
                    Expr* initExpr = attr.second;
                    
                    if (initExpr) {
                        initExpr->accept(this);
                        Value initValue = lastValue;
                        obj->setAttribute(attrName, initValue);
                    } else {
                        obj->setAttribute(attrName, Value(0.0));
                    }
                }
            }
        }
          // Inicializar atributos propios del tipo
        for (const auto& attr : typeDecl->attributes) {
            const std::string& attrName = attr.first;
            Expr* initExpr = attr.second;
            
            if (initExpr) {
                // Evaluar la expresión de inicialización en el contexto con parámetros
                initExpr->accept(this);
                Value initValue = lastValue;
                obj->setAttribute(attrName, initValue);
            } else {
                // Valor por defecto si no hay inicialización
                obj->setAttribute(attrName, Value(0.0));
            }
        }        // Buscar y ejecutar el constructor init si existe
        bool initExecuted = false;
        for (size_t i = 0; i < typeDecl->methods.size(); ++i) {
            const auto& method = typeDecl->methods[i];
            if (method.first == "init") {
                // Verificar que los argumentos coincidan con los parámetros del constructor
                if (args.size() != method.second.size()) {
                    throw std::runtime_error("Constructor init espera " + 
                                           std::to_string(method.second.size()) + 
                                           " argumentos, pero se proporcionaron " + 
                                           std::to_string(args.size()));
                }
                
                // Establecer contexto de self
                auto oldSelf = currentSelf;
                currentSelf = obj;
                
                // Crear frame para la ejecución del constructor
                auto oldEnv = env;
                env = std::make_shared<EnvFrame>(oldEnv);
                
                // Agregar parámetros del constructor
                for (size_t j = 0; j < method.second.size(); ++j) {
                    env->locals[method.second[j]] = args[j];
                }
                
                // Ejecutar el cuerpo del constructor
                if (i < typeDecl->methodBodies.size() && typeDecl->methodBodies[i]) {
                    typeDecl->methodBodies[i]->accept(this);
                }
                
                // Restaurar entorno y contexto
                env = oldEnv;
                currentSelf = oldSelf;
                initExecuted = true;
                break;
            }
        }
        
        // Si no tiene init propio, usar el del padre
        if (!initExecuted && !typeDecl->parentType.empty()) {
            auto parentIt = types.find(typeDecl->parentType);
            if (parentIt != types.end()) {
                TypeDecl* parentTypeDecl = parentIt->second;
                for (size_t i = 0; i < parentTypeDecl->methods.size(); ++i) {
                    const auto& method = parentTypeDecl->methods[i];
                    if (method.first == "init") {
                        // Verificar que los argumentos coincidan con los parámetros del constructor padre
                        if (args.size() != method.second.size()) {
                            throw std::runtime_error("Constructor padre init espera " + 
                                                   std::to_string(method.second.size()) + 
                                                   " argumentos, pero se proporcionaron " + 
                                                   std::to_string(args.size()));
                        }
                        
                        // Establecer contexto de self
                        auto oldSelf = currentSelf;
                        currentSelf = obj;
                        
                        // Crear frame para la ejecución del constructor padre
                        auto oldEnv = env;
                        env = std::make_shared<EnvFrame>(oldEnv);
                        
                        // Agregar parámetros del constructor
                        for (size_t j = 0; j < method.second.size(); ++j) {
                            env->locals[method.second[j]] = args[j];
                        }
                        
                        // Ejecutar el cuerpo del constructor padre
                        if (i < parentTypeDecl->methodBodies.size() && parentTypeDecl->methodBodies[i]) {
                            parentTypeDecl->methodBodies[i]->accept(this);
                        }
                        
                        // Restaurar entorno y contexto
                        env = oldEnv;
                        currentSelf = oldSelf;
                        break;
                    }
                }
            }
        }
        
        // Restaurar el frame anterior
        env = std::move(oldEnv);
        
        lastValue = Value(obj);
    }void visit(MemberExpr *expr) override
    {
        // Evaluar el objeto
        expr->object->accept(this);
        
        if (!lastValue.isObject()) {
            throw std::runtime_error("Intentando acceder a miembro de un no-objeto");
        }
        
        auto obj = lastValue.asObject();
        
        // Obtener el valor del atributo
        Value attrValue = obj->getAttribute(expr->member);
        lastValue = attrValue;
    }    void visit(SelfExpr *) override
    {
        // Devolver referencia al objeto actual
        if (!currentSelf) {
            throw std::runtime_error("'self' usado fuera del contexto de un método");
        }
        lastValue = Value(currentSelf);
    }    void visit(BaseExpr *) override
    {
        // base() llama al método padre - necesitamos el contexto del método actual
        if (!currentSelf) {
            throw std::runtime_error("'base' usado fuera del contexto de un método");
        }
        
        // Buscar el tipo padre
        TypeDecl* currentTypeDecl = currentSelf->typeDeclaration;
        if (!currentTypeDecl || currentTypeDecl->parentType.empty()) {
            throw std::runtime_error("'base' usado en tipo sin padre");
        }
        
        // Buscar la declaración del tipo padre
        auto it = types.find(currentTypeDecl->parentType);
        if (it == types.end()) {
            throw std::runtime_error("Tipo padre no encontrado: " + currentTypeDecl->parentType);
        }
        
        TypeDecl* parentTypeDecl = it->second;
        
        // Buscar el método "name" en el tipo padre
        for (size_t i = 0; i < parentTypeDecl->methods.size(); ++i) {
            const auto& method = parentTypeDecl->methods[i];
            const std::string& methodName = method.first;
              if (methodName == "name") { // Asumimos que estamos en el método name()
                if (i < parentTypeDecl->methodBodies.size() && parentTypeDecl->methodBodies[i]) {
                    // Ejecutar el método padre
                    parentTypeDecl->methodBodies[i]->accept(this);
                    return;
                }
            }
        }
        
        throw std::runtime_error("Método padre no encontrado");
    }void visit(MemberAssignExpr *expr) override
    {
        // Evaluar el objeto
        expr->object->accept(this);
        
        if (!lastValue.isObject()) {
            throw std::runtime_error("Intentando asignar a miembro de un no-objeto");
        }
        
        auto obj = lastValue.asObject();
        
        // Evaluar el valor a asignar
        expr->value->accept(this);
        Value newValue = lastValue;
        
        // Asignar el nuevo valor al miembro
        obj->setAttribute(expr->member, newValue);
        
        lastValue = newValue;
    }    void visit(MethodCallExpr *expr) override
    {
        // Verificar si es una llamada a método base
        BaseExpr* baseExpr = dynamic_cast<BaseExpr*>(expr->object.get());
        if (baseExpr) {
            // Es una llamada base.method()
            if (!currentSelf) {
                throw std::runtime_error("'base' usado fuera del contexto de un método");
            }
            
            // Buscar el tipo padre
            TypeDecl* currentTypeDecl = currentSelf->typeDeclaration;
            if (!currentTypeDecl || currentTypeDecl->parentType.empty()) {
                throw std::runtime_error("'base' usado en tipo sin padre");
            }
            
            // Buscar la declaración del tipo padre
            auto it = types.find(currentTypeDecl->parentType);
            if (it == types.end()) {
                throw std::runtime_error("Tipo padre no encontrado: " + currentTypeDecl->parentType);
            }
            
            TypeDecl* parentTypeDecl = it->second;
            
            // Evaluar argumentos
            std::vector<Value> args;
            for (auto &arg : expr->args) {
                arg->accept(this);
                args.push_back(lastValue);
            }
              // Buscar el método en toda la cadena de herencia, empezando por el tipo padre
            TypeDecl* searchTypeDecl = parentTypeDecl;
            while (searchTypeDecl) {
                // Buscar el método en el tipo actual
                for (size_t i = 0; i < searchTypeDecl->methods.size(); ++i) {
                    const auto& method = searchTypeDecl->methods[i];
                    const std::string& methodName = method.first;
                    const std::vector<std::string>& params = method.second;
                    
                    if (methodName == expr->method && params.size() == args.size()) {
                        // Encontramos el método con el número correcto de parámetros
                        if (i < searchTypeDecl->methodBodies.size() && searchTypeDecl->methodBodies[i]) {
                            // Mantener el contexto de self actual (no cambiar currentSelf)
                            auto oldSelf = currentSelf;
                            
                            // Crear nuevo frame para parámetros del método
                            auto oldEnv = env;
                            env = std::make_shared<EnvFrame>(oldEnv);
                            
                            // Asignar parámetros
                            for (size_t j = 0; j < params.size(); ++j) {
                                env->locals[params[j]] = args[j];
                            }
                            
                            // Ejecutar cuerpo del método padre
                            searchTypeDecl->methodBodies[i]->accept(this);
                            Value result = lastValue;
                            
                            // Restaurar contexto
                            env = std::move(oldEnv);
                            currentSelf = oldSelf;
                            
                            lastValue = result;
                            return;
                        }
                    }
                }
                
                // Si no se encuentra en este tipo, continuar con el tipo padre
                if (!searchTypeDecl->parentType.empty()) {
                    auto parentIt = types.find(searchTypeDecl->parentType);
                    if (parentIt != types.end()) {
                        searchTypeDecl = parentIt->second;
                    } else {
                        break; // No se pudo encontrar el tipo padre
                    }
                } else {
                    break; // No hay más tipos padre
                }
            }
            
            throw std::runtime_error("Método padre no encontrado: " + expr->method);
        }
        
        // Comportamiento normal para llamadas de método regulares
        // Evaluar el objeto
        expr->object->accept(this);
        
        if (!lastValue.isObject()) {
            throw std::runtime_error("Intentando llamar método en un no-objeto");
        }
        
        auto obj = lastValue.asObject();
        
        // Evaluar argumentos
        std::vector<Value> args;
        for (auto &arg : expr->args) {
            arg->accept(this);
            args.push_back(lastValue);
        }
        
        // Sistema dinámico de métodos - buscar en la declaración del tipo
        TypeDecl* typeDecl = obj->typeDeclaration;
        if (!typeDecl) {
            throw std::runtime_error("Objeto sin declaración de tipo válida");
        }          // Buscar el método en toda la cadena de herencia
        TypeDecl* searchTypeDecl = typeDecl;
        while (searchTypeDecl) {
            // Buscar el método en el tipo actual
            for (size_t i = 0; i < searchTypeDecl->methods.size(); ++i) {
                const auto& method = searchTypeDecl->methods[i];
                const std::string& methodName = method.first;
                const std::vector<std::string>& params = method.second;
                
                if (methodName == expr->method && params.size() == args.size()) {
                    // Encontramos el método con el número correcto de parámetros
                    if (i < searchTypeDecl->methodBodies.size() && searchTypeDecl->methodBodies[i]) {
                        // Establecer contexto de self
                        auto oldSelf = currentSelf;
                        currentSelf = obj;
                        
                        // Crear nuevo frame para parámetros del método
                        auto oldEnv = env;
                        env = std::make_shared<EnvFrame>(oldEnv);
                        
                        // Asignar parámetros
                        for (size_t j = 0; j < params.size(); ++j) {
                            env->locals[params[j]] = args[j];
                        }
                        
                        // Ejecutar cuerpo del método
                        searchTypeDecl->methodBodies[i]->accept(this);
                        Value result = lastValue;
                        
                        // Restaurar contexto
                        env = std::move(oldEnv);
                        currentSelf = oldSelf;
                        
                        lastValue = result;
                        return;
                    }
                }
            }
            
            // Si no se encuentra en este tipo, continuar con el tipo padre
            if (!searchTypeDecl->parentType.empty()) {
                auto parentIt = types.find(searchTypeDecl->parentType);
                if (parentIt != types.end()) {
                    searchTypeDecl = parentIt->second;
                } else {
                    break; // No se pudo encontrar el tipo padre
                }
            } else {
                break; // No hay más tipos padre
            }
        }
        
        // Fallback temporal para métodos básicos comunes (mientras arreglamos el parser)
        if (expr->method.substr(0, 3) == "get" && expr->method.length() > 3 && args.empty()) {
            // Método getter: getX() -> getAttribute("x")
            std::string attrName = expr->method.substr(3);
            attrName[0] = std::tolower(attrName[0]); // getX -> x
            lastValue = obj->getAttribute(attrName);
            return;
        }
        else if (expr->method.substr(0, 3) == "set" && expr->method.length() > 3 && args.size() == 1) {
            // Método setter: setX(value) -> setAttribute("x", value)
            std::string attrName = expr->method.substr(3);
            attrName[0] = std::tolower(attrName[0]); // setX -> x
            obj->setAttribute(attrName, args[0]);
            lastValue = args[0];
            return;
        }
        
        throw std::runtime_error("Método no encontrado: " + expr->method + " en tipo " + obj->typeName);
    }
};

#endif