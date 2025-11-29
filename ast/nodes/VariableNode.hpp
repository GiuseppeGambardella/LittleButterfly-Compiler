#pragma once
#include <string>
#include "ast_node.hpp"
class ASTVisitor; // Forward declaration


class VariableNode : public ASTNode {
    public:
        std::string name;

        VariableNode(const std::string &varName) : name(varName) {}

        void accept(ASTVisitor& visitor) override;
};