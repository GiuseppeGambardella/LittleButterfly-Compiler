#pragma once
#include <memory>
#include "../../ast_node.hpp"

class ASTVisitor; // Forward declaration

class PrintNode : public ASTNode {
    public:
        std::unique_ptr<ASTNode> expression;

        PrintNode(std::unique_ptr<ASTNode> expr)
            : expression(std::move(expr)) {}

        void accept(ASTVisitor &visitor) override;
};