%skeleton "lalr1.cc"
%require "3.2"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%define api.namespace {yy}
%define api.parser.class {yyParser}
%define api.token.prefix {TOKEN_}

%code requires {
    #include <string>
    #include <iostream>
    
    // Forward declaration for lexer
    class yyFlexLexer;
}

%param { yyFlexLexer* lexer }

%code {
    #include "..\lib\FlexLexer.h"
    
    // Define a custom yylex that wraps the FlexLexer's yylex
    int yylex(yyFlexLexer* lexer) {
        return lexer->yylex();
    }
    
    using namespace std;
}

%token EOF 0
%token SCONOSCIUTO

// Tipi e Valori
%token INT DOUBLE CHAR STRING VOID BOOL
%token CONST_INTEGER CONST_DOUBLE CONST_CHAR CONST_STRING
%token TRUE FALSE
%token ID

// Keywords
%token IF THEN ELSE LOOP MAIN
%token FUNC PRINT SCAN RETURN

// Operatori
%token EQ NEQ LE GE
%token AND OR

/* Priorità: FONDAMENTALE per evitare conflitti */
%left OR
%left AND
%left EQ NEQ '<' '>' LE GE  /* Tutti i confronti allo stesso livello */
%left '+' '-' '&'
%left '*' '/'
%right '!'

%%

program:
    global_declarations MAIN '(' ')' { cout << "=== [PARSER] MAIN TROVATO CON SUCCESSO! ===" << endl; } '{' instructions '}' {
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
    FUNC ID '(' params ')' ':' type '{' instructions '}' {
        cout << "  -> [FUNC] Nuova funzione definita." << endl;
    }
    ;

params:
      params ',' type ID
    | type ID
    | /* empty */
    ;

function_call:
    ID '(' arguments ')'
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
    | RETURN expression ';' { cout << "  -> [RETURN] Comando return." << endl; }
    | if_command
    | loop_command
    ;

variable_declaration:
      type ID { cout << "  -> [VAR] Dichiarazione." << endl; }
    | type ID '=' expression { cout << "  -> [VAR] Inizializzazione." << endl; }
    ;

assignment_command:
    ID '=' expression { cout << "  -> [ASSIGN] Assegnamento." << endl; }
    ;

type:
    INT | DOUBLE | STRING | BOOL | CHAR | VOID;

print_command:
    PRINT '(' expression ')' { cout << "  -> [PRINT] Output." << endl; }
    ;

scan_command:
    SCAN '(' ID ')' { cout << "  -> [SCAN] Input." << endl; }
    ;

/* --- IF CORRETTO (Senza else_block separato) --- */
if_command:
      /* Caso 1: Solo IF */
      IF expression THEN '{' instructions '}'
      { cout << "  -> [IF] Solo If." << endl; }

      /* Caso 2: IF e ELSE */
    | IF expression THEN '{' instructions '}' ELSE '{' instructions '}'
      { cout << "  -> [IF-ELSE] If con Else." << endl; }
    ;

loop_command:
    LOOP expression THEN '{' instructions '}' {
        cout << "  -> [LOOP] Ciclo." << endl;
    }
    ;

/* --- ESPRESSIONI (Senza comparison_operator intermedio) --- */
expression:
      ID
    | CONST_INTEGER
    | CONST_DOUBLE
    | CONST_STRING
    | CONST_CHAR
    | expression '+' expression
    | expression '-' expression
    | expression '*' expression
    | expression '/' expression
    | expression '&' expression
    | expression EQ expression
    | expression NEQ expression
    | expression '<' expression
    | expression '>' expression
    | expression LE expression
    | expression GE expression
    /* --------------------------------------------------------------- */
    | '(' expression ')'
    | function_call { cout << "    -> [CALL] Funzione chiamata in espressione." << endl; }
    | TRUE
    | FALSE
    | expression AND expression
    | expression OR expression
    | '!' expression
    ;

%%

void yy::yyParser::error(const std::string& msg) {
    cerr << ">>> ERRORE DI SINTASSI (Riga " << lexer->lineno() << "): " << msg << endl;
    if (lexer->YYText()) {
        cerr << "    Token imprevisto: '" << lexer->YYText() << "'" << endl;
    }
}