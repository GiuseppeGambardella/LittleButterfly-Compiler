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
    SymbolTableVisitor(SymbolTable &symbols, const std::vector<std::string>& src);

    const auto& getErrors() const { return errors; }

    // --- ENTRY POINT ---
    void visit(ProgramNode &node) override;

    // --- Variable definition ---
    void visit(VarDeclNode &node) override;

    // --- variable use (existence control) ---
    void visit(VariableNode &node) override;

    void visit(AssignmentNode &node) override;
    void visit(ReadNode &node) override;

    // --- Other visitors (passing)---
    void visit(FunctionCallNode &node) override;
    void visit(BlockNode &node) override;
    void visit(IfNode &node) override;
    void visit(LoopNode &node) override;
    void visit(ReturnNode &node) override;
    void visit(PrintNode &node) override;
    void visit(UnaryOpNode &node) override;
    void visit(BinaryOpNode &node) override;

    // Nodi foglia o non rilevanti per lo scope
    void visit(NumberNode &) override;
    void visit(RealNode &) override;
    void visit(StringNode &) override;
    void visit(BooleanNode &) override;
    void visit(CharNode &) override;
    void visit(VoidNode &) override;
    void visit(TypeNode &) override;
    void visit(FunctionDeclNode &) override;


private:
    void registerFunctionSignature(FunctionDeclNode &node);

    // analyze function body implementation after signature registration
    void analyzeFunctionBody(FunctionDeclNode &node);

    // --- error reporting ---
    void error(const std::string &msg, int line);
};