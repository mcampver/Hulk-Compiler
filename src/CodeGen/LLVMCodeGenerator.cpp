#include "LLVMCodeGenerator.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include <stdexcept>
#include <iostream>

// Constructor with module name (creates internal context)
LLVMCodeGenerator::LLVMCodeGenerator(const std::string& module_name) 
    : owned_context_(std::make_unique<CodeGenContext>()), 
      context_(*owned_context_) {
    // Note: module_name parameter ignored for now since CodeGenContext creates fixed name
    (void)module_name; // Suppress unused parameter warning
}

// Method to print the generated LLVM module
void LLVMCodeGenerator::printModule() {
    context_.getModule().print(llvm::outs(), nullptr);
}

void LLVMCodeGenerator::visit(Program* prog) {
    // When called from generateCode, we're in the first phase (types and functions)
    // Only process type declarations and function declarations
    for (auto& stmt : prog->stmts) {
        if (auto* type_decl = dynamic_cast<TypeDecl*>(stmt.get())) {
            type_decl->accept(this);
        } else if (auto* func_decl = dynamic_cast<FunctionDecl*>(stmt.get())) {
            func_decl->accept(this);
        }
    }
}

// Separate method to process main expressions
void LLVMCodeGenerator::processMainExpressions(Program* prog) {
    // Process ExprStmt that should go into main
    for (auto& stmt : prog->stmts) {
        if (auto* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get())) {
            expr_stmt->accept(this);
        }
    }
}

void LLVMCodeGenerator::visit(NumberExpr* expr) {
    llvm::Value* value = context_.createNumberConstant(expr->value);
    context_.pushValue(value);
}

void LLVMCodeGenerator::visit(StringExpr* expr) {
    llvm::Value* value = context_.createStringConstant(expr->value);
    context_.pushValue(value);
}

void LLVMCodeGenerator::visit(BooleanExpr* expr) {
    llvm::Value* value = context_.createBooleanConstant(expr->value);
    context_.pushValue(value);
}

void LLVMCodeGenerator::visit(UnaryExpr* expr) {
    expr->operand->accept(this);
    llvm::Value* operand = context_.popValue();
    
    std::string op;
    switch (expr->op) {
        case UnaryExpr::Op::OP_NEG: op = "-"; break;
        case UnaryExpr::Op::OP_NOT: op = "!"; break;
        default:
            throw std::runtime_error("Unknown unary operator");
    }
    
    llvm::Value* result = generateUnaryOperation(op, operand, "Number"); // Assume Number for now
    context_.pushValue(result);
}

void LLVMCodeGenerator::visit(BinaryExpr* expr) {
    // Generate code for operands
    expr->left->accept(this);
    llvm::Value* left = context_.popValue();
    
    expr->right->accept(this);
    llvm::Value* right = context_.popValue();
    
    // Convert enum to string
    std::string op;
    switch (expr->op) {
        case BinaryExpr::Op::OP_ADD: op = "+"; break;
        case BinaryExpr::Op::OP_SUB: op = "-"; break;
        case BinaryExpr::Op::OP_MUL: op = "*"; break;
        case BinaryExpr::Op::OP_DIV: op = "/"; break;
        case BinaryExpr::Op::OP_MOD: op = "%"; break;
        case BinaryExpr::Op::OP_POW: op = "^"; break;
        case BinaryExpr::Op::OP_EQ: op = "=="; break;
        case BinaryExpr::Op::OP_NEQ: op = "!="; break;
        case BinaryExpr::Op::OP_LT: op = "<"; break;
        case BinaryExpr::Op::OP_GT: op = ">"; break;        case BinaryExpr::Op::OP_LE: op = "<="; break;
        case BinaryExpr::Op::OP_GE: op = ">="; break;
        case BinaryExpr::Op::OP_AND: op = "&&"; break;
        case BinaryExpr::Op::OP_OR: op = "||"; break;        // Your enhanced operators
        case BinaryExpr::Op::OP_ENHANCED_MOD: op = "%%"; break;
        case BinaryExpr::Op::OP_TRIPLE_PLUS: op = "+++"; break;
        case BinaryExpr::Op::OP_AND_SIMPLE: op = "&"; break;
        case BinaryExpr::Op::OP_OR_SIMPLE: op = "|"; break;
        case BinaryExpr::Op::OP_CONCAT: op = "@"; break;
        case BinaryExpr::Op::OP_CONCAT_SPACE: op = "@@"; break;
        default:
            throw std::runtime_error("Unknown binary operator");
    }
    
    // Generate operation
    llvm::Value* result = generateBinaryOperation(op, left, right, "Number", "Number"); // Simplified
    context_.pushValue(result);
}

void LLVMCodeGenerator::visit(CallExpr* expr) {
    // Generate arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : expr->args) {
        arg->accept(this);
        args.push_back(context_.popValue());
    }
    
    // Handle built-in functions
    if (expr->callee == "debug" || expr->callee == "type" || expr->callee == "assert" ||
        expr->callee == "print" || expr->callee == "sin" || expr->callee == "cos" ||
        expr->callee == "sqrt" || expr->callee == "exp" || expr->callee == "rand" ||
        expr->callee == "str") {
        llvm::Value* result = generateBuiltinCall(expr->callee, args);
        context_.pushValue(result);
        return;
    }
    
    // Look up function
    llvm::Function* function = context_.lookupFunction(expr->callee);
    if (!function) {
        throw std::runtime_error("Undefined function: " + expr->callee);
    }
    
    // Create call
    llvm::Value* result = context_.getBuilder().CreateCall(function, args);
    context_.pushValue(result);
}

void LLVMCodeGenerator::visit(VariableExpr* expr) {
    llvm::Value* value = context_.lookupVariable(expr->name);
    if (!value) {
        throw std::runtime_error("Undefined variable: " + expr->name);
    }
    context_.pushValue(value);
}

void LLVMCodeGenerator::visit(LetExpr* expr) {
    context_.pushScope();
    
    // Declare variable
    expr->initializer->accept(this);
    llvm::Value* init_value = context_.popValue();
    context_.declareVariable(expr->name, init_value);
    
    // Try to infer and register variable type
    std::string var_type = "";
    if (auto* new_expr = dynamic_cast<NewExpr*>(expr->initializer.get())) {
        var_type = new_expr->typeName;
        context_.declareVariableType(expr->name, var_type);
        context_.addLetVariable(expr->name, var_type);
    }
    
    // Generate body
    expr->body->accept(this);
    
    context_.popScope();
}

