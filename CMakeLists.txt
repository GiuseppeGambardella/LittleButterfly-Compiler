cmake_minimum_required(VERSION 3.14)
project(LittleButterfly_Compiler)

set(CMAKE_CXX_STANDARD 17)

# --- 1. TROVA I PACCHETTI FLEX E BISON ---
# Trova Bison automaticamente (prova prima Homebrew, poi sistema)
if(APPLE)
    # Su macOS, prova prima Homebrew
    find_program(BISON_EXECUTABLE 
        NAMES bison
        PATHS /opt/homebrew/opt/bison/bin /usr/local/opt/bison/bin
        NO_DEFAULT_PATH
    )
endif()

# Se non trovato, usa quello di sistema
if(NOT BISON_EXECUTABLE)
    find_package(BISON REQUIRED)
else()
    find_package(BISON REQUIRED)
    message(STATUS "Using Bison: ${BISON_EXECUTABLE}")
endif()

find_package(FLEX REQUIRED)

# --- 2. CONFIGURA BISON (PARSER) ---
# Output: Genera parser.cpp e parser.hpp nella BUILD DIRECTORY
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Genera i file nella BUILD directory (non SOURCE)
BISON_TARGET(
        MyParser
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/parser.y
        ${CMAKE_CURRENT_BINARY_DIR}/generated/parser.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/generated/parser.hpp
)

# Diciamo a Flex: "Salva scanner.cpp DENTRO build/generated"
FLEX_TARGET(
        MyScanner
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/lexer.l
        ${CMAKE_CURRENT_BINARY_DIR}/generated/scanner.cpp
)

# Assicura che il Lexer venga generato DOPO il Parser (per avere parser.hpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

# --- 3. GESTIONE INCLUDE ---
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}              # Root del progetto
        ${CMAKE_CURRENT_SOURCE_DIR}/ast
        ${CMAKE_CURRENT_SOURCE_DIR}/ast/nodes
        ${CMAKE_CURRENT_SOURCE_DIR}/ast/helper
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/semantic
        ${CMAKE_CURRENT_BINARY_DIR}/generated    # File generati nella BUILD dir
)

# --- 4. FILE SORGENTE AST ---
file(GLOB AST_NODES "ast/nodes/*.hpp")

# --- 5. CREA L'ESEGUIBILE ---
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

        # File Symbol Table
        semantic/SymbolTable.hpp

        # Nodi AST
        ${AST_NODES}
        lib/FlexLexer.h
        ast/symbol_table_visitor.hpp
)