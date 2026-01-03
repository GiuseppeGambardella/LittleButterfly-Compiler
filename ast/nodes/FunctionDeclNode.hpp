#pragma once
#include <memory>
#include <vector>
#include <string>
#include "../ast_node.hpp"

class ASTVisitor; // Forward declaration

class FunctionDeclNode : public ASTNode {
    public:
        std::string name;
        std::vector<std::unique_ptr<ASTNode>> parameters;
        std::unique_ptr<ASTNode> returnType;
        std::unique_ptr<ASTNode> body;

        FunctionDeclNode( const std::string &n,
                          std::vector<std::unique_ptr<ASTNode>> params,
                          std::unique_ptr<ASTNode> retType,
                          std::unique_ptr<ASTNode> b)
            : name(n), parameters(std::move(params)), returnType(std::move(retType)), body(std::move(b)) {}

        void accept(ASTVisitor& visitor) override;
};