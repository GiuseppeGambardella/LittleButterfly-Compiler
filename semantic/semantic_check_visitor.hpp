#pragma once
#include "../ast/ast_visitor.hpp"
#include "../ast/helper/helper.h"
#include "../ast/nodes_impl.hpp"
#include "SymbolTable.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

class SemanticCheckVisitor : public ASTVisitor {
    SymbolTable &symTable;
    const std::vector<std::string>& sourceLines;
    BasicType currentType = BasicType::VOID;
    std::vector<std::string> errors;
    bool hasError = false;
    BasicType expectedReturnType = BasicType::VOID;
    std::string currentFunctionName; // used to track the current function for recursion checks

public:
    SemanticCheckVisitor(SymbolTable &symbols, const std::vector<std::string>& src)
        : symTable(symbols), sourceLines(src) {}

    [[nodiscard]] const auto &getErrors() const { return errors; }

    void visit(FunctionDeclNode &node) override {
        std::string previousFunction = currentFunctionName;
        currentFunctionName = node.name;

        node.returnType->accept(*this);
        expectedReturnType = currentType;

        if (node.body) {
            node.body->accept(*this);
        }


        currentFunctionName = previousFunction;
    }

    void visit(FunctionCallNode &node) override {
        // check for recursion
        if (!currentFunctionName.empty() && node.functionName == currentFunctionName) {
            error("Recursion is not supported: function '" + node.functionName + "' cannot call itself.", node.line);
            currentType = BasicType::ERROR;
            return;
        }

        SymbolInfo *info = symTable.lookup(node.functionName);
        if (!info) {
            error("Function not declared: " + node.functionName, node.line);
            currentType = BasicType::ERROR;
            return;
        }
        if (!info->isFunction) {
            error("'" + node.functionName + "' is not a function", node.line);
            currentType = BasicType::ERROR;
            return;
        }
        if (node.arguments.size() != info->paramTypes.size()) {
            error("Wrong number of arguments for function '" + node.functionName +
                  "'. Expected: " + std::to_string(info->paramTypes.size()) +
                  ", Found: " + std::to_string(node.arguments.size()), node.line);
        } else {
            for (size_t i = 0; i < node.arguments.size(); ++i) {
                node.arguments[i]->accept(*this);
                if (currentType != info->paramTypes[i]) {
                    error("Wrong type for argument " + std::to_string(i + 1) +
                          " of function '" + node.functionName +
                          "'. Expected: " + typeToString(info->paramTypes[i]) +
                          ", Found: " + typeToString(currentType), node.arguments[i]->line);
                }
            }
        }
        currentType = info->type;
    }


    void visit(RealNode &node) override { currentType = BasicType::DOUBLE; }
    void visit(NumberNode &node) override { currentType = BasicType::INT; }
    void visit(StringNode &node) override { currentType = BasicType::STRING; }
    void visit(BooleanNode &node) override { currentType = BasicType::BOOL; }
    void visit(CharNode &node) override { currentType = BasicType::CHAR; }
    void visit(VoidNode &node) override { currentType = BasicType::VOID; }
    void visit(TypeNode &node) override { currentType = node.type; }

    void visit(VariableNode &node) override {
        SymbolInfo *info = symTable.lookup(node.name);
        if (!info) {
            error("Variable not found: " + node.name, node.line);
            currentType = BasicType::ERROR;
        } else {
            currentType = info->type;
        }
    }

    void visit(UnaryOpNode &node) override {
        node.operand->accept(*this);
        if (node.op == "!") {
            if (currentType != BasicType::BOOL) {
                error("Operator '!' applied to non-boolean type: " + typeToString(currentType), node.line);
            }
            currentType = BasicType::BOOL;
            return;
        }
        if (node.op == "-") {
            if (currentType != BasicType::INT && currentType != BasicType::DOUBLE) {
                error("Operator '-' applied to non-numeric type: " + typeToString(currentType), node.line);
            }
            return;
        }
        if (node.op == "?") {
            if (currentType != BasicType::INT) {
                error("Operator '?' applied to non-integer type: " + typeToString(currentType), node.line);
            }
        }
    }

