#pragma once
#include <memory>
#include <vector>
#include "../ast_node.hpp"

class ASTVisitor; // Forward declaration

class ProgramNode: public ASTNode {
    public:
        std::vector<std::unique_ptr<ASTNode>> globals; // nodes vector for global declarations
        std::unique_ptr<ASTNode> mainBlock;

        ProgramNode(std::vector<std::unique_ptr<ASTNode>> globals, std::unique_ptr<ASTNode> mainBlock)
            : globals(std::move(globals)), mainBlock(std::move(mainBlock)) {}

        void accept(ASTVisitor& visitor) override;
};