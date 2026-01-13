#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <optional>

#include "../ast/ast_visitor.hpp"
#include "../ast/nodes_impl.hpp"
#include "../semantic/SymbolTable.hpp"   // ← la tua, adattala

#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Verifier.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"

#include "llvm/Support/raw_ostream.h"

class MLIRGenVisitor : public ASTVisitor {

    mlir::Location loc;
    mlir::MLIRContext &context;
    mlir::OpBuilder builder;
    SymbolTable &symbols;
    mlir::ModuleOp module;
    mlir::SymbolTable symbolTable;
    mlir::ModuleOp moduleOp;

public:
    explicit MLIRGenVisitor(mlir::MLIRContext &ctx, SymbolTable &sym);

    [[nodiscard]] mlir::ModuleOp getModule() const { return module; }

    // Entry point: genera tutto (globali + main)
    void codegen(ProgramNode &program);

    // ---- Visitor overrides (adatta i nomi ai tuoi nodi) ----
    void visit(ProgramNode &node) override;
    void visit(VarDeclNode &node) override;
    void visit(AssignmentNode &node) override;
    void visit(BinaryOpNode &node) override {
        node.left->accept(*this);
        mlir::Value leftVal = currentValue;
        node.right->accept(*this);
        mlir::Value rightVal = currentValue;

        if (node.op == "+") {
            currentValue = builder.create<mlir::arith::AddIOp>(loc, leftVal, rightVal);
        }
        if (node.op == "-") {
            currentValue = builder.create<mlir::arith::SubIOp>(loc, leftVal, rightVal);
        }
        if (node.op == "*") {
            currentValue = builder.create<mlir::arith::MulIOp>(loc, leftVal, rightVal);
        }

        if (node.op == "/") {
            currentValue = builder.create<mlir::arith::DivUIOp>(loc, leftVal, rightVal);
        }

        if (node.op == "<") {
            currentValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::slt, leftVal, rightVal);
        }

        if (node.op == "<=") {
            currentValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sle, leftVal, rightVal);
        }
        if (node.op == ">") {
            currentValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sgt, leftVal, rightVal);
        }
        if (node.op == ">=") {
            currentValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sge, leftVal, rightVal);
        }
        if (node.op == "==") {
            currentValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::eq, leftVal, rightVal);
        }
        if (node.op == "<>") {
            currentValue = builder.create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne, leftVal, rightVal);
        }
        // String concatenation
        if (node.op == "&") {
            //TO DO
            // Assuming you have a runtime function for string concatenation
            auto concatFunc = module.lookupSymbol<mlir::func::FuncOp>("string_concat");
            if (!concatFunc) {
                throw std::runtime_error("Runtime function 'string_concat' not found");
            }
            currentValue = builder.create<mlir::func::CallOp>(loc, concatFunc, mlir::TypeRange{builder.getType<mlir::StringType>()}, mlir::ValueRange{leftVal, rightVal}).getResult(0);
        }
        if (node.op == "and") {
            currentValue= builder.create<mlir::arith::AndIOp>(loc, leftVal, rightVal);
        }
        if (node.op == "or") {
            currentValue = builder.create<mlir::arith::OrIOp>(loc, leftVal, rightVal);
        }
        if (node.op == "!") {
            currentValue = builder.create<mlir::arith::XOrIOp>(loc, leftVal, builder.create<mlir::arith::ConstantOp>(loc, builder.getIntegerAttr(builder.getI1Type(), 1)));
        }
    }
    void visit(NumberNode &node) override;
    void visit(RealNode &node) override;
    void visit(BooleanNode &node) override;
    void visit(CharNode &node) override;
    void visit(VariableNode &node) override;
    void visit(BlockNode &node) override;
    void visit(ReturnNode &node) override;
    void visit(IfNode &node) override;
    void visit(LoopNode &node) override;
    void visit(PrintNode &node) override;
    void visit(ReadNode &node) override;
    void visit(FunctionDeclNode &node) override;
    void visit(FunctionCallNode &node) override;
    void visit(TypeNode &node) override;
    void visit(UnaryOpNode &node) override;
    void visit(StringNode &node) override;


private:
    mlir::MLIRContext &context;
    mlir::OpBuilder builder;
    mlir::Location loc;

    SymbolTable &symbols;

    mlir::ModuleOp module;

    // mapping nome variabile globale -> memref.global (symbol name)
    // NB: per usare i globali servono GetGlobalOp, non un Value "allocato"
    std::unordered_map<std::string, mlir::MemRefType> globalTypes;

    // per le espressioni
    mlir::Value currentValue;

private:
    // helpers
    mlir::Type toMLIRScalar(BasicType t);
    mlir::MemRefType toGlobalMemRefType(BasicType t);

    void declareRuntime();  // print, ecc.
    void emitGlobals(ProgramNode &program);
    void emitMain(ProgramNode &program);

    mlir::Value getGlobalAddr(const std::string &name); // memref.get_global
    void ensure(bool cond, const std::string &msg);
};
