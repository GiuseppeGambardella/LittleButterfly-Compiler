#pragma once
#include <memory>
#include <string>
#include "../../ast_node.hpp"

class ASTVisitor; // Forward declaration

class BinaryOpNode : public ASTNode {
    public:
        std::string op;
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;

        BinaryOpNode(const std::string &operation,
                     std::unique_ptr<ASTNode> lhs,
                     std::unique_ptr<ASTNode> rhs)
            : op(operation), left(std::move(lhs)), right(std::move(rhs)) {}

        void accept(ASTVisitor &visitor) override;
};
