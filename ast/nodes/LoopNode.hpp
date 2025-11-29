#pragma once
#include <memory>
#include "ast_node.hpp"
class ASTVisitor; // Forward declaration

class LoopNode : public ASTNode {
    public:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> body;

        LoopNode(std::unique_ptr<ASTNode> cond,
                  std::unique_ptr<ASTNode> bdy)
            : condition(std::move(cond)), body(std::move(bdy)) {}

        void accept(ASTVisitor& visitor) override;
};