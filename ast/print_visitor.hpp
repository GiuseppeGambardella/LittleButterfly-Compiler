/**
 * @file print_visitor.hpp
 * @brief Definizione di un visitor per stampare i nodi dell'AST.
 */
class PrintVisitor : public ASTVisitor {
    public:
        void visit(NumberNode& node) override {
            std::cout << "Number: " << node.value << std::endl;
        }

        void visit(StringNode& node) override {
            std::cout << "String: " << node.value << std::endl;
        }

        void visit(BinaryOpNode& node) override {
            std::cout << "Binary Operation: " << node.op << std::endl;
            std::cout << "Left operand:" << std::endl;
            node.left->accept(*this);
            std::cout << "Right operand:" << std::endl;
            node.right->accept(*this);
        }

        // Aggiungi altri metodi visit per i diversi tipi di nodi AST
};
