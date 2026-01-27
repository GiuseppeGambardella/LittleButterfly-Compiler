#pragma once
#include <memory>
#include <string>
#include "../../ast_node.hpp"

class ASTVisitor; // Forward declaration

class UnaryOpNode : public ASTNode {
    public:
        std::string op;
        std::unique_ptr<ASTNode> operand;

        UnaryOpNode(const std::string &operation,
                    std::unique_ptr<ASTNode> oprnd)
            : op(operation), operand(std::move(oprnd)) {}

        void accept(ASTVisitor &visitor) override;
};