#pragma once
#include "../ast/ast_visitor.hpp"
#include "../ast/nodes_impl.hpp"
#include <iostream>
#include "SymbolTable.hpp"
#include "../ast/helper/helper.h"

class SemanticCheckVisitor : public ASTVisitor {
    SymbolTable& symTable;
    BasicType currentType= BasicType::VOID;
    std::vector<std::string> errors;
    bool hasError = false;
    BasicType expectedReturnType = BasicType::VOID;

    public:
        explicit SemanticCheckVisitor(SymbolTable& symbols) : symTable(symbols) {}

        [[nodiscard]] const auto& getErrors() const { return errors; }

        void visit(RealNode& node) override {
            currentType = BasicType::DOUBLE;
        }

        void visit(NumberNode& node) override {
            currentType = BasicType::INT;
        }

        void visit(StringNode& node) override {
            currentType = BasicType::STRING;
        }

        void visit(BooleanNode& node) override {
            currentType = BasicType::BOOL;
        }

        void visit(CharNode& node) override {
            currentType = BasicType::CHAR;
        }

        void visit(VoidNode& node) override {
            currentType = BasicType::VOID;
        }

        void visit(VariableNode& node) override {
            SymbolInfo* info  = symTable.lookup(node.name);
            if (!info) {
                error("Variabile non trovata: " + node.name);
                currentType = BasicType::ERROR;
            } else {
                currentType = info->type;
            }
        }

        void visit(UnaryOpNode& node) override {
            node.operand->accept(*this);
            // Esempio: se l'operatore è "!" e l'operando è BOOL, il risultato è BOOL
            if (node.op == "!") {
                if (currentType != BasicType::BOOL) {
                    error("Operatore '!' applicato a tipo non booleano: " + typeToString(currentType));
                }
                currentType = BasicType::BOOL;
                return;
            }
            // Altri operatori unari possono essere gestiti qui
            // Esempio: operatore "-" per numeri
            if (node.op == "-") {
                if (currentType != BasicType::INT && currentType != BasicType::DOUBLE) {
                    error("Operatore '-' applicato a tipo non numerico: " + typeToString(currentType));
                }
                // Il tipo rimane lo stesso dell'operando
                return;
            }
        }

        void visit(BinaryOpNode& node) override {
            // 1. Evaluate operands
            node.left->accept(*this);
            BasicType leftT = currentType;

            node.right->accept(*this);
            BasicType rightT = currentType;

            /* ===============================
               ARITHMETIC OPERATORS
               +  -  *  /
               =============================== */
            if (node.op == "+" || node.op == "-" || node.op == "*" || node.op == "/") {
                // Numeric arithmetic: INT / REAL combinations allowed
                if ((leftT == BasicType::INT || leftT == BasicType::DOUBLE) &&
                    (rightT == BasicType::INT || rightT == BasicType::DOUBLE)) {
                    // Type promotion: if either is REAL → REAL
                    currentType = (leftT == BasicType::DOUBLE || rightT == BasicType::DOUBLE)
                                      ? BasicType::DOUBLE
                                      : BasicType::INT;
                    return;
                }

                // String concatenation
                if (node.op == "&") {
                    // L'operatore & accetta tutto (es. "Ciao" & 5, 10 & 20) basta che non sia VOID
                    if (leftT == BasicType::VOID || rightT == BasicType::VOID) {
                        error("Impossibile concatenare tipi VOID.");
                        currentType = BasicType::VOID;
                    } else {
                        // Il risultato di una concatenazione è SEMPRE una stringa
                        currentType = BasicType::STRING;
                    }
                    return; // Esce subito
                }

                error("Invalid operands for arithmetic operator '" + node.op +
                      "': " + typeToString(leftT) + " and " + typeToString(rightT));
                currentType = BasicType::ERROR;
                return;
            }

            /* ===============================
               LOGICAL OPERATORS
               AND  OR
               =============================== */
            if (node.op == "AND" || node.op == "OR") {
                if (leftT == BasicType::BOOL && rightT == BasicType::BOOL) {
                    currentType = BasicType::BOOL;
                    return;
                }

                error("Logical operators require boolean operands.");
                currentType = BasicType::ERROR;
                return;
            }

            /* ===============================
               COMPARISON OPERATORS
               ==  <>  <  >  <=  >=
               =============================== */
            if (node.op == "==" || node.op == "<>" ||
                node.op == "<" || node.op == ">" ||
                node.op == "<=" || node.op == ">=") {
                // VOID cannot be compared
                if (leftT == BasicType::VOID || rightT == BasicType::VOID) {
                    error("VOID values cannot be compared.");
                    currentType = BasicType::ERROR;
                    return;
                }

                // Ordering comparisons (<, >, <=, >=) → numeric only
                if (node.op != "==" && node.op != "<>") {
                    if (!((leftT == BasicType::INT || leftT == BasicType::DOUBLE) &&
                          (rightT == BasicType::INT || rightT == BasicType::DOUBLE))) {
                        error("Relational operator '" + node.op +
                              "' requires numeric operands.");
                        currentType = BasicType::ERROR;
                        return;
                    }
                }
                // Equality (==, <>) rules
                else {
                    // Allow numeric cross-type comparison (INT == REAL)
                    if (leftT != rightT &&
                        !((leftT == BasicType::INT || leftT == BasicType::DOUBLE) &&
                          (rightT == BasicType::INT || rightT == BasicType::DOUBLE))) {
                        error("Equality comparison between incompatible types: " +
                              typeToString(leftT) + " and " + typeToString(rightT));
                        currentType = BasicType::ERROR;
                        return;
                    }
                }

                currentType = BasicType::BOOL;
                return;
            }

            /* ===============================
               UNKNOWN OPERATOR
               =============================== */
            error("Unknown binary operator '" + node.op + "'");
            currentType = BasicType::ERROR;
        }

