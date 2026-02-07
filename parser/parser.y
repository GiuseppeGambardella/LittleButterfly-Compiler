%skeleton "lalr1.cc"
%require "3.2"
%defines
%locations

%define parse.error verbose

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

    // including TypeNode definition
    #include "ast/nodes/expression/TypeNode.hpp"

    // Forward declaration of our Scanner custom
    namespace yy { class Scanner; }
    class ASTNode;
}


// Using our custom scanner in the parser
%param { yy::Scanner& scanner }
%parse-param { std::unique_ptr<ASTNode>& astRoot }
%parse-param { const std::vector<std::string>& sourceLines }

%code {
    #include "Scanner.hpp"
    #include "ast/nodes_impl.hpp"
    #include "ast/ast_factory.hpp"

    #include <iomanip>

    // bridge to call our scanner used by Bison
    static yy::yyParser::symbol_type yylex(yy::Scanner& scanner) {
        return scanner.lex();
    }

    using namespace std;
}

%token EOF 0
%token SCONOSCIUTO

/* --- TYPE TOKEN --- */
%token <int> CONST_INTEGER
%token <double> CONST_DOUBLE
%token <char> CONST_CHAR
%token <std::string> CONST_STRING ID

/* --- KEYWORDS AND OPERATORS --- */
%token INT DOUBLE CHAR STRING VOID BOOL
%token TRUE FALSE
%token IF THEN ELSE LOOP MAIN
%token FUNC PRINT SCAN RETURN
%token EQ NEQ LE GE AND OR

/* Priority */
%left OR
%left AND
%left EQ NEQ '<' '>' LE GE
%left '&'
%left '+' '-'
%left '*' '/'
%right '!'

/* --- NON TERMINAL TYPES --- */
%type <std::vector<std::unique_ptr<ASTNode>>> global_declarations instructions params arguments

// Pointers
%type <std::unique_ptr<ASTNode>> program function_declaration instruction
%type <std::unique_ptr<ASTNode>> variable_declaration assignment_command print_command scan_command
%type <std::unique_ptr<ASTNode>> function_call if_command loop_command expression

%type <TypeNode> type

%%

