%{
#include <iostream>
#include <string>
using namespace std;

// Lexer defined elsewhere
extern int yylex();
extern int yyparse();
extern FILE* yyin;

// Function to handle errors
void yyerror(const char* s) {
    cerr << "SYNTAX ERROR: " << s << endl;
}
%}

/* Tell Bison to create a header file for tokens */
%token-table
%defines "parser.hpp"

%token TOKEN_EOF 0
%token TOKEN_SCONOSCIUTO

// Types and Values
%token TOKEN_INT TOKEN_DOUBLE TOKEN_CHAR TOKEN_STRING TOKEN_VOID TOKEN_BOOL
%token TOKEN_CONST_INTEGER TOKEN_CONST_DOUBLE TOKEN_CONST_CHAR TOKEN_CONST_STRING
%token TOKEN_TRUE TOKEN_FALSE
%token TOKEN_ID

// Keywords
%token TOKEN_IF TOKEN_THEN TOKEN_ELSE TOKEN_LOOP TOKEN_MAIN
%token TOKEN_FUNC TOKEN_PRINT TOKEN_SCAN TOKEN_RETURN

// Operators
%token TOKEN_EQ TOKEN_NEQ TOKEN_LE TOKEN_GE
%token TOKEN_AND TOKEN_OR

/* Operator precedence (from lowest to highest) */
%left TOKEN_OR
%left TOKEN_AND
%left TOKEN_EQ TOKEN_NEQ
%left '<' '>' TOKEN_LE TOKEN_GE
%left '+' '-'
%left '*' '/'

%%

/* --- GRAMMAR RULES --- */
/* The program starts here */

program:
    global_declarations TOKEN_MAIN '(' ')' '{' instructions '}' { cout << "PROGRAM COMPLETED SUCCESSFULLY!" << endl; }
    ;

global_declarations:
    global_declarations function_declaration
    | global_declarations variable_declaration
    | /* empty -> epsilon */
    ;

instructions:
    instructions instruction
    | /* empty -> epsilon */
    ;

function_declaration:
    TOKEN_FUNC TOKEN_ID '(' args ')' ':' type '{' instructions '}' { cout << "  -> Function declared." << endl; }
    ;

function_call:
    TOKEN_ID '(' arguments ')'

arguments:
    arguments ',' expression
    | expression
    | /* empty */
    ;

args:
    args ',' type TOKEN_ID
    | type TOKEN_ID
    | /* empty -> epsilon */
    ;

instruction:
    variable_declaration
    | assignment_command
    | print_command
    | if_command
    | loop_command
    | function_call
    | TOKEN_RETURN expression ';'
    | /* empty -> epsilon */
    ;

variable_declaration:
    type TOKEN_ID ';' { cout << "  -> Variable declared." << endl; }
    | type TOKEN_ID '=' expression ';' { cout << "  -> Variable initialized." << endl; }
    ;

type:
    TOKEN_INT | TOKEN_DOUBLE | TOKEN_STRING | TOKEN_BOOL | TOKEN_CHAR | TOKEN_VOID;

print_command:
    TOKEN_PRINT '(' expression ')' ';' { cout << "  -> PRINT command found." << endl; }
    ;

if_command
    TOKEN_IF expression TOKEN_THEN '{' instructions '}' else { cout << "  -> IF block found." << endl; }
    ;

else_block:
    TOKEN_ELSE '{' instructions '}' { cout << "  -> ELSE block found." << endl; }
    | /* empty -> epsilon */
    ;

loop_command:
    TOKEN_LOOP expression TOKEN_THEN '{' instructions '}' { cout << "  -> LOOP found." << endl; }
    ;

comparison_operator:
    TOKEN_EQ | TOKEN_NEQ | '<' | '>' | TOKEN_LE | TOKEN_GE ;

expression:
      TOKEN_ID
    | TOKEN_CONST_INTEGER
    | TOKEN_CONST_DOUBLE
    | expression '+' expression
    | expression '-' expression
    | expression '*' expression
    | expression '/' expression
    | '(' expression ')'
    | function_call
    | TOKEN_TRUE
    | TOKEN_FALSE
    | expression TOKEN_AND expression
    | expression TOKEN_OR expression
    | expression TOKEN_NEQ expression
    | '!' expression
    | expression comparison_operator expression
    ;


/*
Expr ::= FunCall
	| REAL_CONST
        | INTEGER_CONST
	| STRING_CONST
	| ID
        | TRUE
        | FALSE
        | Expr  PLUS Expr
	| Expr  MINUS Expr
	| Expr  TIMES Expr
	| Expr  DIV Expr
	| Expr  AND Expr
	| Expr  OR Expr
	| Expr  GT Expr
	| Expr  GE Expr
	| Expr  LT Expr
	| Expr  LE Expr
	| Expr  EQ Expr
	| Expr  NE Expr
	| LPAR Expr RPAR
	| MINUS Expr
	| NOT Expr
*/
%%