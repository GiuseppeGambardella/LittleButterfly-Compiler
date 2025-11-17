%{
#include <iostream>
#include <string>
#include "tokens.h" // Includiamo i nostri token condivisi

using namespace std;
%}

/* Attiviamo la modalità C++ */
%option c++
/* Non vogliamo gestire più file di input in sequenza */
%option noyywrap

%%

[0-9]+          {
                  // Esempio: Riconosce "123"
                  cout << "[LEXER] Letto numero: " << yytext << endl;
                  return TOKEN_NUMERO;
                }

[a-zA-Z_][a-zA-Z0-9_]* {
                  // Esempio: Riconosce "variabile_1"
                  cout << "[LEXER] Letto ID: " << yytext << endl;
                  return TOKEN_IDENTIFICATORE;
                }

"+"             { return TOKEN_PIU; }
"-"             { return TOKEN_MENO; }
"="             { return TOKEN_UGUALE; }

[ \t\n\r]+      { /* Ignoriamo spazi, tab e invio */ }

.               {
                  cout << "[LEXER] Carattere illegale: " << yytext << endl;
                  return TOKEN_SCONOSCIUTO;
                }

%%