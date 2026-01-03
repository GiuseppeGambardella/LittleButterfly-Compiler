#pragma once
#include <memory>
#include "../ast_node.hpp"
class ASTVisitor; // Forward declaration

class RealNode : public ASTNode {
    public:
        double value;

        RealNode(double val) : value(val) {}

        void accept(ASTVisitor& visitor) override;
};