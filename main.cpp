#include <iostream>
#include <fstream>
#include "parser/parser.hpp"

// Includiamo la libreria C++ di Flex
#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

using namespace std;

// --- VARIABILI GLOBALI PER BISON ---
// Bison le cerca disperatamente. Noi le creiamo qui.
int yylineno = 1;
char* yytext = nullptr;

// Puntatore al lexer C++
yyFlexLexer *lexer = nullptr;

// --- FUNZIONE PONTE ---
// Bison chiama questa funzione C. Noi dentro chiamiamo la classe C++.
int yylex() {
    if (lexer == nullptr) return 0;

    int token = lexer->yylex();

    // Aggiorniamo le variabili globali così Bison le vede
    yylineno = lexer->lineno();
    yytext = (char*)lexer->YYText();

    return token;
}

// Funzione del parser generata da Bison
extern int yyparse();

int main(int argc, char** argv) {
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
    yyparse();
    cout << "--- FINE ---" << endl;

    delete lexer;
    return 0;
}