#pragma once

// AST Includes
#include "../ast/ast_visitor.hpp"
#include "../ast/nodes_impl.hpp"
#include "../ast/helper/helper.h"
#include "../semantic/SymbolTable.hpp"

// MLIR Includes
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"

// Standard Includes
#include <vector>
#include <map>
#include <string>
#include <iostream>

/**
 * Visitor che attraversa l'AST e genera codice intermedio MLIR.
 * Utilizza una strategia "Single Global Scope" dove tutte le variabili
 * sono allocate come memref.global nel modulo principale.
 */
class MLIRGenVisitor : public ASTVisitor {
public:
    // --- Membri Pubblici ---

    // Riferimento al contesto MLIR (gestione memoria oggetti MLIR)
    mlir::MLIRContext& context;

    // Builder per generare le operazioni IR
    mlir::OpBuilder builder;

    // Il modulo radice che conterrà funzioni e globali
    mlir::ModuleOp theModule;

    // Ultimo valore calcolato (registro di passaggio tra nodi)
    mlir::Value lastValue;

    SymbolTable &symTable;

    // --- Costruttore ---
    explicit MLIRGenVisitor(mlir::MLIRContext& ctx, SymbolTable& symTable);

    // --- Metodi di Utilità ---
    // Stampa il codice MLIR generato su console (stdout)
    void dump();

    // Dichiara le funzioni di runtime esterne (es. print)
    void declareRuntimeFunctions();

    void emitMainWrapper();
    // Converte i tipi AST (BasicType) in tipi MLIR
    mlir::Type getMLIRType(BasicType type);

    // --- Visitor Implementation (Overrides) ---

    // Struttura
    void visit(ProgramNode& node) override;
    void visit(FunctionDeclNode& node) override;
    void visit(BlockNode& node) override;
    void visit(ReturnNode& node) override;

    // Variabili
    void visit(VarDeclNode& node) override;
    void visit(VariableNode& node) override;
    void visit(AssignmentNode& node) override;

    // Control Flow
    void visit(IfNode& node) override;
    void visit(LoopNode& node) override;

    // Operazioni
    void visit(BinaryOpNode& node) override;
    void visit(UnaryOpNode& node) override;

    // Letterali
    void visit(NumberNode& node) override;
    void visit(RealNode& node) override;
    void visit(BooleanNode& node) override;
    void visit(CharNode& node) override;

    // Chiamate e Stampe
    void visit(PrintNode& node) override;
    void visit(FunctionCallNode& node) override;

    // Nodi non implementati / Stub
    void visit(ReadNode& node) override;
    void visit(StringNode& node) override;
    void visit(TypeNode& node) override;
    void visit(VoidNode& node) override;

private:
    // --- Helper Privati ---

    // Recupera l'indirizzo di memoria di una variabile globale.
    // Emette un errore se la variabile non esiste nel modulo.
    mlir::Value getGlobalAddress(const std::string& name);

    //Pool di stringhe per evitare duplicati
    int stringLiteralCounter = 0;
    std::unordered_map<std::string, std::string> stringPool;
};