#pragma once
//Forward declarations of AST node types (alphabetical order)
class AssignmentNode;
class BinaryOpNode;
class BlockNode;
class BooleanNode;
class CharNode;
class FunctionCallNode;
class FunctionDeclNode;
class IfNode;
class LoopNode;
class NumberNode;
class PrintNode;
class ProgramNode;
class ReadNode;
class RealNode;
class ReturnNode;
class StringNode;
class TypeNode;
class UnaryOpNode;
class VarDeclNode;
class VariableNode;
class voidNode;
//**INSERT ABOVE if missing**/

class ASTVisitor {
    public:
        virtual ~ASTVisitor() = default;

        virtual void visit(AssignmentNode &node) = 0;
        virtual void visit(BinaryOpNode &node) = 0;
        virtual void visit(BlockNode &node) = 0;
        virtual void visit(BooleanNode &node) = 0;
        virtual void visit(CharNode &node) = 0;
        virtual void visit(FunctionCallNode &node) = 0;
        virtual void visit(FunctionDeclNode &node) = 0;
        virtual void visit(IfNode &node) = 0;
        virtual void visit(LoopNode &node) = 0;
        virtual void visit(NumberNode &node) = 0;
        virtual void visit(PrintNode &node) = 0;
        virtual void visit(ProgramNode &node) = 0;
        virtual void visit(ReadNode &node) = 0;
        virtual void visit(RealNode &node) = 0;
        virtual void visit(ReturnNode &node) = 0;
        virtual void visit(StringNode &node) = 0;
        virtual void visit(TypeNode &node) = 0;
        virtual void visit(UnaryOpNode &node) = 0;
        virtual void visit(VarDeclNode &node) = 0;
        virtual void visit(VariableNode &node) = 0;
        virtual void visit(VoidNode &node) = 0;
};