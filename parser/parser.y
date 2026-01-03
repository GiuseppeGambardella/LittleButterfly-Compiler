%skeleton "lalr1.cc"
%require "3.2"
%defines
%locations

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%define api.namespace {yy}
%define api.parser.class {yyParser}
%define api.token.prefix {TOKEN_}

%code requires {
    #include <string>
    #include <iostream>
    #include <vector>
    #include <memory>

    // Includiamo TypeNode per passarlo per valore
    #include "ast/nodes/TypeNode.hpp"

    // Forward declaration del nostro Scanner custom
    namespace yy { class Scanner; }
    class ASTNode;
}


// Passiamo il nostro Scanner come riferimento al parser
%param { yy::Scanner& scanner }
%parse-param { std::unique_ptr<ASTNode>& astRoot }

%code {
    #include "Scanner.hpp" // Include la definizione completa della classe Scanner
    #include "ast/nodes_impl.hpp"
    #include "ast/ast_factory.hpp"

    // Funzione bridge che Bison chiama: prende il nostro scanner e chiama lex()
    static yy::yyParser::symbol_type yylex(yy::Scanner& scanner) {
        return scanner.lex();
    }

    using namespace std;
}

%token EOF 0
%token SCONOSCIUTO

/* --- TOKEN CON TIPO --- */
%token <int> CONST_INTEGER
%token <double> CONST_DOUBLE
%token <char> CONST_CHAR
%token <std::string> CONST_STRING ID

/* --- KEYWORDS E OPERATORI --- */
%token INT DOUBLE CHAR STRING VOID BOOL
%token TRUE FALSE
%token IF THEN ELSE LOOP MAIN
%token FUNC PRINT SCAN RETURN
%token EQ NEQ LE GE AND OR

/* Priorità */
%left OR
%left AND
%left EQ NEQ '<' '>' LE GE
%left '+' '-' '&'
%left '*' '/'
%right '!'

/* --- TIPI NON TERMINALI --- */
// Liste (Vettori)
%type <std::vector<std::unique_ptr<ASTNode>>> global_declarations instructions params arguments

// Nodi singoli (Puntatori)
%type <std::unique_ptr<ASTNode>> program function_declaration instruction
%type <std::unique_ptr<ASTNode>> variable_declaration assignment_command print_command scan_command
%type <std::unique_ptr<ASTNode>> function_call if_command loop_command expression

// Tipo Oggetto (TypeNode per valore)
%type <TypeNode> type

%%

program:
    global_declarations function_declaration {
        $$ = make_node<ProgramNode>(std::move($1), std::move($2));
        astRoot = std::move($$);
    }
    ;

global_declarations:
      /* empty */ {
          $$ = std::vector<std::unique_ptr<ASTNode>>();
      }
    | global_declarations function_declaration {
          $1.push_back(std::move($2));
          $$ = std::move($1);
      }
    | global_declarations variable_declaration ';' {
          $1.push_back(std::move($2));
          $$ = std::move($1);
      }
    ;

instructions:
      /* empty */ {
          $$ = std::vector<std::unique_ptr<ASTNode>>();
      }
    | instructions instruction {
          $1.push_back(std::move($2));
          $$ = std::move($1);
      }
    ;

/* --- FUNZIONI --- */
function_declaration:
    FUNC ID '(' params ')' ':' type '{' instructions '}' {
        auto body = make_node<BlockNode>(std::move($9));
        auto retType = std::make_unique<TypeNode>(std::move($7));

        $$ = make_node<FunctionDeclNode>($2, std::move($4), std::move(retType), std::move(body));
    }
    | MAIN '(' ')' '{' instructions '}' {
        auto body = make_node<BlockNode>(std::move($5));
        auto voidType = std::make_unique<VoidNode>();
        std::vector<std::unique_ptr<ASTNode>> emptyParams;

        $$ = make_node<FunctionDeclNode>("fly", std::move(emptyParams), std::move(voidType), std::move(body));
    }
    ;

params:
      /* empty */ {
          $$ = std::vector<std::unique_ptr<ASTNode>>();
      }
    | type ID {
          std::vector<std::unique_ptr<ASTNode>> p;
          p.push_back(make_node<VarDeclNode>(std::move($1), $2));
          $$ = std::move(p);
      }
    | params ',' type ID {
          $1.push_back(make_node<VarDeclNode>(std::move($3), $4));
          $$ = std::move($1);
      }
    ;

function_call:
    ID '(' arguments ')' {
        $$ = make_node<FunctionCallNode>($1, std::move($3));
    }
    ;

arguments:
      /* empty */ {
          $$ = std::vector<std::unique_ptr<ASTNode>>();
      }
    | expression {
          std::vector<std::unique_ptr<ASTNode>> args;
          args.push_back(std::move($1));
          $$ = std::move(args);
      }
    | arguments ',' expression {
          $1.push_back(std::move($3));
          $$ = std::move($1);
      }
    ;