    void visit(BinaryOpNode &node) override {
        node.left->accept(*this);
        BasicType leftT = currentType;
        node.right->accept(*this);
        BasicType rightT = currentType;

        if (node.op == "&") {
            if (leftT == BasicType::VOID || rightT == BasicType::VOID) {
                error("Cannot concatenate VOID types.", node.line);
                currentType = BasicType::ERROR;
            } else {
                currentType = BasicType::STRING;
            }
            return;
        }

        if (node.op == "+" || node.op == "-" || node.op == "*" || node.op == "/") {
            if ((leftT == BasicType::INT || leftT == BasicType::DOUBLE) &&
                (rightT == BasicType::INT || rightT == BasicType::DOUBLE)) {
                currentType = (leftT == BasicType::DOUBLE || rightT == BasicType::DOUBLE) ? BasicType::DOUBLE : BasicType::INT;
                return;
            }
            error("Invalid operands for arithmetic operator '" + node.op + "': " + typeToString(leftT) + " and " + typeToString(rightT), node.line);
            currentType = BasicType::ERROR;
            return;
        }

        if (node.op == "AND" || node.op == "OR") {
            if (leftT == BasicType::BOOL && rightT == BasicType::BOOL) {
                currentType = BasicType::BOOL;
                return;
            }
            error("Logical operators require boolean operands.", node.line);
            currentType = BasicType::ERROR;
            return;
        }

        if (node.op == "==" || node.op == "<>" || node.op == "<" ||
            node.op == ">" || node.op == "<=" || node.op == ">=") {
            if (leftT == BasicType::VOID || rightT == BasicType::VOID) {
                error("VOID values cannot be compared.", node.line);
                currentType = BasicType::ERROR;
                return;
            }
            if (node.op != "==" && node.op != "<>") {
                // Check aggiornato: Accetta (Numerici) OPPURE (Stringhe)
                bool isNumeric = (leftT == BasicType::INT || leftT == BasicType::DOUBLE) &&
                                 (rightT == BasicType::INT || rightT == BasicType::DOUBLE);
                bool isString  = leftT == BasicType::STRING && rightT == BasicType::STRING;
                bool isChar    = leftT == BasicType::CHAR && rightT == BasicType::CHAR;

                if (!isNumeric && !isString && !isChar) {
                    error("Relational operator '" + node.op + "' requires both numeric operands, both chars or both strings.", node.line);
                    currentType = BasicType::ERROR;
                    return;
                }
            } else {
                if (leftT != rightT &&
                    !((leftT == BasicType::INT || leftT == BasicType::DOUBLE) &&
                      (rightT == BasicType::INT || rightT == BasicType::DOUBLE))) {
                    error("Equality comparison between incompatible types: " + typeToString(leftT) + " and " + typeToString(rightT), node.line);
                    currentType = BasicType::ERROR;
                    return;
                }
            }
            currentType = BasicType::BOOL;
            return;
        }

        error("Unknown binary operator '" + node.op + "'", node.line);
        currentType = BasicType::ERROR;
    }

    void visit(ReturnNode &node) override {
        if (node.value) {
            node.value->accept(*this);
            if (currentType == BasicType::ERROR) return;
            if (currentType == expectedReturnType) return;
            error("Return type mismatch. Expected: " + typeToString(expectedReturnType) + ", found: " + typeToString(currentType), node.line);
            return;
        }
        if (expectedReturnType != BasicType::VOID) {
            error("Missing return value in non-void function. Expected: " + typeToString(expectedReturnType), node.line);
        }
    }

    void visit(ProgramNode &node) override {
        for (auto &decl : node.globals) {
            decl->accept(*this);
        }
        if (node.mainBlock) {
            node.mainBlock->accept(*this);
        }
    }

    void visit(VarDeclNode &node) override {
        node.type.accept(*this);
        BasicType varType = currentType;
        if (node.initializer) {
            node.initializer->accept(*this);
            if (currentType != varType) {
                error("Invalid initialization for '" + node.name +
                      "'. Expected: " + typeToString(varType) +
                      ", Found: " + typeToString(currentType), node.line);
            }
        }
    }

    void visit(AssignmentNode &node) override {
        SymbolInfo *info = symTable.lookup(node.variableName);
        if (!info) {
            error("Variable not declared: " + node.variableName, node.line);
        } else {
            node.value->accept(*this);
            if (info->type != currentType) {
                error("Invalid assignment to '" + node.variableName +
                      "'. Expected: " + typeToString(info->type) +
                      ", Found: " + typeToString(currentType), node.line);
            }
        }
    }

    void visit(IfNode &node) override {
        node.condition->accept(*this);
        if (currentType != BasicType::BOOL) {
            error("If condition is not of type BOOL, but of type: " + typeToString(currentType), node.line);
        }
        node.thenBranch->accept(*this);
        if (node.elseBranch) node.elseBranch->accept(*this);
    }

    void visit(LoopNode &node) override {
        node.condition->accept(*this);
        if (currentType != BasicType::BOOL) {
            error("Loop condition is not of type BOOL, but of type: " + typeToString(currentType), node.line);
        }
        node.body->accept(*this);
    }

    void visit(BlockNode &node) override {
        for (auto &s : node.statements) s->accept(*this);
    }

    void visit(PrintNode &node) override {
        node.expression->accept(*this);
        if (currentType == BasicType::VOID || currentType == BasicType::ERROR) {
            error("Cannot print expression of type " + typeToString(currentType), node.line);
        }
        currentType = BasicType::VOID;
    }

    void visit(ReadNode &node) override {
        node.variable->accept(*this);
        if (currentType == BasicType::VOID || currentType == BasicType::ERROR) {
            error("Cannot read into variable of type " + typeToString(currentType), node.line);
        }
        currentType = BasicType::VOID;
    }

private:
    void error(const std::string &message, int line) {
        hasError = true;
        std::stringstream ss;
        ss << "SEMANTIC ERROR \033 at line " << line << ": " << message << "\n";
        if (line > 0 && line <= (int)sourceLines.size()) {
            ss << "    " << std::setw(4) << line << " | " << sourceLines[line - 1] << "\n";
        }
        std::string formatted = ss.str();
        errors.push_back(formatted);
    }
};