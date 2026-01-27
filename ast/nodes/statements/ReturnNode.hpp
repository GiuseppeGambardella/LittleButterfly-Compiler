#pragma once
#include <memory>
#include "../../ast_node.hpp"
class ASTVisitor; // Forward declaration


class ReturnNode : public ASTNode {
    public:
        std::unique_ptr<ASTNode> value; // nullable

        ReturnNode(std::unique_ptr<ASTNode> val = nullptr)
            : value(std::move(val)) {}

        void accept(ASTVisitor &visitor) override;
};