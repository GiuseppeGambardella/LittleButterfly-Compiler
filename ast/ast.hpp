#pragma once
#include <memory>
class ASTVisitor; // Forward declaration

class ASTNode{
    public:
        virtual ~ASTNode() = default;
        virtual void accept(class ASTVisitor &visitor) = 0;
}