cmake_minimum_required(VERSION 4.0)
project(LittleButterfly_Compiler)

set(CMAKE_CXX_STANDARD 17)

include_directories(ast)
include_directories(ast/nodes)
include_directories(lexer)
include_directories(parser)

add_executable(LittleButterfly_Compiler
        ast/nodes/AssignmentNode.hpp
        ast/nodes/BinaryOpNode.hpp
        ast/nodes/BlockNode.hpp
        ast/nodes/BooleanNode.hpp
        ast/nodes/CharNode.hpp
        ast/nodes/FunctionCallNode.hpp
        ast/nodes/FunctionDeclNode.hpp
        ast/nodes/IfNode.hpp
        ast/nodes/LoopNode.hpp
        ast/nodes/NumberNode.hpp
        ast/nodes/PrintNode.hpp
        ast/nodes/ProgramNode.hpp
        ast/nodes/ReadNode.hpp
        ast/nodes/RealNode.hpp
        ast/nodes/ReturnNode.hpp
        ast/nodes/StringNode.hpp
        ast/nodes/TypeNode.hpp
        ast/nodes/UnaryOpNode.hpp
        ast/nodes/VarDeclNode.hpp
        ast/nodes/VariableNode.hpp
        ast/nodes/VoidNode.hpp
        ast/ast_factory.hpp
        ast/ast_node.hpp
        ast/ast_visitor.hpp
        ast/nodes_impl.hpp
        ast/print_visitor.hpp
        lexer/lex.yy.cc
        lexer/main.cpp
        lexer/tokens.hpp
        parser/parser.hpp
        parser/parser2.tab.c
        ast/helper/helper.h
        lexer/scanner.cpp)
