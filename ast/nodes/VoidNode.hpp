#pragma once
#include <memory>
#include "../ast_node.hpp"

class ASTVisitor; // Forward declaration

class VoidNode : public ASTNode {
    public:

        VoidNode() = default;

        void accept(ASTVisitor& visitor) override;
};