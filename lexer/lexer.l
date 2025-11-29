%{
#include <iostream>
#include <string>
//#include "tokens.h"
#include "../parser/parser.hpp"

using namespace std;
%}

%option c++
%option noyywrap
%option yylineno

%%

"fly"                { return TOKEN_MAIN; }
"defef"              { return TOKEN_FUNC; }
"flyback"            { return TOKEN_RETURN; }
"flyout"             { return TOKEN_PRINT; }
"flyin"              { return TOKEN_SCAN; }

 /* --- PAROLE CHIAVE --- */
"ifif"              { return TOKEN_IF; }
"efelsefe"          { return TOKEN_ELSE; }
"thefen"            { return TOKEN_THEN; }
"whifilefe"         { return TOKEN_LOOP; }
"ifint"             { return TOKEN_INT; }
"refeafaefel"       { return TOKEN_DOUBLE; }
"chafar"            { return TOKEN_CHAR; }
"strifing"          { return TOKEN_STRING; }
"notype"            { return TOKEN_VOID; }
"bofoofol"          { return TOKEN_BOOL; }

 /* --- BOOLEANI --- */
"true"              { return TOKEN_TRUE; }
"false"             { return TOKEN_FALSE; }

 /* --- OPERATORI MULTI-CARATTERE --- */
"=="                { return TOKEN_EQ; }
"<>"                { return TOKEN_NEQ; }
">="                { return TOKEN_GE; }
"<="                { return TOKEN_LE; }
"or"                { return TOKEN_OR; }
"and"               { return TOKEN_AND; }

 /* --- OPERATORI SINGOLI --- */
[+\-*/=><!&]        { return yytext[0]; }

 /* --- COSTANTI NUMERICHE --- */
[0-9]+"."[0-9]+     {
                        cout << "[LEXER] Double: " << yytext << endl;
                        return TOKEN_CONST_DOUBLE;
                    }

[0-9]+              {
                        cout << "[LEXER] Intero: " << yytext << endl;
                        return TOKEN_CONST_INTEGER;
                    }

 /* --- CARATTERI E STRINGHE --- */
\'([^\'\\]|\\.)\'   {
                        cout << "[LEXER] Carattere: " << yytext << endl;
                        return TOKEN_CONST_CHAR;
                    }

\"([^\"\\]|\\.)*\"  {
                        cout << "[LEXER] Stringa: " << yytext << endl;
                        return TOKEN_CONST_STRING;
                    }

 /* --- IDENTIFICATORI --- */
[a-zA-Z_][a-zA-Z0-9_]* {
                        cout << "[LEXER] ID: " << yytext << endl;
                        return TOKEN_ID;
                      }

 /* --- PUNTEGGIATURA --- */
[(){};,:]            { return yytext[0]; }

 /* --- COMMENTI (@* ... *@) --- */
"@*"([^*]|\*+[^*@])*\*+"@"   { /* Ignora commenti */ }

 /* --- SPAZI E TAB --- */
[ \t\n\r]+          { /* ignora */ }

 /* --- CARATTERI ILLEGALI --- */
.                   {
                        cout << "[LEXER] Carattere illegale: " << yytext << endl;
                        return TOKEN_SCONOSCIUTO;
                    }

%%