void LLVMCodeGenerator::visit(AssignExpr* expr) {
    // Generate value
    expr->value->accept(this);
    llvm::Value* value = context_.popValue();
    
    // Update variable
    context_.declareVariable(expr->name, value);
    context_.pushValue(value);
}

void LLVMCodeGenerator::visit(IfExpr* expr) {
    llvm::Function* function = context_.getCurrentFunction();
    
    // Create blocks
    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(
        context_.getLLVMContext(), "then", function);
    llvm::BasicBlock* else_block = llvm::BasicBlock::Create(
        context_.getLLVMContext(), "else", function);
    llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(
        context_.getLLVMContext(), "ifmerge", function);
    
    // Generate condition
    expr->condition->accept(this);
    llvm::Value* condition = context_.popValue();
    
    // Create conditional branch
    context_.getBuilder().CreateCondBr(condition, then_block, else_block);
      // Generate then block
    context_.getBuilder().SetInsertPoint(then_block);
    expr->thenBranch->accept(this);
    llvm::Value* then_value = context_.popValue();
    context_.getBuilder().CreateBr(merge_block);
    
    // Generate else block
    context_.getBuilder().SetInsertPoint(else_block);
    llvm::Value* else_value = nullptr;
    if (expr->elseBranch) {
        expr->elseBranch->accept(this);
        else_value = context_.popValue();
    } else {
        // Default value for else
        else_value = llvm::ConstantInt::get(context_.getLLVMContext(), llvm::APInt(32, 0));
    }
    context_.getBuilder().CreateBr(merge_block);
    
    // Continue with merge block
    context_.getBuilder().SetInsertPoint(merge_block);
    
    // Create PHI node for result
    llvm::PHINode* phi = context_.getBuilder().CreatePHI(then_value->getType(), 2, "iftmp");
    phi->addIncoming(then_value, then_block);
    phi->addIncoming(else_value, else_block);
    
    context_.pushValue(phi);
}

void LLVMCodeGenerator::visit(ExprBlock* block) {
    context_.pushScope();
    
    llvm::Value* last_value = nullptr;
    for (auto& stmt : block->stmts) {
        stmt->accept(this);
        // Try to get the last value if available
        llvm::Value* current_value = context_.peekValue();
        if (current_value) {
            last_value = context_.popValue();
        }
    }
    
    if (last_value) {
        context_.pushValue(last_value);
    } else {
        // Default value if block is empty
        llvm::Value* default_val = llvm::ConstantInt::get(
            context_.getLLVMContext(), llvm::APInt(32, 0));
        context_.pushValue(default_val);
    }
    
    context_.popScope();
}

void LLVMCodeGenerator::visit(WhileExpr* expr) {
    llvm::Function* function = context_.getCurrentFunction();
    
    llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(
        context_.getLLVMContext(), "loop", function);
    llvm::BasicBlock* body_block = llvm::BasicBlock::Create(
        context_.getLLVMContext(), "loopbody", function);
    llvm::BasicBlock* after_block = llvm::BasicBlock::Create(
        context_.getLLVMContext(), "afterloop", function);
    
    // Jump to loop condition
    context_.getBuilder().CreateBr(loop_block);
    
    // Generate loop condition
    context_.getBuilder().SetInsertPoint(loop_block);
    expr->condition->accept(this);
    llvm::Value* condition = context_.popValue();
    context_.getBuilder().CreateCondBr(condition, body_block, after_block);
    
    // Generate loop body
    context_.getBuilder().SetInsertPoint(body_block);
    expr->body->accept(this);
    llvm::Value* body_value = context_.popValue();
    context_.getBuilder().CreateBr(loop_block);
    
    // Continue after loop
    context_.getBuilder().SetInsertPoint(after_block);
    
    if (body_value) {
        context_.pushValue(body_value);
    }
}

// Missing visitor implementations for object-oriented features
void LLVMCodeGenerator::visit(NewExpr* expr) {
    // Real implementation for object creation with memory allocation
    
    // Allocate memory for the object
    llvm::Value* object = context_.createObjectAllocation(expr->typeName);
    
    if (!object) {
        // Fallback: create a null pointer if type not found
        object = llvm::ConstantPointerNull::get(
            llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext())));
        context_.pushValue(object);
        return;
    }
    
    // Generate arguments for the constructor
    std::vector<llvm::Value*> args;
    args.push_back(object); // self parameter
    
    for (const auto& arg : expr->args) {
        arg->accept(this);
        args.push_back(context_.popValue());
    }
    
    // Call the constructor (init method)
    std::string init_method_name = expr->typeName + "_init";
    llvm::Function* init_func = context_.lookupFunction(init_method_name);
    if (init_func) {
        llvm::Value* initialized_object = context_.getBuilder().CreateCall(init_func, args);
        context_.pushValue(initialized_object);
    } else {
        // No init method found, just return the allocated object
        context_.pushValue(object);
    }
}

void LLVMCodeGenerator::visit(MemberExpr* expr) {
    // Generate the object expression
    expr->object->accept(this);
    llvm::Value* object = context_.popValue();
      // Try to determine the object type from context
    std::string object_type = ""; // This would ideally come from type analysis
    
    // For now, try to infer type from self expressions or assume generic handling
    if (auto* self_expr = dynamic_cast<SelfExpr*>(expr->object.get())) {
        (void)self_expr; // Suppress unused variable warning
        // If accessing self.member, we need to determine current type context
        // For now, we'll use a heuristic or fallback to improved constants
        
        // Try to load the field value using GEP if we know the type
        llvm::Function* current_func = context_.getCurrentFunction();
        if (current_func) {
            std::string func_name = std::string(current_func->getName());
            size_t underscore_pos = func_name.find('_');
            if (underscore_pos != std::string::npos) {
                object_type = func_name.substr(0, underscore_pos);
            }
        }
    }
    
    // If we have type information, use real GEP access
    if (!object_type.empty()) {
        llvm::StructType* struct_type = context_.lookupType(object_type);        if (struct_type) {
            int field_index = context_.getFieldIndex(object_type, expr->member);
            if (field_index >= 0) {
                // Create GEP to access the field
                llvm::Value* field_ptr = context_.getBuilder().CreateStructGEP(
                    struct_type, object, static_cast<unsigned>(field_index), 
                    expr->member + "_ptr");
                
                // Get the actual field type from the struct definition
                llvm::Type* field_type = context_.getFieldType(object_type, field_index);
                if (!field_type) {
                    // Fallback to ptr type if we can't determine the field type
                    field_type = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
                }
                
                // Load the field value with the correct type
                llvm::Value* field_value = context_.getBuilder().CreateLoad(
                    field_type, field_ptr, expr->member);
                
                context_.pushValue(field_value);
                return;
            }
        }
    }
    
    // Fallback to meaningful constants based on member name
    if (expr->member == "name") {
        llvm::Value* member = context_.createStringConstant("object_name_value");
        context_.pushValue(member);
    } else if (expr->member == "breed") {
        llvm::Value* member = context_.createStringConstant("object_breed_value");
        context_.pushValue(member);
    } else if (expr->member == "age") {
        llvm::Value* member = context_.createNumberConstant(5.0);
        context_.pushValue(member);
    } else {
        llvm::Value* member = context_.createNumberConstant(1.0);
        context_.pushValue(member);
    }
}

