#pragma once
/*
    * @file print_visitor.hpp
    * @brief Definizione di un visitor per stampare i nodi dell'AST.
    */
#include "ast_visitor.hpp"
#include "nodes_impl.hpp"
#include <iostream>
#include "helper/helper.h"

class PrintVisitor : public ASTVisitor {
    public:
        void visit(AssignmentNode& node) override {
            std::cout << "Assignment to variable: " << node.variableName << std::endl;
            std::cout << "Value:" << std::endl;
            node.value->accept(*this);
        }

        void visit(BinaryOpNode& node) override {
            std::cout << "Binary Operation: " << node.op << std::endl;
            std::cout << "Left operand:" << std::endl;
            node.left->accept(*this);
            std::cout << "Right operand:" << std::endl;
            node.right->accept(*this);
        }

        void visit(BlockNode& node) override {
            std::cout << "Block:" << std::endl;
            for (auto& stmt : node.statements) {
                stmt->accept(*this);
            }
        }

        void visit(BooleanNode &node) override {
            std::cout << "Boolean: " << (node.value ? "true" : "false") << std::endl;
        }

        void visit(CharNode& node) override {
            std::cout << "Char: " << node.value << std::endl;
        }

        void visit(FunctionCallNode& node) override {
            std::cout << "Function call: " << node.functionName << std::endl;
            std::cout << "Arguments:" << std::endl;
            for (auto& arg : node.arguments) {
                arg->accept(*this);
            }
        }

        void visit(FunctionDeclNode& node) override {
            std::cout << "Function definition: " << node.name << std::endl;
            std::cout << "Parameters: ";
            for (auto& param : node.parameters) {
                std::cout << param.name << " : ";
                param.type.accept(*this);
                std::cout << "; ";
            }
            std::cout << std::endl;
            std::cout << "Return type:" << std::endl;
            node.returnType->accept(*this);
            std::cout << std::endl;
            std::cout << "Body:" << std::endl;
            node.body->accept(*this);
        }

        void visit(IfNode& node) override {
            std::cout << "If statement:" << std::endl;
            std::cout << "Condition:" << std::endl;
            node.condition->accept(*this);
            std::cout << "Then branch:" << std::endl;
            node.thenBranch->accept(*this);
            if (node.elseBranch) {
                std::cout << "Else branch:" << std::endl;
                node.elseBranch->accept(*this);
            }
        }

        void visit(LoopNode& node) override {
            std::cout << "While loop:" << std::endl;
            std::cout << "Condition:" << std::endl;
            node.condition->accept(*this);
            std::cout << "Body:" << std::endl;
            node.body->accept(*this);
        }

        void visit(NumberNode& node) override {
            std::cout << "Number: " << node.value << std::endl;
        }

        void visit(PrintNode& node) override {
            std::cout << "Print statement:" << std::endl;
            node.expression->accept(*this);
        }

        void visit(ProgramNode& node) override {
            std::cout << "Program:" << std::endl;
            for (auto& stmt : node.globals) {
                stmt->accept(*this);
            }
            std::cout << "Main Block:" << std::endl;
            node.mainBlock->accept(*this);
        }

        void visit(ReadNode& node) override {
            std::cout << "Read statement for variable:" << std::endl;
            node.variable->accept(*this);
        }

        void visit(RealNode& node) override {
            std::cout << "Real: " << node.value << std::endl;
        }

        void visit(ReturnNode& node) override {
            std::cout << "Return statement:" << std::endl;
            node.value->accept(*this);
        }

        void visit(StringNode& node) override {
            std::cout << "String: " << node.value << std::endl;
        }

        void visit(TypeNode& node) override {
            std::cout << "Type: " << typeToString(node.type) << std::endl; //Funzione per ritornare il tipo in Stringa dall'enum
        }

        void visit(UnaryOpNode& node) override {
            std::cout << "Unary Operation: " << node.op << std::endl;
            std::cout << "Operand:" << std::endl;
            node.operand->accept(*this);
        }

        void visit(VarDeclNode &node) override {
            std::cout << "Variable Declaration: " << node.name << std::endl;
            std::cout << "Type:" << std::endl;
            node.type.accept(*this);
            if (node.initializer) {
                node.initializer->accept(*this);
            }
        }

        void visit(VariableNode& node) override {
            std::cout << "Variable: " << node.name << std::endl;
        }

        void visit(VoidNode &node) override {
            std::cout << "Void Node" << std::endl;
        }
};