        void visit (FunctionCallNode& node) override {
            SymbolInfo* info = symTable.lookup(node.functionName);
            if (!info) {
                error("Funzione non dichiarata: " + node.functionName);
                currentType = BasicType::VOID; // TO DO (some error type?)
                return;
            }

            if (!info->isFunction) {
                error("'" + node.functionName + "' is not a function");
                currentType = BasicType::VOID; // TO DO (some error type?)
                return;
            }

            // Controlla i tipi degli argomenti
            if (node.arguments.size() != info->paramTypes.size()) {
                error("Numero di argomenti errato per la funzione '" + node.functionName + "'. Attesi: " +
                      std::to_string(info->paramTypes.size()) + ", Trovati: " + std::to_string(node.arguments.size()));
            } else {
                for (size_t i = 0; i < node.arguments.size(); ++i) {
                    node.arguments[i]->accept(*this);
                    if (currentType != info->paramTypes[i]) {
                        error("Tipo dell'argomento " + std::to_string(i+1) + " errato per la funzione '" +
                              node.functionName + "'. Atteso: " + typeToString(info->paramTypes[i]) +
                              ", Trovato: " + typeToString(currentType));
                    }
                }
            }
            currentType = info->type; // Il tipo dell'espressione è il tipo di ritorno della funzione
        }

        void visit (FunctionDeclNode& node) override {
            node.returnType->accept(*this);
            expectedReturnType = currentType;
            // Controlla il corpo della funzione
            if (node.body) {
                node.body->accept(*this);
            }
        }

        void visit(ReturnNode& node) override {
            // Case: return <expression>;
            if (node.value) {
                node.value->accept(*this);

                // Avoid cascading errors
                if (currentType == BasicType::ERROR)
                    return;

                // Allow INT -> REAL promotion
                if (currentType == expectedReturnType)
                    return;

                if (expectedReturnType == BasicType::DOUBLE &&
                    currentType == BasicType::INT)
                    return;

                error("Return type mismatch. Expected: " +
                      typeToString(expectedReturnType) +
                      ", found: " + typeToString(currentType));
                return;
            }

            // Case: return;
            if (expectedReturnType != BasicType::VOID) {
                error("Missing return value in non-void function. Expected: " +
                      typeToString(expectedReturnType));
            }
        }

        void visit(ProgramNode& node) override {
            for (auto& decl : node.globals) {
                decl->accept(*this);
            }
            if (node.mainBlock) {
                node.mainBlock->accept(*this);
            }
        }

        void visit(VarDeclNode& node) override {
            node.type.accept(*this);
            BasicType varType = currentType;

            // Se c'è un'inizializzazione, controlla il tipo
            if (node.initializer) {
                node.initializer->accept(*this);
                if (currentType != varType) {
                    error("Inizializzazione errata per '" + node.name + "'. Atteso: " +
                          typeToString(varType) + ", Trovato: " + typeToString(currentType));
                }
            }
        }

        void visit(AssignmentNode& node) override {
            SymbolInfo* info = symTable.lookup(node.variableName);
            if (!info) {
                error("Variabile non dichiarata: " + node.variableName);
            } else {
                node.value->accept(*this);
                if (info->type != currentType) {
                    error("Assegnamento errato a '" + node.variableName + "'. Atteso: " +
                          typeToString(info->type) + ", Trovato: " + typeToString(currentType));
                }
            }
        }

        void visit(IfNode& node) override {
            node.condition->accept(*this);
            if (currentType != BasicType::BOOL) {
                error("Condizione dell'if non è di tipo BOOL, ma di tipo: " + typeToString(currentType));
            }
            node.thenBranch->accept(*this);
            if (node.elseBranch) node.elseBranch->accept(*this);
        }

        void visit(LoopNode& node) override {
            node.condition->accept(*this);
            if (currentType != BasicType::BOOL) {
                error("Condizione del loop non è di tipo BOOL, ma di tipo: " + typeToString(currentType));
            }
            node.body->accept(*this);
        }

        void visit (BlockNode& node) override {
            for (auto& s : node.statements) s->accept(*this);
        }

        void visit(PrintNode& node) override {
            node.expression->accept(*this);

            if (currentType == BasicType::VOID ||
                currentType == BasicType::ERROR) {

                error("Cannot print expression of type " +
                      typeToString(currentType));
                }

            currentType = BasicType::VOID;
        }

        void visit(ReadNode& node) override {
            node.variable->accept(*this);

            if (currentType == BasicType::VOID ||
                currentType == BasicType::ERROR) {

                error("Cannot read into variable of type " +
                      typeToString(currentType));
            }

            currentType = BasicType::VOID;
        }

        void visit(TypeNode& node) override {
            currentType = node.type;
        }

    private:

        void error(const std::string& message) {
            hasError = true;
            errors.push_back(message);
            std::cerr << "Type Check Error: " << message << std::endl;
        }
};