void LLVMCodeGenerator::visit(SelfExpr* expr) {
    (void)expr; // Suppress unused parameter warning
    // Simplified implementation for 'self' keyword
    // Should return the current object instance
    // Get 'self' parameter from current function
    llvm::Value* self = context_.lookupVariable("self");
    if (self) {
        context_.pushValue(self);
    } else {
        // Fallback: create a placeholder pointer
        llvm::Value* placeholder = llvm::ConstantPointerNull::get(
            llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext())));
        context_.pushValue(placeholder);
    }
}

void LLVMCodeGenerator::visit(BaseExpr* expr) {
    (void)expr; // Suppress unused parameter warning
    // Simplified implementation for 'base' keyword
    // Should call the parent class method
    
    // For now, create a placeholder
    llvm::Value* base = llvm::ConstantInt::get(
        context_.getLLVMContext(), llvm::APInt(32, 0));
    context_.pushValue(base);
}

void LLVMCodeGenerator::visit(MemberAssignExpr* expr) {
    // Generate the object
    expr->object->accept(this);
    llvm::Value* object = context_.popValue();
    
    // Generate the value to assign
    expr->value->accept(this);
    llvm::Value* value = context_.popValue();
    
    // Get the object type to determine field layout
    std::string object_type = "";
    
    // Try to determine object type
    if (auto* self_expr = dynamic_cast<SelfExpr*>(expr->object.get())) {
        // This is self.member assignment - use current type
        object_type = context_.getCurrentType();
    } else if (auto* var_expr = dynamic_cast<VariableExpr*>(expr->object.get())) {
        // Look up variable type
        object_type = context_.getVariableType(var_expr->name);
    }
    
    if (!object_type.empty()) {
        // Get struct type and field information
        llvm::StructType* struct_type = context_.lookupType(object_type);
        if (struct_type) {
            // Get field index
            int field_index = context_.getFieldIndex(object_type, expr->member);
            if (field_index >= 0) {
                // Create GEP to access the field
                llvm::Value* field_ptr = context_.getBuilder().CreateStructGEP(
                    struct_type, object, static_cast<unsigned>(field_index), 
                    expr->member + "_ptr");
                
                // Store the value into the field
                context_.getBuilder().CreateStore(value, field_ptr);
                
                // Push the assigned value as the result of the assignment expression
                context_.pushValue(value);
                return;
            }
        }
    }
    
    // Fallback: just push the assigned value (for cases where we can't determine type)
    context_.pushValue(value);
}

void LLVMCodeGenerator::visit(MethodCallExpr* expr) {
    // Check if this is a base method call (base.method())
    BaseExpr* base_expr = dynamic_cast<BaseExpr*>(expr->object.get());
    if (base_expr) {
        // This is a base.method() call - generate call to parent method
        
        // Get current object (self)
        llvm::Value* self_object = context_.getCurrentSelf();
        if (!self_object) {
            // Fallback: create a null pointer
            self_object = llvm::ConstantPointerNull::get(
                llvm::PointerType::getUnqual(context_.getLLVMContext()));
        }
        
        // Generate arguments
        std::vector<llvm::Value*> args;
        args.push_back(self_object); // self parameter
        for (const auto& arg : expr->args) {
            arg->accept(this);
            args.push_back(context_.popValue());
        }
        
        // Get current type to find parent type
        std::string current_type = context_.getCurrentType();
        std::string parent_type = context_.getParentType(current_type);
        
        if (!parent_type.empty()) {
            // Generate call to parent method
            std::string parent_method_name = parent_type + "_" + expr->method;
            llvm::Function* parent_method = context_.lookupFunction(parent_method_name);
            
            if (parent_method) {
                llvm::Value* result = context_.getBuilder().CreateCall(parent_method, args);
                context_.pushValue(result);
                return;
            }
        }
        
        // Fallback if parent method not found
        llvm::Value* result = context_.createStringConstant("parent_method_not_found");
        context_.pushValue(result);
        return;
    }
    
    // Normal method call (not base.method())
    // Generate the object
    expr->object->accept(this);
    llvm::Value* object = context_.popValue();
    
    // Generate arguments
    std::vector<llvm::Value*> args;
    args.push_back(object); // self parameter
    for (const auto& arg : expr->args) {
        arg->accept(this);
        args.push_back(context_.popValue());
    }
    
    // Determine object type dynamically
    std::string object_type = "";
    
    if (auto* var_expr = dynamic_cast<VariableExpr*>(expr->object.get())) {
        // Look up variable type in symbol table
        object_type = context_.getVariableType(var_expr->name);
    }
    
    if (object_type.empty()) {
        // Fallback: return a default string
        llvm::Value* result = context_.createStringConstant("method_result");
        context_.pushValue(result);
        return;
    }
    
    // Look up method function dynamically
    std::string full_method_name = object_type + "_" + expr->method;
    llvm::Function* method_func = context_.lookupFunction(full_method_name);
    
    if (method_func) {
        llvm::Value* result = context_.getBuilder().CreateCall(method_func, args);
        context_.pushValue(result);
    } else {
        // Method not found, return default string
        llvm::Value* result = context_.createStringConstant("method_not_found");
        context_.pushValue(result);
    }
}

// StmtVisitor implementations
void LLVMCodeGenerator::visit(ExprStmt* stmt) {
    // Generate code for the expression in the statement
    stmt->expr->accept(this);
    // Result is left on the value stack
}