program:
    global_declarations function_declaration {
        $$ = make_node<ProgramNode>(std::move($1), std::move($2));
        $$->line = @$.begin.line;
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

/* --- FUNCTIONS --- */
function_declaration:
    FUNC ID '(' params ')' ':' type '{' instructions '}' {
        auto body = make_node<BlockNode>(std::move($9));
        auto retType = std::make_unique<TypeNode>(std::move($7));

        $$ = make_node<FunctionDeclNode>($2, std::move($4), std::move(retType), std::move(body));
        $$->line = @$.begin.line;
    }
    | MAIN '(' ')' '{' instructions '}' {
        auto body = make_node<BlockNode>(std::move($5));
        auto voidType = std::make_unique<VoidNode>();
        std::vector<std::unique_ptr<ASTNode>> emptyParams;

        $$ = make_node<FunctionDeclNode>("fly", std::move(emptyParams), std::move(voidType), std::move(body));
        $$->line = @$.begin.line;
    }
    ;

params:
      /* empty */ {
          $$ = std::vector<std::unique_ptr<ASTNode>>();
      }
    | type ID {
          std::vector<std::unique_ptr<ASTNode>> p;
          p.push_back(make_node<VarDeclNode>(std::move($1), $2));
          p.back()->line = @$.begin.line;
          $$ = std::move(p);
      }
    | params ',' type ID {
          $1.push_back(make_node<VarDeclNode>(std::move($3), $4));
          $1.back()->line = @$.begin.line;
          $$ = std::move($1);
      }
    ;

function_call:
    ID '(' arguments ')' {
        $$ = make_node<FunctionCallNode>($1, std::move($3));
        $$->line = @$.begin.line;
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

/* --- INSTRUCTIONS --- */
instruction:
      variable_declaration ';' { $$ = std::move($1); }
    | assignment_command ';'   { $$ = std::move($1); }
    | print_command ';'        { $$ = std::move($1); }
    | scan_command ';'         { $$ = std::move($1); }
    | function_call ';'        { $$ = std::move($1); }
    | RETURN expression ';'    {
        $$ = make_node<ReturnNode>(std::move($2));
        $$->line = @$.begin.line;
    }
    | RETURN ';'               {
        $$ = make_node<ReturnNode>(make_node<VoidNode>());
        $$->line = @$.begin.line;
    }
    | if_command               { $$ = std::move($1); }
    | loop_command             { $$ = std::move($1); }
    ;

variable_declaration:
      type ID {
          $$ = make_node<VarDeclNode>(std::move($1), $2);
          $$->line = @$.begin.line;
      }
    | type ID '=' expression {
          $$ = make_node<VarDeclNode>(std::move($1), $2, std::move($4));
          $$->line = @$.begin.line;
      }
    ;

assignment_command:
    ID '=' expression {
        $$ = make_node<AssignmentNode>($1, std::move($3));
        $$->line = @$.begin.line;
    }
    ;

    type:
      INT    { $$ = TypeNode(BasicType::INT); $$.line = @$.begin.line; }
    | DOUBLE { $$ = TypeNode(BasicType::DOUBLE); $$.line = @$.begin.line; }
    | STRING { $$ = TypeNode(BasicType::STRING); $$.line = @$.begin.line; }
    | BOOL   { $$ = TypeNode(BasicType::BOOL); $$.line = @$.begin.line; }
    | CHAR   { $$ = TypeNode(BasicType::CHAR); $$.line = @$.begin.line; }
    | VOID   { $$ = TypeNode(BasicType::VOID); $$.line = @$.begin.line; }
    ;

print_command:
    PRINT '(' expression ')' {
        $$ = make_node<PrintNode>(std::move($3));
        $$->line = @$.begin.line;
    }
    ;

scan_command:
    SCAN '(' ID ')' {
        auto varNode = make_node<VariableNode>($3);
        $$ = make_node<ReadNode>(std::move(varNode));
        $$->line = @$.begin.line;
    }
    ;

    if_command:
      IF expression THEN '{' instructions '}' {
          auto block = make_node<BlockNode>(std::move($5));
          $$ = make_node<IfNode>(std::move($2), std::move(block), nullptr);
          $$->line = @$.begin.line;
      }
    | IF expression THEN '{' instructions '}' ELSE '{' instructions '}' {
          auto thenBlock = make_node<BlockNode>(std::move($5));
          auto elseBlock = make_node<BlockNode>(std::move($9));
          $$ = make_node<IfNode>(std::move($2), std::move(thenBlock), std::move(elseBlock));
          $$->line = @$.begin.line;
      }
    ;

loop_command:
    LOOP expression THEN '{' instructions '}' {
        auto block = make_node<BlockNode>(std::move($5));
        $$ = make_node<LoopNode>(std::move($2), std::move(block));
        $$->line = @$.begin.line;
    }
    ;

expression:
      ID {
        $$ = make_node<VariableNode>($1);
        $$->line = @$.begin.line;
    }
    | CONST_INTEGER {
        $$ = make_node<NumberNode>($1);
        $$->line = @$.begin.line;
    }
    | CONST_DOUBLE  {
        $$ = make_node<RealNode>($1);
        $$->line = @$.begin.line;
    }
    | CONST_STRING  {
        $$ = make_node<StringNode>($1);
        $$->line = @$.begin.line;
    }
    | CONST_CHAR    {
        $$ = make_node<CharNode>($1);
        $$->line = @$.begin.line;
    }

    | expression '+' expression { $$ = make_node<BinaryOpNode>("+", std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression '-' expression { $$ = make_node<BinaryOpNode>("-", std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression '*' expression { $$ = make_node<BinaryOpNode>("*", std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression '/' expression { $$ = make_node<BinaryOpNode>("/", std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression '&' expression { $$ = make_node<BinaryOpNode>("&", std::move($1), std::move($3)); $$->line = @$.begin.line; }

    | expression EQ expression  { $$ = make_node<BinaryOpNode>("==", std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression NEQ expression { $$ = make_node<BinaryOpNode>("<>", std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression '<' expression { $$ = make_node<BinaryOpNode>("<",  std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression '>' expression { $$ = make_node<BinaryOpNode>(">",  std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression LE expression  { $$ = make_node<BinaryOpNode>("<=", std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression GE expression  { $$ = make_node<BinaryOpNode>(">=", std::move($1), std::move($3)); $$->line = @$.begin.line; }

    | '(' expression ')'        { $$ = std::move($2); }
    | function_call             { $$ = std::move($1); }
    | TRUE                      { $$ = make_node<BooleanNode>(true); $$->line = @$.begin.line; }
    | FALSE                     { $$ = make_node<BooleanNode>(false); $$->line = @$.begin.line; }
    | expression AND expression { $$ = make_node<BinaryOpNode>("AND", std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | expression OR expression  { $$ = make_node<BinaryOpNode>("OR",  std::move($1), std::move($3)); $$->line = @$.begin.line; }
    | '!' expression            { $$ = make_node<UnaryOpNode>("!", std::move($2)); $$->line = @$.begin.line; }
    | '-' expression            { $$ = make_node<UnaryOpNode>("-", std::move($2)); $$->line = @$.begin.line; }
    | '?' expression            { $$ = make_node<UnaryOpNode>("?", std::move($2)); $$->line = @$.begin.line; }
    ;

%%

/* --- IMPROVED ERROR --- */
void yy::yyParser::error(const location_type& loc, const std::string& msg) {
    cerr << "\n>> SYNTAX ERROR at line " << loc.begin.line
         << ", column " << loc.begin.column << ": " << msg << endl;

    // Retrieve the source line
    // loc.begin.line starts by 1, vector starts by 0
    int lineIndex = loc.begin.line - 1;

    if (lineIndex >= 0 && lineIndex < (int)sourceLines.size()) {
        const std::string& line = sourceLines[lineIndex];

        cerr << "    " << std::setw(4) << loc.begin.line << " | " << line << endl;

        // print location marker under the line
        cerr << "         | ";

        // Note: loc.begin.column starts by 1
        for(int i = 1; i < loc.begin.column; ++i) {
            cerr << " ";
        }
        cerr << "\033[1;33m^\033[0m" << endl; // ^ giallo

        // Suggerimento (opzionale)
        cerr << "         \033[1;33mHere\033[0m\n" << endl;
    } else {
        cerr << "(Source line not available)" << endl;
    }
}