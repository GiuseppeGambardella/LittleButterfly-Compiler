#pragma once
#include <memory>
#include "ast_node.hpp"

class ASTVisitor; // Forward declaration

class NumberNode : public ASTNode {
    public:
        int value;

        NumberNode(int val) : value(val) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};