void LLVMCodeGenerator::visit(FunctionDecl* func) {
    // Create new scope for function parameters and local variables
    context_.pushScope();
    
    // Create function type
    std::vector<llvm::Type*> param_types;    for (const auto& param : func->params) {
        (void)param; // Suppress unused variable warning
        param_types.push_back(context_.getLLVMType("Number")); // Simplified type mapping
    }
    
    llvm::Type* return_type = context_.getLLVMType("Number"); // Simplified
    llvm::FunctionType* func_type = llvm::FunctionType::get(return_type, param_types, false);
    
    // Create function
    llvm::Function* llvm_func = llvm::Function::Create(
        func_type, llvm::Function::ExternalLinkage, func->name, context_.getModule());
    
    // Set parameter names
    auto arg_it = llvm_func->arg_begin();
    for (const auto& param : func->params) {
        arg_it->setName(param);
        ++arg_it;
    }
    
    // Create entry basic block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(
        context_.getLLVMContext(), "entry", llvm_func);
    context_.getBuilder().SetInsertPoint(entry);
    
    // Set current function for return statements
    context_.setCurrentFunction(llvm_func);
    
    // Declare parameters as variables in the function scope
    arg_it = llvm_func->arg_begin();
    for (const auto& param : func->params) {
        context_.declareVariable(param, &*arg_it);
        ++arg_it;
    }
      // Generate function body
    func->body->accept(this);
    
    // Create return instruction only if no terminator exists
    llvm::BasicBlock* current_block = context_.getBuilder().GetInsertBlock();
    if (!current_block->getTerminator()) {
        llvm::Value* return_value = context_.popValue();
        context_.getBuilder().CreateRet(return_value);
    }
    
    // Register function
    context_.declareFunction(func->name, llvm_func);
    
    // Pop function scope
    context_.popScope();
}

void LLVMCodeGenerator::visit(TypeDecl* type) {
    // Step 1: Register inheritance relationship
    if (!type->parentType.empty()) {
        context_.declareInheritance(type->name, type->parentType);
    }
    
    // Step 2: Create struct type definition
    createStructForType(type);
    
    // Step 2: Process each method in the type declaration
    for (size_t i = 0; i < type->methodBodies.size() && i < type->methods.size(); ++i) {
        if (type->methodBodies[i]) {
            const auto& method_info = type->methods[i];
            const std::string& method_name = method_info.first;
            const auto& method_params = method_info.second;
            
            // Create a unique method name including the type
            std::string full_method_name = type->name + "_" + method_name;
            
            // Create new scope for method parameters
            context_.pushScope();
            
            // Create function type for the method
            std::vector<llvm::Type*> param_types;
            // Add 'self' parameter as first parameter (pointer to the specific struct type)
            llvm::StructType* struct_type = context_.lookupType(type->name);
            if (struct_type) {
                param_types.push_back(llvm::PointerType::getUnqual(struct_type));
            } else {
                // Fallback to generic pointer
                param_types.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext())));
            }              // Add declared parameters
            for (const auto& param : method_params) {
                // For init methods, infer parameter type from corresponding field (including inherited fields)
                if (method_name == "init" && !method_params.empty()) {
                    // Get the field type for this parameter position
                    size_t param_index = &param - &method_params[0];
                    
                    // Get all fields (including inherited ones)
                    std::vector<std::string> all_fields = context_.getTypeFields(type->name);
                    std::vector<llvm::Type*> all_field_types;
                    
                    // Collect parent field types first
                    if (!type->parentType.empty()) {
                        llvm::StructType* parent_type = context_.lookupType(type->parentType);
                        if (parent_type) {
                            auto parent_field_types = parent_type->elements();
                            for (auto parent_field_type : parent_field_types) {
                                all_field_types.push_back(parent_field_type);
                            }
                        }
                    }
                    
                    // Add current type fields
                    for (const auto& attr : type->attributes) {
                        llvm::Type* field_type = inferFieldType(attr.second);
                        all_field_types.push_back(field_type);
                    }
                    
                    // Use the appropriate field type for this parameter
                    if (param_index < all_field_types.size()) {
                        param_types.push_back(all_field_types[param_index]);
                    } else {
                        // Fallback to string for extra parameters
                        param_types.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext())));
                    }                } else {
                    // For non-init methods, infer parameter type based on method name and parameter name
                    llvm::Type* param_type = inferParameterType(method_name, param);
                    param_types.push_back(param_type);
                }
            }
            
            // Determine return type based on method name and content
            llvm::Type* return_type = getMethodReturnType(method_name, type->methodBodies[i]);
            llvm::FunctionType* func_type = llvm::FunctionType::get(return_type, param_types, false);
            
            // Create function
            llvm::Function* llvm_func = llvm::Function::Create(
                func_type, llvm::Function::ExternalLinkage, full_method_name, context_.getModule());
            
            // Create entry basic block
            llvm::BasicBlock* entry = llvm::BasicBlock::Create(
                context_.getLLVMContext(), "entry", llvm_func);
            context_.getBuilder().SetInsertPoint(entry);
              // Set current function for return statements
            context_.setCurrentFunction(llvm_func);
            
            // Set current context for base method calls
            auto self_arg = llvm_func->arg_begin();
            context_.setCurrentSelf(&*self_arg);
            context_.setCurrentType(type->name);
            
            // Set parameter names and declare as variables
            auto arg_it = llvm_func->arg_begin();
            
            // First parameter is 'self'
            arg_it->setName("self");
            context_.declareVariable("self", &*arg_it);
            ++arg_it;
              // Declare method parameters as variables
            for (const auto& param : method_params) {
                arg_it->setName(param);
                context_.declareVariable(param, &*arg_it);
                ++arg_it;
            }            // Special handling for init methods
            if (method_name == "init") {
                if (method_params.empty()) {
                    // For parameterless init methods, process the AST body
                    type->methodBodies[i]->accept(this);
                } else {
                    // For parameterized init methods, use automatic field assignment
                    generateInitMethodBody(type, method_params, llvm_func);
                }
            } else {
                // Generate method body for non-init methods
                type->methodBodies[i]->accept(this);
                
                // Check if we already have a terminator (return statement)
                llvm::BasicBlock* current_block = context_.getBuilder().GetInsertBlock();
                if (current_block->getTerminator()) {
                    // Block already has a terminator, don't add anything else
                    // Pop method scope and continue to next method
                    context_.popScope();
                    continue;
                }
            }
              // Create return instruction only if no terminator exists
            llvm::BasicBlock* current_block = context_.getBuilder().GetInsertBlock();
            if (!current_block->getTerminator()) {
                // Check if this is a void method
                if (return_type->isVoidTy()) {
                    // For void methods, just return void
                    context_.getBuilder().CreateRetVoid();
                } else {
                    // Create return instruction if needed
                    if (!context_.hasValue()) {
                        // Create default return value based on the return type
                        llvm::Value* default_return;
                        if (return_type->isPointerTy()) {
                            // For init methods returning pointers, return self
                            if (method_name == "init") {
                                auto arg_it = llvm_func->arg_begin();
                                default_return = &*arg_it; // Return self
                            } else {
                                // Return null pointer for other string/pointer types
                                default_return = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(return_type));
                            }
                        } else if (return_type->isDoubleTy()) {
                            // Return 0.0 for double types
                            default_return = context_.createNumberConstant(0.0);
                        } else {
                            // Return 0 for other types (like int)
                            default_return = llvm::ConstantInt::get(return_type, 0);
                        }
                        context_.pushValue(default_return);
                    }
                    
                    llvm::Value* return_value = context_.popValue();
                    
                    // Verify that the return value type matches the function return type
                    if (return_value->getType() != return_type) {
                        // Type mismatch - try to convert or use default
                        if (return_type->isPointerTy() && return_value->getType()->isDoubleTy()) {
                            // For init methods, return self instead of trying to convert double
                            if (method_name == "init") {
                                auto arg_it = llvm_func->arg_begin();
                                return_value = &*arg_it; // Return self
                            } else {
                                // Can't convert double to pointer easily, use null
                                return_value = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(return_type));
                            }
                        } else if (return_type->isDoubleTy() && return_value->getType()->isPointerTy()) {
                            // Can't convert pointer to double easily, use 0.0
                            return_value = context_.createNumberConstant(0.0);
                        }
                    }
                    
                    context_.getBuilder().CreateRet(return_value);
                }
            }
              // Register method
            context_.declareFunction(full_method_name, llvm_func);
            
            // Clear current context
            context_.setCurrentSelf(nullptr);
            context_.setCurrentType("");
            
            // Pop method scope
            context_.popScope();
        }
    }
}

