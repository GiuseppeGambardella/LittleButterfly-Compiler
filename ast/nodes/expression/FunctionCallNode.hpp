#pragma once
#include <memory>
#include <vector>
#include <string>
#include "../../ast_node.hpp"

class ASTVisitor; // Forward declaration

class FunctionCallNode : public ASTNode {
public:
    std::string functionName;
    std::vector<std::unique_ptr<ASTNode>> arguments;

    FunctionCallNode(const std::string &name,
                     std::vector<std::unique_ptr<ASTNode>> args)
        : functionName(name), arguments(std::move(args)) {}

    void accept(ASTVisitor &visitor) override;
};