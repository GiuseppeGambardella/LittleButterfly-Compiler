#pragma once
#include <memory>
#include <string>
#include "ast_node.hpp"
#include "TypeNode.hpp"

class ASTVisitor; // Forward declaration

class VarDeclNode : public ASTNode {
public:
    TypeNode type;
    std::string name;
    std::unique_ptr<ASTNode> initializer; // nullable

    void accept(ASTVisitor& visitor) override;
};