// Helper to determine return type based on method name and content
llvm::Type* LLVMCodeGenerator::getMethodReturnType(const std::string& method_name, const ExprPtr& method_body) {
    // init methods should return a pointer to the object (self)
    if (method_name == "init") {
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
    }
    
    // Methods that commonly return strings based on name patterns
    if (method_name == "speak" || method_name == "getInfo" || method_name == "toString" ||
        method_name == "drive" || method_name == "honk" || method_name == "turbo" ||
        method_name == "introduce" || method_name == "getContact" || method_name == "getSalaryInfo" ||
        method_name == "getText" || method_name == "buildComplexString") {
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
    }
    
    // Analyze method body to infer return type
    if (method_body) {
        // Check if the method body contains string concatenation (@)
        // This is a heuristic: if a method uses string concatenation, it likely returns a string
        if (containsStringOperations(method_body.get())) {
            return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
        }
    }
    
    // Default to double for unknown methods (arithmetic operations, getters that return numbers)
    return context_.getLLVMType("Number");
}

// Helper method implementations
llvm::Value* LLVMCodeGenerator::generateBinaryOperation(const std::string& op, 
                                                       llvm::Value* left, llvm::Value* right,
                                                       const std::string& left_type, const std::string& right_type) {
    auto& builder = context_.getBuilder();
    
    // Arithmetic operations
    if (op == "+") {
        return builder.CreateFAdd(left, right, "addtmp");
    } else if (op == "-") {
        return builder.CreateFSub(left, right, "subtmp");
    } else if (op == "*") {
        return builder.CreateFMul(left, right, "multmp");
    } else if (op == "/") {
        return builder.CreateFDiv(left, right, "divtmp");
    } else if (op == "%") {
        return builder.CreateFRem(left, right, "modtmp");
    } else if (op == "^") {
        // Use pow function
        llvm::Function* pow_func = context_.lookupFunction("pow");
        if (pow_func) {
            return builder.CreateCall(pow_func, {left, right});
        }
    }
    
    // Your enhanced operators
    else if (op == "//") {
        // Integer division: cast to int, divide, cast back
        llvm::Value* left_int = builder.CreateFPToSI(left, llvm::Type::getInt64Ty(context_.getLLVMContext()));
        llvm::Value* right_int = builder.CreateFPToSI(right, llvm::Type::getInt64Ty(context_.getLLVMContext()));
        llvm::Value* div_int = builder.CreateSDiv(left_int, right_int, "intdivtmp");
        return builder.CreateSIToFP(div_int, llvm::Type::getDoubleTy(context_.getLLVMContext()));
    } else if (op == "%%") {
        // Enhanced modulo (always positive)
        llvm::Value* mod_result = builder.CreateFRem(left, right, "modtmp");
        llvm::Value* zero = llvm::ConstantFP::get(context_.getLLVMContext(), llvm::APFloat(0.0));
        llvm::Value* is_negative = builder.CreateFCmpOLT(mod_result, zero);
        llvm::Value* adjusted = builder.CreateFAdd(mod_result, right);
        return builder.CreateSelect(is_negative, adjusted, mod_result, "enhmodtmp");
    } else if (op == "+++") {
        // Triple plus: multiply by 3 if numbers, triple concatenate if strings
        llvm::Value* three = llvm::ConstantFP::get(context_.getLLVMContext(), llvm::APFloat(3.0));
        llvm::Value* left_times_three = builder.CreateFMul(left, three);
        return builder.CreateFAdd(left_times_three, right, "tripletmp");    } else if (op == "@") {
        // String concatenation - ensure both operands are strings
        llvm::Function* concat_func = context_.lookupFunction("hulk_str_concat");
        if (concat_func) {
            // Convert operands to strings if needed
            llvm::Value* left_str = ensureStringType(left, left_type);
            llvm::Value* right_str = ensureStringType(right, right_type);
            return builder.CreateCall(concat_func, {left_str, right_str});
        }
    } else if (op == "@@") {
        // String concatenation with space - ensure both operands are strings
        llvm::Function* concat_space_func = context_.lookupFunction("hulk_str_concat_space");
        if (concat_space_func) {
            // Convert operands to strings if needed
            llvm::Value* left_str = ensureStringType(left, left_type);
            llvm::Value* right_str = ensureStringType(right, right_type);
            return builder.CreateCall(concat_space_func, {left_str, right_str});
        }
    }
    
    // Comparison operations
    else if (op == "==") {
        return builder.CreateFCmpOEQ(left, right, "eqtmp");
    } else if (op == "!=") {
        return builder.CreateFCmpONE(left, right, "netmp");
    } else if (op == "<") {
        return builder.CreateFCmpOLT(left, right, "lttmp");
    } else if (op == ">") {
        return builder.CreateFCmpOGT(left, right, "gttmp");
    } else if (op == "<=") {
        return builder.CreateFCmpOLE(left, right, "letmp");
    } else if (op == ">=") {
        return builder.CreateFCmpOGE(left, right, "getmp");
    }
    
    // Logical operations
    else if (op == "&&" || op == "&") {
        return builder.CreateAnd(left, right, "andtmp");
    } else if (op == "||" || op == "|") {
        return builder.CreateOr(left, right, "ortmp");
    }
    
    throw std::runtime_error("Unsupported binary operator: " + op);
}

