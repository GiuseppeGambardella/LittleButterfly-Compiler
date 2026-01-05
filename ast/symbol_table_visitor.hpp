//
// Created by marcb on 03/01/2026.
//
#pragma once
#include "ast_visitor.hpp"
#include "nodes_impl.hpp"
#include <iostream>

#include "SymbolTable.hpp"

class SymbolTableVisitor final : public ASTVisitor {
    SymbolTable& symTable;
    BasicType currentType= BasicType::VOID;
    std::vector<std::string> errors;
    bool hasError = false;

    public:
        explicit SymbolTableVisitor(SymbolTable& symbols) : symTable(symbols) {}

        const auto& getErrors() const { return errors; }

        //bool isSuccess() const { return !hasError; }

        void visit(ProgramNode& node) override {
                // FASE 1: DISCOVERY (Variabili Globali + Firme Funzioni)
                for (auto& stmt : node.globals) {
                    if (auto var = dynamic_cast<VarDeclNode*>(stmt.get())) {
                        var->accept(*this); // Registra variabile globale
                    } else if (auto func = dynamic_cast<FunctionDeclNode*>(stmt.get())) {
                        registerFunctionSignature(*func); // Registra solo firma
                    }
                }
                // Registra firma Main
                if (auto mainFunc = dynamic_cast<FunctionDeclNode*>(node.mainBlock.get())) {
                    registerFunctionSignature(*mainFunc);
                }

                // FASE 2: CHECK (Corpi Funzioni)
                for (auto& stmt : node.globals) {
                    if (dynamic_cast<VarDeclNode*>(stmt.get())) continue; // Già fatto
                    if (auto func = dynamic_cast<FunctionDeclNode*>(stmt.get())) {
                        analyzeFunctionBody(*func);
                    }
                }
                // Corpo Main
                if (auto mainFunc = dynamic_cast<FunctionDeclNode*>(node.mainBlock.get())) {
                    if (mainFunc->name != "fly") {
                        error("ABSOLUTE ERROR WARNING: Missing main function 'fly'. £()3");
                    }
                    analyzeFunctionBody(*mainFunc);
                }
            }

        void visit(VarDeclNode& node) override {
            if (node.initializer) {
                node.initializer->accept(*this);
            }
            if (node.type.type == BasicType::VOID) {
                error("La variabile '" + node.name + "' non può essere di tipo VOID.");
                return;
            }
            SymbolInfo info = { node.type.type, false, {} };
            if (!symTable.define(node.name, info)) {
                error("Variabile '" + node.name + "' già definita.");
            }
        }

    // --- GESTIONE CHIAMATE A FUNZIONE ---
        void visit(FunctionCallNode& node) override {
            for (const auto & argument : node.arguments) {
                argument->accept(*this);
            }
        }

    // --- ASSEGNAMENTI E VARIABILI ---
        void visit(AssignmentNode& node) override {
            node.value->accept(*this);
        }

        void visit(VariableNode& node) override { }

    // --- FOGLIE E TIPI BASE ---
        void visit(NumberNode& node) override { currentType = BasicType::INT; }
        void visit(RealNode& node) override { currentType = BasicType::DOUBLE; }
        void visit(StringNode& node) override { currentType = BasicType::STRING; }
        void visit(BooleanNode& node) override { currentType = BasicType::BOOL; }
        void visit(CharNode& node) override { currentType = BasicType::CHAR; }
        void visit(VoidNode& node) override { currentType = BasicType::VOID; }
        void visit(TypeNode& node) override { currentType = node.type; }

    // --- ALTRI NODI ---
        void visit(BlockNode& node) override {
                for (auto& s : node.statements) s->accept(*this);
            }
        void visit(IfNode& node) override {
                node.condition->accept(*this);
                node.thenBranch->accept(*this);
                if (node.elseBranch) node.elseBranch->accept(*this);
            }
        void visit(LoopNode& node) override {
                node.condition->accept(*this);
                node.body->accept(*this);
            }
        void visit(ReturnNode& node) override {
                if (node.value) node.value->accept(*this);
            }

        void visit(PrintNode& node) override { node.expression->accept(*this); }

        void visit(ReadNode& node) override { node.variable->accept(*this); }

        void visit(UnaryOpNode& node) override {
                node.operand->accept(*this);
                // Opzionale: perfezionamento tipo
                if (node.op == "!") currentType = BasicType::BOOL;
            }

        void visit (BinaryOpNode& node) override {
            node.left->accept(*this);
            node.right->accept(*this);
        }

        // Stub per FunctionDeclNode (necessario perché è virtual puro in ASTVisitor,
        // ma la logica vera è gestita manualmente in visit(ProgramNode))
        void visit(FunctionDeclNode& node) override {}

        private:
        // --- HELPER 1: Registra SOLO la firma (Passaggio 1) ---
        void registerFunctionSignature(FunctionDeclNode& node) {
            // 1. Calcola il tipo di ritorno
            node.returnType->accept(*this);
            BasicType returnType = currentType;

            // 2. Raccogli i tipi dei parametri
            std::vector<BasicType> paramTypes;
            for (auto& param : node.parameters) {
                // I parametri sono VarDeclNode. Visitiamo il loro TIPO.
                if (auto varDecl = dynamic_cast<VarDeclNode*>(param.get())) {
                    varDecl->type.accept(*this);
                    paramTypes.push_back(currentType);
                }
            }

            // 3. Inserisci nella Symbol Table
            SymbolInfo info = { returnType, true, paramTypes };
            // Nota: Assumiamo che define ritorni false se esiste già
            if (!symTable.define(node.name, info)) {
                error("Funzione '" + node.name + "' già definita.");
            }
        }

        // --- HELPER 2: Analizza il CORPO (Passaggio 2) ---
        void analyzeFunctionBody(FunctionDeclNode& node) {
            // Registra i parametri come variabili (nello scope globale/piatto)
            for (auto& param : node.parameters) {
                param->accept(*this);
            }

            // Analizza il blocco di codice
            if (node.body) {
                node.body->accept(*this);
            }

            // (Opzionale) Se volessi pulire i parametri dalla tabella per permettere
            // il riuso dei nomi in altre funzioni, dovresti farlo qui.
        }

        void error(const std::string& msg) {
            errors.push_back(msg);
        }

};