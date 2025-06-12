#include "LLVMCodeGenerator.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include <stdexcept>
#include <iostream>

void LLVMCodeGenerator::visit(Program* prog) {
    for (auto& stmt : prog->stmts) {
        stmt->accept(this);
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
    expr->expr->accept(this);
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
        case BinaryExpr::Op::OP_GT: op = ">"; break;
        case BinaryExpr::Op::OP_LEQ: op = "<="; break;
        case BinaryExpr::Op::OP_GEQ: op = ">="; break;
        case BinaryExpr::Op::OP_AND: op = "&&"; break;
        case BinaryExpr::Op::OP_OR: op = "||"; break;
        // Your enhanced operators
        case BinaryExpr::Op::OP_INT_DIV: op = "//"; break;
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
    if (expr->name == "debug" || expr->name == "type" || expr->name == "assert" ||
        expr->name == "print" || expr->name == "sin" || expr->name == "cos" ||
        expr->name == "sqrt" || expr->name == "exp" || expr->name == "rand") {
        llvm::Value* result = generateBuiltinCall(expr->name, args);
        context_.pushValue(result);
        return;
    }
    
    // Look up function
    llvm::Function* function = context_.lookupFunction(expr->name);
    if (!function) {
        throw std::runtime_error("Undefined function: " + expr->name);
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
    
    // Declare variables
    for (const auto& decl : expr->decls) {
        decl.init->accept(this);
        llvm::Value* init_value = context_.popValue();
        context_.declareVariable(decl.name, init_value);
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
    expr->then_expr->accept(this);
    llvm::Value* then_value = context_.popValue();
    context_.getBuilder().CreateBr(merge_block);
    
    // Generate else block
    context_.getBuilder().SetInsertPoint(else_block);
    llvm::Value* else_value = nullptr;
    if (expr->else_expr) {
        expr->else_expr->accept(this);
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
    for (auto& expr : block->exprs) {
        expr->accept(this);
        if (context_.peekValue()) {
            last_value = context_.popValue();
        }
    }
    
    if (last_value) {
        context_.pushValue(last_value);
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
        return builder.CreateFAdd(left_times_three, right, "tripletmp");
    } else if (op == "@") {
        // String concatenation
        llvm::Function* concat_func = context_.lookupFunction("hulk_str_concat");
        if (concat_func) {
            return builder.CreateCall(concat_func, {left, right});
        }
    } else if (op == "@@") {
        // String concatenation with space
        llvm::Function* concat_space_func = context_.lookupFunction("hulk_str_concat_space");
        if (concat_space_func) {
            return builder.CreateCall(concat_space_func, {left, right});
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
                return builder.CreateCall(puts_func, {args[0]});
            }
        }
    } else if (name == "debug") {
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
    }
    
    // Return null if function not found or implemented
    return llvm::ConstantPointerNull::get(
        llvm::Type::getInt8PtrTy(context_.getLLVMContext()));
}

// Type conversion helpers (simplified implementations)
llvm::Value* LLVMCodeGenerator::convertToString(llvm::Value* value, const std::string& from_type) {
    // Simplified: just return the value as-is for now
    return value;
}

llvm::Value* LLVMCodeGenerator::convertToNumber(llvm::Value* value, const std::string& from_type) {
    // Simplified: just return the value as-is for now
    return value;
}

llvm::Value* LLVMCodeGenerator::convertToBoolean(llvm::Value* value, const std::string& from_type) {
    // Simplified: just return the value as-is for now
    return value;
}
