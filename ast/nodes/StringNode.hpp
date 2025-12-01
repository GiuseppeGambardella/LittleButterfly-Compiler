#pragma once
#include <string>
#include "../ast_node.hpp"

class ASTVisitor; // Forward declaration

class StringNode : public ASTNode {
    public:
        std::string value;

        StringNode(const std::string &val) : value(val) {}

        void accept(ASTVisitor& visitor) override;
};