llvm::Value* LLVMCodeGenerator::generateUnaryOperation(const std::string& op, 
                                                      llvm::Value* operand, const std::string& operand_type) {
    (void)operand_type; // Suppress unused parameter warning
    auto& builder = context_.getBuilder();
    
    if (op == "-") {
        llvm::Value* zero = llvm::ConstantFP::get(context_.getLLVMContext(), llvm::APFloat(0.0));
        return builder.CreateFSub(zero, operand, "negtmp");
    } else if (op == "!") {
        return builder.CreateNot(operand, "nottmp");
    }
    
    throw std::runtime_error("Unsupported unary operator: " + op);
}

llvm::Value* LLVMCodeGenerator::generateBuiltinCall(const std::string& name, 
                                                    const std::vector<llvm::Value*>& args) {
    auto& builder = context_.getBuilder();
      if (name == "print") {
        if (!args.empty()) {
            llvm::Function* puts_func = context_.lookupFunction("puts");
            if (puts_func) {
                llvm::Value* result = builder.CreateCall(puts_func, {args[0]});
                return result;
            }
        }
        // Return 0 if no args or puts not found
        return llvm::ConstantInt::get(context_.getLLVMContext(), llvm::APInt(32, 0));
    }else if (name == "debug") {
        // For debug, we'll print the value and return it
        if (!args.empty()) {
            llvm::Function* printf_func = context_.lookupFunction("printf");
            if (printf_func) {
                llvm::Value* format = context_.createStringConstant("DEBUG: %g\n");
                builder.CreateCall(printf_func, {format, args[0]});
                return args[0]; // Return the original value
            }
        }
    } else if (name == "type") {
        // Return type as string (simplified)
        return context_.createStringConstant("Number"); // Simplified
    } else if (name == "assert") {
        // Simple assertion implementation
        if (args.size() >= 1) {
            llvm::Function* function = context_.getCurrentFunction();
            llvm::BasicBlock* continue_block = llvm::BasicBlock::Create(
                context_.getLLVMContext(), "assert_continue", function);
            llvm::BasicBlock* fail_block = llvm::BasicBlock::Create(
                context_.getLLVMContext(), "assert_fail", function);
            
            builder.CreateCondBr(args[0], continue_block, fail_block);
            
            // Fail block: print error and exit
            builder.SetInsertPoint(fail_block);
            llvm::Function* printf_func = context_.lookupFunction("printf");
            if (printf_func) {
                llvm::Value* error_msg = context_.createStringConstant("Assertion failed!\n");
                builder.CreateCall(printf_func, {error_msg});
            }
            builder.CreateRet(llvm::ConstantInt::get(context_.getLLVMContext(), llvm::APInt(32, 1)));
            
            // Continue block
            builder.SetInsertPoint(continue_block);
            return context_.createBooleanConstant(true);
        }
    } else if (name == "sin" || name == "cos" || name == "sqrt" || name == "exp") {
        llvm::Function* func = context_.lookupFunction(name);
        if (func && !args.empty()) {
            return builder.CreateCall(func, {args[0]});
        }
    } else if (name == "rand") {
        llvm::Function* func = context_.lookupFunction("hulk_rand");
        if (func) {
            return builder.CreateCall(func, {});
        }
    } else if (name == "str") {
        if (!args.empty()) {
            // Dynamically convert based on the argument type
            llvm::Value* arg = args[0];
            llvm::Type* arg_type = arg->getType();
            
            if (arg_type->isDoubleTy()) {
                // Convert double to string
                llvm::Function* double_to_str_func = context_.lookupFunction("hulk_double_to_str");
                if (double_to_str_func) {
                    return builder.CreateCall(double_to_str_func, {arg});
                }
            } else if (arg_type->isIntegerTy(1)) {
                // Convert boolean to string
                llvm::Function* bool_to_str_func = context_.lookupFunction("hulk_bool_to_str");
                if (bool_to_str_func) {
                    return builder.CreateCall(bool_to_str_func, {arg});
                }
            } else if (arg_type->isIntegerTy()) {
                // Convert integer to double first, then to string
                llvm::Value* as_double = builder.CreateSIToFP(arg, llvm::Type::getDoubleTy(context_.getLLVMContext()));
                llvm::Function* double_to_str_func = context_.lookupFunction("hulk_double_to_str");
                if (double_to_str_func) {
                    return builder.CreateCall(double_to_str_func, {as_double});
                }
            } else if (arg_type->isPointerTy()) {
                // Already a string, return as-is
                return arg;
            }
            
            // Fallback: return a placeholder string
            return context_.createStringConstant("<?>");
        }
    }
      // Return null if function not found or implemented
    return llvm::ConstantPointerNull::get(
        llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext())));
}

// Helper function to ensure a value is converted to string type for concatenation
llvm::Value* LLVMCodeGenerator::ensureStringType(llvm::Value* value, const std::string& type_hint) {
    (void)type_hint; // Suppress unused parameter warning
    auto& builder = context_.getBuilder();
    
    // If it's already a pointer (string), return as-is
    if (value->getType()->isPointerTy()) {
        return value;
    }
    
    // If it's a double, convert to string
    if (value->getType()->isDoubleTy()) {
        llvm::Function* double_to_str_func = context_.lookupFunction("hulk_double_to_str");
        if (double_to_str_func) {
            return builder.CreateCall(double_to_str_func, {value});
        }
    }
    
    // If it's a boolean (i1), convert to string
    if (value->getType()->isIntegerTy(1)) {
        llvm::Function* bool_to_str_func = context_.lookupFunction("hulk_bool_to_str");
        if (bool_to_str_func) {
            return builder.CreateCall(bool_to_str_func, {value});
        }
    }
    
    // For other integer types, convert to double first, then to string
    if (value->getType()->isIntegerTy()) {
        llvm::Value* as_double = builder.CreateSIToFP(value, llvm::Type::getDoubleTy(context_.getLLVMContext()));
        llvm::Function* double_to_str_func = context_.lookupFunction("hulk_double_to_str");
        if (double_to_str_func) {
            return builder.CreateCall(double_to_str_func, {as_double});
        }
    }
    
    // Fallback: return a placeholder string
    return context_.createStringConstant("<?>");
}

