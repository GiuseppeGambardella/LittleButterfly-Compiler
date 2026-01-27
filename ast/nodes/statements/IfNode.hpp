#pragma once
#include <memory>
#include "../../ast_node.hpp"

class ASTVisitor; // Forward declaration

class IfNode : public ASTNode {
    public:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> thenBranch;
        std::unique_ptr<ASTNode> elseBranch; // nullable

        IfNode(std::unique_ptr<ASTNode> cond,
               std::unique_ptr<ASTNode> thenBr,
               std::unique_ptr<ASTNode> elseBr = nullptr)
            : condition(std::move(cond)), thenBranch(std::move(thenBr)), elseBranch(std::move(elseBr)) {}

        void accept(ASTVisitor& visitor) override;
};