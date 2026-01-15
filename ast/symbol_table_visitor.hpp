#pragma once
#include "ast_visitor.hpp"
#include "nodes_impl.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include "SymbolTable.hpp"

class SymbolTableVisitor final : public ASTVisitor {
    SymbolTable &symTable;
    const std::vector<std::string>& sourceLines; // Riferimento al codice sorgente
    BasicType currentType = BasicType::VOID;
    std::vector<std::string> errors;
    bool hasError = false;

public:
    // Costruttore aggiornato: prende anche sourceLines
    SymbolTableVisitor(SymbolTable &symbols, const std::vector<std::string>& src)
        : symTable(symbols), sourceLines(src) {}

    const auto &getErrors() const { return errors; }

    // --- ENTRY POINT ---
    void visit(ProgramNode &node) override {
        // 1. REGISTRAZIONE GLOBALI E FUNZIONI (Solo firme)
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

        // 2. ANALISI CORPI
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

    // --- DEFINIZIONE VARIABILI ---
    void visit(VarDeclNode &node) override {
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

    // --- UTILIZZO VARIABILI (Controllo esistenza) ---
    void visit(VariableNode &node) override {
        if (!symTable.lookup(node.name)) {
            error("Variable '" + node.name + "' used before definition.", node.line);
        }
    }

    void visit(AssignmentNode &node) override {
        node.value->accept(*this);
        if (!symTable.lookup(node.variableName)) {
            error("Variable '" + node.variableName + "' assigned before definition.", node.line);
        }
    }

    void visit(ReadNode &node) override {
        if (auto var = dynamic_cast<VariableNode*>(node.variable.get())) {
            if (!symTable.lookup(var->name)) {
                error("Variable '" + var->name + "' used in scan before definition.", node.line);
            }
        }
    }

    // --- ALTRI VISITOR (Passanti) ---
    void visit(FunctionCallNode &node) override {
        for (const auto &arg : node.arguments) arg->accept(*this);
    }
    void visit(BlockNode &node) override {
        for (auto &s : node.statements) s->accept(*this);
    }
    void visit(IfNode &node) override {
        node.condition->accept(*this);
        node.thenBranch->accept(*this);
        if (node.elseBranch) node.elseBranch->accept(*this);
    }
    void visit(LoopNode &node) override {
        node.condition->accept(*this);
        node.body->accept(*this);
    }
    void visit(ReturnNode &node) override {
        if (node.value) node.value->accept(*this);
    }
    void visit(PrintNode &node) override { node.expression->accept(*this); }
    void visit(UnaryOpNode &node) override { node.operand->accept(*this); }
    void visit(BinaryOpNode &node) override {
        node.left->accept(*this);
        node.right->accept(*this);
    }

    // Nodi foglia o non rilevanti per lo scope
    void visit(NumberNode &) override {}
    void visit(RealNode &) override {}
    void visit(StringNode &) override {}
    void visit(BooleanNode &) override {}
    void visit(CharNode &) override {}
    void visit(VoidNode &) override {}
    void visit(TypeNode &) override {}
    void visit(FunctionDeclNode &) override {}

private:
    void registerFunctionSignature(FunctionDeclNode &node) {
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

    void analyzeFunctionBody(FunctionDeclNode &node) {
        for (auto &param : node.parameters) {
            param->accept(*this);
        }
        if (node.body) {
            node.body->accept(*this);
        }
    }

    // --- FORMATTAZIONE ERRORE AVANZATA ---
    void error(const std::string &msg, int line) {
        hasError = true;
        std::stringstream ss;

        // Intestazione colorata
        ss << "SEMANTIC ERROR \033 at line " << line << ": " << msg << "\n";

        // Stampa la riga di codice
        if (line > 0 && line <= (int)sourceLines.size()) {
            ss << "    " << std::setw(4) << line << " | " << sourceLines[line - 1] << "\n";
        }
        errors.push_back(ss.str());
    }
};