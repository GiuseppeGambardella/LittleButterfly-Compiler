#pragma once
#include "ast_visitor.hpp"

// INCLUDES FOR ALL NODE TYPES (alphabetical order)
#include "nodes/statements/AssignmentNode.hpp"
#include "nodes/expression/BinaryOpNode.hpp"
#include "nodes/BlockNode.hpp"
#include "nodes/expression/BooleanNode.hpp"
#include "nodes/expression/CharNode.hpp"
#include "nodes/expression/FunctionCallNode.hpp"
#include "nodes/statements/FunctionDeclNode.hpp"
#include "nodes/statements/IfNode.hpp"
#include "nodes/statements/LoopNode.hpp"
#include "nodes/expression/NumberNode.hpp"
#include "nodes/statements/PrintNode.hpp"
#include "nodes/ProgramNode.hpp"
#include "nodes/statements/ReadNode.hpp"
#include "nodes/expression/RealNode.hpp"
#include "nodes/statements/ReturnNode.hpp"
#include "nodes/expression/StringNode.hpp"
#include "nodes/expression/TypeNode.hpp"
#include "nodes/expression/UnaryOpNode.hpp"
#include "nodes/statements/VarDeclNode.hpp"
#include "nodes/expression/VariableNode.hpp"
#include "nodes/expression/VoidNode.hpp"

// IMPLEMENTATIONS FOR accept METHODS OF ALL NODE TYPES (alphabetical order)
inline void AssignmentNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void BinaryOpNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void BlockNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void BooleanNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void CharNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void FunctionCallNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void FunctionDeclNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void IfNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void LoopNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void NumberNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void PrintNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void ProgramNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void ReadNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void RealNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void ReturnNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void StringNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void TypeNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void UnaryOpNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void VarDeclNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void VariableNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
inline void VoidNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }
