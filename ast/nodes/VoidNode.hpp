#pragma once
#include <memory>
#include "ast_node.hpp"

class ASTVisitor; // Forward declaration

class VoidNode : public ASTNode {
    public:

        VoidNode() {}

        void accept(ASTVisitor& visitor) override;
};