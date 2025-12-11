%{
#include <iostream>
#include <string>
#include "parser.hpp"

using namespace std;
using namespace yy;

// Define aliases for token enum values (using declarations don't work for enum class members)
#define TOKEN_MAIN yyParser::token::TOKEN_MAIN
#define TOKEN_FUNC yyParser::token::TOKEN_FUNC  
#define TOKEN_RETURN yyParser::token::TOKEN_RETURN
#define TOKEN_PRINT yyParser::token::TOKEN_PRINT
#define TOKEN_SCAN yyParser::token::TOKEN_SCAN
#define TOKEN_IF yyParser::token::TOKEN_IF
#define TOKEN_ELSE yyParser::token::TOKEN_ELSE
#define TOKEN_THEN yyParser::token::TOKEN_THEN
#define TOKEN_LOOP yyParser::token::TOKEN_LOOP
#define TOKEN_INT yyParser::token::TOKEN_INT
#define TOKEN_DOUBLE yyParser::token::TOKEN_DOUBLE
#define TOKEN_CHAR yyParser::token::TOKEN_CHAR
#define TOKEN_STRING yyParser::token::TOKEN_STRING
#define TOKEN_VOID yyParser::token::TOKEN_VOID
#define TOKEN_BOOL yyParser::token::TOKEN_BOOL
#define TOKEN_TRUE yyParser::token::TOKEN_TRUE
#define TOKEN_FALSE yyParser::token::TOKEN_FALSE
#define TOKEN_EQ  yyParser::token::TOKEN_EQ
#define TOKEN_NEQ yyParser::token::TOKEN_NEQ
#define TOKEN_GE  yyParser::token::TOKEN_GE
#define TOKEN_LE  yyParser::token::TOKEN_LE
#define TOKEN_OR  yyParser::token::TOKEN_OR
#define TOKEN_AND yyParser::token::TOKEN_AND
#define TOKEN_CONST_DOUBLE yyParser::token::TOKEN_CONST_DOUBLE
#define TOKEN_CONST_INTEGER yyParser::token::TOKEN_CONST_INTEGER
#define TOKEN_CONST_CHAR yyParser::token::TOKEN_CONST_CHAR
#define TOKEN_CONST_STRING yyParser::token::TOKEN_CONST_STRING
#define TOKEN_ID yyParser::token::TOKEN_ID
#define TOKEN_SCONOSCIUTO yyParser::token::TOKEN_SCONOSCIUTO
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
[+\-*/=><!\&]        { return yytext[0]; }

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
\\'([^\\'\\\\]|\\\\.)\\'   {
                        cout << "[LEXER] Carattere: " << yytext << endl;
                        return TOKEN_CONST_CHAR;
                    }

\\\"([^\\\"\\\\]|\\\\.)*\\\"  {
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