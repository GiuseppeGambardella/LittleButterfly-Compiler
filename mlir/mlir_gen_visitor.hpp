#pragma once

#include "../ast/ast_visitor.hpp"
#include "../ast/nodes_impl.hpp"
#include "../semantic/SymbolTable.hpp"

#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h" // <--- FONDAMENTALE
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"

#include <map>
#include <string>

class MLIRGenVisitor : public ASTVisitor {
public:
    mlir::MLIRContext& context;
    mlir::OpBuilder builder;

    // Ordine importante: il modulo deve essere creato prima della symbol table che lo gestisce
    mlir::ModuleOp theModule;
    mlir::SymbolTable mlirSymTable;

    mlir::Value lastValue;
    SymbolTable& symTable; // La tua tabella semantica

    MLIRGenVisitor(mlir::MLIRContext& ctx, SymbolTable& symTable);

    // ... (Tutti i visit rimangono uguali) ...
    void visit(ProgramNode& node) override;
    void visit(FunctionDeclNode& node) override;
    void visit(BlockNode& node) override;
    void visit(ReturnNode& node) override;
    void visit(VarDeclNode& node) override;
    void visit(VariableNode& node) override;
    void visit(AssignmentNode& node) override;
    void visit(IfNode& node) override;
    void visit(LoopNode& node) override;
    void visit(BinaryOpNode& node) override;
    void visit(UnaryOpNode& node) override;
    void visit(NumberNode& node) override;
    void visit(RealNode& node) override;
    void visit(BooleanNode& node) override;
    void visit(CharNode& node) override;
    void visit(PrintNode& node) override;
    void visit(FunctionCallNode& node) override;
    void visit(ReadNode& node) override;
    void visit(StringNode& node) override;
    void visit(TypeNode& node) override;
    void visit(VoidNode& node) override;

    void dump();
    void declareRuntimeFunctions();
    void emitMainWrapper();
    mlir::Type getMLIRType(BasicType type);

private:
    int stringLiteralCounter = 0;
    std::unordered_map<std::string, std::string> stringPool;

    std::unordered_map<std::string, mlir::Value> stringEnv;

    // Helper interni
    void initializeGlobalsFromSymbolTable();
    mlir::Value getGlobalAddress(const std::string& name);
};