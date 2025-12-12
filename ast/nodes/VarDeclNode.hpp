#pragma once
#include <memory>
#include <string>
#include "../ast_node.hpp"
#include "TypeNode.hpp"

class ASTVisitor; // Forward declaration

class VarDeclNode : public ASTNode {
public:
    TypeNode type;
    std::string name;
    std::unique_ptr<ASTNode> initializer; // nullable

    VarDeclNode(const TypeNode& varType, const std::string& varName, std::unique_ptr<ASTNode> init = nullptr)
        : type(varType), name(varName), initializer(std::move(init)) {}

    void accept(ASTVisitor& visitor) override;
};