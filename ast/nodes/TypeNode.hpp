#pragma once
#include <memory>
#include "ast_node.hpp"

class ASTVisitor; // Forward declaration

enum class BasicType {
    INT,
    DOUBLE,
    STRING,
    BOOL,
    CHAR,
    VOID
};

class TypeNode : public ASTNode {
    public:
        BasicType type;

        TypeNode(BasicType t) : type(t) {}

        void accept(ASTVisitor& visitor) override;
};