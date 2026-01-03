#pragma once
#include <memory>
#include <vector>
#include "../ast_node.hpp"

class ASTVisitor; // Forward declaration

class BlockNode : public ASTNode {
    public:
        std::vector<std::unique_ptr<ASTNode>> statements;

        BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts)
            : statements(std::move(stmts)) {}

        void accept(ASTVisitor &visitor) override;
};