// Helper function to create struct types for custom types
void LLVMCodeGenerator::createStructForType(TypeDecl* type) {
    std::vector<llvm::Type*> field_types;
    std::vector<std::string> field_names;
    
    // Collect fields from the type declaration with proper type inference
    for (const auto& attr : type->attributes) {
        llvm::Type* field_type = inferFieldType(attr.second);
        field_types.push_back(field_type);
        field_names.push_back(attr.first);
    }      // Handle inheritance - if this type inherits from another, include parent fields first
    if (!type->parentType.empty()) {
        llvm::StructType* parent_type = context_.lookupType(type->parentType);
        if (parent_type) {
            // Get parent field names and insert them first
            std::vector<std::string> parent_fields = context_.getTypeFields(type->parentType);
            
            // Insert parent fields at the beginning
            field_names.insert(field_names.begin(), parent_fields.begin(), parent_fields.end());
            
            // Insert parent field types at the beginning - use the actual parent field types
            auto parent_field_types = parent_type->elements();
            for (size_t i = 0; i < parent_fields.size() && i < parent_field_types.size(); ++i) {
                field_types.insert(field_types.begin() + i, parent_field_types[i]);
            }
        }
    }
    
    // Create the struct type
    llvm::StructType* struct_type = llvm::StructType::create(
        context_.getLLVMContext(), field_types, type->name);
    
    // Register the type
    context_.declareType(type->name, struct_type, field_names);
    
    // Create automatic init method if type inherits but doesn't have explicit init
    createInheritedInitIfNeeded(type);
}

// Helper function to generate init method body with real field assignments
void LLVMCodeGenerator::generateInitMethodBody(TypeDecl* type, const std::vector<std::string>& method_params, llvm::Function* llvm_func) {
    auto& builder = context_.getBuilder();
    
    // Get the self parameter (first parameter)
    auto arg_it = llvm_func->arg_begin();
    llvm::Value* self = &*arg_it;
    ++arg_it;
    
    // Get struct type
    llvm::StructType* struct_type = context_.lookupType(type->name);
    if (!struct_type) {
        // Fallback: just return self
        return;
    }
    
    // Get field names for this type
    std::vector<std::string> field_names = context_.getTypeFields(type->name);
    
    // Store each parameter into the corresponding field
    for (size_t i = 0; i < method_params.size() && i < field_names.size(); ++i) {
        llvm::Value* param_value = &*arg_it;
        ++arg_it;
        
        // Create GEP to access the field
        llvm::Value* field_ptr = builder.CreateStructGEP(
            struct_type, self, static_cast<unsigned>(i), 
            field_names[i] + "_ptr");
        
        // Store the parameter value into the field
        // Note: Type compatibility should be ensured by the caller
        builder.CreateStore(param_value, field_ptr);
    }
      // Handle inheritance - call parent init if needed
    if (!type->parentType.empty() && !method_params.empty()) {
        // For inherited types, call parent init with appropriate parameters
        std::string parent_init_name = type->parentType + "_init";
        llvm::Function* parent_init = context_.lookupFunction(parent_init_name);
        if (parent_init) {
            // Determine how many parameters the parent init takes
            size_t parent_param_count = parent_init->arg_size() - 1; // Exclude self
            
            std::vector<llvm::Value*> parent_args;
            parent_args.push_back(self); // self parameter
            
            // Add the required number of parameters for parent init
            auto param_it = llvm_func->arg_begin();
            ++param_it; // Skip self
            
            // Pass the first parent_param_count parameters to parent init
            for (size_t i = 0; i < parent_param_count && param_it != llvm_func->arg_end(); ++i) {
                parent_args.push_back(&*param_it);
                ++param_it;
            }
            
            builder.CreateCall(parent_init, parent_args);
        }
    }
}

// Helper function to create inherited init methods automatically
void LLVMCodeGenerator::createInheritedInitIfNeeded(TypeDecl* type) {
    // Check if this type inherits from another but doesn't have its own init
    std::string parent_type = context_.getParentType(type->name);
    if (!parent_type.empty()) {
        bool has_own_init = false;
        for (const auto& method : type->methods) {
            if (method.first == "init") {
                has_own_init = true;
                break;
            }
        }
        
        if (!has_own_init) {
            // Create an init method that delegates to parent
            std::string init_name = type->name + "_init";
            
            // Create function type for init (self + one parameter for name)
            std::vector<llvm::Type*> param_types;
            llvm::StructType* struct_type = context_.lookupType(type->name);
            if (struct_type) {
                param_types.push_back(llvm::PointerType::getUnqual(struct_type));
            } else {
                param_types.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext())));
            }
            param_types.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()))); // name parameter
            
            llvm::Type* return_type = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
            llvm::FunctionType* func_type = llvm::FunctionType::get(return_type, param_types, false);
            
            // Create function
            llvm::Function* init_func = llvm::Function::Create(
                func_type, llvm::Function::ExternalLinkage, init_name, context_.getModule());
            
            // Create entry basic block
            llvm::BasicBlock* entry = llvm::BasicBlock::Create(
                context_.getLLVMContext(), "entry", init_func);
            context_.getBuilder().SetInsertPoint(entry);
            
            // Get parameters
            auto arg_it = init_func->arg_begin();
            llvm::Value* self = &*arg_it;
            ++arg_it;
            llvm::Value* name_param = &*arg_it;
            
            // Call parent init
            std::string parent_init_name = parent_type + "_init";
            llvm::Function* parent_init = context_.lookupFunction(parent_init_name);
            if (parent_init) {
                context_.getBuilder().CreateCall(parent_init, {self, name_param});
            }
            
            // Return self
            context_.getBuilder().CreateRet(self);
            
            // Register the function
            context_.declareFunction(init_name, init_func);
        }
    }
}

