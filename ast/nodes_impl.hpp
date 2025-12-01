#pragma once
#include "ast_visitor.hpp"

// INCLUDES FOR ALL NODE TYPES (alphabetical order)
#include "nodes/AssignmentNode.hpp"
#include "nodes/BinaryOpNode.hpp"
#include "nodes/BlockNode.hpp"
#include "nodes/BooleanNode.hpp"
#include "nodes/CharNode.hpp"
#include "nodes/FunctionCallNode.hpp"
#include "nodes/FunctionDeclNode.hpp"
#include "nodes/IfNode.hpp"
#include "nodes/LoopNode.hpp"
#include "nodes/NumberNode.hpp"
#include "nodes/PrintNode.hpp"
#include "nodes/ProgramNode.hpp"
#include "nodes/ReadNode.hpp"
#include "nodes/RealNode.hpp"
#include "nodes/ReturnNode.hpp"
#include "nodes/StringNode.hpp"
#include "nodes/TypeNode.hpp"
#include "nodes/UnaryOpNode.hpp"
#include "nodes/VarDeclNode.hpp"
#include "nodes/VariableNode.hpp"
#include "nodes/VoidNode.hpp"

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
