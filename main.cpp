#include <iostream>
#include <fstream>
#include <mlir/IR/MLIRContext.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/Arith/IR/Arith.h>
#include <mlir/Dialect/MemRef/IR/MemRef.h>
#include <mlir/Dialect/SCF/IR/SCF.h>
#include <mlir/Dialect/ControlFlow/IR/ControlFlowOps.h>

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>


#include "lowering.hpp"
#include "Scanner.hpp" // La tua classe Scanner custom
#include "parser.hpp"  // Header generato da Bison
#include "symbol_table_visitor.hpp"
#include "semantic_check_visitor.hpp"
#include "ast/ast_node.hpp"
#include "ast/nodes_impl.hpp"
#include "ast/print_visitor.hpp"
#include "mlir/mlir_gen_visitor.hpp"

using namespace std;

int main(int argc, char** argv) {
    // 1. input handling
    ifstream file;
    istream* input = &cin;

    std::vector<std::string> sourceLines;

    if (argc > 1) {
        file.open(argv[1]);
        if (!file.is_open()) {
            cerr << "CRITICAL ERROR: Unable to open file. '" << argv[1] << "'" << endl;
            return 1;
        }
        input = &file;
    }

    // 2. scanner creation
    yy::Scanner scanner(input);
    std::unique_ptr<ASTNode> astRoot;

    // 3. parser creation
    yy::yyParser parser(scanner, astRoot,sourceLines);

    cout << "--- START... ---" << endl;

    // 4. Starting parsing
    try {
        int result = parser.parse();

        if (result == 0) {
            cout << "--- PARSING COMPLETED! ---" << endl;
            SymbolTable symTable;
            SymbolTableVisitor builder(symTable, sourceLines);
            astRoot->accept(builder);

            if (!builder.getErrors().empty()) {
                for (auto& e : builder.getErrors())
                    std::cerr << e ;
                return 1;
            }
            // Debug
            //symTable.printTable();

            SemanticCheckVisitor typeChecker(symTable, sourceLines);
            astRoot->accept(typeChecker);


            if (!typeChecker.getErrors().empty()) {
                for (const auto& err : typeChecker.getErrors()) {
                    std::cerr << err;
                }
                return 1;
            }
            cout << "--- SEMANTIC CHECK COMPLETED! ---" << endl;

            // =======================
            // MLIR CODE GENERATION
            // =======================
            mlir::MLIRContext context;

            context.getOrLoadDialect<mlir::func::FuncDialect>();
            context.getOrLoadDialect<mlir::arith::ArithDialect>();
            context.getOrLoadDialect<mlir::memref::MemRefDialect>();
            context.getOrLoadDialect<mlir::scf::SCFDialect>();
            context.getOrLoadDialect<mlir::cf::ControlFlowDialect>();

            MLIRGenVisitor mlirGen(context, symTable);
            astRoot->accept(mlirGen);

            mlirGen.emitMainWrapper();

            mlir::ModuleOp module = mlirGen.theModule;

            /*// Dump MLIR stdout
            std::cout << "\n===== MLIR DUMP =====\n";
            mlirGen.dump();
            std::cout << "\n=====================\n";*/

            lowering::registerDialects(context);
            lowering::lowerToLLVMDialect(module);

            /*std::cout << "\n===== MLIR LLVM DIALECT =====\n";
            module.dump();
            std::cout << "\n============================\n";*/

            llvm::LLVMContext llvm_context;
            auto llvmModule = lowering::translateToLLVMIR(module, llvm_context);

            // Scrivi LLVM IR
            std::error_code EC;
            llvm::raw_fd_ostream out("output.ll", EC);
            llvmModule->print(out, nullptr);
            out.flush();

            // Compila con clang
            if (std::system("clang -D_CRT_SECURE_NO_WARNINGS output.ll runtime/runtime.cpp -o program.exe") != 0) {
                std::cerr << "clang failed\n";
                return 1;
            }

            // Esegui
            std::system("program.exe");

        }
        else {
            cerr << "--- PAR"
                    ""
                    "SING FAILED! ---" << endl;
            return result;
        }
        //PrintVisitor printer;
        //astRoot->accept(printer);

        return result;

    } catch (const std::exception& e) {
        cerr << "EXCEPTION: " << e.what() << endl;
        return 1;
    }
}