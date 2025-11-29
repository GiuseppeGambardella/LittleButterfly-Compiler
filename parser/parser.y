%{
#include <iostream>
#include <string>
using namespace std;

int yylex();
extern int yyparse();
extern FILE* yyin;

extern int yylineno;
extern char* yytext;

void yyerror(const char* s) {
    // Ora possiamo usare yylineno e yytext senza errori!
    cerr << ">>> ERRORE DI SINTASSI (Riga " << yylineno << "): " << s << endl;
    if (yytext) {
        cerr << "    Token imprevisto: '" << yytext << "'" << endl;
    }
}
%}

%token-table
%define parse.error verbose
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

/* Priorità: FONDAMENTALE per evitare conflitti */
%left TOKEN_OR
%left TOKEN_AND
%left TOKEN_EQ TOKEN_NEQ '<' '>' TOKEN_LE TOKEN_GE  /* Tutti i confronti allo stesso livello */
%left '+' '-' '&'
%left '*' '/'
%right '!'

%%

program:
    global_declarations TOKEN_MAIN '(' ')' { cout << "=== [PARSER] MAIN TROVATO CON SUCCESSO! ===" << endl; } '{' instructions '}' {
        cout << "=== [PARSER] PROGRAMMA ANALIZZATO CON SUCCESSO! ===" << endl;
    }
    ;

global_declarations:
    global_declarations function_declaration
    | global_declarations variable_declaration
    | /* empty */
    ;

instructions:
    instructions instruction
    | /* empty */
    ;

/* --- FUNZIONI --- */
function_declaration:
    TOKEN_FUNC TOKEN_ID '(' params ')' ':' type '{' instructions '}' {
        cout << "  -> [FUNC] Nuova funzione definita." << endl;
    }
    ;

params:
      params ',' type TOKEN_ID
    | type TOKEN_ID
    | /* empty */
    ;

function_call:
    TOKEN_ID '(' arguments ')'
    ;

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
    | function_call ';' { cout << "  -> [CALL] Funzione chiamata come comando." << endl; }
    | TOKEN_RETURN expression ';' { cout << "  -> [RETURN] Comando return." << endl; }
    | if_command
    | loop_command
    ;

variable_declaration:
      type TOKEN_ID { cout << "  -> [VAR] Dichiarazione." << endl; }
    | type TOKEN_ID '=' expression { cout << "  -> [VAR] Inizializzazione." << endl; }
    ;

assignment_command:
    TOKEN_ID '=' expression { cout << "  -> [ASSIGN] Assegnamento." << endl; }
    ;

type:
    TOKEN_INT | TOKEN_DOUBLE | TOKEN_STRING | TOKEN_BOOL | TOKEN_CHAR | TOKEN_VOID;

print_command:
    TOKEN_PRINT '(' expression ')' { cout << "  -> [PRINT] Output." << endl; }
    ;

scan_command:
    TOKEN_SCAN '(' TOKEN_ID ')' { cout << "  -> [SCAN] Input." << endl; }
    ;

/* --- IF CORRETTO (Senza else_block separato) --- */
if_command:
      /* Caso 1: Solo IF */
      TOKEN_IF expression TOKEN_THEN '{' instructions '}'
      { cout << "  -> [IF] Solo If." << endl; }

      /* Caso 2: IF e ELSE */
    | TOKEN_IF expression TOKEN_THEN '{' instructions '}' TOKEN_ELSE '{' instructions '}'
      { cout << "  -> [IF-ELSE] If con Else." << endl; }
    ;

loop_command:
    TOKEN_LOOP expression TOKEN_THEN '{' instructions '}' {
        cout << "  -> [LOOP] Ciclo." << endl;
    }
    ;

/* --- ESPRESSIONI (Senza comparison_operator intermedio) --- */
expression:
      TOKEN_ID
    | TOKEN_CONST_INTEGER
    | TOKEN_CONST_DOUBLE
    | TOKEN_CONST_STRING
    | TOKEN_CONST_CHAR
    | expression '+' expression
    | expression '-' expression
    | expression '*' expression
    | expression '/' expression
    | expression '&' expression
    | expression TOKEN_EQ expression
    | expression TOKEN_NEQ expression
    | expression '<' expression
    | expression '>' expression
    | expression TOKEN_LE expression
    | expression TOKEN_GE expression
    /* --------------------------------------------------------------- */
    | '(' expression ')'
    | function_call { cout << "    -> [CALL] Funzione chiamata in espressione." << endl; }
    | TOKEN_TRUE
    | TOKEN_FALSE
    | expression TOKEN_AND expression
    | expression TOKEN_OR expression
    | '!' expression
    ;

%%