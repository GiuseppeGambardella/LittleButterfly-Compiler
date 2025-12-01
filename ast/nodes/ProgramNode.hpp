#pragma once
#include <memory>
#include <vector>
#include "../ast_node.hpp"

class ASTVisitor; // Forward declaration

class ProgramNode: public ASTNode {
    public:
        std::vector<std::unique_ptr<ASTNode>> globals; // vettore di nodi per dichiarazioni globali
        std::unique_ptr<ASTNode> mainBlock;

        void accept(ASTVisitor& visitor) override;
};