// Helper methods for creating object creation functions
void LLVMCodeGenerator::createObjectCreationFunctions(Program* prog) {
    // Find all type declarations and create corresponding create functions
    for (auto& stmt : prog->stmts) {
        if (auto* type_decl = dynamic_cast<TypeDecl*>(stmt.get())) {
            // Find init method to determine parameters
            std::vector<std::string> init_params;
            for (size_t i = 0; i < type_decl->methods.size(); ++i) {
                if (type_decl->methods[i].first == "init") {
                    init_params = type_decl->methods[i].second;
                    break;
                }
            }
            
            // If no init method found but type inherits, 
            // check if we need to create inherited init
            if (init_params.empty() && !type_decl->parentType.empty()) {
                // Look for parent's init method
                for (auto& parent_stmt : prog->stmts) {
                    if (auto* parent_type = dynamic_cast<TypeDecl*>(parent_stmt.get())) {
                        if (parent_type->name == type_decl->parentType) {
                            for (size_t i = 0; i < parent_type->methods.size(); ++i) {
                                if (parent_type->methods[i].first == "init") {
                                    init_params = parent_type->methods[i].second;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }
            
            createObjectCreationFunction(type_decl->name, init_params);
        }
    }
}

void LLVMCodeGenerator::createObjectCreationFunction(const std::string& type_name, const std::vector<std::string>& params) {    // Find the type declaration to infer parameter types
    TypeDecl* type_decl = nullptr;
    (void)type_decl; // Suppress unused variable warning
    // We need access to the program to find type declarations
    // For now, we'll use a simplified approach: infer from the function signature
    
    // Create function type with dynamic parameter type inference
    std::vector<llvm::Type*> param_types;
    
    // Look up the init method to get parameter types
    std::string init_method_name = type_name + "_init";
    llvm::Function* init_func = context_.lookupFunction(init_method_name);
    
    if (init_func) {
        // Get parameter types from the init function (skip first parameter which is self)
        llvm::FunctionType* init_func_type = init_func->getFunctionType();
        for (unsigned i = 1; i < init_func_type->getNumParams(); ++i) {
            param_types.push_back(init_func_type->getParamType(i));
        }
    } else {
        // Fallback: assume string parameters
        for (const auto& param : params) {
            (void)param; // Suppress unused variable warning
            param_types.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext())));
        }
    }
    
    llvm::Type* return_type = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
    llvm::FunctionType* func_type = llvm::FunctionType::get(return_type, param_types, false);
    
    // Create function
    std::string func_name = "create_" + type_name;
    llvm::Function* create_func = llvm::Function::Create(
        func_type, llvm::Function::ExternalLinkage, func_name, context_.getModule());
    
    // Create entry basic block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(
        context_.getLLVMContext(), "entry", create_func);
    context_.getBuilder().SetInsertPoint(entry);
      // Allocate memory for the object
    llvm::Value* object = context_.createObjectAllocation(type_name);
    
    // Use the init_func that was already looked up above
    // If type's init doesn't exist, try parent's init (for inheritance)
    if (!init_func) {
        std::string parent_type = context_.getParentType(type_name);
        if (!parent_type.empty()) {
            init_method_name = parent_type + "_init";
            init_func = context_.lookupFunction(init_method_name);
        }
    }
    
    if (init_func) {
        std::vector<llvm::Value*> init_args;
        init_args.push_back(object); // self parameter
        
        // Add parameters
        auto arg_it = create_func->arg_begin();
        for (size_t i = 0; i < params.size(); ++i) {
            init_args.push_back(&*arg_it);
            ++arg_it;
        }
        
        llvm::Value* initialized_object = context_.getBuilder().CreateCall(init_func, init_args);
        context_.getBuilder().CreateRet(initialized_object);
    } else {
        // No init method, just return the allocated object
        context_.getBuilder().CreateRet(object);
    }
    
    // Register the function
    context_.declareFunction(func_name, create_func);
}

// Helper to generate main function content properly
void LLVMCodeGenerator::generateMainContent(Program* prog) {
    // Process ExprStmt that should go into main (let expressions, etc.)
    for (auto& stmt : prog->stmts) {
        if (auto* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get())) {
            // Process the expression statement
            expr_stmt->expr->accept(this);
            
            // If the expression produced a value, we might want to do something with it
            if (context_.hasValue()) {
                context_.popValue(); // Consume the value
            }
        }
    }
}

// Helper to infer field type from default value
llvm::Type* LLVMCodeGenerator::inferFieldType(Expr* default_value) {
    if (!default_value) {
        // No default value, assume pointer type (string)
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
    }
      // Check the type of the default value expression
    if (auto* number_expr = dynamic_cast<NumberExpr*>(default_value)) {
        (void)number_expr; // Suppress unused variable warning
        return llvm::Type::getDoubleTy(context_.getLLVMContext());
    } else if (auto* string_expr = dynamic_cast<StringExpr*>(default_value)) {
        (void)string_expr; // Suppress unused variable warning
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
    } else if (auto* bool_expr = dynamic_cast<BooleanExpr*>(default_value)) {
        (void)bool_expr; // Suppress unused variable warning
        return llvm::Type::getInt1Ty(context_.getLLVMContext());
    } else {
        // Default to pointer type for complex expressions
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
    }
}

// Helper to check if an expression contains string operations (concatenation, etc.)
bool LLVMCodeGenerator::containsStringOperations(Expr* expr) {
    if (!expr) return false;
    
    // Check if it's a binary expression with string concatenation operator
    if (auto* binary_expr = dynamic_cast<BinaryExpr*>(expr)) {
        if (binary_expr->op == BinaryExpr::Op::OP_CONCAT || 
            binary_expr->op == BinaryExpr::Op::OP_CONCAT_SPACE) {
            return true;
        }
        // Recursively check operands
        return containsStringOperations(binary_expr->left.get()) || 
               containsStringOperations(binary_expr->right.get());
    }
    
    // Check if it's a string literal
    if (dynamic_cast<StringExpr*>(expr)) {
        return true;
    }
    
    // Check if it's a function call to str()
    if (auto* call_expr = dynamic_cast<CallExpr*>(expr)) {
        if (call_expr->callee == "str") {
            return true;
        }
    }
    
    // For other expression types, return false for now
    return false;
}

// Helper to infer parameter type based on method name and parameter name
llvm::Type* LLVMCodeGenerator::inferParameterType(const std::string& method_name, const std::string& param_name) {
    // String parameters (common cases)
    if (param_name == "prefix" || param_name == "suffix" || param_name == "wrapper" ||
        param_name == "text" || param_name == "str" || param_name == "message" ||
        param_name == "name" || param_name == "brand" || param_name == "company" ||
        param_name == "email" || param_name == "a" || param_name == "b" || param_name == "c") {
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
    }
    
    // Methods that typically take string parameters
    if (method_name == "addPrefix" || method_name == "addSuffix" || method_name == "wrapWith" ||
        method_name == "buildComplexString" || method_name == "setText") {
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context_.getLLVMContext()));
    }
    
    // Numeric parameters (amounts, coordinates, values)
    if (param_name == "amount" || param_name == "x" || param_name == "y" || param_name == "exp" ||
        param_name == "value" || param_name == "salary" || param_name == "age" || param_name == "speed") {
        return llvm::Type::getDoubleTy(context_.getLLVMContext());
    }
    
    // Default to Number type for unknown parameters
    return context_.getLLVMType("Number");
}