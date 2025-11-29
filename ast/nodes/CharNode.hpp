#pragma once
#include <memory>
#include "ast_node.hpp"
class ASTVisitor; // Forward declaration

class CharNode : public ASTNode {
    public:
        char value;

        CharNode(char val) : value(val) {}

        void accept(ASTVisitor& visitor) override;
};