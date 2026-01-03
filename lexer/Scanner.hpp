#ifndef SCANNER_HPP
#define SCANNER_HPP

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "parser.hpp" // Generato da Bison, contiene yyParser::symbol_type

namespace yy {
    class Scanner : public yyFlexLexer {
    public:
        // Costruttore che accetta lo stream di input (file o cin)
        Scanner(std::istream *in) : yyFlexLexer(in) {}

        // Nascondiamo il metodo yylex() originale che ritorna int
        // e ne definiamo uno nuovo che ritorna symbol_type
        using FlexLexer::yylex;
        virtual yy::yyParser::symbol_type lex();
    };
}

#endif // SCANNER_HPP