#pragma once
#include <memory>
#include "ast_node.hpp"

class ASTVisitor; // Forward declaration

class ReadNode : public ASTNode {
    public:
        std::unique_ptr<ASTNode> variable;

        ReadNode(std::unique_ptr<ASTNode> var)
            : variable(std::move(var)) {}

        void accept(ASTVisitor &visitor) override;
};