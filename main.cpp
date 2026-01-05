#include <iostream>
#include <fstream>
#include "Scanner.hpp" // La tua classe Scanner custom
#include "parser.hpp"  // Header generato da Bison
#include "symbol_table_visitor.hpp"
#include "semantic_check_visitor.hpp"
#include "ast/ast_node.hpp"
#include "ast/nodes_impl.hpp"
#include "ast/print_visitor.hpp"

using namespace std;

int main(int argc, char** argv) {
    // 1. GESTIONE INPUT (File o Stdin)
    ifstream file;
    istream* input = &cin; // Di default leggiamo da tastiera

    if (argc > 1) {
        // Se c'è un argomento, proviamo ad aprire il file
        file.open(argv[1]);
        if (!file.is_open()) {
            cerr << "ERRORE CRITICO: Impossibile aprire il file '" << argv[1] << "'" << endl;
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

    cout << "--- INIZIO ANALISI ---" << endl;

    // 4. AVVIO DEL PARSING
    // parse() restituisce 0 se tutto va bene, 1 se ci sono errori
    try {
        int result = parser.parse();

        if (result == 0) {
            cout << "--- PARSING COMPLETATO CON SUCCESSO! (0) ---" << endl;
        } else {
            cout << "--- FALLITO (Errori di sintassi) ---" << endl;
        }
        //PrintVisitor printer;
        //astRoot->accept(printer);

        SymbolTable symTable;
        SymbolTableVisitor builder(symTable);
        astRoot->accept(builder);

        if (!builder.getErrors().empty()) {
            for (auto& e : builder.getErrors())
                std::cerr << "Semantic error: " << e << "\n";
            return 1;
        }

        // Debug
        symTable.printTable();

        SemanticCheckVisitor typeChecker(symTable);
        astRoot->accept(typeChecker);

        if (!typeChecker.getErrors().empty()) {
            for (auto& e : typeChecker.getErrors())
                std::cerr << "Semantic error: " << e << "\n";
            return 1;
        }

        return result;

    } catch (const std::exception& e) {
        cerr << "ECCEZIONE CATTURATA: " << e.what() << endl;
        return 1;
    }
}