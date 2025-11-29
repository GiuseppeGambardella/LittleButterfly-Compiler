//AST NODES DEFINITION

#pragma once
#include <memory>
#include <vector>
#include <string>
#include "ast_node.hpp"



/*NODO RADICE DELL'AST*/
class ProgramNode: public ASTNode {
    public:
        std::vector<std::unique_ptr<ASTNode>> globals; // vettore di nodi per dichiarazioni globali
        std::unique_ptr<ASTNode> mainBlock;

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

//NODI PER DICHIARAZIONI DI FUNZIONI E VARIABILI
class FunctionDeclNode : public ASTNode {
    public:
        std::string name;
        std::vector<std::unique_ptr<ASTNode>> parameters;
        std::unique_ptr<ASTNode> returnType;
        std::unique_ptr<ASTNode> body;

        FunctionDeclNode( const std::string &n,
                          std::vector<std::unique_ptr<ASTNode>> params,
                          std::unique_ptr<ASTNode> retType,
                          std::unique_ptr<ASTNode> b)
            : name(n), parameters(std::move(params)), returnType(std::move(retType)), body(std::move(b)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class VarDeclNode : public ASTNode {
public:
    TypeNode type;
    std::string name;
    std::unique_ptr<ASTNode> initializer; // nullable

    void accept(ASTVisitor &visitor) override {
        visitor.visit(*this);
    }
};

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

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

//operation nodes
class BinaryOpNode : public ASTNode {
    public:
        std::string op;
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;

        BinaryOpNode(const std::string &operation,
                     std::unique_ptr<ASTNode> lhs,
                     std::unique_ptr<ASTNode> rhs)
            : op(operation), left(std::move(lhs)), right(std::move(rhs)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class UnaryOpNode : public ASTNode {
    public:
        std::string op;
        std::unique_ptr<ASTNode> operand;

        UnaryOpNode(const std::string &operation,
                    std::unique_ptr<ASTNode> oprnd)
            : op(operation), operand(std::move(oprnd)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class FunctionCallNode : public ASTNode {
    public:
        std::string functionName;
        std::vector<std::unique_ptr<ASTNode>> arguments;

        FunctionCallNode(const std::string &name,
                         std::vector<std::unique_ptr<ASTNode>> args)
            : functionName(name), arguments(std::move(args)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class AssignmentNode : public ASTNode {
    public:
        std::string variableName;
        std::unique_ptr<ASTNode> value;

        AssignmentNode(const std::string &varName,
                       std::unique_ptr<ASTNode> val)
            : variableName(varName), value(std::move(val)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};


/*NODI PER VALORI CONSTANTI DEI TIPI DI BASE*/

class NumberNode : public ASTNode {
    public:
        int value;

        NumberNode(int val) : value(val) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class RealNode : public ASTNode {
    public:
        double value;

        RealNode(double val) : value(val) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class StringNode : public ASTNode {
    public:
        std::string value;

        StringNode(const std::string &val) : value(val) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class BooleanNode : public ASTNode {
    public:
        bool value;

        BooleanNode(bool val) : value(val) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class CharNode : public ASTNode {
    public:
        char value;

        CharNode(char val) : value(val) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class VoidNode : public ASTNode {
    public:

        VoidNode() {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

//VARIABLE NODE
class VariableNode : public ASTNode {
    public:
        std::string name;

        VariableNode(const std::string &varName) : name(varName) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

//CONTROL FLOW NODES
class IfNode : public ASTNode {
    public:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> thenBranch;
        std::unique_ptr<ASTNode> elseBranch; // nullable

        IfNode(std::unique_ptr<ASTNode> cond,
               std::unique_ptr<ASTNode> thenBr,
               std::unique_ptr<ASTNode> elseBr = nullptr)
            : condition(std::move(cond)), thenBranch(std::move(thenBr)), elseBranch(std::move(elseBr)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class WhileNode : public ASTNode {
    public:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> body;

        WhileNode(std::unique_ptr<ASTNode> cond,
                  std::unique_ptr<ASTNode> bdy)
            : condition(std::move(cond)), body(std::move(bdy)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class ReturnNode : public ASTNode {
    public:
        std::unique_ptr<ASTNode> value; // nullable

        ReturnNode(std::unique_ptr<ASTNode> val = nullptr)
            : value(std::move(val)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

class BlockNode : public ASTNode {
    public:
        std::vector<std::unique_ptr<ASTNode>> statements;

        BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts)
            : statements(std::move(stmts)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};

//Aggiungi altri nodi AST secondo necessità
class PrintNode : public ASTNode {
    public:
        std::unique_ptr<ASTNode> expression;

        PrintNode(std::unique_ptr<ASTNode> expr)
            : expression(std::move(expr)) {}

        void accept(ASTVisitor &visitor) override {
            visitor.visit(*this);
        }
};