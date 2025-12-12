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
    global_declarations function_declaration {
        $$ = make_node<ProgramNode>(std::move($1), std::move($6));
    }
    ;

global_declarations:
    function_declaration global_declarations
    | variable_declaration global_declarations  ';'
    | /* empty */ { $$ = nullptr; }
    ;

instructions:
    instructions instruction
    | /* empty */ { $$ = nullptr; }
    ;

/* --- FUNZIONI --- */
function_declaration:
    FUNC ID '(' params ')' ':' type '{' instructions '}' {
        $$ = make_node<FunctionDeclNode>($2, std::move($4), std::move($7), std::move($9));
    }
    | MAIN '(' ')' '{' instructions '}' {
        $$ = make_node<FunctionDeclNode>("fly", nullptr, std::move(make_node<VoidNode>()), std::move($7));
    }
    ;

params:
      params ',' type ID
    | type ID { $$ = make_node<VarDeclNode>(std::move($1), $2); }
    | /* empty */ { $$ = nullptr; }
    ;

function_call:
    ID '(' arguments ')' { $$ = make_node<FunctionCallNode>($1, std::move($3)); }
    ;

arguments:
      arguments ',' expression
    | expression { $$ = std::move($1); }
    | /* empty */ { $$ = nullptr; }
    ;

/* --- ISTRUZIONI --- */
instruction:
      variable_declaration ';' { $$ = std::move($1); }
    | assignment_command ';' { $$ = std::move($1); }
    | print_command ';' { $$ = std::move($1); }
    | scan_command ';' { $$ = std::move($1); }
    | function_call ';' { $$ = std::move($1); }
    | RETURN expression ';' { $$ = make_node<ReturnNode>(std::move($2));  }
    | if_command { $$ = std::move($1); }
    | loop_command { $$ = std::move($1); }
    ;

variable_declaration:
      type ID { $$ = make_node<VarDeclNode>(std::move($1), $2); }
    | type ID '=' expression { $$ = make_node<VarDeclNode>(std::move($1), $2, std::move($4)); }
    ;

assignment_command:
    ID '=' expression { $$ = make_node<AssignmentNode>($1, std::move($3)); }
    ;

type:
    INT { $$ = make_node<IntNode>(); }
    | DOUBLE { $$ = make_node<DoubleNode>(); }
    | STRING { $$ = make_node<StringNode>(); }
    | BOOL { $$ = make_node<BoolNode>(); }
    | CHAR { $$ = make_node<CharNode>(); }
    | VOID { $$ = make_node<VoidNode>(); }
    ;

print_command:
    PRINT '(' expression ')' { $$ = make_node<PrintNode>(std::move($3)); }
    ;

scan_command:
    SCAN '(' ID ')' { $$ = make_node<ScanNode>($3); }
    ;

/* --- IF CORRETTO (Senza else_block separato) --- */
if_command:
      /* Caso 1: Solo IF */
      IF expression THEN '{' instructions '}'
      { $$ = make_node<IfNode>(std::move($2), std::move($5), nullptr); }

      /* Caso 2: IF e ELSE */
    | IF expression THEN '{' instructions '}' ELSE '{' instructions '}'
      { $$ = make_node<IfNode>(std::move($2), std::move($5), std::move($9)); }
    ;

loop_command:
    LOOP expression THEN '{' instructions '}' {
        $$ = make_node<LoopNode>(std::move($2), std::move($5));
    }
    ;

/* --- ESPRESSIONI (Senza comparison_operator intermedio) --- */
expression:
      ID { $$ = make_node<VariableNode>($1); }
    | CONST_INTEGER { $$ = make_node<NumberNode>($1); }
    | CONST_DOUBLE { $$ = make_node<RealNode>($1); }
    | CONST_STRING { $$ = make_node<StringNode>($1); }
    | CONST_CHAR { $$ = make_node<CharNode>($1); }
    /* --------------------------------------------------------------- */
    | expression '+' expression { $$ = make_node<BinaryOpNode>("+", std::move($1), std::move($3)); }
    | expression '-' expression { $$ = make_node<BinaryOpNode>("-", std::move($1), std::move($3)); }
    | expression '*' expression { $$ = make_node<BinaryOpNode>("*", std::move($1), std::move($3)); }
    | expression '/' expression { $$ = make_node<BinaryOpNode>("/", std::move($1), std::move($3)); }
    | expression '&' expression { $$ = make_node<BinaryOpNode>("&", std::move($1), std::move($3)); }
    /* --------------------------------------------------------------- */
    | expression EQ expression { $$ = make_node<BinaryOpNode>( $2, std::move($1), std::move($3)); }
    | expression NEQ expression { $$ = make_node<BinaryOpNode>( $2, std::move($1), std::move($3)); }
    | expression '<' expression { $$ = make_node<BinaryOpNode>( "<", std::move($1), std::move($3)); }
    | expression '>' expression { $$ = make_node<BinaryOpNode>( ">", std::move($1), std::move($3)); }
    | expression LE expression { $$ = make_node<BinaryOpNode>( $2, std::move($1), std::move($3)); }
    | expression GE expression { $$ = make_node<BinaryOpNode>( $2, std::move($1), std::move($3)); }
    /* --------------------------------------------------------------- */
    | '(' expression ')' { $$ = std::move($2); }
    | function_call { $$ = std::move($1); }
    | TRUE { $$ = make_node<BoolNode>(true); }
    | FALSE { $$ = make_node<BoolNode>(false); }
    | expression AND expression { $$ = make_node<BinaryOpNode>( "AND", std::move($1), std::move($3)); }
    | expression OR expression { $$ = make_node<BinaryOpNode>( "OR", std::move($1), std::move($3)); }
    | '!' expression { $$ = make_node<UnaryOpNode>( "!", std::move($2)); }
    ;

%%

void yy::yyParser::error(const std::string& msg) {
    cerr << ">>> ERRORE DI SINTASSI (Riga " << lexer->lineno() << "): " << msg << endl;
    if (lexer->YYText()) {
        cerr << "    Token imprevisto: '" << lexer->YYText() << "'" << endl;
    }
}