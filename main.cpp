#include <iostream>
#include <fstream>
#include <mlir/IR/MLIRContext.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/Arith/IR/Arith.h>
#include <mlir/Dialect/MemRef/IR/MemRef.h>
#include <mlir/Dialect/SCF/IR/SCF.h>

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
    // 1. GESTIONE INPUT (File o Stdin)
    ifstream file;
    istream* input = &cin; // Di default leggiamo da tastiera

    if (argc > 1) {
        // Se c'è un argomento, proviamo ad aprire il file
        file.open(argv[1]);
        if (!file.is_open()) {
            cerr << "CRITICAL ERROR: Unable to open file. '" << argv[1] << "'" << endl;
            return 1;
        }
        input = &file;
    }

    // 2. CREAZIONE DELLO SCANNER
    // Istanziamo la classe yy::Scanner che abbiamo definito in Scanner.hpp.
    // Le passiamo il puntatore allo stream di input (file o cin).
    yy::Scanner scanner(input);
    std::unique_ptr<ASTNode> astRoot;

    // 3. CREAZIONE DEL PARSER
    // Il parser prende il nostro scanner come riferimento (vedi %param nel parser.y)
    yy::yyParser parser(scanner, astRoot);

    cout << "--- START... ---" << endl;

    // 4. AVVIO DEL PARSING
    // parse() restituisce 0 se tutto va bene, 1 se ci sono errori
    try {
        int result = parser.parse();

        if (result == 0) {
            cout << "--- PARSING COMPLETED! ---" << endl;
            SymbolTable symTable;
            SymbolTableVisitor builder(symTable);
            astRoot->accept(builder);

            if (!builder.getErrors().empty()) {
                for (auto& e : builder.getErrors())
                    std::cerr << "Semantic error: " << e << "\n";
                return 1;
            }
            // Debug
            //symTable.printTable();

            SemanticCheckVisitor typeChecker(symTable);
            astRoot->accept(typeChecker);
            if (!typeChecker.getErrors().empty()) {
                return 1;
            }
            cout << "--- SEMANTIC CHECK COMPLETED! ---" << endl;

            // =======================
            // MLIR CODE GENERATION
            // =======================
            mlir::MLIRContext context;

            // (opzionale ma consigliato)
            context.getOrLoadDialect<mlir::func::FuncDialect>();
            context.getOrLoadDialect<mlir::arith::ArithDialect>();
            context.getOrLoadDialect<mlir::memref::MemRefDialect>();
            context.getOrLoadDialect<mlir::scf::SCFDialect>();

            MLIRGenVisitor mlirGen(context, symTable);
            astRoot->accept(mlirGen);

            mlir::ModuleOp module = mlirGen.theModule;

            // Dump MLIR su stdout
            std::cout << "\n===== MLIR DUMP =====\n";
            mlirGen.dump();
            std::cout << "\n=====================\n";

            lowering::registerDialects(context);
            lowering::lowerToLLVMDialect(module);

            std::cout << "\n===== MLIR LLVM DIALECT =====\n";
            module.dump();
            std::cout << "\n============================\n";

            llvm::LLVMContext llvm_context;
            auto llvmModule = lowering::translateToLLVMIR(module, llvm_context);

            // Dump LLVM IR
            llvmModule->print(llvm::outs(), nullptr);
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