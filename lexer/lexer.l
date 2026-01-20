%{
#include <iostream>
#include <string>
#include "Scanner.hpp" // Includiamo la nostra classe

using namespace std;
using namespace yy;

// Variabile per la location
yyParser::location_type loc;

#undef YY_DECL
#define YY_DECL yy::yyParser::symbol_type yy::Scanner::lex()
#define yyterminate() return yy::yyParser::make_EOF(loc)
#define YY_USER_ACTION loc.step(); loc.columns(yyleng);

// Macro per i token (uguali a prima)
#define TOKEN_MAIN    return yyParser::make_MAIN(loc);
#define TOKEN_FUNC    return yyParser::make_FUNC(loc);
#define TOKEN_RETURN  return yyParser::make_RETURN(loc);
#define TOKEN_PRINT   return yyParser::make_PRINT(loc);
#define TOKEN_SCAN    return yyParser::make_SCAN(loc);
#define TOKEN_IF      return yyParser::make_IF(loc);
#define TOKEN_ELSE    return yyParser::make_ELSE(loc);
#define TOKEN_THEN    return yyParser::make_THEN(loc);
#define TOKEN_LOOP    return yyParser::make_LOOP(loc);
#define TOKEN_INT     return yyParser::make_INT(loc);
#define TOKEN_DOUBLE  return yyParser::make_DOUBLE(loc);
#define TOKEN_CHAR    return yyParser::make_CHAR(loc);
#define TOKEN_STRING  return yyParser::make_STRING(loc);
#define TOKEN_VOID    return yyParser::make_VOID(loc);
#define TOKEN_BOOL    return yyParser::make_BOOL(loc);
#define TOKEN_TRUE    return yyParser::make_TRUE(loc);
#define TOKEN_FALSE   return yyParser::make_FALSE(loc);
#define TOKEN_EQ      return yyParser::make_EQ(loc);
#define TOKEN_NEQ     return yyParser::make_NEQ(loc);
#define TOKEN_GE      return yyParser::make_GE(loc);
#define TOKEN_LE      return yyParser::make_LE(loc);
#define TOKEN_OR      return yyParser::make_OR(loc);
#define TOKEN_AND     return yyParser::make_AND(loc);

%}

/* MANTENIAMO C++ */
%option c++
%option noyywrap
%option yylineno
%option yyclass="yy::Scanner"

%%

%{
  loc.step();
%}

"defef"             { TOKEN_FUNC }
"flyback"           { TOKEN_RETURN }
"flyout"            { TOKEN_PRINT }
"flyin"             { TOKEN_SCAN }
"ifif"              { TOKEN_IF }
"efelsefe"          { TOKEN_ELSE }
"thefen"            { TOKEN_THEN }
"whifilefe"         { TOKEN_LOOP }
"ifint"             { TOKEN_INT }
"refeafal"       { TOKEN_DOUBLE }
"chafar"            { TOKEN_CHAR }
"strifing"          { TOKEN_STRING }
"notype"            { TOKEN_VOID }
"bofoofol"          { TOKEN_BOOL }
"true"              { TOKEN_TRUE }
"false"             { TOKEN_FALSE }
"=="                { TOKEN_EQ }
"<>"                { TOKEN_NEQ }
">="                { TOKEN_GE }
"<="                { TOKEN_LE }
"or"                { TOKEN_OR }
"and"               { TOKEN_AND }
"fly"               { TOKEN_MAIN }

[+\-*/=><!\&?]       { return yyParser::symbol_type(yytext[0], loc); }
[(){};,:]           { return yyParser::symbol_type(yytext[0], loc); }

[0-9]+"."[0-9]+     { return yyParser::make_CONST_DOUBLE(std::stod(yytext), loc); }

[0-9]+              { return yyParser::make_CONST_INTEGER(std::stoi(yytext), loc); }

\'([^\\'\\\\]|\\\\.)\' { return yyParser::make_CONST_CHAR(yytext[1], loc); }

\"(\\.|[^"\\])*\" { return yyParser::make_CONST_STRING(std::string(yytext + 1, yyleng - 2), loc); }

[a-zA-Z_][a-zA-Z0-9_]* { return yyParser::make_ID(std::string(yytext), loc); }

"@*"([^*]|\*+[^*@])*\*+"@" { /* Commenti */ }

\n+         { loc.lines(yyleng); loc.step(); }
[ \t\r]+    { loc.step(); }

. { throw yyParser::syntax_error(loc, "Invalid character: " + std::string(yytext)); }

%%