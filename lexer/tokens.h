#ifndef TOKENS_H
#define TOKENS_H

// Usiamo numeri positivi (0 è riservato per EOF - fine file)
enum TokenType {
    TOKEN_EOF = 0,
    //Token numerici/operzioni
    TOKEN_NUMBER=1,
    TOKEN_ID=2,
    TOKEN_TRUE=25,
    TOKEN_FALSE=26,
    TOKEN_PLUS=3,
    TOKEN_MINUS=4,
    TOKEN_MUL=5,
    TOKEN_DIV=6,
    TOKEN_LPAREN=7,
    TOKEN_RPAREN=8,
    TOKEN_ASSIGN=15,
    TOKEN_STRING_CONCAT=21,
    TOKEN_POW=22,
    TOKEN_AND=23,
    TOKEN_OR=24,

    //Token parole chiave
    TOKEN_IF=9,
    TOKEN_THEN=10,
    TOKEN_ELSE=11,
    TOKEN_LOOP=13,
    TOKEN_END=14,
    TOKEN_MAIN=20,


    //operatori relazionali
    TOKEN_EQ=16,
    TOKEN_NEQ=17,
    TOKEN_MAX=18,
    TOKEN_MIN=19,

    //separatori
    TOKEN_COMMA=27,
    TO



};

#endif