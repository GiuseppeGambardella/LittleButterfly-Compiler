cmake_minimum_required(VERSION 3.14)
project(LittleButterfly_Compiler)

set(CMAKE_CXX_STANDARD 17)

# --- 1. TROVA I PACCHETTI FLEX E BISON ---
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# --- 2. CONFIGURA BISON (PARSER) ---
# Input: parser/parser.y (Dalla tua cartella parser)
# Output: Genera parser.cpp e parser.hpp nella cartella di build
BISON_TARGET(
        MyParser
        parser/parser.y
        ${CMAKE_CURRENT_BINARY_DIR}/parser/parser.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser/parser.hpp
)

# --- 3. CONFIGURA FLEX (LEXER) ---
# Input: lexer/lexer.l (Dalla tua cartella lexer)
# Output: Genera scanner.cpp nella cartella di build
FLEX_TARGET(
        MyScanner
        lexer/lexer.l
        ${CMAKE_CURRENT_BINARY_DIR}/lexer/scanner.cpp
)

# Assicura che il Lexer venga generato DOPO il Parser (per avere parser.hpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

# --- 4. GESTIONE INCLUDE ---
include_directories(
        .                           # Include la root (per main.cpp)
        ast
        ast/nodes
        ast/helper
        lexer
        parser
        ${CMAKE_CURRENT_BINARY_DIR}/parser  # Per trovare parser.hpp generato
        ${CMAKE_CURRENT_BINARY_DIR}/lexer   # Per trovare scanner.cpp generato
)

# --- 5. FILE SORGENTE AST ---
file(GLOB AST_NODES "ast/nodes/*.hpp")

# --- 6. CREA L'ESEGUIBILE ---
add_executable(LittleButterfly_Compiler
        # File Generati
        ${BISON_MyParser_OUTPUTS}
        ${FLEX_MyScanner_OUTPUTS}

        # Main (Nella root)
        main.cpp

        # File AST Core
        ast/ast_factory.hpp
        ast/ast_node.hpp
        ast/ast_visitor.hpp
        ast/nodes_impl.hpp
        ast/print_visitor.hpp
        ast/helper/helper.h

        # Nodi AST
        ${AST_NODES}
)