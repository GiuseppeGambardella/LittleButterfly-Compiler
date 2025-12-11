#include <iostream>
#include <fstream>
#include <FlexLexer.h>
#include "parser.hpp"

using namespace std;

int main(int argc, char** argv) {
    yyFlexLexer* lexer = nullptr;
    
    // Leggiamo da un file se passato, altrimenti da tastiera
    if (argc > 1) {
        ifstream* file = new ifstream(argv[1]);
        if (!file->is_open()) {
            cerr << "Non posso aprire il file: " << argv[1] << endl;
            return 1;
        }
        lexer = new yyFlexLexer(file, &cout);
    } else {
        lexer = new yyFlexLexer(&cin, &cout);
    }

    cout << "--- COMPILAZIONE INIZIATA ---" << endl;
    
    // Crea il parser C++ e passa il lexer come parametro
    yy::yyParser parser(lexer);
    
    // Esegui il parsing
    int result = parser.parse();
    
    cout << "--- FINE ---" << endl;

    delete lexer;
    return result;
}