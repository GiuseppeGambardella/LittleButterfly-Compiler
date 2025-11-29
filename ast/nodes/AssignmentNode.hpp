#pragma once
#include <memory>
#include <string>
#include "ast_node.hpp"

class ASTVisitor; // Forward declaration

class AssignmentNode : public ASTNode {
    public:
        std::string variableName;
        std::unique_ptr<ASTNode> value;

        AssignmentNode(const std::string &varName,
                       std::unique_ptr<ASTNode> val)
            : variableName(varName), value(std::move(val)) {}

        void accept(ASTVisitor& visitor) override;
};