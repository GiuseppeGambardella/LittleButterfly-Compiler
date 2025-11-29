#pragma once
//Forward declarations of AST node types

//**INSERT ABOVE**/

class ASTVisitor {
    public:
        virtual void visit(NumberNode&) = 0;
        virtual void visit(StringNode&) = 0;
        virtual void visit(RealNode&) = 0;
        virtual void visit(BooleanNode&) = 0;
        virtual void visit(charNode&) = 0;
        virtual void visit(voidNode&) = 0;
        // Aggiungi altri metodi visit per i diversi tipi di nodi AST
}