/* --- ISTRUZIONI --- */
instruction:
      variable_declaration ';' { $$ = std::move($1); }
    | assignment_command ';'   { $$ = std::move($1); }
    | print_command ';'        { $$ = std::move($1); }
    | scan_command ';'         { $$ = std::move($1); }
    | function_call ';'        { $$ = std::move($1); }
    | RETURN expression ';'    { $$ = make_node<ReturnNode>(std::move($2)); }
    | if_command               { $$ = std::move($1); }
    | loop_command             { $$ = std::move($1); }
    ;

variable_declaration:
      type ID {
          $$ = make_node<VarDeclNode>(std::move($1), $2);
      }
    | type ID '=' expression {
          $$ = make_node<VarDeclNode>(std::move($1), $2, std::move($4));
      }
    ;

assignment_command:
    ID '=' expression {
        $$ = make_node<AssignmentNode>($1, std::move($3));
    }
    ;

type:
      INT    { $$ = TypeNode(BasicType::INT); }
    | DOUBLE { $$ = TypeNode(BasicType::DOUBLE); }
    | STRING { $$ = TypeNode(BasicType::STRING); }
    | BOOL   { $$ = TypeNode(BasicType::BOOL); }
    | CHAR   { $$ = TypeNode(BasicType::CHAR); }
    | VOID   { $$ = TypeNode(BasicType::VOID); }
    ;

print_command:
    PRINT '(' expression ')' {
        $$ = make_node<PrintNode>(std::move($3));
    }
    ;

scan_command:
    SCAN '(' ID ')' {
        auto varNode = make_node<VariableNode>($3);
        $$ = make_node<ReadNode>(std::move(varNode));
    }
    ;

if_command:
      IF expression THEN '{' instructions '}' {
          auto block = make_node<BlockNode>(std::move($5));
          $$ = make_node<IfNode>(std::move($2), std::move(block), nullptr);
      }
    | IF expression THEN '{' instructions '}' ELSE '{' instructions '}' {
          auto thenBlock = make_node<BlockNode>(std::move($5));
          auto elseBlock = make_node<BlockNode>(std::move($9));
          $$ = make_node<IfNode>(std::move($2), std::move(thenBlock), std::move(elseBlock));
      }
    ;

loop_command:
    LOOP expression THEN '{' instructions '}' {
        auto block = make_node<BlockNode>(std::move($5));
        $$ = make_node<LoopNode>(std::move($2), std::move(block));
    }
    ;

expression:
      ID { $$ = make_node<VariableNode>($1); }
    | CONST_INTEGER { $$ = make_node<NumberNode>($1); }
    | CONST_DOUBLE  { $$ = make_node<RealNode>($1); }
    | CONST_STRING  { $$ = make_node<StringNode>($1); }
    | CONST_CHAR    { $$ = make_node<CharNode>($1); }

    | expression '+' expression { $$ = make_node<BinaryOpNode>("+", std::move($1), std::move($3)); }
    | expression '-' expression { $$ = make_node<BinaryOpNode>("-", std::move($1), std::move($3)); }
    | expression '*' expression { $$ = make_node<BinaryOpNode>("*", std::move($1), std::move($3)); }
    | expression '/' expression { $$ = make_node<BinaryOpNode>("/", std::move($1), std::move($3)); }
    | expression '&' expression { $$ = make_node<BinaryOpNode>("&", std::move($1), std::move($3)); }

    | expression EQ expression  { $$ = make_node<BinaryOpNode>("==", std::move($1), std::move($3)); }
    | expression NEQ expression { $$ = make_node<BinaryOpNode>("<>", std::move($1), std::move($3)); }
    | expression '<' expression { $$ = make_node<BinaryOpNode>("<",  std::move($1), std::move($3)); }
    | expression '>' expression { $$ = make_node<BinaryOpNode>(">",  std::move($1), std::move($3)); }
    | expression LE expression  { $$ = make_node<BinaryOpNode>("<=", std::move($1), std::move($3)); }
    | expression GE expression  { $$ = make_node<BinaryOpNode>(">=", std::move($1), std::move($3)); }

    | '(' expression ')'        { $$ = std::move($2); }
    | function_call             { $$ = std::move($1); }
    | TRUE                      { $$ = make_node<BooleanNode>(true); }
    | FALSE                     { $$ = make_node<BooleanNode>(false); }
    | expression AND expression { $$ = make_node<BinaryOpNode>("AND", std::move($1), std::move($3)); }
    | expression OR expression  { $$ = make_node<BinaryOpNode>("OR",  std::move($1), std::move($3)); }
    | '!' expression            { $$ = make_node<UnaryOpNode>("!", std::move($2)); }
    ;

%%

// La firma di error ora prende location_type
void yy::yyParser::error(const location_type& loc, const std::string& msg) {
    cerr << ">>> SYNTAX ERROR " << loc << ": " << msg << endl;
}