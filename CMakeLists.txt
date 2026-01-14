cmake_minimum_required(VERSION 3.14)
project(LittleButterfly_Compiler)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/Zc:__cplusplus /wd4005 /wd4065)
endif()


# =========================================================
# 1. FIND PACKAGES
# =========================================================

# --- 1.1 Bison & Flex ---
if(APPLE)
    find_program(BISON_EXECUTABLE
            NAMES bison
            PATHS /opt/homebrew/opt/bison/bin /usr/local/opt/bison/bin
            NO_DEFAULT_PATH
    )
endif()

if(NOT BISON_EXECUTABLE)
    find_package(BISON REQUIRED)
else()
    find_package(BISON REQUIRED)
    message(STATUS "Using Bison: ${BISON_EXECUTABLE}")
endif()

find_package(FLEX REQUIRED)

# --- 1.2 LLVM & MLIR ---
find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
include(AddMLIR)
include(AddLLVM)


# =========================================================
# 2. GLOBAL CONFIGURATION & INCLUDES
# =========================================================

# --- 2.1 LLVM/MLIR Setup ---
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# --- 2.2 Flex/Bison Definitions ---
add_compile_definitions(YY_NO_UNISTD_H)

# --- 2.3 Project Includes ---
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/ast
        ${CMAKE_CURRENT_SOURCE_DIR}/ast/nodes
        ${CMAKE_CURRENT_SOURCE_DIR}/ast/helper
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/semantic
        ${CMAKE_CURRENT_BINARY_DIR}/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/mlir
)

# =========================================================
# 3. CODE GENERATION (Lexer & Parser)
# =========================================================
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)

BISON_TARGET(
        MyParser
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/parser.y
        ${CMAKE_CURRENT_BINARY_DIR}/generated/parser.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/generated/parser.hpp
)

FLEX_TARGET(
        MyScanner
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/lexer.l
        ${CMAKE_CURRENT_BINARY_DIR}/generated/scanner.cpp
)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

# =========================================================
# 4. TARGET DEFINITION
# =========================================================

# --- 4.1 Collect Sources ---
file(GLOB AST_NODES "ast/nodes/*.hpp")
file(GLOB AST_NODES_EXPR "ast/nodes/expr/*.hpp")
file(GLOB AST_NODES_STMT "ast/nodes/stmt/*.hpp")

# --- 4.2 Add Executable ---
add_executable(LittleButterfly_Compiler
        ${BISON_MyParser_OUTPUTS}
        ${FLEX_MyScanner_OUTPUTS}
        main.cpp
        ast/ast_factory.hpp
        ast/ast_node.hpp
        ast/ast_visitor.hpp
        ast/nodes_impl.hpp
        ast/print_visitor.hpp
        ast/helper/helper.h
        semantic/SymbolTable.hpp
        ${AST_NODES}
        lib/FlexLexer.h
        ast/symbol_table_visitor.hpp
        semantic/semantic_check_visitor.hpp
        mlir/mlir_gen_visitor.hpp
        mlir/mlir_gen_visitor.cpp
)

# =========================================================
# 5. LINKING & TARGET PROPERTIES
# =========================================================

# --- 5.1 Include Directories for Target ---
target_include_directories(LittleButterfly_Compiler PRIVATE ${MLIR_INCLUDE_DIRS})

# --- 5.2 Link Libraries ---
target_link_libraries(LittleButterfly_Compiler PRIVATE
        MLIRIR
        MLIRSupport

        MLIRFuncDialect
        MLIRArithDialect
        MLIRSCFDialect
        MLIRMemRefDialect

        LLVMCore
        LLVMSupport

        MLIRParser
        MLIRPass
        MLIRTransforms
)
