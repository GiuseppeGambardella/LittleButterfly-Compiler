#pragma once
#include "symbol_table_visitor.hpp"


// Constructor symbol table visitor: takes symbol table and source lines
SymbolTableVisitor::SymbolTableVisitor(SymbolTable &symbols, const std::vector<std::string>& src)
        : symTable(symbols), sourceLines(src) {}

// ==========================================================
// Helper methods
// ==========================================================

void SymbolTableVisitor::error(const std::string &msg, int line) {
    hasError = true;
    std::stringstream ss;

    // Format error message
    ss << "SEMANTIC ERROR \033 at line " << line << ": " << msg << "\n";

    // Print source line
    if (line > 0 && line <= static_cast<int>(sourceLines.size())) {
        ss << "    " << std::setw(4) << line << " | " << sourceLines[line - 1] << "\n";
    }
    errors.push_back(ss.str());
}

void SymbolTableVisitor::analyzeFunctionBody(FunctionDeclNode &node) {
    for (auto &param : node.parameters) {
        param->accept(*this);
    }
    if (node.body) {
        node.body->accept(*this);
    }
}

void SymbolTableVisitor::registerFunctionSignature(FunctionDeclNode &node) {
    node.returnType->accept(*this);
    std::vector<BasicType> paramTypes;
    for (auto &param : node.parameters) {
        if (auto varDecl = dynamic_cast<VarDeclNode *>(param.get())) {
            paramTypes.push_back(varDecl->type.type);
        }
    }
    SymbolInfo info = {BasicType::VOID, true, paramTypes};
    if (auto typeNode = dynamic_cast<TypeNode*>(node.returnType.get())) {
        info.type = typeNode->type;
    }

    if (!symTable.define(node.name, info)) {
        error("Function '" + node.name + "' already defined.", node.line);
    }
}



// ==========================================================
// Visit implementations
// ==========================================================

    // --- ENTRY POINT ---
void SymbolTableVisitor::visit(ProgramNode &node) {
        // 1. Golbal SIGNATURE REGISTRATION (signatures only)
        for (auto &stmt : node.globals) {
            if (auto var = dynamic_cast<VarDeclNode *>(stmt.get())) {
                var->accept(*this);
            } else if (auto func = dynamic_cast<FunctionDeclNode *>(stmt.get())) {
                registerFunctionSignature(*func);
            }
        }
        if (auto mainFunc = dynamic_cast<FunctionDeclNode *>(node.mainBlock.get())) {
            registerFunctionSignature(*mainFunc);
        }

        // 2. Body ANALYSIS
        for (auto &stmt : node.globals) {
            if (dynamic_cast<VarDeclNode *>(stmt.get())) continue;
            if (auto func = dynamic_cast<FunctionDeclNode *>(stmt.get())) {
                analyzeFunctionBody(*func);
            }
        }
        if (auto mainFunc = dynamic_cast<FunctionDeclNode *>(node.mainBlock.get())) {
            if (mainFunc->name != "fly") {
                error("Missing main function 'fly' or main is not named 'fly'.", mainFunc->line);
            }
            analyzeFunctionBody(*mainFunc);
        }
    }


void SymbolTableVisitor::visit(VarDeclNode &node) {
        if (node.initializer) {
            node.initializer->accept(*this);
        }
        if (node.type.type == BasicType::VOID) {
            error("Variable '" + node.name + "' cannot be VOID.", node.line);
            return;
        }
        SymbolInfo info = {node.type.type, false, {}};
        if (!symTable.define(node.name, info)) {
            error("Variable '" + node.name + "' already defined.", node.line);
        }
    }

void SymbolTableVisitor::visit(VariableNode &node) {
        if (!symTable.lookup(node.name)) {
            error("Variable '" + node.name + "' used before definition.", node.line);
        }
    }

void SymbolTableVisitor::visit(AssignmentNode &node) {
        node.value->accept(*this);
        if (!symTable.lookup(node.variableName)) {
            error("Variable '" + node.variableName + "' assigned before definition.", node.line);
        }
    }

void SymbolTableVisitor::visit(ReadNode &node){
        if (auto var = dynamic_cast<VariableNode*>(node.variable.get())) {
            if (!symTable.lookup(var->name)) {
                error("Variable '" + var->name + "' used in scan before definition.", node.line);
            }
        }
    }

void SymbolTableVisitor::visit(FunctionCallNode &node) {
    for (const auto &arg : node.arguments) arg->accept(*this);
}

void SymbolTableVisitor::visit(BlockNode &node) {
    for (auto &s : node.statements) s->accept(*this);
}

void SymbolTableVisitor::visit(IfNode &node) {
    node.condition->accept(*this);
    node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}

void SymbolTableVisitor::visit(LoopNode &node) {
    node.condition->accept(*this);
    node.body->accept(*this);
}

void SymbolTableVisitor::visit(ReturnNode &node) {
    if (node.value) node.value->accept(*this);
}

void SymbolTableVisitor::visit(PrintNode &node) { node.expression->accept(*this); }
void SymbolTableVisitor::visit(UnaryOpNode &node) { node.operand->accept(*this); }

void SymbolTableVisitor::visit(BinaryOpNode &node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

// leaf nodes - empty implementations
void SymbolTableVisitor::visit(NumberNode &) {}
void SymbolTableVisitor::visit(RealNode &) {}
void SymbolTableVisitor::visit(StringNode &) {}
void SymbolTableVisitor::visit(BooleanNode &) {}
void SymbolTableVisitor::visit(CharNode &) {}
void SymbolTableVisitor::visit(VoidNode &) {}
void SymbolTableVisitor::visit(TypeNode &) {}
void SymbolTableVisitor::visit(FunctionDeclNode &) {}