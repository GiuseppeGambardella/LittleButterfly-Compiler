#pragma once
#include "../ast/ast_visitor.hpp"
#include "../ast/helper/helper.h"
#include "../ast/nodes_impl.hpp"
#include "SymbolTable.hpp"
#include <iostream>

class SemanticCheckVisitor : public ASTVisitor {
  SymbolTable &symTable;
  BasicType currentType = BasicType::VOID;
  std::vector<std::string> errors;
  bool hasError = false;
  BasicType expectedReturnType = BasicType::VOID;

public:
  explicit SemanticCheckVisitor(SymbolTable &symbols) : symTable(symbols) {}

  [[nodiscard]] const auto &getErrors() const { return errors; }

  void visit(RealNode &node) override { currentType = BasicType::DOUBLE; }

  void visit(NumberNode &node) override { currentType = BasicType::INT; }

  void visit(StringNode &node) override { currentType = BasicType::STRING; }

  void visit(BooleanNode &node) override { currentType = BasicType::BOOL; }

  void visit(CharNode &node) override { currentType = BasicType::CHAR; }

  void visit(VoidNode &node) override { currentType = BasicType::VOID; }

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
    // Esempio: se l'operatore è "!" e l'operando è BOOL, il risultato è BOOL
    if (node.op == "!") {
      if (currentType != BasicType::BOOL) {
        error("Operator '!' applied to non-boolean type: " +
                  typeToString(currentType),
              node.line);
      }
      currentType = BasicType::BOOL;
      return;
    }
    // Altri operatori unari possono essere gestiti qui
    // Esempio: operatore "-" per numeri
    if (node.op == "-") {
      if (currentType != BasicType::INT && currentType != BasicType::DOUBLE) {
        error("Operator '-' applied to non-numeric type: " +
                  typeToString(currentType),
              node.line);
      }
      // Il tipo rimane lo stesso dell'operando
      return;
    }
  }

  void visit(BinaryOpNode &node) override {
    // 1. Evaluate operands
    node.left->accept(*this);
    BasicType leftT = currentType;

    node.right->accept(*this);
    BasicType rightT = currentType;

    // --- 1. GESTIONE CONCATENAZIONE (&) ---
    if (node.op == "&") {
      // L'operatore & accetta tutto basta che non sia VOID
      if (leftT == BasicType::VOID || rightT == BasicType::VOID) {
        error("Cannot concatenate VOID types.", node.line);
        currentType = BasicType::VOID;
      } else {
        // Il risultato è SEMPRE una stringa (es. 10 & 20 -> "1020")
        currentType = BasicType::STRING;
      }
      return; // Esce subito.
    }

    // --- 2. GESTIONE ARITMETICA (+, -, *, /) ---
    if (node.op == "+" || node.op == "-" || node.op == "*" || node.op == "/") {
      // Qui entriamo SOLO se l'operatore è matematico
      if ((leftT == BasicType::INT || leftT == BasicType::DOUBLE) &&
          (rightT == BasicType::INT || rightT == BasicType::DOUBLE)) {

        // Type promotion: se c'è un DOUBLE, il risultato è DOUBLE
        currentType =
            (leftT == BasicType::DOUBLE || rightT == BasicType::DOUBLE)
                ? BasicType::DOUBLE
                : BasicType::INT;
        return;
          }

      error("Invalid operands for arithmetic operator '" + node.op +
                "': " + typeToString(leftT) + " and " + typeToString(rightT),
            node.line);
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

      error("Logical operators require boolean operands.", node.line);
      currentType = BasicType::ERROR;
      return;
    }

    /* ===============================
       COMPARISON OPERATORS
       ==  <>  <  >  <=  >=
       =============================== */
    if (node.op == "==" || node.op == "<>" || node.op == "<" ||
        node.op == ">" || node.op == "<=" || node.op == ">=") {
      // VOID cannot be compared
      if (leftT == BasicType::VOID || rightT == BasicType::VOID) {
        error("VOID values cannot be compared.", node.line);
        currentType = BasicType::ERROR;
        return;
      }

      // Ordering comparisons (<, >, <=, >=) → numeric only
      if (node.op != "==" && node.op != "<>") {
        if (!((leftT == BasicType::INT || leftT == BasicType::DOUBLE) &&
              (rightT == BasicType::INT || rightT == BasicType::DOUBLE))) {
          error("Relational operator '" + node.op +
                    "' requires numeric operands.",
                node.line);
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
                    typeToString(leftT) + " and " + typeToString(rightT),
                node.line);
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
    error("Unknown binary operator '" + node.op + "'", node.line);
    currentType = BasicType::ERROR;
  }

  void visit(FunctionCallNode &node) override {
    SymbolInfo *info = symTable.lookup(node.functionName);
    if (!info) {
      error("Function not declared: " + node.functionName, node.line);
      currentType = BasicType::VOID; // TO DO (some error type?)
      return;
    }

    if (!info->isFunction) {
      error("'" + node.functionName + "' is not a function", node.line);
      currentType = BasicType::VOID; // TO DO (some error type?)
      return;
    }

    // Controlla i tipi degli argomenti
    if (node.arguments.size() != info->paramTypes.size()) {
      error("Wrong number of arguments for function '" + node.functionName +
                "'. Expected: " + std::to_string(info->paramTypes.size()) +
                ", Found: " + std::to_string(node.arguments.size()),
            node.line);
    } else {
      for (size_t i = 0; i < node.arguments.size(); ++i) {
        node.arguments[i]->accept(*this);
        if (currentType != info->paramTypes[i]) {
          error("Wrong type for argument " + std::to_string(i + 1) +
                    " of function '" + node.functionName +
                    "'. Expected: " + typeToString(info->paramTypes[i]) +
                    ", Found: " + typeToString(currentType),
                node.arguments[i]->line);
        }
      }
    }
    currentType = info->type; // Il tipo dell'espressione è il tipo di ritorno
                              // della funzione
  }

  void visit(FunctionDeclNode &node) override {
    node.returnType->accept(*this);
    expectedReturnType = currentType;
    // Controlla il corpo della funzione
    if (node.body) {
      node.body->accept(*this);
    }
  }

  void visit(ReturnNode &node) override {
    // Case: return <expression>;
    if (node.value) {
      node.value->accept(*this);

      // Avoid cascading errors
      if (currentType == BasicType::ERROR)
        return;

      // Not allow INT -> REAL promotion
      if (currentType == expectedReturnType)
        return;

      error("Return type mismatch. Expected: " +
                typeToString(expectedReturnType) +
                ", found: " + typeToString(currentType),
            node.line);
      return;
    }

    // Case: return;
    if (expectedReturnType != BasicType::VOID) {
      error("Missing return value in non-void function. Expected: " +
                typeToString(expectedReturnType),
            node.line);
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

    // Se c'è un'inizializzazione, controlla il tipo
    if (node.initializer) {
      node.initializer->accept(*this);
      if (currentType != varType) {
        error("Invalid initialization for '" + node.name +
                  "'. Expected: " + typeToString(varType) +
                  ", Found: " + typeToString(currentType),
              node.line);
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
                  ", Found: " + typeToString(currentType),
              node.line);
      }
    }
  }

  void visit(IfNode &node) override {
    node.condition->accept(*this);
    if (currentType != BasicType::BOOL) {
      error("If condition is not of type BOOL, but of type: " +
                typeToString(currentType),
            node.line);
    }
    node.thenBranch->accept(*this);
    if (node.elseBranch)
      node.elseBranch->accept(*this);
  }

  void visit(LoopNode &node) override {
    node.condition->accept(*this);
    if (currentType != BasicType::BOOL) {
      error("Loop condition is not of type BOOL, but of type: " +
                typeToString(currentType),
            node.line);
    }
    node.body->accept(*this);
  }

  void visit(BlockNode &node) override {
    for (auto &s : node.statements)
      s->accept(*this);
  }

  void visit(PrintNode &node) override {
    node.expression->accept(*this);

    if (currentType == BasicType::VOID || currentType == BasicType::ERROR) {

      error("Cannot print expression of type " + typeToString(currentType),
            node.line);
    }

    currentType = BasicType::VOID;
  }

  void visit(ReadNode &node) override {
    node.variable->accept(*this);

    if (currentType == BasicType::VOID || currentType == BasicType::ERROR) {

      error("Cannot read into variable of type " + typeToString(currentType),
            node.line);
    }

    currentType = BasicType::VOID;
  }

  void visit(TypeNode &node) override { currentType = node.type; }

  private:
    void error(const std::string &message, int line) {
      hasError = true;
      errors.push_back("Line " + std::to_string(line) + ": " + message);
      std::cerr << "Semantic Error: Line " << line << ": " << message
                << std::endl;
    }
};