%{
#include <iostream>
#include <string>
using namespace std;

// Lexer definito altrove
extern int yylex();
extern int yyparse();
extern FILE* yyin;

// Gestione errori
void yyerror(const char* s) {
    cerr << ">>> ERRORE DI SINTASSI: " << s << endl;
}
%}

/* Header tokens */
%token-table
%defines "parser.hpp"

%token TOKEN_EOF 0
%token TOKEN_SCONOSCIUTO

// Tipi e Valori
%token TOKEN_INT TOKEN_DOUBLE TOKEN_CHAR TOKEN_STRING TOKEN_VOID TOKEN_BOOL
%token TOKEN_CONST_INTEGER TOKEN_CONST_DOUBLE TOKEN_CONST_CHAR TOKEN_CONST_STRING
%token TOKEN_TRUE TOKEN_FALSE
%token TOKEN_ID

// Keywords
%token TOKEN_IF TOKEN_THEN TOKEN_ELSE TOKEN_LOOP TOKEN_MAIN
%token TOKEN_FUNC TOKEN_PRINT TOKEN_SCAN TOKEN_RETURN

// Operatori
%token TOKEN_EQ TOKEN_NEQ TOKEN_LE TOKEN_GE
%token TOKEN_AND TOKEN_OR

/* Priorità operatori (dal più basso al più alto) */
%left TOKEN_OR
%left TOKEN_AND
%left TOKEN_EQ TOKEN_NEQ
%left '<' '>' TOKEN_LE TOKEN_GE
%left '+' '-' '&'
%left '*' '/'
%right '!'  /* Il NOT è unario e ha priorità alta */

%%

/* --- REGOLE GRAMMATICALI --- */

/* STRUTTURA PRINCIPALE */
program:
    global_declarations TOKEN_MAIN '(' ')' '{' instructions '}' {
        cout << "=== [PARSER] PROGRAMMA ANALIZZATO CON SUCCESSO! ===" << endl;
    }
    ;

/* DICHIARAZIONI GLOBALI */
global_declarations:
    global_declarations function_declaration
    | global_declarations variable_declaration
    | /* empty */
    ;

/* LISTA ISTRUZIONI */
instructions:
    instructions instruction
    | /* empty */
    ;

/* --- FUNZIONI --- */

/* Definizione funzione: func nome(args) : tipo { ... } */
function_declaration:
    TOKEN_FUNC TOKEN_ID '(' args ')' ':' type '{' instructions '}' {
        cout << "  -> [FUNC] Nuova funzione definita." << endl;
    }
    ;

/* Argomenti nella definizione (int x, double y) */
args:
      args ',' type TOKEN_ID
    | type TOKEN_ID
    | /* empty */
    ;

/* Chiamata a funzione: nome(argomenti) */
function_call:
    TOKEN_ID '(' arguments ')' {
        cout << "    -> [CALL] Chiamata a funzione rilevata." << endl;
    }
    ;

/* Argomenti nella chiamata (5, x+1) */
arguments:
      arguments ',' expression
    | expression
    | /* empty */
    ;

/* --- ISTRUZIONI --- */

instruction:
      variable_declaration ';'
    | assignment_command ';'
    | print_command ';'
    | scan_command ';'
    | function_call ';'
    | TOKEN_RETURN expression ';'
    | if_command
    | loop_command{ cout << "  -> [RETURN] Comando return." << endl; }
    ;

/* Dichiarazione: int x = 10; */
variable_declaration:
    type TOKEN_ID {
        cout << "  -> [VAR] Variabile dichiarata." << endl;
    }
    | type TOKEN_ID '=' expression {
        cout << "  -> [VAR] Variabile inizializzata." << endl;
    }
    ;

/* Assegnamento: x = 10; */
assignment_command:
    TOKEN_ID '=' expression {
        cout << "  -> [ASSIGN] Variabile modificata." << endl;
    }
    ;

type:
    TOKEN_INT | TOKEN_DOUBLE | TOKEN_STRING | TOKEN_BOOL | TOKEN_CHAR | TOKEN_VOID;

print_command:
    TOKEN_PRINT '(' expression ')' {
        cout << "  -> [PRINT] Comando stampa." << endl;
    }
    ;

scan_command:
    TOKEN_SCAN '(' TOKEN_ID ')' {
        cout << "  -> [SCAN] Comando input." << endl;
    }
    ;

if_command:
    TOKEN_IF expression TOKEN_THEN '{' instructions '}' else_block {
        cout << "  -> [IF] Blocco IF completato." << endl;
    }
    ;

else_block:
    TOKEN_ELSE '{' instructions '}' {
        cout << "  -> [ELSE] Blocco ELSE trovato." << endl;
    }
    | /* empty */
    ;

loop_command:
    TOKEN_LOOP expression TOKEN_THEN '{' instructions '}' {
        cout << "  -> [LOOP] Ciclo trovato." << endl;
    }
    ;

comparison_operator:
    TOKEN_EQ | TOKEN_NEQ | '<' | '>' | TOKEN_LE | TOKEN_GE ;

/* --- ESPRESSIONI --- */

expression:
      TOKEN_ID
    | TOKEN_CONST_INTEGER
    | TOKEN_CONST_DOUBLE
    | TOKEN_CONST_STRING
    | expression '+' expression
    | expression '-' expression
    | expression '*' expression
    | expression '/' expression
    | expression '&' expression
    | '(' expression ')'
    | function_call
    | TOKEN_TRUE
    | TOKEN_FALSE
    | expression TOKEN_AND expression
    | expression TOKEN_OR expression
    | '!' expression
    | expression comparison_operator expression
    ;

%%