#pragma once
#include <memory>
class ASTVisitor; // Forward declaration

class ASTNode {
public:
    int line = 0;
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor &visitor) = 0;
};