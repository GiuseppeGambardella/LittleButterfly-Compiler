#pragma once
#include "../../ast_node.hpp"

class ASTVisitor; // Forward declaration

class BooleanNode : public ASTNode {
    public:
        bool value;

        BooleanNode(bool val) : value(val) {}

        void accept(ASTVisitor& visitor) override;
};