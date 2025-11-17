#include <iostream>
#include <fstream>

// FlexLexer.h è un file standard di sistema installato con Flex
// Dobbiamo includerlo per usare la classe yyFlexLexer
#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif
s
#include "tokens.h"

using namespace std;

int main() {
    // Creiamo un'istanza del lexer.
    // Di default legge da std::cin (tastiera) e scrive su std::cout
    yyFlexLexer* lexer = new yyFlexLexer(&cin, &cout);

    cout << "Scrivi qualcosa (es: x = 10 + 5) e premi INVIO + CTRL-D per finire:" << endl;
    cout << "------------------------------------------------------------" << endl;

    int token;
    // Ciclo continuo finché non riceviamo 0 (EOF)
    while ((token = lexer->yylex()) != TOKEN_EOF) {

        switch (token) {
            case TOKEN_NUMERO:
                cout << "  -> Main: Ho ricevuto un NUMERO" << endl;
                break;
            case TOKEN_IDENTIFICATORE:
                cout << "  -> Main: Ho ricevuto un IDENTIFICATORE" << endl;
                break;
            case TOKEN_PIU:
                cout << "  -> Main: Ho ricevuto operatore PIU (+)" << endl;
                break;
            case TOKEN_UGUALE:
                cout << "  -> Main: Ho ricevuto operatore UGUALE (=)" << endl;
                break;
            case TOKEN_SCONOSCIUTO:
                cout << "  -> Main: Errore, token non valido." << endl;
                break;
            default:
                cout << "  -> Main: Token generico ID " << token << endl;
        }
    }

    delete lexer;
    return 0;
}